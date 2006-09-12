/* Pango
 * hangul-fc.c: Hangul shaper for FreeType based backends
 *
 * Copyright (C) 2002-2004 Changwoo Ryu
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

#include <config.h>
#include <string.h>

#include "pango-engine.h"
#include "pango-utils.h"
#include "pangofc-font.h"

#include "hangul-defs.h"
#include "tables-jamos.i"

/* No extra fields needed */
typedef PangoEngineShape      HangulEngineFc;
typedef PangoEngineShapeClass HangulEngineFcClass ;

#define SCRIPT_ENGINE_NAME "HangulScriptEngineFc"
#define RENDER_TYPE PANGO_RENDER_TYPE_FC

static PangoEngineScriptInfo hangul_scripts[] = {
  { PANGO_SCRIPT_HANGUL, "*" }
};

static PangoEngineInfo script_engines[] = {
  {
    SCRIPT_ENGINE_NAME,
    PANGO_ENGINE_TYPE_SHAPE,
    RENDER_TYPE,
    hangul_scripts, G_N_ELEMENTS(hangul_scripts)
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


#define find_char(font,wc) \
    pango_fc_font_get_glyph((PangoFcFont *)font, wc)
#define get_unknown_glyph(font,wc) \
    PANGO_GET_UNKNOWN_GLYPH ( wc)

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
		   get_unknown_glyph (font, tone));
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
  if (!index)				/* Unknown glyph box with 0000 in it */
    index = find_char (font, get_unknown_glyph (font, 0));

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
		   get_unknown_glyph (font, wc));
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
      int oldlen;

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
      oldlen = *n_glyphs;
      for (j = 0; j < 3 && (__jamo_to_ksc5601[jindex][j] != 0); j++)
	{
	  wc = __jamo_to_ksc5601[jindex][j] - KSC_JAMOBASE + UNI_JAMOBASE;
	  index = (wc >= 0x3131) ? find_char (font, wc) : 0;
	  pango_glyph_string_set_size (glyphs, *n_glyphs + 1);
	  if (!index)
	    {
	      *n_glyphs = oldlen;
	      pango_glyph_string_set_size (glyphs, *n_glyphs + 1);
	      set_glyph (font, glyphs, *n_glyphs, cluster_offset,
			 get_unknown_glyph (font, text[i]));
	      (*n_glyphs)++;
	      break;
	    }
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
		   get_unknown_glyph (font, index));
      else
	set_glyph (font, glyphs, *n_glyphs, cluster_offset, index);
      glyphs->log_clusters[*n_glyphs] = cluster_offset;
      (*n_glyphs)++;
    }
  if (tone)
    render_tone(font, tone, glyphs, n_glyphs, cluster_offset);
}

static void
render_basic (PangoFont *font, gunichar wc,
	      PangoGlyphString *glyphs, int *n_glyphs, int cluster_offset)
{
  int index;

  if (wc == 0xa0)	/* non-break-space */
    wc = 0x20;

  pango_glyph_string_set_size (glyphs, *n_glyphs + 1);

  if (pango_is_zero_width (wc))
    set_glyph (font, glyphs, *n_glyphs, cluster_offset, PANGO_GLYPH_EMPTY);
  else
    {
      index = find_char (font, wc);
      if (index)
	set_glyph (font, glyphs, *n_glyphs, cluster_offset, index);
      else
	set_glyph (font, glyphs, *n_glyphs, cluster_offset, get_unknown_glyph (font, wc));
    }
  (*n_glyphs)++;
}

static void 
hangul_engine_shape (PangoEngineShape *engine,
		     PangoFont        *font,
		     const char       *text,
		     gint              length,
		     const PangoAnalysis *analysis,
		     PangoGlyphString *glyphs)
{
  int n_chars, n_glyphs;
  int i;
  const char *p, *start;

  gunichar jamos_static[8];
  gint max_jamos = G_N_ELEMENTS (jamos_static);
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
	  if ((!IS_JAMO (wc) && !IS_S (wc) && !IS_M (wc)) ||
	      (!IS_L (prev) && IS_S (wc)) ||
	      (IS_T (prev) && IS_L (wc)) ||
	      (IS_V (prev) && IS_L (wc)) ||
	      (IS_T (prev) && IS_V (wc)) ||
	      IS_M (prev))
	    {
	      /* Draw a syllable with these jamos. */
	      render_syllable (font, jamos, n_jamos, glyphs,
			       &n_glyphs, start - text);
	      n_jamos = 0;
	      start = p;
	    }
	}
	  
      if (n_jamos >= max_jamos - 3)
	{
	  max_jamos += 8;	/* at most 3 for each syllable code (L+V+T) */
	  if (jamos == jamos_static)
	    {
	      jamos = g_new (gunichar, max_jamos);
	      memcpy (jamos, jamos_static, n_jamos*sizeof(gunichar));
	    }
	  else
	    jamos = g_renew (gunichar, jamos, max_jamos);
	}

      if (!IS_JAMO (wc) && !IS_S (wc) && !IS_M (wc))
	{
	  render_basic (font, wc, glyphs, &n_glyphs, start - text);
	  start = g_utf8_next_char (p);
	}
      else if (IS_S (wc))
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

static void
hangul_engine_fc_class_init (PangoEngineShapeClass *class)
{
  class->script_shape = hangul_engine_shape;
}

PANGO_ENGINE_SHAPE_DEFINE_TYPE (HangulEngineFc, hangul_engine_fc,
				hangul_engine_fc_class_init, NULL)

void 
PANGO_MODULE_ENTRY(init) (GTypeModule *module)
{
  hangul_engine_fc_register_type (module);
}

void 
PANGO_MODULE_ENTRY(exit) (void)
{
}

void 
PANGO_MODULE_ENTRY(list) (PangoEngineInfo **engines,
			  int              *n_engines)
{
  *engines = script_engines;
  *n_engines = G_N_ELEMENTS (script_engines);
}

PangoEngine *
PANGO_MODULE_ENTRY(create) (const char *id)
{
  if (!strcmp (id, SCRIPT_ENGINE_NAME))
    return g_object_new (hangul_engine_fc_type, NULL);
  else
    return NULL;
}
