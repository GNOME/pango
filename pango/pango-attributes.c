/* Pango
 * pango-attributes.c: Attributed text
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

#include <pango/pango-attributes.h>

struct _PangoAttrList
{
  guint ref_count;
  GSList *attributes;
  GSList *attributes_tail;
};

struct _PangoAttrIterator
{
  GSList *next_attribute;
  GList *attribute_stack;
  int start_index;
  int end_index;
};

static PangoAttribute *pango_attr_color_new  (const PangoAttrClass *klass,
					      guint16         red,
					      guint16         green,
					      guint16         blue);
static PangoAttribute *pango_attr_string_new (const PangoAttrClass *klass,
					      const char     *str);
static PangoAttribute *pango_attr_int_new    (const PangoAttrClass *klass,
					      int             value);

/**
 * pango_attr_type_register:
 * @name: an identifier for the type. (Currently unused.)
 * 
 * Allocate a new attribute type ID.
 * 
 * Return value: the new type ID.
 **/
PangoAttrType
pango_attr_type_register (const gchar *name)
{
  static guint current_type = 0x1000000;

  return current_type++;
}

/**
 * pango_attribute_copy:
 * @attr: a #PangoAttribute.
 * 
 * Make a copy of an attribute.
 * 
 * Return value: a newly allocated #PangoAttribute.
 **/
PangoAttribute *
pango_attribute_copy (const PangoAttribute *attr)
{
  g_return_val_if_fail (attr != NULL, NULL);

  return attr->klass->copy (attr);
}

/**
 * pango_attribute_destroy:
 * @attr: a #PangoAttribute.
 * 
 * Destroy a #PangoAttribute and free all associated memory.
 **/
void
pango_attribute_destroy (PangoAttribute *attr)
{
  g_return_if_fail (attr != NULL);

  attr->klass->destroy (attr);
}

/**
 * pango_attribute_compare:
 * @attr1: a #PangoAttribute
 * @attr2: another #PangoAttribute
 * 
 * Compare two attributes for equality. This compares only the
 * actual value of the two attributes and not the ranges that the
 * attributes apply to.
 * 
 * Return value: %TRUE if the two attributes have the same value.
 **/
gboolean
pango_attribute_compare (const PangoAttribute *attr1,
			 const PangoAttribute *attr2)
{
  g_return_val_if_fail (attr1 != NULL, FALSE);
  g_return_val_if_fail (attr2 != NULL, FALSE);

  if (attr1->klass->type != attr2->klass->type)
    return FALSE;

  return attr1->klass->compare (attr1, attr2);
}

static PangoAttribute *
pango_attr_string_copy (const PangoAttribute *attr)
{
  return pango_attr_string_new (attr->klass, ((PangoAttrString *)attr)->value);
}

static void
pango_attr_string_destroy (PangoAttribute *attr)
{
  g_free (((PangoAttrString *)attr)->value);
  g_free (attr);
}

static gboolean
pango_attr_string_compare (const PangoAttribute *attr1,
			   const PangoAttribute *attr2)
{
  return strcmp (((PangoAttrString *)attr1)->value, ((PangoAttrString *)attr2)->value) == 0;
}

static PangoAttribute *
pango_attr_string_new (const PangoAttrClass *klass,
		       const char           *str)
{
  PangoAttrString *result = g_new (PangoAttrString, 1);

  result->attr.klass = klass;
  result->value = g_strdup (str);

  return (PangoAttribute *)result;
}

/**
 * pango_attr_lang_new:
 * @lang: language tag (in the form "en_US")
 * 
 * Create a new language tag attribute. 
 * 
 * Return value: the new #PangoAttribute.
 **/
PangoAttribute *
pango_attr_lang_new (const char *lang)
{
  static const PangoAttrClass klass = {
    PANGO_ATTR_LANG,
    pango_attr_string_copy,
    pango_attr_string_destroy,
    pango_attr_string_compare
  };

  g_return_val_if_fail (lang != NULL, NULL);
  
  return pango_attr_string_new (&klass, lang);
}

