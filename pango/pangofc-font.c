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

#define PANGO_SCALE_26_6 (PANGO_SCALE / (1<<6))
#define PANGO_PIXELS_26_6(d)				\
  (((d) >= 0) ?						\
   ((d) + PANGO_SCALE_26_6 / 2) / PANGO_SCALE_26_6 :	\
   ((d) - PANGO_SCALE_26_6 / 2) / PANGO_SCALE_26_6)
#define PANGO_UNITS_26_6(d) (PANGO_SCALE_26_6 * (d))

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

static void                  pango_fc_font_class_init   (PangoFcFontClass *class);
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

static GObjectClass *parent_class;

GType
pango_fc_font_get_type (void)
{
  static GType object_type = 0;

  if (!object_type)
    {
      static const GTypeInfo object_info =
      {
        sizeof (PangoFcFontClass),
        (GBaseInitFunc) NULL,
        (GBaseFinalizeFunc) NULL,
        (GClassInitFunc) pango_fc_font_class_init,
        NULL,           /* class_finalize */
        NULL,           /* class_data */
        sizeof (PangoFcFont),
        0,              /* n_preallocs */
        NULL            /* init */
      };
      
      object_type = g_type_register_static (PANGO_TYPE_FONT,
                                            "PangoFcFont",
                                            &object_info,
					    G_TYPE_FLAG_ABSTRACT);
    }
  
  return object_type;
}
 
static void
pango_fc_font_class_init (PangoFcFontClass *class)
{
  GObjectClass *object_class = G_OBJECT_CLASS (class);
  PangoFontClass *font_class = PANGO_FONT_CLASS (class);
  
  parent_class = g_type_class_peek_parent (class);
 
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

  g_slist_foreach (fcfont->metrics_by_lang, (GFunc)free_metrics_info, NULL);
  g_slist_free (fcfont->metrics_by_lang);  

  if (fcfont->fontmap)
    _pango_fc_font_map_remove (PANGO_FC_FONT_MAP (fcfont->fontmap), fcfont);

  FcPatternDestroy (fcfont->font_pattern);
  pango_font_description_free (fcfont->description);

  parent_class->finalize (object);
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

  return _pango_fc_font_map_get_coverage (PANGO_FC_FONT_MAP (fcfont->fontmap),
					  fcfont->font_pattern);
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
  else
    {
      metrics->descent = - PANGO_UNITS_26_6 (face->size->metrics.descender);
      metrics->ascent = PANGO_UNITS_26_6 (face->size->metrics.ascender);
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
  g_return_val_if_fail (PANGO_IS_FC_FONT (font), FALSE);

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
  g_return_val_if_fail (PANGO_IS_FC_FONT (font), 0);

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
