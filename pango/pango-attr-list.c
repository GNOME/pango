/* Pango
 * pango-attr-list.c: Attribute lists
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

#include "pango-attr-list-private.h"
#include "pango-attr-private.h"
#include "pango-impl-utils.h"


G_DEFINE_BOXED_TYPE (PangoAttrList, pango_attr_list,
                     pango_attr_list_copy,
                     pango_attr_list_unref);

/* {{{ Utilities */

static void
pango_attr_list_insert_internal (PangoAttrList  *list,
                                 PangoAttribute *attr,
                                 gboolean        before)
{
  const guint start_index = attr->start_index;
  PangoAttribute *last_attr;

  if (G_UNLIKELY (!list->attributes))
    list->attributes = g_ptr_array_new ();

  if (list->attributes->len == 0)
    {
      g_ptr_array_add (list->attributes, attr);
      return;
    }

  g_assert (list->attributes->len > 0);

  last_attr = g_ptr_array_index (list->attributes, list->attributes->len - 1);

  if (last_attr->start_index < start_index ||
      (!before && last_attr->start_index == start_index))
    {
      g_ptr_array_add (list->attributes, attr);
    }
  else
    {
      guint i, p;

      for (i = 0, p = list->attributes->len; i < p; i++)
        {
          PangoAttribute *cur = g_ptr_array_index (list->attributes, i);

          if (cur->start_index > start_index ||
              (before && cur->start_index == start_index))
            {
              g_ptr_array_insert (list->attributes, i, attr);
              break;
            }
        }
    }
}

/* }}} */
/* {{{ Private API */

void
pango_attr_list_init (PangoAttrList *list)
{
  list->ref_count = 1;
  list->attributes = NULL;
}

void
pango_attr_list_destroy (PangoAttrList *list)
{
  if (!list->attributes)
    return;

  g_ptr_array_free (list->attributes, TRUE);
}

gboolean
pango_attr_list_has_attributes (const PangoAttrList *list)
{
  return list && list->attributes != NULL && list->attributes->len > 0;
}

/* }}} */
/* {{{ Public API */

/**
 * pango_attr_list_new:
 *
 * Create a new empty attribute list with a reference
 * count of one.
 *
 * Return value: (transfer full): the newly allocated
 *   `PangoAttrList`, which should be freed with
 *   [method@Pango.AttrList.unref]
 */
PangoAttrList *
pango_attr_list_new (void)
{
  PangoAttrList *list = g_slice_new (PangoAttrList);

  pango_attr_list_init (list);

  return list;
}

/**
 * pango_attr_list_ref:
 * @list: (nullable): a `PangoAttrList`
 *
 * Increase the reference count of the given attribute
 * list by one.
 *
 * Return value: The attribute list passed in
 *
 * Since: 1.10
 */
PangoAttrList *
pango_attr_list_ref (PangoAttrList *list)
{
  if (list == NULL)
    return NULL;

  g_atomic_int_inc ((int *) &list->ref_count);

  return list;
}

/**
 * pango_attr_list_unref:
 * @list: (nullable): a `PangoAttrList`
 *
 * Decrease the reference count of the given attribute
 * list by one.
 *
 * If the result is zero, free the attribute list
 * and the attributes it contains.
 */
void
pango_attr_list_unref (PangoAttrList *list)
{
  if (list == NULL)
    return;

  g_return_if_fail (list->ref_count > 0);

  if (g_atomic_int_dec_and_test ((int *) &list->ref_count))
    {
      pango_attr_list_destroy (list);
      g_slice_free (PangoAttrList, list);
    }
}

/**
 * pango_attr_list_copy:
 * @list: (nullable): a `PangoAttrList`
 *
 * Copy @list and return an identical new list.
 *
 * Return value: (nullable): the newly allocated
 *   `PangoAttrList`, with a reference count of one,
 *   which should be freed with [method@Pango.AttrList.unref].
 *   Returns %NULL if @list was %NULL.
 */
PangoAttrList *
pango_attr_list_copy (PangoAttrList *list)
{
  PangoAttrList *new;

  if (list == NULL)
    return NULL;

  new = pango_attr_list_new ();
  if (!list->attributes || list->attributes->len == 0)
    return new;

  new->attributes = g_ptr_array_copy (list->attributes, (GCopyFunc)pango_attribute_copy, NULL);

  return new;
}

/**
 * pango_attr_list_insert:
 * @list: a `PangoAttrList`
 * @attr: (transfer full): the attribute to insert
 *
 * Insert the given attribute into the `PangoAttrList`.
 *
 * It will be inserted after all other attributes with a
 * matching @start_index.
 */
