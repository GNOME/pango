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

#include <pango/pango-attr.h>

struct _PangoAttribute
{
  guint type;
  guint start_index;
  guint end_index;
  union {
    char *str_value;
    int int_value;
    gboolean boolean_value;
    double double_value;
    PangoColor color_value;
    PangoLanguage *lang_value;
    PangoFontDescription *font_value;
    gpointer pointer_value;
  };
};

char *    pango_attr_value_serialize   (PangoAttribute    *attr);
