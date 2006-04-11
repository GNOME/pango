/* Pango
 * hebrew-fc.h: Hebrew shaper for FreeType-based backends
 *
 * Copyright (C) 2000 Red Hat Software
 * Author: Owen Taylor <otaylor@redhat.com>
 *         Dov Grobgeld <dov.grobgeld@weizmann.ac.il> OpenType support.
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

#include <pango/pango-ot.h>
#include "pango-engine.h"
#include "pango-utils.h"
#include "pangofc-font.h"
#include "hebrew-shaper.h"

/* No extra fields needed */
typedef PangoEngineShape      HebrewEngineFc;
typedef PangoEngineShapeClass HebrewEngineFcClass ;

#define MAX_CLUSTER_CHRS	20

static PangoEngineScriptInfo hebrew_scripts[] = {
  { PANGO_SCRIPT_HEBREW, "*" }
};

#define SCRIPT_ENGINE_NAME "HebrewScriptEngineFc"
#define RENDER_TYPE PANGO_RENDER_TYPE_FC

static PangoEngineInfo script_engines[] = {
  {
    SCRIPT_ENGINE_NAME,
    PANGO_ENGINE_TYPE_SHAPE,
    RENDER_TYPE,
    hebrew_scripts, G_N_ELEMENTS(hebrew_scripts)
  }
};

static void
get_cluster_glyphs(PangoFont      *font,
		   gunichar       cluster[],
		   gint           cluster_size,
		   gboolean       do_mirror,
		   /* output */
		   gint           glyph_num[],
		   PangoGlyph     glyph[],
		   gint           widths[],
		   PangoRectangle ink_rects[])
{
  int i;
  for (i=0; i<cluster_size; i++)
    {
      PangoRectangle logical_rect;
      gunichar wc = cluster[i];
      gunichar mirrored_ch;
      
      if (do_mirror)
	if (pango_get_mirror_char (wc, &mirrored_ch))
	  wc = mirrored_ch;

      if (pango_is_zero_width (wc))
	glyph_num[i] = PANGO_GLYPH_EMPTY;
      else
        {
	  glyph_num[i] = pango_fc_font_get_glyph ((PangoFcFont *)font, wc);

	  if (!glyph_num[i])
	    glyph_num[i] = PANGO_GET_UNKNOWN_GLYPH ( wc);
	}

      glyph[i] = glyph_num[i];

      pango_font_get_glyph_extents (font,
				    glyph[i], &ink_rects[i], &logical_rect);
      
      /* Assign the base char width to the last character in the cluster */
      if (i==0)
	{
	  widths[i] = 0;
	  widths[cluster_size-1] = logical_rect.width;
	}
      else if (i < cluster_size-1)
	widths[i] = 0;
    }
}

static void
add_glyph (PangoGlyphString *glyphs, 
	   gint              cluster_start, 
	   PangoGlyph        glyph,
	   gboolean          is_combining,
	   gint              width,
	   gint              x_offset,
	   gint              y_offset
	   )
{
  gint index = glyphs->num_glyphs;

  pango_glyph_string_set_size (glyphs, index + 1);
  
  glyphs->glyphs[index].glyph = glyph;
  glyphs->glyphs[index].attr.is_cluster_start = is_combining ? 0 : 1;
  
  glyphs->log_clusters[index] = cluster_start;

  glyphs->glyphs[index].geometry.x_offset = x_offset;
  glyphs->glyphs[index].geometry.y_offset = y_offset;
  glyphs->glyphs[index].geometry.width = width;
}

static void
add_cluster(PangoFont        *font,
	    PangoGlyphString *glyphs,
	    int              cluster_size,
	    int              cluster_start,
	    int              glyph_num[],
	    PangoGlyph       glyph[],
	    int              width[],
	    int              x_offset[],
	    int              y_offset[])
{
  int i;

  for (i=0; i<cluster_size; i++)
    {
      add_glyph (glyphs, cluster_start, glyph[i],
		 i == 0 ? FALSE : TRUE, width[i], x_offset[i], y_offset[i]);
    }
}

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
  static const PangoOTTag hebr_tag = FT_MAKE_TAG ('h', 'e', 'b', 'r');
  guint script_index;

  PangoOTInfo *info = pango_ot_info_get (face);

  if (!ruleset_quark)
    ruleset_quark = g_quark_from_string ("pango-hebrew-ruleset");
  
  if (!info)
    return NULL;

  ruleset = g_object_get_qdata (G_OBJECT (info), ruleset_quark);

  if (!ruleset)
    {
      if (pango_ot_info_find_script (info, PANGO_OT_TABLE_GPOS,
				     hebr_tag, &script_index))
	{
	  ruleset = pango_ot_ruleset_new (info);

	  /* Again, tags from the SBL font. */
	  maybe_add_gpos_feature (ruleset, info, script_index,
				  FT_MAKE_TAG ('k','e','r','n'), 0xFFFF);
	  maybe_add_gpos_feature (ruleset, info, script_index,
				  FT_MAKE_TAG ('m','a','r','k'), 0xFFFF);
	}
      else
	/* Return NULL to trigger use of heuristics if there is no
	 * GPOS table in the font.
	 */
	return NULL;

      if (pango_ot_info_find_script (info, PANGO_OT_TABLE_GSUB,
				     hebr_tag, &script_index))
	{
	  /* Add the features that we want */
	  maybe_add_gsub_feature (ruleset, info, script_index,
				  FT_MAKE_TAG ('c','c','m','p'), 0xFFFF);

	  maybe_add_gsub_feature (ruleset, info, script_index,
				  FT_MAKE_TAG ('r','l','i','g'), 0xFFFF);
	}

      g_object_set_qdata_full (G_OBJECT (info), ruleset_quark, ruleset,
			       (GDestroyNotify)g_object_unref);
    }

  return ruleset;
}

