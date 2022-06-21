/* Pango
 * pango-renderer.h: Base class for rendering
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
#include <stdlib.h>

#include "pango-renderer.h"
#include "pango-impl-utils.h"
#include "pango-layout.h"
#include "pango-attr-private.h"
#include "pango-run-private.h"
#include "pango-line-private.h"
#include "pango-attributes-private.h"
#include "pango-glyph-item-private.h"

#define N_RENDER_PARTS 5

#define IS_VALID_PART(part) ((guint)part < N_RENDER_PARTS)

typedef struct _LineState LineState;
typedef struct _Point Point;
typedef struct _PangoRendererPrivate PangoRendererPrivate;

struct _Point
{
  double x, y;
};

struct _LineState
{
  PangoLineStyle underline;
  PangoUnderlinePosition underline_position;
  PangoRectangle underline_rect;

  PangoLineStyle strikethrough;
  PangoRectangle strikethrough_rect;
  int strikethrough_glyphs;

  PangoLineStyle overline;
  PangoRectangle overline_rect;

  int logical_rect_end;
};

struct _PangoRendererPrivate
{
  PangoLineStyle underline;
  PangoUnderlinePosition underline_position;
  PangoLineStyle strikethrough;
  PangoLineStyle overline;
  int active_count;

  PangoMatrix *matrix;
  PangoContext *context;

  PangoColor color[N_RENDER_PARTS];
  gboolean color_set[N_RENDER_PARTS];

  PangoLines *lines;
  PangoLine *line;
  LineState *line_state;
};

static void pango_renderer_finalize                     (GObject          *gobject);
static void pango_renderer_default_draw_glyphs          (PangoRenderer    *renderer,
                                                         PangoFont        *font,
                                                         PangoGlyphString *glyphs,
                                                         int               x,
                                                         int               y);
static void pango_renderer_default_draw_run             (PangoRenderer    *renderer,
                                                         const char       *text,
                                                         PangoRun         *run,
                                                         int               x,
                                                         int               y);
static void pango_renderer_default_draw_rectangle       (PangoRenderer    *renderer,
                                                         PangoRenderPart   part,
                                                         int               x,
                                                         int               y,
                                                         int               width,
                                                         int               height);
static void pango_renderer_default_draw_error_underline (PangoRenderer    *renderer,
                                                         int               x,
                                                         int               y,
                                                         int               width,
                                                         int               height);
static void pango_renderer_default_prepare_run          (PangoRenderer    *renderer,
                                                         PangoRun         *run);

static void pango_renderer_prepare_run                  (PangoRenderer    *renderer,
                                                         PangoRun         *run);

static void
to_device (PangoMatrix *matrix,
           double       x,
           double       y,
           Point       *result)
{
  if (matrix)
    {
      result->x = (x * matrix->xx + y * matrix->xy) / PANGO_SCALE + matrix->x0;
      result->y = (x * matrix->yx + y * matrix->yy) / PANGO_SCALE + matrix->y0;
    }
  else
    {
      result->x = x / PANGO_SCALE;
      result->y = y / PANGO_SCALE;
    }
}

G_DEFINE_ABSTRACT_TYPE_WITH_CODE (PangoRenderer, pango_renderer, G_TYPE_OBJECT,
                                  G_ADD_PRIVATE (PangoRenderer))

static void
pango_renderer_class_init (PangoRendererClass *klass)
{
  GObjectClass *gobject_class = G_OBJECT_CLASS (klass);

  klass->draw_glyphs = pango_renderer_default_draw_glyphs;
  klass->draw_run = pango_renderer_default_draw_run;
  klass->draw_rectangle = pango_renderer_default_draw_rectangle;
  klass->draw_error_underline = pango_renderer_default_draw_error_underline;
  klass->prepare_run = pango_renderer_default_prepare_run;

  gobject_class->finalize = pango_renderer_finalize;
}

static void
pango_renderer_init (PangoRenderer *renderer)
{
  PangoRendererPrivate *priv = pango_renderer_get_instance_private (renderer);
  priv->matrix = NULL;
}

static void
pango_renderer_finalize (GObject *gobject)
{
  PangoRenderer *renderer = PANGO_RENDERER (gobject);
  PangoRendererPrivate *priv = pango_renderer_get_instance_private (renderer);

  if (priv->matrix)
    pango_matrix_free (priv->matrix);

  G_OBJECT_CLASS (pango_renderer_parent_class)->finalize (gobject);
}

static void
pango_renderer_activate_with_context (PangoRenderer *renderer,
                                      PangoContext  *context)
{
  PangoRendererPrivate *priv = pango_renderer_get_instance_private (renderer);

  /* We only change the matrix if the renderer isn't already active. */
  if (!priv->active_count)
    {
      pango_renderer_set_matrix (renderer, context ? pango_context_get_matrix (context) : NULL);
      priv->context = context;
    }

  pango_renderer_activate (renderer);
}

static void
draw_underline (PangoRenderer *renderer,
                LineState     *state)
{
  PangoRectangle *rect = &state->underline_rect;
  PangoLineStyle underline = state->underline;

  state->underline = PANGO_LINE_STYLE_NONE;
  state->underline_position = PANGO_UNDERLINE_POSITION_NORMAL;

  switch (underline)
    {
    case PANGO_LINE_STYLE_NONE:
      break;
    case PANGO_LINE_STYLE_DOUBLE:
      pango_renderer_draw_rectangle (renderer,
                                     PANGO_RENDER_PART_UNDERLINE,
                                     rect->x,
                                     rect->y + 2 * rect->height,
                                     rect->width,
                                     rect->height);
      G_GNUC_FALLTHROUGH;
    case PANGO_LINE_STYLE_SOLID:
    case PANGO_LINE_STYLE_DOTTED:
    case PANGO_LINE_STYLE_DASHED:
      pango_renderer_draw_rectangle (renderer,
                                     PANGO_RENDER_PART_UNDERLINE,
                                     rect->x,
                                     rect->y,
                                     rect->width,
                                     rect->height);
      break;
    case PANGO_LINE_STYLE_WAVY:
      pango_renderer_draw_error_underline (renderer,
                                           rect->x,
                                           rect->y,
                                           rect->width,
                                           3 * rect->height);
      break;
    default:
      break;
    }
}

