/* Pango
 * pangoft2-fontmap.c:
 *
 * Copyright (C) 2000 Red Hat Software
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

#include "config.h"

#include <glib.h>
#include <ctype.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <errno.h>
#ifdef HAVE_DIRENT_H
#include <dirent.h>
#endif

#include "pango-fontmap.h"
#include "pango-utils.h"
#include "pangoft2-private.h"

#ifdef G_OS_WIN32
#define STRICT
#include <windows.h>
#endif

#define PANGO_TYPE_FT2_FONT_MAP              (pango_ft2_font_map_get_type ())
#define PANGO_FT2_FONT_MAP(object)           (G_TYPE_CHECK_INSTANCE_CAST ((object), PANGO_TYPE_FT2_FONT_MAP, PangoFT2FontMap))
#define PANGO_FT2_FONT_MAP_CLASS(klass)      (G_TYPE_CHECK_CLASS_CAST ((klass), PANGO_TYPE_FT2_FONT_MAP, PangoFT2FontMapClass))
#define PANGO_FT2_IS_FONT_MAP(object)        (G_TYPE_CHECK_INSTANCE_TYPE ((object), PANGO_TYPE_FT2_FONT_MAP))
#define PANGO_FT2_IS_FONT_MAP_CLASS(klass)   (G_TYPE_CHECK_CLASS_TYPE ((klass), PANGO_TYPE_FT2_FONT_MAP))
#define PANGO_FT2_FONT_MAP_GET_CLASS(obj)    (G_TYPE_INSTANCE_GET_CLASS ((obj), PANGO_TYPE_FT2_FONT_MAP, PangoFontMapClass))

typedef struct _PangoFT2FamilyEntry  PangoFT2FamilyEntry;
typedef struct _PangoFT2FontMap      PangoFT2FontMap;
typedef struct _PangoFT2FontMapClass PangoFT2FontMapClass;
typedef struct _PangoFT2SizeInfo     PangoFT2SizeInfo;

/* Number of freed fonts */
#define MAX_FREED_FONTS 16

struct _PangoFT2FontMap
{
  PangoFontMap parent_instance;

  FT_Library library;

  PangoFT2FontCache *font_cache;
  GQueue *freed_fonts;

  /* Maps Pango family names to PangoFT2FamilyEntry structs */
  GHashTable *families;

  /* Maps the family and style of a face to a PangoFT2OA struct */
  GHashTable *faces;

  int n_fonts;

  double resolution;		/* (points / pixel) * PANGO_SCALE */
};

struct _PangoFT2FontMapClass
{
  PangoFontMapClass parent_class;
};

struct _PangoFT2FamilyEntry
{
  char *family_name;

  /* List of PangoFT2FontEntry structs */
  GSList *font_entries;
};

struct _PangoFT2FontEntry
{
  FT_Open_Args **open_args;
  FT_Long *face_indices;
  int n_fonts;
  PangoFontDescription description;
  PangoCoverage *coverage;

  GSList *cached_fonts;
};

static GType      pango_ft2_font_map_get_type      (void);

static void       pango_ft2_font_map_init          (PangoFT2FontMap             *fontmap);

static void       pango_ft2_font_map_class_init    (PangoFT2FontMapClass        *class);

static void       pango_ft2_font_map_finalize      (GObject                      *object);

static PangoFont *pango_ft2_font_map_load_font     (PangoFontMap                 *fontmap,
						    const PangoFontDescription   *description);

static void       pango_ft2_font_map_list_fonts    (PangoFontMap                 *fontmap,
						    const gchar                  *family,
						    PangoFontDescription       ***descs,
						    int                          *n_descs);
static void       pango_ft2_font_map_list_families (PangoFontMap                 *fontmap,
						    gchar                      ***families,
						    int                          *n_families);

static void       pango_ft2_fontmap_cache_clear    (PangoFT2FontMap              *ft2fontmap);

static void       pango_ft2_font_map_read_aliases  (PangoFT2FontMap              *ft2fontmap);
    						  
static void       pango_ft2_insert_face            (PangoFT2FontMap   		 *fontmap,
						    FT_Face                       face,
						    const char        	         *path,
						    int                           face_index);

static PangoFontClass  *parent_class;	/* Parent class structure for PangoFT2FontMap */

static PangoFT2FontMap *pango_ft2_global_fontmap = NULL;
static char **pango_ft2_font_directories = NULL;