void
pango_attr_list_insert (PangoAttrList  *list,
                        PangoAttribute *attr)
{
  g_return_if_fail (list != NULL);
  g_return_if_fail (attr != NULL);

  pango_attr_list_insert_internal (list, attr, FALSE);
}

/**
 * pango_attr_list_insert_before:
 * @list: a `PangoAttrList`
 * @attr: (transfer full): the attribute to insert
 *
 * Insert the given attribute into the `PangoAttrList`.
 *
 * It will be inserted before all other attributes with a
 * matching @start_index.
 */
void
pango_attr_list_insert_before (PangoAttrList  *list,
                               PangoAttribute *attr)
{
  g_return_if_fail (list != NULL);
  g_return_if_fail (attr != NULL);

  pango_attr_list_insert_internal (list, attr, TRUE);
}

/**
 * pango_attr_list_change:
 * @list: a `PangoAttrList`
 * @attr: (transfer full): the attribute to insert
 *
 * Insert the given attribute into the `PangoAttrList`.
 *
 * It will replace any attributes of the same type
 * on that segment and be merged with any adjoining
 * attributes that are identical.
 *
 * This function is slower than [method@Pango.AttrList.insert]
 * for creating an attribute list in order (potentially
 * much slower for large lists). However,
 * [method@Pango.AttrList.insert] is not suitable for
 * continually changing a set of attributes since it
 * never removes or combines existing attributes.
 */
void
pango_attr_list_change (PangoAttrList  *list,
                        PangoAttribute *attr)
{
  guint i, p;
  guint start_index = attr->start_index;
  guint end_index = attr->end_index;
  gboolean inserted;

  g_return_if_fail (list != NULL);

  if (start_index == end_index) /* empty, nothing to do */
    {
      pango_attribute_destroy (attr);
      return;
    }

  if (!list->attributes || list->attributes->len == 0)
    {
      pango_attr_list_insert (list, attr);
      return;
    }

  inserted = FALSE;
  for (i = 0, p = list->attributes->len; i < p; i++)
    {
      PangoAttribute *tmp_attr = g_ptr_array_index (list->attributes, i);

      if (tmp_attr->start_index > start_index)
        {
          g_ptr_array_insert (list->attributes, i, attr);
          inserted = TRUE;
          break;
        }

      if (tmp_attr->type != attr->type)
        continue;

      if (tmp_attr->end_index < start_index)
        continue; /* This attr does not overlap with the new one */

      g_assert (tmp_attr->start_index <= start_index);
      g_assert (tmp_attr->end_index >= start_index);

      if (pango_attribute_equal (tmp_attr, attr))
        {
          /* We can merge the new attribute with this attribute
           */
          if (tmp_attr->end_index >= end_index)
            {
              /* We are totally overlapping the previous attribute.
               * No action is needed.
               */
              pango_attribute_destroy (attr);
              return;
            }

          tmp_attr->end_index = end_index;
          pango_attribute_destroy (attr);

          attr = tmp_attr;
          inserted = TRUE;
          break;
        }
      else
        {
          /* Split, truncate, or remove the old attribute
           */
          if (tmp_attr->end_index > end_index)
            {
              PangoAttribute *end_attr = pango_attribute_copy (tmp_attr);

              end_attr->start_index = end_index;
              pango_attr_list_insert (list, end_attr);
            }

          if (tmp_attr->start_index == start_index)
            {
              pango_attribute_destroy (tmp_attr);
              g_ptr_array_remove_index (list->attributes, i);
              break;
            }
          else
            {
              tmp_attr->end_index = start_index;
            }
        }
    }

  if (!inserted)
    /* we didn't insert attr yet */
    pango_attr_list_insert (list, attr);

  /* We now have the range inserted into the list one way or the
   * other. Fix up the remainder
   */
  /* Attention: No i = 0 here. */
  for (i = i + 1, p = list->attributes->len; i < p; i++)
    {
      PangoAttribute *tmp_attr = g_ptr_array_index (list->attributes, i);

      if (tmp_attr->start_index > end_index)
        break;

      if (tmp_attr->type != attr->type)
        continue;

      if (tmp_attr == attr)
        continue;

      if (tmp_attr->end_index <= attr->end_index ||
          pango_attribute_equal (tmp_attr, attr))
        {
          /* We can merge the new attribute with this attribute. */
          attr->end_index = MAX (end_index, tmp_attr->end_index);
          pango_attribute_destroy (tmp_attr);
          g_ptr_array_remove_index (list->attributes, i);
          i--;
          p--;
          continue;
        }
      else
        {
          /* Trim the start of this attribute that it begins at the end
           * of the new attribute. This may involve moving it in the list
           * to maintain the required non-decreasing order of start indices.
           */
          int k, m;

          tmp_attr->start_index = attr->end_index;

          for (k = i + 1, m = list->attributes->len; k < m; k++)
            {
              PangoAttribute *tmp_attr2 = g_ptr_array_index (list->attributes, k);

              if (tmp_attr2->start_index >= tmp_attr->start_index)
                break;

              g_ptr_array_index (list->attributes, k - 1) = tmp_attr2;
              g_ptr_array_index (list->attributes, k) = tmp_attr;
            }
        }
    }
}

