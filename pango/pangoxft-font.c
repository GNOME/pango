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

#include "config.h"

#include <stdlib.h>

#include "pangoxft-private.h"
#include "pango-layout.h"
#include "pango-modules.h"
#include "pango-utils.h"

#define PANGO_XFT_FONT(object)           (G_TYPE_CHECK_INSTANCE_CAST ((object), PANGO_TYPE_XFT_FONT, PangoXftFont))
#define PANGO_XFT_FONT_CLASS(klass)      (G_TYPE_CHECK_CLASS_CAST ((klass), PANGO_TYPE_XFT_FONT, PangoXftFontClass))
#define PANGO_XFT_IS_FONT_CLASS(klass)   (G_TYPE_CHECK_CLASS_TYPE ((klass), PANGO_TYPE_XFT_FONT))
#define PANGO_XFT_FONT_GET_CLASS(obj)    (G_TYPE_INSTANCE_GET_CLASS ((obj), PANGO_TYPE_XFT_FONT, PangoXftFontClass))

#define PANGO_XFT_UNKNOWN_FLAG 0x10000000

typedef struct _PangoXftFontClass    PangoXftFontClass;
typedef struct _PangoXftMetricsInfo  PangoXftMetricsInfo;

struct _PangoXftFontClass
{
  PangoFontClass  parent_class;
};

struct _PangoXftMetricsInfo
{
  const char       *sample_str;
  PangoFontMetrics *metrics;
};

static PangoFontClass *parent_class;	/* Parent class structure for PangoXftFont */

static void pango_xft_font_class_init (PangoXftFontClass *class);
static void pango_xft_font_init       (PangoXftFont      *xfont);
static void pango_xft_font_dispose    (GObject         *object);
static void pango_xft_font_finalize   (GObject         *object);

static PangoFontDescription *pango_xft_font_describe          (PangoFont        *font);
static PangoCoverage *       pango_xft_font_get_coverage      (PangoFont        *font,
							       PangoLanguage    *language);
static PangoEngineShape *    pango_xft_font_find_shaper       (PangoFont        *font,
							       PangoLanguage    *language,
							       guint32           ch);
static void                  pango_xft_font_get_glyph_extents (PangoFont        *font,
							       PangoGlyph        glyph,
							       PangoRectangle   *ink_rect,
							       PangoRectangle   *logical_rect);
static PangoFontMetrics *    pango_xft_font_get_metrics       (PangoFont        *font,
							       PangoLanguage    *language);


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
  xfont->metrics_by_lang = NULL;
  xfont->in_cache = FALSE;
}

static void
pango_xft_font_class_init (PangoXftFontClass *class)
{
  GObjectClass *object_class = G_OBJECT_CLASS (class);
  PangoFontClass *font_class = PANGO_FONT_CLASS (class);

  parent_class = g_type_class_peek_parent (class);
  
  object_class->finalize = pango_xft_font_finalize;
  object_class->dispose = pango_xft_font_dispose;
  
  font_class->describe = pango_xft_font_describe;
  font_class->get_coverage = pango_xft_font_get_coverage;
  font_class->find_shaper = pango_xft_font_find_shaper;
  font_class->get_glyph_extents = pango_xft_font_get_glyph_extents;
  font_class->get_metrics = pango_xft_font_get_metrics;
}

PangoXftFont *
_pango_xft_font_new (PangoFontMap               *fontmap,
		     FcPattern			*pattern)
{
  PangoXftFont *xfont;

  g_return_val_if_fail (fontmap != NULL, NULL);
  g_return_val_if_fail (pattern != NULL, NULL);

  xfont = (PangoXftFont *)g_object_new (PANGO_TYPE_XFT_FONT, NULL);
  
  xfont->fontmap = fontmap;
  xfont->font_pattern = pattern;
  
  g_object_ref (G_OBJECT (fontmap));
  xfont->description = _pango_xft_font_desc_from_pattern (pattern, TRUE);
  xfont->xft_font = NULL;

  _pango_xft_font_map_add (xfont->fontmap, xfont);

  return xfont;
}

