/* Pango
 * basic-fc.c: Basic shaper for FreeType-based backends
 *
 * Copyright (C) 2000 Red Hat Software
 * Author: Owen Taylor <otaylor@redhat.com>
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

#include <glib/gprintf.h>
#include "pango-engine.h"
#include "pango-utils.h"
#include "pangofc-font.h"
#include "pango-ot.h"

#include "basic-common.h"
  
/* No extra fields needed */
typedef PangoEngineShape      BasicEngineFc;
typedef PangoEngineShapeClass BasicEngineFcClass;

#define SCRIPT_ENGINE_NAME "BasicScriptEngineFc"
#define RENDER_TYPE PANGO_RENDER_TYPE_FC

static PangoEngineScriptInfo basic_scripts[] = {
  { PANGO_SCRIPT_ARMENIAN, "*" },
  { PANGO_SCRIPT_BOPOMOFO, "*" },
  { PANGO_SCRIPT_CHEROKEE, "*" },
  { PANGO_SCRIPT_COPTIC,   "*" },
  { PANGO_SCRIPT_CYRILLIC, "*" },
  { PANGO_SCRIPT_DESERET,  "*" },
  { PANGO_SCRIPT_ETHIOPIC, "*" },
  { PANGO_SCRIPT_GEORGIAN, "*" },
  { PANGO_SCRIPT_GOTHIC,   "*" },
  { PANGO_SCRIPT_GREEK,    "*" },
  { PANGO_SCRIPT_HAN,      "*" },
  { PANGO_SCRIPT_HIRAGANA, "*" },
  { PANGO_SCRIPT_KATAKANA, "*" },
  { PANGO_SCRIPT_LATIN,    "*" },
  { PANGO_SCRIPT_OGHAM,    "*" },
  { PANGO_SCRIPT_OLD_ITALIC, "*" },
  { PANGO_SCRIPT_RUNIC,     "*" },
  { PANGO_SCRIPT_CANADIAN_ABORIGINAL, "*" },
  { PANGO_SCRIPT_YI,       "*" },

  /* Unicode-4.0 additions */
  { PANGO_SCRIPT_BRAILLE,  "*" },
  { PANGO_SCRIPT_CYPRIOT,  "*" },
  { PANGO_SCRIPT_LIMBU,    "*" },
  { PANGO_SCRIPT_OSMANYA,  "*" },
  { PANGO_SCRIPT_SHAVIAN,  "*" },
  { PANGO_SCRIPT_LINEAR_B, "*" },
  { PANGO_SCRIPT_UGARITIC, "*" },

  /* Unicode-4.1 additions */
  { PANGO_SCRIPT_GLAGOLITIC, "*" },
    
  /* Unicode-5.0 additions */
  { PANGO_SCRIPT_CUNEIFORM,  "*" },
  { PANGO_SCRIPT_PHOENICIAN, "*" },

  { PANGO_SCRIPT_COMMON,   "" }
};

static PangoEngineInfo script_engines[] = {
  {
    SCRIPT_ENGINE_NAME,
    PANGO_ENGINE_TYPE_SHAPE,
    RENDER_TYPE,
    basic_scripts, G_N_ELEMENTS(basic_scripts)
  }
};

