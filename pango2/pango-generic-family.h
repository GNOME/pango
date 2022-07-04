/*
 * Copyright (C) 2022 Matthias Clasen
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

#include "pango-font.h"

G_BEGIN_DECLS

#define PANGO2_TYPE_GENERIC_FAMILY (pango2_generic_family_get_type ())

PANGO2_AVAILABLE_IN_ALL
PANGO2_DECLARE_INTERNAL_TYPE (Pango2GenericFamily, pango2_generic_family, PANGO2, GENERIC_FAMILY, Pango2FontFamily)

PANGO2_AVAILABLE_IN_ALL
Pango2GenericFamily *    pango2_generic_family_new                (const char              *name);

PANGO2_AVAILABLE_IN_ALL
void                     pango2_generic_family_add_family         (Pango2GenericFamily     *self,
                                                                   Pango2FontFamily        *family);

G_END_DECLS
