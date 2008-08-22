/* Pango
 * pangowin32-fontmap.c: Win32 font handling
 *
 * Copyright (C) 2000 Red Hat Software
 * Copyright (C) 2000 Tor Lillqvist
 * Copyright (C) 2001 Alexander Larsson
 * Copyright (C) 2007 Novell, Inc.
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
#include "pango-impl-utils.h"
#include "pangowin32-private.h"
#include "modules.h"

typedef struct _PangoWin32Family       PangoWin32Family;

struct _PangoWin32Family
{
  PangoFontFamily parent_instance;

  char *family_name;
  GSList *faces;

  gboolean is_monospace;
};

#if !defined(NTM_PS_OPENTYPE)
# define NTM_PS_OPENTYPE 0x20000
#endif

#if !defined(NTM_TYPE1)
# define NTM_TYPE1 0x100000
#endif

#define PANGO_WIN32_TYPE_FAMILY              (pango_win32_family_get_type ())
#define PANGO_WIN32_FAMILY(object)           (G_TYPE_CHECK_INSTANCE_CAST ((object), PANGO_WIN32_TYPE_FAMILY, PangoWin32Family))
#define PANGO_WIN32_IS_FAMILY(object)        (G_TYPE_CHECK_INSTANCE_TYPE ((object), PANGO_WIN32_TYPE_FAMILY))

#define PANGO_WIN32_TYPE_FACE              (pango_win32_face_get_type ())
#define PANGO_WIN32_FACE(object)           (G_TYPE_CHECK_INSTANCE_CAST ((object), PANGO_WIN32_TYPE_FACE, PangoWin32Face))
#define PANGO_WIN32_IS_FACE(object)        (G_TYPE_CHECK_INSTANCE_TYPE ((object), PANGO_WIN32_TYPE_FACE))

static GType      pango_win32_face_get_type          (void);

static GType      pango_win32_family_get_type        (void);

static void       pango_win32_face_list_sizes        (PangoFontFace  *face,
						      int           **sizes,
						      int            *n_sizes);

static void       pango_win32_font_map_finalize      (GObject                      *object);
static PangoFont *pango_win32_font_map_load_font     (PangoFontMap                 *fontmap,
						      PangoContext                 *context,
						      const PangoFontDescription   *description);
static void       pango_win32_font_map_list_families (PangoFontMap                 *fontmap,
						      PangoFontFamily            ***families,
						      int                          *n_families);

static PangoFont *pango_win32_font_map_real_find_font (PangoWin32FontMap           *win32fontmap,
						       PangoContext                *context,
						       PangoWin32Face              *face,
						       const PangoFontDescription  *description);

static void       pango_win32_fontmap_cache_clear    (PangoWin32FontMap            *win32fontmap);

static void       pango_win32_insert_font            (PangoWin32FontMap            *fontmap,
						      LOGFONTW                     *lfp,
						      gboolean			    is_synthetic);

static PangoWin32Family *pango_win32_get_font_family (PangoWin32FontMap            *win32fontmap,
						      const char                   *family_name);

static const char *pango_win32_face_get_face_name    (PangoFontFace *face);

static PangoWin32FontMap *default_fontmap = NULL;

G_DEFINE_TYPE (PangoWin32FontMap, _pango_win32_font_map, PANGO_TYPE_FONT_MAP)

#define TOLOWER(c) \
  (((c) >= 'A' && (c) <= 'Z') ? (c) - 'A' + 'a' : (c))

static guint
case_insensitive_str_hash (const char *key)
{
  const char *p = key;
  guint h = TOLOWER (*p);

  if (h)
    {
      for (p += 1; *p != '\0'; p++)
	h = (h << 5) - h + TOLOWER (*p);
    }

  return h;
}

static gboolean
case_insensitive_str_equal (const char *key1,
			    const char *key2)
{
  while (*key1 && *key2 && TOLOWER (*key1) == TOLOWER (*key2))
    key1++, key2++;
  return (!*key1 && !*key2);
}

static guint
case_insensitive_wcs_hash (const wchar_t *key)
{
  const wchar_t *p = key;
  guint h = TOLOWER (*p);

  if (h)
    {
      for (p += 1; *p != '\0'; p++)
	h = (h << 5) - h + TOLOWER (*p);
    }

  return h;
}

static gboolean
case_insensitive_wcs_equal (const wchar_t *key1,
			    const wchar_t *key2)
{
  while (*key1 && *key2 && TOLOWER (*key1) == TOLOWER (*key2))
    key1++, key2++;
  return (!*key1 && !*key2);
}

/* A hash function for LOGFONTWs that takes into consideration only
 * those fields that indicate a specific .ttf file is in use:
 * lfFaceName, lfItalic and lfWeight. Dunno how correct this is.
 */
static guint
logfontw_nosize_hash (const LOGFONTW *lfp)
{
  return case_insensitive_wcs_hash (lfp->lfFaceName) + (lfp->lfItalic != 0) + lfp->lfWeight;
}

/* Ditto comparison function */
static gboolean
logfontw_nosize_equal (const LOGFONTW *lfp1,
		       const LOGFONTW *lfp2)
{
  return (case_insensitive_wcs_equal (lfp1->lfFaceName, lfp2->lfFaceName)
	  && (lfp1->lfItalic != 0) == (lfp2->lfItalic != 0)
	  && lfp1->lfWeight == lfp2->lfWeight);
}

static int CALLBACK
pango_win32_inner_enum_proc (LOGFONTW    *lfp,
			     TEXTMETRICW *metrics,
			     DWORD        fontType,
			     LPARAM       lParam)
{
  PangoWin32FontMap *win32fontmap = (PangoWin32FontMap *)lParam;

  /* Windows generates synthetic vertical writing versions of East
   * Asian fonts with @ prepended to their name, ignore them.
   */
  if (lfp->lfFaceName[0] != '@')
    pango_win32_insert_font (win32fontmap, lfp, FALSE);

  return 1;
}

