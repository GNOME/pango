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

#include <pango2/pango-attr-list.h>

G_BEGIN_DECLS


PANGO2_AVAILABLE_IN_ALL
GMarkupParseContext *   pango2_markup_parser_new        (gunichar               accel_marker);

PANGO2_AVAILABLE_IN_ALL
gboolean                pango2_markup_parser_finish     (GMarkupParseContext   *context,
                                                         Pango2AttrList       **attr_list,
                                                         char                 **text,
                                                         gunichar              *accel_char,
                                                         GError               **error);

PANGO2_AVAILABLE_IN_ALL
gboolean                pango2_parse_markup             (const char            *markup_text,
                                                         int                    length,
                                                         gunichar               accel_marker,
                                                         Pango2AttrList       **attr_list,
                                                         char                 **text,
                                                         gunichar              *accel_char,
                                                         GError               **error);


G_END_DECLS