static GType
pango_ft2_font_map_get_type (void)
{
  static GType object_type = 0;

  if (!object_type)
    {
      static const GTypeInfo object_info =
      {
        sizeof (PangoFT2FontMapClass),
        (GBaseInitFunc) NULL,
        (GBaseFinalizeFunc) NULL,
        (GClassInitFunc) pango_ft2_font_map_class_init,
        NULL,           /* class_finalize */
        NULL,           /* class_data */
        sizeof (PangoFT2FontMap),
        0,              /* n_preallocs */
        (GInstanceInitFunc) pango_ft2_font_map_init,
      };
      
      object_type = g_type_register_static (PANGO_TYPE_FONT_MAP,
                                            "PangoFT2FontMap",
                                            &object_info, 0);
    }
  
  return object_type;
}

static guint
face_style_hash (gconstpointer v)
{
  PangoFontDescription *desc = (PangoFontDescription *)v;

  return g_str_hash (desc->family_name) +
    desc->style + desc->variant + desc->weight + desc->stretch;
}

static gint
face_style_equal (gconstpointer v1,
		  gconstpointer v2)
{
  PangoFontDescription *desc1 = (PangoFontDescription *)v1;
  PangoFontDescription *desc2 = (PangoFontDescription *)v2;

  return (g_strcasecmp (desc1->family_name, desc2->family_name) == 0 &&
	  desc1->style == desc2->style &&
	  desc1->variant == desc2->variant &&
	  desc1->weight == desc2->weight &&
	  desc1->stretch == desc2->stretch);
}

static void 
pango_ft2_font_map_init (PangoFT2FontMap *ft2fontmap)
{
  ft2fontmap->families = g_hash_table_new (g_str_hash, g_str_equal);
  ft2fontmap->faces = g_hash_table_new (face_style_hash, face_style_equal);
  ft2fontmap->n_fonts = 0;
}

static void
pango_ft2_font_map_class_init (PangoFT2FontMapClass *class)
{
  GObjectClass *object_class = G_OBJECT_CLASS (class);
  PangoFontMapClass *font_map_class = PANGO_FONT_MAP_CLASS (class);
  char *font_path;
  
  parent_class = g_type_class_peek_parent (class);
  
  object_class->finalize = pango_ft2_font_map_finalize;
  font_map_class->load_font = pango_ft2_font_map_load_font;
  font_map_class->list_fonts = pango_ft2_font_map_list_fonts;
  font_map_class->list_families = pango_ft2_font_map_list_families;

  font_path = pango_config_key_get ("PangoFT2/FontPath");

  if (!font_path)
    {
      font_path = g_strconcat
	(pango_get_lib_subdirectory (),
	 G_DIR_SEPARATOR_S "ft2fonts",
	 NULL);

#ifdef G_OS_WIN32
      {
	char win_dir[100];
	char *tmp_str;

	GetWindowsDirectory (win_dir, sizeof (win_dir));
	tmp_str = g_strconcat (font_path,
			       G_SEARCHPATH_SEPARATOR_S,
			       win_dir,
			       G_DIR_SEPARATOR_S "fonts",
			       NULL);
	g_free (font_path);
	font_path = tmp_str;
      }		       
#endif
    }

  pango_ft2_font_directories = pango_split_file_list (font_path);
  g_free (font_path);
}

static gboolean
pango_ft2_is_font_file (const char *name)
{
  int len;

  len = strlen (name);
  if (len > 4 &&
      (g_strncasecmp (&name[len-4], ".pfa", 4) == 0 ||
       g_strncasecmp (&name[len-4], ".pfb", 4) == 0 ||
       g_strncasecmp (&name[len-4], ".ttf", 4) == 0 ||
       g_strncasecmp (&name[len-4], ".ttc", 4) == 0))
    return TRUE;

  return FALSE;
}

static void
pango_ft2_scan_directory (const char      *path,
			  PangoFT2FontMap *ft2fontmap)
{
  DIR *dir;
  struct dirent *entry;
  char *fullname;
  FT_Face face;
  FT_Error error;
  int i;

  dir = opendir (path);
  if (!dir)
    /* Don't warn; it's OK to have nonexistent entries in the font path */
    return;

  while ((entry = readdir (dir)) != NULL)
    {
      fullname = g_strconcat (path,
			      (path[strlen (path)-1] == G_DIR_SEPARATOR ?
			       "" : G_DIR_SEPARATOR_S),
			      entry->d_name,
			      NULL);
      if (pango_ft2_is_font_file (fullname))
	{
	  error = FT_New_Face (ft2fontmap->library, fullname, 0, &face);
	  if (error != FT_Err_Ok)
	    g_warning ("Error loading font from '%s': %s",
		       fullname, pango_ft2_ft_strerror (error));
	  else
	    {
	      if (face->face_flags & FT_FACE_FLAG_SCALABLE)
		pango_ft2_insert_face (ft2fontmap, face, fullname, 0);
	      
	      for (i = 1; i < face->num_faces; i++)
		{
		  error = FT_Done_Face (face);
		  if (error != FT_Err_Ok)
		    g_warning ("Error from FT_Done_Face: %s",
			       pango_ft2_ft_strerror (error));
		  error = FT_New_Face (ft2fontmap->library, fullname, i, &face);
		  if (error != FT_Err_Ok)
		    g_warning ("Error loading font %d from '%s': %s",
			       i, fullname, pango_ft2_ft_strerror (error));
		  else if (face->face_flags & FT_FACE_FLAG_SCALABLE)
		    pango_ft2_insert_face (ft2fontmap, face, fullname, i);
		}
	      error = FT_Done_Face (face);
	      if (error != FT_Err_Ok)
		g_warning ("Error from FT_Done_Face: %s",
			   pango_ft2_ft_strerror (error));
	    }
	}
      g_free (fullname);
    }
  closedir (dir);
}

