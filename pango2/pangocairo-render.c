/* Pango2
 * pangocairo-render.c: Rendering routines to Cairo surfaces
 *
 * Copyright (C) 2004 Red Hat, Inc.
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Library General Public
 * License as published by the Free Software Foundation; either
 * version 2 of the License, or (at your option) any later version.
 *
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Library General Public License for more details.
 *
 * You should have received a copy of the GNU Library General Public
 * License along with this library; if not, write to the
 * Free Software Foundation, Inc., 59 Temple Place - Suite 330,
 * Boston, MA 02111-1307, USA.
 */

#include "config.h"

#include <math.h>

#include "pango-item-private.h"
#include "pango-font-private.h"
#include "pangocairo-private.h"
#include "pango-glyph-item-private.h"
#include "pango-glyph-iter-private.h"
#include "pango-run-private.h"
#include "pango-impl-utils.h"
#include "pango-hbfont-private.h"
#include "pango-attr-private.h"
#include "pango-context-private.h"


typedef struct _Pango2CairoRendererClass Pango2CairoRendererClass;

#define PANGO2_CAIRO_RENDERER_CLASS(klass)    (G_TYPE_CHECK_CLASS_CAST ((klass), PANGO2_TYPE_CAIRO_RENDERER, Pango2CairoRendererClass))
#define PANGO2_IS_CAIRO_RENDERER_CLASS(klass) (G_TYPE_CHECK_CLASS_TYPE ((klass), PANGO2_TYPE_CAIRO_RENDERER))
#define PANGO2_CAIRO_RENDERER_GET_CLASS(obj)  (G_TYPE_INSTANCE_GET_CLASS ((obj), PANGO2_TYPE_CAIRO_RENDERER, Pango2CairoRendererClass))

struct _Pango2CairoRenderer
{
  Pango2Renderer parent_instance;

  cairo_t *cr;
  gboolean do_path;
  gboolean has_show_text_glyphs;
  double x_offset, y_offset;
  GQuark palette;

  /* house-keeping options */
  gboolean is_cached_renderer;
  gboolean cr_had_current_point;
};

struct _Pango2CairoRendererClass
{
  Pango2RendererClass parent_class;
};

G_DEFINE_TYPE (Pango2CairoRenderer, pango2_cairo_renderer, PANGO2_TYPE_RENDERER)

static void
set_color (Pango2CairoRenderer *crenderer,
           Pango2RenderPart     part)
{
  Pango2Color *color = pango2_renderer_get_color ((Pango2Renderer *) (crenderer), part);
  double red, green, blue, alpha;

  if (!color)
    return;

  if (color)
    {
      red = color->red / 65535.;
      green = color->green / 65535.;
      blue = color->blue / 65535.;
      alpha = color->alpha / 65535.;
    }
  else
    {
      cairo_pattern_t *pattern = cairo_get_source (crenderer->cr);

      if (pattern && cairo_pattern_get_type (pattern) == CAIRO_PATTERN_TYPE_SOLID)
        cairo_pattern_get_rgba (pattern, &red, &green, &blue, &alpha);
      else
        {
          red = 0.;
          green = 0.;
          blue = 0.;
          alpha = 1.;
        }
    }

  cairo_set_source_rgba (crenderer->cr, red, green, blue, alpha);
}

/* note: modifies crenderer->cr without doing cairo_save/restore() */
static void
_pango2_cairo_renderer_draw_frame (Pango2CairoRenderer *crenderer,
                                   double               x,
                                   double               y,
                                   double               width,
                                   double               height,
                                   double               line_width,
                                   gboolean             invalid)
{
  cairo_t *cr = crenderer->cr;

  if (crenderer->do_path)
    {
      double d2 = line_width * .5, d = line_width;

      /* we draw an outer box in one winding direction and an inner one in the
       * opposite direction.  This works for both cairo windings rules.
       *
       * what we really want is cairo_stroke_to_path(), but that's not
       * implemented in cairo yet.
       */

      /* outer */
      cairo_rectangle (cr, x-d2, y-d2, width+d, height+d);

      /* inner */
      if (invalid)
        {
          /* delicacies of computing the joint... this is REALLY slow */

          double alpha, tan_alpha2, cos_alpha;
          double sx, sy;

          alpha = atan2 (height, width);

          tan_alpha2 = tan (alpha * .5);
          if (tan_alpha2 < 1e-5 || (sx = d2 / tan_alpha2, 2. * sx > width - d))
            sx = (width - d) * .5;

          cos_alpha = cos (alpha);
          if (cos_alpha < 1e-5 || (sy = d2 / cos_alpha, 2. * sy > height - d))
            sy = (height - d) * .5;

          /* top triangle */
          cairo_new_sub_path (cr);
          cairo_line_to (cr, x+width-sx, y+d2);
          cairo_line_to (cr, x+sx, y+d2);
          cairo_line_to (cr, x+.5*width, y+.5*height-sy);
          cairo_close_path (cr);

          /* bottom triangle */
          cairo_new_sub_path (cr);
          cairo_line_to (cr, x+width-sx, y+height-d2);
          cairo_line_to (cr, x+.5*width, y+.5*height+sy);
          cairo_line_to (cr, x+sx, y+height-d2);
          cairo_close_path (cr);


          alpha = G_PI_2 - alpha;
          tan_alpha2 = tan (alpha * .5);
          if (tan_alpha2 < 1e-5 || (sy = d2 / tan_alpha2, 2. * sy > height - d))
            sy = (height - d) * .5;

          cos_alpha = cos (alpha);
          if (cos_alpha < 1e-5 || (sx = d2 / cos_alpha, 2. * sx > width - d))
            sx = (width - d) * .5;

          /* left triangle */
          cairo_new_sub_path (cr);
          cairo_line_to (cr, x+d2, y+sy);
          cairo_line_to (cr, x+d2, y+height-sy);
          cairo_line_to (cr, x+.5*width-sx, y+.5*height);
          cairo_close_path (cr);

          /* right triangle */
          cairo_new_sub_path (cr);
          cairo_line_to (cr, x+width-d2, y+sy);
          cairo_line_to (cr, x+.5*width+sx, y+.5*height);
          cairo_line_to (cr, x+width-d2, y+height-sy);
          cairo_close_path (cr);
        }
      else
        cairo_rectangle (cr, x+width-d2, y+d2, - (width-d), height-d);
    }
  else
    {
      cairo_rectangle (cr, x, y, width, height);

      if (invalid)
        {
          /* draw an X */

          cairo_new_sub_path (cr);
          cairo_move_to (cr, x, y);
          cairo_rel_line_to (cr, width, height);

          cairo_new_sub_path (cr);
          cairo_move_to (cr, x + width, y);
          cairo_rel_line_to (cr, -width, height);

          cairo_set_line_cap (cr, CAIRO_LINE_CAP_BUTT);
        }

      cairo_set_line_width (cr, line_width);
      cairo_set_line_join (cr, CAIRO_LINE_JOIN_MITER);
      cairo_set_miter_limit (cr, 2.);
      cairo_stroke (cr);
    }
}

