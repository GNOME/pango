/* Pango
 * pangowin32-fontmap.c: Font handling
 *
 * Copyright (C) 2000 Red Hat Software
 * Copyright (C) 2000 Tor Lillqvist
 * Copyright (C) 2001 Alexander Larsson
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

#include <ctype.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <errno.h>

#include "pango-fontmap.h"
#include "pango-utils.h"
#include "pangowin32-private.h"

#define PANGO_TYPE_WIN32_FONT_MAP           (pango_win32_font_map_get_type ())
#define PANGO_WIN32_FONT_MAP(object)        (G_TYPE_CHECK_INSTANCE_CAST ((object), PANGO_TYPE_WIN32_FONT_MAP, PangoWin32FontMap))
#define PANGO_WIN32_IS_FONT_MAP(object)     (G_TYPE_CHECK_INSTANCE_TYPE ((object), PANGO_TYPE_WIN32_FONT_MAP))

typedef struct _PangoWin32FontMap      PangoWin32FontMap;
typedef struct _PangoWin32Family       PangoWin32Family;
typedef struct _PangoWin32SizeInfo     PangoWin32SizeInfo;

/* Number of freed fonts */
#define MAX_FREED_FONTS 16

struct _PangoWin32FontMap
{
  PangoFontMap parent_instance;

  PangoWin32FontCache *font_cache;
  GQueue *freed_fonts;

  /* Map Pango family names tp PangoWin32Family structs */
  GHashTable *families;

  /* Maps the family and style of a face to a PangoWin32Face struct */
  GHashTable *faces;

  /* Map LOGFONTS (taking into account only the lfFaceName, lfItalic
   * and lfWeight fields) to PangoWin32SizeInfo structs.
   */
  GHashTable *size_infos;

  int n_fonts;

  double resolution;		/* (points / pixel) * PANGO_SCALE */
};

struct _PangoWin32FontMapClass
{
  PangoFontMapClass parent_class;
};

struct _PangoWin32Family
{
  PangoFontFamily parent_instance;

  char *family_name;
  GSList *font_entries;
};

struct _PangoWin32SizeInfo
{
  GSList *logfonts;
};

#define PANGO_WIN32_TYPE_FAMILY              (pango_win32_family_get_type ())
#define PANGO_WIN32_FAMILY(object)           (G_TYPE_CHECK_INSTANCE_CAST ((object), PANGO_WIN32_TYPE_FAMILY, PangoWin32Family))
#define PANGO_WIN32_IS_FAMILY(object)        (G_TYPE_CHECK_INSTANCE_TYPE ((object), PANGO_WIN32_TYPE_FAMILY))

#define PANGO_WIN32_TYPE_FACE              (pango_win32_face_get_type ())
#define PANGO_WIN32_FACE(object)           (G_TYPE_CHECK_INSTANCE_CAST ((object), PANGO_WIN32_TYPE_FACE, PangoWin32Face))
#define PANGO_WIN32_IS_FACE(object)        (G_TYPE_CHECK_INSTANCE_TYPE ((object), PANGO_WIN32_TYPE_FACE))

GType             pango_win32_family_get_type        (void);
GType             pango_win32_face_get_type          (void);

static void       pango_win32_font_map_init          (PangoWin32FontMap       *fontmap);
static void       pango_win32_font_map_class_init    (PangoFontMapClass       *class);

static void       pango_win32_font_map_finalize      (GObject                      *object);
static PangoFont *pango_win32_font_map_load_font     (PangoFontMap                 *fontmap,
						      const PangoFontDescription   *description);
static void       pango_win32_font_map_list_families (PangoFontMap                 *fontmap,
						       PangoFontFamily            ***families,
						       int                          *n_families);
  
static void       pango_win32_fontmap_cache_clear    (PangoWin32FontMap            *win32fontmap);

#ifdef _WE_WANT_GLOBAL_ALIASES_
static void       pango_win32_font_map_read_aliases  (PangoWin32FontMap            *win32fontmap);
#endif 