static int CALLBACK
pango_win32_enum_proc (LOGFONTW       *lfp,
		       NEWTEXTMETRICW *metrics,
		       DWORD           fontType,
		       LPARAM          lParam)
{
  LOGFONTW lf;

  PING (("%S: %lu %lx", lfp->lfFaceName, fontType, metrics->ntmFlags));

  if (fontType == TRUETYPE_FONTTYPE ||
      (_pango_win32_os_version_info.dwMajorVersion >= 5 &&
       ((metrics->ntmFlags & NTM_PS_OPENTYPE) || (metrics->ntmFlags & NTM_TYPE1))))
    {
      lf = *lfp;

      EnumFontFamiliesExW (_pango_win32_hdc, &lf,
			   (FONTENUMPROCW) pango_win32_inner_enum_proc,
			   lParam, 0);
    }

  return 1;
}

static void
synthesize_foreach (gpointer key,
		    gpointer value,
		    gpointer user_data)
{
  PangoWin32Family *win32family = value;
  PangoWin32FontMap *win32fontmap = user_data;

  enum { NORMAL, BOLDER, SLANTED };
  PangoWin32Face *variant[4] = { NULL, NULL, NULL, NULL };

  GSList *p;

  LOGFONTW lf;

  p = win32family->faces;
  while (p)
    {
      PangoWin32Face *win32face = p->data;

      /* Don't synthesize anything unless it's a monospace, serif, or sans font */
      if (!((win32face->logfontw.lfPitchAndFamily & 0xF0) == FF_MODERN ||
	    (win32face->logfontw.lfPitchAndFamily & 0xF0) == FF_ROMAN ||
	    (win32face->logfontw.lfPitchAndFamily & 0xF0) == FF_SWISS))
	return;

      if (pango_font_description_get_weight (win32face->description) == PANGO_WEIGHT_NORMAL &&
	  pango_font_description_get_style (win32face->description) == PANGO_STYLE_NORMAL)
	variant[NORMAL] = win32face;

      if (pango_font_description_get_weight (win32face->description) > PANGO_WEIGHT_NORMAL &&
	  pango_font_description_get_style (win32face->description) == PANGO_STYLE_NORMAL)
	variant[BOLDER] = win32face;

      if (pango_font_description_get_weight (win32face->description) == PANGO_WEIGHT_NORMAL &&
	  pango_font_description_get_style (win32face->description) >= PANGO_STYLE_OBLIQUE)
	variant[SLANTED] = win32face;

      if (pango_font_description_get_weight (win32face->description) > PANGO_WEIGHT_NORMAL &&
	  pango_font_description_get_style (win32face->description) >= PANGO_STYLE_OBLIQUE)
	variant[BOLDER+SLANTED] = win32face;

      p = p->next;
    }

  if (variant[NORMAL] != NULL && variant[BOLDER] == NULL)
    {
      lf = variant[NORMAL]->logfontw;
      lf.lfWeight = FW_BOLD;
      
      pango_win32_insert_font (win32fontmap, &lf, TRUE);
    }

  if (variant[NORMAL] != NULL && variant[SLANTED] == NULL)
    {
      lf = variant[NORMAL]->logfontw;
      lf.lfItalic = 255;
      
      pango_win32_insert_font (win32fontmap, &lf, TRUE);
    }

  if (variant[NORMAL] != NULL &&
      variant[BOLDER+SLANTED] == NULL)
    {
      lf = variant[NORMAL]->logfontw;
      lf.lfWeight = FW_BOLD;
      lf.lfItalic = 255;

      pango_win32_insert_font (win32fontmap, &lf, TRUE);
    }
  else if (variant[BOLDER] != NULL &&
	   variant[BOLDER+SLANTED] == NULL)
    {
      lf = variant[BOLDER]->logfontw;
      lf.lfItalic = 255;

      pango_win32_insert_font (win32fontmap, &lf, TRUE);
    }
  else if (variant[SLANTED] != NULL &&
	   variant[BOLDER+SLANTED] == NULL)
    {
      lf = variant[SLANTED]->logfontw;
      lf.lfWeight = FW_BOLD;

      pango_win32_insert_font (win32fontmap, &lf, TRUE);
    }
}

static void
create_standard_family (PangoWin32FontMap *win32fontmap,
			const char        *standard_family_name)
{
  int i;
  int n_aliases;
  char **aliases;

  pango_lookup_aliases (standard_family_name, &aliases, &n_aliases);
  for (i = 0; i < n_aliases; i++)
    {
      PangoWin32Family *existing_family = g_hash_table_lookup (win32fontmap->families, aliases[i]);

      if (existing_family)
	{
	  PangoWin32Family *new_family = pango_win32_get_font_family (win32fontmap, standard_family_name);
	  GSList *p = existing_family->faces;

	  new_family->is_monospace = existing_family->is_monospace;

	  while (p)
	    {
	      const PangoWin32Face *old_face = p->data;
	      PangoWin32Face *new_face = g_object_new (PANGO_WIN32_TYPE_FACE, NULL);
	      int j;

	      new_face->logfontw = old_face->logfontw;
	      new_face->description = pango_font_description_copy_static (old_face->description);
	      pango_font_description_set_family_static (new_face->description, standard_family_name);

	      for (j = 0; j < PANGO_WIN32_N_COVERAGES; j++)
		{
		  if (old_face->coverages[j] != NULL)
		    new_face->coverages[j] = pango_coverage_ref (old_face->coverages[j]);
		  else
		    new_face->coverages[j] = NULL;
		}

	      new_face->is_synthetic = TRUE;

	      new_face->has_cmap = old_face->has_cmap;
	      new_face->cmap_format = old_face->cmap_format;
	      new_face->cmap = old_face->cmap;

	      new_face->cached_fonts = NULL;

	      new_family->faces = g_slist_append (new_family->faces, new_face);

	      p = p->next;
	    }
	  return;
	}
    }
  /* XXX What to do if none of the members of aliases for standard_family_name
   * exists on this machine?
   */
}