static PangoFont *
get_mini_font (PangoFont *font)
{
  PangoXftFont *xfont = (PangoXftFont *)font;

  g_assert (xfont->fontmap);

  if (!xfont->mini_font)
    {
      Display *display;
      PangoFontDescription *desc = pango_font_description_new ();
      int i;
      int width = 0, height = 0;
      XGlyphInfo extents;
      XftFont *mini_xft;
      
      _pango_xft_font_map_get_info (xfont->fontmap, &display, NULL);

      pango_font_description_set_family_static (desc, "monospace");
      pango_font_description_set_size (desc,
				       0.5 * pango_font_description_get_size (xfont->description));
      
      xfont->mini_font = pango_font_map_load_font (xfont->fontmap, NULL, desc);
      pango_font_description_free (desc);
      
      mini_xft = pango_xft_font_get_font (xfont->mini_font);
      
      for (i = 0 ; i < 16 ; i++)
	{
	  char c = i < 10 ? '0' + i : 'A' + i - 10;
	  XftTextExtents8 (display, mini_xft, (FcChar8 *) &c, 1, &extents);
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
draw_rectangle (Display      *display,
		Picture       src_picture,
		Picture       dest_picture,
		XftDraw      *draw,
		XftColor     *color,
		gint          x,
		gint          y,
		gint          width,
		gint          height)
{
  if (draw)
    {
      XftDrawRect (draw, color, x, y, width, height);
    }
  else
    {
      XRenderComposite (display, PictOpOver, src_picture, None, dest_picture,
			0, 0, 0, 0, x, y, width, height);
    }
}

static void
draw_box (Display      *display,
	  Picture       src_picture,
	  Picture       dest_picture,
	  XftDraw      *draw,
	  XftColor     *color,
	  PangoXftFont *xfont,
	  gint          x,
	  gint          y,
	  gint          width,
	  gint          height)
{
  draw_rectangle (display, src_picture, dest_picture, draw, color,
		  x, y, width, xfont->mini_pad);
  draw_rectangle (display, src_picture, dest_picture, draw, color,
		  x, y + xfont->mini_pad, xfont->mini_pad, height - xfont->mini_pad * 2);
  draw_rectangle (display, src_picture, dest_picture, draw, color,
		  x + width - xfont->mini_pad, y + xfont->mini_pad, xfont->mini_pad, height - xfont->mini_pad * 2);
  draw_rectangle (display, src_picture, dest_picture, draw, color,
		  x, y + height - xfont->mini_pad, width, xfont->mini_pad);
}

/**
 * pango_xft_render:
 * @draw:    the <type>XftDraw</type> object.
 * @color:   the color in which to draw the string
 * @font:    the font in which to draw the string
 * @glyphs:  the glyph string to draw
 * @x:       the x position of start of string (in pixels)
 * @y:       the y position of baseline (in pixels)
 *
 * Renders a #PangoGlyphString onto an <type>XftDraw</type> object wrapping an X drawable.
 */
static void
pango_xft_real_render (Display          *display,
		       Picture           src_picture,
		       Picture           dest_picture,
		       XftDraw          *draw,
		       XftColor         *color,
		       PangoFont        *font,
		       PangoGlyphString *glyphs,
		       gint              x,
		       gint              y)
{
  PangoXftFont *xfont = PANGO_XFT_FONT (font);
  XftFont *xft_font = pango_xft_font_get_font (font);
  int i;
  int x_off = 0;
#define N_XFT_LOCAL	1024
  XftGlyphSpec  xft_glyphs[N_XFT_LOCAL];
  XftCharSpec	chars[4];     /* for unknown */
  int		n_xft_glyph = 0;

  if (!xfont->fontmap)		/* Display closed */
    return;

#define FLUSH_GLYPHS() G_STMT_START {						\
  if (n_xft_glyph)								\
    {										\
       if (draw)								\
         XftDrawGlyphSpec (draw, color, xft_font, xft_glyphs, n_xft_glyph);	\
       else									\
         XftGlyphSpecRender (display, PictOpOver, src_picture, xft_font,	\
	  		     dest_picture, 0, 0, xft_glyphs, n_xft_glyph);	\
       n_xft_glyph = 0;								\
    }										\
  } G_STMT_END

  /* Slow initial implementation. For speed, it should really
   * collect the characters into runs, and draw multiple
   * characters with each XftDrawString32 call.
   */
  if (!display)
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
	      XftFont *mini_xft = pango_xft_font_get_font (mini_font);
      
	      glyph &= ~PANGO_XFT_UNKNOWN_FLAG;
	      
	      ys[0] = y + PANGO_PIXELS (glyphs->glyphs[i].geometry.y_offset) - xft_font->ascent + (xft_font->ascent + xft_font->descent - xfont->mini_height * 2 - xfont->mini_pad * 5) / 2;
	      ys[1] = ys[0] + 2 * xfont->mini_pad + xfont->mini_height;
	      ys[2] = ys[1] + xfont->mini_height + xfont->mini_pad;

	      xs[0] = x + PANGO_PIXELS (x_off + glyphs->glyphs[i].geometry.x_offset); 
	      xs[1] = xs[0] + 2 * xfont->mini_pad;
	      xs[2] = xs[1] + xfont->mini_width + xfont->mini_pad;
	      
	      draw_box (display, src_picture, dest_picture, draw, color, xfont,
			xs[0], ys[0],
			xfont->mini_width * 2 + xfont->mini_pad * 5,
			xfont->mini_height * 2 + xfont->mini_pad * 5);
	      
	      g_snprintf (buf, sizeof(buf), "%04X", glyph);

	      FLUSH_GLYPHS ();
	      for (j = 0; j < 2; j++)
		for (k = 0; k < 2; k++)
		  {
		    XftCharSpec *c = &chars[j * 2 + k];
		    c->ucs4 = buf[j * 2 + k] & 0xff;
		    c->x = xs[k+1];
		    c->y = ys[j+1];
		  }
	      if (draw)
		XftDrawCharSpec (draw, color, mini_xft,
				 chars, 4);
	      else
		XftCharSpecRender (display, PictOpOver, src_picture,
				   mini_xft, dest_picture, 0, 0,
				   chars, 4);
	    }
	  else if (glyph)
	    {
	      if (n_xft_glyph == N_XFT_LOCAL)
		FLUSH_GLYPHS ();
	      
	      xft_glyphs[n_xft_glyph].x = x + PANGO_PIXELS (x_off + glyphs->glyphs[i].geometry.x_offset);
	      xft_glyphs[n_xft_glyph].y = y + PANGO_PIXELS (glyphs->glyphs[i].geometry.y_offset);
	      xft_glyphs[n_xft_glyph].glyph = glyph;
	      n_xft_glyph++;
	    }
	}
      
      x_off += glyphs->glyphs[i].geometry.width;
    }
  
  FLUSH_GLYPHS ();

