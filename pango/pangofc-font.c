/* Pango
 * pangofc-font.c: Shared interfaces for fontconfig-based backends
 *
 * Copyright (C) 2003 Red Hat Software
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

#include "pangofc-font.h"
#include "pangofc-fontmap.h"
#include "pangofc-private.h"
#include "pango-layout.h"
#include "pango-modules.h"
#include "pango-utils.h"

#include <fontconfig/fcfreetype.h>

#include FT_TRUETYPE_TABLES_H

typedef struct _PangoFcMetricsInfo  PangoFcMetricsInfo;

struct _PangoFcMetricsInfo
{
  const char       *sample_str;
  PangoFontMetrics *metrics;
};

enum {
  PROP_0,
  PROP_PATTERN
};

typedef struct _PangoFcFontPrivate PangoFcFontPrivate;

#define PANGO_FC_FONT_GET_PRIVATE(obj) (G_TYPE_INSTANCE_GET_PRIVATE ((obj), PANGO_TYPE_FC_FONT, PangoFcFontPrivate))

struct _PangoFcFontPrivate
{
  PangoFcDecoder *decoder;
};

static gboolean pango_fc_font_real_has_char  (PangoFcFont *font,
					      gunichar     wc);
static guint    pango_fc_font_real_get_glyph (PangoFcFont *font,
					      gunichar     wc);

static void                  pango_fc_font_finalize     (GObject          *object);
static void                  pango_fc_font_set_property (GObject          *object,
							 guint             prop_id,
							 const GValue     *value,
							 GParamSpec       *pspec);
static PangoEngineShape *    pango_fc_font_find_shaper  (PangoFont        *font,
							 PangoLanguage    *language,
							 guint32           ch);
static PangoCoverage *       pango_fc_font_get_coverage (PangoFont        *font,
							 PangoLanguage    *language);
static PangoFontMetrics *    pango_fc_font_get_metrics  (PangoFont        *font,
							 PangoLanguage    *language);
static PangoFontDescription *pango_fc_font_describe     (PangoFont        *font);

G_DEFINE_TYPE (PangoFcFont, pango_fc_font, PANGO_TYPE_FONT)

static void
pango_fc_font_class_init (PangoFcFontClass *class)
{
  GObjectClass *object_class = G_OBJECT_CLASS (class);
  PangoFontClass *font_class = PANGO_FONT_CLASS (class);

  class->has_char = pango_fc_font_real_has_char;
  class->get_glyph = pango_fc_font_real_get_glyph;
  
  object_class->finalize = pango_fc_font_finalize;
  object_class->set_property = pango_fc_font_set_property;
  font_class->describe = pango_fc_font_describe;
  font_class->find_shaper = pango_fc_font_find_shaper;
  font_class->get_coverage = pango_fc_font_get_coverage;
  font_class->get_metrics = pango_fc_font_get_metrics;
  
  g_object_class_install_property (object_class, PROP_PATTERN,
				   g_param_spec_pointer ("pattern",
							 "Pattern",
							 "The fontconfig pattern for this font",
							 G_PARAM_WRITABLE | G_PARAM_CONSTRUCT_ONLY));

  g_type_class_add_private (object_class, sizeof (PangoFcFontPrivate));
}

static void
pango_fc_font_init (PangoFcFont *fcfont)
{
}

static void
free_metrics_info (PangoFcMetricsInfo *info)
{
  pango_font_metrics_unref (info->metrics);
  g_free (info);
}

static void
pango_fc_font_finalize (GObject *object)
{
  PangoFcFont *fcfont = PANGO_FC_FONT (object);
  PangoFcFontPrivate *priv = PANGO_FC_FONT_GET_PRIVATE (fcfont);

  g_slist_foreach (fcfont->metrics_by_lang, (GFunc)free_metrics_info, NULL);
  g_slist_free (fcfont->metrics_by_lang);  

  if (fcfont->fontmap)
    _pango_fc_font_map_remove (PANGO_FC_FONT_MAP (fcfont->fontmap), fcfont);

  FcPatternDestroy (fcfont->font_pattern);
  pango_font_description_free (fcfont->description);

  if (priv->decoder)
    _pango_fc_font_set_decoder (fcfont, NULL);

  G_OBJECT_CLASS (pango_fc_font_parent_class)->finalize (object);
}

static gboolean
pattern_is_hinted (FcPattern *pattern)
{
  FcBool hinting;

  if (FcPatternGetBool (pattern,
			FC_HINTING, 0, &hinting) != FcResultMatch)
    hinting = FcTrue;
  
  return hinting;
}

static gboolean
pattern_is_transformed (FcPattern *pattern)
{
  FcMatrix *fc_matrix;

  if (FcPatternGetMatrix (pattern, FC_MATRIX, 0, &fc_matrix) == FcResultMatch)
    {
      FT_Matrix ft_matrix;
      
      ft_matrix.xx = 0x10000L * fc_matrix->xx;
      ft_matrix.yy = 0x10000L * fc_matrix->yy;
      ft_matrix.xy = 0x10000L * fc_matrix->xy;
      ft_matrix.yx = 0x10000L * fc_matrix->yx;

      return ((ft_matrix.xy | ft_matrix.yx) != 0 ||
	      ft_matrix.xx != 0x10000L ||
	      ft_matrix.yy != 0x10000L);
    }
  else
    return FALSE;
}

static void
pango_fc_font_set_property (GObject       *object,
			    guint          prop_id,
			    const GValue  *value,
			    GParamSpec    *pspec)
{
  switch (prop_id)
    {
    case PROP_PATTERN:
      {
	PangoFcFont *fcfont = PANGO_FC_FONT (object);
	FcPattern *pattern = g_value_get_pointer (value);
  
	g_return_if_fail (pattern != NULL);
	g_return_if_fail (fcfont->font_pattern == NULL);

	FcPatternReference (pattern);
	fcfont->font_pattern = pattern;
	fcfont->description = pango_fc_font_description_from_pattern (pattern, TRUE);
	fcfont->is_hinted = pattern_is_hinted (pattern);
	fcfont->is_transformed = pattern_is_transformed (pattern);
      }
      break;
    default:
      G_OBJECT_WARN_INVALID_PROPERTY_ID (object, prop_id, pspec);
      break;    
    }
}

static PangoFontDescription *
pango_fc_font_describe (PangoFont *font)
{
  PangoFcFont *fcfont = (PangoFcFont *)font;

  return pango_font_description_copy (fcfont->description);
}

static PangoMap *
pango_fc_get_shaper_map (PangoLanguage *language)
{
  static guint engine_type_id = 0;
  static guint render_type_id = 0;
  
  if (engine_type_id == 0)
    {
      engine_type_id = g_quark_from_static_string (PANGO_ENGINE_TYPE_SHAPE);
      render_type_id = g_quark_from_static_string (PANGO_RENDER_TYPE_FC);
    }
  
  return pango_find_map (language, engine_type_id, render_type_id);
}

static PangoEngineShape *
pango_fc_font_find_shaper (PangoFont     *font,
			   PangoLanguage *language,
			   guint32        ch)
{
  PangoMap *shaper_map = NULL;

  shaper_map = pango_fc_get_shaper_map (language);
  return (PangoEngineShape *)pango_map_get_engine (shaper_map, ch);
}

static PangoCoverage *
pango_fc_font_get_coverage (PangoFont     *font,
			    PangoLanguage *language)
{
  PangoFcFont *fcfont = (PangoFcFont *)font;
  PangoFcFontPrivate *priv = PANGO_FC_FONT_GET_PRIVATE (fcfont);
  FcCharSet *charset;

  if (priv->decoder)
    {
      charset = pango_fc_decoder_get_charset (priv->decoder, fcfont);
      return _pango_fc_font_map_fc_to_coverage (charset);
    }

  return _pango_fc_font_map_get_coverage (PANGO_FC_FONT_MAP (fcfont->fontmap),
					  fcfont);
}

static void
quantize_position (int *thickness,
		   int *position)
{
  int thickness_pixels = (*thickness + PANGO_SCALE / 2) / PANGO_SCALE;
  if (thickness_pixels == 0)
    thickness_pixels = 1;
  
  if (thickness_pixels & 1)
    {
      int new_center = ((*position - *thickness / 2) & ~(PANGO_SCALE - 1)) + PANGO_SCALE / 2;
      *position = new_center + (PANGO_SCALE * thickness_pixels) / 2;
    }
  else
    {
      int new_center = ((*position - *thickness / 2 + PANGO_SCALE / 2) & ~(PANGO_SCALE - 1));
      *position = new_center + (PANGO_SCALE * thickness_pixels) / 2;
    }

  *thickness = thickness_pixels * PANGO_SCALE;
}

/* For Xft, it would be slightly more efficient to simply to
 * call Xft, and also more robust against changes in Xft.
 * But for now, we simply use the same code for all backends.
 *
 * The code in this function is partly based on code from Xft,
 * Copyright 2000 Keith Packard
 */