static void
_pango_win32_font_map_init (PangoWin32FontMap *win32fontmap)
{
  LOGFONTW logfont;

  win32fontmap->families = g_hash_table_new ((GHashFunc) case_insensitive_str_hash,
					     (GEqualFunc) case_insensitive_str_equal);
  win32fontmap->fonts =
    g_hash_table_new ((GHashFunc) logfontw_nosize_hash, (GEqualFunc) logfontw_nosize_equal);

  win32fontmap->font_cache = pango_win32_font_cache_new ();
  win32fontmap->freed_fonts = g_queue_new ();

  memset (&logfont, 0, sizeof (logfont));
  logfont.lfCharSet = DEFAULT_CHARSET;
  EnumFontFamiliesExW (_pango_win32_hdc, &logfont,
		       (FONTENUMPROCW) pango_win32_enum_proc,
		       (LPARAM) win32fontmap, 0);

  g_hash_table_foreach (win32fontmap->families, synthesize_foreach, win32fontmap);

  /* Create synthetic "Sans", "Serif" and "Monospace" families */
  create_standard_family (win32fontmap, "Sans");
  create_standard_family (win32fontmap, "Serif");
  create_standard_family (win32fontmap, "Monospace");

  win32fontmap->resolution = (PANGO_SCALE / (double) GetDeviceCaps (_pango_win32_hdc, LOGPIXELSY)) * 72.0;
}

static void
_pango_win32_font_map_class_init (PangoWin32FontMapClass *class)
{
  GObjectClass *object_class = G_OBJECT_CLASS (class);
  PangoFontMapClass *fontmap_class = PANGO_FONT_MAP_CLASS (class);
  int i;

  class->find_font = pango_win32_font_map_real_find_font;
  object_class->finalize = pango_win32_font_map_finalize;
  fontmap_class->load_font = pango_win32_font_map_load_font;
  fontmap_class->list_families = pango_win32_font_map_list_families;
  fontmap_class->shape_engine_type = PANGO_RENDER_TYPE_WIN32;

  pango_win32_get_dc ();

  for (i = 0; _pango_included_win32_modules[i].list; i++)
    pango_module_register (&_pango_included_win32_modules[i]);
}

/**
 * pango_win32_font_map_for_display:
 *
 * Returns a #PangoWin32FontMap. Font maps are cached and should
 * not be freed. If the font map is no longer needed, it can
 * be released with pango_win32_shutdown_display().
 *
 * Return value: a #PangoFontMap.
 **/
PangoFontMap *
pango_win32_font_map_for_display (void)
{
  /* Make sure that the type system is initialized */
  g_type_init ();

  if (default_fontmap != NULL)
    return PANGO_FONT_MAP (default_fontmap);

  default_fontmap = g_object_new (PANGO_TYPE_WIN32_FONT_MAP, NULL);

  return PANGO_FONT_MAP (default_fontmap);
}

/**
 * pango_win32_shutdown_display:
 *
 * Free cached resources.
 **/
void
pango_win32_shutdown_display (void)
{
  if (default_fontmap)
    {
      pango_win32_fontmap_cache_clear (default_fontmap);
      g_object_unref (default_fontmap);

      default_fontmap = NULL;
    }
}

static void
pango_win32_font_map_finalize (GObject *object)
{
  PangoWin32FontMap *win32fontmap = PANGO_WIN32_FONT_MAP (object);

  g_list_foreach (win32fontmap->freed_fonts->head, (GFunc)g_object_unref, NULL);
  g_queue_free (win32fontmap->freed_fonts);

  pango_win32_font_cache_free (win32fontmap->font_cache);

  G_OBJECT_CLASS (_pango_win32_font_map_parent_class)->finalize (object);
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
  GSList *p;
  int n;

  p = win32family->faces;
  n = 0;
  while (p)
    {
      n++;
      p = p->next;
    }

  if (faces)
    {
      int i;

      *faces = g_new (PangoFontFace *, n);

      p = win32family->faces;
      i = 0;
      while (p)
	{
	  (*faces)[i++] = p->data;
	  p = p->next;
	}
    }
  if (n_faces)
    *n_faces = n;
}

static const char *
pango_win32_family_get_name (PangoFontFamily  *family)
{
  PangoWin32Family *win32family = PANGO_WIN32_FAMILY (family);

  return win32family->family_name;
}

static gboolean
pango_win32_family_is_monospace (PangoFontFamily *family)
{
  PangoWin32Family *win32family = PANGO_WIN32_FAMILY (family);

  return win32family->is_monospace;
}

static void
pango_win32_family_class_init (PangoFontFamilyClass *class)
{
  class->list_faces = pango_win32_family_list_faces;
  class->get_name = pango_win32_family_get_name;
  class->is_monospace = pango_win32_family_is_monospace;
}

static GType
pango_win32_family_get_type (void)
{
  static GType object_type = 0;

  if (G_UNLIKELY (!object_type))
    {
      const GTypeInfo object_info =
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
					    I_("PangoWin32Family"),
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
pango_win32_font_map_list_families (PangoFontMap      *fontmap,
				    PangoFontFamily ***families,
				    int               *n_families)
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
      win32family->faces = NULL;

      g_hash_table_insert (win32fontmap->families, win32family->family_name, win32family);
    }

  return win32family;
}

