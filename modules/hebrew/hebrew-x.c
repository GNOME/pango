/* Pango
 * hebrew-x.c:
 *
 * Copyright (C) 1999 Red Hat Software
 * Author: Owen Taylor <otaylor@redhat.com>
 *
 * Copyright (c) 1996-2000 by Sun Microsystems, Inc.
 * Author: Chookij Vanatham <Chookij.Vanatham@Eng.Sun.COM>
 *
 * Hebrew points positioning improvements 2001
 * Author: Dov Grobgeld <dov@imagic.weizmann.ac.il>
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

#include <glib.h>
#include <string.h>
#include "pangox.h"
#include "pango-engine.h"

#define ucs2iso8859_8(wc)		(unsigned int)((unsigned int)(wc) - 0x0590 + 0x10)
#define iso8859_8_2uni(c)		((gunichar)(c) - 0x10 + 0x0590)

#define MAX_CLUSTER_CHRS	256
#define MAX_GLYPHS		256

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

/* Unicode definitions ... */
#define UNI_VAV			0x5d5
#define UNI_LAMED		0x5DC
#define UNI_SHIN		0x5E9
#define UNI_FINAL_PE		0x05E3
#define UNI_PE			0x05E4
#define UNI_SHIN_DOT		0x5c1
#define UNI_SIN_DOT		0x5c2
#define UNI_MAPIQ		0x5bc

#define is_char_class(wc, mask)	(char_class_table[ucs2iso8859_8 ((wc))] & (mask))
#define	is_composible(cur_wc, nxt_wc)	(compose_table[char_type_table[ucs2iso8859_8 (cur_wc)]]\
						      [char_type_table[ucs2iso8859_8 (nxt_wc)]])

#define SCRIPT_ENGINE_NAME "HebrewScriptEngineX"

/* We handle the range U+0591 to U+05f4 exactly
 */
static PangoEngineRange hebrew_ranges[] = {
  { 0x0591, 0x05f4, "*" },  /* Hebrew */
};

static PangoEngineInfo script_engines[] = {
  {
    SCRIPT_ENGINE_NAME,
    PANGO_ENGINE_TYPE_SHAPE,
    PANGO_RENDER_TYPE_X,
    hebrew_ranges, G_N_ELEMENTS(hebrew_ranges)
  }
};

/*
 * X window system script engine portion
 */

typedef struct _HebrewFontInfo HebrewFontInfo;

/* The type of encoding that we will use
 */
typedef enum {
  HEBREW_FONT_NONE,
  HEBREW_FONT_ISO8859_8,
  HEBREW_FONT_ISO10646,
} HebrewFontType;

struct _HebrewFontInfo
{
  PangoFont   *font;
  HebrewFontType type;
  PangoXSubfont subfont;
};

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

/* ISO 8859_8 Hebrew Font Layout. Does not include any accents.
 */
static const gint iso_8859_8_shape_table[128] = {
  0x00, 0x00, 0x00, 0x00,  0x00, 0x00, 0x00, 0x00,
  0x00, 0x00, 0x00, 0x00,  0x00, 0x00, 0x00, 0x00,

  0x00, 0x00, 0x00, 0x00,  0x00, 0x00, 0x00, 0x00,
  0x00, 0x00, 0x00, 0x00,  0x00, 0x00, 0x00, 0x00,
  0x00, 0x00, 0x00, 0x00,  0x00, 0x00, 0x00, 0x00,
  0x00, 0x00, 0x00, 0x00,  0x00, 0x00, 0x00, 0x00,
  0x00, 0x00, 0x00, 0x00,  0x00, 0x00, 0x00, 0x00,
  0x00, 0x00, 0x00, 0x00,  0x00, 0x00, 0x00, 0x00,
  0x00, 0x00, 0x00, 0x00,  0x00, 0x00, 0x00, 0x00,
  0x00, 0x00, 0x00, 0x00,  0x00, 0x00, 0x00, 0x00,

  0xE0, 0xE1, 0xE2, 0xE3,  0xE4, 0xE5, 0xE6, 0xE7,
  0xE8, 0xE9, 0xEA, 0xEB,  0xEC, 0xED, 0xEE, 0xEF,
  0xF0, 0xF1, 0xF2, 0xF3,  0xF4, 0xF5, 0xF6, 0xF7,
  0xF8, 0xF9, 0xFA, 0x00,  0x00, 0x00, 0x00, 0x00,
  0x00, 0x00, 0x00, 0x00,  0x00, 0x00, 0x00, 0x00,
  0x00, 0x00, 0x00, 0x00,  0x00, 0x00, 0x00, 0x00,
};

