/* Pango
 * pangoft2.c: Routines for handling FreeType2 fonts
 *
 * Copyright (C) 1999 Red Hat Software
 * Copyright (C) 2000 Tor Lillqvist
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

#include <string.h>
#include <stdlib.h>
#include <math.h>
#include <glib.h>
#include <glib/gprintf.h>

#include "pangoft2.h"
#include "pangoft2-private.h"
#include "pangofc-fontmap.h"

/* for compatibility with older freetype versions */
#ifndef FT_LOAD_TARGET_MONO
#define FT_LOAD_TARGET_MONO  FT_LOAD_MONOCHROME
#endif

#define PANGO_TYPE_FT2_FONT              (pango_ft2_font_get_type ())
#define PANGO_FT2_FONT(object)           (G_TYPE_CHECK_INSTANCE_CAST ((object), PANGO_TYPE_FT2_FONT, PangoFT2Font))
#define PANGO_FT2_FONT_CLASS(klass)      (G_TYPE_CHECK_CLASS_CAST ((klass), PANGO_TYPE_FT2_FONT, PangoFT2FontClass))
#define PANGO_FT2_IS_FONT(object)        (G_TYPE_CHECK_INSTANCE_TYPE ((object), PANGO_TYPE_FT2_FONT))
#define PANGO_FT2_IS_FONT_CLASS(klass)   (G_TYPE_CHECK_CLASS_TYPE ((klass), PANGO_TYPE_FT2_FONT))
#define PANGO_FT2_FONT_GET_CLASS(obj)    (G_TYPE_INSTANCE_GET_CLASS ((obj), PANGO_TYPE_FT2_FONT, PangoFT2FontClass))

typedef struct _PangoFT2FontClass   PangoFT2FontClass;

struct _PangoFT2FontClass
{
  PangoFcFontClass parent_class;
};

typedef struct
{
  FT_Bitmap bitmap;
  int bitmap_left;
  int bitmap_top;
} PangoFT2RenderedGlyph;

static PangoFontClass *parent_class;	/* Parent class structure for PangoFT2Font */

static void pango_ft2_font_class_init (PangoFT2FontClass *class);
static void pango_ft2_font_init       (PangoFT2Font      *ft2font);
static void pango_ft2_font_finalize   (GObject         *object);

static void                   pango_ft2_font_get_glyph_extents (PangoFont      *font,
								PangoGlyph      glyph,
								PangoRectangle *ink_rect,
								PangoRectangle *logical_rect);

static FT_Face    pango_ft2_font_real_lock_face         (PangoFcFont      *font);
static void       pango_ft2_font_real_unlock_face       (PangoFcFont      *font);
static gboolean   pango_ft2_font_real_has_char          (PangoFcFont      *font,
							 gunichar          wc);
static guint      pango_ft2_font_real_get_glyph         (PangoFcFont      *font,
							 gunichar          wc);
static PangoGlyph pango_ft2_font_real_get_unknown_glyph (PangoFcFont      *font,
							 gunichar          wc);

static GType      pango_ft2_font_get_type               (void);

static void       pango_ft2_get_item_properties         (PangoItem      *item,
							 PangoUnderline *uline,
							 gboolean       *strikethrough,
							 gint           *rise,
							 gboolean       *shape_set,
							 PangoRectangle *ink_rect,
							 PangoRectangle *logical_rect);


PangoFT2Font *
_pango_ft2_font_new (PangoFT2FontMap *ft2fontmap,
		     FcPattern       *pattern)
{
  PangoFontMap *fontmap = PANGO_FONT_MAP (ft2fontmap);
  PangoFT2Font *ft2font;
  double d;

  g_return_val_if_fail (fontmap != NULL, NULL);
  g_return_val_if_fail (pattern != NULL, NULL);

  ft2font = (PangoFT2Font *)g_object_new (PANGO_TYPE_FT2_FONT,
					  "pattern", pattern,
					  NULL);

  if (FcPatternGetDouble (pattern, FC_PIXEL_SIZE, 0, &d) == FcResultMatch)
    ft2font->size = d*PANGO_SCALE;

  return ft2font;
}

