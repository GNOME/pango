/* Pango
 * pango-font.h: Font handling
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

#include <ctype.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <errno.h>

#include "pango-fontmap.h"
#include "pangox-private.h"

#define PANGO_TYPE_X_FONT_MAP              (pango_x_font_map_get_type ())
#define PANGO_X_FONT_MAP(object)           (G_TYPE_CHECK_INSTANCE_CAST ((object), PANGO_TYPE_X_FONT_MAP, PangoXFontMap))
#define PANGO_X_FONT_MAP_CLASS(klass)      (G_TYPE_CHECK_CLASS_CAST ((klass), PANGO_TYPE_X_FONT_MAP, PangoXFontMapClass))
#define PANGO_IS_X_FONT_MAP(object)        (G_TYPE_CHECK_INSTANCE_TYPE ((object), PANGO_TYPE_X_FONT_MAP))
#define PANGO_IS_X_FONT_MAP_CLASS(klass)   (G_TYPE_CHECK_CLASS_TYPE ((klass), PANGO_TYPE_X_FONT_MAP))
#define PANGO_X_FONT_MAP_GET_CLASS(obj)    (G_TYPE_INSTANCE_GET_CLASS ((obj), PANGO_TYPE_X_FONT_MAP, PangoXFontMapClass))

typedef struct _PangoXFamilyEntry  PangoXFamilyEntry;
typedef struct _PangoXFontMap      PangoXFontMap;
typedef struct _PangoXFontMapClass PangoXFontMapClass;
typedef struct _PangoXSizeInfo     PangoXSizeInfo;

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

struct _PangoXFontMap
{
  PangoFontMap parent_instance;

  Display *display;

  PangoXFontCache *font_cache;

  GHashTable *families;
  GHashTable *size_infos;

  int n_fonts;

  double resolution;		/* (points / pixel) * PANGO_SCALE */
};

struct _PangoXFontMapClass
{
  PangoFontMapClass parent_class;
};

struct _PangoXFamilyEntry
{
  char *family_name;
  GSList *font_entries;
};

struct _PangoXFontEntry
{
  char *xlfd;
  PangoFontDescription description;
  PangoCoverage *coverage;

  GSList *cached_fonts;
};

struct _PangoXSizeInfo
{
  char *identifier;
  GSList *xlfds;
};

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

static GType    pango_x_font_map_get_type   (void);
static void     pango_x_font_map_init       (PangoXFontMap      *fontmap);
static void     pango_x_font_map_class_init (PangoXFontMapClass *class);

static void       pango_x_font_map_finalize      (GObject                      *object);
static PangoFont *pango_x_font_map_load_font     (PangoFontMap                 *fontmap,
						  const PangoFontDescription   *description);
static void       pango_x_font_map_list_fonts    (PangoFontMap                 *fontmap,
						  const gchar                  *family,
						  PangoFontDescription       ***descs,
						  int                          *n_descs);
static void       pango_x_font_map_list_families (PangoFontMap                 *fontmap,
						  gchar                      ***families,
						  int                          *n_families);

static void pango_x_font_map_read_aliases (PangoXFontMap *xfontmap);

static gint     pango_x_get_size            (PangoXFontMap      *fontmap,
					     const char         *fontname);
static void     pango_x_insert_font         (PangoXFontMap      *fontmap,
					     const char         *fontname);
static gboolean pango_x_is_xlfd_font_name   (const char         *fontname);
static char *   pango_x_get_xlfd_field      (const char         *fontname,
					     FontField           field_num,
					     char               *buffer);
static char *   pango_x_get_identifier      (const char         *fontname);

static GType
pango_x_font_map_get_type (void)
{
  static GType object_type = 0;

  if (!object_type)
    {
      static const GTypeInfo object_info =
      {
        sizeof (PangoXFontMapClass),
        (GBaseInitFunc) NULL,
        (GBaseFinalizeFunc) NULL,
        (GClassInitFunc) pango_x_font_map_class_init,
        NULL,           /* class_finalize */
        NULL,           /* class_data */
        sizeof (PangoXFontMap),
        0,              /* n_preallocs */
        (GInstanceInitFunc) pango_x_font_map_init,
      };
      
      object_type = g_type_register_static (PANGO_TYPE_FONT_MAP,
                                            "PangoXFontMap",
                                            &object_info);
    }
  
  return object_type;
}

static void 
pango_x_font_map_init (PangoXFontMap *xfontmap)
{
  xfontmap->families = g_hash_table_new (g_str_hash, g_str_equal);
  xfontmap->size_infos = g_hash_table_new (g_str_hash, g_str_equal);
  xfontmap->n_fonts = 0;
}

