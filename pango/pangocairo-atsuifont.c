/* Pango
 * pangocairo-atsuifont.c
 *
 * Copyright (C) 2000-2005 Red Hat Software
 * Copyright (C) 2005 Imendio AB
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

#include "pangoatsui-private.h"
#include "pangocairo.h"
#include "pangocairo-private.h"
#include "pangocairo-atsui.h"
#include "pangocairo-atsuifont.h"

struct _PangoCairoATSUIFont
{
  PangoATSUIFont font;

  ATSUFontID font_id;
  cairo_font_face_t *font_face;
  cairo_scaled_font_t *scaled_font;
  cairo_matrix_t font_matrix;
  cairo_matrix_t ctm;
  cairo_font_options_t *options;

  double size;
};

struct _PangoCairoATSUIFontClass
{
  PangoATSUIFontClass parent_class;
};



static cairo_font_face_t *pango_cairo_atsui_font_get_font_face (PangoCairoFont *font);

/**
 * pango_cairo_atsui_font_get_atsu_font_id:
 * @cafont: A #PangoCairoATSUIFont
 *
 * Returns the ATSUFontID of a font.
 *
 * Return value: the ATSUFontID associated to @cafont.
 *
 * Since: 1.12
 */
ATSUFontID
pango_cairo_atsui_font_get_atsu_font_id (PangoCairoATSUIFont *cafont)
{
  return cafont->font_id;
}

static gboolean
pango_cairo_atsui_font_install (PangoCairoFont *font,
				cairo_t        *cr)
{
  PangoCairoATSUIFont *cafont = PANGO_CAIRO_ATSUI_FONT (font);

  cairo_set_font_face (cr,
		       pango_cairo_atsui_font_get_font_face (font));

  cairo_set_font_matrix (cr, &cafont->font_matrix);
  cairo_set_font_options (cr, cafont->options);

  return TRUE;
}

static cairo_font_face_t *
pango_cairo_atsui_font_get_font_face (PangoCairoFont *font)
{
  PangoCairoATSUIFont *cafont = PANGO_CAIRO_ATSUI_FONT (font);

  if (!cafont->font_face)
    {
      cafont->font_face = cairo_atsui_font_face_create_for_atsu_font_id (cafont->font_id);

      /* Failure of the above should only occur for out of memory,
       * we can't proceed at that point
       */
      if (!cafont->font_face)
	g_error ("Unable to create ATSUI cairo font face.\nThis means out of memory or a cairo/fontconfig/FreeType bug");
    }

  return cafont->font_face;
}

static cairo_scaled_font_t *
pango_cairo_atsui_font_get_scaled_font (PangoCairoFont *font)
{
  PangoCairoATSUIFont *cafont = PANGO_CAIRO_ATSUI_FONT (font);

  if (!cafont->scaled_font)
    {
      cairo_font_face_t *font_face;

      font_face = pango_cairo_atsui_font_get_font_face (font);
      cafont->scaled_font = cairo_scaled_font_create (font_face,
						      &cafont->font_matrix,
						      &cafont->ctm,
						      cafont->options);

      /* Failure of the above should only occur for out of memory,
       * we can't proceed at that point
       */
      if (!cafont->scaled_font)
	g_error ("Unable to create ATSUI cairo scaled font.\nThis means out of memory or a cairo/fontconfig/FreeType bug");
    }
  
  return cafont->scaled_font;
}

static void
cairo_font_iface_init (PangoCairoFontIface *iface)
{
  iface->install = pango_cairo_atsui_font_install;
  iface->get_font_face = pango_cairo_atsui_font_get_font_face;
  iface->get_scaled_font = pango_cairo_atsui_font_get_scaled_font;
}

G_DEFINE_TYPE_WITH_CODE (PangoCairoATSUIFont, pango_cairo_atsui_font, PANGO_TYPE_ATSUI_FONT,
    { G_IMPLEMENT_INTERFACE (PANGO_TYPE_CAIRO_FONT, cairo_font_iface_init) });

static void
pango_cairo_atsui_font_get_glyph_extents (PangoFont        *font,
					  PangoGlyph        glyph,
					  PangoRectangle   *ink_rect,
					  PangoRectangle   *logical_rect)
{
  PangoCairoFont *cfont = (PangoCairoFont *)font;
  cairo_scaled_font_t *scaled_font;
  cairo_font_extents_t font_extents;
  cairo_text_extents_t extents;
  cairo_glyph_t cairo_glyph;

  scaled_font = pango_cairo_atsui_font_get_scaled_font (PANGO_CAIRO_FONT (font));

  if (logical_rect)
    cairo_scaled_font_extents (scaled_font, &font_extents);
	  
  cairo_glyph.index = glyph;
  cairo_glyph.x = 0;
  cairo_glyph.y = 0;

  if (glyph == PANGO_GLYPH_EMPTY)
    {
      if (ink_rect)
	{
	  ink_rect->x = ink_rect->y = ink_rect->width = ink_rect->height = 0;
	}
      if (logical_rect)
        {
	  logical_rect->x = 0;
	  logical_rect->y = - font_extents.ascent * PANGO_SCALE;
	  logical_rect->width = 0;
	  logical_rect->height = (font_extents.ascent + font_extents.descent) * PANGO_SCALE;
	}
    }
  else if (glyph & PANGO_GLYPH_UNKNOWN_FLAG)
    {
      /* space for the hex box */
      _pango_cairo_get_glyph_extents_missing(cfont, glyph, ink_rect, logical_rect);
    }
  else
    {
      cairo_scaled_font_glyph_extents (scaled_font,
				       &cairo_glyph, 1, &extents);

      if (ink_rect)
	{
	  ink_rect->x = extents.x_bearing * PANGO_SCALE;
	  ink_rect->y = extents.y_bearing * PANGO_SCALE;
	  ink_rect->width = extents.width * PANGO_SCALE;
	  ink_rect->height = extents.height * PANGO_SCALE;
	}
      
      if (logical_rect)
	{
	  logical_rect->x = 0;
	  logical_rect->y = - font_extents.ascent * PANGO_SCALE;
	  logical_rect->width = extents.x_advance * PANGO_SCALE;
	  logical_rect->height = (font_extents.ascent + font_extents.descent) * PANGO_SCALE;
	}
    }
}