#undef FLUSH_GLYPHS
}

/**
 * pango_xft_render:
 * @draw:    the <type>XftDraw</type> object.
 * @color:   the color in which to draw the string
 * @font:    the font in which to draw the string
 * @glyphs:  the glyph string to draw
 * @x:       the x position of start of string (in pixels)
 * @y:       the y position of baseline (in pixels)
 *
 * Renders a #PangoGlyphString onto an <type>XftDraw</type> object wrapping an X drawable.
 */
void
pango_xft_render (XftDraw          *draw,
		  XftColor         *color,
		  PangoFont        *font,
		  PangoGlyphString *glyphs,
		  gint              x,
		  gint              y)
{
  g_return_if_fail (draw != NULL);
  g_return_if_fail (color != NULL);
  g_return_if_fail (PANGO_XFT_IS_FONT (font));
  g_return_if_fail (glyphs != NULL);
  
  pango_xft_real_render (NULL, None, None, draw, color, font, glyphs, x, y);
}

/**
 * pango_xft_picture_render:
 * @display:      an X display
 * @src_picture:  the source picture to draw the string with
 * @dest_picture: the destination picture to draw the strign onto
 * @font:         the font in which to draw the string
 * @glyphs:       the glyph string to draw
 * @x:            the x position of start of string (in pixels)
 * @y:            the y position of baseline (in pixels)
 *
 * Renders a #PangoGlyphString onto an Xrender <type>Picture</type> object.
 */
