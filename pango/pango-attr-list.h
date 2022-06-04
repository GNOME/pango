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

#include <pango/pango-attributes.h>
#include <glib-object.h>

G_BEGIN_DECLS


typedef struct _PangoAttrList     PangoAttrList;
typedef struct _PangoAttrIterator PangoAttrIterator;

#define PANGO_TYPE_ATTR_LIST pango_attr_list_get_type ()

/**
 * PangoAttrList:
 *
 * A `PangoAttrList` represents a list of attributes that apply to a section
 * of text.
 *
 * The attributes in a `PangoAttrList` are, in general, allowed to overlap in
 * an arbitrary fashion. However, if the attributes are manipulated only through
 * [method@Pango.AttrList.change], the overlap between properties will meet
 * stricter criteria.
 */

PANGO_AVAILABLE_IN_ALL
GType                   pango_attr_list_get_type        (void) G_GNUC_CONST;

PANGO_AVAILABLE_IN_ALL
PangoAttrList *         pango_attr_list_new             (void);
PANGO_AVAILABLE_IN_1_10
PangoAttrList *         pango_attr_list_ref             (PangoAttrList         *list);
PANGO_AVAILABLE_IN_ALL
void                    pango_attr_list_unref           (PangoAttrList         *list);
PANGO_AVAILABLE_IN_ALL
PangoAttrList *         pango_attr_list_copy            (PangoAttrList         *list);
PANGO_AVAILABLE_IN_ALL
void                    pango_attr_list_insert          (PangoAttrList         *list,
                                                         PangoAttribute        *attr);
PANGO_AVAILABLE_IN_ALL
void                    pango_attr_list_insert_before   (PangoAttrList         *list,
                                                         PangoAttribute        *attr);
PANGO_AVAILABLE_IN_ALL
void                    pango_attr_list_change          (PangoAttrList         *list,
                                                         PangoAttribute        *attr);
PANGO_AVAILABLE_IN_ALL
void                    pango_attr_list_splice          (PangoAttrList         *list,
                                                         PangoAttrList         *other,
                                                         int                    pos,
                                                         int                    len);
PANGO_AVAILABLE_IN_1_44
void                    pango_attr_list_update          (PangoAttrList         *list,
                                                         int                    pos,
                                                         int                    remove,
                                                         int                    add);

/**
 * PangoAttrFilterFunc:
 * @attribute: a Pango attribute
 * @user_data: user data passed to the function
 *
 * Callback to filter a list of attributes.
 *
 * Return value: `TRUE` if the attribute should be selected for
 *   filtering, `FALSE` otherwise.
 */
typedef gboolean (*PangoAttrFilterFunc) (PangoAttribute *attribute,
                                         gpointer        user_data);

PANGO_AVAILABLE_IN_1_2
PangoAttrList *         pango_attr_list_filter          (PangoAttrList         *list,
                                                         PangoAttrFilterFunc    func,
                                                         gpointer               data);

PANGO_AVAILABLE_IN_1_44
GSList *                pango_attr_list_get_attributes  (PangoAttrList         *list);

PANGO_AVAILABLE_IN_1_46
gboolean                pango_attr_list_equal           (PangoAttrList         *list,
                                                         PangoAttrList         *other_list);

PANGO_AVAILABLE_IN_1_50
char *                  pango_attr_list_to_string       (PangoAttrList         *list);
PANGO_AVAILABLE_IN_1_50
PangoAttrList *         pango_attr_list_from_string     (const char            *text);

PANGO_AVAILABLE_IN_ALL
PangoAttrIterator *     pango_attr_list_get_iterator    (PangoAttrList         *list);


G_DEFINE_AUTOPTR_CLEANUP_FUNC(PangoAttrList, pango_attr_list_unref)

G_END_DECLS
