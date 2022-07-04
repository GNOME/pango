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
#include <pango2/pango-types.h>
#include <pango2/pango-item.h>
#include <pango2/pango-glyph.h>

PANGO2_AVAILABLE_IN_ALL
Pango2Item *            pango2_run_get_item     (Pango2Run         *run);

PANGO2_AVAILABLE_IN_ALL
Pango2GlyphString *     pango2_run_get_glyphs   (Pango2Run         *run);

PANGO2_AVAILABLE_IN_ALL
void                    pango2_run_get_extents  (Pango2Run         *run,
                                                 Pango2LeadingTrim  trim,
                                                 Pango2Rectangle   *ink_rect,
                                                 Pango2Rectangle   *logical_rect);