static PangoFontMetrics *
pango_cairo_atsui_font_get_metrics (PangoFont        *font,
				    PangoLanguage    *language)
{
  PangoCairoATSUIFont *cafont = PANGO_CAIRO_ATSUI_FONT (font);
  ATSFontRef ats_font;
  ATSFontMetrics ats_metrics;
  PangoFontMetrics *metrics;

  ats_font = FMGetATSFontRefFromFont(cafont->font_id);

  ATSFontGetHorizontalMetrics (ats_font, kATSOptionFlagsDefault, &ats_metrics);

  metrics = pango_font_metrics_new ();

  metrics->ascent = ats_metrics.ascent * cafont->size * PANGO_SCALE;
  metrics->descent = -ats_metrics.descent * cafont->size * PANGO_SCALE;

  metrics->approximate_char_width = ats_metrics.avgAdvanceWidth * cafont->size * PANGO_SCALE;
  metrics->approximate_digit_width = ats_metrics.avgAdvanceWidth * cafont->size * PANGO_SCALE;
  
  metrics->underline_position = ats_metrics.underlinePosition * cafont->size * PANGO_SCALE;
  metrics->underline_thickness = ats_metrics.underlineThickness * cafont->size * PANGO_SCALE;
  
  metrics->strikethrough_position = metrics->ascent / 3;
  metrics->strikethrough_thickness = ats_metrics.underlineThickness * cafont->size * PANGO_SCALE;

  return metrics;
}

static void
pango_cairo_atsui_font_finalize (GObject *object)
{
  PangoCairoATSUIFont *cafont = PANGO_CAIRO_ATSUI_FONT (object);

  cairo_font_face_destroy (cafont->font_face);
  cairo_scaled_font_destroy (cafont->scaled_font);
  cairo_font_options_destroy (cafont->options);

  G_OBJECT_CLASS (pango_cairo_atsui_font_parent_class)->finalize (object);
}

static void
pango_cairo_atsui_font_class_init (PangoCairoATSUIFontClass *class)
{
  GObjectClass *object_class = G_OBJECT_CLASS (class);
  PangoFontClass *font_class = PANGO_FONT_CLASS (class);

  object_class->finalize = pango_cairo_atsui_font_finalize;

  font_class->get_metrics = pango_cairo_atsui_font_get_metrics;
  font_class->get_glyph_extents = pango_cairo_atsui_font_get_glyph_extents;
}

static void
pango_cairo_atsui_font_init (PangoCairoATSUIFont *cafont)
{
}

PangoATSUIFont *
_pango_cairo_atsui_font_new (PangoCairoATSUIFontMap     *cafontmap,
			     PangoContext               *context,
			     const char                 *postscript_name,
			     const PangoFontDescription *desc)
{
  PangoCairoATSUIFont *cafont;
  PangoATSUIFont *afont;
  CFStringRef cfstr;
  ATSFontRef font_ref;
  const PangoMatrix *pango_ctm;
  ATSUFontID font_id;

  cfstr = CFStringCreateWithCString (NULL, postscript_name, 
				     kCFStringEncodingUTF8);

  font_ref = ATSFontFindFromPostScriptName (cfstr, kATSOptionFlagsDefault);
  font_id = FMGetFontFromATSFontRef (font_ref);
  
  CFRelease (cfstr);

  if (!font_id)
    return NULL;

  cafont = g_object_new (PANGO_TYPE_CAIRO_ATSUI_FONT, NULL);
  afont = PANGO_ATSUI_FONT (cafont);

  afont->desc = pango_font_description_copy (desc);
  afont->postscript_name = g_strdup (postscript_name);

  cafont->size = (double) pango_font_description_get_size (desc) / PANGO_SCALE;
  cafont->font_id = font_id;

  if (!pango_font_description_get_size_is_absolute (desc))
    {
      /* FIXME: Need to handle dpi here. See other font implementations for more info. */
    }  

  cairo_matrix_init_scale (&cafont->font_matrix, cafont->size, cafont->size);
  pango_ctm = pango_context_get_matrix (context);
  if (pango_ctm)
    cairo_matrix_init (&cafont->ctm,
		       pango_ctm->xx,
		       pango_ctm->yx,
		       pango_ctm->xy,
		       pango_ctm->yy,
		       0., 0.);
  else
    cairo_matrix_init_identity (&cafont->ctm);

  cafont->options = cairo_font_options_copy (_pango_cairo_context_get_merged_font_options (context));

  return afont;
}