PangoFontMap *
pango_ft2_font_map_for_display (void)
{
  char **tmp_list;
  FT_Error error;

  /* Make sure that the type system is initialized */
  g_type_init();
  
  if (pango_ft2_global_fontmap != NULL)
    return PANGO_FONT_MAP (pango_ft2_global_fontmap);

  pango_ft2_global_fontmap = (PangoFT2FontMap *)g_type_create_instance (PANGO_TYPE_FT2_FONT_MAP);
  
  error = FT_Init_FreeType (&pango_ft2_global_fontmap->library);
  if (error != FT_Err_Ok)
    {
      g_warning ("Error from FT_Init_FreeType: %s",
		 pango_ft2_ft_strerror (error));
      return NULL;
    }

  pango_ft2_global_fontmap->font_cache = pango_ft2_font_cache_new (pango_ft2_global_fontmap->library);
  pango_ft2_global_fontmap->freed_fonts = g_queue_new ();
  
  tmp_list = pango_ft2_font_directories;

  while (*tmp_list)
    {
      pango_ft2_scan_directory ((const char *) *tmp_list, pango_ft2_global_fontmap);
      tmp_list++;
    }

  pango_ft2_font_map_read_aliases (pango_ft2_global_fontmap);

  return PANGO_FONT_MAP (pango_ft2_global_fontmap);
}

/**
 * pango_ft2_shutdown_display:
 * 
 * Free cached resources.
 **/
void
pango_ft2_shutdown_display (void)
{
  pango_ft2_fontmap_cache_clear (pango_ft2_global_fontmap);

  g_object_unref (G_OBJECT (pango_ft2_global_fontmap));

  pango_ft2_global_fontmap = NULL;
}

static void
pango_ft2_font_map_finalize (GObject *object)
{
  PangoFT2FontMap *ft2fontmap = PANGO_FT2_FONT_MAP (object);
  
  g_list_foreach (ft2fontmap->freed_fonts->head, (GFunc)g_object_unref, NULL);
  g_queue_free (ft2fontmap->freed_fonts);
  
  pango_ft2_font_cache_free (ft2fontmap->font_cache);

  FT_Done_FreeType (ft2fontmap->library);

  G_OBJECT_CLASS (parent_class)->finalize (object);
}

typedef struct
{
  int n_found;
  PangoFontDescription **descs;
} ListFontsInfo;

static void
list_fonts_foreach (gpointer key,
		    gpointer value,
		    gpointer user_data)
{
  PangoFT2FamilyEntry *entry = value;
  ListFontsInfo *info = user_data;

  GSList *tmp_list = entry->font_entries;

  while (tmp_list)
    {
      PangoFT2FontEntry *font_entry = tmp_list->data;
      
      info->descs[info->n_found++] = pango_font_description_copy (&font_entry->description);
      tmp_list = tmp_list->next;
    }
}

static void
pango_ft2_font_map_list_fonts (PangoFontMap           *fontmap,
			       const gchar            *family,
			       PangoFontDescription ***descs,
			       int                    *n_descs)
{
  PangoFT2FontMap *ft2fontmap = (PangoFT2FontMap *)fontmap;
  ListFontsInfo info;

  if (!n_descs)
    return;

  if (family)
    {
      PangoFT2FamilyEntry *entry = g_hash_table_lookup (ft2fontmap->families, family);
      if (entry)
	{
	  *n_descs = g_slist_length (entry->font_entries);
	  if (descs)
	    {
	      *descs = g_new (PangoFontDescription *, *n_descs);
	      
	      info.descs = *descs;
	      info.n_found = 0;

	      list_fonts_foreach ((gpointer)family, (gpointer)entry, &info);
	    }
	}
      else
	{
	  *n_descs = 0;
	  if (descs)
	    *descs = NULL;
	}
    }
  else
    {
      *n_descs = ft2fontmap->n_fonts;
      if (descs)
	{
	  *descs = g_new (PangoFontDescription *, ft2fontmap->n_fonts);
	  
	  info.descs = *descs;
	  info.n_found = 0;
	  
	  g_hash_table_foreach (ft2fontmap->families, list_fonts_foreach, &info);
	}
    }
}