void
pango_xft_picture_render (Display          *display,
			  Picture           src_picture,
			  Picture           dest_picture,
			  PangoFont        *font,
			  PangoGlyphString *glyphs,
			  gint              x,
			  gint              y)
{
  g_return_if_fail (display != NULL);
  g_return_if_fail (src_picture != None);
  g_return_if_fail (dest_picture != None);
  g_return_if_fail (PANGO_XFT_IS_FONT (font));
  g_return_if_fail (glyphs != NULL);
  
  pango_xft_real_render (display, src_picture, dest_picture, NULL, NULL, font, glyphs, x, y);
}

static PangoFontMetrics *
pango_xft_font_get_metrics (PangoFont     *font,
			    PangoLanguage *language)
{
  PangoXftFont *xfont = PANGO_XFT_FONT (font);
  PangoXftMetricsInfo *info = NULL; /* Quiet gcc */
  GSList *tmp_list;      

  const char *sample_str = pango_language_get_sample_string (language);
  
  tmp_list = xfont->metrics_by_lang;
  while (tmp_list)
    {
      info = tmp_list->data;
      
      if (info->sample_str == sample_str)    /* We _don't_ need strcmp */
	break;

      tmp_list = tmp_list->next;
    }

  if (!tmp_list)
    {
      PangoLayout *layout;
      PangoRectangle extents;
      PangoContext *context;
      XftFont *xft_font;
      Display *display;

      info = g_new0 (PangoXftMetricsInfo, 1);
      xfont->metrics_by_lang = g_slist_prepend (xfont->metrics_by_lang, 
                                                info);

	  
      if (xfont->fontmap)
	{
	  xft_font = pango_xft_font_get_font (font);
      
	  _pango_xft_font_map_get_info (xfont->fontmap, &display, NULL);
	  context = pango_xft_get_context (display, 0);

	  info->sample_str = sample_str;
	  info->metrics = pango_font_metrics_new ();

	  info->metrics->ascent = PANGO_SCALE * xft_font->ascent;
	  info->metrics->descent = PANGO_SCALE * xft_font->descent;
	  info->metrics->approximate_char_width = 
	    info->metrics->approximate_digit_width = 
	    PANGO_SCALE * xft_font->max_advance_width;

	  pango_context_set_language (context, language);
	  layout = pango_layout_new (context);
	  pango_layout_set_font_description (layout, xfont->description);

	  pango_layout_set_text (layout, sample_str, -1);      
	  pango_layout_get_extents (layout, NULL, &extents);
      
	  info->metrics->approximate_char_width = 
	    extents.width / g_utf8_strlen (sample_str, -1);

	  pango_layout_set_text (layout, "0123456789", -1);      
	  pango_layout_get_extents (layout, NULL, &extents);
      
	  info->metrics->approximate_digit_width = extents.width / 10;

	  g_object_unref (G_OBJECT (layout));
	  g_object_unref (G_OBJECT (context));
	}
    }

  return pango_font_metrics_ref (info->metrics);
}

static void
pango_xft_font_dispose (GObject *object)
{
  PangoXftFont *xfont = PANGO_XFT_FONT (object);

  /* If the font is not already in the freed-fonts cache, add it,
   * if it is already there, do nothing and the font will be
   * freed.
   */
  if (!xfont->in_cache && xfont->fontmap)
    _pango_xft_font_map_cache_add (xfont->fontmap, xfont);

  G_OBJECT_CLASS (parent_class)->dispose (object);
}

static void
free_metrics_info (PangoXftMetricsInfo *info)
{
  pango_font_metrics_unref (info->metrics);
  g_free (info);
}

