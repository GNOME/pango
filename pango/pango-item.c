/* Pango
 * pango-item.c: Single run handling
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
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.	 See the GNU
 * Library General Public License for more details.
 *
 * You should have received a copy of the GNU Library General Public
 * License along with this library; if not, write to the
 * Free Software Foundation, Inc., 59 Temple Place - Suite 330,
 * Boston, MA 02111-1307, USA.
 */

#include "config.h"
#include "pango-attributes.h"
#include "pango-item.h"
#include "pango-impl-utils.h"

/**
 * pango_item_new:
 *
 * Creates a new #PangoItem structure initialized to default values.
 *
 * Return value: the newly allocated #PangoItem, which should
 *               be freed with pango_item_free().
 **/
PangoItem *
pango_item_new (void)
{
  PangoItem *result = g_slice_new0 (PangoItem);

  return result;
}

/**
 * pango_item_copy:
 * @item: a #PangoItem, may be %NULL
 *
 * Copy an existing #PangoItem structure.
 *
 * Return value: the newly allocated #PangoItem, which should
 *               be freed with pango_item_free(), or %NULL if
 *               @item was NULL.
 **/
PangoItem *
pango_item_copy (PangoItem *item)
{
  GSList *extra_attrs, *tmp_list;
  PangoItem *result;

  if (item == NULL)
    return NULL;
  
  result = g_slice_new (PangoItem);

  result->offset = item->offset;
  result->length = item->length;
  result->num_chars = item->num_chars;

  result->analysis = item->analysis;
  if (result->analysis.font)
    g_object_ref (result->analysis.font);

  extra_attrs = NULL;
  tmp_list = item->analysis.extra_attrs;
  while (tmp_list)
    {
      extra_attrs = g_slist_prepend (extra_attrs, pango_attribute_copy (tmp_list->data));
      tmp_list = tmp_list->next;
    }

  result->analysis.extra_attrs = g_slist_reverse (extra_attrs);

  return result;
}

/**
 * pango_item_free:
 * @item: a #PangoItem, may be %NULL
 *
 * Free a #PangoItem and all associated memory.
 **/
void
pango_item_free (PangoItem *item)
{
  if (item == NULL)
    return;

  if (item->analysis.extra_attrs)
    {
      g_slist_foreach (item->analysis.extra_attrs, (GFunc)pango_attribute_destroy, NULL);
      g_slist_free (item->analysis.extra_attrs);
    }

  if (item->analysis.font)
    g_object_unref (item->analysis.font);

  g_slice_free (PangoItem, item);
}

GType
pango_item_get_type (void)
{
  static GType our_type = 0;

  if (G_UNLIKELY (our_type == 0))
    our_type = g_boxed_type_register_static (I_("PangoItem"),
					     (GBoxedCopyFunc) pango_item_copy,
					     (GBoxedFreeFunc) pango_item_free);
  return our_type;
}

/**
 * pango_item_split:
 * @orig: a #PangoItem
 * @split_index: byte index of position to split item, relative to the start of the item
 * @split_offset: number of chars between start of @orig and @split_index
 *
 * Modifies @orig to cover only the text after @split_index, and
 * returns a new item that covers the text before @split_index that
 * used to be in @orig. You can think of @split_index as the length of
 * the returned item. @split_index may not be 0, and it may not be
 * greater than or equal to the length of @orig (that is, there must
 * be at least one byte assigned to each item, you can't create a
 * zero-length item). @split_offset is the length of the first item in
 * chars, and must be provided because the text used to generate the
 * item isn't available, so pango_item_split() can't count the char
 * length of the split items itself.
 *
 * Return value: new item representing text before @split_index, which
 *               should be freed with pango_item_free().
 **/
PangoItem*
pango_item_split (PangoItem  *orig,
		  int         split_index,
		  int         split_offset)
{
  PangoItem *new_item;

  g_return_val_if_fail (orig != NULL, NULL);
  g_return_val_if_fail (split_index > 0, NULL);
  g_return_val_if_fail (split_index < orig->length, NULL);
  g_return_val_if_fail (split_offset > 0, NULL);
  g_return_val_if_fail (split_offset < orig->num_chars, NULL);

  new_item = pango_item_copy (orig);
  new_item->length = split_index;
  new_item->num_chars = split_offset;

  orig->offset += split_index;
  orig->length -= split_index;
  orig->num_chars -= split_offset;

  return new_item;
}