static void
list_families_foreach (gpointer key,
		       gpointer value,
		       gpointer user_data)
{
  GSList **list = user_data;

  *list = g_slist_prepend (*list, key);
}

static void
pango_ft2_font_map_list_families (PangoFontMap *fontmap,
				  gchar      ***families,
				  int          *n_families)
{
  GSList *family_list = NULL;
  GSList *tmp_list;
  PangoFT2FontMap *ft2fontmap = (PangoFT2FontMap *)fontmap;

  if (!n_families)
    return;

  g_hash_table_foreach (ft2fontmap->families, list_families_foreach, &family_list);

  *n_families = g_slist_length (family_list);

  if (families)
    {
      int i = 0;
	
      *families = g_new (gchar *, *n_families);

      tmp_list = family_list;
      while (tmp_list)
	{
	  (*families)[i] = g_strdup (tmp_list->data);
	  i++;
	  tmp_list = tmp_list->next;
	}
    }
  
  g_slist_free (family_list);
}

static PangoFT2FamilyEntry *
pango_ft2_get_family_entry (PangoFT2FontMap *ft2fontmap,
			    const char      *family_name)
{
  PangoFT2FamilyEntry *family_entry = g_hash_table_lookup (ft2fontmap->families, family_name);
  if (!family_entry)
    {
      family_entry = g_new (PangoFT2FamilyEntry, 1);
      family_entry->family_name = g_strdup (family_name);
      family_entry->font_entries = NULL;
      
      g_hash_table_insert (ft2fontmap->families, family_entry->family_name, family_entry);
    }

  return family_entry;
}

static PangoFont *
pango_ft2_font_map_load_font (PangoFontMap               *fontmap,
			      const PangoFontDescription *description)
{
  PangoFT2FontMap *ft2fontmap = (PangoFT2FontMap *)fontmap;
  PangoFT2FamilyEntry *family_entry;
  PangoFont *result = NULL;
  GSList *tmp_list;
  gchar *name;

  g_return_val_if_fail (description != NULL, NULL);
  g_return_val_if_fail (description->size > 0, NULL);
  
  name = g_strdup (description->family_name);
  g_strdown (name);

  family_entry = g_hash_table_lookup (ft2fontmap->families, name);
  if (family_entry)
    {
      PangoFT2FontEntry *best_match = NULL;
      
      tmp_list = family_entry->font_entries;
      while (tmp_list)
	{
	  PangoFT2FontEntry *font_entry = tmp_list->data;
	  
	  if (font_entry->description.style == description->style &&
	      font_entry->description.variant == description->variant &&
	      font_entry->description.stretch == description->stretch)
	    {
	      int distance = abs (font_entry->description.weight - description->weight);
	      int old_distance = best_match ? abs (best_match->description.weight - description->weight) : G_MAXINT;

	      if (distance < old_distance)
		{
		  best_match = font_entry;
		}
	    }

	  tmp_list = tmp_list->next;
	}

      if (best_match)
	{
	  GSList *tmp_list = best_match->cached_fonts;

	  while (tmp_list)
	    {
	      PangoFT2Font *ft2font = tmp_list->data;

	      if (ft2font->size == description->size)
		{
		  result = (PangoFont *)ft2font;
	      
		  g_object_ref (G_OBJECT (result));
		  if (ft2font->in_cache)
		    pango_ft2_fontmap_cache_remove (fontmap, ft2font);
		  break;
		}
	      tmp_list = tmp_list->next;
	    }

	  if (!result)
	    {
	      PangoFT2Font *ft2font =
		(PangoFT2Font *) pango_ft2_load_font (fontmap,
						      best_match->open_args,
						      best_match->face_indices,
						      best_match->n_fonts,
						      description->size);
	      
	      ft2font->fontmap = fontmap;
	      ft2font->entry = best_match;
	      best_match->cached_fonts = g_slist_prepend (best_match->cached_fonts, ft2font);
	      
	      result = (PangoFont *)ft2font;
	    }
	}
    }

  g_free (name);
  return result;
}

