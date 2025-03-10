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

#include <hb-ot.h>

static int opt_annotate = 0;

typedef struct
{
  const CairoViewerIface *iface;

  gpointer backend;

  PangoFontMap *fontmap;
  cairo_font_options_t *font_options;
  gboolean subpixel_positions;
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
      else if (opt_hinting == HINT_SLIGHT ||
               opt_hinting == HINT_AUTO)
	cairo_font_options_set_hint_style (instance->font_options, CAIRO_HINT_STYLE_SLIGHT);
      else if (opt_hinting == HINT_MEDIUM)
	cairo_font_options_set_hint_style (instance->font_options, CAIRO_HINT_STYLE_MEDIUM);
      else if (opt_hinting == HINT_FULL)
	cairo_font_options_set_hint_style (instance->font_options, CAIRO_HINT_STYLE_FULL);
    }

  if (opt_subpixel_order != SUBPIXEL_DEFAULT)
    cairo_font_options_set_subpixel_order (instance->font_options, (cairo_subpixel_order_t)opt_subpixel_order);

  if (opt_hint_metrics != HINT_METRICS_DEFAULT)
    cairo_font_options_set_hint_metrics (instance->font_options, (cairo_hint_metrics_t)opt_hint_metrics);

  if (opt_antialias != ANTIALIAS_DEFAULT)
    cairo_font_options_set_antialias (instance->font_options, (cairo_antialias_t)opt_antialias);

  instance->subpixel_positions = opt_subpixel_positions;

  return instance;
}

static void
pangocairo_view_destroy (gpointer instance)
{
  CairoViewer *c = (CairoViewer *) instance;

  cairo_font_options_destroy (c->font_options);

  g_object_unref (c->fontmap);

  c->iface->backend_class->destroy (c->backend);

  g_slice_free (CairoViewer, c);
}