static void
get_face_metrics (PangoFcFont      *fcfont,
		  PangoFontMetrics *metrics)
{
  FT_Face face = pango_fc_font_lock_face (fcfont);
  FcMatrix *fc_matrix;
  FT_Matrix ft_matrix;
  TT_OS2 *os2;
  gboolean have_transform = FALSE;

  if  (FcPatternGetMatrix (fcfont->font_pattern,
			   FC_MATRIX, 0, &fc_matrix) == FcResultMatch)
    {
      ft_matrix.xx = 0x10000L * fc_matrix->xx;
      ft_matrix.yy = 0x10000L * fc_matrix->yy;
      ft_matrix.xy = 0x10000L * fc_matrix->xy;
      ft_matrix.yx = 0x10000L * fc_matrix->yx;
      
      have_transform = (ft_matrix.xx != 0x10000 || ft_matrix.xy != 0 ||
			ft_matrix.yx != 0 || ft_matrix.yy != 0x10000);
    }

  if (have_transform)
    {
      FT_Vector	vector;
      
      vector.x = 0;
      vector.y = face->size->metrics.descender;
      FT_Vector_Transform (&vector, &ft_matrix);
      metrics->descent = - PANGO_UNITS_26_6 (vector.y);
      
      vector.x = 0;
      vector.y = face->size->metrics.ascender;
      FT_Vector_Transform (&vector, &ft_matrix);
      metrics->ascent = PANGO_UNITS_26_6 (vector.y);
    }
  else if (fcfont->is_hinted ||
	   (face->face_flags & FT_FACE_FLAG_SCALABLE) == 0)
    {
      metrics->descent = - PANGO_UNITS_26_6 (face->size->metrics.descender);
      metrics->ascent = PANGO_UNITS_26_6 (face->size->metrics.ascender);
    }
  else
    {
      FT_Fixed ascender, descender;

      descender = FT_MulFix (face->descender, face->size->metrics.y_scale);
      metrics->descent = - PANGO_UNITS_26_6 (descender);

      ascender = FT_MulFix (face->ascender, face->size->metrics.y_scale);
      metrics->ascent = PANGO_UNITS_26_6 (ascender);
    }

  /* Versions of FreeType < 2.1.8 get underline thickness wrong
   * for Postscript fonts (always zero), so we need a fallback
   */
  if (face->underline_thickness == 0)
    {
      metrics->underline_thickness = (PANGO_SCALE * face->size->metrics.y_ppem) / 14;
      metrics->underline_position = - metrics->underline_thickness;
    }
  else
    {
      FT_Fixed ft_thickness, ft_position;
      
      ft_thickness = FT_MulFix (face->underline_thickness, face->size->metrics.y_scale);
      metrics->underline_thickness = PANGO_UNITS_26_6 (ft_thickness);

      ft_position = FT_MulFix (face->underline_position, face->size->metrics.y_scale);
      metrics->underline_position = PANGO_UNITS_26_6 (ft_position);
    }

  os2 = FT_Get_Sfnt_Table (face, ft_sfnt_os2);
  if (os2 && os2->version != 0xFFFF && os2->yStrikeoutSize != 0)
    {
      FT_Fixed ft_thickness, ft_position;
      
      ft_thickness = FT_MulFix (os2->yStrikeoutSize, face->size->metrics.y_scale);
      metrics->strikethrough_thickness = PANGO_UNITS_26_6 (ft_thickness);

      ft_position = FT_MulFix (os2->yStrikeoutPosition, face->size->metrics.y_scale);
      metrics->strikethrough_position = PANGO_UNITS_26_6 (ft_position);
    }
  else
    {
      metrics->strikethrough_thickness = metrics->underline_thickness;
      metrics->strikethrough_position = (PANGO_SCALE * face->size->metrics.y_ppem) / 4;
    }

  /* If hinting is on for this font, quantize the underline and strikethrough position
   * to integer values.
   */
  if (fcfont->is_hinted)
    {
      quantize_position (&metrics->underline_thickness, &metrics->underline_position);
      quantize_position (&metrics->strikethrough_thickness, &metrics->strikethrough_position);
    }
  
  pango_fc_font_unlock_face (fcfont);
}