static void
pango_ft2_font_map_read_alias_file (PangoFT2FontMap *ft2fontmap,
				    const char      *filename)
{
  FILE *infile;
  int lineno = 0;
  int nfaces;
  int i;
  PangoFT2FontEntry *font_entry = NULL;
  gchar **faces;

  infile = fopen (filename, "r");
  if (infile)
    {
      GString *line_buf = g_string_new (NULL);
      GString *tmp_buf = g_string_new (NULL);

      while (pango_read_line (infile, line_buf))
	{
	  PangoFT2FamilyEntry *family_entry;

	  const char *p = line_buf->str;
	  
	  lineno++;

	  if (!pango_skip_space (&p))
	    continue;

	  if (!pango_scan_string (&p, tmp_buf))
	    goto error;

	  font_entry = g_new (PangoFT2FontEntry, 1);
	  font_entry->n_fonts = 0;
	  font_entry->open_args = NULL;
	  font_entry->face_indices = NULL;

	  font_entry->description.family_name = g_strdup (tmp_buf->str);
	  g_strdown (font_entry->description.family_name);

	  if (!pango_scan_string (&p, tmp_buf))
	    goto error;

	  if (!pango_parse_style (tmp_buf->str, &font_entry->description, TRUE))
	    goto error;

	  if (!pango_scan_string (&p, tmp_buf))
	    goto error;

	  if (!pango_parse_variant (tmp_buf->str, &font_entry->description, TRUE))
	    goto error;

	  if (!pango_scan_string (&p, tmp_buf))
	    goto error;

	  if (!pango_parse_weight (tmp_buf->str, &font_entry->description, TRUE))
	    goto error;
	  
	  if (!pango_scan_string (&p, tmp_buf))
	    goto error;

	  if (!pango_parse_stretch (tmp_buf->str, &font_entry->description, TRUE))
	    goto error;

	  if (!pango_scan_string (&p, tmp_buf))
	    goto error;

	  /* Remove excess whitespace and check for complete fields */

	  faces = g_strsplit (tmp_buf->str, ",", -1);
	  nfaces = 0;
	  for (i = 0; faces[i]; i++)
	    {
	      char *trimmed = pango_trim_string (faces[i]);
	      g_free (faces[i]);
	      faces[i] = trimmed;
	      nfaces++;
	    }

	  font_entry->open_args = g_new (FT_Open_Args *, nfaces);
	  font_entry->face_indices = g_new (FT_Long, nfaces);
	  
	  for (i = 0; i < nfaces; i++)
	    {
	      PangoFontDescription desc;
	      PangoFT2OA *oa;

	      desc = font_entry->description;
	      desc.family_name = faces[i];
	      oa = g_hash_table_lookup (ft2fontmap->faces, &desc);
	      if (!oa)
		g_warning ("Face '%s' on line %d of '%s' not found", faces[i], lineno, filename);
	      else
		{
		  font_entry->open_args[font_entry->n_fonts] = oa->open_args;
		  font_entry->face_indices[font_entry->n_fonts] = oa->face_index;
		  font_entry->n_fonts++;
		}
	    }

	  /* Insert the font entry into our structures */

	  family_entry = pango_ft2_get_family_entry (ft2fontmap, font_entry->description.family_name);
	  family_entry->font_entries = g_slist_prepend (family_entry->font_entries, font_entry);
	  ft2fontmap->n_fonts++;

	  g_free (font_entry->description.family_name);
	  font_entry->description.family_name = family_entry->family_name;
	  font_entry->cached_fonts = NULL;
	  font_entry->coverage = NULL;
	}

      if (ferror (infile))
	g_warning ("Error reading file '%s': %s", filename, g_strerror(errno));

      goto out;

    error:
      if (font_entry)
	{
	  if (font_entry->open_args)
	    g_free (font_entry->open_args);
	  if (font_entry->face_indices)
	    g_free (font_entry->face_indices);
	  if (font_entry->description.family_name)
	    g_free (font_entry->description.family_name);
	  g_free (font_entry);
	}

      g_warning ("Error parsing line %d of alias file '%s'", lineno, filename);

    out:
      g_string_free (tmp_buf, TRUE);
      g_string_free (line_buf, TRUE);

      fclose (infile);
    }

}

static void
pango_ft2_font_map_read_aliases (PangoFT2FontMap *ft2fontmap)
{
  char **files;
  char *files_str = pango_config_key_get ("PangoFT2/AliasFiles");
  char *home;
  char *tmp_str;
  int n;

  if (!files_str)
    {
      home = g_get_home_dir ();
      if (home && *home)
	files_str = g_strconcat
	  (home,
	   G_DIR_SEPARATOR_S ".pangoft2_aliases" G_SEARCHPATH_SEPARATOR_S,
	   NULL);

      tmp_str = g_strconcat (files_str,
			     pango_get_sysconf_subdirectory (),
			     G_DIR_SEPARATOR_S "pangoft2.aliases",
			     NULL);
      g_free (files_str);
      files_str = tmp_str;
    }

  files = pango_split_file_list (files_str);
  
  n = 0;
  while (files[n])
    n++;

  while (n-- > 0)
    pango_ft2_font_map_read_alias_file (ft2fontmap, files[n]);

  g_strfreev (files);
  g_free (files_str);
}