static PangoContext *
pangocairo_view_get_context (gpointer instance)
{
  CairoViewer *c = (CairoViewer *) instance;
  PangoContext *context;

  context = pango_font_map_create_context (c->fontmap);
  pango_cairo_context_set_font_options (context, c->font_options);
  pango_context_set_round_glyph_positions (context, !c->subpixel_positions);

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

enum {
  ANNOTATE_GRAVITY_ROOF      =    1,
  ANNOTATE_BLOCK_PROGRESSION =    2,
  ANNOTATE_BASELINES         =    4,
  ANNOTATE_LAYOUT_EXTENTS    =    8,
  ANNOTATE_LINE_EXTENTS      =   16,
  ANNOTATE_RUN_EXTENTS       =   32,
  ANNOTATE_CLUSTER_EXTENTS   =   64,
  ANNOTATE_CHAR_EXTENTS      =  128,
  ANNOTATE_GLYPH_EXTENTS     =  256,
  ANNOTATE_CARET_POSITIONS   =  512,
  ANNOTATE_CARET_SLOPE       = 1024,
  ANNOTATE_RUN_BASELINES     = 2048,
  ANNOTATE_LAST              = 4096,
};

static struct {
  int value;
  const char *name;
  const char *short_name;
} annotate_options[] = {
  { ANNOTATE_GRAVITY_ROOF,      "gravity-roof",      "gravity" },
  { ANNOTATE_BLOCK_PROGRESSION, "block-progression", "progression" },
  { ANNOTATE_BASELINES,         "baselines",         "baselines" },
  { ANNOTATE_RUN_BASELINES,     "run-baselines",     "run-baselines" },
  { ANNOTATE_LAYOUT_EXTENTS,    "layout-extents",    "layout" },
  { ANNOTATE_LINE_EXTENTS,      "line-extents",      "line" },
  { ANNOTATE_RUN_EXTENTS,       "run-extents",       "run" },
  { ANNOTATE_CLUSTER_EXTENTS,   "cluster-extents",   "cluster" },
  { ANNOTATE_CHAR_EXTENTS,      "char-extents",      "char" },
  { ANNOTATE_GLYPH_EXTENTS,     "glyph-extents",     "glyph" },
  { ANNOTATE_CARET_POSITIONS,   "caret-positions",   "caret" },
  { ANNOTATE_CARET_SLOPE,       "caret-slope",       "slope" },
};

static const char *annotate_arg_help =
     "Annotate the output. Comma-separated list of\n"
     "\t\t\t\t\t\t     gravity, progression, baselines, layout, line,\n"
     "\t\t\t\t\t\t     run, cluster, char, glyph, caret, slope\n";

static void
render_callback (PangoLayout *layout,
                 int          x,
                 int          y,
                 gpointer     context,
                 gpointer     state)
{
  cairo_t *cr = (cairo_t *) context;
  int annotate = (GPOINTER_TO_INT (state) + opt_annotate) % ANNOTATE_LAST;

  cairo_save (cr);
  cairo_translate (cr, x, y);

  if (annotate != 0)
    {
      cairo_pattern_t *pattern;
      PangoRectangle ink, logical;
      double lw = cairo_get_line_width (cr);
      PangoLayoutIter* iter;

      pango_layout_get_extents (layout, &ink, &logical);

      if (annotate & ANNOTATE_GRAVITY_ROOF)
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
       }

      if (annotate & ANNOTATE_BLOCK_PROGRESSION)
        {
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

      if (annotate & ANNOTATE_BASELINES)
        {
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
        }

#if HB_VERSION_ATLEAST(4,0,0)
      if (annotate & ANNOTATE_RUN_BASELINES)
        {
          hb_ot_layout_baseline_tag_t baseline_tag = 0;
          /* draw baselines for runs in blue */
          cairo_save (cr);
          cairo_set_source_rgba (cr, 0.0, 0.0, 1.0, 0.5);

          iter = pango_layout_get_iter (layout);
          do
            {
              PangoLayoutRun *run;
              PangoRectangle rect;
              hb_font_t *hb_font;
              hb_ot_layout_baseline_tag_t baselines[] = {
                HB_OT_LAYOUT_BASELINE_TAG_ROMAN,
                HB_OT_LAYOUT_BASELINE_TAG_HANGING,
                HB_OT_LAYOUT_BASELINE_TAG_IDEO_FACE_BOTTOM_OR_LEFT,
                HB_OT_LAYOUT_BASELINE_TAG_IDEO_FACE_TOP_OR_RIGHT,
                HB_OT_LAYOUT_BASELINE_TAG_IDEO_EMBOX_BOTTOM_OR_LEFT,
                HB_OT_LAYOUT_BASELINE_TAG_IDEO_EMBOX_CENTRAL,
                HB_OT_LAYOUT_BASELINE_TAG_IDEO_EMBOX_TOP_OR_RIGHT,
                HB_OT_LAYOUT_BASELINE_TAG_MATH
              };
              hb_direction_t dir;
              hb_tag_t script, lang;
              hb_position_t coord;

              run = pango_layout_iter_get_run (iter);
              if (!run)
                {
                  baseline_tag = 0;
                  continue;
                }

              if (baseline_tag == 0)
                {
                  hb_script_t script = (hb_script_t) g_unicode_script_to_iso15924 (run->item->analysis.script);

                  if (run->item->analysis.flags & PANGO_ANALYSIS_FLAG_CENTERED_BASELINE)
                    baseline_tag = HB_OT_LAYOUT_BASELINE_TAG_IDEO_EMBOX_CENTRAL;
                  else
                    baseline_tag = hb_ot_layout_get_horizontal_baseline_tag_for_script (script);
                }

              y = pango_layout_iter_get_run_baseline (iter);
              pango_layout_iter_get_run_extents (iter, NULL, &rect);

              hb_font = pango_font_get_hb_font (run->item->analysis.font);
              if (run->item->analysis.flags & PANGO_ANALYSIS_FLAG_CENTERED_BASELINE)
                dir = HB_DIRECTION_TTB;
              else
                dir = HB_DIRECTION_LTR;
              script = (hb_script_t) g_unicode_script_to_iso15924 (run->item->analysis.script);
              lang = HB_TAG_NONE;

              for (int i = 0; i < G_N_ELEMENTS (baselines); i++)
                {
                  char buf[5] = { 0, };

                  hb_tag_to_string (baselines[i], buf);

                  cairo_save (cr);

                  if (baselines[i] == baseline_tag)
                    cairo_set_source_rgba (cr, 1.0, 0.0, 0.0, 0.5);
                  else
                    cairo_set_source_rgba (cr, 0.0, 0.0, 1.0, 0.5);

                  if (hb_ot_layout_get_baseline (hb_font,
                                                 baselines[i],
                                                 dir,
                                                 script,
                                                 lang,
                                                 &coord))
                    {
                      g_print ("baseline %s, value %d\n", buf, coord);
                      cairo_set_dash (cr, NULL, 0, 0);
                    }
                  else
                    {
                      double dashes[] = { 4 * lw, 4 * lw };

                      hb_ot_layout_get_baseline_with_fallback (hb_font,
                                                               baselines[i],
                                                               dir,
                                                               script,
                                                               lang,
                                                               &coord);

                      g_print ("baseline %s, fallback value %d\n", buf, coord);
                      cairo_set_dash (cr, dashes, 2, 0);
                      cairo_set_line_cap (cr, CAIRO_LINE_CAP_SQUARE);
                    }
                  cairo_move_to (cr,
                                 (double)rect.x / PANGO_SCALE,
                                 (double)(y - coord) / PANGO_SCALE);
                  cairo_rel_line_to (cr, (double)rect.width / PANGO_SCALE, 0);
                  cairo_stroke (cr);
                  cairo_set_source_rgb (cr, 0, 0, 0);
                  cairo_move_to (cr,
                                 (double)rect.x / PANGO_SCALE - 5,
                                 (double)(y - coord) / PANGO_SCALE - 5);
                  cairo_show_text (cr, buf);

                  cairo_restore (cr);
                }
            }
          while (pango_layout_iter_next_run (iter));
          pango_layout_iter_free (iter);
          cairo_restore (cr);
        }
#endif

      if (annotate & ANNOTATE_LAYOUT_EXTENTS)
        {
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

      if (annotate & ANNOTATE_LINE_EXTENTS)
        {
          /* draw the logical rects for lines in red */
          cairo_save (cr);
          cairo_set_source_rgba (cr, 1.0, 0.0, 0.0, 0.5);

          iter = pango_layout_get_iter (layout);
          do
            {
              PangoRectangle rect;

              pango_layout_iter_get_line_extents (iter, NULL, &rect);
              cairo_rectangle (cr,
                               (double)rect.x / PANGO_SCALE - lw / 2,
                               (double)rect.y / PANGO_SCALE - lw / 2,
                               (double)rect.width / PANGO_SCALE + lw,
                               (double)rect.height / PANGO_SCALE + lw);
              cairo_stroke (cr);
            }
          while (pango_layout_iter_next_line (iter));
          pango_layout_iter_free (iter);
          cairo_restore (cr);
        }

      if (annotate & ANNOTATE_RUN_EXTENTS)
        {
          /* draw the logical rects for runs in blue */
          cairo_save (cr);
          cairo_set_source_rgba (cr, 0.0, 0.0, 1.0, 0.5);

          iter = pango_layout_get_iter (layout);
          do
            {
              PangoLayoutRun *run;
              PangoRectangle rect;

              run = pango_layout_iter_get_run (iter);
              if (!run)
                continue;

              pango_layout_iter_get_run_extents (iter, NULL, &rect);
              cairo_rectangle (cr,
                               (double)rect.x / PANGO_SCALE - lw / 2,
                               (double)rect.y / PANGO_SCALE - lw / 2,
                               (double)rect.width / PANGO_SCALE + lw,
                               (double)rect.height / PANGO_SCALE + lw);
              cairo_stroke (cr);
            }
          while (pango_layout_iter_next_run (iter));
          pango_layout_iter_free (iter);
          cairo_restore (cr);
        }

      if (annotate & ANNOTATE_CLUSTER_EXTENTS)
        {
          /* draw the logical rects for clusters in purple */
          cairo_save (cr);
          cairo_set_source_rgba (cr, 1.0, 0.0, 1.0, 0.5);

          iter = pango_layout_get_iter (layout);
          do
            {
              PangoRectangle rect;

              pango_layout_iter_get_cluster_extents (iter, NULL, &rect);
              cairo_rectangle (cr,
                               (double)rect.x / PANGO_SCALE - lw / 2,
                               (double)rect.y / PANGO_SCALE - lw / 2,
                               (double)rect.width / PANGO_SCALE + lw,
                               (double)rect.height / PANGO_SCALE + lw);
              cairo_stroke (cr);
            }
          while (pango_layout_iter_next_cluster (iter));
          pango_layout_iter_free (iter);
          cairo_restore (cr);
        }

      if (annotate & ANNOTATE_CHAR_EXTENTS)
        {
          /* draw the logical rects for chars in orange */
          cairo_save (cr);
          cairo_set_source_rgba (cr, 1.0, 0.5, 0.0, 0.5);

          iter = pango_layout_get_iter (layout);
          do
            {
              PangoRectangle rect;

              pango_layout_iter_get_char_extents (iter, &rect);
              cairo_rectangle (cr,
                               (double)rect.x / PANGO_SCALE - lw / 2,
                               (double)rect.y / PANGO_SCALE - lw / 2,
                               (double)rect.width / PANGO_SCALE + lw,
                               (double)rect.height / PANGO_SCALE + lw);
              cairo_stroke (cr);
            }
          while (pango_layout_iter_next_cluster (iter));
          pango_layout_iter_free (iter);
          cairo_restore (cr);
        }

      if (annotate & ANNOTATE_GLYPH_EXTENTS)
        {
          /* draw the glyph_extents in blue */
          cairo_save (cr);
          cairo_set_source_rgba (cr, 0.0, 0.0, 1.0, 0.5);

          iter = pango_layout_get_iter (layout);
          do
            {
              PangoLayoutRun *run;
              PangoRectangle rect;
              int x_pos, y_pos;

              run = pango_layout_iter_get_run (iter);
              if (!run)
                continue;

              pango_layout_iter_get_run_extents (iter, NULL, &rect);

              x_pos = rect.x;
              y_pos = pango_layout_iter_get_run_baseline (iter);

              for (int i = 0; i < run->glyphs->num_glyphs; i++)
                {
                  PangoRectangle extents;

                  pango_font_get_glyph_extents (run->item->analysis.font,
                                                run->glyphs->glyphs[i].glyph,
                                                &extents, NULL);

                  rect.x = x_pos + run->glyphs->glyphs[i].geometry.x_offset + extents.x;
                  rect.y = y_pos + run->glyphs->glyphs[i].geometry.y_offset + extents.y;
                  rect.width = extents.width;
                  rect.height = extents.height;

                  cairo_rectangle (cr,
                                   (double)rect.x / PANGO_SCALE - lw / 2,
                                   (double)rect.y / PANGO_SCALE - lw / 2,
                                   (double)rect.width / PANGO_SCALE + lw,
                                   (double)rect.height / PANGO_SCALE + lw);
                  cairo_stroke (cr);

                  cairo_arc (cr,
                             (double) (x_pos + run->glyphs->glyphs[i].geometry.x_offset) / PANGO_SCALE,
                             (double) (y_pos + run->glyphs->glyphs[i].geometry.y_offset) / PANGO_SCALE,
                             3.0, 0, 2*G_PI);
                  cairo_fill (cr);

                  x_pos += run->glyphs->glyphs[i].geometry.width;
                }
            }
          while (pango_layout_iter_next_run (iter));
          pango_layout_iter_free (iter);
          cairo_restore (cr);
        }

      if (annotate & ANNOTATE_CARET_POSITIONS)
        {
          const PangoLogAttr *attrs;
          int n_attrs;
          int offset;
          int num = 0;

          /* draw the caret positions in purple */
          cairo_save (cr);
          cairo_set_source_rgba (cr, 1.0, 0.0, 1.0, 0.5);

          attrs = pango_layout_get_log_attrs_readonly (layout, &n_attrs);

          iter = pango_layout_get_iter (layout);
          do
            {
              PangoRectangle rect;
              PangoLayoutRun *run;
              const char *text, *start, *p;
              int x, y;
              gboolean trailing;

              pango_layout_iter_get_run_extents (iter, NULL, &rect);
              run = pango_layout_iter_get_run_readonly (iter);

              if (!run)
                continue;

              text = pango_layout_get_text (layout);
              start = text + run->item->offset;

              offset = g_utf8_strlen (text, start - text);

              y = pango_layout_iter_get_run_baseline (iter);

              trailing = FALSE;
              p = start;
              for (int i = 0; i <= run->item->num_chars; i++)
                {
                  if (attrs[offset + i].is_cursor_position)
                    {
                      pango_glyph_string_index_to_x_full (run->glyphs,
                                                          text + run->item->offset,
                                                          run->item->length,
                                                          &run->item->analysis,
                                                          (PangoLogAttr *)attrs + offset,
                                                          p - start,
                                                          trailing,
                                                          &x);
                      x += rect.x;

                      cairo_set_source_rgba (cr, 1.0, 0.0, 1.0, 0.5);
                      cairo_arc (cr, x / PANGO_SCALE, y / PANGO_SCALE, 3.0, 0, 2*G_PI);
                      cairo_close_path (cr);
                      cairo_fill (cr);

                      char *s = g_strdup_printf ("%d", num);
                      cairo_set_source_rgb (cr, 0, 0, 0);
                      cairo_move_to (cr, x / PANGO_SCALE - 5, y / PANGO_SCALE + 15);
                      cairo_show_text (cr, s);
                      g_free (s);
                   }

                  if (i < run->item->num_chars)
                    {
                      num++;
                      p = g_utf8_next_char (p);
                    }
                  else
                    trailing = TRUE;

                }
            }
          while (pango_layout_iter_next_run (iter));
          pango_layout_iter_free (iter);
          cairo_restore (cr);
        }

      if (annotate & ANNOTATE_CARET_SLOPE)
        {
          const char *text = pango_layout_get_text (layout);
          int length = g_utf8_strlen (text, -1);
          const PangoLogAttr *attrs;
          int n_attrs;
          const char *p;
          int i;

          /* draw the caret slope in gray */
          cairo_save (cr);
          cairo_set_source_rgba (cr, 0.0, 0.0, 0.0, 0.5);

          attrs = pango_layout_get_log_attrs_readonly (layout, &n_attrs);

          for (i = 0, p = text; i <= length; i++, p = g_utf8_next_char (p))
            {
              PangoRectangle rect;

              if (!attrs[i].is_cursor_position)
                continue;

              pango_layout_get_caret_pos (layout, p - text, &rect, NULL);

              cairo_move_to (cr,
                             (double)rect.x / PANGO_SCALE + (double)rect.width / PANGO_SCALE - lw / 2,
                             (double)rect.y / PANGO_SCALE - lw / 2);
              cairo_line_to (cr,
                             (double)rect.x / PANGO_SCALE - lw / 2,
                             (double)rect.y / PANGO_SCALE + (double)rect.height / PANGO_SCALE - lw / 2);
              cairo_stroke (cr);
            }

          cairo_restore (cr);
        }
    }

  cairo_move_to (cr, 0, 0);
  pango_cairo_show_layout (cr, layout);

  cairo_restore (cr);

  cairo_surface_flush (cairo_get_target (cr));
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

static gboolean
parse_annotate_arg (const char  *option_name,
                    const char  *value,
                    gpointer     data,
                    GError     **error)
{
  guint64 num;

  if (!g_ascii_string_to_unsigned (value, 10, 0, ANNOTATE_LAST - 1, &num, NULL))
    {
      char **parts;
      int i, j;

      parts = g_strsplit (value, ",", 0);
      num = 0;
      for (i = 0; parts[i]; i++)
        {
          for (j = 0; j < G_N_ELEMENTS (annotate_options); j++)
            {
              if (strcmp (parts[i], annotate_options[j].name) == 0 ||
                  strcmp (parts[i], annotate_options[j].short_name) == 0)
                {
                  num |= annotate_options[j].value;
                  break;
                }
            }

          if (j == G_N_ELEMENTS (annotate_options))
            {
              g_set_error (error,
                           G_OPTION_ERROR, G_OPTION_ERROR_FAILED,
                           "%s is not an allowed value for %s. "
                           "See --help-cairo", parts[i], option_name);
              g_strfreev (parts);
              return FALSE;
            }
        }

      g_strfreev (parts);
    }

  opt_annotate = num;
  return TRUE;
}

static GOptionGroup *
pangocairo_view_get_option_group (const PangoViewer *klass G_GNUC_UNUSED)
{
  GOptionEntry entries[] =
  {
    {"annotate", 0, 0, G_OPTION_ARG_CALLBACK, parse_annotate_arg, annotate_arg_help, "FLAGS"},
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