/**
 * pango_attr_list_update:
 * @list: a `PangoAttrList`
 * @pos: the position of the change
 * @remove: the number of removed bytes
 * @add: the number of added bytes
 *
 * Update indices of attributes in @list for a change in the
 * text they refer to.
 *
 * The change that this function applies is removing @remove
 * bytes at position @pos and inserting @add bytes instead.
 *
 * Attributes that fall entirely in the (@pos, @pos + @remove)
 * range are removed.
 *
 * Attributes that start or end inside the (@pos, @pos + @remove)
 * range are shortened to reflect the removal.
 *
 * Attributes start and end positions are updated if they are
 * behind @pos + @remove.
 *
 * Since: 1.44
 */
void
pango_attr_list_update (PangoAttrList *list,
                        int             pos,
                        int             remove,
                        int             add)
{
  guint i, p;

  g_return_if_fail (pos >= 0);
  g_return_if_fail (remove >= 0);
  g_return_if_fail (add >= 0);

  if (list->attributes)
    for (i = 0, p = list->attributes->len; i < p; i++)
      {
        PangoAttribute *attr = g_ptr_array_index (list->attributes, i);

        if (attr->start_index >= pos &&
          attr->end_index < pos + remove)
          {
            pango_attribute_destroy (attr);
            g_ptr_array_remove_index (list->attributes, i);
            i--; /* Look at this index again */
            p--;
            continue;
          }

        if (attr->start_index != PANGO_ATTR_INDEX_FROM_TEXT_BEGINNING)
          {
            if (attr->start_index >= pos &&
                attr->start_index < pos + remove)
              {
                attr->start_index = pos + add;
              }
            else if (attr->start_index >= pos + remove)
              {
                attr->start_index += add - remove;
              }
          }

        if (attr->end_index != PANGO_ATTR_INDEX_TO_TEXT_END)
          {
            if (attr->end_index >= pos &&
                attr->end_index < pos + remove)
              {
                attr->end_index = pos;
              }
            else if (attr->end_index >= pos + remove)
              {
                if (add > remove &&
                    G_MAXUINT - attr->end_index < add - remove)
                  attr->end_index = G_MAXUINT;
                else
                  attr->end_index += add - remove;
              }
          }
      }
}

/**
 * pango_attr_list_splice:
 * @list: a `PangoAttrList`
 * @other: another `PangoAttrList`
 * @pos: the position in @list at which to insert @other
 * @len: the length of the spliced segment. (Note that this
 *   must be specified since the attributes in @other may only
 *   be present at some subsection of this range)
 *
 * This function opens up a hole in @list, fills it
 * in with attributes from the left, and then merges
 * @other on top of the hole.
 *
 * This operation is equivalent to stretching every attribute
 * that applies at position @pos in @list by an amount @len,
 * and then calling [method@Pango.AttrList.change] with a copy
 * of each attribute in @other in sequence (offset in position
 * by @pos, and limited in length to @len).
 *
 * This operation proves useful for, for instance, inserting
 * a pre-edit string in the middle of an edit buffer.
 *
 * For backwards compatibility, the function behaves differently
 * when @len is 0. In this case, the attributes from @other are
 * not imited to @len, and are just overlayed on top of @list.
 *
 * This mode is useful for merging two lists of attributes together.
 */