static int
max_glyph_width (PangoLayout *layout)
{
  int max_width = 0;
  GSList *l, *r;

  for (l = pango_layout_get_lines (layout); l; l = l->next)
    {
      PangoLayoutLine *line = l->data;

      for (r = line->runs; r; r = r->next)
        {
          PangoGlyphString *glyphs = ((PangoGlyphItem *)r->data)->glyphs;
          int i;

          for (i = 0; i < glyphs->num_glyphs; i++)
            if (glyphs->glyphs[i].geometry.width > max_width)
              max_width = glyphs->glyphs[i].geometry.width;
        }
    }

  return max_width;
}

static PangoFontMetrics *
pango_fc_font_get_metrics (PangoFont     *font,
                           PangoLanguage *language)
{
  PangoFcFont *fcfont = PANGO_FC_FONT (font);
  PangoFcMetricsInfo *info = NULL; /* Quiet gcc */
  GSList *tmp_list;      

  const char *sample_str = pango_language_get_sample_string (language);
  
  tmp_list = fcfont->metrics_by_lang;
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

      info = g_new0 (PangoFcMetricsInfo, 1);
      fcfont->metrics_by_lang = g_slist_prepend (fcfont->metrics_by_lang, 
						 info);

      if (fcfont->fontmap)
	{
	  info->sample_str = sample_str;
	  info->metrics = pango_font_metrics_new ();
	  
	  get_face_metrics (fcfont, info->metrics);

	  context = pango_fc_font_map_create_context (PANGO_FC_FONT_MAP (fcfont->fontmap));
	  pango_context_set_language (context, language);
	  
	  layout = pango_layout_new (context);
	  pango_layout_set_font_description (layout, fcfont->description);

	  pango_layout_set_text (layout, sample_str, -1);      
	  pango_layout_get_extents (layout, NULL, &extents);
      
	  info->metrics->approximate_char_width = 
	    extents.width / g_utf8_strlen (sample_str, -1);

          pango_layout_set_text (layout, "0123456789", -1);
          info->metrics->approximate_digit_width = max_glyph_width (layout);

	  g_object_unref (layout);
	  g_object_unref (context);
	}
    }

  return pango_font_metrics_ref (info->metrics);
}