/**
 * pango_attr_family_new:
 * @family: the family or comma separated list of families
 * 
 * Create a new font family attribute.
 * 
 * Return value: the new #PangoAttribute.
 **/
PangoAttribute *
pango_attr_family_new (const char *family)
{
  static const PangoAttrClass klass = {
    PANGO_ATTR_FAMILY,
    pango_attr_string_copy,
    pango_attr_string_destroy,
    pango_attr_string_compare
  };

  g_return_val_if_fail (family != NULL, NULL);
  
  return pango_attr_string_new (&klass, family);
}

static PangoAttribute *
pango_attr_color_copy (const PangoAttribute *attr)
{
  const PangoAttrColor *color_attr = (PangoAttrColor *)attr;
  
  return pango_attr_color_new (attr->klass,
			       color_attr->red, color_attr->green, color_attr->blue);
}

static void
pango_attr_color_destroy (PangoAttribute *attr)
{
  g_free (attr);
}

static gboolean
pango_attr_color_compare (const PangoAttribute *attr1,
			  const PangoAttribute *attr2)
{
  const PangoAttrColor *color_attr1 = (const PangoAttrColor *)attr1;
  const PangoAttrColor *color_attr2 = (const PangoAttrColor *)attr2;
  
  return (color_attr1->red == color_attr2->red &&
	  color_attr1->blue == color_attr2->blue &&
	  color_attr1->green == color_attr2->green);
}

static PangoAttribute *
pango_attr_color_new (const PangoAttrClass *klass,
		      guint16               red,
		      guint16               green,
		      guint16               blue)
{
  PangoAttrColor *result = g_new (PangoAttrColor, 1);
  result->attr.klass = klass;
  result->red = red;
  result->green = green;
  result->blue = blue;

  return (PangoAttribute *)result;
}

/**
 * pango_attr_foreground_new:
 * @red: the red value (ranging from 0 to 65535)
 * @green: the green value
 * @blue: the blue value
 * 
 * Create a new foreground color attribute.
 * 
 * Return value: the new #PangoAttribute.
 **/
PangoAttribute *
pango_attr_foreground_new (guint16 red,
			   guint16 green,
			   guint16 blue)
{
  static const PangoAttrClass klass = {
    PANGO_ATTR_FOREGROUND,
    pango_attr_color_copy,
    pango_attr_color_destroy,
    pango_attr_color_compare
  };

  return pango_attr_color_new (&klass, red, green, blue);
}

/**
 * pango_attr_background_new:
 * @red: the red value (ranging from 0 to 65535)
 * @green: the green value
 * @blue: the blue value
 * 
 * Create a new background color attribute.
 * 
 * Return value: the new #PangoAttribute.
 **/
PangoAttribute *
pango_attr_background_new (guint16 red,
			   guint16 green,
			   guint16 blue)
{
  static const PangoAttrClass klass = {
    PANGO_ATTR_BACKGROUND,
    pango_attr_color_copy,
    pango_attr_color_destroy,
    pango_attr_color_compare
  };

  return pango_attr_color_new (&klass, red, green, blue);
}

static PangoAttribute *
pango_attr_int_copy (const PangoAttribute *attr)
{
  const PangoAttrInt *int_attr = (PangoAttrInt *)attr;
  
  return pango_attr_int_new (attr->klass, int_attr->value);
}

static void
pango_attr_int_destroy (PangoAttribute *attr)
{
  g_free (attr);
}

static gboolean
pango_attr_int_compare (const PangoAttribute *attr1,
			  const PangoAttribute *attr2)
{
  const PangoAttrInt *int_attr1 = (const PangoAttrInt *)attr1;
  const PangoAttrInt *int_attr2 = (const PangoAttrInt *)attr2;
  
  return (int_attr1->value == int_attr2->value);
}

static PangoAttribute *
pango_attr_int_new (const PangoAttrClass *klass,
		    int                   value)
{
  PangoAttrInt *result = g_new (PangoAttrInt, 1);
  result->attr.klass = klass;
  result->value = value;

  return (PangoAttribute *)result;
}

/**
 * pango_attr_size_new:
 * @size: the font size, in 1000ths of a point.
 * 
 * Create a new font-size attribute.
 * 
 * Return value: the new #PangoAttribute.
 **/