static void
load_fallback_face (PangoFT2Font *ft2font,
		    const char   *original_file)
{
  PangoFcFont *fcfont = PANGO_FC_FONT (ft2font);
  FcPattern *sans;
  FcPattern *matched;
  FcResult result;
  FT_Error error;
  FcChar8 *filename2 = NULL;
  gchar *name;
  int id;
  
  sans = FcPatternBuild (NULL,
			 FC_FAMILY, FcTypeString, "sans",
			 FC_SIZE, FcTypeDouble, (double)pango_font_description_get_size (fcfont->description)/PANGO_SCALE,
			 NULL);
  
  matched = FcFontMatch (0, sans, &result);
  
  if (FcPatternGetString (matched, FC_FILE, 0, &filename2) != FcResultMatch)
    goto bail1;
  
  if (FcPatternGetInteger (matched, FC_INDEX, 0, &id) != FcResultMatch)
    goto bail1;
  
  error = FT_New_Face (_pango_ft2_font_map_get_library (fcfont->fontmap),
		       (char *) filename2, id, &ft2font->face);
  
  
  if (error)
    {
    bail1:
      name = pango_font_description_to_string (fcfont->description);
      g_warning ("Unable to open font file %s for font %s, exiting\n", filename2, name);
      exit (1);
    }
  else
    {
      name = pango_font_description_to_string (fcfont->description);
      g_warning ("Unable to open font file %s for font %s, falling back to %s\n", original_file, name, filename2);
      g_free (name);
    }

  FcPatternDestroy (sans);
  FcPatternDestroy (matched);
}

static void
set_transform (PangoFT2Font *ft2font)
{
  PangoFcFont *fcfont = (PangoFcFont *)ft2font;
  FcMatrix *fc_matrix;

  if (FcPatternGetMatrix (fcfont->font_pattern, FC_MATRIX, 0, &fc_matrix) == FcResultMatch)
    {
      FT_Matrix ft_matrix;
      
      ft_matrix.xx = 0x10000L * fc_matrix->xx;
      ft_matrix.yy = 0x10000L * fc_matrix->yy;
      ft_matrix.xy = 0x10000L * fc_matrix->xy;
      ft_matrix.yx = 0x10000L * fc_matrix->yx;

      FT_Set_Transform (ft2font->face, &ft_matrix, NULL);
    }
}

/**
 * pango_ft2_font_get_face:
 * @font: a #PangoFont
 * 
 * Returns the native FreeType2 FT_Face structure used for this PangoFont.
 * This may be useful if you want to use FreeType2 functions directly.
 *
 * Use pango_fc_font_lock_face() instead; when you are done with a
 * face from pango_fc_font_lock_face() you must call
 * pango_fc_font_unlock_face().
 * 
 * Return value: a pointer to a #FT_Face structure, with the size set correctly
 **/
FT_Face
pango_ft2_font_get_face (PangoFont *font)
{
  PangoFT2Font *ft2font = (PangoFT2Font *)font;
  PangoFcFont *fcfont = (PangoFcFont *)font;
  FT_Error error;
  FcPattern *pattern;
  FcChar8 *filename;
  FcBool antialias, hinting, autohint;
  int id;

  pattern = fcfont->font_pattern;

  if (!ft2font->face)
    {
      ft2font->load_flags = 0;

      /* disable antialiasing if requested */
      if (FcPatternGetBool (pattern,
                            FC_ANTIALIAS, 0, &antialias) != FcResultMatch)
	antialias = FcTrue;

      if (antialias)
        ft2font->load_flags |= FT_LOAD_NO_BITMAP;
      else
	ft2font->load_flags |= FT_LOAD_TARGET_MONO;

      /* disable hinting if requested */
      if (FcPatternGetBool (pattern,
                            FC_HINTING, 0, &hinting) != FcResultMatch)
	hinting = FcTrue;

      if (!hinting)
        ft2font->load_flags |= FT_LOAD_NO_HINTING;

      /* force autohinting if requested */
      if (FcPatternGetBool (pattern,
                            FC_AUTOHINT, 0, &autohint) != FcResultMatch)
	autohint = FcFalse;

      if (autohint)
        ft2font->load_flags |= FT_LOAD_FORCE_AUTOHINT;

      if (FcPatternGetString (pattern, FC_FILE, 0, &filename) != FcResultMatch)
	      goto bail0;
      
      if (FcPatternGetInteger (pattern, FC_INDEX, 0, &id) != FcResultMatch)
	      goto bail0;

      error = FT_New_Face (_pango_ft2_font_map_get_library (fcfont->fontmap),
			   (char *) filename, id, &ft2font->face);
      if (error != FT_Err_Ok)
	{
	bail0:
	  load_fallback_face (ft2font, filename);
	}

      g_assert (ft2font->face);

      set_transform (ft2font);

      error = FT_Set_Char_Size (ft2font->face,
				PANGO_PIXELS_26_6 (ft2font->size),
				PANGO_PIXELS_26_6 (ft2font->size),
				0, 0);
      if (error)
	g_warning ("Error in FT_Set_Char_Size: %d", error);
    }
  
  return ft2font->face;
}

