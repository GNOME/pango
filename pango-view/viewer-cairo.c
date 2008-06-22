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

#ifdef HAVE_CAIRO_XLIB
#include "viewer-x.h"
#include <cairo-xlib.h>

static cairo_surface_t *
cairo_x_view_create_surface (gpointer instance,
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
  cairo_x_view_create_surface
};

const CairoViewerIface *
get_default_cairo_viewer_iface (void)
{
  return &cairo_x_viewer_iface;
}
#endif /* HAVE_CAIRO_XLIB */
