/* viewer-x.c: Common code for X-based viewers
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
#include <string.h>

#include "viewer-render.h"
#include "viewer-x.h"

void
x_view_init (gpointer           instance,
	     const PangoViewer *klass G_GNUC_UNUSED)
{
  XViewer *x = (XViewer *)instance;

  x->display = XOpenDisplay (NULL);
  if (!x->display)
    fail ("Cannot open display %s", XDisplayName (NULL));

  x->screen = DefaultScreen (x->display);
}

gpointer
x_view_create (const PangoViewer *klass)
{
  XViewer *instance;

  instance = g_slice_new (XViewer);

  x_view_init (instance, klass);

  return instance;
}

void
x_view_destroy (gpointer instance)
{
  XViewer *x = (XViewer *)instance;

  XCloseDisplay (x->display);

  g_slice_free (XViewer, instance);
}

gpointer
x_view_create_surface (gpointer instance,
		       int      width,
		       int      height)
{
  XViewer *x = (XViewer *) instance;
  Pixmap pixmap;

  pixmap = XCreatePixmap (x->display, DefaultRootWindow (x->display), width, height,
			  DefaultDepth (x->display, x->screen));

  return (gpointer) pixmap;
}

void
x_view_destroy_surface (gpointer instance,
			gpointer surface)
{
  XViewer *x = (XViewer *) instance;
  Pixmap pixmap = (Pixmap) surface;

  XFreePixmap (x->display, pixmap);
}

static void
update (Display *display,
	Pixmap   pixmap,
	Window   window,
	Region  *update_region)
{
  GC gc;
  XRectangle extents;

  XClipBox (*update_region, &extents);

  gc = XCreateGC (display, pixmap, 0, NULL);

  XCopyArea (display, pixmap, window, gc,
	     extents.x, extents.y,
	     extents.width, extents.height,
	     extents.x, extents.y);

  XFreeGC (display, gc);

  XDestroyRegion (*update_region);
  *update_region = NULL;
}

static void
expose (XExposeEvent *xev,
	Region       *update_region)
{
  XRectangle  r;

  if (!*update_region)
    *update_region = XCreateRegion ();

  r.x = xev->x;
  r.y = xev->y;
  r.width = xev->width;
  r.height = xev->height;

  XUnionRectWithRegion (&r, *update_region, *update_region);
}

gpointer
x_view_create_window (gpointer    instance,
		      const char *title,
		      int         width,
		      int         height)
{
  XViewer *x = (XViewer *) instance;
  unsigned long bg;
  Window window;
  XSizeHints size_hints;

  bg = WhitePixel (x->display, x->screen);
  window = XCreateSimpleWindow (x->display, DefaultRootWindow (x->display),
				0, 0, width, height, 0,
				bg, bg);

  XSelectInput (x->display, window, ExposureMask | KeyPressMask);

  XMapWindow (x->display, window);
  XmbSetWMProperties (x->display, window,
		      title,
		      NULL, NULL, 0, NULL, NULL, NULL);

  memset ((char *)&size_hints, 0, sizeof (XSizeHints));
  size_hints.flags = PSize | PMaxSize;
  size_hints.width = width; size_hints.height = height; /* for compat only */
  size_hints.max_width = width; size_hints.max_height = height;

  XSetWMNormalHints (x->display, window, &size_hints);

  return (gpointer) window;
}

void
x_view_destroy_window (gpointer instance,
		       gpointer window)
{
  XViewer *x = (XViewer *) instance;
  Window win = (Window) window;

  XDestroyWindow (x->display, win);
}

gpointer
x_view_display (gpointer instance,
		gpointer surface,
		gpointer win,
		int      width,
		int      height,
		gpointer state)
{
  XViewer *x = (XViewer *) instance;
  Pixmap pixmap = (Pixmap) surface;
  Window window = (Window) win;
  XEvent xev;
  XRectangle  r;
  Region update_region;
  unsigned int quit_keycode;
  unsigned int annotate_keycode;

  /* force a full redraw */
  update_region = XCreateRegion ();
  r.x = 0;
  r.y = 0;
  r.width = width;
  r.height = height;
  XUnionRectWithRegion (&r, update_region, update_region);

  annotate_keycode = XKeysymToKeycode(x->display, 'B');
  quit_keycode = XKeysymToKeycode(x->display, 'Q');

  while (1)
    {
      if (!XPending (x->display) && update_region)
	update (x->display, pixmap, window, &update_region);

      XNextEvent (x->display, &xev);
      switch (xev.xany.type) {
      case KeyPress:
	if (xev.xkey.keycode == quit_keycode)
	  return GINT_TO_POINTER (-1);
	else if (xev.xkey.keycode == annotate_keycode)
	  {
	    return GUINT_TO_POINTER (GPOINTER_TO_INT (state) + 1);
	  }
	break;
      case Expose:
	expose (&xev.xexpose, &update_region);
	break;
      default:
        break;
      }
    }
}

const PangoViewer x_viewer = {
  "X",
  NULL,
  NULL,
  x_view_create,
  x_view_destroy,
  NULL,
  x_view_create_surface,
  x_view_destroy_surface,
  NULL,
  NULL,
  x_view_create_window,
  x_view_destroy_window,
  x_view_display
};