PangoAttribute *
pango_attr_size_new (int size)
{
  static const PangoAttrClass klass = {
    PANGO_ATTR_SIZE,
    pango_attr_int_copy,
    pango_attr_int_destroy,
    pango_attr_int_compare
  };

  return pango_attr_int_new (&klass, size);
}

/**
 * pango_attr_style_new:
 * @style: the slant style
 * 
 * Create a new font slant style attribute.
 * 
 * Return value: the new #PangoAttribute.
 **/
PangoAttribute *
pango_attr_style_new (PangoStyle style)
{
  static const PangoAttrClass klass = {
    PANGO_ATTR_STYLE,
    pango_attr_int_copy,
    pango_attr_int_destroy,
    pango_attr_int_compare
  };

  return pango_attr_int_new (&klass, (int)style);
}

/**
 * pango_attr_weight_new:
 * @weight: the weight
 * 
 * Create a new font weight attribute.
 * 
 * Return value: the new #PangoAttribute.
 **/
PangoAttribute *
pango_attr_weight_new (PangoWeight weight)
{
  static const PangoAttrClass klass = {
    PANGO_ATTR_WEIGHT,
    pango_attr_int_copy,
    pango_attr_int_destroy,
    pango_attr_int_compare
  };

  return pango_attr_int_new (&klass, (int)weight);
}

/**
 * pango_attr_variant_new:
 * @variant: the variant
 *
 * Create a new font variant attribute (normal or small caps)
 * 
 * Return value: the new #PangoAttribute.
 **/
PangoAttribute *
pango_attr_variant_new (PangoVariant variant)
{
  static const PangoAttrClass klass = {
    PANGO_ATTR_VARIANT,
    pango_attr_int_copy,
    pango_attr_int_destroy,
    pango_attr_int_compare
  };

  return pango_attr_int_new (&klass, (int)variant);
}

/**
 * pango_attr_stretch_new:
 * @stretch: the stretch
 * 
 * Create a new font stretch attribute
 * 
 * Return value: the new #PangoAttribute.
 **/
PangoAttribute *
pango_attr_stretch_new (PangoStretch  stretch)
{
  static const PangoAttrClass klass = {
    PANGO_ATTR_STRETCH,
    pango_attr_int_copy,
    pango_attr_int_destroy,
    pango_attr_int_compare
  };

  return pango_attr_int_new (&klass, (int)stretch);
}

static PangoAttribute *
pango_attr_font_desc_copy (const PangoAttribute *attr)
{
  const PangoAttrFontDesc *desc_attr = (const PangoAttrFontDesc *)attr;
  
  return pango_attr_font_desc_new (&desc_attr->desc);
}

static void
pango_attr_font_desc_destroy (PangoAttribute *attr)
{
  PangoAttrFontDesc *desc_attr = (PangoAttrFontDesc *)attr;

  g_free (desc_attr->desc.family_name);
  g_free (attr);
}

static gboolean
pango_attr_font_desc_compare (const PangoAttribute *attr1,
			      const PangoAttribute *attr2)
{
  const PangoAttrFontDesc *desc_attr1 = (const PangoAttrFontDesc *)attr1;
  const PangoAttrFontDesc *desc_attr2 = (const PangoAttrFontDesc *)attr2;
  
  return pango_font_description_compare (&desc_attr1->desc, &desc_attr2->desc);
}

/**
 * pango_attr_font_desc_new:
 * @desc: 
 * 
 * Create a new font description attribute. (This attribute
 * allows setting family, style, weight, variant, stretch,
 * and size simultaneously.)
 * 
 * Return value: 
 **/
PangoAttribute *
pango_attr_font_desc_new (const PangoFontDescription *desc)
{
  static const PangoAttrClass klass = {
    PANGO_ATTR_FONT_DESC,
    pango_attr_font_desc_copy,
    pango_attr_font_desc_destroy,
    pango_attr_font_desc_compare
  };

  PangoAttrFontDesc *result = g_new (PangoAttrFontDesc, 1);
  result->attr.klass = &klass;
  result->desc = *desc;
  result->desc.family_name = g_strdup (desc->family_name);

  return (PangoAttribute *)result;
}


