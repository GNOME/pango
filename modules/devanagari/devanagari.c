/* Pango - Devanagari module
 * devanagari.c:
 *
 * Copyright (C) 2000 Robert Brady <rwb197@zepler.org>
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

#include <glib.h>
#include <unicode.h>
#include <stdio.h>

#include "utils.h"
#include "pango.h"
#include "pangox.h"

#define VIRAMA 0x94d
#define CANDRA 0x901
#define ANUSWAR 0x902
#define NUKTA 0x93c
#define RA 0x930
#define JOINING_RA 0xe97f
#define REPHA 0xe97e
#define EYELASH_RA 0xe97d
#define RRA 0x931

#define U_S 0x941
#define UU_S 0x942

typedef struct _LigData LigData;

struct _LigData
  {
    int replacement;
    int source[3];
  };

static LigData ligatures[] =
{
#include "dev-ligatures.h"
};

static gint n_ligatures = G_N_ELEMENTS (ligatures);

static char *default_charset[] =
{
  "iso10646-dev",
  /* devanagari encoded in iso10646 way, with PUA used for
   * ligatures and half forms */
};

/* Table about ligatures in the font. This should come from the font
 * somehow : this needs to be co-ordinated with fonts@xfree86.org.
 * (and for whatever passes for a font working group at X.org)
 */

static PangoEngineRange devanagari_range[] =
{
  {0x900, 0x97f, "*"}
};

static PangoEngineInfo script_engines[] =
{
  {
    "DevanagariScriptEngineLang",
    PANGO_ENGINE_TYPE_LANG,
    PANGO_RENDER_TYPE_NONE,
    devanagari_range, G_N_ELEMENTS (devanagari_range)},
  {
    "DevanagariScriptEngineX",
    PANGO_ENGINE_TYPE_SHAPE,
    PANGO_RENDER_TYPE_X,
    devanagari_range, G_N_ELEMENTS (devanagari_range)}
};

static gint n_script_engines = G_N_ELEMENTS (script_engines);

static gboolean
  find_unic_font (PangoFont * font, char *charsets[], PangoXSubfont * rfont);

static PangoCoverage *
devanagari_engine_get_coverage (PangoFont * font, const char *lang)
{
  GUChar4 i;
  PangoCoverage *result = pango_coverage_new ();
  PangoXSubfont subfont;

  int dev_font = find_unic_font (font, default_charset, &subfont);

  if (dev_font)
    {
      for (i = 0x900; i < 0x97f; i++)
	pango_coverage_set (result, i, PANGO_COVERAGE_EXACT);
    }

  return result;
}

static gboolean
find_unic_font (PangoFont * font, char *charsets[], PangoXSubfont * rfont)
{
  int n_subfonts;
  int result = 0;
  PangoXSubfont *subfonts;
  int *subfont_charsets;
  n_subfonts = pango_x_list_subfonts (font, charsets, 1,
				      &subfonts, &subfont_charsets);

  if (n_subfonts > 0)
    {
      rfont[0] = subfonts[0];
      result = 1;
    }

  g_free (subfonts);
  g_free (subfont_charsets);
  return result;
}

static int
is_ligating_consonant (int ch)
{
  /* false for 958 to 961, as these don't ligate in any way */
  return (ch >= 0x915 && ch <= 0x939);
}

static int
is_comb_vowel (int i)
{
  /* one that combines, whether or not it spaces */
  return (i >= 0x93E && i <= 0x94c) || (i >= 0x962 && i <= 0x963);
}

static int
vowelsign_to_letter (int i)
{
  if (i >= 0x93e && i <= 0x94c)
    return i - 0x93e + 0x906;
  return i;
}

static int
is_half_consonant (int i)
{
  return (i >= 0xe915 && i <= 0xe939) || (i >= 0xe970 && i <= 0xe976);
}

static int
is_consonant (int i)
{
  return (i >= 0x915 && i <= 0x939) || (i >= 0x958 && i <= 0x95f);
}