void
pango_attr_list_splice (PangoAttrList *list,
                        PangoAttrList *other,
                        gint           pos,
                        gint           len)
{
  guint i, p;
  guint upos, ulen;
  guint end;

  g_return_if_fail (list != NULL);
  g_return_if_fail (other != NULL);
  g_return_if_fail (pos >= 0);
  g_return_if_fail (len >= 0);

  upos = (guint)pos;
  ulen = (guint)len;

/* This definition only works when a and b are unsigned; overflow
 * isn't defined in the C standard for signed integers
 */
#define CLAMP_ADD(a,b) (((a) + (b) < (a)) ? G_MAXUINT : (a) + (b))

  end = CLAMP_ADD (upos, ulen);

  if (list->attributes)
    for (i = 0, p = list->attributes->len; i < p; i++)
      {
        PangoAttribute *attr = g_ptr_array_index (list->attributes, i);;

        if (attr->start_index <= upos)
          {
            if (attr->end_index > upos)
              attr->end_index = CLAMP_ADD (attr->end_index, ulen);
          }
        else
          {
            /* This could result in a zero length attribute if it
             * gets squashed up against G_MAXUINT, but deleting such
             * an element could (in theory) suprise the caller, so
             * we don't delete it.
             */
            attr->start_index = CLAMP_ADD (attr->start_index, ulen);
            attr->end_index = CLAMP_ADD (attr->end_index, ulen);
         }
      }

  if (!other->attributes || other->attributes->len == 0)
    return;

  for (i = 0, p = other->attributes->len; i < p; i++)
    {
      PangoAttribute *attr = pango_attribute_copy (g_ptr_array_index (other->attributes, i));
      if (ulen > 0)
        {
          attr->start_index = MIN (CLAMP_ADD (attr->start_index, upos), end);
          attr->end_index = MIN (CLAMP_ADD (attr->end_index, upos), end);
        }
      else
        {
          attr->start_index = CLAMP_ADD (attr->start_index, upos);
          attr->end_index = CLAMP_ADD (attr->end_index, upos);
        }

      /* Same as above, the attribute could be squashed to zero-length; here
       * pango_attr_list_change() will take care of deleting it.
       */
      pango_attr_list_change (list, attr);
    }
#undef CLAMP_ADD
}

/**
 * pango_attr_list_get_attributes:
 * @list: a `PangoAttrList`
 *
 * Gets a list of all attributes in @list.
 *
 * Return value: (element-type Pango.Attribute) (transfer full):
 *   a list of all attributes in @list. To free this value,
 *   call [method@Pango.Attribute.destroy] on each value and
 *   g_slist_free() on the list.
 *
 * Since: 1.44
 */
GSList *
pango_attr_list_get_attributes (PangoAttrList *list)
{
  GSList *result = NULL;
  guint i, p;

  g_return_val_if_fail (list != NULL, NULL);

  if (!list->attributes || list->attributes->len == 0)
    return NULL;

  for (i = 0, p = list->attributes->len; i < p; i++)
    {
      PangoAttribute *attr = g_ptr_array_index (list->attributes, i);

      result = g_slist_prepend (result, pango_attribute_copy (attr));
    }

  return g_slist_reverse (result);
}

/**
 * pango_attr_list_equal:
 * @list: a `PangoAttrList`
 * @other_list: the other `PangoAttrList`
 *
 * Checks whether @list and @other_list contain the same
 * attributes and whether those attributes apply to the
 * same ranges.
 *
 * Beware that this will return wrong values if any list
 * contains duplicates.
 *
 * Return value: %TRUE if the lists are equal, %FALSE if
 *   they aren't
 *
 * Since: 1.46
 */
gboolean
pango_attr_list_equal (PangoAttrList *list,
                       PangoAttrList *other_list)
{
  GPtrArray *attrs, *other_attrs;
  guint64 skip_bitmask = 0;
  guint i;

  if (list == other_list)
    return TRUE;

  if (list == NULL || other_list == NULL)
    return FALSE;

  if (list->attributes == NULL || other_list->attributes == NULL)
    return list->attributes == other_list->attributes;

  attrs = list->attributes;
  other_attrs = other_list->attributes;

  if (attrs->len != other_attrs->len)
    return FALSE;

  for (i = 0; i < attrs->len; i++)
    {
      PangoAttribute *attr = g_ptr_array_index (attrs, i);
      gboolean attr_equal = FALSE;
      guint other_attr_index;

      for (other_attr_index = 0; other_attr_index < other_attrs->len; other_attr_index++)
        {
          PangoAttribute *other_attr = g_ptr_array_index (other_attrs, other_attr_index);
          guint64 other_attr_bitmask = other_attr_index < 64 ? 1 << other_attr_index : 0;

          if ((skip_bitmask & other_attr_bitmask) != 0)
            continue;

          if (attr->start_index == other_attr->start_index &&
              attr->end_index == other_attr->end_index &&
              pango_attribute_equal (attr, other_attr))
            {
              skip_bitmask |= other_attr_bitmask;
              attr_equal = TRUE;
              break;
            }

        }

      if (!attr_equal)
        return FALSE;
    }

  return TRUE;
}

/**
 * pango_attr_list_filter:
 * @list: a `PangoAttrList`
 * @func: (scope call) (closure data): callback function;
 *   returns %TRUE if an attribute should be filtered out
 * @data: (closure): Data to be passed to @func
 *
 * Given a `PangoAttrList` and callback function, removes
 * any elements of @list for which @func returns %TRUE and
 * inserts them into a new list.
 *
 * Return value: (transfer full) (nullable): the new
 *   `PangoAttrList` or %NULL if no attributes of the
 *   given types were found
 *
 * Since: 1.2
 */
PangoAttrList *
pango_attr_list_filter (PangoAttrList       *list,
                        PangoAttrFilterFunc  func,
                        gpointer             data)

