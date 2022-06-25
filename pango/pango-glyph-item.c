/* Pango2
 * pango-glyph-item.c: Pair of Pango2Item and a glyph string
 *
 * Copyright (C) 2002 Red Hat Software
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

#include "pango-glyph-item-private.h"
#include "pango-glyph-iter-private.h"
#include "pango-item-private.h"
#include "pango-impl-utils.h"
#include "pango-attr-list-private.h"
#include "pango-attr-iterator-private.h"

#define LTR(glyph_item) (((glyph_item)->item->analysis.level % 2) == 0)

/**
 * pango2_glyph_item_split:
 * @orig: a `Pango2Item`
 * @text: text to which positions in @orig apply
 * @split_index: byte index of position to split item, relative to the
 *   start of the item
 *
 * Modifies @orig to cover only the text after @split_index, and
 * returns a new item that covers the text before @split_index that
 * used to be in @orig.
 *
 * You can think of @split_index as the length of the returned item.
 * @split_index may not be 0, and it may not be greater than or equal
 * to the length of @orig (that is, there must be at least one byte
 * assigned to each item, you can't create a zero-length item).
 *
 * Return value: the newly allocated item representing text before
 *   @split_index, which should be freed with [method@Pango2.GlyphItem.free]
 */
Pango2GlyphItem *
pango2_glyph_item_split (Pango2GlyphItem *orig,
                         const char      *text,
                         int              split_index)
{
  Pango2GlyphItem *new;
  int i;
  int num_glyphs;
  int num_remaining;
  int split_offset;

  g_return_val_if_fail (orig != NULL, NULL);
  g_return_val_if_fail (orig->item->length > 0, NULL);
  g_return_val_if_fail (split_index > 0, NULL);
  g_return_val_if_fail (split_index < orig->item->length, NULL);

  if (LTR (orig))
    {
      for (i = 0; i < orig->glyphs->num_glyphs; i++)
        {
          if (orig->glyphs->log_clusters[i] >= split_index)
            break;
        }

      if (i == orig->glyphs->num_glyphs) /* No splitting necessary */
        return NULL;

      split_index = orig->glyphs->log_clusters[i];
      num_glyphs = i;
    }
  else
    {
      for (i = orig->glyphs->num_glyphs - 1; i >= 0; i--)
        {
          if (orig->glyphs->log_clusters[i] >= split_index)
            break;
        }

      if (i < 0) /* No splitting necessary */
        return NULL;

      split_index = orig->glyphs->log_clusters[i];
      num_glyphs = orig->glyphs->num_glyphs - 1 - i;
    }

  num_remaining = orig->glyphs->num_glyphs - num_glyphs;

  new = g_slice_new (Pango2GlyphItem);
  split_offset = g_utf8_pointer_to_offset (text + orig->item->offset,
                                           text + orig->item->offset + split_index);
  new->item = pango2_item_split (orig->item, split_index, split_offset);

  new->glyphs = pango2_glyph_string_new ();
  pango2_glyph_string_set_size (new->glyphs, num_glyphs);

  if (LTR (orig))
    {
      memcpy (new->glyphs->glyphs, orig->glyphs->glyphs, num_glyphs * sizeof (Pango2GlyphInfo));
      memcpy (new->glyphs->log_clusters, orig->glyphs->log_clusters, num_glyphs * sizeof (int));

      memmove (orig->glyphs->glyphs, orig->glyphs->glyphs + num_glyphs,
               num_remaining * sizeof (Pango2GlyphInfo));
      for (i = num_glyphs; i < orig->glyphs->num_glyphs; i++)
        orig->glyphs->log_clusters[i - num_glyphs] = orig->glyphs->log_clusters[i] - split_index;
    }
  else
    {
      memcpy (new->glyphs->glyphs, orig->glyphs->glyphs + num_remaining, num_glyphs * sizeof (Pango2GlyphInfo));
      memcpy (new->glyphs->log_clusters, orig->glyphs->log_clusters + num_remaining, num_glyphs * sizeof (int));

      for (i = 0; i < num_remaining; i++)
        orig->glyphs->log_clusters[i] = orig->glyphs->log_clusters[i] - split_index;
    }

  pango2_glyph_string_set_size (orig->glyphs, orig->glyphs->num_glyphs - num_glyphs);

  new->y_offset = orig->y_offset;
  new->start_x_offset = orig->start_x_offset;
  new->end_x_offset = -orig->start_x_offset;

  return new;
}

