/* Pango
 * hangul.c:
 *
 * Copyright (C) 1999 Changwoo Ryu
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

#include <glib.h>

#include "pango.h"
#include "pangox.h"
#include "utils.h"
#include <unicode.h>

#define MEMBERS(strct) sizeof(strct) / sizeof(strct[1])

static PangoEngineRange hangul_ranges[] = {

  /* Hangul Jamo U+1100 -- U+11FF */
  { 0x1100, 0x11ff, "*" },
  /* Hangul Compatibility Jamo U+3130 -- U+318F */
/*    { 0x3130, 0x318f, "*" }, */
  /* Hangul (pre-composed) Syllables U+AC00 -- U+D7A3 */
  { 0xac00, 0xd7a3, "*" }
};


static PangoEngineInfo script_engines[] = {
  {
    "HangulScriptEngineLang",
    PANGO_ENGINE_TYPE_LANG,
    PANGO_RENDER_TYPE_NONE,
    hangul_ranges, MEMBERS(hangul_ranges)
  },
  {
    "HangulScriptEngineX",
    PANGO_ENGINE_TYPE_SHAPE,
    PANGO_RENDER_TYPE_X,
    hangul_ranges, MEMBERS(hangul_ranges)
  }
};

static gint n_script_engines = MEMBERS (script_engines);

/*
 * Language script engine
 */

static void 
hangul_engine_break (gchar            *text,
		     gint             len,
		     PangoAnalysis *analysis,
		     PangoLogAttr  *attrs)
{
  /* (FIXME) */
}

static PangoEngine *
hangul_engine_lang_new ()
{
  PangoEngineLang *result;

  result = g_new (PangoEngineLang, 1);

  result->engine.id = "HangulScriptEngine";
  result->engine.type = PANGO_ENGINE_TYPE_LANG;
  result->engine.length = sizeof (result);
  result->script_break = hangul_engine_break;

  return (PangoEngine *) result;
}

/*
 * X window system script engine portion
 */

static void
set_glyph (PangoGlyphString *glyphs, gint i, PangoCFont *cfont,
	   PangoGlyph glyph)
{
  gint width;

  glyphs->glyphs[i].font = cfont;
  glyphs->glyphs[i].glyph = glyph;
  
  glyphs->geometry[i].x_offset = 0;
  glyphs->geometry[i].y_offset = 0;

  pango_x_glyph_extents (&glyphs->glyphs[i],
			    NULL, NULL, &width, NULL, NULL, NULL, NULL);
  glyphs->geometry[i].width = width * 72;
}


/*
 * From 3.10 of the Unicode 2.0 Book; used for combining Jamos.
 */

#define SBASE 0xAC00
#define LBASE 0x1100
#define VBASE 0x1161
#define TBASE 0x11A7
#define SCOUNT 11172
#define LCOUNT 19
#define VCOUNT 21
#define TCOUNT 28
#define NCOUNT (VCOUNT * TCOUNT)

/*
 * Unicode 2.0 doesn't define the fill for trailing consonants, but
 * I'll use 0x11A7 as that purpose internally.
 */

#define LFILL 0x115F
#define VFILL 0x1160
#define TFILL 0x11A7

#define IS_L(wc) (wc >= 0x1100 && wc < 0x115F)
#define IS_V(wc) (wc >= 0x1160 && wc < 0x11A2)
#define IS_T(wc) (wc >= 0x11A7 && wc < 0x11F9)


typedef void (* RenderSyllableFunc) (PangoCFont *cfont,
				     GUChar2 *text, int length,
				     PangoGlyphString *glyphs,
				     int *n_glyphs, int n_clusters);



#include "tables-johabfont.i"
#include "tables-ksc5601.i"

