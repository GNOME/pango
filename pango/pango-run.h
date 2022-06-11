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
#include <pango/pango-types.h>
#include <pango/pango-item.h>
#include <pango/pango-glyph.h>

PANGO_AVAILABLE_IN_ALL
PangoItem *             pango_run_get_item     (PangoRun         *run);

PANGO_AVAILABLE_IN_ALL
PangoGlyphString *      pango_run_get_glyphs   (PangoRun         *run);

PANGO_AVAILABLE_IN_ALL
void                    pango_run_get_extents  (PangoRun         *run,
                                                PangoLeadingTrim  trim,
                                                PangoRectangle   *ink_rect,
                                                PangoRectangle   *logical_rect);
