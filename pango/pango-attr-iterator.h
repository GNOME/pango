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

#include <pango/pango-attr-list.h>
#include <glib-object.h>

G_BEGIN_DECLS


/**
 * Pango2AttrIterator:
 *
 * A `Pango2AttrIterator` is used to iterate through a `Pango2AttrList`.
 *
 * A new iterator is created with [method@Pango2.AttrList.get_iterator].
 * Once the iterator is created, it can be advanced through the style
 * changes in the text using [method@Pango2.AttrIterator.next]. At each
 * style change, the range of the current style segment and the attributes
 * currently in effect can be queried.
 */

PANGO2_AVAILABLE_IN_ALL
GType                   pango2_attr_iterator_get_type    (void) G_GNUC_CONST;

PANGO2_AVAILABLE_IN_ALL
void                    pango2_attr_iterator_range       (Pango2AttrIterator     *iterator,
                                                          int                    *start,
                                                          int                    *end);
PANGO2_AVAILABLE_IN_ALL
gboolean                pango2_attr_iterator_next        (Pango2AttrIterator     *iterator);
PANGO2_AVAILABLE_IN_ALL
Pango2AttrIterator *    pango2_attr_iterator_copy        (Pango2AttrIterator     *iterator);
PANGO2_AVAILABLE_IN_ALL
void                    pango2_attr_iterator_destroy     (Pango2AttrIterator     *iterator);
PANGO2_AVAILABLE_IN_ALL
Pango2Attribute *       pango2_attr_iterator_get         (Pango2AttrIterator     *iterator,
                                                          guint                   type);
PANGO2_AVAILABLE_IN_ALL
void                    pango2_attr_iterator_get_font    (Pango2AttrIterator     *iterator,
                                                          Pango2FontDescription  *desc,
                                                          Pango2Language        **language,
                                                          GSList                **extra_attrs);
PANGO2_AVAILABLE_IN_ALL
GSList *                pango2_attr_iterator_get_attrs  (Pango2AttrIterator      *iterator);

G_DEFINE_AUTOPTR_CLEANUP_FUNC(Pango2AttrIterator, pango2_attr_iterator_destroy)

G_END_DECLS
