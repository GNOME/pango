/* Pango
 * thai-shaper.c:
 *
 * Copyright (C) 1999 Red Hat Software
 * Author: Owen Taylor <otaylor@redhat.com>
 *
 * Software and Language Engineering Laboratory, NECTEC
 * Author: Theppitak Karoonboonyanan <thep@links.nectec.or.th>
 *
 * Copyright (c) 1996-2000 by Sun Microsystems, Inc.
 * Author: Chookij Vanatham <Chookij.Vanatham@Eng.Sun.COM>
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

#include <glib.h>
#include "pango-engine.h"
#include "thai-shaper.h"

#define MAX_CLUSTER_CHRS	256
#define MAX_GLYPHS		256

/* Define TACTIS character classes */
#define CTRL			0
#define NON			1
#define CONS			2
#define LV			3
#define FV1			4
#define FV2			5
#define FV3			6
#define BV1			7
#define BV2			8
#define BD			9
#define TONE			10
#define AD1			11
#define AD2			12
#define AD3			13
#define AV1			14
#define AV2			15
#define AV3			16

#define _ND			0
#define _NC			1
#define _UC			(1<<1)
#define _BC			(1<<2)
#define _SC			(1<<3)
#define _AV			(1<<4)
#define _BV			(1<<5)
#define _TN			(1<<6)
#define _AD			(1<<7)
#define _BD			(1<<8)
#define _AM			(1<<9)

#define NoTailCons		_NC
#define UpTailCons		_UC
#define BotTailCons		_BC
#define SpltTailCons		_SC
#define Cons			(NoTailCons|UpTailCons|BotTailCons|SpltTailCons)
#define AboveVowel		_AV
#define BelowVowel		_BV
#define Tone			_TN
#define AboveDiac		_AD
#define BelowDiac		_BD
#define SaraAm			_AM

#define char_class(wc)		TAC_char_class[(unsigned int)(wc)]
#define is_char_type(wc, mask)	(char_type_table[ucs2tis ((wc))] & (mask))

/* All combining marks for Thai fall in the range U+0E30-U+0E50,
 * so we confine our data tables to that range, and use
 * default values for characters outside those ranges.
 */

/* Map from code point to group used for rendering with XTIS fonts
 * (0 == base character)
 */
static const char groups[32] = {
  0, 1, 0, 0, 1, 1, 1, 1,
  1, 1, 1, 0, 0, 0, 0, 0,
  0, 0, 0, 0, 0, 0, 0, 2,
  2, 2, 2, 2, 2, 2, 1, 0
};

/* Map from code point to index within group 1
 * (0 == no combining mark from group 1)
 */   
static const char group1_map[32] = {
  0, 1, 0, 0, 2, 3, 4, 5,
  6, 7, 8, 0, 0, 0, 0, 0,
  0, 0, 0, 0, 0, 0, 0, 0,
  0, 0, 0, 0, 0, 0, 0, 0
};

/* Map from code point to index within group 2
 * (0 == no combining mark from group 2)
 */   
static const char group2_map[32] = {
  0, 0, 0, 0, 0, 0, 0, 0,
  0, 0, 0, 0, 0, 0, 0, 0,
  0, 0, 0, 0, 0, 0, 0, 1,
  2, 3, 4, 5, 6, 7, 1, 0
};

