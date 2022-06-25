/* Pango2
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
#include "pango-attr-private.h"
#include "pango-item-private.h"
#include "pango-impl-utils.h"

/**
 * Pango2Analysis:
 *
 * The `Pango2Analysis` structure stores information about
 * the properties of a segment of text.
 */

/**
 * Pango2Item:
 *
 * The `Pango2Item` structure stores information about
 * a segment of text.
 *
 * You typically obtain `Pango2Items` by itemizing a piece
 * of text with [func@Pango2.itemize].
 */

/**
 * pango2_item_new:
 *
 * Creates a new `Pango2Item` structure initialized to default values.
 *
 * Return value: the newly allocated `Pango2Item`, which should
 *   be freed with [method@Pango2.Item.free].
 */
Pango2Item *
pango2_item_new (void)
{
  Pango2Item *result = g_slice_new0 (Pango2Item);

  return (Pango2Item *)result;
}

/**
 * pango2_item_copy:
 * @item: (nullable): a `Pango2Item`
 *
 * Copy an existing `Pango2Item` structure.
 *
 * Return value: (nullable): the newly allocated `Pango2Item`
 */
Pango2Item *
pango2_item_copy (Pango2Item *item)
{
  GSList *extra_attrs, *tmp_list;
  Pango2Item *result;

  if (item == NULL)
    return NULL;

  result = pango2_item_new ();

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
      extra_attrs = g_slist_prepend (extra_attrs, pango2_attribute_copy (tmp_list->data));
      tmp_list = tmp_list->next;
    }

  result->analysis.extra_attrs = g_slist_reverse (extra_attrs);

  return result;
}

/**
 * pango2_item_free:
 * @item: (nullable): a `Pango2Item`, may be %NULL
 *
 * Free a `Pango2Item` and all associated memory.
 **/
void
pango2_item_free (Pango2Item *item)
{
  if (item == NULL)
    return;

  if (item->analysis.extra_attrs)
    {
      g_slist_foreach (item->analysis.extra_attrs, (GFunc)pango2_attribute_destroy, NULL);
      g_slist_free (item->analysis.extra_attrs);
    }

  if (item->analysis.size_font)
    g_object_unref (item->analysis.size_font);

  if (item->analysis.font)
    g_object_unref (item->analysis.font);

  g_slice_free (Pango2Item, item);
}

G_DEFINE_BOXED_TYPE (Pango2Item, pango2_item,
                     pango2_item_copy,
                     pango2_item_free);

/**
 * pango2_item_split:
 * @orig: a `Pango2Item`
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
 * so `pango2_item_split()` can't count the char length of the split items
 * itself.
 *
 * Return value: new item representing text before @split_index, which
 *   should be freed with [method@Pango2.Item.free].
 */
Pango2Item *
pango2_item_split (Pango2Item *orig,
                   int         split_index,
                   int         split_offset)
{
  Pango2Item *new_item;

  g_return_val_if_fail (orig != NULL, NULL);
  g_return_val_if_fail (split_index > 0, NULL);
  g_return_val_if_fail (split_index < orig->length, NULL);
  g_return_val_if_fail (split_offset > 0, NULL);
  g_return_val_if_fail (split_offset < orig->num_chars, NULL);

  new_item = pango2_item_copy (orig);
  new_item->length = split_index;
  new_item->num_chars = split_offset;

  orig->offset += split_index;
  orig->length -= split_index;
  orig->num_chars -= split_offset;
  orig->char_offset += split_offset;

  return new_item;
}

/*< private >
 * pango2_item_unsplit:
 * @orig: the item to unsplit
 * @split_index: value passed to pango2_item_split()
 * @split_offset: value passed to pango2_item_split()
 *
 * Undoes the effect of a pango2_item_split() call with
 * the same arguments.
 *
 * You are expected to free the new item that was returned
 * by pango2_item_split() yourself.
 */
void
pango2_item_unsplit (Pango2Item *orig,
                     int         split_index,
                     int         split_offset)
{
  orig->offset -= split_index;
  orig->length += split_index;
  orig->num_chars += split_offset;
  orig->char_offset -= split_offset;
}

static int
compare_attr (gconstpointer p1, gconstpointer p2)
{
  const Pango2Attribute *a1 = p1;
  const Pango2Attribute *a2 = p2;
  if (pango2_attribute_equal (a1, a2) &&
      a1->start_index == a2->start_index &&
      a1->end_index == a2->end_index)
    return 0;

  return 1;
}

/**
 * pango2_item_apply_attrs:
 * @item: a `Pango2Item`
 * @iter: a `Pango2AttrIterator`
 *
 * Add attributes to a `Pango2Item`.
 *
 * The idea is that you have attributes that don't affect itemization,
 * such as font features, so you filter them out using
 * [method@Pango2.AttrList.filter], itemize your text, then reapply the
 * attributes to the resulting items using this function.
 *
 * The @iter should be positioned before the range of the item,
 * and will be advanced past it. This function is meant to be called
 * in a loop over the items resulting from itemization, while passing
 * the iter to each call.
 */
