/* Pango
 * thai.c:
 *
 * Copyright (C) 1999 Red Hat Software
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

#include <iconv.h>

#include <glib.h>
#include "pango.h"
#include "pangox.h"
#include <fribidi/fribidi.h>

/* We handle the range U+0e01 to U+0e5b exactly
 */
static PangoEngineRange thai_ranges[] = {
  { 0x0e01, 0x0e5b, "*" },  /* Thai */
};

static PangoEngineInfo script_engines[] = {
  {
    "ThaiScriptEngineLang",
    PANGO_ENGINE_TYPE_LANG,
    PANGO_RENDER_TYPE_NONE,
    thai_ranges, G_N_ELEMENTS(thai_ranges)
  },
  {
    "ThaiScriptEngineX",
    PANGO_ENGINE_TYPE_SHAPE,
    PANGO_RENDER_TYPE_X,
    thai_ranges, G_N_ELEMENTS(thai_ranges)
  }
};

/*
 * Language script engine
 */

static void 
thai_engine_break (const char     *text,
		    gint            len,
		    PangoAnalysis  *analysis,
		    PangoLogAttr   *attrs)
{
}

static PangoEngine *
thai_engine_lang_new ()
{
  PangoEngineLang *result;
  
  result = g_new (PangoEngineLang, 1);

  result->engine.id = "ThaiScriptEngine";
  result->engine.type = PANGO_ENGINE_TYPE_LANG;
  result->engine.length = sizeof (result);
  result->script_break = thai_engine_break;

  return (PangoEngine *)result;
}

/*
 * X window system script engine portion
 */

typedef struct _ThaiFontInfo ThaiFontInfo;

/* The type of encoding that we will use
 */
typedef enum {
  THAI_FONT_NONE,
  THAI_FONT_XTIS,
  THAI_FONT_TIS,
  THAI_FONT_ISO10646
} ThaiFontType;

struct _ThaiFontInfo
{
  PangoFont   *font;
  ThaiFontType type;
  PangoXSubfont subfont;
};

/* All combining marks for Thai fall in the range U+0E30-U+0E50,
 * so we confine our data tables to that range, and use
 * default values for characters outside those ranges.
 */

/* Map from code point to group used for rendering with XTIS fonts
 * (0 == base character)
 */
static const char groups[32] = {
  0, 1, 0, 0, 1, 1, 1, 1,
  1, 1, 1, 0, 0, 0, 0, 0,
  0, 0, 0, 0, 0, 0, 0, 2,
  2, 2, 2, 2, 2, 2, 1, 0
};

/* Map from code point to index within group 1
 * (0 == no combining mark from group 1)
 */   
static const char group1_map[32] = {
  0, 1, 0, 0, 2, 3, 4, 5,
  6, 7, 8, 0, 0, 0, 0, 0,
  0, 0, 0, 0, 0, 0, 0, 0,
  0, 0, 0, 0, 0, 0, 0, 0
};

/* Map from code point to index within group 2
 * (0 == no combining mark from group 2)
 */   
static const char group2_map[32] = {
  0, 0, 0, 0, 0, 0, 0, 0,
  0, 0, 0, 0, 0, 0, 0, 0,
  0, 0, 0, 0, 0, 0, 0, 1,
  2, 3, 4, 5, 6, 7, 1, 0
};

/* Returns a structure with information we will use to rendering given the
 * #PangoFont. This is computed once per font and cached for later retrieval.
 */
static ThaiFontInfo *
get_font_info (PangoFont *font)
{
  static const char *charsets[] = {
    "xtis620.2529-1",
    "xtis-0",
    "tis620.2533-1",
    "tis620.2529-1",
    "iso8859-11",
    "iso10646-1",
  };

  static const int charset_types[] = {
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
      int *subfont_charsets;
      int n_subfonts, i;

      font_info = g_new (ThaiFontInfo, 1);
      font_info->font = font;
      font_info->type = THAI_FONT_NONE;
      
      g_object_set_qdata_full (G_OBJECT (font), info_id, font_info, (GDestroyNotify)g_free);
      
      n_subfonts = pango_x_list_subfonts (font, (char **)charsets, G_N_ELEMENTS (charsets),
					  &subfont_ids, &subfont_charsets);

      for (i=0; i < n_subfonts; i++)
	{
	  ThaiFontType font_type = charset_types[subfont_charsets[i]];
	  
	  if (font_type != THAI_FONT_ISO10646 ||
	      pango_x_has_glyph (font, PANGO_X_MAKE_GLYPH (subfont_ids[i], 0xe01)))
	    {
	      font_info->type = font_type;
	      font_info->subfont = subfont_ids[i];
	      
	      break;
	    }
	}

      g_free (subfont_ids);
      g_free (subfont_charsets);
    }

  return font_info;
}

