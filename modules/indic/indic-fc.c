/* Pango
 * indic-xft.c:
 *
 * Copyright (C) 2001, 2002 IBM Corporation
 * Author: Eric Mader <mader@jtcsv.com>
 * Based on arabic-xft.c by Owen Taylor <otaylor@redhat.com>
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

#include <string.h>

#include "indic-ot.h"

#include "pangoxft.h"
#include "pango-engine.h"
#include "pango-utils.h"

typedef struct _PangoEngineShapeIndic PangoEngineShapeIndic;
typedef struct _PangoIndicInfo PangoIndicInfo;

struct _PangoEngineShapeIndic
{
  PangoEngineShape shapeEngine;
  PangoIndicInfo  *indicInfo;
};

struct _PangoIndicInfo
{
  PangoOTTag         scriptTag; 
  IndicOTClassTable *classTable;
  gchar             *gsubQuarkName;
  gchar             *gposQuarkName;
};

#define INDIC_ENGINE_INFO(script) {#script "ScriptEngineXft", PANGO_ENGINE_TYPE_SHAPE, PANGO_RENDER_TYPE_XFT, script##_ranges, G_N_ELEMENTS(script##_ranges)}

#define PANGO_INDIC_INFO(script) {OT_TAG_##script, &script##_class_table, "pango-indic-" #script "-GSUB-ruleset", "pango-indic-" #script "-GPOS-rulsest"}

#define INDIC_SCRIPT_RANGE(script) {SCRIPT_RANGE_##script, "*"}

#define OT_TAG_deva FT_MAKE_TAG('d','e','v','a')
#define OT_TAG_beng FT_MAKE_TAG('b','e','n','g')
#define OT_TAG_punj FT_MAKE_TAG('p','u','n','j')
#define OT_TAG_gujr FT_MAKE_TAG('g','u','j','r')
#define OT_TAG_orya FT_MAKE_TAG('o','r','y','a')
#define OT_TAG_taml FT_MAKE_TAG('t','a','m','l')
#define OT_TAG_telu FT_MAKE_TAG('t','e','l','u')
#define OT_TAG_knda FT_MAKE_TAG('k','n','d','a')
#define OT_TAG_mlym FT_MAKE_TAG('m','l','y','m')

static PangoEngineRange deva_ranges[] = {
  INDIC_SCRIPT_RANGE(deva), /* Devanagari */
};

static PangoEngineRange beng_ranges[] = {
  INDIC_SCRIPT_RANGE(beng), /* Bengali */
};

static PangoEngineRange punj_ranges[] = {
  INDIC_SCRIPT_RANGE(punj), /* Punjabi */
};

static PangoEngineRange gujr_ranges[] = {
  INDIC_SCRIPT_RANGE(gujr), /* Gujarati */
};

static PangoEngineRange orya_ranges[] = {
  INDIC_SCRIPT_RANGE(orya), /* Oriya */
};

static PangoEngineRange taml_ranges[] = {
  INDIC_SCRIPT_RANGE(taml), /* Tamil */
};

static PangoEngineRange telu_ranges[] = {
  INDIC_SCRIPT_RANGE(telu), /* Telugu */
};

static PangoEngineRange knda_ranges[] = {
  INDIC_SCRIPT_RANGE(knda), /* Kannada */
};

static PangoEngineRange mlym_ranges[] = {
  INDIC_SCRIPT_RANGE(mlym), /* Malayalam */
};

static PangoEngineInfo script_engines[] = {
  INDIC_ENGINE_INFO(deva), INDIC_ENGINE_INFO(beng), INDIC_ENGINE_INFO(punj),
  INDIC_ENGINE_INFO(gujr), INDIC_ENGINE_INFO(orya), INDIC_ENGINE_INFO(taml),
  INDIC_ENGINE_INFO(telu), INDIC_ENGINE_INFO(knda), INDIC_ENGINE_INFO(mlym)
};

/*
 * WARNING: These entries need to be in the same order as the entries
 * in script_engines[].
 *
 * FIXME: remove this requirement, either by encapsulating the order
 * in a macro that calls a body macro that can be redefined, or by
 * putting the pointers to the PangoEngineInfo in PangoIndicInfo...
 */
