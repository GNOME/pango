/*
 * Copyright (C) 2004, Red Hat, Inc.
 *
 * SPDX-License-Identifier: LGPL-2.1-or-later
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public
 * License as published by the Free Software Foundation; either
 * version 2.1 of the License, or (at your option) any later version.
 *
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public
 * License along with this library; if not, see <http://www.gnu.org/licenses/>.
 */

#pragma once

#include <pango/pango-layout.h>
#include <pango/pango-lines.h>
#include <pango/pango-glyph.h>

G_BEGIN_DECLS

#define PANGO2_TYPE_RENDERER            (pango2_renderer_get_type())
#define PANGO2_RENDERER(object)         (G_TYPE_CHECK_INSTANCE_CAST ((object), PANGO2_TYPE_RENDERER, Pango2Renderer))
#define PANGO2_IS_RENDERER(object)      (G_TYPE_CHECK_INSTANCE_TYPE ((object), PANGO2_TYPE_RENDERER))
#define PANGO2_RENDERER_CLASS(klass)    (G_TYPE_CHECK_CLASS_CAST ((klass), PANGO2_TYPE_RENDERER, Pango2RendererClass))
#define PANGO2_IS_RENDERER_CLASS(klass) (G_TYPE_CHECK_CLASS_TYPE ((klass), PANGO2_TYPE_RENDERER))
#define PANGO2_RENDERER_GET_CLASS(obj)  (G_TYPE_INSTANCE_GET_CLASS ((obj), PANGO2_TYPE_RENDERER, Pango2RendererClass))

typedef struct _Pango2Renderer        Pango2Renderer;
typedef struct _Pango2RendererClass   Pango2RendererClass;

/**
 * Pango2RenderPart:
 * @PANGO2_RENDER_PART_FOREGROUND: the text itself
 * @PANGO2_RENDER_PART_BACKGROUND: the area behind the text
 * @PANGO2_RENDER_PART_UNDERLINE: underlines
 * @PANGO2_RENDER_PART_STRIKETHROUGH: strikethrough lines
 * @PANGO2_RENDER_PART_OVERLINE: overlines
 *
 * `Pango2RenderPart` defines different items to render for such
 * purposes as setting colors.
 */
/* When extending, note N_RENDER_PARTS #define in pango-renderer.c */
typedef enum
{
  PANGO2_RENDER_PART_FOREGROUND,
  PANGO2_RENDER_PART_BACKGROUND,
  PANGO2_RENDER_PART_UNDERLINE,
  PANGO2_RENDER_PART_STRIKETHROUGH,
  PANGO2_RENDER_PART_OVERLINE
} Pango2RenderPart;

/**
 * Pango2Renderer:
 *
 * `Pango2Renderer` is a base class for objects that can render text
 * provided as `Pango2GlyphString` or `Pango2Layout`.
 *
 * By subclassing `Pango2Renderer` and overriding operations such as
 * @draw_glyphs and @draw_rectangle, renderers for particular font
 * backends and destinations can be created.
 */
struct _Pango2Renderer
{
  GObject parent_instance;
};

/**
 * Pango2RendererClass:
 * @draw_glyphs: draws a `Pango2GlyphString`
 * @draw_rectangle: draws a rectangle
 * @draw_line: draws a line in the given style that approximately
 *   covers the given rectangle
 * @draw_trapezoid: draws a trapezoidal filled area
 * @draw_glyph: draws a single glyph
 * @part_changed: do renderer specific processing when rendering
 *   attributes change
 * @begin: Do renderer-specific initialization before drawing
 * @end: Do renderer-specific cleanup after drawing
 * @prepare_run: updates the renderer for a new run
 * @draw_glyph_item: draws a `Pango2GlyphItem`
 *
 * Class structure for `Pango2Renderer`.
 *
 * The following vfuncs take user space coordinates in Pango2 units
 * and have default implementations:
 * - draw_glyphs
 * - draw_rectangle
 * - draw_styled_line
 * - draw_shape
 * - draw_glyph_item
 *
 * The default draw_shape implementation draws nothing.
 *
 * The following vfuncs take device space coordinates as doubles
 * and must be implemented:
 * - draw_trapezoid
 * - draw_glyph
 */
struct _Pango2RendererClass
{
  /*< private >*/
  GObjectClass parent_class;

  /*< public >*/

  void (* draw_glyphs)          (Pango2Renderer    *renderer,
                                 Pango2Font        *font,
                                 Pango2GlyphString *glyphs,
                                 int                x,
                                 int                y);
  void (* draw_rectangle)       (Pango2Renderer    *renderer,
                                 Pango2RenderPart   part,
                                 int                x,
                                 int                y,
                                 int                width,
                                 int                height);
  void (* draw_styled_line)     (Pango2Renderer    *renderer,
                                 Pango2RenderPart   part,
                                 Pango2LineStyle    style,
                                 int                x,
                                 int                y,
                                 int                width,
                                 int                height);
  void (* draw_line)            (Pango2Renderer    *renderer,
                                 Pango2LineStyle    style,
                                 int                x,
                                 int                y,
                                 int                width,
                                 int                height);
  void (* draw_shape)           (Pango2Renderer    *renderer,
                                 Pango2Rectangle   *ink_rect,
                                 Pango2Rectangle   *logical_rect,
                                 gpointer           data,
                                 int                x,
                                 int                y);
  void (* draw_trapezoid)       (Pango2Renderer    *renderer,
                                 Pango2RenderPart   part,
                                 double             y1_,
                                 double             x11,
                                 double             x21,
                                 double             y2,
                                 double             x12,
                                 double             x22);
  void (* draw_glyph)           (Pango2Renderer    *renderer,
                                 Pango2Font        *font,
                                 Pango2Glyph        glyph,
                                 double             x,
                                 double             y);

