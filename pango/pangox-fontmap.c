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
#include <glib.h>

#include <X11/Xatom.h>

#include "pango-fontmap.h"
#include "pango-utils.h"
#include "pangox-private.h"

typedef struct _PangoXFamilyEntry  PangoXFamilyEntry;
typedef struct _PangoXSizeInfo     PangoXSizeInfo;

/* Number of freed fonts */
#define MAX_FREED_FONTS 16

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

static void     pango_x_fontmap_cache_clear (PangoXFontMap   *xfontmap);
static void     pango_x_font_map_read_aliases (PangoXFontMap *xfontmap);

static gint     pango_x_get_size            (PangoXFontMap      *fontmap,
					     const char         *fontname);
static void     pango_x_insert_font         (PangoXFontMap      *fontmap,
					     const char         *fontname);
static gboolean pango_x_is_xlfd_font_name   (const char         *fontname);
static char *   pango_x_get_xlfd_field      (const char         *fontname,
					     FontField           field_num,
					     char               *buffer);
static char *   pango_x_get_identifier      (const char         *fontname);


static PangoFontClass *parent_class;	/* Parent class structure for PangoXFontMap */

GType
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
                                            &object_info, 0);
    }
  
  return object_type;
}

static void 
pango_x_font_map_init (PangoXFontMap *xfontmap)
{
  xfontmap->families = g_hash_table_new (g_str_hash, g_str_equal);
  xfontmap->size_infos = g_hash_table_new (g_str_hash, g_str_equal);
  xfontmap->to_atom_cache = g_hash_table_new (g_str_hash, g_str_equal);
  xfontmap->from_atom_cache = g_hash_table_new (g_direct_hash, g_direct_equal);
  xfontmap->n_fonts = 0;
}

static void
pango_x_font_map_class_init (PangoXFontMapClass *class)
{
  GObjectClass *object_class = G_OBJECT_CLASS (class);
  PangoFontMapClass *font_map_class = PANGO_FONT_MAP_CLASS (class);
  
  parent_class = g_type_class_peek_parent (class);
  
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

  g_return_val_if_fail (display != NULL, NULL);
  
  /* Make sure that the type system is initialized */
  g_type_init (0);
  
  while (tmp_list)
    {
      xfontmap = tmp_list->data;

      if (xfontmap->display == display)
	return PANGO_FONT_MAP (xfontmap);

      tmp_list = tmp_list->next;
    }

  xfontmap = (PangoXFontMap *)g_type_create_instance (PANGO_TYPE_X_FONT_MAP);
  
  xfontmap->display = display;
  xfontmap->font_cache = pango_x_font_cache_new (display);
  xfontmap->freed_fonts = g_queue_new ();
  
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

  return PANGO_FONT_MAP (xfontmap);
}

/**
 * pango_x_shutdown_display:
 * @display: an X #Display
 * 
 * Free cached resources for the given X display structure.
 **/
void
pango_x_shutdown_display (Display *display)
{
  GList *tmp_list;

  g_return_if_fail (display != NULL);

  tmp_list = fontmaps;
  while (tmp_list)
    {
      PangoXFontMap *xfontmap = tmp_list->data;

      if (xfontmap->display == display)
	{
	  fontmaps = g_list_delete_link (fontmaps, tmp_list);
	  pango_x_fontmap_cache_clear (xfontmap);
	  g_object_unref (G_OBJECT (xfontmap));

	  return;
	}

      tmp_list = tmp_list->next;
    }
}

