/* Pango
 * pangoft2-fontmap.c:
 *
 * Copyright (C) 2000 Red Hat Software
 * Copyright (C) 2000 Tor Lillqvist
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
#define PANGO_FT2_IS_FONT_MAP(object)        (G_TYPE_CHECK_INSTANCE_TYPE ((object), PANGO_TYPE_FT2_FONT_MAP))

typedef struct _PangoFT2Family       PangoFT2Family;
typedef struct _PangoFT2FontMap      PangoFT2FontMap;
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

struct _PangoFT2Family
{
  PangoFontFamily parent_instance;
  
  char *family_name;

  /* List of PangoFT2FontEntry structs */
  GSList *font_entries;
};

#define PANGO_FT2_TYPE_FAMILY              (pango_ft2_family_get_type ())
#define PANGO_FT2_FAMILY(object)           (G_TYPE_CHECK_INSTANCE_CAST ((object), PANGO_FT2_TYPE_FAMILY, PangoFT2Family))
#define PANGO_FT2_IS_FAMILY(object)        (G_TYPE_CHECK_INSTANCE_TYPE ((object), PANGO_FT2_TYPE_FAMILY))

#define PANGO_FT2_TYPE_FACE              (pango_ft2_face_get_type ())
#define PANGO_FT2_FACE(object)           (G_TYPE_CHECK_INSTANCE_CAST ((object), PANGO_FT2_TYPE_FACE, PangoFT2Face))
#define PANGO_FT2_IS_FACE(object)        (G_TYPE_CHECK_INSTANCE_TYPE ((object), PANGO_FT2_TYPE_FACE))

static GType    pango_ft2_font_map_get_type      (void);
GType           pango_ft2_family_get_type          (void);
GType           pango_ft2_face_get_type            (void);

static void       pango_ft2_font_map_init          (PangoFT2FontMap             *fontmap);

static void       pango_ft2_font_map_class_init    (PangoFontMapClass           *class);

static void       pango_ft2_font_map_finalize      (GObject                      *object);

static PangoFont *pango_ft2_font_map_load_font     (PangoFontMap                 *fontmap,
						    const PangoFontDescription   *description);

