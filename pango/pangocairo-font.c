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

#include <config.h>

#include "pangocairo.h"
#include "pangocairo-private.h"
#include "pango-utils.h"

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
	NULL,
	NULL
      };

      cairo_font_type =
	g_type_register_static (G_TYPE_INTERFACE, I_("PangoCairoFont"),
				&cairo_font_info, 0);

      g_type_interface_add_prerequisite (cairo_font_type, PANGO_TYPE_FONT);
    }

  return cairo_font_type;
}

/**
 * _pango_cairo_font_install:
 * @font: a #PangoCairoFont
 * @cr: a #cairo_t
 *
 * Makes @font the current font for rendering in the specified
 * Cairo context.
 **/
void
_pango_cairo_font_install (PangoCairoFont *font,
			   cairo_t        *cr)
{
  g_return_if_fail (PANGO_IS_CAIRO_FONT (font));
  
  (* PANGO_CAIRO_FONT_GET_IFACE (font)->install) (font, cr);
}

cairo_font_face_t *
_pango_cairo_font_get_font_face (PangoCairoFont *font)
{
  g_return_val_if_fail (PANGO_IS_CAIRO_FONT (font), NULL);
  
  return (* PANGO_CAIRO_FONT_GET_IFACE (font)->get_font_face) (font);
}

cairo_scaled_font_t *
_pango_cairo_font_get_scaled_font (PangoCairoFont *font)
{
  g_return_val_if_fail (PANGO_IS_CAIRO_FONT (font), NULL);
  
  return (* PANGO_CAIRO_FONT_GET_IFACE (font)->get_scaled_font) (font);
}

static void
_pango_cairo_hex_box_info_destroy (PangoCairoHexBoxInfo *hbi)
{
  if (hbi)
    {
      g_object_unref (hbi->font);
      g_slice_free (PangoCairoHexBoxInfo, hbi);
    }
}  
   
PangoCairoHexBoxInfo *
_pango_cairo_get_hex_box_info (PangoCairoFont *cfont)
{
  static const char hexdigits[] = "0123456789ABCDEF";
  char c[2] = {0, 0};
  PangoFont *mini_font;
  PangoCairoFont *mini_cfont;
  PangoCairoHexBoxInfo *hbi;
  int i;
  double width = 0;
  double height = 0;
  cairo_font_extents_t font_extents;
  PangoFontDescription *mini_desc, *desc;
  cairo_scaled_font_t *scaled_font, *scaled_mini_font;
  cairo_surface_t *surface;
  cairo_t *cr;

  hbi = (PangoCairoHexBoxInfo *) g_object_get_data (G_OBJECT (cfont), "hex_box_info");
  if (hbi)
    return hbi;


  mini_desc = pango_font_description_new ();
  desc = pango_font_describe ((PangoFont *)cfont);

  pango_font_description_set_family_static (mini_desc, "mono-space");

  /* set size on mini_desc */
  {
    int new_size;
    new_size = pango_font_description_get_size (desc) / 2.4 + .9;

    if (pango_font_description_get_size_is_absolute (desc))
	pango_font_description_set_absolute_size (mini_desc, new_size);
    else
	pango_font_description_set_size (mini_desc, new_size);
  }

  pango_font_description_free (desc);

  /* load mini_font */
  {
    PangoContext *context;
    PangoFontMap *fontmap;

    fontmap = pango_font_get_font_map ((PangoFont *)cfont);
    g_assert (fontmap);
    context = pango_cairo_font_map_create_context (PANGO_CAIRO_FONT_MAP (fontmap));

    pango_context_set_language (context, pango_language_from_string ("en"));
    mini_font = pango_font_map_load_font (fontmap, context, mini_desc);

    g_object_unref (context);
    g_object_unref (fontmap);
  }

  pango_font_description_free (mini_desc);

  mini_cfont = (PangoCairoFont *) mini_font;
  scaled_font = _pango_cairo_font_get_scaled_font (cfont);  
  scaled_mini_font = _pango_cairo_font_get_scaled_font (mini_cfont);  

  surface = cairo_image_surface_create (CAIRO_FORMAT_RGB24, 0, 0);
  cr = cairo_create (surface);
  _pango_cairo_font_install (mini_cfont, cr);
  cairo_surface_destroy (surface);

  for (i = 0 ; i < 16 ; i++)
    {
      cairo_text_extents_t extents;

      c[0] = hexdigits[i];
      cairo_text_extents (cr, c, &extents);      
      width = MAX (width, extents.width);
      height = MAX (height, extents.height);
    }

  cairo_destroy (cr);
  cairo_scaled_font_extents (scaled_font, &font_extents);

  hbi = g_slice_new (PangoCairoHexBoxInfo);
  hbi->font = mini_font;

  hbi->digit_width = width;
  hbi->digit_height = height;

  hbi->pad = hbi->digit_height / 8;

  hbi->box_height = 5 * hbi->pad + 2 * hbi->digit_height;
  hbi->box_descent = font_extents.descent -
		    (font_extents.ascent + font_extents.descent - hbi->box_height) / 2;
  
  g_object_set_data_full (G_OBJECT (cfont), "hex_box_info", hbi, (GDestroyNotify)_pango_cairo_hex_box_info_destroy); 

  return hbi;
}

void
_pango_cairo_get_glyph_extents_missing (PangoCairoFont *cfont,
				        PangoGlyph      glyph,
				        PangoRectangle *ink_rect,
				        PangoRectangle *logical_rect)
{
  PangoCairoHexBoxInfo *hbi;  
  gint cols;

  cols = (glyph & ~PANGO_CAIRO_UNKNOWN_FLAG) > 0xffff ? 3 : 2;
  hbi = _pango_cairo_get_hex_box_info (cfont);
  
  if (ink_rect)
    {
      ink_rect->x = PANGO_SCALE * hbi->pad;
      ink_rect->y = PANGO_SCALE * (hbi->box_descent - hbi->box_height);
      ink_rect->width = PANGO_SCALE * (3 * hbi->pad + cols * (hbi->digit_width + hbi->pad));
      ink_rect->height = PANGO_SCALE * hbi->box_height;
    }
  
  if (logical_rect)
    {
      logical_rect->x = 0;
      logical_rect->y = PANGO_SCALE * (hbi->box_descent - (hbi->box_height + hbi->pad));
      logical_rect->width = PANGO_SCALE * (5 * hbi->pad + cols * (hbi->digit_width + hbi->pad));
      logical_rect->height = PANGO_SCALE * (hbi->box_height + 2 * hbi->pad);
    }  
}