static void 
fallback_shape (PangoEngineShape *engine,
		PangoFont        *font,
		const char       *text,
		gint              length,
		const PangoAnalysis *analysis,
		PangoGlyphString *glyphs)
{
  const char *p;
  const char *log_cluster;
  gunichar cluster[MAX_CLUSTER_CHRS];
  gint cluster_size;
  gint glyph_num[MAX_CLUSTER_CHRS];
  gint glyph_width[MAX_CLUSTER_CHRS], x_offset[MAX_CLUSTER_CHRS], y_offset[MAX_CLUSTER_CHRS];
  PangoRectangle ink_rects[MAX_CLUSTER_CHRS];
  PangoGlyph glyph[MAX_CLUSTER_CHRS];

  pango_glyph_string_set_size (glyphs, 0);

  p = text;
  while (p < text + length)
    {
      log_cluster = p;
      p = hebrew_shaper_get_next_cluster (p, text + length - p,
					  /* output */
					  cluster, &cluster_size);
      get_cluster_glyphs(font,
			 cluster,
			 cluster_size,
			 analysis->level % 2,
			 /* output */
			 glyph_num,
			 glyph,
			 glyph_width,
			 ink_rects);
      
      /* Kern the glyphs! */
      hebrew_shaper_get_cluster_kerning(cluster,
					cluster_size,
					/* Input and output */
					ink_rects,
					glyph_width,
					/* output */
					x_offset,
					y_offset);
      
      add_cluster(font,
		  glyphs,
		  cluster_size,
		  log_cluster - text,
		  glyph_num,
		  glyph,
		  glyph_width,
		  x_offset,
		  y_offset);
      
    }

  if (analysis->level % 2)
    hebrew_shaper_bidi_reorder(glyphs);
}

static void 
hebrew_engine_shape (PangoEngineShape *engine,
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
  gint unknown_property = 0;
  glong n_chars;
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
    
  n_chars = g_utf8_strlen (text, length);
      
  p = text;
  for (i=0; i < n_chars; i++)
    {
      gunichar wc;
      gunichar mirrored_ch;
      PangoGlyph index;
      char buf[6];

      wc = g_utf8_get_char (p);
      if (analysis->level % 2)
	if (pango_get_mirror_char (wc, &mirrored_ch))
	  {
	    wc = mirrored_ch;
	    
	    g_unichar_to_utf8 (wc, buf);
	  }

      if (pango_is_zero_width (wc))	/* Zero-width characters */
	{
	  pango_ot_buffer_add_glyph (buffer, PANGO_GLYPH_EMPTY, unknown_property, p - text);
	}
      else
	{
	  index = pango_fc_font_get_glyph (fc_font, wc);
	  
	  if (!index)
	    {
	      pango_ot_buffer_add_glyph (buffer, PANGO_GET_UNKNOWN_GLYPH ( wc),
					 unknown_property, p - text);
	    }
	  else
	    {
	      cluster = p - text;
	      
	      pango_ot_buffer_add_glyph (buffer, index,
					 unknown_property, cluster);
	    }
	}
      
      p = g_utf8_next_char (p);
    }
  

  pango_ot_ruleset_substitute (ruleset, buffer);
  pango_ot_ruleset_position (ruleset, buffer);
  pango_ot_buffer_output (buffer, glyphs);

  pango_ot_buffer_destroy (buffer);
  
 out:
  pango_fc_font_unlock_face (fc_font);
}

static void
hebrew_engine_fc_class_init (PangoEngineShapeClass *class)
{
  class->script_shape = hebrew_engine_shape;
}

PANGO_ENGINE_SHAPE_DEFINE_TYPE (HebrewEngineFc, hebrew_engine_fc,
				hebrew_engine_fc_class_init, NULL)

void 
PANGO_MODULE_ENTRY(init) (GTypeModule *module)
{
  hebrew_engine_fc_register_type (module);
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
    return g_object_new (hebrew_engine_fc_type, NULL);
  else
    return NULL;
}
