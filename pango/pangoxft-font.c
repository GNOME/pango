/* Pango
 * pangoxft-font.c: Routines for handling X fonts
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

#include "pangoxft-private.h"
#include "X11/Xft/XftFreetype.h"
#include "pango-modules.h"

#define PANGO_XFT_FONT(object)           (G_TYPE_CHECK_INSTANCE_CAST ((object), PANGO_TYPE_XFT_FONT, PangoXftFont))
#define PANGO_XFT_FONT_CLASS(klass)      (G_TYPE_CHECK_CLASS_CAST ((klass), PANGO_TYPE_XFT_FONT, PangoXftFontClass))
#define PANGO_XFT_IS_FONT_CLASS(klass)   (G_TYPE_CHECK_CLASS_TYPE ((klass), PANGO_TYPE_XFT_FONT))
#define PANGO_XFT_FONT_GET_CLASS(obj)    (G_TYPE_INSTANCE_GET_CLASS ((obj), PANGO_TYPE_XFT_FONT, PangoXftFontClass))

#define PANGO_XFT_UNKNOWN_FLAG 0x10000000

typedef struct _PangoXftFontClass   PangoXftFontClass;

struct _PangoXftFontClass
{
  PangoFontClass parent_class;
};

static PangoFontClass *parent_class;	/* Parent class structure for PangoXftFont */

static void pango_xft_font_class_init (PangoXftFontClass *class);
static void pango_xft_font_init       (PangoXftFont      *xfont);
static void pango_xft_font_shutdown   (GObject         *object);
static void pango_xft_font_finalize   (GObject         *object);

static PangoFontDescription *pango_xft_font_describe          (PangoFont        *font);
static PangoCoverage *       pango_xft_font_get_coverage      (PangoFont        *font,
							       const char       *lang);
static PangoEngineShape *    pango_xft_font_find_shaper       (PangoFont        *font,
							       const char       *lang,
							       guint32           ch);
static void                  pango_xft_font_get_glyph_extents (PangoFont        *font,
							       PangoGlyph        glyph,
							       PangoRectangle   *ink_rect,
							       PangoRectangle   *logical_rect);
static void                  pango_xft_font_get_metrics       (PangoFont        *font,
							       const gchar      *lang,
							       PangoFontMetrics *metrics);


GType
pango_xft_font_get_type (void)
{
  static GType object_type = 0;

  if (!object_type)
    {
      static const GTypeInfo object_info =
      {
        sizeof (PangoXftFontClass),
        (GBaseInitFunc) NULL,
        (GBaseFinalizeFunc) NULL,
        (GClassInitFunc) pango_xft_font_class_init,
        NULL,           /* class_finalize */
        NULL,           /* class_data */
        sizeof (PangoXftFont),
        0,              /* n_preallocs */
        (GInstanceInitFunc) pango_xft_font_init,
      };
      
      object_type = g_type_register_static (PANGO_TYPE_FONT,
                                            "PangoXftFont",
                                            &object_info, 0);
    }
  
  return object_type;
}

static void 
pango_xft_font_init (PangoXftFont *xfont)
{
  xfont->in_cache = FALSE;
}

static void
pango_xft_font_class_init (PangoXftFontClass *class)
{
  GObjectClass *object_class = G_OBJECT_CLASS (class);
  PangoFontClass *font_class = PANGO_FONT_CLASS (class);

  parent_class = g_type_class_peek_parent (class);
  
  object_class->finalize = pango_xft_font_finalize;
  object_class->shutdown = pango_xft_font_shutdown;
  
  font_class->describe = pango_xft_font_describe;
  font_class->get_coverage = pango_xft_font_get_coverage;
  font_class->find_shaper = pango_xft_font_find_shaper;
  font_class->get_glyph_extents = pango_xft_font_get_glyph_extents;
  font_class->get_metrics = pango_xft_font_get_metrics;
}

PangoXftFont *
_pango_xft_font_new (PangoFontMap               *fontmap,
		     const PangoFontDescription *description,
		     XftFont                    *xft_font)
{
  PangoXftFont *xfont;

  g_return_val_if_fail (fontmap != NULL, NULL);
  g_return_val_if_fail (description != NULL, NULL);
  g_return_val_if_fail (xft_font != NULL, NULL);

  xfont = (PangoXftFont *)g_object_new (PANGO_TYPE_XFT_FONT, NULL);
  
  xfont->fontmap = fontmap;
  
  g_object_ref (G_OBJECT (fontmap));
  xfont->description = pango_font_description_copy (description);
  xfont->xft_font = xft_font;

  _pango_xft_font_map_add (xfont->fontmap, xfont);

  return xfont;
}