static void       pango_ft2_font_map_list_families (PangoFontMap                 *fontmap,
						    PangoFontFamily            ***families,
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
        sizeof (PangoFontMapClass),
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

static void 
pango_ft2_font_map_init (PangoFT2FontMap *ft2fontmap)
{
  ft2fontmap->families = g_hash_table_new (g_str_hash, g_str_equal);
  ft2fontmap->faces = g_hash_table_new ((GHashFunc)pango_font_description_hash,
					(GEqualFunc)pango_font_description_equal);
  ft2fontmap->n_fonts = 0;
}

static void
pango_ft2_font_map_class_init (PangoFontMapClass *class)
{
  GObjectClass *object_class = G_OBJECT_CLASS (class);
  char *font_path;
  
  parent_class = g_type_class_peek_parent (class);
  
  object_class->finalize = pango_ft2_font_map_finalize;
  class->load_font = pango_ft2_font_map_load_font;
  class->list_families = pango_ft2_font_map_list_families;

  font_path = pango_config_key_get ("PangoFT2/FontPath");

  if (!font_path)
    {
      font_path = g_build_filename (pango_get_lib_subdirectory (),
				    "ft2fonts",
				    NULL);

#ifdef G_OS_WIN32
      {
	char win_dir[100];
	char *tmp_str;

	GetWindowsDirectory (win_dir, sizeof (win_dir));
	tmp_str = g_build_filename (font_path,
				    win_dir,
				    "fonts",
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

static gboolean
pango_ft2_scan_directory (const char      *path,
			  PangoFT2FontMap *ft2fontmap)
{
  DIR *dir;
  struct dirent *entry;
  char *fullname;
  FT_Face face;
  FT_Error error;
  int i;
  gboolean found_font = FALSE;

  dir = opendir (path);
  if (!dir)
    /* Don't warn; it's OK to have nonexistent entries in the font path */
    return FALSE;

  while ((entry = readdir (dir)) != NULL)
    {
      fullname = g_build_filename (path, entry->d_name, NULL);
      if (pango_ft2_is_font_file (fullname))
	{
	  error = FT_New_Face (ft2fontmap->library, fullname, 0, &face);
	  if (error != FT_Err_Ok)
	    g_warning ("Error loading font from '%s': %s",
		       fullname, pango_ft2_ft_strerror (error));
	  else
	    {
	      if (face->face_flags & FT_FACE_FLAG_SCALABLE)
		{
		  pango_ft2_insert_face (ft2fontmap, face, fullname, 0);
		  found_font = TRUE;
		}
	      
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
		    {
		      pango_ft2_insert_face (ft2fontmap, face, fullname, i);
		      found_font = TRUE;
		    }
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
  return found_font;
}

PangoFontMap *
pango_ft2_font_map_for_display (void)
{
  char **tmp_list;
  FT_Error error;
  gboolean read_font;

  /* Make sure that the type system is initialized */
  g_type_init ();
  
  if (pango_ft2_global_fontmap != NULL)
    return PANGO_FONT_MAP (pango_ft2_global_fontmap);

  pango_ft2_global_fontmap = (PangoFT2FontMap *)g_object_new (PANGO_TYPE_FT2_FONT_MAP, NULL);
  
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

  read_font = FALSE;
  while (*tmp_list)
    {
      read_font |= pango_ft2_scan_directory ((const char *) *tmp_list, pango_ft2_global_fontmap);
      tmp_list++;
    }

  if (!read_font)
    g_warning ("No fonts found by pangft2. Things will probably not work");
  
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

static void
list_families_foreach (gpointer key, gpointer value, gpointer user_data)
{
  GSList **list = user_data;

  *list = g_slist_prepend (*list, value);
}

static void
pango_ft2_font_map_list_families (PangoFontMap           *fontmap,
				  PangoFontFamily      ***families,
				  int                    *n_families)
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
	
      *families = g_new (PangoFontFamily *, *n_families);

      tmp_list = family_list;
      while (tmp_list)
	{
	  (*families)[i] = tmp_list->data;
	  i++;
	  tmp_list = tmp_list->next;
	}
    }
  
  g_slist_free (family_list);
}

static PangoFT2Family *
pango_ft2_get_family (PangoFT2FontMap *ft2fontmap,
		      const char      *family_name)
{
  PangoFT2Family *ft2family = g_hash_table_lookup (ft2fontmap->families, family_name);
  if (!ft2family)
    {
      ft2family = g_object_new (PANGO_FT2_TYPE_FAMILY, NULL);
      ft2family->family_name = g_strdup (family_name);
      ft2family->font_entries = NULL;
      
      g_hash_table_insert (ft2fontmap->families, ft2family->family_name, ft2family);
    }

  return ft2family;
}

static PangoFont *
pango_ft2_font_map_load_font (PangoFontMap               *fontmap,
			      const PangoFontDescription *description)
{
  PangoFT2FontMap *ft2fontmap = (PangoFT2FontMap *)fontmap;
  PangoFT2Family *family_entry;
  PangoFont *result = NULL;
  GSList *tmp_list;
  gchar *name;

  g_return_val_if_fail (description != NULL, NULL);
  
  name = g_ascii_strdown (pango_font_description_get_family (description), -1);

  family_entry = g_hash_table_lookup (ft2fontmap->families, name);
  g_free (name);
  
  if (family_entry)
    {
      PangoFT2Face *best_match = NULL;
      
      tmp_list = family_entry->font_entries;
      while (tmp_list)
	{
	  PangoFT2Face *face = tmp_list->data;
	  
	  if (pango_font_description_better_match (description,
						   best_match ? best_match->description : NULL,
						   face->description))
	    best_match = face;
	  
	  tmp_list = tmp_list->next;
	}

      if (best_match)
	{
	  GSList *tmp_list = best_match->cached_fonts;

	  gint size = pango_font_description_get_size (description);

	  while (tmp_list)
	    {
	      PangoFT2Font *ft2font = tmp_list->data;

	      if (ft2font->size == size)
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
						      size);
	      
	      ft2font->fontmap = fontmap;
	      ft2font->entry = best_match;
	      best_match->cached_fonts = g_slist_prepend (best_match->cached_fonts, ft2font);
	      
	      result = (PangoFont *)ft2font;
	    }
	}
    }

  return result;
}

static gboolean
pango_ft2_font_map_read_alias_file (PangoFT2FontMap *ft2fontmap,
				    const char      *filename)
{
  FILE *infile;
  int lineno = 0;
  int nfaces;
  int i;
  PangoFT2Face *face = NULL;
  gchar **faces;
  gboolean ret_val = FALSE;

  infile = fopen (filename, "r");
  if (infile)
    {
      GString *line_buf = g_string_new (NULL);
      GString *tmp_buf = g_string_new (NULL);

      while (pango_read_line (infile, line_buf))
	{
	  PangoFT2Family *family_entry;
	  PangoStyle style;
	  PangoVariant variant;
	  PangoWeight weight;
	  PangoStretch stretch;

	  const char *p = line_buf->str;
	  
	  lineno++;

	  if (!pango_skip_space (&p))
	    continue;

	  if (!pango_scan_string (&p, tmp_buf))
	    goto error;

	  face = g_new (PangoFT2Face, 1);
	  face->n_fonts = 0;
	  face->open_args = NULL;
	  face->face_indices = NULL;

	  face->description = pango_font_description_new ();

	  g_string_ascii_down (tmp_buf);
	  pango_font_description_set_family (face->description, tmp_buf->str);
	  
	  if (!pango_scan_string (&p, tmp_buf))
	    goto error;

	  if (!pango_parse_style (tmp_buf->str, &style, TRUE))
	    goto error;
	  pango_font_description_set_style (face->description, style);

	  if (!pango_scan_string (&p, tmp_buf))
	    goto error;

	  if (!pango_parse_variant (tmp_buf->str, &variant, TRUE))
	    goto error;
	  pango_font_description_set_variant (face->description, variant);
	  
	  if (!pango_scan_string (&p, tmp_buf))
	    goto error;

	  if (!pango_parse_weight (tmp_buf->str, &weight, TRUE))
	    goto error;
	  pango_font_description_set_weight (face->description, weight);
	  
	  if (!pango_scan_string (&p, tmp_buf))
	    goto error;

	  if (!pango_parse_stretch (tmp_buf->str, &stretch, TRUE))
	    goto error;
	  pango_font_description_set_stretch (face->description, stretch);

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

	  face->open_args = g_new (FT_Open_Args *, nfaces);
	  face->face_indices = g_new (FT_Long, nfaces);
	  
	  for (i = 0; i < nfaces; i++)
	    {
	      PangoFontDescription *desc = pango_font_description_copy_static (face->description);
	      PangoFT2OA *oa;

	      pango_font_description_set_family_static (desc, faces[i]);
	      oa = g_hash_table_lookup (ft2fontmap->faces, desc);
	      if (!oa)
		g_warning ("Face '%s' on line %d of '%s' not found", faces[i], lineno, filename);
	      else
		{
		  face->open_args[face->n_fonts] = oa->open_args;
		  face->face_indices[face->n_fonts] = oa->face_index;
		  face->n_fonts++;
		}

	      pango_font_description_free (desc);
	    }

	  g_strfreev (faces);
	  
	  /* Insert the font entry into our structures */

	  family_entry = pango_ft2_get_family (ft2fontmap, pango_font_description_get_family (face->description));
	  family_entry->font_entries = g_slist_prepend (family_entry->font_entries, face);
	  ft2fontmap->n_fonts++;

	  /* Save space by consolidating duplicated string */
	  pango_font_description_set_family_static (face->description, family_entry->family_name);
	  face->cached_fonts = NULL;
	  face->coverage = NULL;
	}

      if (ferror (infile))
	g_warning ("Error reading file '%s': %s", filename, g_strerror(errno));

      ret_val = TRUE;
      goto out;

    error:
      if (face)
	{
	  if (face->open_args)
	    g_free (face->open_args);
	  if (face->face_indices)
	    g_free (face->face_indices);
	  if (face->description)
	    pango_font_description_free (face->description);
	  g_free (face);
	}

      g_warning ("Error parsing line %d of alias file '%s'", lineno, filename);

    out:
      g_string_free (tmp_buf, TRUE);
      g_string_free (line_buf, TRUE);

      fclose (infile);
    }

  return ret_val;
}

static void
pango_ft2_font_map_read_aliases (PangoFT2FontMap *ft2fontmap)
{
  char **files;
  char *files_str = pango_config_key_get ("PangoFT2/AliasFiles");
  int n;
  gboolean read_aliasfile;

  if (!files_str)
    {
      const char *home = g_get_home_dir ();
      char *file1 = NULL;
      char *file2;

      if (home && *home)
	file1 = g_build_filename  (home, ".pangoft2_aliases", NULL);
      
      file2 = g_build_filename (pango_get_sysconf_subdirectory (),
				"pangoft2.aliases",
				NULL);

      files_str = g_build_path (G_SEARCHPATH_SEPARATOR_S,
				file1 ? file1 : file2,
				file1 ? file2 : NULL,
				NULL);
			     
      g_free (file1);
      g_free (file2);
    }

  files = pango_split_file_list (files_str);
  
  n = 0;
  while (files[n])
    n++;


  read_aliasfile = FALSE;
  
  while (n-- > 0)
    read_aliasfile |= pango_ft2_font_map_read_alias_file (ft2fontmap, files[n]);

  if (!read_aliasfile)
    g_warning ("Didn't read any pango ft2 fontalias file. Things will probably not work.");

  g_strfreev (files);
  g_free (files_str);
}

#if DEBUGGING

static void
pango_print_desc (PangoFontDescription *desc)
{
  PangoStyle style = pango_font_description_get_style (desc);
  PangoVariant variant = pango_font_description_get_variant (desc);
  PangoWeight weight = pango_font_description_get_weight (desc);
  PangoStretch stretch = pango_font_description_get_stretch (desc);
  
  g_print ("%s%s%s%s%s",
	   pango_font_get_family (desc),
	   (style == PANGO_STYLE_NORMAL ? "" :
	    (style == PANGO_STYLE_OBLIQUE ? " OBLIQUE" :
	     (style == PANGO_STYLE_ITALIC ? " ITALIC" : " ???"))),
	   (variant == PANGO_VARIANT_NORMAL ? "" :
	    (variant == PANGO_VARIANT_SMALL_CAPS ? " SMALL CAPS" : "???")),
	   (weight >= (PANGO_WEIGHT_LIGHT + PANGO_WEIGHT_NORMAL) / 2 &&
	    weight < (PANGO_WEIGHT_NORMAL + PANGO_WEIGHT_BOLD) / 2 ? "" :
	    (weight < (PANGO_WEIGHT_ULTRALIGHT + PANGO_WEIGHT_LIGHT) / 2 ? " ULTRALIGHT" :
	     (weight >= (PANGO_WEIGHT_ULTRALIGHT + PANGO_WEIGHT_LIGHT) / 2 &&
	      weight < (PANGO_WEIGHT_LIGHT + PANGO_WEIGHT_NORMAL) / 2 ? " LIGHT" :
	      (weight >= (PANGO_WEIGHT_NORMAL + PANGO_WEIGHT_BOLD) / 2 &&
	       weight < (PANGO_WEIGHT_BOLD + PANGO_WEIGHT_ULTRABOLD) / 2 ? " BOLD" :
	       (weight >= (PANGO_WEIGHT_BOLD + PANGO_WEIGHT_ULTRABOLD) / 2 &&
		weight < (PANGO_WEIGHT_ULTRABOLD + PANGO_WEIGHT_HEAVY) / 2 ? " ULTRABOLD" :
		" HEAVY"))))),
	   (stretch == PANGO_STRETCH_ULTRA_CONDENSED ? " ULTRA CONDENSED" :
	    (stretch == PANGO_STRETCH_EXTRA_CONDENSED ? " EXTRA CONDENSED" :
	     (stretch == PANGO_STRETCH_CONDENSED ? " CONDENSED" :
	      (stretch == PANGO_STRETCH_SEMI_CONDENSED ? " SEMI CONDENSED" :
	       (stretch == PANGO_STRETCH_NORMAL ? "" :
		(stretch == PANGO_STRETCH_SEMI_EXPANDED ? " SEMI EXPANDED" :
		 (stretch == PANGO_STRETCH_EXPANDED ? " EXPANDED" :
		  (stretch == PANGO_STRETCH_EXTRA_EXPANDED ? " EXTRA EXPANDED" :
		   (stretch == PANGO_STRETCH_ULTRA_EXPANDED ? " ULTRA EXPANDED" : " ???"))))))))));
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
  char *family_name;
  PangoStyle style;
  PangoVariant variant;
  PangoWeight weight;
  PangoStretch stretch;
  GSList *tmp_list;
  PangoFT2Family *family_entry;
  PangoFT2Face *face_entry;
  PangoFT2OA *oa;
  FT_Open_Args *open_args;

  family_name = g_ascii_strdown (face->family_name, -1);

  if (face->style_flags & FT_STYLE_FLAG_ITALIC)
    style = PANGO_STYLE_ITALIC;
  else
    style = PANGO_STYLE_NORMAL;

  variant = PANGO_VARIANT_NORMAL;

  if (face->style_flags & FT_STYLE_FLAG_BOLD)
    weight = PANGO_WEIGHT_BOLD;
  else
    weight = PANGO_WEIGHT_NORMAL;

  stretch = PANGO_STRETCH_NORMAL;

  if (face->style_name)
    {
      gchar **styles = g_strsplit (face->style_name, " ", 0);
      gint i = 0;

      while (styles[i])
	{
	  (void) (pango_parse_style (styles[i], &style, FALSE) ||
		  pango_parse_variant (styles[i], &variant, FALSE) ||
		  pango_parse_weight (styles[i], &weight, FALSE) ||
		  pango_parse_stretch (styles[i], &stretch, FALSE));
	  i++;
	}
      g_strfreev (styles);
    }

#if 0
  PING ((""));
  pango_print_desc (description);
#endif

  family_entry = pango_ft2_get_family (ft2fontmap, family_name);
  g_free (family_name);

  tmp_list = family_entry->font_entries;
  while (tmp_list)
    {
      face_entry = tmp_list->data;

      if (pango_font_description_get_style (face_entry->description) == style &&
	  pango_font_description_get_weight (face_entry->description) == weight &&
	  pango_font_description_get_stretch (face_entry->description) == stretch &&
	  pango_font_description_get_variant (face_entry->description) == variant)
	{
#if 0
	  PING ((" family and description matched (!)"));
#endif
	  return;
	}

      tmp_list = tmp_list->next;
    }

  description = pango_font_description_new ();
  pango_font_description_set_family_static (description, family_entry->family_name);
  pango_font_description_set_style (description, style);
  pango_font_description_set_weight (description, weight);
  pango_font_description_set_stretch (description, stretch);
  pango_font_description_set_variant (description, variant);

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

  face_entry = g_object_new (PANGO_FT2_TYPE_FACE, NULL);
  face_entry->description = description;
  face_entry->cached_fonts = NULL;
  face_entry->coverage = NULL;
  face_entry->open_args = g_new (FT_Open_Args *, 1);
  face_entry->open_args[0] = oa->open_args;
  face_entry->face_indices = g_new (FT_Long, 1);
  face_entry->face_indices[0] = oa->face_index;
  face_entry->n_fonts = 1;
  family_entry->font_entries = g_slist_append (family_entry->font_entries, face_entry);
  ft2fontmap->n_fonts++;
}

static void
free_coverages_foreach (gpointer key,
			gpointer value,
			gpointer data)
{
  pango_coverage_unref (value);
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
pango_ft2_face_dump (int           indent,
		     PangoFT2Face *face)
{
  int i;

  printf ("%*sPangoFT2Face@%p:\n"
	  "%*s  lfp:\n",
	  indent, "", face,
	  indent, "");
  
  for (i = 0; i < face->n_fonts; i++)
    printf ("%*s    PangoFT2OpenArgs:%s:%ld\n",
	    indent, "", face->open_args[i]->pathname, face->face_indices[i]);
  
  printf ("%*s  description:\n"
	  "%*s    family_name: %s\n"
	  "%*s    style: %d\n"
	  "%*s    variant: %d\n"
	  "%*s    weight: %d\n"
	  "%*s    stretch: %d\n"
	  "%*s  coverage: %p\n",
	  indent, "",
	  indent, "", pango_font_description_get_family (face->description),
	  indent, "", pango_font_description_get_style (face->description),
	  indent, "", pango_font_description_get_variant (face->description),
	  indent, "", pango_font_description_get_weight (face->description),
	  indent, "", pango_font_description_get_stretch (face->description),
	  indent, "", face->coverage);
}

static void
pango_ft2_family_entry_dump (int             indent,
			     PangoFT2Family *entry)
{
  GSList *tmp_list = entry->font_entries;
  
  printf ("%*sPangoFT2Family@%p:\n"
	  "%*s  family_name: %s\n"
	  "%*s  font_entries:\n",
	  indent, "", entry,
	  indent, "", entry->family_name,
	  indent, "");

  while (tmp_list)
    {
      PangoFT2Face *face = tmp_list->data;
      
      pango_ft2_face_dump (indent + 2, face);
      tmp_list = tmp_list->next;
    }
}

static void
dump_family (gpointer key,
	     gpointer value,
	     gpointer user_data)
{
  PangoFT2Family *entry = value;
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

/* 
 * PangoFT2Face
 */

static PangoFontDescription *
pango_ft2_face_describe (PangoFontFace *face)
{
  PangoFT2Face *ft2face = PANGO_FT2_FACE (face);

  return pango_font_description_copy (ft2face->description);
}

static const char *
pango_ft2_face_get_face_name (PangoFontFace *face)
{
  PangoFT2Face *ft2face = PANGO_FT2_FACE (face);

  if (!ft2face->face_name)
    {
      PangoFontDescription *desc = pango_font_face_describe (face);
      
      pango_font_description_unset_fields (desc,
					   PANGO_FONT_MASK_FAMILY | PANGO_FONT_MASK_SIZE);

      ft2face->face_name = pango_font_description_to_string (desc);
      pango_font_description_free (desc);
    }

  return ft2face->face_name;
}

static void
pango_ft2_face_class_init (PangoFontFaceClass *class)
{
  class->describe = pango_ft2_face_describe;
  class->get_face_name = pango_ft2_face_get_face_name;
}

GType
pango_ft2_face_get_type (void)
{
  static GType object_type = 0;

  if (!object_type)
    {
      static const GTypeInfo object_info =
      {
        sizeof (PangoFontFaceClass),
        (GBaseInitFunc) NULL,
        (GBaseFinalizeFunc) NULL,
        (GClassInitFunc) pango_ft2_face_class_init,
        NULL,           /* class_finalize */
        NULL,           /* class_data */
        sizeof (PangoFT2Face),
        0,              /* n_preallocs */
        (GInstanceInitFunc) NULL,
      };
      
      object_type = g_type_register_static (PANGO_TYPE_FONT_FACE,
                                            "PangoFT2Face",
                                            &object_info, 0);
    }
  
  return object_type;
}

PangoCoverage *
pango_ft2_face_get_coverage (PangoFT2Face      *face,
			     PangoFont         *font,
			     PangoLanguage     *language)
{
  guint32 ch;
  PangoMap *shape_map;
  PangoCoverage *coverage;
  PangoCoverage *result;
  PangoCoverageLevel font_level;
  PangoMapEntry *map_entry;
  GHashTable *coverage_hash;
  PangoFontDescription *description;
  FILE *cache_file;
  char *file_name;
  char *cache_file_name;
  char *font_as_filename;
  guchar *buf;
  size_t buflen;

  if (face)
    if (face->coverage)
      {
	pango_coverage_ref (face->coverage);
	return face->coverage;
      }

  description = pango_font_describe (font);
  font_as_filename = pango_font_description_to_filename (description);
  file_name = g_strconcat (font_as_filename, ".",
			   language ? pango_language_to_string (language) : "",
			   NULL);
  g_free (font_as_filename);
  cache_file_name = g_build_filename (pango_get_sysconf_subdirectory (),
				      "cache.ft2", file_name, NULL);
  g_free (file_name);
  pango_font_description_free (description);

  PING (("trying to load %s", cache_file_name));
  result = NULL;
  if (g_file_get_contents (cache_file_name, (char **)&buf, &buflen, NULL))
    {
      result = pango_coverage_from_bytes (buf, buflen);
      g_free (buf);
    }

  if (!result)
    {
      result = pango_coverage_new ();

      coverage_hash = g_hash_table_new (g_str_hash, g_str_equal);
  
      shape_map = pango_ft2_get_shaper_map (language);

      for (ch = 0; ch < 65536; ch++)
	{
	  map_entry = pango_map_get_entry (shape_map, ch);
	  if (map_entry->info)
	    {
	      coverage = g_hash_table_lookup (coverage_hash, map_entry->info->id);
	      if (!coverage)
		{
		  PangoEngineShape *engine = (PangoEngineShape *)pango_map_get_engine (shape_map, ch);
		  coverage = engine->get_coverage (font, language);
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

      cache_file = fopen (cache_file_name, "wb");
      if (cache_file)
	{
	  pango_coverage_to_bytes (result, &buf, &buflen);
	  PING (("saving %d bytes to %s", buflen, cache_file_name));
	  fwrite (buf, buflen, 1, cache_file);
	  fclose (cache_file);
	  g_free (buf);
	}
    }

  if (face)
    {
      face->coverage = result;
      pango_coverage_ref (result);
    }

  g_free (cache_file_name);

  return result;
}

void
pango_ft2_face_remove (PangoFT2Face *face,
		       PangoFont    *font)
{
  face->cached_fonts = g_slist_remove (face->cached_fonts, font);
}

/*
 * PangoXFontFamily
 */

static void
pango_ft2_family_list_faces (PangoFontFamily  *family,
			   PangoFontFace  ***faces,
			   int              *n_faces)
{
  PangoFT2Family *ft2family = PANGO_FT2_FAMILY (family);

  *n_faces = g_slist_length (ft2family->font_entries);
  if (faces)
    {
      GSList *tmp_list;
      int i = 0;
      
      *faces = g_new (PangoFontFace *, *n_faces);

      tmp_list = ft2family->font_entries;
      while (tmp_list)
	{
	  (*faces)[i++] = tmp_list->data;
	  tmp_list = tmp_list->next;
	}
    }
}

const char *
pango_ft2_family_get_name (PangoFontFamily  *family)
{
  PangoFT2Family *ft2family = PANGO_FT2_FAMILY (family);
  return ft2family->family_name;
}

static void
pango_ft2_family_class_init (PangoFontFamilyClass *class)
{
  class->list_faces = pango_ft2_family_list_faces;
  class->get_name = pango_ft2_family_get_name;
}

GType
pango_ft2_family_get_type (void)
{
  static GType object_type = 0;

  if (!object_type)
    {
      static const GTypeInfo object_info =
      {
        sizeof (PangoFontFamilyClass),
        (GBaseInitFunc) NULL,
        (GBaseFinalizeFunc) NULL,
        (GClassInitFunc) pango_ft2_family_class_init,
        NULL,           /* class_finalize */
        NULL,           /* class_data */
        sizeof (PangoFT2Family),
        0,              /* n_preallocs */
        (GInstanceInitFunc) NULL,
      };
      
      object_type = g_type_register_static (PANGO_TYPE_FONT_FAMILY,
                                            "PangoFT2Family",
                                            &object_info, 0);
    }
  
  return object_type;
}