static gboolean
pango_fc_font_real_has_char (PangoFcFont *font,
			     gunichar     wc)
{
  FcCharSet *charset;

  if (FcPatternGetCharSet (font->font_pattern,
                           FC_CHARSET, 0, &charset) != FcResultMatch)
    return FALSE;

  return FcCharSetHasChar (charset, wc);
}

static guint
pango_fc_font_real_get_glyph (PangoFcFont *font,
			      gunichar     wc)
{
  FT_Face face;
  FT_UInt index;

  face = pango_fc_font_lock_face (font);
  
  index = FcFreeTypeCharIndex (face, wc);
  if (index > face->num_glyphs)
    index = 0;

  pango_fc_font_unlock_face (font);

  return index;
}

/**
 * pango_fc_font_lock_face:
 * @font: a #PangoFcFont.
 *
 * Gets the FreeType FT_Face associated with a font,
 * This face will be kept around until you call
 * pango_fc_font_unlock_face().
 *
 * Returns: the FreeType FT_Face associated with @font.
 *
 * Since: 1.4
 **/
FT_Face
pango_fc_font_lock_face (PangoFcFont *font)
{
  g_return_val_if_fail (PANGO_IS_FC_FONT (font), NULL);

  return PANGO_FC_FONT_GET_CLASS (font)->lock_face (font);
}

/**
 * pango_fc_font_unlock_face:
 * @font: a #PangoFcFont.
 *
 * Releases a font previously obtained with
 * pango_fc_font_lock_face().
 *
 * Since: 1.4
 **/
void
pango_fc_font_unlock_face (PangoFcFont *font)
{
  g_return_if_fail (PANGO_IS_FC_FONT (font));
  
  PANGO_FC_FONT_GET_CLASS (font)->unlock_face (font);
}

