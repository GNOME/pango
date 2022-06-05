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

#include <pango/pango-types.h>
#include <glib-object.h>

G_BEGIN_DECLS


typedef struct _PangoColor PangoColor;

/**
 * PangoColor:
 * @red: value of red component
 * @green: value of green component
 * @blue: value of blue component
 *
 * The `PangoColor` structure is used to
 * represent a color in an uncalibrated RGB color-space.
 */
struct _PangoColor
{
  guint16 red;
  guint16 green;
  guint16 blue;
};

#define PANGO_TYPE_COLOR (pango_color_get_type ())

PANGO_AVAILABLE_IN_ALL
GType       pango_color_get_type         (void) G_GNUC_CONST;

PANGO_AVAILABLE_IN_ALL
PangoColor *pango_color_copy             (const PangoColor *src);

PANGO_AVAILABLE_IN_ALL
void        pango_color_free             (PangoColor       *color);

PANGO_AVAILABLE_IN_ALL
gboolean    pango_color_parse            (PangoColor       *color,
                                          const char       *spec);

PANGO_AVAILABLE_IN_ALL
gboolean    pango_color_parse_with_alpha (PangoColor       *color,
                                          guint16          *alpha,
                                          const char       *spec);

PANGO_AVAILABLE_IN_ALL
char       *pango_color_to_string        (const PangoColor *color);


G_END_DECLS
