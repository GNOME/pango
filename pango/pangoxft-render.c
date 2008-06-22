/* Pango
 * pangoxft-render.c: Rendering routines for the Xft library
 *
 * Copyright (C) 2004 Red Hat Software
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
#include <math.h>

#include "pangoxft-render.h"
#include "pangoxft-private.h"

enum {
  PROP_0,
  PROP_DISPLAY,
  PROP_SCREEN
};

struct _PangoXftRendererPrivate
{
  PangoColor default_color;
  guint16 alpha;

  Picture src_picture;
  Picture dest_picture;

  XRenderPictFormat *mask_format;

  GArray *trapezoids;
  PangoRenderPart trapezoid_part;

  GArray *glyphs;
  PangoFont *glyph_font;
};

static void pango_xft_renderer_finalize     (GObject      *object);
static void pango_xft_renderer_set_property (GObject      *object,
					     guint         prop_id,
					     const GValue *value,
					     GParamSpec   *pspec);

static void pango_xft_renderer_real_composite_trapezoids (PangoXftRenderer *xftrenderer,
							  PangoRenderPart   part,
							  XTrapezoid       *trapezoids,
							  int               n_trapezoids);
static void pango_xft_renderer_real_composite_glyphs     (PangoXftRenderer *xftrenderer,
							  XftFont          *xft_font,
							  XftGlyphSpec     *glyphs,
							  int               n_glyphs);

static void pango_xft_renderer_draw_glyphs    (PangoRenderer    *renderer,
					       PangoFont        *font,
					       PangoGlyphString *glyphs,
					       int               x,
					       int               y);
static void pango_xft_renderer_draw_trapezoid (PangoRenderer    *renderer,
					       PangoRenderPart   part,
					       double            y1,
					       double            x11,
					       double            x21,
					       double            y2,
					       double            x12,
					       double            x22);
static void pango_xft_renderer_part_changed   (PangoRenderer    *renderer,
					       PangoRenderPart   part);
static void pango_xft_renderer_end            (PangoRenderer    *renderer);

static void flush_trapezoids (PangoXftRenderer *xftrenderer);
static void flush_glyphs (PangoXftRenderer *xftrenderer);

G_DEFINE_TYPE (PangoXftRenderer, pango_xft_renderer, PANGO_TYPE_RENDERER)

static void
pango_xft_renderer_init (PangoXftRenderer *xftrenderer)
{
  xftrenderer->priv = G_TYPE_INSTANCE_GET_PRIVATE (xftrenderer,
						   PANGO_TYPE_XFT_RENDERER,
						   PangoXftRendererPrivate);
  xftrenderer->priv->alpha = 0xffff;
}

static void
pango_xft_renderer_class_init (PangoXftRendererClass *klass)
{
  GObjectClass *object_class = G_OBJECT_CLASS (klass);
  PangoRendererClass *renderer_class = PANGO_RENDERER_CLASS (klass);

  klass->composite_glyphs = pango_xft_renderer_real_composite_glyphs;
  klass->composite_trapezoids = pango_xft_renderer_real_composite_trapezoids;

  renderer_class->draw_glyphs = pango_xft_renderer_draw_glyphs;
  renderer_class->draw_trapezoid = pango_xft_renderer_draw_trapezoid;
  renderer_class->part_changed = pango_xft_renderer_part_changed;
  renderer_class->end = pango_xft_renderer_end;

  object_class->finalize = pango_xft_renderer_finalize;
  object_class->set_property = pango_xft_renderer_set_property;

  g_object_class_install_property (object_class, PROP_DISPLAY,
				   g_param_spec_pointer ("display",
							 "Display",
							 "The display being rendered to",
							 G_PARAM_WRITABLE | G_PARAM_CONSTRUCT_ONLY));
  g_object_class_install_property (object_class, PROP_SCREEN,
				   g_param_spec_int ("screen",
						     "Screen",
						     "The screen being rendered to",
						     0, G_MAXINT, 0,
						     G_PARAM_WRITABLE | G_PARAM_CONSTRUCT_ONLY));

  g_type_class_add_private (object_class, sizeof (PangoXftRendererPrivate));
}

static void
pango_xft_renderer_finalize (GObject *object)
{
  PangoXftRenderer *renderer = PANGO_XFT_RENDERER (object);

  if (renderer->priv->glyphs)
    g_array_free (renderer->priv->glyphs, TRUE);
  if (renderer->priv->trapezoids)
    g_array_free (renderer->priv->trapezoids, TRUE);

  G_OBJECT_CLASS (pango_xft_renderer_parent_class)->finalize (object);
}

static void
pango_xft_renderer_set_property (GObject      *object,
				 guint         prop_id,
				 const GValue *value,
				 GParamSpec   *pspec)
{
  PangoXftRenderer *xftrenderer = PANGO_XFT_RENDERER (object);

  switch (prop_id)
    {
    case PROP_DISPLAY:
      xftrenderer->display = g_value_get_pointer (value);
      /* We possibly should use ARGB format when subpixel-AA is turned
       * on for the fontmap; we could discover that using the technique
       * for FC_DPI in pango_fc_face_list_sizes.
       */
      xftrenderer->priv->mask_format = XRenderFindStandardFormat (xftrenderer->display,
								  PictStandardA8);
      break;
    case PROP_SCREEN:
      xftrenderer->screen = g_value_get_int (value);
      break;
    default:
      G_OBJECT_WARN_INVALID_PROPERTY_ID (object, prop_id, pspec);
      break;
    }
}

