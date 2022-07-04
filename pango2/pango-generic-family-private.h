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
#include "pango-generic-family.h"
#include "pango-font-family-private.h"


struct _Pango2GenericFamily
{
  Pango2FontFamily parent_instance;

  GPtrArray *families;
};


Pango2FontFace *         pango2_generic_family_find_face          (Pango2GenericFamily     *self,
                                                                   Pango2FontDescription   *description,
                                                                   Pango2Language          *language,
                                                                   gunichar                 wc);