/**
 * pango2_glyph_item_copy:
 * @orig: (nullable): a `Pango2GlyphItem`
 *
 * Make a deep copy of an existing `Pango2GlyphItem` structure.
 *
 * Return value: (nullable): the newly allocated `Pango2GlyphItem`
 */
Pango2GlyphItem *
pango2_glyph_item_copy (Pango2GlyphItem *orig)
{
  Pango2GlyphItem *result;

  if (orig == NULL)
    return NULL;

  result = g_slice_new (Pango2GlyphItem);

  result->item = pango2_item_copy (orig->item);
  result->glyphs = pango2_glyph_string_copy (orig->glyphs);
  result->y_offset = orig->y_offset;
  result->start_x_offset = orig->start_x_offset;
  result->end_x_offset = orig->end_x_offset;

  return result;
}

/**
 * pango2_glyph_item_free:
 * @glyph_item: (nullable): a `Pango2GlyphItem`
 *
 * Frees a `Pango2GlyphItem` and resources to which it points.
 */
void
pango2_glyph_item_free (Pango2GlyphItem *glyph_item)
{
  if (glyph_item == NULL)
    return;

  if (glyph_item->item)
    pango2_item_free (glyph_item->item);
  if (glyph_item->glyphs)
    pango2_glyph_string_free (glyph_item->glyphs);

  g_slice_free (Pango2GlyphItem, glyph_item);
}

G_DEFINE_BOXED_TYPE (Pango2GlyphItem, pango2_glyph_item,
                     pango2_glyph_item_copy,
                     pango2_glyph_item_free);


/**
 * pango2_glyph_item_iter_copy:
 * @orig: (nullable): a `Pango2GlyphItem`Iter
 *
 * Make a shallow copy of an existing `Pango2GlyphItemIter` structure.
 *
 * Return value: (nullable): the newly allocated `Pango2GlyphItemIter`
 */
Pango2GlyphItemIter *
pango2_glyph_item_iter_copy (Pango2GlyphItemIter *orig)
{
  Pango2GlyphItemIter *result;

  if (orig == NULL)
    return NULL;

  result = g_slice_new (Pango2GlyphItemIter);

  *result = *orig;

  return result;
}

/**
 * pango2_glyph_item_iter_free:
 * @iter: (nullable): a `Pango2GlyphItemIter`
 *
 * Frees a `Pango2GlyphItemIter`.
 */
void
pango2_glyph_item_iter_free (Pango2GlyphItemIter *iter)
{
  if (iter == NULL)
    return;

  g_slice_free (Pango2GlyphItemIter, iter);
}

G_DEFINE_BOXED_TYPE (Pango2GlyphItemIter, pango2_glyph_item_iter,
                     pango2_glyph_item_iter_copy,
                     pango2_glyph_item_iter_free)

/**
 * pango2_glyph_item_iter_next_cluster:
 * @iter: a `Pango2GlyphItemIter`
 *
 * Advances the iterator to the next cluster in the glyph item.
 *
 * See `Pango2GlyphItemIter` for details of cluster orders.
 *
 * Return value: %TRUE if the iterator was advanced,
 *   %FALSE if we were already on the  last cluster.
 */
gboolean
pango2_glyph_item_iter_next_cluster (Pango2GlyphItemIter *iter)
{
  int glyph_index = iter->end_glyph;
  Pango2GlyphString *glyphs = iter->glyph_item->glyphs;
  int cluster;
  Pango2Item *item = iter->glyph_item->item;

  if (LTR (iter->glyph_item))
    {
      if (glyph_index == glyphs->num_glyphs)
        return FALSE;
    }
  else
    {
      if (glyph_index < 0)
        return FALSE;
    }

  iter->start_glyph = iter->end_glyph;
  iter->start_index = iter->end_index;
  iter->start_char = iter->end_char;

  if (LTR (iter->glyph_item))
    {
      cluster = glyphs->log_clusters[glyph_index];
      while (TRUE)
        {
          glyph_index++;

          if (glyph_index == glyphs->num_glyphs)
            {
              iter->end_index = item->offset + item->length;
              iter->end_char = item->num_chars;
              break;
            }

          if (glyphs->log_clusters[glyph_index] > cluster)
            {
              iter->end_index = item->offset + glyphs->log_clusters[glyph_index];
              iter->end_char += pango2_utf8_strlen (iter->text + iter->start_index,
                                                    iter->end_index - iter->start_index);
              break;
            }
        }
    }
  else                  /* RTL */
    {
      cluster = glyphs->log_clusters[glyph_index];
      while (TRUE)
        {
          glyph_index--;

          if (glyph_index < 0)
            {
              iter->end_index = item->offset + item->length;
              iter->end_char = item->num_chars;
              break;
            }

          if (glyphs->log_clusters[glyph_index] > cluster)
            {
              iter->end_index = item->offset + glyphs->log_clusters[glyph_index];
              iter->end_char += pango2_utf8_strlen (iter->text + iter->start_index,
                                                    iter->end_index - iter->start_index);
              break;
            }
        }
    }

  iter->end_glyph = glyph_index;

  g_assert (iter->start_char <= iter->end_char);
  g_assert (iter->end_char <= item->num_chars);

  return TRUE;
}

