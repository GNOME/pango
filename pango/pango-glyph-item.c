/* -*- mode: C; c-file-style: "gnu" -*- */
/* Pango
 * pango-glyph-item.c: Pair of PangoItem and a glyph string
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
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.	 See the GNU
 * Library General Public License for more details.
 *
 * You should have received a copy of the GNU Library General Public
 * License along with this library; if not, write to the
 * Free Software Foundation, Inc., 59 Temple Place - Suite 330,
 * Boston, MA 02111-1307, USA.
 */

#include <string.h>

#include "pango-glyph-item.h"

#define LTR(glyph_item) (((glyph_item)->item->analysis.level % 2) == 0)

/**
 * pango_glyph_item_split:
 * @orig: a #PangoItem
 * @text: text to which positions in @orig apply.
 * @split_index: byte index of position to split item, relative to the start of the item
 * 
 * Modifies @orig to cover only the text after @split_index, and
 * returns a new item that covers the text before @split_index that
 * used to be in @orig. You can think of @split_index as the length of
 * the returned item. @split_index may not be 0, and it may not be
 * greater than or equal to the length of @orig (that is, there must
 * be at least one byte assigned to each item, you can't create a
 * zero-length item).
 *
 * This function is similar in function to pango_item_split() (and uses
 * it internally)
 * 
 * Return value: new item representing text before @split_index
 **/
PangoGlyphItem *
pango_glyph_item_split (PangoGlyphItem *orig,
			const char     *text,
			int             split_index)
{
  PangoGlyphItem *new;
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

  new = g_new (PangoGlyphItem, 1);
  split_offset = g_utf8_pointer_to_offset (text + orig->item->offset,
					   text + orig->item->offset + split_index);
  new->item = pango_item_split (orig->item, split_index, split_offset);

  new->glyphs = pango_glyph_string_new ();
  pango_glyph_string_set_size (new->glyphs, num_glyphs);

  if (LTR (orig))
    {
      memcpy (new->glyphs->glyphs, orig->glyphs->glyphs, num_glyphs * sizeof (PangoGlyphInfo));
      memcpy (new->glyphs->log_clusters, orig->glyphs->log_clusters, num_glyphs * sizeof (int));
      
      memmove (orig->glyphs->glyphs, orig->glyphs->glyphs + num_glyphs,
	       num_remaining * sizeof (PangoGlyphInfo));
      for (i = num_glyphs; i < orig->glyphs->num_glyphs; i++)
	orig->glyphs->log_clusters[i - num_glyphs] = orig->glyphs->log_clusters[i] - split_index;
    }
  else
    {
      memcpy (new->glyphs->glyphs, orig->glyphs->glyphs + num_remaining, num_glyphs * sizeof (PangoGlyphInfo));
      memcpy (new->glyphs->log_clusters, orig->glyphs->log_clusters + num_remaining, num_glyphs * sizeof (int));

      for (i = 0; i < num_remaining; i++)
	orig->glyphs->log_clusters[i] = orig->glyphs->log_clusters[i] - split_index;
    }

  pango_glyph_string_set_size (orig->glyphs, orig->glyphs->num_glyphs - num_glyphs);

  return new;
}

/* Structure holding state when we're iterating over a GlyphItem for
 * pango_glyph_item_apply_attrs(). cluster_start/cluster_end (and
 * range_start/range_end in apply_attrs()) are offsets into the
 * text, so note the difference of glyph_item->item->offset between
 * them and clusters in the log_clusters[] array.
 */
typedef struct 
{
  PangoGlyphItem *glyph_item;
  const gchar *text;
  
  int glyph_index;
  int cluster_start;
  int cluster_end;
  
  GSList *segment_attrs;
} ApplyAttrsState;

/* Tack @attrs onto the attributes of glyph_item
 */
static void
append_attrs (PangoGlyphItem *glyph_item,
	      GSList         *attrs)
{
  glyph_item->item->analysis.extra_attrs =
    g_slist_concat (glyph_item->item->analysis.extra_attrs, attrs);
}

/* Make a deep copy of a GSlist of PangoAttribute
 */
static GSList *
attr_slist_copy (GSList *attrs)
{
  GSList *tmp_list;
  GSList *new_attrs;
  
  new_attrs = g_slist_copy (attrs);
  
  for (tmp_list = new_attrs; tmp_list; tmp_list = tmp_list->next)
    tmp_list->data = pango_attribute_copy (tmp_list->data);

  return new_attrs;
}

/* Advance to the next logical cluster
 */
static gboolean
next_cluster (ApplyAttrsState *state)
{
  int glyph_index = state->glyph_index;
  PangoGlyphString *glyphs = state->glyph_item->glyphs;
  PangoItem *item = state->glyph_item->item;
    
  state->cluster_start = state->cluster_end;
  
  if (LTR (state->glyph_item))
    {
      if (glyph_index == glyphs->num_glyphs)
	return FALSE;
      
      while (TRUE)
	{
	  glyph_index++;
	  
	  if (glyph_index == glyphs->num_glyphs)
	    {
	      state->cluster_end = item->offset + item->length;
	      break;
	    }
	  
	  if (item->offset + glyphs->log_clusters[glyph_index] >= state->cluster_start)
	    {
	      state->cluster_end = item->offset + glyphs->log_clusters[glyph_index];
	      break; 
	    }
	}
    }
  else			/* RTL */
    {
      if (glyph_index < 0)
	return FALSE;
      
      while (TRUE)
	{
	  glyph_index--;
	  
	  if (glyph_index < 0)
	    {
	      state->cluster_end = item->offset + item->length;
	      break;
	    }
	  
	  if (item->offset + glyphs->log_clusters[glyph_index] >= state->cluster_start)
	    {
	      state->cluster_end = item->offset + glyphs->log_clusters[glyph_index];
	      break; 
	    }
	}
    }

  state->glyph_index = glyph_index;
  return TRUE;
}

