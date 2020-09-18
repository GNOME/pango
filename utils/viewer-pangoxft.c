/* viewer-pangoxft.c: PangoXft viewer backend.
 *
 * Copyright (C) 1999,2004,2005 Red Hat, Inc.
 * Copyright (C) 2001 Sun Microsystems
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

#include "viewer-render.h"
#include "viewer-x.h"

#include <pango/pangoxft.h>

static void
default_substitute (FcPattern *pattern,
		    gpointer   data G_GNUC_UNUSED)
{
  FcPatternDel (pattern, FC_DPI);
  FcPatternAddInteger (pattern, FC_DPI, opt_dpi);

  if (opt_hinting != HINT_DEFAULT)
    {
      FcPatternDel (pattern, FC_HINTING);
      FcPatternAddBool (pattern, FC_HINTING, opt_hinting != HINT_NONE);

      FcPatternDel (pattern, FC_AUTOHINT);
      FcPatternAddBool (pattern, FC_AUTOHINT, opt_hinting == HINT_AUTO);
    }
}

static gpointer
pangoxft_view_create (const PangoViewer *klass)
{
  XViewer *instance;

  instance = x_view_create (klass);

  XftInit (NULL);

  pango_fc_font_map_set_default_substitute (PANGO_FC_FONT_MAP (pango_xft_get_font_map (instance->display, instance->screen)),
				            default_substitute, NULL, NULL);

  return instance;
}

static void
pangoxft_view_destroy (gpointer instance)
{
  XViewer *x = (XViewer *)instance;

  pango_xft_shutdown_display (x->display, x->screen);

  x_view_destroy (instance);
}

static PangoContext *
pangoxft_view_get_context (gpointer instance)
{
  XViewer *x = (XViewer *) instance;

  return pango_font_map_create_context (pango_xft_get_font_map (x->display, x->screen));
}

typedef struct
{
  XftDraw *draw;
  XftColor color;
} MyXftContext;

static void
render_callback (PangoLayout *layout,
		 int          x,
		 int          y,
		 gpointer     context,
		 gpointer     state G_GNUC_UNUSED)
{
  MyXftContext *xft_context = (MyXftContext *) context;

  pango_xft_render_layout (xft_context->draw,
			   &xft_context->color,
			   layout,
			   x * PANGO_SCALE, y * PANGO_SCALE);
}

static void
pangoxft_view_render (gpointer      instance,
		      gpointer      surface,
		      PangoContext *context,
		      int          *width,
		      int          *height,
		      gpointer      state)
{
  XViewer *x = (XViewer *) instance;
  Pixmap pixmap = (Pixmap) surface;
  MyXftContext xft_context;
  XftDraw *draw;
  XftColor color;

  draw = XftDrawCreate (x->display, pixmap,
			DefaultVisual (x->display, x->screen),
			DefaultColormap (x->display, x->screen));

  /* XftDrawRect only fills solid.
   * Flatten with white.
   */
  color.color.red = ((opt_bg_color.red * opt_bg_alpha) >> 16) + (65535 - opt_bg_alpha);
  color.color.green = ((opt_bg_color.green * opt_bg_alpha) >> 16) + (65535 - opt_bg_alpha);
  color.color.blue = ((opt_bg_color.blue * opt_bg_alpha) >> 16) + (65535 - opt_bg_alpha);
  color.color.alpha = 65535;

  XftDrawRect (draw, &color, 0, 0, *width, *height);

  color.color.red = opt_fg_color.red;
  color.color.blue = opt_fg_color.green;
  color.color.green = opt_fg_color.blue;
  color.color.alpha = opt_fg_alpha;

  xft_context.draw = draw;
  xft_context.color = color;

  do_output (context, render_callback, NULL, &xft_context, state, width, height);

  XftDrawDestroy (draw);
}

const PangoViewer pangoxft_viewer = {
  "PangoXft",
  "xft",
  NULL,
  pangoxft_view_create,
  pangoxft_view_destroy,
  pangoxft_view_get_context,
  x_view_create_surface,
  x_view_destroy_surface,
  pangoxft_view_render,
  NULL,
  x_view_create_window,
  x_view_destroy_window,
  x_view_display
};
