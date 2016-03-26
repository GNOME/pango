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
#include <glib/gstdio.h>

#include "pango-fontmap.h"
#include "pango-impl-utils.h"
#include "pangowin32-private.h"

typedef struct _PangoWin32Family PangoWin32Family;
typedef PangoFontFamilyClass PangoWin32FamilyClass;

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
#define PANGO_WIN32_IS_FAMILY (object)       (G_TYPE_CHECK_INSTANCE_TYPE ((object), PANGO_WIN32_TYPE_FAMILY))
#define PANGO_WIN32_FAMILY_CLASS (klass)     (G_TYPE_CHECK_CLASS_CAST (klass), PANGO_WIN32_TYPE_FAMILY, PangoWin32Family))
#define PANGO_WIN32_IS_FAMILY_CLASS (klass)  (G_TYPE_CHECK_CLASS_CAST (klass), PANGO_WIN32_TYPE_FAMILY))
#define PANGO_WIN32_FAMILY_GET_CLASS(object) (G_TYPE_INSTANCE_GET_CLASS((object), PANGO_WIN32_FAMILY, PangoWin32FamilyClass))

#define PANGO_WIN32_TYPE_FACE              (pango_win32_face_get_type ())
#define PANGO_WIN32_FACE(object)           (G_TYPE_CHECK_INSTANCE_CAST ((object), PANGO_WIN32_TYPE_FACE, PangoWin32Face))
#define PANGO_WIN32_IS_FACE(object)        (G_TYPE_CHECK_INSTANCE_TYPE ((object), PANGO_WIN32_TYPE_FACE))
#define PANGO_WIN32_FACE_CLASS (klass)     (G_TYPE_CHECK_CLASS_CAST (klass), PANGO_WIN32_TYPE_FACE, PangoWin32Face))
#define PANGO_WIN32_IS_FACE_CLASS (klass)  (G_TYPE_CHECK_CLASS_CAST (klass), PANGO_WIN32_TYPE_FACE))
#define PANGO_WIN32_FACE_GET_CLASS(object) (G_TYPE_INSTANCE_GET_CLASS((object), PANGO_WIN32_FACE, PangoWin32FaceClass))

static GType      pango_win32_face_get_type          (void);

static GType      pango_win32_family_get_type        (void);

static void       pango_win32_face_list_sizes        (PangoFontFace  *face,
						      int           **sizes,
						      int            *n_sizes);

static void       pango_win32_font_map_finalize      (GObject                      *object);
static PangoFont *pango_win32_font_map_load_font     (PangoFontMap                 *fontmap,
						      PangoContext                 *context,
						      const PangoFontDescription   *description);
static PangoFontset *pango_win32_font_map_load_fontset (PangoFontMap               *fontmap,
						      PangoContext                 *context,
						      const PangoFontDescription   *desc,
						      PangoLanguage                *language);
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

static PangoWin32FontMap *default_fontmap = NULL; /* MT-safe */

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

struct PangoAlias
{
  char *alias;
  int n_families;
  char **families;
  gboolean visible; /* Do we want/need this? */
};

static guint
alias_hash (struct PangoAlias *alias)
{
  return g_str_hash (alias->alias);
}

static gboolean
alias_equal (struct PangoAlias *alias1,
             struct PangoAlias *alias2)
{
  return g_str_equal (alias1->alias,
                      alias2->alias);
}

static void
alias_free (struct PangoAlias *alias)
{
  int i;
  g_free (alias->alias);

  for (i = 0; i < alias->n_families; i++)
    g_free (alias->families[i]);

  g_free (alias->families);

  g_slice_free (struct PangoAlias, alias);
}

