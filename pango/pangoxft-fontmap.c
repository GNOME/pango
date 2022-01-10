/* Pango
 * pangoxft-fontmap.c: Xft font handling
 *
 * Copyright (C) 2000-2003 Red Hat Software
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
#include <stdlib.h>
#include <string.h>

#include "pangofc-fontmap-private.h"
#include "pangoxft.h"
#include "pangoxft-private.h"

/* For XExtSetCloseDisplay */
#include <X11/Xlibint.h>

typedef struct _PangoXftFamily       PangoXftFamily;
typedef struct _PangoXftFontMapClass PangoXftFontMapClass;

#define PANGO_TYPE_XFT_FONT_MAP              (pango_xft_font_map_get_type ())
#define PANGO_XFT_FONT_MAP(object)           (G_TYPE_CHECK_INSTANCE_CAST ((object), PANGO_TYPE_XFT_FONT_MAP, PangoXftFontMap))
#define PANGO_XFT_IS_FONT_MAP(object)        (G_TYPE_CHECK_INSTANCE_TYPE ((object), PANGO_TYPE_XFT_FONT_MAP))

struct _PangoXftFontMap
{
  PangoFcFontMap parent_instance;

  guint serial;

  Display *display;
  int screen;

  PangoRenderer *renderer;
};

struct _PangoXftFontMapClass
{
  PangoFcFontMapClass parent_class;
};

static guint         pango_xft_font_map_get_serial         (PangoFontMap         *fontmap);
static void          pango_xft_font_map_changed            (PangoFontMap         *fontmap);
static void          pango_xft_font_map_default_substitute (PangoFcFontMap       *fcfontmap,
							    FcPattern            *pattern);
static PangoFcFont * pango_xft_font_map_new_font           (PangoFcFontMap       *fcfontmap,
							    FcPattern            *pattern);
static void          pango_xft_font_map_finalize           (GObject              *object);

G_LOCK_DEFINE_STATIC (fontmaps);
static GSList *fontmaps = NULL; /* MT-safe */

G_DEFINE_TYPE (PangoXftFontMap, pango_xft_font_map, PANGO_TYPE_FC_FONT_MAP)

static void
pango_xft_font_map_class_init (PangoXftFontMapClass *class)
{
  GObjectClass *gobject_class = G_OBJECT_CLASS (class);
  PangoFontMapClass *fontmap_class = PANGO_FONT_MAP_CLASS (class);
  PangoFcFontMapClass *fcfontmap_class = PANGO_FC_FONT_MAP_CLASS (class);

  gobject_class->finalize  = pango_xft_font_map_finalize;

  fontmap_class->get_serial = pango_xft_font_map_get_serial;
  fontmap_class->changed = pango_xft_font_map_changed;

  fcfontmap_class->default_substitute = pango_xft_font_map_default_substitute;
  fcfontmap_class->new_font = pango_xft_font_map_new_font;
}

static void
pango_xft_font_map_init (PangoXftFontMap *xftfontmap)
{
  xftfontmap->serial = 1;
}

static void
pango_xft_font_map_finalize (GObject *object)
{
  PangoXftFontMap *xftfontmap = PANGO_XFT_FONT_MAP (object);

  if (xftfontmap->renderer)
    g_object_unref (xftfontmap->renderer);

  G_LOCK (fontmaps);
  fontmaps = g_slist_remove (fontmaps, object);
  G_UNLOCK (fontmaps);

  G_OBJECT_CLASS (pango_xft_font_map_parent_class)->finalize (object);
}


static guint
pango_xft_font_map_get_serial (PangoFontMap *fontmap)
{
  PangoXftFontMap *xftfontmap = PANGO_XFT_FONT_MAP (fontmap);

  return xftfontmap->serial;
}

static void
pango_xft_font_map_changed (PangoFontMap *fontmap)
{
  PangoXftFontMap *xftfontmap = PANGO_XFT_FONT_MAP (fontmap);

  xftfontmap->serial++;
  if (xftfontmap->serial == 0)
    xftfontmap->serial++;
}

static PangoFontMap *
pango_xft_find_font_map (Display *display,
			 int      screen)
{
  GSList *tmp_list;

  G_LOCK (fontmaps);
  tmp_list = fontmaps;
  while (tmp_list)
    {
      PangoXftFontMap *xftfontmap = tmp_list->data;

      if (xftfontmap->display == display &&
	  xftfontmap->screen == screen)
        {
          G_UNLOCK (fontmaps);
	  return PANGO_FONT_MAP (xftfontmap);
        }

      tmp_list = tmp_list->next;
    }
  G_UNLOCK (fontmaps);

  return NULL;
}

/*
 * Hackery to set up notification when a Display is closed
 */
static GSList *registered_displays; /* MT-safe, protected by fontmaps lock */