static GType
pango_ft2_font_get_type (void)
{
  static GType object_type = 0;

  if (!object_type)
    {
      static const GTypeInfo object_info =
      {
        sizeof (PangoFT2FontClass),
        (GBaseInitFunc) NULL,
        (GBaseFinalizeFunc) NULL,
        (GClassInitFunc) pango_ft2_font_class_init,
        NULL,           /* class_finalize */
        NULL,           /* class_data */
        sizeof (PangoFT2Font),
        0,              /* n_preallocs */
        (GInstanceInitFunc) pango_ft2_font_init,
      };
      
      object_type = g_type_register_static (PANGO_TYPE_FC_FONT,
                                            "PangoFT2Font",
                                            &object_info, 0);
    }
  
  return object_type;
}

static void 
pango_ft2_font_init (PangoFT2Font *ft2font)
{
  ft2font->face = NULL;

  ft2font->size = 0;

  ft2font->glyph_info = g_hash_table_new (NULL, NULL);
}

static void
pango_ft2_font_class_init (PangoFT2FontClass *class)
{
  GObjectClass *object_class = G_OBJECT_CLASS (class);
  PangoFontClass *font_class = PANGO_FONT_CLASS (class);
  PangoFcFontClass *fc_font_class = PANGO_FC_FONT_CLASS (class);
  
  parent_class = g_type_class_peek_parent (class);
  
  object_class->finalize = pango_ft2_font_finalize;
  
  font_class->get_glyph_extents = pango_ft2_font_get_glyph_extents;
  
  fc_font_class->lock_face = pango_ft2_font_real_lock_face;
  fc_font_class->unlock_face = pango_ft2_font_real_unlock_face;
  fc_font_class->has_char = pango_ft2_font_real_has_char;
  fc_font_class->get_glyph = pango_ft2_font_real_get_glyph;
  fc_font_class->get_unknown_glyph = pango_ft2_font_real_get_unknown_glyph;
}

static void
pango_ft2_free_rendered_glyph (PangoFT2RenderedGlyph *rendered)
{
  g_free (rendered->bitmap.buffer);
  g_free (rendered);
}

static PangoFT2RenderedGlyph *
pango_ft2_font_render_glyph (PangoFont *font,
			     int glyph_index)
{
  PangoFT2RenderedGlyph *rendered;
  FT_Face face;

  rendered = g_new (PangoFT2RenderedGlyph, 1);

  face = pango_ft2_font_get_face (font);
  
  if (face)
    {
      PangoFT2Font *ft2font = (PangoFT2Font *) font;

      /* Draw glyph */
      FT_Load_Glyph (face, glyph_index, ft2font->load_flags);
      FT_Render_Glyph (face->glyph,
		       (ft2font->load_flags & FT_LOAD_TARGET_MONO ?
			ft_render_mode_mono : ft_render_mode_normal));

      rendered->bitmap = face->glyph->bitmap;
      rendered->bitmap.buffer = g_memdup (face->glyph->bitmap.buffer,
					  face->glyph->bitmap.rows * face->glyph->bitmap.pitch);
      rendered->bitmap_left = face->glyph->bitmap_left;
      rendered->bitmap_top = face->glyph->bitmap_top;
    }
  else
    g_error ("Couldn't get face for PangoFT2Face");

  return rendered;
}

static void
transform_point (PangoMatrix *matrix,
		 int          x,
		 int          y,
		 int         *x_out_pixels,
		 int         *y_out_pixels)
{
  double x_out = (matrix->xx * x + matrix->xy * y) / PANGO_SCALE + matrix->x0;
  double y_out = (matrix->yx * x + matrix->yy * y) / PANGO_SCALE + matrix->y0;

  *x_out_pixels = floor (x_out + 0.5);
  *y_out_pixels = floor (y_out + 0.5);
}

/**
 * pango_ft2_render_transformed:
 * @bitmap:  the FreeType2 bitmap onto which to draw the string
 * @font:    the font in which to draw the string
 * @matrix: a #PangoMatrix, or %NULL to use an identity transformation
 * @glyphs:  the glyph string to draw
 * @x:       the x position of the start of the string (in Pango
 *           units in user space coordinates)
 * @y:       the y position of the baseline (in Pango units
 *           in user space coordinates)
 *
 * Renders a #PangoGlyphString onto a FreeType2 bitmap, possibly
 * transforming the layed-out coordinates through a transformation
 * matrix. Note that the transformation matrix for @font is not
 * changed, so to produce correct rendering results, the @font
 * must have been loaded using a #PangoContext with an identical
 * transformation matrix to that passed in to this function.
 **/