static void       pango_win32_insert_font            (PangoWin32FontMap            *fontmap,
						      LOGFONT                      *lfp);

static PangoFontClass *parent_class;	/* Parent class structure for PangoWin32FontMap */

static GType
pango_win32_font_map_get_type (void)
{
  static GType object_type = 0;

  if (!object_type)
    {
      static const GTypeInfo object_info =
      {
        sizeof (PangoFontMapClass),
        (GBaseInitFunc) NULL,
        (GBaseFinalizeFunc) NULL,
        (GClassInitFunc) pango_win32_font_map_class_init,
        NULL,           /* class_finalize */
        NULL,           /* class_data */
        sizeof (PangoWin32FontMap),
        0,              /* n_preallocs */
        (GInstanceInitFunc) pango_win32_font_map_init,
      };
      
      object_type = g_type_register_static (PANGO_TYPE_FONT_MAP,
                                            "PangoWin32FontMap",
                                            &object_info, 0);
    }
  
  return object_type;
}

/* A hash function for LOGFONTs that takes into consideration
 * only those fields that indicate a specific .ttf file is in
 * use. Dunno how correct this is.
 */

static guint
logfont_nosize_hash (gconstpointer v)
{
  const LOGFONT *lfp = v;

  return g_str_hash (lfp->lfFaceName) +
    lfp->lfItalic +
    lfp->lfWeight;
}

/* Ditto comparison function */
static gboolean
logfont_nosize_equal (gconstpointer v1,
		      gconstpointer v2)
{
  const LOGFONT *lfp1 = v1, *lfp2 = v2;

  return (strcmp (lfp1->lfFaceName, lfp2->lfFaceName) == 0
	  && lfp1->lfItalic == lfp2->lfItalic
	  && lfp1->lfWeight == lfp2->lfWeight);
}
  
static void 
pango_win32_font_map_init (PangoWin32FontMap *win32fontmap)
{
  win32fontmap->families = g_hash_table_new (g_str_hash, g_str_equal);
  win32fontmap->size_infos = g_hash_table_new (logfont_nosize_hash, logfont_nosize_equal);
  win32fontmap->faces = g_hash_table_new ((GHashFunc)pango_font_description_hash,
                                          (GEqualFunc)pango_font_description_equal);
  win32fontmap->n_fonts = 0;
}

static void
pango_win32_font_map_class_init (PangoFontMapClass *class)
{
  GObjectClass *object_class = G_OBJECT_CLASS (class);
  
  parent_class = g_type_class_peek_parent (class);
  
  object_class->finalize = pango_win32_font_map_finalize;
  class->load_font = pango_win32_font_map_load_font;
  class->list_families = pango_win32_font_map_list_families;

  if (pango_win32_hdc == NULL)
    pango_win32_hdc = CreateDC ("DISPLAY", NULL, NULL, NULL);
}

static PangoWin32FontMap *fontmap = NULL;

static int CALLBACK
pango_win32_inner_enum_proc (LOGFONT    *lfp,
			     TEXTMETRIC *metrics,
			     DWORD       fontType,
			     LPARAM      lParam)
{
  pango_win32_insert_font (fontmap, lfp);

  return 1;
}

static int CALLBACK
pango_win32_enum_proc (LOGFONT    *lfp,
		       TEXTMETRIC *metrics,
		       DWORD       fontType,
		       LPARAM      lParam)
{
  LOGFONT lf;

  if (fontType != TRUETYPE_FONTTYPE)
    return 1;

  lf = *lfp;

  EnumFontFamiliesEx (pango_win32_hdc, &lf, (FONTENUMPROC) pango_win32_inner_enum_proc, lParam, 0);

  return 1;
}