static void
draw_overline (PangoRenderer *renderer,
               LineState     *state)
{
  PangoRectangle *rect = &state->overline_rect;
  PangoLineStyle overline = state->overline;

  state->overline = PANGO_LINE_STYLE_NONE;

  switch (overline)
    {
    case PANGO_LINE_STYLE_NONE:
      break;
    case PANGO_LINE_STYLE_DOUBLE:
      pango_renderer_draw_rectangle (renderer,
                                     PANGO_RENDER_PART_OVERLINE,
                                     rect->x,
                                     rect->y - 2 * rect->height,
                                     rect->width,
                                     rect->height);
      G_GNUC_FALLTHROUGH;
    case PANGO_LINE_STYLE_SOLID:
    case PANGO_LINE_STYLE_DOTTED:
    case PANGO_LINE_STYLE_DASHED:
    case PANGO_LINE_STYLE_WAVY:
      pango_renderer_draw_rectangle (renderer,
                                     PANGO_RENDER_PART_OVERLINE,
                                     rect->x,
                                     rect->y,
                                     rect->width,
                                     rect->height);
      break;
    default:
      break;
    }
}

static void
draw_strikethrough (PangoRenderer *renderer,
                    LineState     *state)
{
  PangoRectangle *rect = &state->strikethrough_rect;

  if (state->strikethrough_glyphs > 0)
    {
      rect->y /= state->strikethrough_glyphs;
      rect->height /= state->strikethrough_glyphs;

      switch (state->strikethrough)
        {
        case PANGO_LINE_STYLE_NONE:
          break;
        case PANGO_LINE_STYLE_DOUBLE:
          pango_renderer_draw_rectangle (renderer,
                                         PANGO_RENDER_PART_STRIKETHROUGH,
                                         rect->x,
                                         rect->y - rect->height,
                                         rect->width,
                                         rect->height);
          rect->y += rect->height;
          G_GNUC_FALLTHROUGH;
        case PANGO_LINE_STYLE_SOLID:
        case PANGO_LINE_STYLE_DOTTED:
        case PANGO_LINE_STYLE_DASHED:
        case PANGO_LINE_STYLE_WAVY:
          pango_renderer_draw_rectangle (renderer,
                                         PANGO_RENDER_PART_STRIKETHROUGH,
                                         rect->x,
                                         rect->y,
                                         rect->width,
                                         rect->height);
          break;
        default:
          break;
        }
    }

  state->strikethrough = PANGO_LINE_STYLE_NONE;
  state->strikethrough_glyphs = 0;
  rect->x += rect->width;
  rect->width = 0;
  rect->y = 0;
  rect->height = 0;
}

static void
handle_line_state_change (PangoRenderer  *renderer,
                          PangoRenderPart part)
{
  PangoRendererPrivate *priv = pango_renderer_get_instance_private (renderer);
  LineState *state = priv->line_state;
  if (!state)
    return;

  if (part == PANGO_RENDER_PART_UNDERLINE &&
      state->underline != PANGO_LINE_STYLE_NONE)
    {
      PangoRectangle *rect = &state->underline_rect;

      rect->width = state->logical_rect_end - rect->x;
      draw_underline (renderer, state);
      state->underline = priv->underline;
      state->underline_position = priv->underline_position;
      rect->x = state->logical_rect_end;
      rect->width = 0;
    }

  if (part == PANGO_RENDER_PART_OVERLINE &&
      state->overline != PANGO_LINE_STYLE_NONE)
    {
      PangoRectangle *rect = &state->overline_rect;

      rect->width = state->logical_rect_end - rect->x;
      draw_overline (renderer, state);
      state->overline = priv->overline;
      rect->x = state->logical_rect_end;
      rect->width = 0;
    }

  if (part == PANGO_RENDER_PART_STRIKETHROUGH &&
      state->strikethrough != PANGO_LINE_STYLE_NONE)
    {
      PangoRectangle *rect = &state->strikethrough_rect;

      rect->width = state->logical_rect_end - rect->x;
      draw_strikethrough (renderer, state);
      state->strikethrough = priv->strikethrough;
    }
}

static void
add_underline (PangoRenderer    *renderer,
               LineState        *state,
               PangoFontMetrics *metrics,
               int               base_x,
               int               base_y,
               PangoRectangle   *ink_rect,
               PangoRectangle   *logical_rect)
{
  PangoRendererPrivate *priv = pango_renderer_get_instance_private (renderer);
  PangoRectangle *current_rect = &state->underline_rect;
  PangoRectangle new_rect;

  int underline_thickness = pango_font_metrics_get_underline_thickness (metrics);
  int underline_position = pango_font_metrics_get_underline_position (metrics);

  new_rect.x = base_x + MIN (ink_rect->x, logical_rect->x);
  new_rect.width = MAX (ink_rect->width, logical_rect->width);
  new_rect.height = underline_thickness;
  new_rect.y = base_y;

  switch (priv->underline)
    {
    case PANGO_LINE_STYLE_NONE:
      g_assert_not_reached ();
      break;
    case PANGO_LINE_STYLE_SOLID:
    case PANGO_LINE_STYLE_DASHED:
    case PANGO_LINE_STYLE_DOTTED:
      if (priv->underline_position == PANGO_UNDERLINE_POSITION_UNDER)
        {
          new_rect.y += ink_rect->y + ink_rect->height + underline_thickness;
          break;
        }
      G_GNUC_FALLTHROUGH;
    case PANGO_LINE_STYLE_DOUBLE:
    case PANGO_LINE_STYLE_WAVY:
      new_rect.y -= underline_position;
      if (state->underline == priv->underline)
        {
          new_rect.y = MAX (current_rect->y, new_rect.y);
          new_rect.height = MAX (current_rect->height, new_rect.height);
          current_rect->y = new_rect.y;
          current_rect->height = new_rect.height;
        }
      break;
    default:
      break;
    }

  if (priv->underline == state->underline &&
      priv->underline_position == state->underline_position &&
      new_rect.y == current_rect->y &&
      new_rect.height == current_rect->height)
    {
      current_rect->width = new_rect.x + new_rect.width - current_rect->x;
    }
  else
    {
      draw_underline (renderer, state);

      *current_rect = new_rect;
      state->underline = priv->underline;
      state->underline_position = priv->underline_position;
    }
}

static void
add_overline (PangoRenderer    *renderer,
              LineState        *state,
              PangoFontMetrics *metrics,
              int               base_x,
              int               base_y,
              PangoRectangle   *ink_rect,
              PangoRectangle   *logical_rect)
{
  PangoRendererPrivate *priv = pango_renderer_get_instance_private (renderer);
  PangoRectangle *current_rect = &state->overline_rect;
  PangoRectangle new_rect;
  int underline_thickness = pango_font_metrics_get_underline_thickness (metrics);
  int ascent = pango_font_metrics_get_ascent (metrics);

  new_rect.x = base_x + ink_rect->x;
  new_rect.width = ink_rect->width;
  new_rect.height = underline_thickness;
  new_rect.y = base_y;

  switch (priv->overline)
    {
    case PANGO_LINE_STYLE_NONE:
      g_assert_not_reached ();
      break;
    case PANGO_LINE_STYLE_SOLID:
    case PANGO_LINE_STYLE_DOUBLE:
    case PANGO_LINE_STYLE_DASHED:
    case PANGO_LINE_STYLE_DOTTED:
    case PANGO_LINE_STYLE_WAVY:
      new_rect.y -= ascent;
      if (state->overline == priv->overline)
        {
          new_rect.y = MIN (current_rect->y, new_rect.y);
          new_rect.height = MAX (current_rect->height, new_rect.height);
          current_rect->y = new_rect.y;
          current_rect->height = new_rect.height;
        }
      break;
    default:
      break;
    }

  if (priv->overline == state->overline &&
      new_rect.y == current_rect->y &&
      new_rect.height == current_rect->height)
    {
      current_rect->width = new_rect.x + new_rect.width - current_rect->x;
    }
  else
    {
      draw_overline (renderer, state);

      *current_rect = new_rect;
      state->overline = priv->overline;
    }
}

