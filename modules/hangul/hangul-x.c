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
#include <stdlib.h>
#include <string.h>

#include "pangox.h"
#include "pango-engine.h"

#define SCRIPT_ENGINE_NAME "HangulScriptEngineX"

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
    SCRIPT_ENGINE_NAME,
    PANGO_ENGINE_TYPE_SHAPE,
    PANGO_RENDER_TYPE_X,
    hangul_ranges, G_N_ELEMENTS(hangul_ranges)
  }
};

static int n_script_engines = G_N_ELEMENTS (script_engines);

/*
 * X window system script engine portion
 */

static void
set_glyph (PangoGlyphString *glyphs,
	   int               i,
	   PangoFont        *font,
	   PangoXSubfont     subfont,
	   guint16           gindex)
{
  PangoRectangle logical_rect;

  glyphs->glyphs[i].glyph = PANGO_X_MAKE_GLYPH (subfont, gindex);
  
  glyphs->glyphs[i].geometry.x_offset = 0;
  glyphs->glyphs[i].geometry.y_offset = 0;

  pango_font_get_glyph_extents (font, glyphs->glyphs[i].glyph, NULL, &logical_rect);
  glyphs->glyphs[i].geometry.width = logical_rect.width;
}

static void
set_unknown_glyph (PangoGlyphString *glyphs,
		   int              *n_glyphs,
		   PangoFont        *font,
		   gunichar          wc,
		   int               cluster_offset)
{
  PangoRectangle logical_rect;
  gint i = *n_glyphs;

  (*n_glyphs)++;
  pango_glyph_string_set_size (glyphs, *n_glyphs);

  glyphs->glyphs[i].glyph = pango_x_get_unknown_glyph (font);
  
  glyphs->glyphs[i].geometry.x_offset = 0;
  glyphs->glyphs[i].geometry.y_offset = 0;

  pango_font_get_glyph_extents (font, glyphs->glyphs[i].glyph, NULL, &logical_rect);
  glyphs->glyphs[i].geometry.width = logical_rect.width;

  glyphs->log_clusters[i] = cluster_offset;
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


typedef void (* RenderSyllableFunc) (PangoFont *font, PangoXSubfont subfont,
				     gunichar2 *text, int length,
				     PangoGlyphString *glyphs,
				     int *n_glyphs, int cluster_offset);



#include "tables-johabfont.i"
#include "tables-ksc5601.i"

#define JOHAB_COMMON							      \
  int i;								      \
  guint16 gindex;							      \
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
      gunichar2 l, v, t;						      \
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
	      pango_glyph_string_set_size (glyphs, *n_glyphs + 1);	      \
	      set_glyph (glyphs, *n_glyphs, font, subfont, gindex);	      \
	      glyphs->log_clusters[*n_glyphs] = cluster_offset;		      \
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
	      pango_glyph_string_set_size (glyphs, *n_glyphs + 1);	      \
	      set_glyph (glyphs, *n_glyphs, font, subfont, gindex);	      \
	      glyphs->log_clusters[*n_glyphs] = cluster_offset;		      \
	      (*n_glyphs)++;						      \
	    }								      \
									      \
	  if (t != TFILL)						      \
	    {								      \
	      gindex = __jongseong_johabfont_base[t - TBASE] +		      \
		__jongseong_map[v - VBASE];				      \
	      pango_glyph_string_set_size (glyphs, *n_glyphs + 1);	      \
	      set_glyph (glyphs, *n_glyphs, font, subfont, gindex);	      \
	      glyphs->log_clusters[*n_glyphs] = cluster_offset;		      \
	      (*n_glyphs)++;						      \
	    }								      \
									      \
	  if (v == VFILL && t == TFILL) /* dummy for no zero width */	      \
	    {								      \
	      pango_glyph_string_set_size (glyphs, *n_glyphs + 1);	      \
	      set_glyph (glyphs, *n_glyphs, font, subfont, JOHAB_FILLER);     \
	      glyphs->log_clusters[*n_glyphs] = cluster_offset;		      \
	      (*n_glyphs)++;						      \
	    }								      \
									      \
	  return;							      \
	}								      \
    }

