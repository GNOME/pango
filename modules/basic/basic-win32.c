/* Pango
 * basic-win32.c:
 *
 * Copyright (C) 1999 Red Hat Software
 * Copyright (C) 2001 Alexander Larsson
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

#include "config.h"

#define BASIC_WIN32_DEBUGGING

#include <stdlib.h>

#include <glib.h>

#include "pangowin32.h"
#include "pango-engine.h"
#include "pango-utils.h"

#include "basic-common.h"

#define SCRIPT_ENGINE_NAME "BasicScriptEngineWin32"

static gboolean pango_win32_debug = FALSE;

#ifdef HAVE_USP10_H

#include "usp10.h"

/* Some languages missing from mingw ("w32api") headers */

#ifndef LANG_INVARIANT
#define LANG_INVARIANT 0x7f
#endif
#ifndef LANG_DIVEHI
#define LANG_DIVEHI 0x65
#endif
#ifndef LANG_GALICIAN
#define LANG_GALICIAN 0x56
#endif
#ifndef LANG_KYRGYZ
#define LANG_KYRGYZ 0x40
#endif
#ifndef LANG_MONGOLIAN
#define LANG_MONGOLIAN 0x50
#endif
#ifndef LANG_SYRIAC
#define LANG_SYRIAC 0x5a
#endif

static gboolean have_uniscribe = FALSE;

static PangoWin32FontCache *font_cache;

static HDC hdc;

typedef HRESULT (WINAPI *pScriptGetProperties) (const SCRIPT_PROPERTIES ***,
						int *);

typedef HRESULT (WINAPI *pScriptItemize) (const WCHAR *,
					  int,
					  int,
					  const SCRIPT_CONTROL *,
					  const SCRIPT_STATE *,
					  SCRIPT_ITEM *,
					  int *);

typedef HRESULT (WINAPI *pScriptShape) (HDC,
					SCRIPT_CACHE *,
					const WCHAR *,
					int,
					int,
					SCRIPT_ANALYSIS *,
					WORD *,
					WORD *,
					SCRIPT_VISATTR *,
					int *);

typedef HRESULT (WINAPI *pScriptPlace) (HDC,
					SCRIPT_CACHE *,
					const WORD *,
					int,
					const SCRIPT_VISATTR *,
					SCRIPT_ANALYSIS *,
					int *,
					GOFFSET *,
					ABC *);

typedef HRESULT (WINAPI *pScriptFreeCache) (SCRIPT_CACHE *);

static pScriptGetProperties script_get_properties;
static pScriptItemize script_itemize;
static pScriptShape script_shape;
static pScriptPlace script_place;
static pScriptFreeCache script_free_cache;

static const SCRIPT_PROPERTIES **scripts;
static int nscripts;

#endif

#ifdef HAVE_USP10_H
static PangoEngineRange uniscribe_ranges[] = {
  /* We claim to cover everything ;-) */
  { 0x0020, 0xffee, "*" }
};
#endif

