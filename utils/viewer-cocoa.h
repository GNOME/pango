/* viewer-cocoa.h: Header for Cocoa-based viewer
 *
 * Copyright (C) 2025 Khaled Hosny
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
#ifndef VIEWER_COCOA_H
#define VIEWER_COCOA_H

#include <pango/pango.h>
#include "viewer.h"

typedef struct _CocoaViewer CocoaViewer;

extern const PangoViewer cocoa_viewer;

gpointer cocoa_view_create (const PangoViewer *klass);

void cocoa_view_destroy (gpointer instance);

gpointer cocoa_view_create_surface (gpointer instance,
				    int      width,
				    int      height);

void cocoa_view_destroy_surface (gpointer instance,
				 gpointer surface);

gpointer cocoa_view_create_window (gpointer    instance,
				   const char *title,
				   int         width,
				   int         height);

void cocoa_view_destroy_window (gpointer instance,
			        gpointer window);

gpointer cocoa_view_display (gpointer instance,
			     gpointer surface,
			     gpointer window,
			     int      width,
			     int      height,
			     gpointer state);

#endif /* VIEWER_COCOA_H */
