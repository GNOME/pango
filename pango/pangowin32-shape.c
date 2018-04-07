/* Pango
 * pangowin32-shape.c:
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

/*#define BASIC_WIN32_DEBUGGING */

#include <math.h>
#include <stdlib.h>

#include <glib.h>

#include "pangowin32-private.h"

#include "pango-utils.h"
#include "pango-impl-utils.h"

static gboolean pango_win32_debug = FALSE;

#include <usp10.h>

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
lang_name (int lang)
{
  LCID lcid = MAKELCID (lang, SORT_DEFAULT);
  static char retval[10];

  if (!GetLocaleInfo (lcid, LOCALE_SISO639LANGNAME, retval, G_N_ELEMENTS (retval)))
    sprintf (retval, "%#02x", lang);

  return retval;
}

#endif /* BASIC_WIN32_DEBUGGING */

static WORD
make_langid (PangoLanguage *lang)
{
#define CASE(t,p,s) if (pango_language_matches (lang, t)) return MAKELANGID (LANG_##p, SUBLANG_##p##_##s)
#define CASEN(t,p) if (pango_language_matches (lang, t)) return MAKELANGID (LANG_##p, SUBLANG_NEUTRAL)

  /* Languages that most probably don't affect Uniscribe have been
   * left out. Uniscribe is documented to use
   * SCRIPT_CONTROL::uDefaultLanguage only to select digit shapes, so
   * just leave languages with own digits.
   */

  CASEN ("ar", ARABIC);
  CASEN ("hy", ARMENIAN);
  CASEN ("as", ASSAMESE);
  CASEN ("az", AZERI);
  CASEN ("bn", BENGALI);
  CASE ("zh-tw", CHINESE, TRADITIONAL);
  CASE ("zh-cn", CHINESE, SIMPLIFIED);
  CASE ("zh-hk", CHINESE, HONGKONG);
  CASE ("zh-sg", CHINESE, SINGAPORE);
  CASE ("zh-mo", CHINESE, MACAU);
  CASEN ("dib", DIVEHI);
  CASEN ("fa", FARSI);
  CASEN ("ka", GEORGIAN);
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
  CASE ("ur-pk", URDU, PAKISTAN);
  CASE ("ur-in", URDU, INDIA);
  CASEN ("ur", URDU);
  CASEN ("uz", UZBEK);

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

    g_print ("  ScriptShape: nglyphs=%d: ", nglyphs);

    for (j = 0; j < nglyphs; j++)
      g_print ("%d%s", iglyphs[j], (j < nglyphs-1) ? "," : "");
    g_print ("\n");

    g_print ("  log_clusters: ");
    for (j = 0; j < itemlen; j++)
      g_print ("%d ", log_clusters[j]);
    g_print ("\n");
    nclusters = 0;
    for (j = 0; j < itemlen; j++)
      {
	if (j == 0 || log_clusters[j-1] != log_clusters[j])
	  nclusters++;
      }
    g_print ("  %d clusters:\n", nclusters);

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
	      g_print ("  Cluster %d: chars %d--",
		       clusterix, charix);
	    if (j == 0 || log_clusters[j-1] != log_clusters[j])
	      {
		ng = log_clusters[j] - firstglyphix + 1;
		g_print ("%d: %d glyphs: ",
			 charix, ng);
		for (k = firstglyphix; k <= log_clusters[j]; k++)
		  {
		    g_print ("%d", iglyphs[k]);
		    if (k < log_clusters[j])
		      g_print (",");
		  }
		firstglyphix = log_clusters[j] + 1;
		clusterix++;
		g_print ("\n");
	      }
	  }
      }
    else
      {
	for (j = 0, charix = charix0; j < itemlen; j++, charix++)
	  {
	    if (j == 0 || log_clusters[j-1] != log_clusters[j])
	      g_print ("  Cluster %d: wchar_t %d--",
		       clusterix, charix);
	    if (j == itemlen - 1 || log_clusters[j] != log_clusters[j+1])
	      {
		int klim = ((j == itemlen-1) ? nglyphs : log_clusters[j+1]);
		ng = klim - log_clusters[j];
		g_print ("%d: %d glyphs: ",
			 charix, ng);
		for (k = log_clusters[j]; k < klim; k++)
		  {
		    g_print ("%d", iglyphs[k]);
		    if (k != klim - 1)
		      g_print (",");
		  }
		clusterix++;
		g_print ("\n");
	      }
	  }
      }
  }
}

