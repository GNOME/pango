/* Pango - Tamil module
 * tamil.c:
 *
 * Copyright (C) 2000 Sivaraj D
 *
 */

#include <stdio.h>
#include <glib.h>
#include "pangox.h"
#include "taconv.h"
#include "pango-engine.h"
#include <string.h>

#define SCRIPT_ENGINE_NAME "TamilScriptEngineX"

static PangoEngineRange tamil_range[] = {
  { 0x0b80, 0x0bff, "*" },
};

static PangoEngineInfo script_engines[] = {
  {
    SCRIPT_ENGINE_NAME,
    PANGO_ENGINE_TYPE_SHAPE,
    PANGO_RENDER_TYPE_X,
    tamil_range, G_N_ELEMENTS(tamil_range)
  }
};

static gint n_script_engines = G_N_ELEMENTS (script_engines);

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
  gunichar *wc, *uni_str;
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
  
  n_chars = g_utf8_strlen (text, length);

  /* temporarily set the size to 3 times the number of unicode chars */
  pango_glyph_string_set_size (glyphs, n_chars * 3);

  wc = (gunichar *)g_malloc(sizeof(gunichar)*n_chars);  

  p = text;
  for (i=0; i < n_chars; i++)
    {
      wc[i] = g_utf8_get_char (p);
      p = g_utf8_next_char (p);
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
	cluster_start = g_utf8_next_char (cluster_start);
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
      gunichar i;

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

  result->engine.id = SCRIPT_ENGINE_NAME;
  result->engine.type = PANGO_ENGINE_TYPE_SHAPE;
  result->engine.length = sizeof (result);
  result->script_shape = tamil_engine_shape;
  result->get_coverage = tamil_engine_get_coverage;

  return (PangoEngine *)result;
}

/* The following three functions provide the public module API for
 * Pango
 */

#ifdef X_MODULE_PREFIX
#define MODULE_ENTRY(func) _pango_tamil_x_##func
#else
#define MODULE_ENTRY(func) func
#endif

void 
MODULE_ENTRY(script_engine_list) (PangoEngineInfo **engines, int *n_engines)
{
  *engines = script_engines;
  *n_engines = n_script_engines;
}

PangoEngine *
MODULE_ENTRY(script_engine_load) (const char *id)
{
  if (!strcmp (id, SCRIPT_ENGINE_NAME))
    return tamil_engine_x_new ();
  else
    return NULL;
}

void 
MODULE_ENTRY(script_engine_unload) (PangoEngine *engine)
{
}

