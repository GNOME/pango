/* Pango - Myanmar module
 * myanmar.c:
 *
 * Copyright (C) 2000 SuSE Linux Ltd
 * Author: Robert Brady <robert@suse.co.uk>
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Library General Public
 * License as published by the Free Software Foundation; either
 * version 2 of the License, or (at your option) any later version.
 *
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Library General Public License for more details.
 *
 * You should have received a copy of the GNU Library General Public
 * License along with this library; if not, write to the
 * Free Software Foundation, Inc., 59 Temple Place - Suite 330,
 * Boston, MA 02111-1307, USA.
 */

/* 
 * This is the renderer for the Burmese (Myanmar) script. This 
 * script is used to write the following languages :
 * 
 *   my Burmese
 *  shn Shan
 *  mkh Mon
 *   pi Pali
 *
 */


#include <glib.h>
#include <stdio.h>
#include <string.h>

#include "pangox.h"
#include "pango-indic.h"
#include "pango-engine.h"

#define RANGE_START 0x1000
#define RANGE_SIZE 0x80
#define SCRIPT_STRING "Myanmar"
#define SCRIPT_ENGINE_NAME SCRIPT_STRING "ScriptEngineX"
#define VIRAMA 0x1039

#include "pango-indic-script.h"

static gboolean is_prefixing_vowel (gunichar i);
static gboolean is_vowel_sign (gunichar i);
static gunichar vowel_sign_to_matra (gunichar i);

static PangoIndicScript script = {
  SCRIPT_STRING,
  &is_prefixing_vowel,
  &is_vowel_sign,
  &vowel_sign_to_matra,
};

static char *default_charset = "iso10646-brm";

static PangoEngineRange pango_indic_range[] =
{
  {RANGE_START, RANGE_END, "*"},
};

SCRIPT_ENGINE_DEFINITION

static PangoCoverage *
pango_engine_get_coverage (PangoFont * font, PangoLanguage *lang)
{
  gunichar i;
  PangoCoverage *result = pango_coverage_new ();
  PangoXSubfont subfont;

  int dev_font = pango_x_find_first_subfont (font, &default_charset, 1, &subfont);

  if (dev_font)
    {
      for (i = RANGE_START; i <= RANGE_END; i++)
	pango_coverage_set (result, i, PANGO_COVERAGE_EXACT);

    }

  return result;
}

static gboolean
is_vowel_sign (gunichar i)
{
  /* one that combines, whether or not it spaces */
  return (i >= 0x102c && i <= 0x1032) || (i >= 0x1056 && i <= 0x1059);
}

static gunichar
vowel_sign_to_matra (gunichar i)
{
  if (i >= 0x102d && i <= 0x1032)
    return i - 0x102d + 0x1023;
  return i;
}

static int
is_consonant (int i)
{
  return (i >= 0x1000 && i <= 0x1020) || (i >= 0x1052 && i <= 0x1053);
}

static int
is_ind_vowel (int i)
{
  return (i >= 0x1021 && i <= 0x102a);
}

static int
is_prefixing_vowel (gunichar what) 
{
  return (what == 0x1031);
}

static void
pango_engine_shape (PangoFont * font,
			 const char *text,
			 int length,
			 PangoAnalysis * analysis, PangoGlyphString * glyphs)
{
  PangoXSubfont subfont;

  int n_chars, n_glyph;
  int lvl;
  int i;
  gunichar *wc;
  int sb;
  int n_syls;
  gunichar **syls = g_new (gunichar *, 2);

  g_return_if_fail (font != NULL);
  g_return_if_fail (text != NULL);
  g_return_if_fail (length >= 0);
  g_return_if_fail (analysis != NULL);

  n_chars = n_glyph = g_utf8_strlen (text, length);
  lvl = pango_x_find_first_subfont (font, &default_charset, 1, &subfont);
  if (!lvl)
    {
      pango_x_fallback_shape (font, glyphs, text, n_chars);
      return;
    }
  
  pango_indic_split_out_characters (&script, text, n_chars, &wc, 
				    &n_glyph, glyphs);
  pango_indic_convert_vowels (&script, FALSE, &n_glyph, wc, FALSE);

  n_syls = 1;
  syls[0] = wc;
  sb = glyphs->log_clusters[0];
  for (i = 0; i < n_glyph; i++)
    {
      if (i && (is_consonant (wc[i]) | is_ind_vowel (wc[i]))
	  && wc[i - 1] != VIRAMA)
	{
	  syls = g_renew (gunichar *, syls, n_syls + 2);
	  syls[n_syls] = wc + i;
	  n_syls++;
	  sb = glyphs->log_clusters[i];
	}
      glyphs->log_clusters[i] = sb;
    }
  syls[n_syls] = wc + i;

  for (i = 0; i < n_syls; i++)
    {
      pango_indic_shift_vowels (&script, syls[i], syls[i + 1]);
    }

  pango_indic_compact (&script, &n_glyph, wc, glyphs->log_clusters);
  while ((i=pango_x_apply_ligatures (font, subfont, &wc, &n_glyph, &glyphs->log_clusters))) 
    {
      pango_indic_compact (&script, &n_glyph, wc, glyphs->log_clusters);
    }
  pango_indic_compact (&script, &n_glyph, wc, glyphs->log_clusters);
  pango_glyph_string_set_size (glyphs, n_glyph);

  for (i = 0; i < n_glyph; i++)
    {
      PangoRectangle logical_rect;       
      glyphs->glyphs[i].glyph = PANGO_X_MAKE_GLYPH (subfont, wc[i]);
      pango_font_get_glyph_extents (font, glyphs->glyphs[i].glyph,
				    NULL, &logical_rect);
      glyphs->glyphs[i].geometry.x_offset = 0;
      glyphs->glyphs[i].geometry.y_offset = 0;
      glyphs->glyphs[i].geometry.width = logical_rect.width;
    }
  g_free (syls);
}

static PangoEngine *
pango_engine_x_new ()
{
  PangoEngineShape *result;
  result = g_new (PangoEngineShape, 1);
  result->engine.id = SCRIPT_ENGINE_NAME;
  result->engine.type = PANGO_ENGINE_TYPE_SHAPE;
  result->engine.length = sizeof (result);
  result->script_shape = pango_engine_shape;
  result->get_coverage = pango_engine_get_coverage;
  return (PangoEngine *) result;
}

#ifdef MYANMAR_X_MODULE_PREFIX
#define MODULE_ENTRY(func) _pango_myanmar_x_##func
#else
#define MODULE_ENTRY(func) func
#endif

void
MODULE_ENTRY(script_engine_list) (PangoEngineInfo ** engines, int *n_engines)
{
  *engines = script_engines;
  *n_engines = n_script_engines;
}

PangoEngine *
MODULE_ENTRY(script_engine_load) (const char *id)
{
  if (!strcmp (id, SCRIPT_ENGINE_NAME))
    return pango_engine_x_new ();
  else
    return NULL;
}

void
MODULE_ENTRY(script_engine_unload) (PangoEngine * engine)
{
}