/**
 * pango_attr_underline_new:
 * @underline: the underline style.
 * 
 * Create a new underline-style object.
 * 
 * Return value: the new #PangoAttribute.
 **/
PangoAttribute *
pango_attr_underline_new (PangoUnderline underline)
{
  static const PangoAttrClass klass = {
    PANGO_ATTR_UNDERLINE,
    pango_attr_int_copy,
    pango_attr_int_destroy,
    pango_attr_int_compare
  };

  return pango_attr_int_new (&klass, (int)underline);
}

/**
 * pango_attr_strikethrough_new:
 * @strikethrough: %TRUE if the text should be struck-through.
 * 
 * Create a new font strike-through attribute.
 * 
 * Return value: the new #PangoAttribute.
 **/
PangoAttribute *
pango_attr_strikethrough_new (gboolean strikethrough)
{
  static const PangoAttrClass klass = {
    PANGO_ATTR_STRIKETHROUGH,
    pango_attr_int_copy,
    pango_attr_int_destroy,
    pango_attr_int_compare
  };

  return pango_attr_int_new (&klass, (int)strikethrough);
}

/**
 * pango_attr_rise_new:
 * @rise: the amount that the text should be displaced vertically,
 *        in 10'000ths of an em. Positive values displace the
 *        text upwards.
 * 
 * Create a new baseline displacement attribute.
 * 
 * Return value: the new #PangoAttribute.
 **/
PangoAttribute *
pango_attr_rise_new (int rise)
{
  static const PangoAttrClass klass = {
    PANGO_ATTR_RISE,
    pango_attr_int_copy,
    pango_attr_int_destroy,
    pango_attr_int_compare
  };

  return pango_attr_int_new (&klass, (int)rise);
}

/**
 * pango_attr_list_new:
 * 
 * Create a new empty attribute list with a reference count of 1.
 * 
 * Return value: the newly allocated #PangoAttrList.
 **/
PangoAttrList *
pango_attr_list_new (void)
{
  PangoAttrList *list = g_new (PangoAttrList, 1);

  list->ref_count = 1;
  list->attributes = NULL;
  list->attributes_tail = NULL;

  return list;
}

/**
 * pango_attr_list_ref:
 * @list: a #PangoAttrList
 * 
 * Increase the reference count of the given attribute list by one.
 **/
void
pango_attr_list_ref (PangoAttrList *list)
{
  g_return_if_fail (list != NULL);

  list->ref_count++;
}

/**
 * pango_attr_list_unref:
 * @list: a #PangoAttrList
 * 
 * Decrease the reference count of the given attribute list by one.
 * If the result is zero, free the attribute list and the attributes
 * it contains.
 **/
void
pango_attr_list_unref (PangoAttrList *list)
{
  GSList *tmp_list;
  
  g_return_if_fail (list != NULL);
  g_return_if_fail (list->ref_count > 0);

  list->ref_count--;
  if (list->ref_count == 0)
    {
      tmp_list = list->attributes;
      while (tmp_list)
	{
	  PangoAttribute *attr = tmp_list->data;
	  tmp_list = tmp_list->next;

	  attr->klass->destroy (attr);
	}

      g_slist_free (list->attributes);

      g_free (list);
    }
}

/**
 * pango_attr_list_insert:
 * @list: a #PangoAttrList
 * @attr: the attribute to insert. Ownership of this value is
 *        assumed by the list.
 * 
 * Insert the given attribute into the #PangoAttrList. It will
 * be inserted after all other attributes with a matching
 * @start_index.
 **/