static void
render_syllable_with_johabs (PangoFont *font, PangoXSubfont subfont,
			     gunichar2 *text, int length,
			     PangoGlyphString *glyphs,
			     int *n_glyphs, int cluster_offset)
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
	      set_glyph (glyphs, *n_glyphs, font, subfont, gindex);
	      glyphs->log_clusters[*n_glyphs] = cluster_offset;
	      (*n_glyphs)++;
	    }
	  else
	    break;
	}
      if (j == 0)
	set_unknown_glyph (glyphs, n_glyphs, font, text[i], cluster_offset);
    }
}

static void
render_syllable_with_johab (PangoFont *font, PangoXSubfont subfont,
			    gunichar2 *text, int length,
			    PangoGlyphString *glyphs,
			    int *n_glyphs, int cluster_offset)
{
JOHAB_COMMON

  /* Render as uncomposed forms as a fallback.  */
  for (i = 0; i < length; i++)
    {
      int j;
      gunichar2 wc;

      wc = text[i];
      for (j = 0; (j < 3) && (__jamo_to_johabfont[wc-LBASE][j] != 0); j++)
	{
	  pango_glyph_string_set_size (glyphs, *n_glyphs + 1);
	  set_glyph (glyphs, *n_glyphs, font, subfont,
		     __jamo_to_johabfont[wc - LBASE][j]);
	  glyphs->log_clusters[*n_glyphs] = cluster_offset;
	  (*n_glyphs)++;
	  if (IS_L (wc))
	    {
	      pango_glyph_string_set_size (glyphs, *n_glyphs + 1);
	      set_glyph (glyphs, *n_glyphs, font, subfont, JOHAB_FILLER);
	      glyphs->log_clusters[*n_glyphs] = cluster_offset;
	      (*n_glyphs)++;
	    }
	}
      if (j == 0)
	set_unknown_glyph (glyphs, n_glyphs, font, wc, cluster_offset);
    }
}

static void
render_syllable_with_iso10646 (PangoFont *font, PangoXSubfont subfont,
			       gunichar2 *text, int length,
			       PangoGlyphString *glyphs,
			       int *n_glyphs, int cluster_offset)
{
  guint16 gindex;
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
	  set_glyph (glyphs, *n_glyphs, font, subfont, gindex);
	  glyphs->log_clusters[*n_glyphs] = cluster_offset;
	  (*n_glyphs)++;
	  return;
	}
    }

  /* Render as uncomposed forms as a fallback.  */
  for (i = 0; i < length; i++)
    {
      gindex = text[i];
      pango_glyph_string_set_size (glyphs, *n_glyphs + 1);
      set_glyph (glyphs, *n_glyphs, font, subfont, gindex);
      glyphs->log_clusters[*n_glyphs] = cluster_offset;
      (*n_glyphs)++;
    }
}

static void
render_syllable_with_ksc5601 (PangoFont *font, PangoXSubfont subfont,
			      gunichar2 *text, int length,
			      PangoGlyphString *glyphs,
			      int *n_glyphs, int cluster_offset)
{
  guint16 sindex;
  guint16 gindex;
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
		  set_glyph (glyphs, *n_glyphs, font, subfont, gindex);
		  glyphs->log_clusters[*n_glyphs] = cluster_offset;
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
	  set_glyph (glyphs, *n_glyphs, font, subfont,
		     __jamo_to_ksc5601[gindex - LBASE][j]);
	  glyphs->log_clusters[*n_glyphs] = cluster_offset;
	  (*n_glyphs)++;
	}
      if (j == 0)
	set_unknown_glyph (glyphs, n_glyphs, font, gindex, cluster_offset);
    }
}

static gboolean
subfont_has_korean (PangoFont    *font,
		    PangoXSubfont subfont)
{
  /* Check for syllables and for uncomposed jamos. We do this
   * very unscientifically - if we have the first glyph, we
   * assume we have them all. It might be better to try
   * for the last one, to account for lazy font designers.
   */

  if (!pango_x_has_glyph (font, PANGO_X_MAKE_GLYPH (subfont, 0xac00)))
    return FALSE;
  if (!pango_x_has_glyph (font, PANGO_X_MAKE_GLYPH (subfont, 0x1100)))
    return FALSE;

  return TRUE;
}

