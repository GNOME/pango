/*
 * Copyright 2000 Red Hat Software
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

#include <pango/pango-font-family.h>
#include <pango/pango-font.h>
#include <pango/pango-types.h>

#include <glib-object.h>

#ifdef HAVE_CAIRO
#include <cairo.h>
#endif

struct _PangoFont
{
  GObject parent_instance;

  PangoFontFace *face;

  hb_font_t *hb_font;

  int size; /* point size, scaled by PANGO_SCALE */
  float dpi;
  PangoGravity gravity;
  PangoMatrix matrix;

#ifdef HAVE_CAIRO
  cairo_font_options_t *options;
#endif
};

typedef struct _PangoFontClass       PangoFontClass;
struct _PangoFontClass
{
  GObjectClass parent_class;

  PangoFontDescription * (* describe)           (PangoFont      *font);
  void                   (* get_glyph_extents)  (PangoFont      *font,
                                                PangoGlyph      glyph,
                                                PangoRectangle *ink_rect,
                                                PangoRectangle *logical_rect);
  PangoFontMetrics *     (* get_metrics)        (PangoFont      *font,
                                                PangoLanguage  *language);
  void                   (* get_features)       (PangoFont      *font,
                                                 hb_feature_t   *features,
                                                 guint           len,
                                                 guint          *num_features);
  hb_font_t *            (* create_hb_font)     (PangoFont      *font);
  gboolean               (* is_hinted)          (PangoFont      *font);
  void                   (* get_scale_factors)  (PangoFont      *font,
                                                 double         *x_scale,
                                                 double         *y_scale);
  void                   (* get_matrix)         (PangoFont      *font,
                                                 PangoMatrix    *matrix);
  int                    (* get_absolute_size)  (PangoFont      *font);
};

#define PANGO_FONT_CLASS(klass)      (G_TYPE_CHECK_CLASS_CAST ((klass), PANGO_TYPE_FONT, PangoFontClass))
#define PANGO_FONT_GET_CLASS(obj)    (G_TYPE_INSTANCE_GET_CLASS ((obj), PANGO_TYPE_FONT, PangoFontClass))

static inline void
pango_font_set_face (PangoFont     *font,
                     PangoFontFace *face)
{
  font->face = g_object_ref (face);
}

static inline void
pango_font_set_size (PangoFont *font,
                     int        size)
{
  font->size = size;
}

static inline void
pango_font_set_dpi (PangoFont *font,
                    float      dpi)
{
  font->dpi = dpi;
}

static inline void
pango_font_set_gravity (PangoFont    *font,
                        PangoGravity  gravity)
{
  font->gravity = gravity;
}

static inline void
pango_font_set_matrix (PangoFont         *font,
                       const PangoMatrix *matrix)
{
  font->matrix = *matrix;
}

gboolean pango_font_is_hinted         (PangoFont *font);
void     pango_font_get_scale_factors (PangoFont *font,
                                       double    *x_scale,
                                       double    *y_scale);
void     pango_font_get_matrix        (PangoFont   *font,
                                       PangoMatrix *matrix);

static inline int pango_font_get_absolute_size (PangoFont *font)
{
  return PANGO_FONT_GET_CLASS (font)->get_absolute_size (font);
}

gboolean pango_font_description_is_similar       (const PangoFontDescription *a,
                                                  const PangoFontDescription *b);

int      pango_font_description_compute_distance (const PangoFontDescription *a,
                                                  const PangoFontDescription *b);

/* We use these values in a few places as a fallback size for an
 * unknown glyph, if we have no better information.
 */

#define PANGO_UNKNOWN_GLYPH_WIDTH  10
#define PANGO_UNKNOWN_GLYPH_HEIGHT 14