static void
pango_x_font_map_finalize (GObject *object)
{
  PangoXFontMap *xfontmap = PANGO_X_FONT_MAP (object);
  
  g_list_foreach (xfontmap->freed_fonts->head, (GFunc)g_object_unref, NULL);
  g_queue_free (xfontmap->freed_fonts);
  
  pango_x_font_cache_free (xfontmap->font_cache);

  /* FIXME: Lots more here */

  fontmaps = g_list_remove (fontmaps, xfontmap);
  
  G_OBJECT_CLASS (parent_class)->finalize (object);
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
      int best_distance = G_MAXINT;
      
      tmp_list = family_entry->font_entries;
      while (tmp_list)
	{
	  PangoXFontEntry *font_entry = tmp_list->data;
	  
	  if (font_entry->description.variant == description->variant &&
	      font_entry->description.stretch == description->stretch)
	    {
	      if (font_entry->description.style == description->style)
		{
		  int distance = abs(font_entry->description.weight - description->weight);
		  if (distance < best_distance)
		    {
		      best_match = font_entry;
		      best_distance = distance;
		    }
		}
	      else if (font_entry->description.style != PANGO_STYLE_NORMAL &&
		       description->style != PANGO_STYLE_NORMAL)
		{
		  /* Equate oblique and italic, but with a big penalty
		   */
		  int distance = PANGO_SCALE * 1000 + abs(font_entry->description.weight - description->weight);

		  if (distance < best_distance)
		    {
		      best_match = font_entry;
		      best_distance = distance;
		    }
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
		  if (xfont->in_cache)
		    pango_x_fontmap_cache_remove (fontmap, xfont);
		  
		  break;
		}
	      tmp_list = tmp_list->next;
	    }

	  if (!result)
	    {
	      PangoXFont *xfont = pango_x_font_new (fontmap, best_match->xlfd, description->size);
	      
	      xfont->fontmap = fontmap;
	      xfont->entry = best_match;
	      best_match->cached_fonts = g_slist_prepend (best_match->cached_fonts, xfont);

	      result = (PangoFont *)xfont;
	    }
	}
    }

  g_free (name);
  return result;
}


/************************
 * Coverage Map Caching *
 ************************/

/* We need to be robust against errors accessing the coverage
 * cache window, since it is not our window. So we temporarily
 * set this error handler while accessing it. The error_occured
 * global allows us to tell whether an error occured for
 * XChangeProperty
 */
static gboolean error_occured;

static int 
ignore_error (Display *d, XErrorEvent *e)
{
  return 0;
}

/* Retrieve the coverage window for the given display.
 * We look for a property on the root window, and then
 * check that the window that property points to also
 * has the same property pointing to itself. The second
 * check allows us to make sure that a stale property
 * isn't just pointing to some other apps window
 */
static Window
pango_x_real_get_coverage_win (Display *display)
{
  Atom type;
  int format;
  gulong n_items;
  gulong bytes_after;
  Atom *data;
  Window retval = None;
  int (*old_handler) (Display *, XErrorEvent *);
  
  Atom coverage_win_atom = XInternAtom (display,
					"PANGO_COVERAGE_WIN",
					False);
  
  XGetWindowProperty (display,
		      DefaultRootWindow (display),
		      coverage_win_atom,
		      0, 4,
		      False, XA_WINDOW,
		      &type, &format, &n_items, &bytes_after,
		      (guchar **)&data);
  
  if (type == XA_WINDOW)
    {
      if (format == 32 && n_items == 1 && bytes_after == 0)
	retval = *data;
      
      XFree (data);
    }

  old_handler= XSetErrorHandler (ignore_error);

  if (XGetWindowProperty (display,
			  retval,
			  coverage_win_atom,
			  0, 4,
			  False, XA_WINDOW,
			  &type, &format, &n_items, &bytes_after,
			  (guchar **)&data) == Success &&
      type == XA_WINDOW)
    {
      if (format != 32 || n_items != 1 || bytes_after != 0 ||
	  *data != retval)
	retval = None;
      
      XFree (data);
    }
  else
    retval = None;

  XSync (display, False);
  XSetErrorHandler (old_handler);

  return retval;
}

/* Find or create the peristant window for caching font coverage
 * information.
 *
 * To assure atomic creation, we first look for the window, then if we
 * don't find it, grab the server, look for it again, and then if that
 * still didn't find it, create it and ungrab.
 */
