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
#include <pango/pango-break.h>
#include <pango/pango-layout.h>
#include <pango/pango-line.h>

G_BEGIN_DECLS

#define PANGO_TYPE_LINE_BREAKER pango_line_breaker_get_type ()

PANGO_AVAILABLE_IN_ALL
G_DECLARE_FINAL_TYPE (PangoLineBreaker, pango_line_breaker, PANGO, LINE_BREAKER, GObject);

PANGO_AVAILABLE_IN_ALL
PangoLineBreaker *      pango_line_breaker_new          (PangoContext          *context);

PANGO_AVAILABLE_IN_ALL
PangoContext *          pango_line_breaker_get_context  (PangoLineBreaker      *self);

PANGO_AVAILABLE_IN_ALL
void                    pango_line_breaker_set_tabs     (PangoLineBreaker      *self,
                                                         PangoTabArray         *tabs);
PANGO_AVAILABLE_IN_ALL
PangoTabArray *         pango_line_breaker_get_tabs     (PangoLineBreaker      *self);

PANGO_AVAILABLE_IN_ALL
void                    pango_line_breaker_set_base_dir (PangoLineBreaker      *self,
                                                         PangoDirection         direction);
PANGO_AVAILABLE_IN_ALL
PangoDirection          pango_line_breaker_get_base_dir (PangoLineBreaker      *self);

PANGO_AVAILABLE_IN_ALL
void                    pango_line_breaker_add_text     (PangoLineBreaker      *self,
                                                         const char            *text,
                                                         int                    length,
                                                         PangoAttrList         *attrs);

PANGO_AVAILABLE_IN_ALL
PangoDirection          pango_line_breaker_get_direction (PangoLineBreaker      *self);

PANGO_AVAILABLE_IN_ALL
gboolean                pango_line_breaker_has_line     (PangoLineBreaker      *self);

PANGO_AVAILABLE_IN_ALL
PangoLine *             pango_line_breaker_next_line    (PangoLineBreaker      *self,
                                                         int                    x,
                                                         int                    width,
                                                         PangoWrapMode          wrap,
                                                         PangoEllipsizeMode     ellipsize);

PANGO_AVAILABLE_IN_ALL
gboolean                pango_line_breaker_undo_line    (PangoLineBreaker      *self,
                                                         PangoLine             *line);

G_END_DECLS
