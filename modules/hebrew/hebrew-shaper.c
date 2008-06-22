/* Pango
 * hebrew-shaper.c:
 *
 * Copyright (c) 2001 by Sun Microsystems, Inc.
 * Author: Chookij Vanatham <Chookij.Vanatham@Eng.Sun.COM>
 *
 * Hebrew points positioning improvements 2001
 * Author: Dov Grobgeld <dov.grobgeld@gmail.com>
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
 *
 * Note March 9, 2003: I fixed a crash with regards to precomposed
 * characters, by wraping all of them to be considered as ALEF as
 * far as consideration about composability is concerned. The rendering
 * with regards to precomposed characters AND nikud comes out really
 * bad though, and should be fixed, once I have more time.
 */

#include "config.h"
#include <glib.h>
#include "pango-engine.h"
#include "hebrew-shaper.h"

/* Wrap all characters above 0xF00 to ALEF. */
#define ishebrew(wc)                    ((wc)>0x590 && (wc)<0x600)
#define ucs2iso8859_8(wc)		((unsigned int)((unsigned int)(wc) - 0x0590 + 0x10))
#define iso8859_8_2uni(c)		((gunichar)(c) - 0x10 + 0x0590)

#define MAX_CLUSTER_CHRS	256

/* Define Hebrew character classes */
#define _ND			0
#define _SP			1
#define _NS			(1<<1)
#define	_DA			(1<<2)	/* only for dagesh... */

#define	NoDefine		_ND
#define	SpacingLetter		_SP
#define	NonSpacingPunc		_NS

/* Define Hebrew character types */
#define	__ND			0
#define	__SP			1
#define	__NS			2
#define	__DA			3

/* Unicode definitions needed in logics below... */
#define	UNI_ALEF                0x05D0
#define	UNI_BET			0x05D1
#define UNI_GIMMEL              0x05d2
#define	UNI_DALED		0x05D3
#define	UNI_KAF			0x05DB
#define	UNI_FINAL_KAF           0x05DA
#define UNI_VAV			0x05D5
#define	UNI_YOD			0x05D9
#define	UNI_RESH		0x05E8
#define UNI_LAMED		0x05DC
#define UNI_SHIN		0x05E9
#define UNI_FINAL_PE		0x05E3
#define UNI_PE			0x05E4
#define UNI_QOF                 0x05E7
#define	UNI_TAV			0x05EA
#define UNI_SHIN_DOT		0x05C1
#define UNI_SIN_DOT		0x05C2
#define UNI_MAPIQ		0x05BC
#define	UNI_SHEVA		0x05B0
#define	UNI_HOLAM		0x05B9
#define	UNI_QUBUTS		0x05BB
#define UNI_HATAF_SEGOL         0x05B1
#define UNI_HATAF_QAMATZ        0x05B3
#define UNI_TSERE               0x05B5
#define UNI_QAMATS              0x05B8
#define UNI_QUBUTS              0x05BB

/*======================================================================
//  In the tables below all Hebrew characters are categorized to
//  one of the following four classes:
//
//      non used entries              Not defined  (ND)
//      accents, points               Non spacing  (NS)
//      punctuation and characters    Spacing characters (SP)
//      dagesh                        "Dagesh"    (DA)
//----------------------------------------------------------------------*/
static const gint char_class_table[128] = {
  /*       0,   1,   2,   3,   4,   5,   6,   7 */

  /*00*/ _ND, _ND, _ND, _ND, _ND, _ND, _ND, _ND,
	 _ND, _ND, _ND, _ND, _ND, _ND, _ND, _ND,

  /*10*/ _ND, _NS, _NS, _NS, _NS, _NS, _NS, _NS,
	 _NS, _NS, _NS, _NS, _NS, _NS, _NS, _NS,
  /*20*/ _NS, _NS, _ND, _NS, _NS, _NS, _NS, _NS,
	 _NS, _NS, _NS, _NS, _NS, _NS, _NS, _NS,
  /*30*/ _NS, _NS, _NS, _NS, _NS, _NS, _NS, _NS,
	 _NS, _NS, _ND, _NS, _DA, _NS, _SP, _NS,
  /*40*/ _SP, _NS, _NS, _SP, _NS, _ND, _ND, _ND,
	 _ND, _ND, _ND, _ND, _ND, _ND, _ND, _ND,
  /*50*/ _SP, _SP, _SP, _SP, _SP, _SP, _SP, _SP,
	 _SP, _SP, _SP, _SP, _SP, _SP, _SP, _SP,
  /*60*/ _SP, _SP, _SP, _SP, _SP, _SP, _SP, _SP,
	 _SP, _SP, _SP, _ND, _ND, _ND, _ND, _ND,
  /*70*/ _SP, _SP, _SP, _SP, _SP, _ND, _ND, _ND,
	 _ND, _ND, _ND, _ND, _ND, _ND, _ND, _ND,
};