  void (* part_changed)         (Pango2Renderer    *renderer,
                                 Pango2RenderPart   part);

  void (* begin)                (Pango2Renderer    *renderer);
  void (* end)                  (Pango2Renderer    *renderer);

  void (* prepare_run)          (Pango2Renderer    *renderer,
                                 Pango2Run         *run);

  void (* draw_run)             (Pango2Renderer    *renderer,
                                 const char        *text,
                                 Pango2Run         *run,
                                 int                x,
                                 int                y);

  /*< private >*/

  /* Padding for future expansion */
  gpointer _pango2_reserved[8];
};

PANGO2_AVAILABLE_IN_ALL
GType                   pango2_renderer_get_type            (void) G_GNUC_CONST;

PANGO2_AVAILABLE_IN_ALL
void                    pango2_renderer_draw_lines           (Pango2Renderer    *renderer,
                                                              Pango2Lines       *lines,
                                                              int                x,
                                                              int                y);

PANGO2_AVAILABLE_IN_ALL
void                    pango2_renderer_draw_line            (Pango2Renderer    *renderer,
                                                              Pango2Line        *line,
                                                              int                x,
                                                              int                y);
PANGO2_AVAILABLE_IN_ALL
void                    pango2_renderer_draw_glyphs          (Pango2Renderer    *renderer,
                                                              Pango2Font        *font,
                                                              Pango2GlyphString *glyphs,
                                                              int                x,
                                                              int                y);
PANGO2_AVAILABLE_IN_ALL
void                    pango2_renderer_draw_run             (Pango2Renderer    *renderer,
                                                              const char        *text,
                                                              Pango2Run         *run,
                                                              int                x,
                                                              int                y);
PANGO2_AVAILABLE_IN_ALL
void                    pango2_renderer_draw_rectangle       (Pango2Renderer    *renderer,
                                                              Pango2RenderPart   part,
                                                              int                x,
                                                              int                y,
                                                              int                width,
                                                              int                height);
PANGO2_AVAILABLE_IN_ALL
void                    pango2_renderer_draw_styled_line     (Pango2Renderer    *renderer,
                                                              Pango2RenderPart   part,
                                                              Pango2LineStyle    style,
                                                              int                x,
                                                              int                y,
                                                              int                width,
                                                              int                height);
PANGO2_AVAILABLE_IN_ALL
void                    pango2_renderer_draw_error_underline (Pango2Renderer    *renderer,
                                                              int                x,
                                                              int                y,
                                                              int                width,
                                                              int                height);
PANGO2_AVAILABLE_IN_ALL
void                    pango2_renderer_draw_trapezoid       (Pango2Renderer    *renderer,
                                                              Pango2RenderPart   part,
                                                              double             y1_,
                                                              double             x11,
                                                              double             x21,
                                                              double             y2,
                                                              double             x12,
                                                              double             x22);
PANGO2_AVAILABLE_IN_ALL
void                    pango2_renderer_draw_glyph           (Pango2Renderer    *renderer,
                                                              Pango2Font        *font,
                                                              Pango2Glyph        glyph,
                                                              double             x,
                                                              double             y);

PANGO2_AVAILABLE_IN_ALL
void                    pango2_renderer_activate             (Pango2Renderer    *renderer);
PANGO2_AVAILABLE_IN_ALL
void                    pango2_renderer_deactivate           (Pango2Renderer    *renderer);

PANGO2_AVAILABLE_IN_ALL
void                    pango2_renderer_part_changed         (Pango2Renderer   *renderer,
                                                              Pango2RenderPart  part);

PANGO2_AVAILABLE_IN_ALL
void                    pango2_renderer_set_color            (Pango2Renderer    *renderer,
                                                              Pango2RenderPart   part,
                                                              const Pango2Color *color);
PANGO2_AVAILABLE_IN_ALL
Pango2Color *           pango2_renderer_get_color            (Pango2Renderer    *renderer,
                                                              Pango2RenderPart   part);

PANGO2_AVAILABLE_IN_ALL
void                    pango2_renderer_set_matrix           (Pango2Renderer     *renderer,
                                                              const Pango2Matrix *matrix);
PANGO2_AVAILABLE_IN_ALL
const Pango2Matrix *    pango2_renderer_get_matrix           (Pango2Renderer     *renderer);

PANGO2_AVAILABLE_IN_ALL
Pango2Lines *           pango2_renderer_get_lines            (Pango2Renderer     *renderer);

PANGO2_AVAILABLE_IN_ALL
Pango2Line *            pango2_renderer_get_layout_line      (Pango2Renderer     *renderer);

PANGO2_AVAILABLE_IN_ALL
Pango2Context *         pango2_renderer_get_context          (Pango2Renderer     *renderer);

G_END_DECLS