static gboolean
find_subfont (PangoFont *font, char **charsets, int n_charsets,
	      PangoXSubfont *subfont, RenderSyllableFunc *render_func)
{
  int i;
  int n_subfonts;
  PangoXSubfont *subfonts;
  int *subfont_charsets;
  
  n_subfonts = pango_x_list_subfonts (font, charsets, n_charsets, &subfonts, &subfont_charsets);

  *subfont = 0;

  for (i=0; i<n_subfonts; i++)
    {
      if (strcmp (charsets[subfont_charsets[i]], "johabs-1") == 0)
	{
	  *subfont = subfonts[i];
	  *render_func = render_syllable_with_johabs;
	  break;
	}
      else if (strcmp (charsets[subfont_charsets[i]], "johab-1") == 0)
	{
	  *subfont = subfonts[i];
	  *render_func = render_syllable_with_johab;
	  break;
	}
      else if (strcmp (charsets[subfont_charsets[i]], "iso10646-1") == 0)
	{
	  if (subfont_has_korean (font, subfonts[i]))
	    {
	      *subfont = subfonts[i];
	      *render_func = render_syllable_with_iso10646;
	      break;
	    }
	}
      else if (strcmp (charsets[subfont_charsets[i]], "ksc5601.1987-0") == 0)
	{
	  *subfont = subfonts[i];
	  *render_func = render_syllable_with_ksc5601;
	  break;
	}
    }

  g_free (subfonts);
  g_free (subfont_charsets);

  return (*subfont != 0);
}

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

static void 
hangul_engine_shape (PangoFont        *font,
		     const char       *text,
		     int               length,
		     PangoAnalysis    *analysis,
		     PangoGlyphString *glyphs)
{
  PangoXSubfont subfont;
  RenderSyllableFunc render_func = NULL;

  const char *ptr;
  const char *next;
  int i, n_chars;
  gunichar2 jamos_static[4];
  guint jamos_max = G_N_ELEMENTS (jamos_static);
  gunichar2 *jamos = jamos_static;
  int n_jamos = 0;

  int n_glyphs = 0, cluster_offset = 0;

  g_return_if_fail (font != NULL);
  g_return_if_fail (text != NULL);
  g_return_if_fail (length >= 0);
  g_return_if_fail (analysis != NULL);

  /* check available fonts. Always use a johab font if available;
   * otherwise use iso-10646 or KSC font depending on the ordering
   * of the fontlist.
   */
  if (!find_subfont (font, default_charset, G_N_ELEMENTS (default_charset), &subfont, &render_func))
    if (!find_subfont (font, secondary_charset, G_N_ELEMENTS (secondary_charset), &subfont, &render_func))
      if (!find_subfont (font, fallback_charsets, G_N_ELEMENTS (fallback_charsets), &subfont, &render_func))
	{
	  n_chars = g_utf8_strlen (text, length);
	  pango_x_fallback_shape (font, glyphs, text, n_chars);
	  return;
	}

  n_chars = g_utf8_strlen (text, length);
  ptr = text;
  
  for (i = 0; i < n_chars; i++)
    {
      gunichar wc4;
      gunichar2 wcs[4], wc;
      int n_code = 0;

      wc4 = g_utf8_get_char (ptr);
      next = g_utf8_next_char (ptr);
      
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
	      (*render_func) (font, subfont, jamos, n_jamos,
			      glyphs, &n_glyphs, cluster_offset);
	      cluster_offset = next - text;
	      n_jamos = 0;
	    }

	  /* Draw a syllable.  */
	  (*render_func) (font, subfont, wcs, 3,
			  glyphs, &n_glyphs, cluster_offset);
	  cluster_offset = next - text;
	  /* Clear.  */
	}
      else if (wc4 >= 0x1100 && wc4 <= 0x11ff)
	{
	  wc = (gunichar2) wc4;

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
		  (*render_func) (font, subfont, jamos, n_jamos,
				  glyphs, &n_glyphs, cluster_offset);
		  cluster_offset = next - text;
		  /* Clear.  */
		  n_jamos = 0;
		}
	      if (n_jamos == jamos_max)
		{
		  gunichar2 *new_jamos;
		    
		  jamos_max++;
		  new_jamos = g_new (gunichar2, jamos_max);
		  memcpy (new_jamos, jamos, n_jamos * sizeof (gunichar2));

		  jamos = new_jamos;
		}
	      jamos[n_jamos++] = wc;
	    }
	}
      else
	{
	  g_warning ("Character not handled by Hangul shaper: %#04x", wc4);
	  continue;
	}

      ptr = next;
    }

  /* Draw the remaining Jamos.  */ 
  if (n_jamos > 0)
    {
      (*render_func) (font, subfont, jamos, n_jamos,
		      glyphs, &n_glyphs, cluster_offset);
      cluster_offset = next - text;
      n_jamos = 0;
    }

  if (jamos != jamos_static)
    g_free (jamos);
}

