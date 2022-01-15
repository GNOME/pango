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
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Library General Public License for more details.
 *
 * You should have received a copy of the GNU Library General Public
 * License along with this library; if not, write to the
 * Free Software Foundation, Inc., 59 Temple Place - Suite 330,
 * Boston, MA 02111-1307, USA.
 */

#include "config.h"
#include "pango-attributes.h"
#include "pango-item-private.h"
#include "pango-impl-utils.h"

/**
 * pango_item_new:
 *
 * Creates a new `PangoItem` structure initialized to default values.
 *
 * Return value: the newly allocated `PangoItem`, which should
 *   be freed with [method@Pango.Item.free].
 */
PangoItem *
pango_item_new (void)
{
  PangoItem *result = g_slice_new0 (PangoItem);

  return (PangoItem *)result;
}

/**
 * pango_item_copy:
 * @item: (nullable): a `PangoItem`
 *
 * Copy an existing `PangoItem` structure.
 *
 * Return value: (nullable): the newly allocated `PangoItem`
 */
PangoItem *
pango_item_copy (PangoItem *item)
{
  GSList *extra_attrs, *tmp_list;
  PangoItem *result;

  if (item == NULL)
    return NULL;

  result = pango_item_new ();

  result->offset = item->offset;
  result->length = item->length;
  result->num_chars = item->num_chars;
  result->char_offset = item->char_offset;

  result->analysis = item->analysis;
  if (result->analysis.size_font)
    g_object_ref (result->analysis.size_font);

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
 * @item: (nullable): a `PangoItem`, may be %NULL
 *
 * Free a `PangoItem` and all associated memory.
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

  if (item->analysis.size_font)
    g_object_unref (item->analysis.size_font);

  if (item->analysis.font)
    g_object_unref (item->analysis.font);

  g_slice_free (PangoItem, item);
}

G_DEFINE_BOXED_TYPE (PangoItem, pango_item,
                     pango_item_copy,
                     pango_item_free);

/**
 * pango_item_split:
 * @orig: a `PangoItem`
 * @split_index: byte index of position to split item, relative to the
 *   start of the item
 * @split_offset: number of chars between start of @orig and @split_index
 *
 * Modifies @orig to cover only the text after @split_index, and
 * returns a new item that covers the text before @split_index that
 * used to be in @orig.
 *
 * You can think of @split_index as the length of the returned item.
 * @split_index may not be 0, and it may not be greater than or equal
 * to the length of @orig (that is, there must be at least one byte
 * assigned to each item, you can't create a zero-length item).
 * @split_offset is the length of the first item in chars, and must be
 * provided because the text used to generate the item isn't available,
 * so `pango_item_split()` can't count the char length of the split items
 * itself.
 *
 * Return value: new item representing text before @split_index, which
 *   should be freed with [method@Pango.Item.free].
 */
PangoItem *
pango_item_split (PangoItem *orig,
                  int        split_index,
                  int        split_offset)
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
  orig->char_offset += split_offset;

  return new_item;
}

/*< private >
 * pango_item_unsplit:
 * @orig: the item to unsplit
 * @split_index: value passed to pango_item_split()
 * @split_offset: value passed to pango_item_split()
 *
 * Undoes the effect of a pango_item_split() call with
 * the same arguments.
 *
 * You are expected to free the new item that was returned
 * by pango_item_split() yourself.
 */
void
pango_item_unsplit (PangoItem *orig,
                    int        split_index,
                    int        split_offset)
{
  orig->offset -= split_index;
  orig->length += split_index;
  orig->num_chars += split_offset;
  orig->char_offset -= split_offset;
}

static int
compare_attr (gconstpointer p1, gconstpointer p2)
{
  const PangoAttribute *a1 = p1;
  const PangoAttribute *a2 = p2;
  if (pango_attribute_equal (a1, a2) &&
      a1->start_index == a2->start_index &&
      a1->end_index == a2->end_index)
    return 0;

  return 1;
}

