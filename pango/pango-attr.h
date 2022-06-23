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

#include <pango/pango-font.h>
#include <pango/pango-color.h>
#include <glib-object.h>

G_BEGIN_DECLS


typedef struct _PangoAttribute PangoAttribute;

/**
 * PangoAttrValueType:
 * @PANGO_ATTR_VALUE_STRING: A string
 * @PANGO_ATTR_VALUE_INT: An integer
 * @PANGO_ATTR_VALUE_BOOLEAN: A boolean
 * @PANGO_ATTR_VALUE_FLOAT: A floating point number
 * @PANGO_ATTR_VALUE_COLOR: A `PangoColor`
 * @PANGO_ATTR_VALUE_LANGUAGE: A `PangoLanguage`
 * @PANGO_ATTR_VALUE_FONT_DESC: A `PangoFontDescription`
 * @PANGO_ATTR_VALUE_POINTER: A generic pointer
 *
 * `PangoAttrValueType` enumerates the types of values
 * that a `PangoAttribute` can contain.
 *
 * The `PangoAttrValueType` of a `PangoAttribute` is part
 * of its type, and can be obtained with `PANGO_ATTR_VALUE_TYPE()`.
 */
typedef enum
{
  PANGO_ATTR_VALUE_STRING,
  PANGO_ATTR_VALUE_INT,
  PANGO_ATTR_VALUE_BOOLEAN,
  PANGO_ATTR_VALUE_FLOAT,
  PANGO_ATTR_VALUE_COLOR,
  PANGO_ATTR_VALUE_LANGUAGE,
  PANGO_ATTR_VALUE_FONT_DESC,
  PANGO_ATTR_VALUE_POINTER
} PangoAttrValueType;

/**
 * PangoAttrAffects:
 * @PANGO_ATTR_AFFECTS_NONE: The attribute does not affect rendering
 * @PANGO_ATTR_AFFECTS_ITEMIZATION: The attribute affecs itemization
 * @PANGO_ATTR_AFFECTS_BREAKING: The attribute affects `PangoLogAttr` determination
 * @PANGO_ATTR_AFFECTS_SHAPING: The attribute affects shaping
 * @PANGO_ATTR_AFFECTS_RENDERING: The attribute affects rendering
 *
 * A `PangoAttrAffects` value indicates what part of Pango's processing
 * pipeline is affected by an attribute.
 *
 * Marking an attribute with `PANGO_ATTR_AFFECTS_ITEMIZATION` ensures
 * that the attribute values are constant across items.
 */
typedef enum
{
  PANGO_ATTR_AFFECTS_NONE        = 0,
  PANGO_ATTR_AFFECTS_ITEMIZATION = 1 << 0,
  PANGO_ATTR_AFFECTS_BREAKING    = 1 << 1,
  PANGO_ATTR_AFFECTS_SHAPING     = 1 << 2,
  PANGO_ATTR_AFFECTS_RENDERING   = 1 << 3
} PangoAttrAffects;

/**
 * PangoAttrMerge:
 * @PANGO_ATTR_MERGE_OVERRIDES: Only the attribute with the narrowest range is used
 * @PANGO_ATTR_MERGE_ACCUMULATES: All attributes with overlapping range are kept
 *
 * A `PangoAttrMerge` value indicates how overlapping attribute values
 * should be reconciled to determine the effective attribute value.
 *
 * These options influence the @extra_attrs returned by
 * [method@Pango.AttrIterator.get_font].
 */
typedef enum
{
  PANGO_ATTR_MERGE_OVERRIDES,
  PANGO_ATTR_MERGE_ACCUMULATES
} PangoAttrMerge;

/**
 * PANGO_ATTR_TYPE_VALUE_TYPE:
 * @type: an attribute type
 *
 * Extracts the `PangoAttrValueType` from an attribute type.
 */
#define PANGO_ATTR_TYPE_VALUE_TYPE(type) ((PangoAttrValueType)((type) & 0xff))

/**
 * PANGO_ATTR_TYPE_AFFECTS:
 * @type: an attribute type
 *
 * Extracts the `PangoAttrAffects` flags from an attribute type.
 */
#define PANGO_ATTR_TYPE_AFFECTS(type) ((PangoAttrAffects)(((type) >> 8) & 0xf))

/**
 * PANGO_ATTR_TYPE_MERGE:
 * @type: an attribute type
 *
 * Extracts the `PangoAttrMerge` flags from an attribute type.
 */
#define PANGO_ATTR_TYPE_MERGE(type) ((PangoAttrMerge)(((type) >> 12) & 0xf))

/**
 * PANGO_ATTR_VALUE_TYPE:
 * @attr: a `PangoAttribute`
 *
 * Obtains the `PangoAttrValueType of a `PangoAttribute`.
 */
#define PANGO_ATTR_VALUE_TYPE(attr) PANGO_ATTR_TYPE_VALUE_TYPE (pango_attribute_type (attr))