static PangoFont *
pango_win32_font_map_load_font (PangoFontMap               *fontmap,
				PangoContext               *context,
				const PangoFontDescription *description)
{
  PangoWin32FontMap *win32fontmap = (PangoWin32FontMap *)fontmap;
  PangoWin32Family *win32family;
  PangoFont *result = NULL;
  GSList *tmp_list;

  g_return_val_if_fail (description != NULL, NULL);

  PING (("name=%s", pango_font_description_get_family (description)));

  win32family = g_hash_table_lookup (win32fontmap->families,
				     pango_font_description_get_family (description));
  if (win32family)
    {
      PangoWin32Face *best_match = NULL;

      PING (("got win32family"));
      tmp_list = win32family->faces;
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
	result = PANGO_WIN32_FONT_MAP_GET_CLASS (win32fontmap)->find_font (win32fontmap, context,
									   best_match,
									   description);
	/* TODO: Handle the case that result == NULL. */
      else
	PING (("no best match!"));
    }

  return result;
}

static PangoWin32Font *
pango_win32_font_neww (PangoFontMap   *fontmap,
		       const LOGFONTW *lfp,
		       int	      size)
{
  PangoWin32Font *result;

  g_return_val_if_fail (fontmap != NULL, NULL);
  g_return_val_if_fail (lfp != NULL, NULL);

  result = (PangoWin32Font *)g_object_new (PANGO_TYPE_WIN32_FONT, NULL);

  g_assert (result->fontmap == NULL);
  result->fontmap = fontmap;
  g_object_add_weak_pointer (G_OBJECT (result->fontmap), (gpointer *) (gpointer) &result->fontmap);

  result->size = size;
  _pango_win32_make_matching_logfontw (fontmap, lfp, size, &result->logfontw);

  return result;
}

static PangoFont *
pango_win32_font_map_real_find_font (PangoWin32FontMap          *win32fontmap,
				     PangoContext               *context,
				     PangoWin32Face             *face,
				     const PangoFontDescription *description)
{
  PangoFontMap *fontmap = PANGO_FONT_MAP (win32fontmap);
  PangoWin32Font *win32font;
  GSList *tmp_list = face->cached_fonts;
  int size = pango_font_description_get_size (description);

  if (pango_font_description_get_size_is_absolute (description))
    size = (int) 0.5 + (size * win32fontmap->resolution) / PANGO_SCALE;

  PING (("got best match:%S size=%d",face->logfontw.lfFaceName,size));

  while (tmp_list)
    {
      win32font = tmp_list->data;
      if (win32font->size == size)
	{
	  PING (("size matches"));

	  g_object_ref (win32font);
	  if (win32font->in_cache)
	    _pango_win32_fontmap_cache_remove (fontmap, win32font);

	  return (PangoFont *)win32font;
	}
      tmp_list = tmp_list->next;
    }

  win32font = pango_win32_font_neww (fontmap, &face->logfontw, size);

  if (!win32font)
    return NULL;

  win32font->win32face = face;
  face->cached_fonts = g_slist_prepend (face->cached_fonts, win32font);

  return (PangoFont *)win32font;
}

static gchar *
get_family_nameA (const LOGFONTA *lfp)
{
  HFONT hfont;
  HFONT oldhfont;

  struct name_header header;
  struct name_record record;

  gint unicode_ix = -1, mac_ix = -1, microsoft_ix = -1;
  gint name_ix;
  gchar *codeset;

  gchar *string = NULL;
  gchar *name;

  gint i, l;
  gsize nbytes;

  /* If lfFaceName is ASCII, assume it is the common (English) name
   * for the font. Is this valid? Do some TrueType fonts have
   * different names in French, German, etc, and does the system
   * return these if the locale is set to use French, German, etc?
   */
  l = strlen (lfp->lfFaceName);
  for (i = 0; i < l; i++)
    if (lfp->lfFaceName[i] < ' ' || lfp->lfFaceName[i] > '~')
      break;

  if (i == l)
    return g_strdup (lfp->lfFaceName);

  if ((hfont = CreateFontIndirect (lfp)) == NULL)
    goto fail0;

  if ((oldhfont = SelectObject (_pango_win32_hdc, hfont)) == NULL)
    goto fail1;

  if (!_pango_win32_get_name_header (_pango_win32_hdc, &header))
    goto fail2;

  PING (("%d name records", header.num_records));

  for (i = 0; i < header.num_records; i++)
    {
      if (!_pango_win32_get_name_record (_pango_win32_hdc, i, &record))
	goto fail2;

      if ((record.name_id != 1 && record.name_id != 16) || record.string_length <= 0)
	continue;

      PING (("platform:%d encoding:%d language:%04x name_id:%d",
	     record.platform_id, record.encoding_id, record.language_id, record.name_id));

      if (record.platform_id == APPLE_UNICODE_PLATFORM_ID ||
	  record.platform_id == ISO_PLATFORM_ID)
	unicode_ix = i;
      else if (record.platform_id == MACINTOSH_PLATFORM_ID &&
	       record.encoding_id == 0 && /* Roman */
	       record.language_id == 0)	/* English */
	mac_ix = i;
      else if (record.platform_id == MICROSOFT_PLATFORM_ID)
	if ((microsoft_ix == -1 ||
	     PRIMARYLANGID (record.language_id) == LANG_ENGLISH) &&
	    (record.encoding_id == SYMBOL_ENCODING_ID ||
	     record.encoding_id == UNICODE_ENCODING_ID ||
	     record.encoding_id == UCS4_ENCODING_ID))
	  microsoft_ix = i;
    }

  if (microsoft_ix >= 0)
    name_ix = microsoft_ix;
  else if (mac_ix >= 0)
    name_ix = mac_ix;
  else if (unicode_ix >= 0)
    name_ix = unicode_ix;
  else
    goto fail2;

  if (!_pango_win32_get_name_record (_pango_win32_hdc, name_ix, &record))
    goto fail2;

  string = g_malloc (record.string_length + 1);
  if (GetFontData (_pango_win32_hdc, NAME,
		   header.string_storage_offset + record.string_offset,
		   string, record.string_length) != record.string_length)
    goto fail2;

  string[record.string_length] = '\0';

  if (name_ix == microsoft_ix)
    if (record.encoding_id == SYMBOL_ENCODING_ID ||
	record.encoding_id == UNICODE_ENCODING_ID ||
	record.encoding_id == UCS4_ENCODING_ID)
      codeset = "UTF-16BE";
    else
      codeset = "UCS-4BE";
  else if (name_ix == mac_ix)
    codeset = "MacRoman";
  else /* name_ix == unicode_ix */
    codeset = "UCS-4BE";

  name = g_convert (string, record.string_length, "UTF-8", codeset, NULL, &nbytes, NULL);
  if (name == NULL)
    goto fail2;
  g_free (string);

  PING (("%s", name));

  SelectObject (_pango_win32_hdc, oldhfont);
  DeleteObject (hfont);

  return name;

 fail2:
  g_free (string);
  SelectObject (_pango_win32_hdc, oldhfont);

 fail1:
  DeleteObject (hfont);

 fail0:
  return g_locale_to_utf8 (lfp->lfFaceName, -1, NULL, NULL, NULL);
}