static const gint char_type_table[256] = {
  /*       0,   1,   2,   3,   4,   5,   6,   7,
           8,   9,   A,   B,   C,   D,   E,   F  */

  /*00*/ _ND, _ND, _ND, _ND, _ND, _ND, _ND, _ND,
         _ND, _ND, _ND, _ND, _ND, _ND, _ND, _ND,
  /*10*/ _ND, _ND, _ND, _ND, _ND, _ND, _ND, _ND,
         _ND, _ND, _ND, _ND, _ND, _ND, _ND, _ND,
  /*20*/ _ND, _ND, _ND, _ND, _ND, _ND, _ND, _ND,
         _ND, _ND, _ND, _ND, _ND, _ND, _ND, _ND,
  /*30*/ _ND, _ND, _ND, _ND, _ND, _ND, _ND, _ND,
         _ND, _ND, _ND, _ND, _ND, _ND, _ND, _ND,
  /*40*/ _ND, _ND, _ND, _ND, _ND, _ND, _ND, _ND,
         _ND, _ND, _ND, _ND, _ND, _ND, _ND, _ND,
  /*50*/ _ND, _ND, _ND, _ND, _ND, _ND, _ND, _ND,
         _ND, _ND, _ND, _ND, _ND, _ND, _ND, _ND,
  /*60*/ _ND, _ND, _ND, _ND, _ND, _ND, _ND, _ND,
         _ND, _ND, _ND, _ND, _ND, _ND, _ND, _ND,
  /*70*/ _ND, _ND, _ND, _ND, _ND, _ND, _ND, _ND,
         _ND, _ND, _ND, _ND, _ND, _ND, _ND, _ND,
  /*80*/ _ND, _ND, _ND, _ND, _ND, _ND, _ND, _ND,
         _ND, _ND, _ND, _ND, _ND, _ND, _ND, _ND,
  /*90*/ _ND, _ND, _ND, _ND, _ND, _ND, _ND, _ND,
         _ND, _ND, _ND, _ND, _ND, _ND, _ND, _ND,
		
  /*A0*/ _ND, _NC, _NC, _NC, _NC, _NC, _NC, _NC,
         _NC, _NC, _NC, _NC, _NC, _SC, _BC, _BC,
  /*B0*/ _SC, _NC, _NC, _NC, _NC, _NC, _NC, _NC,
         _NC, _NC, _NC, _UC, _NC, _UC, _NC, _UC,
  /*C0*/ _NC, _NC, _NC, _NC, _ND, _NC, _ND, _NC,
         _NC, _NC, _NC, _NC, _UC, _NC, _NC, _ND,
  /*D0*/ _ND, _AV, _ND, _AM, _AV, _AV, _AV, _AV,
         _BV, _BV, _BD, _ND, _ND, _ND, _ND, _ND,
  /*E0*/ _ND, _ND, _ND, _ND, _ND, _ND, _ND, _AD,
         _TN, _TN, _TN, _TN, _AD, _AD, _AD, _ND,
  /*F0*/ _ND, _ND, _ND, _ND, _ND, _ND, _ND, _ND,
         _ND, _ND, _ND, _ND, _ND, _ND, _ND, _ND,
};