/**
 * pango_fc_font_has_char:
 * @font: a #PangoFcFont
 * @wc: Unicode codepoint to look up
 * 
 * Determines whether @font has a glyph for the codepoint @wc.
 * 
 * Return value: %TRUE if @font has the requested codepoint.
 *
 * Since: 1.4
 **/
gboolean
pango_fc_font_has_char (PangoFcFont *font,
			gunichar     wc)
{
  PangoFcFontPrivate *priv = PANGO_FC_FONT_GET_PRIVATE (font);
  FcCharSet *charset;

  g_return_val_if_fail (PANGO_IS_FC_FONT (font), FALSE);

  if (priv->decoder)
    {
      charset = pango_fc_decoder_get_charset (priv->decoder, font);
      return FcCharSetHasChar (charset, wc);
    }

  return PANGO_FC_FONT_GET_CLASS (font)->has_char (font, wc);
}

/**
 * pango_fc_font_get_glyph:
 * @font: a #PangoFcFont
 * @wc: Unicode codepoint to look up
 * 
 * Gets the glyph index for a given unicode codepoint
 * for @font. If you only want to determine
 * whether the font has the glyph, use pango_fc_font_has_char().
 * 
 * Return value: the glyph index, or 0, if the unicode
 *  codepoint doesn't exist in the font.
 *
 * Since: 1.4
 **/
PangoGlyph
pango_fc_font_get_glyph (PangoFcFont *font,
			 gunichar     wc)
{
  PangoFcFontPrivate *priv = PANGO_FC_FONT_GET_PRIVATE (font);

  g_return_val_if_fail (PANGO_IS_FC_FONT (font), 0);

  /* Replace NBSP with a normal space; it should be invariant that
   * they shape the same other than breaking properties.
   */
  if (wc == 0xA0)
	  wc = 0x20;

  if (priv->decoder)
    return pango_fc_decoder_get_glyph (priv->decoder, font, wc);

  return PANGO_FC_FONT_GET_CLASS (font)->get_glyph (font, wc);
}


/**
 * pango_fc_font_get_unknown_glyph:
 * @font: a #PangoFcFont
 * @wc: the Unicode character for which a glyph is needed.
 *
 * Returns the index of a glyph suitable for drawing @wc as an
 * unknown character.
 *
 * Return value: a glyph index into @font.
 *
 * Since: 1.4
 **/
PangoGlyph
pango_fc_font_get_unknown_glyph (PangoFcFont *font,
				 gunichar     wc)
{
  g_return_val_if_fail (PANGO_IS_FC_FONT (font), 0);

  return PANGO_FC_FONT_GET_CLASS (font)->get_unknown_glyph (font, wc);
}

void
_pango_fc_font_shutdown (PangoFcFont *font)
{
  g_return_if_fail (PANGO_IS_FC_FONT (font));

  if (PANGO_FC_FONT_GET_CLASS (font)->shutdown)
    PANGO_FC_FONT_GET_CLASS (font)->shutdown (font);
}

/**
 * pango_fc_font_kern_glyphs
 * @font: a #PangoFcFont
 * @glyphs: a #PangoGlyphString
 * 
 * Adjust each adjacent pair of glyphs in @glyphs according to
 * kerning information in @font.
 * 
 * Since: 1.4
 **/
void
pango_fc_font_kern_glyphs (PangoFcFont      *font,
			   PangoGlyphString *glyphs)
{
  FT_Face face;
  FT_Error error;
  FT_Vector kerning;
  int i;

  g_return_if_fail (PANGO_IS_FC_FONT (font));
  g_return_if_fail (glyphs != NULL);

  face = pango_fc_font_lock_face (font);
  if (!face)
    return;

  if (!FT_HAS_KERNING (face))
    {
      pango_fc_font_unlock_face (font);
      return;
    }
  
  for (i = 1; i < glyphs->num_glyphs; ++i)
    {
      error = FT_Get_Kerning (face,
			      glyphs->glyphs[i-1].glyph,
			      glyphs->glyphs[i].glyph,
			      ft_kerning_default,
			      &kerning);

      if (error == FT_Err_Ok)
	glyphs->glyphs[i-1].geometry.width += PANGO_UNITS_26_6 (kerning.x);
    }
  
  pango_fc_font_unlock_face (font);
}