/* Split the glyph item at the start of the current cluster
 */
static PangoGlyphItem *
split_before_cluster_start (ApplyAttrsState *state)
{
  PangoGlyphItem *split_item;
  int split_len = state->cluster_start - state->glyph_item->item->offset;

  split_item = pango_glyph_item_split (state->glyph_item, state->text, split_len);
  append_attrs (split_item, state->segment_attrs);

  /* Adjust iteration to account for the split
   */
  if (LTR (state->glyph_item))
    state->glyph_index -= split_item->glyphs->num_glyphs;
 
  return split_item;
}

/**
 * pango_glyph_item_apply_attrs:
 * @glyph_item: a shaped item 
 * @text: text that @list applies to
 * @list: a #PangoAttrList
 *   
 * Splits a shaped item (PangoGlyphItem) into multiple items based
 * on an attribute list. The idea is that if you have attributes
 * that don't affect shaping, such as color or underline, to avoid
 * affecting shaping, you filter them out (pango_attr_list_filter()),
 * apply the shaping process and then reapply them to the result using
 * this function.
 *
 * All attributes that start or end inside a cluster are applied
 * to that cluster; for instance, if half of a cluster is underlined
 * and the other-half strikethough, then the cluster will end
 * up with both underline and strikethrough attributes. In these
 * cases, it may happen that item->extra_attrs for some of the
 * result items can have multiple attributes of the same type.
 *
 * This function takes ownership of @glyph_item; it will be reused
 * as one of the elements in the list.
 * 
 * Return value: a list of glyph items resulting from splitting
 *   @glyph_item. Free the elements using pango_glyph_item_free(),
 *   the list using g_slist_free().
 **/
GSList *
pango_glyph_item_apply_attrs (PangoGlyphItem   *glyph_item,
			      const char       *text,
			      PangoAttrList    *list)
{
  PangoAttrIterator *iter = pango_attr_list_get_iterator (list);
  GSList *result = NULL;
  ApplyAttrsState state;
  gboolean start_new_segment = FALSE;
  int range_start, range_end;

  /* This routine works by iterating through the item cluster by
   * cluster; we accumulate the attributes that we need to
   * add to the next output item, and decide when to split
   * off an output item based on two criteria:
   *
   * A) If cluster_start < attribute_start < cluster_end
   *    (attribute starts within cluster) then we need
   *    to split between the last cluster and this cluster.
   * B) If cluster_start < attribute_end <= cluster_end,
   *    (attribute ends within cluster) then we need to
   *    split between this cluster and the next one.
   */

  state.glyph_item = glyph_item;
  state.text = text;
  
  if (LTR (glyph_item))
    state.glyph_index = 0;
  else
    state.glyph_index = glyph_item->glyphs->num_glyphs - 1;

  state.cluster_end = glyph_item->item->offset;
  
  /* Advance the attr iterator to the start of the item
   */
  while (TRUE)
    {
      pango_attr_iterator_range (iter, &range_start, &range_end);
      if (range_end > state.cluster_end)
	break;
      
      g_assert (pango_attr_iterator_next (iter));
    }

  state.segment_attrs = pango_attr_iterator_get_attrs (iter);
  
  /* Short circuit the case when we don't actually need to
   * split the item
   */
  if (range_start <= glyph_item->item->offset &&
      range_end >= glyph_item->item->offset + glyph_item->item->length)
    goto out;
  
  while (TRUE)
    {
      /* Find the next logical cluster; [range_start,range_end]
       * is the first range that intersects the new cluster.
       */
      if (!next_cluster (&state))
	break;

      /* Split item into two, if this cluster isn't a continuation
       * of the last cluster
       */
      if (start_new_segment)
	{
	  result = g_slist_prepend (result,
				    split_before_cluster_start (&state));
	  state.segment_attrs = pango_attr_iterator_get_attrs (iter);
	}
      
      start_new_segment = FALSE;

      /* Loop over all ranges that intersect this cluster; exiting
       * leaving [range_start,range_end] being the first range that
       * intersects the next cluster.
       */
      while (TRUE)
	{
	  /* If any ranges end in this cluster, then the next cluster
	   * goes into a separate segment
	   */
	  if (range_end <= state.cluster_end)
	    start_new_segment = TRUE;

	  if (range_end > state.cluster_end) /* Range intersects next cluster */
	    break;

	  g_assert (pango_attr_iterator_next (iter));
	  pango_attr_iterator_range (iter, &range_start, &range_end);

	  if (range_start >= state.cluster_end) /* New range doesn't intersect this cluster */
	    {
	      /* No gap between ranges, so previous range must of ended
	       * at cluster boundary.
	       */
	      g_assert (range_start == state.cluster_end && start_new_segment);
	      break;
	    }

	  /* If any ranges start *inside* this cluster, then we need
	   * to split the previous cluster into a separate segment
	   */
	  if (range_start > state.cluster_start &&
	      state.cluster_start != glyph_item->item->offset)
	    {
	      GSList *new_attrs = attr_slist_copy (state.segment_attrs);
	      result = g_slist_prepend (result, 
					split_before_cluster_start (&state));
	      state.segment_attrs = new_attrs;
	    }
	  
	  state.segment_attrs = g_slist_concat (state.segment_attrs,
						pango_attr_iterator_get_attrs (iter));
	}
    }

 out:
  /* What's left in glyph_item is the remaining portion
   */
  append_attrs (glyph_item, state.segment_attrs);
  result = g_slist_prepend (result, glyph_item);

  if (LTR (glyph_item))
    result = g_slist_reverse (result);

  pango_attr_iterator_destroy (iter);
  
  return result;
}