static PangoEngineRange basic_ranges[] = {
  /* Those characters that can be rendered legibly without Uniscribe.
   * I am not certain this list is correct.
   */

  /* Basic Latin, Latin-1 Supplement, Latin Extended-A, Latin Extended-B,
   * IPA Extensions
   */
  { 0x0020, 0x02af, "*" },

  /* Spacing Modifier Letters */
  { 0x02b0, 0x02ff, "" },

  /* Not covered: Combining Diacritical Marks */

  /* Greek, Cyrillic, Armenian */
  { 0x0380, 0x058f, "*" },

  /* Hebrew */
  { 0x0591, 0x05f4, "*" },

  /* Arabic */
  { 0x060c, 0x06f9, "" },

  /* Not covered: Syriac, Thaana, Devanagari, Bengali, Gurmukhi, Gujarati,
   * Oriya, Tamil, Telugu, Kannada, Malayalam, Sinhala
   */

  /* Thai */
  { 0x0e01, 0x0e5b, "" },

  /* Not covered: Lao, Tibetan, Myanmar */

  /* Georgian */
  { 0x10a0, 0x10ff, "*" },

  /* Not covered: Hangul Jamo */

  /* Ethiopic, Cherokee, Unified Canadian Aboriginal Syllabics, Ogham,
   * Runic */
  { 0x1200, 0x16ff, "*" },

  /* Not covered: Khmer, Mongolian */

  /* Latin Extended Additional, Greek Extended */
  { 0x1e00, 0x1fff, "*" },

  /* General Punctuation, Superscripts and Subscripts, Currency
   * Symbols, Combining Marks for Symbols, Letterlike Symbols, Number
   * Forms, Arrows, Mathematical Operators, Miscellaneous Technical,
   * Control Pictures, Optical Character Recognition, Enclosed
   * Alphanumerics, Box Drawing, Block Elements, Geometric Shapes,
   * Miscellaneous Symbols, Dingbats, Braille Patterns, CJK Radicals
   * Supplement, Kangxi Radicals, Ideographic Description Characters,
   * CJK Symbols and Punctuation, Hiragana, Katakana, Bopomofo, Hangul
   * Compatibility Jamo, Kanbun, Bopomofo Extended, Enclosed CJK
   * Letters and Months, CJK Compatibility, CJK Unified Ideographs
   * Extension A, CJK Unified Ideographs, Yi Syllables, Yi Radicals
   */
  { 0x2000, 0xa4c6, "*" },

  /* Hangul Syllables */
  { 0xac00, 0xd7a3, "kr" },

  /* Not covered: Private Use */

  /* CJK Compatibility Ideographs (partly) */
  { 0xf900, 0xfa0b, "kr" },

  /* Not covered: CJK Compatibility Ideographs (partly), Alphabetic
   * Presentation Forms, Arabic Presentation Forms-A, Combining Half
   * Marks, CJK Compatibility Forms, Small Form Variants, Arabic
   * Presentation Forms-B, Specials
   */

  /* Halfwidth and Fullwidth Forms */
  { 0xff00, 0xffed, "*" }
};

static PangoEngineInfo script_engines[] = {
  {
    SCRIPT_ENGINE_NAME,
    PANGO_ENGINE_TYPE_SHAPE,
    PANGO_RENDER_TYPE_WIN32,
    NULL, 0
  }
};

static PangoGlyph 
find_char (PangoFont *font,
	   gunichar   wc)
{
  return pango_win32_font_get_glyph_index (font, wc);
}

static void
set_glyph (PangoFont        *font,
	   PangoGlyphString *glyphs,
	   int               i,
	   int               offset,
	   PangoGlyph        glyph)
{
  PangoRectangle logical_rect;

  glyphs->glyphs[i].glyph = glyph;
  
  glyphs->glyphs[i].geometry.x_offset = 0;
  glyphs->glyphs[i].geometry.y_offset = 0;

  glyphs->log_clusters[i] = offset;

  pango_font_get_glyph_extents (font, glyphs->glyphs[i].glyph, NULL, &logical_rect);
  glyphs->glyphs[i].geometry.width = logical_rect.width;
}

static void
swap_range (PangoGlyphString *glyphs,
	    int               start,
	    int               end)
{
  int i, j;
  
  for (i = start, j = end - 1; i < j; i++, j--)
    {
      PangoGlyphInfo glyph_info;
      gint log_cluster;
      
      glyph_info = glyphs->glyphs[i];
      glyphs->glyphs[i] = glyphs->glyphs[j];
      glyphs->glyphs[j] = glyph_info;
      
      log_cluster = glyphs->log_clusters[i];
      glyphs->log_clusters[i] = glyphs->log_clusters[j];
      glyphs->log_clusters[j] = log_cluster;
    }
}

#ifdef BASIC_WIN32_DEBUGGING

static char *
charset_name (int charset)
{
  static char unk[10];

  switch (charset)
    {
#define CASE(n) case n##_CHARSET: return #n
      CASE (ANSI);
      CASE (DEFAULT);
      CASE (SYMBOL);
      CASE (SHIFTJIS);
      CASE (HANGEUL);
      CASE (GB2312);
      CASE (CHINESEBIG5);
      CASE (OEM);
      CASE (JOHAB);
      CASE (HEBREW);
      CASE (ARABIC);
      CASE (GREEK);
      CASE (TURKISH);
      CASE (VIETNAMESE);
      CASE (THAI);
      CASE (EASTEUROPE);
      CASE (RUSSIAN);
      CASE (MAC);
      CASE (BALTIC);
#undef CASE
    default:
      sprintf (unk, "%d", charset);
      return unk;
    }
}

