/* Pango
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
#include "pango-impl-utils.h"


/* {{{ Generic attribute code */

G_LOCK_DEFINE_STATIC (attr_type);
static GArray *attr_type;

#define MIN_CUSTOM 1000

typedef struct _PangoAttrClass PangoAttrClass;

struct _PangoAttrClass {
  guint type;
  const char *name;
  PangoAttrDataCopyFunc copy;
  GDestroyNotify destroy;
  GEqualFunc equal;
  PangoAttrDataSerializeFunc serialize;
};

static const char *
get_attr_type_nick (PangoAttrType type)
{
  GEnumClass *enum_class;
  GEnumValue *enum_value;

  enum_class = g_type_class_ref (pango_attr_type_get_type ());
  enum_value = g_enum_get_value (enum_class, type);
  g_type_class_unref (enum_class);

  return enum_value->value_nick;
}

static gboolean
is_valid_attr_type (guint type)
{
  switch (type)
    {
    case PANGO_ATTR_INVALID:
      return FALSE;
    case PANGO_ATTR_LANGUAGE:
    case PANGO_ATTR_FAMILY:
    case PANGO_ATTR_STYLE:
    case PANGO_ATTR_WEIGHT:
    case PANGO_ATTR_VARIANT:
    case PANGO_ATTR_STRETCH:
    case PANGO_ATTR_SIZE:
    case PANGO_ATTR_FONT_DESC:
    case PANGO_ATTR_FOREGROUND:
    case PANGO_ATTR_BACKGROUND:
    case PANGO_ATTR_UNDERLINE:
    case PANGO_ATTR_STRIKETHROUGH:
    case PANGO_ATTR_RISE:
    case PANGO_ATTR_SCALE:
    case PANGO_ATTR_FALLBACK:
    case PANGO_ATTR_LETTER_SPACING:
    case PANGO_ATTR_UNDERLINE_COLOR:
    case PANGO_ATTR_STRIKETHROUGH_COLOR:
    case PANGO_ATTR_ABSOLUTE_SIZE:
    case PANGO_ATTR_GRAVITY:
    case PANGO_ATTR_GRAVITY_HINT:
    case PANGO_ATTR_FONT_FEATURES:
    case PANGO_ATTR_FOREGROUND_ALPHA:
    case PANGO_ATTR_BACKGROUND_ALPHA:
    case PANGO_ATTR_ALLOW_BREAKS:
    case PANGO_ATTR_SHOW:
    case PANGO_ATTR_INSERT_HYPHENS:
    case PANGO_ATTR_OVERLINE:
    case PANGO_ATTR_OVERLINE_COLOR:
    case PANGO_ATTR_LINE_HEIGHT:
    case PANGO_ATTR_ABSOLUTE_LINE_HEIGHT:
    case PANGO_ATTR_TEXT_TRANSFORM:
    case PANGO_ATTR_WORD:
    case PANGO_ATTR_SENTENCE:
    case PANGO_ATTR_BASELINE_SHIFT:
    case PANGO_ATTR_FONT_SCALE:
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
              PangoAttrClass *class = &g_array_index (attr_type, PangoAttrClass, i);
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
 * pango_attr_type_register:
 * @name: (nullable): an identifier for the type
 * @value_type: the `PangoAttrValueType` for the attribute
 * @affects: `PangoAttrAffects` flags for the attribute
 * @merge: `PangoAttrMerge` flags for the attribute
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
 * by using [func@Pango.AttrType.get_name].
 *
 * The callbacks are only needed if @type is `PANGO_ATTR_VALUE_POINTER`,
 * Pango knows how to handle other value types and will only use the
 * callbacks for generic pointer values.
 *
 * If @name and @serialize are provided, they will be used
 * to serialize attributes of this type.
 *
 * To create attributes with the new type, use [func@Pango.attr_custom_new].
 *
 * Return value: the attribute type ID
 */
guint
pango_attr_type_register (const char                 *name,
                          PangoAttrValueType          value_type,
                          PangoAttrAffects            affects,
                          PangoAttrMerge              merge,
                          PangoAttrDataCopyFunc       copy,
                          GDestroyNotify              destroy,
                          GEqualFunc                  equal,
                          PangoAttrDataSerializeFunc  serialize)
{
  static guint current_id = MIN_CUSTOM; /* MT-safe */
  PangoAttrClass class;

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
    attr_type = g_array_new (FALSE, FALSE, sizeof (PangoAttrClass));

  g_array_append_val (attr_type, class);

  G_UNLOCK (attr_type);

  return class.type;
}

/**
 * pango_attr_type_get_name:
 * @type: an attribute type ID to fetch the name for
 *
 * Fetches the attribute type name.
 *
 * The attribute type name is the string passed in
 * when registering the type using
 * [func@Pango.AttrType.register].
 *
 * The returned value is an interned string (see
 * g_intern_string() for what that means) that should
 * not be modified or freed.
 *
 * Return value: (nullable): the type ID name (which
 *   may be %NULL), or %NULL if @type is a built-in Pango
 *   attribute type or invalid.
 */
const char *
pango_attr_type_get_name (guint type)
{
  const char *result = NULL;

  if ((type >> 16) < MIN_CUSTOM)
    return get_attr_type_nick (type);

  G_LOCK (attr_type);

  for (int i = 0; i < attr_type->len; i++)
    {
      PangoAttrClass *class = &g_array_index (attr_type, PangoAttrClass, i);
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
 * pango_attribute_copy:
 * @attr: a `PangoAttribute`
 *
 * Make a copy of an attribute.
 *
 * Return value: (transfer full): the newly allocated
 *   `PangoAttribute`, which should be freed with
 *   [method@Pango.Attribute.destroy].
 */
PangoAttribute *
pango_attribute_copy (const PangoAttribute *attr)
{
  PangoAttribute *result;

  g_return_val_if_fail (attr != NULL, NULL);

  result = g_slice_dup (PangoAttribute, attr);

  switch (PANGO_ATTR_VALUE_TYPE (attr))
    {
    case PANGO_ATTR_VALUE_STRING:
      result->str_value = g_strdup (attr->str_value);
      break;

    case PANGO_ATTR_VALUE_FONT_DESC:
      result->font_value = pango_font_description_copy (attr->font_value);
      break;

    case PANGO_ATTR_VALUE_POINTER:
      {
        PangoAttrDataCopyFunc copy = NULL;

        G_LOCK (attr_type);

        g_assert (attr_type != NULL);

        for (int i = 0; i < attr_type->len; i++)
          {
            PangoAttrClass *class = &g_array_index (attr_type, PangoAttrClass, i);
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

    case PANGO_ATTR_VALUE_INT:
    case PANGO_ATTR_VALUE_BOOLEAN:
    case PANGO_ATTR_VALUE_FLOAT:
    case PANGO_ATTR_VALUE_COLOR:
    case PANGO_ATTR_VALUE_LANGUAGE:
    default: ;
    }

  return result;
}

/**
 * pango_attribute_destroy:
 * @attr: a `PangoAttribute`.
 *
 * Destroy a `PangoAttribute` and free all associated memory.
 */
void
pango_attribute_destroy (PangoAttribute *attr)
{
  g_return_if_fail (attr != NULL);

  switch (PANGO_ATTR_VALUE_TYPE (attr))
    {
    case PANGO_ATTR_VALUE_STRING:
      g_free (attr->str_value);
      break;

    case PANGO_ATTR_VALUE_FONT_DESC:
      pango_font_description_free (attr->font_value);
      break;

    case PANGO_ATTR_VALUE_POINTER:
      {
        GDestroyNotify destroy = NULL;

        G_LOCK (attr_type);

        g_assert (attr_type != NULL);

        for (int i = 0; i < attr_type->len; i++)
          {
            PangoAttrClass *class = &g_array_index (attr_type, PangoAttrClass, i);
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

    case PANGO_ATTR_VALUE_INT:
    case PANGO_ATTR_VALUE_BOOLEAN:
    case PANGO_ATTR_VALUE_FLOAT:
    case PANGO_ATTR_VALUE_COLOR:
    case PANGO_ATTR_VALUE_LANGUAGE:
    default: ;
    }

  g_slice_free (PangoAttribute, attr);
}

G_DEFINE_BOXED_TYPE (PangoAttribute, pango_attribute,
                     pango_attribute_copy,
                     pango_attribute_destroy);

/**
 * pango_attribute_equal:
 * @attr1: a `PangoAttribute`
 * @attr2: another `PangoAttribute`
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
pango_attribute_equal (const PangoAttribute *attr1,
                       const PangoAttribute *attr2)
{
  g_return_val_if_fail (attr1 != NULL, FALSE);
  g_return_val_if_fail (attr2 != NULL, FALSE);

  if (attr1->type != attr2->type)
    return FALSE;

  switch (PANGO_ATTR_VALUE_TYPE (attr1))
    {
    case PANGO_ATTR_VALUE_STRING:
      return strcmp (attr1->str_value, attr2->str_value) == 0;

    case PANGO_ATTR_VALUE_INT:
      return attr1->int_value == attr2->int_value;

    case PANGO_ATTR_VALUE_BOOLEAN:
      return attr1->boolean_value == attr2->boolean_value;

    case PANGO_ATTR_VALUE_FLOAT:
      return attr1->double_value == attr2->double_value;

    case PANGO_ATTR_VALUE_COLOR:
      return memcmp (&attr1->color_value, &attr2->color_value, sizeof (PangoColor)) == 0;

    case PANGO_ATTR_VALUE_LANGUAGE:
      return attr1->lang_value == attr2->lang_value;

    case PANGO_ATTR_VALUE_FONT_DESC:
      return pango_font_description_equal (attr1->font_value, attr2->font_value);

    case PANGO_ATTR_VALUE_POINTER:
      {
        GEqualFunc equal = NULL;

        G_LOCK (attr_type);

        g_assert (attr_type != NULL);

        for (int i = 0; i < attr_type->len; i++)
          {
            PangoAttrClass *class = &g_array_index (attr_type, PangoAttrClass, i);
            if (class->type == attr1->type)
              {
                equal = class->equal;
                break;
              }
          }

        G_UNLOCK (attr_type);

        g_assert (equal != NULL);

        if (equal)
          return equal (attr1->pointer_value, attr2->pointer_value);

        return attr1->pointer_value == attr2->pointer_value;
      }

    default:
      g_assert_not_reached ();
    }
}

/**
 * pango_attribute_new:
 * @type: the attribute type
 *
 * Creates a new attribute for the given type.
 *
 * The type must be one of the `PangoAttrType` values, or
 * have been registered with [func@Pango.register_attr_type].
 *
 * Pango will initialize @start_index and @end_index to an
 * all-inclusive range of `[0,G_MAXUINT]`.  The caller is
 * responsible for filling the proper value field with the
 * desired value.
 *
 * Return value: (transfer full): the newly allocated
 *   `PangoAttribute`, which should be freed with
 *   [method@Pango.Attribute.destroy]
 */
PangoAttribute *
pango_attribute_new (guint type)
{
  PangoAttribute *attr;

  g_return_val_if_fail (is_valid_attr_type (type), NULL);

  attr = g_slice_new0 (PangoAttribute);
  attr->type = type;
  attr->start_index = PANGO_ATTR_INDEX_FROM_TEXT_BEGINNING;
  attr->end_index = PANGO_ATTR_INDEX_TO_TEXT_END;

  return attr;
}

/* }}} */
/* {{{ Private API */

char *
pango_attr_value_serialize (PangoAttribute *attr)
{
  PangoAttrDataSerializeFunc serialize = NULL;

  G_LOCK (attr_type);

  g_assert (attr_type != NULL);

  for (int i = 0; i < attr_type->len; i++)
    {
      PangoAttrClass *class = &g_array_index (attr_type, PangoAttrClass, i);
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

gboolean
pango_attribute_get_string (PangoAttribute   *attribute,
                            const char      **value)
{
  if (PANGO_ATTR_VALUE_TYPE (attribute) != PANGO_ATTR_VALUE_STRING)
    return FALSE;

  *value = attribute->str_value;
  return TRUE;
}

gboolean
pango_attribute_get_language (PangoAttribute  *attribute,
                              PangoLanguage  **value)
{
  if (PANGO_ATTR_VALUE_TYPE (attribute) != PANGO_ATTR_VALUE_LANGUAGE)
    return FALSE;

  *value = attribute->lang_value;
  return TRUE;
}

gboolean
pango_attribute_get_int (PangoAttribute *attribute,
                         int            *value)
{
  if (PANGO_ATTR_VALUE_TYPE (attribute) != PANGO_ATTR_VALUE_INT)
    return FALSE;

  *value = attribute->int_value;
  return TRUE;
}

gboolean
pango_attribute_get_boolean (PangoAttribute *attribute,
                             int            *value)
{
  if (PANGO_ATTR_VALUE_TYPE (attribute) != PANGO_ATTR_VALUE_BOOLEAN)
    return FALSE;

  *value = attribute->boolean_value;
  return TRUE;
}

gboolean
pango_attribute_get_float (PangoAttribute *attribute,
                           double         *value)
{
  if (PANGO_ATTR_VALUE_TYPE (attribute) != PANGO_ATTR_VALUE_FLOAT)
    return FALSE;

  *value = attribute->double_value;
  return TRUE;
}

gboolean
pango_attribute_get_color (PangoAttribute *attribute,
                           PangoColor     *value)
{
  if (PANGO_ATTR_VALUE_TYPE (attribute) != PANGO_ATTR_VALUE_COLOR)
    return FALSE;

  *value = attribute->color_value;
  return TRUE;
}

gboolean
pango_attribute_get_font_desc (PangoAttribute        *attribute,
                               PangoFontDescription **value)
{
  if (PANGO_ATTR_VALUE_TYPE (attribute) != PANGO_ATTR_VALUE_FONT_DESC)
    return FALSE;

  *value = attribute->font_value;
  return TRUE;
}

gboolean
pango_attribute_get_pointer (PangoAttribute *attribute,
                             gpointer       *value)
{
  if (PANGO_ATTR_VALUE_TYPE (attribute) != PANGO_ATTR_VALUE_POINTER)
    return FALSE;

  *value = attribute->pointer_value;
  return TRUE;
}

/* }}} */

/* vim:set foldmethod=marker expandtab: */