static void
_pango2_cairo_renderer_draw_box_glyph (Pango2CairoRenderer *crenderer,
                                       Pango2GlyphInfo     *gi,
                                       double               cx,
                                       double               cy,
                                       gboolean             invalid)
{
  cairo_save (crenderer->cr);

  _pango2_cairo_renderer_draw_frame (crenderer,
                                     cx + 1.5,
                                     cy + 1.5 - PANGO2_UNKNOWN_GLYPH_HEIGHT,
                                     (double)gi->geometry.width / PANGO2_SCALE - 3.0,
                                     PANGO2_UNKNOWN_GLYPH_HEIGHT - 3.0,
                                     1.0,
                                     invalid);

  cairo_restore (crenderer->cr);
}

static void
_pango2_cairo_renderer_draw_unknown_glyph (Pango2CairoRenderer *crenderer,
                                           Pango2Font          *font,
                                           Pango2GlyphInfo     *gi,
                                           double               cx,
                                           double               cy)
{
  char buf[7];
  double x0, y0;
  int row, col;
  int rows, cols;
  double width, lsb;
  char hexbox_string[2] = { 0, 0 };
  Pango2CairoFontHexBoxInfo *hbi;
  gunichar ch;
  gboolean invalid_input;
  const char *p;
  const char *name;

  cairo_save (crenderer->cr);

  ch = gi->glyph & ~PANGO2_GLYPH_UNKNOWN_FLAG;
  invalid_input = G_UNLIKELY (gi->glyph == PANGO2_GLYPH_INVALID_INPUT || ch > 0x10FFFF);

  if (PANGO2_IS_HB_FONT (font))
    hbi = PANGO2_HB_FONT (font)->hex_box_info;
  else
    hbi = _pango2_cairo_font_get_hex_box_info (font);
  if (!hbi || !_pango2_cairo_font_install ((Pango2Font *)(hbi->font), 0, crenderer->cr))
    {
      _pango2_cairo_renderer_draw_box_glyph (crenderer, gi, cx, cy, invalid_input);
      goto done;
    }

  if (G_UNLIKELY (invalid_input))
    {
      rows = hbi->rows;
      cols = 1;
    }
  else if (ch == 0x2423 ||
           g_unichar_type (ch) == G_UNICODE_SPACE_SEPARATOR)
    {
      /* We never want to show a hex box or other drawing for
       * space. If we want space to be visible, we replace 0x20
       * by 0x2423 (visible space).
       *
       * Since we don't want to rely on glyph availability,
       * we render a centered dot ourselves.
       */
      double x = cx + 0.5 *((double)gi->geometry.width / PANGO2_SCALE);
      double y = cy + hbi->box_descent - 0.5 * hbi->box_height;

      cairo_new_sub_path (crenderer->cr);
      cairo_arc (crenderer->cr, x, y, 1.5 * hbi->line_width, 0, 2 * G_PI);
      cairo_close_path (crenderer->cr);
      cairo_fill (crenderer->cr);
      goto done;
    }
  else if (ch == '\t')
    {
      /* Since we don't want to rely on glyph availability,
       * we render an arrow like ↦ ourselves.
       */
      double y = cy + hbi->box_descent - 0.5 * hbi->box_height;
      double width = (double)gi->geometry.width / PANGO2_SCALE;
      double offset = 0.2 * width;
      double x = cx + offset;
      double al = width - 2 * offset; /* arrow length */
      double tl = MIN (hbi->digit_width, 0.75 * al); /* tip length */
      double tw2 = 2.5 * hbi->line_width; /* tip width / 2 */
      double lw2 = 0.5 * hbi->line_width; /* line width / 2 */

      cairo_move_to (crenderer->cr, x - lw2, y - tw2);
      cairo_line_to (crenderer->cr, x + lw2, y - tw2);
      cairo_line_to (crenderer->cr, x + lw2, y - lw2);
      cairo_line_to (crenderer->cr, x + al - tl, y - lw2);
      cairo_line_to (crenderer->cr, x + al - tl, y - tw2);
      cairo_line_to (crenderer->cr, x + al,  y);
      cairo_line_to (crenderer->cr, x + al - tl, y + tw2);
      cairo_line_to (crenderer->cr, x + al - tl, y + lw2);
      cairo_line_to (crenderer->cr, x + lw2, y + lw2);
      cairo_line_to (crenderer->cr, x + lw2, y + tw2);
      cairo_line_to (crenderer->cr, x - lw2, y + tw2);
      cairo_close_path (crenderer->cr);
      cairo_fill (crenderer->cr);
      goto done;
    }
  else if (ch == '\n' || ch == 0x2028 || ch == 0x2029)
    {
      /* Since we don't want to rely on glyph availability,
       * we render an arrow like ↵ ourselves.
       */
      double width = (double)gi->geometry.width / PANGO2_SCALE;
      double offset = 0.2 * width;
      double al = width - 2 * offset; /* arrow length */
      double tl = MIN (hbi->digit_width, 0.75 * al); /* tip length */
      double ah = al - 0.5 * tl; /* arrow height */
      double tw2 = 2.5 * hbi->line_width; /* tip width / 2 */
      double x = cx + offset;
      double y = cy - (hbi->box_height - al) / 2;
      double lw2 = 0.5 * hbi->line_width; /* line width / 2 */

      cairo_move_to (crenderer->cr, x, y);
      cairo_line_to (crenderer->cr, x + tl, y - tw2);
      cairo_line_to (crenderer->cr, x + tl, y - lw2);
      cairo_line_to (crenderer->cr, x + al - lw2, y - lw2);
      cairo_line_to (crenderer->cr, x + al - lw2, y - ah);
      cairo_line_to (crenderer->cr, x + al + lw2, y - ah);
      cairo_line_to (crenderer->cr, x + al + lw2, y + lw2);
      cairo_line_to (crenderer->cr, x + tl, y + lw2);
      cairo_line_to (crenderer->cr, x + tl, y + tw2);
      cairo_close_path (crenderer->cr);
      cairo_fill (crenderer->cr);
      goto done;
    }
  else if ((name = pango2_get_ignorable_size (ch, &rows, &cols)))
    {
      /* Nothing else to do, we render 'default ignorable' chars
       * as hex box with their nick.
       */
    }
  else
    {
      /* Everything else gets a traditional hex box. */
      rows = hbi->rows;
      cols = (ch > 0xffff ? 6 : 4) / rows;
      g_snprintf (buf, sizeof(buf), (ch > 0xffff) ? "%06X" : "%04X", ch);
      name = buf;
    }

  width = (3 * hbi->pad_x + cols * (hbi->digit_width + hbi->pad_x));
  lsb = ((double)gi->geometry.width / PANGO2_SCALE - width) * .5;
  lsb = floor (lsb / hbi->pad_x) * hbi->pad_x;

  _pango2_cairo_renderer_draw_frame (crenderer,
                                     cx + lsb + .5 * hbi->pad_x,
                                     cy + hbi->box_descent - hbi->box_height + hbi->pad_y * 0.5,
                                     width - hbi->pad_x,
                                     (hbi->box_height - hbi->pad_y),
                                     hbi->line_width,
                                     invalid_input);

  if (invalid_input)
    goto done;

  x0 = cx + lsb + hbi->pad_x * 2;
  y0 = cy + hbi->box_descent - hbi->pad_y * 2 - ((hbi->rows - rows) * hbi->digit_height / 2);

  for (row = 0, p = name; row < rows; row++)
    {
      double y = y0 - (rows - 1 - row) * (hbi->digit_height + hbi->pad_y);
      for (col = 0; col < cols; col++, p++)
        {
          double x = x0 + col * (hbi->digit_width + hbi->pad_x);

          if (!p)
            goto done;

          cairo_move_to (crenderer->cr, x, y);

          hexbox_string[0] = p[0];

          if (crenderer->do_path)
              cairo_text_path (crenderer->cr, hexbox_string);
          else
              cairo_show_text (crenderer->cr, hexbox_string);
        }
    }

done:
  cairo_restore (crenderer->cr);
}

