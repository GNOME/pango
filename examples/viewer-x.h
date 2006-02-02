/* viewer-x.h: Common code X-based rendering demos
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

#include <pango/pango.h>

#include <X11/Xlib.h>
#include <X11/Xutil.h>

/* to be implemented by the backend-specific part */

void do_init (Display *display,
	      int screen,
	      int dpi,
	      /* output */
	      PangoContext **context,
	      int *width,
	      int *height);

void do_render (Display *display,
		int screen,
		Window window,
		Pixmap pixmap,
		PangoContext *context,
		int width,
		int height,
		gboolean show_borders);