static const gint TAC_char_class[256] = {
  /*	   0,   1,   2,   3,   4,   5,   6,   7,
           8,   9,   A,   B,   C,   D,   E,   F  */

  /*00*/ CTRL,CTRL,CTRL,CTRL,CTRL,CTRL,CTRL,CTRL,
         CTRL,CTRL,CTRL,CTRL,CTRL,CTRL,CTRL,CTRL,
  /*10*/ CTRL,CTRL,CTRL,CTRL,CTRL,CTRL,CTRL,CTRL,
         CTRL,CTRL,CTRL,CTRL,CTRL,CTRL,CTRL,CTRL,
  /*20*/  NON, NON, NON, NON, NON, NON, NON, NON,
          NON, NON, NON, NON, NON, NON, NON, NON,
  /*30*/  NON, NON, NON, NON, NON, NON, NON, NON,
          NON, NON, NON, NON, NON, NON, NON, NON,
  /*40*/  NON, NON, NON, NON, NON, NON, NON, NON,
          NON, NON, NON, NON, NON, NON, NON, NON,
  /*50*/  NON, NON, NON, NON, NON, NON, NON, NON,
          NON, NON, NON, NON, NON, NON, NON, NON,
  /*60*/  NON, NON, NON, NON, NON, NON, NON, NON,
          NON, NON, NON, NON, NON, NON, NON, NON,
  /*70*/  NON, NON, NON, NON, NON, NON, NON, NON,
          NON, NON, NON, NON, NON, NON, NON,CTRL,
  /*80*/ CTRL,CTRL,CTRL,CTRL,CTRL,CTRL,CTRL,CTRL,
         CTRL,CTRL,CTRL,CTRL,CTRL,CTRL,CTRL,CTRL,
  /*90*/ CTRL,CTRL,CTRL,CTRL,CTRL,CTRL,CTRL,CTRL,
         CTRL,CTRL,CTRL,CTRL,CTRL,CTRL,CTRL,CTRL,
  /*A0*/  NON,CONS,CONS,CONS,CONS,CONS,CONS,CONS,
         CONS,CONS,CONS,CONS,CONS,CONS,CONS,CONS,
  /*B0*/ CONS,CONS,CONS,CONS,CONS,CONS,CONS,CONS,
         CONS,CONS,CONS,CONS,CONS,CONS,CONS,CONS,
  /*C0*/ CONS,CONS,CONS,CONS, FV3,CONS, FV3,CONS,
         CONS,CONS,CONS,CONS,CONS,CONS,CONS, NON,
  /*D0*/  FV1, AV2, FV1, FV1, AV1, AV3, AV2, AV3,
          BV1, BV2,  BD, NON, NON, NON, NON, NON,
  /*E0*/   LV,  LV,  LV,  LV,  LV, FV2, NON, AD2,
         TONE,TONE,TONE,TONE, AD1, AD1, AD3, NON,
  /*F0*/  NON, NON, NON, NON, NON, NON, NON, NON,
          NON, NON, NON, NON, NON, NON, NON,CTRL,
};

static const gchar TAC_compose_and_input_check_type_table[17][17] = {
      /* Cn */ /* 0,   1,   2,   3,   4,   5,   6,   7,
                  8,   9,   A,   B,   C,   D,   E,   F       */
  /* Cn-1 00 */	{ 'X', 'A', 'A', 'A', 'A', 'A', 'A', 'R',
                'R', 'R', 'R', 'R', 'R', 'R', 'R', 'R', 'R' },
  /* 10 */      { 'X', 'A', 'A', 'A', 'S', 'S', 'A', 'R',
                'R', 'R', 'R', 'R', 'R', 'R', 'R', 'R', 'R' },
  /* 20 */      { 'X', 'A', 'A', 'A', 'A', 'S', 'A', 'C',
                'C', 'C', 'C', 'C', 'C', 'C', 'C', 'C', 'C' },
  /* 30 */      {'X', 'S', 'A', 'S', 'S', 'S', 'S', 'R',
                'R', 'R', 'R', 'R', 'R', 'R', 'R', 'R', 'R' },
  /* 40 */      { 'X', 'S', 'A', 'A', 'S', 'S', 'A', 'R',
                'R', 'R', 'R', 'R', 'R', 'R', 'R', 'R', 'R' },
  /* 50 */      { 'X', 'A', 'A', 'A', 'A', 'S', 'A', 'R',
                'R', 'R', 'R', 'R', 'R', 'R', 'R', 'R', 'R' },
  /* 60 */      { 'X', 'A', 'A', 'A', 'S', 'A', 'S', 'R',
                'R', 'R', 'R', 'R', 'R', 'R', 'R', 'R', 'R' },
  /* 70 */      { 'X', 'A', 'A', 'A', 'S', 'S', 'A', 'R',
                'R', 'R', 'C', 'C', 'R', 'R', 'R', 'R', 'R' },
  /* 80 */      { 'X', 'A', 'A', 'A', 'S', 'S', 'A', 'R',
                'R', 'R', 'C', 'R', 'R', 'R', 'R', 'R', 'R' },
  /* 90 */      { 'X', 'A', 'A', 'A', 'S', 'S', 'A', 'R',
                'R', 'R', 'R', 'R', 'R', 'R', 'R', 'R', 'R' },
  /* A0 */      { 'X', 'A', 'A', 'A', 'A', 'A', 'A', 'R',
                'R', 'R', 'R', 'R', 'R', 'R', 'R', 'R', 'R' },
  /* B0 */      { 'X', 'A', 'A', 'A', 'S', 'S', 'A', 'R',
                'R', 'R', 'R', 'R', 'R', 'R', 'R', 'R', 'R' },
  /* C0 */      { 'X', 'A', 'A', 'A', 'S', 'S', 'A', 'R',
                'R', 'R', 'R', 'R', 'R', 'R', 'R', 'R', 'R' },
  /* D0 */      { 'X', 'A', 'A', 'A', 'S', 'S', 'A', 'R',
                'R', 'R', 'R', 'R', 'R', 'R', 'R', 'R', 'R' },
  /* E0 */      { 'X', 'A', 'A', 'A', 'S', 'S', 'A', 'R',
                'R', 'R', 'C', 'C', 'R', 'R', 'R', 'R', 'R' },
  /* F0 */      { 'X', 'A', 'A', 'A', 'S', 'S', 'A', 'R',
                'R', 'R', 'C', 'R', 'R', 'R', 'R', 'R', 'R' },
                { 'X', 'A', 'A', 'A', 'S', 'S', 'A', 'R',
 	        'R', 'R', 'C', 'R', 'C', 'R', 'R', 'R', 'R' }
};