/**
 * pango2_glyph_item_iter_prev_cluster:
 * @iter: a `Pango2GlyphItemIter`
 *
 * Moves the iterator to the preceding cluster in the glyph item.
 * See `Pango2GlyphItemIter` for details of cluster orders.
 *
 * Return value: %TRUE if the iterator was moved,
 *   %FALSE if we were already on the first cluster.
 */
gboolean
pango2_glyph_item_iter_prev_cluster (Pango2GlyphItemIter *iter)
{
  int glyph_index = iter->start_glyph;
  Pango2GlyphString *glyphs = iter->glyph_item->glyphs;
  int cluster;
  Pango2Item *item = iter->glyph_item->item;

  if (LTR (iter->glyph_item))
    {
      if (glyph_index == 0)
        return FALSE;
    }
  else
    {
      if (glyph_index == glyphs->num_glyphs - 1)
        return FALSE;

    }

  iter->end_glyph = iter->start_glyph;
  iter->end_index = iter->start_index;
  iter->end_char = iter->start_char;

  if (LTR (iter->glyph_item))
    {
      cluster = glyphs->log_clusters[glyph_index - 1];
      while (TRUE)
        {
          if (glyph_index == 0)
            {
              iter->start_index = item->offset;
              iter->start_char = 0;
              break;
            }

          glyph_index--;

          if (glyphs->log_clusters[glyph_index] < cluster)
            {
              glyph_index++;
              iter->start_index = item->offset + glyphs->log_clusters[glyph_index];
              iter->start_char -= pango2_utf8_strlen (iter->text + iter->start_index,
                                                      iter->end_index - iter->start_index);
              break;
            }
        }
    }
  else                  /* RTL */
    {
      cluster = glyphs->log_clusters[glyph_index + 1];
      while (TRUE)
        {
          if (glyph_index == glyphs->num_glyphs - 1)
            {
              iter->start_index = item->offset;
              iter->start_char = 0;
              break;
            }

          glyph_index++;

          if (glyphs->log_clusters[glyph_index] < cluster)
            {
              glyph_index--;
              iter->start_index = item->offset + glyphs->log_clusters[glyph_index];
              iter->start_char -= pango2_utf8_strlen (iter->text + iter->start_index,
                                                      iter->end_index - iter->start_index);
              break;
            }
        }
    }

  iter->start_glyph = glyph_index;

  g_assert (iter->start_char <= iter->end_char);
  g_assert (0 <= iter->start_char);

  return TRUE;
}

/**
 * pango2_glyph_item_iter_init_start:
 * @iter: a `Pango2GlyphItemIter`
 * @glyph_item: the glyph item to iterate over
 * @text: text corresponding to the glyph item
 *
 * Initializes a `Pango2GlyphItemIter` structure to point to the
 * first cluster in a glyph item.
 *
 * See `Pango2GlyphItemIter` for details of cluster orders.
 *
 * Return value: %FALSE if there are no clusters in the glyph item
 */
gboolean
pango2_glyph_item_iter_init_start (Pango2GlyphItemIter *iter,
                                   Pango2GlyphItem     *glyph_item,
                                   const char          *text)
{
  iter->glyph_item = glyph_item;
  iter->text = text;

  if (LTR (glyph_item))
    iter->end_glyph = 0;
  else
    iter->end_glyph = glyph_item->glyphs->num_glyphs - 1;

  iter->end_index = glyph_item->item->offset;
  iter->end_char = 0;

  iter->start_glyph = iter->end_glyph;
  iter->start_index = iter->end_index;
  iter->start_char = iter->end_char;

  /* Advance onto the first cluster of the glyph item */
  return pango2_glyph_item_iter_next_cluster (iter);
}