#define JOHAB_COMMON							      \
  int i;								      \
  PangoGlyph gindex;							      \
									      \
  /*									      \
   * Check if there are one CHOSEONG, one JUNGSEONG, and no more	      \
   * than one JONGSEONG.						      \
   */									      \
  int n_cho = 0, n_jung = 0, n_jong = 0;				      \
  i = 0;								      \
  while (i < length && IS_L (text[i]))					      \
    {									      \
      n_cho++;								      \
      i++;								      \
    }									      \
  while (i < length && IS_V (text[i]))					      \
    {									      \
      n_jung++;								      \
      i++;								      \
    }									      \
  while (i < length && IS_T (text[i]))					      \
    {									      \
      n_jong++;								      \
      i++;								      \
    }									      \
									      \
  if (n_cho <= 1 && n_jung <= 1 && n_jong <= 1)				      \
    {									      \
      GUChar2 l, v, t;							      \
									      \
      if (n_cho > 0)							      \
	l = text[0];							      \
      else								      \
	l = LFILL;							      \
									      \
      if (n_jung > 0)							      \
	v = text[n_cho];						      \
      else								      \
	v = VFILL;							      \
									      \
      if (n_jong > 0)							      \
	t = text[n_cho + n_jung];					      \
      else								      \
	t = TFILL;							      \
									      \
      /* COMPOSABLE */							      \
      if ((__choseong_johabfont_base[l - LBASE] != 0 || l == LFILL) &&	      \
	  (__jungseong_johabfont_base[v - VBASE] != 0 || v == VFILL) &&	      \
	  (__jongseong_johabfont_base[t - TBASE] != 0 || t == TFILL))	      \
	{								      \
	  if (l != LFILL)						      \
	    {								      \
	      gindex = __choseong_johabfont_base[l - LBASE];		      \
	      if (t == TFILL)						      \
		{							      \
		  if (v == VFILL)					      \
		    gindex += 1;					      \
		  else							      \
		    gindex += __choseong_map_1[v - VBASE];		      \
		}							      \
	      else							      \
		{							      \
		  if (v == VFILL)					      \
		    gindex += 6;					      \
		  else							      \
		    gindex += __choseong_map_2[v - VBASE];		      \
		}							      \
	      pango_glyph_string_set_size (glyphs, *n_glyphs + 1);		      \
	      set_glyph (glyphs, *n_glyphs, cfont, gindex);		      \
	      glyphs->log_clusters[*n_glyphs] = n_clusters;		      \
	      (*n_glyphs)++;						      \
	    }								      \
									      \
	  if (v != VFILL)						      \
	    {								      \
	      gindex = __jungseong_johabfont_base[v - VBASE];		      \
	      switch (__johabfont_jungseong_kind[v - VBASE])		      \
		{							      \
		case 3:							      \
		  gindex += __johabfont_jongseong_kind[t - TBASE];	      \
		  break;						      \
		case 4:							      \
		  gindex += ((l == 0x1100 || l == 0x110f) ? 0 : 1) +	      \
		    ((t != TFILL) ? 2 : 0);				      \
		  break;						      \
		}							      \
  									      \
	      pango_glyph_string_set_size (glyphs, *n_glyphs + 1);		      \
	      set_glyph (glyphs, *n_glyphs, cfont, gindex);		      \
	      glyphs->log_clusters[*n_glyphs] = n_clusters;		      \
	      (*n_glyphs)++;						      \
	    }								      \
									      \
	  if (t != TFILL)						      \
	    {								      \
	      gindex = __jongseong_johabfont_base[t - TBASE] +		      \
		__jongseong_map[v - VBASE];				      \
	      pango_glyph_string_set_size (glyphs, *n_glyphs + 1);		      \
	      set_glyph (glyphs, *n_glyphs, cfont, gindex);		      \
	      glyphs->log_clusters[*n_glyphs] = n_clusters;		      \
	      (*n_glyphs)++;						      \
	    }								      \
									      \
	  if (v == VFILL && t == TFILL) /* dummy for no zero width */	      \
	    {								      \
	      pango_glyph_string_set_size (glyphs, *n_glyphs + 1);		      \
	      set_glyph (glyphs, *n_glyphs, cfont, JOHAB_FILLER);	      \
	      glyphs->log_clusters[*n_glyphs] = n_clusters;		      \
	      (*n_glyphs)++;						      \
	    }								      \
									      \
	  return;							      \
	}								      \
    }

static void
render_syllable_with_johabs (PangoCFont *cfont,
			     GUChar2 *text, int length,
			     PangoGlyphString *glyphs,
			     int *n_glyphs, int n_clusters)
{
JOHAB_COMMON

  /* Render as uncomposed forms as a fallback.  */
  for (i = 0; i < length; i++)
    {
      int j;

      /*
       * Uses KSC5601 symbol glyphs which johabS-1 has; they're
       * not in the normal johab-1.  The glyphs are better than composable
       * components. 
       */
      for (j = 0; j < 3; j++)
	{
	  gindex = __jamo_to_ksc5601[text[i] - LBASE][j];
	  
	  if (gindex != 0)
	    {
	      if ((gindex >= 0x2421) && (gindex <= 0x247f))
		gindex += (0x032b - 0x2420);
	      else if ((gindex >= 0x2421) && (gindex <= 0x247f))
		gindex += (0x02cd - 0x2321);
	      else
		break;
	      
	      pango_glyph_string_set_size (glyphs, *n_glyphs + 1);
	      set_glyph (glyphs, *n_glyphs, cfont, gindex);
	      glyphs->log_clusters[*n_glyphs] = n_clusters;
	      (*n_glyphs)++;
	    }
	  else
	    break;
	}
    }
}