/**
 * PANGO_ATTR_AFFECTS:
 * @attr: a `PangoAttribute`
 *
 * Obtains the `PangoAttrAffects` flags of a `PangoAttribute`.
 */
#define PANGO_ATTR_AFFECTS(attr) PANGO_ATTR_TYPE_AFFECTS (pango_attribute_type (attr))

/**
 * PANGO_ATTR_MERGE:
 * @attr: a `PangoAttribute`
 *
 * Obtains the `PangoAttrMerge` flags of a `PangoAttribute`.
 */
#define PANGO_ATTR_MERGE(attr) PANGO_ATTR_TYPE_MERGE (pango_attribute_type (attr))

/**
 * PANGO_ATTR_INDEX_FROM_TEXT_BEGINNING:
 *
 * Value for @start_index in `PangoAttribute` that indicates
 * the beginning of the text.
 */
#define PANGO_ATTR_INDEX_FROM_TEXT_BEGINNING ((guint)0)

/**
 * PANGO_ATTR_INDEX_TO_TEXT_END: (value 4294967295)
 *
 * Value for @end_index in `PangoAttribute` that indicates
 * the end of the text.
 */
#define PANGO_ATTR_INDEX_TO_TEXT_END ((guint)(G_MAXUINT + 0))

/**
 * PangoAttrDataCopyFunc:
 * @value: value to copy
 *
 * Callback to duplicate the value of an attribute.
 *
 * Return value: new copy of @value.
 **/
typedef gpointer (*PangoAttrDataCopyFunc) (gconstpointer value);

/**
 * PangoAttrDataSerializeFunc:
 * @value: value to serialize
 *
 * Callback to serialize the value of an attribute.
 *
 * Return value: a newly allocated string holding the serialization of @value
 */
typedef char * (*PangoAttrDataSerializeFunc) (gconstpointer value);

PANGO_AVAILABLE_IN_ALL
GType                   pango_attribute_get_type                (void) G_GNUC_CONST;

PANGO_AVAILABLE_IN_ALL
guint                   pango_attr_type_register                (const char                 *name,
                                                                 PangoAttrValueType          value_type,
                                                                 PangoAttrAffects            affects,
                                                                 PangoAttrMerge              merge,
                                                                 PangoAttrDataCopyFunc       copy,
                                                                 GDestroyNotify              destroy,
                                                                 GEqualFunc                  equal,
                                                                 PangoAttrDataSerializeFunc  serialize);

PANGO_AVAILABLE_IN_ALL
const char *            pango_attr_type_get_name                (guint                       type) G_GNUC_CONST;
PANGO_AVAILABLE_IN_ALL
PangoAttribute *        pango_attribute_copy                    (const PangoAttribute       *attr);
PANGO_AVAILABLE_IN_ALL
void                    pango_attribute_destroy                 (PangoAttribute             *attr);
PANGO_AVAILABLE_IN_ALL
gboolean                pango_attribute_equal                   (const PangoAttribute       *attr1,
                                                                 const PangoAttribute       *attr2) G_GNUC_PURE;

PANGO_AVAILABLE_IN_ALL
PangoAttribute *        pango_attribute_new                     (guint                       type,
                                                                 gconstpointer               value);

PANGO_AVAILABLE_IN_ALL
guint                   pango_attribute_type                    (const PangoAttribute        *attribute);

PANGO_AVAILABLE_IN_ALL
void                    pango_attribute_set_range               (PangoAttribute              *attribute,
                                                                 guint                        start_index,
                                                                 guint                        end_index);

PANGO_AVAILABLE_IN_ALL
void                    pango_attribute_get_range               (PangoAttribute              *attribute,
                                                                 guint                       *start_index,
                                                                 guint                       *end_index);

PANGO_AVAILABLE_IN_ALL
const char *            pango_attribute_get_string              (PangoAttribute              *attribute);
PANGO_AVAILABLE_IN_ALL
PangoLanguage *         pango_attribute_get_language            (PangoAttribute              *attribute);
PANGO_AVAILABLE_IN_ALL
int                     pango_attribute_get_int                 (PangoAttribute              *attribute);
PANGO_AVAILABLE_IN_ALL
gboolean                pango_attribute_get_boolean             (PangoAttribute              *attribute);
PANGO_AVAILABLE_IN_ALL
double                  pango_attribute_get_float               (PangoAttribute              *attribute);
PANGO_AVAILABLE_IN_ALL
PangoColor *            pango_attribute_get_color               (PangoAttribute              *attribute);
PANGO_AVAILABLE_IN_ALL
PangoFontDescription *  pango_attribute_get_font_desc           (PangoAttribute              *attribute);

PANGO_AVAILABLE_IN_ALL
gpointer                pango_attribute_get_pointer             (PangoAttribute              *attribute);


G_DEFINE_AUTOPTR_CLEANUP_FUNC(PangoAttribute, pango_attribute_destroy)

G_END_DECLS