#if DEBUGGING

static void
pango_print_desc (PangoFontDescription *desc)
{
  g_print ("%s%s%s%s%s",
	   desc->family_name,
	   (desc->style == PANGO_STYLE_NORMAL ? "" :
	    (desc->style == PANGO_STYLE_OBLIQUE ? " OBLIQUE" :
	     (desc->style == PANGO_STYLE_ITALIC ? " ITALIC" : " ???"))),
	   (desc->variant == PANGO_VARIANT_NORMAL ? "" :
	    (desc->variant == PANGO_VARIANT_SMALL_CAPS ? " SMALL CAPS" : "???")),
	   (desc->weight >= (PANGO_WEIGHT_LIGHT + PANGO_WEIGHT_NORMAL) / 2 &&
	    desc->weight < (PANGO_WEIGHT_NORMAL + PANGO_WEIGHT_BOLD) / 2 ? "" :
	    (desc->weight < (PANGO_WEIGHT_ULTRALIGHT + PANGO_WEIGHT_LIGHT) / 2 ? " ULTRALIGHT" :
	     (desc->weight >= (PANGO_WEIGHT_ULTRALIGHT + PANGO_WEIGHT_LIGHT) / 2 &&
	      desc->weight < (PANGO_WEIGHT_LIGHT + PANGO_WEIGHT_NORMAL) / 2 ? " LIGHT" :
	      (desc->weight >= (PANGO_WEIGHT_NORMAL + PANGO_WEIGHT_BOLD) / 2 &&
	       desc->weight < (PANGO_WEIGHT_BOLD + PANGO_WEIGHT_ULTRABOLD) / 2 ? " BOLD" :
	       (desc->weight >= (PANGO_WEIGHT_BOLD + PANGO_WEIGHT_ULTRABOLD) / 2 &&
		desc->weight < (PANGO_WEIGHT_ULTRABOLD + PANGO_WEIGHT_HEAVY) / 2 ? " ULTRABOLD" :
		" HEAVY"))))),
	   (desc->stretch == PANGO_STRETCH_ULTRA_CONDENSED ? " ULTRA CONDENSED" :
	    (desc->stretch == PANGO_STRETCH_EXTRA_CONDENSED ? " EXTRA CONDENSED" :
	     (desc->stretch == PANGO_STRETCH_CONDENSED ? " CONDENSED" :
	      (desc->stretch == PANGO_STRETCH_SEMI_CONDENSED ? " SEMI CONDENSED" :
	       (desc->stretch == PANGO_STRETCH_NORMAL ? "" :
		(desc->stretch == PANGO_STRETCH_SEMI_EXPANDED ? " SEMI EXPANDED" :
		 (desc->stretch == PANGO_STRETCH_EXPANDED ? " EXPANDED" :
		  (desc->stretch == PANGO_STRETCH_EXTRA_EXPANDED ? " EXTRA EXPANDED" :
		   (desc->stretch == PANGO_STRETCH_ULTRA_EXPANDED ? " ULTRA EXPANDED" : " ???"))))))))));
}

static void
pango_ft2_print_oa (PangoFT2OA *oa)
{
  g_print ("%s:%ld", oa->open_args->pathname, oa->face_index);
}

#endif

