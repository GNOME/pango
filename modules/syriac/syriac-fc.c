/* Pango
 * syriac-fc.h:
 *
 * Copyright (C) 2000, 2003 Red Hat Software
 * Author: Owen Taylor <otaylor@redhat.com>
 *
 * Copyright (C) 2004 Emil Soleyman-Zomalan
 * Author: Emil Soleyman-Zomalan <emil@soleyman.com>
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
#include <config.h>
#include <string.h>

#include "syriac-ot.h"

#include "pango-engine.h"
#include "pango-utils.h"
#include "pangofc-font.h"

/* No extra fields needed */
typedef PangoEngineShape      SyriacEngineFc;
typedef PangoEngineShapeClass SyriacEngineFcClass ;

#define SCRIPT_ENGINE_NAME "SyriacScriptEngineFc"
#define RENDER_TYPE PANGO_RENDER_TYPE_FC

static PangoEngineScriptInfo syriac_scripts[] = {
  { PANGO_SCRIPT_SYRIAC, "*" },
};

static PangoEngineInfo script_engines[] = {
  {
    SCRIPT_ENGINE_NAME,
    PANGO_ENGINE_TYPE_SHAPE,
    RENDER_TYPE,
    syriac_scripts, G_N_ELEMENTS(syriac_scripts)
  }
};

static void
maybe_add_gsub_feature (PangoOTRuleset *ruleset,
			PangoOTInfo    *info,
			guint           script_index,
			PangoOTTag      tag,
			gulong          property_bit)
{
  guint feature_index;
  
  /* 0xffff == default language system */
  if (pango_ot_info_find_feature (info, PANGO_OT_TABLE_GSUB,
				  tag, script_index, 0xffff, &feature_index))
    pango_ot_ruleset_add_feature (ruleset, PANGO_OT_TABLE_GSUB, feature_index,
				  property_bit);
}

static void
maybe_add_gpos_feature (PangoOTRuleset *ruleset,
			PangoOTInfo    *info,
			guint           script_index,
			PangoOTTag      tag,
			gulong          property_bit)
{
  guint feature_index;
  
  /* 0xffff == default language system */
  if (pango_ot_info_find_feature (info, PANGO_OT_TABLE_GPOS,
				  tag, script_index, 0xffff, &feature_index))
    pango_ot_ruleset_add_feature (ruleset, PANGO_OT_TABLE_GPOS, feature_index,
				  property_bit);
}

static PangoOTRuleset *
get_ruleset (FT_Face face)
{
  PangoOTRuleset *ruleset;
  static GQuark ruleset_quark = 0;

  PangoOTInfo *info = pango_ot_info_get (face);

  if (!ruleset_quark)
    ruleset_quark = g_quark_from_string ("pango-syriac-ruleset");
  
  if (!info)
    return NULL;

  ruleset = g_object_get_qdata (G_OBJECT (info), ruleset_quark);

  if (!ruleset)
    {
      PangoOTTag syrc_tag = FT_MAKE_TAG ('s', 'y', 'r', 'c');
      guint script_index;

      ruleset = pango_ot_ruleset_new (info);

#define ALL_GLYPHS ~(gulong)0
		
      if (pango_ot_info_find_script (info, PANGO_OT_TABLE_GSUB,
				     syrc_tag, &script_index))
	{
	  maybe_add_gsub_feature (ruleset, info, script_index, FT_MAKE_TAG ('c','c','m','p'), ALL_GLYPHS);
	  maybe_add_gsub_feature (ruleset, info, script_index, FT_MAKE_TAG ('i','s','o','l'), isolated);
	  maybe_add_gsub_feature (ruleset, info, script_index, FT_MAKE_TAG ('f','i','n','a'), final);
	  maybe_add_gsub_feature (ruleset, info, script_index, FT_MAKE_TAG ('f','i','n','2'), final2);
	  maybe_add_gsub_feature (ruleset, info, script_index, FT_MAKE_TAG ('f','i','n','3'), final3);
	  maybe_add_gsub_feature (ruleset, info, script_index, FT_MAKE_TAG ('m','e','d','i'), medial);
	  maybe_add_gsub_feature (ruleset, info, script_index, FT_MAKE_TAG ('m','e','d','2'), medial2);
	  maybe_add_gsub_feature (ruleset, info, script_index, FT_MAKE_TAG ('i','n','i','t'), initial);
	  maybe_add_gsub_feature (ruleset, info, script_index, FT_MAKE_TAG ('r','l','i','g'), ALL_GLYPHS);
	  maybe_add_gsub_feature (ruleset, info, script_index, FT_MAKE_TAG ('c','a','l','t'), ALL_GLYPHS);
	  maybe_add_gsub_feature (ruleset, info, script_index, FT_MAKE_TAG ('l','i','g','a'), ALL_GLYPHS);
	  maybe_add_gsub_feature (ruleset, info, script_index, FT_MAKE_TAG ('d','l','i','g'), ALL_GLYPHS);
	}

      if (pango_ot_info_find_script (info, PANGO_OT_TABLE_GPOS,
				     syrc_tag, &script_index))
	{
	  maybe_add_gpos_feature (ruleset, info, script_index, FT_MAKE_TAG ('k','e','r','n'), ALL_GLYPHS);
	  maybe_add_gpos_feature (ruleset, info, script_index, FT_MAKE_TAG ('m','a','r','k'), ALL_GLYPHS);
	  maybe_add_gpos_feature (ruleset, info, script_index, FT_MAKE_TAG ('m','k','m','k'), ALL_GLYPHS);
	}

      g_object_set_qdata_full (G_OBJECT (info), ruleset_quark, ruleset,
			       (GDestroyNotify)g_object_unref);
    }

  return ruleset;
}