PangoFontMap *
pango_win32_font_map_for_display (void)
{
  LOGFONT logfont;
  RECT rect;

  /* Make sure that the type system is initialized */
  g_type_init ();
  
  if (fontmap != NULL)
    return PANGO_FONT_MAP (fontmap);

  fontmap = (PangoWin32FontMap *)g_type_create_instance (PANGO_TYPE_WIN32_FONT_MAP);
  
  fontmap->font_cache = pango_win32_font_cache_new ();
  fontmap->freed_fonts = g_queue_new ();

  memset (&logfont, 0, sizeof (logfont));
  logfont.lfCharSet = DEFAULT_CHARSET;
  EnumFontFamiliesEx (pango_win32_hdc, &logfont, (FONTENUMPROC) pango_win32_enum_proc, 0, 0);

#ifdef _WE_WANT_GLOBAL_ALIASES_
  pango_win32_font_map_read_aliases (fontmap);
#endif

  SystemParametersInfo(SPI_GETWORKAREA, 0, &rect, 0);
  fontmap->resolution =
    (PANGO_SCALE * 72.27 / 25.4) * ((double) GetDeviceCaps (pango_win32_hdc, HORZSIZE) /
				    (rect.right - rect.left));

  return PANGO_FONT_MAP (fontmap);
}

/**
 * pango_win32_shutdown_display:
 * 
 * Free cached resources.
 **/
void
pango_win32_shutdown_display (void)
{
  pango_win32_fontmap_cache_clear (fontmap);
  g_object_unref (G_OBJECT (fontmap));
}

static void
pango_win32_font_map_finalize (GObject *object)
{
  PangoWin32FontMap *win32fontmap = PANGO_WIN32_FONT_MAP (object);
  
  g_list_foreach (win32fontmap->freed_fonts->head, (GFunc)g_object_unref, NULL);
  g_queue_free (win32fontmap->freed_fonts);
  
  pango_win32_font_cache_free (win32fontmap->font_cache);

  /* ??? */
  G_OBJECT_CLASS (parent_class)->finalize (object);
}

/*
 * PangoWin32Family
 */
static void
pango_win32_family_list_faces (PangoFontFamily  *family,
			   PangoFontFace  ***faces,
			   int              *n_faces)
{
  PangoWin32Family *win32family = PANGO_WIN32_FAMILY (family);
 
  *n_faces = g_slist_length (win32family->font_entries);
  if (faces)
    {
      GSList *tmp_list;
      int i = 0;
      
      *faces = g_new (PangoFontFace *, *n_faces);

      tmp_list = win32family->font_entries;
      while (tmp_list)
	{
	  (*faces)[i++] = tmp_list->data;
	  tmp_list = tmp_list->next;
	}
    }
}

const char *
pango_win32_family_get_name (PangoFontFamily  *family)
{
  PangoWin32Family *win32family = PANGO_WIN32_FAMILY (family);
  return win32family->family_name;
}

static void
pango_win32_family_class_init (PangoFontFamilyClass *class)
{
  class->list_faces = pango_win32_family_list_faces;
  class->get_name = pango_win32_family_get_name;
}

GType
pango_win32_family_get_type (void)
{
  static GType object_type = 0;

  if (!object_type)
    {
      static const GTypeInfo object_info =
      {
        sizeof (PangoFontFamilyClass),
        (GBaseInitFunc) NULL,
        (GBaseFinalizeFunc) NULL,
        (GClassInitFunc) pango_win32_family_class_init,
        NULL,           /* class_finalize */
        NULL,           /* class_data */
        sizeof (PangoWin32Family),
        0,              /* n_preallocs */
        (GInstanceInitFunc) NULL,
      };
      
      object_type = g_type_register_static (PANGO_TYPE_FONT_FAMILY,
                                            "PangoWin32Family",
                                            &object_info, 0);
    }
  
  return object_type;
}

static void
list_families_foreach (gpointer key,
		       gpointer value,
		       gpointer user_data)
{
  GSList **list = user_data;

  *list = g_slist_prepend (*list, value);
}