static void
render_syllable_with_johab (PangoCFont *cfont,
			    GUChar2 *text, int length,
			    PangoGlyphString *glyphs,
			    int *n_glyphs, int n_clusters)
{
JOHAB_COMMON

  /* Render as uncomposed forms as a fallback.  */
  for (i = 0; i < length; i++)
    {
      int j;
      GUChar2 wc;

      wc = text[i];
      for (j = 0; (j < 3) && (__jamo_to_johabfont[wc-LBASE][j] != 0); j++)
	{
	  pango_glyph_string_set_size (glyphs, *n_glyphs + 1);
	  set_glyph (glyphs, *n_glyphs, cfont,
		     __jamo_to_johabfont[wc - LBASE][j]);
	  glyphs->log_clusters[*n_glyphs] = n_clusters;
	  (*n_glyphs)++;
	  if (IS_L (wc))
	    {
	      pango_glyph_string_set_size (glyphs, *n_glyphs + 1);
	      set_glyph (glyphs, *n_glyphs, cfont, JOHAB_FILLER);
	      glyphs->log_clusters[*n_glyphs] = n_clusters;
	      (*n_glyphs)++;
	    }
	}
    }
}

static void
render_syllable_with_iso10646 (PangoCFont *cfont,
			       GUChar2 *text, int length,
			       PangoGlyphString *glyphs,
			       int *n_glyphs, int n_clusters)
{
  PangoGlyph gindex;
  int i;
  
  /*
   * Check if there are one CHOSEONG, one JUNGSEONG, and no more
   * than one JONGSEONG.
   */
  int n_cho = 0, n_jung = 0, n_jong = 0;
  i = 0;
  while (i < length && IS_L (text[i]))
    {
      n_cho++;
      i++;
    }
  while (i < length && IS_V (text[i]))
    {
      n_jung++;
      i++;
    }
  while (i < length && IS_T (text[i]))
    {
      n_jong++;
      i++;
    }
  
  if (n_cho == 1 && n_jung == 1 && n_jong <= 1)
    {
      int lindex, vindex, tindex;

      lindex = text[0] - LBASE;
      vindex = text[1] - VBASE;
      if (n_jong > 0)
	tindex = text[2] - TBASE;
      else
	tindex = 0;

      /* COMPOSABLE */
      if (lindex >= 0 && lindex < LCOUNT &&
	  vindex >= 0 && vindex < VCOUNT &&
	  tindex >= 0 && tindex < TCOUNT)
	{
	  gindex = (lindex * VCOUNT + vindex) * TCOUNT + tindex + SBASE;
	  /* easy for composed syllables. */
	  pango_glyph_string_set_size (glyphs, *n_glyphs + 1);
	  set_glyph (glyphs, *n_glyphs, cfont, gindex);
	  glyphs->log_clusters[*n_glyphs] = n_clusters;
	  (*n_glyphs)++;
	  return;
	}
    }

  /* Render as uncomposed forms as a fallback.  */
  for (i = 0; i < length; i++)
    {
      gindex = text[i];
      pango_glyph_string_set_size (glyphs, *n_glyphs + 1);
      set_glyph (glyphs, *n_glyphs, cfont, gindex);
      glyphs->log_clusters[*n_glyphs] = n_clusters;
      (*n_glyphs)++;
    }
}

