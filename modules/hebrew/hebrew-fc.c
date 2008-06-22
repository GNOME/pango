/* Pango
 * hebrew-fc.h: Hebrew shaper for FreeType-based backends
 *
 * Copyright (C) 2000, 2007 Red Hat Software
 * Authors:
 *   Owen Taylor <otaylor@redhat.com>
 *   Dov Grobgeld <dov.grobgeld@weizmann.ac.il>
 *   Behdad Esfahbod <behdad@behdad.org>
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

static const PangoOTFeatureMap gsub_features[] =
{
  {"ccmp", PANGO_OT_ALL_GLYPHS},
  {"locl", PANGO_OT_ALL_GLYPHS},
  {"rlig", PANGO_OT_ALL_GLYPHS},
  /* 'dlig' should be turned-on/off-able.  lets turn off for now. */
  /* {"dlig", PANGO_OT_ALL_GLYPHS}, */
};

static const PangoOTFeatureMap gpos_features[] =
{
  {"kern", PANGO_OT_ALL_GLYPHS},
  {"mark", PANGO_OT_ALL_GLYPHS},
  {"mkmk", PANGO_OT_ALL_GLYPHS}
};

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
  PangoOTRulesetDescription desc;
  const PangoOTRuleset *ruleset;
  PangoOTBuffer *buffer;
  guint n_gpos_features = 0;
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

  desc.script = analysis->script;
  desc.language = analysis->language;

  desc.n_static_gsub_features = G_N_ELEMENTS (gsub_features);
  desc.static_gsub_features = gsub_features;
  desc.n_static_gpos_features = G_N_ELEMENTS (gpos_features);
  desc.static_gpos_features = gpos_features;

  /* TODO populate other_features from analysis->extra_attrs */
  desc.n_other_features = 0;
  desc.other_features = NULL;

  ruleset = pango_ot_ruleset_get_for_description (pango_ot_info_get (face), &desc);

  pango_ot_ruleset_get_feature_count (ruleset, NULL, &n_gpos_features);
  if (n_gpos_features == 0)
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
      PangoGlyph glyph;

      wc = g_utf8_get_char (p);

      if (g_unichar_type (wc) != G_UNICODE_NON_SPACING_MARK)
	cluster = p - text;

      if (pango_is_zero_width (wc))
        glyph = PANGO_GLYPH_EMPTY;
      else
        {
	  gunichar c = wc;

	  if (analysis->level % 2)
	    g_unichar_get_mirror_char (c, &c);

	  glyph = pango_fc_font_get_glyph (fc_font, c);
	}

      if (!glyph)
	glyph = PANGO_GET_UNKNOWN_GLYPH (wc);

      pango_ot_buffer_add_glyph (buffer, glyph, 0, cluster);

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
