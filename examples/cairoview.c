/* Pango
 * cairoview.c: Example program to view a UTF-8 encoding file
 *              using PangoCairo to render result
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

#include <pango/pangocairo.h>
#include <cairo-xlib.h>

static void
render_callback (PangoLayout *layout,
		 int          x,
		 int          y,
		 gpointer     data,
		 gboolean     show_borders)
{
  cairo_t *cr = (cairo_t *)data;

  cairo_move_to (cr, x, y);
  pango_cairo_show_layout (cr, layout);

  if (show_borders)
    {
      PangoRectangle ink, logical;
      double lw = cairo_get_line_width (cr);
      
      pango_layout_get_extents (layout, &ink, &logical);

      cairo_save (cr);
      cairo_set_source_rgba (cr, 1.0, 0.0, 0.0, 0.75);

      cairo_rectangle (cr,
		       (double)logical.x / PANGO_SCALE - lw / 2,
		       (double)logical.y / PANGO_SCALE - lw / 2,
		       (double)logical.width / PANGO_SCALE + lw,
		       (double)logical.height / PANGO_SCALE + lw);
      cairo_stroke (cr);
      
      cairo_set_source_rgba (cr, 0.0, 1.0, 0.0, 0.75);

      cairo_rectangle (cr,
		       (double)ink.x / PANGO_SCALE - lw / 2,
		       (double)ink.y / PANGO_SCALE - lw / 2,
		       (double)ink.width / PANGO_SCALE + lw,
		       (double)ink.height / PANGO_SCALE + lw);
      cairo_stroke (cr);

      cairo_restore (cr);
    }
}

static void
transform_callback (PangoContext *context,
		    PangoMatrix  *matrix,
		    gpointer      data)
{
  cairo_t *cr = (cairo_t *)data;
  cairo_matrix_t cairo_matrix;

  if (matrix)
    {
      cairo_matrix.xx = matrix->xx;
      cairo_matrix.yx = matrix->yx;
      cairo_matrix.xy = matrix->xy;
      cairo_matrix.yy = matrix->yy;
      cairo_matrix.x0 = matrix->x0;
      cairo_matrix.y0 = matrix->y0;
    }
  else
    {
      cairo_matrix_init_identity (&cairo_matrix);
    }

  cairo_set_matrix (cr, &cairo_matrix);
      
  pango_context_set_matrix (context, matrix);
  pango_cairo_update_context (cr, context);
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
  PangoFontMap *fontmap;
  cairo_t *cr;
  cairo_surface_t *surface;
  fontmap = pango_cairo_font_map_get_default ();
  pango_cairo_font_map_set_resolution (PANGO_CAIRO_FONT_MAP (fontmap), dpi);
  *context = pango_cairo_font_map_create_context (PANGO_CAIRO_FONT_MAP (fontmap));

  /* This is annoying ... we have to create a temporary surface just to
   * get the extents of the text.
   */
  surface = cairo_image_surface_create (CAIRO_FORMAT_RGB24, 1, 1);
  cr = cairo_create (surface);
  cairo_surface_destroy (surface);
  do_output (*context, NULL, transform_callback, cr, width, height, FALSE);
  cairo_destroy (cr);
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
  cairo_t *cr;
  cairo_surface_t *surface;

  surface = cairo_xlib_surface_create (display, pixmap,
				       DefaultVisual (display, screen),
				       width, height);

  cr = cairo_create (surface);
  cairo_surface_destroy (surface);
  
  transform_callback (context, NULL, cr);

  cairo_set_source_rgb (cr, 1.0, 1.0, 1.0);
  cairo_paint (cr);
  
  /* Draw the text in black */
  cairo_set_source_rgb (cr, 0.0, 0.0, 0.0);
  do_output (context, render_callback, transform_callback, cr, NULL, NULL, show_borders);

  cairo_destroy (cr);
}