static PangoIndicInfo indic_info[] = {
  PANGO_INDIC_INFO(deva), PANGO_INDIC_INFO(beng), PANGO_INDIC_INFO(punj),
  PANGO_INDIC_INFO(gujr), PANGO_INDIC_INFO(orya), PANGO_INDIC_INFO(taml),
  PANGO_INDIC_INFO(telu), PANGO_INDIC_INFO(knda), PANGO_INDIC_INFO(mlym)
};

static void
maybe_add_GSUB_feature (PangoOTRuleset *ruleset,
			PangoOTInfo    *info,
			guint           script_index,
			PangoOTTag      feature_tag,
			gulong          property_bit)
{
  guint feature_index;
  
  /* 0xffff == default language system */
  if (pango_ot_info_find_feature (info, PANGO_OT_TABLE_GSUB,
				  feature_tag, script_index, 0xffff, &feature_index))
    {
      /*
      printf("Added GSUB feature '%c%c%c%c' = %8.8X\n", feature_tag>>24, feature_tag>>16&0xFF, feature_tag>>8&0xFF, feature_tag&0xFF, property_bit);
      */

      pango_ot_ruleset_add_feature (ruleset, PANGO_OT_TABLE_GSUB, feature_index,
				    property_bit);
    }
}

static void maybe_add_GPOS_feature (PangoOTRuleset *ruleset,
				    PangoOTInfo    *info,
				    guint           script_index,
				    PangoOTTag      feature_tag,
				    gulong          property_bit)
{
  guint feature_index;

  if (pango_ot_info_find_feature (info, PANGO_OT_TABLE_GPOS,
				  feature_tag,script_index, 0xffff, &feature_index))
    {
      /*
      printf("Added GPOS feature '%c%c%c%c' = %8.8X\n", feature_tag>>24, feature_tag>>16&0xFF, feature_tag>>8&0xFF, feature_tag&0xFF, property_bit);
      */

      pango_ot_ruleset_add_feature (ruleset, PANGO_OT_TABLE_GPOS, feature_index,
				    property_bit);
    }
}

static PangoOTRuleset *
get_gsub_ruleset (FT_Face face, PangoIndicInfo *indic_info)
{
  PangoOTInfo    *info = pango_ot_info_get (face);
  GQuark          ruleset_quark = g_quark_from_string (indic_info->gsubQuarkName);
  PangoOTRuleset *ruleset;

  if (!info)
    return NULL;

  ruleset = g_object_get_qdata (G_OBJECT (info), ruleset_quark);

  if (!ruleset)
    {
      guint    script_index;

      ruleset = pango_ot_ruleset_new (info);

     if (pango_ot_info_find_script (info, PANGO_OT_TABLE_GSUB,
				     indic_info->scriptTag, &script_index))
	{
	  maybe_add_GSUB_feature (ruleset, info, script_index, FT_MAKE_TAG ('n','u','k','t'), nukt);
	  maybe_add_GSUB_feature (ruleset, info, script_index, FT_MAKE_TAG ('a','k','h','n'), akhn);
	  maybe_add_GSUB_feature (ruleset, info, script_index, FT_MAKE_TAG ('r','p','h','f'), rphf);
	  maybe_add_GSUB_feature (ruleset, info, script_index, FT_MAKE_TAG ('b','l','w','f'), blwf);
	  maybe_add_GSUB_feature (ruleset, info, script_index, FT_MAKE_TAG ('h','a','l','f'), half);
	  maybe_add_GSUB_feature (ruleset, info, script_index, FT_MAKE_TAG ('p','s','t','f'), pstf);
	  maybe_add_GSUB_feature (ruleset, info, script_index, FT_MAKE_TAG ('v','a','t','u'), vatu);
	  maybe_add_GSUB_feature (ruleset, info, script_index, FT_MAKE_TAG ('p','r','e','s'), pres);
	  maybe_add_GSUB_feature (ruleset, info, script_index, FT_MAKE_TAG ('b','l','w','s'), blws);
	  maybe_add_GSUB_feature (ruleset, info, script_index, FT_MAKE_TAG ('a','b','v','s'), abvs);
	  maybe_add_GSUB_feature (ruleset, info, script_index, FT_MAKE_TAG ('p','s','t','s'), psts);
	  maybe_add_GSUB_feature (ruleset, info, script_index, FT_MAKE_TAG ('h','a','l','n'), haln);
	}

      g_object_set_qdata_full (G_OBJECT (info), ruleset_quark, ruleset,
			       (GDestroyNotify)g_object_unref);
    }

  return ruleset;
}