/**
 * pango_item_apply_attrs:
 * @item: a `PangoItem`
 * @iter: a `PangoAttrIterator`
 *
 * Add attributes to a `PangoItem`.
 *
 * The idea is that you have attributes that don't affect itemization,
 * such as font features, so you filter them out using
 * [method@Pango.AttrList.filter], itemize your text, then reapply the
 * attributes to the resulting items using this function.
 *
 * The @iter should be positioned before the range of the item,
 * and will be advanced past it. This function is meant to be called
 * in a loop over the items resulting from itemization, while passing
 * the iter to each call.
 *
 * Since: 1.44
 */
void
pango_item_apply_attrs (PangoItem         *item,
                        PangoAttrIterator *iter)
{
  int start, end;
  GSList *attrs = NULL;

  do
    {
      pango_attr_iterator_range (iter, &start, &end);

      if (start >= item->offset + item->length)
        break;

      if (end >= item->offset)
        {
          GSList *list, *l;

          list = pango_attr_iterator_get_attrs (iter);
          for (l = list; l; l = l->next)
            {
              if (!g_slist_find_custom (attrs, l->data, compare_attr))

                attrs = g_slist_prepend (attrs, pango_attribute_copy (l->data));
            }
          g_slist_free_full (list, (GDestroyNotify)pango_attribute_destroy);
        }

      if (end >= item->offset + item->length)
        break;
    }
  while (pango_attr_iterator_next (iter));

  item->analysis.extra_attrs = g_slist_concat (item->analysis.extra_attrs, attrs);
}

void
pango_analysis_collect_features (const PangoAnalysis *analysis,
                                 hb_feature_t        *features,
                                 guint                length,
                                 guint               *num_features)
{
  GSList *l;

  pango_font_get_features (analysis->font, features, length, num_features);

  for (l = analysis->extra_attrs; l && *num_features < length; l = l->next)
    {
      PangoAttribute *attr = l->data;
      if (attr->klass->type == PANGO_ATTR_FONT_FEATURES)
        {
          PangoAttrFontFeatures *fattr = (PangoAttrFontFeatures *) attr;
          const gchar *feat;
          const gchar *end;
          int len;

          feat = fattr->features;

          while (feat != NULL && *num_features < length)
            {
              end = strchr (feat, ',');
              if (end)
                len = end - feat;
              else
                len = -1;
              if (hb_feature_from_string (feat, len, &features[*num_features]))
                {
                  features[*num_features].start = attr->start_index;
                  features[*num_features].end = attr->end_index;
                  (*num_features)++;
                }

              if (end == NULL)
                break;

              feat = end + 1;
            }
        }
    }

  /* Turn off ligatures when letterspacing */
  for (l = analysis->extra_attrs; l && *num_features < length; l = l->next)
    {
      PangoAttribute *attr = l->data;
      if (attr->klass->type == PANGO_ATTR_LETTER_SPACING)
        {
          hb_tag_t tags[] = {
            HB_TAG('l','i','g','a'),
            HB_TAG('c','l','i','g'),
            HB_TAG('d','l','i','g'),
            HB_TAG('h','l','i','g'),
          };
          int i;
          for (i = 0; i < G_N_ELEMENTS (tags); i++)
            {
              features[*num_features].tag = tags[i];
              features[*num_features].value = 0;
              features[*num_features].start = attr->start_index;
              features[*num_features].end = attr->end_index;
              (*num_features)++;
            }
        }
    }
}

/*< private >
 * pango_analysis_set_size_font:
 * @analysis: a `PangoAnalysis`
 * @font: a `PangoFont`
 *
 * Sets the font to use for determining the line height.
 *
 * This is used when scaling fonts for emulated Small Caps,
 * to preserve the original line height.
 */
void
pango_analysis_set_size_font (PangoAnalysis *analysis,
                              PangoFont     *font)
{
  if (analysis->size_font)
    g_object_unref (analysis->size_font);
  analysis->size_font = font;
  if (analysis->size_font)
    g_object_ref (analysis->size_font);
}