/**
 * pango_win32_font_description_from_logfont:
 * @lfp: a LOGFONTA
 *
 * Creates a #PangoFontDescription that matches the specified LOGFONTA.
 *
 * The face name, italicness and weight fields in the LOGFONTA are used
 * to set up the resulting #PangoFontDescription. If the face name in
 * the LOGFONTA contains non-ASCII characters the font is temporarily
 * loaded (using CreateFontIndirect()) and an ASCII (usually English)
 * name for it is looked up from the font name tables in the font
 * data. If that doesn't work, the face name is converted from the
 * system codepage to UTF-8 and that is used.
 *
 * Return value: the newly allocated #PangoFontDescription, which
 *  should be freed using pango_font_description_free()
 *
 * Since: 1.12
 */
PangoFontDescription *
pango_win32_font_description_from_logfont (const LOGFONT *lfp)
{
  PangoFontDescription *description;
  gchar *family;
  PangoStyle style;
  PangoVariant variant;
  PangoWeight weight;
  PangoStretch stretch;

  family = get_family_nameA (lfp);

  if (!lfp->lfItalic)
    style = PANGO_STYLE_NORMAL;
  else
    style = PANGO_STYLE_ITALIC;

  variant = PANGO_VARIANT_NORMAL;

  /* The PangoWeight values PANGO_WEIGHT_* map exactly do Windows FW_*
   * values.  Is this on purpose? Quantize the weight to exact
   * PANGO_WEIGHT_* values. Is this a good idea?
   */
  if (lfp->lfWeight == FW_DONTCARE)
    weight = PANGO_WEIGHT_NORMAL;
  else if (lfp->lfWeight <= (FW_ULTRALIGHT + FW_LIGHT) / 2)
    weight = PANGO_WEIGHT_ULTRALIGHT;
  else if (lfp->lfWeight <= (FW_LIGHT + FW_NORMAL) / 2)
    weight = PANGO_WEIGHT_LIGHT;
  else if (lfp->lfWeight <= (FW_NORMAL + FW_SEMIBOLD) / 2)
    weight = PANGO_WEIGHT_NORMAL;
  else if (lfp->lfWeight <= (FW_SEMIBOLD + FW_BOLD) / 2)
    weight = PANGO_WEIGHT_SEMIBOLD;
  else if (lfp->lfWeight <= (FW_BOLD + FW_ULTRABOLD) / 2)
    weight = PANGO_WEIGHT_BOLD;
  else if (lfp->lfWeight <= (FW_ULTRABOLD + FW_HEAVY) / 2)
    weight = PANGO_WEIGHT_ULTRABOLD;
  else
    weight = PANGO_WEIGHT_HEAVY;

  /* XXX No idea how to figure out the stretch */
  stretch = PANGO_STRETCH_NORMAL;

  description = pango_font_description_new ();
  pango_font_description_set_family (description, family);
  g_free(family);
  pango_font_description_set_style (description, style);
  pango_font_description_set_weight (description, weight);
  pango_font_description_set_stretch (description, stretch);
  pango_font_description_set_variant (description, variant);

  return description;
}