static void
pango_xft_font_finalize (GObject *object)
{
  PangoXftFont *xfont = (PangoXftFont *)object;

  if (xfont->fontmap)
    _pango_xft_font_map_remove (xfont->fontmap, xfont);

  if (xfont->mini_font)
    g_object_unref (xfont->mini_font);

  pango_font_description_free (xfont->description);

  g_slist_foreach (xfont->metrics_by_lang, (GFunc)free_metrics_info, NULL);
  g_slist_free (xfont->metrics_by_lang);  

  if (xfont->xft_font)
    {
      Display *display;
      
      _pango_xft_font_map_get_info (xfont->fontmap, &display, NULL);
      XftFontClose (display, xfont->xft_font);
    }

  FcPatternDestroy (xfont->font_pattern);

  G_OBJECT_CLASS (parent_class)->finalize (object);
}

static PangoFontDescription *
pango_xft_font_describe (PangoFont *font)
{
  PangoXftFont *xfont = (PangoXftFont *)font;

  return pango_font_description_copy (xfont->description);
}

static PangoCoverage *
pango_xft_font_get_coverage (PangoFont     *font,
			     PangoLanguage *language)
{
  PangoXftFont *xfont = (PangoXftFont *)font;
  PangoXftCoverageKey key;
  PangoCoverage *coverage;
  Display *display;
  FcChar32  map[FC_CHARSET_MAP_SIZE];
  FcChar32  ucs4, pos;
  FcCharSet *charset;
  int i;
  
  _pango_xft_font_map_get_info (xfont->fontmap, &display, NULL);

  /*
   * Assume that coverage information is identified by
   * a filename/index pair; there shouldn't be any reason
   * this isn't true, but it's not specified anywhere
   */
  if (FcPatternGetString (xfont->font_pattern, FC_FILE, 0, (FcChar8 **) &key.filename) != FcResultMatch)
    return NULL;

  if (FcPatternGetInteger (xfont->font_pattern, FC_INDEX, 0, &key.id) != FcResultMatch)
    return NULL;
  
  coverage = _pango_xft_font_map_get_coverage (xfont->fontmap, &key);

  if (coverage)
    return pango_coverage_ref (coverage);

  /*
   * Pull the coverage out of the pattern, this
   * doesn't require loading the font
   */
  if (FcPatternGetCharSet (xfont->font_pattern, FC_CHARSET, 0, &charset) != FcResultMatch)
    return NULL;
  
  /*
   * Convert an Fc CharSet into a pango coverage structure.  Sure
   * would be nice to just use the Fc structure in place...
   */
  coverage = pango_coverage_new ();
  for (ucs4 = FcCharSetFirstPage (charset, map, &pos);
       ucs4 != FC_CHARSET_DONE;
       ucs4 = FcCharSetNextPage (charset, map, &pos))
    {
      for (i = 0; i < FC_CHARSET_MAP_SIZE; i++)
	{
	  FcChar32  bits = map[i];
	  FcChar32  base = ucs4 + i * 32;
	  int b = 0;
	  bits = map[i];
	  while (bits)
	    {
	      if (bits & 1)
		pango_coverage_set (coverage, base + b, PANGO_COVERAGE_EXACT);

	      bits >>= 1;
	      b++;
	    }
	}
    }

  _pango_xft_font_map_set_coverage (xfont->fontmap, &key, coverage);
 
  return coverage;
}