void 
pango_ft2_render_transformed (FT_Bitmap        *bitmap,
			      PangoMatrix      *matrix,
			      PangoFont        *font,
			      PangoGlyphString *glyphs,
			      int               x, 
			      int               y)
{
  FT_UInt glyph_index;
  int i;
  int x_position = 0;
  int ix, iy, ixoff, iyoff, y_start, y_limit, x_start, x_limit;
  PangoGlyphInfo *gi;
  guchar *dest, *src;
  gboolean add_glyph_to_cache;

  g_return_if_fail (bitmap != NULL);
  g_return_if_fail (glyphs != NULL);

  PING (("bitmap: %dx%d@+%d+%d", bitmap->width, bitmap->rows, x, y));

  gi = glyphs->glyphs;
  for (i = 0; i < glyphs->num_glyphs; i++, gi++)
    {
      if (gi->glyph)
	{
	  PangoFT2RenderedGlyph *rendered_glyph;
	  glyph_index = gi->glyph;

	  rendered_glyph = pango_ft2_font_get_cache_glyph_data (font,
								glyph_index);
	  add_glyph_to_cache = FALSE;
	  if (rendered_glyph == NULL)
	    {
	      rendered_glyph = pango_ft2_font_render_glyph (font, glyph_index);
	      add_glyph_to_cache = TRUE;
	    }

	  if (matrix)
	    {
	      transform_point (matrix,
			       x + x_position + gi->geometry.x_offset,
			       y + gi->geometry.y_offset,
			       &ixoff, &iyoff);
	    }
	  else
	    {
	      ixoff = PANGO_PIXELS (x + x_position + gi->geometry.x_offset);
	      iyoff =  PANGO_PIXELS (y + gi->geometry.y_offset);
	    }
	  
	  x_start = MAX (0, - (ixoff + rendered_glyph->bitmap_left));
	  x_limit = MIN (rendered_glyph->bitmap.width,
			 bitmap->width - (ixoff + rendered_glyph->bitmap_left));

	  y_start = MAX (0,  - (iyoff - rendered_glyph->bitmap_top));
	  y_limit = MIN (rendered_glyph->bitmap.rows,
			 bitmap->rows - (iyoff - rendered_glyph->bitmap_top));

	  PING (("glyph %d:%d: bitmap: %dx%d, left:%d top:%d",
		 i, glyph_index,
		 rendered_glyph->bitmap.width, rendered_glyph->bitmap.rows,
		 rendered_glyph->bitmap_left, rendered_glyph->bitmap_top));
	  PING (("xstart:%d xlim:%d ystart:%d ylim:%d",
		 x_start, x_limit, y_start, y_limit));

	  src = rendered_glyph->bitmap.buffer +
	    y_start * rendered_glyph->bitmap.pitch;

	  dest = bitmap->buffer +
	    (y_start + iyoff - rendered_glyph->bitmap_top) * bitmap->pitch +
	    x_start + ixoff + rendered_glyph->bitmap_left;

	  switch (rendered_glyph->bitmap.pixel_mode)
	    {
	    case ft_pixel_mode_grays:
	      src += x_start;
	      for (iy = y_start; iy < y_limit; iy++)
		{
		  guchar *s = src;
		  guchar *d = dest;

		  for (ix = x_start; ix < x_limit; ix++)
		    {
		      switch (*s)
			{
			case 0:
			  break;
			case 0xff:
			  *d = 0xff;
			default:
			  *d = MIN ((gushort) *d + (gushort) *s, 0xff);
			  break;
			}

		      s++;
		      d++;
		    }

		  dest += bitmap->pitch;
		  src  += rendered_glyph->bitmap.pitch;
		}
	      break;

	    case ft_pixel_mode_mono:
	      src += x_start / 8;
	      for (iy = y_start; iy < y_limit; iy++)
		{
		  guchar *s = src;
		  guchar *d = dest;

		  for (ix = x_start; ix < x_limit; ix++)
		    {
		      if ((*s) & (1 << (7 - (ix % 8))))
			*d |= 0xff;
		      
		      if ((ix % 8) == 7)
			s++;
		      d++;
		    }

		  dest += bitmap->pitch;
		  src  += rendered_glyph->bitmap.pitch;
		}
	      break;
	      
	    default:
	      g_warning ("pango_ft2_render: "
			 "Unrecognized glyph bitmap pixel mode %d\n",
			 rendered_glyph->bitmap.pixel_mode);
	      break;
	    }

	  if (add_glyph_to_cache)
	    {
	      pango_ft2_font_set_glyph_cache_destroy (font,
						      (GDestroyNotify) pango_ft2_free_rendered_glyph);
	      pango_ft2_font_set_cache_glyph_data (font,
						   glyph_index, rendered_glyph);
	    }
	}

      x_position += glyphs->glyphs[i].geometry.width;
    }
}

/**
 * pango_ft2_render:
 * @bitmap:  the FreeType2 bitmap onto which to draw the string
 * @font:    the font in which to draw the string
 * @glyphs:  the glyph string to draw
 * @x:       the x position of the start of the string (in pixels)
 * @y:       the y position of the baseline (in pixels)
 *
 * Renders a #PangoGlyphString onto a FreeType2 bitmap.
 **/
