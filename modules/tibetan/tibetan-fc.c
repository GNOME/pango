/* Pango
 * tibetan-fc.c: Shaper for Tibetan script
 * based on thai-fc.c and basic-fc.c
 *
 * Copyright (C) 1999-2004 Red Hat Software
 * Author: Owen Taylor <otaylor@redhat.com>
 *
 * Copyright (C) 2004 Theppitak Karoonboonyanan <thep@linux.thai.net>
 *
 * Copyright (C) 2004 G Karunakar <karunakar@freedomink.org>
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

#include <glib.h>
#include <pango-engine.h>
#include "pango-ot.h"
#include "pangofc-font.h"

typedef PangoEngineShape	TibetanEngineFc;
typedef PangoEngineShapeClass	TibetanEngineFcClass;

#define SCRIPT_ENGINE_NAME "TibetanScriptEngineFc"
#define RENDER_TYPE PANGO_RENDER_TYPE_FC

static PangoEngineScriptInfo tibetan_scripts[] = {
  { PANGO_SCRIPT_TIBETAN, "*" }
};

static PangoEngineInfo script_engines[] = {
  {
    SCRIPT_ENGINE_NAME,
    PANGO_ENGINE_TYPE_SHAPE,
    RENDER_TYPE,
    tibetan_scripts, G_N_ELEMENTS(tibetan_scripts)
  }
};

/* GPOS tables are not present in Joyig font */
#undef DO_GPOS

static void
maybe_add_gsub_feature (PangoOTRuleset *ruleset,
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
      pango_ot_ruleset_add_feature (ruleset, PANGO_OT_TABLE_GSUB, feature_index,
				    property_bit);
    }
}

#ifdef DO_GPOS
static void
maybe_add_gpos_feature (PangoOTRuleset *ruleset,
		        PangoOTInfo    *info,
			guint           script_index,
			PangoOTTag      feature_tag,
			gulong          property_bit)
{
  guint feature_index;

  if (pango_ot_info_find_feature (info, PANGO_OT_TABLE_GPOS,
				  feature_tag, script_index, 0xffff, &feature_index))
    {
      pango_ot_ruleset_add_feature (ruleset, PANGO_OT_TABLE_GPOS, feature_index,
				    property_bit);
    }
}
#endif

static PangoOTRuleset *
get_gsub_ruleset (FT_Face face)
{
  PangoOTInfo    *info = pango_ot_info_get (face);
  GQuark          ruleset_quark = g_quark_from_string ("tibetan-gsub-ruleset");
  PangoOTRuleset *ruleset;

  if (!info)
    return NULL;

  ruleset = g_object_get_qdata (G_OBJECT (info), ruleset_quark);

  if (!ruleset)
    {
      PangoOTTag tibt_tag = FT_MAKE_TAG ('t', 'i', 'b', 't');
      guint      script_index;

      ruleset = pango_ot_ruleset_new (info);

     if (pango_ot_info_find_script (info, PANGO_OT_TABLE_GSUB,
				     tibt_tag, &script_index))
	{
	  maybe_add_gsub_feature (ruleset, info, script_index, FT_MAKE_TAG ('c','c','m','p'), 0xFFFF);
	  maybe_add_gsub_feature (ruleset, info, script_index, FT_MAKE_TAG ('b','l','w','s'), 0xFFFF);
	  maybe_add_gsub_feature (ruleset, info, script_index, FT_MAKE_TAG ('a','b','v','s'), 0xFFFF);
	}

      g_object_set_qdata_full (G_OBJECT (info), ruleset_quark, ruleset,
			       (GDestroyNotify)g_object_unref);
    }

  return ruleset;
}

static PangoOTRuleset *
get_gpos_ruleset (FT_Face face)
{
#ifdef DO_GPOS
  PangoOTInfo    *info = pango_ot_info_get (face);
  GQuark          ruleset_quark = g_quark_from_string ("tibetan-gpos-ruleset");
  PangoOTRuleset *ruleset;

  if (!info)
    return NULL;

  ruleset = g_object_get_qdata (G_OBJECT (info), ruleset_quark);

  if (!ruleset)
    {
      PangoOTTag tibetan_tag = FT_MAKE_TAG ('t', 'i', 'b', 't');
      guint      script_index;

      ruleset = pango_ot_ruleset_new (info);

      if (pango_ot_info_find_script (info, PANGO_OT_TABLE_GPOS,
				     tibetan_tag, &script_index))
	{
	  maybe_add_gpos_feature (ruleset, info, script_index, FT_MAKE_TAG ('k','e','r','n'), 0xFFFF);
	  maybe_add_gpos_feature (ruleset, info, script_index, FT_MAKE_TAG ('m','a','r','k'), 0xFFFF);
	  maybe_add_gpos_feature (ruleset, info, script_index, FT_MAKE_TAG ('m','k','m','k'), 0xFFFF);
	}

      g_object_set_qdata_full (G_OBJECT (info), ruleset_quark, ruleset,
			       (GDestroyNotify)g_object_unref);
    }

  return ruleset;
#else
  return NULL;
#endif
}

