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

#include <pango-attributes.h>

struct _PangoAttrList
{
  GSList *attributes;
};

struct _PangoAttrIterator
{
  GSList *next_attribute;
  GList *attribute_stack;
};

/**
 * pango_attribute_register_type:
 * @name: an identifier for the type. (Currently unused.)
 * 
 * Allocate a new attribute type ID.
 * 
 * Return value: the new type ID.
 **/
PangoAttributeType
pango_attribute_register_type (const gchar *name)
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
pango_attribute_copy (PangoAttribute *attr)
{
  g_return_if_fail (attr != NULL);

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

  attr->klass->free (attr);
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
pango_attribute_compare (PangoAttribute    *attr1,
			 PangoAttribute    *attr2)
{
  g_return_if_fail (attr1 != NULL);
  g_return_if_fail (attr2 != NULL);

  if (attr1->klass->type != attr2->klass->type)
    return FALSE;

  return attr1->klass->compare (attr1, attr2);
}

static PangoAttribute
pango_attr_lang_copy (PangoAttribute *attr);
{
  return pango_attr_lang_new (((PangoAttrLang *)attr)->lang);
}

static gboolean
pango_attr_lang_destroy (PangoAttribute *attr);
{
  g_free (((PangoAttrLang *)attr)->lang);
  g_free (attr);
}

static gboolean
pango_attr_lang_compare (const PangoAttribute *attr1,
			 const PangoAttribute *attr2)
{
  return strcmp (((PangoAttrLang *)attr1)->lang, ((PangoAttrLang *)attr2)->lang) == 0;
}

PangoAttribute *
pango_attr_lang_new (const char *lang)
{
  static const PangoAttributeClass klass = {
    PANGO_ATTR_LANG
    pango_attr_lang_copy,
    pango_attr_lang_destroy,
    pango_attr_lang_compare,
  };

  PangoAttrLang *result = g_new (PangoAttrLang, 1);
  result->lang = g_strdup (lang);

  return (PangoAttribute *)result;
}

PangoAttribute *pango_attr_color_new (guint16 red,
				      guint16 green,
				      guint16 blue)
{
}

PangoAttribute *
pango_attr_size_new (double size)
{
}

PangoAttribute *
pango_attr_style_new (PangoStyle style)
{
}

PangoAttribute *
pango_attr_weight_new (PangoWeight weight)
{
}

PangoAttribute *
pango_attr_variant_new (PangoVariant variant)
{
}

PangoAttribute *
pango_attr_stretch_new (PangoStretch  stretch)
{
}

PangoAttribute *
pango_attr_underline_new (gboolean underline)
{
}

PangoAttribute *
pango_attr_strikethrough_new (gboolean strikethrough)
{
}

PangoAttribute *
pango_attr_rise_new (int rise)
{
}

PangoAttrList *
pango_attr_list_new (void)
{
}

PangoAttrList *
pango_attr_list_ref (PangoAttrList *list)
{
}

PangoAttrList *
pango_attr_list_unref (PangoAttrList *list)
{
}

PangoAttrList *
pango_attr_list_append (PangoAttrList  *list,
			PangoAttribute *attr)
{
}

PangoAttrList *
pango_attr_list_change (PangoAttrList  *list,
				       PangoAttribute *attr)
{
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
  PangoAttrIterator *result;

  g_return_if_fail (list != NULL);

  result = g_new (PangoAttrIterator, 1);
  result->next_attribute = list->attributes;
  result->attribute_stack = NULL;

  while (result->next_attribute &&
	 ((PangoAttribute *)result->next_attribute->data)->start_index == 0)
    {
      result->attribute_stack = g_list_prepend (result->attribute_stack, result->next_attribute->data);
      result->next_attribute = result->next_attribute->next;
    }
}

/**
 * pango_attr_iterator_next:
 * @iterator: a #PangoAttrIterator
 * 
 * Advance the iterator until the next change of style.
 * 
 * Return value: the index of the next change of position, or -1 if iterator
 *               is at the end of the list.
 **/
int
pango_attr_iterator_next (PangoAttrIterator *iterator)
{
  int next_index = G_MAXINT;
  GList *tmp_list;

  g_return_if_fail (iterator != NULL);

  tmp_list = result->attribute_stack;
  while (tmp_list)
    {
      PangoAttribute *attr = tmp_list->data;
      if (attr->end_index < next_index)
	next_index = attr->end_index;
    }

  tmp_list = result->attribute_stack;
  
  if (result->next_attribute)
    {
      int next_start = ((PangoAttribute *)result->next_attribute->data)->start_index;

      if (next_start <= next_index)
	{
	  do
	    {
	      result->attribute_stack = g_list_prepend (result->attribute_stack, result->next_attribute->data);
	      result->next_attribute = result->next_attribute->next;
	    }
	  while (result->next_attribute &&
		 ((PangoAttribute *)result->next_attribute->data)->start_index == next_start);
	}

      if (next_start < next_index)
	return next_start;
    }

  while (tmp_list)
    {
      GList *next = tmp_list->next;

      if (((PangoAttribute *)tmp_list->data)->start_index == next_index)
	{
	  result->attribute_stack = g_list_remove_link (result->attribute_stack, tmp_list);
	  g_list_free_1 (tmp_list);
	}
      
      tmp_list = next;
    }
  
  return next_index;
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
 * location.
 * 
 * Return value: the current attribute of the given type, or %NULL
 *               if no attribute of that type applies to the current
 *               location.
 **/
PangoAttribute *
pango_attr_iterator_get (PangoAttrIterator *iterator,
			 PangoAttrType      type)
{
  GList *tmp_list = iterator->attribute_stack;

  while (tmp_list)
    {
      PangoAttribute *attr = tmp_list->data;

      if (attr->klass->type == type)
	return attr;
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
 **/
void
pango_attr_iterator_get_font (PangoAttrIterator    *iterator,
			      PangoFontDescription *base,
			      PangoFontDescription *current)
{
  *current = *base;
  PangoAttribute *attr;

  if ((attr = pango_attr_iterator_get (iterator, PANGO_ATTR_FAMILY)))
    current->family = ((PangoAttrFamily *)attr)->family;

  if ((attr = pango_attr_iterator_get (iterator, PANGO_ATTR_STYLE)))
    current->style = ((PangoAttrStyle *)attr)->style;
  
  if ((attr = pango_attr_iterator_get (iterator, PANGO_ATTR_VARIANT)))
    current->variant = ((PangoAttrVariant *)attr)->variant;
  
  if ((attr = pango_attr_iterator_get (iterator, PANGO_ATTR_WEIGHT)))
    current->weight = ((PangoAttrWeight *)attr)->weight;

  if ((attr = pango_attr_iterator_get (iterator, PANGO_ATTR_STRETCH)))
    current->stretch = ((PangoAttrStretch *)attr)->stretch;
}




