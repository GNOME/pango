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

#include <pango2/pango-types.h>
#include <pango2/pango-font-description.h>
#include <pango2/pango-font-metrics.h>
#include <pango2/pango-font-family.h>

#include <glib-object.h>
#include <hb.h>

G_BEGIN_DECLS


#define PANGO2_TYPE_FONT              (pango2_font_get_type ())

PANGO2_AVAILABLE_IN_ALL
PANGO2_DECLARE_INTERNAL_TYPE (Pango2Font, pango2_font, PANGO2, FONT, GObject)

PANGO2_AVAILABLE_IN_ALL
Pango2FontDescription * pango2_font_describe            (Pango2Font        *font);
PANGO2_AVAILABLE_IN_ALL
Pango2FontDescription * pango2_font_describe_with_absolute_size (Pango2Font        *font);
PANGO2_AVAILABLE_IN_ALL
Pango2FontMetrics *     pango2_font_get_metrics         (Pango2Font        *font,
                                                         Pango2Language    *language);
PANGO2_AVAILABLE_IN_ALL
void                    pango2_font_get_glyph_extents   (Pango2Font        *font,
                                                         Pango2Glyph        glyph,
                                                         Pango2Rectangle   *ink_rect,
                                                         Pango2Rectangle   *logical_rect);

PANGO2_AVAILABLE_IN_ALL
Pango2FontFace *        pango2_font_get_face            (Pango2Font        *font);

PANGO2_AVAILABLE_IN_ALL
hb_font_t *             pango2_font_get_hb_font         (Pango2Font        *font);

PANGO2_AVAILABLE_IN_ALL
int                     pango2_font_get_size            (Pango2Font        *font);

PANGO2_AVAILABLE_IN_ALL
double                  pango2_font_get_absolute_size   (Pango2Font        *font);

PANGO2_AVAILABLE_IN_ALL
Pango2Gravity           pango2_font_get_gravity         (Pango2Font        *font);

PANGO2_AVAILABLE_IN_ALL
GBytes *                pango2_font_serialize           (Pango2Font        *font);

PANGO2_AVAILABLE_IN_ALL
Pango2Font *            pango2_font_deserialize         (Pango2Context     *context,
                                                         GBytes            *bytes,
                                                         GError           **error);

G_END_DECLS
