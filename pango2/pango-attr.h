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

#include <pango2/pango-font.h>
#include <pango2/pango-color.h>
#include <glib-object.h>

G_BEGIN_DECLS


typedef struct _Pango2Attribute Pango2Attribute;

/**
 * Pango2AttrValueType:
 * @PANGO2_ATTR_VALUE_STRING: A string
 * @PANGO2_ATTR_VALUE_INT: An integer
 * @PANGO2_ATTR_VALUE_BOOLEAN: A boolean
 * @PANGO2_ATTR_VALUE_FLOAT: A floating point number
 * @PANGO2_ATTR_VALUE_COLOR: A `Pango2Color`
 * @PANGO2_ATTR_VALUE_LANGUAGE: A `Pango2Language`
 * @PANGO2_ATTR_VALUE_FONT_DESC: A `Pango2FontDescription`
 * @PANGO2_ATTR_VALUE_POINTER: A generic pointer
 *
 * `Pango2AttrValueType` enumerates the types of values
 * that a `Pango2Attribute` can contain.
 *
 * The `Pango2AttrValueType` of a `Pango2Attribute` is part
 * of its type, and can be obtained with `PANGO2_ATTR_VALUE_TYPE()`.
 */
typedef enum
{
  PANGO2_ATTR_VALUE_STRING,
  PANGO2_ATTR_VALUE_INT,
  PANGO2_ATTR_VALUE_BOOLEAN,
  PANGO2_ATTR_VALUE_FLOAT,
  PANGO2_ATTR_VALUE_COLOR,
  PANGO2_ATTR_VALUE_LANGUAGE,
  PANGO2_ATTR_VALUE_FONT_DESC,
  PANGO2_ATTR_VALUE_POINTER
} Pango2AttrValueType;

/**
 * Pango2AttrAffects:
 * @PANGO2_ATTR_AFFECTS_NONE: The attribute does not affect rendering
 * @PANGO2_ATTR_AFFECTS_ITEMIZATION: The attribute affecs itemization
 * @PANGO2_ATTR_AFFECTS_BREAKING: The attribute affects `Pango2LogAttr` determination
 * @PANGO2_ATTR_AFFECTS_SHAPING: The attribute affects shaping
 * @PANGO2_ATTR_AFFECTS_RENDERING: The attribute affects rendering
 *
 * A `Pango2AttrAffects` value indicates what part of Pango2's processing
 * pipeline is affected by an attribute.
 *
 * Marking an attribute with `PANGO2_ATTR_AFFECTS_ITEMIZATION` ensures
 * that the attribute values are constant across items.
 */
typedef enum
{
  PANGO2_ATTR_AFFECTS_NONE        = 0,
  PANGO2_ATTR_AFFECTS_ITEMIZATION = 1 << 0,
  PANGO2_ATTR_AFFECTS_BREAKING    = 1 << 1,
  PANGO2_ATTR_AFFECTS_SHAPING     = 1 << 2,
  PANGO2_ATTR_AFFECTS_RENDERING   = 1 << 3
} Pango2AttrAffects;

/**
 * Pango2AttrMerge:
 * @PANGO2_ATTR_MERGE_OVERRIDES: Only the attribute with the narrowest range is used
 * @PANGO2_ATTR_MERGE_ACCUMULATES: All attributes with overlapping range are kept
 *
 * A `Pango2AttrMerge` value indicates how overlapping attribute values
 * should be reconciled to determine the effective attribute value.
 *
 * These options influence the @extra_attrs returned by
 * [method@Pango2.AttrIterator.get_font].
 */
typedef enum
{
  PANGO2_ATTR_MERGE_OVERRIDES,
  PANGO2_ATTR_MERGE_ACCUMULATES
} Pango2AttrMerge;

/**
 * PANGO2_ATTR_TYPE_VALUE_TYPE:
 * @type: an attribute type
 *
 * Extracts the `Pango2AttrValueType` from an attribute type.
 */
#define PANGO2_ATTR_TYPE_VALUE_TYPE(type) ((Pango2AttrValueType)((type) & 0xff))

/**
 * PANGO2_ATTR_TYPE_AFFECTS:
 * @type: an attribute type
 *
 * Extracts the `Pango2AttrAffects` flags from an attribute type.
 */
#define PANGO2_ATTR_TYPE_AFFECTS(type) ((Pango2AttrAffects)(((type) >> 8) & 0xf))

/**
 * PANGO2_ATTR_TYPE_MERGE:
 * @type: an attribute type
 *
 * Extracts the `Pango2AttrMerge` flags from an attribute type.
 */
#define PANGO2_ATTR_TYPE_MERGE(type) ((Pango2AttrMerge)(((type) >> 12) & 0xf))

/**
 * PANGO2_ATTR_VALUE_TYPE:
 * @attr: a `Pango2Attribute`
 *
 * Obtains the `Pango2AttrValueType of a `Pango2Attribute`.
 */
#define PANGO2_ATTR_VALUE_TYPE(attr) PANGO2_ATTR_TYPE_VALUE_TYPE (pango2_attribute_type (attr))