static void
pango_win32_font_map_list_families (PangoFontMap *fontmap,
				    PangoFontFamily ***families,
				    int          *n_families)
{
  GSList *family_list = NULL;
  GSList *tmp_list;
  PangoWin32FontMap *win32fontmap = (PangoWin32FontMap *)fontmap;

  if (!n_families)
    return;

  g_hash_table_foreach (win32fontmap->families, list_families_foreach, &family_list);

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

static PangoWin32Family *
pango_win32_get_font_family (PangoWin32FontMap *win32fontmap,
                        const char        *family_name)
{
  PangoWin32Family *win32family = g_hash_table_lookup (win32fontmap->families, family_name);
  if (!win32family)
    {
      win32family = g_object_new (PANGO_WIN32_TYPE_FAMILY, NULL);
      win32family->family_name = g_strdup (family_name);
      win32family->font_entries = NULL;
      
      g_hash_table_insert (win32fontmap->families, win32family->family_name, win32family);
    }

  return win32family;
}

static PangoFont *
pango_win32_font_map_load_font (PangoFontMap               *fontmap,
				const PangoFontDescription *description)
{
  PangoWin32FontMap *win32fontmap = (PangoWin32FontMap *)fontmap;
  PangoWin32Family *win32family;
  PangoFont *result = NULL;
  GSList *tmp_list;
  gchar *name;

  g_return_val_if_fail (description != NULL, NULL);
  
  name = g_ascii_strdown (pango_font_description_get_family (description), -1);

  win32family = g_hash_table_lookup (win32fontmap->families, name);
  if (win32family)
    {
      PangoWin32Face *best_match = NULL;
      
      tmp_list = win32family->font_entries;
      while (tmp_list)
	{
	  PangoWin32Face *face = tmp_list->data;

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
	      PangoWin32Font *win32font = tmp_list->data;
	      if (win32font->size == size)
		{
		  result = (PangoFont *)win32font;

		  g_object_ref (G_OBJECT (result));
		  if (win32font->in_cache)
		    pango_win32_fontmap_cache_remove (fontmap, win32font);
		  
		  break;
		}
	      tmp_list = tmp_list->next;
	    }

	  if (!result)
	    {
	      PangoWin32Font *win32font;
	      
	      win32font = pango_win32_font_new (fontmap, &best_match->logfont, size);
	      win32font->fontmap = fontmap;
	      win32font->entry = best_match;
	      best_match->cached_fonts = g_slist_prepend (best_match->cached_fonts, win32font);

	      result = (PangoFont *)win32font;
	    }
	}
    }

  g_free (name);
  return result;
}