static void
add_strikethrough (PangoRenderer    *renderer,
                   LineState        *state,
                   PangoFontMetrics *metrics,
                   int               base_x,
                   int               base_y,
                   PangoRectangle   *ink_rect G_GNUC_UNUSED,
                   PangoRectangle   *logical_rect,
                   int               num_glyphs)
{
  PangoRendererPrivate *priv = pango_renderer_get_instance_private (renderer);
  PangoRectangle *current_rect = &state->strikethrough_rect;
  PangoRectangle new_rect;

  int strikethrough_thickness = pango_font_metrics_get_strikethrough_thickness (metrics);
  int strikethrough_position = pango_font_metrics_get_strikethrough_position (metrics);

  new_rect.x = base_x + ink_rect->x;
  new_rect.width = ink_rect->width;
  new_rect.y = (base_y - strikethrough_position) * num_glyphs;
  new_rect.height = strikethrough_thickness * num_glyphs;

  if (state->strikethrough == priv->strikethrough)
    {
      current_rect->width = new_rect.x + new_rect.width - current_rect->x;
      current_rect->y += new_rect.y;
      current_rect->height += new_rect.height;
      state->strikethrough_glyphs += num_glyphs;
    }
  else
    {
      draw_strikethrough (renderer, state);

      *current_rect = new_rect;
      state->strikethrough = priv->strikethrough;
      state->strikethrough_glyphs = num_glyphs;
    }
}

static void pango_renderer_draw_runs (PangoRenderer *renderer,
                                      PangoLine     *line,
                                      GSList        *runs,
                                      const char    *text,
                                      int            x,
                                      int            y);

static void
draw_shaped_glyphs (PangoRenderer    *renderer,
                    PangoGlyphString *glyphs,
                    PangoAttribute   *attr,
                    int               x,
                    int               y)
{
  PangoRendererClass *class = PANGO_RENDERER_GET_CLASS (renderer);
  int i;

  if (!class->draw_shape)
    return;

  for (i = 0; i < glyphs->num_glyphs; i++)
    {
      PangoGlyphInfo *gi = &glyphs->glyphs[i];
      ShapeData *data = (ShapeData *)attr->pointer_value;

      class->draw_shape (renderer,
                         &data->ink_rect,
                         &data->logical_rect,
                         data->data,
                         x, y);

      x += gi->geometry.width;
    }
}

/**
 * pango_renderer_draw_line:
 * @renderer: a `PangoRenderer`
 * @line: a `PangoLine`
 * @x: X position of left edge of baseline, in user space coordinates
 *   in Pango units.
 * @y: Y position of left edge of baseline, in user space coordinates
 *   in Pango units.
 *
 * Draws @line with the specified `PangoRenderer`.
 *
 * This draws the glyph items that make up the line, as well as
 * shapes, backgrounds and lines that are specified by the attributes
 * of those items.
 */
void
pango_renderer_draw_line (PangoRenderer *renderer,
                          PangoLine     *line,
                          int            x,
                          int            y)
{
  PangoRendererPrivate *priv = pango_renderer_get_instance_private (renderer);
  LineState state = { 0, };

  g_return_if_fail (PANGO_IS_RENDERER (renderer));

  pango_renderer_activate_with_context (renderer, line->context);

  priv->line = line;
  priv->line_state = &state;

  state.underline = PANGO_LINE_STYLE_NONE;
  state.underline_position = PANGO_UNDERLINE_POSITION_NORMAL;
  state.overline = PANGO_LINE_STYLE_NONE;
  state.strikethrough = PANGO_LINE_STYLE_NONE;

  pango_renderer_draw_runs (renderer, line, line->runs, line->data->text, x, y);

  /* Finish off any remaining underlines */
  draw_underline (renderer, &state);
  draw_overline (renderer, &state);
  draw_strikethrough (renderer, &state);

  priv->line = NULL;
  priv->line_state = NULL;
  priv->line = NULL;

  pango_renderer_deactivate (renderer);
}

/**
 * pango_renderer_draw_lines:
 * @renderer: a `PangoRenderer`
 * @lines: a `PangoLines` object
 * @x: X position of left edge of baseline, in user space coordinates
 *   in Pango units.
 * @y: Y position of left edge of baseline, in user space coordinates
 *   in Pango units.
 *
 * Draws @lines with the specified `PangoRenderer`.
 */
void
pango_renderer_draw_lines (PangoRenderer *renderer,
                           PangoLines    *lines,
                           int            x,
                           int            y)
{
  PangoRendererPrivate *priv = pango_renderer_get_instance_private (renderer);
  int n;
  PangoLine *line;
  int line_x, line_y;
  PangoLine **l;

  g_return_if_fail (PANGO_IS_RENDERER (renderer));

  priv->lines = lines;

  l = pango_lines_get_lines (lines);
  for (n = 0; n < pango_lines_get_line_count (lines); n++)
    {
      line = l[n];
      pango_lines_get_line_position (lines, n, &line_x, &line_y);

      if (n == 0)
        pango_renderer_activate_with_context (renderer, line->context);

      pango_renderer_draw_line (renderer, line, x + line_x, y + line_y);
    }

  if (n > 0)
    pango_renderer_deactivate (renderer);

  priv->lines = NULL;
}

static void
pango_shape_get_extents (int             n_chars,
                         PangoRectangle *shape_ink,
                         PangoRectangle *shape_logical,
                         PangoRectangle *ink_rect,
                         PangoRectangle *logical_rect)
{
  if (n_chars > 0)
    {
      ink_rect->x = MIN (shape_ink->x, shape_ink->x + shape_logical->width * (n_chars - 1));
      ink_rect->width = MAX (shape_ink->width, shape_ink->width + shape_logical->width * (n_chars - 1));
      ink_rect->y = shape_ink->y;
      ink_rect->height = shape_ink->height;

      logical_rect->x = MIN (shape_logical->x, shape_logical->x + shape_logical->width * (n_chars - 1));
      logical_rect->width = MAX (shape_logical->width, shape_logical->width + shape_logical->width * (n_chars - 1));
      logical_rect->y = shape_logical->y;
      logical_rect->height = shape_logical->height;
    }
  else
    {
      ink_rect->x = 0;
      ink_rect->y = 0;
      ink_rect->width = 0;
      ink_rect->height = 0;

      logical_rect->x = 0;
      logical_rect->y = 0;
      logical_rect->width = 0;
      logical_rect->height = 0;
    }
}