#ifndef STACK_BUFFER_SIZE
#define STACK_BUFFER_SIZE (512 * sizeof (int))
#endif

#define STACK_ARRAY_LENGTH(T) (STACK_BUFFER_SIZE / sizeof(T))

static void
pango2_cairo_renderer_show_text_glyphs (Pango2Renderer       *renderer,
                                        const char           *text,
                                        int                   text_len,
                                        Pango2GlyphString    *glyphs,
                                        cairo_text_cluster_t *clusters,
                                        int                   num_clusters,
                                        gboolean              backward,
                                        Pango2Font           *font,
                                        GQuark                palette,
                                        int                   x,
                                        int                   y)
{
  Pango2CairoRenderer *crenderer = (Pango2CairoRenderer *) (renderer);
  int i, count;
  int x_position = 0;
  cairo_glyph_t *cairo_glyphs;
  cairo_glyph_t stack_glyphs[STACK_ARRAY_LENGTH (cairo_glyph_t)];
  double base_x = crenderer->x_offset + (double)x / PANGO2_SCALE;
  double base_y = crenderer->y_offset + (double)y / PANGO2_SCALE;

  cairo_save (crenderer->cr);
  if (!crenderer->do_path)
    set_color (crenderer, PANGO2_RENDER_PART_FOREGROUND);

  if (!_pango2_cairo_font_install (font, palette, crenderer->cr))
    {
      for (i = 0; i < glyphs->num_glyphs; i++)
        {
          Pango2GlyphInfo *gi = &glyphs->glyphs[i];

          if (gi->glyph != PANGO2_GLYPH_EMPTY)
            {
              double cx = base_x + (double)(x_position + gi->geometry.x_offset) / PANGO2_SCALE;
              double cy = gi->geometry.y_offset == 0 ?
                          base_y :
                          base_y + (double)(gi->geometry.y_offset) / PANGO2_SCALE;

              _pango2_cairo_renderer_draw_unknown_glyph (crenderer, font, gi, cx, cy);
            }
          x_position += gi->geometry.width;
        }

      goto done;
    }

  if (glyphs->num_glyphs > (int) G_N_ELEMENTS (stack_glyphs))
    cairo_glyphs = g_new (cairo_glyph_t, glyphs->num_glyphs);
  else
    cairo_glyphs = stack_glyphs;

  count = 0;
  for (i = 0; i < glyphs->num_glyphs; i++)
    {
      Pango2GlyphInfo *gi = &glyphs->glyphs[i];

      if (gi->glyph != PANGO2_GLYPH_EMPTY)
        {
          double cx = base_x + (double)(x_position + gi->geometry.x_offset) / PANGO2_SCALE;
          double cy = gi->geometry.y_offset == 0 ?
                      base_y :
                      base_y + (double)(gi->geometry.y_offset) / PANGO2_SCALE;

          if (gi->glyph & PANGO2_GLYPH_UNKNOWN_FLAG)
            {
              if (gi->glyph == (0x20 | PANGO2_GLYPH_UNKNOWN_FLAG))
                ; /* no hex boxes for space, please */
              else
                _pango2_cairo_renderer_draw_unknown_glyph (crenderer, font, gi, cx, cy);
            }
          else
            {
              cairo_glyphs[count].index = gi->glyph;
              cairo_glyphs[count].x = cx;
              cairo_glyphs[count].y = cy;
              count++;
            }
        }
      x_position += gi->geometry.width;
    }

  if (G_UNLIKELY (crenderer->do_path))
    cairo_glyph_path (crenderer->cr, cairo_glyphs, count);
  else
    if (G_UNLIKELY (clusters))
      cairo_show_text_glyphs (crenderer->cr,
                              text, text_len,
                              cairo_glyphs, count,
                              clusters, num_clusters,
                              backward ? CAIRO_TEXT_CLUSTER_FLAG_BACKWARD : 0);
    else
      cairo_show_glyphs (crenderer->cr, cairo_glyphs, count);

  if (cairo_glyphs != stack_glyphs)
    g_free (cairo_glyphs);

done:
  cairo_restore (crenderer->cr);
}