static char *
lang_name (int lang)
{
  static char unk[10];
  
  switch (PRIMARYLANGID (lang))
    {
#define CASE(n) case LANG_##n: return #n
      CASE (NEUTRAL);
      CASE (INVARIANT);
      CASE (AFRIKAANS);
      CASE (ALBANIAN);
      CASE (ARABIC);
      CASE (ARMENIAN);
      CASE (ASSAMESE);
      CASE (AZERI);
      CASE (BASQUE);
      CASE (BELARUSIAN);
      CASE (BENGALI);
      CASE (BULGARIAN);
      CASE (CATALAN);
      CASE (CHINESE);
      CASE (CROATIAN);
      CASE (CZECH);
      CASE (DANISH);
      CASE (DIVEHI);
      CASE (DUTCH);
      CASE (ENGLISH);
      CASE (ESTONIAN);
      CASE (FAEROESE);
      CASE (FARSI);
      CASE (FINNISH);
      CASE (FRENCH);
      CASE (GALICIAN);
      CASE (GEORGIAN);
      CASE (GERMAN);
      CASE (GREEK);
      CASE (GUJARATI);
      CASE (HEBREW);
      CASE (HINDI);
      CASE (HUNGARIAN);
      CASE (ICELANDIC);
      CASE (INDONESIAN);
      CASE (ITALIAN);
      CASE (JAPANESE);
      CASE (KANNADA);
      CASE (KASHMIRI);
      CASE (KAZAK);
      CASE (KONKANI);
      CASE (KOREAN);
      CASE (KYRGYZ);
      CASE (LATVIAN);
      CASE (LITHUANIAN);
      CASE (MACEDONIAN);
      CASE (MALAY);
      CASE (MALAYALAM);
      CASE (MANIPURI);
      CASE (MARATHI);
      CASE (MONGOLIAN);
      CASE (NEPALI);
      CASE (NORWEGIAN);
      CASE (ORIYA);
      CASE (POLISH);
      CASE (PORTUGUESE);
      CASE (PUNJABI);
      CASE (ROMANIAN);
      CASE (RUSSIAN);
      CASE (SANSKRIT);
      CASE (SINDHI);
      CASE (SLOVAK);
      CASE (SLOVENIAN);
      CASE (SPANISH);
      CASE (SWAHILI);
      CASE (SWEDISH);
      CASE (SYRIAC);
      CASE (TAMIL);
      CASE (TATAR);
      CASE (TELUGU);
      CASE (THAI);
      CASE (TURKISH);
      CASE (UKRAINIAN);
      CASE (URDU);
      CASE (UZBEK);
      CASE (VIETNAMESE);
#undef CASE
    default:
      sprintf (unk, "%#02x", lang);
      return unk;
    }
}

#endif /* BASIC_WIN32_DEBUGGING */

#ifdef HAVE_USP10_H

static WORD
make_langid (PangoLanguage *lang)
{
#define CASE(t,p,s) if (pango_language_matches (lang, t)) return MAKELANGID (LANG_##p, SUBLANG_##p##_##s)
#define CASEN(t,p) if (pango_language_matches (lang, t)) return MAKELANGID (LANG_##p, SUBLANG_NEUTRAL)

  /* Languages that most probably don't affect Uniscribe have been
   * left out. Presumably still many of those mentioned below don't
   * have any effect either.
   */

  CASEN ("sq", ALBANIAN);
  CASEN ("ar", ARABIC);
  CASEN ("az", AZERI);
  CASEN ("hy", ARMENIAN);
  CASEN ("as", ASSAMESE);
  CASEN ("az", AZERI);
  CASEN ("eu", BASQUE);
  CASEN ("be", BELARUSIAN);
  CASEN ("bn", BENGALI);
  CASEN ("bg", BULGARIAN);
  CASEN ("ca", CATALAN);
  CASE ("zh-tw", CHINESE, TRADITIONAL);
  CASE ("zh-cn", CHINESE, SIMPLIFIED);
  CASE ("zh-hk", CHINESE, HONGKONG);
  CASE ("zh-sg", CHINESE, SINGAPORE);
  CASE ("zh-mo", CHINESE, MACAU);
  CASEN ("hr", CROATIAN);
  CASE ("sr", SERBIAN, CYRILLIC);
  CASEN ("dib", DIVEHI);
  CASEN ("fa", FARSI);
  CASEN ("ka", GEORGIAN);
  CASEN ("el", GREEK);
  CASEN ("gu", GUJARATI);
  CASEN ("he", HEBREW);
  CASEN ("hi", HINDI);
  CASEN ("ja", JAPANESE);
  CASEN ("kn", KANNADA);
  CASE ("ks-in", KASHMIRI, INDIA);
  CASEN ("ks", KASHMIRI);
  CASEN ("kk", KAZAK);
  CASEN ("kok", KONKANI);
  CASEN ("ko", KOREAN);
  CASEN ("ky", KYRGYZ);
  CASEN ("ml", MALAYALAM);
  CASEN ("mni", MANIPURI);
  CASEN ("mr", MARATHI);
  CASEN ("mn", MONGOLIAN);
  CASE ("ne-in", NEPALI, INDIA);
  CASEN ("ne", NEPALI);
  CASEN ("or", ORIYA);
  CASEN ("pa", PUNJABI);
  CASEN ("sa", SANSKRIT);
  CASEN ("sd", SINDHI);
  CASEN ("syr", SYRIAC);
  CASEN ("ta", TAMIL);
  CASEN ("tt", TATAR);
  CASEN ("te", TELUGU);
  CASEN ("th", THAI);
  CASEN ("tr", TURKISH);
  CASEN ("uk", UKRAINIAN);
  CASE ("ur-pk", URDU, PAKISTAN);
  CASE ("ur-in", URDU, INDIA);
  CASEN ("ur", URDU);
  CASEN ("uz", UZBEK);
  CASEN ("vi", VIETNAMESE);

#undef CASE
#undef CASEN

  return MAKELANGID (LANG_NEUTRAL, SUBLANG_NEUTRAL);
}