static void
pango_renderer_draw_runs (PangoRenderer *renderer,
                          PangoLine     *line,
                          GSList        *runs,
                          const char    *text,
                          int            x,
                          int            y)
{
  PangoRendererPrivate *priv = pango_renderer_get_instance_private (renderer);
  GSList *l;
  int x_off = 0;
  int glyph_string_width;
  gboolean got_overall = FALSE;
  PangoRectangle overall_rect;

  for (l = runs; l; l = l->next)
    {
      PangoFontMetrics *metrics;
      PangoRun *run = l->data;
      PangoGlyphItem *glyph_item = pango_run_get_glyph_item (run);
      PangoItem *item = pango_run_get_item (run);
      PangoGlyphString *glyphs = pango_run_get_glyphs (run);
      PangoRectangle ink_rect, *ink = NULL;
      PangoRectangle logical_rect, *logical = NULL;
      ItemProperties properties;
      int y_off;

      if (item->analysis.flags & PANGO_ANALYSIS_FLAG_CENTERED_BASELINE)
        logical = &logical_rect;

      pango_renderer_prepare_run (renderer, run);

      pango_item_get_properties (item, &properties);

      if (properties.shape)
        {
          ShapeData *data = (ShapeData *)properties.shape->pointer_value;

          ink = &ink_rect;
          logical = &logical_rect;
          pango_shape_get_extents (glyphs->num_glyphs,
                                   &data->ink_rect,
                                   &data->logical_rect,
                                   ink,
                                   logical);

          glyph_string_width = logical->width;
        }
      else
        {
          if (priv->underline != PANGO_LINE_STYLE_NONE ||
              priv->overline != PANGO_LINE_STYLE_NONE ||
              priv->strikethrough != PANGO_LINE_STYLE_NONE)
            {
              ink = &ink_rect;
              logical = &logical_rect;
            }
          if (G_UNLIKELY (ink || logical))
            pango_glyph_string_extents (glyphs, item->analysis.font, ink, logical);
          if (logical)
            glyph_string_width = logical_rect.width;
          else
            glyph_string_width = pango_glyph_string_get_width (glyphs);
        }

      priv->line_state->logical_rect_end = x + x_off + glyph_string_width;

      x_off += glyph_item->start_x_offset;
      y_off = glyph_item->y_offset;

      if (item->analysis.flags & PANGO_ANALYSIS_FLAG_CENTERED_BASELINE)
        {
          gboolean is_hinted = ((logical_rect.y | logical_rect.height) & (PANGO_SCALE - 1)) == 0;
          int adjustment = logical_rect.y + logical_rect.height / 2;

          if (is_hinted)
            adjustment = PANGO_UNITS_ROUND (adjustment);

          y_off += adjustment;
        }

      if (priv->color_set[PANGO_RENDER_PART_BACKGROUND])
        {
          if (!got_overall)
            {
              pango_line_get_extents (line, NULL, &overall_rect);
              got_overall = TRUE;
            }

          pango_renderer_draw_rectangle (renderer,
                                         PANGO_RENDER_PART_BACKGROUND,
                                         x + x_off,
                                         y + overall_rect.y,
                                         glyph_string_width,
                                         overall_rect.height);
        }

      if (properties.shape)
        draw_shaped_glyphs (renderer, glyphs, properties.shape, x + x_off, y - y_off);
      else
        pango_renderer_draw_run (renderer, text, run, x + x_off, y - y_off);

      if (priv->underline != PANGO_LINE_STYLE_NONE ||
          priv->overline != PANGO_LINE_STYLE_NONE ||
          priv->strikethrough != PANGO_LINE_STYLE_NONE)
        {
          metrics = pango_font_get_metrics (item->analysis.font,
                                            item->analysis.language);

          if (priv->underline != PANGO_LINE_STYLE_NONE)
            add_underline (renderer, priv->line_state, metrics,
                           x + x_off, y - y_off,
                           ink, logical);

          if (priv->overline != PANGO_LINE_STYLE_NONE)
            add_overline (renderer, priv->line_state, metrics,
                           x + x_off, y - y_off,
                           ink, logical);

          if (priv->strikethrough != PANGO_LINE_STYLE_NONE)
            add_strikethrough (renderer, priv->line_state, metrics,
                               x + x_off, y - y_off,
                               ink, logical, glyphs->num_glyphs);

          pango_font_metrics_free (metrics);
        }

      if (priv->underline == PANGO_LINE_STYLE_NONE &&
          priv->line_state->underline != PANGO_LINE_STYLE_NONE)
        draw_underline (renderer, priv->line_state);

      if (priv->overline == PANGO_LINE_STYLE_NONE &&
          priv->line_state->overline != PANGO_LINE_STYLE_NONE)
        draw_overline (renderer, priv->line_state);

      if (priv->strikethrough == PANGO_LINE_STYLE_NONE &&
          priv->line_state->strikethrough != PANGO_LINE_STYLE_NONE)
        draw_strikethrough (renderer, priv->line_state);

      x_off += glyph_string_width;
      x_off += glyph_item->end_x_offset;
    }
}

/**
 * pango_renderer_draw_glyphs:
 * @renderer: a `PangoRenderer`
 * @font: a `PangoFont`
 * @glyphs: a `PangoGlyphString`
 * @x: X position of left edge of baseline, in user space coordinates
 *   in Pango units.
 * @y: Y position of left edge of baseline, in user space coordinates
 *   in Pango units.
 *
 * Draws the glyphs in @glyphs with the specified `PangoRenderer`.
 */
void
pango_renderer_draw_glyphs (PangoRenderer    *renderer,
                            PangoFont        *font,
                            PangoGlyphString *glyphs,
                            int               x,
                            int               y)
{
  g_return_if_fail (PANGO_IS_RENDERER (renderer));

  pango_renderer_activate (renderer);

  PANGO_RENDERER_GET_CLASS (renderer)->draw_glyphs (renderer, font, glyphs, x, y);

  pango_renderer_deactivate (renderer);
}

