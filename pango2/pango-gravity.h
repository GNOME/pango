/*
 * Copyright (C) 2006, 2007 Red Hat Software
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
 * Pango2Gravity:
 * @PANGO2_GRAVITY_SOUTH: Glyphs stand upright (default) <img align="right" valign="center" src="m-south.png">
 * @PANGO2_GRAVITY_EAST: Glyphs are rotated 90 degrees counter-clockwise. <img align="right" valign="center" src="m-east.png">
 * @PANGO2_GRAVITY_NORTH: Glyphs are upside-down. <img align="right" valign="cener" src="m-north.png">
 * @PANGO2_GRAVITY_WEST: Glyphs are rotated 90 degrees clockwise. <img align="right" valign="center" src="m-west.png">
 * @PANGO2_GRAVITY_AUTO: Gravity is resolved from the context matrix
 *
 * `Pango2Gravity` represents the orientation of glyphs in a segment
 * of text.
 *
 * This is useful when rendering vertical text layouts. In those situations,
 * the layout is rotated using a non-identity [struct@Pango2.Matrix], and then
 * glyph orientation is controlled using `Pango2Gravity`.
 *
 * Not every value in this enumeration makes sense for every usage of
 * `Pango2Gravity`; for example, %PANGO2_GRAVITY_AUTO only can be passed to
 * [method@Pango2.Context.set_base_gravity] and can only be returned by
 * [method@Pango2.Context.get_base_gravity].
 *
 * See also: [enum@Pango2.GravityHint]
 */
typedef enum {
  PANGO2_GRAVITY_SOUTH,
  PANGO2_GRAVITY_EAST,
  PANGO2_GRAVITY_NORTH,
  PANGO2_GRAVITY_WEST,
  PANGO2_GRAVITY_AUTO
} Pango2Gravity;

/**
 * Pango2GravityHint:
 * @PANGO2_GRAVITY_HINT_NATURAL: scripts will take their natural gravity based
 *   on the base gravity and the script.  This is the default.
 * @PANGO2_GRAVITY_HINT_STRONG: always use the base gravity set, regardless of
 *   the script.
 * @PANGO2_GRAVITY_HINT_LINE: for scripts not in their natural direction (eg.
 *   Latin in East gravity), choose per-script gravity such that every script
 *   respects the line progression. This means, Latin and Arabic will take
 *   opposite gravities and both flow top-to-bottom for example.
 *
 * `Pango2GravityHint` defines how horizontal scripts should behave in a
 * vertical context.
 *
 * That is, English excerpts in a vertical paragraph for example.
 *
 * See also [enum@Pango2.Gravity]
 */
typedef enum {
  PANGO2_GRAVITY_HINT_NATURAL,
  PANGO2_GRAVITY_HINT_STRONG,
  PANGO2_GRAVITY_HINT_LINE
} Pango2GravityHint;

/**
 * PANGO2_GRAVITY_IS_VERTICAL:
 * @gravity: the `Pango2Gravity` to check
 *
 * Whether a `Pango2Gravity` represents vertical writing directions.
 *
 * Returns: %TRUE if @gravity is %PANGO2_GRAVITY_EAST or %PANGO2_GRAVITY_WEST,
 *   %FALSE otherwise.
 */
#define PANGO2_GRAVITY_IS_VERTICAL(gravity) \
        ((gravity) == PANGO2_GRAVITY_EAST || (gravity) == PANGO2_GRAVITY_WEST)

/**
 * PANGO2_GRAVITY_IS_IMPROPER:
 * @gravity: the `Pango2Gravity` to check
 *
 * Whether a `Pango2Gravity` represents a gravity that results in reversal
 * of text direction.
 *
 * Returns: %TRUE if @gravity is %PANGO2_GRAVITY_WEST or %PANGO2_GRAVITY_NORTH,
 *   %FALSE otherwise.
 */
#define PANGO2_GRAVITY_IS_IMPROPER(gravity) \
        ((gravity) == PANGO2_GRAVITY_WEST || (gravity) == PANGO2_GRAVITY_NORTH)

#include <pango2/pango-matrix.h>
#include <pango2/pango-script.h>

PANGO2_AVAILABLE_IN_ALL
double                  pango2_gravity_to_rotation    (Pango2Gravity       gravity) G_GNUC_CONST;
PANGO2_AVAILABLE_IN_ALL
Pango2Gravity           pango2_gravity_get_for_matrix (const Pango2Matrix *matrix) G_GNUC_PURE;
PANGO2_AVAILABLE_IN_ALL
Pango2Gravity           pango2_gravity_get_for_script (GUnicodeScript      script,
                                                       Pango2Gravity       base_gravity,
                                                       Pango2GravityHint   hint) G_GNUC_CONST;
PANGO2_AVAILABLE_IN_ALL
Pango2Gravity           pango2_gravity_get_for_script_and_width
                                                      (GUnicodeScript      script,
                                                       gboolean            wide,
                                                       Pango2Gravity       base_gravity,
                                                       Pango2GravityHint   hint) G_GNUC_CONST;


G_END_DECLS
