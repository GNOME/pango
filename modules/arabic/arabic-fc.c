/* Pango
 * arabic-fc.c: Arabic shaper for FreeType-based backends
 *
 * Copyright (C) 2000, 2003, 2007 Red Hat Software
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

#include "arabic-ot.h"

#include "pango-engine.h"
#include "pango-utils.h"
#include "pangofc-font.h"

/* No extra fields needed */
typedef PangoEngineShape      ArabicEngineFc;
typedef PangoEngineShapeClass ArabicEngineFcClass ;

#define SCRIPT_ENGINE_NAME "ArabicScriptEngineFc"
#define RENDER_TYPE PANGO_RENDER_TYPE_FC

static PangoEngineScriptInfo arabic_scripts[] = {
  { PANGO_SCRIPT_ARABIC, "*" },
  { PANGO_SCRIPT_NKO, "*" }
};

static PangoEngineInfo script_engines[] = {
  {
    SCRIPT_ENGINE_NAME,
    PANGO_ENGINE_TYPE_SHAPE,
    RENDER_TYPE,
    arabic_scripts, G_N_ELEMENTS(arabic_scripts)
  }
};

static const PangoOTFeatureMap gsub_features[] =
{
  {"ccmp", PANGO_OT_ALL_GLYPHS},
  {"locl", PANGO_OT_ALL_GLYPHS},
  {"isol", isolated},
  {"fina", final},
  {"medi", medial},
  {"init", initial},
  {"rlig", PANGO_OT_ALL_GLYPHS},
  {"calt", PANGO_OT_ALL_GLYPHS},
  {"liga", PANGO_OT_ALL_GLYPHS},
  /* 'dlig' should be turned-on/off-able.  lets turn off for now. */
  /* {"dlig", PANGO_OT_ALL_GLYPHS}, */
  {"cswh", PANGO_OT_ALL_GLYPHS},
  {"mset", PANGO_OT_ALL_GLYPHS}
};

static const PangoOTFeatureMap gpos_features[] =
{
  {"curs", PANGO_OT_ALL_GLYPHS},
  {"kern", PANGO_OT_ALL_GLYPHS},
  {"mark", PANGO_OT_ALL_GLYPHS},
  {"mkmk", PANGO_OT_ALL_GLYPHS}
};

static void
arabic_engine_shape (PangoEngineShape *engine,
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
  gulong *properties = NULL;
  glong n_chars;
  gunichar *wcs;
  const char *p;
  int cluster = 0;
  gboolean rtl = analysis->level % 2 != 0;
  gboolean reverse;
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
  pango_ot_buffer_set_rtl (buffer, rtl);
  pango_ot_buffer_set_zero_width_marks (buffer, TRUE);

  wcs = g_utf8_to_ucs4_fast (text, length, &n_chars);
  properties = g_new0 (gulong, n_chars);

  reverse = !rtl ^ (analysis->gravity == PANGO_GRAVITY_NORTH || analysis->gravity == PANGO_GRAVITY_WEST);
  Arabic_Assign_Properties (wcs, properties, n_chars, reverse);

  g_free (wcs);

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

	  /* Hack - Microsoft fonts are strange and don't contain the
	   * correct rules to shape ARABIC LETTER FARSI YEH in
	   * medial/initial position. It looks identical to ARABIC LETTER
	   * YEH in these positions, so we substitute if the font contains
	   * ARABIC LETTER YEH
	   */
	  if (c == 0x6cc &&
	      ((properties[i] & (initial | medial)) != (initial | medial)) &&
	      pango_fc_font_has_char (fc_font, 0x64a))
	    c = 0x64a;

	  glyph = pango_fc_font_get_glyph (fc_font, c);
	}

      if (!glyph)
	glyph = PANGO_GET_UNKNOWN_GLYPH (wc);

      pango_ot_buffer_add_glyph (buffer, glyph, properties[i], cluster);

      p = g_utf8_next_char (p);
    }

  g_free (properties);

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

  pango_ot_ruleset_substitute (ruleset, buffer);
  pango_ot_ruleset_position (ruleset, buffer);
  pango_ot_buffer_output (buffer, glyphs);

  pango_ot_buffer_destroy (buffer);

  pango_fc_font_unlock_face (fc_font);
}

static void
arabic_engine_fc_class_init (PangoEngineShapeClass *class)
{
  class->script_shape = arabic_engine_shape;
}

PANGO_ENGINE_SHAPE_DEFINE_TYPE (ArabicEngineFc, arabic_engine_fc,
				arabic_engine_fc_class_init, NULL)

void
PANGO_MODULE_ENTRY(init) (GTypeModule *module)
{
  arabic_engine_fc_register_type (module);
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
    return g_object_new (arabic_engine_fc_type, NULL);
  else
    return NULL;
}