void 
pango_ft2_render (FT_Bitmap        *bitmap,
		  PangoFont        *font,
		  PangoGlyphString *glyphs,
		  int               x, 
		  int               y)
{
  pango_ft2_render_transformed (bitmap, NULL, font, glyphs, x * PANGO_SCALE, y * PANGO_SCALE);
}

static FT_Glyph_Metrics *
pango_ft2_get_per_char (PangoFont *font,
			guint32    glyph_index)
{
  PangoFT2Font *ft2font = (PangoFT2Font *)font;
  FT_Face face;
	
  face = pango_ft2_font_get_face (font);
  
  FT_Load_Glyph (face, glyph_index, ft2font->load_flags);
  return &face->glyph->metrics;
}

static PangoFT2GlyphInfo *
pango_ft2_font_get_glyph_info (PangoFont   *font,
			       PangoGlyph   glyph,
			       gboolean     create)
{
  PangoFT2Font *ft2font = (PangoFT2Font *)font;
  PangoFT2GlyphInfo *info;
  FT_Glyph_Metrics *gm;

  info = g_hash_table_lookup (ft2font->glyph_info, GUINT_TO_POINTER (glyph));

  if ((info == NULL) && create)
    {
      FT_Face face = pango_ft2_font_get_face (font);
      info = g_new0 (PangoFT2GlyphInfo, 1);
      
      if (glyph && (gm = pango_ft2_get_per_char (font, glyph)))
	{
	  info->ink_rect.x = PANGO_UNITS_26_6 (gm->horiBearingX);
	  info->ink_rect.width = PANGO_UNITS_26_6 (gm->width);
	  info->ink_rect.y = -PANGO_UNITS_26_6 (gm->horiBearingY);
	  info->ink_rect.height = PANGO_UNITS_26_6 (gm->height);
	      
	  info->logical_rect.x = 0;
	  info->logical_rect.width = PANGO_UNITS_26_6 (gm->horiAdvance);
	  if (ft2font->load_flags & FT_LOAD_NO_HINTING)
	    {
	      FT_Fixed ascender, descender;

	      ascender = FT_MulFix (face->ascender, face->size->metrics.y_scale);
	      descender = FT_MulFix (face->descender, face->size->metrics.y_scale);

	      info->logical_rect.y = - PANGO_UNITS_26_6 (ascender);
	      info->logical_rect.height = PANGO_UNITS_26_6 (ascender - descender);
	    }
	  else
	    {
	      info->logical_rect.y = - PANGO_UNITS_26_6 (face->size->metrics.ascender);
	      info->logical_rect.height = PANGO_UNITS_26_6 (face->size->metrics.ascender - face->size->metrics.descender);
	    }
	}
      else
	{
	  info->ink_rect.x = 0;
	  info->ink_rect.width = 0;
	  info->ink_rect.y = 0;
	  info->ink_rect.height = 0;

	  info->logical_rect.x = 0;
	  info->logical_rect.width = 0;
	  info->logical_rect.y = 0;
	  info->logical_rect.height = 0;
	}

      g_hash_table_insert (ft2font->glyph_info, GUINT_TO_POINTER(glyph), info);
    }

  return info;
}
     
static void
pango_ft2_font_get_glyph_extents (PangoFont      *font,
				  PangoGlyph      glyph,
				  PangoRectangle *ink_rect,
				  PangoRectangle *logical_rect)
{
  PangoFT2GlyphInfo *info;

  info = pango_ft2_font_get_glyph_info (font, glyph, TRUE);

  if (ink_rect)
    *ink_rect = info->ink_rect;
  if (logical_rect)
    *logical_rect = info->logical_rect;
}

/**
 * pango_ft2_font_get_kerning:
 * @font: a #PangoFont
 * @left: the left #PangoGlyph
 * @right: the right #PangoGlyph
 * 
 * Retrieves kerning information for a combination of two glyphs.
 *
 * Use pango_fc_font_kern_glyphs() instead.
 * 
 * Return value: The amount of kerning (in Pango units) to apply for 
 * the given combination of glyphs.
 **/
int
pango_ft2_font_get_kerning (PangoFont *font,
			    PangoGlyph left,
			    PangoGlyph right)
{
  PangoFcFont *fc_font = PANGO_FC_FONT (font);
  
  FT_Face face;
  FT_Error error;
  FT_Vector kerning;

  face = pango_fc_font_lock_face (fc_font);
  if (!face)
    return 0;

  if (!FT_HAS_KERNING (face))
    {
      pango_fc_font_unlock_face (fc_font);
      return 0;
    }

  error = FT_Get_Kerning (face, left, right, ft_kerning_default, &kerning);
  if (error != FT_Err_Ok)
    {
      pango_fc_font_unlock_face (fc_font);
      return 0;
    }

  pango_fc_font_unlock_face (fc_font);
  return PANGO_UNITS_26_6 (kerning.x);
}

