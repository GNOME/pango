/* Pango
 * hangul-xft.c:
 *
 * Copyright (C) 2002 Changwoo Ryu
 * Author: Changwoo Ryu <cwryu@debian.org>
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

#include <string.h>

#include "pangoxft.h"
#include "pango-engine.h"
#include "pango-utils.h"

#include "hangul-defs.h"
#include "tables-jamos.i"

#define SCRIPT_ENGINE_NAME "HangulScriptEngineXft"

static PangoEngineRange hangul_ranges[] = {
  /* Language characters */
  { 0x1100, 0x11ff, "*" }, /* Hangul Jamo */
  { 0x302e, 0x302f, "*" }, /* Hangul Tone Marks */
  { 0xac00, 0xd7a3, "*" }, /* Hangul Syllables */
};

static PangoEngineInfo script_engines[] = {
  {
    SCRIPT_ENGINE_NAME,
    PANGO_ENGINE_TYPE_SHAPE,
    PANGO_RENDER_TYPE_XFT,
    hangul_ranges, G_N_ELEMENTS(hangul_ranges)
  }
};

static void
set_glyph (PangoFont *font, PangoGlyphString *glyphs, int i, int offset, PangoGlyph glyph)
{
  PangoRectangle logical_rect;

  glyphs->glyphs[i].glyph = glyph;
  glyphs->glyphs[i].geometry.x_offset = 0;
  glyphs->glyphs[i].geometry.y_offset = 0;
  glyphs->log_clusters[i] = offset;

  pango_font_get_glyph_extents (font, glyphs->glyphs[i].glyph, NULL, &logical_rect);
  glyphs->glyphs[i].geometry.width = logical_rect.width;
}

/* Add a Hangul tone mark glyph in a glyph string.
 * Non-spacing glyph works pretty much automatically.
 * Spacing-glyph takes some care:
 *   1. Make a room for a tone mark at the beginning(leftmost end) of a cluster 
 *   to attach it to.
 *   2. Adjust x_offset so that it is drawn to the left of a cluster.
 *   3. Set the logical width to zero. 
 */

static void
set_glyph_tone (PangoFont *font, PangoGlyphString *glyphs, int i,
		            int offset, PangoGlyph glyph)
{
  PangoRectangle logical_rect, ink_rect;
  PangoRectangle logical_rect_cluster;

  glyphs->glyphs[i].glyph = glyph;
  glyphs->glyphs[i].geometry.y_offset = 0;
  glyphs->log_clusters[i] = offset;

  pango_font_get_glyph_extents (font, glyphs->glyphs[i].glyph, 
				&ink_rect, &logical_rect);

  /* tone mark is not the first in a glyph string. We have info. on the 
   * preceding glyphs in a glyph string 
   */
    {
      int j = i - 1;
      /* search for the beg. of the preceding cluster */
      while (j >= 0 && glyphs->log_clusters[j] == glyphs->log_clusters[i - 1]) 
	j--;

      /* In .._extents_range(...,start,end,...), to my surprise  start is 
       * inclusive but end is exclusive !! 
       */
      pango_glyph_string_extents_range (glyphs, j + 1, i, font, 
					NULL, &logical_rect_cluster); 

      /* logical_rect_cluster.width is all the offset we need so that the
       * inherent x_offset in the glyph (ink_rect.x) should be canceled out.
       */
      glyphs->glyphs[i].geometry.x_offset = - logical_rect_cluster.width
					    - ink_rect.x ; 
					    

      /* make an additional room for a tone mark if it has a spacing glyph
       * because that's likely to be an indication that glyphs for other 
       * characters in the font are not designed for combining with tone marks.
       */
      if (logical_rect.width)
	{
	  glyphs->glyphs[i].geometry.x_offset -= ink_rect.width;
          glyphs->glyphs[j + 1].geometry.width += ink_rect.width;
          glyphs->glyphs[j + 1].geometry.x_offset += ink_rect.width;
	}
    }

  glyphs->glyphs[i].geometry.width = 0;
}


#define find_char pango_xft_font_get_glyph


static void
render_tone (PangoFont *font, gunichar tone, PangoGlyphString *glyphs,
             int *n_glyphs, int cluster_offset)
{
  int index;

  index = find_char (font, tone);
  pango_glyph_string_set_size (glyphs, *n_glyphs + 1);
  if (index)
    {
      set_glyph_tone (font, glyphs, *n_glyphs, cluster_offset, index);
    }
  else 
    {
      /* fall back : HTONE1(0x302e) => middle-dot, HTONE2(0x302f) => colon */
      index = find_char (font, tone == HTONE1 ? 0x00b7 : 0x003a);
      if (index)
        {
          set_glyph_tone (font, glyphs, *n_glyphs, cluster_offset, index);
        }
      else 
        set_glyph (font, glyphs, *n_glyphs, cluster_offset,
                   pango_xft_font_get_unknown_glyph (font, tone));
    }
  (*n_glyphs)++;
}