static const gint char_type_table[128] = {
  /*       0,   1,   2,   3,   4,   5,   6,   7 */

  /*00*/ __ND, __ND, __ND, __ND, __ND, __ND, __ND, __ND,
	 __ND, __ND, __ND, __ND, __ND, __ND, __ND, __ND,

  /*10*/ __ND, __NS, __NS, __NS, __NS, __NS, __NS, __NS,
	 __NS, __NS, __NS, __NS, __NS, __NS, __NS, __NS,
  /*20*/ __NS, __NS, __ND, __NS, __NS, __NS, __NS, __NS,
	 __NS, __NS, __NS, __NS, __NS, __NS, __NS, __NS,
  /*30*/ __NS, __NS, __NS, __NS, __NS, __NS, __NS, __NS,
	 __NS, __NS, __ND, __NS, __DA, __NS, __SP, __NS,
  /*40*/ __SP, __NS, __NS, __SP, __NS, __ND, __ND, __ND,
	 __ND, __ND, __ND, __ND, __ND, __ND, __ND, __ND,
  /*50*/ __SP, __SP, __SP, __SP, __SP, __SP, __SP, __SP,
	 __SP, __SP, __SP, __SP, __SP, __SP, __SP, __SP,
  /*60*/ __SP, __SP, __SP, __SP, __SP, __SP, __SP, __SP,
	 __SP, __SP, __SP, __ND, __ND, __ND, __ND, __ND,
  /*70*/ __SP, __SP, __SP, __SP, __SP, __ND, __ND, __ND,
	 __ND, __ND, __ND, __ND, __ND, __ND, __ND, __ND,
};

/*======================================================================
//  The following table answers the question whether two characters
//  are composible or not. The decision is made by looking at the
//  char_type_table values for the first character in a cluster
//  vs a following charactrer. The only three combinations that
//  are composible in Hebrew according to the table are:
//
//     1. a spacing character followed by non-spacing character
//     2. a spacing character followed by a dagesh.
//     3. a dagesh followed by a non-spacing character.
//
//  Note that a spacing character may be followed by several non-spacing
//  accents, as the decision is always made on the base character of
//  a combination.
//----------------------------------------------------------------------*/
static const gboolean compose_table[4][4] = {
      /* Cn */ /*     0,     1,     2,     3, */
/* Cn-1 00 */	{ FALSE, FALSE, FALSE, FALSE },
  /* 10 */      { FALSE, FALSE,  TRUE,  TRUE },
  /* 20 */      { FALSE, FALSE, FALSE, FALSE },
  /* 30 */	{ FALSE, FALSE,  TRUE, FALSE },
};

/* Treat all characters above 0xF000 as characters */
#define is_hebrew(wc) ((wc) >= 0x590 && (wc) < 0x600)
#define is_char_class(wc, mask)	(char_class_table[ucs2iso8859_8 ((wc))] & (mask))
#define	is_composible(cur_wc, nxt_wc)	(compose_table[char_type_table[ucs2iso8859_8 (cur_wc)]]\
						      [char_type_table[ucs2iso8859_8 (nxt_wc)]])

G_CONST_RETURN char *
hebrew_shaper_get_next_cluster(const char      *text,
			       gint		length,
			       gunichar        *cluster,
			       gint	       *num_chrs)
{
  const char *p;
  gint n_chars = 0;

  p = text;

  while (p < text + length && n_chars < MAX_CLUSTER_CHRS)
    {
      gunichar current = g_utf8_get_char (p);

      if (!ishebrew (current) ||
	  (n_chars == 0 && is_char_class(current, ~(NoDefine|SpacingLetter))))
	{
	  /* Not a legal Hebrew cluster */

	  if (n_chars == 0)
	    {
	      cluster[n_chars++] = current;
	      p = g_utf8_next_char (p);
	    }
	  break;
	}
      else if (n_chars == 0 ||
	       is_composible (cluster[0], current))
	{
	  cluster[n_chars++] = current;
	  p = g_utf8_next_char (p);
	}
      else
	break;
    }

  *num_chrs = n_chars;
  return p;
}