static Window
pango_x_get_coverage_win (PangoXFontMap *xfontmap)
{
  if (!xfontmap->coverage_win)
    xfontmap->coverage_win = pango_x_real_get_coverage_win (xfontmap->display);

  if (!xfontmap->coverage_win)
    {
      Display *persistant_display;

      persistant_display = XOpenDisplay (DisplayString (xfontmap->display));
      if (!persistant_display)
	{
	  g_warning ("Cannot create or retrieve display for font coverage cache");
	  return None;
	}

      XGrabServer (persistant_display);

      xfontmap->coverage_win = pango_x_real_get_coverage_win (xfontmap->display);
      if (!xfontmap->coverage_win)
	{
	  XSetWindowAttributes attr;
	  
	  attr.override_redirect = True;
	  
	  XSetCloseDownMode (persistant_display, RetainPermanent);
	  
	  xfontmap->coverage_win = 
	    XCreateWindow (persistant_display,
			   DefaultRootWindow (persistant_display),
			   -100, -100, 10, 10, 0, 0,
			   InputOnly, CopyFromParent,
			   CWOverrideRedirect, &attr);
	  
	  XChangeProperty (persistant_display,
			   DefaultRootWindow (persistant_display),
			   XInternAtom (persistant_display,
					"PANGO_COVERAGE_WIN",
					FALSE),
			   XA_WINDOW,
			   32, PropModeReplace,
			   (guchar *)&xfontmap->coverage_win, 1);
	  

	  XChangeProperty (persistant_display,
			   xfontmap->coverage_win,
			   XInternAtom (persistant_display,
					"PANGO_COVERAGE_WIN",
					FALSE),
			   XA_WINDOW,
			   32, PropModeReplace,
			   (guchar *)&xfontmap->coverage_win, 1);
	}
      
      XUngrabServer (persistant_display);
      
      XSync (persistant_display, False);
      XCloseDisplay (persistant_display);
    }

  return xfontmap->coverage_win;
}

/* Find the cached value for the coverage map on the
 * coverage cache window, if it exists. *atom is set
 * to the interned value of str for later use in storing
 * the property if the lookup fails
 */
static PangoCoverage *
pango_x_get_cached_coverage (PangoXFontMap *xfontmap,
			     const char    *str,
			     Atom          *atom)
{
  int (*old_handler) (Display *, XErrorEvent *);
  Window coverage_win;
  PangoCoverage *result = NULL;

  Atom type;
  int format;
  int tries = 5;
  gulong n_items;
  gulong bytes_after;
  guchar *data;

  *atom = XInternAtom (xfontmap->display, str, False);
      
  while (tries--)
    {
      coverage_win = pango_x_get_coverage_win (xfontmap);
      
      if (!coverage_win)
	return NULL;
      
      old_handler= XSetErrorHandler (ignore_error);
      
      if (XGetWindowProperty (xfontmap->display,
			      coverage_win, *atom,
			      0, G_MAXLONG,
			      False, XA_STRING,
			      &type, &format, &n_items, &bytes_after,
			      &data) == Success
	  && type == XA_STRING)
	{
	  if (format == 8 && bytes_after == 0)
	    result = pango_coverage_from_bytes (data, n_items);
	  
	  XSetErrorHandler (old_handler);
	  XFree (data);
	  break;
	}
      else
	{
	  /* Window disappeared out from under us */
	  XSetErrorHandler (old_handler);
	  xfontmap->coverage_win = None;
	}

    }
      
  return result;
}

/* Store the given coverage map on the coverage cache window.
 * atom is the intern'ed value of the string that identifies
 * the cache entry.
 */
