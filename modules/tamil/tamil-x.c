/* Pango - Tamil module
 * tamil.c:
 *
 * Copyright (C) 2000 Sivaraj D
 *
 */

#include <stdio.h>
#include <glib.h>
#include "pango.h"
#include "pangox.h"
#include "utils.h"
#include "taconv.h"
#include <unicode.h>

static PangoEngineRange tamil_range[] = {
  { 0x0b80, 0x0bff, "*" },
};

static PangoEngineInfo script_engines[] = {
  {
    "TamilScriptEngineLang",
    PANGO_ENGINE_TYPE_LANG,
    PANGO_RENDER_TYPE_NONE,
    tamil_range, G_N_ELEMENTS(tamil_range)
  },
  {
    "TamilScriptEngineX",
    PANGO_ENGINE_TYPE_SHAPE,
    PANGO_RENDER_TYPE_X,
    tamil_range, G_N_ELEMENTS(tamil_range)
  }
};

static gint n_script_engines = G_N_ELEMENTS (script_engines);

/*
 * Language script engine
 */

static void 
tamil_engine_break (const char   *text,
		    int            len,
		    PangoAnalysis *analysis,
		    PangoLogAttr  *attrs)
{
/* Most of the code comes from pango_break
 * only difference is char stop based on modifiers
 */

  const char *cur = text;
  const char *next;
  gint i = 0;
  GUChar4 wc;

  while (*cur)
    {
      if (!_pango_utf8_iterate (cur, &next, &wc))
         return;
      if (cur == next)
        break;
      if ((next - text) > len)
        break;
      cur = next;

      attrs[i].is_white = (wc == ' ' || wc == '\t' || wc == 'n') ? 1 : 0;
      attrs[i].is_break = (i > 0 && attrs[i-1].is_white) || attrs[i].is_white;
      attrs[i].is_char_stop = (is_uni_modi(wc)) ? 0 : 1;
      attrs[i].is_word_stop = (i == 0) || attrs[i-1].is_white;

      i++;
    }
}

static PangoEngine *
tamil_engine_lang_new ()
{
  PangoEngineLang *result;
  
  result = g_new (PangoEngineLang, 1);

  result->engine.id = "TamilScriptEngine";
  result->engine.type = PANGO_ENGINE_TYPE_LANG;
  result->engine.length = sizeof (result);
  result->script_break = tamil_engine_break;

  return (PangoEngine *)result;
}

/*
 * X window system script engine portion
 */

/* We will need some type of kerning support for use with ikaram/iikaaram.
 * But we can live with this for time being 
 */
static void
set_glyph (PangoGlyphString *glyphs, int i, int cluster_start,
	   PangoFont *font, PangoXSubfont subfont, guint16 gindex)
{
  int width;
  PangoRectangle logical_rect;

  glyphs->glyphs[i].glyph = PANGO_X_MAKE_GLYPH (subfont, gindex);
  
  glyphs->glyphs[i].geometry.x_offset = 0;
  glyphs->glyphs[i].geometry.y_offset = 0;

  glyphs->log_clusters[i] = cluster_start;

  pango_font_get_glyph_extents (font, glyphs->glyphs[i].glyph, NULL, &logical_rect);
  glyphs->glyphs[i].geometry.width = logical_rect.width;
}

static PangoXSubfont
find_tscii_font (PangoFont *font)
{
  char *charsets[] = { "tscii-0" };
  PangoXSubfont *subfonts;
  int *subfont_charsets;
  int n_subfonts;
  PangoXSubfont result = 0;

  n_subfonts = pango_x_list_subfonts (font, charsets, 1, &subfonts, &subfont_charsets);

  if (n_subfonts > 0)
    result = subfonts[0];

  g_free (subfonts);
  g_free (subfont_charsets);

  return result;
}

static void 
tamil_engine_shape (PangoFont        *font,
		    const char       *text,
		    int               length,
		    PangoAnalysis    *analysis,
		    PangoGlyphString *glyphs)
{
  int n_chars, n_glyph;
  int i, j;
  const char *cluster_start;
  const char *p;
  const char *next;
  GUChar4 *wc, *uni_str;
  int res;
  unsigned char tsc_str[6];
  int ntsc, nuni;

  PangoXSubfont tscii_font;

  g_return_if_fail (font != NULL);
  g_return_if_fail (text != NULL);
  g_return_if_fail (length >= 0);
  g_return_if_fail (analysis != NULL);

  tscii_font = find_tscii_font (font);
  if (!tscii_font)
    {
      g_warning ("Cannot find a tscii font!\n");
      return;
    }
  
  n_chars = _pango_utf8_len (text, length);

  /* temporarily set the size to 3 times the number of unicode chars */
  pango_glyph_string_set_size (glyphs, n_chars * 3);
  wc = (GUChar4 *)g_malloc(sizeof(GUChar4)*n_chars);  

  p = text;
  for (i=0; i < n_chars; i++)
    {
      _pango_utf8_iterate (p, &next, &wc[i]);
      p = next;
    }

  n_glyph = 0;
  uni_str = wc;

  cluster_start = text;
  j = 0;
  while (j < n_chars)
    {
      res = uni2tsc(uni_str, tsc_str, &nuni, &ntsc, n_chars - j, 6); 

      uni_str = uni_str + nuni;
      /* We need to differentiate between different return codes later */
      if (res != TA_SUCCESS)
        {
          set_glyph (glyphs, n_glyph, cluster_start - text, font, tscii_font, ' ');
          n_glyph++;
          j = j + nuni;
	  continue; 
        }
      for (i = 0; i < ntsc; i++)
        {
          set_glyph (glyphs, n_glyph, cluster_start - text, font, tscii_font, (PangoGlyph) tsc_str[i]);
          n_glyph++;
        }
      j = j + nuni;
      while (nuni--)
	cluster_start = unicode_next_utf8 (cluster_start);
    }
	  
  pango_glyph_string_set_size (glyphs, n_glyph);

  g_free(wc);
}

static PangoCoverage *
tamil_engine_get_coverage (PangoFont  *font,
			   const char *lang)
{
  PangoCoverage *result = pango_coverage_new ();

  PangoXSubfont tscii_font = find_tscii_font (font);
  if (tscii_font)
    {
      GUChar4 i;

      for (i = 0xb80; i <= 0xbff; i++)
	pango_coverage_set (result, i, PANGO_COVERAGE_EXACT);
    }

  return result;
}

static PangoEngine *
tamil_engine_x_new ()
{
  PangoEngineShape *result;
  
  result = g_new (PangoEngineShape, 1);

  result->engine.id = "TamilScriptEngine";
  result->engine.type = PANGO_ENGINE_TYPE_LANG;
  result->engine.length = sizeof (result);
  result->script_shape = tamil_engine_shape;
  result->get_coverage = tamil_engine_get_coverage;

  return (PangoEngine *)result;
}

/* The following three functions provide the public module API for
 * Pango
 */
void 
script_engine_list (PangoEngineInfo **engines, int *n_engines)
{
  *engines = script_engines;
  *n_engines = n_script_engines;
}

PangoEngine *
script_engine_load (const char *id)
{
  if (!strcmp (id, "TamilScriptEngineLang"))
    return tamil_engine_lang_new ();
  else if (!strcmp (id, "TamilScriptEngineX"))
    return tamil_engine_x_new ();
  else
    return NULL;
}

void 
script_engine_unload (PangoEngine *engine)
{
}

