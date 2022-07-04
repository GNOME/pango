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

#include "pango-glyph-item-private.h"

G_BEGIN_DECLS

/**
 * Pango2GlyphItemIter:
 *
 * A `Pango2GlyphItemIter` is an iterator over the clusters in a
 * `Pango2GlyphItem`.
 *
 * The *forward direction* of the iterator is the logical direction of text.
 * That is, with increasing @start_index and @start_char values. If @glyph_item
 * is right-to-left (that is, if `glyph_item->item->analysis.level` is odd),
 * then @start_glyph decreases as the iterator moves forward.  Moreover,
 * in right-to-left cases, @start_glyph is greater than @end_glyph.
 *
 * An iterator should be initialized using either
 * [method@Pango2.GlyphItemIter.init_start] or
 * [method@Pango2.GlyphItemIter.init_end], for forward and backward iteration
 * respectively, and walked over using any desired mixture of
 * [method@Pango2.GlyphItemIter.next_cluster] and
 * [method@Pango2.GlyphItemIter.prev_cluster].
 *
 * A common idiom for doing a forward iteration over the clusters is:
 *
 * ```
 * Pango2GlyphItemIter cluster_iter;
 * gboolean have_cluster;
 *
 * for (have_cluster = pango2_glyph_item_iter_init_start (&cluster_iter,
 *                                                        glyph_item, text);
 *      have_cluster;
 *      have_cluster = pango2_glyph_item_iter_next_cluster (&cluster_iter))
 * {
 *   ...
 * }
 * ```
 *
 * Note that @text is the start of the text for layout, which is then
 * indexed by `glyph_item->item->offset` to get to the text of @glyph_item.
 * The @start_index and @end_index values can directly index into @text. The
 * @start_glyph, @end_glyph, @start_char, and @end_char values however are
 * zero-based for the @glyph_item.  For each cluster, the item pointed at by
 * the start variables is included in the cluster while the one pointed at by
 * end variables is not.
 *
 * None of the members of a `Pango2GlyphItemIter` should be modified manually.
 */
typedef struct _Pango2GlyphItemIter Pango2GlyphItemIter;

struct _Pango2GlyphItemIter
{
  Pango2GlyphItem *glyph_item;
  const char *text;

  int start_glyph;
  int start_index;
  int start_char;

  int end_glyph;
  int end_index;
  int end_char;
};

#define PANGO2_TYPE_GLYPH_ITEM_ITER (pango2_glyph_item_iter_get_type ())

PANGO2_AVAILABLE_IN_ALL
GType                   pango2_glyph_item_iter_get_type     (void) G_GNUC_CONST;
PANGO2_AVAILABLE_IN_ALL
Pango2GlyphItemIter *   pango2_glyph_item_iter_copy         (Pango2GlyphItemIter *orig);
PANGO2_AVAILABLE_IN_ALL
void                    pango2_glyph_item_iter_free         (Pango2GlyphItemIter *iter);

PANGO2_AVAILABLE_IN_ALL
gboolean                pango2_glyph_item_iter_init_start   (Pango2GlyphItemIter *iter,
                                                             Pango2GlyphItem     *glyph_item,
                                                             const char          *text);
PANGO2_AVAILABLE_IN_ALL
gboolean                pango2_glyph_item_iter_init_end     (Pango2GlyphItemIter *iter,
                                                             Pango2GlyphItem     *glyph_item,
                                                             const char          *text);
PANGO2_AVAILABLE_IN_ALL
gboolean                pango2_glyph_item_iter_next_cluster (Pango2GlyphItemIter *iter);
PANGO2_AVAILABLE_IN_ALL
gboolean                pango2_glyph_item_iter_prev_cluster (Pango2GlyphItemIter *iter);

G_DEFINE_AUTOPTR_CLEANUP_FUNC(Pango2GlyphItemIter, pango2_glyph_item_iter_free)

G_END_DECLS
