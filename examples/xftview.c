/* Pango
 * pangoft2topgm.c: Example program to view a UTF-8 encoding file
 *                  using Pango to render result.
 *
 * Copyright (C) 1999,2004 Red Hat, Inc.
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

#include <errno.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>

#include "renderdemo.h"

#include <pango/pangoxft.h>

static Region update_region = NULL;
static XftDraw *draw;
static PangoContext *context;

static void
xft_render (PangoLayout *layout,
	    int          x,
	    int          y,
	    gpointer     data)
{
  XftColor color;

  color.color.red = 0x0;
  color.color.green = 0x0;
  color.color.blue = 0x0;
  color.color.alpha = 0xffff;

  pango_xft_render_layout (draw, &color, layout, x, y);
}

void
update ()
{
  XRectangle area;
  XftColor color;

  XClipBox (update_region, &area);
  XftDrawSetClip (draw, update_region);
  XDestroyRegion (update_region);
  update_region = NULL;

  color.color.red = 0xffff;
  color.color.blue = 0xffff;
  color.color.green = 0xffff;
  color.color.alpha = 0xffff;

  XftDrawRect (draw, &color,
	       area.x, area.y, area.width, area.height);

  do_output (context, xft_render, draw, NULL, NULL);
}

void
expose (XExposeEvent *xev)
{
  XRectangle area;
  
  if (!update_region)
    update_region = XCreateRegion ();

  area.x = xev->x;
  area.y = xev->y;
  area.width = xev->width;
  area.height = xev->height;

  XUnionRectWithRegion (&area, update_region, update_region);
}

int main (int argc, char **argv)
{
  Display *display;
  int screen;
  Window window;
  XEvent xev;
  unsigned long bg;
  int width, height;
  
  g_type_init();

  XftInit (NULL);
  
  parse_options (argc, argv);

  display = XOpenDisplay (NULL);
  if (!display)
    fail ("Cannot open display %s\n", XDisplayName (NULL));

  screen = DefaultScreen (display);
  bg = WhitePixel (display, screen);

  context = pango_xft_get_context (display, screen);
  do_output (context, NULL, NULL, &width, &height);

  window = XCreateSimpleWindow (display, DefaultRootWindow (display),
				0, 0, width, height, 0,
				bg, bg);
  XSelectInput (display, window, ExposureMask);
  
  XMapWindow (display, window);
  draw = XftDrawCreate (display, window,
			DefaultVisual (display, screen),
			DefaultColormap (display, screen));
  XmbSetWMProperties (display, window,
		      get_options_string (),
		      NULL, NULL, 0, NULL, NULL, NULL);

  while (1)
    {
      if (!XPending (display) && update_region)
	update ();
	
      XNextEvent (display, &xev);
      if (xev.xany.type == Expose)
	{
	  expose (&xev.xexpose);
	}
    }
  
  return 0;
}