static void
pango_ft2_insert_face (PangoFT2FontMap *ft2fontmap,
		       FT_Face          face,
		       const char      *path,
		       int              face_index)
{
  PangoFontDescription *description;
  GSList *tmp_list;
  PangoFT2FamilyEntry *family_entry;
  PangoFT2FontEntry *font_entry;
  PangoFT2OA *oa;
  FT_Open_Args *open_args;

  description = g_new (PangoFontDescription, 1);
  description->family_name = g_strdup (face->family_name);
  g_strdown (description->family_name);

  if (face->style_flags & FT_STYLE_FLAG_ITALIC)
    description->style = PANGO_STYLE_ITALIC;
  else
    description->style = PANGO_STYLE_NORMAL;

  description->variant = PANGO_VARIANT_NORMAL;

  if (face->style_flags & FT_STYLE_FLAG_BOLD)
    description->weight = PANGO_WEIGHT_BOLD;
  else
    description->weight = PANGO_WEIGHT_NORMAL;

  description->stretch = PANGO_STRETCH_NORMAL;

  if (face->style_name)
    {
      gchar **styles = g_strsplit (face->style_name, " ", 0);
      gint i = 0;

      while (styles[i])
	{
	  (void) (pango_parse_style (styles[i], description, FALSE) ||
		  pango_parse_variant (styles[i], description, FALSE) ||
		  pango_parse_weight (styles[i], description, FALSE) ||
		  pango_parse_stretch (styles[i], description, FALSE));
	  i++;
	}
      g_strfreev (styles);
    }

  description->size = 0;

#if 0
  PING ((""));
  pango_print_desc (description);
#endif

  family_entry = pango_ft2_get_family_entry (ft2fontmap, description->family_name);

  tmp_list = family_entry->font_entries;
  while (tmp_list)
    {
      font_entry = tmp_list->data;

      if (font_entry->description.style == description->style &&
	  font_entry->description.variant == description->variant &&
	  font_entry->description.weight == description->weight &&
	  font_entry->description.stretch == description->stretch)
	{
	  g_free (description->family_name);
	  g_free (description);
#if 0
	  PING ((" family and description matched (!)\n"));
#endif
	  return;
	}

      tmp_list = tmp_list->next;
    }

  oa = g_hash_table_lookup (ft2fontmap->faces, description);
  if (!oa)
    {
      oa = g_new (PangoFT2OA, 1);
      open_args = g_new (FT_Open_Args, 1);
      open_args->flags = ft_open_pathname;
      open_args->pathname = g_strdup (path);
      open_args->driver = NULL;
      open_args->num_params = 0;
      oa->open_args = open_args;
      oa->face_index = face_index;
#if 0
      PING (("adding mapping: "));
      pango_ft2_print_oa (oa);
#endif
      g_hash_table_insert (ft2fontmap->faces, description, oa);
    }
#if 0
  g_print ("\n");
#endif

  font_entry = g_new (PangoFT2FontEntry, 1);
  font_entry->description = *description;
  font_entry->description.family_name = family_entry->family_name;
  font_entry->cached_fonts = NULL;
  font_entry->coverage = NULL;
  font_entry->open_args = g_new (FT_Open_Args *, 1);
  font_entry->open_args[0] = oa->open_args;
  font_entry->face_indices = g_new (FT_Long, 1);
  font_entry->face_indices[0] = oa->face_index;
  font_entry->n_fonts = 1;
  family_entry->font_entries = g_slist_append (family_entry->font_entries, font_entry);
  ft2fontmap->n_fonts++;
}

static void
free_coverages_foreach (gpointer key,
			gpointer value,
			gpointer data)
{
  pango_coverage_unref (value);
}

PangoCoverage *
pango_ft2_font_entry_get_coverage (PangoFT2FontEntry *entry,
				   PangoFont         *font,
				   const char        *lang)
{
  guint32 ch;
  PangoMap *shape_map;
  PangoCoverage *coverage;
  PangoCoverage *result;
  PangoCoverageLevel font_level;
  PangoMapEntry *map_entry;
  GHashTable *coverage_hash;

  if (entry)
    if (entry->coverage)
      {
	pango_coverage_ref (entry->coverage);
	return entry->coverage;
      }

  result = pango_coverage_new ();

  coverage_hash = g_hash_table_new (g_str_hash, g_str_equal);
  
  shape_map = pango_ft2_get_shaper_map (lang);

  for (ch = 0; ch < 65536; ch++)
    {
      map_entry = pango_map_get_entry (shape_map, ch);
      if (map_entry->info)
	{
	  coverage = g_hash_table_lookup (coverage_hash, map_entry->info->id);
	  if (!coverage)
	    {
	      PangoEngineShape *engine = (PangoEngineShape *)pango_map_get_engine (shape_map, ch);
	      coverage = engine->get_coverage (font, lang);
	      g_hash_table_insert (coverage_hash, map_entry->info->id, coverage);
	    }
	  
	  font_level = pango_coverage_get (coverage, ch);
	  if (font_level == PANGO_COVERAGE_EXACT && !map_entry->is_exact)
	    font_level = PANGO_COVERAGE_APPROXIMATE;

	  if (font_level != PANGO_COVERAGE_NONE)
	    pango_coverage_set (result, ch, font_level);
	}
    }

  g_hash_table_foreach (coverage_hash, free_coverages_foreach, NULL);
  g_hash_table_destroy (coverage_hash);

  if (entry)
    {
      entry->coverage = result;
      pango_coverage_ref (result);
    }

  return result;
}

void
pango_ft2_font_entry_remove (PangoFT2FontEntry *entry,
			     PangoFont         *font)
{
  entry->cached_fonts = g_slist_remove (entry->cached_fonts, font);
}