typedef struct {
  gint ShiftDown_TONE_AD[8];
  gint ShiftDownLeft_TONE_AD[8];
  gint ShiftLeft_TONE_AD[8];
  gint ShiftLeft_AV[7];
  gint ShiftDown_BV_BD[3];
  gint TailCutCons[4];
} ThaiShapeTable;

#define shiftdown_tone_ad(c,tbl)      ((tbl)->ShiftDown_TONE_AD[(c)-0xE7])
#define shiftdownleft_tone_ad(c,tbl)  ((tbl)->ShiftDownLeft_TONE_AD[(c)-0xE7])
#define shiftleft_tone_ad(c,tbl)      ((tbl)->ShiftLeft_TONE_AD[(c)-0xE7])
#define shiftleft_av(c,tbl)           ((tbl)->ShiftLeft_AV[(c)-0xD1])
#define shiftdown_bv_bd(c,tbl)        ((tbl)->ShiftDown_BV_BD[(c)-0xD8])
#define tailcutcons(c,tbl)            ((tbl)->TailCutCons[(c)-0xAD])

/* No adjusted vowel/tonemark glyphs (tis620-0)
 */
static const ThaiShapeTable tis620_0_shape_table = {
    { 0xE7, 0xE8, 0xE9, 0xEA, 0xEB, 0xEC, 0xED, 0xEE },
    { 0xE7, 0xE8, 0xE9, 0xEA, 0xEB, 0xEC, 0xED, 0xEE },
    { 0xE7, 0xE8, 0xE9, 0xEA, 0xEB, 0xEC, 0xED, 0xEE },
    { 0xD1, 0x00, 0x00, 0xD4, 0xD5, 0xD6, 0xD7 },
    { 0xD8, 0xD9, 0xDA },
    { 0xAD, 0x00, 0x00, 0xB0 }
};

/* Macintosh
 */
static const ThaiShapeTable Mac_shape_table = {
  { 0xE7, 0x88, 0x89, 0x8A, 0x8B, 0x8C, 0xED, 0xEE },
  { 0x93, 0x83, 0x84, 0x85, 0x86, 0x87, 0x8F, 0xEE },
  { 0x93, 0x98, 0x99, 0x9A, 0x9B, 0x9C, 0x8F, 0xEE },
  { 0x92, 0x00, 0x00, 0x94, 0x95, 0x96, 0x97 },
  { 0xD8, 0xD9, 0xDA },
  { 0xAD, 0x00, 0x00, 0xB0 }
};

/* Microsoft Window
 */