static void
pango_xft_renderer_set_pictures (PangoXftRenderer *renderer,
				 Picture           src_picture,
				 Picture           dest_picture)
{
  renderer->priv->src_picture = src_picture;
  renderer->priv->dest_picture = dest_picture;
}

static void
flush_glyphs (PangoXftRenderer *xftrenderer)
{
  XftFont *xft_font;

  if (!xftrenderer->priv->glyphs ||
      xftrenderer->priv->glyphs->len == 0)
    return;

  xft_font = pango_xft_font_get_font (xftrenderer->priv->glyph_font);

  PANGO_XFT_RENDERER_GET_CLASS (xftrenderer)->composite_glyphs (xftrenderer,
								xft_font,
								(XftGlyphSpec *)xftrenderer->priv->glyphs->data,
								xftrenderer->priv->glyphs->len);

  g_array_set_size (xftrenderer->priv->glyphs, 0);
  g_object_unref (xftrenderer->priv->glyph_font);
  xftrenderer->priv->glyph_font = NULL;
}

#define MAX_GLYPHS	1024

static void
draw_glyph (PangoRenderer *renderer,
	    PangoFont     *font,
	    FT_UInt        glyph,
	    int            x,
	    int            y)
{
  PangoXftRenderer *xftrenderer = PANGO_XFT_RENDERER (renderer);
  XftGlyphSpec gs;
  int pixel_x, pixel_y;

  if (renderer->matrix)
    {
      pixel_x = floor (0.5 + (x * renderer->matrix->xx + y * renderer->matrix->xy) / PANGO_SCALE + renderer->matrix->x0);
      pixel_y = floor (0.5 + (x * renderer->matrix->yx + y * renderer->matrix->yy) / PANGO_SCALE + renderer->matrix->y0);
    }
  else
    {
      pixel_x = PANGO_PIXELS (x);
      pixel_y = PANGO_PIXELS (y);
    }

  /* Clip glyphs into the X coordinate range; we really
   * want to clip glyphs with an ink rect outside the
   * [0,32767] x [0,32767] rectangle but looking up
   * the ink rect here would be a noticeable speed hit.
   * This is close enough.
   */
  if (pixel_x < -32768 || pixel_x > 32767 ||
      pixel_y < -32768 || pixel_y > 32767)
    return;

  flush_trapezoids (xftrenderer);

  if (!xftrenderer->priv->glyphs)
    xftrenderer->priv->glyphs = g_array_new (FALSE, FALSE,
					     sizeof (XftGlyphSpec));

  if (xftrenderer->priv->glyph_font != font ||
      xftrenderer->priv->glyphs->len == MAX_GLYPHS)
    {
      flush_glyphs (xftrenderer);

      xftrenderer->priv->glyph_font = g_object_ref (font);
    }

  gs.x = pixel_x;
  gs.y = pixel_y;
  gs.glyph = glyph;

  g_array_append_val (xftrenderer->priv->glyphs, gs);
}