static void
add_glyph (ThaiFontInfo     *font_info, 
	   PangoGlyphString *glyphs, 
	   int               cluster_start, 
	   PangoGlyph        glyph,
	   gboolean          combining)
{
  PangoRectangle ink_rect, logical_rect;
  int index = glyphs->num_glyphs;

  pango_glyph_string_set_size (glyphs, index + 1);
  
  glyphs->glyphs[index].glyph = glyph;
  glyphs->glyphs[index].attr.is_cluster_start = combining ? 0 : 1;
  
  glyphs->log_clusters[index] = cluster_start;

  pango_font_get_glyph_extents (font_info->font,
				glyphs->glyphs[index].glyph, &ink_rect, &logical_rect);

  if (combining)
    {
      glyphs->glyphs[index].geometry.width = 
	MAX (logical_rect.width, glyphs->glyphs[index - 1].geometry.width);
      glyphs->glyphs[index - 1].geometry.width = 0;
      glyphs->glyphs[index].geometry.x_offset = 0;
    }
  else
    {
      glyphs->glyphs[index].geometry.x_offset = 0;
      glyphs->glyphs[index].geometry.width = logical_rect.width;
    }
  
  glyphs->glyphs[index].geometry.y_offset = 0;
}

/* Return the glyph code within the font for the given Unicode Thai 
 * code pointer
 */
static int
get_glyph (ThaiFontInfo *font_info, gunichar wc)
{
  switch (font_info->type)
    {
    case THAI_FONT_NONE:
      return pango_x_get_unknown_glyph (font_info->font);
    case THAI_FONT_XTIS:
      return PANGO_X_MAKE_GLYPH (font_info->subfont, 0x100 * (wc - 0xe00 + 0x20) + 0x30);
    case THAI_FONT_TIS:
      return PANGO_X_MAKE_GLYPH (font_info->subfont, wc - 0xe00 + 0xA0);
    case THAI_FONT_ISO10646:
      return PANGO_X_MAKE_GLYPH (font_info->subfont, wc);
    }
  return 0;			/* Quiet GCC */
}

static void
add_cluster (ThaiFontInfo *font_info,
	     PangoGlyphString *glyphs,
	     int cluster_start,
	     gunichar base, 
	     gunichar group1,
	     gunichar group2)
{
  /* If we are rendering with an XTIS font, we try to find a precomposed
   * glyph for the cluster.
   */
  if (font_info->type == THAI_FONT_XTIS)
    {
      PangoGlyph glyph;
      int xtis_index = 0x100 * (base - 0xe00 + 0x20) + 0x30;
      if (group1)
	xtis_index +=8 * group1_map[group1 - 0xe30];
      if (group2)
	xtis_index += group2_map[group2 - 0xe30];
      
      glyph = PANGO_X_MAKE_GLYPH (font_info->subfont, xtis_index);

      if (pango_x_has_glyph (font_info->font, glyph))
	{
	  add_glyph (font_info, glyphs, cluster_start, glyph, FALSE);
	  return;
	}
    }

  /* If that failed, then we add compose the cluster out of three 
   * individual glyphs
   */
  add_glyph (font_info, glyphs, cluster_start, get_glyph (font_info, base), FALSE);
  if (group1)
    add_glyph (font_info, glyphs, cluster_start, get_glyph (font_info, group1), TRUE);
  if (group2)
    add_glyph (font_info, glyphs, cluster_start, get_glyph (font_info, group2), TRUE);
}

static void 
thai_engine_shape (PangoFont        *font,
		   const char       *text,
		   gint              length,
		   PangoAnalysis    *analysis,
		   PangoGlyphString *glyphs)
{
  ThaiFontInfo *font_info;
  const char *p;

  gunichar base = 0;
  gunichar group1 = 0;
  gunichar group2 = 0;
  int cluster_start = 0;

  pango_glyph_string_set_size (glyphs, 0);

  font_info = get_font_info (font);
  
  p = text;
  while (p < text + length)
    {
      int group;
      gunichar wc;

      wc = g_utf8_get_char (p);

      if (wc >= 0xe30 && wc < 0xe50)
	group = groups[wc - 0xe30];
      else
	group = 0;

      switch (group)
	{
	case 0:
	  if (base)
	    {
	      add_cluster (font_info, glyphs, cluster_start, base, group1, group2);
	      group1 = 0;
	      group2 = 0;
	    }
	  cluster_start = p - text;
	  base = wc;
	  break;
	case 1:
	  group1 = wc;
	  break;
	case 2:
	  group2 = wc;
	  break;
	}
      
      p = g_utf8_next_char (p);
    }

  if (base)
    add_cluster (font_info, glyphs, cluster_start, base, group1, group2);
}

static PangoCoverage *
thai_engine_get_coverage (PangoFont  *font,
			   const char *lang)
{
  PangoCoverage *result = pango_coverage_new ();
  
  ThaiFontInfo *font_info = get_font_info (font);
  
  if (font_info->type != THAI_FONT_NONE)
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

  result->engine.id = "ThaiScriptEngine";
  result->engine.type = PANGO_ENGINE_TYPE_LANG;
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
 * _pango_thai_
 */
#ifdef MODULE_PREFIX
#define MODULE_ENTRY(func) _pango_thai_##func
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
  if (!strcmp (id, "ThaiScriptEngineLang"))
    return thai_engine_lang_new ();
  else if (!strcmp (id, "ThaiScriptEngineX"))
    return thai_engine_x_new ();
  else
    return NULL;
}

void 
MODULE_ENTRY(script_engine_unload) (PangoEngine *engine)
{
}

