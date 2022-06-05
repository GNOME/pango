/*
 * Copyright (C) 2018 Matthias Clasen
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

#include <glib.h>

G_BEGIN_DECLS

/**
 * PangoDirection:
 * @PANGO_DIRECTION_LTR: A strong left-to-right direction
 * @PANGO_DIRECTION_RTL: A strong right-to-left direction
 * @PANGO_DIRECTION_WEAK_LTR: A weak left-to-right direction
 * @PANGO_DIRECTION_WEAK_RTL: A weak right-to-left direction
 * @PANGO_DIRECTION_NEUTRAL: No direction specified
 *
 * `PangoDirection` represents a direction in the Unicode bidirectional
 * algorithm.
 *
 * Not every value in this enumeration makes sense for every usage of
 * `PangoDirection`; for example, the return value of [func@unichar_direction]
 * and [func@find_base_dir] cannot be `PANGO_DIRECTION_WEAK_LTR` or
 * `PANGO_DIRECTION_WEAK_RTL`, since every character is either neutral
 * or has a strong direction; on the other hand `PANGO_DIRECTION_NEUTRAL`
 * doesn't make sense to pass to [func@itemize].
 *
 * See `PangoGravity` for how vertical text is handled in Pango.
 *
 * If you are interested in text direction, you should really use fribidi
 * directly. `PangoDirection` is only retained because it is used in some
 * public apis.
 */
typedef enum {
  PANGO_DIRECTION_LTR,
  PANGO_DIRECTION_RTL,
  PANGO_DIRECTION_WEAK_LTR,
  PANGO_DIRECTION_WEAK_RTL,
  PANGO_DIRECTION_NEUTRAL
} PangoDirection;

G_END_DECLS
