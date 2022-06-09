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

#include <pango/pangocairo.h>
#include <pango/pango-renderer.h>

typedef struct _PangoCairoFontPrivate                PangoCairoFontPrivate;
typedef struct _HexBoxInfo                           PangoCairoFontHexBoxInfo;
typedef struct _PangoCairoFontPrivateScaledFontData  PangoCairoFontPrivateScaledFontData;

struct _PangoCairoFontPrivateScaledFontData
{
  cairo_matrix_t font_matrix;
  cairo_matrix_t ctm;
  cairo_font_options_t *options;
};

struct _PangoCairoFontPrivate
{
  PangoFont *cfont;

  PangoCairoFontPrivateScaledFontData *data;

  cairo_scaled_font_t *scaled_font;
  PangoCairoFontHexBoxInfo *hbi;

  gboolean is_hinted;
  PangoGravity gravity;

  PangoRectangle font_extents;
};

gboolean _pango_cairo_font_install (PangoFont *font,
                                    cairo_t   *cr);
PangoCairoFontHexBoxInfo *_pango_cairo_font_get_hex_box_info (PangoFont *font);

#define PANGO_TYPE_CAIRO_RENDERER            (pango_cairo_renderer_get_type())
#define PANGO_CAIRO_RENDERER(object)         (G_TYPE_CHECK_INSTANCE_CAST ((object), PANGO_TYPE_CAIRO_RENDERER, PangoCairoRenderer))
#define PANGO_IS_CAIRO_RENDERER(object)      (G_TYPE_CHECK_INSTANCE_TYPE ((object), PANGO_TYPE_CAIRO_RENDERER))

typedef struct _PangoCairoRenderer PangoCairoRenderer;

_PANGO_EXTERN
GType pango_cairo_renderer_get_type    (void) G_GNUC_CONST;


const cairo_font_options_t *pango_cairo_context_get_merged_font_options (PangoContext *context);