/*< private >
 * pango_analysis_get_size_font:
 * @analysis: a `PangoAnalysis`
 *
 * Gets the font to use for determining line height.
 *
 * If this returns `NULL`, use analysis->font.
 *
 * Returns: (nullable) (transfer none): the font
 */
PangoFont *
pango_analysis_get_size_font (const PangoAnalysis *analysis)
{
  return analysis->size_font;
}

/*< private >
 * pango_item_get_properties:
 * @item: a `PangoItem`
 * @properties: `ItemProperties` struct to populate
 *
 * Extract useful information from the @item's attributes.
 *
 * Note that letter-spacing and shape are required to be constant
 * across items. But underline and strikethrough can vary across
 * an item, so we collect all the values that we find.
 */
void
pango_item_get_properties (PangoItem      *item,
                           ItemProperties *properties)
{
  GSList *tmp_list = item->analysis.extra_attrs;

  properties->uline_single = FALSE;
  properties->uline_double = FALSE;
  properties->uline_low = FALSE;
  properties->uline_error = FALSE;
  properties->oline_single = FALSE;
  properties->strikethrough = FALSE;
  properties->showing_space = FALSE;
  properties->letter_spacing = 0;
  properties->shape_set = FALSE;
  properties->shape_ink_rect = NULL;
  properties->shape_logical_rect = NULL;
  properties->line_height = 0.0;
  properties->absolute_line_height = 0;

  while (tmp_list)
    {
      PangoAttribute *attr = tmp_list->data;

      switch ((int) attr->klass->type)
        {
        case PANGO_ATTR_UNDERLINE:
          switch (((PangoAttrInt *)attr)->value)
            {
            case PANGO_UNDERLINE_NONE:
              break;
            case PANGO_UNDERLINE_SINGLE:
            case PANGO_UNDERLINE_SINGLE_LINE:
              properties->uline_single = TRUE;
              break;
            case PANGO_UNDERLINE_DOUBLE:
            case PANGO_UNDERLINE_DOUBLE_LINE:
              properties->uline_double = TRUE;
              break;
            case PANGO_UNDERLINE_LOW:
              properties->uline_low = TRUE;
              break;
            case PANGO_UNDERLINE_ERROR:
            case PANGO_UNDERLINE_ERROR_LINE:
              properties->uline_error = TRUE;
              break;
            default:
              g_assert_not_reached ();
              break;
            }
          break;
        case PANGO_ATTR_OVERLINE:
          switch (((PangoAttrInt *)attr)->value)
            {
            case PANGO_OVERLINE_SINGLE:
              properties->oline_single = TRUE;
              break;
            default:
              g_assert_not_reached ();
              break;
            }
          break;

        case PANGO_ATTR_STRIKETHROUGH:
          properties->strikethrough = ((PangoAttrInt *)attr)->value;
          break;

        case PANGO_ATTR_LETTER_SPACING:
          properties->letter_spacing = ((PangoAttrInt *)attr)->value;
          break;

        case PANGO_ATTR_SHAPE:
          properties->shape_set = TRUE;
          properties->shape_logical_rect = &((PangoAttrShape *)attr)->logical_rect;
          properties->shape_ink_rect = &((PangoAttrShape *)attr)->ink_rect;
          break;

        case PANGO_ATTR_LINE_HEIGHT:
          properties->line_height = ((PangoAttrFloat *)attr)->value;
          break;

        case PANGO_ATTR_ABSOLUTE_LINE_HEIGHT:
          properties->absolute_line_height = ((PangoAttrInt *)attr)->value;
          break;

        case PANGO_ATTR_SHOW:
          properties->showing_space = (((PangoAttrInt *)attr)->value & PANGO_SHOW_SPACES) != 0;
          break;

        default:
          break;
        }
      tmp_list = tmp_list->next;
    }
}
