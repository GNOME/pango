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

#include <pango-attributes.h>
#include <pango-item.h>

/**
 * pango_item_new:
 * 
 * Creates a new #PangoItem structure initialized to default values.
 * 
 * Return value: the new #PangoItem
 **/
PangoItem *
pango_item_new (void)
{
  PangoItem *result = g_new0 (PangoItem, 1);

  return result;
}

/**
 * pango_item_copy:
 * @item: a #PangoItem
 * 
 * Copy an existing #PangoItem structure.
 * 
 * Return value: the new #PangoItem
 **/
PangoItem *
pango_item_copy (PangoItem *item)
{
  GSList *extra_attrs, *tmp_list;
  PangoItem *result = g_new (PangoItem, 1);

  result->offset = item->offset;
  result->length = item->length;
  result->num_chars = item->num_chars;

  extra_attrs = NULL;
  tmp_list = item->extra_attrs;
  while (tmp_list)
    {
      extra_attrs = g_slist_prepend (extra_attrs, pango_attribute_copy (tmp_list->data));
      tmp_list = tmp_list->next;
    }

  result->extra_attrs = g_slist_reverse (extra_attrs);
	  
  result->analysis = item->analysis;
  g_object_ref (G_OBJECT (result->analysis.font));

  return result;
}

/**
 * pango_item_free:
 * @item: a #PangoItem
 * 
 * Free a #PangoItem and all associated memory.
 **/
void
pango_item_free (PangoItem *item)
{
  if (item->extra_attrs)
    g_slist_foreach (item->extra_attrs, (GFunc)pango_attribute_destroy, NULL);
  
  g_object_unref (G_OBJECT (item->analysis.font));

  g_free (item);
}