/* This is a fallback for when we get a tone mark not preceded
 * by a syllable.
 */
static void
render_isolated_tone (PangoFont *font, gunichar tone, PangoGlyphString *glyphs,
		      int *n_glyphs, int cluster_offset)
{
  /* Find a base character to render the mark on
   */
  int index = find_char (font, 0x25cc);	/* DOTTED CIRCLE */
  if (!index)
    index = find_char (font, 0x25cb);   /* WHITE CIRCLE, in KSC-5601 */
  if (!index)
    index = find_char (font, ' ');      /* Space */
  if (!index)			        /* Unknown glyph box with 0000 in it */
    index = find_char (font, pango_xft_font_get_unknown_glyph (font, 0));

  /* Add the base character
   */
  pango_glyph_string_set_size (glyphs, *n_glyphs + 1);
  set_glyph (font, glyphs, *n_glyphs, cluster_offset, index);
  (*n_glyphs)++;

  /* And the tone mrak
   */
  render_tone(font, tone, glyphs, n_glyphs, cluster_offset);
}

static void
render_syllable (PangoFont *font, gunichar *text, int length,
		 PangoGlyphString *glyphs, int *n_glyphs, int cluster_offset)
{
  int n_prev_glyphs = *n_glyphs;
  int index;
  gunichar wc, tone;
  int i, j, composed;

  if (IS_M (text[length - 1]))
    {
      tone = text[length - 1];
      length--;
    }
  else
    tone = 0;

  if (length >= 3 && IS_L_S(text[0]) && IS_V_S(text[1]) && IS_T_S(text[2]))
    composed = 3;
  else if (length >= 2 && IS_L_S(text[0]) && IS_V_S(text[1]))
    composed = 2;
  else
    composed = 0;

  if (composed)
    {
      if (composed == 3)
	wc = S_FROM_LVT(text[0], text[1], text[2]);
      else
	wc = S_FROM_LV(text[0], text[1]);
      index = find_char (font, wc);
      pango_glyph_string_set_size (glyphs, *n_glyphs + 1);
      if (!index)
	set_glyph (font, glyphs, *n_glyphs, cluster_offset,
		   pango_xft_font_get_unknown_glyph (font, wc));
      else
	set_glyph (font, glyphs, *n_glyphs, cluster_offset, index);
      (*n_glyphs)++;
      text += composed;
      length -= composed;
    }

  /* Render the remaining text as uncomposed forms as a fallback.  */
  for (i = 0; i < length; i++)
    {
      int jindex;

      if (text[i] == LFILL || text[i] == VFILL)
	continue;

      index = find_char (font, text[i]);
      if (index)
	{
	  pango_glyph_string_set_size (glyphs, *n_glyphs + 1);
	  set_glyph (font, glyphs, *n_glyphs, cluster_offset, index);
	  (*n_glyphs)++;
	  continue;
	}

      /* This font has no glyphs on the Hangul Jamo area!  Find a
	 fallback from the Hangul Compatibility Jamo area.  */
      jindex = text[i] - LBASE;
      for (j = 0; j < 3 && (__jamo_to_ksc5601[jindex][j] != 0); j++)
	{
	  wc = __jamo_to_ksc5601[jindex][j] - KSC_JAMOBASE + UNI_JAMOBASE;
	  index = (wc >= 0x3131) ? find_char (font, wc) : 0;
	  pango_glyph_string_set_size (glyphs, *n_glyphs + 1);
	  if (!index)
	    set_glyph (font, glyphs, *n_glyphs, cluster_offset,
		       pango_xft_font_get_unknown_glyph (font, index));
	  else
	    set_glyph (font, glyphs, *n_glyphs, cluster_offset, index);
	  (*n_glyphs)++;
	}
    }
  if (n_prev_glyphs == *n_glyphs)
    {
      index = find_char (font, 0x3164);
      pango_glyph_string_set_size (glyphs, *n_glyphs + 1);
      if (!index)
	set_glyph (font, glyphs, *n_glyphs, cluster_offset,
		   pango_xft_font_get_unknown_glyph (font, index));
      else
	set_glyph (font, glyphs, *n_glyphs, cluster_offset, index);
      glyphs->log_clusters[*n_glyphs] = cluster_offset;
      (*n_glyphs)++;
    }
  if (tone)
    render_tone(font, tone, glyphs, n_glyphs, cluster_offset);
}

