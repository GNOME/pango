/* Pango
 * gscriptx.c: Routines for handling X fonts
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

#include <X11/Xlib.h>
#include <fribidi/fribidi.h>
#include <unicode.h>
#include "modules.h"
#include "pangox.h"
#include "utils.h"
#include <ctype.h>
#include <math.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <errno.h>

#include <config.h>

typedef struct _PangoXFont PangoXFont;
typedef struct _PangoXFontMap PangoXFontMap;
typedef struct _PangoXSubfontInfo PangoXSubfontInfo;

typedef struct _PangoXSizeInfo PangoXSizeInfo;
typedef struct _PangoXMetricsInfo PangoXMetricsInfo;
typedef struct _PangoXFamilyEntry PangoXFamilyEntry;
typedef struct _PangoXFontEntry PangoXFontEntry;

typedef struct _PangoXContextInfo PangoXContextInfo;

struct _PangoXSizeInfo
{
  char *identifier;
  GSList *xlfds;
};

struct _PangoXFontEntry
{
  char *xlfd;
  PangoFontDescription description;
  PangoCoverage *coverage;

  GSList *cached_fonts;
};

struct _PangoXFamilyEntry
{
  char *family_name;
  GSList *font_entries;
};

struct _PangoXSubfontInfo
{
  char *xlfd;
  XFontStruct *font_struct;
};

struct _PangoXMetricsInfo
{
  const char *lang;
  PangoFontMetrics metrics;
};

struct _PangoXFont
{
  PangoFont font;
  Display *display;

  char **fonts;
  int n_fonts;
  int size;

  /* hash table mapping from charset-name to array of PangoXSubfont ids,
   * of length n_fonts
   */
  GHashTable *subfonts_by_charset;
  
  PangoXSubfontInfo **subfonts;

  int n_subfonts;
  int max_subfonts;

  GSList *metrics_by_lang;
  
  PangoXFontEntry *entry;	/* Used to remove cached fonts */
};

struct _PangoXFontMap
{
  PangoFontMap fontmap;

  Display *display;

  GHashTable *families;
  GHashTable *size_infos;

  int n_fonts;

  double resolution;		/* (points / pixel) * PANGO_SCALE */
};

struct _PangoXContextInfo
{
  PangoGetGCFunc  get_gc_func;
  PangoFreeGCFunc free_gc_func;
};

/* This is the largest field length we will accept. If a fontname has a field
   larger than this we will skip it. */
#define XLFD_MAX_FIELD_LEN 64
#define MAX_FONTS 32767

/* These are the field numbers in the X Logical Font Description fontnames,
   e.g. -adobe-courier-bold-o-normal--25-180-100-100-m-150-iso8859-1 */
typedef enum
{
  XLFD_FOUNDRY		= 0,
  XLFD_FAMILY		= 1,
  XLFD_WEIGHT		= 2,
  XLFD_SLANT		= 3,
  XLFD_SET_WIDTH	= 4,
  XLFD_ADD_STYLE	= 5,
  XLFD_PIXELS		= 6,
  XLFD_POINTS		= 7,
  XLFD_RESOLUTION_X	= 8,
  XLFD_RESOLUTION_Y	= 9,
  XLFD_SPACING		= 10,
  XLFD_AVERAGE_WIDTH	= 11,
  XLFD_CHARSET		= 12,
  XLFD_NUM_FIELDS     
} FontField;

const struct {
  const gchar *text;
  PangoWeight value;
} weights_map[] = {
  { "light",     300 },
  { "regular",   400 },
  { "book",      400 },
  { "medium",    500 },
  { "semibold",  600 },
  { "demibold",  600 },
  { "bold",      700 },
  { "extrabold", 800 },
  { "ultrabold", 800 },
  { "heavy",     900 },
  { "black",     900 }
};

const struct {
  const gchar *text;
  PangoStyle value;
} styles_map[] = {
  { "r", PANGO_STYLE_NORMAL },
  { "i", PANGO_STYLE_ITALIC },
  { "o", PANGO_STYLE_OBLIQUE }
};

const struct {
  const gchar *text;
  PangoStretch value;
} stretches_map[] = {
  { "normal",        PANGO_STRETCH_NORMAL },
  { "semicondensed", PANGO_STRETCH_SEMI_CONDENSED },
  { "condensed",     PANGO_STRETCH_CONDENSED },
};

static void       pango_x_font_map_destroy       (PangoFontMap                 *fontmap);
static PangoFont *pango_x_font_map_load_font     (PangoFontMap                 *fontmap,
						  const PangoFontDescription   *desc);
static void       pango_x_font_map_list_fonts    (PangoFontMap                 *fontmap,
						  const gchar                  *family,
						  PangoFontDescription       ***descs,
						  int                          *n_descs);
static void       pango_x_font_map_list_families (PangoFontMap                 *fontmap,
						  gchar                      ***families,
						  int                          *n_families);
static void       pango_x_font_map_read_aliases  (PangoXFontMap                *xfontmap);

static void                  pango_x_font_destroy           (PangoFont        *font);
static PangoFontDescription *pango_x_font_describe          (PangoFont        *font);
static PangoCoverage *       pango_x_font_get_coverage      (PangoFont        *font,
							     const char       *lang);
static PangoEngineShape *    pango_x_font_find_shaper       (PangoFont        *font,
							     const char       *lang,
							     guint32           ch);
static void                  pango_x_font_get_glyph_extents (PangoFont        *font,
							     PangoGlyph        glyph,
							     PangoRectangle   *ink_rect,
							     PangoRectangle   *logical_rect);
static void                  pango_x_font_get_metrics       (PangoFont        *font,
							     const gchar      *lang,
							     PangoFontMetrics *metrics);

static PangoXSubfontInfo * pango_x_find_subfont    (PangoFont          *font,
						    PangoXSubfont       subfont_index);
static XCharStruct *       pango_x_get_per_char    (PangoFont          *font,
						    PangoXSubfontInfo  *subfont,
						    guint16             char_index);
static gboolean            pango_x_find_glyph      (PangoFont          *font,
						    PangoGlyph          glyph,
						    PangoXSubfontInfo **subfont_return,
						    XCharStruct       **charstruct_return);
static XFontStruct *       pango_x_get_font_struct (PangoFont          *font,
						    PangoXSubfontInfo  *info);

static PangoFont *pango_x_load_font_with_size (Display *display,
					       char    *spec,
					       int      size);

static gboolean pango_x_is_xlfd_font_name   (const char     *fontname);
static char *   pango_x_get_xlfd_field      (const char     *fontname,
					     FontField       field_num,
					     char           *buffer);
static char *   pango_x_get_identifier      (const char     *fontname);
static gint     pango_x_get_size            (PangoXFontMap  *fontmap,
					     const char     *fontname);
static void     pango_x_insert_font         (PangoXFontMap  *fontmap,
					     const char     *fontname);
static void     pango_x_get_item_properties (PangoItem      *item,
					     PangoUnderline *uline,
					     PangoAttrColor *fg_color,
					     gboolean       *fg_set,
					     PangoAttrColor *bg_color,
					     gboolean       *bg_set);

static GList *fontmaps;

PangoFontClass pango_x_font_class = {
  pango_x_font_destroy,
  pango_x_font_describe,
  pango_x_font_get_coverage,
  pango_x_font_find_shaper,
  pango_x_font_get_glyph_extents,
  pango_x_font_get_metrics
};

PangoFontMapClass pango_x_font_map_class = {
  pango_x_font_map_destroy,
  pango_x_font_map_load_font,
  pango_x_font_map_list_fonts,
  pango_x_font_map_list_families
};

static PangoFontMap *
pango_x_font_map_for_display (Display *display)
{
  PangoXFontMap *xfontmap;
  GList *tmp_list = fontmaps;
  char **xfontnames;
  int num_fonts, i;
  int screen;

  while (tmp_list)
    {
      xfontmap = tmp_list->data;

      if (xfontmap->display == display)
	{
	  pango_font_map_ref ((PangoFontMap *)xfontmap);
	  return (PangoFontMap *)xfontmap;
	}
    }

  xfontmap = g_new (PangoXFontMap, 1);

  xfontmap->fontmap.klass = &pango_x_font_map_class;
  xfontmap->display = display;
  xfontmap->families = g_hash_table_new (g_str_hash, g_str_equal);
  xfontmap->size_infos = g_hash_table_new (g_str_hash, g_str_equal);
  xfontmap->n_fonts = 0;

  pango_font_map_init ((PangoFontMap *)xfontmap);

  /* Get a maximum of MAX_FONTS fontnames from the X server.
     Use "-*" as the pattern rather than "-*-*-*-*-*-*-*-*-*-*-*-*-*-*" since
     the latter may result in fonts being returned which don't actually exist.
     xlsfonts also uses "*" so I think it's OK. "-*" gets rid of aliases. */
  xfontnames = XListFonts (xfontmap->display, "-*", MAX_FONTS, &num_fonts);
  if (num_fonts == MAX_FONTS)
    g_warning("MAX_FONTS exceeded. Some fonts may be missing.");

  /* Insert the font families into the main table */
  for (i = 0; i < num_fonts; i++)
    {
      if (pango_x_is_xlfd_font_name (xfontnames[i]))
	pango_x_insert_font (xfontmap, xfontnames[i]);
    }
  
  XFreeFontNames (xfontnames);

  pango_x_font_map_read_aliases (xfontmap);

  fontmaps = g_list_prepend (fontmaps, xfontmap);

  /* This is a little screwed up, since different screens on the same display
   * might have different resolutions
   */
  screen = DefaultScreen (xfontmap->display);
  xfontmap->resolution = (PANGO_SCALE * 72.27 / 25.4) * ((double) DisplayWidthMM (xfontmap->display, screen) /
							 DisplayWidth (xfontmap->display, screen));

  return (PangoFontMap *)xfontmap;
}