#ifdef BASIC_WIN32_DEBUGGING

static void
dump_glyphs_and_log_clusters (gboolean rtl,
			      int      itemlen,
			      int      charix0,
			      WORD    *log_clusters,
			      WORD    *iglyphs,
			      int      nglyphs)
{
  if (pango_win32_debug)
  {
    int j, k, nclusters, clusterix, charix, ng;

    printf ("  ScriptShape: nglyphs=%d: ", nglyphs);

    for (j = 0; j < nglyphs; j++)
      printf ("%d%s", iglyphs[j], (j < nglyphs-1) ? "," : "");
    printf ("\n");

    printf ("  log_clusters: ");
    for (j = 0; j < itemlen; j++)
      printf ("%d ", log_clusters[j]);
    printf ("\n");    nclusters = 0;
    for (j = 0; j < itemlen; j++)
      {
	if (j == 0 || log_clusters[j-1] != log_clusters[j])
	  nclusters++;
      }
    printf ("  %d clusters:\n", nclusters);

    /* If RTL, first char is the last in the run, otherwise the
     * first.
     */
    clusterix = 0;
    if (rtl)
      {
	int firstglyphix = 0;
	for (j = itemlen - 1, charix = charix0 + j; j >= 0; j--, charix--)
	  {
	    if (j == itemlen - 1 || log_clusters[j] != log_clusters[j+1])
	      printf ("  Cluster %d: chars %d--",
		      clusterix, charix);
	    if (j == 0 || log_clusters[j-1] != log_clusters[j])
	      {
		ng = log_clusters[j] - firstglyphix + 1;
		printf ("%d: %d glyphs: ",
			charix, ng);
		for (k = firstglyphix; k <= log_clusters[j]; k++)
		  {
		    printf ("%d", iglyphs[k]);
		    if (k < log_clusters[j])
		      printf (",");
		  }
		firstglyphix = log_clusters[j] + 1;
		clusterix++;
		printf ("\n");
	      }
	  }
      }
    else
      {
	for (j = 0, charix = charix0; j < itemlen; j++, charix++)
	  {
	    if (j == 0 || log_clusters[j-1] != log_clusters[j])
	      printf ("  Cluster %d: chars %d--",
		      clusterix, charix);
	    if (j == itemlen - 1 || log_clusters[j] != log_clusters[j+1])
	      {
		int klim = ((j == itemlen-1) ? nglyphs : log_clusters[j+1]);
		ng = klim - log_clusters[j];
		printf ("%d: %d glyphs: ",
			charix, ng);
		for (k = log_clusters[j]; k < klim; k++)
		  {
		    printf ("%d", iglyphs[k]);
		    if (k != klim - 1)
		      printf (",");
		  }
		clusterix++;
		printf ("\n");
	      }
	  }
      }
  }
}

#endif /* BASIC_WIN32_DEBUGGING */