void
hebrew_shaper_get_cluster_kerning(gunichar            *cluster,
				  gint                cluster_length,
				  PangoRectangle      ink_rect[],

				  /* input and output */
				  gint                width[],
				  gint                x_offset[],
				  gint                y_offset[])
{
  int i;
  int base_ink_x_offset, base_ink_y_offset, base_ink_width, base_ink_height;
  gunichar base_char = cluster[0];

  x_offset[0] = 0;
  y_offset[0] = 0;

  if (cluster_length == 1)
    {
      /* Make lone 'vav dot' have zero width */
      if (base_char == UNI_SHIN_DOT
	  || base_char == UNI_SIN_DOT
	  || base_char == UNI_HOLAM
	  ) {
	x_offset[0] = -ink_rect[0].x - ink_rect[0].width;
	width[0] = 0;
      }

      return;
    }

  base_ink_x_offset = ink_rect[0].x;
  base_ink_y_offset = ink_rect[0].y;
  base_ink_width = ink_rect[0].width;
  base_ink_height = ink_rect[0].height;

  /* Do heuristics */
  for (i=1; i<cluster_length; i++)
    {
      int gl = cluster[i];
      x_offset[i] = 0;
      y_offset[i] = 0;

      /* Check if it is a point */
      if (gl < 0x5B0 || gl >= 0x05D0)
	continue;

      /* Center dot of VAV */
      if (gl == UNI_MAPIQ && base_char == UNI_VAV)
	{
	  x_offset[i] = base_ink_x_offset - ink_rect[i].x;

	  /* If VAV is a vertical bar without a roof, then we
	     need to make room for the dot by increasing the
	     cluster width. But how can I check if that is the
	     case??
	  */
	  /* This is wild, but it does the job of differentiating
	     between two M$ fonts... Base the decision on the
	     aspect ratio of the vav...
	  */
	  if (base_ink_height > base_ink_width * 3.5)
	    {
	      int j;
	      double space = 0.7;
	      double kern = 0.5;

	      /* Shift all characters to make place for the mapiq */
	      for (j=0; j<i; j++)
		  x_offset[j] += ink_rect[i].width*(1+space-kern);

	      width[cluster_length-1] += ink_rect[i].width*(1+space-kern);
	      x_offset[i] -= ink_rect[i].width*(kern);
	    }
	}

      /* Dot over SHIN */
      else if (gl == UNI_SHIN_DOT && base_char == UNI_SHIN)
	{
	  x_offset[i] = base_ink_x_offset + base_ink_width
	    - ink_rect[i].x - ink_rect[i].width;
	}

      /* Dot over SIN */
      else if (gl == UNI_SIN_DOT && base_char == UNI_SHIN)
	{
	  x_offset[i] = base_ink_x_offset - ink_rect[i].x;
	}

      /* VOWEL DOT above to any other character than
	 SHIN or VAV should stick out a bit to the left. */
      else if ((gl == UNI_SIN_DOT || gl == UNI_HOLAM)
	       && base_char != UNI_SHIN && base_char != UNI_VAV)
	{
	  x_offset[i] = base_ink_x_offset -ink_rect[i].x - ink_rect[i].width * 3/ 2;
	}

      /* VOWELS under resh or vav are right aligned, if they are
	 narrower than the characters. Otherwise they are centered.
       */
      else if ((base_char == UNI_VAV
		|| base_char == UNI_RESH
		|| base_char == UNI_YOD
		|| base_char == UNI_DALED
		)
	       && ((gl >= UNI_SHEVA && gl <= UNI_QAMATS) ||
		   gl == UNI_QUBUTS)
	       && ink_rect[i].width < base_ink_width
	       )
	{
	  x_offset[i] = base_ink_x_offset + base_ink_width
	    - ink_rect[i].x - ink_rect[i].width;
	}

      /* VOWELS under FINAL KAF are offset centered and offset in
	 y */
      else if ((base_char == UNI_FINAL_KAF
		)
	       && ((gl >= UNI_SHEVA && gl <= UNI_QAMATS) ||
		   gl == UNI_QUBUTS))
	{
	  /* x are at 1/3 to take into accoun the stem */
	  x_offset[i] = base_ink_x_offset - ink_rect[i].x
	    + base_ink_width * 1/3 - ink_rect[i].width/2;

	  /* Center in y */
	  y_offset[i] = base_ink_y_offset - ink_rect[i].y
	    + base_ink_height * 1/2 - ink_rect[i].height/2;
	}


      /* MAPIQ in PE or FINAL PE */
      else if (gl == UNI_MAPIQ
	       && (base_char == UNI_PE || base_char == UNI_FINAL_PE))
	{
	  x_offset[i]= base_ink_x_offset - ink_rect[i].x
	    + base_ink_width * 2/3 - ink_rect[i].width/2;

	  /* Another option is to offset the MAPIQ in y...
	     glyphs->glyphs[cluster_start_idx+i].geometry.y_offset
	     -= base_ink_height/5; */
	}

      /* MAPIQ in SHIN should be moved a bit to the right */
      else if (gl == UNI_MAPIQ
	       && base_char == UNI_SHIN)
	{
	  x_offset[i]=  base_ink_x_offset - ink_rect[i].x
	    + base_ink_width * 3/5 - ink_rect[i].width/2;
	}

      /* MAPIQ in YUD is right aligned */
      else if (gl == UNI_MAPIQ
	       && base_char == UNI_YOD)
	{
	  x_offset[i]=  base_ink_x_offset - ink_rect[i].x;

	  /* Lower left in y */
	  y_offset[i] = base_ink_y_offset - ink_rect[i].y
	    + base_ink_height - ink_rect[i].height*1.75;

	  if (base_ink_height > base_ink_width * 2)
	    {
	      int j;
	      double space = 0.7;
	      double kern = 0.5;

	      /* Shift all cluster characters to make space for mapiq */
	      for (j=0; j<i; j++)
		x_offset[j] += ink_rect[i].width*(1+space-kern);

	      width[cluster_length-1] += ink_rect[i].width*(1+space-kern);
	    }

	}

      /* VOWEL DOT next to any other character */
      else if ((gl == UNI_SIN_DOT || gl == UNI_HOLAM)
	       && (base_char != UNI_VAV))
	{
	  x_offset[i] = base_ink_x_offset -ink_rect[i].x;
	}

      /* Move nikud of taf a bit ... */
      else if (base_char == UNI_TAV && gl == UNI_MAPIQ)
	{
	  x_offset[i] = base_ink_x_offset - ink_rect[i].x
	    + base_ink_width * 5/8 - ink_rect[i].width/2;
	}

      /* Move center dot of characters with a right stem and no
	 left stem. */
      else if (gl == UNI_MAPIQ &&
	       (base_char == UNI_BET
		|| base_char == UNI_DALED
		|| base_char == UNI_KAF
		|| base_char == UNI_GIMMEL
		))
	{
	  x_offset[i] = base_ink_x_offset - ink_rect[i].x
	    + base_ink_width * 3/8 - ink_rect[i].width/2;
	}

      /* Right align wide nikud under QOF */
      else if (base_char == UNI_QOF &&
	       ( (gl >= UNI_HATAF_SEGOL
		  && gl <= UNI_HATAF_QAMATZ)
		 || (gl >= UNI_TSERE
		     && gl<= UNI_QAMATS)
		 || (gl == UNI_QUBUTS)))
	{
	  x_offset[i] = base_ink_x_offset + base_ink_width
	    - ink_rect[i].x - ink_rect[i].width;
	}

      /* Center by default */
      else
	{
	  x_offset[i] = base_ink_x_offset - ink_rect[i].x
	    + base_ink_width/2 - ink_rect[i].width/2;
	}
    }

}

void
hebrew_shaper_swap_range (PangoGlyphString *glyphs,
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

void
hebrew_shaper_bidi_reorder(PangoGlyphString *glyphs)
{
  int start, end;

  /* Swap all glyphs */
  hebrew_shaper_swap_range (glyphs, 0, glyphs->num_glyphs);

  /* Now reorder glyphs within each cluster back to LTR */
  for (start = 0; start < glyphs->num_glyphs;)
    {
      end = start;
      while (end < glyphs->num_glyphs &&
	     glyphs->log_clusters[end] == glyphs->log_clusters[start])
	end++;

      hebrew_shaper_swap_range (glyphs, start, end);
      start = end;
    }
}
