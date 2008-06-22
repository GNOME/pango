/* Pango
 * thai-shaper.c:
 *
 * Copyright (C) 1999 Red Hat Software
 * Author: Owen Taylor <otaylor@redhat.com>
 *
 * Copyright (C) 2005 Theppitak Karoonboonyanan
 * Copyright (C) 2002 Software and Language Engineering Laboratory, NECTEC
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
#include "thai-charprop.h"
#include "thai-shaper.h"

#define MAX_CLUSTER_CHRS	256
#define MAX_GLYPHS		256


typedef struct {
  guchar Start_TONE_AD, Start_AV, Start_BV_BD, Start_TailCutCons;

  guchar ShiftDown_TONE_AD[8];
  guchar ShiftDownLeft_TONE_AD[8];
  guchar ShiftLeft_TONE_AD[8];
  guchar ShiftLeft_AV[7];
  guchar ShiftDown_BV_BD[3];
  guchar TailCutCons[4];

  guchar AmComp[2];  /* Sara Am components */
} ThaiShapeTable;

#define shiftdown_tone_ad(c,tbl) \
	((tbl)->ShiftDown_TONE_AD[(c)-(tbl)->Start_TONE_AD])
#define shiftdownleft_tone_ad(c,tbl) \
	((tbl)->ShiftDownLeft_TONE_AD[(c)-(tbl)->Start_TONE_AD])
#define shiftleft_tone_ad(c,tbl) \
	((tbl)->ShiftLeft_TONE_AD[(c)-(tbl)->Start_TONE_AD])
#define shiftleft_av(c,tbl) \
	((tbl)->ShiftLeft_AV[(c)-(tbl)->Start_AV])
#define shiftdown_bv_bd(c,tbl) \
	((tbl)->ShiftDown_BV_BD[(c)-(tbl)->Start_BV_BD])
#define tailcutcons(c,tbl) \
	((tbl)->TailCutCons[(c)-(tbl)->Start_TailCutCons])

/* No adjusted vowel/tonemark glyphs (tis620-0)
 */
static const ThaiShapeTable tis620_0_shape_table = {
    0xE7, 0xD1, 0xD8, 0xAD,
    { 0xE7, 0xE8, 0xE9, 0xEA, 0xEB, 0xEC, 0xED, 0xEE },
    { 0xE7, 0xE8, 0xE9, 0xEA, 0xEB, 0xEC, 0xED, 0xEE },
    { 0xE7, 0xE8, 0xE9, 0xEA, 0xEB, 0xEC, 0xED, 0xEE },
    { 0xD1, 0x00, 0x00, 0xD4, 0xD5, 0xD6, 0xD7 },
    { 0xD8, 0xD9, 0xDA },
    { 0xAD, 0x00, 0x00, 0xB0 },
    { 0xED, 0xD2 }
};

/* Macintosh
 */
static const ThaiShapeTable Mac_shape_table = {
  0xE7, 0xD1, 0xD8, 0xAD,
  { 0xE7, 0x88, 0x89, 0x8A, 0x8B, 0x8C, 0xED, 0xEE },
  { 0x93, 0x83, 0x84, 0x85, 0x86, 0x87, 0x8F, 0xEE },
  { 0x93, 0x98, 0x99, 0x9A, 0x9B, 0x9C, 0x8F, 0xEE },
  { 0x92, 0x00, 0x00, 0x94, 0x95, 0x96, 0x97 },
  { 0xFC, 0xFD, 0xFE },
  { 0x90, 0x00, 0x00, 0x80 },
  { 0xED, 0xD2 }
};

/* Microsoft Window
 */
