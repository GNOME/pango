/* Pango
 * modules.c:
 *
 * Copyright (C) 1999 Red Hat Software
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Library General Public
 * License as published by the Free Software Foundation; either
 * version 2 of the License, or (at your option) any later version.
 *
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.	 See the GNU
 * Library General Public License for more details.
 *
 * You should have received a copy of the GNU Library General Public
 * License along with this library; if not, write to the
 * Free Software Foundation, Inc., 59 Temple Place - Suite 330,
 * Boston, MA 02111-1307, USA.
 */

#include <ctype.h>
#include <stdio.h>
#include <string.h>

#include <gmodule.h>
#include <pango.h>

#include "modules.h"
#include "utils.h"

typedef struct _PangoMapInfo PangoMapInfo;
typedef struct _PangoEnginePair PangoEnginePair;

struct _PangoMapInfo
{
  const gchar *lang;
  const gchar *engine_type;
  const gchar *render_type;
};

struct _PangoEnginePair
{
  PangoEngineInfo info;
  gchar *module;
  PangoEngine *engine;
};

GList *engines;

static PangoMap *build_map      (PangoMapInfo       *info);
static void      read_modules   (void);
static guint     map_info_hash  (const PangoMapInfo *map);
static gboolean  map_info_equal (const PangoMapInfo *map_a, 
				 const PangoMapInfo *map_b);

PangoMap *
_pango_find_map (const char *lang,
		 const char *engine_type,
		 const char *render_type)
{
  PangoMapInfo map_info;
  PangoMap *map;
  
  static GHashTable *map_hash = NULL;

  if (!map_hash)
    map_hash = g_hash_table_new ((GHashFunc)map_info_hash,
				 (GCompareFunc)map_info_equal);

  map_info.lang = lang ? lang : "NONE";
  map_info.engine_type = engine_type;
  map_info.render_type = render_type;

  map = g_hash_table_lookup (map_hash, &map_info);
  if (!map)
    {
      PangoMapInfo *new_info = g_new (PangoMapInfo, 1);
      new_info->lang = g_strdup (map_info.lang);
      new_info->engine_type = g_strdup (engine_type);
      new_info->render_type = g_strdup (render_type);

      map = build_map (new_info);
      g_hash_table_insert (map_hash, new_info, map);
    }

  return map;
}

PangoEngine *
_pango_load_engine (const char *id)
{
  GList *tmp_list;
  
  read_modules();

  tmp_list = engines;
  while (tmp_list)
    {
      PangoEnginePair *pair = tmp_list->data;
      tmp_list = tmp_list->next;

      if (!strcmp (pair->info.id, id))
	{
	  GModule *module;
	  PangoEngine *(*load) (const gchar *id);

	  if (!pair->engine)
	    {
	      module = g_module_open (pair->module, 0);
	      if (!module)
		{
		  fprintf(stderr, "Cannot load module %s: %s\n",
			  pair->module, g_module_error());
		  return NULL;
		}
	      
	      g_module_symbol (module, "script_engine_load", (gpointer)&load);
	      if (!load)
		{
		  fprintf(stderr, "cannot retrieve script_engine_load from %s: %s\n",
			  pair->module, g_module_error());
		  g_module_close (module);
		  return NULL;
		}

	      pair->engine = (*load) (id);
	    }

	  return pair->engine;
	}
    }

  return NULL;
}


static guint 
map_info_hash (const PangoMapInfo *map)
{
  return g_str_hash (map->lang) |
         g_str_hash (map->engine_type) |
         g_str_hash (map->render_type);
}

static gboolean
map_info_equal (const PangoMapInfo *map_a, const PangoMapInfo *map_b)
{
  return (strcmp (map_a->lang, map_b->lang) == 0 &&
	  strcmp (map_a->engine_type, map_b->engine_type) == 0 &&
	  strcmp (map_a->render_type, map_b->render_type) == 0);
}

static char *
readline(FILE *file)
{
  static GString *bufstring = NULL;
  int c;

  if (!bufstring)
    bufstring = g_string_new (NULL);
  else
    g_string_truncate (bufstring, 0);

  while ((c = getc(file)) != EOF)
    {
      g_string_append_c (bufstring, c);
      if (c == '\n')
	break;
    }

  if (bufstring->len == 0)
    return NULL;
  else
    return g_strdup (bufstring->str);
}

