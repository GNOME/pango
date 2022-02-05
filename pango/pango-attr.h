/* Pango
 * pango-attributes.h: Attributed text
 *
 * Copyright (C) 2000 Red Hat Software
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Library General Public
 * License as published by the Free Software Foundation; either
 * version 2 of the License, or (at your option) any later version.
 *
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Library General Public License for more details.
 *
 * You should have received a copy of the GNU Library General Public
 * License along with this library; if not, write to the
 * Free Software Foundation, Inc., 59 Temple Place - Suite 330,
 * Boston, MA 02111-1307, USA.
 */

#pragma once

#include <pango/pango-font.h>
#include <pango/pango-color.h>
#include <glib-object.h>

G_BEGIN_DECLS


typedef struct _PangoAttribute PangoAttribute;

/**
 * PangoAttrValueType:
 * @PANGO_ATTR_VALUE_STRING: a string
 * @PANGO_ATTR_VALUE_INT: an integer
 * @PANGO_ATTR_VALUE_BOOLEAN: a boolean
 * @PANGO_ATTR_VALUE_FLOAT: a floating point number
 * @PANGO_ATTR_VALUE_COLOR: a `PangoColor`
 * @PANGO_ATTR_VALUE_LANGUAGE: a `PangoLanguage`
 * @PANGO_ATTR_VALUE_FONT_DESC: a `PangoFontDescription`
 * @PANGO_ATTR_VALUE_POINTER: a generic pointer
 *
 * The `PangoAttrValueType` enumeration describes the various types
 * of values that a `PangoAttribute` can contain.
 *
 * The `PangoAttrValueType` of a `PangoAttribute` is part of its type,
 * and can be obtained with `PANGO_ATTR_VALUE_TYPE()`.
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
 * PANGO_ATTR_TYPE_VALUE_TYPE:
 * @type: an attribute type
 *
 * Extracts the `PangoAttrValueType` from an attribute type.
 */
#define PANGO_ATTR_TYPE_VALUE_TYPE(type) ((PangoAttrValueType)((type) & 0xff))

/**
 * PANGO_ATTR_VALUE_TYPE:
 * @attr: a `PangoAttribute`
 *
 * Obtains the `PangoAttrValueType of a `PangoAttribute`.
 */
#define PANGO_ATTR_VALUE_TYPE(attr) PANGO_ATTR_TYPE_VALUE_TYPE ((attr)->type)

/**
 * PANGO_ATTR_INDEX_FROM_TEXT_BEGINNING:
 *
 * Value for @start_index in `PangoAttribute` that indicates
 * the beginning of the text.
 *
 * Since: 1.24
 */
#define PANGO_ATTR_INDEX_FROM_TEXT_BEGINNING ((guint)0)

/**
 * PANGO_ATTR_INDEX_TO_TEXT_END: (value 4294967295)
 *
 * Value for @end_index in `PangoAttribute` that indicates
 * the end of the text.
 *
 * Since: 1.24
 */
#define PANGO_ATTR_INDEX_TO_TEXT_END ((guint)(G_MAXUINT + 0))

/**
 * PangoAttribute:
 * @klass: the class structure holding information about the type of the attribute
 * @start_index: the start index of the range (in bytes).
 * @end_index: end index of the range (in bytes). The character at this index
 *   is not included in the range.
 * @str_value: the string value
 * @int_value: the integer value
 * @boolean_value: the boolean value
 * @double_value: the double value
 * @color_value: the `PangoColor` value
 * @lang_value: the `PangoLanguage` value
 * @font_value: the `PangoFontDescription` value
 * @pointer_value: a custom value
 *
 * The `PangoAttribute` structure contains information for a single
 * attribute.
 *
 * The common portion of the attribute holds the range to which
 * the value in the type-specific part of the attribute applies.
 * By default, an attribute will have an all-inclusive range of
 * [0,%G_MAXUINT].
 *
 * Which of the values is used depends on the value type of the
 * attribute, which can be obtained with `PANGO_ATTR_VALUE_TYPE()`.
 */
struct _PangoAttribute
{
  guint type;
  guint start_index;
  guint end_index;
  union {
    char *str_value;
    int  int_value;
    gboolean boolean_value;
    double double_value;
    PangoColor color_value;
    PangoLanguage *lang_value;
    PangoFontDescription *font_value;
    gpointer pointer_value;
  };
};

/**
 * PangoAttrDataCopyFunc:
 * @value: value to copy
 *
 * Type of a function that can duplicate the value of an attribute.
 *
 * Return value: new copy of @value.
 **/
typedef gpointer (*PangoAttrDataCopyFunc) (gconstpointer value);

/**
 * PangoAttrDataSerializeFunc:
 * @value: value to serialize
 *
 * Type of a function that can serialize the value of an attribute.
 *
 * Return value: a newly allocated string holding the serialization of @value
 */
typedef char * (*PangoAttrDataSerializeFunc) (gconstpointer value);

PANGO_AVAILABLE_IN_ALL
GType                   pango_attribute_get_type                (void) G_GNUC_CONST;

PANGO_AVAILABLE_IN_ALL
guint                   pango_attr_type_register                (PangoAttrDataCopyFunc       copy,
                                                                 GDestroyNotify              destroy,
                                                                 GEqualFunc                  equal,
                                                                 const char                 *name,
                                                                 PangoAttrDataSerializeFunc  serialize);

PANGO_AVAILABLE_IN_1_22
const char *            pango_attr_type_get_name                (guint                       type) G_GNUC_CONST;
PANGO_AVAILABLE_IN_ALL
PangoAttribute *        pango_attribute_copy                    (const PangoAttribute       *attr);
PANGO_AVAILABLE_IN_ALL
void                    pango_attribute_destroy                 (PangoAttribute             *attr);
PANGO_AVAILABLE_IN_ALL
gboolean                pango_attribute_equal                   (const PangoAttribute       *attr1,
                                                                 const PangoAttribute       *attr2) G_GNUC_PURE;

PANGO_AVAILABLE_IN_1_52
PangoAttribute *        pango_attr_custom_new                   (guint                       type,
                                                                 gpointer                     user_data);

PANGO_AVAILABLE_IN_1_52
gboolean                pango_attribute_get_string              (PangoAttribute              *attribute,
                                                                 const char                 **value);
PANGO_AVAILABLE_IN_1_52
gboolean                pango_attribute_get_language            (PangoAttribute              *attribute,
                                                                 PangoLanguage              **value);
PANGO_AVAILABLE_IN_1_52
gboolean                pango_attribute_get_int                 (PangoAttribute              *attribute,
                                                                 int                         *value);
PANGO_AVAILABLE_IN_1_52
gboolean                pango_attribute_get_boolean             (PangoAttribute              *attribute,
                                                                 gboolean                    *value);
PANGO_AVAILABLE_IN_1_52
gboolean                pango_attribute_get_float               (PangoAttribute              *attribute,
                                                                 double                      *value);
PANGO_AVAILABLE_IN_1_52
gboolean                pango_attribute_get_color               (PangoAttribute              *attribute,
                                                                 PangoColor                  *value);
PANGO_AVAILABLE_IN_1_52
gboolean                pango_attribute_get_font_desc           (PangoAttribute              *attribute,
                                                                 PangoFontDescription       **value);

PANGO_AVAILABLE_IN_1_52
gboolean                pango_attribute_get_custom              (PangoAttribute              *attribute,
                                                                 gpointer                    *value);


G_DEFINE_AUTOPTR_CLEANUP_FUNC(PangoAttribute, pango_attribute_destroy)

G_END_DECLS