static const ThaiShapeTable Win_shape_table = {
    0xE7, 0xD1, 0xD8, 0xAD,
    { 0xE7, 0x8B, 0x8C, 0x8D, 0x8E, 0x8F, 0xED, 0xEE },
    { 0x9A, 0x86, 0x87, 0x88, 0x89, 0x8A, 0x99, 0xEE },
    { 0x9A, 0x9B, 0x9C, 0x9D, 0x9E, 0x9F, 0x99, 0xEE },
    { 0x98, 0x00, 0x00, 0x81, 0x82, 0x83, 0x84 },
    { 0xFC, 0xFD, 0xFE },
    { 0x90, 0x00, 0x00, 0x80 },
    { 0xED, 0xD2 }
};

static const ThaiShapeTable Lao_shape_table = {
  0x67, 0x51, 0x58, 0x2D,
  { 0x67, 0x68, 0x69, 0x6A, 0x6B, 0x6C, 0x6D, 0x6E },
  { 0x67, 0x68, 0x69, 0x6A, 0x6B, 0x6C, 0x6D, 0x6E },
  { 0x67, 0x68, 0x69, 0x6A, 0x6B, 0x6C, 0x6D, 0x6E },
  { 0x51, 0x00, 0x00, 0x54, 0x55, 0x56, 0x57 },
  { 0x58, 0x59, 0x00 },
  { 0x2D, 0x00, 0x00, 0x30 },
  { 0x6D, 0x52 }
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

static PangoGlyph
get_null_base_glyph (ThaiFontInfo *font_info)
{
  return thai_get_glyph_uni (font_info, 0x25cc);
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
	if (is_char_type (cluster[0],
			  BelowVowel|BelowDiac|AboveVowel|AboveDiac|Tone|SaraAm))
	  {
	    gint n;
	    glyph_lists[0] = get_null_base_glyph (font_info);
	    n = glyph_lists[0] ? 1 : 0;
	    glyph_lists[n++] =
		thai_make_glyph_tis (font_info, ucs2tis (cluster[0]));
	    return n;
	  }
	else
	  {
	    glyph_lists[0] =
		thai_make_glyph_tis (font_info, ucs2tis (cluster[0]));
	    return 1;
	  }
	break;

      case 2:
	if (is_char_type (cluster[0], NoTailCons|BotTailCons|SpltTailCons) &&
	    is_char_type (cluster[1], SaraAm))
	  {
	    glyph_lists[0] =
		thai_make_glyph_tis (font_info, ucs2tis (cluster[0]));
	    glyph_lists[1] = thai_make_glyph_tis (font_info, shaping_table->AmComp[0]);
	    glyph_lists[2] = thai_make_glyph_tis (font_info, shaping_table->AmComp[1]);
	    return 3;
	  }
	else if (is_char_type (cluster[0], UpTailCons) &&
		 is_char_type (cluster[1], SaraAm))
	  {
	    glyph_lists[0] =
		thai_make_glyph_tis (font_info, ucs2tis (cluster[0]));
	    glyph_lists[1] = thai_make_glyph_tis (font_info,
					shiftleft_tone_ad (shaping_table->AmComp[0], shaping_table));
	    glyph_lists[2] = thai_make_glyph_tis (font_info, shaping_table->AmComp[1]);
	    return 3;
	  }
	else if (is_char_type (cluster[0], NoTailCons|BotTailCons|SpltTailCons) &&
		 is_char_type (cluster[1], AboveVowel))
	  {
	    glyph_lists[0] =
		thai_make_glyph_tis (font_info, ucs2tis (cluster[0]));
	    glyph_lists[1] =
		thai_make_glyph_tis (font_info, ucs2tis (cluster[1]));
	    return 2;
	  }
	else if (is_char_type (cluster[0], NoTailCons|BotTailCons|SpltTailCons) &&
		 is_char_type (cluster[1], AboveDiac|Tone))
	  {
	    glyph_lists[0] =
		thai_make_glyph_tis (font_info, ucs2tis (cluster[0]));
	    glyph_lists[1] = thai_make_glyph_tis (font_info,
			shiftdown_tone_ad (ucs2tis (cluster[1]), shaping_table));
	    return 2;
	  }
	else if (is_char_type (cluster[0], UpTailCons) &&
		 is_char_type (cluster[1], AboveVowel))
	  {
	    glyph_lists[0] =
		thai_make_glyph_tis (font_info, ucs2tis (cluster[0]));
	    glyph_lists[1] = thai_make_glyph_tis (font_info,
			shiftleft_av (ucs2tis (cluster[1]), shaping_table));
	    return 2;
	  }
	else if (is_char_type (cluster[0], UpTailCons) &&
		 is_char_type (cluster[1], AboveDiac|Tone))
	  {
	    glyph_lists[0] =
		thai_make_glyph_tis (font_info, ucs2tis (cluster[0]));
	    glyph_lists[1] = thai_make_glyph_tis (font_info,
			shiftdownleft_tone_ad (ucs2tis (cluster[1]), shaping_table));
	    return 2;
	  }
	else if (is_char_type (cluster[0], NoTailCons|UpTailCons) &&
		 is_char_type (cluster[1], BelowVowel|BelowDiac))
	  {
	    glyph_lists[0] =
		thai_make_glyph_tis (font_info, ucs2tis (cluster[0]));
	    glyph_lists[1] =
		thai_make_glyph_tis (font_info, ucs2tis (cluster[1]));
	    return 2;
	  }
	else if (is_char_type (cluster[0], BotTailCons) &&
		 is_char_type (cluster[1], BelowVowel|BelowDiac))
	  {
	    glyph_lists[0] =
		thai_make_glyph_tis (font_info, ucs2tis (cluster[0]));
	    glyph_lists[1] =
		thai_make_glyph_tis (font_info,
			shiftdown_bv_bd (ucs2tis (cluster[1]), shaping_table));
	    return 2;
	  }
	else if (is_char_type (cluster[0], SpltTailCons) &&
		 is_char_type (cluster[1], BelowVowel|BelowDiac))
	  {
	    glyph_lists[0] = thai_make_glyph_tis (font_info,
			       tailcutcons (ucs2tis (cluster[0]), shaping_table));
	    glyph_lists[1] =
		thai_make_glyph_tis (font_info, ucs2tis (cluster[1]));
	    return 2;
	  }
	else
	  {
	    gint n;
	    glyph_lists[0] = get_null_base_glyph (font_info);
	    n = glyph_lists[0] ? 1 : 0;
	    glyph_lists[n++] =
		thai_make_glyph_tis (font_info, ucs2tis (cluster[0]));
	    glyph_lists[n++] =
		thai_make_glyph_tis (font_info, ucs2tis (cluster[1]));
	    return n;
	  }
	break;

      case 3:
	if (is_char_type (cluster[0], NoTailCons|BotTailCons|SpltTailCons) &&
	    is_char_type (cluster[1], Tone) &&
	    is_char_type (cluster[2], SaraAm))
	  {
	    glyph_lists[0] =
		thai_make_glyph_tis (font_info, ucs2tis (cluster[0]));
	    glyph_lists[1] = thai_make_glyph_tis (font_info, shaping_table->AmComp[0]);
	    glyph_lists[2] =
		thai_make_glyph_tis (font_info, ucs2tis (cluster[1]));
	    glyph_lists[3] = thai_make_glyph_tis (font_info, shaping_table->AmComp[1]);
	    return 4;
	  }
	else if (is_char_type (cluster[0], UpTailCons) &&
		 is_char_type (cluster[1], Tone) &&
		 is_char_type (cluster[2], SaraAm))
	  {
	    glyph_lists[0] =
		thai_make_glyph_tis (font_info, ucs2tis (cluster[0]));
	    glyph_lists[1] = thai_make_glyph_tis (font_info,
				shiftleft_tone_ad (shaping_table->AmComp[0], shaping_table));
	    glyph_lists[2] = thai_make_glyph_tis (font_info,
				shiftleft_tone_ad (ucs2tis (cluster[1]), shaping_table));
	    glyph_lists[3] = thai_make_glyph_tis (font_info, shaping_table->AmComp[1]);
	    return 4;
	  }
	else if (is_char_type (cluster[0], UpTailCons) &&
		 is_char_type (cluster[1], AboveVowel) &&
		 is_char_type (cluster[2], AboveDiac|Tone))
	  {
	    glyph_lists[0] =
		thai_make_glyph_tis (font_info, ucs2tis (cluster[0]));
	    glyph_lists[1] = thai_make_glyph_tis (font_info,
				shiftleft_av (ucs2tis (cluster[1]), shaping_table));
	    glyph_lists[2] = thai_make_glyph_tis (font_info,
				shiftleft_tone_ad (ucs2tis (cluster[2]), shaping_table));
	    return 3;
	  }
	else if (is_char_type (cluster[0], UpTailCons) &&
		 is_char_type (cluster[1], BelowVowel) &&
		 is_char_type (cluster[2], AboveDiac|Tone))
	  {
	    glyph_lists[0] =
		thai_make_glyph_tis (font_info, ucs2tis (cluster[0]));
	    glyph_lists[1] =
		thai_make_glyph_tis (font_info, ucs2tis (cluster[1]));
	    glyph_lists[2] = thai_make_glyph_tis (font_info,
			shiftdownleft_tone_ad (ucs2tis (cluster[2]), shaping_table));
	    return 3;
	  }
	else if (is_char_type (cluster[0], NoTailCons) &&
		 is_char_type (cluster[1], BelowVowel) &&
		 is_char_type (cluster[2], AboveDiac|Tone))
	  {
	    glyph_lists[0] =
		thai_make_glyph_tis (font_info, ucs2tis (cluster[0]));
	    glyph_lists[1] =
		thai_make_glyph_tis (font_info, ucs2tis (cluster[1]));
	    glyph_lists[2] =
		thai_make_glyph_tis (font_info,
			shiftdown_tone_ad (ucs2tis (cluster[2]), shaping_table));
	    return 3;
	  }
	else if (is_char_type (cluster[0], SpltTailCons) &&
		 is_char_type (cluster[1], BelowVowel) &&
		 is_char_type (cluster[2], AboveDiac|Tone))
	  {
	    glyph_lists[0] = thai_make_glyph_tis (font_info,
				tailcutcons (ucs2tis (cluster[0]), shaping_table));
	    glyph_lists[1] =
		thai_make_glyph_tis (font_info, ucs2tis (cluster[1]));
	    glyph_lists[2] = thai_make_glyph_tis (font_info,
				shiftdown_tone_ad (ucs2tis (cluster[2]), shaping_table));
	    return 3;
	  }
	else if (is_char_type (cluster[0], BotTailCons) &&
		 is_char_type (cluster[1], BelowVowel) &&
		 is_char_type (cluster[2], AboveDiac|Tone))
	  {
	    glyph_lists[0] =
		thai_make_glyph_tis (font_info, ucs2tis (cluster[0]));
	    glyph_lists[1] = thai_make_glyph_tis (font_info,
				shiftdown_bv_bd (ucs2tis (cluster[1]), shaping_table));
	    glyph_lists[2] = thai_make_glyph_tis (font_info,
				shiftdown_tone_ad (ucs2tis (cluster[2]), shaping_table));
	    return 3;
	  }
	else
	  {
	    glyph_lists[0] =
		thai_make_glyph_tis (font_info, ucs2tis (cluster[0]));
	    glyph_lists[1] =
		thai_make_glyph_tis (font_info, ucs2tis (cluster[1]));
	    glyph_lists[2] =
		thai_make_glyph_tis (font_info, ucs2tis (cluster[2]));
	    return 3;
	  }
      break;

      default: /* e.g. Lao cluster with below cons + upper/lower vowel + tone */
	{
	  gint i;
	  for (i = 0; i < num_chrs; i++)
	    glyph_lists[i] = thai_make_glyph_tis (font_info, ucs2tis (cluster[i]));
	  return num_chrs;
	}
    }

    return 0;
}

