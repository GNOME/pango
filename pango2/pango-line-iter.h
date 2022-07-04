/*
 * Copyright 2022 Red Hat, Inc.
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

#include <glib-object.h>

#include <pango2/pango-types.h>
#include <pango2/pango-lines.h>

G_BEGIN_DECLS

PANGO2_AVAILABLE_IN_ALL
GType                   pango2_line_iter_get_type                 (void) G_GNUC_CONST;

PANGO2_AVAILABLE_IN_ALL
Pango2LineIter *        pango2_line_iter_copy                     (Pango2LineIter    *iter);

PANGO2_AVAILABLE_IN_ALL
void                    pango2_line_iter_free                     (Pango2LineIter    *iter);

PANGO2_AVAILABLE_IN_ALL
Pango2Lines *           pango2_line_iter_get_lines                (Pango2LineIter    *iter);

PANGO2_AVAILABLE_IN_ALL
Pango2Line *            pango2_line_iter_get_line                 (Pango2LineIter    *iter);

PANGO2_AVAILABLE_IN_ALL
gboolean                pango2_line_iter_at_last_line             (Pango2LineIter    *iter);

PANGO2_AVAILABLE_IN_ALL
Pango2Run *             pango2_line_iter_get_run                  (Pango2LineIter    *iter);

PANGO2_AVAILABLE_IN_ALL
int                     pango2_line_iter_get_index                (Pango2LineIter    *iter);

PANGO2_AVAILABLE_IN_ALL
gboolean                pango2_line_iter_next_line                (Pango2LineIter    *iter);

PANGO2_AVAILABLE_IN_ALL
gboolean                pango2_line_iter_next_run                 (Pango2LineIter    *iter);

PANGO2_AVAILABLE_IN_ALL
gboolean                pango2_line_iter_next_cluster             (Pango2LineIter    *iter);

PANGO2_AVAILABLE_IN_ALL
gboolean                pango2_line_iter_next_char                (Pango2LineIter    *iter);

PANGO2_AVAILABLE_IN_ALL
void                    pango2_line_iter_get_layout_extents       (Pango2LineIter    *iter,
                                                                   Pango2Rectangle   *ink_rect,
                                                                   Pango2Rectangle   *logical_rect);

PANGO2_AVAILABLE_IN_ALL
void                    pango2_line_iter_get_line_extents         (Pango2LineIter    *iter,
                                                                   Pango2Rectangle   *ink_rect,
                                                                   Pango2Rectangle   *logical_rect);

PANGO2_AVAILABLE_IN_ALL
void                    pango2_line_iter_get_trimmed_line_extents (Pango2LineIter    *iter,
                                                                   Pango2LeadingTrim  trim,
                                                                   Pango2Rectangle   *logical_rect);

PANGO2_AVAILABLE_IN_ALL
void                    pango2_line_iter_get_run_extents          (Pango2LineIter    *iter,
                                                                   Pango2Rectangle   *ink_rect,
                                                                   Pango2Rectangle   *logical_rect);

PANGO2_AVAILABLE_IN_ALL
void                    pango2_line_iter_get_cluster_extents      (Pango2LineIter    *iter,
                                                                   Pango2Rectangle   *ink_rect,
                                                                   Pango2Rectangle   *logical_rect);

PANGO2_AVAILABLE_IN_ALL
void                    pango2_line_iter_get_char_extents         (Pango2LineIter    *iter,
                                                                   Pango2Rectangle   *logical_rect);

PANGO2_AVAILABLE_IN_ALL
int                     pango2_line_iter_get_line_baseline        (Pango2LineIter    *iter);

PANGO2_AVAILABLE_IN_ALL
int                     pango2_line_iter_get_run_baseline         (Pango2LineIter    *iter);

G_DEFINE_AUTOPTR_CLEANUP_FUNC(Pango2LineIter, pango2_line_iter_free)

G_END_DECLS
