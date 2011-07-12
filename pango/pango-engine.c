/* Pango
 * pango-engine.c: Engines for script and language specific processing
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

#include "pango-engine.h"
#include "pango-engine-private.h"
#include "pango-impl-utils.h"


G_DEFINE_ABSTRACT_TYPE (PangoEngine, pango_engine, G_TYPE_OBJECT);

static void
pango_engine_init (PangoEngine *self)
{
}

static void
pango_engine_class_init (PangoEngineClass *klass)
{
}


G_DEFINE_ABSTRACT_TYPE (PangoEngineLang, pango_engine_lang, PANGO_TYPE_ENGINE);

static void
pango_engine_lang_init (PangoEngineLang *self)
{
}

static void
pango_engine_lang_class_init (PangoEngineLangClass *klass)
{
}


static PangoCoverageLevel
pango_engine_shape_real_covers (PangoEngineShape *engine G_GNUC_UNUSED,
				PangoFont        *font,
				PangoLanguage    *language,
				gunichar          wc)
{

  PangoCoverage *coverage = pango_font_get_coverage (font, language);
  PangoCoverageLevel result = pango_coverage_get (coverage, wc);

  pango_coverage_unref (coverage);

  return result;
}


G_DEFINE_ABSTRACT_TYPE (PangoEngineShape, pango_engine_shape, PANGO_TYPE_ENGINE);

static void
pango_engine_shape_init (PangoEngineShape *klass)
{
}

static void
pango_engine_shape_class_init (PangoEngineShapeClass *class)
{
  class->covers = pango_engine_shape_real_covers;
}

void
_pango_engine_shape_shape (PangoEngineShape *engine,
			   PangoFont        *font,
			   const char       *text,
			   int               length,
			   const PangoAnalysis *analysis,
			   PangoGlyphString *glyphs)
{
  glyphs->num_glyphs = 0;

  PANGO_ENGINE_SHAPE_GET_CLASS (engine)->script_shape (engine,
						       font,
						       text, length,
						       analysis,
						       glyphs);
}

PangoCoverageLevel
_pango_engine_shape_covers (PangoEngineShape *engine,
			    PangoFont        *font,
			    PangoLanguage    *language,
			    gunichar          wc)
{
  g_return_val_if_fail (PANGO_IS_ENGINE_SHAPE (engine), PANGO_COVERAGE_NONE);
  g_return_val_if_fail (PANGO_IS_FONT (font), PANGO_COVERAGE_NONE);

  return PANGO_ENGINE_SHAPE_GET_CLASS (engine)->covers (engine,
							font,
							language,
							wc);
}

/* No extra fields needed */
typedef PangoEngineShape PangoFallbackEngine;
typedef PangoEngineShapeClass PangoFallbackEngineClass;

static void
fallback_engine_shape (PangoEngineShape *engine G_GNUC_UNUSED,
		       PangoFont        *font G_GNUC_UNUSED,
		       const char       *text,
		       gint              length,
		       const PangoAnalysis *analysis,
		       PangoGlyphString *glyphs)
{
  int n_chars;
  const char *p;
  int cluster = 0;
  int i;

  n_chars = text ? pango_utf8_strlen (text, length) : 0;

  pango_glyph_string_set_size (glyphs, n_chars);

  p = text;
  for (i = 0; i < n_chars; i++)
    {
      gunichar wc;
      PangoGlyph glyph;
      PangoRectangle logical_rect;

      wc = g_utf8_get_char (p);

      if (g_unichar_type (wc) != G_UNICODE_NON_SPACING_MARK)
	cluster = p - text;

      if (pango_is_zero_width (wc))
	glyph = PANGO_GLYPH_EMPTY;
      else
	glyph = PANGO_GET_UNKNOWN_GLYPH (wc);

      pango_font_get_glyph_extents (analysis->font, glyph, NULL, &logical_rect);

      glyphs->glyphs[i].glyph = glyph;

      glyphs->glyphs[i].geometry.x_offset = 0;
      glyphs->glyphs[i].geometry.y_offset = 0;
      glyphs->glyphs[i].geometry.width = logical_rect.width;

      glyphs->log_clusters[i] = cluster;

      p = g_utf8_next_char (p);
    }

  if (analysis->level & 1)
    pango_glyph_string_reverse_range (glyphs, 0, glyphs->num_glyphs);
}

static PangoCoverageLevel
fallback_engine_covers (PangoEngineShape *engine G_GNUC_UNUSED,
			PangoFont        *font G_GNUC_UNUSED,
			PangoLanguage    *lang G_GNUC_UNUSED,
			gunichar          wc G_GNUC_UNUSED)
{
  return PANGO_COVERAGE_NONE;
}


static GType pango_fallback_engine_get_type (void);

G_DEFINE_TYPE (PangoFallbackEngine, pango_fallback_engine, PANGO_TYPE_ENGINE_SHAPE);

static void
pango_fallback_engine_init (PangoFallbackEngine *self)
{
}

static void
pango_fallback_engine_class_init (PangoFallbackEngineClass *class)
{
  class->covers = fallback_engine_covers;
  class->script_shape = fallback_engine_shape;
}

PangoEngineShape *
_pango_get_fallback_shaper (void)
{
  static PangoEngineShape *fallback_shaper = NULL;
  if (!fallback_shaper)
    fallback_shaper = g_object_new (pango_fallback_engine_get_type (), NULL);

  return fallback_shaper;
}

