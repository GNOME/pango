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

/**
 * SECTION:engines
 * @short_description:Language-specific and rendering-system-specific processing
 * @title:Engines
 *
 * Pango utilizes a module architecture in which the language-specific
 * and render-system-specific components are provided by loadable
 * modules. Each loadable module supplies one or more
 * <firstterm>engines</firstterm>.  Each <firstterm>engine</firstterm>
 * has an associated <firstterm>engine type</firstterm> and
 * <firstterm>render type</firstterm>. These two types are represented by strings.
 *
 * Each dynamically-loaded module exports several functions which provide
 * the public API. These functions are script_engine_list(),
 * script_engine_init() and script_engine_exit, and
 * script_engine_create(). The latter three functions are used when
 * creating engines from the module at run time, while the first
 * function is used when building a catalog of all available modules.
 *
 * Deprecated: 1.38
 */
/**
 * SECTION:pango-engine-lang
 * @short_description:Rendering-system independent script engines
 * @title:PangoEngineLang
 * @stability:Unstable
 *
 * The <firstterm>language engines</firstterm> are rendering-system independent
 * engines that determine line, word, and character breaks for character strings.
 * These engines are used in pango_break().
 *
 * Deprecated: 1.38
 */
/**
 * SECTION:pango-engine-shape
 * @short_description:Rendering-system dependent script engines
 * @title:PangoEngineShape
 * @stability:Unstable
 *
 * The <firstterm>shape engines</firstterm> are rendering-system dependent
 * engines that convert character strings into glyph strings.
 * These engines are used in pango_shape().
 *
 * Deprecated: 1.38
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

G_GNUC_BEGIN_IGNORE_DEPRECATIONS
G_DEFINE_ABSTRACT_TYPE (PangoEngineLang, pango_engine_lang, PANGO_TYPE_ENGINE);
G_GNUC_END_IGNORE_DEPRECATIONS

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

G_GNUC_BEGIN_IGNORE_DEPRECATIONS
G_DEFINE_ABSTRACT_TYPE (PangoEngineShape, pango_engine_shape, PANGO_TYPE_ENGINE);
G_GNUC_END_IGNORE_DEPRECATIONS

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
_pango_engine_shape_shape (PangoEngineShape    *engine,
			   PangoFont           *font,
			   const char          *item_text,
			   unsigned int         item_length,
			   const char          *paragraph_text,
			   unsigned int         paragraph_len,
			   const PangoAnalysis *analysis,
			   PangoGlyphString    *glyphs)
{
  glyphs->num_glyphs = 0;

  PANGO_ENGINE_SHAPE_GET_CLASS (engine)->script_shape (engine,
						       font,
						       item_text,
						       item_length,
						       analysis,
						       glyphs,
						       paragraph_text,
						       paragraph_len);
}

PangoCoverageLevel
_pango_engine_shape_covers (PangoEngineShape *engine,
			    PangoFont        *font,
			    PangoLanguage    *language,
			    gunichar          wc)
{
  if (G_UNLIKELY (!engine || !font))
    return PANGO_COVERAGE_NONE;

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
		       unsigned int      length,
		       const PangoAnalysis *analysis,
		       PangoGlyphString *glyphs,
                       const char       *paragraph_text G_GNUC_UNUSED,
                       unsigned int      paragraph_length G_GNUC_UNUSED)
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

G_GNUC_BEGIN_IGNORE_DEPRECATIONS
G_DEFINE_TYPE (PangoFallbackEngine, pango_fallback_engine, PANGO_TYPE_ENGINE_SHAPE);
G_GNUC_END_IGNORE_DEPRECATIONS

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
  static PangoEngineShape *fallback_shaper = NULL; /* MT-safe */
  if (g_once_init_enter (&fallback_shaper))
    g_once_init_leave(&fallback_shaper, g_object_new (pango_fallback_engine_get_type (), NULL));

  return fallback_shaper;
}