static void
pango_renderer_default_draw_glyphs (PangoRenderer    *renderer,
                                    PangoFont        *font,
                                    PangoGlyphString *glyphs,
                                    int               x,
                                    int               y)
{
  PangoRendererPrivate *priv = pango_renderer_get_instance_private (renderer);
  int i;
  int x_position = 0;

  for (i = 0; i < glyphs->num_glyphs; i++)
    {
      PangoGlyphInfo *gi = &glyphs->glyphs[i];
      Point p;

      to_device (priv->matrix,
                 x + x_position + gi->geometry.x_offset,
                 y +              gi->geometry.y_offset,
                 &p);

      pango_renderer_draw_glyph (renderer, font, gi->glyph, p.x, p.y);

      x_position += gi->geometry.width;
    }
}

/**
 * pango_renderer_draw_run:
 * @renderer: a `PangoRenderer`
 * @text: (nullable): the UTF-8 text that @glyph_item refers to
 * @run: a `PangoRun`
 * @x: X position of left edge of baseline, in user space coordinates
 *   in Pango units
 * @y: Y position of left edge of baseline, in user space coordinates
 *   in Pango units
 *
 * Draws the glyphs in @run with the specified `PangoRenderer`,
 * embedding the text associated with the glyphs in the output if the
 * output format supports it.
 *
 * This is useful for rendering text in PDF.
 *
 * Note that this method does not handle attributes in @run.
 * If you want colors, shapes and lines handled automatically according
 * to those attributes, you need to use [method@Pango.Renderer.draw_line]
 * or [method@Pango.Renderer.draw_lines].
 *
 * Note that @text is the start of the text for layout, which is then
 * indexed by `run->item->offset`.
 *
 * If @text is %NULL, this simply calls [method@Pango.Renderer.draw_glyphs].
 *
 * The default implementation of this method simply falls back to
 * [method@Pango.Renderer.draw_glyphs].
 */
void
pango_renderer_draw_run (PangoRenderer  *renderer,
                         const char     *text,
                         PangoRun       *run,
                         int             x,
                         int             y)
{
  if (!text)
    {
      PangoItem *item = pango_run_get_item (run);
      PangoGlyphString *glyphs = pango_run_get_glyphs (run);

      pango_renderer_draw_glyphs (renderer, item->analysis.font, glyphs, x, y);
      return;
    }

  g_return_if_fail (PANGO_IS_RENDERER (renderer));

  pango_renderer_activate (renderer);

  PANGO_RENDERER_GET_CLASS (renderer)->draw_run (renderer, text, run, x, y);

  pango_renderer_deactivate (renderer);
}

static void
pango_renderer_default_draw_run (PangoRenderer  *renderer,
                                 const char     *text G_GNUC_UNUSED,
                                 PangoRun       *run,
                                 int             x,
                                 int             y)
{
  PangoItem *item = pango_run_get_item (run);
  PangoGlyphString *glyphs = pango_run_get_glyphs (run);

  pango_renderer_draw_glyphs (renderer, item->analysis.font, glyphs, x, y);
}

/**
 * pango_renderer_draw_rectangle:
 * @renderer: a `PangoRenderer`
 * @part: type of object this rectangle is part of
 * @x: X position at which to draw rectangle, in user space coordinates
 *   in Pango units
 * @y: Y position at which to draw rectangle, in user space coordinates
 *   in Pango units
 * @width: width of rectangle in Pango units
 * @height: height of rectangle in Pango units
 *
 * Draws an axis-aligned rectangle in user space coordinates with the
 * specified `PangoRenderer`.
 *
 * This should be called while @renderer is already active.
 * Use [method@Pango.Renderer.activate] to activate a renderer.
 */
void
pango_renderer_draw_rectangle (PangoRenderer   *renderer,
                               PangoRenderPart  part,
                               int              x,
                               int              y,
                               int              width,
                               int              height)
{
  PangoRendererPrivate *priv = pango_renderer_get_instance_private (renderer);

  g_return_if_fail (PANGO_IS_RENDERER (renderer));
  g_return_if_fail (IS_VALID_PART (part));
  g_return_if_fail (priv->active_count > 0);

  PANGO_RENDERER_GET_CLASS (renderer)->draw_rectangle (renderer, part, x, y, width, height);
}

static int
compare_points (const void *a,
                const void *b)
{
  const Point *pa = a;
  const Point *pb = b;

  if (pa->y < pb->y)
    return -1;
  else if (pa->y > pb->y)
    return 1;
  else if (pa->x < pb->x)
    return -1;
  else if (pa->x > pb->x)
    return 1;
  else
    return 0;
}

static void
draw_rectangle (PangoRenderer   *renderer,
                PangoMatrix     *matrix,
                PangoRenderPart  part,
                int              x,
                int              y,
                int              width,
                int              height)
{
  Point points[4];

  /* Convert the points to device coordinates, and sort
   * in ascending Y order. (Ordering by X for ties)
   */
  to_device (matrix, x, y, &points[0]);
  to_device (matrix, x + width, y, &points[1]);
  to_device (matrix, x, y + height, &points[2]);
  to_device (matrix, x + width, y + height, &points[3]);

  qsort (points, 4, sizeof (Point), compare_points);

  /* There are essentially three cases. (There is a fourth
   * case where trapezoid B is degenerate and we just have
   * two triangles, but we don't need to handle it separately.)
   *
   *     1            2             3
   *
   *     ______       /\           /\
   *    /     /      /A \         /A \
   *   /  B  /      /____\       /____\
   *  /_____/      /  B  /       \  B  \
   *              /_____/         \_____\
   *              \ C  /           \ C  /
   *               \  /             \  /
   *                \/               \/
   */
  if (points[0].y == points[1].y)
    {
     /* Case 1 (pure shear) */
      pango_renderer_draw_trapezoid (renderer, part,                                      /* B */
                                     points[0].y, points[0].x, points[1].x,
                                     points[2].y, points[2].x, points[3].x);
    }
  else if (points[1].x < points[2].x)
    {
      /* Case 2 */
      double tmp_width = ((points[2].x - points[0].x) * (points[1].y - points[0].y)) / (points[2].y - points[0].y);
      double base_width = tmp_width + points[0].x - points[1].x;

      pango_renderer_draw_trapezoid (renderer, part,                                      /* A */
                                     points[0].y, points[0].x, points[0].x,
                                     points[1].y, points[1].x, points[1].x + base_width);
      pango_renderer_draw_trapezoid (renderer, part,                                      /* B */
                                     points[1].y, points[1].x, points[1].x + base_width,
                                     points[2].y, points[2].x - base_width, points[2].x);
      pango_renderer_draw_trapezoid (renderer, part,                                      /* C */
                                     points[2].y, points[2].x - base_width, points[2].x,
                                     points[3].y, points[3].x, points[3].x);
    }
  else
    {
      /* case 3 */
      double tmp_width = ((points[0].x - points[2].x) * (points[1].y - points[0].y)) / (points[2].y - points[0].y);
      double base_width = tmp_width + points[1].x - points[0].x;

      pango_renderer_draw_trapezoid (renderer, part,                                     /* A */
                                     points[0].y, points[0].x, points[0].x,
                                     points[1].y,  points[1].x - base_width, points[1].x);
      pango_renderer_draw_trapezoid (renderer, part,                                     /* B */
                                     points[1].y, points[1].x - base_width, points[1].x,
                                     points[2].y, points[2].x, points[2].x + base_width);
      pango_renderer_draw_trapezoid (renderer, part,                                     /* C */
                                     points[2].y, points[2].x, points[2].x + base_width,
                                     points[3].y, points[3].x, points[3].x);
    }
}

