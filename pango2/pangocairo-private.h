/*
 * Copyright (C) 2000, 2004 Red Hat, Inc.
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

#include "pangocairo.h"
#include "pango-renderer.h"

G_BEGIN_DECLS

typedef struct _Pango2CairoFontPrivate                Pango2CairoFontPrivate;
typedef struct _HexBoxInfo                            Pango2CairoFontHexBoxInfo;
typedef struct _Pango2CairoFontPrivateScaledFontData  Pango2CairoFontPrivateScaledFontData;

struct _Pango2CairoFontPrivateScaledFontData
{
  cairo_matrix_t font_matrix;
  cairo_matrix_t ctm;
  cairo_font_options_t *options;
};

struct _Pango2CairoFontPrivate
{
  Pango2Font *cfont;

  Pango2CairoFontPrivateScaledFontData *data;

  GQuark palette;
  cairo_scaled_font_t *scaled_font;
  Pango2CairoFontHexBoxInfo *hbi;

  gboolean is_hinted;
  Pango2Gravity gravity;

  Pango2Rectangle font_extents;
};

gboolean                _pango2_cairo_font_install                      (Pango2Font     *font,
                                                                         GQuark          palette,
                                                                         cairo_t        *cr);
Pango2CairoFontHexBoxInfo *
                        _pango2_cairo_font_get_hex_box_info             (Pango2Font     *font);

#define PANGO2_TYPE_CAIRO_RENDERER            (pango2_cairo_renderer_get_type())
#define PANGO2_CAIRO_RENDERER(object)         (G_TYPE_CHECK_INSTANCE_CAST ((object), PANGO2_TYPE_CAIRO_RENDERER, Pango2CairoRenderer))
#define PANGO2_IS_CAIRO_RENDERER(object)      (G_TYPE_CHECK_INSTANCE_TYPE ((object), PANGO2_TYPE_CAIRO_RENDERER))

typedef struct _Pango2CairoRenderer Pango2CairoRenderer;

_PANGO2_EXTERN
GType                   pango2_cairo_renderer_get_type                  (void) G_GNUC_CONST;

const cairo_font_options_t *
                        pango2_cairo_context_get_merged_font_options    (Pango2Context  *context);

cairo_font_face_t *     create_cairo_user_font_face                     (Pango2Font     *font);

#ifdef CAIRO_HAS_FT_FONT
cairo_font_face_t *     create_cairo_ft_font_face                       (Pango2Font     *font);
#endif

#ifdef HAVE_CORE_TEXT
cairo_font_face_t *     create_cairo_core_text_font_face                (Pango2Font     *font);
#endif

#ifdef HAVE_DIRECT_WRITE
cairo_font_face_t *     create_cairo_dwrite_font_face                   (Pango2Font     *font);
#endif

G_END_DECLS