static PangoOTRuleset *
get_gpos_ruleset (FT_Face face, PangoIndicInfo *indic_info)
{
  PangoOTInfo    *info = pango_ot_info_get (face);
  GQuark          ruleset_quark = g_quark_from_string (indic_info->gposQuarkName);
  PangoOTRuleset *ruleset;

  if (!info)
    return NULL;

  ruleset = g_object_get_qdata (G_OBJECT (info), ruleset_quark);

  if (!ruleset)
    {
      guint    script_index;

      ruleset = pango_ot_ruleset_new (info);

      if (1 && pango_ot_info_find_script (info, PANGO_OT_TABLE_GPOS,
				     indic_info->scriptTag, &script_index))
	{
	  maybe_add_GPOS_feature (ruleset, info, script_index, FT_MAKE_TAG ('b','l','w','m'), blwm);
	  maybe_add_GPOS_feature (ruleset, info, script_index, FT_MAKE_TAG ('a','b','v','m'), abvm);
	  maybe_add_GPOS_feature (ruleset, info, script_index, FT_MAKE_TAG ('d','i','s','t'), dist);
	}

      g_object_set_qdata_full (G_OBJECT (info), ruleset_quark, ruleset,
			       (GDestroyNotify)g_object_unref);
    }

  return ruleset;
}
static void
set_glyphs (PangoFont *font, FT_Face face, const gunichar *wcs, const glong *indices, glong n_glyphs, PangoGlyphString *glyphs)
{
  gint i;

  g_assert (face);

  pango_glyph_string_set_size (glyphs, n_glyphs);

  for (i = 0; i < n_glyphs; i += 1)
    {
      PangoGlyph glyph = FT_Get_Char_Index (face, wcs[i]);

      glyphs->glyphs[i].glyph = glyph;
      glyphs->log_clusters[i] = indices[i];
    }
}

/*
 * FIXME: should this check for null pointers, etc.?
 */
static gunichar *
expand_text(const gchar *text, glong length, glong **offsets, glong *n_chars)
{
  const gchar *p;
  gunichar *wcs, *wco;
  glong i, *oo;

  *n_chars = g_utf8_strlen (text, length);
  wcs      = g_new (gunichar, *n_chars);
  *offsets = g_new (glong, *n_chars + 1);

  p   = text;
  wco = wcs;
  oo  = *offsets;
  for (i = 0; i < *n_chars; i += 1)
    {
      *wco++ = g_utf8_get_char (p);
      *oo++  = p - text;

      p = g_utf8_next_char (p);
    }

  *oo = p - text;

  return wcs;
}


