/* Pango2
 * pango-attr.c: Attributed text
 *
 * Copyright (C) 2000-2002 Red Hat Software
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

#include "config.h"
#include <string.h>

#include "pango-attr-private.h"
#include "pango-attributes-private.h"
#include "pango-impl-utils.h"

/**
 * Pango2Attribute:
 *
 * A `Pango2Attribute` structure associates a named attribute with
 * a certain range of a text.
 *
 * Attributes can have basic types, such as int, float, boolean
 * `Pango2Color`, `Pango2FontDescription or `Pango2Language`. It is
 * also possible to define new, custom attributes.
 *
 * By default, an attribute will have an all-inclusive range of
 * `[0,G_MAXUINT]`.
 */

/* {{{ Generic attribute code */

G_LOCK_DEFINE_STATIC (attr_type);
static GArray *attr_type;

#define MIN_CUSTOM 1000

typedef struct _Pango2AttrClass Pango2AttrClass;

struct _Pango2AttrClass {
  guint type;
  const char *name;
  Pango2AttrDataCopyFunc copy;
  GDestroyNotify destroy;
  GEqualFunc equal;
  Pango2AttrDataSerializeFunc serialize;
};

static const char *
get_attr_type_nick (Pango2AttrType type)
{
  GEnumClass *enum_class;
  GEnumValue *enum_value;

  enum_class = g_type_class_ref (pango2_attr_type_get_type ());
  enum_value = g_enum_get_value (enum_class, type);
  g_type_class_unref (enum_class);

  return enum_value->value_nick;
}

static gboolean
is_valid_attr_type (guint type)
{
  switch (type)
    {
    case PANGO2_ATTR_INVALID:
      return FALSE;
    case PANGO2_ATTR_LANGUAGE:
    case PANGO2_ATTR_FAMILY:
    case PANGO2_ATTR_STYLE:
    case PANGO2_ATTR_WEIGHT:
    case PANGO2_ATTR_VARIANT:
    case PANGO2_ATTR_STRETCH:
    case PANGO2_ATTR_SIZE:
    case PANGO2_ATTR_FONT_DESC:
    case PANGO2_ATTR_FOREGROUND:
    case PANGO2_ATTR_BACKGROUND:
    case PANGO2_ATTR_UNDERLINE:
    case PANGO2_ATTR_STRIKETHROUGH:
    case PANGO2_ATTR_RISE:
    case PANGO2_ATTR_SCALE:
    case PANGO2_ATTR_FALLBACK:
    case PANGO2_ATTR_LETTER_SPACING:
    case PANGO2_ATTR_UNDERLINE_COLOR:
    case PANGO2_ATTR_STRIKETHROUGH_COLOR:
    case PANGO2_ATTR_ABSOLUTE_SIZE:
    case PANGO2_ATTR_GRAVITY:
    case PANGO2_ATTR_GRAVITY_HINT:
    case PANGO2_ATTR_FONT_FEATURES:
    case PANGO2_ATTR_ALLOW_BREAKS:
    case PANGO2_ATTR_SHOW:
    case PANGO2_ATTR_INSERT_HYPHENS:
    case PANGO2_ATTR_OVERLINE:
    case PANGO2_ATTR_OVERLINE_COLOR:
    case PANGO2_ATTR_LINE_HEIGHT:
    case PANGO2_ATTR_ABSOLUTE_LINE_HEIGHT:
    case PANGO2_ATTR_TEXT_TRANSFORM:
    case PANGO2_ATTR_WORD:
    case PANGO2_ATTR_SENTENCE:
    case PANGO2_ATTR_BASELINE_SHIFT:
    case PANGO2_ATTR_FONT_SCALE:
    case PANGO2_ATTR_SHAPE:
      return TRUE;
    default:
      if (!attr_type)
        return FALSE;
      else
        {
          gboolean result = FALSE;

          G_LOCK (attr_type);

          for (int i = 0; i < attr_type->len; i++)
            {
              Pango2AttrClass *class = &g_array_index (attr_type, Pango2AttrClass, i);
              if (class->type == type)
                {
                  result = TRUE;
                  break;
                }
            }

          G_UNLOCK (attr_type);

          return result;
        }
    }
}

