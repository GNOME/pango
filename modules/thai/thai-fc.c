/* Pango
 * thai-fc.c:
 *
 * Copyright (C) 1999, 2007 Red Hat Software
 * Copyright (C) 2002 NECTEC
 * Copyright (c) 1996-2000 by Sun Microsystems, Inc.
 * Authors:
 *   Owen Taylor <otaylor@redhat.com>
 *   Theppitak Karoonboonyanan <thep@links.nectec.or.th>
 *   Chookij Vanatham <Chookij.Vanatham@Eng.Sun.COM>
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

#include <glib.h>
#include "pango-ot.h"
#include "pango-engine.h"
#include "pangofc-font.h"

#include "thai-shaper.h"

/* No extra fields needed */
typedef PangoEngineShape      ThaiEngineFc;
typedef PangoEngineShapeClass ThaiEngineFcClass ;

#define SCRIPT_ENGINE_NAME "ThaiScriptEngineFc"
#define RENDER_TYPE PANGO_RENDER_TYPE_FC

/* We handle the range U+0e01 to U+0e5b exactly
 */
static PangoEngineScriptInfo thai_scripts[] = {
  { PANGO_SCRIPT_THAI, "*" },
  { PANGO_SCRIPT_LAO,  "*" },
};

static PangoEngineInfo script_engines[] = {
  {
    SCRIPT_ENGINE_NAME,
    PANGO_ENGINE_TYPE_SHAPE,
    RENDER_TYPE,
    thai_scripts, G_N_ELEMENTS(thai_scripts)
  }
};

/* TIS-to-Unicode glyph maps for characters 0x80-0xff
 */
static const int tis620_0[128] = {
    /**/ 0,      0,      0,      0,      0,      0,      0,      0,
    /**/ 0,      0,      0,      0,      0,      0,      0,      0,
    /**/ 0,      0,      0,      0,      0,      0,      0,      0,
    /**/ 0,      0,      0,      0,      0,      0,      0,      0,
    0x0020, 0x0e01, 0x0e02, 0x0e03, 0x0e04, 0x0e05, 0x0e06, 0x0e07,
    0x0e08, 0x0e09, 0x0e0a, 0x0e0b, 0x0e0c, 0x0e0d, 0x0e0e, 0x0e0f,
    0x0e10, 0x0e11, 0x0e12, 0x0e13, 0x0e14, 0x0e15, 0x0e16, 0x0e17,
    0x0e18, 0x0e19, 0x0e1a, 0x0e1b, 0x0e1c, 0x0e1d, 0x0e1e, 0x0e1f,
    0x0e20, 0x0e21, 0x0e22, 0x0e23, 0x0e24, 0x0e25, 0x0e26, 0x0e27,
    0x0e28, 0x0e29, 0x0e2a, 0x0e2b, 0x0e2c, 0x0e2d, 0x0e2e, 0x0e2f,
    0x0e30, 0x0e31, 0x0e32, 0x0e33, 0x0e34, 0x0e35, 0x0e36, 0x0e37,
    0x0e38, 0x0e39, 0x0e3a,      0,      0,      0,      0, 0x0e3f,
    0x0e40, 0x0e41, 0x0e42, 0x0e43, 0x0e44, 0x0e45, 0x0e46, 0x0e47,
    0x0e48, 0x0e49, 0x0e4a, 0x0e4b, 0x0e4c, 0x0e4d, 0x0e4e, 0x0e4f,
    0x0e50, 0x0e51, 0x0e52, 0x0e53, 0x0e54, 0x0e55, 0x0e56, 0x0e57,
    0x0e58, 0x0e59, 0x0e5a, 0x0e5b,      0,      0,      0,      0
};

static const int tis620_1[128] = {
    0xf89e,      0,      0, 0xf88c, 0xf88f, 0xf892, 0xf895, 0xf898,
    0xf88b, 0xf88e, 0xf891, 0xf894, 0xf897,      0,      0, 0xf899,
    0xf89a,      0, 0xf884, 0xf889, 0xf885, 0xf886, 0xf887, 0xf888,
    0xf88a, 0xf88d, 0xf890, 0xf893, 0xf896,      0,      0,      0,
    /**/ 0, 0x0e01, 0x0e02, 0x0e03, 0x0e04, 0x0e05, 0x0e06, 0x0e07,
    0x0e08, 0x0e09, 0x0e0a, 0x0e0b, 0x0e0c, 0x0e0d, 0x0e0e, 0x0e0f,
    0x0e10, 0x0e11, 0x0e12, 0x0e13, 0x0e14, 0x0e15, 0x0e16, 0x0e17,
    0x0e18, 0x0e19, 0x0e1a, 0x0e1b, 0x0e1c, 0x0e1d, 0x0e1e, 0x0e1f,
    0x0e20, 0x0e21, 0x0e22, 0x0e23, 0x0e24, 0x0e25, 0x0e26, 0x0e27,
    0x0e28, 0x0e29, 0x0e2a, 0x0e2b, 0x0e2c, 0x0e2d, 0x0e2e, 0x0e2f,
    0x0e30, 0x0e31, 0x0e32, 0x0e33, 0x0e34, 0x0e35, 0x0e36, 0x0e37,
    0x0e38, 0x0e39, 0x0e3a,      0,      0,      0,      0, 0x0e3f,
    0x0e40, 0x0e41, 0x0e42, 0x0e43, 0x0e44, 0x0e45, 0x0e46, 0x0e47,
    0x0e48, 0x0e49, 0x0e4a, 0x0e4b, 0x0e4c, 0x0e4d,      0, 0x0e4f,
    0x0e50, 0x0e51, 0x0e52, 0x0e53, 0x0e54, 0x0e55, 0x0e56, 0x0e57,
    0x0e58, 0x0e59,      0,      0, 0xf89b, 0xf89c, 0xf89d,      0
};