void
pango_attr_list_insert (PangoAttrList  *list,
			PangoAttribute *attr)
{
  GSList *tmp_list, *prev, *link;
  gint start_index = attr->start_index;
  
  g_return_if_fail (list != NULL);

  if (!list->attributes)
    {
      list->attributes = g_slist_prepend (NULL, attr);
      list->attributes_tail = list->attributes;
    }
  else if (((PangoAttribute *)list->attributes_tail->data)->start_index <= start_index)
    {
      g_slist_append (list->attributes_tail, attr);
    }
  else
    {
      prev = NULL;
      tmp_list = list->attributes;
      while (1)
	{
	  PangoAttribute *tmp_attr = tmp_list->data;
	  
	  if (tmp_attr->start_index > start_index)
	    {
	      link = g_slist_alloc ();
	      link->next = tmp_list;
	      link->data = attr;

	      if (prev)
		prev->next = link;
	      else
		list->attributes = link;

	      if (!tmp_list)
		list->attributes_tail = link;
	    }
	  
	  prev = tmp_list;
	  tmp_list = tmp_list->next;
	}
    }
}

/**
 * pango_attr_list_change:
 * @list: a #PangoAttrList
 * @attr: the attribute to insert. Ownership of this value is
 *        assumed by the list.
 * 
 * Insert the given attribute into the #PangoAttrList. It will
 * replace any attributes of the same type on that segment
 * and be merged with any adjoining attributes that are identical.
 *
 * This function is slower than pango_attr_list_insert() for
 * creating a attribute list in order (potentially much slower
 * for large lists). However, pango_attr_list_insert() is not
 * suitable for continually changing a set of attributes 
 * since it never removes or combines existing attributes.
 **/
void
pango_attr_list_change (PangoAttrList  *list,
			PangoAttribute *attr)
{
  GSList *tmp_list, *prev, *link;
  gint start_index = attr->start_index;
  gint end_index = attr->end_index;
  
  g_return_if_fail (list != NULL);

  tmp_list = list->attributes;
  prev = NULL;
  while (tmp_list)
    {
      PangoAttribute *tmp_attr;

      if (!tmp_list ||
	  ((PangoAttribute *)tmp_list->data)->start_index > start_index)
	{
	  /* We need to insert a new attribute
	   */
	  link = g_slist_alloc ();
	  link->next = tmp_list;
	  link->data = attr;
	  
	  if (prev)
	    prev->next = link;
	  else
	    list->attributes = link;
	  
	  if (!tmp_list)
	    list->attributes_tail = link;
	  
	  prev = link;
	}

      tmp_attr = tmp_list->data;

      if (tmp_attr->end_index >= start_index &&
	       pango_attribute_compare (tmp_attr, attr))
	{
	  /* We can merge the new attribute with this attribute
	   */
	  end_index = MAX (end_index, tmp_attr->end_index);
	  tmp_attr->end_index = end_index;
	  pango_attribute_destroy (attr);

	  attr = tmp_attr;

	  prev = tmp_list;
	  tmp_list = tmp_list->next;

	  break;
	}
      
      prev = tmp_list;
      tmp_list = tmp_list->next;
    }

  /* We now have the range inserted into the list one way or the
   * other. Fix up the remainder
   */
  while (tmp_list)
    {
      PangoAttribute *tmp_attr = tmp_list->data;

      if (tmp_attr->start_index > end_index)
	break;
      else if (pango_attribute_compare (tmp_attr, attr))
	{
	  /* We can merge the new attribute with this attribute
	   */
	  attr->end_index = MAX (end_index, tmp_attr->end_index);
	  
	  pango_attribute_destroy (tmp_attr);
	  prev->next = tmp_list->next;

	  g_slist_free_1 (tmp_list);
	  tmp_list = prev->next;
	}
      else
	{
	  prev = tmp_list;
	  tmp_list = tmp_list->next;
	}
    }
}

/**
 * pango_attr_list_get_iterator:
 * @list: a #PangoAttrList
 * 
 * Create a iterator initialized to the beginning of the list.
 * 
 * Return value: a new #PangoIterator. @list must not be modified
 *               until this iterator is freed with pango_attr_iterator_destroy().
 **/
PangoAttrIterator *
pango_attr_list_get_iterator (PangoAttrList  *list)
{
  PangoAttrIterator *iterator;

  g_return_val_if_fail (list != NULL, NULL);

  iterator = g_new (PangoAttrIterator, 1);
  iterator->next_attribute = list->attributes;
  iterator->attribute_stack = NULL;

  iterator->start_index = 0;
  iterator->end_index = 0;

  if (!pango_attr_iterator_next (iterator))
    iterator->end_index = G_MAXINT;

  return iterator;
}

