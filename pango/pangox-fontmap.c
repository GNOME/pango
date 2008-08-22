/* Pango
 * pangox-fontmap.c: X font handling
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
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <errno.h>
#include <glib.h>

#include <X11/Xatom.h>

/* For XExtSetCloseDisplay */
#include <X11/Xlibint.h>

#include "pango-engine-private.h"
#include "pango-fontmap.h"
#include "pango-impl-utils.h"
#include "modules.h"

#undef PANGO_DISABLE_DEPRECATED

#include "pangox-private.h"

typedef struct _PangoXFamily       PangoXFamily;
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

struct _PangoXFamily
{
  PangoFontFamily parent_instance;

  char *family_name;
  GSList *font_entries;
};

struct _PangoXFace
{
  PangoFontFace parent_instance;

  char *xlfd;
  PangoFontDescription *description;
  PangoCoverage *coverage;

  char *face_name;

  GSList *cached_fonts;
};

struct _PangoXSizeInfo
{
  char *identifier;
  GSList *xlfds;
};

static const struct {
  const gchar text[12];
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

static const struct {
  const gchar text[4];
  PangoStyle value;
} styles_map[] = {
  { "r", PANGO_STYLE_NORMAL },
  { "i", PANGO_STYLE_ITALIC },
  { "o", PANGO_STYLE_OBLIQUE }
};

static const struct {
  const gchar text[16];
  PangoStretch value;
} stretches_map[] = {
  { "normal",        PANGO_STRETCH_NORMAL },
  { "semicondensed", PANGO_STRETCH_SEMI_CONDENSED },
  { "condensed",     PANGO_STRETCH_CONDENSED },
};

static void     pango_x_font_map_init       (PangoXFontMap      *fontmap);
static void     pango_x_font_map_class_init (PangoFontMapClass *class);

static void       pango_x_font_map_finalize      (GObject                      *object);
static PangoFont *pango_x_font_map_load_font     (PangoFontMap                 *fontmap,
						  PangoContext                 *context,
						  const PangoFontDescription   *description);
static void       pango_x_font_map_list_families (PangoFontMap                 *fontmap,
						  PangoFontFamily            ***families,
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


#define PANGO_X_TYPE_FAMILY              (pango_x_family_get_type ())
#define PANGO_X_FAMILY(object)           (G_TYPE_CHECK_INSTANCE_CAST ((object), PANGO_X_TYPE_FAMILY, PangoXFamily))
#define PANGO_X_IS_FAMILY(object)        (G_TYPE_CHECK_INSTANCE_TYPE ((object), PANGO_X_TYPE_FAMILY))

GType           pango_x_family_get_type (void);


#define PANGO_X_TYPE_FACE              (pango_x_face_get_type ())
#define PANGO_X_FACE(object)           (G_TYPE_CHECK_INSTANCE_CAST ((object), PANGO_X_TYPE_FACE, PangoXFace))
#define PANGO_X_IS_FACE(object)        (G_TYPE_CHECK_INSTANCE_TYPE ((object), PANGO_X_TYPE_FACE))

GType           pango_x_face_get_type (void);


static PangoFontClass *font_map_parent_class;	/* Parent class structure for PangoXFontMap */

GType
pango_x_font_map_get_type (void)
{
  static GType object_type = 0;

  if (G_UNLIKELY (!object_type))
    {
      const GTypeInfo object_info =
      {
	sizeof (PangoFontMapClass),
	(GBaseInitFunc) NULL,
	(GBaseFinalizeFunc) NULL,
	(GClassInitFunc) pango_x_font_map_class_init,
	NULL,           /* class_finalize */
	NULL,           /* class_data */
	sizeof (PangoXFontMap),
	0,              /* n_preallocs */
	(GInstanceInitFunc) pango_x_font_map_init,
	NULL            /* value_table */
      };

      object_type = g_type_register_static (PANGO_TYPE_FONT_MAP,
					    I_("PangoXFontMap"),
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
pango_x_font_map_class_init (PangoFontMapClass *class)
{
  GObjectClass *object_class = G_OBJECT_CLASS (class);

  font_map_parent_class = g_type_class_peek_parent (class);

  object_class->finalize = pango_x_font_map_finalize;
  class->load_font = pango_x_font_map_load_font;
  class->list_families = pango_x_font_map_list_families;
  class->shape_engine_type = PANGO_RENDER_TYPE_X;
}

/*
 * Hackery to set up notification when a Display is closed
 */
static GSList *registered_displays;

static int
close_display_cb (Display   *display,
		  XExtCodes *extcodes)
{
  pango_x_shutdown_display (display);
  registered_displays = g_slist_remove (registered_displays, display);

  return 0;
}

static void
register_display (Display *display)
{
  XExtCodes *extcodes;
  GSList *tmp_list;

  for (tmp_list = registered_displays; tmp_list; tmp_list = tmp_list->next)
    {
      if (tmp_list->data == display)
	return;
    }

  registered_displays = g_slist_prepend (registered_displays, display);

  extcodes = XAddExtension (display);
  XESetCloseDisplay (display, extcodes->extension, close_display_cb);
}

static GList *fontmaps = NULL;

/**
 * pango_x_font_map_for_display:
 * @display: an X #Display.
 *
 * Returns a #PangoXFontMap for @display. Font maps are cached and should
 * not be freed. If the font map for a display is no longer needed, it can
 * be released with pango_x_shutdown_display().
 *
 * Return value: a #PangoXFontMap for @display.
 **/
PangoFontMap *
pango_x_font_map_for_display (Display *display)
{
  PangoXFontMap *xfontmap;
  GList *tmp_list = fontmaps;
  char **xfontnames;
  int num_fonts, i;
  int screen;
  static gboolean registered_modules = FALSE;

  g_return_val_if_fail (display != NULL, NULL);

  if (!registered_modules)
    {
      registered_modules = TRUE;

      for (i = 0; _pango_included_x_modules[i].list; i++)
	pango_module_register (&_pango_included_x_modules[i]);
    }

  /* Make sure that the type system is initialized */
  g_type_init ();

  while (tmp_list)
    {
      xfontmap = tmp_list->data;

      if (xfontmap->display == display)
	return PANGO_FONT_MAP (xfontmap);

      tmp_list = tmp_list->next;
    }

  xfontmap = g_object_new (PANGO_TYPE_X_FONT_MAP, NULL);

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

  register_display (xfontmap->display);

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
	  g_object_unref (xfontmap);

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

  /* FIXME: None of these hashtables free their key/values
  g_hash_table_destroy (xfontmap->families);
  g_hash_table_destroy (xfontmap->size_infos);
  g_hash_table_destroy (xfontmap->to_atom_cache);
  g_hash_table_destroy (xfontmap->from_atom_cache);
  */

  fontmaps = g_list_remove (fontmaps, xfontmap);

  G_OBJECT_CLASS (font_map_parent_class)->finalize (object);
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
pango_x_font_map_list_families (PangoFontMap           *fontmap,
				PangoFontFamily      ***families,
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

static PangoXFamily *
pango_x_get_font_family (PangoXFontMap *xfontmap,
			 const char    *family_name)
{
  PangoXFamily *font_family = g_hash_table_lookup (xfontmap->families, family_name);
  if (!font_family)
    {
      font_family = g_object_new (PANGO_X_TYPE_FAMILY, NULL);
      font_family->family_name = g_strdup (family_name);
      font_family->font_entries = NULL;

      g_hash_table_insert (xfontmap->families, font_family->family_name, font_family);
    }

  return font_family;
}

static PangoFont *
pango_x_font_map_load_font (PangoFontMap               *fontmap,
			    PangoContext               *context,
			    const PangoFontDescription *description)
{
  PangoXFontMap *xfontmap = (PangoXFontMap *)fontmap;
  PangoXFamily *font_family;
  PangoFont *result = NULL;
  GSList *tmp_list;
  gchar *name;
  gint size;

  g_return_val_if_fail (description != NULL, NULL);

  name = g_ascii_strdown (pango_font_description_get_family (description), -1);
  size = pango_font_description_get_size (description);

  if (size < 0)
    return NULL;

  font_family = g_hash_table_lookup (xfontmap->families, name);
  if (font_family)
    {
      PangoXFace *best_match = NULL;

      tmp_list = font_family->font_entries;
      while (tmp_list)
	{
	  PangoXFace *font_entry = tmp_list->data;

	  if (pango_font_description_better_match (description,
						   best_match ? best_match->description : NULL,
						   font_entry->description))
	    best_match = font_entry;

	  tmp_list = tmp_list->next;
	}

      if (best_match)
	{
	  GSList *tmp_list = best_match->cached_fonts;

	  while (tmp_list)
	    {
	      PangoXFont *xfont = tmp_list->data;
	      if (xfont->size == size)
		{
		  result = (PangoFont *)xfont;

		  g_object_ref (result);
		  if (xfont->in_cache)
		    pango_x_fontmap_cache_remove (fontmap, xfont);

		  break;
		}
	      tmp_list = tmp_list->next;
	    }

	  if (!result)
	    {
	      PangoXFont *xfont = pango_x_font_new (fontmap, best_match->xlfd, size);

	      xfont->xface = best_match;
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
 * set this error handler while accessing it. The error_occurred
 * global allows us to tell whether an error occurred for
 * XChangeProperty
 */
static gboolean error_occurred;

static int
ignore_error (Display     *d,
	      XErrorEvent *e)
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
  guchar *data;
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
		      &data);

  if (type == XA_WINDOW)
    {
      if (format == 32 && n_items == 1 && bytes_after == 0)
	retval = *(Atom *)data;

      XFree (data);
    }

  old_handler= XSetErrorHandler (ignore_error);

  if (XGetWindowProperty (display,
			  retval,
			  coverage_win_atom,
			  0, 4,
			  False, XA_WINDOW,
			  &type, &format, &n_items, &bytes_after,
			  &data) == Success &&
      type == XA_WINDOW)
    {
      if (format != 32 || n_items != 1 || bytes_after != 0 ||
	  *(Atom *)data != retval)
	retval = None;

      XFree (data);
    }
  else
    retval = None;

  XSync (display, False);
  XSetErrorHandler (old_handler);

  return retval;
}

/* Find or create the persistent window for caching font coverage
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
			   InputOnly, (Visual *)CopyFromParent,
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
      error_occurred = False;

      XChangeProperty (xfontmap->display,
		       coverage_win,
		       atom,
		       XA_STRING,
		       8, PropModeReplace,
		       bytes, size);

      XSync (xfontmap->display, False);
      XSetErrorHandler (old_handler);

      if (!error_occurred)
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
  PangoXFace *xface = NULL;

  infile = fopen (filename, "r");
  if (infile)
    {
      GString *line_buf = g_string_new (NULL);
      GString *tmp_buf = g_string_new (NULL);
      gint lines_read;

      while ((lines_read = pango_read_line (infile, line_buf)))
	{
	  PangoXFamily *font_family;
	  PangoStyle style;
	  PangoVariant variant;
	  PangoWeight weight;
	  PangoStretch stretch;

	  const char *p = line_buf->str;

	  lineno += lines_read;

	  if (!pango_skip_space (&p))
	    continue;

	  if (!pango_scan_string (&p, tmp_buf))
	    goto error;

	  xface = g_object_new (PANGO_X_TYPE_FACE, NULL);
	  xface->xlfd = NULL;
	  xface->description = pango_font_description_new ();

	  g_string_ascii_down (tmp_buf);
	  pango_font_description_set_family (xface->description, tmp_buf->str);

	  if (!pango_scan_string (&p, tmp_buf))
	    goto error;

	  if (!pango_parse_style (tmp_buf->str, &style, TRUE))
	    goto error;
	  pango_font_description_set_style (xface->description, style);

	  if (!pango_scan_string (&p, tmp_buf))
	    goto error;

	  if (!pango_parse_variant (tmp_buf->str, &variant, TRUE))
	    goto error;
	  pango_font_description_set_variant (xface->description, variant);

	  if (!pango_scan_string (&p, tmp_buf))
	    goto error;

	  if (!pango_parse_weight (tmp_buf->str, &weight, TRUE))
	    goto error;
	  pango_font_description_set_weight (xface->description, weight);

	  if (!pango_scan_string (&p, tmp_buf))
	    goto error;

	  if (!pango_parse_stretch (tmp_buf->str, &stretch, TRUE))
	    goto error;
	  pango_font_description_set_stretch (xface->description, stretch);

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

	  xface->xlfd = g_strjoinv (",", xlfds);
	  g_strfreev (xlfds);

	  /* Insert the font entry into our structures */

	  font_family = pango_x_get_font_family (xfontmap,
						 pango_font_description_get_family (xface->description));
	  font_family->font_entries = g_slist_prepend (font_family->font_entries, xface);
	  xfontmap->n_fonts++;

	  /* Save space by consolidating duplicated string */
	  pango_font_description_set_family_static (xface->description, font_family->family_name);
	  xface->cached_fonts = NULL;
	  xface->coverage = NULL;
	}

      if (ferror (infile))
	g_warning ("Error reading '%s': %s", filename, g_strerror(errno));

      goto out;

    error:
      if (xface)
	{
	  g_free (xface->xlfd);
	  if (xface->description)
	    pango_font_description_free (xface->description);
	  g_free (xface);
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
 * Returns %TRUE if the fontname is a valid XLFD.
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
 * Description name, and returns it. If fontname is %NULL or the field is
 * longer than XFLD_MAX_FIELD_LEN it returns %NULL.
 * Note: For the charset field, we also return the encoding, e.g. 'iso8859-1'.
 */
static char*
pango_x_get_xlfd_field (const char *fontname,
			FontField   field_num,
			char       *buffer)
{
  const char *t1, *t2;
  char *p;
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
      for (p = buffer; *p; p++)
	*p = g_ascii_tolower (*p);
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
		     const char    *fontname)
{
  PangoFontDescription *description;
  char *family_name;
  PangoStyle style;
  PangoVariant variant;
  PangoWeight weight;
  PangoStretch stretch;
  char family_buffer[XLFD_MAX_FIELD_LEN];
  char weight_buffer[XLFD_MAX_FIELD_LEN];
  char slant_buffer[XLFD_MAX_FIELD_LEN];
  char set_width_buffer[XLFD_MAX_FIELD_LEN];
  GSList *tmp_list;
  PangoXFamily *font_family;
  PangoXFace *xface;
  PangoXSizeInfo *size_info;
  char *identifier;
  unsigned int i;

  /* First insert the XLFD into the list of XLFDs for the "identifier" - which
   * is the 2-4th fields of the XLFD
   */
  identifier = pango_x_get_identifier (fontname);
  size_info = g_hash_table_lookup (xfontmap->size_infos, identifier);
  if (!size_info)
    {
      size_info = g_slice_new (PangoXSizeInfo);
      size_info->identifier = identifier;
      size_info->xlfds = NULL;

      g_hash_table_insert (xfontmap->size_infos, identifier, size_info);
    }
  else
    g_free (identifier);

  size_info->xlfds = g_slist_prepend (size_info->xlfds, g_strdup (fontname));

  /* Convert the XLFD into a PangoFontDescription */

  family_name = pango_x_get_xlfd_field (fontname, XLFD_FAMILY, family_buffer);
  if (!family_name)
    return;

  style = PANGO_STYLE_NORMAL;
  if (pango_x_get_xlfd_field (fontname, XLFD_SLANT, slant_buffer))
    {
      for (i=0; i<G_N_ELEMENTS(styles_map); i++)
	{
	  if (!strcmp (styles_map[i].text, slant_buffer))
	    {
	      style = styles_map[i].value;
	      break;
	    }
	}
    }
  else
    strcpy (slant_buffer, "*");

  variant = PANGO_VARIANT_NORMAL;

  weight = PANGO_WEIGHT_NORMAL;
  if (pango_x_get_xlfd_field (fontname, XLFD_WEIGHT, weight_buffer))
    {
      for (i=0; i<G_N_ELEMENTS(weights_map); i++)
	{
	  if (!strcmp (weights_map[i].text, weight_buffer))
	    {
	      weight = weights_map[i].value;
	      break;
	    }
	}
    }
  else
    strcpy (weight_buffer, "*");

  stretch = PANGO_STRETCH_NORMAL;
  if (pango_x_get_xlfd_field (fontname, XLFD_SET_WIDTH, set_width_buffer))
    {
      for (i=0; i<G_N_ELEMENTS(stretches_map); i++)
	{
	  if (!strcmp (stretches_map[i].text, set_width_buffer))
	    {
	      stretch = stretches_map[i].value;
	      break;
	    }
	}
    }
  else
    strcpy (set_width_buffer, "*");

  font_family = pango_x_get_font_family (xfontmap, family_name);

  tmp_list = font_family->font_entries;
  while (tmp_list)
    {
      xface = tmp_list->data;

      if (pango_font_description_get_style (xface->description) == style &&
	  pango_font_description_get_weight (xface->description) == weight &&
	  pango_font_description_get_stretch (xface->description) == stretch &&
	  pango_font_description_get_variant (xface->description) == variant)
	return;

      tmp_list = tmp_list->next;
    }

  description = pango_font_description_new ();
  pango_font_description_set_family_static (description, font_family->family_name);
  pango_font_description_set_style (description, style);
  pango_font_description_set_weight (description, weight);
  pango_font_description_set_stretch (description, stretch);
  pango_font_description_set_variant (description, variant);

  xface = g_object_new (PANGO_X_TYPE_FACE, NULL);
  xface->description = description;
  xface->cached_fonts = NULL;
  xface->coverage = NULL;

  xface->xlfd = g_strconcat ("-*-",
				  family_buffer,
				  "-",
				  weight_buffer,
				  "-",
				  slant_buffer,
				  "-",
				  set_width_buffer,
				  "--*-*-*-*-*-*-*-*",
				  NULL);

  font_family->font_entries = g_slist_append (font_family->font_entries, xface);
  xfontmap->n_fonts++;
}

/* Compare the tail of a to b */
static gboolean
match_end (const char *a, const char *b)
{
  size_t len_a = strlen (a);
  size_t len_b = strlen (b);

  if (len_b > len_a)
    return FALSE;
  else
    return (strcmp (a + len_a - len_b, b) == 0);
}

/* Given a XLFD, charset and size, find the best matching installed X font.
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

/**
 * pango_x_font_map_get_font_cache:
 * @font_map: a #PangoXFontMap.
 *
 * Obtains the font cache associated with the given font map.
 *
 * Return value: the #PangoXFontCache of @font_map.
 **/
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
      g_object_unref (old_font);
    }

  g_object_ref (xfont);
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

  g_object_unref (xfont);
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


G_CONST_RETURN char *
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

/*
 * PangoXFace
 */

static PangoFontDescription *
pango_x_face_describe (PangoFontFace *face)
{
  PangoXFace *xface = PANGO_X_FACE (face);

  return pango_font_description_copy (xface->description);
}

static const char *
pango_x_face_get_face_name (PangoFontFace *face)
{
  PangoXFace *xface = PANGO_X_FACE (face);

  if (!xface->face_name)
    {
      PangoFontDescription *desc = pango_font_face_describe (face);

      pango_font_description_unset_fields (desc,
					   PANGO_FONT_MASK_FAMILY | PANGO_FONT_MASK_SIZE);

      xface->face_name = pango_font_description_to_string (desc);
      pango_font_description_free (desc);
    }

  return xface->face_name;
}

static void
pango_x_face_class_init (PangoFontFaceClass *class)
{
  class->describe = pango_x_face_describe;
  class->get_face_name = pango_x_face_get_face_name;
}

GType
pango_x_face_get_type (void)
{
  static GType object_type = 0;

  if (G_UNLIKELY (!object_type))
    {
      const GTypeInfo object_info =
      {
	sizeof (PangoFontFaceClass),
	(GBaseInitFunc) NULL,
	(GBaseFinalizeFunc) NULL,
	(GClassInitFunc) pango_x_face_class_init,
	NULL,           /* class_finalize */
	NULL,           /* class_data */
	sizeof (PangoXFace),
	0,              /* n_preallocs */
	(GInstanceInitFunc) NULL,
	NULL            /* value_table */
      };

      object_type = g_type_register_static (PANGO_TYPE_FONT_FACE,
					    I_("PangoXFace"),
					    &object_info, 0);
    }

  return object_type;
}

/* Cut and paste here to avoid an inter-module dependency */
static PangoCoverageLevel
engine_shape_covers (PangoEngineShape *engine,
		     PangoFont        *font,
		     PangoLanguage    *language,
		     gunichar          wc)
{
  g_return_val_if_fail (PANGO_IS_ENGINE_SHAPE (engine), PANGO_COVERAGE_NONE);
  g_return_val_if_fail (PANGO_IS_FONT (font), PANGO_COVERAGE_NONE);

  return PANGO_ENGINE_SHAPE_GET_CLASS (engine)->covers (engine,
							font,
							language,
							wc);
}

PangoCoverage *
pango_x_face_get_coverage (PangoXFace      *xface,
			   PangoFont       *font,
			   PangoLanguage   *language)
{
  PangoXFont *xfont;
  PangoXFontMap *xfontmap = NULL; /* Quiet gcc */
  PangoCoverage *result = NULL;
  Atom atom = None;

  if (xface)
    {
      if (xface->coverage)
	{
	  pango_coverage_ref (xface->coverage);
	  return xface->coverage;
	}

      xfont = (PangoXFont *)font;

      xfontmap = (PangoXFontMap *)pango_x_font_map_for_display (xfont->display);
      if (xface->xlfd)
	{
	  const char *lang_str = language ? pango_language_to_string (language) : "*";

	  char *str = g_strconcat (lang_str, "|", xface->xlfd, NULL);
	  result = pango_x_get_cached_coverage (xfontmap, str, &atom);
	  g_free (str);
	}
    }

  if (!result)
    {
      PangoMap *shape_map;
      PangoEngineShape *engine;
      gunichar wc;

      result = pango_coverage_new ();

      shape_map = pango_x_get_shaper_map (language);
      engine = (PangoEngineShape *)pango_map_get_engine (shape_map, PANGO_SCRIPT_COMMON);

      for (wc = 0; wc < 65536; wc++)
	{
	  PangoCoverageLevel level;

	  level = engine_shape_covers (engine, font, language, wc);
	  if (level != PANGO_COVERAGE_NONE)
	    pango_coverage_set (result, wc, level);
	}

      if (atom)
	pango_x_store_cached_coverage (xfontmap, atom, result);
    }

  if (xface)
    {
      xface->coverage = result;
      pango_coverage_ref (result);
    }

  return result;
}

void
pango_x_face_remove (PangoXFace  *xface,
		     PangoFont   *font)
{
  xface->cached_fonts = g_slist_remove (xface->cached_fonts, font);
}

/*
 * PangoXFontFamily
 */

static void
pango_x_family_list_faces (PangoFontFamily  *family,
			   PangoFontFace  ***faces,
			   int              *n_faces)
{
  PangoXFamily *xfamily = PANGO_X_FAMILY (family);

  *n_faces = g_slist_length (xfamily->font_entries);
  if (faces)
    {
      GSList *tmp_list;
      int i = 0;

      *faces = g_new (PangoFontFace *, *n_faces);

      tmp_list = xfamily->font_entries;
      while (tmp_list)
	{
	  (*faces)[i++] = tmp_list->data;
	  tmp_list = tmp_list->next;
	}
    }
}

G_CONST_RETURN char *
pango_x_family_get_name (PangoFontFamily  *family)
{
  PangoXFamily *xfamily = PANGO_X_FAMILY (family);

  return xfamily->family_name;
}

static void
pango_x_family_class_init (PangoFontFamilyClass *class)
{
  class->list_faces = pango_x_family_list_faces;
  class->get_name = pango_x_family_get_name;
}

GType
pango_x_family_get_type (void)
{
  static GType object_type = 0;

  if (G_UNLIKELY (!object_type))
    {
      const GTypeInfo object_info =
      {
	sizeof (PangoFontFamilyClass),
	(GBaseInitFunc) NULL,
	(GBaseFinalizeFunc) NULL,
	(GClassInitFunc) pango_x_family_class_init,
	NULL,           /* class_finalize */
	NULL,           /* class_data */
	sizeof (PangoXFamily),
	0,              /* n_preallocs */
	(GInstanceInitFunc) NULL,
	NULL            /* value_table */
      };

      object_type = g_type_register_static (PANGO_TYPE_FONT_FAMILY,
					    I_("PangoXFamily"),
					    &object_info, 0);
    }

  return object_type;
}