/**
 * _pango_fc_font_get_decoder
 * @font: a #PangoFcFont
 *
 * This will return any custom decoder set on this font.
 *
 * Returns: The custom decoder
 *
 * Since: 1.6
 **/

PangoFcDecoder *
_pango_fc_font_get_decoder (PangoFcFont *font)
{
  PangoFcFontPrivate *priv = PANGO_FC_FONT_GET_PRIVATE (font);

  return priv->decoder;
}

/**
 * _pango_fc_font_set_decoder
 * @font: a #PangoFcFont
 * @decoder: a #PangoFcDecoder to set for this font
 *
 * This sets a custom decoder for this font.  Any previous decoder
 * will be released before this one is set.
 *
 * Since: 1.6
 **/

void
_pango_fc_font_set_decoder (PangoFcFont    *font,
			    PangoFcDecoder *decoder)
{
  PangoFcFontPrivate *priv = PANGO_FC_FONT_GET_PRIVATE (font);

  if (priv->decoder)
    g_object_unref (priv->decoder);

  priv->decoder = decoder;

  if (priv->decoder)
    g_object_ref (priv->decoder);
}

static FT_Glyph_Metrics *
get_per_char (FT_Face      face,
	      FT_Int32     load_flags,
	      PangoGlyph   glyph)
{
  FT_Error error;
  FT_Glyph_Metrics *result;
	
  error = FT_Load_Glyph (face, glyph, load_flags);
  if (error == FT_Err_Ok)
    result = &face->glyph->metrics;
  else
    result = NULL;

  return result;
}

/**
 * pango_fc_font_get_raw_extents:
 * @fcfont: a #PangoFcFont
 * @load_flags: flags to pass to FT_Load_Glyph()
 * @glyph: the glyph index to load
 * @ink_rect: location to store ink extents of the glyph, or %NULL
 * @logical_rect: location to store logical extents of the glyph or %NULL
 * 
 * Gets the extents of a single glyph from a font. The extents are in
 * user space; that is, they are not transformed by any matrix in effect
 * for the font.
 *
 * Long term, this functionality probably belongs in the default
 * implementation of the get_glyph_extents() virtual function.
 * The other possibility would be to to make it public in something
 * like it's current form, and also expose glyph information
 * caching functionality similar to pango_ft2_font_set_glyph_info().
 **/
void
pango_fc_font_get_raw_extents (PangoFcFont    *fcfont,
			       FT_Int32        load_flags,
			       PangoGlyph      glyph,
			       PangoRectangle *ink_rect,
			       PangoRectangle *logical_rect)
{
  FT_Glyph_Metrics *gm;
  FT_Face face;

  face = pango_fc_font_lock_face (fcfont);

  if (!glyph)
    gm = NULL;
  else
    gm = get_per_char (face, load_flags, glyph);
  
  if (gm)
    {
      if (ink_rect)
	{
	  ink_rect->x = PANGO_UNITS_26_6 (gm->horiBearingX);
	  ink_rect->width = PANGO_UNITS_26_6 (gm->width);
	  ink_rect->y = -PANGO_UNITS_26_6 (gm->horiBearingY);
	  ink_rect->height = PANGO_UNITS_26_6 (gm->height);
	}

      if (logical_rect)
	{
	  logical_rect->x = 0;
	  logical_rect->width = PANGO_UNITS_26_6 (gm->horiAdvance);
	  if (fcfont->is_hinted ||
	      (face->face_flags & FT_FACE_FLAG_SCALABLE) == 0)
	    {
	      logical_rect->y = - PANGO_UNITS_26_6 (face->size->metrics.ascender);
	      logical_rect->height = PANGO_UNITS_26_6 (face->size->metrics.ascender - face->size->metrics.descender);
	    }
	  else
	    {
	      FT_Fixed ascender, descender;
	      
	      ascender = FT_MulFix (face->ascender, face->size->metrics.y_scale);
	      descender = FT_MulFix (face->descender, face->size->metrics.y_scale);
	      
	      logical_rect->y = - PANGO_UNITS_26_6 (ascender);
	      logical_rect->height = PANGO_UNITS_26_6 (ascender - descender);
	    }
	}
    }
  else
    {
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
      
  pango_fc_font_unlock_face (fcfont);
}