static int
is_ind_vowel (int i)
{
  return (i >= 0x905 && i <= 0x914);
}

static int
is_nonspacing_vowel (GUChar4 c)
{
  /* one that doesn't space. ie 93f and 940 don't count */
  return (c >= 0x941 && c <= 0x948) || (c >= 0x962 && c <= 0x963);
}

static int
get_char (GUChar4 * chars, GUChar4 * end)
{
  if (chars >= end)
    return 0;
  return *chars;
}

static void
devanagari_shift_vowels (GUChar4 * chars, GUChar4 * end)
{
  /* moves 0x93f (I) before consonant clusters where appropriate. */
  GUChar4 *strt = chars;
  while (chars < end)
    {
      if (*chars == 0x93f && chars > strt)
	{
	  GUChar4 *bubble = chars;
	  int i = 1;

	  /* move back TO START! */

	  while (bubble > strt)
	    {
	      bubble[0] = bubble[-1];
	      bubble[-1] = 0x93f;
	      i = 0;
	      bubble--;
	    }
	}
      chars++;
    }
}

void
devanagari_convert_vowels (int *num, GUChar4 * chars)
{
  /* goes along and converts matras to vowel letters if needed.
   * this is only currently done at the beginning of the string. */
  GUChar4 *end = chars + *num;
  GUChar4 *start = chars;
  while (chars < end)
    {
      if ((chars == start && is_comb_vowel (chars[0])) ||
	  (chars != start && is_comb_vowel (chars[0])
	   && is_comb_vowel (chars[-1])))
	{
	  chars[0] = vowelsign_to_letter (chars[0]);
	}
      chars++;
    }
}

static void
devanagari_compact (int *num, GUChar4 * chars, gint * cluster)
{
  /* shuffle stuff up into the blanked out elements. */
  GUChar4 *dest = chars;
  GUChar4 *end = chars + *num;
  gint *cluster_dest = cluster;
  while (chars < end)
    {
      if (*chars)
	{
	  *dest = *chars;
	  *cluster_dest = *cluster;
	  dest++;
	  chars++;
	  cluster++;
	  cluster_dest++;
	}
      else
	{
	  chars++;
	  cluster++;
	}
    }
  *num -= (chars - dest);
}

#if 0
const char *foo[] =
{
  "k", "kh", "g", "gh", "ng",
  "c", "ch", "j", "jh", "ny",
  "tt", "tth", "dd", "ddh", "nn",
  "t", "th", "d", "dh", "n", "nnn",
  "p", "ph", "b", "bh", "m",

  "y", "r", "rr", "l", "ll", "lll",

  "v", "sh", "ss", "s", "h",

  "-", "-", "-", "-",

  "aa",
  "i", "ii",
  "u", "uu",
  "[r]", "[rr]",
  "[e]", "{e}",
  "e", "ai",
  "[o]", "{o}",
  "o", "au",
};

const char *bar[] =
{
  "A", "AA",
  "I", "II",
  "U", "UU",
  "[R]", "[RR]",
  "[E]", "{E}",
  "E", "AI",
  "[O]", "{O}",
  "O", "AU",
};
#endif