static const ThaiShapeTable Win_shape_table = {
    { 0xE7, 0x8B, 0x8C, 0x8D, 0x8E, 0x8F, 0xED, 0xEE },
    { 0x9A, 0x86, 0x87, 0x88, 0x89, 0x8A, 0x99, 0xEE },
    { 0x9A, 0x9B, 0x9C, 0x9D, 0x9E, 0x9F, 0x99, 0xEE },
    { 0x98, 0x00, 0x00, 0x81, 0x82, 0x83, 0x84 },
    { 0xFC, 0xFD, 0xFE },
    { 0x90, 0x00, 0x00, 0x80 }
};

static void
add_glyph (ThaiFontInfo     *font_info, 
	   PangoGlyphString *glyphs, 
	   gint              cluster_start, 
	   PangoGlyph        glyph,
	   gboolean          combining)
{
  PangoRectangle ink_rect, logical_rect;
  gint index = glyphs->num_glyphs;

  pango_glyph_string_set_size (glyphs, index + 1);
  
  glyphs->glyphs[index].glyph = glyph;
  glyphs->glyphs[index].attr.is_cluster_start = combining ? 0 : 1;
  
  glyphs->log_clusters[index] = cluster_start;

  pango_font_get_glyph_extents (font_info->font,
				glyphs->glyphs[index].glyph, &ink_rect, &logical_rect);

  if (combining || logical_rect.width > 0)
    {
      glyphs->glyphs[index].geometry.x_offset = 0;
      glyphs->glyphs[index].geometry.width = logical_rect.width;
    }
  else
    {
      glyphs->glyphs[index].geometry.x_offset = ink_rect.width;
      glyphs->glyphs[index].geometry.width = ink_rect.width;
    }
  glyphs->glyphs[index].geometry.y_offset = 0;
}

