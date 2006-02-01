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

#include <math.h>

#include "pango-impl-utils.h"
#include "pangocairo.h"
#include "pangocairo-private.h"
#include "pango-utils.h"

PangoCairoWarningHistory _pango_cairo_warning_history = { FALSE };

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
  if (G_UNLIKELY (!PANGO_IS_CAIRO_FONT (font)))
    {
      if (!_pango_cairo_warning_history.font_install)
        {
	  _pango_cairo_warning_history.font_install = TRUE;
	  g_critical ("_pango_cairo_font_install called with font == NULL, expect ugly output");
	}
      return;
    }
  
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

  /* for metrics hinting */
  double scale_x, scale_x_inv, scale_y, scale_y_inv;

  int i;
  int rows;
  double pad;
  double width = 0;
  double height = 0;
  cairo_font_extents_t font_extents;
  PangoFontDescription *mini_desc, *desc;
  cairo_scaled_font_t *scaled_font, *scaled_mini_font;

  hbi = (PangoCairoHexBoxInfo *) g_object_get_data (G_OBJECT (cfont), "hex_box_info");
  if (hbi)
    return hbi;

  scaled_font = _pango_cairo_font_get_scaled_font (cfont);  

  /* prepare for some hinting */
  {
    cairo_matrix_t ctm;
    double x, y;
    cairo_scaled_font_get_ctm (scaled_font, &ctm);

    x = 1.; y = 0.;
    cairo_matrix_transform_distance (&ctm, &x, &y);
    scale_x = sqrt (x*x + y*y);
    scale_x_inv = 1 / scale_x;

    x = 0.; y = 1.;
    cairo_matrix_transform_distance (&ctm, &x, &y);
    scale_y = sqrt (x*x + y*y);
    scale_y_inv = 1 / scale_y;
  }

/* we hint to the nearest device units */
#define HINT(value, scale, scale_inv) (ceil ((value) * scale) * scale_inv)
#define HINT_X(value) HINT ((value), scale_x, scale_x_inv)
#define HINT_Y(value) HINT ((value), scale_y, scale_y_inv)
  
  /* create mini_font description */
  {
    double size, mini_size;

    desc = pango_font_describe ((PangoFont *)cfont);
    size = pango_font_description_get_size (desc) / (1.*PANGO_SCALE);

    mini_desc = pango_font_description_new ();
    pango_font_description_set_family_static (mini_desc, "mono-space");

    /* TODO: The stuff here should give a shit to whether it's
     * absolute size or not. */
    rows = 2;
    mini_size = HINT_Y (size / 2.4);
    if (mini_size < 5.0)
      {
        rows = 1;
	mini_size = MIN (size, 5.0);
      }

    if (pango_font_description_get_size_is_absolute (desc))
	pango_font_description_set_absolute_size (mini_desc, mini_size * PANGO_SCALE);
    else
	pango_font_description_set_size (mini_desc, mini_size * PANGO_SCALE);

    pango_font_description_free (desc);
  }

  /* load mini_font */
  {
    PangoContext *context;
    PangoFontMap *fontmap;

    fontmap = pango_font_get_font_map ((PangoFont *)cfont);
    g_assert (fontmap);
    context = pango_cairo_font_map_create_context (PANGO_CAIRO_FONT_MAP (fontmap));

    pango_context_set_language (context, pango_language_from_string ("en"));
    mini_font = pango_font_map_load_font (fontmap, context, mini_desc);
    pango_font_description_free (mini_desc);

    g_object_unref (context);
    g_object_unref (fontmap);
  }


  mini_cfont = (PangoCairoFont *) mini_font;
  scaled_mini_font = _pango_cairo_font_get_scaled_font (mini_cfont);  

  for (i = 0 ; i < 16 ; i++)
    {
      cairo_text_extents_t extents;

      c[0] = hexdigits[i];
      cairo_scaled_font_text_extents (scaled_mini_font, c, &extents);
      width = MAX (width, extents.width);
      height = MAX (height, extents.height);
    }

  cairo_scaled_font_extents (scaled_font, &font_extents);

  hbi = g_slice_new (PangoCairoHexBoxInfo);
  hbi->font = mini_font;
  hbi->rows = rows;

  hbi->digit_width = HINT_X (width);
  hbi->digit_height = HINT_Y (height);

  pad = MIN (hbi->digit_width / 10, hbi->digit_height / 12);
  hbi->pad_x = HINT_X (pad);
  hbi->pad_y = HINT_Y (pad);
  hbi->line_width = MIN (hbi->pad_x, hbi->pad_y);

  hbi->box_height = 3 * hbi->pad_y + rows * (hbi->pad_y + hbi->digit_height);
  hbi->box_descent = HINT_Y (font_extents.descent -
			     (font_extents.ascent + font_extents.descent - hbi->box_height) / 2);

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
  gint rows, cols;

  hbi = _pango_cairo_get_hex_box_info (cfont);

  rows = hbi->rows;
  cols = ((glyph & ~PANGO_CAIRO_UNKNOWN_FLAG) > 0xffff ? 6 : 4) / rows;
  
  if (ink_rect)
    {
      ink_rect->x = PANGO_SCALE * hbi->pad_x;
      ink_rect->y = PANGO_SCALE * (hbi->box_descent - hbi->box_height);
      ink_rect->width = PANGO_SCALE * (3 * hbi->pad_x + cols * (hbi->digit_width + hbi->pad_x));
      ink_rect->height = PANGO_SCALE * hbi->box_height;
    }
  
  if (logical_rect)
    {
      logical_rect->x = 0;
      logical_rect->y = PANGO_SCALE * (hbi->box_descent - (hbi->box_height + hbi->pad_y));
      logical_rect->width = PANGO_SCALE * (5 * hbi->pad_x + cols * (hbi->digit_width + hbi->pad_x));
      logical_rect->height = PANGO_SCALE * (hbi->box_height + 2 * hbi->pad_y);
    }  
}