static void
pango_xft_font_get_glyph_extents (PangoFont        *font,
				  PangoGlyph        glyph,
				  PangoRectangle   *ink_rect,
				  PangoRectangle   *logical_rect)
{
  PangoXftFont *xfont = (PangoXftFont *)font;
  XftFont *xft_font = pango_xft_font_get_font (font);
  XGlyphInfo extents;
  Display *display;

  if (!xfont->fontmap)		/* Display closed */
    goto fallback;

  _pango_xft_font_map_get_info (xfont->fontmap, &display, NULL);
  
  if (glyph == (PangoGlyph)-1)
    glyph = 0;

  if (glyph & PANGO_XFT_UNKNOWN_FLAG)
    {
      get_mini_font (font);
      
      if (ink_rect)
	{
	  ink_rect->x = 0;
	  ink_rect->y = PANGO_SCALE * (- xft_font->ascent + (xft_font->ascent + xft_font->descent - xfont->mini_height * 2 - xfont->mini_pad * 5) / 2);
	  ink_rect->width = PANGO_SCALE * (xfont->mini_width * 2 + xfont->mini_pad * 5);
	  ink_rect->height = PANGO_SCALE * (xfont->mini_height * 2 + xfont->mini_pad * 5);
	}
      
      if (logical_rect)
	{
	  logical_rect->x = 0;
	  logical_rect->y = - PANGO_SCALE * xft_font->ascent;
	  logical_rect->width = PANGO_SCALE * (xfont->mini_width * 2 + xfont->mini_pad * 6);
	  logical_rect->height = (xft_font->ascent + xft_font->descent) * PANGO_SCALE;
	}
    }
  else if (glyph)
    {
      FT_UInt ft_glyph = glyph;
      XftGlyphExtents (display, xft_font, &ft_glyph, 1, &extents);

      if (ink_rect)
	{
	  ink_rect->x = - extents.x * PANGO_SCALE; /* Xft crack-rock sign choice */
	  ink_rect->y = - extents.y * PANGO_SCALE; /*             "              */
	  ink_rect->width = extents.width * PANGO_SCALE;
	  ink_rect->height = extents.height * PANGO_SCALE;
	}
      
      if (logical_rect)
	{
	  logical_rect->x = 0;
	  logical_rect->y = - xft_font->ascent * PANGO_SCALE;
	  logical_rect->width = extents.xOff * PANGO_SCALE;
	  logical_rect->height = (xft_font->ascent + xft_font->descent) * PANGO_SCALE;
	}
    }
  else
    {
    fallback:
      
      if (ink_rect)
	{
	  ink_rect->x = 0;
	  ink_rect->width = 0;
	  ink_rect->y = 0;
	  ink_rect->height = 0;
	}
      if (logical_rect)
	{
	  logical_rect->x = 0;
	  logical_rect->width = 0;
	  logical_rect->y = 0;
	  logical_rect->height = 0;
	}
    }
}

static PangoMap *
pango_xft_get_shaper_map (PangoLanguage *language)
{
  static guint engine_type_id = 0;
  static guint render_type_id = 0;
  
  if (engine_type_id == 0)
    {
      engine_type_id = g_quark_from_static_string (PANGO_ENGINE_TYPE_SHAPE);
      render_type_id = g_quark_from_static_string (PANGO_RENDER_TYPE_XFT);
    }
  
  return pango_find_map (language, engine_type_id, render_type_id);
}

static PangoEngineShape *
pango_xft_font_find_shaper (PangoFont     *font,
			    PangoLanguage *language,
			    guint32        ch)
{
  PangoMap *shape_map = NULL;

  shape_map = pango_xft_get_shaper_map (language);
  return (PangoEngineShape *)pango_map_get_engine (shape_map, ch);
}

static gboolean
set_unicode_charmap (FT_Face face)
{
  int charmap;

  for (charmap = 0; charmap < face->num_charmaps; charmap++)
    if (face->charmaps[charmap]->encoding == ft_encoding_unicode)
      {
	FT_Error error = FT_Set_Charmap(face, face->charmaps[charmap]);
	return error == FT_Err_Ok;
      }

  return FALSE;
}

static void
load_fallback_font (PangoXftFont *xfont)
{
  Display *display;
  int screen;
  XftFont *xft_font;
  
  _pango_xft_font_map_get_info (xfont->fontmap, &display, &screen);
      
  xft_font = XftFontOpen (display,  screen,
                          FC_FAMILY, FcTypeString, "sans",
                          FC_SIZE, FcTypeDouble, (double)pango_font_description_get_size (xfont->description)/PANGO_SCALE,
                          NULL);
  
  if (!xft_font)
    {
      g_warning ("Cannot open fallback font, nothing to do");
      exit (1);
    }

  xfont->xft_font = xft_font;
}

/**
 * pango_xft_font_get_font:
 * @font: a #PangoFont.
 *
 * Returns the XftFont of a font.
 *
 * Returns: the XftFont associated to @font.
 **/
