/* viewer-cocoa.m: Code for Cocoa-based viewer
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
#include "config.h"
#import <Cocoa/Cocoa.h>

#include <cairo.h>
#include <cairo-quartz.h>
#include <gio/gio.h>

#include "viewer-render.h"
#include "viewer-cocoa.h"
#include "viewer-cocoa-resources.h"

struct _CocoaViewer
{
  NSApplication *app;
};

@interface PangoView : NSView

@property (nonatomic, assign, nullable) cairo_surface_t *surface;

@end

@implementation PangoView

- (void)drawRect:(NSRect)dirtyRect
{
  if ([self surface])
  {
    NSGraphicsContext *context = [NSGraphicsContext currentContext];
    CGContextRef cgContext = [context CGContext];
    CGFloat scaleFactor = [[self window] backingScaleFactor];

    CGContextTranslateCTM (cgContext, 0.0, [self bounds].size.height);
    CGContextScaleCTM (cgContext, 1/scaleFactor, -1/scaleFactor);

    cairo_surface_t *viewSurface = cairo_quartz_surface_create_for_cg_context (cgContext,
									       [self bounds].size.width * scaleFactor,
									       [self bounds].size.height * scaleFactor);

    cairo_surface_set_device_scale (viewSurface, scaleFactor, scaleFactor);
    cairo_t *cr = cairo_create (viewSurface);

    cairo_set_source_surface (cr, [self surface], 0, 0);
    cairo_paint (cr);

    cairo_destroy (cr);
    cairo_surface_destroy (viewSurface);
  }
}

@end

static void
set_app_icon (NSApplication *app)
{
  GBytes *icon_bytes = g_resource_lookup_data (viewer_cocoa_get_resource (),
					       "/org/gnome/pango/viewer-cocoa/icon",
					       G_RESOURCE_LOOKUP_FLAGS_NONE, NULL);
  if (icon_bytes)
  {
    gsize data_size;
    gconstpointer data = g_bytes_get_data (icon_bytes, &data_size);
    NSData *iconData = [NSData dataWithBytes:data length:data_size];
    NSImage *appIcon = [[NSImage alloc] initWithData:iconData];
    if (appIcon)
    {
      [app setApplicationIconImage:appIcon];
      [appIcon release];
    }
    g_bytes_unref (icon_bytes);
  }
}

gpointer
cocoa_view_create (const PangoViewer *klass G_GNUC_UNUSED)
{
  CocoaViewer *viewer = g_slice_new (CocoaViewer);

  NSApplication *app = [NSApplication sharedApplication];
  [app setActivationPolicy:NSApplicationActivationPolicyRegular];
  [app finishLaunching];

  set_app_icon (app);

  viewer->app = app;

  return viewer;
}

void
cocoa_view_destroy (gpointer instance)
{
  CocoaViewer *viewer = (CocoaViewer *)instance;
  [viewer->app terminate:nil];
  g_slice_free (CocoaViewer, instance);
}

gpointer
cocoa_view_create_surface (gpointer instance G_GNUC_UNUSED,
			   int      width G_GNUC_UNUSED,
			   int      height G_GNUC_UNUSED)
{
  return cairo_recording_surface_create (CAIRO_CONTENT_COLOR_ALPHA, NULL);
}

void
cocoa_view_destroy_surface (gpointer instance G_GNUC_UNUSED,
			    gpointer surface)
{
  cairo_surface_destroy ((cairo_surface_t *)surface);
}

gpointer
cocoa_view_create_window (gpointer    instance G_GNUC_UNUSED,
			  const char *title,
			  int         width,
			  int         height)
{
  NSRect windowRect = NSMakeRect (0, 0, width, height);
  NSWindow *window = [[NSWindow alloc] initWithContentRect:windowRect
						 styleMask:(NSWindowStyleMaskTitled |
						           NSWindowStyleMaskClosable)
						   backing:NSBackingStoreBuffered
						     defer:NO];

  [window setTitle:[NSString stringWithUTF8String:title]];
  [window setContentView:[[PangoView alloc] initWithFrame:windowRect]];
  [window makeKeyAndOrderFront:nil];
  [window center];
  [window setReleasedWhenClosed:NO];

  return window;
}

void
cocoa_view_destroy_window (gpointer instance G_GNUC_UNUSED,
			   gpointer win)
{
  NSWindow *window = (NSWindow *)win;

  [[window contentView] release];
  [window close];
}

gpointer
cocoa_view_display (gpointer instance,
		    gpointer surface,
		    gpointer win,
		    int      width G_GNUC_UNUSED,
		    int      height G_GNUC_UNUSED,
		    gpointer state)
{
  CocoaViewer *viewer = (CocoaViewer *)instance;

  NSWindow *window = (NSWindow *)win;
  [window makeKeyAndOrderFront:nil];
  [window orderFrontRegardless];
  [window makeMainWindow];
  [window makeKeyWindow];

  PangoView *view = [window contentView];
  [view setSurface:(cairo_surface_t *)surface];
  [view setNeedsDisplay:YES];

  NSApplication *app = viewer->app;
  [app activateIgnoringOtherApps:YES];

  while ([window isVisible])
  {
    NSEvent *event = [app nextEventMatchingMask:NSEventMaskAny
				      untilDate:[NSDate dateWithTimeIntervalSinceNow:0.1]
				         inMode:NSDefaultRunLoopMode
				        dequeue:YES];
    if (event)
    {
      if ([event type] == NSEventTypeKeyDown)
      {
	NSString *characters = [event characters];
	if ([characters length] > 0)
	{
	  switch ([characters characterAtIndex:0])
	  {
	    case 'q':
	    case 'w':
	      return GINT_TO_POINTER (-1);
	    case 'b':
	      return GINT_TO_POINTER (GPOINTER_TO_INT (state) + 1);
	    default:
	      break;
	  }
	}
      }
      [app sendEvent:event];
    }
  }

  return GINT_TO_POINTER (-1);
}

const PangoViewer cocoa_viewer = {
  "Cocoa",
  NULL,
  NULL,
  cocoa_view_create,
  cocoa_view_destroy,
  NULL,
  cocoa_view_create_surface,
  cocoa_view_destroy_surface,
  NULL,
  NULL,
  cocoa_view_create_window,
  cocoa_view_destroy_window,
  cocoa_view_display
};