static void
set_up_pango_log_clusters (gboolean rtl,
			   int      itemlen,
			   WORD    *usp_log_clusters,
			   int      nglyphs,
			   gint    *pango_log_clusters,
			   int      char_offset)
{
  int j, k;
  int first_char_in_cluster;

  if (rtl)
    {
      /* RTL. Walk Uniscribe log_clusters array backwards, build Pango
       * log_clusters array forwards.
       */
      int glyph0 = 0;
      first_char_in_cluster = itemlen - 1;
      for (j = itemlen - 1; j >= 0; j--)
	{
	  if (j < itemlen - 1 && usp_log_clusters[j+1] != usp_log_clusters[j])
	    {
	      /* Cluster starts */
	      first_char_in_cluster = j;
	    }
	  if (j == 0)
	    {
	      /* First char, cluster ends */
	      for (k = glyph0; k < nglyphs; k++)
		pango_log_clusters[k] = first_char_in_cluster + char_offset;
	    }
	  else if (usp_log_clusters[j-1] == usp_log_clusters[j])
	    {
	      /* Cluster continues */
	      first_char_in_cluster = j-1;
	    }
	  else
	    {
	      /* Cluster ends */
	      for (k = glyph0; k <= usp_log_clusters[j]; k++)
		pango_log_clusters[k] = first_char_in_cluster + char_offset;
	      glyph0 = usp_log_clusters[j] + 1;
	    }
	}
    }
  else
    {
      /* LTR. Walk Uniscribe log_clusters array forwards, build Pango
       * log_clusters array also forwards.
       */
      first_char_in_cluster = 0;
      for (j = 0; j < itemlen; j++)
	{
	  if (j > 0 && usp_log_clusters[j-1] != usp_log_clusters[j])
	    {
	      /* Cluster starts */
	      first_char_in_cluster = j;
	    }
	  if (j == itemlen - 1)
	    {
	      /* Last char, cluster ends */
	      for (k = usp_log_clusters[j]; k < nglyphs; k++)
		pango_log_clusters[k] = first_char_in_cluster + char_offset;
	    }
	  else if (usp_log_clusters[j] == usp_log_clusters[j+1])
	    {
	      /* Cluster continues */
	    }
	  else
	    {
	      /* Cluster ends */
	      for (k = usp_log_clusters[j]; k < usp_log_clusters[j+1]; k++)
		pango_log_clusters[k] = first_char_in_cluster + char_offset;
	    }
	}
    }
}

static void
convert_log_clusters_to_byte_offsets (const char       *text,
				      gint              length,
				      PangoAnalysis    *analysis,
				      PangoGlyphString *glyphs)
{
  const char *p;
  int charix, glyphix;

  /* Convert char indexes in the log_clusters array to byte offsets in
   * the UTF-8 text.
   */
  p = text;
  charix = 0;
  if (analysis->level % 2)
    {
      glyphix = glyphs->num_glyphs - 1;
      while (p < text + length)
	{
	  while (glyphix >= 0 &&
		 glyphs->log_clusters[glyphix] == charix)
	    {
	      glyphs->log_clusters[glyphix] = p - text;
	      glyphix--;
	    }
	  charix++;
	  p = g_utf8_next_char (p);
	}
    }
  else
    {
      glyphix = 0;
      while (p < text + length)
	{
	  while (glyphix < glyphs->num_glyphs &&
		 glyphs->log_clusters[glyphix] == charix)
	    {
	      glyphs->log_clusters[glyphix] = p - text;
	      glyphix++;
	    }
	  charix++;
	  p = g_utf8_next_char (p);
	}
    }
}