static void
swap_range (PangoGlyphString *glyphs,
	    int               start,
	    int               end)
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
  PangoRectangle logical_rect;

  glyphs->glyphs[i].glyph = glyph;
  
  glyphs->glyphs[i].geometry.x_offset = 0;
  glyphs->glyphs[i].geometry.y_offset = 0;

  glyphs->log_clusters[i] = offset;

  pango_font_get_glyph_extents (font, glyphs->glyphs[i].glyph, NULL, &logical_rect);
  glyphs->glyphs[i].geometry.width = logical_rect.width;
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
      char buf[6];

      wc = g_utf8_get_char (p);

      if (analysis->level % 2)
	if (pango_get_mirror_char (wc, &mirrored_ch))
	  {
	    wc = mirrored_ch;
	    
	    g_unichar_to_utf8 (wc, buf);
	  }

      if (wc == 0xa0)	/* non-break-space */
	wc = 0x20;

      if (pango_is_zero_width (wc))
	{
	  set_glyph (font, glyphs, i, p - text, PANGO_GLYPH_EMPTY);
	}
      else
	{
	  index = pango_fc_font_get_glyph (fc_font, wc);

	  if (!index)
            {
	      index = PANGO_GET_UNKNOWN_GLYPH ( wc);
              set_glyph (font, glyphs, i, p - text, index);
	    }
	  else
	    {
	      set_glyph (font, glyphs, i, p - text, index);
	      
	      if (g_unichar_type (wc) == G_UNICODE_NON_SPACING_MARK)
		{
		  if (i > 0)
		    {
		      PangoRectangle logical_rect, ink_rect;
		      
		      glyphs->glyphs[i].geometry.width = MAX (glyphs->glyphs[i-1].geometry.width,
							      glyphs->glyphs[i].geometry.width);
		      glyphs->glyphs[i-1].geometry.width = 0;
		      glyphs->log_clusters[i] = glyphs->log_clusters[i-1];
		      
		      /* Some heuristics to try to guess how overstrike glyphs are
		       * done and compensate
		       */
		      pango_font_get_glyph_extents (font, glyphs->glyphs[i].glyph, &ink_rect, &logical_rect);
		      if (logical_rect.width == 0 && ink_rect.x == 0)
			glyphs->glyphs[i].geometry.x_offset = (glyphs->glyphs[i].geometry.width - ink_rect.width) / 2;
		    }
		}
	    }
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

static const gchar scripts[][5] =
{
  "latn",
  "cyrl",
  "grek",
  "armn",
  "geor",
  "runr",
  "ogam"
};

static const gchar gsub_features[][5] =
{
  "ccmp",
  "liga",
  "clig",
};

static const gchar gpos_features[][5] =
{
  "kern",
  "mark",
  "mkmk"
};

static PangoOTRuleset *
get_ruleset (FT_Face face)
{
  PangoOTRuleset *ruleset;
  PangoOTInfo *info = NULL;
  static GQuark ruleset_quark = 0;
  unsigned int i, j;

  info = pango_ot_info_get (face);
  if (!info)
	  return NULL;

  if (!ruleset_quark)
	  ruleset_quark = g_quark_from_string ("pango-basic-ruleset");

  ruleset = g_object_get_qdata (G_OBJECT (info), ruleset_quark);

  if (!ruleset)
  {
    ruleset = pango_ot_ruleset_new (info);

    for (i = 0; i < G_N_ELEMENTS (scripts); i++)
      {
         PangoOTTag script_tag = FT_MAKE_TAG (scripts[i][0], scripts[i][1], scripts[i][2], scripts[i][3]);
         guint script_index;


         if (pango_ot_info_find_script (info, PANGO_OT_TABLE_GPOS, script_tag, &script_index))
           for (j = 0; j < G_N_ELEMENTS (gpos_features); j++)
	     {
               PangoOTTag feature_tag = FT_MAKE_TAG (gpos_features[j][0], gpos_features[j][1], 
                                                     gpos_features[j][2], gpos_features[j][3]);
               guint feature_index;

               /* 0xffff means default language */
               if (pango_ot_info_find_feature (info, PANGO_OT_TABLE_GPOS, feature_tag, script_index, 0xffff,&feature_index))
               {
                 pango_ot_ruleset_add_feature (ruleset, PANGO_OT_TABLE_GPOS, feature_index, 0xffff);
	       }
	     }

          if (pango_ot_info_find_script (info, PANGO_OT_TABLE_GSUB, script_tag, &script_index))
            for (j = 0; j < G_N_ELEMENTS (gsub_features); j++)
              {
                PangoOTTag feature_tag = FT_MAKE_TAG (gsub_features[j][0], gsub_features[j][1], 
                                                      gsub_features[j][2], gsub_features[j][3]);
                guint feature_index;

                /* 0xffff means default language */
                if (pango_ot_info_find_feature (info, PANGO_OT_TABLE_GSUB, feature_tag, 
                                                script_index, 0xffff, &feature_index))
                  {
                    pango_ot_ruleset_add_feature (ruleset, PANGO_OT_TABLE_GSUB, feature_index, 0xffff);
                  }
              }
      } 

    g_object_set_qdata_full (G_OBJECT (info), ruleset_quark, ruleset, (GDestroyNotify) g_object_unref);
  }

  return ruleset;

}

static void 
basic_engine_shape (PangoEngineShape *engine,
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
      pango_fc_font_kern_glyphs (fc_font, glyphs);
      goto out;
    }

  buffer = pango_ot_buffer_new (fc_font);
  pango_ot_buffer_set_rtl (buffer, analysis->level % 2 != 0);

  n_chars = g_utf8_strlen (text, length);
  pango_glyph_string_set_size (glyphs, n_chars);

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
	      if (g_unichar_type (wc) != G_UNICODE_NON_SPACING_MARK)
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
basic_engine_fc_class_init (PangoEngineShapeClass *class)
{
  class->script_shape = basic_engine_shape;
}

PANGO_ENGINE_SHAPE_DEFINE_TYPE (BasicEngineFc, basic_engine_fc,
				basic_engine_fc_class_init, NULL)

void 
PANGO_MODULE_ENTRY(init) (GTypeModule *module)
{
  basic_engine_fc_register_type (module);
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
    return g_object_new (basic_engine_fc_type, NULL);
  else
    return NULL;
}