static gboolean
point_in_bounds (PangoRenderer *renderer,
		 gint           x,
		 gint           y)
{
  gdouble pixel_x = (x * renderer->matrix->xx + y * renderer->matrix->xy) / PANGO_SCALE + renderer->matrix->x0;
  gdouble pixel_y = (x * renderer->matrix->yx + y * renderer->matrix->yy) / PANGO_SCALE + renderer->matrix->y0;

  return (pixel_x >= -32768. && pixel_x < 32768. &&
	  pixel_y >= -32768. && pixel_y < 32768.);
}

static gboolean
box_in_bounds (PangoRenderer *renderer,
	       gint           x,
	       gint           y,
	       gint           width,
	       gint           height)
{
  if (!renderer->matrix)
    {
#define COORD_MIN (PANGO_SCALE * -16384 - PANGO_SCALE / 2)
#define COORD_MAX (PANGO_SCALE * 32767 + PANGO_SCALE / 2 - 1)
      return (x >= COORD_MIN && x + width <= COORD_MAX &&
	      y >= COORD_MIN && y + width <= COORD_MAX);
#undef COORD_MIN
#undef COORD_MAX
    }
  else
    {
      return (point_in_bounds (renderer, x, y) &&
	      point_in_bounds (renderer, x + width, y) &&
	      point_in_bounds (renderer, x + width, y + height) &&
	      point_in_bounds (renderer, x, y + height));
    }
}

static void
get_total_matrix (PangoMatrix       *total,
		  const PangoMatrix *global,
		  double             x,
		  double             y,
		  double             width,
		  double             height)
{
  PangoMatrix local = PANGO_MATRIX_INIT;
  gdouble angle = atan2 (height, width);

  pango_matrix_translate (&local, x, y);
  pango_matrix_rotate (&local, -angle * (180. / G_PI));

  *total = *global;
  pango_matrix_concat (total, &local);
}

static void
draw_box (PangoRenderer *renderer,
	  gint           line_width,
	  gint           x,
	  gint           y,
	  gint           width,
	  gint           height,
	  gboolean       invalid)
{
  pango_renderer_draw_rectangle (renderer, PANGO_RENDER_PART_FOREGROUND,
				 x, y, width, line_width);
  pango_renderer_draw_rectangle (renderer, PANGO_RENDER_PART_FOREGROUND,
				 x, y + line_width, line_width, height - line_width * 2);
  pango_renderer_draw_rectangle (renderer, PANGO_RENDER_PART_FOREGROUND,
				 x + width - line_width, y + line_width, line_width, height - line_width * 2);
  pango_renderer_draw_rectangle (renderer, PANGO_RENDER_PART_FOREGROUND,
				 x, y + height - line_width, width, line_width);

  if (invalid)
    {
      int length;
      double in_width, in_height;
      PangoMatrix orig_matrix = PANGO_MATRIX_INIT, new_matrix;
      const PangoMatrix *porig_matrix;

      in_width  = pango_units_to_double (width  - 2 * line_width);
      in_height = pango_units_to_double (height - 2 * line_width);
      length = PANGO_SCALE * sqrt (in_width*in_width + in_height*in_height);

      porig_matrix = pango_renderer_get_matrix (renderer);
      if (porig_matrix)
        {
	  orig_matrix = *porig_matrix;
	  porig_matrix = &orig_matrix;
	}

      get_total_matrix (&new_matrix, &orig_matrix,
			pango_units_to_double (x + line_width), pango_units_to_double (y + line_width),
			in_width, in_height);
      pango_renderer_set_matrix (renderer, &new_matrix);
      pango_renderer_draw_rectangle (renderer, PANGO_RENDER_PART_FOREGROUND,
				     0, -line_width / 2, length, line_width);

      get_total_matrix (&new_matrix, &orig_matrix,
			pango_units_to_double (x + line_width), pango_units_to_double (y + height - line_width),
			in_width, -in_height);
      pango_renderer_set_matrix (renderer, &new_matrix);
      pango_renderer_draw_rectangle (renderer, PANGO_RENDER_PART_FOREGROUND,
				     0, -line_width / 2, length, line_width);

      pango_renderer_set_matrix (renderer, porig_matrix);
    }
}