static gchar *
get_family_nameW (const LOGFONTW *lfp)
{
  HFONT hfont;
  HFONT oldhfont;

  struct name_header header;
  struct name_record record;

  gint unicode_ix = -1, mac_ix = -1, microsoft_ix = -1;
  gint name_ix;
  gchar *codeset;

  gchar *string = NULL;
  gchar *name;

  gint i, l;
  gsize nbytes;

  /* If lfFaceName is ASCII, assume it is the common (English) name
   * for the font. Is this valid? Do some TrueType fonts have
   * different names in French, German, etc, and does the system
   * return these if the locale is set to use French, German, etc?
   */
  l = wcslen (lfp->lfFaceName);
  for (i = 0; i < l; i++)
    if (lfp->lfFaceName[i] < ' ' || lfp->lfFaceName[i] > '~')
      break;

  if (i == l)
    return g_utf16_to_utf8 (lfp->lfFaceName, -1, NULL, NULL, NULL);

  if ((hfont = CreateFontIndirectW (lfp)) == NULL)
    goto fail0;

  if ((oldhfont = SelectObject (_pango_win32_hdc, hfont)) == NULL)
    goto fail1;

  if (!_pango_win32_get_name_header (_pango_win32_hdc, &header))
    goto fail2;

  PING (("%d name records", header.num_records));

  for (i = 0; i < header.num_records; i++)
    {
      if (!_pango_win32_get_name_record (_pango_win32_hdc, i, &record))
	goto fail2;

      if ((record.name_id != 1 && record.name_id != 16) || record.string_length <= 0)
	continue;

      PING (("platform:%d encoding:%d language:%04x name_id:%d",
	     record.platform_id, record.encoding_id, record.language_id, record.name_id));

      if (record.platform_id == APPLE_UNICODE_PLATFORM_ID ||
	  record.platform_id == ISO_PLATFORM_ID)
	unicode_ix = i;
      else if (record.platform_id == MACINTOSH_PLATFORM_ID &&
	       record.encoding_id == 0 && /* Roman */
	       record.language_id == 0)	/* English */
	mac_ix = i;
      else if (record.platform_id == MICROSOFT_PLATFORM_ID)
	if ((microsoft_ix == -1 ||
	     PRIMARYLANGID (record.language_id) == LANG_ENGLISH) &&
	    (record.encoding_id == SYMBOL_ENCODING_ID ||
	     record.encoding_id == UNICODE_ENCODING_ID ||
	     record.encoding_id == UCS4_ENCODING_ID))
	  microsoft_ix = i;
    }

  if (microsoft_ix >= 0)
    name_ix = microsoft_ix;
  else if (mac_ix >= 0)
    name_ix = mac_ix;
  else if (unicode_ix >= 0)
    name_ix = unicode_ix;
  else
    goto fail2;

  if (!_pango_win32_get_name_record (_pango_win32_hdc, name_ix, &record))
    goto fail2;

  string = g_malloc (record.string_length + 1);
  if (GetFontData (_pango_win32_hdc, NAME,
		   header.string_storage_offset + record.string_offset,
		   string, record.string_length) != record.string_length)
    goto fail2;

  string[record.string_length] = '\0';

  if (name_ix == microsoft_ix)
    if (record.encoding_id == SYMBOL_ENCODING_ID ||
	record.encoding_id == UNICODE_ENCODING_ID ||
	record.encoding_id == UCS4_ENCODING_ID)
      codeset = "UTF-16BE";
    else
      codeset = "UCS-4BE";
  else if (name_ix == mac_ix)
    codeset = "MacRoman";
  else /* name_ix == unicode_ix */
    codeset = "UCS-4BE";

  name = g_convert (string, record.string_length, "UTF-8", codeset, NULL, &nbytes, NULL);
  if (name == NULL)
    goto fail2;
  g_free (string);

  PING (("%s", name));

  SelectObject (_pango_win32_hdc, oldhfont);
  DeleteObject (hfont);

  return name;

 fail2:
  g_free (string);
  SelectObject (_pango_win32_hdc, oldhfont);

 fail1:
  DeleteObject (hfont);

 fail0:
  return g_utf16_to_utf8 (lfp->lfFaceName, -1, NULL, NULL, NULL);
}

/**
 * pango_win32_font_description_from_logfontw:
 * @lfp: a LOGFONTW
 *
 * Creates a #PangoFontDescription that matches the specified LOGFONTW.
 *
 * The face name, italicness and weight fields in the LOGFONTW are used
 * to set up the resulting #PangoFontDescription. If the face name in
 * the LOGFONTW contains non-ASCII characters the font is temporarily
 * loaded (using CreateFontIndirect()) and an ASCII (usually English)
 * name for it is looked up from the font name tables in the font
 * data. If that doesn't work, the face name is converted from UTF-16
 * to UTF-8 and that is used.
 *
 * Return value: the newly allocated #PangoFontDescription, which
 *  should be freed using pango_font_description_free()
 *
 * Since: 1.16
 */
