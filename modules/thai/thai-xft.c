/* Pango
 * thai-xft.c:
 *
 * Copyright (C) 1999 Red Hat Software
 * Author: Owen Taylor <otaylor@redhat.com>
 *
 * Copyright (C) 2002 NECTEC
 * Author: Theppitak Karoonboonyanan <thep@links.nectec.or.th>
 *
 * Copyright (c) 1996-2000 by Sun Microsystems, Inc.
 * Author: Chookij Vanatham <Chookij.Vanatham@Eng.Sun.COM>
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
#include "pango-engine.h"
#include "pangoxft.h"
#include "thai-shaper.h"

#define SCRIPT_ENGINE_NAME "ThaiScriptEngineXft"

/* We handle the range U+0e01 to U+0e5b exactly
 */
static PangoEngineRange thai_ranges[] = {
  { 0x0e01, 0x0e5b, "*" },  /* Thai */
};

static PangoEngineInfo script_engines[] = {
  {
    SCRIPT_ENGINE_NAME,
    PANGO_ENGINE_TYPE_SHAPE,
    PANGO_RENDER_TYPE_XFT,
    thai_ranges, G_N_ELEMENTS(thai_ranges)
  }
};

/* TIS-to-Unicode glyph maps for characters 0x80-0xff
 */
static int tis620_0[128] = {
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

static int tis620_1[128] = {
    0x00ab, 0x00bb, 0x2026, 0xf88c, 0xf88f, 0xf892, 0xf895, 0xf898,
    0xf88b, 0xf88e, 0xf891, 0xf894, 0xf897, 0x201c, 0x201d, 0xf899,
    /**/ 0, 0x2022, 0xf884, 0xf889, 0xf885, 0xf886, 0xf887, 0xf888,
    0xf88a, 0xf88d, 0xf890, 0xf893, 0xf896, 0x2018, 0x2019,      0,
    0x00a0, 0x0e01, 0x0e02, 0x0e03, 0x0e04, 0x0e05, 0x0e06, 0x0e07,
    0x0e08, 0x0e09, 0x0e0a, 0x0e0b, 0x0e0c, 0x0e0d, 0x0e0e, 0x0e0f,
    0x0e10, 0x0e11, 0x0e12, 0x0e13, 0x0e14, 0x0e15, 0x0e16, 0x0e17,
    0x0e18, 0x0e19, 0x0e1a, 0x0e1b, 0x0e1c, 0x0e1d, 0x0e1e, 0x0e1f,
    0x0e20, 0x0e21, 0x0e22, 0x0e23, 0x0e24, 0x0e25, 0x0e26, 0x0e27,
    0x0e28, 0x0e29, 0x0e2a, 0x0e2b, 0x0e2c, 0x0e2d, 0x0e2e, 0x0e2f,
    0x0e30, 0x0e31, 0x0e32, 0x0e33, 0x0e34, 0x0e35, 0x0e36, 0x0e37,
    0x0e38, 0x0e39, 0x0e3a, 0xfeff, 0x200b, 0x2013, 0x2014, 0x0e3f,
    0x0e40, 0x0e41, 0x0e42, 0x0e43, 0x0e44, 0x0e45, 0x0e46, 0x0e47,
    0x0e48, 0x0e49, 0x0e4a, 0x0e4b, 0x0e4c, 0x0e4d, 0x2122, 0x0e4f,
    0x0e50, 0x0e51, 0x0e52, 0x0e53, 0x0e54, 0x0e55, 0x0e56, 0x0e57,
    0x0e58, 0x0e59, 0x00ae, 0x00a9,      0,      0,      0,      0
};

static int tis620_2[128] = {
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

static int
contain_glyphs(PangoFont *font, const int glyph_map[128])
{
  unsigned char c;

  for (c = 0; c < 0x80; c++)
    {
      if (glyph_map[c])
        {
	  if (!pango_xft_font_has_char (font, glyph_map[c]))
            return 0;
        }
    }
  return 1;
}

/* Returns a structure with information we will use to rendering given the
 * #PangoFont. This is computed once per font and cached for later retrieval.
 */
ThaiFontInfo *
get_font_info (PangoFont *font)
{
  ThaiFontInfo *font_info;
  GQuark info_id = g_quark_from_string ("thai-font-info");
  
  font_info = g_object_get_qdata (G_OBJECT (font), info_id);

  if (!font_info)
    {
      /* No cached information not found, so we need to compute it
       * from scratch
       */
      font_info = g_new (ThaiFontInfo, 1);
      font_info->font = font;
  
      /* detect font set by determining availibility of glyphs */
      if (contain_glyphs(font, tis620_2))
        font_info->font_set = THAI_FONT_TIS_WIN;
      else if (contain_glyphs(font, tis620_1))
        font_info->font_set = THAI_FONT_TIS_MAC;
      else if (contain_glyphs(font, tis620_0))
        font_info->font_set = THAI_FONT_TIS;
      else
        font_info->font_set = THAI_FONT_ISO10646;
  
      g_object_set_qdata_full (G_OBJECT (font), info_id, font_info, (GDestroyNotify)g_free);
    }

  return font_info;
}

PangoGlyph
make_glyph (ThaiFontInfo *font_info, unsigned int c)
{
  int index;
  PangoGlyph result;

  switch (font_info->font_set) {
    case THAI_FONT_ISO10646:index = c; break;
    case THAI_FONT_TIS:     index = (c & 0x80) ? tis620_0[c & 0x7f] : c; break;
    case THAI_FONT_TIS_MAC: index = (c & 0x80) ? tis620_1[c & 0x7f] : c; break;
    case THAI_FONT_TIS_WIN: index = (c & 0x80) ? tis620_2[c & 0x7f] : c; break;
    default:                index = 0; break;
  }
  
  result = pango_xft_font_get_glyph (font_info->font, index);
  if (result)
    return result;
  else
    return pango_xft_font_get_unknown_glyph (font_info->font, index);
}

PangoGlyph
make_unknown_glyph (ThaiFontInfo *font_info, unsigned int c)
{
  return pango_xft_font_get_unknown_glyph (font_info->font, c);
}

static PangoCoverage *
thai_engine_get_coverage (PangoFont  *font,
			   PangoLanguage *lang)
{
  return pango_font_get_coverage (font, lang);
}

static PangoEngine *
thai_engine_xft_new ()
{
  PangoEngineShape *result;
  
  result = g_new (PangoEngineShape, 1);

  result->engine.id = SCRIPT_ENGINE_NAME;
  result->engine.type = PANGO_ENGINE_TYPE_SHAPE;
  result->engine.length = sizeof (result);
  result->script_shape = thai_engine_shape;
  result->get_coverage = thai_engine_get_coverage;

  return (PangoEngine *)result;
}

/* The following three functions provide the public module API for
 * Pango. If we are compiling it is a module, then we name the
 * entry points script_engine_list, etc. But if we are compiling
 * it for inclusion directly in Pango, then we need them to
 * to have distinct names for this module, so we prepend
 * _pango_thai_xft_
 */
#ifdef XFT_MODULE_PREFIX
#define MODULE_ENTRY(func) _pango_thai_xft_##func
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
  if (!strcmp (id, SCRIPT_ENGINE_NAME))
    return thai_engine_xft_new ();
  else
    return NULL;
}

void 
MODULE_ENTRY(script_engine_unload) (PangoEngine *engine)
{
}