/**
 * pango_attr_iterator_range:
 * @iterator: a #PangoAttrIterator
 * @start: location to store the start of the range
 * @end: location to store the end of the range
 * 
 * Get the range of the current segment.
 **/
void
pango_attr_iterator_range (PangoAttrIterator *iterator,
			   gint              *start,
			   gint              *end)
{
  g_return_if_fail (iterator != NULL);

  if (start)
    *start = iterator->start_index;
  if (end)
    *end = iterator->end_index;
}

/**
 * pango_attr_iterator_next:
 * @iterator: a #PangoAttrIterator
 * 
 * Advance the iterator until the next change of style.
 * 
 * Return value: %FALSE if the iterator is at the end of the list, otherwise %TRUE
 **/
gboolean
pango_attr_iterator_next (PangoAttrIterator *iterator)
{
  GList *tmp_list;

  g_return_val_if_fail (iterator != NULL, -1);

  if (!iterator->next_attribute && !iterator->attribute_stack)
    return FALSE;

  iterator->start_index = iterator->end_index;
  iterator->end_index = G_MAXINT;
  
  tmp_list = iterator->attribute_stack;
  while (tmp_list)
    {
      GList *next = tmp_list->next;
      PangoAttribute *attr = tmp_list->data;

      if (attr->end_index == iterator->start_index)
	{
	  iterator->attribute_stack = g_list_remove_link (iterator->attribute_stack, tmp_list);
	  g_list_free_1 (tmp_list);
	}
      else
	{
	  iterator->end_index = MIN (iterator->end_index, attr->end_index);
	}
      
      tmp_list = next;
    }

  while (iterator->next_attribute &&
	 ((PangoAttribute *)iterator->next_attribute->data)->start_index == iterator->start_index)
    {
      iterator->attribute_stack = g_list_prepend (iterator->attribute_stack, iterator->next_attribute->data);
      iterator->end_index = MIN (iterator->end_index, ((PangoAttribute *)iterator->next_attribute->data)->end_index);
      iterator->next_attribute = iterator->next_attribute->next;
    }

  if (iterator->next_attribute)
    iterator->end_index = MIN (iterator->end_index, ((PangoAttribute *)iterator->next_attribute->data)->start_index);

  return TRUE;
}

/**
 * pango_attr_iterator_destroy:
 * @iterator: a #PangoIterator.
 * 
 * Destroy a #PangoIterator and free all associated memory.
 **/
void
pango_attr_iterator_destroy (PangoAttrIterator *iterator)
{
  g_return_if_fail (iterator != NULL);
  
  g_list_free (iterator->attribute_stack);
  g_free (iterator);
}

/**
 * pango_attr_iterator_get:
 * @iterator: a #PangoIterator
 * @type: the type of attribute to find.
 * 
 * Find the current attribute of a particular type at the iterator
 * location. When multiple attributes of the same type overlap,
 * the attribute whose range starts closest to the current location
 * is used.
 * 
 * Return value: the current attribute of the given type, or %NULL
 *               if no attribute of that type applies to the current
 *               location.
 **/
PangoAttribute *
pango_attr_iterator_get (PangoAttrIterator *iterator,
			 PangoAttrType      type)
{
  GList *tmp_list;

  g_return_val_if_fail (iterator != NULL, NULL);

  tmp_list = iterator->attribute_stack;
  while (tmp_list)
    {
      PangoAttribute *attr = tmp_list->data;

      if (attr->klass->type == type)
	return attr;

      tmp_list = tmp_list->next;
    }

  return NULL;
}

/**
 * pango_attr_iterator_get_font:
 * @iterator: a #PangoIterator
 * @base: the default values to use for fields not currently overridden.
 * @current: a #PangoFontDescription to fill in with the current values. This
 *           #PangoFontDescription structure cannot be freed with
 *           pango_font_description_free. The @family member of this structure
 *           will be filled in with a string that is either shared with
 *           an attribute in the #PangoAttrList associated with the structure,
 *           or with @base. If you want to save this value, you should
 *           allocate it on the stack and then use pango_font_description_copy().
 * @extra_attrs: if non-%NULL, location in which to store a list of non-font
 *           attributes at the the current position; only the highest priority
 *           value of each attribute will be added to this list. In order
 *           to free this value, you must call pango_attribute_destroy() on
 *           each member.
 *
 * Get the font 
 **/