static void
_pango_xft_renderer_draw_box_glyph (PangoRenderer    *renderer,
				    PangoGlyphInfo   *gi,
				    int               glyph_x,
				    int               glyph_y,
				    gboolean          invalid)
{
  int x = glyph_x + PANGO_SCALE;
  int y = glyph_y - PANGO_SCALE * (PANGO_UNKNOWN_GLYPH_HEIGHT - 1);
  int width = gi->geometry.width - PANGO_SCALE * 2;
  int height = PANGO_SCALE * (PANGO_UNKNOWN_GLYPH_HEIGHT - 2);

  if (box_in_bounds (renderer, x, y, width, height))
    draw_box (renderer, PANGO_SCALE, x, y, width, height, invalid);
}

static void
_pango_xft_renderer_draw_unknown_glyph (PangoRenderer    *renderer,
					PangoXftFont     *xfont,
					XftFont          *xft_font,
					PangoGlyphInfo   *gi,
					int               glyph_x,
					int               glyph_y)
{
  char buf[7];
  int ys[3];
  int xs[4];
  int row, col;
  int cols;
  gunichar ch;
  gboolean invalid_input;

  PangoFont *mini_font;
  XftFont *mini_xft_font;

  ch = gi->glyph & ~PANGO_GLYPH_UNKNOWN_FLAG;
  if (G_UNLIKELY (gi->glyph == PANGO_GLYPH_INVALID_INPUT || ch > 0x10FFFF))
    {
      invalid_input = TRUE;
      cols = 1;
    }
  else
    {
      invalid_input = FALSE;
      cols = ch > 0xffff ? 3 : 2;
      g_snprintf (buf, sizeof(buf), (ch > 0xffff) ? "%06X" : "%04X", ch);
    }

  mini_font = _pango_xft_font_get_mini_font (xfont);
  mini_xft_font = pango_xft_font_get_font (mini_font);
  if (!mini_xft_font)
    {
      _pango_xft_renderer_draw_box_glyph (renderer, gi, glyph_x, glyph_y, invalid_input);
      return;
    }


  ys[0] = glyph_y - PANGO_SCALE * xft_font->ascent + PANGO_SCALE * (((xft_font->ascent + xft_font->descent) - (xfont->mini_height * 2 + xfont->mini_pad * 5 + PANGO_SCALE / 2) / PANGO_SCALE) / 2);
  ys[1] = ys[0] + 2 * xfont->mini_pad + xfont->mini_height;
  ys[2] = ys[1] + xfont->mini_height + xfont->mini_pad;

  xs[0] = glyph_x;
  xs[1] = xs[0] + 2 * xfont->mini_pad;
  xs[2] = xs[1] + xfont->mini_width + xfont->mini_pad;
  xs[3] = xs[2] + xfont->mini_width + xfont->mini_pad;

  if (box_in_bounds (renderer,
		     xs[0], ys[0],
		     xfont->mini_width * cols + xfont->mini_pad * (2 * cols + 1),
		     xfont->mini_height * 2 + xfont->mini_pad * 5))
    {
      if (xfont->mini_pad)
	draw_box (renderer, xfont->mini_pad,
		  xs[0], ys[0],
		  xfont->mini_width * cols + xfont->mini_pad * (2 * cols + 1),
		  xfont->mini_height * 2 + xfont->mini_pad * 5,
		  invalid_input);

      if (invalid_input)
        return;

      for (row = 0; row < 2; row++)
	for (col = 0; col < cols; col++)
	  {
	    draw_glyph (renderer, mini_font,
			XftCharIndex (NULL, mini_xft_font,
				      buf[row * cols + col] & 0xff),
			xs[col+1],
			ys[row+1]);
	  }
    }
}