static const int tis620_2[128] = {
    0xf700, 0xf701, 0xf702, 0xf703, 0xf704, 0x2026, 0xf705, 0xf706,
    0xf707, 0xf708, 0xf709, 0xf70a, 0xf70b, 0xf70c, 0xf70d, 0xf70e,
    0xf70f, 0x2018, 0x2019, 0x201c, 0x201d, 0x2022, 0x2013, 0x2014,
    0xf710, 0xf711, 0xf712, 0xf713, 0xf714, 0xf715, 0xf716, 0xf717,
    0x00a0, 0x0e01, 0x0e02, 0x0e03, 0x0e04, 0x0e05, 0x0e06, 0x0e07,
    0x0e08, 0x0e09, 0x0e0a, 0x0e0b, 0x0e0c, 0x0e0d, 0x0e0e, 0x0e0f,
    0x0e10, 0x0e11, 0x0e12, 0x0e13, 0x0e14, 0x0e15, 0x0e16, 0x0e17,
    0x0e18, 0x0e19, 0x0e1a, 0x0e1b, 0x0e1c, 0x0e1d, 0x0e1e, 0x0e1f,
    0x0e20, 0x0e21, 0x0e22, 0x0e23, 0x0e24, 0x0e25, 0x0e26, 0x0e27,
    0x0e28, 0x0e29, 0x0e2a, 0x0e2b, 0x0e2c, 0x0e2d, 0x0e2e, 0x0e2f,
    0x0e30, 0x0e31, 0x0e32, 0x0e33, 0x0e34, 0x0e35, 0x0e36, 0x0e37,
    0x0e38, 0x0e39, 0x0e3a,      0,      0,      0,      0, 0x0e3f,
    0x0e40, 0x0e41, 0x0e42, 0x0e43, 0x0e44, 0x0e45, 0x0e46, 0x0e47,
    0x0e48, 0x0e49, 0x0e4a, 0x0e4b, 0x0e4c, 0x0e4d, 0x0e4e, 0x0e4f,
    0x0e50, 0x0e51, 0x0e52, 0x0e53, 0x0e54, 0x0e55, 0x0e56, 0x0e57,
    0x0e58, 0x0e59, 0x0e5a, 0x0e5b, 0xf718, 0xf719, 0xf71a,      0
};

static const int lao_0[128] = {
    /**/ 0,      0,      0,      0,      0,      0,      0,      0,
    /**/ 0,      0,      0,      0,      0,      0,      0,      0,
    /**/ 0,      0,      0,      0,      0,      0,      0,      0,
    /**/ 0,      0,      0,      0,      0,      0,      0,      0,
    0x0020, 0x0e81, 0x0e82,      0, 0x0e84,      0,      0, 0x0e87,
    0x0e88,      0, 0x0e8a,      0,      0, 0x0e8d,      0,      0,
	 0,      0,      0,      0, 0x0e94, 0x0e95, 0x0e96, 0x0e97,
    /**/ 0, 0x0e99, 0x0e9a, 0x0e9b, 0x0e9c, 0x0e9d, 0x0e9e, 0x0e9f,
    /**/ 0, 0x0ea1, 0x0ea2, 0x0ea3,      0, 0x0ea5,      0, 0x0ea7,
    /**/ 0,      0, 0x0eaa, 0x0eab,      0, 0x0ead, 0x0eae, 0x0eaf,
    0x0eb0, 0x0eb1, 0x0eb2, 0x0eb3, 0x0eb4, 0x0eb5, 0x0eb6, 0x0eb7,
    0x0eb8, 0x0eb9,      0, 0x0ebb, 0x0ebc, 0x0ebd,      0,      0,
    0x0ec0, 0x0ec1, 0x0ec2, 0x0ec3, 0x0ec4,      0, 0x0ec6,      0,
    0x0ec8, 0x0ec9, 0x0eca, 0x0ecb, 0x0ecc, 0x0ecd,      0,      0,
    0x0ed0, 0x0ed1, 0x0ed2, 0x0ed3, 0x0ed4, 0x0ed5, 0x0ed6, 0x0ed7,
    0x0ed8, 0x0ed9,      0,      0, 0x0edc, 0x0edd,      0,      0
};
static int
contain_glyphs(PangoFont *font, const int glyph_map[128])
{
  PangoFcFont *fc_font = (PangoFcFont *)font;
  unsigned char c;

  for (c = 0; c < 0x80; c++)
    {
      if (glyph_map[c])
	{
	  if (!pango_fc_font_has_char (fc_font, glyph_map[c]))
	    return 0;
	}
    }
  return 1;
}

