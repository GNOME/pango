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
GList *                 pango_itemize                (PangoContext      *context,
                                                      PangoDirection     base_dir,
                                                      const char        *text,
                                                      int                start_index,
                                                      int                length,
                                                      PangoAttrList     *attrs);

PANGO_AVAILABLE_IN_ALL
PangoFont *             pango_analysis_get_font                 (const PangoAnalysis *analysis);
PANGO_AVAILABLE_IN_ALL
int                     pango_analysis_get_bidi_level           (const PangoAnalysis *analysis);
PANGO_AVAILABLE_IN_ALL
PangoGravity            pango_analysis_get_gravity              (const PangoAnalysis *analysis);
PANGO_AVAILABLE_IN_ALL
guint                   pango_analysis_get_flags                (const PangoAnalysis *analysis);
PANGO_AVAILABLE_IN_ALL
GUnicodeScript          pango_analysis_get_script               (const PangoAnalysis *analysis);
PANGO_AVAILABLE_IN_ALL
PangoLanguage *         pango_analysis_get_language             (const PangoAnalysis *analysis);
PANGO_AVAILABLE_IN_ALL
GSList *                pango_analysis_get_extra_attributes     (const PangoAnalysis *analysis);
PANGO_AVAILABLE_IN_ALL
const PangoAnalysis *   pango_item_get_analysis                 (PangoItem *item);
PANGO_AVAILABLE_IN_ALL
int                     pango_item_get_byte_offset              (PangoItem *item);
PANGO_AVAILABLE_IN_ALL
int                     pango_item_get_byte_length              (PangoItem *item);
PANGO_AVAILABLE_IN_ALL
int                     pango_item_get_char_offset              (PangoItem *item);
PANGO_AVAILABLE_IN_ALL
int                     pango_item_get_char_length              (PangoItem *item);

G_END_DECLS
