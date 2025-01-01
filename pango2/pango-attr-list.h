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

#include <pango2/pango-attributes.h>
#include <glib-object.h>

G_BEGIN_DECLS


typedef struct _Pango2AttrList     Pango2AttrList;
typedef struct _Pango2AttrIterator Pango2AttrIterator;

#define PANGO2_TYPE_ATTR_LIST pango2_attr_list_get_type ()

/**
 * Pango2AttrList:
 *
 * A `Pango2AttrList` represents a list of attributes that apply to a section
 * of text.
 *
 * The attributes in a `Pango2AttrList` are, in general, allowed to overlap in
 * an arbitrary fashion. However, if the attributes are manipulated only through
 * [method@Pango2.AttrList.change], the overlap between properties will meet
 * stricter criteria.
 */

PANGO2_AVAILABLE_IN_ALL
GType                   pango2_attr_list_get_type        (void) G_GNUC_CONST;

PANGO2_AVAILABLE_IN_ALL
Pango2AttrList *         pango2_attr_list_new            (void);
PANGO2_AVAILABLE_IN_ALL
Pango2AttrList *         pango2_attr_list_ref            (Pango2AttrList         *list);
PANGO2_AVAILABLE_IN_ALL
void                    pango2_attr_list_unref           (Pango2AttrList         *list);
PANGO2_AVAILABLE_IN_ALL
Pango2AttrList *         pango2_attr_list_copy           (Pango2AttrList         *list);
PANGO2_AVAILABLE_IN_ALL
void                    pango2_attr_list_insert          (Pango2AttrList         *list,
                                                          Pango2Attribute        *attr);
PANGO2_AVAILABLE_IN_ALL
void                    pango2_attr_list_insert_before   (Pango2AttrList         *list,
                                                          Pango2Attribute        *attr);
PANGO2_AVAILABLE_IN_ALL
void                    pango2_attr_list_change          (Pango2AttrList         *list,
                                                          Pango2Attribute        *attr);
PANGO2_AVAILABLE_IN_ALL
void                    pango2_attr_list_splice          (Pango2AttrList         *list,
                                                          Pango2AttrList         *other,
                                                          int                     pos,
                                                          int                     len);
PANGO2_AVAILABLE_IN_ALL
void                    pango2_attr_list_update          (Pango2AttrList         *list,
                                                          int                     pos,
                                                          int                     remove,
                                                          int                     add);

/**
 * Pango2AttrFilterFunc:
 * @attribute: a Pango attribute
 * @user_data: (closure): user data passed to the function
 *
 * Callback to filter a list of attributes.
 *
 * Return value: `TRUE` if the attribute should be selected for
 *   filtering, `FALSE` otherwise.
 */
typedef gboolean (*Pango2AttrFilterFunc) (Pango2Attribute *attribute,
                                         gpointer        user_data);

PANGO2_AVAILABLE_IN_ALL
Pango2AttrList *         pango2_attr_list_filter         (Pango2AttrList         *list,
                                                          Pango2AttrFilterFunc    func,
                                                          gpointer                data);

PANGO2_AVAILABLE_IN_ALL
GSList *                pango2_attr_list_get_attributes  (Pango2AttrList         *list);

PANGO2_AVAILABLE_IN_ALL
gboolean                pango2_attr_list_equal           (Pango2AttrList         *list,
                                                          Pango2AttrList         *other_list);

PANGO2_AVAILABLE_IN_ALL
char *                  pango2_attr_list_to_string       (Pango2AttrList         *list);
PANGO2_AVAILABLE_IN_ALL
Pango2AttrList *         pango2_attr_list_from_string    (const char             *text);

PANGO2_AVAILABLE_IN_ALL
Pango2AttrIterator *     pango2_attr_list_get_iterator   (Pango2AttrList         *list);


G_DEFINE_AUTOPTR_CLEANUP_FUNC(Pango2AttrList, pango2_attr_list_unref)

G_END_DECLS
