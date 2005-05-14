/* Pango
 * cairoview.c: Example program to view a UTF-8 encoding file
 *              using Cairo to render result
 *
 * Copyright (C) 2005 Red Hat, Inc.
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

#include <errno.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>

#include <X11/Xutil.h>

#include "renderdemo.h"

#include <pango/pangocairo.h>
#include <cairo-xlib.h>
#include <pixman.h>

static pixman_region16_t *update_region = NULL;
static PangoContext *context;
static Display *display;
int screen;
static Window window;
gboolean show_borders;

typedef struct
{
  cairo_t *cr;
  int x_offset;
  int y_offset;
} RenderData;

static void
do_cairo_render (PangoLayout *layout,
		 int          x,
		 int          y,
		 gpointer     data)
{
  RenderData *render_data = data;
  cairo_t *cr = render_data->cr;

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
do_cairo_transform (PangoContext *context,
		    PangoMatrix  *matrix,
		    gpointer      data)
{
  RenderData *render_data = data;
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

  cairo_matrix_translate (&cairo_matrix,
			  render_data->x_offset,
			  render_data->y_offset);

  cairo_set_matrix (render_data->cr, &cairo_matrix);
      
  pango_context_set_matrix (context, matrix);
  pango_cairo_update_context (render_data->cr, context);
}

void
update ()
{
  RenderData render_data;
  cairo_surface_t *surface;
  cairo_t *cr;
  Pixmap pixmap;
  GC gc;
  pixman_box16_t *extents;
  int n_rects;
  pixman_box16_t *rects;
  XRectangle *xrects;
  int i;

  /* Create a temporary pixmap and a Cairo context pointing to it */
  extents = pixman_region_extents (update_region);

  pixmap = XCreatePixmap (display, window,
			  extents->x2 - extents->x1,
			  extents->y2 - extents->y1,
			  DefaultDepth (display, screen));
  surface = cairo_xlib_surface_create_with_visual (display,
						   pixmap,
						   DefaultVisual (display, screen));
							      

  cr = render_data.cr = cairo_create (surface);
  cairo_surface_destroy (surface);
  
  render_data.x_offset = - extents->x1;
  render_data.y_offset = - extents->y1;

  do_cairo_transform (context, NULL, &render_data);

  /* Clip to the current update region and fill with white */
  n_rects = pixman_region_num_rects (update_region);
  rects = pixman_region_rects (update_region);
  xrects = g_new (XRectangle, n_rects);

  for (i = 0; i < n_rects; i++)
    {
      xrects[i].x = rects[i].x1;
      xrects[i].y = rects[i].y1;
      xrects[i].width = rects[i].x2 - rects[i].x1;
      xrects[i].height = rects[i].y2 - rects[i].y1;
      
      cairo_rectangle (cr, xrects[i].x, xrects[i].y,
		       xrects[i].width, xrects[i].height);
    }
  
  cairo_clip (cr);
  cairo_set_source_rgb (cr, 1.0, 1.0, 1.0);
  cairo_paint (cr);
  
  /* Draw the text in black */
  cairo_set_source_rgb (cr, 0.0, 0.0, 0.0);
  do_output (context, do_cairo_render, do_cairo_transform, &render_data, NULL, NULL);
  cairo_destroy (cr);

  /* Copy the updated area onto the window */
  gc = XCreateGC (display, pixmap, 0, NULL);
  XSetClipRectangles (display, gc, 0, 0, xrects, n_rects, YXBanded);

  XCopyArea (display, pixmap, window, gc,
	     0, 0,
	     extents->x2 - extents->x1, extents->y2 - extents->y1,
	     extents->x1, extents->y1);

  g_free (xrects);
  XFreeGC (display, gc);
  XFreePixmap (display, pixmap);
	     
  pixman_region_destroy (update_region);
  update_region = NULL;
}

void
expose (XExposeEvent *xev)
{
  if (!update_region)
    update_region = pixman_region_create ();

  pixman_region_union_rect (update_region, update_region,
			    xev->x, xev->y, xev->width, xev->height);
}

int main (int argc, char **argv)
{
  PangoFontMap *fontmap;
  XEvent xev;
  unsigned long bg;
  int width, height;
  XSizeHints size_hints;
  RenderData render_data;
  unsigned int quit_keycode;
  unsigned int borders_keycode;
  cairo_surface_t *surface;
  
  g_type_init();

  parse_options (argc, argv);

  display = XOpenDisplay (NULL);
  if (!display)
    fail ("Cannot open display %s\n", XDisplayName (NULL));
  screen = DefaultScreen (display);

  fontmap = pango_cairo_font_map_get_default ();
  context = pango_cairo_font_map_create_context (PANGO_CAIRO_FONT_MAP (fontmap));

  /* This is annoying ... we have to create a temporary surface just to
   * get the extents of the text.
   */
  surface = cairo_image_surface_create (CAIRO_FORMAT_RGB24, 1, 1);
  render_data.cr = cairo_create (surface);
  cairo_surface_destroy (surface);
  render_data.x_offset = 0;
  render_data.y_offset = 0;
  do_output (context, NULL, do_cairo_transform, &render_data, &width, &height);
  cairo_destroy (render_data.cr);

  bg = WhitePixel (display, screen);

  window = XCreateSimpleWindow (display, DefaultRootWindow (display),
				0, 0, width, height, 0,
				bg, bg);
  XSelectInput (display, window, ExposureMask | KeyPressMask);
  
  XMapWindow (display, window);
  XmbSetWMProperties (display, window,
		      get_options_string (),
		      NULL, NULL, 0, NULL, NULL, NULL);
  
  memset ((char *)&size_hints, 0, sizeof (XSizeHints));
  size_hints.flags = PSize | PMaxSize;
  size_hints.width = width; size_hints.height = height; /* for compat only */
  size_hints.max_width = width; size_hints.max_height = height;
  
  XSetWMNormalHints (display, window, &size_hints);

  borders_keycode = XKeysymToKeycode(display, 'B');
  quit_keycode = XKeysymToKeycode(display, 'Q');

  while (1)
    {
      if (!XPending (display) && update_region)
	update ();
	
      XNextEvent (display, &xev);
      switch (xev.xany.type) {
      case KeyPress:
	if (xev.xkey.keycode == quit_keycode)
	  goto done;
	else if (xev.xkey.keycode == borders_keycode)
	  {
	    show_borders = !show_borders;
	    if (!update_region)
	      update_region = pixman_region_create ();

	    pixman_region_union_rect (update_region, update_region,
				      0, 0, width, height);
	  }
	break;
      case Expose:
	expose (&xev.xexpose);
	break;
      }
    }

 done:
  return 0;
}