static gint
get_adjusted_glyphs_list (ThaiFontInfo *font_info,
			  gunichar *cluster,
			  gint num_chrs,
			  PangoGlyph *glyph_lists,
			  const ThaiShapeTable *shaping_table)
{
  switch (num_chrs)
    {
      case 1:
        if (is_char_type (cluster[0], BelowVowel|BelowDiac|AboveVowel|AboveDiac|Tone))
	  {
	    if (font_info->font_set == THAI_FONT_TIS)
	      glyph_lists[0] = thai_make_glyph (font_info, 0x20);
	    else
	      glyph_lists[0] = thai_make_glyph (font_info, 0x7F);
            glyph_lists[1] =
		thai_make_glyph (font_info, ucs2tis (cluster[0]));
	    return 2;
          }
	else
	  {
	    glyph_lists[0] =
		thai_make_glyph (font_info, ucs2tis (cluster[0]));
            return 1;
          }
        break;
        
      case 2:
        if (is_char_type (cluster[0], NoTailCons|BotTailCons|SpltTailCons) &&
            is_char_type (cluster[1], SaraAm))
	  {
            glyph_lists[0] =
		thai_make_glyph (font_info, ucs2tis (cluster[0]));
            glyph_lists[1] = thai_make_glyph (font_info, 0xED);
            glyph_lists[2] = thai_make_glyph (font_info, 0xD2);
            return 3;
          }
	else if (is_char_type (cluster[0], UpTailCons) &&
          	 is_char_type (cluster[1], SaraAm))
	  {
	    glyph_lists[0] =
		thai_make_glyph (font_info, ucs2tis (cluster[0]));
            glyph_lists[1] = thai_make_glyph (font_info,
					shiftleft_tone_ad (0xED, shaping_table));
            glyph_lists[2] = thai_make_glyph (font_info, 0xD2);
            return 3;
          }
	else if (is_char_type (cluster[0], NoTailCons|BotTailCons|SpltTailCons) &&
          	 is_char_type (cluster[1], AboveVowel))
	  {
	    glyph_lists[0] =
		thai_make_glyph (font_info, ucs2tis (cluster[0]));
	    glyph_lists[1] =
		thai_make_glyph (font_info, ucs2tis (cluster[1]));
	    return 2;
          }
	else if (is_char_type (cluster[0], NoTailCons|BotTailCons|SpltTailCons) &&
          	 is_char_type (cluster[1], AboveDiac|Tone))
	  {
	    glyph_lists[0] =
		thai_make_glyph (font_info, ucs2tis (cluster[0]));
	    glyph_lists[1] = thai_make_glyph (font_info,
		    	shiftdown_tone_ad (ucs2tis (cluster[1]), shaping_table));
	    return 2;
	  }
	else if (is_char_type (cluster[0], UpTailCons) &&
          	 is_char_type (cluster[1], AboveVowel))
	  {
	    glyph_lists[0] =
		thai_make_glyph (font_info, ucs2tis (cluster[0]));
	    glyph_lists[1] = thai_make_glyph (font_info,
		    	shiftleft_av (ucs2tis (cluster[1]), shaping_table));
	    return 2;
          }
	else if (is_char_type (cluster[0], UpTailCons) &&
          	 is_char_type (cluster[1], AboveDiac|Tone))
	  {
	    glyph_lists[0] =
		thai_make_glyph (font_info, ucs2tis (cluster[0]));
	    glyph_lists[1] = thai_make_glyph (font_info,
			shiftdownleft_tone_ad (ucs2tis (cluster[1]), shaping_table));
	    return 2;
	  }
	else if (is_char_type (cluster[0], NoTailCons|UpTailCons) &&
          	 is_char_type (cluster[1], BelowVowel|BelowDiac))
	  {
	    glyph_lists[0] =
		thai_make_glyph (font_info, ucs2tis (cluster[0]));
	    glyph_lists[1] =
		thai_make_glyph (font_info, ucs2tis (cluster[1]));
	    return 2;
          }
	else if (is_char_type (cluster[0], BotTailCons) &&
		 is_char_type (cluster[1], BelowVowel|BelowDiac))
	  {
	    glyph_lists[0] =
		thai_make_glyph (font_info, ucs2tis (cluster[0]));
	    glyph_lists[1] =
		thai_make_glyph (font_info,
			shiftdown_bv_bd (ucs2tis (cluster[1]), shaping_table));
	    return 2;
	  }
	else if (is_char_type (cluster[0], SpltTailCons) &&
          	 is_char_type (cluster[1], BelowVowel|BelowDiac))
	  {
	    glyph_lists[0] = thai_make_glyph (font_info,
		    		tailcutcons (ucs2tis (cluster[0]), shaping_table));
	    glyph_lists[1] =
		thai_make_glyph (font_info, ucs2tis (cluster[1]));
	    return 2;
	  }
	else
	  {
	    if (font_info->font_set == THAI_FONT_TIS)
	      glyph_lists[0] = thai_make_glyph (font_info, 0x20);
	    else
	      glyph_lists[0] = thai_make_glyph (font_info, 0x7F);
	    glyph_lists[1] =
		thai_make_glyph (font_info, ucs2tis (cluster[0]));
	    glyph_lists[2] =
		thai_make_glyph (font_info, ucs2tis (cluster[1]));
	    return 3;
	  }
        break;
          
      case 3:
        if (is_char_type (cluster[0], NoTailCons|BotTailCons|SpltTailCons) &&
            is_char_type (cluster[1], Tone) &&
            is_char_type (cluster[2], SaraAm))
	  {
            glyph_lists[0] =
		thai_make_glyph (font_info, ucs2tis (cluster[0]));
            glyph_lists[1] = thai_make_glyph (font_info, 0xED);
            glyph_lists[2] =
		thai_make_glyph (font_info, ucs2tis (cluster[1]));
            glyph_lists[3] = thai_make_glyph (font_info, 0xD2);
            return 4;
          }
	else if (is_char_type (cluster[0], UpTailCons) &&
		 is_char_type (cluster[1], Tone) &&
		 is_char_type (cluster[2], SaraAm))
	  {
	    glyph_lists[0] =
		thai_make_glyph (font_info, ucs2tis (cluster[0]));
	    glyph_lists[1] = thai_make_glyph (font_info,
				shiftleft_tone_ad (0xED, shaping_table));
	    glyph_lists[2] = thai_make_glyph (font_info,
		    		shiftleft_tone_ad (ucs2tis (cluster[1]), shaping_table));
	    glyph_lists[3] = thai_make_glyph (font_info, 0xD2);
	    return 4;
	  }
	else if (is_char_type (cluster[0], UpTailCons) &&
		 is_char_type (cluster[1], AboveVowel) &&
		 is_char_type (cluster[2], AboveDiac|Tone))
	  {
	    glyph_lists[0] =
		thai_make_glyph (font_info, ucs2tis (cluster[0]));
	    glyph_lists[1] = thai_make_glyph (font_info,
				shiftleft_av (ucs2tis (cluster[1]), shaping_table));
	    glyph_lists[2] = thai_make_glyph (font_info,
		    		shiftleft_tone_ad (ucs2tis (cluster[2]), shaping_table));
	    return 3;
	  }
	else if (is_char_type (cluster[0], UpTailCons) &&
		 is_char_type (cluster[1], BelowVowel) &&
		 is_char_type (cluster[2], AboveDiac|Tone))
	  {
	    glyph_lists[0] =
		thai_make_glyph (font_info, ucs2tis (cluster[0]));
	    glyph_lists[1] =
		thai_make_glyph (font_info, ucs2tis (cluster[1]));
	    glyph_lists[2] = thai_make_glyph (font_info,
			shiftdownleft_tone_ad (ucs2tis (cluster[2]), shaping_table));
	    return 3;
	  }
	else if (is_char_type (cluster[0], NoTailCons) &&
		 is_char_type (cluster[1], BelowVowel) &&
		 is_char_type (cluster[2], AboveDiac|Tone))
	  {
	    glyph_lists[0] =
		thai_make_glyph (font_info, ucs2tis (cluster[0]));
	    glyph_lists[1] =
		thai_make_glyph (font_info, ucs2tis (cluster[1]));
	    glyph_lists[2] =
		thai_make_glyph (font_info,
			shiftdown_tone_ad (ucs2tis (cluster[2]), shaping_table));
	    return 3;
	  }
	else if (is_char_type (cluster[0], SpltTailCons) &&
		 is_char_type (cluster[1], BelowVowel) &&
		 is_char_type (cluster[2], AboveDiac|Tone))
	  {
	    glyph_lists[0] = thai_make_glyph (font_info,
		    		tailcutcons (ucs2tis (cluster[0]), shaping_table));
	    glyph_lists[1] =
		thai_make_glyph (font_info, ucs2tis (cluster[1]));
	    glyph_lists[2] = thai_make_glyph (font_info,
		    		shiftdown_tone_ad (ucs2tis (cluster[2]), shaping_table));
	    return 3;
	  }
	else if (is_char_type (cluster[0], BotTailCons) &&
		 is_char_type (cluster[1], BelowVowel) &&
		 is_char_type (cluster[2], AboveDiac|Tone))
	  {
	    glyph_lists[0] =
		thai_make_glyph (font_info, ucs2tis (cluster[0]));
	    glyph_lists[1] = thai_make_glyph (font_info,
		    		shiftdown_bv_bd (ucs2tis (cluster[1]), shaping_table));
	    glyph_lists[2] = thai_make_glyph (font_info,
				shiftdown_tone_ad (ucs2tis (cluster[2]), shaping_table));
	    return 3;
	  }
	else
	  {
	    glyph_lists[0] =
		thai_make_glyph (font_info, ucs2tis (cluster[0]));
	    glyph_lists[1] =
		thai_make_glyph (font_info, ucs2tis (cluster[1]));
	    glyph_lists[2] =
		thai_make_glyph (font_info, ucs2tis (cluster[2]));
	    return 3;
	  }
      break;
    }

    return 0;
}

