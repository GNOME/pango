/* viewer-x.c: Common code X-based rendering demos
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
#include <config.h>
#include <errno.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>

#include "viewer-x.h"
#include "renderdemo.h"

/* initialized by main() */
static Display *display;
static int screen;
static Window window;
static Pixmap pixmap;

/* initialized by do_init() */
static PangoContext *context;
static int width, height;

/* runtime stuff */
static Region update_region = NULL;
static gboolean show_borders;

static void
update (void)
{
  GC gc;
  XRectangle extents;
  int width, height;

  XClipBox (update_region, &extents);

  width = extents.width;
  height = extents.height;

  gc = XCreateGC (display, pixmap, 0, NULL);

  XCopyArea (display, pixmap, window, gc,
	     extents.x, extents.y,
	     extents.width, extents.height,
	     extents.x, extents.y);

  XFreeGC (display, gc);
	     
  XDestroyRegion (update_region);
  update_region = NULL;
}

static void
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

int
main (int argc, char **argv)
{
  XEvent xev;
  unsigned long bg;
  XSizeHints size_hints;
  unsigned int quit_keycode;
  unsigned int borders_keycode;
  
  g_type_init();

  parse_options (argc, argv);

  display = XOpenDisplay (NULL);
  if (!display)
    fail ("Cannot open display %s\n", XDisplayName (NULL));
  screen = DefaultScreen (display);

  do_init (display, screen, opt_dpi, &context, &width, &height);

  bg = WhitePixel (display, screen);
  window = XCreateSimpleWindow (display, DefaultRootWindow (display),
				0, 0, width, height, 0,
				bg, bg);
  pixmap = XCreatePixmap (display, window, width, height,
			  DefaultDepth (display, screen));
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

  do_render (display, screen, window, pixmap, context, width, height, show_borders);

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
	    do_render (display, screen, window, pixmap, context, width, height, show_borders);

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
  XFreePixmap (display, pixmap);
  finalize ();

  return 0;
}
