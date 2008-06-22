/* Pango
 * basic-fc.c: Basic shaper for FreeType-based backends
 *
 * Copyright (C) 2000, 2007 Red Hat Software
 * Authors:
 *   Owen Taylor <otaylor@redhat.com>
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

#include <glib/gprintf.h>
#include "pango-engine.h"
#include "pango-utils.h"
#include "pangofc-font.h"
#include "pango-ot.h"

/* No extra fields needed */
typedef PangoEngineShape      BasicEngineFc;
typedef PangoEngineShapeClass BasicEngineFcClass;

#define SCRIPT_ENGINE_NAME "BasicScriptEngineFc"
#define RENDER_TYPE PANGO_RENDER_TYPE_FC

static PangoEngineScriptInfo basic_scripts[] = {
  /* Listed in OpenType "Standard scripts" standard */
  { PANGO_SCRIPT_LATIN,    "*" },
  { PANGO_SCRIPT_CYRILLIC, "*" },
  { PANGO_SCRIPT_GREEK,    "*" },
  { PANGO_SCRIPT_ARMENIAN, "*" },
  { PANGO_SCRIPT_GEORGIAN, "*" },
  { PANGO_SCRIPT_RUNIC,    "*" },
  { PANGO_SCRIPT_OGHAM,    "*" },

  /* The following are simple and can be shaped easily too */

  { PANGO_SCRIPT_BOPOMOFO, "*" },
  { PANGO_SCRIPT_CHEROKEE, "*" },
  { PANGO_SCRIPT_COPTIC,   "*" },
  { PANGO_SCRIPT_DESERET,  "*" },
  { PANGO_SCRIPT_ETHIOPIC, "*" },
  { PANGO_SCRIPT_GOTHIC,   "*" },
  { PANGO_SCRIPT_HAN,      "*" },
  { PANGO_SCRIPT_HIRAGANA, "*" },
  { PANGO_SCRIPT_KATAKANA, "*" },
  { PANGO_SCRIPT_OLD_ITALIC, "*" },
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

  /* In fact any script we don't know how to shape can go here */
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

static const PangoOTFeatureMap gsub_features[] =
{
  {"ccmp", PANGO_OT_ALL_GLYPHS},
  {"locl", PANGO_OT_ALL_GLYPHS},
  {"liga", PANGO_OT_ALL_GLYPHS},
  {"clig", PANGO_OT_ALL_GLYPHS}
};

static const PangoOTFeatureMap gpos_features[] =
{
  {"kern", PANGO_OT_ALL_GLYPHS},
  {"mark", PANGO_OT_ALL_GLYPHS},
  {"mkmk", PANGO_OT_ALL_GLYPHS}
};

static const PangoOTFeatureMap vertical_gsub_features[] =
{
  {"ccmp", PANGO_OT_ALL_GLYPHS},
  {"locl", PANGO_OT_ALL_GLYPHS},
  {"vert", PANGO_OT_ALL_GLYPHS}
};

static const PangoOTFeatureMap vertical_gpos_features[] =
{
  {"vkrn", PANGO_OT_ALL_GLYPHS}
};

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
  PangoOTRulesetDescription desc;
  const PangoOTRuleset *ruleset;
  PangoOTBuffer *buffer;
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

  buffer = pango_ot_buffer_new (fc_font);
  pango_ot_buffer_set_rtl (buffer, analysis->level % 2 != 0);

  n_chars = g_utf8_strlen (text, length);
  pango_glyph_string_set_size (glyphs, n_chars);

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

  desc.script = analysis->script;
  desc.language = analysis->language;

  if (PANGO_GRAVITY_IS_VERTICAL (analysis->gravity))
    {
      desc.n_static_gsub_features = G_N_ELEMENTS (vertical_gsub_features);
      desc.static_gsub_features = vertical_gsub_features;
      desc.n_static_gpos_features = G_N_ELEMENTS (vertical_gpos_features);
      desc.static_gpos_features = vertical_gpos_features;
    }
  else
    {
      desc.n_static_gsub_features = G_N_ELEMENTS (gsub_features);
      desc.static_gsub_features = gsub_features;
      desc.n_static_gpos_features = G_N_ELEMENTS (gpos_features);
      desc.static_gpos_features = gpos_features;
    }

  /* TODO populate other_features from analysis->extra_attrs */
  desc.n_other_features = 0;
  desc.other_features = NULL;

  ruleset = pango_ot_ruleset_get_for_description (pango_ot_info_get (face), &desc);

  pango_ot_ruleset_substitute (ruleset, buffer);
  pango_ot_ruleset_position (ruleset, buffer);
  pango_ot_buffer_output (buffer, glyphs);

  pango_ot_buffer_destroy (buffer);

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