static gboolean
itemize_shape_and_place (PangoFont        *font,
			 HDC               hdc,
			 wchar_t          *wtext,
			 int               wlen,
			 PangoAnalysis    *analysis,
			 PangoGlyphString *glyphs,
			 SCRIPT_CACHE     *script_cache)
{
  int item, nitems;
  int itemlen, glyphix, nglyphs;
  int nc;
  SCRIPT_CONTROL control;
  SCRIPT_STATE state;
  SCRIPT_ITEM items[100];

  memset (&control, 0, sizeof (control));
  memset (&state, 0, sizeof (state));

  control.uDefaultLanguage = make_langid (analysis->language);
  state.uBidiLevel = analysis->level;
  
  if ((*script_itemize) (wtext, wlen, G_N_ELEMENTS (items), &control, NULL,
			 items, &nitems))
    {
#ifdef BASIC_WIN32_DEBUGGING
      if (pango_win32_debug)
	printf ("pango-basic-win32: ScriptItemize failed\n");
#endif
      return FALSE;
    }

#ifdef BASIC_WIN32_DEBUGGING
  if (pango_win32_debug)
    printf (G_STRLOC ": ScriptItemize: %d items\n", nitems);
#endif

  nc = 0;
  for (item = 0; item < nitems; item++)
    {
      WORD iglyphs[1000];
      WORD log_clusters[1000];
      SCRIPT_VISATTR visattrs[1000];
      int advances[1000];
      GOFFSET offsets[1000];
      ABC abc;
      int script = items[item].a.eScript;
      int ng;

      memset (advances, 0, sizeof (advances));
      memset (offsets, 0, sizeof (offsets));
      memset (&abc, 0, sizeof (abc));
	  
      itemlen = items[item+1].iCharPos - items[item].iCharPos;

#ifdef BASIC_WIN32_DEBUGGING
      if (pango_win32_debug)
	printf ("  Item %d: iCharPos=%d eScript=%d (%s) %s%s%s%s%s%s%s chars %d--%d (%d)\n",
		item, items[item].iCharPos, script,
		lang_name (scripts[script]->langid),
		scripts[script]->fComplex ? "complex" : "simple",
		items[item].a.fRTL ? " fRTL" : "",
		items[item].a.fLayoutRTL ? " fLayoutRTL" : "",
		items[item].a.fLinkBefore ? " fLinkBefore" : "",
		items[item].a.fLinkAfter ? " fLinkAfter" : "",
		items[item].a.fLogicalOrder ? " fLogicalOrder" : "",
		items[item].a.fNoGlyphIndex ? " fNoGlyphIndex" : "",
		items[item].iCharPos, items[item+1].iCharPos-1, itemlen);
#endif

      if ((*script_shape) (hdc, &script_cache[script], wtext + items[item].iCharPos, itemlen,
			   G_N_ELEMENTS (iglyphs),
			   &items[item].a,
			   iglyphs,
			   log_clusters,
			   visattrs,
			   &nglyphs))
	{
#ifdef BASIC_WIN32_DEBUGGING
	  if (pango_win32_debug)
	    printf ("pango-basic-win32: ScriptShape failed\n");
#endif
	  return FALSE;
	}
      
#ifdef BASIC_WIN32_DEBUGGING
      dump_glyphs_and_log_clusters (items[item].a.fRTL, itemlen,
				    items[item].iCharPos, log_clusters,
				    iglyphs, nglyphs);
#endif

      ng = glyphs->num_glyphs;
      pango_glyph_string_set_size (glyphs, ng + nglyphs);
      
      set_up_pango_log_clusters (items[item].a.fRTL, itemlen, log_clusters,
				 nglyphs, glyphs->log_clusters + ng, nc);

      if ((*script_place) (hdc, &script_cache[script], iglyphs, nglyphs,
			   visattrs, &items[item].a,
			   advances, offsets, &abc))
	{
#ifdef BASIC_WIN32_DEBUGGING
	  if (pango_win32_debug)
	    printf ("pango-basic-win32: ScriptPlace failed\n");
#endif
	  return FALSE;
	}

      for (glyphix = 0; glyphix < nglyphs; glyphix++)
	{
	  if (iglyphs[glyphix] != 0)
	    {
	      glyphs->glyphs[ng+glyphix].glyph = iglyphs[glyphix];
	      glyphs->glyphs[ng+glyphix].geometry.width = PANGO_SCALE * advances[glyphix];
	      glyphs->glyphs[ng+glyphix].geometry.x_offset = PANGO_SCALE * offsets[glyphix].du;
	      glyphs->glyphs[ng+glyphix].geometry.y_offset = PANGO_SCALE * offsets[glyphix].dv;
	    }
	  else
	    {
	      PangoRectangle logical_rect;
	      /* Should pass actual char that was not found to
	       * pango_win32_get_unknown_glyph(), but a bit hard to
	       * find out that at this point, so cheat and use 0.
	       */
	      PangoGlyph unk = pango_win32_get_unknown_glyph (font, 0);

	      glyphs->glyphs[ng+glyphix].glyph = unk;
	      pango_font_get_glyph_extents (font, unk, NULL, &logical_rect);
	      glyphs->glyphs[ng+glyphix].geometry.width = logical_rect.width;
	      glyphs->glyphs[ng+glyphix].geometry.x_offset = 0;
	      glyphs->glyphs[ng+glyphix].geometry.y_offset = 0;
	    }
	}
      nc += itemlen;
    }

#ifdef BASIC_WIN32_DEBUGGING
  if (pango_win32_debug)
    {
      printf ("  Pango log_clusters, char index:");
      for (glyphix = 0; glyphix < glyphs->num_glyphs; glyphix++)
	printf ("%d ", glyphs->log_clusters[glyphix]);
      printf ("\n");
    }
#endif

  return TRUE;
}

