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

#include "config.h"

#include "pangofc-font.h"
#include "pangofc-fontmap.h"
#include "pangofc-private.h"
#include "pango-layout.h"
#include "pango-modules.h"
#include "pango-impl-utils.h"

#include <fontconfig/fcfreetype.h>

#include FT_TRUETYPE_TABLES_H

enum {
  PROP_0,
  PROP_PATTERN
};

typedef struct _GUnicharToGlyphCacheEntry GUnicharToGlyphCacheEntry;

/* An entry in the fixed-size cache for the gunichar -> glyph mapping.
 * The cache is indexed by the lower N bits of the gunichar (see
 * GLYPH_CACHE_NUM_ENTRIES).  For scripts with few code points,
 * this should provide pretty much instant lookups.
 *
 * The "ch" field is zero if that cache entry is invalid.
 */
struct _GUnicharToGlyphCacheEntry
{
  gunichar   ch;
  PangoGlyph glyph;
};

typedef struct _PangoFcFontPrivate PangoFcFontPrivate;

struct _PangoFcFontPrivate
{
  PangoFcDecoder *decoder;
  gpointer context_key;
  GUnicharToGlyphCacheEntry *char_to_glyph_cache;
};

#define GLYPH_CACHE_NUM_ENTRIES 256 /* should be power of two */
#define GLYPH_CACHE_MASK (GLYPH_CACHE_NUM_ENTRIES - 1)

static gboolean pango_fc_font_real_has_char  (PangoFcFont *font,
					      gunichar     wc);
static guint    pango_fc_font_real_get_glyph (PangoFcFont *font,
					      gunichar     wc);

static void                  pango_fc_font_finalize     (GObject          *object);
static void                  pango_fc_font_set_property (GObject          *object,
							 guint             prop_id,
							 const GValue     *value,
							 GParamSpec       *pspec);
static void                  pango_fc_font_get_property (GObject          *object,
							 guint             prop_id,
							 GValue           *value,
							 GParamSpec       *pspec);
static PangoEngineShape *    pango_fc_font_find_shaper  (PangoFont        *font,
							 PangoLanguage    *language,
							 guint32           ch);
static PangoCoverage *       pango_fc_font_get_coverage (PangoFont        *font,
							 PangoLanguage    *language);
static PangoFontMetrics *    pango_fc_font_get_metrics  (PangoFont        *font,
							 PangoLanguage    *language);
static PangoFontMap *        pango_fc_font_get_font_map (PangoFont        *font);
static PangoFontDescription *pango_fc_font_describe     (PangoFont        *font);
static PangoFontDescription *pango_fc_font_describe_absolute (PangoFont        *font);


#define PANGO_FC_FONT_LOCK_FACE(font)	(PANGO_FC_FONT_GET_CLASS (font)->lock_face (font))
#define PANGO_FC_FONT_UNLOCK_FACE(font)	(PANGO_FC_FONT_GET_CLASS (font)->unlock_face (font))

G_DEFINE_ABSTRACT_TYPE (PangoFcFont, pango_fc_font, PANGO_TYPE_FONT)

static void
pango_fc_font_class_init (PangoFcFontClass *class)
{
  GObjectClass *object_class = G_OBJECT_CLASS (class);
  PangoFontClass *font_class = PANGO_FONT_CLASS (class);

  class->has_char = pango_fc_font_real_has_char;
  class->get_glyph = pango_fc_font_real_get_glyph;
  class->get_unknown_glyph = NULL;

  object_class->finalize = pango_fc_font_finalize;
  object_class->set_property = pango_fc_font_set_property;
  object_class->get_property = pango_fc_font_get_property;
  font_class->describe = pango_fc_font_describe;
  font_class->describe_absolute = pango_fc_font_describe_absolute;
  font_class->find_shaper = pango_fc_font_find_shaper;
  font_class->get_coverage = pango_fc_font_get_coverage;
  font_class->get_metrics = pango_fc_font_get_metrics;
  font_class->get_font_map = pango_fc_font_get_font_map;

  g_object_class_install_property (object_class, PROP_PATTERN,
				   g_param_spec_pointer ("pattern",
							 "Pattern",
							 "The fontconfig pattern for this font",
							 G_PARAM_READWRITE | G_PARAM_CONSTRUCT_ONLY |
							 G_PARAM_STATIC_STRINGS));

  g_type_class_add_private (object_class, sizeof (PangoFcFontPrivate));
}

static void
pango_fc_font_init (PangoFcFont *fcfont)
{
  fcfont->priv = G_TYPE_INSTANCE_GET_PRIVATE (fcfont,
					      PANGO_TYPE_FC_FONT,
					      PangoFcFontPrivate);
}

