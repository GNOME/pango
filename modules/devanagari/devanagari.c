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

void
devanagari_make_ligatures (int *num, GUChar4 * chars, gint * cluster)
{
  /* perhaps a syllable based approach would be better? */
  GUChar4 *src = chars;
  GUChar4 *start = chars;
  GUChar4 *end = chars + *num;
  gint *c_src = cluster;
  while (src < end)
    {
      int t0, t1, t2, t3, p1;
      if (chars != start)
	p1 = chars[-1];
      else
	p1 = 0;
      t0 = get_char (src, end);
      t1 = get_char (src + 1, end);
      t2 = get_char (src + 2, end);
      t3 = get_char (src + 3, end);

      if (!is_half_consonant (p1))
	{
	  int i;
	  /* This makes T.T.T.T come out OK. We need an expert in Devanagari
	   * to explain what 3 and 4-consonant ligatures are supposed to
	   * look like, especially when some of the adjacent characters
	   * form ligatures in 2 consonant form. 
	   *
	   * (T.T.T.T is significant as T.T forms a conjunt with a half-form
	   * which looks very similar so it was producing TT (half-form), 
	   * joined to TT unfortunately, this was indistinguishable from
	   * T.T.T )
	   */
	  for (i = 0; i < n_ligatures; i++)
	    {
	      /* handle the conjuncts */
	      LigData *l = ligatures + i;
	      if (t0 == l->source[0] && t1 == l->source[1]
		  && t2 == l->source[2])
		{
		  /* RA ligature handling magic */
		  if (t2 == RA && (is_consonant (t3) || (t3 == 0x94d)))
		    continue;

		  chars[0] = l->replacement;
		  src += 3;
		  chars++;

		  *cluster = *c_src;
		  c_src += 3;
		  cluster++;
		  break;
		}
	    }
	  if (i != n_ligatures)
	    {
	      /* if we made a conjunct here, loop... */
	      continue;
	    }
	}

      if ((is_consonant (t0)) &&
	  (t1 == VIRAMA) && (t2 == RA) &&
	  (!is_consonant (t3)) && (t3 != 0x94d))
	{
	  /* turn C vir RA to C joining-RA */
	  chars[0] = *src;
	  chars[1] = JOINING_RA;

	  *cluster = *c_src;
	  cluster[1] = *c_src;

	  src += 3;
	  chars += 2;

	  c_src += 3;
	  cluster += 2;
	  continue;
	}

      /* some ligatures have half-forms. use them. */
      if ((p1 >= 0xe900 && p1 <= 0xe906) && t0 == VIRAMA && is_consonant (t1))
	{
	  chars[-1] = 0xe972;
	  src++;
	  c_src++;
	  continue;
	}

      /* is_ligating_consonant(t2) probably wants to
       * be is_consonant(t2), not sure. */
      if (is_ligating_consonant (t0) &&
	  t1 == VIRAMA && is_ligating_consonant (t2))
	{
	  chars[0] = t0 + 0xe000;
	  src += 2;
	  chars++;

	  *cluster = *c_src;
	  c_src += 2;
	  cluster++;
	  continue;
	}

      /* Handle Virama followed by Nukta. This suppresses the special-case
       * ligature, and just enables regular half-form building.
       * 
       * Cavaet as above. */
      if (is_ligating_consonant (t0) &&
	  t1 == VIRAMA && t2 == NUKTA && is_ligating_consonant (t3))
	{
	  chars[0] = t0 + 0xe000;
	  src += 3;
	  chars++;

	  *cluster = *c_src;
	  c_src += 3;
	  cluster++;

	  continue;
	}

      /* convert R virama vowel to full-vowel with repha */
      if (p1 != VIRAMA &&
	  !is_half_consonant (p1) &&
	  t0 == RA && t1 == VIRAMA && is_comb_vowel (t2))
	{
	  chars[0] = vowelsign_to_letter (t2);
	  chars[1] = REPHA;
	  *cluster = *c_src;
	  cluster[1] = *c_src;
	  chars += 2;
	  cluster += 2;

	  c_src += 3;
	  src += 3;
	  continue;
	}

      *chars = *src;
      src++;
      chars++;

      *cluster = *c_src;
      cluster++;
      c_src++;
    }
  *num = chars - start;
}

void
devanagari_shift_vowels (int *num, GUChar4 * chars, gint * clusters)
{
  /* moves 0x93f (I) before consonant clusters where appropriate. */
  GUChar4 *strt = chars, *end = chars + *num;
  while (chars < end)
    {
      if (*chars == 0x93f && chars > strt)
	{
	  GUChar4 *bubble = chars;
	  int i = 1;
	  /* move back one consonant, and past any half consonants */
	  /* How should this interact with vowel letters and other 
	   * non-consonant signs? */

	  /* also, should it go back past consonants that have a virama
	   * attached, so as to be at the start of the syllable? */

	  /* probably should go past JOINING RA as well. */
	  while (bubble > strt && (i || is_half_consonant (bubble[-1])))
	    {
	      bubble[0] = bubble[-1];
	      bubble[-1] = 0x93f;
	      i = 0;
	      bubble--;
	    }
	  /* XXX : if we bubble the cluster stuff here back with the
	     glyph, it breaks. */
	}
      chars++;
      clusters++;
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
      if (chars == start && is_comb_vowel (chars[0]))
	{
	  chars[0] = vowelsign_to_letter (chars[0]);
	}
      chars++;
    }
}

void
devanagari_remove_explicit_virama (int *num, GUChar4 * chars)
{
  /* collapse two viramas in a row to one virama. This is defined
   * to mean 'show it with the virama, don't ligate'. */
  GUChar4 *end = chars + *num;
  while (chars < end)
    {
      if (chars[0] == VIRAMA && chars[1] == VIRAMA)
	chars[1] = 0;
      chars++;
    }
}

void
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
  devanagari_make_ligatures (&n_glyph, wc, glyphs->log_clusters);
  devanagari_remove_explicit_virama (&n_glyph, wc);
  devanagari_compact (&n_glyph, wc, glyphs->log_clusters);
  devanagari_shift_vowels (&n_glyph, wc, glyphs->log_clusters);
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

      if ((wc[i] == VIRAMA || wc[i] == ANUSWAR || wc[i] == CANDRA ||
	   wc[i] == JOINING_RA || wc[i] == REPHA ||
	   is_nonspacing_vowel (wc[i])) && i)
	{
	  if (wc[i] == VIRAMA)
	    {
	      glyphs->glyphs[i].geometry.x_offset =
		(-glyphs->glyphs[i - 1].geometry.width / 2);
	    }
	  else if (is_nonspacing_vowel (wc[i]))
	    {
	      glyphs->glyphs[i].geometry.x_offset =
		-((glyphs->glyphs[i - 1].geometry.width) +
		  (logical_rect.width)) / 2;
	    }
	  else
	    glyphs->glyphs[i].geometry.x_offset = -logical_rect.width * 2;

	  glyphs->glyphs[i].geometry.width = 0;
	  glyphs->log_clusters[i] = glyphs->log_clusters[i - 1];
	}
    }
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

void
script_engine_list (PangoEngineInfo ** engines, int *n_engines)
{
  *engines = script_engines;
  *n_engines = n_script_engines;
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

PangoEngine *
script_engine_load (const char *id)
{
  if (!strcmp (id, "DevanagariScriptEngineLang"))
    return devanagari_engine_lang_new ();
  else if (!strcmp (id, "DevanagariScriptEngineX"))
    return devanagari_engine_x_new ();
  else
    return NULL;
}

void
script_engine_unload (PangoEngine * engine)
{
}