PangoFT2FontCache *
pango_ft2_font_map_get_font_cache (PangoFontMap *font_map)
{
  g_return_val_if_fail (font_map != NULL, NULL);
  g_return_val_if_fail (PANGO_FT2_IS_FONT_MAP (font_map), NULL);

  return PANGO_FT2_FONT_MAP (font_map)->font_cache;
}

void
pango_ft2_fontmap_cache_add (PangoFontMap *fontmap,
			     PangoFT2Font *ft2font)
{
  PangoFT2FontMap *ft2fontmap = PANGO_FT2_FONT_MAP (fontmap);

  if (ft2fontmap->freed_fonts->length == MAX_FREED_FONTS)
    {
      PangoFT2Font *old_font = g_queue_pop_tail (ft2fontmap->freed_fonts);
      g_object_unref (G_OBJECT (old_font));
    }

  g_object_ref (G_OBJECT (ft2font));
  g_queue_push_head (ft2fontmap->freed_fonts, ft2font);
  ft2font->in_cache = TRUE;
}

void
pango_ft2_fontmap_cache_remove (PangoFontMap *fontmap,
				PangoFT2Font *ft2font)
{
  PangoFT2FontMap *ft2fontmap = PANGO_FT2_FONT_MAP (fontmap);

  GList *link = g_list_find (ft2fontmap->freed_fonts->head, ft2font);
  if (link == ft2fontmap->freed_fonts->tail)
    {
      ft2fontmap->freed_fonts->tail = ft2fontmap->freed_fonts->tail->prev;
      if (ft2fontmap->freed_fonts->tail)
	ft2fontmap->freed_fonts->tail->next = NULL;
    }
  
  ft2fontmap->freed_fonts->head = g_list_delete_link (ft2fontmap->freed_fonts->head, link);
  ft2fontmap->freed_fonts->length--;
  ft2font->in_cache = FALSE;

  g_object_unref (G_OBJECT (ft2font));
}

static void
pango_ft2_fontmap_cache_clear (PangoFT2FontMap *ft2fontmap)
{
  g_list_foreach (ft2fontmap->freed_fonts->head, (GFunc)g_object_unref, NULL);
  g_list_free (ft2fontmap->freed_fonts->head);
  ft2fontmap->freed_fonts->head = NULL;
  ft2fontmap->freed_fonts->tail = NULL;
  ft2fontmap->freed_fonts->length = 0;
}

static void
pango_ft2_font_entry_dump (int                indent,
			   PangoFT2FontEntry *font_entry)
{
  int i;

  printf ("%*sPangoFT2FontEntry@%p:\n"
	  "%*s  lfp:\n",
	  indent, "", font_entry,
	  indent, "");
  
  for (i = 0; i < font_entry->n_fonts; i++)
    printf ("%*s    PangoFT2OpenArgs:%s:%ld\n",
	    indent, "", font_entry->open_args[i]->pathname, font_entry->face_indices[i]);
  
  printf ("%*s  description:\n"
	  "%*s    family_name: %s\n"
	  "%*s    style: %d\n"
	  "%*s    variant: %d\n"
	  "%*s    weight: %d\n"
	  "%*s    stretch: %d\n"
	  "%*s  coverage: %p\n",
	  indent, "",
	  indent, "", font_entry->description.family_name,
	  indent, "", font_entry->description.style,
	  indent, "", font_entry->description.variant,
	  indent, "", font_entry->description.weight,
	  indent, "", font_entry->description.stretch,
	  indent, "", font_entry->coverage);
}

static void
pango_ft2_family_entry_dump (int                  indent,
			     PangoFT2FamilyEntry *entry)
{
  GSList *tmp_list = entry->font_entries;
  
  printf ("%*sPangoFT2FamilyEntry@%p:\n"
	  "%*s  family_name: %s\n"
	  "%*s  font_entries:\n",
	  indent, "", entry,
	  indent, "", entry->family_name,
	  indent, "");

  while (tmp_list)
    {
      PangoFT2FontEntry *font_entry = tmp_list->data;
      
      pango_ft2_font_entry_dump (indent + 2, font_entry);
      tmp_list = tmp_list->next;
    }
}

static void
dump_family (gpointer key,
	     gpointer value,
	     gpointer user_data)
{
  PangoFT2FamilyEntry *entry = value;
  int indent = (int) user_data;

  pango_ft2_family_entry_dump (indent, entry);
}

void
pango_ft2_fontmap_dump (int           indent,
			PangoFontMap *fontmap)
{
  PangoFT2FontMap *ft2fontmap = PANGO_FT2_FONT_MAP (fontmap);

  printf ("%*sPangoFT2FontMap@%p:\n",
	  indent, "", ft2fontmap);
  g_hash_table_foreach (ft2fontmap->families, dump_family, (gpointer) (indent + 2));
}
