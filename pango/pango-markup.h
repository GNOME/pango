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

#include <pango/pango-attr-list.h>

G_BEGIN_DECLS


PANGO_AVAILABLE_IN_ALL
GMarkupParseContext * pango_markup_parser_new    (gunichar               accel_marker);

PANGO_AVAILABLE_IN_ALL
gboolean              pango_markup_parser_finish (GMarkupParseContext   *context,
                                                  PangoAttrList        **attr_list,
                                                  char                 **text,
                                                  gunichar              *accel_char,
                                                  GError               **error);

PANGO_AVAILABLE_IN_ALL
gboolean               pango_parse_markup        (const char            *markup_text,
                                                  int                    length,
                                                  gunichar               accel_marker,
                                                  PangoAttrList        **attr_list,
                                                  char                 **text,
                                                  gunichar              *accel_char,
                                                  GError               **error);


G_END_DECLS