/* Unicode Hebrew Font Layout
 */
static const gint Unicode_shape_table[128] = {
  /* 00 */    0x00, 0x00, 0x00, 0x00,  0x00, 0x00, 0x00, 0x00,
              0x00, 0x00, 0x00, 0x00,  0x00, 0x00, 0x00, 0x00,
  /* cantillation marks followed by accents */
  /* 10 */    0x0000, 0x0591, 0x0592, 0x0593, 0x0594, 0x0595, 0x0596, 0x0597,
              0x0598, 0x0599, 0x059A, 0x059B, 0x059C, 0x059D, 0x059E, 0x059F,
  /* 20 */    0x05A0, 0x05A1, 0x0000, 0x05A3, 0x05A4, 0x05A5, 0x05A6, 0x05A7,
              0x05A8, 0x05A9, 0x05AA, 0x05AB, 0x05AC, 0x05AD, 0x05AE, 0x05AF,
  /* 30 */    0x05B0, 0x05B1, 0x05B2, 0x05B3, 0x05B4, 0x05B5, 0x05B6, 0x05B7,
              0x05B8, 0x05B9, 0x0000, 0x05BB, 0x05BC, 0x05BD, 0x05BE, 0x05BF,
  /* 40 */    0x05C0, 0x05C1, 0x05C2, 0x05C3, 0x05C4, 0x0000, 0x0000, 0x0000,
              0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000,

  /* Aleph-Tav, Yiddish ligatures, and punctuation */
  /* 50 */    0x05D0, 0x05D1, 0x05D2, 0x05D3, 0x05D4, 0x05D5, 0x05D6, 0x05D7,
              0x05D8, 0x05D9, 0x05DA, 0x05DB, 0x05DC, 0x05DD, 0x05DE, 0x05DF,
  /* 60 */    0x05E0, 0x05E1, 0x05E2, 0x05E3, 0x05E4, 0x05E5, 0x05E6, 0x05E7,
              0x05E8, 0x05E9, 0x05EA, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000,
  /* 70 */    0x05F0, 0x05F1, 0x05F2, 0x05F3, 0x05F4, 0x0000, 0x0000, 0x0000,
              0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000,
};

/* Returns a structure with information we will use to rendering given the
 * #PangoFont. This is computed once per font and cached for later retrieval.
 */
static HebrewFontInfo *
get_font_info (PangoFont *font)
{
  static const char *charsets[] = {
    "iso8859-8",
    "iso10646-1",
  };

  static const int charset_types[] = {
    HEBREW_FONT_ISO8859_8,
    HEBREW_FONT_ISO10646,
  };
  
  HebrewFontInfo *font_info;
  GQuark info_id = g_quark_from_string ("hebrew-font-info");
  
  font_info = g_object_get_qdata (G_OBJECT (font), info_id);

  if (!font_info)
    {
      /* No cached information not found, so we need to compute it
       * from scratch
       */
      PangoXSubfont *subfont_ids;
      gint *subfont_charsets;
      gint n_subfonts, i;

      font_info = g_new (HebrewFontInfo, 1);
      font_info->font = font;
      font_info->type = HEBREW_FONT_NONE;
      
      g_object_set_qdata_full (G_OBJECT (font), info_id, font_info, (GDestroyNotify)g_free);
      
      n_subfonts = pango_x_list_subfonts (font, (char **)charsets, G_N_ELEMENTS (charsets),
					  &subfont_ids, &subfont_charsets);

      for (i=0; i < n_subfonts; i++)
	{
	  HebrewFontType font_type = charset_types[subfont_charsets[i]];
	  
	  if (font_type == HEBREW_FONT_ISO10646 &&
	      pango_x_has_glyph (font, PANGO_X_MAKE_GLYPH (subfont_ids[i], 0x05D0)))
	    {
	      font_info->type = font_type;
	      font_info->subfont = subfont_ids[i];
	      
	      break;
	    }
	  else if (font_type == HEBREW_FONT_ISO8859_8 &&
		   pango_x_has_glyph (font, PANGO_X_MAKE_GLYPH (subfont_ids[i], 0xE0)))
	    {
	      font_info->type = font_type;
	      font_info->subfont = subfont_ids[i];
	      
	      break;
	    }
	}

      g_free (subfont_ids);
      g_free (subfont_charsets);
    }

  return font_info;
}

