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

#include "pango-attributes.h"
#include "pango-break.h"
#include "pango-item.h"
#include "pango-glyph.h"

G_BEGIN_DECLS

/**
 * Pango2GlyphItem:
 * @item: corresponding `Pango2Item`
 * @glyphs: corresponding `Pango2GlyphString`
 * @y_offset: shift of the baseline, relative to the baseline
 *   of the containing line. Positive values shift upwards
 * @start_x_offset: horizontal displacement to apply before the
 *   glyph item. Positive values shift right
 * @end_x_offset: horizontal displacement to apply after th
 *   glyph item. Positive values shift right
 *
 * A pair of a `Pango2Item` and the glyphs resulting from shaping
 * the items text.
 *
 * As an example of the usage of `Pango2GlyphItem`, the results
 * of shaping text with `Pango2Layout` is a list of `Pango2Line`,
 * each of which contains a list of `Pango2GlyphItem`.
 */
typedef struct _Pango2GlyphItem Pango2GlyphItem;

struct _Pango2GlyphItem
{
  Pango2Item *item;
  Pango2GlyphString *glyphs;
  int y_offset;
  int start_x_offset;
  int end_x_offset;
};

#define PANGO2_TYPE_GLYPH_ITEM (pango2_glyph_item_get_type ())

PANGO2_AVAILABLE_IN_ALL
GType                   pango2_glyph_item_get_type               (void) G_GNUC_CONST;

PANGO2_AVAILABLE_IN_ALL
Pango2GlyphItem *       pango2_glyph_item_split                  (Pango2GlyphItem *orig,
                                                                  const char      *text,
                                                                  int              split_index);
PANGO2_AVAILABLE_IN_ALL
Pango2GlyphItem *       pango2_glyph_item_copy                   (Pango2GlyphItem *orig);
PANGO2_AVAILABLE_IN_ALL
void                    pango2_glyph_item_free                   (Pango2GlyphItem *glyph_item);
PANGO2_AVAILABLE_IN_ALL
GSList *                pango2_glyph_item_apply_attrs            (Pango2GlyphItem *glyph_item,
                                                                  const char      *text,
                                                                  Pango2AttrList  *list);
PANGO2_AVAILABLE_IN_ALL
void                    pango2_glyph_item_letter_space           (Pango2GlyphItem *glyph_item,
                                                                  const char      *text,
                                                                  Pango2LogAttr   *log_attrs,
                                                                  int              letter_spacing);
PANGO2_AVAILABLE_IN_ALL
void                    pango2_glyph_item_get_logical_widths     (Pango2GlyphItem *glyph_item,
                                                                  const char      *text,
                                                                  int             *logical_widths);

G_DEFINE_AUTOPTR_CLEANUP_FUNC(Pango2GlyphItem, pango2_glyph_item_free)


G_END_DECLS