static void
pango2_cairo_renderer_draw_glyphs (Pango2Renderer     *renderer,
                                   Pango2Font         *font,
                                   Pango2GlyphString  *glyphs,
                                   int                 x,
                                   int                 y)
{
  Pango2CairoRenderer *crenderer = (Pango2CairoRenderer *) (renderer);
  pango2_cairo_renderer_show_text_glyphs (renderer, NULL, 0, glyphs, NULL, 0, FALSE, font, crenderer->palette, x, y);
}

static void
pango2_cairo_renderer_draw_run (Pango2Renderer *renderer,
                                const char     *text,
                                Pango2Run      *run,
                                int             x,
                                int             y)
{
  Pango2CairoRenderer *crenderer = (Pango2CairoRenderer *) (renderer);
  Pango2Item *item = pango2_run_get_item (run);
  Pango2GlyphString *glyphs = pango2_run_get_glyphs (run);
  Pango2Font *font = item->analysis.font;
  gboolean backward  = (item->analysis.level & 1) != 0;
  Pango2GlyphItem *glyph_item = pango2_run_get_glyph_item (run);
  Pango2GlyphItemIter iter;
  cairo_text_cluster_t *cairo_clusters;
  cairo_text_cluster_t stack_clusters[STACK_ARRAY_LENGTH (cairo_text_cluster_t)];
  int num_clusters;

  if (!crenderer->has_show_text_glyphs || crenderer->do_path)
    {
      pango2_cairo_renderer_show_text_glyphs (renderer, NULL, 0, glyphs, NULL, 0, FALSE, font, crenderer->palette, x, y);
      return;
    }

  if (glyphs->num_glyphs > (int) G_N_ELEMENTS (stack_clusters))
    cairo_clusters = g_new (cairo_text_cluster_t, glyphs->num_glyphs);
  else
    cairo_clusters = stack_clusters;

  num_clusters = 0;
  if (pango2_glyph_item_iter_init_start (&iter, glyph_item, text))
    {
      do {
        int num_bytes, num_glyphs, i;

        num_bytes  = iter.end_index - iter.start_index;
        num_glyphs = backward ? iter.start_glyph - iter.end_glyph : iter.end_glyph - iter.start_glyph;

        if (num_bytes < 1)
          g_warning ("pango2_cairo_renderer_draw_glyph_item: bad cluster has num_bytes %d", num_bytes);
        if (num_glyphs < 1)
          g_warning ("pango2_cairo_renderer_draw_glyph_item: bad cluster has num_glyphs %d", num_glyphs);

        /* Discount empty and unknown glyphs */
        for (i = MIN (iter.start_glyph, iter.end_glyph+1);
             i < MAX (iter.start_glyph+1, iter.end_glyph);
             i++)
          {
            Pango2GlyphInfo *gi = &glyphs->glyphs[i];

            if (gi->glyph == PANGO2_GLYPH_EMPTY ||
                gi->glyph & PANGO2_GLYPH_UNKNOWN_FLAG)
              num_glyphs--;
          }

        cairo_clusters[num_clusters].num_bytes  = num_bytes;
        cairo_clusters[num_clusters].num_glyphs = num_glyphs;
        num_clusters++;
      } while (pango2_glyph_item_iter_next_cluster (&iter));
    }

  pango2_cairo_renderer_show_text_glyphs (renderer,
                                          text + item->offset, item->length,
                                          glyphs,
                                          cairo_clusters, num_clusters,
                                          backward,
                                          font,
                                          crenderer->palette,
                                          x, y);

  if (cairo_clusters != stack_clusters)
    g_free (cairo_clusters);
}

static void
pango2_cairo_renderer_draw_rectangle (Pango2Renderer   *renderer,
                                      Pango2RenderPart  part,
                                      int               x,
                                      int               y,
                                      int               width,
                                      int               height)
{
  Pango2CairoRenderer *crenderer = (Pango2CairoRenderer *) (renderer);

  if (!crenderer->do_path)
    {
      cairo_save (crenderer->cr);

      set_color (crenderer, part);
    }

  cairo_rectangle (crenderer->cr,
                   crenderer->x_offset + (double)x / PANGO2_SCALE,
                   crenderer->y_offset + (double)y / PANGO2_SCALE,
                   (double)width / PANGO2_SCALE, (double)height / PANGO2_SCALE);

  if (!crenderer->do_path)
    {
      cairo_fill (crenderer->cr);

      cairo_restore (crenderer->cr);
    }
}