#if _WE_WANT_GLOBAL_ALIASES_
static void
pango_win32_font_map_read_alias_file (PangoWin32FontMap *win32fontmap,
				      const char        *filename)
{
  PangoWin32Face *face = NULL;
  FILE *infile;
  char **faces;
  int lineno = 0;
  int nfaces;
  int i;

  infile = fopen (filename, "r");
  if (infile)
    {
      GString *line_buf = g_string_new (NULL);
      GString *tmp_buf = g_string_new (NULL);

      while (pango_read_line (infile, line_buf))
	{
	  PangoWin32Family *family_entry;
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

	  face = g_object_new g_object_new (PANGO_WIN32_TYPE_FACE, NULL);
	  face->n_fonts = 0;
	  face->description = pango_font_description_new ();

	  g_string_ascii_down (tmp_buf);
	  pango_font_description_set_family (face->description, tmp_buf->str);
	  
	  if (!pango_scan_string (&p, tmp_buf))
	    goto error;

	  if (!pango_parse_style (tmp_buf->str, &style, TRUE))
	    goto error;
	  pango_font_description_set_style (font_entry->description, style);

	  if (!pango_scan_string (&p, tmp_buf))
	    goto error;

	  if (!pango_parse_variant (tmp_buf->str, &variant, TRUE))
	    goto error;
	  pango_font_description_set_variant (font_entry->description, variant);
	  
	  if (!pango_scan_string (&p, tmp_buf))
	    goto error;

	  if (!pango_parse_weight (tmp_buf->str, &weight, TRUE))
	    goto error;
	  pango_font_description_set_weight (font_entry->description, weight);
	  
	  if (!pango_scan_string (&p, tmp_buf))
	    goto error;

	  if (!pango_parse_stretch (tmp_buf->str, &stretch, TRUE))
	    goto error;
	  pango_font_description_set_stretch (font_entry->description, stretch);

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
	  font_entry->lfp = g_new0 (LOGFONT, nfaces);
	  font_entry->n_fonts = nfaces;
	  for (i = 0; i < nfaces; i++)
	    {
	      strcpy (font_entry->lfp[i].lfFaceName, faces[i]);

	      /* Set LOGFONT properties based on the PangoFontDescription */
	      if (font_entry->description.style == PANGO_STYLE_OBLIQUE ||
		  font_entry->description.style == PANGO_STYLE_ITALIC)
		font_entry->lfp[i].lfItalic = TRUE;

	      /* Quantize weight */
	      if (font_entry->description.weight <=
		  (PANGO_WEIGHT_ULTRALIGHT + PANGO_WEIGHT_LIGHT) / 2)
		font_entry->lfp[i].lfWeight = FW_ULTRALIGHT;
	      else if (font_entry->description.weight <=
		       (PANGO_WEIGHT_LIGHT + PANGO_WEIGHT_NORMAL) / 2)
		font_entry->lfp[i].lfWeight = FW_LIGHT;
	      else if (font_entry->description.weight <=
		       (PANGO_WEIGHT_NORMAL + PANGO_WEIGHT_BOLD) / 2)
		font_entry->lfp[i].lfWeight = FW_NORMAL;
	      else if (font_entry->description.weight <=
		       (PANGO_WEIGHT_BOLD + PANGO_WEIGHT_ULTRABOLD) / 2)
		font_entry->lfp[i].lfWeight = FW_BOLD;
	      else if (font_entry->description.weight <=
		       (PANGO_WEIGHT_ULTRABOLD + PANGO_WEIGHT_HEAVY) / 2)
		font_entry->lfp[i].lfWeight = FW_ULTRABOLD;
	      else
		font_entry->lfp[i].lfWeight = FW_HEAVY;

	      font_entry->lfp[i].lfQuality = ANTIALIASED_QUALITY;

	      /* Stretch ? */

	      /* Charset is ignored anyway when used with the widechar
	       * API?
	       */
	      font_entry->lfp[i].lfCharSet = DEFAULT_CHARSET;
	    }
	  g_strfreev (faces);
	  
	  /* Insert the font entry into our structures */

	  family_entry = pango_win32_get_family_entry (win32fontmap, font_entry->description.family_name);
	  family_entry->font_entries = g_slist_prepend (family_entry->font_entries, font_entry);
	  win32fontmap->n_fonts++;

	  g_free (font_entry->description.family_name);
	  font_entry->description.family_name = family_entry->family_name;
	  font_entry->cached_fonts = NULL;
	  font_entry->coverage = NULL;
	}

      if (ferror (infile))
	g_warning ("Error reading '%s': %s", filename, g_strerror(errno));

      goto out;

    error:
      if (font_entry)
	{
	  if (font_entry->lfp)
	    g_free (font_entry->lfp);
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
pango_win32_font_map_read_aliases (PangoWin32FontMap *win32fontmap)
{
  char **files;
  char *files_str = pango_config_key_get ("PangoWin32/AliasFiles");
  char *home;
  char *tmp_str;
  int n;

  if (!files_str)
    {
      home = g_get_home_dir ();
      if (home && *home)
	files_str = g_strconcat (home, "\\.pangowin32_aliases;", NULL);

      tmp_str = g_strconcat (files_str, pango_get_sysconf_subdirectory (),
			     "\\pangowin32.aliases",
			     NULL);
      g_free (files_str);
      files_str = tmp_str;
    }

  files = pango_split_file_list (files_str);
  
  n = 0;
  while (files[n])
    n++;

  while (n-- > 0)
    pango_win32_font_map_read_alias_file (fontmap, files[n]);

  g_strfreev (files);
  g_free (files_str);
}

#endif /* __WE_WANT_GLOVAL_ALIASES__ */

/* This inserts the given font into the SizeInfo table.
 * If a SizeInfo already exists with the same typeface name, then the
 * fontname is added to the SizeInfos list of fontnames, else a new SizeInfo
 * is created and inserted in the table.
 */
static void
pango_win32_insert_font (PangoWin32FontMap *win32fontmap,
			 LOGFONT           *lfp)
{
  LOGFONT *lfp2;
  PangoFontDescription *description;
  char *family_name;
  GSList *tmp_list;
  PangoWin32Family *font_family;
  PangoWin32Face *win32face;
  PangoWin32SizeInfo *size_info;
  PangoStyle style;
  PangoVariant variant;
  PangoWeight weight;
  PangoStretch stretch;

  PING(("lfp.face=%s,wt=%ld,ht=%ld",lfp->lfFaceName,lfp->lfWeight,lfp->lfHeight));
  
  /* First insert the LOGFONT into the list of LOGFONTs for the typeface name
   */
  lfp2 = g_new (LOGFONT, 1);
  *lfp2 = *lfp;
  g_strdown (lfp2->lfFaceName);
  size_info = g_hash_table_lookup (win32fontmap->size_infos, lfp);
  if (!size_info)
    {
      PING(("SizeInfo not found"));
      size_info = g_new (PangoWin32SizeInfo, 1);
      size_info->logfonts = NULL;

      g_hash_table_insert (win32fontmap->size_infos, lfp2, size_info);
    }

  size_info->logfonts = g_slist_prepend (size_info->logfonts, lfp2);
  
  /* Convert the LOGFONT into a PangoFontDescription */
  
  family_name = g_strdup (lfp2->lfFaceName);
  
  if (!lfp2->lfItalic)
    style = PANGO_STYLE_NORMAL;
  else
    style = PANGO_STYLE_ITALIC;

  variant = PANGO_VARIANT_NORMAL;
  
  /* The PangoWeight values PANGO_WEIGHT_* map exactly do Windows FW_* values.
   * Is this on purpose? Quantize the weight to exact PANGO_WEIGHT_*
   * values. Is this a good idea?
   */
  if (lfp2->lfWeight == FW_DONTCARE)
    weight = PANGO_WEIGHT_NORMAL;
  else if (lfp2->lfWeight <= (FW_ULTRALIGHT + FW_LIGHT) / 2)
    weight = PANGO_WEIGHT_ULTRALIGHT;
  else if (lfp2->lfWeight <= (FW_LIGHT + FW_NORMAL) / 2)
    weight = PANGO_WEIGHT_LIGHT;
  else if (lfp2->lfWeight <= (FW_NORMAL + FW_BOLD) / 2)
    weight = PANGO_WEIGHT_NORMAL;
  else if (lfp2->lfWeight <= (FW_BOLD + FW_ULTRABOLD) / 2)
    weight = PANGO_WEIGHT_BOLD;
  else if (lfp2->lfWeight <= (FW_ULTRABOLD + FW_HEAVY) / 2)
    weight = PANGO_WEIGHT_ULTRABOLD;
  else
    weight = PANGO_WEIGHT_HEAVY;

  /* XXX No idea how to figure out the stretch */
  stretch = PANGO_STRETCH_NORMAL;

  font_family = pango_win32_get_font_family (win32fontmap, family_name);

  tmp_list = font_family->font_entries;
  while (tmp_list)
    {
      win32face = tmp_list->data;

      if (pango_font_description_get_style (win32face->description) == style &&
	  pango_font_description_get_weight (win32face->description) == weight &&
	  pango_font_description_get_stretch (win32face->description) == stretch &&
	  pango_font_description_get_variant (win32face->description) == variant)
	return;

      tmp_list = tmp_list->next;
    }

  description = pango_font_description_new ();
  pango_font_description_set_family_static (description, font_family->family_name);
  pango_font_description_set_style (description, style);
  pango_font_description_set_weight (description, weight);
  pango_font_description_set_stretch (description, stretch);
  pango_font_description_set_variant (description, variant);

  /*
   * Create the FontFace
   */
  win32face = g_object_new (PANGO_WIN32_TYPE_FACE, NULL);
  win32face->description = description;
  win32face->cached_fonts = NULL;
  win32face->coverage = NULL;
  win32face->logfont = *lfp;
  win32face->unicode_table = NULL;
  g_strdown (win32face->logfont.lfFaceName);
  font_family->font_entries = g_slist_append (font_family->font_entries, win32face);
  win32fontmap->n_fonts++;

  /*
   * There are magic family names coming from the X implementation.
   * They can be simply mapped to lfPitchAndFamily flag of the logfont
   * struct. These additional entries should probably only be references
   * to the respective entry created above. Thy are simply using the
   * same entry at the moment and it isn't crashing on g_free () ???
   * Maybe a memory leak ...
   */
  switch (lfp->lfPitchAndFamily & (0x0F << 4)) /* bit 4...7 */
    { 
    case FF_MODERN : /* monospace */
      font_family = pango_win32_get_font_family (win32fontmap, "monospace");
      font_family->font_entries = g_slist_append (font_family->font_entries, win32face);
      win32fontmap->n_fonts++;
      break;
    case FF_ROMAN : /* serif */
      font_family = pango_win32_get_font_family (win32fontmap, "serif");
      font_family->font_entries = g_slist_append (font_family->font_entries, win32face);
      win32fontmap->n_fonts++;
      break;
    case  FF_SWISS  : /* sans */
      font_family = pango_win32_get_font_family (win32fontmap, "sans");
      font_family->font_entries = g_slist_append (font_family->font_entries, win32face);
      win32fontmap->n_fonts++;
      break;
    }

}

/* Given a LOGFONT and size, make a matching LOGFONT corresponding to
 * an installed font.
 */
void
pango_win32_make_matching_logfont (PangoFontMap  *fontmap,
				   const LOGFONT *lfp,
				   int            size,
				   LOGFONT       *out)
{
  PangoWin32FontMap *win32fontmap;
  GSList *tmp_list;
  PangoWin32SizeInfo *size_info;
  LOGFONT *closest_match = NULL;
  gint match_distance = 0;

  PING(("lfp.face=%s,wt=%ld,ht=%ld,size:%d",lfp->lfFaceName,lfp->lfWeight,lfp->lfHeight,size));
  win32fontmap = PANGO_WIN32_FONT_MAP (fontmap);
  
  size_info = g_hash_table_lookup (win32fontmap->size_infos, lfp);

  if (!size_info)
    {
      PING(("SizeInfo not found"));
      return;
    }

  tmp_list = size_info->logfonts;
  while (tmp_list)
    {
      LOGFONT *tmp_logfont = tmp_list->data;
      int font_size = tmp_logfont->lfHeight;

      if (size != -1)
	{
	  int new_distance = (font_size == 0) ? 0 : abs (font_size - size);
	  
	  if (!closest_match ||
	      new_distance < match_distance ||
	      (new_distance < PANGO_SCALE && font_size != 0))
	    {
	      closest_match = tmp_logfont;
	      match_distance = new_distance;
	    }
	}

      tmp_list = tmp_list->next;
    }

  if (closest_match)
    {
      /* OK, we have a match; let's modify it to fit this size */

      *out = *closest_match;
      out->lfHeight = (int)((double)size / win32fontmap->resolution + 0.5);
      out->lfWidth = 0;
    }
  else
    *out = *lfp; /* Whatever. We need to pass something... */
}

static void
free_coverages_foreach (gpointer key,
			gpointer value,
			gpointer data)
{
  pango_coverage_unref (value);
}

void
pango_win32_font_entry_set_coverage (PangoWin32Face *face,
				     PangoCoverage       *coverage)
{
  face->coverage = pango_coverage_ref (coverage);
}

static PangoFontDescription *
pango_win32_face_describe (PangoFontFace *face)
{
  PangoWin32Face *win32face = PANGO_WIN32_FACE (face);

  return pango_font_description_copy (win32face->description);
}

static const char *
pango_win32_face_get_face_name (PangoFontFace *face)
{
  PangoWin32Face *win32face = PANGO_WIN32_FACE (face);

  if (!win32face->face_name)
    {
      PangoFontDescription *desc = pango_font_face_describe (face);
      
      pango_font_description_unset_fields (desc,
					   PANGO_FONT_MASK_FAMILY | PANGO_FONT_MASK_SIZE);

      win32face->face_name = pango_font_description_to_string (desc);
      pango_font_description_free (desc);
    }

  return win32face->face_name;
}

static void
pango_win32_face_class_init (PangoFontFaceClass *class)
{
  class->describe = pango_win32_face_describe;
  class->get_face_name = pango_win32_face_get_face_name;
}

GType
pango_win32_face_get_type (void)
{
  static GType object_type = 0;

  if (!object_type)
    {
      static const GTypeInfo object_info =
      {
        sizeof (PangoFontFaceClass),
        (GBaseInitFunc) NULL,
        (GBaseFinalizeFunc) NULL,
        (GClassInitFunc) pango_win32_face_class_init,
        NULL,           /* class_finalize */
        NULL,           /* class_data */
        sizeof (PangoWin32Face),
        0,              /* n_preallocs */
        (GInstanceInitFunc) NULL,
      };
      
      object_type = g_type_register_static (PANGO_TYPE_FONT_FACE,
                                            "PangoWin32Face",
                                            &object_info, 0);
    }
  
  return object_type;
}

PangoCoverage *
pango_win32_font_entry_get_coverage (PangoWin32Face *face)
{
  if (face->coverage)
    {
      pango_coverage_ref (face->coverage);
      return face->coverage;
    }

  return NULL;
}

void
pango_win32_font_entry_remove (PangoWin32Face *face,
			       PangoFont           *font)
{
  face->cached_fonts = g_slist_remove (face->cached_fonts, font);
}

PangoWin32FontCache *
pango_win32_font_map_get_font_cache (PangoFontMap *font_map)
{
  g_return_val_if_fail (font_map != NULL, NULL);
  g_return_val_if_fail (PANGO_WIN32_IS_FONT_MAP (font_map), NULL);

  return PANGO_WIN32_FONT_MAP (font_map)->font_cache;
}

void
pango_win32_fontmap_cache_add (PangoFontMap    *fontmap,
			       PangoWin32Font  *win32font)
{
  PangoWin32FontMap *win32fontmap = PANGO_WIN32_FONT_MAP (fontmap);

  if (win32fontmap->freed_fonts->length == MAX_FREED_FONTS)
    {
      PangoWin32Font *old_font = g_queue_pop_tail (win32fontmap->freed_fonts);
      g_object_unref (G_OBJECT (old_font));
    }

  g_object_ref (G_OBJECT (win32font));
  g_queue_push_head (win32fontmap->freed_fonts, win32font);
  win32font->in_cache = TRUE;
}

void
pango_win32_fontmap_cache_remove (PangoFontMap    *fontmap,
				  PangoWin32Font  *win32font)
{
  PangoWin32FontMap *win32fontmap = PANGO_WIN32_FONT_MAP (fontmap);

  GList *link = g_list_find (win32fontmap->freed_fonts->head, win32font);
  if (link == win32fontmap->freed_fonts->tail)
    {
      win32fontmap->freed_fonts->tail = win32fontmap->freed_fonts->tail->prev;
      if (win32fontmap->freed_fonts->tail)
	win32fontmap->freed_fonts->tail->next = NULL;
    }
  
  win32fontmap->freed_fonts->head = g_list_delete_link (win32fontmap->freed_fonts->head, link);
  win32fontmap->freed_fonts->length--;
  win32font->in_cache = FALSE;

  g_object_unref (G_OBJECT (win32font));
}

static void
pango_win32_fontmap_cache_clear (PangoWin32FontMap *win32fontmap)
{
  g_list_foreach (win32fontmap->freed_fonts->head, (GFunc)g_object_unref, NULL);
  g_list_free (win32fontmap->freed_fonts->head);
  win32fontmap->freed_fonts->head = NULL;
  win32fontmap->freed_fonts->tail = NULL;
  win32fontmap->freed_fonts->length = 0;
}