/**
 * pango2_glyph_item_iter_init_end:
 * @iter: a `Pango2GlyphItemIter`
 * @glyph_item: the glyph item to iterate over
 * @text: text corresponding to the glyph item
 *
 * Initializes a `Pango2GlyphItemIter` structure to point to the
 * last cluster in a glyph item.
 *
 * See `Pango2GlyphItemIter` for details of cluster orders.
 *
 * Return value: %FALSE if there are no clusters in the glyph item
 */
gboolean
pango2_glyph_item_iter_init_end (Pango2GlyphItemIter *iter,
                                 Pango2GlyphItem     *glyph_item,
                                 const char          *text)
{
  iter->glyph_item = glyph_item;
  iter->text = text;

  if (LTR (glyph_item))
    iter->start_glyph = glyph_item->glyphs->num_glyphs;
  else
    iter->start_glyph = -1;

  iter->start_index = glyph_item->item->offset + glyph_item->item->length;
  iter->start_char = glyph_item->item->num_chars;

  iter->end_glyph = iter->start_glyph;
  iter->end_index = iter->start_index;
  iter->end_char = iter->start_char;

  /* Advance onto the first cluster of the glyph item */
  return pango2_glyph_item_iter_prev_cluster (iter);
}

typedef struct
{
  Pango2GlyphItemIter iter;

  GSList *segment_attrs;
} ApplyAttrsState;

/* Tack @attrs onto the attributes of glyph_item
 */
static void
append_attrs (Pango2GlyphItem *glyph_item,
              GSList          *attrs)
{
  glyph_item->item->analysis.extra_attrs =
    g_slist_concat (glyph_item->item->analysis.extra_attrs, attrs);
}

/* Make a deep copy of a GSList of Pango2Attribute
 */
static GSList *
attr_slist_copy (GSList *attrs)
{
  GSList *tmp_list;
  GSList *new_attrs;

  new_attrs = g_slist_copy (attrs);

  for (tmp_list = new_attrs; tmp_list; tmp_list = tmp_list->next)
    tmp_list->data = pango2_attribute_copy (tmp_list->data);

  return new_attrs;
}

/* Split the glyph item at the start of the current cluster
 */
static Pango2GlyphItem *
split_before_cluster_start (ApplyAttrsState *state)
{
  Pango2GlyphItem *split_item;
  int split_len = state->iter.start_index - state->iter.glyph_item->item->offset;

  split_item = pango2_glyph_item_split (state->iter.glyph_item, state->iter.text, split_len);
  append_attrs (split_item, state->segment_attrs);

  /* Adjust iteration to account for the split
   */
  if (LTR (state->iter.glyph_item))
    {
      state->iter.start_glyph -= split_item->glyphs->num_glyphs;
      state->iter.end_glyph -= split_item->glyphs->num_glyphs;
    }

  state->iter.start_char -= split_item->item->num_chars;
  state->iter.end_char -= split_item->item->num_chars;

  return split_item;
}

/**
 * pango2_glyph_item_apply_attrs:
 * @glyph_item: a shaped item
 * @text: text that @list applies to
 * @list: a `Pango2AttrList`
 *
 * Splits a shaped item (`Pango2GlyphItem`) into multiple items based
 * on an attribute list.
 *
 * The idea is that if you have attributes that don't affect shaping,
 * such as color or underline, to avoid affecting shaping, you filter
 * them out ([method@Pango2.AttrList.filter]), apply the shaping process
 * and then reapply them to the result using this function.
 *
 * All attributes that start or end inside a cluster are applied
 * to that cluster; for instance, if half of a cluster is underlined
 * and the other-half strikethrough, then the cluster will end
 * up with both underline and strikethrough attributes. In these
 * cases, it may happen that @item->extra_attrs for some of the
 * result items can have multiple attributes of the same type.
 *
 * This function takes ownership of @glyph_item; it will be reused
 * as one of the elements in the list.
 *
 * Return value: (transfer full) (element-type Pango2.GlyphItem): a
 *   list of glyph items resulting from splitting @glyph_item. Free
 *   the elements using [method@Pango2.GlyphItem.free], the list using
 *   [func@GLib.SList.free]
 */