static gint
get_glyphs_list (ThaiFontInfo	*font_info,
		 PangoScript     script,
		 gunichar	*cluster,
		 gint		num_chrs,
		 PangoGlyph	*glyph_lists)
{
  gint i;

  switch (script)
    {
      case PANGO_SCRIPT_THAI:
	switch (font_info->font_set)
	  {
	    default:
	    case THAI_FONT_NONE:
	      for (i=0; i < num_chrs; i++)
		glyph_lists[i] = thai_make_unknown_glyph (font_info, cluster[i]);
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
	  }
	break;

      case PANGO_SCRIPT_LAO:
	return get_adjusted_glyphs_list (font_info, cluster,
		num_chrs, glyph_lists, &Lao_shape_table);

      default:
	for (i=0; i < num_chrs; i++)
	  glyph_lists[i] = thai_make_unknown_glyph (font_info, cluster[i]);
	return num_chrs;
    }

  return 0;			/* Quiet GCC */
}

static void
add_cluster (ThaiFontInfo	*font_info,
	     PangoScript         script,
	     PangoGlyphString	*glyphs,
	     gint		cluster_start,
	     gunichar		*cluster,
	     gint		num_chrs)
{
  PangoGlyph glyphs_list[MAX_GLYPHS];
  gint num_glyphs;
  gint i;

  if (isthai (cluster[0]))
    {
      num_glyphs = get_glyphs_list(font_info, script, cluster, num_chrs, glyphs_list);
      for (i=0; i<num_glyphs; i++)
	add_glyph (font_info, glyphs, cluster_start, glyphs_list[i],
		   i == 0 ? FALSE : TRUE);
    }
  else if (islao (cluster[0]))
    {
      num_glyphs = get_glyphs_list(font_info, script, cluster, num_chrs, glyphs_list);
      for (i=0; i<num_glyphs; i++)
	add_glyph (font_info, glyphs, cluster_start, glyphs_list[i],
		   i == 0 ? FALSE : TRUE);
    }
  else
    {
      g_assert (num_chrs == 1);
      add_glyph (font_info, glyphs, cluster_start,
		 thai_make_glyph_uni (font_info, cluster[0]),
		 FALSE);
    }
}

