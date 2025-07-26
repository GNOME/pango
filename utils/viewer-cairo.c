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
#include "viewer-render.h"

#include <cairo.h>

#include <string.h>



#ifdef HAVE_CAIRO_XLIB
#ifdef HAVE_XFT
#include "viewer-x.h"
#include <cairo-xlib.h>

static cairo_surface_t *
cairo_x_view_iface_create_surface (gpointer instance,
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

static void
cairo_x_view_iface_paint_background (gpointer  instance G_GNUC_UNUSED,
				     cairo_t  *cr)
{
  cairo_set_source_rgb (cr, 1, 1, 1);
  cairo_paint (cr);

  if (opt_bg_set)
    {
      cairo_set_source_rgba (cr,
			     opt_bg_color.red / 65535.,
			     opt_bg_color.green / 65535.,
			     opt_bg_color.blue / 65535.,
			     opt_bg_alpha / 65535.);
      cairo_paint (cr);
    }
}

static CairoViewerIface cairo_x_viewer_iface = {
  &x_viewer,
  cairo_x_view_iface_create_surface,
  cairo_x_view_iface_paint_background
};
#endif /* HAVE_XFT */
#endif /* HAVE_CAIRO_XLIB */




#ifdef HAVE_CAIRO_QUARTZ
#include "viewer-cocoa.h"

static cairo_surface_t *
cairo_cocoa_view_iface_create_surface (gpointer instance,
				       gpointer surface,
				       int      width,
				       int      height)
{
  return cairo_surface_reference (surface);
}

static void
cairo_cocoa_view_iface_paint_background (gpointer  instance G_GNUC_UNUSED,
				     cairo_t  *cr)
{
  cairo_set_source_rgb (cr, 1, 1, 1);
  cairo_paint (cr);

  if (opt_bg_set)
    {
      cairo_set_source_rgba (cr,
			     opt_bg_color.red / 65535.,
			     opt_bg_color.green / 65535.,
			     opt_bg_color.blue / 65535.,
			     opt_bg_alpha / 65535.);
      cairo_paint (cr);
    }
}

static CairoViewerIface cairo_cocoa_viewer_iface = {
  &cocoa_viewer,
  cairo_cocoa_view_iface_create_surface,
  cairo_cocoa_view_iface_paint_background
};
#endif /* HAVE_CAIRO_QUARTZ */



static cairo_surface_t *
cairo_view_iface_create_surface (gpointer instance,
				 gpointer surface,
				 int      width,
				 int      height)
{
  return cairo_surface_reference (surface);
}



static gpointer
cairo_image_view_create (const PangoViewer *klass G_GNUC_UNUSED)
{
  return NULL;
}

static void
cairo_image_view_destroy (gpointer instance G_GNUC_UNUSED)
{
}

static gpointer
cairo_image_view_create_surface (gpointer instance,
				 int      width,
				 int      height)
{
  cairo_t *cr;
  cairo_surface_t *surface;

  /* TODO: Be smarter about format? */
  surface = cairo_image_surface_create (CAIRO_FORMAT_ARGB32, width, height);

  cr = cairo_create (surface);
  cairo_set_source_rgb (cr, 1.0, 1.0, 1.0);
  cairo_paint (cr);
  cairo_destroy (cr);

  return surface;
}

static void
cairo_image_view_destroy_surface (gpointer instance,
				  gpointer surface)
{
  cairo_surface_destroy (surface);
}

const PangoViewer cairo_image_viewer = {
  "CairoImage",
  NULL,
  NULL,
  cairo_image_view_create,
  cairo_image_view_destroy,
  NULL,
  cairo_image_view_create_surface,
  cairo_image_view_destroy_surface,
  NULL,
  NULL,
  NULL,
  NULL,
  NULL
};

static void
cairo_image_view_iface_paint_background (gpointer  instance G_GNUC_UNUSED,
					 cairo_t  *cr)
{
  cairo_set_source_rgb (cr, 1, 1, 1);
  cairo_paint (cr);

  if (opt_bg_set)
    {
      cairo_set_operator (cr, CAIRO_OPERATOR_SOURCE);
      cairo_set_source_rgba (cr,
			     opt_bg_color.red / 65535.,
			     opt_bg_color.green / 65535.,
			     opt_bg_color.blue / 65535.,
			     opt_bg_alpha / 65535.);
      cairo_paint (cr);
    }
}

static CairoViewerIface cairo_image_viewer_iface = {
  &cairo_image_viewer,
  cairo_view_iface_create_surface,
  cairo_image_view_iface_paint_background
};




#ifdef CAIRO_HAS_SVG_SURFACE
#    include <cairo-svg.h>
#endif
#ifdef CAIRO_HAS_PDF_SURFACE
#    include <cairo-pdf.h>
#endif
#ifdef CAIRO_HAS_PS_SURFACE
#    include <cairo-ps.h>
#  if CAIRO_VERSION >= CAIRO_VERSION_ENCODE(1,6,0)
#    define HAS_EPS 1

static cairo_surface_t *
_cairo_eps_surface_create (const char *filename,
			   double      width,
			   double      height)
{
  cairo_surface_t *surface;

  surface = cairo_ps_surface_create (filename, width, height);
  cairo_ps_surface_set_eps (surface, TRUE);

  return surface;
}

#  else
#    undef HAS_EPS
#  endif
#endif

typedef cairo_surface_t *(*CairoVectorFileCreateFunc) (const char *filename,
						       double width,
						       double height);

typedef struct
{
  const char *filename;
  CairoVectorFileCreateFunc constructor;
} CairoVectorViewer;

static gpointer
cairo_vector_view_create (const PangoViewer *klass G_GNUC_UNUSED)
{
  const char *extension = NULL;
  CairoVectorFileCreateFunc constructor = NULL;

  if (opt_output)
    {
      extension = strrchr (opt_output, '.');
      if (extension)
	  extension++; /* skip the dot */
    }

  if (!extension)
    return NULL;

  if (0)
    {}
  #ifdef CAIRO_HAS_SVG_SURFACE
    else if (0 == g_ascii_strcasecmp (extension, "svg"))
      constructor = cairo_svg_surface_create;
  #endif
  #ifdef CAIRO_HAS_PDF_SURFACE
    else if (0 == g_ascii_strcasecmp (extension, "pdf"))
      constructor = cairo_pdf_surface_create;
  #endif
  #ifdef CAIRO_HAS_PS_SURFACE
    else if (0 == g_ascii_strcasecmp (extension, "ps"))
      constructor = cairo_ps_surface_create;
   #ifdef HAS_EPS
    else if (0 == g_ascii_strcasecmp (extension, "eps"))
      constructor = _cairo_eps_surface_create;
   #endif
  #endif

  if (constructor)
    {
      CairoVectorViewer *instance;

      instance = g_slice_new (CairoVectorViewer);

      /* save output filename and unset it such that the viewer layer
       * doesn't try to save to file.
       */
     instance->filename = opt_output;
     opt_output = NULL;

     instance->constructor = constructor;

     /* Fix dpi on 72.  That's what cairo vector surfaces are. */
     opt_dpi = 72;

     return instance;
    }

  return NULL;
}

static void
cairo_vector_view_destroy (gpointer instance G_GNUC_UNUSED)
{
  CairoVectorViewer *c = (CairoVectorViewer *) instance;

  g_slice_free (CairoVectorViewer, c);
}

static gpointer
cairo_vector_view_create_surface (gpointer instance,
				  int      width,
				  int      height)
{
  CairoVectorViewer *c = (CairoVectorViewer *) instance;
  cairo_surface_t *surface;

  surface = c->constructor (c->filename, width, height);

    /*cairo_surface_set_fallback_resolution (surface, fallback_resolution_x, fallback_resolution_y);*/

  return surface;
}

static void
cairo_vector_view_destroy_surface (gpointer instance,
				   gpointer surface)
{
  /* TODO: check for errors */
  cairo_surface_destroy (surface);
}

const PangoViewer cairo_vector_viewer = {
  "CairoFile",
  NULL,
  NULL,
  cairo_vector_view_create,
  cairo_vector_view_destroy,
  NULL,
  cairo_vector_view_create_surface,
  cairo_vector_view_destroy_surface,
  NULL,
  NULL,
  NULL,
  NULL,
  NULL
};

static void
cairo_vector_view_iface_paint_background (gpointer  instance G_GNUC_UNUSED,
					  cairo_t  *cr)
{
  if (opt_bg_set)
    {
      cairo_set_operator (cr, CAIRO_OPERATOR_SOURCE);
      cairo_set_source_rgba (cr,
			     opt_bg_color.red / 65535.,
			     opt_bg_color.green / 65535.,
			     opt_bg_color.blue / 65535.,
			     opt_bg_alpha / 65535.);
      cairo_paint (cr);
    }
}

static CairoViewerIface cairo_vector_viewer_iface = {
  &cairo_vector_viewer,
  cairo_view_iface_create_surface,
  cairo_vector_view_iface_paint_background
};



gpointer
cairo_viewer_iface_create (const CairoViewerIface **iface)
{
  gpointer ret;

  *iface = &cairo_vector_viewer_iface;
  ret = (*iface)->backend_class->create ((*iface)->backend_class);
  if (ret)
    return ret;

  if (opt_display)
    {
#ifdef HAVE_CAIRO_QUARTZ
      *iface = &cairo_cocoa_viewer_iface;
      return (*iface)->backend_class->create ((*iface)->backend_class);
#endif /* HAVE_CAIRO_QUARTZ */

#ifdef HAVE_CAIRO_XLIB
#ifdef HAVE_XFT
      *iface = &cairo_x_viewer_iface;
      return (*iface)->backend_class->create ((*iface)->backend_class);
#endif /* HAVE_XFT */
#endif /* HAVE_CAIRO_XLIB */
    }

  *iface = &cairo_image_viewer_iface;
  return (*iface)->backend_class->create ((*iface)->backend_class);
}

void
cairo_viewer_add_options (GOptionGroup *group G_GNUC_UNUSED)
{
}