static PangoFont *
get_mini_font (PangoFont *font)
{
  PangoXftFont *xfont = (PangoXftFont *)font;

  if (!xfont->mini_font)
    {
      Display *display;
      PangoFontDescription desc;
      int i;
      int width = 0, height = 0;
      XGlyphInfo extents;
      XftFont *mini_xft;
      FT_Face face;
      
      _pango_xft_font_map_get_info (xfont->fontmap, &display, NULL);
      
      desc.family_name = "monospace";
      desc.style = PANGO_STYLE_NORMAL;
      desc.variant = PANGO_VARIANT_NORMAL;
      desc.weight = PANGO_WEIGHT_NORMAL;
      
      desc.size = 0.5 * xfont->description->size;
      
      xfont->mini_font = pango_font_map_load_font (xfont->fontmap, &desc);
      mini_xft = ((PangoXftFont *)xfont->mini_font)->xft_font;
      face = pango_xft_font_get_face (xfont->mini_font);
      
      for (i = 0 ; i < 16 ; i++)
	{
	  char c = i < 10 ? '0' + i : 'A' + i - 10;
	  XftChar32 glyph = FT_Get_Char_Index (face, c);
	  
	  XftTextExtents32 (display, mini_xft, &glyph, 1, &extents);

	  width = MAX (width, extents.width);
	  height = MAX (height, extents.height);
	}
      
      xfont->mini_width = width;
      xfont->mini_height = height;
      xfont->mini_pad = MAX (height / 10, 1);
    }

  return xfont->mini_font;
}

static void
draw_box (XftDraw      *draw,
	  XftColor     *color,
	  PangoXftFont *xfont,
	  gint          x,
	  gint          y,
	  gint          width,
	  gint          height)
{
  XftDrawRect (draw, color,
	       x, y, width, xfont->mini_pad);
  XftDrawRect (draw, color,
	       x, y + xfont->mini_pad, xfont->mini_pad, height - xfont->mini_pad * 2);
  XftDrawRect (draw, color,
	       x + width - xfont->mini_pad, y + xfont->mini_pad, xfont->mini_pad, height - xfont->mini_pad * 2);
  XftDrawRect (draw, color,
	       x, y + height - xfont->mini_pad, width, xfont->mini_pad);
}

void
pango_xft_render (XftDraw          *draw,
		  XftColor         *color,
		  PangoFont        *font,
		  PangoGlyphString *glyphs,
		  gint              x,
		  gint              y)
{
  PangoXftFont *xfont = PANGO_XFT_FONT (font);
  Display *display;
  int i;
  int x_off = 0;

  g_return_if_fail (draw != NULL);
  g_return_if_fail (color != NULL);
  g_return_if_fail (font != NULL);
  g_return_if_fail (glyphs != NULL);
  
  /* Slow initial implementation. For speed, it should really
   * collect the characters into runs, and draw multiple
   * characters with each XftDrawString32 call.
   */
  
  _pango_xft_font_map_get_info (xfont->fontmap, &display, NULL);
  
  for (i=0; i<glyphs->num_glyphs; i++)
    {
      PangoGlyph glyph = glyphs->glyphs[i].glyph;

      if (glyph)
	{
	  if (glyph & PANGO_XFT_UNKNOWN_FLAG)
	    {
	      char buf[5];
	      int ys[3];
	      int xs[3];
	      int j, k;
	      
	      PangoFont *mini_font = get_mini_font (font);
	      XftFont *mini_xft = ((PangoXftFont *)mini_font)->xft_font;
	      FT_Face face = pango_xft_font_get_face (xfont->mini_font);
      
	      glyph &= ~PANGO_XFT_UNKNOWN_FLAG;

	      ys[0] = y + PANGO_PIXELS (glyphs->glyphs[i].geometry.y_offset) - xfont->xft_font->ascent + (xfont->xft_font->ascent + xfont->xft_font->descent - xfont->mini_height * 2 - xfont->mini_pad * 5) / 2;
	      ys[1] = ys[0] + 2 * xfont->mini_pad + xfont->mini_height;
	      ys[2] = ys[1] + xfont->mini_height + xfont->mini_pad;

	      xs[0] = x + PANGO_PIXELS (x_off + glyphs->glyphs[i].geometry.x_offset); 
	      xs[1] = xs[0] + 2 * xfont->mini_pad;
	      xs[2] = xs[1] + xfont->mini_width + xfont->mini_pad;
	      
	      draw_box (draw, color, xfont,
			xs[0], ys[0],
			xfont->mini_width * 2 + xfont->mini_pad * 5,
			xfont->mini_height * 2 + xfont->mini_pad * 5);
	      
	      g_snprintf (buf, sizeof(buf), "%04X", glyph);

	      for (j = 0; j < 2; j++)
		for (k = 0; k < 2; k++)
		  {
		    XftChar32 glyph = FT_Get_Char_Index (face, buf[2*j + k]);
		    XftDrawString32 (draw, color, mini_xft,
				     xs[k+1], ys[j+1],
				     &glyph, 1);
		  }
	    }
	  else
	    XftDrawString32 (draw, color, xfont->xft_font,
			     x + PANGO_PIXELS (x_off + glyphs->glyphs[i].geometry.x_offset),
			     y + PANGO_PIXELS (glyphs->glyphs[i].geometry.y_offset),
			     &glyph, 1);
	}
      
      x_off += glyphs->glyphs[i].geometry.width;
    }
}