static void
swap_range (PangoGlyphString *glyphs, int start, int end)
{
  int i, j;
  
  for (i = start, j = end - 1; i < j; i++, j--)
    {
      PangoGlyphInfo glyph_info;
      gint log_cluster;
      
      glyph_info = glyphs->glyphs[i];
      glyphs->glyphs[i] = glyphs->glyphs[j];
      glyphs->glyphs[j] = glyph_info;
      
      log_cluster = glyphs->log_clusters[i];
      glyphs->log_clusters[i] = glyphs->log_clusters[j];
      glyphs->log_clusters[j] = log_cluster;
    }
}

static void
set_glyph (PangoFont        *font,
	   PangoGlyphString *glyphs,
	   int               i,
	   int               offset,
	   PangoGlyph        glyph)
{
  glyphs->glyphs[i].glyph = glyph;
  glyphs->log_clusters[i] = offset;
}

static void 
fallback_shape (PangoEngineShape *engine,
		PangoFont        *font,
		const char       *text,
		gint              length,
		const PangoAnalysis *analysis,
		PangoGlyphString *glyphs)
{
  PangoFcFont *fc_font = PANGO_FC_FONT (font);
  glong n_chars = g_utf8_strlen (text, length);
  const char *p;
  int i;
  
  pango_glyph_string_set_size (glyphs, n_chars);
  p = text;
  
  for (i=0; i < n_chars; i++)
    {
      gunichar wc;
      gunichar mirrored_ch;
      PangoGlyph index;

      wc = g_utf8_get_char (p);

      if ((analysis->level % 2) &&
	  pango_get_mirror_char (wc, &mirrored_ch))
	wc = mirrored_ch;

      if (pango_is_zero_width (wc))
	{
	  set_glyph (font, glyphs, i, p - text, PANGO_GLYPH_EMPTY);
	}
      else
	{
	  index = pango_fc_font_get_glyph (fc_font, wc);

	  if (!index)
	    index = PANGO_GET_UNKNOWN_GLYPH ( wc);

	  set_glyph (font, glyphs, i, p - text, index);
	}
      
      p = g_utf8_next_char (p);
    }

  /* Apply default positioning */
  for (i = 0; i < glyphs->num_glyphs; i++)
    {
      if (glyphs->glyphs[i].glyph)
	{
	  PangoRectangle logical_rect;
	  
	  pango_font_get_glyph_extents (font, glyphs->glyphs[i].glyph, NULL, &logical_rect);
	  glyphs->glyphs[i].geometry.width = logical_rect.width;
	}
      else
	glyphs->glyphs[i].geometry.width = 0;
      
      glyphs->glyphs[i].geometry.x_offset = 0;
      glyphs->glyphs[i].geometry.y_offset = 0;
    }
  
  if (analysis->level % 2 != 0)
    {
      /* Swap all glyphs */
      swap_range (glyphs, 0, glyphs->num_glyphs);
    }
}