{
  PangoAttrList *new = NULL;
  guint i, p;

  g_return_val_if_fail (list != NULL, NULL);

  if (!list->attributes || list->attributes->len == 0)
    return NULL;

  for (i = 0, p = list->attributes->len; i < p; i++)
    {
      PangoAttribute *tmp_attr = g_ptr_array_index (list->attributes, i);

      if ((*func) (tmp_attr, data))
        {
          g_ptr_array_remove_index (list->attributes, i);
          i--; /* Need to look at this index again */
          p--;

          if (G_UNLIKELY (!new))
            {
              new = pango_attr_list_new ();
              new->attributes = g_ptr_array_new ();
            }

          g_ptr_array_add (new->attributes, tmp_attr);
        }
    }

  return new;
}

/* }}} */
/* {{{ Serialization */

/* We serialize attribute lists to strings. The format
 * is a comma-separated list of the attributes in the order
 * in which they are in the list, with each attribute having
 * this format:
 *
 * START END NICK VALUE
 *
 * Values that can contain a comma, such as font descriptions
 * are quoted with "".
 */

static GType
get_attr_value_type (PangoAttrType type)
{
  switch ((int)type)
    {
    case PANGO_ATTR_STYLE: return PANGO_TYPE_STYLE;
    case PANGO_ATTR_WEIGHT: return PANGO_TYPE_WEIGHT;
    case PANGO_ATTR_VARIANT: return PANGO_TYPE_VARIANT;
    case PANGO_ATTR_STRETCH: return PANGO_TYPE_STRETCH;
    case PANGO_ATTR_GRAVITY: return PANGO_TYPE_GRAVITY;
    case PANGO_ATTR_GRAVITY_HINT: return PANGO_TYPE_GRAVITY_HINT;
    case PANGO_ATTR_UNDERLINE: return PANGO_TYPE_LINE_STYLE;
    case PANGO_ATTR_OVERLINE: return PANGO_TYPE_OVERLINE;
    case PANGO_ATTR_BASELINE_SHIFT: return PANGO_TYPE_BASELINE_SHIFT;
    case PANGO_ATTR_FONT_SCALE: return PANGO_TYPE_FONT_SCALE;
    case PANGO_ATTR_TEXT_TRANSFORM: return PANGO_TYPE_TEXT_TRANSFORM;
    default: return G_TYPE_INVALID;
    }
}

static void
append_enum_value (GString *str,
                   GType    type,
                   int      value)
{
  GEnumClass *enum_class;
  GEnumValue *enum_value;

  enum_class = g_type_class_ref (type);
  enum_value = g_enum_get_value (enum_class, value);
  g_type_class_unref (enum_class);

  if (enum_value)
    g_string_append_printf (str, " %s", enum_value->value_nick);
  else
    g_string_append_printf (str, " %d", value);
}

static void
attr_print (GString        *str,
            PangoAttribute *attr)
{
  const char *name;

  name = pango_attr_type_get_name (attr->type);
  if (!name)
    return;

  g_string_append_printf (str, "%u %u %s", attr->start_index, attr->end_index, name);

  switch (PANGO_ATTR_VALUE_TYPE (attr))
    {
    case PANGO_ATTR_VALUE_INT:
      if (attr->type == PANGO_ATTR_WEIGHT ||
          attr->type == PANGO_ATTR_STYLE ||
          attr->type == PANGO_ATTR_STRETCH ||
          attr->type == PANGO_ATTR_VARIANT ||
          attr->type == PANGO_ATTR_GRAVITY ||
          attr->type == PANGO_ATTR_GRAVITY_HINT ||
          attr->type == PANGO_ATTR_UNDERLINE ||
          attr->type == PANGO_ATTR_OVERLINE ||
          attr->type == PANGO_ATTR_BASELINE_SHIFT ||
          attr->type == PANGO_ATTR_FONT_SCALE ||
          attr->type == PANGO_ATTR_TEXT_TRANSFORM)
        append_enum_value (str, get_attr_value_type (attr->type), attr->int_value);
      else
        g_string_append_printf (str, " %d", attr->int_value);
      break;

    case PANGO_ATTR_VALUE_BOOLEAN:
      g_string_append (str, attr->int_value ? " true" : " false");
      break;

    case PANGO_ATTR_VALUE_STRING:
      g_string_append_printf (str, " \"%s\"", attr->str_value);
      break;

    case PANGO_ATTR_VALUE_LANGUAGE:
      g_string_append_printf (str, " %s", pango_language_to_string (attr->lang_value));
      break;

    case PANGO_ATTR_VALUE_FLOAT:
      {
        char buf[20];
        g_ascii_formatd (buf, 20, "%f", attr->double_value);
        g_string_append_printf (str, " %s", buf);
      }
      break;

    case PANGO_ATTR_VALUE_FONT_DESC:
      {
        char *s = pango_font_description_to_string (attr->font_value);
        g_string_append_printf (str, " \"%s\"", s);
        g_free (s);
      }
      break;

    case PANGO_ATTR_VALUE_COLOR:
      {
        char *s = pango_color_to_string (&attr->color_value);
        g_string_append_printf (str, " %s", s);
        g_free (s);
      }
      break;

    case PANGO_ATTR_VALUE_POINTER:
      {
        char *s = pango_attr_value_serialize (attr);
        g_string_append_printf (str, " %s", s);
        g_free (s);
      }
      break;

    default:
      g_assert_not_reached ();
    }
}