static void
pango_xft_font_get_metrics (PangoFont        *font,
			    const gchar      *lang,
			    PangoFontMetrics *metrics)
{
  PangoXftFont *xfont = (PangoXftFont *)font;

  metrics->ascent = PANGO_SCALE * xfont->xft_font->ascent;
  metrics->descent = PANGO_SCALE * xfont->xft_font->descent;
  
  return;
}

static void
pango_xft_font_shutdown (GObject *object)
{
  PangoXftFont *xfont = PANGO_XFT_FONT (object);

  /* If the font is not already in the freed-fonts cache, add it,
   * if it is already there, do nothing and the font will be
   * freed.
   */
  if (!xfont->in_cache && xfont->fontmap)
    _pango_xft_font_map_cache_add (xfont->fontmap, xfont);

  G_OBJECT_CLASS (parent_class)->shutdown (object);
}

static void
pango_xft_font_finalize (GObject *object)
{
  PangoXftFont *xfont = (PangoXftFont *)object;

  _pango_xft_font_map_remove (xfont->fontmap, xfont);

  if (xfont->mini_font)
    g_object_unref (xfont->mini_font);

  if (xfont->ot_info)
    g_object_unref (xfont->ot_info);

  G_OBJECT_CLASS (parent_class)->finalize (object);
}

static PangoFontDescription *
pango_xft_font_describe (PangoFont *font)
{
  PangoXftFont *xfont = (PangoXftFont *)font;

  return pango_font_description_copy (xfont->description);
}

static PangoCoverage *
pango_xft_font_get_coverage (PangoFont  *font,
			     const char *lang)
{
  PangoXftFont *xfont = (PangoXftFont *)font;
  FT_Face face;
  PangoCoverage *coverage;
  Display *display;
  int i;
  
  _pango_xft_font_map_get_info (xfont->fontmap, &display, NULL);
  
  coverage = _pango_xft_font_map_get_coverage (xfont->fontmap,
					       xfont->description->family_name);

  if (coverage)
    return pango_coverage_ref (coverage);

  /* Ugh, this is going to be SLOW */

  face = pango_xft_font_get_face (font);

  coverage = pango_coverage_new ();
  for (i = 0; i < G_MAXUSHORT; i++)
    {
      FT_UInt glyph = FT_Get_Char_Index (face, i);

      if (glyph && glyph < face->num_glyphs)
	pango_coverage_set (coverage, i, PANGO_COVERAGE_EXACT);
    }

  _pango_xft_font_map_set_coverage (xfont->fontmap, xfont->description->family_name, coverage);
      
  return coverage;
}