/* analysis->shape_engine has the PangoEngine... */
static void 
indic_engine_shape (PangoFont        *font,
		    const char       *text,
		    gint              length,
		    PangoAnalysis    *analysis,
		    PangoGlyphString *glyphs)
{
  glong i, n_chars, n_glyphs;
  gulong *tags = NULL;
  gunichar *wc_in = NULL, *wc_out = NULL;
  glong *utf8_offsets = NULL;
  glong *indices = NULL;
  FT_Face face;
  PangoOTRuleset *gsub_ruleset = NULL, *gpos_ruleset = NULL;
  PangoEngineShapeIndic *indic_shape_engine = NULL;
  PangoIndicInfo *indic_info = NULL;

  g_return_if_fail (font != NULL);
  g_return_if_fail (text != NULL);
  g_return_if_fail (length >= 0);
  g_return_if_fail (analysis != NULL);

  face = pango_xft_font_lock_face (font);
  g_assert (face != NULL);

  indic_shape_engine = (PangoEngineShapeIndic *) analysis->shape_engine;

#if 1
  g_assert (indic_shape_engine->shapeEngine.engine.length == sizeof (PangoEngineShapeIndic));
#endif

  indic_info = indic_shape_engine->indicInfo;

  wc_in    = expand_text (text, length, &utf8_offsets, &n_chars);
  n_glyphs = indic_ot_reorder (wc_in, utf8_offsets, n_chars, indic_info->classTable, NULL, NULL, NULL);

  wc_out  = g_new (gunichar, n_glyphs);
  indices = g_new (glong,    n_glyphs);
  tags    = g_new (gulong,   n_glyphs);

  n_glyphs  = indic_ot_reorder (wc_in, utf8_offsets, n_chars, indic_info->classTable, wc_out, indices, tags);

  pango_glyph_string_set_size (glyphs, n_glyphs);
  set_glyphs(font, face, wc_out, indices, n_glyphs, glyphs);

  /* do gsub processing */
  gsub_ruleset = get_gsub_ruleset (face, indic_info);
  if (gsub_ruleset != NULL)
    {
      pango_ot_ruleset_shape (gsub_ruleset, glyphs, tags);
    }

  /* apply default positioning */
  for (i = 0; i < glyphs->num_glyphs; i += 1)
    {
      if (glyphs->glyphs[i].glyph != 0)
	{
	  PangoRectangle logical_rect;

	  pango_font_get_glyph_extents (font, glyphs->glyphs[i].glyph, NULL, &logical_rect);
	  glyphs->glyphs[i].geometry.width = logical_rect.width;
	}
      else
	{
	  glyphs->glyphs[i].geometry.width = 0;
	}

      glyphs->glyphs[i].geometry.x_offset = 0;
      glyphs->glyphs[i].geometry.y_offset = 0;
    }

#if 1
  /* do gpos processing */
  gpos_ruleset = get_gpos_ruleset (face, indic_info);
  if (gpos_ruleset != NULL)
    {
      pango_ot_ruleset_shape (gpos_ruleset, glyphs, tags);
    }
#endif

  pango_xft_font_unlock_face (font);

  g_free (tags);
  g_free (indices);
  g_free (wc_out);
  g_free (wc_in);
  g_free (utf8_offsets);
}

static PangoCoverage *
indic_engine_get_coverage (PangoFont  *font,
			   PangoLanguage *lang)
{
  return pango_font_get_coverage (font, lang);
}

static PangoEngine *
indic_engine_xft_new (gint index)
{
  PangoEngineShapeIndic *result;
  
  result = g_new (PangoEngineShapeIndic, 1);

  result->shapeEngine.engine.id     = script_engines[index].id;
  result->shapeEngine.engine.type   = PANGO_ENGINE_TYPE_SHAPE;
  result->shapeEngine.engine.length = sizeof (*result);
  result->shapeEngine.script_shape  = indic_engine_shape;
  result->shapeEngine.get_coverage  = indic_engine_get_coverage;

  result->indicInfo = &indic_info[index];

  return (PangoEngine *)result;
}

/* The following three functions provide the public module API for
 * Pango. If we are compiling it as a module, then we name the
 * entry points script_engine_list, etc. But if we are compiling
 * it for inclusion directly in Pango, then we need them to
 * to have distinct names for this module, so we prepend
 * _pango_indic_xft_
 */
#ifdef XFT_MODULE_PREFIX
#define MODULE_ENTRY(func) _pango_indic_xft_##func
#else
#define MODULE_ENTRY(func) func
#endif

/* List the engines contained within this module
 */
void 
MODULE_ENTRY(script_engine_list) (PangoEngineInfo **engines, gint *n_engines)
{
  *engines = script_engines;
  *n_engines = G_N_ELEMENTS (script_engines);
}

/* Load a particular engine given the ID for the engine
 */
PangoEngine *
MODULE_ENTRY(script_engine_load) (const char *id)
{
  gint i;

  for (i = 0; i < G_N_ELEMENTS(script_engines); i += 1)
    {
      if (!strcmp(id, script_engines[i].id))
	{
	  return indic_engine_xft_new(i);
	}
    }

  return NULL;
}

void 
MODULE_ENTRY(script_engine_unload) (PangoEngine *engine)
{
}
