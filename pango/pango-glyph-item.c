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
  gboolean ltr = (orig->item->analysis.level % 2) == 0;

  g_return_val_if_fail (orig != NULL, NULL);
  g_return_val_if_fail (orig->item->length > 0, NULL);
  g_return_val_if_fail (split_index > 0, NULL);
  g_return_val_if_fail (split_index < orig->item->length, NULL);

  if (ltr)
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

  if (ltr)
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

static void
append_attrs (PangoGlyphItem *glyph_item,
	      GSList         *attrs)
{
  glyph_item->item->analysis.extra_attrs =
    g_slist_concat (glyph_item->item->analysis.extra_attrs, attrs);
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
  PangoGlyphItem *new;
  GSList *result = NULL;
  int start;
  int end;

  gboolean ltr = (glyph_item->item->analysis.level % 2) == 0;

  while (TRUE)
    {
      pango_attr_iterator_range (iter, &start, &end);

      if (start > glyph_item->item->offset)
	{
	  if (start >= glyph_item->item->offset + glyph_item->item->length)
	    break;

	  new = pango_glyph_item_split (glyph_item, text,
					start - glyph_item->item->offset);
	  
	  result = g_slist_prepend (result, new);
	}

      if (end > glyph_item->item->offset)
	{
	  if (end >= glyph_item->item->offset + glyph_item->item->length)
	    {
	      append_attrs (glyph_item, pango_attr_iterator_get_attrs (iter));
	      break;
	    }

	  new = pango_glyph_item_split (glyph_item, text,
					end - glyph_item->item->offset);

	  append_attrs (new, pango_attr_iterator_get_attrs (iter));

	  result = g_slist_prepend (result, new);
	}

      if (!pango_attr_iterator_next (iter))
	  break;
    }

  result = g_slist_prepend (result, glyph_item);

  if (ltr)
    result = g_slist_reverse (result);
  
  return result;
}