/**
 * pango2_attr_type_register:
 * @name: (nullable): an identifier for the type
 * @value_type: the `Pango2AttrValueType` for the attribute
 * @affects: `Pango2AttrAffects` flags for the attribute
 * @merge: `Pango2AttrMerge` flags for the attribute
 * @copy: (scope forever) (nullable): function to copy the data of an attribute
 *   of this type
 * @destroy: (scope forever) (nullable): function to free the data of an attribute
 *   of this type
 * @equal: (scope forever) (nullable): function to compare the data of two attributes
 *   of this type
 * @serialize: (scope forever) (nullable): function to serialize the data
 *   of an attribute of this type
 *
 * Register a new attribute type.
 *
 * The attribute type name can be accessed later
 * by using [func@Pango2.AttrType.get_name].
 *
 * The callbacks are only needed if @type is `PANGO2_ATTR_VALUE_POINTER`,
 * Pango knows how to handle other value types and will only use the
 * callbacks for generic pointer values.
 *
 * If @name and @serialize are provided, they will be used
 * to serialize attributes of this type.
 *
 * To create attributes with the new type, use [ctor@Pango2.Attribute.new].
 *
 * Return value: the attribute type ID
 */
guint
pango2_attr_type_register (const char                  *name,
                           Pango2AttrValueType          value_type,
                           Pango2AttrAffects            affects,
                           Pango2AttrMerge              merge,
                           Pango2AttrDataCopyFunc       copy,
                           GDestroyNotify               destroy,
                           GEqualFunc                   equal,
                           Pango2AttrDataSerializeFunc  serialize)
{
  static guint current_id = MIN_CUSTOM; /* MT-safe */
  Pango2AttrClass class;

  G_LOCK (attr_type);

  class.type = value_type | (affects << 8) | (merge << 12) | (current_id << 16);
  current_id++;

  class.copy = copy;
  class.destroy = destroy;
  class.equal = equal;
  class.serialize = serialize;

  if (name)
    class.name = g_intern_string (name);

  if (attr_type == NULL)
    attr_type = g_array_new (FALSE, FALSE, sizeof (Pango2AttrClass));

  g_array_append_val (attr_type, class);

  G_UNLOCK (attr_type);

  return class.type;
}

/**
 * pango2_attr_type_get_name:
 * @type: an attribute type ID to fetch the name for
 *
 * Fetches the attribute type name.
 *
 * The attribute type name is the string passed in
 * when registering the type using
 * [func@Pango2.AttrType.register].
 *
 * The returned value is an interned string (see
 * [func@GLib.intern_string] for what that means) that should
 * not be modified or freed.
 *
 * Return value: (nullable): the type ID name (which
 *   may be %NULL), or %NULL if @type is a built-in Pango2
 *   attribute type or invalid.
 */
const char *
pango2_attr_type_get_name (guint type)
{
  const char *result = NULL;

  if ((type >> 16) < MIN_CUSTOM)
    return get_attr_type_nick (type);

  G_LOCK (attr_type);

  for (int i = 0; i < attr_type->len; i++)
    {
      Pango2AttrClass *class = &g_array_index (attr_type, Pango2AttrClass, i);
      if (class->type == type)
        {
          result = class->name;
          break;
        }
    }

  G_UNLOCK (attr_type);

  return result;
}

/**
 * pango2_attribute_copy:
 * @attr: a `Pango2Attribute`
 *
 * Make a copy of an attribute.
 *
 * Return value: (transfer full): the newly allocated
 *   `Pango2Attribute`, which should be freed with
 *   [method@Pango2.Attribute.destroy].
 */
Pango2Attribute *
pango2_attribute_copy (const Pango2Attribute *attr)
{
  Pango2Attribute *result;

  g_return_val_if_fail (attr != NULL, NULL);

  result = g_slice_dup (Pango2Attribute, attr);

  switch (PANGO2_ATTR_VALUE_TYPE (attr))
    {
    case PANGO2_ATTR_VALUE_STRING:
      result->str_value = g_strdup (attr->str_value);
      break;

    case PANGO2_ATTR_VALUE_FONT_DESC:
      result->font_value = pango2_font_description_copy (attr->font_value);
      break;

    case PANGO2_ATTR_VALUE_POINTER:
      if (attr->type == PANGO2_ATTR_SHAPE)
        {
          ShapeData *shape_data;

          shape_data = g_memdup2 (attr->pointer_value, sizeof (ShapeData));
          if (shape_data->copy)
            shape_data->data = shape_data->copy (shape_data->data);

          result->pointer_value = shape_data;
        }
      else
        {
          Pango2AttrDataCopyFunc copy = NULL;

          G_LOCK (attr_type);

          g_assert (attr_type != NULL);

          for (int i = 0; i < attr_type->len; i++)
            {
              Pango2AttrClass *class = &g_array_index (attr_type, Pango2AttrClass, i);
              if (class->type == attr->type)
                {
                  copy = class->copy;
                  break;
                }
            }

          G_UNLOCK (attr_type);

          if (copy)
            result->pointer_value = copy (attr->pointer_value);
        }
      break;

    case PANGO2_ATTR_VALUE_INT:
    case PANGO2_ATTR_VALUE_BOOLEAN:
    case PANGO2_ATTR_VALUE_FLOAT:
    case PANGO2_ATTR_VALUE_COLOR:
    case PANGO2_ATTR_VALUE_LANGUAGE:
    default: ;
    }

  return result;
}