static void
pango_x_font_map_class_init (PangoXFontMapClass *class)
{
  GObjectClass *object_class = G_OBJECT_CLASS (class);
  PangoFontMapClass *font_map_class = PANGO_FONT_MAP_CLASS (class);
  
  object_class->finalize = pango_x_font_map_finalize;
  font_map_class->load_font = pango_x_font_map_load_font;
  font_map_class->list_fonts = pango_x_font_map_list_fonts;
  font_map_class->list_families = pango_x_font_map_list_families;
}

static GList *fontmaps = NULL;

PangoFontMap *
pango_x_font_map_for_display (Display *display)
{
  PangoXFontMap *xfontmap;
  GList *tmp_list = fontmaps;
  char **xfontnames;
  int num_fonts, i;
  int screen;

  /* Make sure that the type system is initialized */
  g_type_init();
  
  while (tmp_list)
    {
      xfontmap = tmp_list->data;

      if (xfontmap->display == display)
	{
	  return PANGO_FONT_MAP (xfontmap);
	}
    }

  xfontmap = (PangoXFontMap *)g_type_create_instance (PANGO_TYPE_X_FONT_MAP);
  
  xfontmap->display = display;
  xfontmap->font_cache = pango_x_font_cache_new (display);

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

  g_object_ref (G_OBJECT (xfontmap));
  fontmaps = g_list_prepend (fontmaps, xfontmap);

  /* This is a little screwed up, since different screens on the same display
   * might have different resolutions
   */
  screen = DefaultScreen (xfontmap->display);
  xfontmap->resolution = (PANGO_SCALE * 72.27 / 25.4) * ((double) DisplayWidthMM (xfontmap->display, screen) /
							 DisplayWidth (xfontmap->display, screen));
  return PANGO_FONT_MAP (xfontmap);
}

static void
pango_x_font_map_finalize (GObject *object)
{
  PangoXFontMap *xfontmap = PANGO_X_FONT_MAP (object);
  
  pango_x_font_cache_free (xfontmap->font_cache);
  /* FIXME: Lots more here */
  
  fontmaps = g_list_remove (fontmaps, object);
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
		  g_object_ref (G_OBJECT (result));
		  break;
		}
	      tmp_list = tmp_list->next;
	    }

	  if (!result)
	    {
	      result = (PangoFont *)pango_x_font_new (xfontmap->display, best_match->xlfd, description->size);
	      ((PangoXFont *)result)->entry = best_match;
	      best_match->cached_fonts = g_slist_prepend (best_match->cached_fonts, result);
	    }

	  /* HORRIBLE performance hack until some better caching scheme is arrived at
	   */
	  if (result)
	    g_object_ref (G_OBJECT (result));
	}
    }

  g_free (name);
  return result;
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

/* Given a xlfd, charset and size, find the best matching installed X font.
 * The XLFD must be a full XLFD (14 fields)
 */
char *
pango_x_make_matching_xlfd (PangoFontMap *fontmap, char *xlfd, const char *charset, int size)
{
  PangoXFontMap *xfontmap;
  
  GSList *tmp_list;
  PangoXSizeInfo *size_info;
  char *identifier;
  char *closest_match = NULL;
  gint match_distance = 0;
  gboolean match_scaleable = FALSE;
  char *result = NULL;

  char *dash_charset;

  xfontmap = PANGO_X_FONT_MAP (fontmap);
  
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

static void
free_coverages_foreach (gpointer key,
			gpointer value,
			gpointer data)
{
  pango_coverage_unref (value);
}

PangoCoverage *
pango_x_font_entry_get_coverage (PangoXFontEntry *entry,
				 PangoFont       *font,
				 const char      *lang)
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
  
  shape_map = pango_x_get_shaper_map (lang);

  for (ch = 0; ch < 65536; ch++)
    {
      map_entry = _pango_map_get_entry (shape_map, ch);
      if (map_entry->info)
	{
	  coverage = g_hash_table_lookup (coverage_hash, map_entry->info->id);
	  if (!coverage)
	    {
	      PangoEngineShape *engine = (PangoEngineShape *)_pango_map_get_engine (shape_map, ch);
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
pango_x_font_entry_remove (PangoXFontEntry *entry,
			   PangoFont       *font)
{
  entry->cached_fonts = g_slist_remove (entry->cached_fonts, font);
}

PangoXFontCache *
pango_x_font_map_get_font_cache (PangoFontMap *font_map)
{
  g_return_val_if_fail (font_map != NULL, NULL);
  g_return_val_if_fail (PANGO_IS_X_FONT_MAP (font_map), NULL);

  return PANGO_X_FONT_MAP (font_map)->font_cache;
}