static int
close_display_cb (Display   *display,
		  XExtCodes *extcodes G_GNUC_UNUSED)
{
  GSList *tmp_list;

  G_LOCK (fontmaps);
  tmp_list = g_slist_copy (fontmaps);
  G_UNLOCK (fontmaps);

  while (tmp_list)
    {
      PangoXftFontMap *xftfontmap = tmp_list->data;
      tmp_list = tmp_list->next;

      if (xftfontmap->display == display)
	pango_xft_shutdown_display (display, xftfontmap->screen);
    }

  g_slist_free (tmp_list);

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

/**
 * pango_xft_get_font_map:
 * @display: an X display
 * @screen: the screen number of a screen within @display
 *
 * Returns the `PangoXftFontMap` for the given display and screen.
 * The fontmap is owned by Pango and will be valid until
 * the display is closed.
 *
 * Return value: (transfer none): a `PangoFontMap` object, owned by Pango.
 *
 * Since: 1.2
 **/
PangoFontMap *
pango_xft_get_font_map (Display *display,
			int      screen)
{
  PangoFontMap *fontmap;
  PangoXftFontMap *xftfontmap;

  g_return_val_if_fail (display != NULL, NULL);

  fontmap = pango_xft_find_font_map (display, screen);
  if (fontmap)
    return fontmap;

  xftfontmap = (PangoXftFontMap *)g_object_new (PANGO_TYPE_XFT_FONT_MAP, NULL);

  xftfontmap->display = display;
  xftfontmap->screen = screen;

  G_LOCK (fontmaps);

  register_display (display);

  fontmaps = g_slist_prepend (fontmaps, xftfontmap);

  G_UNLOCK (fontmaps);

  return PANGO_FONT_MAP (xftfontmap);
}

/**
 * pango_xft_shutdown_display:
 * @display: an X display
 * @screen: the screen number of a screen within @display
 *
 * Release any resources that have been cached for the
 * combination of @display and @screen. Note that when the
 * X display is closed, resources are released automatically,
 * without needing to call this function.
 *
 * Since: 1.2
 **/
void
pango_xft_shutdown_display (Display *display,
			    int      screen)
{
  PangoFontMap *fontmap;

  fontmap = pango_xft_find_font_map (display, screen);
  if (fontmap)
    {
      PangoXftFontMap *xftfontmap = PANGO_XFT_FONT_MAP (fontmap);

      G_LOCK (fontmaps);
      fontmaps = g_slist_remove (fontmaps, fontmap);
      G_UNLOCK (fontmaps);
      pango_fc_font_map_shutdown (PANGO_FC_FONT_MAP (fontmap));

      xftfontmap->display = NULL;
      g_object_unref (fontmap);
    }
}

void
_pango_xft_font_map_get_info (PangoFontMap *fontmap,
			      Display     **display,
			      int          *screen)
{
  PangoXftFontMap *xftfontmap = (PangoXftFontMap *)fontmap;

  if (display)
    *display = xftfontmap->display;
  if (screen)
    *screen = xftfontmap->screen;
}

/**
 * _pango_xft_font_map_get_renderer:
 * @fontmap: a `PangoXftFontMap`
 *
 * Gets the singleton `PangoXFTRenderer` for this fontmap.
 *
 * Return value: the renderer.
 **/
PangoRenderer *
_pango_xft_font_map_get_renderer (PangoXftFontMap *xftfontmap)
{
  if (!xftfontmap->renderer)
    xftfontmap->renderer = pango_xft_renderer_new (xftfontmap->display,
						   xftfontmap->screen);

  return xftfontmap->renderer;
}

static void
pango_xft_font_map_default_substitute (PangoFcFontMap *fcfontmap,
				       FcPattern      *pattern)
{
  PangoXftFontMap *xftfontmap = PANGO_XFT_FONT_MAP (fcfontmap);
  double d;

  FcConfigSubstitute (NULL, pattern, FcMatchPattern);
  if (fcfontmap->substitute_func)
    fcfontmap->substitute_func (pattern, fcfontmap->substitute_data);
  XftDefaultSubstitute (xftfontmap->display, xftfontmap->screen, pattern);
  if (FcPatternGetDouble (pattern, FC_PIXEL_SIZE, 0, &d) == FcResultMatch && d == 0.0)
    {
      FcValue v;
      v.type = FcTypeDouble;
      v.u.d = 1.0;
      FcPatternAdd (pattern, FC_PIXEL_SIZE, v, FcFalse);
    }
}

static PangoFcFont *
pango_xft_font_map_new_font (PangoFcFontMap  *fcfontmap,
			     FcPattern       *pattern)
{
  return (PangoFcFont *)_pango_xft_font_new (PANGO_XFT_FONT_MAP (fcfontmap), pattern);
}
