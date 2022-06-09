/*
 * Copyright (C) 2002 Red Hat Software
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

#include <pango/pango-attributes.h>
#include <pango/pango-break.h>
#include <pango/pango-item.h>
#include <pango/pango-glyph.h>

G_BEGIN_DECLS

/**
 * PangoGlyphItem:
 * @item: corresponding `PangoItem`
 * @glyphs: corresponding `PangoGlyphString`
 * @y_offset: shift of the baseline, relative to the baseline
 *   of the containing line. Positive values shift upwards
 * @start_x_offset: horizontal displacement to apply before the
 *   glyph item. Positive values shift right
 * @end_x_offset: horizontal displacement to apply after th
 *   glyph item. Positive values shift right
 *
 * A `PangoGlyphItem` is a pair of a `PangoItem` and the glyphs
 * resulting from shaping the items text.
 *
 * As an example of the usage of `PangoGlyphItem`, the results
 * of shaping text with `PangoLayout` is a list of `PangoLayoutLine`,
 * each of which contains a list of `PangoGlyphItem`.
 */
typedef struct _PangoGlyphItem PangoGlyphItem;

struct _PangoGlyphItem
{
  PangoItem *item;
  PangoGlyphString *glyphs;
  int y_offset;
  int start_x_offset;
  int end_x_offset;
};

#define PANGO_TYPE_GLYPH_ITEM (pango_glyph_item_get_type ())

PANGO_AVAILABLE_IN_ALL
GType                   pango_glyph_item_get_type               (void) G_GNUC_CONST;

PANGO_AVAILABLE_IN_ALL
PangoGlyphItem *        pango_glyph_item_split                  (PangoGlyphItem *orig,
                                                                 const char     *text,
                                                                 int             split_index);
PANGO_AVAILABLE_IN_ALL
PangoGlyphItem *        pango_glyph_item_copy                   (PangoGlyphItem *orig);
PANGO_AVAILABLE_IN_ALL
void                    pango_glyph_item_free                   (PangoGlyphItem *glyph_item);
PANGO_AVAILABLE_IN_ALL
GSList *                pango_glyph_item_apply_attrs            (PangoGlyphItem *glyph_item,
                                                                 const char     *text,
                                                                 PangoAttrList  *list);
PANGO_AVAILABLE_IN_ALL
void                    pango_glyph_item_letter_space           (PangoGlyphItem *glyph_item,
                                                                 const char     *text,
                                                                 PangoLogAttr   *log_attrs,
                                                                 int             letter_spacing);
PANGO_AVAILABLE_IN_ALL
void                    pango_glyph_item_get_logical_widths     (PangoGlyphItem *glyph_item,
                                                                 const char     *text,
                                                                 int            *logical_widths);


G_END_DECLS
