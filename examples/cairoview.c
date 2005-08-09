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

#include <X11/Xlib.h>
#include <X11/Xutil.h>

#include "renderdemo.h"

#include <pango/pangocairo.h>
#include <cairo-xlib.h>

static Region update_region = NULL;
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
  XRectangle extents;
  int width, height;

  /* Create a temporary pixmap and a Cairo context pointing to it */
  XClipBox (update_region, &extents);

  width = extents.width;
  height = extents.height;

  pixmap = XCreatePixmap (display, window, width, height,
			  DefaultDepth (display, screen));

  surface = cairo_xlib_surface_create (display, pixmap,
				       DefaultVisual (display, screen),
				       width, height);
							      

  cr = render_data.cr = cairo_create (surface);
  cairo_surface_destroy (surface);
  
  render_data.x_offset = - extents.x;
  render_data.y_offset = - extents.y;

  do_cairo_transform (context, NULL, &render_data);

  /* Clip to the current update region and fill with white */
  cairo_rectangle (cr, extents.x, extents.y, extents.width, extents.height);
  
  cairo_clip (cr);
  cairo_set_source_rgb (cr, 1.0, 1.0, 1.0);
  cairo_paint (cr);
  
  /* Draw the text in black */
  cairo_set_source_rgb (cr, 0.0, 0.0, 0.0);
  do_output (context, do_cairo_render, do_cairo_transform, &render_data, NULL, NULL);
  cairo_destroy (cr);

  /* Copy the updated area onto the window */
  gc = XCreateGC (display, pixmap, 0, NULL);

  XCopyArea (display, pixmap, window, gc,
	     0, 0,
	     extents.width, extents.height, extents.x, extents.y);

  XFreeGC (display, gc);
  XFreePixmap (display, pixmap);
	     
  XDestroyRegion (update_region);
  update_region = NULL;
}

void
expose (XExposeEvent *xev)
{
  XRectangle  r;
  if (!update_region)
    update_region = XCreateRegion ();

  r.x = xev->x;
  r.y = xev->y;
  r.width = xev->width;
  r.height = xev->height;
  XUnionRectWithRegion (&r, update_region, update_region);
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
	    XRectangle  r;
	    show_borders = !show_borders;

	    if (!update_region)
	      update_region = XCreateRegion ();

	    r.x = 0;
	    r.y = 0;
	    r.width = width;
	    r.height = height;
	    XUnionRectWithRegion (&r, update_region, update_region);
	  }
	break;
      case Expose:
	expose (&xev.xexpose);
	break;
      }
    }

done:

  g_object_unref (context);
  finalize ();

  return 0;
}
