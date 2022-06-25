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

G_BEGIN_DECLS

typedef struct _Pango2TabArray Pango2TabArray;

/**
 * Pango2TabAlign:
 * @PANGO2_TAB_LEFT: the text appears to the right of the tab stop position
 * @PANGO2_TAB_RIGHT: the text appears to the left of the tab stop position
 *   until the available space is filled.
 * @PANGO2_TAB_CENTER: the text is centered at the tab stop position
 *   until the available space is filled.
 * @PANGO2_TAB_DECIMAL: text before the first occurrence of the decimal point
 *   character appears to the left of the tab stop position (until the available
 *   space is filled), the rest to the right.
 *
 * `Pango2TabAlign` specifies where the text appears relative to the tab stop
 * position.
 */
typedef enum
{
  PANGO2_TAB_LEFT,
  PANGO2_TAB_RIGHT,
  PANGO2_TAB_CENTER,
  PANGO2_TAB_DECIMAL
} Pango2TabAlign;

#define PANGO2_TYPE_TAB_ARRAY (pango2_tab_array_get_type ())

PANGO2_AVAILABLE_IN_ALL
GType                   pango2_tab_array_get_type                (void) G_GNUC_CONST;

PANGO2_AVAILABLE_IN_ALL
Pango2TabArray *        pango2_tab_array_new                     (int              initial_size,
                                                                  gboolean         positions_in_pixels);
PANGO2_AVAILABLE_IN_ALL
Pango2TabArray *        pango2_tab_array_new_with_positions      (int              size,
                                                                  gboolean         positions_in_pixels,
                                                                  Pango2TabAlign   first_alignment,
                                                                  int              first_position,
                                                                  ...);
PANGO2_AVAILABLE_IN_ALL
Pango2TabArray *        pango2_tab_array_copy                    (Pango2TabArray  *src);
PANGO2_AVAILABLE_IN_ALL
void                    pango2_tab_array_free                    (Pango2TabArray  *tab_array);
PANGO2_AVAILABLE_IN_ALL
int                     pango2_tab_array_get_size                (Pango2TabArray  *tab_array);
PANGO2_AVAILABLE_IN_ALL
void                    pango2_tab_array_resize                  (Pango2TabArray  *tab_array,
                                                                  int              new_size);
PANGO2_AVAILABLE_IN_ALL
void                    pango2_tab_array_set_tab                 (Pango2TabArray  *tab_array,
                                                                  int              tab_index,
                                                                  Pango2TabAlign   alignment,
                                                                  int              location);
PANGO2_AVAILABLE_IN_ALL
void                    pango2_tab_array_get_tab                 (Pango2TabArray  *tab_array,
                                                                  int              tab_index,
                                                                  Pango2TabAlign  *alignment,
                                                                  int             *location);
PANGO2_AVAILABLE_IN_ALL
void                    pango2_tab_array_get_tabs                (Pango2TabArray  *tab_array,
                                                                  Pango2TabAlign **alignments,
                                                                  int            **locations);

PANGO2_AVAILABLE_IN_ALL
gboolean                pango2_tab_array_get_positions_in_pixels (Pango2TabArray  *tab_array);

PANGO2_AVAILABLE_IN_ALL
void                    pango2_tab_array_set_positions_in_pixels (Pango2TabArray  *tab_array,
                                                                  gboolean         positions_in_pixels);

PANGO2_AVAILABLE_IN_ALL
char *                  pango2_tab_array_to_string               (Pango2TabArray  *tab_array);
PANGO2_AVAILABLE_IN_ALL
Pango2TabArray *        pango2_tab_array_from_string             (const char      *text);

PANGO2_AVAILABLE_IN_ALL
void                    pango2_tab_array_set_decimal_point       (Pango2TabArray  *tab_array,
                                                                  int              tab_index,
                                                                  gunichar         decimal_point);
PANGO2_AVAILABLE_IN_ALL
gunichar                pango2_tab_array_get_decimal_point       (Pango2TabArray  *tab_array,
                                                                  int              tab_index);

PANGO2_AVAILABLE_IN_ALL
void                    pango2_tab_array_sort                    (Pango2TabArray  *tab_array);

G_DEFINE_AUTOPTR_CLEANUP_FUNC(Pango2TabArray, pango2_tab_array_free)

G_END_DECLS