static gint
get_glyphs_list (ThaiFontInfo	*font_info,
		 gunichar	*cluster,
		 gint		num_chrs,
		 PangoGlyph	*glyph_lists)
{
  gint i;

  switch (font_info->font_set)
    {
      case THAI_FONT_NONE:
        for (i=0; i < num_chrs; i++)
	  glyph_lists[i] = thai_make_unknown_glyph (font_info, glyph_lists[i]);
        return num_chrs;

      case THAI_FONT_TIS:
	/* TIS620-0 + Wtt2.0 Extension
	 */
        return get_adjusted_glyphs_list (font_info, cluster,
		num_chrs, glyph_lists, &tis620_0_shape_table);
      
      case THAI_FONT_TIS_MAC:
	/* MacIntosh Extension
	 */
        return get_adjusted_glyphs_list (font_info, cluster,
		num_chrs, glyph_lists, &Mac_shape_table);
      
      case THAI_FONT_TIS_WIN:
	/* Microsoft Extension
	 */
        return get_adjusted_glyphs_list (font_info, cluster,
		num_chrs, glyph_lists, &Win_shape_table);
      
      case THAI_FONT_ISO10646:
        for (i=0; i < num_chrs; i++)
          glyph_lists[i] = thai_make_glyph (font_info, cluster[i]);
        return num_chrs;
    }

  return 0;			/* Quiet GCC */
}