static void
add_glyph (HebrewFontInfo   *font_info, 
	   PangoGlyphString *glyphs, 
	   gint              cluster_start, 
	   PangoGlyph        glyph,
	   gboolean          is_combining)
{
  PangoRectangle ink_rect, logical_rect;
  gint index = glyphs->num_glyphs;

  pango_glyph_string_set_size (glyphs, index + 1);
  
  glyphs->glyphs[index].glyph = glyph;
  glyphs->glyphs[index].attr.is_cluster_start = is_combining ? 0 : 1;
  
  glyphs->log_clusters[index] = cluster_start;

  pango_font_get_glyph_extents (font_info->font,
				glyphs->glyphs[index].glyph, &ink_rect, &logical_rect);

  if (is_combining)
    {
      if (font_info->type == HEBREW_FONT_ISO8859_8)
	{
	  /* There are no accents in 8859_8 so this should never be
	     called... Therefore I have't even checked his. */
          glyphs->glyphs[index].geometry.width =
		logical_rect.width + glyphs->glyphs[index - 1].geometry.width;
          if (logical_rect.width > 0)
	      glyphs->glyphs[index].geometry.x_offset = glyphs->glyphs[index - 1].geometry.width;
          else
	      glyphs->glyphs[index].geometry.x_offset = glyphs->glyphs[index].geometry.width;
	  glyphs->glyphs[index - 1].geometry.width = 0;
	}
      else
        {
	  /* Unicode. Always make width of cluster according to the width
	     of the base character and never take the punctuation into
	     consideration.
	   */
	  glyphs->glyphs[index].geometry.width =
		MAX (logical_rect.width, glyphs->glyphs[index -1].geometry.width);
	  /* Dov's new logic... */
	  glyphs->glyphs[index].geometry.width =  glyphs->glyphs[index -1].geometry.width;

	  glyphs->glyphs[index - 1].geometry.width = 0;

	  /* Here we should put in heuristics to center nikud. */
	  glyphs->glyphs[index].geometry.x_offset = 0;
        }
    }
  else
    {
      glyphs->glyphs[index].geometry.x_offset = 0;
      glyphs->glyphs[index].geometry.width = logical_rect.width;
    }
  
  glyphs->glyphs[index].geometry.y_offset = 0;
}

static gint
get_adjusted_glyphs_list (HebrewFontInfo *font_info,
			  gunichar *cluster,
			  gint num_chrs,
			  PangoGlyph *glyph_lists,
			  const gint *shaping_table)
{
  gint i = 0;

  if ((num_chrs == 1) &&
      is_char_class (cluster[0], NonSpacingPunc))
    {
      if (font_info->type == HEBREW_FONT_ISO8859_8)
	{
	  glyph_lists[0] = PANGO_X_MAKE_GLYPH (font_info->subfont, 0x20);
	  glyph_lists[1] = PANGO_X_MAKE_GLYPH (font_info->subfont,
			shaping_table[ucs2iso8859_8 (cluster[0])]);
	  return 2;
	}
      else
	{
	  glyph_lists[0] = PANGO_X_MAKE_GLYPH (font_info->subfont,
			shaping_table[ucs2iso8859_8 (cluster[0])]);
	  return 1;
	}
    }
  else
    {
      while (i < num_chrs) {
	glyph_lists[i] = PANGO_X_MAKE_GLYPH (font_info->subfont,
				shaping_table[ucs2iso8859_8 (cluster[i])]);
	i++;
      }
      return num_chrs;
    }
}

static gint
get_glyphs_list (HebrewFontInfo	*font_info,
		 gunichar	*cluster,
		 gint		num_chrs,
		 PangoGlyph	*glyph_lists)
{
  gint i;

  switch (font_info->type)
    {
      case HEBREW_FONT_NONE:
        for (i=0; i < num_chrs; i++)
	  glyph_lists[i] = pango_x_get_unknown_glyph (font_info->font);
        return num_chrs;

      case HEBREW_FONT_ISO8859_8:
        return get_adjusted_glyphs_list (font_info, cluster,
			num_chrs, glyph_lists, iso_8859_8_shape_table);
      
      case HEBREW_FONT_ISO10646:
        return get_adjusted_glyphs_list (font_info, cluster,
			num_chrs, glyph_lists, Unicode_shape_table);
    }
  return 0;
}