static void
devanagari_make_ligs (GUChar4 * start, GUChar4 * end, int *cluster)
{
  GUChar4 t0 = get_char (start, end);
  GUChar4 t1 = get_char (start + 1, end);
  GUChar4 t2 = get_char (start + 2, end);
  GUChar4 t3 = get_char (start + 3, end);

  int i, j;
  int repha = 0, ligature = 0;

  for (i = 0; i < (end - start); i++)
    {
      t0 = get_char (start + i, end);
      t1 = get_char (start + 1 + i, end);
      t2 = get_char (start + 2 + i, end);
      t3 = get_char (start + 3 + i, end);

      if (!ligature)
	{
	  for (j = 0; j < n_ligatures; j++)
	    {
	      /* handle the conjuncts */
	      LigData *l = ligatures + j;
	      if (t0 == l->source[0] && t1 == l->source[1]
		  && t2 == l->source[2])
		{
		  start[i + 0] = 0;
		  start[i + 1] = 0;
		  start[i + 2] = l->replacement;
		  ligature = 1;
		  break;
		}
	    }
	  if (j != n_ligatures)
	    continue;
	}

      if ((t0 >= 0xe900 && t0 <= 0xe906) && t1 == VIRAMA
	  && is_ligating_consonant (t2))
	{
	  start[i + 1] = start[i] + 0x70;
	  start[i] = 0;
	  continue;
	}

      if (is_consonant (t0) && t1 == VIRAMA && t2 == RA)
	{
	  start[i + 1] = 0;
	  start[i + 2] = JOINING_RA;
	  continue;
	}

      if (t0 == RRA && t1 == VIRAMA)
	{
	  start[i] = 0;
	  start[i + 1] = EYELASH_RA;
	  continue;
	}

      if (t0 == RA && t1 == VIRAMA && is_ligating_consonant (t2))
	{

	  start[i + 0] = 0;
	  start[i + 1] = 0;
	  start[i + 2] = t2;
	  repha = 1;
	  continue;
	}

      if (is_ligating_consonant (t0) &&
	  t1 == VIRAMA && is_ligating_consonant (t2))
	{
	  start[i + 0] = t0 + 0xe000;
	  start[i + 1] = 0;
	  start[i + 2] = t2;
	  continue;
	}

      if (t0 == RA && (t1 == U_S || t1 == UU_S))
	{

	  if (t1 == U_S)
	    start[i + 1] = 0xe90e;

	  if (t1 == UU_S)
	    start[i + 1] = 0xe90f;

	  start[i] = 0;

	}
    }

  for (i = 0; i < (end - start); i++)
    {
      t0 = get_char (start + i, end);
      t1 = get_char (start + 1 + i, end);
      t2 = get_char (start + 2 + i, end);
      t3 = get_char (start + 3 + i, end);
    }

  if (repha)
    {
      int src = 0, dest = 0;
      while (src < (end - start))
	{
	  start[dest] = start[src];
	  src++;
	  if (start[dest])
	    dest++;
	}
      while (dest < (end - start))
	{
	  start[dest] = 0;
	  dest++;
	}
      end[-1] = REPHA;
    }
}

