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
 * PangoAttrIterator:
 *
 * A `PangoAttrIterator` is used to iterate through a `PangoAttrList`.
 *
 * A new iterator is created with [method@Pango.AttrList.get_iterator].
 * Once the iterator is created, it can be advanced through the style
 * changes in the text using [method@Pango.AttrIterator.next]. At each
 * style change, the range of the current style segment and the attributes
 * currently in effect can be queried.
 */

PANGO_AVAILABLE_IN_1_44
GType                   pango_attr_iterator_get_type    (void) G_GNUC_CONST;

PANGO_AVAILABLE_IN_ALL
void                    pango_attr_iterator_range       (PangoAttrIterator     *iterator,
                                                         int                   *start,
                                                         int                   *end);
PANGO_AVAILABLE_IN_ALL
gboolean                pango_attr_iterator_next        (PangoAttrIterator     *iterator);
PANGO_AVAILABLE_IN_ALL
PangoAttrIterator *     pango_attr_iterator_copy        (PangoAttrIterator     *iterator);
PANGO_AVAILABLE_IN_ALL
void                    pango_attr_iterator_destroy     (PangoAttrIterator     *iterator);
PANGO_AVAILABLE_IN_ALL
PangoAttribute *        pango_attr_iterator_get         (PangoAttrIterator     *iterator,
                                                         guint                  type);
PANGO_AVAILABLE_IN_ALL
void                    pango_attr_iterator_get_font    (PangoAttrIterator     *iterator,
                                                         PangoFontDescription  *desc,
                                                         PangoLanguage        **language,
                                                         GSList               **extra_attrs);
PANGO_AVAILABLE_IN_1_2
GSList *                pango_attr_iterator_get_attrs   (PangoAttrIterator     *iterator);

G_DEFINE_AUTOPTR_CLEANUP_FUNC(PangoAttrIterator, pango_attr_iterator_destroy)

G_END_DECLS
