/* Pango
 * fonts.c:
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

#include "pango.h"

/**
 * pango_font_init:
 * @font:    a #PangoFont
 *
 * Initialize a #PangoFont structure. This should
 * only be called from the "new" routine of code which
 * is implementing  a "subclass" of #PangoFont
 */
void 
pango_font_init (PangoFont *font)
{
  g_return_if_fail (font != NULL);

  g_datalist_init (&font->data);
  font->ref_count = 1;
}

/**
 * pango_font_ref:
 * @font:    a #PangoFont
 *
 * Increase the reference count of a #PangoFont.
 */
void 
pango_font_ref (PangoFont *font)
{
  g_return_if_fail (font != NULL);

  font->ref_count++;
}


/**
 * pango_font_unref:
 * @font: a #PangoFont
 *
 * Decrease the reference count of a #PangoFont.
 * if the result is zero, destroy the font
 * and free the associated memory.
 */
void 
pango_font_unref (PangoFont *font)
{
  g_return_if_fail (font != NULL);
  g_return_if_fail (font->ref_count > 0);
  
  font->ref_count--;
  if (font->ref_count == 0)
    {
      g_datalist_clear (&font->data);
      font->klass->destroy (font);
    }
}

/**
 * pango_font_set_data:
 * @font:         a #PangoFont
 * @key:          a string identifying the type of user data.
 * @data:         the data to store. If %NULL, the current
 *                data for the key will be removed.
 * @destroy_func: a function to call when the data is no
 *                longer stored, either because the font has
 *                been destroyed, or because the data has
 *                been replaced. This can be %NULL, in which
 *                case no function will be called.
 *
 * Associate user data, tagged with a string id, with a particular
 * font. 
 */
void
pango_font_set_data (PangoFont   *font,
		     gchar         *key,
		     gpointer       data,
		     GDestroyNotify destroy_func)
{
  g_datalist_set_data_full (&font->data, key, data, destroy_func);
}

/**
 * pango_font_get_data:
 * @font:    a #PangoFont
 * @key:     a string identifying the type of user data.
 *
 * Look up user data tagged with a particular key.
 * 
 * Returns the data, or NULL if that key does not exist.
 */
gpointer
pango_font_get_data (PangoFont *font,
			gchar       *key)
{
  return g_datalist_get_data (&font->data, key);
}

