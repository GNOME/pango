/* Pango - Arabic module 
 * arabic module
 *
 * (C) 2000 K. Koehler  <koehler@or.uni-bonn.de>
 * 
 */

#include <stdio.h>
#include <glib.h>
#include "pango.h"
#include "pangox.h"
#include "utils.h"
#include <unicode.h>

#include "arconv.h" 

/* 
** I hope thing's work easily ...
*/

static PangoEngineRange arabic_range[] = {
  { 0x060B, 0x067F, "*" },
};

static PangoEngineInfo script_engines[] = {
  {
    "ArabicScriptEngineLang",
    PANGO_ENGINE_TYPE_LANG,
    PANGO_RENDER_TYPE_NONE,
    arabic_range, G_N_ELEMENTS(arabic_range)
  },
  {
    "ArabicScriptEngineX",
    PANGO_ENGINE_TYPE_SHAPE,
    PANGO_RENDER_TYPE_X,
    arabic_range, G_N_ELEMENTS(arabic_range)
  }
};

static gint n_script_engines = G_N_ELEMENTS (script_engines);




/*
 * Language script engine
 */

static void 
arabic_engine_break (const char   *text,
                     int            len,
                     PangoAnalysis *analysis,
                     PangoLogAttr  *attrs)
{
  /* Most of the code comes from tamil_engine_break
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
      attrs[i].is_char_stop = 1;
      attrs[i].is_word_stop = (i == 0) || attrs[i-1].is_white;

      i++;
    }
}

static PangoEngine *
arabic_engine_lang_new ()
{
  PangoEngineLang *result;
  
  result = g_new (PangoEngineLang, 1);

  result->engine.id = "ArabicScriptEngine";
  result->engine.type = PANGO_ENGINE_TYPE_LANG;
  result->engine.length = sizeof (result);
  result->script_break = arabic_engine_break;

  return (PangoEngine *)result;
}

/*
 * X window system script engine portion
 */

static PangoXSubfont
find_unic_font (PangoFont *font,char* charsets[])
{
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

static char *default_charset[] = {
  "iso10646-1"
};
  


static void
set_glyph (PangoGlyphString *glyphs, 
           PangoFont *font, PangoXSubfont subfont,
	   int i, int cluster_start, int glyph)
{
  PangoRectangle logical_rect;

  glyphs->glyphs[i].glyph = PANGO_X_MAKE_GLYPH (subfont, glyph);
  
  glyphs->glyphs[i].geometry.x_offset = 0;
  glyphs->glyphs[i].geometry.y_offset = 0;

  pango_font_get_glyph_extents (font, glyphs->glyphs[i].glyph, NULL, &logical_rect);
  glyphs->log_clusters[i] = cluster_start;
  if (arabic_isvowel(glyph))
      {
	  glyphs->glyphs[i].geometry.width = 0;
      }
  else
      {
	  glyphs->glyphs[i].geometry.width = logical_rect.width;
      }
}


/* The following thing is actually critical ... */

static void 
arabic_engine_shape (PangoFont        *font, 
                     const char       *text,
                     int               length,
                     PangoAnalysis    *analysis, 
                     PangoGlyphString *glyphs)
{
  PangoXSubfont subfont;

  int n_chars, n_glyph;
  int i;
  const char *p;
  const char *pold;
  const char *next;
  GUChar4     *wc;

  g_return_if_fail (font != NULL);
  g_return_if_fail (text != NULL);
  g_return_if_fail (length >= 0);
  g_return_if_fail (analysis != NULL);

  /* We assume we have an unicode-font like 10x20 which containes
  **   the needed chars 
  */
  
  n_chars = n_glyph = unicode_strlen (text, length);

  if (!(subfont = find_unic_font (font, default_charset)))
    {
      PangoGlyph unknown_glyph = pango_x_get_unknown_glyph (font);
          
      pango_glyph_string_set_size (glyphs, n_chars);

      p = text;
      for (i=0; i<n_chars; i++)
	{
	  set_glyph (glyphs, font, subfont, i, p - text, unknown_glyph);
	  p = unicode_next_utf8 (p);
	}

      return;
    }

  if (analysis->level % 2 == 0)
    {
      /* We were called on a LTR directional run (e.g. some numbers); 
	 fallback as simply as possible */

      pango_glyph_string_set_size (glyphs, n_chars);

      p = text;
      for (i=0; i < n_chars; i++)
	{
	  GUChar4 tmp_char;
	  
	  _pango_utf8_iterate (p, &next, &tmp_char);
	  set_glyph (glyphs, font, subfont, i, p - text, tmp_char);
	  p = next;
	}

      return;
    }

  wc = (GUChar4 *)g_malloc(sizeof(GUChar4)*n_chars);  

  p = text;
  for (i=0; i < n_chars; i++)
    {
      _pango_utf8_iterate (p, &next, &wc[n_chars - i - 1]);
      p = next;
    }
  
  
  reshape(&n_glyph,wc);
/*    raise(2); */

  pango_glyph_string_set_size (glyphs, n_glyph);

  p    = text;
  pold = p;
  i    = n_chars-1;
  while(i >= 0)
    {
	  if (wc[i] == 0)
	      {
		  p = unicode_next_utf8 (p);
		  i--;
	      }
	  else 
	      {
		  set_glyph (glyphs, font, subfont, n_glyph - 1, p - text, wc[i]);
		  if ( arabic_isvowel(wc[i])) 
		    {
		      glyphs->log_clusters[n_glyph-1] = pold - text;
		    }
      
		  pold = p;
		  p = unicode_next_utf8 (p);
		  n_glyph--;
		  i--;
	      };
    }

/*    if ((i != 0)||(n_glyph-1 != 0)){ */
/*      printf(" i= %x , n_glyph = %x "); */
/*    }; */
  g_free(wc);
}


static PangoCoverage *
arabic_engine_get_coverage (PangoFont  *font,
                            const char *lang)
{
  GUChar4 i;
  PangoCoverage *result = pango_coverage_new ();

  for (i = 0x60B; i <= 0x67E; i++)
    pango_coverage_set (result, i, PANGO_COVERAGE_EXACT);

  return result;
}

static PangoEngine *
arabic_engine_x_new ()
{
  PangoEngineShape *result;
  
  result = g_new (PangoEngineShape, 1);

  result->engine.id = "ArabicScriptEngine";
  result->engine.type = PANGO_ENGINE_TYPE_LANG;
  result->engine.length = sizeof (result);
  result->script_shape = arabic_engine_shape;
  result->get_coverage = arabic_engine_get_coverage;

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
  if (!strcmp (id, "ArabicScriptEngineLang"))
    return arabic_engine_lang_new ();
  else if (!strcmp (id, "ArabicScriptEngineX"))
    return arabic_engine_x_new ();
  else
    return NULL;
}

void 
script_engine_unload (PangoEngine *engine)
{
}