static void
devanagari_engine_shape (PangoFont * font,
			 const char *text,
			 int length,
			 PangoAnalysis * analysis, PangoGlyphString * glyphs)
{
  PangoXSubfont subfont;

  int n_chars, n_glyph;
  int lvl;
  const char *p, *next;
  int i;
  GUChar4 *wc;
  int sb;
  int n_syls;
  GUChar4 **syls = g_malloc (sizeof (GUChar4 **));

  g_return_if_fail (font != NULL);
  g_return_if_fail (text != NULL);
  g_return_if_fail (length >= 0);
  g_return_if_fail (analysis != NULL);

  n_chars = n_glyph = unicode_strlen (text, length);
  lvl = find_unic_font (font, default_charset, &subfont);
  if (!lvl)
    {
      PangoGlyph unknown_glyph = pango_x_get_unknown_glyph (font);
      PangoRectangle logical_rect;
      pango_font_get_glyph_extents (font, unknown_glyph, NULL, &logical_rect);
      pango_glyph_string_set_size (glyphs, n_chars);
      p = text;
      for (i = 0; i < n_chars; i++)
	{
	  glyphs->glyphs[i].glyph = unknown_glyph;
	  glyphs->glyphs[i].geometry.x_offset = 0;
	  glyphs->glyphs[i].geometry.y_offset = 0;
	  glyphs->glyphs[i].geometry.width = logical_rect.width;
	  glyphs->log_clusters[i] = 0;

	  p = unicode_next_utf8 (p);
	}
      return;
    }
  p = text;
  wc = (GUChar4 *) g_malloc (sizeof (GUChar4) * n_chars);
  pango_glyph_string_set_size (glyphs, n_glyph);
  for (i = 0; i < n_chars; i++)
    {
      _pango_utf8_iterate (p, &next, &wc[i]);
      glyphs->log_clusters[i] = p - text;
      p = next;
    }

  devanagari_convert_vowels (&n_glyph, wc);

  n_syls = 1;
  syls[0] = wc;
  sb = glyphs->log_clusters[0];
  for (i = 0; i < n_chars; i++)
    {
      if (i && (is_consonant (wc[i]) | is_ind_vowel (wc[i]))
	  && wc[i - 1] != 0x94d)
	{
	  syls = g_realloc (syls, ((n_syls + 2) * sizeof (GUChar4 **)));
	  syls[n_syls] = wc + i;
	  n_syls++;
	  sb = glyphs->log_clusters[i];
	}
      glyphs->log_clusters[i] = sb;
    }
  syls[n_syls] = wc + i;

  for (i = 0; i < n_syls; i++)
    {
      devanagari_make_ligs (syls[i], syls[i + 1], glyphs->log_clusters +
			    (syls[i] - wc));
      devanagari_shift_vowels (syls[i], syls[i + 1]);
    }

  devanagari_compact (&n_glyph, wc, glyphs->log_clusters);

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

      if (wc[i] == JOINING_RA || wc[i] == ANUSWAR ||
	  wc[i] == REPHA || wc[i] == VIRAMA || wc[i] == CANDRA ||
	  is_nonspacing_vowel (wc[i]))
	{
	  if (wc[i] == VIRAMA)
	    {
	      glyphs->glyphs[i].geometry.x_offset =
		(-glyphs->glyphs[i - 1].geometry.width / 2);

	      if (!glyphs->glyphs[i].geometry.x_offset)
		glyphs->glyphs[i].geometry.x_offset =
		  (-glyphs->glyphs[i - 2].geometry.width / 2);
	    }
	  else
	    glyphs->glyphs[i].geometry.x_offset = -logical_rect.width * 2;

	  glyphs->glyphs[i].geometry.width = 0;
	  glyphs->log_clusters[i] = glyphs->log_clusters[i - 1];
	}
    }
  g_free (syls);
}

static PangoEngine *
devanagari_engine_x_new ()
{
  PangoEngineShape *result;
  result = g_new (PangoEngineShape, 1);
  result->engine.id = "DevanagariScriptEngine";
  result->engine.type = PANGO_ENGINE_TYPE_LANG;
  result->engine.length = sizeof (result);
  result->script_shape = devanagari_engine_shape;
  result->get_coverage = devanagari_engine_get_coverage;
  return (PangoEngine *) result;
}

static void
devanagari_engine_break (const char *text,
			 int len,
			 PangoAnalysis * analysis, PangoLogAttr * attrs)
{
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
      attrs[i].is_break = (i > 0 && attrs[i - 1].is_white) ||
	attrs[i].is_white;
      attrs[i].is_char_stop = 1;
      attrs[i].is_word_stop = (i == 0) || attrs[i - 1].is_white;
      /* actually, is_word_stop in not correct, but simple and good enough. */
      i++;
    }
}


static PangoEngine *
devanagari_engine_lang_new ()
{
  PangoEngineLang *result;

  result = g_new (PangoEngineLang, 1);

  result->engine.id = "DevanagariScriptEngine";
  result->engine.type = PANGO_ENGINE_TYPE_LANG;
  result->engine.length = sizeof (result);
  result->script_break = devanagari_engine_break;

  return (PangoEngine *) result;
}

#ifdef MODULE_PREFIX
#define MODULE_ENTRY(func) _pango_devanagari_##func
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
  if (!strcmp (id, "DevanagariScriptEngineLang"))
    return devanagari_engine_lang_new ();
  else if (!strcmp (id, "DevanagariScriptEngineX"))
    return devanagari_engine_x_new ();
  else
    return NULL;
}

void
MODULE_ENTRY(script_engine_unload) (PangoEngine * engine)
{
}
