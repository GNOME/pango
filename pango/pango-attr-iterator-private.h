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

#include <pango/pango-attr-iterator.h>


struct _PangoAttrIterator
{
  GPtrArray *attrs; /* From the list */
  guint n_attrs; /* Copied from the list */

  GPtrArray *attribute_stack;

  guint attr_index;
  guint start_index;
  guint end_index;
};

void     pango_attr_iterator_clear    (PangoAttrIterator *iterator);
gboolean pango_attr_iterator_advance  (PangoAttrIterator *iterator,
                                       int                index);
