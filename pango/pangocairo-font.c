/* Pango
 * pangocairo-font.c: Cairo font handling
 *
 * Copyright (C) 2000-2005 Red Hat Software
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

#include "pangocairo.h"
#include "pangocairo-private.h"

GType
pango_cairo_font_get_type (void)
{
  static GType cairo_font_type = 0;

  if (! cairo_font_type)
    {
      static const GTypeInfo cairo_font_info =
      {
	sizeof (PangoCairoFontIface), /* class_size */
	NULL,           /* base_init */
	NULL,		/* base_finalize */
	NULL,
	NULL,		/* class_finalize */
	NULL,		/* class_data */
	0,
	0,
	NULL
      };

      cairo_font_type =
	g_type_register_static (G_TYPE_INTERFACE, "PangoCairoFont",
				&cairo_font_info, 0);

      g_type_interface_add_prerequisite (cairo_font_type, PANGO_TYPE_FONT);
    }

  return cairo_font_type;
}

/**
 * _pango_cairo_font_get_cairo_font:
 * @font: a #PangoCairoFont
 *
 * Get the cairo_font_t for a PangoCairoFont.
 **/
cairo_font_t *
_pango_cairo_font_get_cairo_font (PangoCairoFont *font)
{
  g_return_val_if_fail (PANGO_IS_CAIRO_FONT (font), NULL);
  
  return (* PANGO_CAIRO_FONT_GET_IFACE (font)->get_cairo_font) (font);
}
