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
 * Pango2Direction:
 * @PANGO2_DIRECTION_LTR: A strong left-to-right direction
 * @PANGO2_DIRECTION_RTL: A strong right-to-left direction
 * @PANGO2_DIRECTION_WEAK_LTR: A weak left-to-right direction
 * @PANGO2_DIRECTION_WEAK_RTL: A weak right-to-left direction
 * @PANGO2_DIRECTION_NEUTRAL: No direction specified
 *
 * `Pango2Direction` represents a direction in the Unicode bidirectional
 * algorithm.
 *
 * Not every value in this enumeration makes sense for every usage of
 * `Pango2Direction`; for example, the direction of characters cannot be
 * `PANGO2_DIRECTION_WEAK_LTR` or `PANGO2_DIRECTION_WEAK_RTL`, since every
 * character is either neutral or has a strong direction; on the other hand
 * `PANGO2_DIRECTION_NEUTRAL` doesn't make sense to pass to [func@itemize].
 *
 * See `Pango2Gravity` for how vertical text is handled in Pango2.
 *
 * If you are interested in text direction, you should really use
 * [fribidi](http://fribidi.org/) directly. `Pango2Direction` is only
 * retained because it is used in some public apis.
 */
typedef enum {
  PANGO2_DIRECTION_LTR,
  PANGO2_DIRECTION_RTL,
  PANGO2_DIRECTION_WEAK_LTR,
  PANGO2_DIRECTION_WEAK_RTL,
  PANGO2_DIRECTION_NEUTRAL
} Pango2Direction;

G_END_DECLS
