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
#include <pango2/pango-run.h>

G_BEGIN_DECLS

PANGO2_AVAILABLE_IN_ALL
GType                   pango2_line_get_type               (void) G_GNUC_CONST;

PANGO2_AVAILABLE_IN_ALL
Pango2Line *            pango2_line_copy                   (Pango2Line        *line);

PANGO2_AVAILABLE_IN_ALL
void                    pango2_line_free                   (Pango2Line        *line);

PANGO2_AVAILABLE_IN_ALL
Pango2Line *            pango2_line_justify                (Pango2Line        *line,
                                                            int                width);

PANGO2_AVAILABLE_IN_ALL
int                     pango2_line_get_run_count          (Pango2Line        *line);

PANGO2_AVAILABLE_IN_ALL
Pango2Run **            pango2_line_get_runs               (Pango2Line        *line);

PANGO2_AVAILABLE_IN_ALL
const char *            pango2_line_get_text               (Pango2Line        *line,
                                                            int               *start_index,
                                                            int               *length);

PANGO2_AVAILABLE_IN_ALL
int                     pango2_line_get_start_index        (Pango2Line        *line);

PANGO2_AVAILABLE_IN_ALL
int                     pango2_line_get_length             (Pango2Line        *line);

PANGO2_AVAILABLE_IN_ALL
const Pango2LogAttr *   pango2_line_get_log_attrs          (Pango2Line        *line,
                                                            int               *start_offset,
                                                            int               *n_attrs);

PANGO2_AVAILABLE_IN_ALL
gboolean                pango2_line_is_wrapped             (Pango2Line        *line);

PANGO2_AVAILABLE_IN_ALL
gboolean                pango2_line_is_ellipsized          (Pango2Line        *line);

PANGO2_AVAILABLE_IN_ALL
gboolean                pango2_line_is_hyphenated          (Pango2Line        *line);

PANGO2_AVAILABLE_IN_ALL
gboolean                pango2_line_is_justified           (Pango2Line        *line);

PANGO2_AVAILABLE_IN_ALL
gboolean                pango2_line_is_paragraph_start     (Pango2Line        *line);

PANGO2_AVAILABLE_IN_ALL
gboolean                pango2_line_is_paragraph_end       (Pango2Line        *line);

PANGO2_AVAILABLE_IN_ALL
Pango2Direction         pango2_line_get_resolved_direction (Pango2Line        *line);

PANGO2_AVAILABLE_IN_ALL
void                    pango2_line_get_extents            (Pango2Line        *line,
                                                            Pango2Rectangle   *ink_rect,
                                                            Pango2Rectangle   *logical_rect);

PANGO2_AVAILABLE_IN_ALL
void                    pango2_line_get_trimmed_extents    (Pango2Line        *line,
                                                            Pango2LeadingTrim  trim,
                                                            Pango2Rectangle   *logical_rect);

PANGO2_AVAILABLE_IN_ALL
void                    pango2_line_index_to_pos           (Pango2Line        *line,
                                                            int                idx,
                                                            Pango2Rectangle   *pos);

PANGO2_AVAILABLE_IN_ALL
void                    pango2_line_index_to_x             (Pango2Line        *line,
                                                            int                idx,
                                                            int                trailing,
                                                            int               *x_pos);

PANGO2_AVAILABLE_IN_ALL
gboolean                pango2_line_x_to_index             (Pango2Line        *line,
                                                            int                x_pos,
                                                            int               *idx,
                                                            int               *trailing);

PANGO2_AVAILABLE_IN_ALL
void                    pango2_line_get_cursor_pos         (Pango2Line        *line,
                                                            int                idx,
                                                            Pango2Rectangle   *strong_pos,
                                                            Pango2Rectangle   *weak_pos);

PANGO2_AVAILABLE_IN_ALL
void                    pango2_line_get_caret_pos          (Pango2Line        *line,
                                                            int                idx,
                                                            Pango2Rectangle   *strong_pos,
                                                            Pango2Rectangle   *weak_pos);

G_DEFINE_AUTOPTR_CLEANUP_FUNC(Pango2Line, pango2_line_free)

G_END_DECLS