static void
handle_alias_line (GString    *line_buffer,
                   char       **errstring,
                   GHashTable *ht_aliases)
{
  GString *tmp_buffer1;
  GString *tmp_buffer2;
  const char *pos;
  struct PangoAlias alias_key;
  struct PangoAlias *alias;
  gboolean append = FALSE;
  char **new_families;
  int n_new;
  int i;

  tmp_buffer1 = g_string_new (NULL);
  tmp_buffer2 = g_string_new (NULL);

  pos = line_buffer->str;
  if (!pango_skip_space (&pos))
    return;

  if (!pango_scan_string (&pos, tmp_buffer1) ||
      !pango_skip_space (&pos))
    {
      *errstring = g_strdup ("Line is not of the form KEY=VALUE or KEY+=VALUE");
      goto error;
    }

  if (*pos == '+')
    {
      append = TRUE;
      pos++;
    }

  if (*(pos++) != '=')
    {
      *errstring = g_strdup ("Line is not of the form KEY=VALUE or KEY+=VALUE");
      goto error;
    }

  if (!pango_scan_string (&pos, tmp_buffer2))
    {
      *errstring = g_strdup ("Error parsing value string");
      goto error;
    }
  if (pango_skip_space (&pos))
    {
      *errstring = g_strdup ("Junk after value string");
      goto error;
    }

  alias_key.alias = g_ascii_strdown (tmp_buffer1->str, -1);

  /* Remove any existing values */
  alias = g_hash_table_lookup (ht_aliases, &alias_key);

  if (!alias)
    {
      alias = g_slice_new0 (struct PangoAlias);
      alias->alias = alias_key.alias;

      g_hash_table_insert (ht_aliases, alias, alias);
    }
  else
    g_free (alias_key.alias);

  new_families = g_strsplit (tmp_buffer2->str, ",", -1);

  n_new = 0;
  while (new_families[n_new])
    n_new++;

  if (alias->families && append)
    {
      alias->families = g_realloc (alias->families,
                                   sizeof (char *) *(n_new + alias->n_families));
      for (i = 0; i < n_new; i++)
        alias->families[alias->n_families + i] = new_families[i];
      g_free (new_families);
      alias->n_families += n_new;
    }
  else
    {
      for (i = 0; i < alias->n_families; i++)
        g_free (alias->families[i]);
      g_free (alias->families);
      
      alias->families = new_families;
      alias->n_families = n_new;
    }

 error:
  
  g_string_free (tmp_buffer1, TRUE);
  g_string_free (tmp_buffer2, TRUE);
}

#ifdef HAVE_CAIRO_WIN32

static const char * const builtin_aliases[] = {
  "courier = \"courier new\"",
  "\"segoe ui\" = \"segoe ui,meiryo,malgun gothic,microsoft jhenghei,microsoft yahei,gisha,leelawadee,arial unicode ms,browallia new,mingliu,simhei,gulimche,ms gothic,sylfaen,kartika,latha,mangal,raavi\"",
  "tahoma = \"tahoma,arial unicode ms,lucida sans unicode,browallia new,mingliu,simhei,gulimche,ms gothic,sylfaen,kartika,latha,mangal,raavi\"",
  /* It sucks to use the same GulimChe, MS Gothic, Sylfaen, Kartika,
   * Latha, Mangal and Raavi fonts for all three of sans, serif and
   * mono, but it isn't like there would be much choice. For most
   * non-Latin scripts that Windows includes any font at all for, it
   * has ony one. One solution is to install the free DejaVu fonts
   * that are popular on Linux. They are listed here first.
   */
  "sans = \"dejavu sans,tahoma,arial unicode ms,lucida sans unicode,browallia new,mingliu,simhei,gulimche,ms gothic,sylfaen,kartika,latha,mangal,raavi\"",
  "sans-serif = \"dejavu sans,tahoma,arial unicode ms,lucida sans unicode,browallia new,mingliu,simhei,gulimche,ms gothic,sylfaen,kartika,latha,mangal,raavi\"",
  "serif = \"dejavu serif,georgia,angsana new,mingliu,simsun,gulimche,ms gothic,sylfaen,kartika,latha,mangal,raavi\"",
 "mono = \"dejavu sans mono,courier new,lucida console,courier monothai,mingliu,simsun,gulimche,ms gothic,sylfaen,kartika,latha,mangal,raavi\"",
  "monospace = \"dejavu sans mono,courier new,lucida console,courier monothai,mingliu,simsun,gulimche,ms gothic,sylfaen,kartika,latha,mangal,raavi\""
};

static void
read_builtin_aliases (GHashTable *ht_aliases)
{

  GString *line_buffer;
  char *errstring = NULL;
  int line;

  line_buffer = g_string_new (NULL);

  for (line = 0; line < G_N_ELEMENTS (builtin_aliases) && errstring == NULL; line++)
    {
      g_string_assign (line_buffer, builtin_aliases[line]);
      handle_alias_line (line_buffer, &errstring, ht_aliases);
    }

  if (errstring)
    {
      g_error ("error in built-in aliases:%d: %s\n", line, errstring);
      g_free (errstring);
    }

  g_string_free (line_buffer, TRUE);
}
#endif