static void
pango2_cairo_renderer_draw_trapezoid (Pango2Renderer   *renderer,
                                      Pango2RenderPart  part,
                                      double            y1_,
                                      double            x11,
                                      double            x21,
                                      double            y2,
                                      double            x12,
                                      double            x22)
{
  Pango2CairoRenderer *crenderer = (Pango2CairoRenderer *) (renderer);
  cairo_t *cr;
  double x, y;

  cr = crenderer->cr;

  cairo_save (cr);

  if (!crenderer->do_path)
    set_color (crenderer, part);

  x = crenderer->x_offset,
  y = crenderer->y_offset;
  cairo_user_to_device_distance (cr, &x, &y);
  cairo_identity_matrix (cr);
  cairo_translate (cr, x, y);

  cairo_move_to (cr, x11, y1_);
  cairo_line_to (cr, x21, y1_);
  cairo_line_to (cr, x22, y2);
  cairo_line_to (cr, x12, y2);
  cairo_close_path (cr);

  if (!crenderer->do_path)
    cairo_fill (cr);

  cairo_restore (cr);
}

/* Draws an error underline that looks like one of:
 *              H       E                H
 *     /\      /\      /\        /\      /\               -
 *   A/  \    /  \    /  \     A/  \    /  \              |
 *    \   \  /    \  /   /D     \   \  /    \             |
 *     \   \/  C   \/   /        \   \/   C  \            | height = HEIGHT_SQUARES * square
 *      \      /\  F   /          \  F   /\   \           |
 *       \    /  \    /            \    /  \   \G         |
 *        \  /    \  /              \  /    \  /          |
 *         \/      \/                \/      \/           -
 *         B                         B
 *         |---|
 *       unit_width = (HEIGHT_SQUARES - 1) * square
 *
 * The x, y, width, height passed in give the desired bounding box;
 * x/width are adjusted to make the underline a integer number of units
 * wide.
 */
#define HEIGHT_SQUARES 2.5

static void
draw_wavy_line (cairo_t *cr,
                double   x,
                double   y,
                double   width,
                double   height)
{
  double square = height / HEIGHT_SQUARES;
  double unit_width = (HEIGHT_SQUARES - 1) * square;
  double double_width = 2 * unit_width;
  int width_units = (width + unit_width / 2) / unit_width;
  double y_top, y_bottom;
  double x_left, x_middle, x_right;
  int i;

  x += (width - width_units * unit_width) / 2;

  y_top = y;
  y_bottom = y + height;

  /* Bottom of squiggle */
  x_middle = x + unit_width;
  x_right  = x + double_width;
  cairo_move_to (cr, x - square / 2, y_top + square / 2); /* A */
  for (i = 0; i < width_units-2; i += 2)
    {
      cairo_line_to (cr, x_middle, y_bottom); /* B */
      cairo_line_to (cr, x_right, y_top + square); /* C */

      x_middle += double_width;
      x_right  += double_width;
    }
  cairo_line_to (cr, x_middle, y_bottom); /* B */

  if (i + 1 == width_units)
    cairo_line_to (cr, x_middle + square / 2, y_bottom - square / 2); /* G */
  else if (i + 2 == width_units) {
    cairo_line_to (cr, x_right + square / 2, y_top + square / 2); /* D */
    cairo_line_to (cr, x_right, y_top); /* E */
  }

  /* Top of squiggle */
  x_left = x_middle - unit_width;
  for (; i >= 0; i -= 2)
    {
      cairo_line_to (cr, x_middle, y_bottom - square); /* F */
      cairo_line_to (cr, x_left, y_top);   /* H */

      x_left   -= double_width;
      x_middle -= double_width;
    }
}