static FT_Face
pango_ft2_font_real_lock_face (PangoFcFont *font)
{
  return pango_ft2_font_get_face ((PangoFont *)font);
}

static void
pango_ft2_font_real_unlock_face (PangoFcFont *font)
{
}

static gboolean
pango_ft2_font_real_has_char (PangoFcFont *font,
			      gunichar     wc)
{
  FcCharSet *charset;

  if (FcPatternGetCharSet (font->font_pattern,
                           FC_CHARSET, 0, &charset) != FcResultMatch)
    return FALSE;

  return FcCharSetHasChar (charset, wc);
}

static guint
pango_ft2_font_real_get_glyph (PangoFcFont *font,
			       gunichar     wc)
{
  FT_Face face;
  FT_UInt index;

  face = pango_ft2_font_get_face ((PangoFont *)font);
  index = FcFreeTypeCharIndex (face, wc);
  if (index && index <= face->num_glyphs)
    return index;

  return 0;
}

static PangoGlyph
pango_ft2_font_real_get_unknown_glyph (PangoFcFont *font,
				       gunichar     wc)
{
  return 0;
}

static gboolean
pango_ft2_free_glyph_info_callback (gpointer key, gpointer value, gpointer data)
{
  PangoFT2Font *font = PANGO_FT2_FONT (data);
  PangoFT2GlyphInfo *info = value;
  
  if (font->glyph_cache_destroy && info->cached_glyph)
    (*font->glyph_cache_destroy) (info->cached_glyph);

  g_free (value);
  return TRUE;
}

static void
pango_ft2_font_finalize (GObject *object)
{
  PangoFT2Font *ft2font = (PangoFT2Font *)object;

  if (ft2font->face)
    {
      FT_Done_Face (ft2font->face);
      ft2font->face = NULL;
    }

  g_hash_table_foreach_remove (ft2font->glyph_info,
			       pango_ft2_free_glyph_info_callback, object);
  g_hash_table_destroy (ft2font->glyph_info);
  
  G_OBJECT_CLASS (parent_class)->finalize (object);
}

/**
 * pango_ft2_font_get_coverage:
 * @font: a #PangoFT2Font.
 * @language: a language tag.
 * @returns: a #PangoCoverage.
 * 
 * Gets the #PangoCoverage for a #PangoFT2Font. Use pango_font_get_coverage() instead.
 **/
PangoCoverage *
pango_ft2_font_get_coverage (PangoFont     *font,
			     PangoLanguage *language)
{
  return pango_font_get_coverage (font, language);
}

/* Utility functions */

/**
 * pango_ft2_get_unknown_glyph:
 * @font: a #PangoFont
 * 
 * Return the index of a glyph suitable for drawing unknown characters.
 * 
 * Use pango_fc_font_get_unknown_glyph() instead.
 * 
 * Return value: a glyph index into @font
 **/
PangoGlyph
pango_ft2_get_unknown_glyph (PangoFont *font)
{
  return 0;
}

static void
draw_underline (FT_Bitmap         *bitmap,
		PangoMatrix       *matrix,
		PangoFontMetrics  *metrics,
		PangoUnderline     uline,
		int                x,
		int                width,
		int                base_y,
		int                descent)
{
  int underline_thickness = pango_font_metrics_get_underline_thickness (metrics);
  int underline_position = pango_font_metrics_get_underline_position (metrics);
  int y_off = 0;		/* Quiet GCC */

  switch (uline)
    {
    case PANGO_UNDERLINE_NONE:
      g_assert_not_reached();
      break;
    case PANGO_UNDERLINE_SINGLE:
      y_off = - underline_position;
      break;
    case PANGO_UNDERLINE_DOUBLE:
      y_off = - underline_position;
      break;
    case PANGO_UNDERLINE_LOW:
      y_off = underline_thickness + descent;
      break;
    case PANGO_UNDERLINE_ERROR:
      {
	_pango_ft2_draw_error_underline (bitmap, matrix,
					 x,
					 base_y - underline_position,
					 width,
					 3 * underline_thickness);
	return;
      }
    }

  _pango_ft2_draw_rect (bitmap, matrix,
			x,
			base_y + y_off,
			width,
			underline_thickness);

  if (uline == PANGO_UNDERLINE_DOUBLE)
    {
      y_off += 2 * underline_thickness;
      
      _pango_ft2_draw_rect (bitmap, matrix,
			    x,
			    base_y + y_off,
			    width,
			    underline_thickness);
    }
}

static void
draw_strikethrough (FT_Bitmap        *bitmap,
		    PangoMatrix      *matrix,
		    PangoFontMetrics *metrics,
		    int               x,
		    int               width,
		    int               base_y)
{
  int strikethrough_thickness = pango_font_metrics_get_strikethrough_thickness (metrics);
  int strikethrough_position = pango_font_metrics_get_strikethrough_position (metrics);

  _pango_ft2_draw_rect (bitmap, matrix,
			x,
			base_y - strikethrough_position,
			width,
			strikethrough_thickness);
}