/* Returns a structure with information we will use to rendering given the
 * #PangoFont. This is computed once per font and cached for later retrieval.
 */
static ThaiFontInfo *
thai_get_font_info (PangoFont            *font,
		    const PangoOTRuleset *ruleset)
{
  ThaiFontInfo *font_info;
  static GQuark info_id = 0;
  
  if (G_UNLIKELY (!info_id))
    info_id = g_quark_from_string ("thai-font-info");

  font_info = g_object_get_qdata (G_OBJECT (font), info_id);

  if (G_UNLIKELY (!font_info))
    {
      /* No cached information not found, so we need to compute it
       * from scratch
       */
      font_info = g_new (ThaiFontInfo, 1);
      font_info->font = font;

      /* detect font set by determining availibility of OT ruleset & glyphs */
      if (pango_ot_ruleset_get_feature_count (ruleset, NULL, NULL))
	font_info->font_set = THAI_FONT_TIS;
      else if (contain_glyphs(font, tis620_2))
	font_info->font_set = THAI_FONT_TIS_WIN;
      else if (contain_glyphs(font, tis620_1))
	font_info->font_set = THAI_FONT_TIS_MAC;
      else
	font_info->font_set = THAI_FONT_TIS;

      g_object_set_qdata_full (G_OBJECT (font), info_id, font_info, (GDestroyNotify)g_free);
    }

  return font_info;
}

static gunichar
get_glyph_index_tis (ThaiFontInfo *font_info, guchar c)
{
  if (!(c & 0x80))
    return lao_0[c];

  switch (font_info->font_set) {
    case THAI_FONT_TIS:     return tis620_0[c & 0x7f];
    case THAI_FONT_TIS_MAC: return tis620_1[c & 0x7f];
    case THAI_FONT_TIS_WIN: return tis620_2[c & 0x7f];
    default:                return 0;
  }
}

PangoGlyph
thai_get_glyph_tis (ThaiFontInfo *font_info, guchar c)
{
  return thai_get_glyph_uni (font_info, get_glyph_index_tis (font_info, c));
}

PangoGlyph
thai_make_glyph_tis (ThaiFontInfo *font_info, guchar c)
{
  return thai_make_glyph_uni (font_info, get_glyph_index_tis (font_info, c));
}

PangoGlyph
thai_get_glyph_uni (ThaiFontInfo *font_info, gunichar uc)
{
  return pango_fc_font_get_glyph ((PangoFcFont *)font_info->font, uc);
}

PangoGlyph
thai_make_glyph_uni (ThaiFontInfo *font_info, gunichar uc)
{
  PangoGlyph result;
  PangoFcFont *fc_font = (PangoFcFont *)font_info->font;

  result = pango_fc_font_get_glyph (fc_font, uc);
  if (result)
    return result;
  else
    return PANGO_GET_UNKNOWN_GLYPH ( uc);
}

static const PangoOTFeatureMap gsub_features[] =
{
  {"ccmp", PANGO_OT_ALL_GLYPHS},
  {"locl", PANGO_OT_ALL_GLYPHS},
  {"liga", PANGO_OT_ALL_GLYPHS},
};

static const PangoOTFeatureMap gpos_features[] =
{
  {"kern", PANGO_OT_ALL_GLYPHS},
  {"mark", PANGO_OT_ALL_GLYPHS},
  {"mkmk", PANGO_OT_ALL_GLYPHS}
};

static void
thai_engine_shape (PangoEngineShape *engine,
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
  gint i;
  ThaiFontInfo *font_info;

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

  font_info = thai_get_font_info (font, ruleset);

  thai_set_glyphs (font_info, text, length, analysis->script, glyphs); 

  buffer = pango_ot_buffer_new (PANGO_FC_FONT (font));

  for (i = 0; i < glyphs->num_glyphs; i++)
    pango_ot_buffer_add_glyph (buffer,
			       glyphs->glyphs[i].glyph,
			       0,
			       glyphs->log_clusters[i]);

  pango_ot_ruleset_substitute (ruleset, buffer);
  pango_ot_ruleset_position (ruleset, buffer);

  pango_ot_buffer_output (buffer, glyphs);
  pango_ot_buffer_destroy (buffer);
}

PangoGlyph
thai_make_unknown_glyph (ThaiFontInfo *font_info, gunichar uc)
{
  return PANGO_GET_UNKNOWN_GLYPH (uc);
}

static void
thai_engine_fc_class_init (PangoEngineShapeClass *class)
{
  class->script_shape = thai_engine_shape;
}

PANGO_ENGINE_SHAPE_DEFINE_TYPE (ThaiEngineFc, thai_engine_fc,
				thai_engine_fc_class_init, NULL)

void
PANGO_MODULE_ENTRY(init) (GTypeModule *module)
{
  thai_engine_fc_register_type (module);
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
    return g_object_new (thai_engine_fc_type, NULL);
  else
    return NULL;
}