static void 
syriac_engine_shape (PangoEngineShape *engine,
		     PangoFont        *font,
		     const char       *text,
		     gint              length,
		     const PangoAnalysis *analysis,
		     PangoGlyphString *glyphs)
{
  PangoFcFont *fc_font;
  FT_Face face;
  PangoOTRuleset *ruleset;
  PangoOTBuffer *buffer;
  gulong *properties = NULL;
  glong n_chars;
  gunichar *wcs;
  const char *p;
  int cluster = 0;
  int i;

  g_return_if_fail (font != NULL);
  g_return_if_fail (text != NULL);
  g_return_if_fail (length >= 0);
  g_return_if_fail (analysis != NULL);

  fc_font = PANGO_FC_FONT (font);
  face = pango_fc_font_lock_face (fc_font);
  if (!face)
    return;

  ruleset = get_ruleset (face);
  if (!ruleset)
    {
      fallback_shape (engine, font, text, length, analysis, glyphs);
      goto out;
    }

  buffer = pango_ot_buffer_new (fc_font);
  pango_ot_buffer_set_rtl (buffer, analysis->level % 2 != 0);
  pango_ot_buffer_set_zero_width_marks (buffer, TRUE);
    
  wcs = g_utf8_to_ucs4_fast (text, length, &n_chars);
  properties = g_new0 (gulong, n_chars);
      
  syriac_assign_properties (wcs, properties, n_chars);

  g_free (wcs);
  
  p = text;
  for (i=0; i < n_chars; i++)
    {
      gunichar wc;
      gunichar mirrored_ch;
      PangoGlyph index;

      wc = g_utf8_get_char (p);

      if ((analysis->level % 2) &&
	  pango_get_mirror_char (wc, &mirrored_ch))
	wc = mirrored_ch;

      if (pango_is_zero_width (wc))
	{
	  pango_ot_buffer_add_glyph (buffer, PANGO_GLYPH_EMPTY, properties[i], p - text);
	}
      else
	{
	  index = pango_fc_font_get_glyph (fc_font, wc);

	  if (!index)
	    {
	      pango_ot_buffer_add_glyph (buffer, PANGO_GET_UNKNOWN_GLYPH ( wc),
					 properties[i], p - text);
	    }
	  else
	    {
	      if (g_unichar_type (wc) != G_UNICODE_NON_SPACING_MARK)
		cluster = p - text;
		    
	      pango_ot_buffer_add_glyph (buffer, index,
					 properties[i], cluster);
	    }
	}
      
      p = g_utf8_next_char (p);
    }

  pango_ot_ruleset_substitute (ruleset, buffer);
  pango_ot_ruleset_position (ruleset, buffer);
  pango_ot_buffer_output (buffer, glyphs);
  
  g_free (properties);
  pango_ot_buffer_destroy (buffer);

 out:
  pango_fc_font_unlock_face (fc_font);
}

static void
syriac_engine_fc_class_init (PangoEngineShapeClass *class)
{
  class->script_shape = syriac_engine_shape;
}

PANGO_ENGINE_SHAPE_DEFINE_TYPE (SyriacEngineFc, syriac_engine_fc,
				syriac_engine_fc_class_init, NULL)

void 
PANGO_MODULE_ENTRY(init) (GTypeModule *module)
{
  syriac_engine_fc_register_type (module);
}

void 
PANGO_MODULE_ENTRY(exit) (void)
{
}

void 
PANGO_MODULE_ENTRY(list) (PangoEngineInfo **engines,
			  int              *n_engines)
{
  *engines = script_engines;
  *n_engines = G_N_ELEMENTS (script_engines);
}

PangoEngine *
PANGO_MODULE_ENTRY(create) (const char *id)
{
  if (!strcmp (id, SCRIPT_ENGINE_NAME))
    return g_object_new (syriac_engine_fc_type, NULL);
  else
    return NULL;
}