/**
 * pango_ft2_render_layout_line_subpixel:
 * @bitmap:    a FT_Bitmap to render the line onto
 * @line:      a #PangoLayoutLine
 * @x:         the x position of start of string (in pango units)
 * @y:         the y position of baseline (in pango units)
 *
 * Render a #PangoLayoutLine onto a FreeType2 bitmap, with he
 * location specified in fixed-point pango units rather than
 * pixels. (Using this will avoid extra inaccuracies from
 * rounding to integer pixels multiple times, even if the
 * final glyph positions are integers.)
 */
void 
pango_ft2_render_layout_line_subpixel (FT_Bitmap       *bitmap,
				       PangoLayoutLine *line,
				       int              x, 
				       int              y)
{
  GSList *tmp_list = line->runs;
  PangoRectangle logical_rect;
  PangoRectangle ink_rect;
  int x_off = 0;
  PangoMatrix *matrix;

  matrix = pango_context_get_matrix (pango_layout_get_context (line->layout));

  while (tmp_list)
    {
      PangoUnderline uline = PANGO_UNDERLINE_NONE;
      gboolean strike, shape_set;
      gint rise;
      PangoLayoutRun *run = tmp_list->data;

      tmp_list = tmp_list->next;

      pango_ft2_get_item_properties (run->item,
				     &uline, &strike, &rise,
				     &shape_set, &ink_rect, &logical_rect);

      if (!shape_set)
	{
	  if (uline == PANGO_UNDERLINE_NONE && !strike)
	    pango_glyph_string_extents (run->glyphs, run->item->analysis.font,
					NULL, &logical_rect);
	  else
	    pango_glyph_string_extents (run->glyphs, run->item->analysis.font,
					&ink_rect, &logical_rect);
	  
	  pango_ft2_render_transformed (bitmap, matrix,
					run->item->analysis.font, run->glyphs,
					x + x_off, y - rise);

	  if (uline != PANGO_UNDERLINE_NONE || strike)
	    {
	      PangoFontMetrics *metrics = pango_font_get_metrics (run->item->analysis.font,
								  run->item->analysis.language);
	      
	      if (uline != PANGO_UNDERLINE_NONE)
		draw_underline (bitmap, matrix, metrics,
				uline,
				x_off + ink_rect.x,
				ink_rect.width,
				y - rise,
				ink_rect.y + ink_rect.height);
	      
	      if (strike)
		draw_strikethrough (bitmap, matrix, metrics,
				    x_off + ink_rect.x,
				    ink_rect.width,
				    y - rise);

	      pango_font_metrics_unref (metrics);
	    }
	}

      x_off += logical_rect.width;
    }
}


/**
 * pango_ft2_render_layout_line:
 * @bitmap:    a FT_Bitmap to render the line onto
 * @line:      a #PangoLayoutLine
 * @x:         the x position of start of string (in pixels)
 * @y:         the y position of baseline (in pixels)
 *
 * Render a #PangoLayoutLine onto a FreeType2 bitmap
 */
void 
pango_ft2_render_layout_line (FT_Bitmap       *bitmap,
			      PangoLayoutLine *line,
			      int              x, 
			      int              y)
{
  pango_ft2_render_layout_line_subpixel (bitmap, line, x * PANGO_SCALE, y * PANGO_SCALE);
}


/**
 * pango_ft2_render_layout_subpixel:
 * @bitmap:    a FT_Bitmap to render the layout onto
 * @layout:    a #PangoLayout
 * @x:         the X position of the left of the layout (in Pango units)
 * @y:         the Y position of the top of the layout (in Pango units)
 *
 * Render a #PangoLayout onto a FreeType2 bitmap, with he
 * location specified in fixed-point pango units rather than
 * pixels. (Using this will avoid extra inaccuracies from
 * rounding to integer pixels multiple times, even if the
 * final glyph positions are integers.)
 */
void 
pango_ft2_render_layout_subpixel (FT_Bitmap   *bitmap,
				  PangoLayout *layout,
				  int          x, 
				  int          y)
{
  PangoLayoutIter *iter;

  g_return_if_fail (bitmap != NULL);
  g_return_if_fail (PANGO_IS_LAYOUT (layout));

  iter = pango_layout_get_iter (layout);

  do
    {
      PangoRectangle   logical_rect;
      PangoLayoutLine *line;
      int              baseline;
      
      line = pango_layout_iter_get_line (iter);
      
      pango_layout_iter_get_line_extents (iter, NULL, &logical_rect);
      baseline = pango_layout_iter_get_baseline (iter);

      pango_ft2_render_layout_line_subpixel (bitmap,
					     line,
					     x + logical_rect.x,
					     y + baseline);
    }
  while (pango_layout_iter_next_line (iter));

  pango_layout_iter_free (iter);
}