static void 
hangul_engine_shape (PangoFont        *font,
		     const char       *text,
		     gint              length,
		     PangoAnalysis    *analysis,
		     PangoGlyphString *glyphs)
{
  int n_chars, n_glyphs;
  int i;
  const char *p, *start;

  gunichar jamos_static[8];
  guint max_jamos = G_N_ELEMENTS (jamos_static);
  gunichar *jamos = jamos_static;
  int n_jamos;

  n_chars = g_utf8_strlen (text, length);
  n_glyphs = 0;
  start = p = text;
  n_jamos = 0;

  for (i = 0; i < n_chars; i++)
    {
      gunichar wc;

      wc = g_utf8_get_char (p);

      /* Check syllable boundaries. */
      if (n_jamos)
	{
	  gunichar prev = jamos[n_jamos - 1];
	  if ((!IS_L (prev) && IS_S (wc)) ||
	      (IS_T (prev) && IS_L (wc)) ||
	      (IS_V (prev) && IS_L (wc)) ||
	      (IS_T (prev) && IS_V (wc)) ||
	      IS_M(prev))
	    {
	      /* Draw a syllable. */
	      render_syllable (font, jamos, n_jamos, glyphs,
			       &n_glyphs, start - text);
	      n_jamos = 0;
	      start = p;
	    }
	}
	  
      if (n_jamos == max_jamos)
	{
	  max_jamos += 3;	/* at most 3 for each syllable code (L+V+T) */
	  if (jamos == jamos_static)
	    {
	      jamos = g_new (gunichar, max_jamos);
	      memcpy (jamos, jamos_static, n_jamos*sizeof(gunichar));
	    }
	  else
	    jamos = g_renew (gunichar, jamos, max_jamos);
	}

      if (IS_S (wc))
	{
	  jamos[n_jamos++] = L_FROM_S (wc);
	  jamos[n_jamos++] = V_FROM_S (wc);
	  if (S_HAS_T (wc))
	    jamos[n_jamos++] = T_FROM_S (wc);
	}
      else if (IS_M (wc) && !n_jamos)
	{
	  /* Tone mark not following syllable */
	  render_isolated_tone (font, wc, glyphs, &n_glyphs, start - text);
	  start = g_utf8_next_char (p);
	}
      else
	jamos[n_jamos++] = wc;
      p = g_utf8_next_char (p);
    }

  if (n_jamos != 0)
    render_syllable (font, jamos, n_jamos, glyphs, &n_glyphs,
		     start - text);

  if (jamos != jamos_static)
    g_free(jamos);
}

static PangoCoverage *
hangul_engine_get_coverage (PangoFont  *font,
			   PangoLanguage *lang)
{
  PangoCoverage *result = pango_coverage_new ();
  int i;

  /* Well, no unicode rendering engine could render Hangul Jamo area
     _exactly_, I sure.  */
  for (i = 0x1100; i <= 0x11ff; i++)
    pango_coverage_set (result, i, PANGO_COVERAGE_FALLBACK);
  pango_coverage_set (result, 0x302e, PANGO_COVERAGE_FALLBACK);
  pango_coverage_set (result, 0x302f, PANGO_COVERAGE_FALLBACK);
  for (i = 0xac00; i <= 0xd7a3; i++)
    pango_coverage_set (result, i, PANGO_COVERAGE_EXACT);
  return result;
}

static PangoEngine *
hangul_engine_xft_new ()
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
 * Pango. If we are compiling it is a module, then we name the
 * entry points script_engine_list, etc. But if we are compiling
 * it for inclusion directly in Pango, then we need them to
 * to have distinct names for this module, so we prepend
 * _pango_hangul_xft__
 */
#ifdef XFT_MODULE_PREFIX
#define MODULE_ENTRY(func) _pango_hangul_xft_##func
#else
#define MODULE_ENTRY(func) func
#endif

/* List the engines contained within this module
 */
void 
MODULE_ENTRY(script_engine_list) (PangoEngineInfo **engines, gint *n_engines)
{
  *engines = script_engines;
  *n_engines = G_N_ELEMENTS (script_engines);
}

/* Load a particular engine given the ID for the engine
 */
PangoEngine *
MODULE_ENTRY(script_engine_load) (const char *id)
{
  if (!strcmp (id, SCRIPT_ENGINE_NAME))
    return hangul_engine_xft_new ();
  else
    return NULL;
}

void 
MODULE_ENTRY(script_engine_unload) (PangoEngine *engine)
{
}
