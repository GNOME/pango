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
#include <pango2/pango-line.h>

G_BEGIN_DECLS

#define PANGO2_TYPE_LINES pango2_lines_get_type ()

PANGO2_AVAILABLE_IN_ALL
G_DECLARE_FINAL_TYPE (Pango2Lines, pango2_lines, PANGO2, LINES, GObject);

PANGO2_AVAILABLE_IN_ALL
Pango2Lines *           pango2_lines_new             (void);

PANGO2_AVAILABLE_IN_ALL
guint                   pango2_lines_get_serial      (Pango2Lines        *lines);

PANGO2_AVAILABLE_IN_ALL
void                    pango2_lines_add_line        (Pango2Lines        *lines,
                                                      Pango2Line         *line,
                                                      int                 line_x,
                                                      int                 line_y);

PANGO2_AVAILABLE_IN_ALL
int                     pango2_lines_get_line_count  (Pango2Lines        *lines);

PANGO2_AVAILABLE_IN_ALL
Pango2Line **           pango2_lines_get_lines       (Pango2Lines        *lines);

PANGO2_AVAILABLE_IN_ALL
void                    pango2_lines_get_line_position
                                                     (Pango2Lines        *lines,
                                                      int                 num,
                                                      int                *line_x,
                                                      int                *line_y);

PANGO2_AVAILABLE_IN_ALL
Pango2LineIter *        pango2_lines_get_iter        (Pango2Lines        *lines);

PANGO2_AVAILABLE_IN_ALL
void                    pango2_lines_get_extents     (Pango2Lines        *lines,
                                                      Pango2Rectangle    *ink_rect,
                                                      Pango2Rectangle    *logical_rect);

PANGO2_AVAILABLE_IN_ALL
void                    pango2_lines_get_trimmed_extents
                                                     (Pango2Lines        *lines,
                                                      Pango2LeadingTrim   trim,
                                                      Pango2Rectangle    *ink_rect,
                                                      Pango2Rectangle    *logical_rect);

PANGO2_AVAILABLE_IN_ALL
void                    pango2_lines_get_size        (Pango2Lines        *lines,
                                                      int                *width,
                                                      int                *height);

PANGO2_AVAILABLE_IN_ALL
int                     pango2_lines_get_baseline    (Pango2Lines        *lines);

PANGO2_AVAILABLE_IN_ALL
void                    pango2_lines_get_x_ranges    (Pango2Lines        *lines,
                                                      Pango2Line         *line,
                                                      Pango2Line         *start_line,
                                                      int                 start_index,
                                                      Pango2Line         *end_line,
                                                      int                 end_index,
                                                      int               **ranges,
                                                      int                *n_ranges);

PANGO2_AVAILABLE_IN_ALL
int                     pango2_lines_get_unknown_glyphs_count
                                                    (Pango2Lines        *lines);

PANGO2_AVAILABLE_IN_ALL
gboolean                pango2_lines_is_wrapped      (Pango2Lines        *lines);

PANGO2_AVAILABLE_IN_ALL
gboolean                pango2_lines_is_ellipsized   (Pango2Lines        *lines);

PANGO2_AVAILABLE_IN_ALL
gboolean                pango2_lines_is_hyphenated   (Pango2Lines        *lines);

PANGO2_AVAILABLE_IN_ALL
void                    pango2_lines_index_to_line   (Pango2Lines        *lines,
                                                      int                 idx,
                                                      Pango2Line        **line,
                                                      int                *line_no,
                                                      int                *x_offset,
                                                      int                *y_offset);

PANGO2_AVAILABLE_IN_ALL
Pango2Line *            pango2_lines_pos_to_line     (Pango2Lines        *lines,
                                                      int                 x,
                                                      int                 y,
                                                      int                *line_x,
                                                      int                *line_y);

PANGO2_AVAILABLE_IN_ALL
void                    pango2_lines_index_to_pos    (Pango2Lines        *lines,
                                                      Pango2Line         *line,
                                                      int                 idx,
                                                      Pango2Rectangle    *pos);

PANGO2_AVAILABLE_IN_ALL
Pango2Line *            pango2_lines_pos_to_index    (Pango2Lines        *lines,
                                                      int                 x,
                                                      int                 y,
                                                      int                *idx,
                                                      int                *trailing);

PANGO2_AVAILABLE_IN_ALL
void                    pango2_lines_get_cursor_pos  (Pango2Lines        *lines,
                                                      Pango2Line         *line,
                                                      int                 idx,
                                                      Pango2Rectangle    *strong_pos,
                                                      Pango2Rectangle    *weak_pos);

PANGO2_AVAILABLE_IN_ALL
void                    pango2_lines_get_caret_pos   (Pango2Lines        *lines,
                                                      Pango2Line         *line,
                                                      int                 idx,
                                                      Pango2Rectangle    *strong_pos,
                                                      Pango2Rectangle    *weak_pos);

PANGO2_AVAILABLE_IN_ALL
void                    pango2_lines_move_cursor     (Pango2Lines        *lines,
                                                      gboolean            strong,
                                                      Pango2Line         *line,
                                                      int                 idx,
                                                      int                 trailing,
                                                      int                 direction,
                                                      Pango2Line        **new_line,
                                                      int                *new_idx,
                                                      int                *new_trailing);

PANGO2_AVAILABLE_IN_ALL
GBytes *                pango2_lines_serialize       (Pango2Lines        *lines);

G_END_DECLS