PangoFontDescription *
pango_win32_font_description_from_logfontw (const LOGFONTW *lfp)
{
  PangoFontDescription *description;
  gchar *family;
  PangoStyle style;
  PangoVariant variant;
  PangoWeight weight, name_weight;
  PangoStretch stretch;

  static const struct {
    const char *marker;
    int marker_len;
    int remove_len;
    PangoWeight weight;
  } weight_names[] = {
#define ENTRY(n, s) ENTRY2 (n, sizeof (#n) - 1, s)
#define ENTRY2(n, l, s) ENTRY3 (n, l, l, s)
#define ENTRY3(n, marker_len, remove_len, s) { #n, marker_len, remove_len, PANGO_WEIGHT_##s }
    ENTRY (Ultra Light, ULTRALIGHT),
    ENTRY (UltraLight, ULTRALIGHT),
    ENTRY (Light, LIGHT),
    ENTRY (Medium, NORMAL),
    ENTRY (Demi Bold, SEMIBOLD),
    ENTRY (Demi, SEMIBOLD),
    ENTRY (Ultra Bold, ULTRABOLD),
    ENTRY (Extra Bold, ULTRABOLD),
    ENTRY (SemiBold, SEMIBOLD),
    ENTRY (DemiBold, SEMIBOLD),
    ENTRY (UltraBold, ULTRABOLD),
    ENTRY (ExtraBold, ULTRABOLD),
    ENTRY (Bold, BOLD),
    ENTRY (Heavy, HEAVY),
    ENTRY (Black, HEAVY),
#undef ENTRY
#undef ENTRY2
#undef ENTRY3
  };

  static const struct {
    const char *marker;
    int marker_len;
    PangoStretch stretch;
  } stretch_names[] = {
#define ENTRY(n, s) { #n, sizeof (#n) - 1, PANGO_STRETCH_##s }
    ENTRY (Ext Condensed, EXTRA_CONDENSED),
    ENTRY (Extra Condensed, EXTRA_CONDENSED),
    ENTRY (UltraCondensed, ULTRA_CONDENSED),
    ENTRY (ExtraCondensed, EXTRA_CONDENSED),
    ENTRY (Condensed, CONDENSED),
    ENTRY (Cond, CONDENSED),
    ENTRY (Narrow, CONDENSED),
    ENTRY (Ext Expanded, EXTRA_EXPANDED),
    ENTRY (Extra Expanded, EXTRA_EXPANDED),
    ENTRY (Ultra Expanded, ULTRA_EXPANDED),
    ENTRY (ExtraExpanded, EXTRA_EXPANDED),
    ENTRY (UltraExpanded, ULTRA_EXPANDED),
    ENTRY (Expanded, EXPANDED),
#undef ENTRY
  };

  int i;
  char *p;

  family = get_family_nameW (lfp);

  if ((lfp->lfPitchAndFamily & 0xF0) == FF_ROMAN && lfp->lfItalic)
    style = PANGO_STYLE_ITALIC;
  else if (lfp->lfItalic)
    style = PANGO_STYLE_OBLIQUE;
  else
    style = PANGO_STYLE_NORMAL;

  variant = PANGO_VARIANT_NORMAL;

  /* The PangoWeight values PANGO_WEIGHT_* map exactly do Windows FW_*
   * values.  Is this on purpose? Quantize the weight to exact
   * PANGO_WEIGHT_* values. Is this a good idea?
   */
  if (lfp->lfWeight == FW_DONTCARE)
    weight = PANGO_WEIGHT_NORMAL;
  else if (lfp->lfWeight <= (FW_ULTRALIGHT + FW_LIGHT) / 2)
    weight = PANGO_WEIGHT_ULTRALIGHT;
  else if (lfp->lfWeight <= (FW_LIGHT + FW_NORMAL) / 2)
    weight = PANGO_WEIGHT_LIGHT;
  else if (lfp->lfWeight <= (FW_NORMAL + FW_SEMIBOLD) / 2)
    weight = PANGO_WEIGHT_NORMAL;
  else if (lfp->lfWeight <= (FW_SEMIBOLD + FW_BOLD) / 2)
    weight = PANGO_WEIGHT_SEMIBOLD;
  else if (lfp->lfWeight <= (FW_BOLD + FW_ULTRABOLD) / 2)
    weight = PANGO_WEIGHT_BOLD;
  else if (lfp->lfWeight <= (FW_ULTRABOLD + FW_HEAVY) / 2)
    weight = PANGO_WEIGHT_ULTRABOLD;
  else
    weight = PANGO_WEIGHT_HEAVY;

  name_weight = 0;

  p = family;
  while ((p = strchr (p, ' ')) != NULL)
    {
      for (i = 0; i < G_N_ELEMENTS (weight_names); i++)
	{
	  if (g_ascii_strncasecmp (p + 1, weight_names[i].marker, weight_names[i].marker_len) == 0 &&
	      (p[1 + weight_names[i].marker_len] == '\0' ||
	       p[1 + weight_names[i].marker_len] == ' '))
	    {
	      strcpy (p, p + 1 + weight_names[i].remove_len);
	      name_weight = weight_names[i].weight;
	      break;
	    }
	}
      if (i < G_N_ELEMENTS (weight_names))
	break;
      p++;
    }

  if (weight == PANGO_WEIGHT_NORMAL && name_weight > 0)
    weight = name_weight;

  stretch = PANGO_STRETCH_NORMAL;

  p = family;
  while ((p = strchr (p, ' ')) != NULL)
    {
      for (i = 0; i < G_N_ELEMENTS (stretch_names); i++)
	{
	  if (g_ascii_strncasecmp (p + 1, stretch_names[i].marker, stretch_names[i].marker_len) == 0 &&
	      (p[1 + stretch_names[i].marker_len] == '\0' ||
	       p[1 + stretch_names[i].marker_len] == ' '))
	    {
	      strcpy (p, p + 1 + stretch_names[i].marker_len);
	      stretch = stretch_names[i].stretch;
	      break;
	    }
	}
      if (i < G_N_ELEMENTS (stretch_names))
	break;
      p++;
    }

  description = pango_font_description_new ();
  pango_font_description_set_family (description, family);
  g_free(family);
  pango_font_description_set_style (description, style);
  pango_font_description_set_weight (description, weight);
  pango_font_description_set_stretch (description, stretch);
  pango_font_description_set_variant (description, variant);

  return description;
}

static char *
charset_name (int charset)
{
  static char num[10];

  switch (charset)
    {
#define CASE(x) case x##_CHARSET: return #x
      CASE (ANSI);
      CASE (DEFAULT);
      CASE (SYMBOL);
      CASE (SHIFTJIS);
      CASE (HANGUL);
      CASE (GB2312);
      CASE (CHINESEBIG5);
      CASE (GREEK);
      CASE (TURKISH);
      CASE (HEBREW);
      CASE (ARABIC);
      CASE (BALTIC);
      CASE (RUSSIAN);
      CASE (THAI);
      CASE (EASTEUROPE);
      CASE (OEM);
      CASE (JOHAB);
      CASE (VIETNAMESE);
      CASE (MAC);
#undef CASE
    default:
      sprintf (num, "%d", charset);
      return num;
    }
}

static char *
ff_name (int ff)
{
  static char num[10];

  switch (ff)
    {
#define CASE(x) case FF_##x: return #x
      CASE (DECORATIVE);
      CASE (DONTCARE);
      CASE (MODERN);
      CASE (ROMAN);
      CASE (SCRIPT);
      CASE (SWISS);
#undef CASE
    default:
      sprintf (num, "%d", ff);
      return num;
    }
}