/**
 * pango_attr_list_to_string:
 * @list: a `PangoAttrList`
 *
 * Serializes a `PangoAttrList` to a string.
 *
 * No guarantees are made about the format of the string,
 * it may change between Pango versions.
 *
 * The intended use of this function is testing and
 * debugging. The format is not meant as a permanent
 * storage format.
 *
 * Returns: (transfer full): a newly allocated string
 * Since: 1.50
 */
char *
pango_attr_list_to_string (PangoAttrList *list)
{
  GString *s;

  s = g_string_new ("");

  if (list->attributes)
    for (int i = 0; i < list->attributes->len; i++)
      {
        PangoAttribute *attr = g_ptr_array_index (list->attributes, i);

        if (i > 0)
          g_string_append (s, "\n");
        attr_print (s, attr);
      }

  return g_string_free (s, FALSE);
}

static PangoAttrType
get_attr_type_by_nick (const char *nick,
                       int         len)
{
  GEnumClass *enum_class;

  enum_class = g_type_class_ref (pango_attr_type_get_type ());
  for (GEnumValue *ev = enum_class->values; ev->value_name; ev++)
    {
      if (ev->value_nick && strncmp (ev->value_nick, nick, len) == 0)
        {
          g_type_class_unref (enum_class);
          return (PangoAttrType) ev->value;
        }
    }

  g_type_class_unref (enum_class);
  return PANGO_ATTR_INVALID;
}

static int
get_attr_value (PangoAttrType  type,
                const char    *str,
                int            len)
{
  GEnumClass *enum_class;
  char *endp;
  int value;

  enum_class = g_type_class_ref (get_attr_value_type (type));
  for (GEnumValue *ev = enum_class->values; ev->value_name; ev++)
    {
      if (ev->value_nick && strncmp (ev->value_nick, str, len) == 0)
        {
          g_type_class_unref (enum_class);
          return ev->value;
        }
    }
  g_type_class_unref (enum_class);

  value = g_ascii_strtoll (str, &endp, 10);
  if (endp - str == len)
    return value;

  return -1;
}

static gboolean
is_valid_end_char (char c)
{
  return c == ',' || c == '\n' || c == '\0';
}

/**
 * pango_attr_list_from_string:
 * @text: a string
 *
 * Deserializes a `PangoAttrList` from a string.
 *
 * This is the counterpart to [method@Pango.AttrList.to_string].
 * See that functions for details about the format.
 *
 * Returns: (transfer full) (nullable): a new `PangoAttrList`
 * Since: 1.50
 */