static void
pango2_cairo_renderer_draw_styled_line (Pango2Renderer   *renderer,
                                        Pango2RenderPart  part,
                                        Pango2LineStyle   style,
                                        int               x,
                                        int               y,
                                        int               width,
                                        int               height)
{
  Pango2CairoRenderer *crenderer = (Pango2CairoRenderer *) (renderer);
  cairo_t *cr = crenderer->cr;

  if (!crenderer->do_path)
    {
      cairo_save (cr);

      set_color (crenderer, part);

      cairo_new_path (cr);
    }

  switch (style)
    {
    case PANGO2_LINE_STYLE_NONE:
      break;

    case PANGO2_LINE_STYLE_DOTTED:
    case PANGO2_LINE_STYLE_DASHED:
      cairo_save (cr);
      cairo_set_line_width (cr, (double)height / PANGO2_SCALE);
      if (style == PANGO2_LINE_STYLE_DOTTED)
        {
          cairo_set_line_cap (cr, CAIRO_LINE_CAP_ROUND);
          cairo_set_dash (cr, (const double []){0., (double)(2 * height) / PANGO2_SCALE}, 2, 0.);
        }
      else
        {
          cairo_set_line_cap (cr, CAIRO_LINE_CAP_SQUARE);
          cairo_set_dash (cr, (const double []){(double)(3 * height) / PANGO2_SCALE, (double)(3 * height) / PANGO2_SCALE}, 2, 0.);
        }
      cairo_move_to (cr,
                     crenderer->x_offset + (double)x / PANGO2_SCALE + (double)height / (2 * PANGO2_SCALE),
                     crenderer->y_offset + (double)y / PANGO2_SCALE + (double)height / (2 * PANGO2_SCALE));
      cairo_line_to (cr,
                     crenderer->x_offset + (double)x / PANGO2_SCALE + (double)width / PANGO2_SCALE - (double)height / PANGO2_SCALE,
                     crenderer->y_offset + (double)y / PANGO2_SCALE + (double)height / (2 * PANGO2_SCALE));
      cairo_stroke (cr);
      cairo_restore (cr);
      break;

    case PANGO2_LINE_STYLE_SOLID:
      cairo_rectangle (cr,
                       crenderer->x_offset + (double)x / PANGO2_SCALE,
                       crenderer->y_offset + (double)y / PANGO2_SCALE,
                       (double)width / PANGO2_SCALE, (double)height / PANGO2_SCALE);
      break;

    case PANGO2_LINE_STYLE_DOUBLE:
      cairo_rectangle (cr,
                       crenderer->x_offset + (double)x / PANGO2_SCALE,
                       crenderer->y_offset + (double)y / PANGO2_SCALE,
                       (double)width / PANGO2_SCALE, (double)height / (3 * PANGO2_SCALE));
      cairo_rectangle (cr,
                       crenderer->x_offset + (double)x / PANGO2_SCALE,
                       crenderer->y_offset + (double)y / PANGO2_SCALE + (double)(2 * height) / (3 * PANGO2_SCALE),
                       (double)width / PANGO2_SCALE, (double)height / (3 * PANGO2_SCALE));
      break;


    case PANGO2_LINE_STYLE_WAVY:
      draw_wavy_line (cr,
                      crenderer->x_offset + (double)x / PANGO2_SCALE,
                      crenderer->y_offset + (double)y / PANGO2_SCALE,
                      (double)width / PANGO2_SCALE, (double)height / PANGO2_SCALE);
      break;
    default:
      g_assert_not_reached ();
    }

  if (!crenderer->do_path)
    {
      cairo_fill (cr);

      cairo_restore (cr);
    }
}

static GQuark
find_palette (Pango2Context *context,
              Pango2Item    *item)
{
  GSList *l;

  for (l = item->analysis.extra_attrs; l; l = l->next)
    {
      Pango2Attribute *attr = l->data;

      if (attr->type == PANGO2_ATTR_PALETTE)
        return g_quark_from_string (attr->str_value);
    }

  return context->palette;
}

static void
pango2_cairo_renderer_prepare_run (Pango2Renderer *renderer,
                                   Pango2Run      *run)
{
  Pango2CairoRenderer *crenderer = (Pango2CairoRenderer *) (renderer);

  PANGO2_RENDERER_CLASS (pango2_cairo_renderer_parent_class)->prepare_run (renderer, run);

  crenderer->palette = find_palette (pango2_renderer_get_context (renderer),
                                     pango2_run_get_item (run));
}

static void
pango2_cairo_renderer_init (Pango2CairoRenderer *renderer G_GNUC_UNUSED)
{
}

static void
pango2_cairo_renderer_class_init (Pango2CairoRendererClass *klass)
{
  Pango2RendererClass *renderer_class = PANGO2_RENDERER_CLASS (klass);

  renderer_class->draw_glyphs = pango2_cairo_renderer_draw_glyphs;
  renderer_class->draw_run = pango2_cairo_renderer_draw_run;
  renderer_class->draw_rectangle = pango2_cairo_renderer_draw_rectangle;
  renderer_class->draw_styled_line = pango2_cairo_renderer_draw_styled_line;
  renderer_class->draw_trapezoid = pango2_cairo_renderer_draw_trapezoid;
  renderer_class->prepare_run = pango2_cairo_renderer_prepare_run;
}

static Pango2CairoRenderer *cached_renderer = NULL; /* MT-safe */
G_LOCK_DEFINE_STATIC (cached_renderer);

static Pango2CairoRenderer *
acquire_renderer (void)
{
  Pango2CairoRenderer *renderer;

  if (G_LIKELY (G_TRYLOCK (cached_renderer)))
    {
      if (G_UNLIKELY (!cached_renderer))
        {
          cached_renderer = g_object_new (PANGO2_TYPE_CAIRO_RENDERER, NULL);
          cached_renderer->is_cached_renderer = TRUE;
        }

      renderer = cached_renderer;
    }
  else
    {
      renderer = g_object_new (PANGO2_TYPE_CAIRO_RENDERER, NULL);
    }

  return renderer;
}

static void
release_renderer (Pango2CairoRenderer *renderer)
{
  if (G_LIKELY (renderer->is_cached_renderer))
    {
      renderer->cr = NULL;
      renderer->do_path = FALSE;
      renderer->has_show_text_glyphs = FALSE;
      renderer->x_offset = 0.;
      renderer->y_offset = 0.;

      G_UNLOCK (cached_renderer);
    }
  else
    g_object_unref (renderer);
}

static void
save_current_point (Pango2CairoRenderer *renderer)
{
  renderer->cr_had_current_point = cairo_has_current_point (renderer->cr);
  cairo_get_current_point (renderer->cr, &renderer->x_offset, &renderer->y_offset);

  /* abuse save_current_point() to cache cairo_has_show_text_glyphs() result */
  renderer->has_show_text_glyphs = cairo_surface_has_show_text_glyphs (cairo_get_target (renderer->cr));
}

static void
restore_current_point (Pango2CairoRenderer *renderer)
{
  if (renderer->cr_had_current_point)
    /* XXX should do cairo_set_current_point() when we have that function */
    cairo_move_to (renderer->cr, renderer->x_offset, renderer->y_offset);
  else
    cairo_new_sub_path (renderer->cr);
}


/* convenience wrappers using the default renderer */


