/*
 * Copyright (C) 2000 Red Hat Software
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

#include <pango/pango-types.h>
#include <pango/pango-font-description.h>
#include <pango/pango-font-metrics.h>
#include <pango/pango-font-family.h>

#include <glib-object.h>
#include <hb.h>

G_BEGIN_DECLS


#define PANGO_TYPE_FONT              (pango_font_get_type ())

PANGO_AVAILABLE_IN_ALL
PANGO_DECLARE_INTERNAL_TYPE (PangoFont, pango_font, PANGO, FONT, GObject)

PANGO_AVAILABLE_IN_ALL
PangoFontDescription *pango_font_describe          (PangoFont        *font);
PANGO_AVAILABLE_IN_ALL
PangoFontDescription *pango_font_describe_with_absolute_size (PangoFont        *font);
PANGO_AVAILABLE_IN_ALL
PangoFontMetrics *    pango_font_get_metrics       (PangoFont        *font,
                                                    PangoLanguage    *language);
PANGO_AVAILABLE_IN_ALL
void                  pango_font_get_glyph_extents (PangoFont        *font,
                                                    PangoGlyph        glyph,
                                                    PangoRectangle   *ink_rect,
                                                    PangoRectangle   *logical_rect);

PANGO_AVAILABLE_IN_ALL
PangoFontFace *       pango_font_get_face          (PangoFont        *font);

PANGO_AVAILABLE_IN_ALL
void                  pango_font_get_features      (PangoFont        *font,
                                                    hb_feature_t     *features,
                                                    guint             len,
                                                    guint            *num_features);
PANGO_AVAILABLE_IN_ALL
hb_font_t *           pango_font_get_hb_font       (PangoFont        *font);

PANGO_AVAILABLE_IN_ALL
GBytes *              pango_font_serialize         (PangoFont        *font);

PANGO_AVAILABLE_IN_ALL
PangoFont *           pango_font_deserialize       (PangoContext     *context,
                                                    GBytes           *bytes,
                                                    GError          **error);

G_END_DECLS