static void
pango_x_font_map_destroy (PangoFontMap *fontmap)
{
  fontmaps = g_list_remove (fontmaps, fontmap);

  g_free (fontmap);
}

static PangoXFamilyEntry *
pango_x_get_family_entry (PangoXFontMap *xfontmap,
			   const char    *family_name)
{
  PangoXFamilyEntry *family_entry = g_hash_table_lookup (xfontmap->families, family_name);
  if (!family_entry)
    {
      family_entry = g_new (PangoXFamilyEntry, 1);
      family_entry->family_name = g_strdup (family_name);
      family_entry->font_entries = NULL;
      
      g_hash_table_insert (xfontmap->families, family_entry->family_name, family_entry);
    }

  return family_entry;
}

static PangoFont *
pango_x_font_map_load_font (PangoFontMap               *fontmap,
			    const PangoFontDescription *description)
{
  PangoXFontMap *xfontmap = (PangoXFontMap *)fontmap;
  PangoXFamilyEntry *family_entry;
  PangoFont *result = NULL;
  GSList *tmp_list;
  gchar *name;

  g_return_val_if_fail (description != NULL, NULL);
  g_return_val_if_fail (description->size > 0, NULL);
  
  name = g_strdup (description->family_name);
  g_strdown (name);

  family_entry = g_hash_table_lookup (xfontmap->families, name);
  if (family_entry)
    {
      PangoXFontEntry *best_match = NULL;
      
      tmp_list = family_entry->font_entries;
      while (tmp_list)
	{
	  PangoXFontEntry *font_entry = tmp_list->data;
	  
	  if (font_entry->description.style == description->style &&
	      font_entry->description.variant == description->variant &&
	      font_entry->description.stretch == description->stretch)
	    {
	      int distance = abs(font_entry->description.weight - description->weight);
	      int old_distance = best_match ? abs(best_match->description.weight - description->weight) : G_MAXINT;

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
	      PangoXFont *xfont = tmp_list->data;
	      if (xfont->size == description->size)
		{
		  result = (PangoFont *)xfont;
		  pango_font_ref (result);
		  break;
		}
	      tmp_list = tmp_list->next;
	    }

	  if (!result)
	    {
	      result = pango_x_load_font_with_size (xfontmap->display, best_match->xlfd, description->size);
	      ((PangoXFont *)result)->entry = best_match;
	      best_match->cached_fonts = g_slist_prepend (best_match->cached_fonts, result);
	    }

	  /* HORRIBLE performance hack until some better caching scheme is arrived at
	   */
	  if (result)
	    pango_font_ref (result);
	}
    }

  g_free (name);
  return result;
}

typedef struct
{
  int n_found;
  PangoFontDescription **descs;
} ListFontsInfo;

static void
list_fonts_foreach (gpointer key, gpointer value, gpointer user_data)
{
  PangoXFamilyEntry *entry = value;
  ListFontsInfo *info = user_data;

  GSList *tmp_list = entry->font_entries;

  while (tmp_list)
    {
      PangoXFontEntry *font_entry = tmp_list->data;
      
      info->descs[info->n_found++] = pango_font_description_copy (&font_entry->description);
      tmp_list = tmp_list->next;
    }
}

static void
pango_x_font_map_list_fonts (PangoFontMap           *fontmap,
			     const gchar            *family,
			     PangoFontDescription ***descs,
			     int                    *n_descs)
{
  PangoXFontMap *xfontmap = (PangoXFontMap *)fontmap;
  ListFontsInfo info;

  if (!n_descs)
    return;

  if (family)
    {
      PangoXFamilyEntry *entry = g_hash_table_lookup (xfontmap->families, family);
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
      *n_descs = xfontmap->n_fonts;
      if (descs)
	{
	  *descs = g_new (PangoFontDescription *, xfontmap->n_fonts);
	  
	  info.descs = *descs;
	  info.n_found = 0;
	  
	  g_hash_table_foreach (xfontmap->families, list_fonts_foreach, &info);
	}
    }
}

static void
list_families_foreach (gpointer key, gpointer value, gpointer user_data)
{
  GSList **list = user_data;

  *list = g_slist_prepend (*list, key);
}