void
pango2_item_apply_attrs (Pango2Item         *item,
                         Pango2AttrIterator *iter)
{
  int start, end;
  GSList *attrs = NULL;

  do
    {
      pango2_attr_iterator_range (iter, &start, &end);

      if (start >= item->offset + item->length)
        break;

      if (end >= item->offset)
        {
          GSList *list, *l;

          list = pango2_attr_iterator_get_attrs (iter);
          for (l = list; l; l = l->next)
            {
              if (!g_slist_find_custom (attrs, l->data, compare_attr))

                attrs = g_slist_prepend (attrs, pango2_attribute_copy (l->data));
            }
          g_slist_free_full (list, (GDestroyNotify)pango2_attribute_destroy);
        }

      if (end >= item->offset + item->length)
        break;
    }
  while (pango2_attr_iterator_next (iter));

  item->analysis.extra_attrs = g_slist_concat (item->analysis.extra_attrs, attrs);
}

void
pango2_analysis_collect_features (const Pango2Analysis *analysis,
                                  hb_feature_t         *features,
                                  guint                 length,
                                  guint                *num_features)
{
  GSList *l;

  if (PANGO2_IS_HB_FONT (analysis->font))
    {
      const hb_feature_t *font_features;
      guint n_font_features;

      font_features = pango2_hb_font_get_features (PANGO2_HB_FONT (analysis->font),
                                                   &n_font_features);
      *num_features = MIN (length, n_font_features);
      memcpy (features, font_features, sizeof (hb_feature_t) * *num_features);
   }

  for (l = analysis->extra_attrs; l && *num_features < length; l = l->next)
    {
      Pango2Attribute *attr = l->data;
      if (attr->type == PANGO2_ATTR_FONT_FEATURES)
        {
          const char *feat;
          const char *end;
          int len;

          feat = attr->str_value;

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
      Pango2Attribute *attr = l->data;
      if (attr->type == PANGO2_ATTR_LETTER_SPACING)
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
 * pango2_analysis_set_size_font:
 * @analysis: a `Pango2Analysis`
 * @font: a `Pango2Font`
 *
 * Sets the font to use for determining the line height.
 *
 * This is used when scaling fonts for emulated Small Caps,
 * to preserve the original line height.
 */
void
pango2_analysis_set_size_font (Pango2Analysis *analysis,
                               Pango2Font     *font)
{
  if (analysis->size_font)
    g_object_unref (analysis->size_font);
  analysis->size_font = font;
  if (analysis->size_font)
    g_object_ref (analysis->size_font);
}

/*< private >
 * pango2_analysis_get_size_font:
 * @analysis: a `Pango2Analysis`
 *
 * Gets the font to use for determining line height.
 *
 * If this returns `NULL`, use analysis->font.
 *
 * Returns: (nullable) (transfer none): the font
 */
Pango2Font *
pango2_analysis_get_size_font (const Pango2Analysis *analysis)
{
  return analysis->size_font;
}

/*< private >
 * pango2_item_get_properties:
 * @item: a `Pango2Item`
 * @properties: `ItemProperties` struct to populate
 *
 * Extract useful information from the @item's attributes.
 *
 * Note that letter-spacing and shape are required to be constant
 * across items. But underline and strikethrough can vary across
 * an item, so we collect all the values that we find.
 */
void
pango2_item_get_properties (Pango2Item     *item,
                            ItemProperties *properties)
{
  GSList *tmp_list = item->analysis.extra_attrs;

  properties->uline_style = PANGO2_LINE_STYLE_NONE;
  properties->uline_position = PANGO2_UNDERLINE_POSITION_NORMAL;
  properties->oline_style = PANGO2_LINE_STYLE_NONE;
  properties->strikethrough_style = PANGO2_LINE_STYLE_NONE;
  properties->showing_space = FALSE;
  properties->no_paragraph_break = FALSE;
  properties->letter_spacing = 0;
  properties->line_height = 0.0;
  properties->absolute_line_height = 0;
  properties->line_spacing = 0;
  properties->shape = NULL;

  while (tmp_list)
    {
      Pango2Attribute *attr = tmp_list->data;

      switch ((int) attr->type)
        {
        case PANGO2_ATTR_UNDERLINE:
          properties->uline_style = attr->int_value;
          break;

        case PANGO2_ATTR_UNDERLINE_POSITION:
          properties->uline_position = attr->int_value;
          break;

        case PANGO2_ATTR_OVERLINE:
          properties->oline_style = attr->int_value;
          break;

        case PANGO2_ATTR_STRIKETHROUGH:
          properties->strikethrough_style = attr->int_value;
          break;

        case PANGO2_ATTR_LETTER_SPACING:
          properties->letter_spacing = attr->int_value;
          break;

        case PANGO2_ATTR_LINE_HEIGHT:
          properties->line_height = attr->double_value;
          break;

        case PANGO2_ATTR_ABSOLUTE_LINE_HEIGHT:
          properties->absolute_line_height = attr->int_value;
          break;

        case PANGO2_ATTR_LINE_SPACING:
          properties->line_spacing = attr->int_value;
          break;

        case PANGO2_ATTR_SHOW:
          properties->showing_space = (attr->int_value & PANGO2_SHOW_SPACES) != 0;
          break;

        case PANGO2_ATTR_PARAGRAPH:
          properties->no_paragraph_break = TRUE;
          break;

        case PANGO2_ATTR_SHAPE:
          properties->shape = attr;
          break;

        default:
          break;
        }
      tmp_list = tmp_list->next;
    }
}

/**
 * pango2_analysis_get_font:
 * @analysis: a `Pango2Analysis`
 *
 * Returns the font that will be used for text
 * with this `Pango2Analysis`.
 *
 * Return value: (transfer none): the `Pango2Font`
 */
Pango2Font *
pango2_analysis_get_font (const Pango2Analysis *analysis)
{
  return analysis->font;
}

/**
 * pango2_analysis_get_bidi_level:
 * @analysis: a `Pango2Analysis`
 *
 * Returns the bidi embedding level for text
 * with this `Pango2Analysis`.
 *
 * Return value: the bidi embedding level
 */
int
pango2_analysis_get_bidi_level (const Pango2Analysis *analysis)
{
  return analysis->level;
}

/**
 * pango2_analysis_get_gravity:
 * @analysis: a `Pango2Analysis`
 *
 * Returns the gravity for text with this `Pango2Analysis`.
 *
 * Return value: the gravity
 */
Pango2Gravity
pango2_analysis_get_gravity (const Pango2Analysis *analysis)
{
  return (Pango2Gravity) analysis->gravity;
}

/**
 * pango2_analysis_get_flags:
 * @analysis: a `Pango2Analysis`
 *
 * Returns flags for this `Pango2Analysis`.
 *
 * Possible flag values are
 * `PANGO2_ANALYSIS_FLAG_CENTERED_BASELINE`,
 * `PANGO2_ANALYSIS_FLAG_IS_ELLIPSIS` and
 * `PANGO2_ANALYSIS_FLAG_NEED_HYPHEN`.
 *
 * Return value: the flags
 */
guint
pango2_analysis_get_flags (const Pango2Analysis *analysis)
{
  return analysis->flags;
}

/**
 * pango2_analysis_get_script:
 * @analysis: a `Pango2Analysis`
 *
 * Returns the script for text with this `Pango2Analysis`.
 *
 * Return value: the script
 */
GUnicodeScript
pango2_analysis_get_script (const Pango2Analysis *analysis)
{
  return (GUnicodeScript) analysis->script;
}

/**
 * pango2_analysis_get_language:
 * @analysis: a `Pango2Analysis`
 *
 * Returns the language for text with this `Pango2Analysis`.
 *
 * Return value: the script
 */
Pango2Language *
pango2_analysis_get_language (const Pango2Analysis *analysis)
{
  return analysis->language;
}

/**
 * pango2_analysis_get_extra_attributes:
 * @analysis: a `Pango2Analysis`
 *
 * Returns attributes to apply to text with this
 * `Pango2Analysis`.
 *
 * Return value: (transfer none) (element-type Pango2Attribute):
 *   a `GSList` with `Pango2Attribute` values
 */
GSList *
pango2_analysis_get_extra_attributes (const Pango2Analysis *analysis)
{
  return analysis->extra_attrs;
}

/**
 * pango2_item_get_analysis:
 * @item: a `Pango2Item`
 *
 * Returns the `Pango2Analysis` of @item.
 *
 * Return value: (transfer none): a `Pango2Analysis`
 */
const Pango2Analysis *
pango2_item_get_analysis (Pango2Item *item)
{
  return &item->analysis;
}

/**
 * pango2_item_get_byte_offset:
 * @item: a `Pango2Item`
 *
 * Returns the byte offset of this items
 * text in the overall paragraph text.
 *
 * Return value: the byte offset
 */
int
pango2_item_get_byte_offset (Pango2Item *item)
{
  return item->offset;
}

/**
 * pango2_item_get_byte_length:
 * @item: a `Pango2Item`
 *
 * Returns the length of this items
 * text in bytes.
 *
 * Return value: the length of @item
 */
int
pango2_item_get_byte_length (Pango2Item *item)
{
  return item->length;
}

/**
 * pango2_item_get_char_offset:
 * @item: a `Pango2Item`
 *
 * Returns the offset of this items text
 * in the overall paragraph text, in characters.
 *
 * Returns value: the character offset
 */
int
pango2_item_get_char_offset (Pango2Item *item)
{
  return item->char_offset;
}

/**
 * pango2_item_get_char_length:
 * @item: a `Pango2Item`
 *
 * Returns the number of characters in this
 * items text.
 *
 * Return value: the number of characters in @item
 */
int
pango2_item_get_char_length (Pango2Item *item)
{
  return item->num_chars;
}