static void
read_modules (void)
{
  FILE *module_file;
  static gboolean init = FALSE;
  char *line;

  if (init)
    return;
  else
    init = TRUE;

  /* FIXME FIXME FIXME - this is a potential security problem from leaving
   * pango.modules files scattered around to trojan modules.
   */
  module_file = fopen ("pango.modules", "r");
  if (!module_file)
    {
      module_file = fopen (LOCALSTATEDIR "/lib/pango/pango.modules", "r");
      if (!module_file)
	{
	  fprintf(stderr, "Cannot load module file!\n");
	  return;			/* FIXME: Error */
	}
    }

  engines = NULL;
  while ((line = readline (module_file)))
    {
      PangoEnginePair *pair = g_new (PangoEnginePair, 1);
      PangoEngineRange *range;
      GList *ranges;
      GList *tmp_list;
      char *p, *q;
      int i;
      int start;
      int end;

      p = line;
      q = line;
      ranges = NULL;

      /* Break line into words on whitespace */
      i = 0;
      while (1)
	{
	  if (!*p || isspace(*p))
	    {
	      switch (i)
		{
		case 0:
		  pair->module = g_strndup (q, p-q);
		  break;
		case 1:
		  pair->info.id = g_strndup (q, p-q);
		  break;
		case 2:
		  pair->info.engine_type = g_strndup (q, p-q);
		  break;
		case 3:
		  pair->info.render_type = g_strndup (q, p-q);
		  break;
		default:
		  range = g_new (PangoEngineRange, 1);
		  if (sscanf(q, "%d-%d:", &start, &end) != 2)
		    {
		      fprintf(stderr, "Error reading pango.modules");
		      return;
		    }
		  q = strchr (q, ':');
		  if (!q)
		    {
		      fprintf(stderr, "Error reading pango.modules");
		      return;
		    }
		  q++;
		  range->start = start;
		  range->end = end;
		  range->langs = g_strndup (q, p-q);

		  ranges = g_list_prepend (ranges, range);
		}
	      
	      i++;

	      do
		p++;
	      while (*p && isspace(*p));

	      if (!*p)
		break;

	      q = p;
	    }
	  else
	    p++;
	}

      if (i<3)
	{
	  fprintf(stderr, "Error reading pango.modules");
	  return;
	}
      
      ranges = g_list_reverse (ranges);
      pair->info.n_ranges = g_list_length (ranges);
      pair->info.ranges = g_new (PangoEngineRange, pair->info.n_ranges);
      
      tmp_list = ranges;
      for (i=0; i<pair->info.n_ranges; i++)
	{
	  pair->info.ranges[i] = *(PangoEngineRange *)tmp_list->data;
	  tmp_list = tmp_list->next;
	}

      g_list_foreach (ranges, (GFunc)g_free, NULL);
      g_list_free (ranges);
      g_free (line);

      pair->engine = NULL;

      engines = g_list_prepend (engines, pair);
    }
  engines = g_list_reverse (engines);
}

static void
set_entry (PangoMapEntry *entry, gboolean is_exact, PangoEngineInfo *info)
{
  if ((is_exact && !entry->is_exact) ||
      !entry->info)
    {
      entry->is_exact = is_exact;
      entry->info = info;
    }
}

static PangoMap *
build_map (PangoMapInfo *info)
{
  GList *tmp_list;
  int i, j;
  PangoMap *map;
  
  read_modules();

  map = g_new (PangoMap, 1);
  for (i=0; i<256; i++)
    {
      map->submaps[i].is_leaf = TRUE;
      map->submaps[i].d.entry.info = NULL;
      map->submaps[i].d.entry.is_exact = FALSE;
    }
  
  tmp_list = engines;
  while (tmp_list)
    {
      PangoEnginePair *pair = tmp_list->data;
      tmp_list = tmp_list->next;

      if (strcmp (pair->info.engine_type, info->engine_type) == 0 &&
	  strcmp (pair->info.render_type, info->render_type) == 0)
	{
	  int submap;

	  for (i=0; i<pair->info.n_ranges; i++)
	    {
	      gchar **langs;
	      gboolean is_exact = FALSE;

	      if (pair->info.ranges[i].langs)
		{
		  langs = g_strsplit (pair->info.ranges[i].langs, ";", -1);
		  for (j=0; langs[j]; j++)
		    if (strcmp (langs[j], "*") == 0 ||
			strcmp (langs[j], info->lang) == 0)
		      {
			is_exact = TRUE;
			break;
		      }
		  g_strfreev (langs);
		}

	      for (submap = pair->info.ranges[i].start / 256;
		   submap <= pair->info.ranges[i].end / 256;
		   submap ++)
		{
		  GUChar4 start;
		  GUChar4 end;
		  
		  if (submap == pair->info.ranges[i].start / 256)
		    start = pair->info.ranges[i].start % 256;
		  else
		    start = 0;
		  
		  if (submap == pair->info.ranges[i].end / 256)
		    end = pair->info.ranges[i].end % 256;
		  else
		    end = 255;
		  
		  if (map->submaps[submap].is_leaf &&
		      start == 0 && end == 255)
		    {
		      set_entry (&map->submaps[submap].d.entry,
				 is_exact, &pair->info);
		    }
		  else
		    {
		      if (map->submaps[submap].is_leaf)
			{
			  map->submaps[submap].is_leaf = FALSE;
			  map->submaps[submap].d.leaves = g_new (PangoMapEntry, 256);
			  for (j=0; j<256; j++)
			    {
			      map->submaps[submap].d.leaves[j].info = NULL;
			      map->submaps[submap].d.leaves[j].is_exact = FALSE;
			    }
			}
		      
		      for (j=start; j<=end; j++)
			set_entry (&map->submaps[submap].d.leaves[j],
				   is_exact, &pair->info);
		      
		    }
		}
	    }
	}
    }

  return map;
}