/**
 * pango_ft2_render_layout:
 * @bitmap:    a FT_Bitmap to render the layout onto
 * @layout:    a #PangoLayout
 * @x:         the X position of the left of the layout (in pixels)
 * @y:         the Y position of the top of the layout (in pixels)
 *
 * Render a #PangoLayout onto a FreeType2 bitmap
 */
void 
pango_ft2_render_layout (FT_Bitmap   *bitmap,
			 PangoLayout *layout,
			 int          x, 
			 int          y)
{
  pango_ft2_render_layout_subpixel (bitmap, layout, x * PANGO_SCALE, y * PANGO_SCALE);
}

/* This utility function is duplicated here and in pango-layout.c; should it be
 * public? Trouble is - what is the appropriate set of properties?
 */
static void
pango_ft2_get_item_properties (PangoItem      *item,
			       PangoUnderline *uline,
			       gboolean       *strikethrough,
                               gint           *rise,
			       gboolean       *shape_set,
			       PangoRectangle *ink_rect,
			       PangoRectangle *logical_rect)
{
  GSList *tmp_list = item->analysis.extra_attrs;

  if (strikethrough)
    *strikethrough = FALSE;

  if (rise)
    *rise = 0;

  if (shape_set)
    *shape_set = FALSE;

  while (tmp_list)
    {
      PangoAttribute *attr = tmp_list->data;

      switch (attr->klass->type)
	{
	case PANGO_ATTR_UNDERLINE:
	  if (uline)
	    *uline = ((PangoAttrInt *)attr)->value;
	  break;
	  
	case PANGO_ATTR_STRIKETHROUGH:
	  if (strikethrough)
	    *strikethrough = ((PangoAttrInt *)attr)->value;
	  break;

	case PANGO_ATTR_SHAPE:
	  if (shape_set)
	    *shape_set = TRUE;
	  if (logical_rect)
	    *logical_rect = ((PangoAttrShape *)attr)->logical_rect;
	  if (ink_rect)
	    *ink_rect = ((PangoAttrShape *)attr)->ink_rect;
	  break;

        case PANGO_ATTR_RISE:
          if (rise)
            *rise = ((PangoAttrInt *)attr)->value;
          break;

	default:
	  break;
	}

      tmp_list = tmp_list->next;
    }
}

typedef struct
{
  FT_Error     code;
  const char*  msg;
} ft_error_description;

static int
ft_error_compare (const void *pkey,
		  const void *pbase)
{
  return ((ft_error_description *) pkey)->code - ((ft_error_description *) pbase)->code;
}

const char *
_pango_ft2_ft_strerror (FT_Error error)
{
#undef __FTERRORS_H__
#define FT_ERRORDEF( e, v, s )  { e, s },
#define FT_ERROR_START_LIST  {
#define FT_ERROR_END_LIST    { 0, 0 } };

  const ft_error_description ft_errors[] =
#include FT_ERRORS_H

#undef FT_ERRORDEF
#undef FT_ERROR_START_LIST
#undef FT_ERROR_END_LIST

  ft_error_description *found =
    bsearch (&error, ft_errors, G_N_ELEMENTS (ft_errors) - 1,
	     sizeof (ft_errors[0]), ft_error_compare);
  if (found != NULL)
    return found->msg;
  else
    {
      static char default_msg[100];

      g_sprintf (default_msg, "Unknown FreeType2 error %#x", error);
      return default_msg;
    }
}


void *
pango_ft2_font_get_cache_glyph_data (PangoFont *font,
				     int        glyph_index)
{
  PangoFT2GlyphInfo *info;

  g_return_val_if_fail (PANGO_FT2_IS_FONT (font), NULL);
  
  info = pango_ft2_font_get_glyph_info (font, glyph_index, FALSE);

  if (info == NULL)
    return NULL;
  
  return info->cached_glyph;
}

void
pango_ft2_font_set_cache_glyph_data (PangoFont     *font,
				     int            glyph_index,
				     void          *cached_glyph)
{
  PangoFT2GlyphInfo *info;

  g_return_if_fail (PANGO_FT2_IS_FONT (font));
  
  info = pango_ft2_font_get_glyph_info (font, glyph_index, TRUE);

  info->cached_glyph = cached_glyph;

  /* TODO: Implement limiting of the number of cached glyphs */
}

void
pango_ft2_font_set_glyph_cache_destroy (PangoFont      *font,
					GDestroyNotify  destroy_notify)
{
  g_return_if_fail (PANGO_FT2_IS_FONT (font));
  
  PANGO_FT2_FONT (font)->glyph_cache_destroy = destroy_notify;
}
