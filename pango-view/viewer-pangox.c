/* viewer-pangox.c: PangoX viewer backend.
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

#include <pango/pangox.h>

static void
pangox_view_destroy (gpointer instance)
{
  XViewer *x = (XViewer *)instance;

  pango_x_shutdown_display (x->display);

  x_view_destroy (instance);
}

static PangoContext *
pangox_view_get_context (gpointer instance)
{
  XViewer *x = (XViewer *) instance;
  PangoContext *context;
  PangoMatrix matrix = {0., 0., 0., 0., 0., 0.};

  context = pango_font_map_create_context (pango_x_font_map_for_display (x->display));

  /* We set an all-zero matrix on the context, to negotiate that
   * this backend doesn't support transformations.
   */
  pango_context_set_matrix (context, &matrix);

  return context;
}

typedef struct
{
  XViewer *x;
  Drawable drawable;
  GC gc;
} MyXContext;

static void
render_callback (PangoLayout *layout,
		 int          x,
		 int          y,
		 gpointer     context,
		 gpointer     state)
{
  MyXContext *x_context = (MyXContext *) context;

  pango_x_render_layout (x_context->x->display,
			 x_context->drawable,
			 x_context->gc,
			 layout,
			 x, y);
}

static void
pangox_view_render (gpointer      instance,
		    gpointer      surface,
		    PangoContext *context,
		    int           width,
		    int           height,
		    gpointer      state)
{
  XViewer *x = (XViewer *) instance;
  Pixmap pixmap = (Pixmap) surface;
  GC gc;
  MyXContext x_context;

  gc = XCreateGC (x->display, pixmap, 0, NULL);

  XSetForeground(x->display, gc, WhitePixel(x->display, x->screen));
  XFillRectangle (x->display, pixmap, gc, 0, 0, width, height);

  x_context.x = x;
  x_context.drawable = pixmap;
  x_context.gc = gc;

  XSetForeground(x->display, gc, BlackPixel(x->display, x->screen));
  do_output (context, render_callback, NULL, &x_context, state, NULL, NULL);

  XFlush(x->display);

  XFreeGC (x->display, gc);
}

const PangoViewer pangox_viewer = {
  "PangoX",
  "x",
  NULL,
  x_view_create,
  pangox_view_destroy,
  pangox_view_get_context,
  x_view_create_surface,
  x_view_destroy_surface,
  pangox_view_render,
  NULL,
  x_view_create_window,
  x_view_destroy_window,
  x_view_display
};
