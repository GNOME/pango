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


gboolean output_to_stream_after_destroying_surface = FALSE;

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
  if (opt_trim)
    surface = cairo_image_surface_create (CAIRO_FORMAT_ARGB32, (int) opt_trim_width, (int) opt_trim_height);
  else
    surface = cairo_image_surface_create (CAIRO_FORMAT_ARGB32, width, height);

  cr = cairo_create (surface);
  cairo_set_source_rgb (cr, 1.0, 1.0, 1.0);
  cairo_paint (cr);
  cairo_destroy (cr);

  return surface;
}

static void
cairo_image_view_destroy_surface (gpointer instance,
				  gpointer surface,
				  gboolean output_on_destroy G_GNUC_UNUSED)
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
_cairo_eps_surface_create (cairo_write_func_t write_func,
			   void        *closure,
			   double      width,
			   double      height)
{
  cairo_surface_t *surface;

  surface = cairo_ps_surface_create_for_stream (write_func, closure, width, height);
  cairo_ps_surface_set_eps (surface, TRUE);

  return surface;
}

#  else
#    undef HAS_EPS
#  endif
#endif

typedef cairo_surface_t *(*CairoVectorFileCreateFunc) (cairo_write_func_t write_func,
						       void   *closure,
						       double width,
						       double height);

typedef struct
{
  FILE *file_handle;
  CairoVectorFileCreateFunc constructor;
} CairoVectorViewer;

static cairo_status_t
cairo_surface_write_func (void  *closure,
	        const unsigned char *data,
	        unsigned int        length)
{
  if (output_to_stream_after_destroying_surface)
    {
      FILE *stream = (FILE *) closure;
      unsigned int l;
      l = fwrite (data, 1, length, stream);

      return l == length ? CAIRO_STATUS_SUCCESS : CAIRO_STATUS_WRITE_ERROR;
    }

  return CAIRO_STATUS_SUCCESS;
}

static gpointer
cairo_vector_view_create (const PangoViewer *klass G_GNUC_UNUSED)
{
  CairoVectorFileCreateFunc constructor = NULL;

  if (!opt_output_format)
    return NULL;

  if (0)
    ;
  #ifdef CAIRO_HAS_SVG_SURFACE
    else if (0 == g_ascii_strcasecmp (opt_output_format, "svg"))
      constructor = cairo_svg_surface_create_for_stream;
  #endif
  #ifdef CAIRO_HAS_PDF_SURFACE
    else if (0 == g_ascii_strcasecmp (opt_output_format, "pdf"))
      constructor = cairo_svg_surface_create_for_stream;
  #endif
  #ifdef CAIRO_HAS_PS_SURFACE
    else if (0 == g_ascii_strcasecmp (opt_output_format, "ps"))
      constructor = cairo_svg_surface_create_for_stream;
   #ifdef HAS_EPS
    else if (0 == g_ascii_strcasecmp (opt_output_format, "eps"))
      constructor = _cairo_eps_surface_create;
   #endif
  #endif

  if (constructor)
    {
      FILE *output_file_handle = stdout;
      CairoVectorViewer *instance;

      instance = g_slice_new (CairoVectorViewer);
      
      if (0 != strcmp (opt_output_file, "-"))
        output_file_handle = fopen (opt_output_file, "wb");

      /* save output filename and unset it such that the viewer layer
       * doesn't try to save to file.
       */
     opt_output_file = NULL;
     instance->file_handle = output_file_handle;

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

  if (opt_trim)
    surface = c->constructor (&cairo_surface_write_func, c->file_handle, opt_trim_width, opt_trim_height);
  else
    surface = c->constructor (&cairo_surface_write_func, c->file_handle, width, height);

    /*cairo_surface_set_fallback_resolution (surface, fallback_resolution_x, fallback_resolution_y);*/

  return surface;
}

static void
cairo_vector_view_destroy_surface (gpointer instance,
				   gpointer surface,
				   gboolean output_on_destroy)
{
  output_to_stream_after_destroying_surface = output_on_destroy;
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

#ifdef HAVE_CAIRO_XLIB
#ifdef HAVE_XFT
  if (opt_display)
    {
      *iface = &cairo_x_viewer_iface;
      return (*iface)->backend_class->create ((*iface)->backend_class);
    }
#endif /* HAVE_XFT */
#endif /* HAVE_CAIRO_XLIB */

  *iface = &cairo_image_viewer_iface;
  return (*iface)->backend_class->create ((*iface)->backend_class);
}

void
cairo_viewer_add_options (GOptionGroup *group G_GNUC_UNUSED)
{
}