static void
pango_x_store_cached_coverage (PangoXFontMap *xfontmap,
			       Atom           atom,
			       PangoCoverage *coverage)
{
  int (*old_handler) (Display *, XErrorEvent *);
  guchar *bytes;
  gint size;

  int tries = 5;
  
  pango_coverage_to_bytes (coverage, &bytes, &size);

  while (tries--)
    {
      Window coverage_win = pango_x_get_coverage_win (xfontmap);

      if (!coverage_win)
	break;

      old_handler = XSetErrorHandler (ignore_error);
      error_occured = False;
  
      XChangeProperty (xfontmap->display,
		       coverage_win,
		       atom, 
		       XA_STRING,
		       8, PropModeReplace,
		       bytes, size);
      
      XSync (xfontmap->display, False);
      XSetErrorHandler (old_handler);

      if (!error_occured)
	break;
      else
	{
	  /* Window disappeared out from under us */
	  XSetErrorHandler (old_handler);
	  xfontmap->coverage_win = None;
	}	  
    }
  
  g_free (bytes);
}


static void
pango_x_font_map_read_alias_file (PangoXFontMap *xfontmap,
				  const char   *filename)
{
  FILE *infile;
  char **xlfds;
  int lineno = 0;
  int i;
  PangoXFontEntry *font_entry = NULL;

  infile = fopen (filename, "r");
  if (infile)
    {
      GString *line_buf = g_string_new (NULL);
      GString *tmp_buf = g_string_new (NULL);
      gint lines_read;

      while ((lines_read = pango_read_line (infile, line_buf)))
	{
	  PangoXFamilyEntry *family_entry;

	  const char *p = line_buf->str;
	  
	  lineno += lines_read;

	  if (!pango_skip_space (&p))
	    continue;

	  if (!pango_scan_string (&p, tmp_buf))
	    goto error;

	  font_entry = g_new (PangoXFontEntry, 1);
	  font_entry->xlfd = NULL;
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

	  xlfds = g_strsplit (tmp_buf->str, ",", -1);
	  for (i=0; xlfds[i]; i++)
	    {
	      char *trimmed = pango_trim_string (xlfds[i]);
	      g_free (xlfds[i]);
	      xlfds[i] = trimmed;
	      
	      if (!pango_x_is_xlfd_font_name (xlfds[i]))
		{
		  g_warning ("XLFD '%s' must be complete (14 fields)", xlfds[i]);
		  g_strfreev (xlfds);
		  goto error;
		}
	    }

	  font_entry->xlfd = g_strjoinv (",", xlfds);
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
      g_string_free (tmp_buf, TRUE);
      g_string_free (line_buf, TRUE);

      fclose (infile);
    }

}

static void
pango_x_font_map_read_aliases (PangoXFontMap *xfontmap)
{
  char **files;
  char *files_str = pango_config_key_get ("PangoX/AliasFiles");
  int n;

  if (!files_str)
    files_str = g_strdup ("~/.pangox_aliases:" SYSCONFDIR "/pango/pangox.aliases");

  files = pango_split_file_list (files_str);
  
  n = 0;
  while (files[n])
    n++;

  while (n-- > 0)
    pango_x_font_map_read_alias_file (xfontmap, files[n]);

  g_strfreev (files);
  g_free (files_str);
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
				 PangoLanguage   *language)
{
  PangoXFont *xfont;
  PangoXFontMap *xfontmap = NULL; /* Quiet gcc */
  PangoCoverage *result = NULL;
  GHashTable *coverage_hash;
  Atom atom = None;

  if (entry)
    {
      if (entry->coverage)
	{
	  pango_coverage_ref (entry->coverage);
	  return entry->coverage;
	}
      
      xfont = (PangoXFont *)font;
      
      xfontmap = (PangoXFontMap *)pango_x_font_map_for_display (xfont->display);
      if (entry->xlfd)
	{
	  const char *lang_str = language ? pango_language_to_string (language) : "*";
	  
	  char *str = g_strconcat (lang_str, "|", entry->xlfd, NULL);
	  result = pango_x_get_cached_coverage (xfontmap, str, &atom);
	  g_free (str);
	}
    }

  if (!result)
    {
      guint32 ch;
      PangoMap *shape_map;
      PangoCoverage *coverage;
      PangoCoverageLevel font_level;
      PangoMapEntry *map_entry;
      
      result = pango_coverage_new ();
      
      coverage_hash = g_hash_table_new (g_str_hash, g_str_equal);
      
      shape_map = pango_x_get_shaper_map (language);
      
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

      if (atom)
	pango_x_store_cached_coverage (xfontmap, atom, result);
    }
      
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
  g_return_val_if_fail (PANGO_X_IS_FONT_MAP (font_map), NULL);

  return PANGO_X_FONT_MAP (font_map)->font_cache;
}

Display *
pango_x_fontmap_get_display (PangoFontMap    *fontmap)
{
  g_return_val_if_fail (fontmap != NULL, NULL);
  g_return_val_if_fail (PANGO_X_IS_FONT_MAP (fontmap), NULL);

  return PANGO_X_FONT_MAP (fontmap)->display;
}

void
pango_x_fontmap_cache_add (PangoFontMap    *fontmap,
			   PangoXFont      *xfont)
{
  PangoXFontMap *xfontmap = PANGO_X_FONT_MAP (fontmap);

  if (xfontmap->freed_fonts->length == MAX_FREED_FONTS)
    {
      PangoXFont *old_font = g_queue_pop_tail (xfontmap->freed_fonts);
      g_object_unref (G_OBJECT (old_font));
    }

  g_object_ref (G_OBJECT (xfont));
  g_queue_push_head (xfontmap->freed_fonts, xfont);
  xfont->in_cache = TRUE;
}

void
pango_x_fontmap_cache_remove (PangoFontMap    *fontmap,
			      PangoXFont      *xfont)
{
  PangoXFontMap *xfontmap = PANGO_X_FONT_MAP (fontmap);

  GList *link = g_list_find (xfontmap->freed_fonts->head, xfont);
  if (link == xfontmap->freed_fonts->tail)
    {
      xfontmap->freed_fonts->tail = xfontmap->freed_fonts->tail->prev;
      if (xfontmap->freed_fonts->tail)
	xfontmap->freed_fonts->tail->next = NULL;
    }
  
  xfontmap->freed_fonts->head = g_list_delete_link (xfontmap->freed_fonts->head, link);
  xfontmap->freed_fonts->length--;
  xfont->in_cache = FALSE;

  g_object_unref (G_OBJECT (xfont));
}

static void
pango_x_fontmap_cache_clear (PangoXFontMap   *xfontmap)
{
  g_list_foreach (xfontmap->freed_fonts->head, (GFunc)g_object_unref, NULL);
  g_list_free (xfontmap->freed_fonts->head);
  xfontmap->freed_fonts->head = NULL;
  xfontmap->freed_fonts->tail = NULL;
  xfontmap->freed_fonts->length = 0;
}


Atom
pango_x_fontmap_atom_from_name (PangoFontMap *fontmap, 
				const char   *atomname)
{
  PangoXFontMap *xfm = PANGO_X_FONT_MAP(fontmap);
  gpointer found;
  Atom atom;

  found = g_hash_table_lookup (xfm->to_atom_cache, atomname);

  if (found) 
    return (Atom)(GPOINTER_TO_UINT(found));

  atom = XInternAtom (xfm->display, atomname, FALSE);
  g_hash_table_insert (xfm->to_atom_cache, g_strdup (atomname), 
		       (gpointer)atom);

  return atom;
}


const char *
pango_x_fontmap_name_from_atom (PangoFontMap *fontmap, 
				Atom          atom)
{
  PangoXFontMap *xfm = PANGO_X_FONT_MAP(fontmap);
  gpointer found;
  char *name, *name2;

  found = g_hash_table_lookup (xfm->from_atom_cache, GUINT_TO_POINTER(atom));

  if (found) 
    return (const char *)found;

  name = XGetAtomName (xfm->display, atom);
  name2 = g_strdup (name);
  XFree (name);

  g_hash_table_insert (xfm->from_atom_cache, (gpointer)atom, name2);

  return name2;
}
