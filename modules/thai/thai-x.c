/* Pango
 * thai-x.c:
 *
 * Copyright (C) 1999 Red Hat Software
 * Author: Owen Taylor <otaylor@redhat.com>
 *
 * Software and Language Engineering Laboratory, NECTEC
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
#include "thai-shaper.h"


#define SCRIPT_ENGINE_NAME "ThaiScriptEngineX"

/* We handle the range U+0e01 to U+0e5b exactly
 */
static PangoEngineRange thai_ranges[] = {
  { 0x0e01, 0x0e5b, "*" },  /* Thai */
};

static PangoEngineInfo script_engines[] = {
  {
    SCRIPT_ENGINE_NAME,
    PANGO_ENGINE_TYPE_SHAPE,
    PANGO_RENDER_TYPE_X,
    thai_ranges, G_N_ELEMENTS(thai_ranges)
  }
};

/* Returns a structure with information we will use to rendering given the
 * #PangoFont. This is computed once per font and cached for later retrieval.
 */
ThaiFontInfo *
thai_get_font_info (PangoFont *font)
{
  static const char *charsets[] = {
    "tis620-2",
    "tis620-1",
    "tis620-0",
    "xtis620.2529-1",
    "xtis-0",
    "tis620.2533-1",
    "tis620.2529-1",
    "iso8859-11",
    "iso10646-1",
  };

  static const int charset_types[] = {
    THAI_FONT_TIS_WIN,
    THAI_FONT_TIS_MAC,
    THAI_FONT_TIS,
    THAI_FONT_XTIS,
    THAI_FONT_XTIS,
    THAI_FONT_TIS,
    THAI_FONT_TIS,
    THAI_FONT_TIS,
    THAI_FONT_ISO10646
  };
  
  ThaiFontInfo *font_info;
  GQuark info_id = g_quark_from_string ("thai-font-info");
  
  font_info = g_object_get_qdata (G_OBJECT (font), info_id);

  if (!font_info)
    {
      /* No cached information not found, so we need to compute it
       * from scratch
       */
      PangoXSubfont *subfont_ids;
      gint *subfont_charsets;
      gint n_subfonts, i;

      font_info = g_new (ThaiFontInfo, 1);
      font_info->font = font;
      font_info->font_set = THAI_FONT_NONE;
      
      g_object_set_qdata_full (G_OBJECT (font), info_id, font_info, (GDestroyNotify)g_free);
      
      n_subfonts = pango_x_list_subfonts (font, (char **)charsets, G_N_ELEMENTS (charsets),
					  &subfont_ids, &subfont_charsets);

      for (i=0; i < n_subfonts; i++)
	{
	  ThaiFontSet font_set = charset_types[subfont_charsets[i]];
	  
	  if (font_set != THAI_FONT_ISO10646 ||
	      pango_x_has_glyph (font, PANGO_X_MAKE_GLYPH (subfont_ids[i], 0xe01)))
	    {
	      font_info->font_set = font_set;
	      font_info->info_type = THAI_FONTINFO_X;
	      font_info->info.subfont = subfont_ids[i];

	      break;
	    }
	}

      g_free (subfont_ids);
      g_free (subfont_charsets);
    }

  return font_info;
}

PangoGlyph
thai_make_glyph (ThaiFontInfo *font_info, unsigned int c)
{
  return PANGO_X_MAKE_GLYPH (font_info->info.subfont, c);
}

PangoGlyph
thai_make_unknown_glyph (ThaiFontInfo *font_info, unsigned int c)
{
  return pango_x_get_unknown_glyph (font_info->font);
}

static PangoCoverage *
thai_engine_get_coverage (PangoFont  *font,
			   PangoLanguage *lang)
{
  PangoCoverage *result = pango_coverage_new ();
  
  ThaiFontInfo *font_info = thai_get_font_info (font);
  
  if (font_info->font_set != THAI_FONT_NONE)
    {
      gunichar wc;
      
      for (wc = 0xe01; wc <= 0xe3a; wc++)
	pango_coverage_set (result, wc, PANGO_COVERAGE_EXACT);
      for (wc = 0xe3f; wc <= 0xe5b; wc++)
	pango_coverage_set (result, wc, PANGO_COVERAGE_EXACT);
    }

  return result;
}

static PangoEngine *
thai_engine_x_new ()
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
 * _pango_thai_x_
 */
#ifdef X_MODULE_PREFIX
#define MODULE_ENTRY(func) _pango_thai_x_##func
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
    return thai_engine_x_new ();
  else
    return NULL;
}

void 
MODULE_ENTRY(script_engine_unload) (PangoEngine *engine)
{
}