static void
_pango2_cairo_do_glyph_string (cairo_t           *cr,
                               Pango2Font        *font,
                               GQuark             palette,
                               Pango2GlyphString *glyphs,
                               gboolean           do_path)
{
  Pango2CairoRenderer *crenderer = acquire_renderer ();
  Pango2Renderer *renderer = (Pango2Renderer *) crenderer;

  crenderer->cr = cr;
  crenderer->do_path = do_path;
  crenderer->palette = palette;
  save_current_point (crenderer);

  if (!do_path)
    {
      /* unset all part colors, since when drawing just a glyph string,
       * prepare_run() isn't called.
       */

      pango2_renderer_activate (renderer);

      pango2_renderer_set_color (renderer, PANGO2_RENDER_PART_FOREGROUND, NULL);
      pango2_renderer_set_color (renderer, PANGO2_RENDER_PART_BACKGROUND, NULL);
      pango2_renderer_set_color (renderer, PANGO2_RENDER_PART_UNDERLINE, NULL);
      pango2_renderer_set_color (renderer, PANGO2_RENDER_PART_STRIKETHROUGH, NULL);
      pango2_renderer_set_color (renderer, PANGO2_RENDER_PART_OVERLINE, NULL);
    }

  pango2_renderer_draw_glyphs (renderer, font, glyphs, 0, 0);

  if (!do_path)
    {
      pango2_renderer_deactivate (renderer);
    }

  restore_current_point (crenderer);

  release_renderer (crenderer);
}

static void
_pango2_cairo_do_run (cairo_t    *cr,
                      const char *text,
                      Pango2Run  *run,
                      gboolean    do_path)
{
  Pango2CairoRenderer *crenderer = acquire_renderer ();
  Pango2Renderer *renderer = (Pango2Renderer *) crenderer;

  crenderer->cr = cr;
  crenderer->do_path = do_path;
  save_current_point (crenderer);

  if (!do_path)
    {
      /* unset all part colors, since when drawing just a glyph string,
       * prepare_run() isn't called.
       */

      pango2_renderer_activate (renderer);

      pango2_renderer_set_color (renderer, PANGO2_RENDER_PART_FOREGROUND, NULL);
      pango2_renderer_set_color (renderer, PANGO2_RENDER_PART_BACKGROUND, NULL);
      pango2_renderer_set_color (renderer, PANGO2_RENDER_PART_UNDERLINE, NULL);
      pango2_renderer_set_color (renderer, PANGO2_RENDER_PART_STRIKETHROUGH, NULL);
      pango2_renderer_set_color (renderer, PANGO2_RENDER_PART_OVERLINE, NULL);
    }

  pango2_renderer_draw_run (renderer, text, run, 0, 0);

  if (!do_path)
    pango2_renderer_deactivate (renderer);

  restore_current_point (crenderer);

  release_renderer (crenderer);
}

static void
_pango2_cairo_do_line (cairo_t    *cr,
                       Pango2Line *line,
                       gboolean    do_path)
{
  Pango2CairoRenderer *crenderer = acquire_renderer ();
  Pango2Renderer *renderer = (Pango2Renderer *) crenderer;

  crenderer->cr = cr;
  crenderer->do_path = do_path;
  save_current_point (crenderer);

  pango2_renderer_draw_line (renderer, line, 0, 0);

  restore_current_point (crenderer);

  release_renderer (crenderer);
}

static void
_pango2_cairo_do_lines (cairo_t     *cr,
                        Pango2Lines *lines,
                        gboolean     do_path)
{
  Pango2CairoRenderer *crenderer = acquire_renderer ();
  Pango2Renderer *renderer = (Pango2Renderer *) crenderer;

  crenderer->cr = cr;
  crenderer->do_path = do_path;
  save_current_point (crenderer);

  pango2_renderer_draw_lines (renderer, lines, 0, 0);

  restore_current_point (crenderer);

  release_renderer (crenderer);
}

static void
_pango2_cairo_do_layout (cairo_t      *cr,
                         Pango2Layout *layout,
                         gboolean      do_path)
{
  Pango2CairoRenderer *crenderer = acquire_renderer ();
  Pango2Renderer *renderer = (Pango2Renderer *) crenderer;

  crenderer->cr = cr;
  crenderer->do_path = do_path;
  save_current_point (crenderer);

  pango2_renderer_draw_lines (renderer, pango2_layout_get_lines (layout), 0, 0);

  restore_current_point (crenderer);

  release_renderer (crenderer);
}

/* public wrapper of above to show or append path */


/**
 * pango2_cairo_show_glyph_string:
 * @cr: a Cairo context
 * @font: a `Pango2Font` from a `Pango2CairoFontMap`
 * @glyphs: a `Pango2GlyphString`
 *
 * Draws the glyphs in @glyphs in the specified cairo context.
 *
 * The origin of the glyphs (the left edge of the baseline) will
 * be drawn at the current point of the cairo context.
 */
void
pango2_cairo_show_glyph_string (cairo_t           *cr,
                                Pango2Font        *font,
                                Pango2GlyphString *glyphs)
{
  g_return_if_fail (cr != NULL);
  g_return_if_fail (glyphs != NULL);

  _pango2_cairo_do_glyph_string (cr, font, 0, glyphs, FALSE);
}

/**
 * pango2_cairo_show_color_glyph_string:
 * @cr: a Cairo context
 * @font: a `Pango2Font` from a `Pango2CairoFontMap`
 * @palette: a palette name, as quark
 * @glyphs: a `Pango2GlyphString`
 *
 * Draws the glyphs in @glyphs in the specified cairo context,
 * and with the given palette name.
 *
 * This is a variation of [func@Pango2.cairo_show_glyph_string]
 * for use with fonts that have color palettes.
 */
void
pango2_cairo_show_color_glyph_string (cairo_t           *cr,
                                      Pango2Font        *font,
                                      GQuark             palette,
                                      Pango2GlyphString *glyphs)
{
  g_return_if_fail (cr != NULL);
  g_return_if_fail (glyphs != NULL);

  _pango2_cairo_do_glyph_string (cr, font, palette, glyphs, FALSE);
}