static PangoCoverage *
hangul_engine_get_coverage (PangoFont  *font,
			     PangoLanguage *lang)
{
  PangoCoverage *result = pango_coverage_new ();
  PangoXSubfont subfont;
  RenderSyllableFunc render_func = NULL;
  int i;

  /* An approximate implementation, please check and fix as necessary!
   *                                           OWT 2 Feb 2000
   */
  if (find_subfont (font, default_charset, G_N_ELEMENTS (default_charset), &subfont, &render_func) ||
      find_subfont (font, secondary_charset, G_N_ELEMENTS (secondary_charset), &subfont, &render_func) ||
      find_subfont (font, fallback_charsets, G_N_ELEMENTS (fallback_charsets), &subfont, &render_func))
    {
      if (render_func == render_syllable_with_johabs ||
	  render_func == render_syllable_with_johab)
	{
	  for (i = 0x1100; i <= 0x11ff; i++)
	    pango_coverage_set (result, i, PANGO_COVERAGE_EXACT);

	  for (i = 0xac00; i <= 0xd7a3; i++)
	    pango_coverage_set (result, i, PANGO_COVERAGE_EXACT);

	}
      else if (render_func == render_syllable_with_iso10646)
	{
	  for (i = 0x1100; i <= 0x11ff; i++)
	    pango_coverage_set (result, i, PANGO_COVERAGE_FALLBACK);

	  for (i = 0xac00; i <= 0xd7a3; i++)
	    pango_coverage_set (result, i, PANGO_COVERAGE_EXACT);
	}
      else if (render_func == render_syllable_with_ksc5601)
	{
	  for (i = 0x1100; i <= 0x11ff; i++)
	    pango_coverage_set (result, i, PANGO_COVERAGE_FALLBACK);

	  for (i = 0xac00; i <= 0xd7a3; i++)
	    pango_coverage_set (result, i, PANGO_COVERAGE_FALLBACK);

	  for (i=0; i<KSC5601_HANGUL; i++)
	    pango_coverage_set (result, __ksc5601_hangul_to_ucs[i], PANGO_COVERAGE_EXACT);
	}
      else
	g_assert_not_reached();
    }

  return result;
}

static PangoEngine *
hangul_engine_x_new ()
{
  PangoEngineShape *result;

  result = g_new (PangoEngineShape, 1);

  result->engine.id = SCRIPT_ENGINE_NAME;
  result->engine.type = PANGO_ENGINE_TYPE_SHAPE;
  result->engine.length = sizeof (result);
  result->script_shape = hangul_engine_shape;
  result->get_coverage = hangul_engine_get_coverage;

  return (PangoEngine *)result;
}



/* The following three functions provide the public module API for
 * Pango
 */

#ifdef X_MODULE_PREFIX
#define MODULE_ENTRY(func) _pango_hangul_x_##func
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
    return hangul_engine_x_new ();
  else
    return NULL;
}

void 
MODULE_ENTRY(script_engine_unload) (PangoEngine *engine)
{
}