PangoAttrList *
pango_attr_list_from_string (const char *text)
{
  PangoAttrList *list;
  const char *p;

  g_return_val_if_fail (text != NULL, NULL);

  list = pango_attr_list_new ();

  if (*text == '\0')
    return list;

  list->attributes = g_ptr_array_new ();

  p = text + strspn (text, " \t\n");
  while (*p)
    {
      char *endp;
      gint64 start_index;
      gint64 end_index;
      char *str;
      PangoAttrType attr_type;
      PangoAttribute *attr;
      PangoLanguage *lang;
      gint64 integer;
      PangoFontDescription *desc;
      PangoColor color;
      double num;

      start_index = g_ascii_strtoll (p, &endp, 10);
      if (*endp != ' ')
        goto fail;

      p = endp + strspn (endp, " ");
      if (!*p)
        goto fail;

      end_index = g_ascii_strtoll (p, &endp, 10);
      if (*endp != ' ')
        goto fail;

      p = endp + strspn (endp, " ");

      endp = (char *)p + strcspn (p, " ");
      attr_type = get_attr_type_by_nick (p, endp - p);

      p = endp + strspn (endp, " ");
      if (*p == '\0')
        goto fail;

#define INT_ATTR(name,type) \
          integer = g_ascii_strtoll (p, &endp, 10); \
          if (!is_valid_end_char (*endp)) goto fail; \
          attr = pango_attr_##name##_new ((type)integer);

#define BOOLEAN_ATTR(name,type) \
          if (strncmp (p, "true", strlen ("true")) == 0) \
            { \
              integer = 1; \
              endp = (char *)(p + strlen ("true")); \
            } \
          else if (strncmp (p, "false", strlen ("false")) == 0) \
            { \
              integer = 0; \
              endp = (char *)(p + strlen ("false")); \
            } \
          else \
            integer = g_ascii_strtoll (p, &endp, 10); \
          if (!is_valid_end_char (*endp)) goto fail; \
          attr = pango_attr_##name##_new ((type)integer);

#define ENUM_ATTR(name, type, min, max) \
          endp = (char *)p + strcspn (p, ",\n"); \
          integer = get_attr_value (attr_type, p, endp - p); \
          attr = pango_attr_##name##_new ((type) CLAMP (integer, min, max));

#define FLOAT_ATTR(name) \
          num = g_ascii_strtod (p, &endp); \
          if (!is_valid_end_char (*endp)) goto fail; \
          attr = pango_attr_##name##_new ((float)num);

#define COLOR_ATTR(name) \
          endp = (char *)p + strcspn (p, ",\n"); \
          if (!is_valid_end_char (*endp)) goto fail; \
          str = g_strndup (p, endp - p); \
          if (!pango_color_parse (&color, str)) \
            { \
              g_free (str); \
              goto fail; \
            } \
          attr = pango_attr_##name##_new (&color); \
          g_free (str);

      switch (attr_type)
        {
        case PANGO_ATTR_INVALID:
          pango_attr_list_unref (list);
          return NULL;

        case PANGO_ATTR_LANGUAGE:
          endp = (char *)p + strcspn (p, ",\n");
          if (!is_valid_end_char (*endp)) goto fail;
          str = g_strndup (p, endp - p);
          lang = pango_language_from_string (str);
          attr = pango_attr_language_new (lang);
          g_free (str);
          break;

        case PANGO_ATTR_FAMILY:
          p++;
          endp = strchr (p, '"');
          if (!endp) goto fail;
          str = g_strndup (p, endp - p);
          attr = pango_attr_family_new (str);
          g_free (str);
          endp++;
          if (!is_valid_end_char (*endp)) goto fail;
          break;

        case PANGO_ATTR_STYLE:
          ENUM_ATTR(style, PangoStyle, PANGO_STYLE_NORMAL, PANGO_STYLE_ITALIC);
          break;

        case PANGO_ATTR_WEIGHT:
          ENUM_ATTR(weight, PangoWeight, PANGO_WEIGHT_THIN, PANGO_WEIGHT_ULTRAHEAVY);
          break;

        case PANGO_ATTR_VARIANT:
          ENUM_ATTR(variant, PangoVariant, PANGO_VARIANT_NORMAL, PANGO_VARIANT_TITLE_CAPS);
          break;

        case PANGO_ATTR_STRETCH:
          ENUM_ATTR(stretch, PangoStretch, PANGO_STRETCH_ULTRA_CONDENSED, PANGO_STRETCH_ULTRA_EXPANDED);
          break;

        case PANGO_ATTR_SIZE:
          INT_ATTR(size, int);
          break;

        case PANGO_ATTR_FONT_DESC:
          p++;
          endp = strchr (p, '"');
          if (!endp) goto fail;
          str = g_strndup (p, endp - p);
          desc = pango_font_description_from_string (str);
          attr = pango_attr_font_desc_new (desc);
          pango_font_description_free (desc);
          g_free (str);
          endp++;
          if (!is_valid_end_char (*endp)) goto fail;
          break;

        case PANGO_ATTR_FOREGROUND:
          COLOR_ATTR(foreground);
          break;

        case PANGO_ATTR_BACKGROUND:
          COLOR_ATTR(background);
          break;

        case PANGO_ATTR_UNDERLINE:
          ENUM_ATTR(underline, PangoLineStyle, PANGO_LINE_STYLE_NONE, PANGO_LINE_STYLE_DOTTED);
          break;

        case PANGO_ATTR_UNDERLINE_POSITION:
          ENUM_ATTR(underline_position, PangoUnderlinePosition, PANGO_UNDERLINE_POSITION_NORMAL, PANGO_UNDERLINE_POSITION_UNDER);
          break;

        case PANGO_ATTR_STRIKETHROUGH:
          BOOLEAN_ATTR(strikethrough, gboolean);
          break;

        case PANGO_ATTR_RISE:
          INT_ATTR(rise, int);
          break;

        case PANGO_ATTR_SCALE:
          FLOAT_ATTR(scale);
          break;

        case PANGO_ATTR_FALLBACK:
          BOOLEAN_ATTR(fallback, gboolean);
          break;

        case PANGO_ATTR_LETTER_SPACING:
          INT_ATTR(letter_spacing, int);
          break;

        case PANGO_ATTR_UNDERLINE_COLOR:
          COLOR_ATTR(underline_color);
          break;

        case PANGO_ATTR_STRIKETHROUGH_COLOR:
          COLOR_ATTR(strikethrough_color);
          break;

        case PANGO_ATTR_ABSOLUTE_SIZE:
          integer = g_ascii_strtoll (p, &endp, 10);
          if (!is_valid_end_char (*endp)) goto fail;
          attr = pango_attr_size_new_absolute (integer);
          break;

        case PANGO_ATTR_GRAVITY:
          ENUM_ATTR(gravity, PangoGravity, PANGO_GRAVITY_SOUTH, PANGO_GRAVITY_WEST);
          break;

        case PANGO_ATTR_FONT_FEATURES:
          p++;
          endp = strchr (p, '"');
          if (!endp) goto fail;
          str = g_strndup (p, endp - p);
          attr = pango_attr_font_features_new (str);
          g_free (str);
          endp++;
          if (!is_valid_end_char (*endp)) goto fail;
          break;

        case PANGO_ATTR_GRAVITY_HINT:
          ENUM_ATTR(gravity_hint, PangoGravityHint, PANGO_GRAVITY_HINT_NATURAL, PANGO_GRAVITY_HINT_LINE);
          break;

        case PANGO_ATTR_FOREGROUND_ALPHA:
          INT_ATTR(foreground_alpha, int);
          break;

        case PANGO_ATTR_BACKGROUND_ALPHA:
          INT_ATTR(background_alpha, int);
          break;

        case PANGO_ATTR_ALLOW_BREAKS:
          BOOLEAN_ATTR(allow_breaks, gboolean);
          break;

        case PANGO_ATTR_SHOW:
          INT_ATTR(show, PangoShowFlags);
          break;

        case PANGO_ATTR_INSERT_HYPHENS:
          BOOLEAN_ATTR(insert_hyphens, gboolean);
          break;

        case PANGO_ATTR_OVERLINE:
          ENUM_ATTR(overline, PangoOverline, PANGO_OVERLINE_NONE, PANGO_OVERLINE_SINGLE);
          break;

        case PANGO_ATTR_OVERLINE_COLOR:
          COLOR_ATTR(overline_color);
          break;

        case PANGO_ATTR_LINE_HEIGHT:
          FLOAT_ATTR(line_height);
          break;

        case PANGO_ATTR_ABSOLUTE_LINE_HEIGHT:
          integer = g_ascii_strtoll (p, &endp, 10);
          if (!is_valid_end_char (*endp)) goto fail;
          attr = pango_attr_line_height_new_absolute (integer);
          break;

        case PANGO_ATTR_TEXT_TRANSFORM:
          ENUM_ATTR(text_transform, PangoTextTransform, PANGO_TEXT_TRANSFORM_NONE, PANGO_TEXT_TRANSFORM_CAPITALIZE);
          break;

        case PANGO_ATTR_WORD:
          integer = g_ascii_strtoll (p, &endp, 10);
          if (!is_valid_end_char (*endp)) goto fail;
          attr = pango_attr_word_new ();
          break;

        case PANGO_ATTR_SENTENCE:
          integer = g_ascii_strtoll (p, &endp, 10);
          if (!is_valid_end_char (*endp)) goto fail;
          attr = pango_attr_sentence_new ();
          break;

        case PANGO_ATTR_PARAGRAPH:
          integer = g_ascii_strtoll (p, &endp, 10);
          if (!is_valid_end_char (*endp)) goto fail;
          attr = pango_attr_paragraph_new ();
          break;

        case PANGO_ATTR_BASELINE_SHIFT:
          ENUM_ATTR(baseline_shift, PangoBaselineShift, 0, G_MAXINT);
          break;

        case PANGO_ATTR_FONT_SCALE:
          ENUM_ATTR(font_scale, PangoFontScale, PANGO_FONT_SCALE_NONE, PANGO_FONT_SCALE_SMALL_CAPS);
          break;

        case PANGO_ATTR_LINE_SPACING:
          INT_ATTR(line_spacing, int);
          break;

        default:
          g_assert_not_reached ();
        }

      attr->start_index = (guint)start_index;
      attr->end_index = (guint)end_index;
      g_ptr_array_add (list->attributes, attr);

      p = endp;
      if (*p)
        {
          if (*p == ',')
            p++;
          p += strspn (p, " \n");
        }
    }

  goto success;

fail:
  pango_attr_list_unref (list);
  list = NULL;

success:
  return list;
}

/* }}} */

/* vim:set foldmethod=marker expandtab: */