/**
 * pango2_attribute_destroy:
 * @attr: a `Pango2Attribute`.
 *
 * Destroy a `Pango2Attribute` and free all associated memory.
 */
void
pango2_attribute_destroy (Pango2Attribute *attr)
{
  g_return_if_fail (attr != NULL);

  switch (PANGO2_ATTR_VALUE_TYPE (attr))
    {
    case PANGO2_ATTR_VALUE_STRING:
      g_free (attr->str_value);
      break;

    case PANGO2_ATTR_VALUE_FONT_DESC:
      pango2_font_description_free (attr->font_value);
      break;

    case PANGO2_ATTR_VALUE_POINTER:
      if (attr->type == PANGO2_ATTR_SHAPE)
        {
          ShapeData *shape_data = attr->pointer_value;

          if (shape_data->destroy)
            shape_data->destroy (shape_data->data);

          g_free (shape_data);
        }
      else
        {
          GDestroyNotify destroy = NULL;

          G_LOCK (attr_type);

          g_assert (attr_type != NULL);

          for (int i = 0; i < attr_type->len; i++)
            {
              Pango2AttrClass *class = &g_array_index (attr_type, Pango2AttrClass, i);
              if (class->type == attr->type)
                {
                  destroy = class->destroy;
                  break;
                }
            }

          G_UNLOCK (attr_type);

          if (destroy)
            destroy (attr->pointer_value);
        }
      break;

    case PANGO2_ATTR_VALUE_INT:
    case PANGO2_ATTR_VALUE_BOOLEAN:
    case PANGO2_ATTR_VALUE_FLOAT:
    case PANGO2_ATTR_VALUE_COLOR:
    case PANGO2_ATTR_VALUE_LANGUAGE:
    default: ;
    }

  g_slice_free (Pango2Attribute, attr);
}

G_DEFINE_BOXED_TYPE (Pango2Attribute, pango2_attribute,
                     pango2_attribute_copy,
                     pango2_attribute_destroy);

/**
 * pango2_attribute_equal:
 * @attr1: a `Pango2Attribute`
 * @attr2: another `Pango2Attribute`
 *
 * Compare two attributes for equality.
 *
 * This compares only the actual value of the two
 * attributes and not the ranges that the attributes
 * apply to.
 *
 * Return value: %TRUE if the two attributes have the same value
 */
gboolean
pango2_attribute_equal (const Pango2Attribute *attr1,
                        const Pango2Attribute *attr2)
{
  g_return_val_if_fail (attr1 != NULL, FALSE);
  g_return_val_if_fail (attr2 != NULL, FALSE);

  if (attr1->type != attr2->type)
    return FALSE;

  switch (PANGO2_ATTR_VALUE_TYPE (attr1))
    {
    case PANGO2_ATTR_VALUE_STRING:
      return strcmp (attr1->str_value, attr2->str_value) == 0;

    case PANGO2_ATTR_VALUE_INT:
      return attr1->int_value == attr2->int_value;

    case PANGO2_ATTR_VALUE_BOOLEAN:
      return attr1->boolean_value == attr2->boolean_value;

    case PANGO2_ATTR_VALUE_FLOAT:
      return attr1->double_value == attr2->double_value;

    case PANGO2_ATTR_VALUE_COLOR:
      return memcmp (&attr1->color_value, &attr2->color_value, sizeof (Pango2Color)) == 0;

    case PANGO2_ATTR_VALUE_LANGUAGE:
      return attr1->lang_value == attr2->lang_value;

    case PANGO2_ATTR_VALUE_FONT_DESC:
      return pango2_font_description_equal (attr1->font_value, attr2->font_value);

    case PANGO2_ATTR_VALUE_POINTER:
      {
        GEqualFunc equal = NULL;

        G_LOCK (attr_type);

        g_assert (attr_type != NULL);

        for (int i = 0; i < attr_type->len; i++)
          {
            Pango2AttrClass *class = &g_array_index (attr_type, Pango2AttrClass, i);
            if (class->type == attr1->type)
              {
                equal = class->equal;
                break;
              }
          }

        G_UNLOCK (attr_type);

        if (equal)
          return equal (attr1->pointer_value, attr2->pointer_value);

        return attr1->pointer_value == attr2->pointer_value;
      }

    default:
      g_assert_not_reached ();
    }
}

