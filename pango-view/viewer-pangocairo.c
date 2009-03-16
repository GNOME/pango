/* viewer-pangocairo.c: PangoCairo viewer backend.
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

#include "viewer-render.h"
#include "viewer-cairo.h"

#include <pango/pangocairo.h>

static int opt_annotate = 0;

typedef struct
{
  const CairoViewerIface *iface;

  gpointer backend;

  PangoFontMap *fontmap;
  cairo_font_options_t *font_options;
} CairoViewer;

static gpointer
pangocairo_view_create (const PangoViewer *klass G_GNUC_UNUSED)
{
  CairoViewer *instance;

  instance = g_slice_new (CairoViewer);

  instance->backend = cairo_viewer_iface_create (&instance->iface);

  instance->fontmap = pango_cairo_font_map_new ();
  pango_cairo_font_map_set_resolution (PANGO_CAIRO_FONT_MAP (instance->fontmap), opt_dpi);

  instance->font_options = cairo_font_options_create ();
  if (opt_hinting != HINT_DEFAULT)
    {
      if (opt_hinting == HINT_NONE)
	cairo_font_options_set_hint_style (instance->font_options, CAIRO_HINT_STYLE_NONE);
      else if (opt_hinting == HINT_FULL)
	cairo_font_options_set_hint_style (instance->font_options, CAIRO_HINT_STYLE_FULL);
    }

  return instance;
}

static void
pangocairo_view_destroy (gpointer instance)
{
  CairoViewer *c = (CairoViewer *) instance;

  cairo_font_options_destroy (c->font_options);

  g_object_unref (c->fontmap);

  c->iface->backend_class->destroy (c->backend);

  cairo_debug_reset_static_data ();

  g_slice_free (CairoViewer, c);
}

static PangoContext *
pangocairo_view_get_context (gpointer instance)
{
  CairoViewer *c = (CairoViewer *) instance;
  PangoContext *context;

  context = pango_font_map_create_context (c->fontmap);
  pango_cairo_context_set_font_options (context, c->font_options);

  return context;
}

typedef struct
{
  gpointer backend;

  cairo_surface_t *cairo;
} CairoSurface;

static gpointer
pangocairo_view_create_surface (gpointer instance,
				int      width,
				int      height)
{
  CairoViewer *c = (CairoViewer *) instance;
  CairoSurface *surface;

  surface = g_slice_new (CairoSurface);

  surface->backend = c->iface->backend_class->create_surface (c->backend,
							      width, height);

  surface->cairo = c->iface->create_surface (c->backend,
					     surface->backend,
					     width, height);

  return surface;
}

static void
pangocairo_view_destroy_surface (gpointer instance,
				 gpointer surface)
{
  CairoViewer *c = (CairoViewer *) instance;
  CairoSurface *c_surface = (CairoSurface *) surface;

  c->iface->backend_class->destroy_surface (c->backend, c_surface->backend);
  cairo_surface_destroy (c_surface->cairo);

  g_slice_free (CairoSurface, surface);
}

static void
render_callback (PangoLayout *layout,
		 int          x,
		 int          y,
		 gpointer     context,
		 gpointer     state)
{
  cairo_t *cr = (cairo_t *) context;
  int annotate = (GPOINTER_TO_INT (state) + opt_annotate) % 3;

  cairo_save (cr);
  cairo_translate (cr, x, y);

  if (annotate)
    {
      cairo_pattern_t *pattern;
      PangoRectangle ink, logical;
      double lw = cairo_get_line_width (cr);
      PangoLayoutIter* iter;

      pango_layout_get_extents (layout, &ink, &logical);

      if (annotate >= 2)
        {
	  /* draw resolved gravity "roof" in blue */
	  cairo_save (cr);
	  cairo_translate (cr,
			   (double)logical.x / PANGO_SCALE,
			   (double)logical.y / PANGO_SCALE);
	  cairo_scale     (cr,
			   (double)logical.width / PANGO_SCALE * 0.5,
			   (double)logical.height / PANGO_SCALE * 0.5);
	  cairo_translate   (cr,  1.0,  1.0);
	  cairo_rotate (cr,
	    pango_gravity_to_rotation (
	      pango_context_get_gravity (
		pango_layout_get_context (layout))));
	  cairo_move_to     (cr, -1.0, -1.0);
	  cairo_rel_line_to (cr, +1.0, -0.2); /* /   */
	  cairo_rel_line_to (cr, +1.0, +0.2); /*   \ */
	  cairo_close_path  (cr);             /*  -  */
	  pattern = cairo_pattern_create_linear (0, -1.0, 0, -1.2);
	  cairo_pattern_add_color_stop_rgba (pattern, 0.0, 0.0, 0.0, 1.0, 0.0);
	  cairo_pattern_add_color_stop_rgba (pattern, 1.0, 0.0, 0.0, 1.0, 0.15);
	  cairo_set_source (cr, pattern);
	  cairo_fill (cr);
	  /* once more, without close_path this time */
	  cairo_move_to     (cr, -1.0, -1.0);
	  cairo_rel_line_to (cr, +1.0, -0.2); /* /   */
	  cairo_rel_line_to (cr, +1.0, +0.2); /*   \ */
	  /* silly line_width is not locked :(. get rid of scale. */
	  cairo_restore (cr);
	  cairo_save (cr);
	  cairo_set_source_rgba (cr, 0.0, 0.0, 0.7, 0.2);
	  cairo_stroke (cr);
	  cairo_restore (cr);


	  /* draw block progression arrow in green */
	  cairo_save (cr);
	  cairo_translate (cr,
			   (double)logical.x / PANGO_SCALE,
			   (double)logical.y / PANGO_SCALE);
	  cairo_scale     (cr,
			   (double)logical.width / PANGO_SCALE * 0.5,
			   (double)logical.height / PANGO_SCALE * 0.5);
	  cairo_translate   (cr,  1.0,  1.0);
	  cairo_move_to     (cr, -0.4, -0.7);
	  cairo_rel_line_to (cr, +0.8,  0.0); /*  --   */
	  cairo_rel_line_to (cr,  0.0, +0.9); /*    |  */
	  cairo_rel_line_to (cr, +0.4,  0.0); /*     - */
	  cairo_rel_line_to (cr, -0.8, +0.5); /*    /  */
	  cairo_rel_line_to (cr, -0.8, -0.5); /*  \    */
	  cairo_rel_line_to (cr, +0.4,  0.0); /* -     */
	  cairo_close_path  (cr);             /*  |    */
	  pattern = cairo_pattern_create_linear (0, -0.7, 0, 0.7);
	  cairo_pattern_add_color_stop_rgba (pattern, 0.0, 0.0, 1.0, 0.0, 0.0);
	  cairo_pattern_add_color_stop_rgba (pattern, 1.0, 0.0, 1.0, 0.0, 0.15);
	  cairo_set_source (cr, pattern);
	  cairo_fill_preserve (cr);
	  /* silly line_width is not locked :(. get rid of scale. */
	  cairo_restore (cr);
	  cairo_save (cr);
	  cairo_set_source_rgba (cr, 0.0, 0.7, 0.0, 0.2);
	  cairo_stroke (cr);
	  cairo_restore (cr);
	}

      /* draw baselines with line direction arrow in orange */
      cairo_save (cr);
      cairo_set_source_rgba (cr, 1.0, 0.5, 0.0, 0.5);
      iter = pango_layout_get_iter (layout);
      do
	{
	  PangoLayoutLine *line = pango_layout_iter_get_line (iter);
	  double width = (double)logical.width / PANGO_SCALE;

	  y = pango_layout_iter_get_baseline (iter);
	  cairo_save (cr);
	  cairo_translate (cr,
			 (double)logical.x / PANGO_SCALE + width * 0.5,
			 (double)y / PANGO_SCALE);
	  if (line->resolved_dir)
	    cairo_scale (cr, -1, 1);
	  cairo_move_to     (cr, -width * .5, -lw*0.2);
	  cairo_rel_line_to (cr, +width * .9, -lw*0.3);
	  cairo_rel_line_to (cr,  0,          -lw);
	  cairo_rel_line_to (cr, +width * .1, +lw*1.5);
	  cairo_rel_line_to (cr, -width * .1, +lw*1.5);
	  cairo_rel_line_to (cr, 0,           -lw);
	  cairo_rel_line_to (cr, -width * .9, -lw*0.3);
	  cairo_close_path (cr);
	  cairo_fill (cr);
	  cairo_restore (cr);
	}
      while (pango_layout_iter_next_line (iter));
      pango_layout_iter_free (iter);
      cairo_restore (cr);

      /* draw the logical rect in red */
      cairo_save (cr);
      cairo_set_source_rgba (cr, 1.0, 0.0, 0.0, 0.5);
      cairo_rectangle (cr,
		       (double)logical.x / PANGO_SCALE - lw / 2,
		       (double)logical.y / PANGO_SCALE - lw / 2,
		       (double)logical.width / PANGO_SCALE + lw,
		       (double)logical.height / PANGO_SCALE + lw);
      cairo_stroke (cr);
      cairo_restore (cr);

      /* draw the ink rect in green */
      cairo_save (cr);
      cairo_set_source_rgba (cr, 0.0, 1.0, 0.0, 0.5);
      cairo_rectangle (cr,
		       (double)ink.x / PANGO_SCALE - lw / 2,
		       (double)ink.y / PANGO_SCALE - lw / 2,
		       (double)ink.width / PANGO_SCALE + lw,
		       (double)ink.height / PANGO_SCALE + lw);
      cairo_stroke (cr);
      cairo_restore (cr);
    }

  cairo_move_to (cr, 0, 0);
  pango_cairo_show_layout (cr, layout);

  cairo_restore (cr);
}