static GHashTable *
load_aliases (void)
{
  GHashTable *ht_aliases = g_hash_table_new_full ((GHashFunc)alias_hash,
                                                  (GEqualFunc)alias_equal,
                                                  (GDestroyNotify)alias_free,
                                                  NULL);

#ifdef HAVE_CAIRO_WIN32
  read_builtin_aliases (ht_aliases);
#endif

  return ht_aliases;
}

static void
lookup_aliases (const char   *fontname,
                char       ***families,
                int          *n_families)
{
  static GHashTable *aliases_ht = NULL; /* MT-safe */

  struct PangoAlias alias_key;
  struct PangoAlias *alias;

  if (g_once_init_enter (&aliases_ht))
    {
      g_once_init_leave (&aliases_ht, load_aliases ());
    }

  alias_key.alias = g_ascii_strdown (fontname, -1);
  alias = g_hash_table_lookup (aliases_ht, &alias_key);
  g_free (alias_key.alias);

  if (alias)
    {
      *families = alias->families;
      *n_families = alias->n_families;
    }
  else
    {
      *families = NULL;
      *n_families = 0;
    }
}

static void
create_standard_family (PangoWin32FontMap *win32fontmap,
			const char        *standard_family_name)
{
  int i;
  int n_aliases;
  char **aliases;

  lookup_aliases (standard_family_name, &aliases, &n_aliases);
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

	      new_face->face_name = NULL;

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

  win32fontmap->families =
    g_hash_table_new_full ((GHashFunc) case_insensitive_str_hash,
                           (GEqualFunc) case_insensitive_str_equal, NULL, g_object_unref);
  win32fontmap->fonts =
    g_hash_table_new_full ((GHashFunc) logfontw_nosize_hash,
                           (GEqualFunc) logfontw_nosize_equal, NULL, g_free);

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
pango_win32_font_map_fontset_add_fonts (PangoFontMap          *fontmap,
				  PangoContext          *context,
				  PangoFontsetSimple *fonts,
				  PangoFontDescription  *desc,
				  const char            *family)
{
  /* Mostly use the "old" pango_font_map_fontset_add_fonts() */
  /* on Windows so that we can go through the .aliases file */
  /* to load the appropriate fontset for various texts */
  PangoFont *font;
  char **aliases;
  int n_aliases;
  int j;

  lookup_aliases (family, &aliases, &n_aliases);

  if (n_aliases)
  {
    for (j = 0; j < n_aliases; j++)
    {
      pango_font_description_set_family_static (desc, aliases[j]);
      font = pango_win32_font_map_load_font (fontmap, context, desc);
      if (font)
        pango_fontset_simple_append (fonts, font);
    }
  }
  else
  {
    pango_font_description_set_family_static (desc, family);
    font = pango_win32_font_map_load_font (fontmap, context, desc);
    if (font)
      pango_fontset_simple_append (fonts, font);
  }
}

static void
_pango_win32_font_map_class_init (PangoWin32FontMapClass *class)
{
  GObjectClass *object_class = G_OBJECT_CLASS (class);
  PangoFontMapClass *fontmap_class = PANGO_FONT_MAP_CLASS (class);

  class->find_font = pango_win32_font_map_real_find_font;
  object_class->finalize = pango_win32_font_map_finalize;
  fontmap_class->load_font = pango_win32_font_map_load_font;
  /* we now need a load_fontset implementation for the Win32 backend */
  fontmap_class->load_fontset = pango_win32_font_map_load_fontset;
  fontmap_class->list_families = pango_win32_font_map_list_families;
  fontmap_class->shape_engine_type = PANGO_RENDER_TYPE_WIN32;

  pango_win32_get_dc ();
}

/**
 * pango_win32_font_map_for_display:
 *
 * Returns a <type>PangoWin32FontMap</type>. Font maps are cached and should
 * not be freed. If the font map is no longer needed, it can
 * be released with pango_win32_shutdown_display().
 *
 * Return value: a #PangoFontMap.
 **/
PangoFontMap *
pango_win32_font_map_for_display (void)
{
#if !GLIB_CHECK_VERSION (2, 35, 3)
  /* Make sure that the type system is initialized */
  g_type_init ();
#endif

  if (g_once_init_enter ((gsize*)&default_fontmap))
    g_once_init_leave((gsize*)&default_fontmap, (gsize)g_object_new (PANGO_TYPE_WIN32_FONT_MAP, NULL));

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

  g_hash_table_destroy (win32fontmap->fonts);

  g_hash_table_destroy (win32fontmap->families);

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

G_DEFINE_TYPE (PangoWin32Family, pango_win32_family, PANGO_TYPE_FONT_FAMILY)

static void
pango_win32_family_finalize (GObject *object)
{
  PangoWin32Family *win32family = PANGO_WIN32_FAMILY (object);

  g_free (win32family->family_name);

  g_slist_free_full (win32family->faces, g_object_unref);

  G_OBJECT_CLASS (pango_win32_family_parent_class)->finalize (object);
}

static void
pango_win32_family_class_init (PangoFontFamilyClass *class)
{
  GObjectClass *object_class = G_OBJECT_CLASS (class);

  object_class->finalize = pango_win32_family_finalize;
  class->list_faces = pango_win32_family_list_faces;
  class->get_name = pango_win32_family_get_name;
  class->is_monospace = pango_win32_family_is_monospace;
}

static void
pango_win32_family_init (PangoWin32Family *family)
{
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
  const char *family;

  g_return_val_if_fail (description != NULL, NULL);

  family = pango_font_description_get_family (description);
  family = family ? family : "";
  PING (("name=%s", family));

  win32family = g_hash_table_lookup (win32fontmap->families, family);
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

  if (G_UNLIKELY(result->fontmap))
    return result;
  g_weak_ref_set ((GWeakRef *)&result->fontmap, fontmap);

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

  if (lfp->lfWeight == FW_DONTCARE)
    weight = PANGO_WEIGHT_NORMAL;
  else
    /* The PangoWeight values PANGO_WEIGHT_* map exactly to Windows FW_*. */
    weight = (PangoWeight) lfp->lfWeight;

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
  PangoWeight weight;
  PangoStretch stretch;

  family = get_family_nameW (lfp);

  if ((lfp->lfPitchAndFamily & 0xF0) == FF_ROMAN && lfp->lfItalic)
    style = PANGO_STYLE_ITALIC;
  else if (lfp->lfItalic)
    style = PANGO_STYLE_OBLIQUE;
  else
    style = PANGO_STYLE_NORMAL;

  variant = PANGO_VARIANT_NORMAL;

  if (lfp->lfWeight == FW_DONTCARE)
    weight = PANGO_WEIGHT_NORMAL;
  else
    /* The PangoWeight values PANGO_WEIGHT_* map exactly to Windows FW_*. */
    weight = (PangoWeight) lfp->lfWeight;

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

static char *
charset_name (int charset, char* num)
{
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
ff_name (int ff, char* num)
{
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

  char tmp_for_charset_name[10];
  char tmp_for_ff_name[10];

  PING (("face=%S,charset=%s,it=%s,wt=%ld,ht=%ld,ff=%s%s",
	 lfp->lfFaceName,
	 charset_name (lfp->lfCharSet, tmp_for_charset_name),
	 lfp->lfItalic ? "yes" : "no",
	 lfp->lfWeight,
	 lfp->lfHeight,
	 ff_name (lfp->lfPitchAndFamily & 0xF0, tmp_for_ff_name),
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

  win32face->face_name = NULL;

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

G_DEFINE_TYPE (PangoWin32Face, pango_win32_face, PANGO_TYPE_FONT_FACE)

static void
pango_win32_face_finalize (GObject *object)
{
  int j;
  PangoWin32Face *win32face = PANGO_WIN32_FACE (object);

  pango_font_description_free (win32face->description);

  for (j = 0; j < PANGO_WIN32_N_COVERAGES; j++)
    if (win32face->coverages[j] != NULL)
      pango_coverage_unref (win32face->coverages[j]);

  g_free (win32face->face_name);

  //g_free (win32face->cmap); // Err, cmap does not have lifecycle management currently :(

  g_slist_free (win32face->cached_fonts);
//  g_slist_free_full (win32face->cached_fonts, g_object_unref); // This doesn't work.

  G_OBJECT_CLASS (pango_win32_family_parent_class)->finalize (object);
}

static void
pango_win32_face_class_init (PangoFontFaceClass *class)
{
  GObjectClass *object_class = G_OBJECT_CLASS (class);

  object_class->finalize = pango_win32_face_finalize;
  class->describe = pango_win32_face_describe;
  class->get_face_name = pango_win32_face_get_face_name;
  class->list_sizes = pango_win32_face_list_sizes;
  class->is_synthesized = pango_win32_face_is_synthesized;
}

static void
pango_win32_face_init (PangoWin32Face *face)
{
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

/**
 * pango_win32_font_map_get_font_cache:
 * @font_map: a <type>PangoWin32FontMap</type>.
 *
 * Obtains the font cache associated with the given font map.
 *
 * Return value: the #PangoWin32FontCache of @font_map.
 **/
PangoWin32FontCache *
pango_win32_font_map_get_font_cache (PangoFontMap *font_map)
{
  if (G_UNLIKELY (!font_map))
    return NULL;

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

static PangoFontset *
pango_win32_font_map_load_fontset (PangoFontMap                 *fontmap,
				PangoContext                 *context,
				const PangoFontDescription   *desc,
				PangoLanguage                *language)
{
  /* This "adds" a load_fontset() for the Win32 backend */
  /* which is needed to make sure we use an appropriate */
  /* font for various texts when we are on Windows */
  /* (Copied directly from pango-fontmap.c) */
  PangoFontDescription *tmp_desc = pango_font_description_copy_static (desc);
  const char *family;
  char **families;
  int i;
  PangoFontsetSimple *fonts;
  static GHashTable *warned_fonts = NULL; /* MT-safe */
  G_LOCK_DEFINE_STATIC (warned_fonts);

  g_return_val_if_fail (fontmap != NULL, NULL);

  family = pango_font_description_get_family (desc);
  families = g_strsplit (family ? family : "", ",", -1);

  fonts = pango_fontset_simple_new (language);

  for (i = 0; families[i]; i++)
    pango_win32_font_map_fontset_add_fonts (fontmap,
				      context,
				      fonts,
				      tmp_desc,
				      families[i]);

  g_strfreev (families);

  /* The font description was completely unloadable, try with
   * family == "Sans"
   */
  if (pango_fontset_simple_size (fonts) == 0)
    {
      char *ctmp1, *ctmp2;

      pango_font_description_set_family_static (tmp_desc,
						pango_font_description_get_family (desc));

      ctmp1 = pango_font_description_to_string (desc);
      pango_font_description_set_family_static (tmp_desc, "Sans");

      G_LOCK (warned_fonts);
      if (!warned_fonts || !g_hash_table_lookup (warned_fonts, ctmp1))
	{
	  if (!warned_fonts)
	    warned_fonts = g_hash_table_new (g_str_hash, g_str_equal);

	  g_hash_table_insert (warned_fonts, g_strdup (ctmp1), GINT_TO_POINTER (1));

	  ctmp2 = pango_font_description_to_string (tmp_desc);
	  g_warning ("couldn't load font \"%s\", falling back to \"%s\", "
		     "expect ugly output.", ctmp1, ctmp2);
	  g_free (ctmp2);
	}
      G_UNLOCK (warned_fonts);
      g_free (ctmp1);

      pango_win32_font_map_fontset_add_fonts (fontmap,
					context,
					fonts,
					tmp_desc,
					"Sans");
    }

  /* We couldn't try with Sans and the specified style. Try Sans Normal
   */
  if (pango_fontset_simple_size (fonts) == 0)
    {
      char *ctmp1, *ctmp2;

      pango_font_description_set_family_static (tmp_desc, "Sans");
      ctmp1 = pango_font_description_to_string (tmp_desc);
      pango_font_description_set_style (tmp_desc, PANGO_STYLE_NORMAL);
      pango_font_description_set_weight (tmp_desc, PANGO_WEIGHT_NORMAL);
      pango_font_description_set_variant (tmp_desc, PANGO_VARIANT_NORMAL);
      pango_font_description_set_stretch (tmp_desc, PANGO_STRETCH_NORMAL);

      G_LOCK (warned_fonts);
      if (!warned_fonts || !g_hash_table_lookup (warned_fonts, ctmp1))
	{
	  g_hash_table_insert (warned_fonts, g_strdup (ctmp1), GINT_TO_POINTER (1));

	  ctmp2 = pango_font_description_to_string (tmp_desc);

	  g_warning ("couldn't load font \"%s\", falling back to \"%s\", "
		     "expect ugly output.", ctmp1, ctmp2);
	  g_free (ctmp2);
	}
      G_UNLOCK (warned_fonts);
      g_free (ctmp1);

      pango_win32_font_map_fontset_add_fonts (fontmap,
					context,
					fonts,
					tmp_desc,
					"Sans");
    }

  pango_font_description_free (tmp_desc);

  /* Everything failed, we are screwed, there is no way to continue,
   * but lets just not crash here.
   */
  if (pango_fontset_simple_size (fonts) == 0)
      g_warning ("All font fallbacks failed!!!!");

  return PANGO_FONTSET (fonts);
}
