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
#include <pango/pango-coverage.h>
#include <pango/pango-types.h>

#include <glib-object.h>


struct _PangoFont
{
  GObject parent_instance;

  hb_font_t *hb_font;
};

typedef struct _PangoFontClass       PangoFontClass;
struct _PangoFontClass
{
  GObjectClass parent_class;

  PangoFontDescription * (* describe)           (PangoFont      *font);
  PangoCoverage *        (* get_coverage)       (PangoFont      *font,
                                                PangoLanguage  *language);
  void                   (* get_glyph_extents)  (PangoFont      *font,
                                                PangoGlyph      glyph,
                                                PangoRectangle *ink_rect,
                                                PangoRectangle *logical_rect);
  PangoFontMetrics *     (* get_metrics)        (PangoFont      *font,
                                                PangoLanguage  *language);
  PangoFontMap *         (* get_font_map)       (PangoFont      *font);
  PangoFontDescription * (* describe_absolute)  (PangoFont      *font);
  void                   (* get_features)       (PangoFont      *font,
                                                 hb_feature_t   *features,
                                                 guint           len,
                                                 guint          *num_features);
  hb_font_t *            (* create_hb_font)     (PangoFont      *font);
  PangoLanguage **       (* get_languages)      (PangoFont      *font);
  gboolean               (* is_hinted)          (PangoFont      *font);
  void                   (* get_scale_factors)  (PangoFont      *font,
                                                 double         *x_scale,
                                                 double         *y_scale);
  gboolean               (* has_char)           (PangoFont      *font,
                                                 gunichar        wc);
  PangoFontFace *        (* get_face)           (PangoFont      *font);
  void                   (* get_matrix)         (PangoFont      *font,
                                                 PangoMatrix    *matrix);
  int                    (* get_absolute_size)  (PangoFont      *font);
};

#define PANGO_FONT_CLASS(klass)      (G_TYPE_CHECK_CLASS_CAST ((klass), PANGO_TYPE_FONT, PangoFontClass))
#define PANGO_FONT_GET_CLASS(obj)    (G_TYPE_INSTANCE_GET_CLASS ((obj), PANGO_TYPE_FONT, PangoFontClass))

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