static void
transform_callback (PangoContext *context,
		    PangoMatrix  *matrix,
		    gpointer      cr_context,
		    gpointer      state G_GNUC_UNUSED)
{
  cairo_t *cr = (cairo_t *)cr_context;
  cairo_matrix_t cairo_matrix;

  if (matrix)
    {
      cairo_matrix.xx = matrix->xx;
      cairo_matrix.yx = matrix->yx;
      cairo_matrix.xy = matrix->xy;
      cairo_matrix.yy = matrix->yy;
      cairo_matrix.x0 = matrix->x0;
      cairo_matrix.y0 = matrix->y0;
    }
  else
    {
      cairo_matrix_init_identity (&cairo_matrix);
    }

  cairo_set_matrix (cr, &cairo_matrix);

  pango_cairo_update_context (cr, context);
}

static void
pangocairo_view_render (gpointer      instance,
			gpointer      surface,
			PangoContext *context,
			int          *width,
			int          *height,
			gpointer      state)
{
  CairoViewer *c = (CairoViewer *) instance;
  cairo_t *cr;
  CairoSurface *c_surface = (CairoSurface *) surface;

  g_assert (surface);

  cr = cairo_create (c_surface->cairo);

  transform_callback (context, NULL, cr, state);

  c->iface->paint_background (instance, cr);

  cairo_set_operator (cr, CAIRO_OPERATOR_OVER);
  cairo_set_source_rgba (cr,
			 opt_fg_color.red / 65535.,
			 opt_fg_color.green / 65535.,
			 opt_fg_color.blue / 65535.,
			 opt_fg_alpha / 65535.);

  do_output (context, render_callback, transform_callback, cr, state, width, height);

  cairo_destroy (cr);
}

