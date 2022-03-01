/* Pango
 * reorder-items.c:
 *
 * Copyright (C) 1999 Red Hat Software
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
#include "pango-item-private.h"

/* Inspired by https://github.com/fribidi/linear-reorder.git */

struct _PangoItemRange
{
  int level;
  GList *left;
  GList *right;
  PangoItemRange *previous;
  PangoItemRange *left_range;
  PangoItemRange *right_range;
};

static PangoItemRange *
merge_range_with_previous (PangoItemRange *range)
{
  PangoItemRange *previous = range->previous;
  PangoItemRange *left_range, *right_range;

  g_assert (range->previous != NULL);
  g_assert (previous->level < range->level);

  if (previous->level % 2)
    {
      left_range = range;
      right_range = previous;
    }
  else
    {
      left_range = previous;
      right_range = range;
    }

  previous->left = left_range->left;
  previous->right = right_range->right;
  previous->left_range = left_range->left_range;
  previous->right_range = right_range->right_range;

  g_free (range);

  return previous;
}

/*< private >
 * pango_item_range_add:
 * @range: (nullable): `PangoItemRange` to add @item to, or `NULL`
 * @item: `PangoItem` to add to @range
 *
 * Adds an item to a `PangoItemRange`.
 *
 * The `PangoItemRange` is an auxiliary structure to help
 * with ordering items in visual order.
 */
PangoItemRange *
pango_item_range_add (PangoItemRange *range,
                      PangoItem      *item)
{
  GList *run;

  run = g_list_append (NULL, item);

  while (range && range->level > item->analysis.level &&
         range->previous && range->previous->level >= item->analysis.level)
    range = merge_range_with_previous (range);

  if (range && range->level >= item->analysis.level)
    {
      if (item->analysis.level % 2)
        {
          run->next = range->left;
          run->next->prev = run;
          range->left = run;

          if (range->left_range)
            {
              range->left->prev = range->left_range->right;
              range->left->prev->next = range->left;
            }
        }
      else
        {
          range->right->next = run;
          run->prev = range->right;
          range->right = run;

          if (range->right_range)
            {
              range->right->next = range->right_range->left;
              range->right->next->prev = range->right;
            }
        }

      range->level = item->analysis.level;
    }
  else
    {
      PangoItemRange *new_range = g_new (PangoItemRange, 1);

      new_range->left = new_range->right = run;
      new_range->level = item->analysis.level;

      if (!range)
        {
          new_range->left_range = NULL;
          new_range->right_range = NULL;
        }
      else if (range->level % 2)
        {
          new_range->left_range = range->left_range;
          new_range->right_range = range;
          if (new_range->left_range)
            new_range->left_range->right_range = new_range;
          range->left_range = new_range;
        }
      else
        {
          new_range->left_range = range;
          new_range->right_range = range->right_range;
          if (new_range->right_range)
            new_range->right_range->left_range = new_range;
          range->right_range = new_range;
        }

      if (new_range->left_range)
        {
          new_range->left->prev = new_range->left_range->right;
          new_range->left->prev->next = new_range->left;
        }

      if (new_range->right_range)
        {
          new_range->right->next = new_range->right_range->left;
          new_range->right->next->prev = new_range->right;
        }

      new_range->previous = range;
      range = new_range;
    }

  return range;
}

/*< private >
 * pango_item_range_free:
 * @range: a `PangoItemRange`
 *
 * Frees @range and returns the items that were
 * added to it in visual order.
 *
 * Returns: (transfer container): a list with
 *   the items of @range, in visual order
 */
GList *
pango_item_range_free (PangoItemRange *range)
{
  GList *list;

  while (range->previous)
    range = merge_range_with_previous (range);

  list = range->left;

  g_free (range);

  return list;
}

/*< private >
 * pango_item_range_get_items:
 * @range: a `PangoItemRange`
 *
 * Returns a list with the items that have been
 * added to @range, in visual order.
 *
 * Returns: (transfer none): The items of @range,
 *   in visual order
 */
GList *
pango_item_range_get_items (PangoItemRange *range)
{
  while (range->left_range)
    range = range->left_range;

  return range->left;
}

/**
 * pango_reorder_items:
 * @items: (element-type Pango.Item): a `GList` of `PangoItem`
 *   in logical order.
 *
 * Reorder items from logical order to visual order.
 *
 * The visual order is determined from the associated directional
 * levels of the items. The original list is unmodified.
 *
 * (Please open a bug if you use this function.
 *  It is not a particularly convenient interface, and the code
 *  is duplicated elsewhere in Pango for that reason.)
 *
 * Returns: (transfer full) (element-type Pango.Item): a `GList`
 *   of `PangoItem` structures in visual order.
 */
GList *
pango_reorder_items (GList *items)
{
  PangoItemRange *range = NULL;

  if (!items)
    return NULL;

  for (GList *l = items; l; l = l->next)
    range = pango_item_range_add (range, (PangoItem *)l->data);

  return pango_item_range_free (range);
}
