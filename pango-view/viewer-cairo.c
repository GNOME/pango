/* viewer-cairo.c: Common code for Cairo-based viewers
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

#include "viewer-cairo.h"
#include "viewer-render.h"

#include <cairo.h>



#ifdef HAVE_CAIRO_XLIB
#include "viewer-x.h"
#include <cairo-xlib.h>

static cairo_surface_t *
cairo_x_view_iface_create_surface (gpointer instance,
				   gpointer surface,
				   int      width,
				   int      height)
{
  XViewer *x = (XViewer *)instance;
  Drawable drawable = (Drawable) surface;

  return cairo_xlib_surface_create (x->display, drawable,
				    DefaultVisual (x->display, x->screen),
				    width, height);
}

static CairoViewerIface cairo_x_viewer_iface = {
  &x_viewer,
  cairo_x_view_iface_create_surface
};
#endif /* HAVE_CAIRO_XLIB */



static gpointer
cairo_image_view_create (const PangoViewer *klass G_GNUC_UNUSED)
{
  return NULL;
}

static void
cairo_image_view_destroy (gpointer instance G_GNUC_UNUSED)
{
}

static gpointer
cairo_image_view_create_surface (gpointer instance,
				 int      width,
				 int      height)
{
  /* TODO: Be smarter about format? */
  return cairo_image_surface_create (CAIRO_FORMAT_ARGB32, width, height);
}

static void
cairo_image_view_destroy_surface (gpointer instance,
				  gpointer surface)
{
  cairo_surface_destroy (surface);
}

const PangoViewer cairo_image_viewer = {
  "CairoImage",
  NULL,
  NULL,
  cairo_image_view_create,
  cairo_image_view_destroy,
  NULL,
  cairo_image_view_create_surface,
  cairo_image_view_destroy_surface,
  NULL,
  NULL,
  NULL,
  NULL,
  NULL
};

static cairo_surface_t *
cairo_image_view_iface_create_surface (gpointer instance,
				       gpointer surface,
				       int      width,
				       int      height)
{
  return surface;
}

static CairoViewerIface cairo_image_viewer_iface = {
  &cairo_image_viewer,
  cairo_image_view_iface_create_surface
};

const CairoViewerIface *
get_cairo_viewer_iface (void)
{
#ifdef HAVE_CAIRO_XLIB
  if (opt_display)
    return &cairo_x_viewer_iface;
#endif /* HAVE_CAIRO_XLIB */

  return &cairo_image_viewer_iface;
}
