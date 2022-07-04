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

#include "pango-font-family.h"
#include "pango-font.h"
#include "pango-types.h"

#include <glib-object.h>

#ifdef HAVE_CAIRO
#include <cairo.h>
#endif

struct _Pango2Font
{
  GObject parent_instance;

  Pango2FontFace *face;

  hb_font_t *hb_font;

  int size; /* point size, scaled by PANGO2_SCALE */
  float dpi;
  Pango2Gravity gravity;
  Pango2Matrix ctm;

#ifdef HAVE_CAIRO
  cairo_font_options_t *options;
#endif
};

typedef struct _Pango2FontClass       Pango2FontClass;
struct _Pango2FontClass
{
  GObjectClass parent_class;

  Pango2FontDescription * (* describe)           (Pango2Font      *font);
  void                    (* get_glyph_extents)  (Pango2Font      *font,
                                                  Pango2Glyph      glyph,
                                                  Pango2Rectangle *ink_rect,
                                                  Pango2Rectangle *logical_rect);
  Pango2FontMetrics *     (* get_metrics)        (Pango2Font      *font,
                                                  Pango2Language  *language);
  hb_font_t *             (* create_hb_font)     (Pango2Font      *font);
  gboolean                (* is_hinted)          (Pango2Font      *font);
  void                    (* get_scale_factors)  (Pango2Font      *font,
                                                  double          *x_scale,
                                                  double          *y_scale);
  void                    (* get_transform)      (Pango2Font      *font,
                                                  Pango2Matrix    *matrix);
};

#define PANGO2_FONT_CLASS(klass)      (G_TYPE_CHECK_CLASS_CAST ((klass), PANGO2_TYPE_FONT, Pango2FontClass))
#define PANGO2_FONT_GET_CLASS(obj)    (G_TYPE_INSTANCE_GET_CLASS ((obj), PANGO2_TYPE_FONT, Pango2FontClass))

static inline void
pango2_font_set_face (Pango2Font     *font,
                     Pango2FontFace *face)
{
  font->face = (Pango2FontFace *) g_object_ref (face);
}

static inline void
pango2_font_set_size (Pango2Font *font,
                      int         size)
{
  font->size = size;
}

static inline void
pango2_font_set_dpi (Pango2Font *font,
                     float       dpi)
{
  font->dpi = dpi;
}

static inline void
pango2_font_set_gravity (Pango2Font    *font,
                         Pango2Gravity  gravity)
{
  font->gravity = gravity;
}

static inline void
pango2_font_set_ctm (Pango2Font         *font,
                     const Pango2Matrix *ctm)
{
  const Pango2Matrix matrix_init = PANGO2_MATRIX_INIT;
  font->ctm = ctm ? *ctm : matrix_init;
}

gboolean pango2_font_is_hinted         (Pango2Font  *font);
void     pango2_font_get_scale_factors (Pango2Font  *font,
                                        double      *x_scale,
                                        double      *y_scale);
void     pango2_font_get_transform     (Pango2Font  *font,
                                       Pango2Matrix *matrix);

gboolean pango2_font_description_is_similar       (const Pango2FontDescription *a,
                                                   const Pango2FontDescription *b);

int      pango2_font_description_compute_distance (const Pango2FontDescription *a,
                                                   const Pango2FontDescription *b);

/* We use these values in a few places as a fallback size for an
 * unknown glyph, if we have no better information.
 */

#define PANGO2_UNKNOWN_GLYPH_WIDTH  10
#define PANGO2_UNKNOWN_GLYPH_HEIGHT 14
