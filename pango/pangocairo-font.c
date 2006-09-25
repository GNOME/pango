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

#include "pangocairo.h"
#include "pangocairo-private.h"
#include "pango-impl-utils.h"

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
gboolean
_pango_cairo_font_install (PangoCairoFont *font,
			   cairo_t        *cr)
{
  if (G_UNLIKELY (!PANGO_IS_CAIRO_FONT (font)))
    {
      if (!_pango_cairo_warning_history.font_install)
        {
	  _pango_cairo_warning_history.font_install = TRUE;
	  g_warning ("_pango_cairo_font_install called with bad font, expect ugly output");
	  cairo_set_font_face (cr, NULL);
	}
      return FALSE;
    }
  
  return (* PANGO_CAIRO_FONT_GET_IFACE (font)->install) (font, cr);
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
_pango_cairo_font_get_hex_box_info (PangoCairoFont *cfont)
{
  static const char hexdigits[] = "0123456789ABCDEF";
  char c[2] = {0, 0};
  PangoFont *mini_font;
  PangoCairoFont *mini_cfont;
  PangoCairoHexBoxInfo *hbi;

  /* for metrics hinting */
  double scale_x, scale_x_inv, scale_y, scale_y_inv;
  gboolean is_hinted;

  int i;
  int rows;
  double pad;
  double width = 0;
  double height = 0;
  cairo_font_options_t *font_options;
  cairo_font_extents_t font_extents;
  double size, mini_size;
  PangoFontDescription *desc, *mini_desc;
  cairo_scaled_font_t *scaled_font, *scaled_mini_font;
  PangoMatrix pango_ctm;
  cairo_matrix_t cairo_ctm;

  if (!cfont)
    return NULL;

  hbi = (PangoCairoHexBoxInfo *) g_object_get_data (G_OBJECT (cfont), "hex_box_info");
  if (hbi)
    return hbi;

  scaled_font = _pango_cairo_font_get_scaled_font (cfont);  
  if (!scaled_font)
    {
      g_object_set_data_full (G_OBJECT (cfont), "hex_box_info", NULL, NULL); 
      return NULL;
    }

  font_options = cairo_font_options_create ();
  cairo_scaled_font_get_font_options (scaled_font, font_options);
  is_hinted = (cairo_font_options_get_hint_metrics(font_options) != CAIRO_HINT_METRICS_OFF);

  cairo_scaled_font_get_ctm (scaled_font, &cairo_ctm);
  pango_ctm.xx = cairo_ctm.xx;
  pango_ctm.yx = cairo_ctm.yx;
  pango_ctm.xy = cairo_ctm.xy;
  pango_ctm.yy = cairo_ctm.yy;
  pango_ctm.x0 = cairo_ctm.x0;
  pango_ctm.y0 = cairo_ctm.y0;

  if (is_hinted)
    {
      /* prepare for some hinting */
      double x, y;

      x = 1.; y = 0.;
      cairo_matrix_transform_distance (&cairo_ctm, &x, &y);
      scale_x = sqrt (x*x + y*y);
      scale_x_inv = 1 / scale_x;

      x = 0.; y = 1.;
      cairo_matrix_transform_distance (&cairo_ctm, &x, &y);
      scale_y = sqrt (x*x + y*y);
      scale_y_inv = 1 / scale_y;
    }

/* we hint to the nearest device units */
#define HINT(value, scale, scale_inv) (ceil ((value-1e-5) * scale) * scale_inv)
#define HINT_X(value) HINT ((value), scale_x, scale_x_inv)
#define HINT_Y(value) HINT ((value), scale_y, scale_y_inv)
  
  /* create mini_font description */
  {
    PangoContext *context;
    PangoFontMap *fontmap;

    fontmap = pango_font_get_font_map ((PangoFont *)cfont);

    desc = pango_font_describe_with_absolute_size ((PangoFont *)cfont);
    size = pango_font_description_get_size (desc) / (1.*PANGO_SCALE);

    mini_desc = pango_font_description_new ();
    pango_font_description_set_family_static (mini_desc, "monospace");


    rows = 2;
    mini_size = size / 2.4;
    if (is_hinted)
      {
	mini_size = HINT_Y (mini_size);

	if (mini_size < 5.0)
	  {
	    rows = 1;
	    mini_size = MIN (MAX (size - 1, 0), 5.0);
	  }
      }

    pango_font_description_set_absolute_size (mini_desc, mini_size * PANGO_SCALE);


    /* load mini_font */

    context = pango_cairo_font_map_create_context (PANGO_CAIRO_FONT_MAP (fontmap));

    pango_context_set_matrix (context, &pango_ctm);
    pango_context_set_language (context, pango_language_from_string ("en"));
    pango_cairo_context_set_font_options (context, font_options);
    mini_font = pango_font_map_load_font (fontmap, context, mini_desc);
    pango_font_description_free (mini_desc);

    pango_font_description_free (desc);
    g_object_unref (context);
  }

  cairo_font_options_destroy (font_options);


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
  pad = (font_extents.ascent + font_extents.descent) / 43;
  pad = MIN (pad, mini_size);

  hbi = g_slice_new (PangoCairoHexBoxInfo);
  hbi->font = mini_font;
  hbi->rows = rows;

  hbi->digit_width  = width;
  hbi->digit_height = height;

  hbi->pad_x = pad;
  hbi->pad_y = pad;

  if (is_hinted)
    {
      hbi->digit_width  = HINT_X (hbi->digit_width);
      hbi->digit_height = HINT_Y (hbi->digit_height);
      hbi->pad_x = HINT_X (hbi->pad_x);
      hbi->pad_y = HINT_Y (hbi->pad_y);
    }

  hbi->line_width = MIN (hbi->pad_x, hbi->pad_y);

  hbi->box_height = 3 * hbi->pad_y + rows * (hbi->pad_y + hbi->digit_height);

  if (rows == 1)
    {
      hbi->box_descent = hbi->pad_y;
    }
  else
    {
      hbi->box_descent = font_extents.descent * hbi->box_height /
			 (font_extents.ascent + font_extents.descent);
    }
  if (is_hinted)
    {
       hbi->box_descent = HINT_Y (hbi->box_descent);
    }

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

  hbi = _pango_cairo_font_get_hex_box_info (cfont);
  if (!hbi)
    {
      pango_font_get_glyph_extents (NULL, glyph, ink_rect, logical_rect);
      return;
    }

  rows = hbi->rows;
  cols = ((glyph & ~PANGO_GLYPH_UNKNOWN_FLAG) > 0xffff ? 6 : 4) / rows;
  
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

