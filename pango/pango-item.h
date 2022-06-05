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
#include <pango/pango-attr-list.h>
#include <pango/pango-attr-list.h>

G_BEGIN_DECLS

typedef struct _PangoAnalysis PangoAnalysis;
typedef struct _PangoItem PangoItem;

/**
 * PANGO_ANALYSIS_FLAG_CENTERED_BASELINE:
 *
 * Whether the segment should be shifted to center around the baseline.
 *
 * This is mainly used in vertical writing directions.
 */
#define PANGO_ANALYSIS_FLAG_CENTERED_BASELINE (1 << 0)

/**
 * PANGO_ANALYSIS_FLAG_IS_ELLIPSIS:
 *
 * Whether this run holds ellipsized text.
 */
#define PANGO_ANALYSIS_FLAG_IS_ELLIPSIS (1 << 1)

/**
 * PANGO_ANALYSIS_FLAG_NEED_HYPHEN:
 *
 * Whether to add a hyphen at the end of the run during shaping.
 */
#define PANGO_ANALYSIS_FLAG_NEED_HYPHEN (1 << 2)

/**
 * PangoAnalysis:
 * @size_font: font to use for determining line height
 * @font: the font for this segment
 * @level: the bidirectional level for this segment.
 * @gravity: the glyph orientation for this segment (A `PangoGravity`).
 * @flags: boolean flags for this segment
 * @script: the detected script for this segment (A `PangoScript`)
 * @language: the detected language for this segment.
 * @extra_attrs: extra attributes for this segment.
 *
 * The `PangoAnalysis` structure stores information about
 * the properties of a segment of text.
 */
struct _PangoAnalysis
{
  PangoFont *size_font;
  PangoFont *font;

  guint8 level;
  guint8 gravity;
  guint8 flags;

  guint8 script;
  PangoLanguage *language;

  GSList *extra_attrs;
};

/**
 * PangoItem:
 * @offset: byte offset of the start of this item in text.
 * @length: length of this item in bytes.
 * @num_chars: number of Unicode characters in the item.
 * @char_offset: character offset of the start of this item in text. Since 1.50
 * @analysis: analysis results for the item.
 *
 * The `PangoItem` structure stores information about a segment of text.
 *
 * You typically obtain `PangoItems` by itemizing a piece of text
 * with [func@itemize].
 */
struct _PangoItem
{
  int offset;
  int length;
  int num_chars;
  int char_offset;
  PangoAnalysis analysis;
};

#define PANGO_TYPE_ITEM (pango_item_get_type ())

PANGO_AVAILABLE_IN_ALL
GType                   pango_item_get_type          (void) G_GNUC_CONST;

PANGO_AVAILABLE_IN_ALL
PangoItem *             pango_item_new               (void);
PANGO_AVAILABLE_IN_ALL
PangoItem *             pango_item_copy              (PangoItem         *item);
PANGO_AVAILABLE_IN_ALL
void                    pango_item_free              (PangoItem         *item);

PANGO_AVAILABLE_IN_ALL
PangoItem *             pango_item_split             (PangoItem         *orig,
                                                      int                split_index,
                                                      int                split_offset);

PANGO_AVAILABLE_IN_ALL
void                    pango_item_apply_attrs       (PangoItem         *item,
                                                      PangoAttrIterator *iter);

PANGO_AVAILABLE_IN_ALL
GList *                 pango_reorder_items          (GList             *items);

/* Itemization */

PANGO_AVAILABLE_IN_ALL
GList *                 pango_itemize (PangoContext      *context,
                                       PangoDirection     base_dir,
                                       const char        *text,
                                       int                start_index,
                                       int                length,
                                       PangoAttrList     *attrs);


G_END_DECLS