static void
pango_renderer_default_draw_rectangle (PangoRenderer  *renderer,
                                       PangoRenderPart part,
                                       int             x,
                                       int             y,
                                       int             width,
                                       int             height)
{
  PangoRendererPrivate *priv = pango_renderer_get_instance_private (renderer);

  draw_rectangle (renderer, priv->matrix, part, x, y, width, height);
}

/**
 * pango_renderer_draw_error_underline:
 * @renderer: a `PangoRenderer`
 * @x: X coordinate of underline, in Pango units in user coordinate system
 * @y: Y coordinate of underline, in Pango units in user coordinate system
 * @width: width of underline, in Pango units in user coordinate system
 * @height: height of underline, in Pango units in user coordinate system
 *
 * Draw a squiggly line that approximately covers the given rectangle
 * in the style of an underline used to indicate a spelling error.
 *
 * The width of the underline is rounded to an integer number
 * of up/down segments and the resulting rectangle is centered
 * in the original rectangle.
 *
 * This should be called while @renderer is already active.
 * Use [method@Pango.Renderer.activate] to activate a renderer.
 */
void
pango_renderer_draw_error_underline (PangoRenderer *renderer,
                                     int            x,
                                     int            y,
                                     int            width,
                                     int            height)
{
  PangoRendererPrivate *priv = pango_renderer_get_instance_private (renderer);

  g_return_if_fail (PANGO_IS_RENDERER (renderer));
  g_return_if_fail (priv->active_count > 0);

  PANGO_RENDERER_GET_CLASS (renderer)->draw_error_underline (renderer, x, y, width, height);
}

/* We are drawing an error underline that looks like one of:
 *
 *  /\      /\      /\        /\      /\               -
 * /  \    /  \    /  \      /  \    /  \              |
 * \   \  /\   \  /   /      \   \  /\   \             |
 *  \   \/B \   \/ C /        \   \/B \   \            | height = HEIGHT_SQUARES * square
 *   \ A \  /\ A \  /          \ A \  /\ A \           |
 *    \   \/  \   \/            \   \/  \   \          |
 *     \  /    \  /              \  /    \  /          |
 *      \/      \/                \/      \/           -
 *      |---|
 *    unit_width = (HEIGHT_SQUARES - 1) * square
 *
 * To do this conveniently, we work in a coordinate system where A,B,C
 * are axis aligned rectangles. (If fonts were square, the diagrams
 * would be clearer)
 *
 *             (0,0)
 *              /\      /\
 *             /  \    /  \
 *            /\  /\  /\  /
 *           /  \/  \/  \/
 *          /    \  /\  /
 *      Y axis    \/  \/
 *                 \  /\
 *                  \/  \
 *                       \ X axis
 *
 * Note that the long side in this coordinate system is HEIGHT_SQUARES + 1
 * units long
 *
 * The diagrams above are shown with HEIGHT_SQUARES an integer, but
 * that is actually incidental; the value 2.5 below seems better than
 * either HEIGHT_SQUARES=3 (a little long and skinny) or
 * HEIGHT_SQUARES=2 (a bit short and stubby)
 */

#define HEIGHT_SQUARES 2.5

static void
get_total_matrix (PangoMatrix       *total,
                  const PangoMatrix *global,
                  int                x,
                  int                y,
                  int                square)
{
  PangoMatrix local;
  double scale = 0.5 * square;

  /* The local matrix translates from the axis aligned coordinate system
   * to the original user space coordinate system.
   */
  local.xx = scale;
  local.xy = - scale;
  local.yx = scale;
  local.yy = scale;
  local.x0 = 0;
  local.y0 = 0;

  *total = *global;
  pango_matrix_concat (total, &local);

  total->x0 = (global->xx * x + global->xy * y) / PANGO_SCALE + global->x0;
  total->y0 = (global->yx * x + global->yy * y) / PANGO_SCALE + global->y0;
}

static void
pango_renderer_default_draw_error_underline (PangoRenderer *renderer,
                                             int            x,
                                             int            y,
                                             int            width,
                                             int            height)
{
  PangoRendererPrivate *priv = pango_renderer_get_instance_private (renderer);
  int square;
  int unit_width;
  int width_units;
  const PangoMatrix identity = PANGO_MATRIX_INIT;
  const PangoMatrix *matrix;
  double dx, dx0, dy0;
  PangoMatrix total;
  int i;

  if (width <= 0 || height <= 0)
    return;

  square = height / HEIGHT_SQUARES;
  unit_width = (HEIGHT_SQUARES - 1) * square;
  width_units = (width + unit_width / 2) / unit_width;

  x += (width - width_units * unit_width) / 2;

  if (priv->matrix)
    matrix = priv->matrix;
  else
    matrix = &identity;

  get_total_matrix (&total, matrix, x, y, square);
  dx = unit_width * 2;
  dx0 = (matrix->xx * dx) / PANGO_SCALE;
  dy0 = (matrix->yx * dx) / PANGO_SCALE;

  i = (width_units - 1) / 2;
  while (TRUE)
    {
      draw_rectangle (renderer, &total, PANGO_RENDER_PART_UNDERLINE, /* A */
                      0,                      0,
                      HEIGHT_SQUARES * 2 - 1, 1);

      if (i <= 0)
        break;
      i--;

      draw_rectangle (renderer, &total, PANGO_RENDER_PART_UNDERLINE, /* B */
                      HEIGHT_SQUARES * 2 - 2, - (HEIGHT_SQUARES * 2 - 3),
                      1,                      HEIGHT_SQUARES * 2 - 3);

      total.x0 += dx0;
      total.y0 += dy0;
    }
  if (width_units % 2 == 0)
    {
      draw_rectangle (renderer, &total, PANGO_RENDER_PART_UNDERLINE, /* C */
                      HEIGHT_SQUARES * 2 - 2, - (HEIGHT_SQUARES * 2 - 2),
                      1,                      HEIGHT_SQUARES * 2 - 2);
    }
}

