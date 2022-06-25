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

typedef struct _Pango2Analysis Pango2Analysis;
typedef struct _Pango2Item Pango2Item;

/**
 * PANGO2_ANALYSIS_FLAG_CENTERED_BASELINE:
 *
 * Whether the segment should be shifted to center around the baseline.
 *
 * This is mainly used in vertical writing directions.
 */
#define PANGO2_ANALYSIS_FLAG_CENTERED_BASELINE (1 << 0)

/**
 * PANGO2_ANALYSIS_FLAG_IS_ELLIPSIS:
 *
 * Whether this run holds ellipsized text.
 */
#define PANGO2_ANALYSIS_FLAG_IS_ELLIPSIS (1 << 1)

/**
 * PANGO2_ANALYSIS_FLAG_NEED_HYPHEN:
 *
 * Whether to add a hyphen at the end of the run during shaping.
 */
#define PANGO2_ANALYSIS_FLAG_NEED_HYPHEN (1 << 2)

#define PANGO2_TYPE_ITEM (pango2_item_get_type ())

PANGO2_AVAILABLE_IN_ALL
GType                   pango2_item_get_type          (void) G_GNUC_CONST;

PANGO2_AVAILABLE_IN_ALL
Pango2Item *            pango2_item_copy              (Pango2Item         *item);
PANGO2_AVAILABLE_IN_ALL
void                    pango2_item_free              (Pango2Item         *item);

PANGO2_AVAILABLE_IN_ALL
GList *                 pango2_itemize                (Pango2Context      *context,
                                                       Pango2Direction     base_dir,
                                                       const char         *text,
                                                       int                 start_index,
                                                       int                 length,
                                                       Pango2AttrList     *attrs);

PANGO2_AVAILABLE_IN_ALL
Pango2Font *            pango2_analysis_get_font                 (const Pango2Analysis *analysis);
PANGO2_AVAILABLE_IN_ALL
int                     pango2_analysis_get_bidi_level           (const Pango2Analysis *analysis);
PANGO2_AVAILABLE_IN_ALL
Pango2Gravity           pango2_analysis_get_gravity              (const Pango2Analysis *analysis);
PANGO2_AVAILABLE_IN_ALL
guint                   pango2_analysis_get_flags                (const Pango2Analysis *analysis);
PANGO2_AVAILABLE_IN_ALL
GUnicodeScript          pango2_analysis_get_script               (const Pango2Analysis *analysis);
PANGO2_AVAILABLE_IN_ALL
Pango2Language *        pango2_analysis_get_language             (const Pango2Analysis *analysis);
PANGO2_AVAILABLE_IN_ALL
GSList *                pango2_analysis_get_extra_attributes     (const Pango2Analysis *analysis);
PANGO2_AVAILABLE_IN_ALL
const Pango2Analysis *  pango2_item_get_analysis                 (Pango2Item *item);
PANGO2_AVAILABLE_IN_ALL
int                     pango2_item_get_byte_offset              (Pango2Item *item);
PANGO2_AVAILABLE_IN_ALL
int                     pango2_item_get_byte_length              (Pango2Item *item);
PANGO2_AVAILABLE_IN_ALL
int                     pango2_item_get_char_offset              (Pango2Item *item);
PANGO2_AVAILABLE_IN_ALL
int                     pango2_item_get_char_length              (Pango2Item *item);

G_DEFINE_AUTOPTR_CLEANUP_FUNC(Pango2Item, pango2_item_free)

G_END_DECLS