/**
 * pango2_cairo_show_run:
 * @cr: a Cairo context
 * @text: the UTF-8 text that @run refers to
 * @run: a `Pango2Run`
 *
 * Draws the glyphs in @run in the specified cairo context,
 * embedding the text associated with the glyphs in the output if the
 * output format supports it (PDF for example), otherwise it acts
 * similar to [func@Pango2.cairo_show_glyph_string].
 *
 * The origin of the glyphs (the left edge of the baseline) will
 * be drawn at the current point of the cairo context.
 *
 * Note that @text is the start of the text for layout, which is then
 * indexed by `run->item->offset`.
 */
void
pango2_cairo_show_run (cairo_t    *cr,
                       const char *text,
                       Pango2Run  *run)
{
  g_return_if_fail (cr != NULL);
  g_return_if_fail (text != NULL);
  g_return_if_fail (run != NULL);

  _pango2_cairo_do_run (cr, text, run, FALSE);
}

/**
 * pango2_cairo_show_line:
 * @cr: a Cairo context
 * @line: a `Pango2Line`
 *
 * Draws a `Pango2Line` in the specified cairo context.
 *
 * The origin of the glyphs (the left edge of the line) will
 * be drawn at the current point of the cairo context.
 */
void
pango2_cairo_show_line (cairo_t    *cr,
                        Pango2Line *line)
{
  g_return_if_fail (cr != NULL);
  g_return_if_fail (line != NULL);

  _pango2_cairo_do_line (cr, line, FALSE);
}

/**
 * pango2_cairo_show_lines:
 * @cr: a Cairo context
 * @lines: a `Pango2Lines` object
 *
 * Draws a `Pango2Lines` object in the specified cairo context.
 *
 * The top-left corner of the `Pango2Lines` will be drawn
 * at the current point of the cairo context.
 */
void
pango2_cairo_show_lines (cairo_t     *cr,
                         Pango2Lines *lines)
{
  g_return_if_fail (cr != NULL);
  g_return_if_fail (lines != NULL);

  _pango2_cairo_do_lines (cr, lines, FALSE);
}

/**
 * pango2_cairo_show_layout:
 * @cr: a Cairo context
 * @layout: a `Pango2Layout`
 *
 * Draws a `Pango2Layout` in the specified cairo context.
 *
 * The top-left corner of the `Pango2Layout` will be drawn
 * at the current point of the cairo context.
 */
void
pango2_cairo_show_layout (cairo_t      *cr,
                          Pango2Layout *layout)
{
  g_return_if_fail (cr != NULL);
  g_return_if_fail (PANGO2_IS_LAYOUT (layout));

  _pango2_cairo_do_layout (cr, layout, FALSE);
}

/**
 * pango2_cairo_glyph_string_path:
 * @cr: a Cairo context
 * @font: a `Pango2Font` from a `Pango2CairoFontMap`
 * @glyphs: a `Pango2GlyphString`
 *
 * Adds the glyphs in @glyphs to the current path in the specified
 * cairo context.
 *
 * The origin of the glyphs (the left edge of the baseline)
 * will be at the current point of the cairo context.
 */
void
pango2_cairo_glyph_string_path (cairo_t           *cr,
                                Pango2Font        *font,
                                Pango2GlyphString *glyphs)
{
  g_return_if_fail (cr != NULL);
  g_return_if_fail (glyphs != NULL);

  _pango2_cairo_do_glyph_string (cr, font, 0, glyphs, TRUE);
}

/**
 * pango2_cairo_run_path:
 * @cr: a Cairo context
 * @text: the UTF-8 text that @run refers to
 * @run: a `Pango2Run`
 *
 * Adds the text in `Pango2Run` to the current path in the
 * specified cairo context.
 *
 * The origin of the glyphs (the left edge of the line) will be
 * at the current point of the cairo context.
 */
void
pango2_cairo_run_path (cairo_t    *cr,
                       const char *text,
                       Pango2Run  *run)
{
  _pango2_cairo_do_run (cr, text, run, TRUE);
}

/**
 * pango2_cairo_line_path:
 * @cr: a Cairo context
 * @line: a `Pango2Line`
 *
 * Adds the text in `Pango2Line` to the current path in the
 * specified cairo context.
 *
 * The origin of the glyphs (the left edge of the line) will be
 * at the current point of the cairo context.
 */
void
pango2_cairo_line_path (cairo_t    *cr,
                        Pango2Line *line)
{
  g_return_if_fail (cr != NULL);

  _pango2_cairo_do_line (cr, line, TRUE);
}

/**
 * pango2_cairo_layout_path:
 * @cr: a Cairo context
 * @layout: a `Pango2Layout`
 *
 * Adds the text in a `Pango2Layout` to the current path in the
 * specified cairo context.
 *
 * The top-left corner of the `Pango2Layout` will be at the
 * current point of the cairo context.
 */
void
pango2_cairo_layout_path (cairo_t      *cr,
                          Pango2Layout *layout)
{
  g_return_if_fail (cr != NULL);
  g_return_if_fail (PANGO2_IS_LAYOUT (layout));

  _pango2_cairo_do_layout (cr, layout, TRUE);
}

/**
 * pango2_cairo_lines_path:
 * @cr: a Cairo context
 * @lines: a `Pango2Lines` object
 *
 * Adds the text in a `Pango2Lines` to the current path in the
 * specified cairo context.
 *
 * The top-left corner of the `Pango2Layout` will be at the
 * current point of the cairo context.
 */
void
pango2_cairo_lines_path (cairo_t     *cr,
                         Pango2Lines *lines)
{
  g_return_if_fail (cr != NULL);
  g_return_if_fail (PANGO2_IS_LINES (lines));

  _pango2_cairo_do_lines (cr, lines, TRUE);
}