#endif /* BASIC_WIN32_DEBUGGING */

static void
set_up_pango_log_clusters (wchar_t *wtext,
			   gboolean rtl,
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
				      PangoGlyphString *glyphs,
				      gint              utf16_len)
{
  const char *p;
  int charix, glyphix;
  int *byte_offset = g_new (int, utf16_len);

  p = text;
  charix = 0;
  while (p < text + length)
    {
      byte_offset[charix] = p - text;
      charix++;
      if (g_utf8_get_char (p) > 0xFFFF)
      {
	byte_offset[charix] = p - text;
	charix++;
      }
      p = g_utf8_next_char (p);
    }
  g_assert (charix <= utf16_len);

  /* Convert utf16 indexes in the log_clusters array to byte offsets.
   */
  for (glyphix = 0; glyphix < glyphs->num_glyphs; glyphix++)
    {
      g_assert (glyphs->log_clusters[glyphix] < utf16_len);
      glyphs->log_clusters[glyphix] = byte_offset[glyphs->log_clusters[glyphix]];
    }

  g_free (byte_offset);
}

typedef struct {
  HFONT hfont;
  gint32 script;
} script_cache_key;

static guint
script_cache_key_hash_func (script_cache_key *key)
{
  return g_direct_hash (key->hfont) ^ g_int_hash (&(key->script));
}

static gboolean
script_cache_key_equal_func (script_cache_key *a,
                             script_cache_key *b)
{
  return (a->hfont == b->hfont) && (a->script == b->script);
}

