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
PangoFontMap         *pango_font_get_font_map      (PangoFont        *font);

PANGO_AVAILABLE_IN_ALL
PangoFontFace *       pango_font_get_face          (PangoFont        *font);

PANGO_AVAILABLE_IN_ALL
gboolean              pango_font_has_char          (PangoFont        *font,
                                                    gunichar          wc);
PANGO_AVAILABLE_IN_ALL
void                  pango_font_get_features      (PangoFont        *font,
                                                    hb_feature_t     *features,
                                                    guint             len,
                                                    guint            *num_features);
PANGO_AVAILABLE_IN_ALL
hb_font_t *           pango_font_get_hb_font       (PangoFont        *font);

PANGO_AVAILABLE_IN_ALL
PangoLanguage **      pango_font_get_languages     (PangoFont        *font);

PANGO_AVAILABLE_IN_ALL
GBytes *              pango_font_serialize         (PangoFont        *font);

PANGO_AVAILABLE_IN_ALL
PangoFont *           pango_font_deserialize       (PangoContext     *context,
                                                    GBytes           *bytes,
                                                    GError          **error);

/**
 * PANGO_GLYPH_EMPTY:
 *
 * A `PangoGlyph` value that indicates a zero-width empty glpyh.
 *
 * This is useful for example in shaper modules, to use as the glyph for
 * various zero-width Unicode characters (those passing [func@is_zero_width]).
 */

/**
 * PANGO_GLYPH_INVALID_INPUT:
 *
 * A `PangoGlyph` value for invalid input.
 *
 * `PangoLayout` produces one such glyph per invalid input UTF-8 byte and such
 * a glyph is rendered as a crossed box.
 *
 * Note that this value is defined such that it has the %PANGO_GLYPH_UNKNOWN_FLAG
 * set.
 */
/**
 * PANGO_GLYPH_UNKNOWN_FLAG:
 *
 * Flag used in `PangoGlyph` to turn a `gunichar` value of a valid Unicode
 * character into an unknown-character glyph for that `gunichar`.
 *
 * Such unknown-character glyphs may be rendered as a 'hex box'.
 */
/**
 * PANGO_GET_UNKNOWN_GLYPH:
 * @wc: a Unicode character
 *
 * The way this unknown glyphs are rendered is backend specific. For example,
 * a box with the hexadecimal Unicode code-point of the character written in it
 * is what is done in the most common backends.
 *
 * Returns: a `PangoGlyph` value that means no glyph was found for @wc.
 */
#define PANGO_GLYPH_EMPTY           ((PangoGlyph)0x0FFFFFFF)
#define PANGO_GLYPH_INVALID_INPUT   ((PangoGlyph)0xFFFFFFFF)
#define PANGO_GLYPH_UNKNOWN_FLAG    ((PangoGlyph)0x10000000)
#define PANGO_GET_UNKNOWN_GLYPH(wc) ((PangoGlyph)(wc)|PANGO_GLYPH_UNKNOWN_FLAG)

G_END_DECLS