static gboolean
uniscribe_shape (PangoFont        *font,
		 const char       *text,
		 gint              length,
		 PangoAnalysis    *analysis,
		 PangoGlyphString *glyphs)
{
  wchar_t *wtext;
  int wlen, i;
  gboolean retval = TRUE;
  GError *error;
  HGDIOBJ old_font;
  LOGFONT *lf;
  SCRIPT_CACHE script_cache[100];

  wtext = (wchar_t *) g_convert (text, length, "UTF-16LE", "UTF-8",
				 NULL, &wlen, &error);
  if (wtext == NULL)
    return FALSE;

  wlen /= 2;

  lf = pango_win32_font_logfont (font);
  old_font = SelectObject (hdc, pango_win32_font_cache_load (font_cache, lf));
  if (old_font == NULL)
    {
#ifdef BASIC_WIN32_DEBUGGING
      if (pango_win32_debug)
	printf ("pango-basic-win32: SelectObject for font failed\n");
#endif
      retval = FALSE;
    }

  if (retval)
    {
      memset (script_cache, 0, sizeof (script_cache));
      retval = itemize_shape_and_place (font, hdc, wtext, wlen, analysis, glyphs, script_cache);
      for (i = 0; i < G_N_ELEMENTS (script_cache); i++)
	if (script_cache[i])
	  (*script_free_cache)(&script_cache[i]);
    }
  
  if (retval)
    {
      convert_log_clusters_to_byte_offsets (text, length, analysis, glyphs);
#ifdef BASIC_WIN32_DEBUGGING
      if (pango_win32_debug)
	{
	  int glyphix;

	  printf ("  Pango log_clusters, byte offsets:");
	  for (glyphix = 0; glyphix < glyphs->num_glyphs; glyphix++)
	    printf ("%d ", glyphs->log_clusters[glyphix]);
	  printf ("\n");
	}
#endif

    }

  g_free (wtext);
  if (old_font != NULL)
    SelectObject (hdc, old_font);

  return retval;
}

#endif /* HAVE_USP10_H */

static void 
basic_engine_shape (PangoFont        *font,
		    const char       *text,
		    gint              length,
		    PangoAnalysis    *analysis,
		    PangoGlyphString *glyphs)
{
  int n_chars;
  int i;
  const char *p;

  g_return_if_fail (font != NULL);
  g_return_if_fail (text != NULL);
  g_return_if_fail (length >= 0);
  g_return_if_fail (analysis != NULL);

#ifdef HAVE_USP10_H

  if (have_uniscribe && uniscribe_shape (font, text, length, analysis, glyphs))
    return;

#endif

  n_chars = g_utf8_strlen (text, length);

  pango_glyph_string_set_size (glyphs, n_chars);

  p = text;
  for (i = 0; i < n_chars; i++)
    {
      gunichar wc;
      gunichar mirrored_ch;
      PangoGlyph index;

      wc = g_utf8_get_char (p);

      if (analysis->level % 2)
	if (pango_get_mirror_char (wc, &mirrored_ch))
	  wc = mirrored_ch;

      if (wc == 0xa0)	/* non-break-space */
	wc = 0x20;
		
      if (ZERO_WIDTH_CHAR (wc))
	{
	  set_glyph (font, glyphs, i, p - text, 0);
	}
      else
	{
	  index = find_char (font, wc);
	  if (index)
	    {
	      set_glyph (font, glyphs, i, p - text, index);
	      
	      if (g_unichar_type (wc) == G_UNICODE_NON_SPACING_MARK)
		{
		  if (i > 0)
		    {
		      PangoRectangle logical_rect, ink_rect;
		      
		      glyphs->glyphs[i].geometry.width = MAX (glyphs->glyphs[i-1].geometry.width,
							      glyphs->glyphs[i].geometry.width);
		      glyphs->glyphs[i-1].geometry.width = 0;
		      glyphs->log_clusters[i] = glyphs->log_clusters[i-1];

		      /* Some heuristics to try to guess how overstrike glyphs are
		       * done and compensate
		       */
		      /* FIXME: (alex) Is this double call to get_glyph_extents really necessary? */
		      pango_font_get_glyph_extents (font, glyphs->glyphs[i].glyph, &ink_rect, &logical_rect);
		      if (logical_rect.width == 0 && ink_rect.x == 0)
			glyphs->glyphs[i].geometry.x_offset = (glyphs->glyphs[i].geometry.width - ink_rect.width) / 2;
		    }
		}
	    }
	  else
	    set_glyph (font, glyphs, i, p - text, pango_win32_get_unknown_glyph (font, wc));
	}
      
      p = g_utf8_next_char (p);
    }

  /* Simple bidi support... may have separate modules later */

  if (analysis->level % 2)
    {
      int start, end;

      /* Swap all glyphs */
      swap_range (glyphs, 0, n_chars);
      
      /* Now reorder glyphs within each cluster back to LTR */
      for (start = 0; start < n_chars;)
	{
	  end = start;
	  while (end < n_chars &&
		 glyphs->log_clusters[end] == glyphs->log_clusters[start])
	    end++;
	  
	  swap_range (glyphs, start, end);
	  start = end;
	}
    }
}