GSList *
pango2_glyph_item_apply_attrs (Pango2GlyphItem *glyph_item,
                               const char      *text,
                               Pango2AttrList  *list)
{
  Pango2AttrIterator iter;
  GSList *result = NULL;
  ApplyAttrsState state;
  gboolean start_new_segment = FALSE;
  gboolean have_cluster;
  int range_start, range_end;
  gboolean is_ellipsis;

  /* This routine works by iterating through the item cluster by
   * cluster; we accumulate the attributes that we need to
   * add to the next output item, and decide when to split
   * off an output item based on two criteria:
   *
   * A) If start_index < attribute_start < end_index
   *    (attribute starts within cluster) then we need
   *    to split between the last cluster and this cluster.
   * B) If start_index < attribute_end <= end_index,
   *    (attribute ends within cluster) then we need to
   *    split between this cluster and the next one.
   */

  /* Advance the attr iterator to the start of the item
   */
  pango2_attr_list_init_iterator (list, &iter);
  do
    {
      pango2_attr_iterator_range (&iter, &range_start, &range_end);
      if (range_end > glyph_item->item->offset)
        break;
    }
  while (pango2_attr_iterator_next (&iter));

  state.segment_attrs = pango2_attr_iterator_get_attrs (&iter);

  is_ellipsis = (glyph_item->item->analysis.flags & PANGO2_ANALYSIS_FLAG_IS_ELLIPSIS) != 0;

  /* Short circuit the case when we don't actually need to
   * split the item
   */
  if (is_ellipsis ||
      (range_start <= glyph_item->item->offset &&
       range_end >= glyph_item->item->offset + glyph_item->item->length))
    goto out;

  for (have_cluster = pango2_glyph_item_iter_init_start (&state.iter, glyph_item, text);
       have_cluster;
       have_cluster = pango2_glyph_item_iter_next_cluster (&state.iter))
    {
      gboolean have_next;

      /* [range_start,range_end] is the first range that intersects
       * the current cluster.
       */

      /* Split item into two, if this cluster isn't a continuation
       * of the last cluster
       */
      if (start_new_segment)
        {
          result = g_slist_prepend (result,
                                    split_before_cluster_start (&state));
          state.segment_attrs = pango2_attr_iterator_get_attrs (&iter);
        }

      start_new_segment = FALSE;

      /* Loop over all ranges that intersect this cluster; exiting
       * leaving [range_start,range_end] being the first range that
       * intersects the next cluster.
       */
      do
        {
          if (range_end > state.iter.end_index) /* Range intersects next cluster */
            break;

          /* Since ranges end in this cluster, the next cluster goes into a
           * separate segment
           */
          start_new_segment = TRUE;

          have_next = pango2_attr_iterator_next (&iter);
          pango2_attr_iterator_range (&iter, &range_start, &range_end);

          if (range_start >= state.iter.end_index) /* New range doesn't intersect this cluster */
            {
              /* No gap between ranges, so previous range must of ended
               * at cluster boundary.
               */
              g_assert (range_start == state.iter.end_index && start_new_segment);
              break;
            }

          /* If any ranges start *inside* this cluster, then we need
           * to split the previous cluster into a separate segment
           */
          if (range_start > state.iter.start_index &&
              state.iter.start_index != glyph_item->item->offset)
            {
              GSList *new_attrs = attr_slist_copy (state.segment_attrs);
              result = g_slist_prepend (result,
                                        split_before_cluster_start (&state));
              state.segment_attrs = new_attrs;
            }

          state.segment_attrs = g_slist_concat (state.segment_attrs,
                                                pango2_attr_iterator_get_attrs (&iter));
        }
      while (have_next);
    }

 out:
  /* What's left in glyph_item is the remaining portion
   */
  append_attrs (glyph_item, state.segment_attrs);
  result = g_slist_prepend (result, glyph_item);

  if (LTR (glyph_item))
    result = g_slist_reverse (result);

  pango2_attr_iterator_clear (&iter);

  return result;
}

/**
 * pango2_glyph_item_letter_space:
 * @glyph_item: a `Pango2GlyphItem`
 * @text: text that @glyph_item corresponds to
 *   (glyph_item->item->offset is an offset from the
 *   start of @text)
 * @log_attrs: (array): logical attributes for the item
 *   (the first logical attribute refers to the position
 *   before the first character in the item)
 * @letter_spacing: amount of letter spacing to add
 *   in Pango2 units. May be negative, though too large
 *   negative values will give ugly results.
 *
 * Adds spacing between the graphemes of @glyph_item to
 * give the effect of typographic letter spacing.
 */
