/*
 * Copyright (C) 2021 Red Hat, Inc
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

#include <pango2/pango.h>

G_BEGIN_DECLS

#define PANGO2_TYPE_CORE_TEXT_FONT_MAP      (pango2_core_text_font_map_get_type ())

PANGO2_AVAILABLE_IN_ALL
PANGO2_DECLARE_INTERNAL_TYPE (Pango2CoreTextFontMap, pango2_core_text_font_map, PANGO2, CORE_TEXT_FONT_MAP, Pango2FontMap)

PANGO2_AVAILABLE_IN_ALL
Pango2CoreTextFontMap * pango2_core_text_font_map_new (void);


G_END_DECLS