#ifdef HAVE_CAIRO_PNG
static cairo_status_t
write_func (void                *closure,
	    const unsigned char *data,
	    unsigned int         length)
{
  FILE *stream = (FILE *) closure;
  unsigned int l;

  l = fwrite (data, 1, length, stream);

  return l == length ? CAIRO_STATUS_SUCCESS : CAIRO_STATUS_WRITE_ERROR;
}

static void
pangocairo_view_write (gpointer instance G_GNUC_UNUSED,
		       gpointer surface,
		       FILE    *stream,
		       int      width G_GNUC_UNUSED,
		       int      height G_GNUC_UNUSED)
{
  CairoSurface *c_surface = (CairoSurface *) surface;

  cairo_surface_write_to_png_stream (c_surface->cairo, write_func, stream);
}
#endif

static gpointer
pangocairo_view_create_window (gpointer    instance,
			       const char *title,
			       int         width,
			       int         height)
{
  CairoViewer *c = (CairoViewer *) instance;

  if (!c->iface->backend_class->create_window)
    return NULL;

  return c->iface->backend_class->create_window (c->backend,
						 title,
						 width, height);
}

static void
pangocairo_view_destroy_window (gpointer instance,
				gpointer window)
{
  CairoViewer *c = (CairoViewer *) instance;

  c->iface->backend_class->destroy_window (c->backend,
					   window);
}

static gpointer
pangocairo_view_display (gpointer instance,
			 gpointer surface,
			 gpointer window,
			 int width, int height,
			 gpointer state)
{
  CairoViewer *c = (CairoViewer *) instance;
  CairoSurface *c_surface = (CairoSurface *) surface;

  return c->iface->backend_class->display (c->backend,
					   c_surface->backend,
					   window,
					   width, height,
					   state);
}

static GOptionGroup *
pangocairo_view_get_option_group (const PangoViewer *klass G_GNUC_UNUSED)
{
  GOptionEntry entries[] =
  {
    {"annotate",	0, 0, G_OPTION_ARG_INT, &opt_annotate,
     "Annotate the output",				"1 or 2"},
    {NULL}
  };
  GOptionGroup *group;

  group = g_option_group_new ("cairo",
			      "Cairo backend options:",
			      "Options understood by the cairo backend",
			      NULL,
			      NULL);

  g_option_group_add_entries (group, entries);

  cairo_viewer_add_options (group);

  return group;
}

const PangoViewer pangocairo_viewer = {
  "PangoCairo",
  "cairo",
#ifdef HAVE_CAIRO_PNG
  "png",
#else
  NULL,
#endif
  pangocairo_view_create,
  pangocairo_view_destroy,
  pangocairo_view_get_context,
  pangocairo_view_create_surface,
  pangocairo_view_destroy_surface,
  pangocairo_view_render,
#ifdef HAVE_CAIRO_PNG
  pangocairo_view_write,
#else
  NULL,
#endif
  pangocairo_view_create_window,
  pangocairo_view_destroy_window,
  pangocairo_view_display,
  NULL,
  NULL,
  pangocairo_view_get_option_group
};
