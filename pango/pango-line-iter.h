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

#include <pango/pango-types.h>
#include <pango/pango-lines.h>
#include <pango/pango-glyph-item.h>

G_BEGIN_DECLS

PANGO_AVAILABLE_IN_ALL
GType                   pango_line_iter_get_type           (void) G_GNUC_CONST;

PANGO_AVAILABLE_IN_ALL
PangoLineIter *         pango_line_iter_copy                (PangoLineIter *iter);

PANGO_AVAILABLE_IN_ALL
void                    pango_line_iter_free                (PangoLineIter *iter);

PANGO_AVAILABLE_IN_ALL
PangoLines *            pango_line_iter_get_lines           (PangoLineIter *iter);

PANGO_AVAILABLE_IN_ALL
PangoLine *             pango_line_iter_get_line            (PangoLineIter *iter);

PANGO_AVAILABLE_IN_ALL
gboolean                pango_line_iter_at_last_line        (PangoLineIter *iter);

PANGO_AVAILABLE_IN_ALL
PangoLayoutRun *        pango_line_iter_get_run             (PangoLineIter *iter);

PANGO_AVAILABLE_IN_ALL
int                     pango_line_iter_get_index           (PangoLineIter *iter);

PANGO_AVAILABLE_IN_ALL
gboolean                pango_line_iter_next_line           (PangoLineIter *iter);

PANGO_AVAILABLE_IN_ALL
gboolean                pango_line_iter_next_run            (PangoLineIter *iter);

PANGO_AVAILABLE_IN_ALL
gboolean                pango_line_iter_next_cluster        (PangoLineIter *iter);

PANGO_AVAILABLE_IN_ALL
gboolean                pango_line_iter_next_char           (PangoLineIter *iter);

PANGO_AVAILABLE_IN_ALL
void                    pango_line_iter_get_layout_extents  (PangoLineIter  *iter,
                                                             PangoRectangle *ink_rect,
                                                             PangoRectangle *logical_rect);

PANGO_AVAILABLE_IN_ALL
void                    pango_line_iter_get_line_extents    (PangoLineIter  *iter,
                                                             PangoRectangle *ink_rect,
                                                             PangoRectangle *logical_rect);

PANGO_AVAILABLE_IN_ALL
void                    pango_line_iter_get_trimmed_line_extents
                                                            (PangoLineIter    *iter,
                                                             PangoLeadingTrim  trim,
                                                             PangoRectangle   *logical_rect);

PANGO_AVAILABLE_IN_ALL
void                    pango_line_iter_get_run_extents     (PangoLineIter  *iter,
                                                             PangoRectangle *ink_rect,
                                                             PangoRectangle *logical_rect);

PANGO_AVAILABLE_IN_ALL
void                    pango_line_iter_get_cluster_extents (PangoLineIter  *iter,
                                                             PangoRectangle *ink_rect,
                                                             PangoRectangle *logical_rect);

PANGO_AVAILABLE_IN_ALL
void                    pango_line_iter_get_char_extents    (PangoLineIter  *iter,
                                                             PangoRectangle *logical_rect);

PANGO_AVAILABLE_IN_ALL
int                     pango_line_iter_get_line_baseline   (PangoLineIter  *iter);

PANGO_AVAILABLE_IN_ALL
int                     pango_line_iter_get_run_baseline    (PangoLineIter  *iter);


G_END_DECLS