static void
pango_xft_renderer_draw_glyphs (PangoRenderer    *renderer,
				PangoFont        *font,
				PangoGlyphString *glyphs,
				int               x,
				int               y)
{
  PangoXftFont *xfont = PANGO_XFT_FONT (font);
  PangoFcFont *fcfont = PANGO_FC_FONT (font);
  XftFont *xft_font = pango_xft_font_get_font (font);
  int i;
  int x_off = 0;

  if (!fcfont)
    {
      for (i=0; i<glyphs->num_glyphs; i++)
	{
	  PangoGlyphInfo *gi = &glyphs->glyphs[i];

	  if (gi->glyph != PANGO_GLYPH_EMPTY)
	    {
	      int glyph_x = x + x_off + gi->geometry.x_offset;
	      int glyph_y = y + gi->geometry.y_offset;

	      _pango_xft_renderer_draw_unknown_glyph (renderer,
						      xfont,
						      xft_font,
						      gi,
						      glyph_x,
						      glyph_y);
	    }

	  x_off += gi->geometry.width;
	}
      return;
    }

  if (!fcfont->fontmap)	/* Display closed */
    return;

  for (i=0; i<glyphs->num_glyphs; i++)
    {
      PangoGlyphInfo *gi = &glyphs->glyphs[i];

      if (gi->glyph != PANGO_GLYPH_EMPTY)
	{
	  int glyph_x = x + x_off + gi->geometry.x_offset;
	  int glyph_y = y + gi->geometry.y_offset;

	  if (gi->glyph & PANGO_GLYPH_UNKNOWN_FLAG)
	    {
	      _pango_xft_renderer_draw_unknown_glyph (renderer,
						      xfont,
						      xft_font,
						      gi,
						      glyph_x,
						      glyph_y);
	    }
	  else
	    {
	      draw_glyph (renderer, font, gi->glyph, glyph_x, glyph_y);
	    }
	}

      x_off += gi->geometry.width;
    }
}

static void
flush_trapezoids (PangoXftRenderer *xftrenderer)
{
  if (!xftrenderer->priv->trapezoids ||
      xftrenderer->priv->trapezoids->len == 0)
    return;

  PANGO_XFT_RENDERER_GET_CLASS (xftrenderer)->composite_trapezoids (xftrenderer,
								    xftrenderer->priv->trapezoid_part,
								    (XTrapezoid *)xftrenderer->priv->trapezoids->data,
								    xftrenderer->priv->trapezoids->len);

  g_array_set_size (xftrenderer->priv->trapezoids, 0);
}

static void
pango_xft_renderer_draw_trapezoid (PangoRenderer   *renderer,
				   PangoRenderPart  part,
				   double           y1,
				   double           x11,
				   double           x21,
				   double           y2,
				   double           x12,
				   double           x22)
{
  PangoXftRenderer *xftrenderer = PANGO_XFT_RENDERER (renderer);
  XTrapezoid trap;

  flush_glyphs (xftrenderer);

  if (!xftrenderer->priv->trapezoids)
    xftrenderer->priv->trapezoids = g_array_new (FALSE, FALSE,
						 sizeof (XTrapezoid));

  if (xftrenderer->draw)
    {
      if (xftrenderer->priv->trapezoids->len > 0 &&
	  xftrenderer->priv->trapezoid_part != part)
	flush_trapezoids (xftrenderer);

      xftrenderer->priv->trapezoid_part = part;
    }

  trap.top = XDoubleToFixed (y1);
  trap.bottom = XDoubleToFixed (y2);
  trap.left.p1.x = XDoubleToFixed (x11);
  trap.left.p1.y = XDoubleToFixed (y1);
  trap.left.p2.x = XDoubleToFixed (x12);
  trap.left.p2.y = XDoubleToFixed (y2);
  trap.right.p1.x = XDoubleToFixed (x21);
  trap.right.p1.y = XDoubleToFixed (y1);
  trap.right.p2.x = XDoubleToFixed (x22);
  trap.right.p2.y = XDoubleToFixed (y2);

  g_array_append_val (xftrenderer->priv->trapezoids, trap);
}