static void
pango_xft_font_get_glyph_extents (PangoFont        *font,
				  PangoGlyph        glyph,
				  PangoRectangle   *ink_rect,
				  PangoRectangle   *logical_rect)
{
  PangoXftFont *xfont = (PangoXftFont *)font;
  XGlyphInfo extents;
  Display *display;

  _pango_xft_font_map_get_info (xfont->fontmap, &display, NULL);
  
  if (glyph == (PangoGlyph)-1)
    glyph = 0;

  if (glyph & PANGO_XFT_UNKNOWN_FLAG)
    {
      get_mini_font (font);
      
      if (ink_rect)
	{
	  ink_rect->x = 0;
	  ink_rect->y = PANGO_SCALE * (- xfont->xft_font->ascent + (xfont->xft_font->ascent + xfont->xft_font->descent - xfont->mini_height * 2 - xfont->mini_pad * 5) / 2);
	  ink_rect->width = PANGO_SCALE * (xfont->mini_width * 2 + xfont->mini_pad * 5);
	  ink_rect->height = PANGO_SCALE * (xfont->mini_height * 2 + xfont->mini_pad * 5);
	}
      
      if (logical_rect)
	{
	  logical_rect->x = 0;
	  logical_rect->y = - PANGO_SCALE * xfont->xft_font->ascent;
	  logical_rect->width = PANGO_SCALE * (xfont->mini_width * 2 + xfont->mini_pad * 6);
	  logical_rect->height = (xfont->xft_font->ascent + xfont->xft_font->descent) * PANGO_SCALE;
	}
    }
  else
    {
      XftTextExtents32 (display, xfont->xft_font, &glyph, 1, &extents);

      if (ink_rect)
	{
	  ink_rect->x = extents.x * PANGO_SCALE;
	  ink_rect->y = extents.y * PANGO_SCALE;
	  ink_rect->width = extents.width * PANGO_SCALE;
	  ink_rect->height = extents.height * PANGO_SCALE;
	}
      
      if (logical_rect)
	{
	  logical_rect->x = 0;
	  logical_rect->y = - xfont->xft_font->ascent * PANGO_SCALE;
	  logical_rect->width = extents.xOff * PANGO_SCALE;
	  logical_rect->height = (xfont->xft_font->ascent + xfont->xft_font->descent) * PANGO_SCALE;
	}
    }
}

static PangoMap *
pango_xft_get_shaper_map (const char *lang)
{
  static guint engine_type_id = 0;
  static guint render_type_id = 0;
  
  if (engine_type_id == 0)
    {
      engine_type_id = g_quark_from_static_string (PANGO_ENGINE_TYPE_SHAPE);
      render_type_id = g_quark_from_static_string (PANGO_RENDER_TYPE_XFT);
    }
  
  return pango_find_map (lang, engine_type_id, render_type_id);
}

static PangoEngineShape *
pango_xft_font_find_shaper (PangoFont   *font,
			  const gchar *lang,
			  guint32      ch)
{
  PangoMap *shape_map = NULL;

  shape_map = pango_xft_get_shaper_map (lang);
  return (PangoEngineShape *)pango_map_get_engine (shape_map, ch);
}

XftFont *
pango_xft_font_get_font (PangoFont *font)
{
  PangoXftFont *xfont;

  g_return_val_if_fail (PANGO_XFT_IS_FONT (font), NULL);
  
  xfont = PANGO_XFT_FONT (font);

  return xfont->xft_font;
}

Display *
pango_xft_font_get_display (PangoFont *font)
{
  PangoXftFont *xfont;
  Display *display;

  g_return_val_if_fail (PANGO_XFT_IS_FONT (font), NULL);

  xfont = PANGO_XFT_FONT (font);
  _pango_xft_font_map_get_info (xfont->fontmap, &display, NULL);

  return display;
}

PangoGlyph
pango_xft_font_get_unknown_glyph (PangoFont *font,
				  gunichar   wc)
{
  g_return_val_if_fail (PANGO_XFT_IS_FONT (font), -1);

  return wc | PANGO_XFT_UNKNOWN_FLAG;
}

FT_Face
pango_xft_font_get_face (PangoFont *font)
{
  PangoXftFont *xfont;

  g_return_val_if_fail (PANGO_XFT_IS_FONT (font), NULL);

  xfont = PANGO_XFT_FONT (font);
  
  if (xfont->xft_font->core)
    return NULL;
  else
    return xfont->xft_font->u.ft.font->face;
}

PangoOTInfo *
pango_xft_font_get_ot_info (PangoFont *font)
{
  PangoXftFont *xfont;

  g_return_val_if_fail (PANGO_XFT_IS_FONT (font), NULL);

  xfont = PANGO_XFT_FONT (font);

  if (!xfont->ot_info)
    {
      FT_Face face = pango_xft_font_get_face (font);
      
      if (!face)
	return NULL;

      xfont->ot_info = pango_ot_info_new (face);
    }

  return xfont->ot_info;
}