static void
pango_win32_insert_font (PangoWin32FontMap *win32fontmap,
			 LOGFONTW          *lfp,
			 gboolean	    is_synthetic)
{
  LOGFONTW *lfp2 = NULL;
  PangoFontDescription *description;
  PangoWin32Family *win32family;
  PangoWin32Face *win32face;
  gint i;

  PING (("face=%S,charset=%s,it=%s,wt=%ld,ht=%ld,ff=%s%s",
	 lfp->lfFaceName,
	 charset_name (lfp->lfCharSet),
	 lfp->lfItalic ? "yes" : "no",
	 lfp->lfWeight,
	 lfp->lfHeight,
	 ff_name (lfp->lfPitchAndFamily & 0xF0),
	 is_synthetic ? " synthetic" : ""));

  /* Ignore Symbol fonts (which don't have any Unicode mapping
   * table). We could also be fancy and use the PostScript glyph name
   * table for such if present, and build a Unicode map by mapping
   * each PostScript glyph name to Unicode character. Oh well.
   */
  if (lfp->lfCharSet == SYMBOL_CHARSET)
    return;

  if (g_hash_table_lookup (win32fontmap->fonts, lfp))
    {
      PING (("already have it"));
      return;
    }

  PING (("not found"));
  lfp2 = g_new (LOGFONTW, 1);
  *lfp2 = *lfp;
  g_hash_table_insert (win32fontmap->fonts, lfp2, lfp2);

  description = pango_win32_font_description_from_logfontw (lfp2);

  /* In some cases, extracting a name for a font can fail; such fonts
   * aren't usable for us
   */
  if (!pango_font_description_get_family (description))
    {
      pango_font_description_free (description);
      return;
    }

  win32face = g_object_new (PANGO_WIN32_TYPE_FACE, NULL);

  PING (("win32face created: %p for %S", win32face, lfp->lfFaceName));

  win32face->logfontw = *lfp;
  win32face->description = description;

  for (i = 0; i < PANGO_WIN32_N_COVERAGES; i++)
     win32face->coverages[i] = NULL;

  win32face->is_synthetic = is_synthetic;

  win32face->has_cmap = TRUE;
  win32face->cmap_format = 0;
  win32face->cmap = NULL;

  win32face->cached_fonts = NULL;

  win32family =
    pango_win32_get_font_family (win32fontmap,
				 pango_font_description_get_family (win32face->description));
  if ((lfp->lfPitchAndFamily & 0xF0) == FF_MODERN)
    win32family->is_monospace = TRUE;
    
  win32family->faces = g_slist_append (win32family->faces, win32face);

  PING (("name=%s, length(faces)=%d",
	 win32family->family_name, g_slist_length (win32family->faces)));
}

/* Given a LOGFONTW and size, make a matching LOGFONTW corresponding to
 * an installed font.
 */
void
_pango_win32_make_matching_logfontw (PangoFontMap   *fontmap,
				     const LOGFONTW *lfp,
				     int             size,
				     LOGFONTW       *out)
{
  PangoWin32FontMap *win32fontmap;
  LOGFONTW *match;

  PING (("lfp.face=%S,wt=%ld,ht=%ld,size:%d",
	 lfp->lfFaceName, lfp->lfWeight, lfp->lfHeight, size));
  win32fontmap = PANGO_WIN32_FONT_MAP (fontmap);

  match = g_hash_table_lookup (win32fontmap->fonts, lfp);

  if (!match)
    {
      PING (("not found"));
      return;
    }

  /* OK, we have a match; let's modify it to fit this size */

  *out = *match;
  out->lfHeight = -(int)((double)size / win32fontmap->resolution + 0.5);
  out->lfWidth = 0;
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

static gboolean
pango_win32_face_is_synthesized (PangoFontFace *face)
{
  PangoWin32Face *win32face = PANGO_WIN32_FACE (face);

  return win32face->is_synthetic;
}

static void
pango_win32_face_class_init (PangoFontFaceClass *class)
{
  class->describe = pango_win32_face_describe;
  class->get_face_name = pango_win32_face_get_face_name;
  class->list_sizes = pango_win32_face_list_sizes;
  class->is_synthesized = pango_win32_face_is_synthesized;
}

static void
pango_win32_face_list_sizes (PangoFontFace  *face,
			     int           **sizes,
			     int            *n_sizes)
{
  /*
   * for scalable fonts it's simple, and currently we only have such
   * see : pango_win32_enum_proc(), TRUETYPE_FONTTYPE
   */
  *sizes = NULL;
  *n_sizes = 0;
}

static GType
pango_win32_face_get_type (void)
{
  static GType object_type = 0;

  if (G_UNLIKELY (!object_type))
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
                                            I_("PangoWin32Face"),
                                            &object_info, 0);
    }
  
  return object_type;
}

/**
 * pango_win32_font_map_get_font_cache:
 * @font_map: a #PangoWin32FontMap.
 *
 * Obtains the font cache associated with the given font map.
 *
 * Return value: the #PangoWin32FontCache of @font_map.
 **/
PangoWin32FontCache *
pango_win32_font_map_get_font_cache (PangoFontMap *font_map)
{
  g_return_val_if_fail (font_map != NULL, NULL);
  g_return_val_if_fail (PANGO_WIN32_IS_FONT_MAP (font_map), NULL);

  return PANGO_WIN32_FONT_MAP (font_map)->font_cache;
}

void
_pango_win32_fontmap_cache_remove (PangoFontMap   *fontmap,
				   PangoWin32Font *win32font)
{
  PangoWin32FontMap *win32fontmap = PANGO_WIN32_FONT_MAP (fontmap);
  GList *link = g_queue_find (win32fontmap->freed_fonts, win32font);

  if (link)
    g_queue_delete_link (win32fontmap->freed_fonts, link);
  win32font->in_cache = FALSE;

  g_object_unref (win32font);
}

static void
pango_win32_fontmap_cache_clear (PangoWin32FontMap *win32fontmap)
{
  g_list_foreach (win32fontmap->freed_fonts->head, (GFunc)g_object_unref, NULL);
  g_queue_free (win32fontmap->freed_fonts);
  win32fontmap->freed_fonts = g_queue_new ();
}