static void
set_glyph (PangoFont        *font,
           PangoGlyphString *glyphs,
           int               i,
           int               offset,
           PangoGlyph        glyph)
{
  PangoRectangle logical_rect;

  glyphs->glyphs[i].glyph = glyph;
  
  glyphs->glyphs[i].geometry.x_offset = 0;
  glyphs->glyphs[i].geometry.y_offset = 0;

  glyphs->log_clusters[i] = offset;

  pango_font_get_glyph_extents (font, glyphs->glyphs[i].glyph, NULL, 
				&logical_rect);
  glyphs->glyphs[i].geometry.width = logical_rect.width;
}

static void 
fallback_shape (PangoFont        *font,
		const char       *text,
		gint              length,
		PangoGlyphString *glyphs)
{
  PangoFcFont *fc_font = PANGO_FC_FONT (font);
  const char *p;
  long n_chars, i;

  n_chars = g_utf8_strlen (text, length);
  pango_glyph_string_set_size (glyphs, n_chars);
  
  for (i = 0, p = text; i < n_chars; i++, p = g_utf8_next_char (p))
    {
      gunichar wc;
      PangoGlyph index;

      wc = g_utf8_get_char (p);

      index = pango_fc_font_get_glyph (fc_font, wc);
      if (!index)
	index = pango_fc_font_get_unknown_glyph (fc_font, wc);
      
      set_glyph (font, glyphs, i, p - text, index);
    }
}

static void
ot_shape (PangoFont        *font,
	  PangoOTRuleset   *gsub_ruleset,
	  PangoOTRuleset   *gpos_ruleset,
	  const char       *text,
	  gint              length,
	  PangoGlyphString *glyphs)
{
  PangoFcFont *fc_font = PANGO_FC_FONT(font);
  PangoOTBuffer *buffer = pango_ot_buffer_new (fc_font);
  const char *p;
  
  for (p = text; p - text < length; p = g_utf8_next_char (p))
    {
      gunichar wc;
      PangoGlyph index;
      
      wc = g_utf8_get_char (p);
      
      index = pango_fc_font_get_glyph (fc_font, wc);	  
      if (!index)
	index = pango_fc_font_get_unknown_glyph (fc_font, wc);
      
      pango_ot_buffer_add_glyph (buffer, index, 0, p - text);
    }
  
  if (gsub_ruleset != NULL)
    pango_ot_ruleset_substitute (gsub_ruleset, buffer);
  
  if (gpos_ruleset != NULL)
    pango_ot_ruleset_position (gpos_ruleset, buffer);
  
  pango_ot_buffer_output (buffer, glyphs);
  pango_ot_buffer_destroy (buffer);
}

static void
tibetan_engine_shape (PangoEngineShape *engine,
		      PangoFont        *font,
		      const char       *text,
		      int               length,
		      PangoAnalysis    *analysis,
		      PangoGlyphString *glyphs)
{
  PangoFcFont *fc_font = PANGO_FC_FONT(font);
  PangoOTRuleset *gsub_ruleset;
  PangoOTRuleset *gpos_ruleset;
  FT_Face face;

  g_return_if_fail (length >= 0);

  face = pango_fc_font_lock_face (fc_font);
  g_assert (face != NULL);
  
  gsub_ruleset = get_gsub_ruleset (face);
  gpos_ruleset = get_gpos_ruleset (face);
  
  if (gsub_ruleset != NULL)
    ot_shape (font, gsub_ruleset, gpos_ruleset, text, length, glyphs);
  else
    fallback_shape (font, text, length, glyphs);

  pango_fc_font_unlock_face (fc_font);
}

static void
tibetan_engine_fc_class_init (PangoEngineShapeClass *class)
{
  class->script_shape = tibetan_engine_shape;
}

PANGO_ENGINE_SHAPE_DEFINE_TYPE (TibetanEngineFc, tibetan_engine_fc,
				tibetan_engine_fc_class_init, NULL);

void
PANGO_MODULE_ENTRY(init) (GTypeModule *module)
{
  tibetan_engine_fc_register_type (module);
}

void
PANGO_MODULE_ENTRY(exit) (void)
{
}

void
PANGO_MODULE_ENTRY(list) (PangoEngineInfo **engines,
			  int		   *n_engines)
{
  *engines = script_engines;
  *n_engines = G_N_ELEMENTS (script_engines);
}

PangoEngine *
PANGO_MODULE_ENTRY(create) (const char *id)
{
  if (!strcmp (id, SCRIPT_ENGINE_NAME))
    return g_object_new (tibetan_engine_fc_type, NULL);
  else
    return NULL;
}
