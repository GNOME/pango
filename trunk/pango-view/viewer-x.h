/* viewer-x.h: Common headers for X-based viewers
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
#ifndef VIEWER_X_H
#define VIEWER_X_H

#include <pango/pango.h>

#include <X11/Xlib.h>
#include <X11/Xutil.h>

#include "viewer.h"


typedef struct
{
  Display *display;
  int screen;
} XViewer;


extern const PangoViewer x_viewer;

void x_view_init (gpointer           instance,
		  const PangoViewer *klass);

gpointer x_view_create (const PangoViewer *klass);

void x_view_destroy (gpointer instance);

gpointer x_view_create_surface (gpointer instance,
				int      width,
				int      height);

void x_view_destroy_surface (gpointer instance,
			     gpointer surface);

gpointer x_view_create_window (gpointer    instance,
			       const char *title,
			       int         width,
			       int         height);

void x_view_destroy_window (gpointer instance,
			    gpointer window);

gpointer x_view_display (gpointer instance,
			 gpointer surface,
			 gpointer window,
			 int      width,
			 int      height,
			 gpointer state);

#endif /* VIEWER_X_H */