static void
free_metrics_info (PangoFcMetricsInfo *info)
{
  pango_font_metrics_unref (info->metrics);
  g_slice_free (PangoFcMetricsInfo, info);
}

static void
pango_fc_font_finalize (GObject *object)
{
  PangoFcFont *fcfont = PANGO_FC_FONT (object);
  PangoFcFontPrivate *priv = fcfont->priv;

  g_slist_foreach (fcfont->metrics_by_lang, (GFunc)free_metrics_info, NULL);
  g_slist_free (fcfont->metrics_by_lang);

  if (fcfont->fontmap)
    _pango_fc_font_map_remove (PANGO_FC_FONT_MAP (fcfont->fontmap), fcfont);

  FcPatternDestroy (fcfont->font_pattern);
  pango_font_description_free (fcfont->description);

  if (priv->decoder)
    _pango_fc_font_set_decoder (fcfont, NULL);

  g_free (priv->char_to_glyph_cache);

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

static void
pango_fc_font_get_property (GObject       *object,
			    guint          prop_id,
			    GValue        *value,
			    GParamSpec    *pspec)
{
  switch (prop_id)
    {
    case PROP_PATTERN:
      {
	PangoFcFont *fcfont = PANGO_FC_FONT (object);
	g_value_set_pointer (value, fcfont->font_pattern);
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

static PangoFontDescription *
pango_fc_font_describe_absolute (PangoFont *font)
{
  PangoFcFont *fcfont = (PangoFcFont *)font;
  PangoFontDescription *desc = pango_font_description_copy (fcfont->description);
  double size;

  if (FcPatternGetDouble (fcfont->font_pattern, FC_PIXEL_SIZE, 0, &size) == FcResultMatch)
    pango_font_description_set_absolute_size (desc, size * PANGO_SCALE);

  return desc;
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
  PangoScript script;

  shaper_map = pango_fc_get_shaper_map (language);
  script = pango_script_for_unichar (ch);
  return (PangoEngineShape *)pango_map_get_engine (shaper_map, script);
}

static PangoCoverage *
pango_fc_font_get_coverage (PangoFont     *font,
			    PangoLanguage *language)
{
  PangoFcFont *fcfont = (PangoFcFont *)font;
  PangoFcFontPrivate *priv = fcfont->priv;
  FcCharSet *charset;

  if (priv->decoder)
    {
      charset = pango_fc_decoder_get_charset (priv->decoder, fcfont);
      return _pango_fc_font_map_fc_to_coverage (charset);
    }

  return _pango_fc_font_map_get_coverage (PANGO_FC_FONT_MAP (fcfont->fontmap),
					  fcfont);
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
  FT_Face face = PANGO_FC_FONT_LOCK_FACE (fcfont);
  FcMatrix *fc_matrix;
  FT_Matrix ft_matrix;
  TT_OS2 *os2;
  gboolean have_transform = FALSE;

  if (G_UNLIKELY (!face))
    {
      metrics->descent = 0;
      metrics->ascent = PANGO_SCALE * PANGO_UNKNOWN_GLYPH_HEIGHT;
      metrics->underline_thickness = PANGO_SCALE;
      metrics->underline_position = - PANGO_SCALE;
      metrics->strikethrough_thickness = PANGO_SCALE;
      metrics->strikethrough_position = PANGO_SCALE * (PANGO_UNKNOWN_GLYPH_HEIGHT/2);
      return;
    }

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


  metrics->underline_thickness = 0;
  metrics->underline_position = 0;

  {
    FT_Fixed ft_thickness, ft_position;

    ft_thickness = FT_MulFix (face->underline_thickness, face->size->metrics.y_scale);
    metrics->underline_thickness = PANGO_UNITS_26_6 (ft_thickness);

    ft_position = FT_MulFix (face->underline_position, face->size->metrics.y_scale);
    metrics->underline_position = PANGO_UNITS_26_6 (ft_position);
  }

  if (metrics->underline_thickness == 0 || metrics->underline_position == 0)
    {
      metrics->underline_thickness = (PANGO_SCALE * face->size->metrics.y_ppem) / 14;
      metrics->underline_position = - metrics->underline_thickness;
    }


  metrics->strikethrough_thickness = 0;
  metrics->strikethrough_position = 0;

  os2 = FT_Get_Sfnt_Table (face, ft_sfnt_os2);
  if (os2 && os2->version != 0xFFFF)
    {
      FT_Fixed ft_thickness, ft_position;

      ft_thickness = FT_MulFix (os2->yStrikeoutSize, face->size->metrics.y_scale);
      metrics->strikethrough_thickness = PANGO_UNITS_26_6 (ft_thickness);

      ft_position = FT_MulFix (os2->yStrikeoutPosition, face->size->metrics.y_scale);
      metrics->strikethrough_position = PANGO_UNITS_26_6 (ft_position);
    }

  if (metrics->strikethrough_thickness == 0 || metrics->strikethrough_position == 0)
    {
      metrics->strikethrough_thickness = metrics->underline_thickness;
      metrics->strikethrough_position = (PANGO_SCALE * face->size->metrics.y_ppem) / 4;
    }


  /* If hinting is on for this font, quantize the underline and strikethrough position
   * to integer values.
   */
  if (fcfont->is_hinted)
    {
      pango_quantize_line_geometry (&metrics->underline_thickness,
				    &metrics->underline_position);
      pango_quantize_line_geometry (&metrics->strikethrough_thickness,
				    &metrics->strikethrough_position);

      /* Quantizing may have pushed underline_position to 0.  Not good */
      if (metrics->underline_position == 0)
	metrics->underline_position = - metrics->underline_thickness;
    }


  PANGO_FC_FONT_UNLOCK_FACE (fcfont);
}

static int
max_glyph_width (PangoLayout *layout)
{
  int max_width = 0;
  GSList *l, *r;

  for (l = pango_layout_get_lines_readonly (layout); l; l = l->next)
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

PangoFontMetrics *
pango_fc_font_create_metrics_for_context (PangoFcFont   *fcfont,
					  PangoContext  *context)
{
  PangoFontMetrics *metrics;
  PangoLayout *layout;
  PangoRectangle extents;
  PangoLanguage *language = pango_context_get_language (context);
  const char *sample_str = pango_language_get_sample_string (language);
  PangoFontDescription *desc = pango_font_describe_with_absolute_size (PANGO_FONT (fcfont));

  metrics = pango_font_metrics_new ();

  get_face_metrics (fcfont, metrics);

  layout = pango_layout_new (context);
  pango_layout_set_font_description (layout, desc);
  pango_font_description_free (desc);

  pango_layout_set_text (layout, sample_str, -1);
  pango_layout_get_extents (layout, NULL, &extents);

  metrics->approximate_char_width =
    extents.width / g_utf8_strlen (sample_str, -1);

  pango_layout_set_text (layout, "0123456789", -1);
  metrics->approximate_digit_width = max_glyph_width (layout);

  g_object_unref (layout);

  return metrics;
}

/* This function is cut-and-pasted into pangocairo-fcfont.c - it might be
 * better to add a virtual fcfont->create_context (font).
 */
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
      PangoFontMap *fontmap;
      PangoContext *context;

      /* XXX this is racy.  because weakref's are racy... */
      fontmap = fcfont->fontmap;
      if (!fontmap)
	return pango_font_metrics_new ();
      fontmap = g_object_ref (fontmap);

      info = g_slice_new0 (PangoFcMetricsInfo);

      fcfont->metrics_by_lang = g_slist_prepend (fcfont->metrics_by_lang,
						 info);

      info->sample_str = sample_str;

      context = pango_font_map_create_context (fontmap);
      pango_context_set_language (context, language);

      info->metrics = pango_fc_font_create_metrics_for_context (fcfont, context);

      g_object_unref (context);
      g_object_unref (fontmap);
    }

  return pango_font_metrics_ref (info->metrics);
}

static PangoFontMap *
pango_fc_font_get_font_map (PangoFont *font)
{
  PangoFcFont *fcfont = PANGO_FC_FONT (font);

  return fcfont->fontmap;
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

  guint idx;
  GUnicharToGlyphCacheEntry *entry;

  PangoFcFontPrivate *priv = font->priv;

  if (G_UNLIKELY (priv->char_to_glyph_cache == NULL))
    {
      priv->char_to_glyph_cache = g_new0 (GUnicharToGlyphCacheEntry, GLYPH_CACHE_NUM_ENTRIES);
      /* Make sure all cache entries are invalid initially */
      priv->char_to_glyph_cache[0].ch = 1; /* char 1 cannot happen in bucket 0 */
    }

  idx = wc & GLYPH_CACHE_MASK;
  entry = priv->char_to_glyph_cache + idx;

  if (entry->ch != wc)
    {
      face = PANGO_FC_FONT_LOCK_FACE (font);
      if (G_LIKELY (face))
        {
	  index = FcFreeTypeCharIndex (face, wc);
	  if (index > (FT_UInt)face->num_glyphs)
	    index = 0;

	  PANGO_FC_FONT_UNLOCK_FACE (font);
	}
      else
        index = 0;

      entry->ch = wc;
      entry->glyph = index;
    }

  return entry->glyph;
}

/**
 * pango_fc_font_lock_face:
 * @font: a #PangoFcFont.
 *
 * Gets the FreeType <type>FT_Face</type> associated with a font,
 * This face will be kept around until you call
 * pango_fc_font_unlock_face().
 *
 * Return value: the FreeType <type>FT_Face</type> associated with @font.
 *
 * Since: 1.4
 **/
FT_Face
pango_fc_font_lock_face (PangoFcFont *font)
{
  g_return_val_if_fail (PANGO_IS_FC_FONT (font), NULL);

  return PANGO_FC_FONT_LOCK_FACE (font);
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

  PANGO_FC_FONT_UNLOCK_FACE (font);
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
  PangoFcFontPrivate *priv = font->priv;
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
 * @wc: Unicode character to look up
 *
 * Gets the glyph index for a given Unicode character
 * for @font. If you only want to determine
 * whether the font has the glyph, use pango_fc_font_has_char().
 *
 * Return value: the glyph index, or 0, if the Unicode
 *   character doesn't exist in the font.
 *
 * Since: 1.4
 **/
PangoGlyph
pango_fc_font_get_glyph (PangoFcFont *font,
			 gunichar     wc)
{
  PangoFcFontPrivate *priv = font->priv;

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
 * Use PANGO_GET_UNKNOWN_GLYPH() instead.
 *
 * Return value: a glyph index into @font.
 *
 * Since: 1.4
 **/
PangoGlyph
pango_fc_font_get_unknown_glyph (PangoFcFont *font,
				 gunichar     wc)
{
  if (font && PANGO_FC_FONT_GET_CLASS (font)->get_unknown_glyph)
    return PANGO_FC_FONT_GET_CLASS (font)->get_unknown_glyph (font, wc);

  return PANGO_GET_UNKNOWN_GLYPH (wc);
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
  gboolean hinting = font->is_hinted;

  g_return_if_fail (PANGO_IS_FC_FONT (font));
  g_return_if_fail (glyphs != NULL);

  face = PANGO_FC_FONT_LOCK_FACE (font);
  if (G_UNLIKELY (!face))
    return;

  if (!FT_HAS_KERNING (face))
    {
      PANGO_FC_FONT_UNLOCK_FACE (font);
      return;
    }

  for (i = 1; i < glyphs->num_glyphs; ++i)
    {
      error = FT_Get_Kerning (face,
			      glyphs->glyphs[i-1].glyph,
			      glyphs->glyphs[i].glyph,
			      ft_kerning_default,
			      &kerning);

      if (error == FT_Err_Ok) {
	int adjustment = PANGO_UNITS_26_6 (kerning.x);

	if (hinting)
	  adjustment = PANGO_UNITS_ROUND (adjustment);

	glyphs->glyphs[i-1].geometry.width += adjustment;
      }
    }

  PANGO_FC_FONT_UNLOCK_FACE (font);
}

/**
 * _pango_fc_font_get_decoder
 * @font: a #PangoFcFont
 *
 * This will return any custom decoder set on this font.
 *
 * Return value: The custom decoder
 *
 * Since: 1.6
 **/

PangoFcDecoder *
_pango_fc_font_get_decoder (PangoFcFont *font)
{
  PangoFcFontPrivate *priv = font->priv;

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
  PangoFcFontPrivate *priv = font->priv;

  if (priv->decoder)
    g_object_unref (priv->decoder);

  priv->decoder = decoder;

  if (priv->decoder)
    g_object_ref (priv->decoder);
}

gpointer
_pango_fc_font_get_context_key (PangoFcFont *fcfont)
{
  PangoFcFontPrivate *priv = fcfont->priv;

  return priv->context_key;
}

void
_pango_fc_font_set_context_key (PangoFcFont *fcfont,
				gpointer     context_key)
{
  PangoFcFontPrivate *priv = fcfont->priv;

  priv->context_key = context_key;
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
 *
 * Since: 1.6
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

  g_return_if_fail (PANGO_IS_FC_FONT (fcfont));

  face = PANGO_FC_FONT_LOCK_FACE (fcfont);
  if (G_UNLIKELY (!face))
    {
      /* Get generic unknown-glyph extents. */
      pango_font_get_glyph_extents (NULL, glyph, ink_rect, logical_rect);
      return;
    }

  if (glyph == PANGO_GLYPH_EMPTY)
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

  PANGO_FC_FONT_UNLOCK_FACE (fcfont);
}

