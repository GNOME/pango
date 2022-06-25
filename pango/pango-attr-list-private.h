/*
 * Copyright 2022 RedHat, Inc.
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

#include <pango/pango-attr-list.h>


struct _Pango2AttrList
{
  guint ref_count;
  GPtrArray *attributes;
};

void     pango2_attr_list_init           (Pango2AttrList       *list);
void     pango2_attr_list_destroy        (Pango2AttrList       *list);
gboolean pango2_attr_list_has_attributes (const Pango2AttrList *list);

void     pango2_attr_list_init_iterator  (Pango2AttrList       *list,
                                          Pango2AttrIterator   *iterator);
