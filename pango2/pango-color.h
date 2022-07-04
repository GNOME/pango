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

#include <pango2/pango-types.h>
#include <glib-object.h>

G_BEGIN_DECLS


typedef struct _Pango2Color Pango2Color;

/**
 * Pango2Color:
 * @red: value of the red component
 * @green: value of the green component
 * @blue: value of the blue component
 * @alpha: value of the alpha component
 *
 * The `Pango2Color` structure is used to
 * represent a color in an uncalibrated RGB color-space.
 */
struct _Pango2Color
{
  guint16 red;
  guint16 green;
  guint16 blue;
  guint16 alpha;
};

#define PANGO2_TYPE_COLOR (pango2_color_get_type ())

PANGO2_AVAILABLE_IN_ALL
GType        pango2_color_get_type         (void) G_GNUC_CONST;

PANGO2_AVAILABLE_IN_ALL
Pango2Color *pango2_color_copy             (const Pango2Color *src);

PANGO2_AVAILABLE_IN_ALL
void         pango2_color_free             (Pango2Color       *color);

PANGO2_AVAILABLE_IN_ALL
gboolean     pango2_color_equal            (const Pango2Color *color1,
                                            const Pango2Color *color2);
PANGO2_AVAILABLE_IN_ALL
gboolean     pango2_color_parse            (Pango2Color       *color,
                                            const char        *spec);

PANGO2_AVAILABLE_IN_ALL
char *       pango2_color_to_string        (const Pango2Color *color);

G_DEFINE_AUTOPTR_CLEANUP_FUNC(Pango2Color, pango2_color_free)

G_END_DECLS