static void
pango_xft_renderer_part_changed (PangoRenderer   *renderer,
				 PangoRenderPart  part)
{
  PangoXftRenderer *xftrenderer = PANGO_XFT_RENDERER (renderer);

  if (part == PANGO_RENDER_PART_FOREGROUND)
    flush_glyphs (xftrenderer);

  if (part == xftrenderer->priv->trapezoid_part)
    flush_trapezoids (xftrenderer);
}

static void
pango_xft_renderer_end (PangoRenderer *renderer)
{
  PangoXftRenderer *xftrenderer = PANGO_XFT_RENDERER (renderer);

  flush_glyphs (xftrenderer);
  flush_trapezoids (xftrenderer);
}

static void
pango_xft_renderer_real_composite_trapezoids (PangoXftRenderer *xftrenderer,
					      PangoRenderPart   part,
					      XTrapezoid       *trapezoids,
					      int               n_trapezoids)
{
  Picture src_picture;
  Picture dest_picture;

  if (!XftDefaultHasRender (xftrenderer->display))
      return;

  if (xftrenderer->priv->src_picture != None)
    {
      src_picture = xftrenderer->priv->src_picture;
      dest_picture = xftrenderer->priv->dest_picture;
    }
  else
    {
      XftColor xft_color;
      PangoColor *color = pango_renderer_get_color (PANGO_RENDERER (xftrenderer),
						    part);
      if (!color)
	color = &xftrenderer->priv->default_color;

      xft_color.color.red = color->red;
      xft_color.color.green = color->green;
      xft_color.color.blue = color->blue;
      xft_color.color.alpha = xftrenderer->priv->alpha;

      src_picture = XftDrawSrcPicture (xftrenderer->draw, &xft_color);
      dest_picture = XftDrawPicture (xftrenderer->draw);
    }

  XRenderCompositeTrapezoids (xftrenderer->display,
			      PictOpOver,
			      src_picture, dest_picture,
			      xftrenderer->priv->mask_format,
			      0, 0,
			      trapezoids, n_trapezoids);
}

static void
pango_xft_renderer_real_composite_glyphs (PangoXftRenderer *xftrenderer,
					  XftFont          *xft_font,
					  XftGlyphSpec     *glyphs,
					  int               n_glyphs)
{
  if (xftrenderer->priv->src_picture != None)
    {
      XftGlyphSpecRender (xftrenderer->display, PictOpOver,
			  xftrenderer->priv->src_picture,
			  xft_font,
			  xftrenderer->priv->dest_picture, 0, 0,
			  glyphs, n_glyphs);
    }
  else
    {
      XftColor xft_color;
      PangoColor *color = pango_renderer_get_color (PANGO_RENDERER (xftrenderer),
						    PANGO_RENDER_PART_FOREGROUND);
      if (!color)
	color = &xftrenderer->priv->default_color;

      xft_color.color.red = color->red;
      xft_color.color.green = color->green;
      xft_color.color.blue = color->blue;
      xft_color.color.alpha = xftrenderer->priv->alpha;

      XftDrawGlyphSpec (xftrenderer->draw, &xft_color,
			xft_font,
			glyphs, n_glyphs);
    }
}