/**
 * pango2_attribute_new:
 * @type: the attribute type
 * @value: pointer to the value to be copied, must be of the right type
 *
 * Creates a new attribute for the given type.
 *
 * The type must be one of the `Pango2AttrType` values, or
 * have been registered with [func@Pango2.AttrType.register].
 *
 * Pango will initialize @start_index and @end_index to an
 * all-inclusive range of `[0,G_MAXUINT]`. The value is copied.
 *
 * Return value: (transfer full): the newly allocated
 *   `Pango2Attribute`, which should be freed with
 *   [method@Pango2.Attribute.destroy]
 */
Pango2Attribute *
pango2_attribute_new (guint         type,
                      gconstpointer value)
{
  Pango2Attribute attr;

  g_return_val_if_fail (is_valid_attr_type (type), NULL);

  attr.type = type;
  attr.start_index = PANGO2_ATTR_INDEX_FROM_TEXT_BEGINNING;
  attr.end_index = PANGO2_ATTR_INDEX_TO_TEXT_END;

  switch (PANGO2_ATTR_TYPE_VALUE_TYPE (type))
    {
    case PANGO2_ATTR_VALUE_STRING:
      attr.str_value = (char *)value;
      break;
    case PANGO2_ATTR_VALUE_INT:
      attr.int_value = *(int *)value;
      break;
    case PANGO2_ATTR_VALUE_BOOLEAN:
      attr.boolean_value = *(gboolean *)value;
      break;
    case PANGO2_ATTR_VALUE_FLOAT:
      attr.double_value = *(double *)value;
      break;
    case PANGO2_ATTR_VALUE_COLOR:
      attr.color_value = *(Pango2Color *)value;
      break;
    case PANGO2_ATTR_VALUE_LANGUAGE:
      attr.lang_value = (Pango2Language *)value;
      break;
    case PANGO2_ATTR_VALUE_FONT_DESC:
      attr.font_value = (Pango2FontDescription *)value;
      break;
    case PANGO2_ATTR_VALUE_POINTER:
      attr.pointer_value = (gpointer)value;
      break;
    default:
      g_assert_not_reached ();
    }

  /* Copy the value */
  return pango2_attribute_copy (&attr);
}

/**
 * pango2_attribute_type:
 * @attribute: a `Pango2Attribute`
 *
 * Returns the type of @attribute, either a
 * value from the [enum@Pango2.AttrType] enumeration
 * or a registered custom type.
 *
 * Return value: the type of `Pango2Attribute`
 */
guint
pango2_attribute_type (const Pango2Attribute *attribute)
{
  return attribute->type;
}

/* }}} */
 /* {{{ Private API */

char *
pango2_attr_value_serialize (Pango2Attribute *attr)
{
  Pango2AttrDataSerializeFunc serialize = NULL;

  G_LOCK (attr_type);

  g_assert (attr_type != NULL);

  for (int i = 0; i < attr_type->len; i++)
    {
      Pango2AttrClass *class = &g_array_index (attr_type, Pango2AttrClass, i);
      if (class->type == attr->type)
        {
          serialize = class->serialize;
          break;
        }
    }

  G_UNLOCK (attr_type);

  if (serialize)
    return serialize (attr->pointer_value);

  return g_strdup_printf ("%p", attr->pointer_value);
}

/* }}} */
/* {{{ Binding Helpers */

/**
 * pango2_attribute_set_range:
 * @attribute: a `Pango2Attribute`
 * @start_index: start index
 * @end_index: end index
 *
 * Sets the range of the attribute.
 */
void
pango2_attribute_set_range (Pango2Attribute *attribute,
                            guint            start_index,
                            guint            end_index)
{
  attribute->start_index = start_index;
  attribute->end_index = end_index;
}