static void
add_cluster (HebrewFontInfo	*font_info,
	     PangoGlyphString	*glyphs,
	     gint		cluster_start,
	     gunichar		*cluster,
	     gint		num_chrs)
	     
{
  PangoGlyph glyphs_list[MAX_GLYPHS];
  gint num_glyphs;
  gint i;
  
  num_glyphs = get_glyphs_list(font_info, cluster, num_chrs, glyphs_list);
  for (i=0; i<num_glyphs; i++)
       add_glyph (font_info, glyphs, cluster_start, glyphs_list[i],
	    		i == 0 ? FALSE : TRUE);

  /* Here the fun starts. Post process the positions of glyphs in the
     cluster in order to make nikud look nice... The following is based
     on lots of heuristic rules and could probably be improved. Especially
     we could improve things considerably if we would access the rendered
     bitmap and move nikud to avoid collisions etc.

     Todo:
     
     * Take care of several points and accents below the characters.
     
     * Figure out what to do with dot inside vav if it the vav does
       not have a "roof". (Happens e.g. in Ariel).
  */
  if (num_glyphs > 1)
    {
      int i;
      int cluster_start_idx = glyphs->num_glyphs - num_glyphs;
      
      if (font_info->type == HEBREW_FONT_ISO10646)
	{
	  PangoRectangle ink_rect, logical_rect;
	  int base_char = glyphs_list[0] & 0x0fff;
	  int base_ink_x_offset;
	  int base_ink_width, base_ink_height;
	  
	  pango_font_get_glyph_extents (font_info->font,
					glyphs->glyphs[cluster_start_idx].glyph, &ink_rect, &logical_rect);
	  base_ink_x_offset = ink_rect.x;
	  base_ink_width = ink_rect.width;
	  base_ink_height = ink_rect.height;
	  
	  for (i=1; i<num_glyphs; i++)
	    {
	      int gl = glyphs_list[i] & 0x0fff;

	      /* Check if it is a point */
	      if (gl < 0x5B0 || gl >= 0x05D0)
		continue;
	      
	      pango_font_get_glyph_extents (font_info->font,
					    glyphs->glyphs[cluster_start_idx+i].glyph, &ink_rect, &logical_rect);

	      /* The list of logical rules */

	      /* Center dot of VAV */
	      if (gl == UNI_MAPIQ && base_char == UNI_VAV)
		{   
		  glyphs->glyphs[cluster_start_idx+i].geometry.x_offset
		    = base_ink_x_offset - ink_rect.x;

		  /* If VAV is a vertical bar without a roof, then we
		     need to make room for the dot by increasing the
		     cluster width. But how can I check if that is the
		     case??
		  */
		}

	      /* Dot over SHIN */
	      else if (gl == UNI_SHIN_DOT && base_char == UNI_SHIN)
		{   
		  glyphs->glyphs[cluster_start_idx+i].geometry.x_offset
		    = base_ink_x_offset + base_ink_width
		    - ink_rect.x - ink_rect.width;
		}

	      /* Dot over SIN */
	      else if (gl == UNI_SIN_DOT && base_char == UNI_SHIN)
		{  
		  glyphs->glyphs[cluster_start_idx+i].geometry.x_offset
		    = base_ink_x_offset -ink_rect.x;
		}

	      /* VOWEL DOT next to LAMED */
	      else if (gl == UNI_SIN_DOT && base_char == UNI_LAMED)
		{  
		  glyphs->glyphs[cluster_start_idx+i].geometry.x_offset
		    = base_ink_x_offset -ink_rect.x - 2*ink_rect.width;
		}

	      /* MAPIQ in PE or FINAL PE */
	      else if (gl == UNI_MAPIQ
		       && (base_char == UNI_PE || base_char == UNI_FINAL_PE))
		{
		  glyphs->glyphs[cluster_start_idx+i].geometry.x_offset
		    = base_ink_x_offset - ink_rect.x
		    + base_ink_width * 2/3 - ink_rect.width/2;

		  /* Another option is to offset the MAPIQ in y...
		     glyphs->glyphs[cluster_start_idx+i].geometry.y_offset
		     -= base_ink_height/5; */
		}

	      /* VOWEL DOT next to any other character */
	      else if (gl == UNI_SIN_DOT)
		{   
		  glyphs->glyphs[cluster_start_idx+i].geometry.x_offset
		    = base_ink_x_offset -ink_rect.x;
		}

	      /* Center by default */
	      else
		{  
		  glyphs->glyphs[cluster_start_idx+i].geometry.x_offset
		    = base_ink_x_offset - ink_rect.x
		    + base_ink_width/2 - ink_rect.width/2;
		}
	    }
	}
    }
}

