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
#include <pango/pango-run.h>

G_BEGIN_DECLS

PANGO_AVAILABLE_IN_ALL
GType                pango_line_get_type               (void) G_GNUC_CONST;

PANGO_AVAILABLE_IN_ALL
PangoLine *          pango_line_copy                   (PangoLine        *line);

PANGO_AVAILABLE_IN_ALL
void                 pango_line_free                   (PangoLine        *line);

PANGO_AVAILABLE_IN_ALL
PangoLine *          pango_line_justify                (PangoLine        *line,
                                                        int               width);

PANGO_AVAILABLE_IN_ALL
GSList *             pango_line_get_runs               (PangoLine        *line);

PANGO_AVAILABLE_IN_ALL
const char *         pango_line_get_text               (PangoLine        *line,
                                                        int              *start_index,
                                                        int              *length);

PANGO_AVAILABLE_IN_ALL
int                  pango_line_get_start_index        (PangoLine        *line);

PANGO_AVAILABLE_IN_ALL
int                  pango_line_get_length             (PangoLine        *line);

PANGO_AVAILABLE_IN_ALL
const PangoLogAttr * pango_line_get_log_attrs          (PangoLine        *line,
                                                        int              *start_offset,
                                                        int              *n_attrs);

PANGO_AVAILABLE_IN_ALL
gboolean             pango_line_is_wrapped             (PangoLine        *line);

PANGO_AVAILABLE_IN_ALL
gboolean             pango_line_is_ellipsized          (PangoLine        *line);

PANGO_AVAILABLE_IN_ALL
gboolean             pango_line_is_hyphenated          (PangoLine        *line);

PANGO_AVAILABLE_IN_ALL
gboolean             pango_line_is_justified           (PangoLine        *line);

PANGO_AVAILABLE_IN_ALL
gboolean             pango_line_is_paragraph_start     (PangoLine        *line);

PANGO_AVAILABLE_IN_ALL
gboolean             pango_line_is_paragraph_end       (PangoLine        *line);

PANGO_AVAILABLE_IN_ALL
PangoDirection       pango_line_get_resolved_direction (PangoLine        *line);

PANGO_AVAILABLE_IN_ALL
void                 pango_line_get_extents            (PangoLine        *line,
                                                        PangoRectangle   *ink_rect,
                                                        PangoRectangle   *logical_rect);

PANGO_AVAILABLE_IN_ALL
void                 pango_line_get_trimmed_extents    (PangoLine        *line,
                                                        PangoLeadingTrim  trim,
                                                        PangoRectangle   *logical_rect);

PANGO_AVAILABLE_IN_ALL
void                 pango_line_index_to_pos           (PangoLine        *line,
                                                        int               idx,
                                                        PangoRectangle   *pos);

PANGO_AVAILABLE_IN_ALL
void                 pango_line_index_to_x             (PangoLine        *line,
                                                        int               idx,
                                                        int               trailing,
                                                        int              *x_pos);

PANGO_AVAILABLE_IN_ALL
gboolean             pango_line_x_to_index             (PangoLine        *line,
                                                        int               x,
                                                        int              *idx,
                                                        int              *trailing);

PANGO_AVAILABLE_IN_ALL
void                 pango_line_get_cursor_pos         (PangoLine        *line,
                                                        int               idx,
                                                        PangoRectangle   *strong_pos,
                                                        PangoRectangle   *weak_pos);

PANGO_AVAILABLE_IN_ALL
void                 pango_line_get_caret_pos          (PangoLine        *line,
                                                        int               idx,
                                                        PangoRectangle   *strong_pos,
                                                        PangoRectangle   *weak_pos);

G_END_DECLS
