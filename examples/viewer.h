/* viewer.h: PangoViewer class
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
#ifndef VIEWER_H
#define VIEWER_H

#include <stdio.h>
#include <pango/pango.h>

typedef struct _PangoViewer PangoViewer;

struct _PangoViewer {

  const char *name;

  const char *id;

  const char *write_suffix;

  gpointer (*create) (const PangoViewer *klass);

  void (*destroy) (gpointer instance);

  PangoContext * (*get_context) (gpointer instance);

  gpointer (*create_surface) (gpointer instance,
			      int      width,
			      int      height);

  void (*destroy_surface) (gpointer instance,
			   gpointer surface);

  void (*render) (gpointer      instance,
		  gpointer      surface,
		  PangoContext *context,
		  int           width,
		  int           height,
		  gpointer      state);

  /* The following can be NULL */

  void (*write) (gpointer instance,
		 gpointer surface,
		 FILE    *stream,
		 int      width,
		 int      height);

  gpointer (*create_window) (gpointer    instance,
			     const char *title,
			     int         width,
			     int         height);

  void (*destroy_window) (gpointer instance,
			  gpointer window);

  gpointer (*display) (gpointer instance,
		       gpointer surface,
		       gpointer window,
		       int      width,
		       int      height,
		       gpointer state);

  void (*load) (gpointer instance,
		gpointer surface,
		guchar  *buffer,
		int      width,
		int      height,
		int      stride);

  void (*save) (gpointer instance,
		gpointer surface,
		guchar  *buffer,
		int      width,
		int      height,
		int      stride);

};

extern const PangoViewer *fallback_viewer;
extern const PangoViewer *viewers[];

#endif /* VIEWER_H */
