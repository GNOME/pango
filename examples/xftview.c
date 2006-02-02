/* Pango
 * xftview.c: Example program to view a UTF-8 encoding file
 *            using PangoXft to render result
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

#include "renderdemo.h"
#include "viewer-x.h"

#include <pango/pangoxft.h>

static void
default_substitute (FcPattern *pattern,
		    gpointer data)
{
  int dpi = GPOINTER_TO_INT (data);
  FcPatternAddInteger (pattern, FC_DPI, dpi);
}

static void
render_callback (PangoLayout *layout,
		 int          x,
		 int          y,
		 gpointer     data,
		 gboolean     show_borders)
{
  XftDraw *draw = (XftDraw *)data;
  XftColor color;

  color.color.red = 0x0;
  color.color.green = 0x0;
  color.color.blue = 0x0;
  color.color.alpha = 0xffff;

  pango_xft_render_layout (draw, &color, layout, x, y);

  /*
  if (show_borders)
    {
      PangoContext *context;
      PangoXftFontMap *fontmap;
      PangoRenderer *renderer;
      PangoRectangle ink, logical;
      
      pango_layout_get_extents (layout, &ink, &logical);

      context = pango_layout_get_context (layout);
      fontmap = (PangoXftFontMap *)pango_context_get_font_map (context);

      humm, we cannot go on, as the following is private api.
      need to implement pango_font_map_get_renderer...

      renderer = _pango_xft_font_map_get_renderer (fontmap);
    }
  */
}

void
do_init (Display *display,
	 int screen,
	 int dpi,
	 /* output */
	 PangoContext **context,
	 int *width,
	 int *height)
{
  XftInit (NULL);
  *context = pango_xft_get_context (display, screen);
  pango_xft_set_default_substitute (display, screen, default_substitute, GINT_TO_POINTER (dpi), NULL);
  do_output (*context, NULL, NULL, NULL, width, height, FALSE);
}

void
do_render (Display *display,
	   int screen,
	   Window window,
	   Pixmap pixmap,
	   PangoContext *context,
	   int width,
	   int height,
	   gboolean show_borders)
{
  XftDraw *draw;
  XftColor color;

  draw = XftDrawCreate (display, pixmap,
			DefaultVisual (display, screen),
			DefaultColormap (display, screen));

  color.color.red = 0xffff;
  color.color.blue = 0xffff;
  color.color.green = 0xffff;
  color.color.alpha = 0xffff;

  XftDrawRect (draw, &color, 0, 0, width, height);

  do_output (context, render_callback, NULL, draw, NULL, NULL, show_borders);

  XftDrawDestroy (draw);
}