static PangoRenderer *
get_renderer (PangoFontMap *fontmap,
	      XftDraw      *draw,
	      XftColor     *color)
{
  PangoRenderer *renderer;
  PangoXftRenderer *xftrenderer;
  PangoColor pango_color;

  renderer = _pango_xft_font_map_get_renderer (PANGO_XFT_FONT_MAP (fontmap));
  xftrenderer = PANGO_XFT_RENDERER (renderer);

  pango_xft_renderer_set_draw (xftrenderer, draw);

  pango_color.red = color->color.red;
  pango_color.green = color->color.green;
  pango_color.blue = color->color.blue;
  pango_xft_renderer_set_default_color (xftrenderer, &pango_color);
  xftrenderer->priv->alpha = color->color.alpha;

  return renderer;
}

static void
release_renderer (PangoRenderer *renderer)
{
  PangoXftRenderer *xftrenderer = PANGO_XFT_RENDERER (renderer);

  xftrenderer->priv->alpha = 0xffff;
}

/**
 * pango_xft_render_layout:
 * @draw:      an #XftDraw
 * @color:     the foreground color in which to draw the layout
 *             (may be overridden by color attributes)
 * @layout:    a #PangoLayout
 * @x:         the X position of the left of the layout (in Pango units)
 * @y:         the Y position of the top of the layout (in Pango units)
 *
 * Render a #PangoLayout onto a #XftDraw
 *
 * Since: 1.8
 */
void
pango_xft_render_layout (XftDraw     *draw,
			 XftColor    *color,
			 PangoLayout *layout,
			 int          x,
			 int          y)
{
  PangoContext *context;
  PangoFontMap *fontmap;
  PangoRenderer *renderer;

  g_return_if_fail (draw != NULL);
  g_return_if_fail (color != NULL);
  g_return_if_fail (PANGO_IS_LAYOUT (layout));

  context = pango_layout_get_context (layout);
  fontmap = pango_context_get_font_map (context);
  renderer = get_renderer (fontmap, draw, color);

  pango_renderer_draw_layout (renderer, layout, x, y);

  release_renderer (renderer);
}

/**
 * pango_xft_render_layout_line:
 * @draw:      an #XftDraw
 * @color:     the foreground color in which to draw the layout line
 *             (may be overridden by color attributes)
 * @line:      a #PangoLayoutLine
 * @x:         the x position of start of string (in Pango units)
 * @y:         the y position of baseline (in Pango units)
 *
 * Render a #PangoLayoutLine onto a #XftDraw
 *
 * Since: 1.8
 */
void
pango_xft_render_layout_line (XftDraw         *draw,
			      XftColor        *color,
			      PangoLayoutLine *line,
			      int              x,
			      int              y)
{
  PangoContext *context;
  PangoFontMap *fontmap;
  PangoRenderer *renderer;

  g_return_if_fail (draw != NULL);
  g_return_if_fail (color != NULL);
  g_return_if_fail (line != NULL);

  context = pango_layout_get_context (line->layout);
  fontmap = pango_context_get_font_map (context);
  renderer = get_renderer (fontmap, draw, color);

  pango_renderer_draw_layout_line (renderer, line, x, y);

  release_renderer (renderer);
}

/**
 * pango_xft_render_transformed:
 * @draw:    an #XftDraw
 * @color:   the color in which to draw the glyphs
 * @font:    the font in which to draw the string
 * @matrix:  a #PangoMatrix, or %NULL to use an identity transformation
 * @glyphs:  the glyph string to draw
 * @x:       the x position of the start of the string (in Pango
 *           units in user space coordinates)
 * @y:       the y position of the baseline (in Pango units
 *           in user space coordinates)
 *
 * Renders a #PangoGlyphString onto a #XftDraw, possibly
 * transforming the layed-out coordinates through a transformation
 * matrix. Note that the transformation matrix for @font is not
 * changed, so to produce correct rendering results, the @font
 * must have been loaded using a #PangoContext with an identical
 * transformation matrix to that passed in to this function.
 *
 * Since: 1.8
 **/