static void
render_syllable_with_ksc5601 (PangoCFont *cfont,
			      GUChar2 *text, int length,
			      PangoGlyphString *glyphs,
			      int *n_glyphs, int n_clusters)
{
  guint16 sindex;
  PangoGlyph gindex;
  int i;

  /*
   * Check if there are one CHOSEONG, one JUNGSEONG, and no more
   * than one JONGSEONG.
   */
  int n_cho = 0, n_jung = 0, n_jong = 0;
  i = 0;
  while (i < length && IS_L (text[i]))
    {
      n_cho++;
      i++;
    }
  while (i < length && IS_V (text[i]))
    {
      n_jung++;
      i++;
    }
  while (i < length && IS_T (text[i]))
    {
      n_jong++;
      i++;
    }
  
  if (n_cho == 1 && n_jung == 1 && n_jong <= 1)
    {
      int lindex, vindex, tindex;

      lindex = text[0] - LBASE;
      vindex = text[1] - VBASE;
      if (n_jong > 0)
	tindex = text[2] - TBASE;
      else
	tindex = 0;

      /* COMPOSABLE */
      if (lindex >= 0 && lindex < LCOUNT &&
	  vindex >= 0 && vindex < VCOUNT &&
	  tindex >= 0 && tindex < TCOUNT)
	{
	  int l = 0;
	  int u = KSC5601_HANGUL - 1;
	  guint16 try;

	  sindex = (lindex * VCOUNT + vindex) * TCOUNT + tindex + SBASE;

	  /* Do binary search. */
	  while (l <= u)
	    {
	      int m = (l + u) / 2;
	      try = __ksc5601_hangul_to_ucs[m];
	      if (try > sindex)
		u = m - 1;
	      else if (try < sindex)
		l = m + 1;
	      else
		{
		  gindex = (((m / 94) + 0x30) << 8) | ((m % 94) + 0x21);

		  pango_glyph_string_set_size (glyphs, *n_glyphs + 1);
		  set_glyph (glyphs, *n_glyphs, cfont, gindex);
		  glyphs->log_clusters[*n_glyphs] = n_clusters;
		  (*n_glyphs)++;
		  return;
		}
	    }
	}
    }

  for (i = 0; i < length; i++)
    {
      int j;
      
      gindex = text[i];
      for (j = 0; (j < 3) && (__jamo_to_ksc5601[gindex - LBASE][j] != 0); j++)
	{
	  pango_glyph_string_set_size (glyphs, *n_glyphs + 1);
	  set_glyph (glyphs, *n_glyphs, cfont,
		     __jamo_to_ksc5601[gindex - LBASE][j]);
	  glyphs->log_clusters[*n_glyphs] = n_clusters;
	  (*n_glyphs)++;
	}
    }
}

static gboolean
ranges_include_korean (int *ranges,
		       int  n_ranges)
{
  gboolean have_syllables = FALSE;
  gboolean have_jamos = FALSE;
  gint i;
  
  /* Check for syllables and for uncomposed jamos */

  for (i=0; i<n_ranges; i++)
    {
      if (ranges[2*i] <= 0xac00 && ranges[2*i+1] >= 0xd7a3)
	have_syllables = 1;
      if (ranges[2*i] <= 0x1100 && ranges[2*i+1] >= 0x11ff)
	have_jamos = 1;
    }

  return have_syllables && have_jamos;
}

static gboolean
find_charset (PangoFont *font, gchar **charsets, gint n_charsets,
	      PangoXCharset *charset, RenderSyllableFunc *render_func)
{
  int i;
  char **xlfds;
  int n_xlfds;
  
  pango_x_list_cfonts (font, charsets, n_charsets, &xlfds, &n_xlfds);

  *cfont = NULL;
  for (i=0; i<n_xlfds; i++)
    {
      if (match_end (xlfds[i], "johabs-1"))
	{
	  *cfont = pango_x_load_xlfd (font, xlfds[i]);
	  *render_func = render_syllable_with_johabs;
	  break;
	}
      if (match_end (xlfds[i], "johab-1"))
	{
	  *cfont = pango_x_load_xlfd (font, xlfds[i]);
	  *render_func = render_syllable_with_johab;
	  break;
	}
      else if (match_end (xlfds[i], "iso10646-1"))
	{
	  gint *ranges;
	  int n_ranges;
	  
	  pango_x_xlfd_get_ranges (font, xlfds[i], &ranges, &n_ranges);

	  if (ranges_include_korean (ranges, n_ranges))
	    {
	      *cfont = pango_x_load_xlfd (font, xlfds[i]);
	      *render_func = render_syllable_with_iso10646;
	      g_free (ranges);
	      break;
	    }

	  g_free (ranges);
	}
      else if (match_end (xlfds[i], "ksc5601.1987-0"))
	{
	  *cfont = pango_x_load_xlfd (font, xlfds[i]);
	  *render_func = render_syllable_with_ksc5601;
	  break;
	}
    }

  for (i=0; i<n_xlfds; i++)
    g_free (xlfds[i]);
  g_free (xlfds);

  return (*cfont != NULL);
}