static const char *
get_next_cluster(const char	*text,
		 gint		length,
		 gunichar       *cluster,
		 gint		*num_chrs)
{  
  const char *p;
  gint n_chars = 0;
  
  p = text;
  /* What is the maximum size of a Hebrew cluster? It is certainly
     bigger than two characters... */
  while (p < text + length && n_chars < MAX_CLUSTER_CHRS)  

    {
      gunichar current = g_utf8_get_char (p);
      
      if (n_chars == 0 ||
	  is_composible ((gunichar)(cluster[0]), current) )
	{
	  cluster[n_chars++] = current;
	  p = g_utf8_next_char (p);
	  if (n_chars == 1 &&
	      is_char_class(cluster[0], ~(NoDefine|SpacingLetter)) )
	      break;
	}
      else
	break;
    }

  *num_chrs = n_chars;
  return p;
}

static void
swap_range (PangoGlyphString *glyphs, int start, int end)
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

static void 
hebrew_engine_shape (PangoFont      *font,
		   const char       *text,
		   gint              length,
		   PangoAnalysis    *analysis,
		   PangoGlyphString *glyphs)
{
  HebrewFontInfo *font_info;
  const char *p;
  const char *log_cluster;
  gunichar cluster[MAX_CLUSTER_CHRS];
  gint num_chrs;

  pango_glyph_string_set_size (glyphs, 0);

  font_info = get_font_info (font);

  p = text;
  while (p < text + length)
    {
	log_cluster = p;
	p = get_next_cluster (p, text + length - p, cluster, &num_chrs);
	add_cluster (font_info, glyphs, log_cluster - text, cluster, num_chrs);
    }

  /* Simple bidi support */
  if (analysis->level % 2)
    {
      int start, end;

      /* Swap all glyphs */
      swap_range (glyphs, 0, glyphs->num_glyphs);

      /* Now reorder glyphs within each cluster back to LTR */
      for (start=0; start<glyphs->num_glyphs;)
  	{
	  end = start;
	  while (end < glyphs->num_glyphs &&
		 glyphs->log_clusters[end] == glyphs->log_clusters[start])
	    end++;

	  if (end > start + 1)
	    swap_range (glyphs, start, end);
	  start = end;
  	}
    }
}

static PangoCoverage *
hebrew_engine_get_coverage (PangoFont *font,
			   PangoLanguage *lang)
{
  PangoCoverage *result = pango_coverage_new ();
  
  HebrewFontInfo *font_info = get_font_info (font);
  
  if (font_info->type != HEBREW_FONT_NONE)
    {
      gunichar wc;
      
      for (wc = 0x590; wc <= 0x5f4; wc++)
	pango_coverage_set (result, wc, PANGO_COVERAGE_EXACT);
    }

  return result;
}

static PangoEngine *
hebrew_engine_x_new ()
{
  PangoEngineShape *result;
  
  result = g_new (PangoEngineShape, 1);

  result->engine.id = SCRIPT_ENGINE_NAME;
  result->engine.type = PANGO_ENGINE_TYPE_SHAPE;
  result->engine.length = sizeof (result);
  result->script_shape = hebrew_engine_shape;
  result->get_coverage = hebrew_engine_get_coverage;

  return (PangoEngine *)result;
}

/* The following three functions provide the public module API for
 * Pango. If we are compiling it is a module, then we name the
 * entry points script_engine_list, etc. But if we are compiling
 * it for inclusion directly in Pango, then we need them to
 * to have distinct names for this module, so we prepend
 * _pango_hebrew_x_
 */
#ifdef X_MODULE_PREFIX
#define MODULE_ENTRY(func) _pango_hebrew_x_##func
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
    return hebrew_engine_x_new ();
  else
    return NULL;
}

void 
MODULE_ENTRY(script_engine_unload) (PangoEngine *engine)
{
}