/**
 * pango_renderer_draw_trapezoid:
 * @renderer: a `PangoRenderer`
 * @part: type of object this trapezoid is part of
 * @y1_: Y coordinate of top of trapezoid
 * @x11: X coordinate of left end of top of trapezoid
 * @x21: X coordinate of right end of top of trapezoid
 * @y2: Y coordinate of bottom of trapezoid
 * @x12: X coordinate of left end of bottom of trapezoid
 * @x22: X coordinate of right end of bottom of trapezoid
 *
 * Draws a trapezoid with the parallel sides aligned with the X axis
 * using the given `PangoRenderer`; coordinates are in device space.
 */
void
pango_renderer_draw_trapezoid (PangoRenderer   *renderer,
                               PangoRenderPart  part,
                               double           y1_,
                               double           x11,
                               double           x21,
                               double           y2,
                               double           x12,
                               double           x22)
{
  PangoRendererPrivate *priv = pango_renderer_get_instance_private (renderer);

  g_return_if_fail (PANGO_IS_RENDERER (renderer));
  g_return_if_fail (priv->active_count > 0);

  if (PANGO_RENDERER_GET_CLASS (renderer)->draw_trapezoid)
    PANGO_RENDERER_GET_CLASS (renderer)->draw_trapezoid (renderer, part,
                                                         y1_, x11, x21,
                                                         y2, x12, x22);
}

/**
 * pango_renderer_draw_glyph:
 * @renderer: a `PangoRenderer`
 * @font: a `PangoFont`
 * @glyph: the glyph index of a single glyph
 * @x: X coordinate of left edge of baseline of glyph
 * @y: Y coordinate of left edge of baseline of glyph
 *
 * Draws a single glyph with coordinates in device space.
 */
void
pango_renderer_draw_glyph (PangoRenderer *renderer,
                           PangoFont     *font,
                           PangoGlyph     glyph,
                           double         x,
                           double         y)
{
  PangoRendererPrivate *priv = pango_renderer_get_instance_private (renderer);

  g_return_if_fail (PANGO_IS_RENDERER (renderer));
  g_return_if_fail (priv->active_count > 0);

  if (glyph == PANGO_GLYPH_EMPTY) /* glyph PANGO_GLYPH_EMPTY never renders */
    return;

  if (PANGO_RENDERER_GET_CLASS (renderer)->draw_glyph)
    PANGO_RENDERER_GET_CLASS (renderer)->draw_glyph (renderer, font, glyph, x, y);
}

/**
 * pango_renderer_activate:
 * @renderer: a `PangoRenderer`
 *
 * Does initial setup before rendering operations on @renderer.
 *
 * [method@Pango.Renderer.deactivate] should be called when done drawing.
 * Calls such as [method@Pango.Renderer.draw_lines] automatically
 * activate the layout before drawing on it.
 *
 * Calls to [method@Pango.Renderer.activate] and
 * [method@Pango.Renderer.deactivate] can be nested and the
 * renderer will only be initialized and deinitialized once.
 */
void
pango_renderer_activate (PangoRenderer *renderer)
{
  PangoRendererPrivate *priv = pango_renderer_get_instance_private (renderer);

  g_return_if_fail (PANGO_IS_RENDERER (renderer));

  priv->active_count++;
  if (priv->active_count == 1)
    {
      if (PANGO_RENDERER_GET_CLASS (renderer)->begin)
        PANGO_RENDERER_GET_CLASS (renderer)->begin (renderer);
    }
}

/**
 * pango_renderer_deactivate:
 * @renderer: a `PangoRenderer`
 *
 * Cleans up after rendering operations on @renderer.
 *
 * See docs for [method@Pango.Renderer.activate].
 */
void
pango_renderer_deactivate (PangoRenderer *renderer)
{
  PangoRendererPrivate *priv = pango_renderer_get_instance_private (renderer);

  g_return_if_fail (PANGO_IS_RENDERER (renderer));
  g_return_if_fail (priv->active_count > 0);

  if (priv->active_count == 1)
    {
      if (PANGO_RENDERER_GET_CLASS (renderer)->end)
        PANGO_RENDERER_GET_CLASS (renderer)->end (renderer);
    }
  priv->active_count--;
}

/**
 * pango_renderer_set_color:
 * @renderer: a `PangoRenderer`
 * @part: the part to change the color of
 * @color: (nullable): the new color or %NULL to unset the current color
 *
 * Sets the color for part of the rendering.
 */
void
pango_renderer_set_color (PangoRenderer    *renderer,
                          PangoRenderPart   part,
                          const PangoColor *color)
{
  PangoRendererPrivate *priv = pango_renderer_get_instance_private (renderer);

  g_return_if_fail (PANGO_IS_RENDERER (renderer));
  g_return_if_fail (IS_VALID_PART (part));

  if ((!color && !priv->color_set[part]) ||
      (color && priv->color_set[part] &&
       pango_color_equal (color, &priv->color[part])))
    return;

  pango_renderer_part_changed (renderer, part);

  if (color)
    {
      priv->color_set[part] = TRUE;
      priv->color[part] = *color;
    }
  else
    {
      priv->color_set[part] = FALSE;
    }
}

/**
 * pango_renderer_get_color:
 * @renderer: a `PangoRenderer`
 * @part: the part to get the color for
 *
 * Gets the current rendering color for the specified part.
 *
 * Return value: (transfer none) (nullable): the color for the
 *   specified part, or %NULL if it hasn't been set and should be
 *   inherited from the environment.
 */
PangoColor *
pango_renderer_get_color (PangoRenderer   *renderer,
                          PangoRenderPart  part)
{
  PangoRendererPrivate *priv = pango_renderer_get_instance_private (renderer);

  g_return_val_if_fail (PANGO_IS_RENDERER (renderer), NULL);
  g_return_val_if_fail (IS_VALID_PART (part), NULL);

  if (priv->color_set[part])
    return &priv->color[part];
  else
    return NULL;
}

/**
 * pango_renderer_part_changed:
 * @renderer: a `PangoRenderer`
 * @part: the part for which rendering has changed.
 *
 * Informs Pango that the way that the rendering is done
 * for @part has changed.
 *
 * This should be called if the rendering changes in a way that would
 * prevent multiple pieces being joined together into one drawing call.
 * For instance, if a subclass of `PangoRenderer` was to add a stipple
 * option for drawing underlines, it needs to call
 *
 * ```
 * pango_renderer_part_changed (render, PANGO_RENDER_PART_UNDERLINE);
 * ```
 *
 * When the stipple changes or underlines with different stipples
 * might be joined together. Pango automatically calls this for
 * changes to colors. (See [method@Pango.Renderer.set_color])
 */