static void 
hangul_engine_shape (PangoFont        *font,
		     gchar            *text,
		     gint              length,
		     PangoAnalysis    *analysis,
		     PangoGlyphString *glyphs)
{
  PangoXCharset *charset_id;
  RenderSyllableFunc render_func = NULL;

  char *ptr, *next;
  int i, n_chars;
  GUChar2 jamos[4];
  int n_jamos = 0;

  int n_glyphs = 0, n_clusters = 0;

  static char *default_charset[] = {
    "johabs-1"
  };
  
  static char *secondary_charset[] = {
    "johab-1"
  };
  
  static char *fallback_charsets[] = {
    "iso10646-1",
    "ksc5601.1987-0"
  };

  g_return_if_fail (font != NULL);
  g_return_if_fail (text != NULL);
  g_return_if_fail (length >= 0);
  g_return_if_fail (analysis != NULL);

  /* check available fonts. Always use a johab font if available;
   * otherwise use iso-10646 or KSC font depending on the ordering
   * of the fontlist.
   */
  if (!find_charset (font, default_charset, 1, &cfont, &render_func))
    if (!find_charset (font, secondary_charset, 1, &cfont, &render_func))
      if (!find_charset (font, fallback_charsets, 2, &cfont, &render_func))
	{
	  g_warning ("No available Hangul fonts.");
	  return;
	}

  n_chars = unicode_strlen (text, length);
  ptr = text;
  
  for (i = 0; i < n_chars; i++)
    {
      GUChar4 wc4;
      GUChar2 wcs[4], wc;
      int n_code = 0;

      _pango_utf8_iterate (ptr, &next, &wc4);
      if (wc4 >= SBASE && wc4 < (SBASE + SCOUNT))
	{
	  /* decompose the syllable.  */
	  gint16 sindex;

	  sindex = wc4 - SBASE;
	  wcs[0] = LBASE + (sindex / NCOUNT);
	  wcs[1] = VBASE + ((sindex % NCOUNT) / TCOUNT);
	  wcs[2] = TBASE + (sindex % TCOUNT);
	  n_code = 3;

	  if (n_jamos > 0)
	    {
	      (*render_func) (cfont, jamos, n_jamos,
			      glyphs, &n_glyphs, n_clusters);
	      n_clusters++;
	      n_jamos = 0;
	    }

	  /* Draw a syllable.  */
	  (*render_func) (cfont, wcs, 3,
			  glyphs, &n_glyphs, n_clusters);
	  n_clusters++;
	  /* Clear.  */
	}
      else if (wc4 >= 0x1100 && wc4 <= 0x11ff)
	{
	  wc = (GUChar2) wc4;

	  if (n_jamos == 0)
	    {
	      jamos[n_jamos++] = wc;
	    }
	  else
	    {
	      /* Check syllable boundaries. */
	      if ((IS_T (jamos[n_jamos - 1]) && IS_L (wc)) ||
		  (IS_V (jamos[n_jamos - 1]) && IS_L (wc)) ||
		  (IS_T (jamos[n_jamos - 1]) && IS_V (wc)))
		{
		  /* Draw a syllable.  */
		  (*render_func) (cfont, jamos, n_jamos,
				  glyphs, &n_glyphs, n_clusters);
		  n_clusters++;
		  /* Clear.  */
		  n_jamos = 0;
		}
	      jamos[n_jamos++] = wc;
	    }
	}
      else
	{
	  g_warning ("Unknown character 0x04%x", wc4);
	  continue;
	}

      ptr = next;
    }

  /* Draw the remaining Jamos.  */ 
  if (n_jamos > 0)
    {
      (*render_func) (cfont, jamos, n_jamos,
		      glyphs, &n_glyphs, n_clusters);
      n_clusters++;
      n_jamos = 0;
    }
}


static PangoEngine *
hangul_engine_x_new ()
{
  PangoEngineShape *result;

  result = g_new (PangoEngineShape, 1);

  result->engine.id = "HangulScriptEngine";
  result->engine.type = PANGO_ENGINE_TYPE_LANG;
  result->engine.length = sizeof (result);
  result->script_shape = hangul_engine_shape;

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
  if (!strcmp (id, "HangulScriptEngineLang"))
    return hangul_engine_lang_new ();
  else if (!strcmp (id, "HangulScriptEngineX"))
    return hangul_engine_x_new ();
  else
    return NULL;
}

void 
script_engine_unload (PangoEngine *engine)
{
}