void
pango_attr_iterator_get_font (PangoAttrIterator     *iterator,
			      PangoFontDescription  *base,
			      PangoFontDescription  *current,
			      GSList               **extra_attrs)
{
  GList *tmp_list1;
  GSList *tmp_list2;

  gboolean have_family = FALSE;
  gboolean have_style = FALSE;
  gboolean have_variant = FALSE;
  gboolean have_weight = FALSE;
  gboolean have_stretch = FALSE;
  gboolean have_size = FALSE;

  g_return_if_fail (iterator != NULL);

  *current = *base;

  if (extra_attrs)
    *extra_attrs = NULL;
  
  tmp_list1 = iterator->attribute_stack;
  while (tmp_list1)
    {
      PangoAttribute *attr = tmp_list1->data;
      tmp_list1 = tmp_list1->next;

      switch (attr->klass->type)
	{
	case PANGO_ATTR_FONT_DESC:
	  {
	  if (!have_family)
	    {
	      have_family = TRUE;
	      current->family_name = ((PangoAttrFontDesc *)attr)->desc.family_name;
	    }
	  if (!have_style)
	    {
	      have_style = TRUE;
	      current->style = ((PangoAttrFontDesc *)attr)->desc.style;
	    }
	  if (!have_variant)
	    {
	      have_variant = TRUE;
	      current->variant = ((PangoAttrFontDesc *)attr)->desc.variant;
	    }
	  if (!have_weight)
	    {
	      have_weight = TRUE;
	      current->weight = ((PangoAttrFontDesc *)attr)->desc.weight;
	    }
	  if (!have_stretch)
	    {
	      have_stretch = TRUE;
	      current->stretch = ((PangoAttrFontDesc *)attr)->desc.stretch;
	    }
	  if (!have_size)
	    {
	      have_size = TRUE;
	      current->size = ((PangoAttrFontDesc *)attr)->desc.size;
	    }
	}
	  
	case PANGO_ATTR_FAMILY:
	  if (!have_family)
	    {
	      have_family = TRUE;
	      current->family_name = ((PangoAttrString *)attr)->value;
	    }
	  break;
	case PANGO_ATTR_STYLE:
	  if (!have_style)
	    {
	      have_style = TRUE;
	      current->style = ((PangoAttrInt *)attr)->value;
	    }
	  break;
	case PANGO_ATTR_VARIANT:
	  if (!have_variant)
	    {
	      have_variant = TRUE;
	      current->variant = ((PangoAttrInt *)attr)->value;
	    }
	  break;
	case PANGO_ATTR_WEIGHT:
	  if (!have_weight)
	    {
	      have_weight = TRUE;
	      current->weight = ((PangoAttrInt *)attr)->value;
	    }
	  break;
	case PANGO_ATTR_STRETCH:
	  if (!have_stretch)
	    {
	      have_stretch = TRUE;
	      current->stretch = ((PangoAttrInt *)attr)->value;
	    }
	  break;
	case PANGO_ATTR_SIZE:
	  if (!have_size)
	    {
	      have_size = TRUE;
	      current->size = ((PangoAttrInt *)attr)->value;
	    }
	  break;
	default:
	  if (extra_attrs)
	    {
	      gboolean found = FALSE;
	  
	      tmp_list2 = *extra_attrs;
	      while (tmp_list2)
		{
		  PangoAttribute *old_attr = tmp_list2->data;
		  if (attr->klass->type == old_attr->klass->type)
		    {
		      found = TRUE;
		      break;
		    }

		  tmp_list2 = tmp_list2->next;
		}

	      if (!found)
		*extra_attrs = g_slist_prepend (*extra_attrs, pango_attribute_copy (attr));
	    }
	}
    }
}