void
pango_renderer_part_changed (PangoRenderer   *renderer,
                             PangoRenderPart  part)
{
  PangoRendererPrivate *priv = pango_renderer_get_instance_private (renderer);

  g_return_if_fail (PANGO_IS_RENDERER (renderer));
  g_return_if_fail (IS_VALID_PART (part));
  g_return_if_fail (priv->active_count > 0);

  handle_line_state_change (renderer, part);

  if (PANGO_RENDERER_GET_CLASS (renderer)->part_changed)
    PANGO_RENDERER_GET_CLASS (renderer)->part_changed (renderer, part);
}

/**
 * pango_renderer_prepare_run:
 * @renderer: a `PangoRenderer`
 * @run: a `PangoRun`
 *
 * Set up the state of the `PangoRenderer` for rendering @run.
 */
static void
pango_renderer_prepare_run (PangoRenderer  *renderer,
                            PangoRun       *run)
{
  g_return_if_fail (PANGO_IS_RENDERER (renderer));

  PANGO_RENDERER_GET_CLASS (renderer)->prepare_run (renderer, run);
}

static void
pango_renderer_default_prepare_run (PangoRenderer  *renderer,
                                    PangoRun       *run)
{
  PangoRendererPrivate *priv = pango_renderer_get_instance_private (renderer);
  PangoColor *fg_color = NULL;
  PangoColor *bg_color = NULL;
  PangoColor *underline_color = NULL;
  PangoColor *overline_color = NULL;
  PangoColor *strikethrough_color = NULL;
  GSList *l;
  PangoGlyphItem *glyph_item;

  glyph_item = pango_run_get_glyph_item (run);

  priv->underline = PANGO_LINE_STYLE_NONE;
  priv->underline_position = PANGO_UNDERLINE_POSITION_NORMAL;
  priv->overline = PANGO_LINE_STYLE_NONE;
  priv->strikethrough = PANGO_LINE_STYLE_NONE;

  for (l = glyph_item->item->analysis.extra_attrs; l; l = l->next)
    {
      PangoAttribute *attr = l->data;

      switch ((int) attr->type)
        {
        case PANGO_ATTR_UNDERLINE:
          priv->underline = attr->int_value;
          break;

        case PANGO_ATTR_UNDERLINE_POSITION:
          priv->underline_position = attr->int_value;
          break;

        case PANGO_ATTR_OVERLINE:
          priv->overline = attr->int_value;
          break;

        case PANGO_ATTR_STRIKETHROUGH:
          priv->strikethrough = attr->int_value;
          break;

        case PANGO_ATTR_FOREGROUND:
          fg_color = &attr->color_value;
          break;

        case PANGO_ATTR_BACKGROUND:
          bg_color = &attr->color_value;
          break;

        case PANGO_ATTR_UNDERLINE_COLOR:
          underline_color = &attr->color_value;
          break;

        case PANGO_ATTR_OVERLINE_COLOR:
          overline_color = &attr->color_value;
          break;

        case PANGO_ATTR_STRIKETHROUGH_COLOR:
          strikethrough_color = &attr->color_value;
          break;

        default:
          break;
        }
    }

  if (!underline_color)
    underline_color = fg_color;

  if (!overline_color)
    overline_color = fg_color;

  if (!strikethrough_color)
    strikethrough_color = fg_color;

  pango_renderer_set_color (renderer, PANGO_RENDER_PART_FOREGROUND, fg_color);
  pango_renderer_set_color (renderer, PANGO_RENDER_PART_BACKGROUND, bg_color);
  pango_renderer_set_color (renderer, PANGO_RENDER_PART_UNDERLINE, underline_color);
  pango_renderer_set_color (renderer, PANGO_RENDER_PART_STRIKETHROUGH, strikethrough_color);
  pango_renderer_set_color (renderer, PANGO_RENDER_PART_OVERLINE, overline_color);
}

/**
 * pango_renderer_set_matrix:
 * @renderer: a `PangoRenderer`
 * @matrix: (nullable): a `PangoMatrix`, or %NULL to unset any existing matrix
 *  (No matrix set is the same as setting the identity matrix.)
 *
 * Sets the transformation matrix that will be applied when rendering.
 */
void
pango_renderer_set_matrix (PangoRenderer     *renderer,
                           const PangoMatrix *matrix)
{
  PangoRendererPrivate *priv = pango_renderer_get_instance_private (renderer);

  g_return_if_fail (PANGO_IS_RENDERER (renderer));

  pango_matrix_free (priv->matrix);
  priv->matrix = pango_matrix_copy (matrix);
}

/**
 * pango_renderer_get_matrix:
 * @renderer: a `PangoRenderer`
 *
 * Gets the transformation matrix that will be applied when
 * rendering.
 *
 * See [method@Pango.Renderer.set_matrix].
 *
 * Return value: (nullable): the matrix, or %NULL if no matrix has
 *   been set (which is the same as the identity matrix). The returned
 *   matrix is owned by Pango and must not be modified or freed.
 */
const PangoMatrix *
pango_renderer_get_matrix (PangoRenderer *renderer)
{
  PangoRendererPrivate *priv = pango_renderer_get_instance_private (renderer);

  g_return_val_if_fail (PANGO_IS_RENDERER (renderer), NULL);

  return priv->matrix;
}

/**
 * pango_renderer_get_lines:
 * @renderer: a `PangoRenderer`
 *
 * Gets the `PangoLines` currently being rendered using @renderer.
 *
 * Calling this function only makes sense from inside a subclass's
 * methods, like in its draw_shape vfunc, for example.
 *
 * The returned layout should not be modified while still being
 * rendered.
 *
 * Return value: (transfer none) (nullable): the `PangoLines`, or
 *   %NULL if no layout is being rendered using @renderer at this time.
 */
PangoLines *
pango_renderer_get_lines (PangoRenderer *renderer)
{
  PangoRendererPrivate *priv = pango_renderer_get_instance_private (renderer);

  return priv->lines;
}

/**
 * pango_renderer_get_layout_line:
 * @renderer: a `PangoRenderer`
 *
 * Gets the line currently being rendered using @renderer.
 *
 * Calling this function only makes sense from inside a subclass's
 * methods, like in its draw_shape vfunc, for example.
 *
 * The returned line should not be modified while still being
 * rendered.
 *
 * Return value: (transfer none) (nullable): the line, or %NULL
 *   if no layout line is being rendered using @renderer at this time.
 */
PangoLine *
pango_renderer_get_layout_line (PangoRenderer *renderer)
{
  PangoRendererPrivate *priv = pango_renderer_get_instance_private (renderer);

  return priv->line;
}

/**
 * pango_renderer_get_context:
 * @renderer: a `PangoRenderer`
 *
 * Gets the current context in which @renderer operates.
 *
 * Returns: (nullable) (transfer none): the `PangoContext`
 */
PangoContext *
pango_renderer_get_context (PangoRenderer *renderer)
{
  PangoRendererPrivate *priv = pango_renderer_get_instance_private (renderer);

  return priv->context;
}
