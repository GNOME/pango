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
#include <dirent.h>
#include <limits.h>

#include <gmodule.h>

#include "pango-modules.h"
#include "modules.h"
#include "utils.h"

typedef struct _PangoMapInfo PangoMapInfo;
typedef struct _PangoEnginePair PangoEnginePair;
typedef struct _PangoSubmap PangoSubmap;

struct _PangoSubmap
{
  gboolean is_leaf;
  union {
    PangoMapEntry entry;
    PangoMapEntry *leaves;
  } d;
};

struct _PangoMap
{
  gint n_submaps;
  PangoSubmap submaps[256];
};

struct _PangoMapInfo
{
  const gchar *lang;
  guint engine_type_id;
  guint render_type_id;
  PangoMap *map;
};

struct _PangoEnginePair
{
  PangoEngineInfo info;
  gboolean included;
  void *load_info;
  PangoEngine *engine;
};

GList *maps;
GList *engines;

static void      build_map      (PangoMapInfo       *info);
static void      init_modules   (void);

PangoMap *
pango_find_map (const char *lang,
		guint       engine_type_id,
		guint       render_type_id)
{
  GList *tmp_list = maps;
  PangoMapInfo *map_info = NULL;
  gboolean found_earlier = FALSE;

  while (tmp_list)
    {
      map_info = tmp_list->data;
      if (map_info->engine_type_id == engine_type_id &&
	  map_info->render_type_id == render_type_id)
	{
	  if (strcmp (map_info->lang, lang) == 0)
	    break;
	  else
	    found_earlier = TRUE;
	}

      tmp_list = tmp_list->next;
    }

  if (!tmp_list)
    {
      map_info = g_new (PangoMapInfo, 1);
      map_info->lang = g_strdup (lang);
      map_info->engine_type_id = engine_type_id;
      map_info->render_type_id = render_type_id;

      build_map (map_info);

      maps = g_list_prepend (maps, map_info);
    }
  else if (found_earlier)
    {
      /* Move the found map to the beginning of the list
       * for speed next time around if we had to do
       * any failing strcmps.
       */
      if (tmp_list->next)
	tmp_list->next->prev = tmp_list->prev;
      tmp_list->prev->next = tmp_list->next;
      tmp_list->next = maps;
      tmp_list->prev = NULL;
      maps = tmp_list;
    }
  
  return map_info->map;
}

static PangoEngine *
pango_engine_pair_get_engine (PangoEnginePair *pair)
{
  if (!pair->engine)
    {
      if (pair->included)
	{
	  PangoIncludedModule *included_module = pair->load_info;
	  
	  pair->engine = included_module->load (pair->info.id);
	}
      else
	{
	  GModule *module;
	  char *module_name = pair->load_info;
	  PangoEngine *(*load) (const gchar *id);
  	  
	  module = g_module_open (module_name, 0);
	  if (!module)
	    {
	      fprintf(stderr, "Cannot load module %s: %s\n",
		      module_name, g_module_error());
	      return NULL;
	    }
	  
	  g_module_symbol (module, "script_engine_load", (gpointer)&load);
	  if (!load)
	    {
	      fprintf(stderr, "cannot retrieve script_engine_load from %s: %s\n",
		      module_name, g_module_error());
	      g_module_close (module);
	      return NULL;
	    }
	  
	  pair->engine = (*load) (pair->info.id);
	}
      
    }

  return pair->engine;
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
add_included_modules (void)
{
  int i, j;

  for (i = 0; _pango_included_modules[i].list; i++)
    {
      PangoEngineInfo *engine_info;
      int n_engines;

      _pango_included_modules[i].list (&engine_info, &n_engines);

      for (j=0; j < n_engines; j++)
	{
	  PangoEnginePair *pair = g_new (PangoEnginePair, 1);

	  pair->info = engine_info[j];
	  pair->included = TRUE;
	  pair->load_info = &_pango_included_modules[i];
	  pair->engine = NULL;

	  engines = g_list_prepend (engines, pair);
	}
    }
}

static gboolean /* Returns true if succeeded, false if failed */
process_module_file(FILE *module_file)
{
  char *line;

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

      pair->included = FALSE;
      
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
		  pair->load_info = g_strndup (q, p-q);
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
		      return FALSE;
		    }
		  q = strchr (q, ':');
		  if (!q)
		    {
		      fprintf(stderr, "Error reading pango.modules");
		      return FALSE;
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
	  return FALSE;
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

  return TRUE;
}

static void
read_modules (void)
{
  FILE *module_file;
  gboolean read_module_file = FALSE;

  /* FIXME FIXME FIXME - this is a potential security problem from leaving
   * pango.modules files scattered around to trojan modules.
   */
  module_file = fopen ("pango.modules", "r");
  if(module_file)
    {
      read_module_file = read_module_file || process_module_file(module_file);
      process_module_file(module_file);
      fclose(module_file);
    }
  else
    {
      DIR *dirh;

      dirh = opendir(DOTMODULEDIR);
      if(dirh)
	{
	  struct dirent *dent;

	  while((dent = readdir(dirh)))
	    {
	      char fullfn[PATH_MAX];
	      char *ctmp;

	      if(dent->d_name[0] == '.')
		continue;

	      ctmp = strrchr(dent->d_name, '.');
	      if(!ctmp || strcmp(ctmp, ".modules") != 0)
		continue;

	      g_snprintf(fullfn, sizeof(fullfn), DOTMODULEDIR "/%s", dent->d_name);
	      module_file = fopen(fullfn, "r");
	      if(module_file)
		{
		  read_module_file = read_module_file || process_module_file(module_file);
		  fclose(module_file);
		}
	    }
	}

      closedir(dirh);
    }

  if (!read_module_file)
    {
      fprintf(stderr, "Could not load any module files!\n");
      /* FIXME: Error */
    }
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

static void
init_modules (void)
{
  static gboolean init = FALSE;

  if (init)
    return;
  else
    init = TRUE;

  engines = NULL;

  add_included_modules();
  read_modules();

  engines = g_list_reverse (engines);
}

static void
build_map (PangoMapInfo *info)
{
  GList *tmp_list;
  int i, j;
  PangoMap *map;

  char *engine_type = g_quark_to_string (info->engine_type_id);
  char *render_type = g_quark_to_string (info->render_type_id);
  
  init_modules();

  info->map = map = g_new (PangoMap, 1);
  map->n_submaps = 0;
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

      if (strcmp (pair->info.engine_type, engine_type) == 0 &&
	  strcmp (pair->info.render_type, render_type) == 0)
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
}

PangoMapEntry *
pango_map_get_entry (PangoMap   *map,
		     guint32     wc)
{
  PangoSubmap *submap = &map->submaps[wc / 256];
  return submap->is_leaf ? &submap->d.entry : &submap->d.leaves[wc % 256];
}

PangoEngine *
pango_map_get_engine (PangoMap *map,
		      guint32   wc)
{
  PangoSubmap *submap = &map->submaps[wc / 256];
  PangoMapEntry *entry = submap->is_leaf ? &submap->d.entry : &submap->d.leaves[wc % 256];

  if (entry->info)
    return pango_engine_pair_get_engine ((PangoEnginePair *)entry->info);
  else
    return NULL;
}