/**
 * PANGO2_ATTR_AFFECTS:
 * @attr: a `Pango2Attribute`
 *
 * Obtains the `Pango2AttrAffects` flags of a `Pango2Attribute`.
 */
#define PANGO2_ATTR_AFFECTS(attr) PANGO2_ATTR_TYPE_AFFECTS (pango2_attribute_type (attr))

/**
 * PANGO2_ATTR_MERGE:
 * @attr: a `Pango2Attribute`
 *
 * Obtains the `Pango2AttrMerge` flags of a `Pango2Attribute`.
 */
#define PANGO2_ATTR_MERGE(attr) PANGO2_ATTR_TYPE_MERGE (pango2_attribute_type (attr))

/**
 * PANGO2_ATTR_INDEX_FROM_TEXT_BEGINNING:
 *
 * Value for @start_index in `Pango2Attribute` that indicates
 * the beginning of the text.
 */
#define PANGO2_ATTR_INDEX_FROM_TEXT_BEGINNING ((guint)0)

/**
 * PANGO2_ATTR_INDEX_TO_TEXT_END: (value 4294967295)
 *
 * Value for @end_index in `Pango2Attribute` that indicates
 * the end of the text.
 */
#define PANGO2_ATTR_INDEX_TO_TEXT_END ((guint)(G_MAXUINT + 0))

/**
 * Pango2AttrDataCopyFunc:
 * @value: value to copy
 *
 * Callback to duplicate the value of an attribute.
 *
 * Return value: new copy of @value.
 **/
typedef gpointer (*Pango2AttrDataCopyFunc) (gconstpointer value);

/**
 * Pango2AttrDataSerializeFunc:
 * @value: value to serialize
 *
 * Callback to serialize the value of an attribute.
 *
 * Return value: a newly allocated string holding the serialization of @value
 */
typedef char * (*Pango2AttrDataSerializeFunc) (gconstpointer value);

PANGO2_AVAILABLE_IN_ALL
GType                   pango2_attribute_get_type               (void) G_GNUC_CONST;

PANGO2_AVAILABLE_IN_ALL
guint                   pango2_attr_type_register               (const char                  *name,
                                                                 Pango2AttrValueType          value_type,
                                                                 Pango2AttrAffects            affects,
                                                                 Pango2AttrMerge              merge,
                                                                 Pango2AttrDataCopyFunc       copy,
                                                                 GDestroyNotify               destroy,
                                                                 GEqualFunc                   equal,
                                                                 Pango2AttrDataSerializeFunc  serialize);

PANGO2_AVAILABLE_IN_ALL
const char *            pango2_attr_type_get_name               (guint                        type) G_GNUC_CONST;
PANGO2_AVAILABLE_IN_ALL
Pango2Attribute *       pango2_attribute_copy                   (const Pango2Attribute       *attr);
PANGO2_AVAILABLE_IN_ALL
void                    pango2_attribute_destroy                (Pango2Attribute             *attr);
PANGO2_AVAILABLE_IN_ALL
gboolean                pango2_attribute_equal                  (const Pango2Attribute       *attr1,
                                                                 const Pango2Attribute       *attr2) G_GNUC_PURE;

PANGO2_AVAILABLE_IN_ALL
Pango2Attribute *       pango2_attribute_new                    (guint                        type,
                                                                 gconstpointer                value);

PANGO2_AVAILABLE_IN_ALL
guint                   pango2_attribute_type                   (const Pango2Attribute        *attribute);

PANGO2_AVAILABLE_IN_ALL
void                    pango2_attribute_set_range              (Pango2Attribute              *attribute,
                                                                 guint                         start_index,
                                                                 guint                         end_index);

PANGO2_AVAILABLE_IN_ALL
void                    pango2_attribute_get_range              (Pango2Attribute              *attribute,
                                                                 guint                        *start_index,
                                                                 guint                        *end_index);

PANGO2_AVAILABLE_IN_ALL
const char *            pango2_attribute_get_string             (Pango2Attribute              *attribute);
PANGO2_AVAILABLE_IN_ALL
Pango2Language *        pango2_attribute_get_language           (Pango2Attribute              *attribute);
PANGO2_AVAILABLE_IN_ALL
int                     pango2_attribute_get_int                (Pango2Attribute              *attribute);
PANGO2_AVAILABLE_IN_ALL
gboolean                pango2_attribute_get_boolean            (Pango2Attribute              *attribute);
PANGO2_AVAILABLE_IN_ALL
double                  pango2_attribute_get_float              (Pango2Attribute              *attribute);
PANGO2_AVAILABLE_IN_ALL
Pango2Color *           pango2_attribute_get_color              (Pango2Attribute              *attribute);
PANGO2_AVAILABLE_IN_ALL
Pango2FontDescription * pango2_attribute_get_font_desc          (Pango2Attribute              *attribute);

PANGO2_AVAILABLE_IN_ALL
gpointer                pango2_attribute_get_pointer            (Pango2Attribute              *attribute);


G_DEFINE_AUTOPTR_CLEANUP_FUNC(Pango2Attribute, pango2_attribute_destroy)

G_END_DECLS
