/* Pango
 * hangul-fc.c: Hangul shaper for FreeType based backends
 *
 * Copyright (C) 2002-2006 Changwoo Ryu
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

#include "config.h"
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
		   PANGO_GET_UNKNOWN_GLYPH (tone));
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
#if 0 /* FIXME: what kind of hack is it?  it draws dummy glyphs.  */
  /* Find a base character to render the mark on
   */
  int index = find_char (font, 0x25cc);	/* DOTTED CIRCLE */
  if (!index)
    index = find_char (font, 0x25cb);   /* WHITE CIRCLE, in KSC-5601 */
  if (!index)
    index = find_char (font, ' ');      /* Space */
  if (!index)				/* Unknown glyph box with 0000 in it */
    index = find_char (font, PANGO_GET_UNKNOWN_GLYPH (0));

  /* Add the base character
   */
  pango_glyph_string_set_size (glyphs, *n_glyphs + 1);
  set_glyph (font, glyphs, *n_glyphs, cluster_offset, index);
  (*n_glyphs)++;
#endif

  /* And the tone mark
   */
  render_tone(font, tone, glyphs, n_glyphs, cluster_offset);
}

static void
render_syllable (PangoFont *font, const char *str, int length,
		 PangoGlyphString *glyphs, int *n_glyphs, int cluster_offset)
{
  int n_prev_glyphs = *n_glyphs;
  int index;
  gunichar wc = 0, tone = 0, text[4];
  int i, j, composed = 0;
  const char *p;

  /* Normalize it only when the entire sequence is equivalent to a
   * precomposed syllable. It's usually better than prefix
   * normalization both for poor-featured fonts and for smart fonts.
   * I have seen no smart font which can render S+T as a syllable
   * form.
   */

  if (length == 3 || length == 4)
    {
      p = str;
      text[0] = g_utf8_get_char(p);
      p = g_utf8_next_char(p);
      text[1] = g_utf8_get_char(p);
      p = g_utf8_next_char(p);
      text[2] = g_utf8_get_char(p);

      if (length == 4 && !IS_M(g_utf8_get_char(g_utf8_next_char(p))))
	goto lvt_out;		/* draw the tone mark later */

      if (IS_L_S(text[0]) && IS_V_S(text[1]) &&  IS_T_S(text[2]))
	{
	  composed = 3;
	  wc = S_FROM_LVT(text[0], text[1], text[2]);
	  str = g_utf8_next_char(p);
	  goto normalize_out;
	}
    }
 lvt_out:

  if (length == 2 || length == 3)
    {
      p = str;
      text[0] = g_utf8_get_char(p);
      p = g_utf8_next_char(p);
      text[1] = g_utf8_get_char(p);

      if (length == 3 && !IS_M(g_utf8_get_char(g_utf8_next_char(p))))
	goto lv_out;		/* draw the tone mark later */
      if (IS_L_S(text[0]) && IS_V_S(text[1]))
	{
	  composed = 2;
	  wc = S_FROM_LV(text[0], text[1]);
	  str = g_utf8_next_char(p);
	}
      else if (IS_S(text[0] && !S_HAS_T(text[0]) && IS_T_S(text[1])))
	{
	  composed = 2;
	  wc = text[0] + (text[1] - TBASE);
	  str = g_utf8_next_char(p);
	  goto normalize_out;
	}
    }
 lv_out:
 normalize_out:

  if (composed)
    {
      index = find_char (font, wc);
      pango_glyph_string_set_size (glyphs, *n_glyphs + 1);
      if (!index)
	set_glyph (font, glyphs, *n_glyphs, cluster_offset,
		   PANGO_GET_UNKNOWN_GLYPH (wc));
      else
	set_glyph (font, glyphs, *n_glyphs, cluster_offset, index);
      (*n_glyphs)++;
      length -= composed;
    }

  /* Render the remaining text as uncomposed forms as a fallback.  */
  for (i = 0; i < length; i++, str = g_utf8_next_char(str))
    {
      int jindex;
      int oldlen;

      wc = g_utf8_get_char(str);

      if (wc == LFILL || wc == VFILL)
	continue;

      if (IS_M(wc))
	{
	  tone = wc;
	  break;
	}

      if (IS_S(wc))
	{
	  oldlen = *n_glyphs;

	  text[0] = L_FROM_S(wc);
	  text[1] = V_FROM_S(wc);
	  if (S_HAS_T(wc))
	    {
	      text[2] = T_FROM_S(wc);
	      composed = 3;
	    }
	  else
	      composed = 2;

	  for (j = 0; j < composed; j++)
	    {
	      index = find_char (font, text[j]);
	      if (index)
		{
		  pango_glyph_string_set_size (glyphs, *n_glyphs + 1);
		  set_glyph (font, glyphs, *n_glyphs, cluster_offset, index);
		  (*n_glyphs)++;
		}
	      else
		goto decompose_cancel;
	    }

	  continue;

	decompose_cancel:
	  /* The font doesn't have jamos.  Cancel it. */
	  *n_glyphs = oldlen;
	  pango_glyph_string_set_size (glyphs, *n_glyphs);
	}

      index = find_char (font, wc);
      if (index)
	{
	  pango_glyph_string_set_size (glyphs, *n_glyphs + 1);
	  set_glyph (font, glyphs, *n_glyphs, cluster_offset, index);
	  (*n_glyphs)++;
	  continue;
	}
      else if (IS_S(wc))
	{
	  pango_glyph_string_set_size (glyphs, *n_glyphs + 1);
	  set_glyph (font, glyphs, *n_glyphs, cluster_offset,
		     PANGO_GET_UNKNOWN_GLYPH (wc));
	  (*n_glyphs)++;
	  continue;
	}

      /* This font has no glyphs on the Hangul Jamo area!  Find a
	 fallback from the Hangul Compatibility Jamo area.  */
      jindex = wc - LBASE;
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
			 PANGO_GET_UNKNOWN_GLYPH (text[i]));
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
      index = find_char (font, 0x3164);	/* U+3164 HANGUL FILLER */
      pango_glyph_string_set_size (glyphs, *n_glyphs + 1);
      if (!index)
	set_glyph (font, glyphs, *n_glyphs, cluster_offset,
		   PANGO_GET_UNKNOWN_GLYPH (index));
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
	set_glyph (font, glyphs, *n_glyphs, cluster_offset, PANGO_GET_UNKNOWN_GLYPH (wc));
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
  int n_chars = g_utf8_strlen (text, length);
  int n_glyphs;
  int i;
  const char *p, *start;

  int n_jamos;
  gunichar prev = 0;

  n_glyphs = 0;
  start = p = text;
  n_jamos = 0;

  for (i = 0; i < n_chars; i++)
    {
      gunichar wc;

      wc = g_utf8_get_char (p);

      /* Check syllable boundaries. */
      if (n_jamos && IS_BOUNDARY (prev, wc))
	{
	  if (n_jamos == 1 && IS_S (prev))
	    /* common case which the most people use */
	    render_basic (font, prev, glyphs, &n_glyphs, start - text);
	  else
	    /* possibly complex composition */
	    render_syllable (font, start, n_jamos, glyphs,
			     &n_glyphs, start - text);
	  n_jamos = 0;
	  start = p;
	}

      prev = wc;

      if (!IS_HANGUL (wc))
	{
	  render_basic (font, wc, glyphs, &n_glyphs, start - text);
	  start = g_utf8_next_char (p);
	}
      else if (IS_M (wc) && !n_jamos)
	{
	  /* Tone mark not following syllable */
	  render_isolated_tone (font, wc, glyphs, &n_glyphs, start - text);
	  start = g_utf8_next_char (p);
	}
      else
	n_jamos++;
      p = g_utf8_next_char (p);
    }

  if (n_jamos == 1 && IS_S (prev))
    render_basic (font, prev, glyphs, &n_glyphs, start - text);
  else if (n_jamos > 0)
    render_syllable (font, start, n_jamos, glyphs, &n_glyphs,
		     start - text);
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