/**
 * pango2_attribute_get_range:
 * @attribute: a `Pango2Attribute`
 * @start_index: (out caller-allocates): return location for start index
 * @end_index: (out caller-allocates): return location for end index
 *
 * Gets the range of the attribute.
 */
void
pango2_attribute_get_range (Pango2Attribute *attribute,
                            guint           *start_index,
                            guint           *end_index)
{
  *start_index = attribute->start_index;
  *end_index = attribute->end_index;
}

/**
 * pango2_attribute_get_string:
 * @attribute: a `Pango2Attribute`
 *
 * Gets the value of an attribute whose value type is string.
 *
 * Returns: (nullable): the string value
 */
const char *
pango2_attribute_get_string (Pango2Attribute *attribute)
{
  if (PANGO2_ATTR_VALUE_TYPE (attribute) != PANGO2_ATTR_VALUE_STRING)
    return NULL;

  return attribute->str_value;
}

/**
 * pango2_attribute_get_language:
 * @attribute: a `Pango2Attribute`
 *
 * Gets the value of an attribute whose value type is `Pango2Language`.
 *
 * Returns: (nullable): the language value
 */
Pango2Language *
pango2_attribute_get_language (Pango2Attribute *attribute)
{
  if (PANGO2_ATTR_VALUE_TYPE (attribute) != PANGO2_ATTR_VALUE_LANGUAGE)
    return NULL;

  return attribute->lang_value;
}

/**
 * pango2_attribute_get_int:
 * @attribute: a `Pango2Attribute`
 *
 * Gets the value of an attribute whose value type is `int`.
 *
 * Returns: the integer value, or 0
 */
int
pango2_attribute_get_int (Pango2Attribute *attribute)
{
  if (PANGO2_ATTR_VALUE_TYPE (attribute) != PANGO2_ATTR_VALUE_INT)
    return 0;

  return attribute->int_value;
}

/**
 * pango2_attribute_get_boolean:
 * @attribute: a `Pango2Attribute`
 *
 * Gets the value of an attribute whose value type is `gboolean`.
 *
 * Returns: the boolean value, or `FALSE`
 */
gboolean
pango2_attribute_get_boolean (Pango2Attribute *attribute)
{
  if (PANGO2_ATTR_VALUE_TYPE (attribute) != PANGO2_ATTR_VALUE_BOOLEAN)
    return FALSE;

  return attribute->boolean_value;
}

/**
 * pango2_attribute_get_float:
 * @attribute: a `Pango2Attribute`
 *
 * Gets the value of an attribute whose value type is `double`.
 *
 * Returns: the float value, or 0
 */
double
pango2_attribute_get_float (Pango2Attribute *attribute)
{
  if (PANGO2_ATTR_VALUE_TYPE (attribute) != PANGO2_ATTR_VALUE_FLOAT)
    return 0.;

  return attribute->double_value;
}

/**
 * pango2_attribute_get_color:
 * @attribute: a `Pango2Attribute`
 *
 * Gets the value of an attribute whose value type is `Pango2Color`.
 *
 * Returns: (nullable): pointer to the color value
 */
Pango2Color *
pango2_attribute_get_color (Pango2Attribute *attribute)
{
  if (PANGO2_ATTR_VALUE_TYPE (attribute) != PANGO2_ATTR_VALUE_COLOR)
    return NULL;

  return &attribute->color_value;
}

/**
 * pango2_attribute_get_font_desc:
 * @attribute: a `Pango2Attribute`
 *
 * Gets the value of an attribute whose value type is `Pango2FontDescription`.
 *
 * Returns: (nullable): the font description
 */
Pango2FontDescription *
pango2_attribute_get_font_desc (Pango2Attribute *attribute)
{
  if (PANGO2_ATTR_VALUE_TYPE (attribute) != PANGO2_ATTR_VALUE_FONT_DESC)
    return NULL;

  return attribute->font_value;
}

/**
 * pango2_attribute_get_pointer:
 * @attribute: a `Pango2Attribute`
 *
 * Gets the value of an attribute whose value type is `gpointer`.
 *
 * Returns: (nullable): the pointer value
 */
gpointer
pango2_attribute_get_pointer (Pango2Attribute *attribute)
{
  if (PANGO2_ATTR_VALUE_TYPE (attribute) != PANGO2_ATTR_VALUE_POINTER)
    return NULL;

  return attribute->pointer_value;
}

/* }}} */

/* vim:set foldmethod=marker expandtab: */