void
pango2_glyph_item_letter_space (Pango2GlyphItem *glyph_item,
                                const char      *text,
                                Pango2LogAttr   *log_attrs,
                                int              letter_spacing)
{
  Pango2GlyphItemIter iter;
  Pango2GlyphInfo *glyphs = glyph_item->glyphs->glyphs;
  gboolean have_cluster;
  int space_left, space_right;

  space_left = letter_spacing / 2;

  /* hinting */
  if ((letter_spacing & (PANGO2_SCALE - 1)) == 0)
    {
      space_left = PANGO2_UNITS_ROUND (space_left);
    }

  space_right = letter_spacing - space_left;

  for (have_cluster = pango2_glyph_item_iter_init_start (&iter, glyph_item, text);
       have_cluster;
       have_cluster = pango2_glyph_item_iter_next_cluster (&iter))
    {
      if (!log_attrs[iter.start_char].is_cursor_position)
        {
          if (glyphs[iter.start_glyph].geometry.width == 0)
            {
              if (iter.start_glyph < iter.end_glyph) /* LTR */
                glyphs[iter.start_glyph].geometry.x_offset -= space_right;
              else
                glyphs[iter.start_glyph].geometry.x_offset += space_left;
            }
          continue;
        }

      if (iter.start_glyph < iter.end_glyph) /* LTR */
        {
          if (iter.start_char > 0)
            {
              glyphs[iter.start_glyph].geometry.width    += space_left ;
              glyphs[iter.start_glyph].geometry.x_offset += space_left ;
            }
          if (iter.end_char < glyph_item->item->num_chars)
            {
              glyphs[iter.end_glyph-1].geometry.width    += space_right;
            }
        }
      else                                       /* RTL */
        {
          if (iter.start_char > 0)
            {
              glyphs[iter.start_glyph].geometry.width    += space_right;
            }
          if (iter.end_char < glyph_item->item->num_chars)
            {
              glyphs[iter.end_glyph+1].geometry.x_offset += space_left ;
              glyphs[iter.end_glyph+1].geometry.width    += space_left ;
            }
        }
    }
}

/**
 * pango2_glyph_item_get_logical_widths:
 * @glyph_item: a `Pango2GlyphItem`
 * @text: text that @glyph_item corresponds to
 *   (glyph_item->item->offset is an offset from the
 *   start of @text)
 * @logical_widths: (array): an array whose length is the number of
 *   characters in glyph_item (equal to glyph_item->item->num_chars)
 *   to be filled in with the resulting character widths.
 *
 * Given a `Pango2GlyphItem` and the corresponding text, determine the
 * width corresponding to each character.
 *
 * When multiple characters compose a single cluster, the width of the
 * entire cluster is divided equally among the characters.
 *
 * See also [method@Pango2.GlyphString.get_logical_widths].
 */
void
pango2_glyph_item_get_logical_widths (Pango2GlyphItem *glyph_item,
                                      const char      *text,
                                      int             *logical_widths)
{
  Pango2GlyphItemIter iter;
  gboolean has_cluster;
  int dir;

  dir = glyph_item->item->analysis.level % 2 == 0 ? +1 : -1;
  for (has_cluster = pango2_glyph_item_iter_init_start (&iter, glyph_item, text);
       has_cluster;
       has_cluster = pango2_glyph_item_iter_next_cluster (&iter))
    {
      int glyph_index, char_index, num_chars, cluster_width = 0, char_width;

      for (glyph_index  = iter.start_glyph;
           glyph_index != iter.end_glyph;
           glyph_index += dir)
        {
          cluster_width += glyph_item->glyphs->glyphs[glyph_index].geometry.width;
        }

      num_chars = iter.end_char - iter.start_char;
      if (num_chars) /* pedantic */
        {
          char_width = cluster_width / num_chars;

          for (char_index = iter.start_char;
               char_index < iter.end_char;
               char_index++)
            {
              logical_widths[char_index] = char_width;
            }

          /* add any residues to the first char */
          logical_widths[iter.start_char] += cluster_width - (char_width * num_chars);
        }
    }
}