static PangoCoverage *
basic_engine_get_coverage (PangoFont     *font,
			   PangoLanguage *lang)
{
  return pango_font_get_coverage (font, lang);
}

static PangoEngine *
basic_engine_win32_new (void)
{
  PangoEngineShape *result;
  
  result = g_new (PangoEngineShape, 1);

  result->engine.id = SCRIPT_ENGINE_NAME;
  result->engine.type = PANGO_ENGINE_TYPE_SHAPE;
  result->engine.length = sizeof (result);
  result->script_shape = basic_engine_shape;
  result->get_coverage = basic_engine_get_coverage;

  return (PangoEngine *)result;
}

static void
init_uniscribe (void)
{
#ifdef HAVE_USP10_H
  HMODULE usp10_dll;

  have_uniscribe = FALSE;
  
  if (getenv ("PANGO_WIN32_NO_UNISCRIBE") == NULL &&
      (usp10_dll = LoadLibrary ("usp10.dll")) != NULL)
    {
      (script_get_properties = (pScriptGetProperties)
       GetProcAddress (usp10_dll, "ScriptGetProperties")) &&
      (script_itemize = (pScriptItemize)
       GetProcAddress (usp10_dll, "ScriptItemize")) &&
      (script_shape = (pScriptShape)
       GetProcAddress (usp10_dll, "ScriptShape")) &&
      (script_place = (pScriptPlace)
       GetProcAddress (usp10_dll, "ScriptPlace")) &&
      (script_free_cache = (pScriptFreeCache)
       GetProcAddress (usp10_dll, "ScriptFreeCache")) &&
	(have_uniscribe = TRUE);
    }
  if (have_uniscribe)
    {
      (*script_get_properties) (&scripts, &nscripts);
      
      font_cache = pango_win32_font_map_get_font_cache
	(pango_win32_font_map_for_display ());
      
      hdc = pango_win32_get_dc ();
    }
#endif
} 

/* The following three functions provide the public module API for
 * Pango
 */
#ifdef WIN32_MODULE_PREFIX
#define MODULE_ENTRY(func) _pango_basic_win32_##func
#else
#define MODULE_ENTRY(func) func
#endif

void
MODULE_ENTRY(script_engine_list) (PangoEngineInfo **engines,
				  gint             *n_engines)
{
  init_uniscribe ();

  script_engines[0].ranges = basic_ranges;
  script_engines[0].n_ranges = G_N_ELEMENTS (basic_ranges);

#ifdef HAVE_USP10_H
  if (have_uniscribe)
    {
#if 0
      int i;
      GArray *ranges = g_array_new (FALSE, FALSE, sizeof (PangoEngineRange));

      /* Walk through scripts supported by the Uniscribe implementation on this
       * machine, and mark corresponding Unicode ranges.
       */
      for (i = 0; i < nscripts; i++)
	{
	}

      /* Sort range array */
      g_array_sort (ranges, compare_range);
      script_engines[0].ranges = ranges;
      script_engines[0].n_ranges = ranges->len;
#else
      script_engines[0].ranges = uniscribe_ranges;
      script_engines[0].n_ranges = G_N_ELEMENTS (uniscribe_ranges);
#endif
    }
#endif

  *engines = script_engines;
  *n_engines = G_N_ELEMENTS (script_engines);
}

PangoEngine *
MODULE_ENTRY(script_engine_load) (const char *id)
{
  init_uniscribe ();

  if (pango_win32_get_debug_flag ())
    pango_win32_debug = TRUE;

  if (!strcmp (id, SCRIPT_ENGINE_NAME))
    return basic_engine_win32_new ();
  else
    return NULL;
}

void 
MODULE_ENTRY(script_engine_unload) (PangoEngine *engine)
{
}