static gboolean
is_wtt_composible (gunichar cur_wc, gunichar nxt_wc)
{
  switch (TAC_compose_input(cur_wc, nxt_wc))
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
  PangoScript cluster_script = PANGO_SCRIPT_INVALID_CODE, cur_script;
  const char *p;
  gint n_chars = 0;
  gunichar current;

  for (p = text; p < text + length; p = g_utf8_next_char (p))
    {
      current = g_utf8_get_char (p);
      cur_script = pango_script_for_unichar (current);
      if (cluster_script == PANGO_SCRIPT_INVALID_CODE)
	cluster_script = cur_script;
      if (cur_script != cluster_script ||
	  (n_chars > 0 &&
	   !is_wtt_composible (cluster[n_chars - 1], current)))
	break;
      cluster[n_chars++] = current;
    }

  *num_chrs = n_chars;
  return p;
}

void
thai_set_glyphs (ThaiFontInfo     *font_info,
		 const char       *text,
		 gint              length,
		 PangoScript       script,
		 PangoGlyphString *glyphs)
{
  gint n_chars;
  const char *p;
  const char *log_cluster;
  gunichar cluster[MAX_CLUSTER_CHRS];

  pango_glyph_string_set_size (glyphs, 0);

  p = text;
  while (p < text + length)
    {
	log_cluster = p;
	p = get_next_cluster (p, text + length - p, cluster, &n_chars);
	add_cluster (font_info, script, glyphs, log_cluster - text, cluster, n_chars);
    }
}