static gboolean
itemize_shape_and_place (PangoFont           *font,
			 HDC                  hdc,
			 wchar_t             *wtext,
			 int                  wlen,
			 const PangoAnalysis *analysis,
			 PangoGlyphString    *glyphs)
{
  int i;
  int item, nitems, item_step;
  int itemlen, glyphix, nglyphs;
  SCRIPT_CONTROL control;
  SCRIPT_STATE state;
  SCRIPT_ITEM items[100];
  double scale = pango_win32_font_get_metrics_factor (font);
  HFONT hfont = _pango_win32_font_get_hfont (font);
  static GHashTable *script_cache_hash = NULL;

  if (!script_cache_hash)
    script_cache_hash = g_hash_table_new ((GHashFunc)script_cache_key_hash_func,
                                          (GEqualFunc)script_cache_key_equal_func);

  memset (&control, 0, sizeof (control));
  memset (&state, 0, sizeof (state));

  control.uDefaultLanguage = make_langid (analysis->language);
  state.uBidiLevel = analysis->level;

#ifdef BASIC_WIN32_DEBUGGING
  if (pango_win32_debug)
    g_print (G_STRLOC ": ScriptItemize: uDefaultLanguage:%04x uBidiLevel:%d\n",
	     control.uDefaultLanguage, state.uBidiLevel);
#endif
  if (ScriptItemize (wtext, wlen, G_N_ELEMENTS (items), &control, NULL,
		     items, &nitems))
    {
#ifdef BASIC_WIN32_DEBUGGING
      if (pango_win32_debug)
	g_print ("ScriptItemize failed\n");
#endif
      return FALSE;
    }

#ifdef BASIC_WIN32_DEBUGGING
  if (pango_win32_debug)
    g_print ("%d items:\n", nitems);
#endif

  if (analysis->level % 2)
    {
      item = nitems - 1;
      item_step = -1;
    }
  else
    {
      item = 0;
      item_step = 1;
    }

  for (i = 0; i < nitems; i++, item += item_step)
    {
      WORD iglyphs[1000];
      WORD log_clusters[1000];
      SCRIPT_VISATTR visattrs[1000];
      int advances[1000];
      GOFFSET offsets[1000];
      ABC abc;
      gint32 script = items[item].a.eScript;
      int ng;
      int char_offset;
      SCRIPT_CACHE *script_cache;
      script_cache_key font_and_script_key;

      memset (advances, 0, sizeof (advances));
      memset (offsets, 0, sizeof (offsets));
      memset (&abc, 0, sizeof (abc));

      /* Note that itemlen is number of wchar_t's i.e. surrogate pairs
       * count as two!
       */
      itemlen = items[item+1].iCharPos - items[item].iCharPos;
      char_offset = items[item].iCharPos;

#ifdef BASIC_WIN32_DEBUGGING
      if (pango_win32_debug)
       {
	static const SCRIPT_PROPERTIES **scripts;
	static int nscripts;
	if (!nscripts)
	  ScriptGetProperties (&scripts, &nscripts);
	g_print ("  Item %d: iCharPos=%d eScript=%d (%s) %s%s%s%s%s%s%s wchar_t %d--%d (%d)\n",
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
      /* Create a hash key based on hfont and script engine */
      font_and_script_key.hfont = hfont;
      font_and_script_key.script = script;

      /* Get the script cache for this hfont and script */
      script_cache = g_hash_table_lookup (script_cache_hash, &font_and_script_key);
      if (!script_cache)
	{
	  script_cache_key *key_n;
	  SCRIPT_CACHE *new_script_cache;

	  key_n = g_new (script_cache_key, 1);
	  *key_n = font_and_script_key;

	  new_script_cache = g_new0 (SCRIPT_CACHE, 1);
	  script_cache = new_script_cache;

	  /* Insert the new value */
	  g_hash_table_insert (script_cache_hash, key_n, new_script_cache);

#ifdef BASIC_WIN32_DEBUGGING
	  if (pango_win32_debug)
	    g_print ("  New SCRIPT_CACHE for font %p and script %d\n", hfont, script);
#endif
	}

      items[item].a.fRTL = analysis->level % 2;
      if (ScriptShape (hdc, script_cache,
		       wtext + items[item].iCharPos, itemlen,
		       G_N_ELEMENTS (iglyphs),
		       &items[item].a,
		       iglyphs,
		       log_clusters,
		       visattrs,
		       &nglyphs))
	{
#ifdef BASIC_WIN32_DEBUGGING
	  if (pango_win32_debug)
	    g_print ("pangowin32-shape: ScriptShape failed\n");
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

      set_up_pango_log_clusters (wtext + items[item].iCharPos,
				 items[item].a.fRTL, itemlen, log_clusters,
				 nglyphs, glyphs->log_clusters + ng,
				 char_offset);

      if (ScriptPlace (hdc, script_cache, iglyphs, nglyphs,
		       visattrs, &items[item].a,
		       advances, offsets, &abc))
	{
#ifdef BASIC_WIN32_DEBUGGING
	  if (pango_win32_debug)
	    g_print ("pangowin32-shape: ScriptPlace failed\n");
#endif
	  return FALSE;
	}

      for (glyphix = 0; glyphix < nglyphs; glyphix++)
	{
	  if (iglyphs[glyphix] != 0)
	    {
	      glyphs->glyphs[ng+glyphix].glyph = iglyphs[glyphix];
	      glyphs->glyphs[ng+glyphix].geometry.width = floor (0.5 + scale * advances[glyphix]);
	      glyphs->glyphs[ng+glyphix].geometry.x_offset =  floor (0.5 + scale * offsets[glyphix].du);
	      glyphs->glyphs[ng+glyphix].geometry.y_offset = -floor (0.5 + scale * offsets[glyphix].dv);
	    }
	  else
	    {
	      PangoRectangle logical_rect;
	      /* Should pass actual char that was not found to
	       * PANGO_GET_UNKNOWN_GLYPH(), but a bit hard to
	       * find out that at this point, so cheat and use 0.
	       */
	      PangoGlyph unk = PANGO_GET_UNKNOWN_GLYPH (0);

	      glyphs->glyphs[ng+glyphix].glyph = unk;
	      pango_font_get_glyph_extents (font, unk, NULL, &logical_rect);
	      glyphs->glyphs[ng+glyphix].geometry.width = logical_rect.width;
	      glyphs->glyphs[ng+glyphix].geometry.x_offset = 0;
	      glyphs->glyphs[ng+glyphix].geometry.y_offset = 0;
	    }
	}
    }

#ifdef BASIC_WIN32_DEBUGGING
  if (pango_win32_debug)
    {
      g_print ("  Pango log_clusters (level:%d), char index:", analysis->level);
      for (glyphix = 0; glyphix < glyphs->num_glyphs; glyphix++)
	g_print ("%d ", glyphs->log_clusters[glyphix]);
      g_print ("\n");
    }
#endif

  return TRUE;
}

static gboolean
uniscribe_shape (PangoFont           *font,
		 const char          *text,
		 gint                 length,
		 const PangoAnalysis *analysis,
		 PangoGlyphString    *glyphs)
{
  wchar_t *wtext;
  long wlen;
  HDC hdc = _pango_win32_hdc;
  gboolean retval = TRUE;

  if (!pango_win32_font_select_font (font, hdc))
    return FALSE;

  wtext = g_utf8_to_utf16 (text, length, NULL, &wlen, NULL);
  if (wtext == NULL)
    retval = FALSE;

  if (retval)
    {
      retval = itemize_shape_and_place (font, hdc, wtext, wlen, analysis, glyphs);
    }

  if (retval)
    {
      convert_log_clusters_to_byte_offsets (text, length, glyphs, wlen);
#ifdef BASIC_WIN32_DEBUGGING
      if (pango_win32_debug)
	{
	  int glyphix;

	  g_print ("  Pango log_clusters, byte offsets:");
	  for (glyphix = 0; glyphix < glyphs->num_glyphs; glyphix++)
	    g_print ("%d ", glyphs->log_clusters[glyphix]);
	  g_print ("\n");
	}
#endif
    }

  pango_win32_font_done_font (font);

  g_free (wtext);

  return retval && glyphs->num_glyphs > 0;
}

static gboolean
text_is_simple (const char *text,
		gint        length)
{
  gboolean retval;
  wchar_t *wtext;
  long wlen;

  wtext = (wchar_t *) g_utf8_to_utf16 (text, length, NULL, &wlen, NULL);
  if (wtext == NULL)
    return TRUE;

  retval = (ScriptIsComplex (wtext, wlen, SIC_COMPLEX) == S_FALSE);

  g_free (wtext);

#ifdef BASIC_WIN32_DEBUGGING
  if (pango_win32_debug)
    g_print ("text_is_simple: %.*s (%ld wchar_t): %s\n",
	     MIN (length, 10), text, wlen, retval ? "YES" : "NO");
#endif

  return retval;
}

void
_pango_win32_shape (PangoFont        	*font,
		    const char       	*text,
		    unsigned int     	 length,
		    const PangoAnalysis *analysis,
		    PangoGlyphString    *glyphs,
		    const char          *paragraph_text G_GNUC_UNUSED,
		    unsigned int         paragraph_length G_GNUC_UNUSED)
{
  int n_chars;
  int i;
  const char *p;

  g_return_if_fail (font != NULL);
  g_return_if_fail (text != NULL);
  g_return_if_fail (length >= 0);
  g_return_if_fail (analysis != NULL);

  if (!text_is_simple (text, length) &&
      uniscribe_shape (font, text, length, analysis, glyphs))
    return;

  n_chars = pango_utf8_strlen (text, length);

  pango_glyph_string_set_size (glyphs, n_chars);

  p = text;
  for (i = 0; i < n_chars; i++)
    {
      gunichar wc;
      gunichar mirrored_ch;
      PangoGlyph index;

      wc = g_utf8_get_char (p);

      if (analysis->level % 2)
	if (g_unichar_get_mirror_char (wc, &mirrored_ch))
	  wc = mirrored_ch;

      if (wc == 0xa0)	/* non-break-space */
	wc = 0x20;

      if (pango_is_zero_width (wc))
	{
	  set_glyph (font, glyphs, i, p - text, PANGO_GLYPH_EMPTY);
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
	    set_glyph (font, glyphs, i, p - text, PANGO_GET_UNKNOWN_GLYPH (wc));
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
