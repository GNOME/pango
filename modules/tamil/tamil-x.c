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

#define MEMBERS(strct) sizeof(strct) / sizeof(strct[1])

static PangoEngineRange tamil_range[] = {
  { 0x0b80, 0x0bff, "*" },
};

static PangoEngineInfo script_engines[] = {
  {
    "TamilScriptEngineLang",
    PANGO_ENGINE_TYPE_LANG,
    PANGO_RENDER_TYPE_NONE,
    tamil_range, MEMBERS(tamil_range)
  },
  {
    "TamilScriptEngineX",
    PANGO_ENGINE_TYPE_SHAPE,
    PANGO_RENDER_TYPE_X,
    tamil_range, MEMBERS(tamil_range)
  }
};

static gint n_script_engines = MEMBERS (script_engines);

/*
 * Language script engine
 */

static void 
tamil_engine_break (gchar            *text,
		    gint             len,
		    PangoAnalysis *analysis,
		    PangoLogAttr  *attrs)
{
/* Most of the code comes from pango_break
 * only difference is char stop based on modifiers
 */

  gchar *cur = text;
  gchar *next;
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
set_glyph (PangoGlyphString *glyphs, gint i, PangoCFont *cfont, PangoGlyphIndex glyph)
{
  gint width;

  glyphs->glyphs[i].font = cfont;
  glyphs->glyphs[i].glyph = glyph;
  
  glyphs->geometry[i].x_offset = 0;
  glyphs->geometry[i].y_offset = 0;

  glyphs->log_clusters[i] = i;

  pango_x_glyph_extents (&glyphs->glyphs[i],
			    NULL, NULL, &width, NULL, NULL, NULL, NULL);
  glyphs->geometry[i].width = width * 72;
}

static void 
tamil_engine_shape (PangoFont     *font,
		    gchar           *text,
		    gint             length,
		    PangoAnalysis *analysis,
		    PangoGlyphString    *glyphs)
{
  int n_chars, n_glyph;
  int i, j;
  char *p, *next;
  GUChar4 *wc, *uni_str;
  int res;
  unsigned char tsc_str[6];
  int ntsc, nuni;

  PangoCFont *tscii_font = NULL;

  g_return_if_fail (font != NULL);
  g_return_if_fail (text != NULL);
  g_return_if_fail (length >= 0);
  g_return_if_fail (analysis != NULL);

  tscii_font = pango_x_find_cfont (font, "tscii-0");
  pango_cfont_ref (tscii_font);
  
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

  j = 0;
  while (j < n_chars)
    {
      res = uni2tsc(uni_str, tsc_str, &nuni, &ntsc, n_chars - j, 6); 

      uni_str = uni_str + nuni;
      /* We need to differentiate between different return codes later */
      if (res != TA_SUCCESS)
        {
          set_glyph (glyphs, n_glyph, tscii_font, ' ');
          n_glyph++;
          j = j + nuni;
	  continue; 
        }
      for (i = 0; i < ntsc; i++)
        {
          set_glyph (glyphs, n_glyph, tscii_font, (PangoGlyphIndex) tsc_str[i]);
          n_glyph++;
        }
      j = j + nuni;
    }
	  
  pango_glyph_string_set_size (glyphs, n_glyph);

  if (tscii_font)
    pango_cfont_unref (tscii_font);
  g_free(wc);
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

  return (PangoEngine *)result;
}

/* The following three functions provide the public module API for
 * Pango
 */
void 
script_engine_list (PangoEngineInfo **engines, gint *n_engines)
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