void
pango_xft_render_transformed (XftDraw          *draw,
			      XftColor         *color,
			      PangoMatrix      *matrix,
			      PangoFont        *font,
			      PangoGlyphString *glyphs,
			      int               x,
			      int               y)
{
  PangoFontMap *fontmap;
  PangoRenderer *renderer;

  g_return_if_fail (draw != NULL);
  g_return_if_fail (color != NULL);
  g_return_if_fail (PANGO_XFT_IS_FONT (font));
  g_return_if_fail (glyphs != NULL);

  fontmap = PANGO_FC_FONT (font)->fontmap;
  renderer = get_renderer (fontmap, draw, color);

  pango_renderer_set_matrix (renderer, matrix);

  pango_renderer_draw_glyphs (renderer, font, glyphs, x, y);

  release_renderer (renderer);
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

  pango_xft_render_transformed (draw, color, NULL, font, glyphs,
				x * PANGO_SCALE, y * PANGO_SCALE);
}

/**
 * pango_xft_picture_render:
 * @display:      an X display
 * @src_picture:  the source picture to draw the string with
 * @dest_picture: the destination picture to draw the string onto
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
  PangoFontMap *fontmap;
  PangoRenderer *renderer;

  g_return_if_fail (display != NULL);
  g_return_if_fail (src_picture != None);
  g_return_if_fail (dest_picture != None);
  g_return_if_fail (PANGO_XFT_IS_FONT (font));
  g_return_if_fail (glyphs != NULL);

  fontmap = PANGO_FC_FONT (font)->fontmap;
  renderer = _pango_xft_font_map_get_renderer (PANGO_XFT_FONT_MAP (fontmap));

  pango_xft_renderer_set_pictures (PANGO_XFT_RENDERER (renderer),
				   src_picture, dest_picture);
  pango_renderer_set_matrix (renderer, NULL);

  pango_renderer_draw_glyphs (renderer, font, glyphs, x * PANGO_SCALE, y * PANGO_SCALE);

  pango_xft_renderer_set_pictures (PANGO_XFT_RENDERER (renderer),
				   None, None);
}

/**
 * pango_xft_renderer_new:
 * @display: an X display
 * @screen:   the index of the screen for @display to which rendering will be done
 *
 * Create a new #PangoXftRenderer to allow rendering Pango objects
 * with the Xft library. You must call pango_xft_renderer_set_draw() before
 * using the renderer.
 *
 * Return value: the newly created #PangoXftRenderer, which should
 *               be freed with g_object_unref().
 *
 * Since: 1.8
 **/
PangoRenderer *
pango_xft_renderer_new (Display *display,
			int      screen)
{
  PangoXftRenderer *xftrenderer;

  xftrenderer = g_object_new (PANGO_TYPE_XFT_RENDERER,
			      "display", display,
			      "screen", screen,
			      NULL);

  return PANGO_RENDERER (xftrenderer);
}

/**
 * pango_xft_renderer_set_draw:
 * @xftrenderer: a #PangoXftRenderer
 * @draw: a #XftDraw
 *
 * Sets the #XftDraw object that the renderer is drawing to.
 * The renderer must not be currently active.
 *
 * Since: 1.8
 **/
void
pango_xft_renderer_set_draw (PangoXftRenderer *xftrenderer,
			     XftDraw          *draw)
{
  g_return_if_fail (PANGO_IS_XFT_RENDERER (xftrenderer));

  xftrenderer->draw = draw;
}

/**
 * pango_xft_renderer_set_default_color:
 * @xftrenderer: a #XftRenderer
 * @default_color: the default foreground color
 *
 * Sets the default foreground color for a #XftRenderer.
 *
 * Since: 1.8
 **/
void
pango_xft_renderer_set_default_color (PangoXftRenderer *xftrenderer,
				      PangoColor       *default_color)
{
  g_return_if_fail (PANGO_IS_XFT_RENDERER (xftrenderer));

  xftrenderer->priv->default_color = *default_color;
}
