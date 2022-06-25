/*
 * Copyright 2021 Red Hat, Inc.
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

#include <pango/pango-item.h>
#include <pango/pango-break.h>

/*< private >
 * Pango2Analysis:
 * @size_font: font to use for determining line height
 * @font: the font for this segment
 * @level: the bidirectional level for this segment.
 * @gravity: the glyph orientation for this segment (A `Pango2Gravity`).
 * @flags: boolean flags for this segment
 * @script: the detected script for this segment (A `Pango2Script`)
 * @language: the detected language for this segment.
 * @extra_attrs: extra attributes for this segment.
 *
 * The `Pango2Analysis` structure stores information about
 * the properties of a segment of text.
 */
struct _Pango2Analysis
{
  Pango2Font *size_font;
  Pango2Font *font;

  guint8 level;
  guint8 gravity;
  guint8 flags;

  guint8 script;
  Pango2Language *language;

  GSList *extra_attrs;
};

/*< private>
 * Pango2Item:
 * @offset: byte offset of the start of this item in text.
 * @length: length of this item in bytes.
 * @num_chars: number of Unicode characters in the item.
 * @char_offset: character offset of the start of this item in text. Since 1.50
 * @analysis: analysis results for the item.
 *
 * The `Pango2Item` structure stores information about a segment of text.
 *
 * You typically obtain `Pango2Items` by itemizing a piece of text
 * with [func@itemize].
 */
struct _Pango2Item
{
  int offset;
  int length;
  int num_chars;
  int char_offset;
  Pango2Analysis analysis;
};


void               pango2_analysis_collect_features    (const Pango2Analysis        *analysis,
                                                        hb_feature_t                *features,
                                                        guint                        length,
                                                        guint                       *num_features);

void               pango2_analysis_set_size_font       (Pango2Analysis              *analysis,
                                                        Pango2Font                  *font);
Pango2Font *       pango2_analysis_get_size_font       (const Pango2Analysis        *analysis);

GList *            pango2_itemize_with_font            (Pango2Context               *context,
                                                        Pango2Direction              base_dir,
                                                        const char                  *text,
                                                        int                          start_index,
                                                        int                          length,
                                                        Pango2AttrList              *attrs,
                                                        Pango2AttrIterator          *cached_iter,
                                                        const Pango2FontDescription *desc);

GList *            pango2_itemize_post_process_items   (Pango2Context               *context,
                                                        const char                  *text,
                                                        Pango2LogAttr               *log_attrs,
                                                        GList                       *items);

Pango2Item *       pango2_item_new                     (void);
Pango2Item *       pango2_item_split                   (Pango2Item                  *orig,
                                                        int                         split_index,
                                                        int                         split_offset);
void               pango2_item_unsplit                 (Pango2Item                  *orig,
                                                        int                         split_index,
                                                        int                         split_offset);
void               pango2_item_apply_attrs             (Pango2Item                  *item,
                                                        Pango2AttrIterator          *iter);


typedef struct _ItemProperties ItemProperties;
struct _ItemProperties
{
  Pango2LineStyle uline_style;
  Pango2UnderlinePosition uline_position;
  Pango2LineStyle strikethrough_style;
  Pango2LineStyle oline_style;
  guint oline_single        : 1;
  guint showing_space       : 1;
  guint no_paragraph_break  : 1;
  int letter_spacing;
  int line_spacing;
  int absolute_line_height;
  double line_height;
  Pango2Attribute *shape;
};

void               pango2_item_get_properties          (Pango2Item        *item,
                                                       ItemProperties   *properties);