static void
add_cluster (ThaiFontInfo	*font_info,
	     PangoGlyphString	*glyphs,
	     gint		cluster_start,
	     gunichar		*cluster,
	     gint		num_chrs)
{
  PangoGlyph glyphs_list[MAX_GLYPHS];
  gint num_glyphs;
  gint i;

  if (!isthai (cluster[0]))
    {
      g_assert (num_chrs == 1);
      add_glyph (font_info, glyphs, cluster_start,
		 thai_make_glyph (font_info, cluster[0]),
		 FALSE);
    }
  else
    {
      num_glyphs = get_glyphs_list(font_info, cluster, num_chrs, glyphs_list);
      for (i=0; i<num_glyphs; i++)
	add_glyph (font_info, glyphs, cluster_start, glyphs_list[i],
		   i == 0 ? FALSE : TRUE);
    }
}

static gboolean
is_wtt_composible (gunichar cur_wc, gunichar nxt_wc)
{
  switch (TAC_compose_and_input_check_type_table[char_class (ucs2tis (cur_wc))]
						[char_class (ucs2tis (nxt_wc))])
    {
      case 'A':
      case 'S':
      case 'R':
      case 'X':
        return FALSE;

      case 'C':
        return TRUE;
    }

  g_assert_not_reached ();
  return FALSE;
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
  while (p < text + length && n_chars < 3)  
    {
      gunichar current = g_utf8_get_char (p);

      /* Non-thai characters get split into a single character cluster */
      if (!isthai (current))
	{
	  if (n_chars == 0)
	    {
	      cluster[n_chars++] = current;
	      p = g_utf8_next_char (p);
	    }
	  break;
	}
      else if (n_chars == 0 ||
	       is_wtt_composible ((gunichar)(cluster[n_chars - 1]), current) ||
	       (n_chars == 1 &&
		is_char_type (cluster[0], Cons) && 
		is_char_type (current, SaraAm)) ||
	       (n_chars == 2 &&
		is_char_type (cluster[0], Cons) &&
		is_char_type (cluster[1], Tone) &&
		is_char_type (current, SaraAm)))
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
thai_engine_shape (PangoEngineShape *engine,
		   PangoFont        *font,
		   const char       *text,
		   gint              length,
		   PangoAnalysis    *analysis,
		   PangoGlyphString *glyphs)
{
  ThaiFontInfo *font_info;
  const char *p;
  const char *log_cluster;
  gunichar cluster[MAX_CLUSTER_CHRS];
  gint num_chrs;

  pango_glyph_string_set_size (glyphs, 0);

  font_info = thai_get_font_info (font);

  p = text;
  while (p < text + length)
    {
	log_cluster = p;
	p = get_next_cluster (p, text + length - p, cluster, &num_chrs);
	add_cluster (font_info, glyphs, log_cluster - text, cluster, num_chrs);
    }
}

