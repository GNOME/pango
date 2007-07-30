/* viewer-cairo.h: Common headers for Cairo-based viewers
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
#ifndef VIEWER_CAIRO_H
#define VIEWER_CAIRO_H

#include <cairo.h>

#include "viewer.h"

typedef struct _CairoViewerIface CairoViewerIface;

struct _CairoViewerIface
{
  const PangoViewer *backend_class;

  cairo_surface_t * (*create_surface) (gpointer instance,
				       gpointer surface,
				       int      width,
				       int      height);
};

const CairoViewerIface *get_default_cairo_viewer_iface (void);

#endif /* VIEWER_CAIRO_H */