XftFont *
pango_xft_font_get_font (PangoFont *font)
{
  PangoXftFont *xfont;
  Display *display;
  int screen;

  g_return_val_if_fail (PANGO_XFT_IS_FONT (font), NULL);
  
  xfont = PANGO_XFT_FONT (font);

  if (xfont->xft_font == NULL)
    {
      _pango_xft_font_map_get_info (xfont->fontmap, &display, &screen);

      xfont->xft_font = XftFontOpenPattern (display, FcPatternDuplicate (xfont->font_pattern));
      if (!xfont->xft_font)
	{
	  gchar *name = pango_font_description_to_string (xfont->description);
	  g_warning ("Cannot open font file for font %s", name);
  	  g_free (name);
  	  
 	  load_fallback_font (xfont);
	}
    }
  
  return xfont->xft_font;
}

/**
 * pango_xft_font_get_display:
 * @font: a #PangoFont.
 *
 * Returns the X display of the XftFont of a font.
 *
 * Returns: the X display of the XftFont associated to @font.
 **/
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

/**
 * pango_xft_font_get_unknown_glyph:
 * @font: a #PangoFont.
 * @wc: the Unicode character for which a glyph is needed.
 *
 * Returns the index of a glyph suitable for drawing @wc as an
 * unknown character.
 *
 * Return value: a glyph index into @font.
 **/
PangoGlyph
pango_xft_font_get_unknown_glyph (PangoFont *font,
				  gunichar   wc)
{
  g_return_val_if_fail (PANGO_XFT_IS_FONT (font), -1);

  return wc | PANGO_XFT_UNKNOWN_FLAG;
}

/**
 * pango_xft_font_get_face:
 * @font: a #PangoFont.
 *
 * Gets the FreeType FT_Face associated with a font.
 *
 * Returns: the FreeType FT_Face associated with @font.
 **/
FT_Face
pango_xft_font_lock_face (PangoFont *font)
{
  XftFont *xft_font;

  g_return_val_if_fail (PANGO_XFT_IS_FONT (font), NULL);

  xft_font = pango_xft_font_get_font (font);
  
  return XftLockFace (xft_font);
}

/**
 * pango_xft_font_unlock_face:
 * @font: a #PangoFont.
 *
 * Gets the FreeType FT_Face associated with a font.
 *
 * Returns: the FreeType FT_Face associated with @font.
 **/
void
pango_xft_font_unlock_face (PangoFont *font)
{
  XftFont *xft_font;

  g_return_if_fail (PANGO_XFT_IS_FONT (font));

  xft_font = pango_xft_font_get_font (font);
  
  XftUnlockFace (xft_font);
}

/**
 * pango_xft_font_get_glyph:
 * @font: a #PangoFont for the Xft backend
 * @wc: Unicode codepoint to look up
 * 
 * Gets the glyph index for a given unicode codepoint
 * for @font. If you only want to determine
 * whether the font has the glyph, use pango_xft_font_has_char().
 * 
 * Return value: the glyph index, or 0, if the unicode
 *  codepoint doesn't exist in the font.
 **/
guint
pango_xft_font_get_glyph (PangoFont *font,
			  gunichar   wc)
{
  XftFont *xft_font;

  g_return_val_if_fail (PANGO_XFT_IS_FONT (font), 0);

  xft_font = pango_xft_font_get_font (font);
  
  return XftCharIndex (NULL, xft_font, wc);
}

/**
 * pango_xft_font_has_char:
 * @font: a #PangoFont for the Xft backend
 * @wc: Unicode codepoint to look up
 * 
 * Determines whether @font has a glyph for the codepoint @wc.
 * 
 * Return value: %TRUE if @font has the requested codepoint.
 **/
gboolean
pango_xft_font_has_char (PangoFont *font,
			 gunichar   wc)
{
  XftFont *xft_font;

  g_return_val_if_fail (PANGO_XFT_IS_FONT (font), 0);

  xft_font = pango_xft_font_get_font (font);
  
  return XftCharExists (NULL, xft_font, wc);
}
