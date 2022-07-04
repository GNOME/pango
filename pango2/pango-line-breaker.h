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
#include <pango2/pango-break.h>
#include <pango2/pango-layout.h>
#include <pango2/pango-line.h>

G_BEGIN_DECLS

#define PANGO2_TYPE_LINE_BREAKER pango2_line_breaker_get_type ()

PANGO2_AVAILABLE_IN_ALL
G_DECLARE_FINAL_TYPE (Pango2LineBreaker, pango2_line_breaker, PANGO2, LINE_BREAKER, GObject);

PANGO2_AVAILABLE_IN_ALL
Pango2LineBreaker *     pango2_line_breaker_new          (Pango2Context          *context);

PANGO2_AVAILABLE_IN_ALL
Pango2Context *         pango2_line_breaker_get_context  (Pango2LineBreaker      *self);

PANGO2_AVAILABLE_IN_ALL
void                    pango2_line_breaker_set_tabs     (Pango2LineBreaker      *self,
                                                          Pango2TabArray         *tabs);
PANGO2_AVAILABLE_IN_ALL
Pango2TabArray *        pango2_line_breaker_get_tabs     (Pango2LineBreaker      *self);

PANGO2_AVAILABLE_IN_ALL
void                    pango2_line_breaker_set_base_dir (Pango2LineBreaker      *self,
                                                          Pango2Direction         direction);
PANGO2_AVAILABLE_IN_ALL
Pango2Direction         pango2_line_breaker_get_base_dir (Pango2LineBreaker      *self);

PANGO2_AVAILABLE_IN_ALL
void                    pango2_line_breaker_add_text     (Pango2LineBreaker      *self,
                                                          const char             *text,
                                                          int                     length,
                                                          Pango2AttrList         *attrs);

PANGO2_AVAILABLE_IN_ALL
Pango2Direction         pango2_line_breaker_get_direction (Pango2LineBreaker     *self);

PANGO2_AVAILABLE_IN_ALL
gboolean                pango2_line_breaker_has_line     (Pango2LineBreaker      *self);

PANGO2_AVAILABLE_IN_ALL
Pango2Line *            pango2_line_breaker_next_line    (Pango2LineBreaker      *self,
                                                          int                     x,
                                                          int                     width,
                                                          Pango2WrapMode          wrap,
                                                          Pango2EllipsizeMode     ellipsize);

PANGO2_AVAILABLE_IN_ALL
gboolean                pango2_line_breaker_undo_line    (Pango2LineBreaker      *self,
                                                          Pango2Line             *line);

G_END_DECLS