static void
pango_x_font_map_list_families (PangoFontMap           *fontmap,
				gchar                ***families,
				int                    *n_families)
{
  GSList *family_list = NULL;
  GSList *tmp_list;
  PangoXFontMap *xfontmap = (PangoXFontMap *)fontmap;

  if (!n_families)
    return;

  g_hash_table_foreach (xfontmap->families, list_families_foreach, &family_list);

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

/* Similar to GNU libc's getline, but buffer is g_malloc'd */
static size_t
pango_getline (char **lineptr, size_t *n, FILE *stream)
{
#define EXPAND_CHUNK 16

  int n_read = 0;
  int result = -1;

  g_return_val_if_fail (lineptr != NULL, -1);
  g_return_val_if_fail (n != NULL, -1);
  g_return_val_if_fail (*lineptr != NULL || *n == 0, -1);
  
#ifdef HAVE_FLOCKFILE
  flockfile (stream);
#endif  
  
  while (1)
    {
      int c;
      
#ifdef HAVE_FLOCKFILE
      c = getc_unlocked (stream);
#else
      c = getc (stream);
#endif      

      if (c == EOF)
	{
	  if (n_read > 0)
	    {
	      result = n_read;
	      (*lineptr)[n_read] = '\0';
	    }
	  break;
	}

      if (n_read + 2 >= *n)
	{
	  *n += EXPAND_CHUNK;
	  *lineptr = g_realloc (*lineptr, *n);
	}

      (*lineptr)[n_read] = c;
      n_read++;

      if (c == '\n' || c == '\r')
	{
	  result = n_read;
	  (*lineptr)[n_read] = '\0';
	  break;
	}
    }

#ifdef HAVE_FLOCKFILE
  funlockfile (stream);
#endif

  return n_read - 1;
}

static int
find_tok (char **start, char **tok)
{
  char *p = *start;
  
  while (*p && (*p == ' ' || *p == '\t'))
    p++;

  if (*p == 0 || *p == '\n' || *p == '\r')
    return -1;

  if (*p == '"')
    {
      p++;
      *tok = p;

      while (*p && *p != '"')
	p++;

      if (*p != '"')
	return -1;

      *start = p + 1;
      return p - *tok;
    }
  else
    {
      *tok = p;
      
      while (*p && *p != ' ' && *p != '\t' && *p != '\r' && *p != '\n')
	p++;

      *start = p;
      return p - *tok;
    }
}

static gboolean
get_style (char *tok, int toksize, PangoFontDescription *desc)
{
  if (toksize == 0)
    return FALSE;

  switch (tok[0])
    {
    case 'n':
    case 'N':
      if (strncasecmp (tok, "normal", toksize) == 0)
	{
	  desc->style = PANGO_STYLE_NORMAL;
	  return TRUE;
	}
      break;
    case 'i':
      if (strncasecmp (tok, "italic", toksize) == 0)
	{
	  desc->style = PANGO_STYLE_ITALIC;
	  return TRUE;
	}
      break;
    case 'o':
      if (strncasecmp (tok, "oblique", toksize) == 0)
	{
	  desc->style = PANGO_STYLE_OBLIQUE;
	  return TRUE;
	}
      break;
    }
  g_warning ("Style must be normal, italic, or oblique");
  
  return FALSE;
}

static gboolean
get_variant (char *tok, int toksize, PangoFontDescription *desc)
{
  if (toksize == 0)
    return FALSE;

  switch (tok[0])
    {
    case 'n':
    case 'N':
      if (strncasecmp (tok, "normal", toksize) == 0)
	{
	  desc->variant = PANGO_VARIANT_NORMAL;
	  return TRUE;
	}
      break;
    case 's':
    case 'S':
      if (strncasecmp (tok, "small_caps", toksize) == 0)
	{
	  desc->variant = PANGO_VARIANT_SMALL_CAPS;
	  return TRUE;
	}
      break;
    }
  
  g_warning ("Variant must be normal, or small_caps");
  return FALSE;
}

static gboolean
get_weight (char *tok, int toksize, PangoFontDescription *desc)
{
  if (toksize == 0)
    return FALSE;

  switch (tok[0])
    {
    case 'n':
    case 'N':
      if (strncasecmp (tok, "normal", toksize) == 0)
	{
	  desc->weight = PANGO_WEIGHT_NORMAL;
	  return TRUE;
	}
      break;
    case 'b':
    case 'B':
      if (strncasecmp (tok, "bold", toksize) == 0)
	{
	  desc->weight = PANGO_WEIGHT_BOLD;
	  return TRUE;
	}
      break;
    case '0':
    case '1':
    case '2':
    case '3':
    case '4':
    case '5':
    case '6':
    case '7':
    case '8':
    case '9':
      {
	char *numstr, *end;

	numstr = g_strndup (tok, toksize);

	desc->weight = strtol (numstr, &end, 0);
	if (*end != '\0')
	  {
	    g_warning ("Cannot parse numerical weight '%s'", numstr);
	    g_free (numstr);
	    return FALSE;
	  }

	g_free (numstr);
	return TRUE;
      }
    }
  
  g_warning ("Weight must be normal, bold, or an integer");
  return FALSE;
}

static gboolean
get_stretch (char *tok, int toksize, PangoFontDescription *desc)
{
  if (toksize == 0)
    return FALSE;

  switch (tok[0])
    { 
    case 'c':
    case 'C':
      if (strncasecmp (tok, "condensed", toksize) == 0)
	{
	  desc->stretch = PANGO_STRETCH_CONDENSED;
	  return TRUE;
	}
      break;
    case 'e':
    case 'E':
      if (strncasecmp (tok, "extra_condensed", toksize) == 0)
	{
	  desc->stretch = PANGO_STRETCH_EXTRA_CONDENSED;
	  return TRUE;
	}
     if (strncasecmp (tok, "extra_expanded", toksize) == 0)
	{
	  desc->stretch = PANGO_STRETCH_EXTRA_EXPANDED;
	  return TRUE;
	}
      if (strncasecmp (tok, "expanded", toksize) == 0)
	{
	  desc->stretch = PANGO_STRETCH_EXPANDED;
	  return TRUE;
	}
      break;
    case 'n':
    case 'N':
      if (strncasecmp (tok, "normal", toksize) == 0)
	{
	  desc->stretch = PANGO_STRETCH_NORMAL;
	  return TRUE;
	}
      break;
    case 's':
    case 'S':
      if (strncasecmp (tok, "semi_condensed", toksize) == 0)
	{
	  desc->stretch = PANGO_STRETCH_SEMI_CONDENSED;
	  return TRUE;
	}
      if (strncasecmp (tok, "semi_expanded", toksize) == 0)
	{
	  desc->stretch = PANGO_STRETCH_SEMI_EXPANDED;
	  return TRUE;
	}
      break;
    case 'u':
    case 'U':
      if (strncasecmp (tok, "ultra_condensed", toksize) == 0)
	{
	  desc->stretch = PANGO_STRETCH_ULTRA_CONDENSED;
	  return TRUE;
	}
      if (strncasecmp (tok, "ultra_expanded", toksize) == 0)
	{
	  desc->variant = PANGO_STRETCH_ULTRA_EXPANDED;
	  return TRUE;
	}
      break;
    }

  g_warning ("Stretch must be ultra_condensed, extra_condensed, condensed, semi_condensed, normal, semi_expanded, expanded, extra_expanded, or ultra_expanded");
  return FALSE;
}

static void
pango_x_font_map_read_alias_file (PangoXFontMap *xfontmap,
				  const char   *filename)
{
  FILE *infile;
  char **xlfds;
  char *buf = NULL;
  size_t bufsize = 0;
  int lineno = 0;
  int i;
  PangoXFontEntry *font_entry = NULL;

  infile = fopen (filename, "r");
  if (infile)
    {
      while (pango_getline (&buf, &bufsize, infile) != EOF)
	{
	  PangoXFamilyEntry *family_entry;

	  char *tok;
	  char *p = buf;
	  
	  int toksize;
	  lineno++;

	  while (*p && (*p == ' ' || *p == '\t'))
	    p++;

	  if (*p == 0 || *p == '#' || *p == '\n' || *p == '\r')
	    continue;

	  toksize = find_tok (&p, &tok);
	  if (toksize == -1)
	    goto error;

	  font_entry = g_new (PangoXFontEntry, 1);
	  font_entry->xlfd = NULL;
	  font_entry->description.family_name = g_strndup (tok, toksize);
	  g_strdown (font_entry->description.family_name);

	  toksize = find_tok (&p, &tok);
	  if (toksize == -1)
	    goto error;

	  if (!get_style (tok, toksize, &font_entry->description))
	    goto error;

	  toksize = find_tok (&p, &tok);
	  if (toksize == -1)
	    goto error;

	  if (!get_variant (tok, toksize, &font_entry->description))
	    goto error;

	  toksize = find_tok (&p, &tok);
	  if (toksize == -1)
	    goto error;

	  if (!get_weight (tok, toksize, &font_entry->description))
	    goto error;
	  
	  toksize = find_tok (&p, &tok);
	  if (toksize == -1)
	    goto error;

	  if (!get_stretch (tok, toksize, &font_entry->description))
	    goto error;

	  toksize = find_tok (&p, &tok);
	  if (toksize == -1)
	    goto error;

	  font_entry->xlfd = g_strndup (tok, toksize);

	  /* Check for complete fields */

	  xlfds = g_strsplit (font_entry->xlfd, ",", -1);
	  for (i=0; xlfds[i]; i++)
	    if (!pango_x_is_xlfd_font_name (xlfds[i]))
	      {
		g_warning ("XLFD '%s' must be complete (14 fields)", xlfds[i]);
		g_strfreev (xlfds);
		goto error;
	      }

	  g_strfreev (xlfds);
	  

	  /* Insert the font entry into our structures */

	  family_entry = pango_x_get_family_entry (xfontmap, font_entry->description.family_name);
	  family_entry->font_entries = g_slist_prepend (family_entry->font_entries, font_entry);
	  xfontmap->n_fonts++;

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
	  if (font_entry->xlfd)
	    g_free (font_entry->xlfd);
	  if (font_entry->description.family_name)
	    g_free (font_entry->description.family_name);
	  g_free (font_entry);
	}

      g_warning ("Error parsing line %d of alias file '%s'", lineno, filename);

    out:
      g_free (buf);
      fclose (infile);
      return;
    }

}

static void
pango_x_font_map_read_aliases (PangoXFontMap *xfontmap)
{
  char *filename;

  pango_x_font_map_read_alias_file (xfontmap, SYSCONFDIR "/pango/pangox_aliases");

  filename = g_strconcat (g_get_home_dir(), "/.pangox_aliases", NULL);
  pango_x_font_map_read_alias_file (xfontmap, filename);
  g_free (filename);

  /* FIXME: Remove this one */
  pango_x_font_map_read_alias_file (xfontmap, "pangox_aliases");
}


/*
 * Returns TRUE if the fontname is a valid XLFD.
 * (It just checks if the number of dashes is 14, and that each
 * field < XLFD_MAX_FIELD_LEN  characters long - that's not in the XLFD but it
 * makes it easier for me).
 */
static gboolean
pango_x_is_xlfd_font_name (const char *fontname)
{
  int i = 0;
  int field_len = 0;
  
  while (*fontname)
    {
      if (*fontname++ == '-')
	{
	  if (field_len > XLFD_MAX_FIELD_LEN) return FALSE;
	  field_len = 0;
	  i++;
	}
      else
	field_len++;
    }
  
  return (i == 14) ? TRUE : FALSE;
}

static int
pango_x_get_size (PangoXFontMap *xfontmap, const char *fontname)
{
  char size_buffer[XLFD_MAX_FIELD_LEN];
  int size;

  if (!pango_x_get_xlfd_field (fontname, XLFD_PIXELS, size_buffer))
    return -1;

  size = atoi (size_buffer);
  if (size != 0)
    {
      return (int)(0.5 + size * xfontmap->resolution);
    }
  else
    {
      /* We use the trick that scaled bitmaps have a non-zero RESOLUTION_X, while
       * actual scaleable fonts have a zero RESOLUTION_X */
      if (!pango_x_get_xlfd_field (fontname, XLFD_RESOLUTION_X, size_buffer))
	return -1;

      if (atoi (size_buffer) == 0)
	return 0;
      else
	return -1;
    }
}

static char *
pango_x_get_identifier (const char *fontname)
{
  const char *p = fontname;
  const char *start;
  int n_dashes = 0;

  while (n_dashes < 2)
    {
      if (*p == '-')
	n_dashes++;
      p++;
    }

  start = p;

  while (n_dashes < 6)
    {
      if (*p == '-')
	n_dashes++;
      p++;
    }

  return g_strndup (start, (p - 1 - start));
}

/*
 * This fills the buffer with the specified field from the X Logical Font
 * Description name, and returns it. If fontname is NULL or the field is
 * longer than XFLD_MAX_FIELD_LEN it returns NULL.
 * Note: For the charset field, we also return the encoding, e.g. 'iso8859-1'.
 */
static char*
pango_x_get_xlfd_field (const char *fontname,
			 FontField    field_num,
			 char       *buffer)
{
  const char *t1, *t2;
  int countdown, len, num_dashes;
  
  if (!fontname)
    return NULL;
  
  /* we assume this is a valid fontname...that is, it has 14 fields */
  
  countdown = field_num;
  t1 = fontname;
  while (*t1 && (countdown >= 0))
    if (*t1++ == '-')
      countdown--;
  
  num_dashes = (field_num == XLFD_CHARSET) ? 2 : 1;
  for (t2 = t1; *t2; t2++)
    { 
      if (*t2 == '-' && --num_dashes == 0)
	break;
    }
  
  if (t1 != t2)
    {
      /* Check we don't overflow the buffer */
      len = (long) t2 - (long) t1;
      if (len > XLFD_MAX_FIELD_LEN - 1)
	return NULL;
      strncpy (buffer, t1, len);
      buffer[len] = 0;
      /* Convert to lower case. */
      g_strdown (buffer);
    }
  else
    strcpy(buffer, "(nil)");
  
  return buffer;
}

/* This inserts the given fontname into the FontInfo table.
   If a FontInfo already exists with the same family and foundry, then the
   fontname is added to the FontInfos list of fontnames, else a new FontInfo
   is created and inserted in alphabetical order in the table. */
static void
pango_x_insert_font (PangoXFontMap *xfontmap,
		     const char  *fontname)
{
  PangoFontDescription description;
  char family_buffer[XLFD_MAX_FIELD_LEN];
  char weight_buffer[XLFD_MAX_FIELD_LEN];
  char slant_buffer[XLFD_MAX_FIELD_LEN];
  char set_width_buffer[XLFD_MAX_FIELD_LEN];
  GSList *tmp_list;
  PangoXFamilyEntry *family_entry;
  PangoXFontEntry *font_entry;
  PangoXSizeInfo *size_info;
  char *identifier;
  int i;

  description.size = 0;
  
  /* First insert the XLFD into the list of XLFDs for the "identifier" - which
   * is the 2-4th fields of the XLFD
   */
  identifier = pango_x_get_identifier (fontname);
  size_info = g_hash_table_lookup (xfontmap->size_infos, identifier);
  if (!size_info)
    {
      size_info = g_new (PangoXSizeInfo, 1);
      size_info->identifier = identifier;
      size_info->xlfds = NULL;

      g_hash_table_insert (xfontmap->size_infos, identifier, size_info);
    }
  else
    g_free (identifier);

  size_info->xlfds = g_slist_prepend (size_info->xlfds, g_strdup (fontname));
  
  /* Convert the XLFD into a PangoFontDescription */
  
  description.family_name = pango_x_get_xlfd_field (fontname, XLFD_FAMILY, family_buffer);
  g_strdown (description.family_name);
  
  if (!description.family_name)
    return;

  description.style = PANGO_STYLE_NORMAL;
  if (pango_x_get_xlfd_field (fontname, XLFD_SLANT, slant_buffer))
    {
      for (i=0; i<G_N_ELEMENTS(styles_map); i++)
	{
	  if (!strcmp (styles_map[i].text, slant_buffer))
	    {
	      description.style = styles_map[i].value;
	      break;
	    }
	}
    }
  else
    strcpy (slant_buffer, "*");

  description.variant = PANGO_VARIANT_NORMAL;
  
  description.weight = PANGO_WEIGHT_NORMAL;
  if (pango_x_get_xlfd_field (fontname, XLFD_WEIGHT, weight_buffer))
    {
      for (i=0; i<G_N_ELEMENTS(weights_map); i++)
	{
	  if (!strcmp (weights_map[i].text, weight_buffer))
	    {
	      description.weight = weights_map[i].value;
	      break;
	    }
	}
    }
  else
    strcpy (weight_buffer, "*");

  description.stretch = PANGO_STRETCH_NORMAL;
  if (pango_x_get_xlfd_field (fontname, XLFD_SET_WIDTH, set_width_buffer))
    {
      for (i=0; i<G_N_ELEMENTS(stretches_map); i++)
	{
	  if (!strcmp (stretches_map[i].text, set_width_buffer))
	    {
	      description.stretch = stretches_map[i].value;
	      break;
	    }
	}
    }
  else
    strcpy (set_width_buffer, "*");

  family_entry = pango_x_get_family_entry (xfontmap, description.family_name);

  tmp_list = family_entry->font_entries;
  while (tmp_list)
    {
      font_entry = tmp_list->data;

      if (font_entry->description.style == description.style &&
	  font_entry->description.variant == description.variant &&
	  font_entry->description.weight == description.weight &&
	  font_entry->description.stretch == description.stretch)
	return;

      tmp_list = tmp_list->next;
    }

  font_entry = g_new (PangoXFontEntry, 1);
  font_entry->description = description;
  font_entry->description.family_name = family_entry->family_name;
  font_entry->cached_fonts = NULL;
  font_entry->coverage = NULL;

  font_entry->xlfd = g_strconcat ("-*-",
				  family_buffer,
				  "-",
				  weight_buffer,
				  "-",
				  slant_buffer,
				  "-",
				  set_width_buffer,
				  "--*-*-*-*-*-*-*-*",
				  NULL);

  family_entry->font_entries = g_slist_append (family_entry->font_entries, font_entry);
  xfontmap->n_fonts++;
}


/**
 * pango_x_get_context:
 * @display: an X display (As returned by XOpenDisplay().)
 * 
 * Retrieves a #PangoContext appropriate for rendering with X fonts on the given display.
  * 
 * Return value: the new #PangoContext
 **/
PangoContext *
pango_x_get_context (Display *display)
{
  PangoContext *result;
  PangoXContextInfo *info;

  g_return_val_if_fail (display != NULL, NULL);

  result = pango_context_new ();
  
  info = g_new (PangoXContextInfo, 1);
  info->get_gc_func = NULL;
  info->free_gc_func = NULL;
  pango_context_set_data (result, "pango-x-info", info, (GDestroyNotify)g_free);
  
  pango_context_add_font_map (result, pango_x_font_map_for_display (display));

  return result;
}

/**
 * pango_x_context_set_funcs:
 * @context: a #PangoContext.
 * @get_gc_func: function called to create a new GC for a given color.
 * @free_gc_func: function called to free a GC created with @get_gc_func.
 * 
 * Sets the functions that will be used to get GC's in various colors when
 * rendering layouts with this context.
 **/
void
pango_x_context_set_funcs  (PangoContext     *context,
			    PangoGetGCFunc    get_gc_func,
			    PangoFreeGCFunc   free_gc_func)
{
  PangoXContextInfo *info;
  
  g_return_if_fail (context != NULL);

  info = pango_context_get_data (context, "pango-x-info");

  info->get_gc_func = get_gc_func;
  info->free_gc_func = free_gc_func;
}

/**
 * pango_x_load_font:
 * @display: the X display
 * @spec:    a comma-separated list of XLFD's
 *
 * Loads up a logical font based on a "fontset" style
 * text specification.
 *
 * Returns a new #PangoFont
 */
PangoFont *
pango_x_load_font (Display *display,
		   char    *spec)
{
  PangoXFont *result;

  g_return_val_if_fail (display != NULL, NULL);
  g_return_val_if_fail (spec != NULL, NULL);
  
  result = g_new (PangoXFont, 1);

  result->display = display;

  pango_font_init (&result->font);
  result->font.klass = &pango_x_font_class;

  result->fonts = g_strsplit(spec, ",", -1);

  for (result->n_fonts = 0; result->fonts[result->n_fonts]; result->n_fonts++)
    ; /* Nothing */

  result->subfonts_by_charset = g_hash_table_new (g_str_hash, g_str_equal);
  result->subfonts = g_new (PangoXSubfontInfo *, 1);

  result->n_subfonts = 0;
  result->max_subfonts = 1;

  result->metrics_by_lang = NULL;

  result->size = -1;
  result->entry = NULL;
  
  return (PangoFont *)result;
}
 
/**
 * pango_x_load_font_with_size:
 * @display: the X display
 * @spec:    a comma-separated list of XLFD's, unsized
 * @size:    the size at which to load the font
 *
 * Load up a logical font based on a "fontset" style
 * text specification.
 *
 * Returns a new #PangoFont
 */
static PangoFont *
pango_x_load_font_with_size (Display *display,
			     char    *spec,
			     int      size)
{
  PangoXFont *result;

  g_return_val_if_fail (display != NULL, NULL);
  g_return_val_if_fail (spec != NULL, NULL);
  
  result = g_new (PangoXFont, 1);

  result->display = display;

  pango_font_init (&result->font);
  result->font.klass = &pango_x_font_class;

  result->fonts = g_strsplit(spec, ",", -1);

  for (result->n_fonts = 0; result->fonts[result->n_fonts]; result->n_fonts++)
    ; /* Nothing */

  result->subfonts_by_charset = g_hash_table_new (g_str_hash, g_str_equal);
  result->subfonts = g_new (PangoXSubfontInfo *, 1);

  result->n_subfonts = 0;
  result->max_subfonts = 1;

  result->metrics_by_lang = NULL;

  result->size = size;
  result->entry = NULL;
  
  return (PangoFont *)result;
}
 
/**
 * pango_x_render:
 * @display: the X display
 * @d:       the drawable on which to draw string
 * @gc:      the graphics context
 * @font:    the font in which to draw the string
 * @glyphs:  the glyph string to draw
 * @x:       the x position of start of string (in pixels)
 * @y:       the y position of baseline (in pixels)
 *
 * Render a PangoGlyphString onto an X drawable
 */
void 
pango_x_render  (Display           *display,
		 Drawable           d,
		 GC                 gc,
		 PangoFont         *font,
		 PangoGlyphString  *glyphs,
		 int                x, 
		 int                y)
{
  /* Slow initial implementation. For speed, it should really
   * collect the characters into runs, and draw multiple
   * characters with each XDrawString16 call.
   */
  Font old_fid = None;
  XFontStruct *fs;
  int i;
  int x_off = 0;

  g_return_if_fail (display != NULL);
  g_return_if_fail (glyphs != NULL);
  
  for (i=0; i<glyphs->num_glyphs; i++)
    {
      guint16 index = PANGO_X_GLYPH_INDEX (glyphs->glyphs[i].glyph);
      guint16 subfont_index = PANGO_X_GLYPH_SUBFONT (glyphs->glyphs[i].glyph);
      PangoXSubfontInfo *subfont;
      
      XChar2b c;

      subfont = pango_x_find_subfont (font, subfont_index);
      if (subfont)
	{
	  c.byte1 = index / 256;
	  c.byte2 = index % 256;

	  fs = pango_x_get_font_struct (font, subfont);
	  if (!fs)
	    continue;
	  
	  if (fs->fid != old_fid)
	    {
	      XSetFont (display, gc, fs->fid);
	      old_fid = fs->fid;
	    }
	  
	  XDrawString16 (display, d, gc,
			 x + (x_off + glyphs->glyphs[i].geometry.x_offset) / PANGO_SCALE,
			 y + glyphs->glyphs[i].geometry.y_offset / PANGO_SCALE,
			 &c, 1);
	}

      x_off += glyphs->glyphs[i].geometry.width;
    }
}

static void
pango_x_font_get_glyph_extents  (PangoFont      *font,
				 PangoGlyph      glyph,
				 PangoRectangle *ink_rect,
				 PangoRectangle *logical_rect)
{
  XCharStruct *cs;
  PangoXSubfontInfo *subfont;

  if (pango_x_find_glyph (font, glyph, &subfont, &cs))
    {
      if (ink_rect)
	{
	  ink_rect->x = PANGO_SCALE * cs->lbearing;
	  ink_rect->width = PANGO_SCALE * (cs->rbearing - cs->lbearing);
	  ink_rect->y = PANGO_SCALE * -cs->ascent;
	  ink_rect->height = PANGO_SCALE * (cs->ascent + cs->descent);
	}
      if (logical_rect)
	{
	  logical_rect->x = 0;
	  logical_rect->width = PANGO_SCALE * cs->width;
	  logical_rect->y = - PANGO_SCALE * subfont->font_struct->ascent;
	  logical_rect->height = PANGO_SCALE * (subfont->font_struct->ascent + subfont->font_struct->descent);
	}
    }
  else
    {
      if (ink_rect)
	{
	  ink_rect->x = 0;
	  ink_rect->width = 0;
	  ink_rect->y = 0;
	  ink_rect->height = 0;
	}
      if (logical_rect)
	{
	  logical_rect->x = 0;
	  logical_rect->width = 0;
	  logical_rect->y = 0;
	  logical_rect->height = 0;
	}
    }
}

/* Get composite font metrics for all subfonts in list
 */
static void
get_font_metrics_from_subfonts (PangoFont        *font,
				GSList           *subfonts,
				PangoFontMetrics *metrics)
{
  GSList *tmp_list = subfonts;
  gboolean first = TRUE;
  
  metrics->ascent = 0;
  metrics->descent = 0;
  
  while (tmp_list)
    {
      PangoXSubfontInfo *subfont = pango_x_find_subfont (font, GPOINTER_TO_UINT (tmp_list->data));
      
      if (subfont)
	{
	  XFontStruct *fs = pango_x_get_font_struct (font, subfont);
	  if (fs)
	    {
	      if (first)
		{
		  metrics->ascent = fs->ascent * PANGO_SCALE;
		  metrics->descent = fs->descent * PANGO_SCALE;
		  first = FALSE;
		}
	      else
		{
		  metrics->ascent = MAX (fs->ascent * PANGO_SCALE, metrics->ascent);
		  metrics->descent = MAX (fs->descent * PANGO_SCALE, metrics->descent);
		}
	    }
	}
      else
	g_warning ("Invalid subfont %d in get_font_metrics_from_subfonts", GPOINTER_TO_UINT (tmp_list->data));
	  
      tmp_list = tmp_list->next;
    }
}

/* Get composite font metrics for all subfonts resulting from shaping
 * string str with the given font
 *
 * This duplicates quite a bit of code from pango_itemize. This function
 * should die and we should simply add the ability to specify particular
 * fonts when itemizing.
 */
static void
get_font_metrics_from_string (PangoFont        *font,
			      const char       *lang,
			      const char       *str,
			      PangoFontMetrics *metrics)
{
  const char *start, *p;
  PangoGlyphString *glyph_str = pango_glyph_string_new ();
  PangoEngineShape *shaper, *last_shaper;
  int last_level;
  GUChar4 *text_ucs4;
  int n_chars, i;
  guint8 *embedding_levels;
  FriBidiCharType base_dir = PANGO_DIRECTION_LTR;
  GSList *subfonts = NULL;
  
  n_chars = unicode_strlen (str, -1);

  text_ucs4 = _pango_utf8_to_ucs4 (str, strlen (str));
  if (!text_ucs4)
    return;

  embedding_levels = g_new (guint8, n_chars);
  fribidi_log2vis_get_embedding_levels (text_ucs4, n_chars, &base_dir,
					embedding_levels);
  g_free (text_ucs4);

  last_shaper = NULL;
  last_level = 0;
  
  i = 0;
  p = start = str;
  while (*p)
    {
      unicode_char_t wc;
      p = unicode_get_utf8 (p, &wc);

      shaper = pango_font_find_shaper (font, lang, wc);
      if (p > start &&
	  (shaper != last_shaper || last_level != embedding_levels[i]))
	{
	  PangoAnalysis analysis;
	  int j;

	  analysis.shape_engine = shaper;
	  analysis.lang_engine = NULL;
	  analysis.font = font;
	  analysis.level = last_level;
	  
	  pango_shape (start, p - start, &analysis, glyph_str);

	  for (j = 0; j < glyph_str->num_glyphs; j++)
	    {
	      PangoXSubfont subfont = PANGO_X_GLYPH_SUBFONT (glyph_str->glyphs[j].glyph);
	      if (!g_slist_find (subfonts, GUINT_TO_POINTER ((guint)subfont)))
		subfonts = g_slist_prepend (subfonts, GUINT_TO_POINTER ((guint)subfont));
	    }
	  
	  start = p;
	}

      last_shaper = shaper;
      last_level = embedding_levels[i];
      i++;
    }

  get_font_metrics_from_subfonts (font, subfonts, metrics);
  g_slist_free (subfonts);
  
  pango_glyph_string_free (glyph_str);
  g_free (embedding_levels);
}

typedef struct {
  const char *lang;
  const char *str;
} LangInfo;

int
lang_info_compare (const void *key, const void *val)
{
  const LangInfo *lang_info = val;

  return strncmp (key, lang_info->lang, 2);
}

/* The following array is supposed to contain enough text to tickle all necessary fonts for each
 * of the languages in the following. Yes, it's pretty lame. Not all of the languages
 * in the following have sufficient text to excercise all the accents for the language, and
 * there are obviously many more languages to include as well.
 */
LangInfo lang_texts[] = {
  { "ar", "Arabic  السلام عليكم" },
  { "cs", "Czech (česky)  Dobrý den" },
  { "da", "Danish (Dansk)  Hej, Goddag" },
  { "el", "Greek (Ελληνικά) Γειά σας" },
  { "en", "English Hello" },
  { "eo", "Esperanto Saluton" },
  { "es", "Spanish (Español) ¡Hola!" },
  { "et", "Estonian  Tere, Tervist" },
  { "fi", "Finnish (Suomi)  Hei" },
  { "fr", "French (Français)" },
  { "de", "German Grüß Gott" },
  { "iw", "Hebrew   שלום" },
  { "il", "Italiano  Ciao, Buon giorno" },
  { "ja", "Japanese (日本語) こんにちは, ｺﾝﾆﾁﾊ" },
  { "ko", "Korean (한글)   안녕하세요, 안녕하십니까" },
  { "mt", "Maltese   Ċaw, Saħħa" },
  { "nl", "Nederlands, Vlaams Hallo, Dag" },
  { "no", "Norwegian (Norsk) Hei, God dag" },
  { "pl", "Polish   Dzień dobry, Hej" },
  { "ru", "Russian (Русский)" },
  { "sk", "Slovak   Dobrý deň" },
  { "sv", "Swedish (Svenska) Hej, Goddag" },
  { "tr", "Turkish (Türkçe) Merhaba" },
  { "zh", "Chinese (中文,普通话,汉语)" }
};

static void
pango_x_font_get_metrics (PangoFont        *font,
			  const gchar      *lang,
			  PangoFontMetrics *metrics)
{
  PangoXMetricsInfo *info;
  PangoXFont *xfont = (PangoXFont *)font;
  GSList *tmp_list;
      
  const char *lookup_lang;
  const char *str;

  if (lang)
    {
      LangInfo *lang_info = bsearch (lang, lang_texts,
				     G_N_ELEMENTS (lang_texts), sizeof (LangInfo),
				     lang_info_compare);

      if (lang_info)
	{
	  lookup_lang = lang_info->lang;
	  str = lang_info->str;
	}
      else
	{
	  lookup_lang = "UNKNOWN";
	  str = "French (Français)";     /* Assume iso-8859-1 */
	}
    }
  else
    {
      lookup_lang = "NONE";

      /* Complete junk
       */
      str = "السلام عليكم česky Ελληνικά Français 日本語 한글 Русский 中文,普通话,汉语 Türkçe";
    }
  
  tmp_list = xfont->metrics_by_lang;
  while (tmp_list)
    {
      info = tmp_list->data;
      
      if (info->lang == lookup_lang)        /* We _don't_ need strcmp */
	break;

      tmp_list = tmp_list->next;
    }

  if (!tmp_list)
    {
      info = g_new (PangoXMetricsInfo, 1);
      info->lang = lookup_lang;

      xfont->metrics_by_lang = g_slist_prepend (xfont->metrics_by_lang, info);
      
      get_font_metrics_from_string (font, lang, str, &info->metrics);
    }
      
  *metrics = info->metrics;
  return;
}

/* Compare the tail of a to b */
static gboolean
match_end (char *a, char *b)
{
  size_t len_a = strlen (a);
  size_t len_b = strlen (b);

  if (len_b > len_a)
    return FALSE;
  else
    return (strcmp (a + len_a - len_b, b) == 0);
}

/* Substitute in a charset into an XLFD. Return the
 * (g_malloc'd) new name, or NULL if the XLFD cannot
 * match the charset
 */
static char *
name_for_charset (char *xlfd, char *charset)
{
  char *p;
  char *dash_charset = g_strconcat ("-", charset, NULL);
  char *result = NULL;
  int ndashes = 0;

  for (p = xlfd; *p; p++)
    if (*p == '-')
      ndashes++;
  
  if (ndashes == 14) /* Complete XLFD */
    {
      if (match_end (xlfd, "-*-*"))
	{
	  result = g_malloc (strlen (xlfd) - 4 + strlen (dash_charset) + 1);
	  strncpy (result, xlfd, strlen (xlfd) - 4);
	  strcpy (result + strlen (xlfd) - 4, dash_charset);
	}
      if (match_end (xlfd, dash_charset))
	result = g_strdup (xlfd);
    }
  else if (ndashes == 13)
    {
      if (match_end (xlfd, "-*"))
	{
	  result = g_malloc (strlen (xlfd) - 2 + strlen (dash_charset) + 1);
	  strncpy (result, xlfd, strlen (xlfd) - 2);
	  strcpy (result + strlen (xlfd) - 2, dash_charset);
	}
      if (match_end (xlfd, dash_charset))
	result = g_strdup (xlfd);
    }
  else
    {
      if (match_end (xlfd, "*"))
	{
	  result = g_malloc (strlen (xlfd) + strlen (dash_charset) + 1);
	  strcpy (result, xlfd);
	  strcpy (result + strlen (xlfd), dash_charset);
	}
      if (match_end (xlfd, dash_charset))
	result = g_strdup (xlfd);
    }

  g_free (dash_charset);
  return result;
}

/* Given a xlfd, charset and size, find the best matching installed X font.
 * The XLFD must be a full XLFD (14 fields)
 */
static char *
pango_x_make_matching_xlfd (PangoXFontMap *xfontmap, char *xlfd, const char *charset, int size)
{
  GSList *tmp_list;
  PangoXSizeInfo *size_info;
  char *identifier;
  char *closest_match = NULL;
  gint match_distance = 0;
  gboolean match_scaleable = FALSE;
  char *result = NULL;

  char *dash_charset;

  dash_charset = g_strconcat ("-", charset, NULL);

  if (!match_end (xlfd, "-*-*") && !match_end (xlfd, dash_charset))
    {
      g_free (dash_charset);
      return NULL;
    }

  identifier = pango_x_get_identifier (xlfd);
  size_info = g_hash_table_lookup (xfontmap->size_infos, identifier);
  g_free (identifier);

  if (!size_info)
    {
      g_free (dash_charset);
      return NULL;
    }

  tmp_list = size_info->xlfds;
  while (tmp_list)
    {
      char *tmp_xlfd = tmp_list->data;
      
      if (match_end (tmp_xlfd, dash_charset))
	{
	  int font_size = pango_x_get_size (xfontmap, tmp_xlfd);

	  if (size != -1)
	    {
	      int new_distance = (font_size == 0) ? 0 : abs (font_size - size);

	      if (!closest_match ||
		  new_distance < match_distance ||
		  (new_distance < PANGO_SCALE && match_scaleable && font_size != 0))
		{
		  closest_match = tmp_xlfd;
		  match_scaleable = (font_size == 0);
		  match_distance = new_distance;
		}
	    }
	}

      tmp_list = tmp_list->next;
    }

  if (closest_match)
    {
      if (match_scaleable)
	{
	  char *prefix_end, *p;
	  char *size_end;
	  int n_dashes = 0;
	  int target_size;
	  char *prefix;
	  
	  /* OK, we have a match; let's modify it to fit this size and charset */

	  p = closest_match;
	  while (n_dashes < 6)
	    {
	      if (*p == '-')
		n_dashes++;
	      p++;
	    }
	  
	  prefix_end = p - 1;
	  
	  while (n_dashes < 9)
	    {
	      if (*p == '-')
		n_dashes++;
	      p++;
	    }
	  
	  size_end = p - 1;

	  target_size = (int)((double)size / xfontmap->resolution + 0.5);
	  prefix = g_strndup (closest_match, prefix_end - closest_match);
	  result  = g_strdup_printf ("%s--%d-*-*-*-*-*-%s", prefix, target_size, charset);
	  g_free (prefix);
	}
      else
	{
	  result = g_strdup (closest_match);
	}
    }

  g_free (dash_charset);

  return result;
}

static PangoXSubfont
pango_x_insert_subfont (PangoFont *font, const char *xlfd)
{
  PangoXFont *xfont = (PangoXFont *)font;
  PangoXSubfontInfo *info;
  
  info = g_new (PangoXSubfontInfo, 1);
  
  info->xlfd = g_strdup (xlfd);
  info->font_struct = NULL;

  xfont->n_subfonts++;
  
  if (xfont->n_subfonts > xfont->max_subfonts)
    {
      xfont->max_subfonts *= 2;
      xfont->subfonts = g_renew (PangoXSubfontInfo *, xfont->subfonts, xfont->max_subfonts);
    }
  
  xfont->subfonts[xfont->n_subfonts - 1] = info;
  
  return xfont->n_subfonts;
}

/**
 * pango_x_list_subfonts:
 * @font: a PangoFont
 * @charsets: the charsets to list subfonts for.
 * @n_charsets: the number of charsets in @charsets
 * @subfont_ids: location to store a pointer to an array of subfont IDs for each found subfont
 *               the result must be freed using g_free()
 * @subfont_charsets: location to store a pointer to an array of subfont IDs for each found subfont
 *               the result must be freed using g_free()
 * 
 * Returns number of charsets found
 **/
int
pango_x_list_subfonts (PangoFont        *font,
		       char            **charsets,
		       int               n_charsets,
		       PangoXSubfont   **subfont_ids,
		       int             **subfont_charsets)
{
  PangoXFont *xfont = (PangoXFont *)font;
  PangoXSubfont **subfont_lists;
  PangoXFontMap *xfontmap;
  int i, j;
  int n_subfonts = 0;

  g_return_val_if_fail (font != NULL, 0);
  g_return_val_if_fail (n_charsets == 0 || charsets != NULL, 0);

  xfontmap = (PangoXFontMap *)pango_x_font_map_for_display (xfont->display);
  
  subfont_lists = g_new (PangoXSubfont *, n_charsets);

  for (j=0; j<n_charsets; j++)
    {
      subfont_lists[j] = g_hash_table_lookup (xfont->subfonts_by_charset, charsets[j]);
      if (!subfont_lists[j])
	{
	  subfont_lists[j] = g_new (PangoXSubfont, xfont->n_fonts);
	  
	  for (i = 0; i < xfont->n_fonts; i++)
	    {
	      PangoXSubfont subfont = 0;
	      char *xlfd;

	      if (xfont->size == -1)
		{
		  xlfd = name_for_charset (xfont->fonts[i], charsets[j]);
	      
		  if (xlfd)
		    {
		      int count;
		      char **names = XListFonts (xfont->display, xlfd, 1, &count);
		      if (count > 0)
			subfont = pango_x_insert_subfont (font, names[0]);
		      
		      XFreeFontNames (names);
		      g_free (xlfd);
		    }
		}
	      else
		{
		  xlfd = pango_x_make_matching_xlfd (xfontmap, xfont->fonts[i], charsets[j], xfont->size);
		  if (xlfd)
		    {
		      subfont = pango_x_insert_subfont (font, xlfd);
		      g_free (xlfd);
		    }
		}
		  
	      subfont_lists[j][i] = subfont;
	    }

	  g_hash_table_insert (xfont->subfonts_by_charset, g_strdup (charsets[j]), subfont_lists[j]);
	}

      for (i = 0; i < xfont->n_fonts; i++)
	if (subfont_lists[j][i])
	  n_subfonts++;
    }

  *subfont_ids = g_new (PangoXSubfont, n_subfonts);
  *subfont_charsets = g_new (int, n_subfonts);

  n_subfonts = 0;

  for (i=0; i<xfont->n_fonts; i++)
    for (j=0; j<n_charsets; j++)
      if (subfont_lists[j][i])
	{
	  (*subfont_ids)[n_subfonts] = subfont_lists[j][i];
	  (*subfont_charsets)[n_subfonts] = j;
	  n_subfonts++;
	}

  g_free (subfont_lists);

  return n_subfonts;
}

gboolean
pango_x_has_glyph (PangoFont       *font,
		   PangoGlyph  glyph)
{
  g_return_val_if_fail (font != NULL, FALSE);

  return pango_x_find_glyph (font, glyph, NULL, NULL);
}

static void
subfonts_foreach (gpointer key, gpointer value, gpointer data)
{
  g_free (key);
  g_free (value);
}

static void
pango_x_font_destroy (PangoFont *font)
{
  PangoXFont *xfont = (PangoXFont *)font;
  int i;

  for (i=0; i<xfont->n_subfonts; i++)
    {
      PangoXSubfontInfo *info = xfont->subfonts[i];

      g_free (info->xlfd);
      if (info->font_struct)
	XFreeFont (xfont->display, info->font_struct);

      g_free (info);
    }

  g_free (xfont->subfonts);

  g_hash_table_foreach (xfont->subfonts_by_charset, subfonts_foreach, NULL);
  g_hash_table_destroy (xfont->subfonts_by_charset);

  g_slist_foreach (xfont->metrics_by_lang, (GFunc)g_free, NULL);
  g_slist_free (xfont->metrics_by_lang);
  
  if (xfont->entry)
    xfont->entry->cached_fonts = g_slist_remove (xfont->entry->cached_fonts, xfont);

  g_strfreev (xfont->fonts);
  g_free (font);
}

static PangoFontDescription *
pango_x_font_describe (PangoFont *font)
{
  /* FIXME: implement */
  return NULL;
}

static void
free_coverages_foreach (gpointer key,
			gpointer value,
			gpointer data)
{
  pango_coverage_unref (value);
}

static PangoCoverage *
pango_x_font_get_coverage (PangoFont  *font,
			   const char *lang)
{
  guint32 ch;
  PangoMap *shape_map;
  PangoSubmap *submap;
  PangoMapEntry *entry;
  PangoCoverage *coverage;
  PangoCoverage *result;
  PangoCoverageLevel font_level;
  GHashTable *coverage_hash;
  PangoXFont *xfont = (PangoXFont *)font;

  if (xfont->entry)
    if (xfont->entry->coverage)
      {
	pango_coverage_ref (xfont->entry->coverage);
	return xfont->entry->coverage;
      }

  result = pango_coverage_new ();

  coverage_hash = g_hash_table_new (g_str_hash, g_str_equal);
  
  shape_map = _pango_find_map (lang, PANGO_ENGINE_TYPE_SHAPE,
			       PANGO_RENDER_TYPE_X);

  for (ch = 0; ch < 65536; ch++)
    {
      submap = &shape_map->submaps[ch / 256];
      entry = submap->is_leaf ? &submap->d.entry : &submap->d.leaves[ch % 256];

      if (entry->info)
	{
	  coverage = g_hash_table_lookup (coverage_hash, entry->info->id);
	  if (!coverage)
	    {
	      PangoEngineShape *engine = (PangoEngineShape *)_pango_load_engine (entry->info->id);
	      coverage = engine->get_coverage (font, lang);
	      g_hash_table_insert (coverage_hash, entry->info->id, coverage);
	    }
	  
	  font_level = pango_coverage_get (coverage, ch);
	  if (font_level == PANGO_COVERAGE_EXACT && !entry->is_exact)
	    font_level = PANGO_COVERAGE_APPROXIMATE;

	  if (font_level != PANGO_COVERAGE_NONE)
	    pango_coverage_set (result, ch, font_level);
	}
    }

  g_hash_table_foreach (coverage_hash, free_coverages_foreach, NULL);
  g_hash_table_destroy (coverage_hash);

  if (xfont->entry)
    {
      xfont->entry->coverage = result;
      pango_coverage_ref (result);
    }

  return result;
}

static PangoEngineShape *
pango_x_font_find_shaper (PangoFont   *font,
			  const gchar *lang,
			  guint32      ch)
{
  PangoMap *shape_map = NULL;
  PangoSubmap *submap;
  PangoMapEntry *entry;

  shape_map = _pango_find_map (lang, PANGO_ENGINE_TYPE_SHAPE,
			       PANGO_RENDER_TYPE_X);

  submap = &shape_map->submaps[ch / 256];
  entry = submap->is_leaf ? &submap->d.entry : &submap->d.leaves[ch % 256];

  /* FIXME: check entry->is_exact */
  if (entry->info)
    return (PangoEngineShape *)_pango_load_engine (entry->info->id);
  else
    return NULL;
}

/* Utility functions */

static XFontStruct *
pango_x_get_font_struct (PangoFont *font, PangoXSubfontInfo *info)
{
  PangoXFont *xfont = (PangoXFont *)font;
  
  if (!info->font_struct)
    {
      info->font_struct = XLoadQueryFont (xfont->display, info->xlfd);
      if (!info->font_struct)
	g_warning ("Cannot load font for XLFD '%s\n", info->xlfd);
    }
  
  return info->font_struct;
}

static PangoXSubfontInfo *
pango_x_find_subfont (PangoFont  *font,
		      PangoXSubfont subfont_index)
{
  PangoXFont *xfont = (PangoXFont *)font;
  
  if (subfont_index < 1 || subfont_index > xfont->n_subfonts)
    {
      g_warning ("Invalid subfont %d", subfont_index);
      return NULL;
    }
  
  return xfont->subfonts[subfont_index-1];
}

static XCharStruct *
pango_x_get_per_char (PangoFont         *font,
		      PangoXSubfontInfo *subfont,
		      guint16            char_index)
{
  XFontStruct *fs;

  int index;
  guint8 byte1;
  guint8 byte2;

  byte1 = char_index / 256;
  byte2 = char_index % 256;

  fs = pango_x_get_font_struct (font, subfont);
  if (!fs)
    return NULL;
	  
  if ((fs->min_byte1 == 0) && (fs->max_byte1 == 0))
    {
      if (byte2 < fs->min_char_or_byte2 || byte2 > fs->max_char_or_byte2)
	return NULL;
      
      index = byte2 - fs->min_char_or_byte2;
    }
  else
    {
      if (byte1 < fs->min_byte1 || byte1 > fs->max_byte1 ||
	  byte2 < fs->min_char_or_byte2 || byte2 > fs->max_char_or_byte2)
	return FALSE;
      
      index = ((byte1 - fs->min_byte1) *
	       (fs->max_char_or_byte2 - fs->min_char_or_byte2 + 1)) +
	byte2 - fs->min_char_or_byte2;
    }
  
  if (fs->per_char)
    return &fs->per_char[index];
  else
    return &fs->min_bounds;
}

static gboolean
pango_x_find_glyph (PangoFont *font,
		    PangoGlyph glyph,
		    PangoXSubfontInfo **subfont_return,
		    XCharStruct **charstruct_return)
{
  PangoXSubfontInfo *subfont;
  XCharStruct *cs;

  guint16 char_index = PANGO_X_GLYPH_INDEX (glyph);
  guint16 subfont_index = PANGO_X_GLYPH_SUBFONT (glyph);

  subfont = pango_x_find_subfont (font, subfont_index);
  if (!subfont)
    return FALSE;
  
  cs = pango_x_get_per_char (font, subfont, char_index);

  if (cs && (cs->lbearing != cs->rbearing || cs->width != 0))
    {
      if (subfont_return)
	*subfont_return = subfont;

      if (charstruct_return)
	*charstruct_return = cs;
      
      return TRUE;
    }
  else
    return FALSE;
}

/**
 * pango_x_get_unknown_glyph:
 * @font: a #PangoFont
 * 
 * Return the index of a glyph suitable for drawing unknown characters.
 * 
 * Return value: a glyph index into @font
 **/
PangoGlyph
pango_x_get_unknown_glyph (PangoFont *font)
{
  PangoXFont *xfont = (PangoXFont *)font;
  
  /* The strategy here is to find _a_ X font, any X font in the fontset, and
   * then get the unknown glyph for that font.
   */

  g_return_val_if_fail (font != 0, 0);
  g_return_val_if_fail (xfont->n_fonts != 0, 0);

  if (xfont->n_subfonts == 0)
    {
      int count;
      char **names = XListFonts (xfont->display, xfont->fonts[0], 1, &count);

      if (count > 0)
	pango_x_insert_subfont (font, names[0]);

      XFreeFontNames (names);
    }

  if (xfont->n_subfonts > 0)
    {
      XFontStruct *font_struct = pango_x_get_font_struct (font, xfont->subfonts[0]);

      if (font_struct)
	return PANGO_X_MAKE_GLYPH (1,font_struct->default_char);
    }

  return 0;
}

/**
 * pango_x_render_layout_line:
 * @display:   the X display
 * @drawable:  the drawable on which to draw string
 * @gc:        GC to use for uncolored drawing
 * @line:      a #PangoLayoutLine
 * @x:         the x position of start of string (in pixels)
 * @y:         the y position of baseline (in pixels)
 *
 * Render a #PangoLayoutLine onto an X drawable
 */
void 
pango_x_render_layout_line (Display          *display,
			    Drawable          drawable,
			    GC                gc,
			    PangoLayoutLine  *line,
			    int               x, 
			    int               y)
{
  GSList *tmp_list = line->runs;
  PangoRectangle overall_rect;
  PangoRectangle logical_rect;
  PangoRectangle ink_rect;
  PangoContext *context = pango_layout_get_context (line->layout);
  PangoXContextInfo *info = pango_context_get_data (context, "pango-x-info");
  
  int x_off = 0;

  pango_layout_line_get_extents (line,NULL, &overall_rect);
  
  while (tmp_list)
    {
      PangoUnderline uline = PANGO_UNDERLINE_NONE;
      PangoLayoutRun *run = tmp_list->data;
      PangoAttrColor fg_color, bg_color;
      gboolean fg_set, bg_set;
      GC fg_gc;
      
      tmp_list = tmp_list->next;

      pango_x_get_item_properties (run->item, &uline, &fg_color, &fg_set, &bg_color, &bg_set);

      if (fg_set && info->get_gc_func)
	fg_gc = info->get_gc_func (context, &fg_color, gc);
      else
	fg_gc = gc;

      if (uline == PANGO_UNDERLINE_NONE)
	pango_glyph_string_extents (run->glyphs, run->item->analysis.font,
				    NULL, &logical_rect);
      else
	pango_glyph_string_extents (run->glyphs, run->item->analysis.font,
				    &ink_rect, &logical_rect);

      if (bg_set && info->get_gc_func)
	{
	  GC bg_gc = info->get_gc_func (context, &bg_color, gc);

	  XFillRectangle (display, drawable, bg_gc,
			  x + (x_off + logical_rect.x) / PANGO_SCALE,
			  y + overall_rect.y / PANGO_SCALE,
			  logical_rect.width / PANGO_SCALE,
			  overall_rect.height / PANGO_SCALE);

	  if (info->free_gc_func)
	    info->free_gc_func (context, bg_gc);
	}

      pango_x_render (display, drawable, fg_gc, run->item->analysis.font, run->glyphs,
		      x + x_off / PANGO_SCALE, y);

      switch (uline)
	{
	case PANGO_UNDERLINE_NONE:
	  break;
	case PANGO_UNDERLINE_DOUBLE:
	  XDrawLine (display, drawable, fg_gc,
		     x + (x_off + ink_rect.x) / PANGO_SCALE - 1, y + 4,
		     x + (x_off + ink_rect.x + ink_rect.width) / PANGO_SCALE, y + 4);
	  /* Fall through */
	case PANGO_UNDERLINE_SINGLE:
	  XDrawLine (display, drawable, fg_gc,
		     x + (x_off + ink_rect.x) / PANGO_SCALE - 1, y + 2,
		     x + (x_off + ink_rect.x + ink_rect.width) / PANGO_SCALE, y + 2);
	  break;
	case PANGO_UNDERLINE_LOW:
	  XDrawLine (display, drawable, fg_gc,
		     x + (x_off + ink_rect.x) / PANGO_SCALE - 1, y + (ink_rect.y + ink_rect.height) / PANGO_SCALE + 2,
		     x + (x_off + ink_rect.x + ink_rect.width) / PANGO_SCALE, y + (ink_rect.y + ink_rect.height) / PANGO_SCALE + 2);
	  break;
	}

      if (fg_set && info->get_gc_func && info->free_gc_func)
	info->free_gc_func (context, fg_gc);
      
      x_off += logical_rect.width;
    }
}

/**
 * pango_x_render_layout:
 * @display:   the X display
 * @drawable:  the drawable on which to draw string
 * @gc:        GC to use for uncolored drawing
 * @layout:    a #PangoLayout
 * @x:         the X position of the left of the layout (in pixels)
 * @y:         the Y position of the top of the layout (in pixels)
 *
 * Render a #PangoLayoutLine onto an X drawable
 */
void 
pango_x_render_layout (Display         *display,
		       Drawable         drawable,
		       GC               gc,
		       PangoLayout     *layout,
		       int              x, 
		       int              y)
{
  PangoRectangle logical_rect;
  GSList *tmp_list;
  PangoAlignment align;
  int indent;
  int width;
  int y_offset = 0;

  gboolean first = FALSE;
  
  g_return_if_fail (display != NULL);
  g_return_if_fail (layout != NULL);

  indent = pango_layout_get_indent (layout);
  width = pango_layout_get_width (layout);
  align = pango_layout_get_alignment (layout);

  if (width == -1 && align != PANGO_ALIGN_LEFT)
    {
      pango_layout_get_extents (layout, NULL, &logical_rect);
      width = logical_rect.width;
    }
  
  tmp_list = pango_layout_get_lines (layout);
  while (tmp_list)
    {
      PangoLayoutLine *line = tmp_list->data;
      int x_offset;
      
      pango_layout_line_get_extents (line, NULL, &logical_rect);

      if (width != 1 && align == PANGO_ALIGN_RIGHT)
	x_offset = width - logical_rect.width;
      else if (width != 1 && align == PANGO_ALIGN_CENTER)
	x_offset = (width - logical_rect.width) / 2;
      else
	x_offset = 0;

      if (first)
	{
	  if (indent > 0)
	    {
	      if (align == PANGO_ALIGN_LEFT)
		x_offset += indent;
	      else
		x_offset -= indent;
	    }

	  first = FALSE;
	}
      else
	{
	  if (indent < 0)
	    {
	      if (align == PANGO_ALIGN_LEFT)
		x_offset -= indent;
	      else
		x_offset += indent;
	    }
	}
	  
      pango_x_render_layout_line (display, drawable, gc,
				  line, x + x_offset / PANGO_SCALE, y + (y_offset - logical_rect.y) / PANGO_SCALE);

      y_offset += logical_rect.height;
      tmp_list = tmp_list->next;
    }
}

/* This utility function is duplicated here and in pango-layout.c; should it be
 * public? Trouble is - what is the appropriate set of properties?
 */
static void
pango_x_get_item_properties (PangoItem      *item,
			     PangoUnderline *uline,
			     PangoAttrColor *fg_color,
			     gboolean       *fg_set,
			     PangoAttrColor *bg_color,
			     gboolean       *bg_set)
{
  GSList *tmp_list = item->extra_attrs;

  if (fg_set)
    *fg_set = FALSE;
  
  if (bg_set)
    *bg_set = FALSE;
  
  while (tmp_list)
    {
      PangoAttribute *attr = tmp_list->data;

      switch (attr->klass->type)
	{
	case PANGO_ATTR_UNDERLINE:
	  if (uline)
	    *uline = ((PangoAttrInt *)attr)->value;
	  break;
	  
	case PANGO_ATTR_FOREGROUND:
	  if (fg_color)
	    *fg_color = *((PangoAttrColor *)attr);
	  if (fg_set)
	    *fg_set = TRUE;
	  
	  break;
	  
	case PANGO_ATTR_BACKGROUND:
	  if (bg_color)
	    *bg_color = *((PangoAttrColor *)attr);
	  if (bg_set)
	    *bg_set = TRUE;
	  
	  break;
	  
	default:
	  break;
	}
      tmp_list = tmp_list->next;
    }
}
