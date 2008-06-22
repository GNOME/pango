/* Pango
 * tibetan-fc.c: Shaper for Tibetan script
 *
 * Copyright (C) 2005 DIT, Government of Bhutan <http://www.dit.gov.bt>
 * Contact person : Pema Geyleg <pema_geyleg@druknet.bt>
 *
 *  Based on code from khmer shapers developed by Jens Herden
 *  <jens@tibetanos.inf > and Javier Sola <javier@tibetanos.info>
 *
 * Based on code from other shapers
 * Copyright (C) 1999-2004 Red Hat Software
 * Author: Owen Taylor <otaylor@redhat.com>

 * Partially based on Indic shaper
 * Copyright (C) 2001, 2002 IBM Corporation
 * Author: Eric Mader <mader@jtcsv.com>
 *
 * The first module for Tibetan shaper was developed by Mr. Karunakar under
 * PanLocalization project.
 * Mr. Chris Fynn, Mr.Javier Sola, Mr. Namgay Thinley were involved
 * while developing this shaper.
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
 * The license on the original Indic shaper code is as follows:
 *
 *  Permission is hereby granted, free of charge, to any person
 * obtaining a copy of this software and associated documentation
 * files (the "Software"), to deal in the Software without
 * restriction, including without limitation the rights to use, copy,
 * modify, merge, publish, distribute, and/or sell copies of the
 * Software, and to permit persons to whom the Software is furnished
 * to do so, provided that the above copyright notice(s) and this
 * permission notice appear in all copies of the Software and that
 * both the above copyright notice(s) and this permission notice
 * appear in supporting documentation.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND,
 * EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF
 * MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND
 * NONINFRINGEMENT OF THIRD PARTY RIGHTS. IN NO EVENT SHALL THE
 * COPYRIGHT HOLDER OR HOLDERS INCLUDED IN THIS NOTICE BE LIABLE FOR
 * ANY CLAIM, OR ANY SPECIAL INDIRECT OR CONSEQUENTIAL DAMAGES, OR ANY
 * DAMAGES WHATSOEVER RESULTING FROM LOSS OF USE, DATA OR PROFITS,
 * WHETHER IN AN ACTION OF CONTRACT, NEGLIGENCE OR OTHER TORTIOUS
 * ACTION, ARISING OUT OF OR IN CONNECTION WITH THE USE OR PERFORMANCE
 * OF THIS SOFTWARE.
 *
 * Except as contained in this notice, the name of a copyright holder
 * shall not be used in advertising or otherwise to promote the sale,
 * use or other dealings in this Software without prior written
 * authorization of the copyright holder.
 */
#include "config.h"
#include <string.h>

#include "pango-engine.h"
#include "pango-ot.h"
#include "pango-utils.h"
#include "pangofc-font.h"


#define SCRIPT_ENGINE_NAME "TibetanScriptEngineFc"
#define RENDER_TYPE PANGO_RENDER_TYPE_FC


typedef PangoEngineShape      TibetanEngineFc;
typedef PangoEngineShapeClass TibetanEngineFcClass ;


static PangoEngineScriptInfo tibetan_scripts[] =
{
  { PANGO_SCRIPT_TIBETAN, "*" }
};

static PangoEngineInfo script_engines[] =
{
  {
    SCRIPT_ENGINE_NAME,
    PANGO_ENGINE_TYPE_SHAPE,
    RENDER_TYPE,
    tibetan_scripts, G_N_ELEMENTS (tibetan_scripts)
  }
};


/*
 * Vocabulary
 *     Base ->         A consonant in its full (not subscript) form. It is the
 *	             center of the syllable, it can be souranded by subjoined consonants, vowels,
 *	             signs... but there is only one base in a stack, it has to be coded as
 *	             the first character of the syllable.Included here are also groups of base + subjoined
 *                   which are represented by one single code point in unicode (e.g. 0F43) Also other characters
 *                   that might take subjoined consonants or other combining characters.
 *     Subjoined ->    Subjoined consonants and groups of subjoined consonants which have a single code-point
 *	             to repersent the group (even if each subjoined consonant is represented independently
 *	             by anothe code-point
 *     Tsa Phru -->    Tsa Phru character, Bhutanese people will always place it right after the base, but sometimes, due to
 *	             "normalization" is placed after all the subjoined consonants, and it is also permitted there.
 *     A Chung  Vowel lengthening mark --> . 0F71 It is placed after the base and any subjoined consonants but before any vowels
 *     Precomposed Sanskrit vowels --> The are combinations of subjoined consonants + vowels that have been assigned
 *	             a given code-point (in spite of each single part of them having also a code-point
 *	             They are avoided, and users are encouraged to use the combination of code-points that
 *	             represents the same sound instead of using this combined characters. This is included here
 *	             for compatibility with possible texts that use them (they are not in the Dzongkha keyboard).
 *     Halanta ->      The Halanta or Virama character 0F84 indicates that a consonant should not use its inheernt vowel,
 *	             in spite of not having other vowels present. It is usually placed immediatly after a base consonant,
 *	             but in some special cases it can also be placed after a subjoined consonant, so this is also
 *	             permitted in this algorithm. (Halanta is always displayed in Tibetan not used as a connecting char)
 *
 *     Subjoined vowels -> Dependent vowels (matras) placed below the base and below all subjoined consonants. There
 *	             might be as much as three subjoined vowels in a given stack (only one in general text, but up
 *	             to three for abreviations, they have to be permitted).
 *     Superscript vowels -> There are three superscript vowels, and they can be repeated or combined (up to three
 *	             times. They can combine with subjoined vowels, and are always coded after these.
 *     Anusvara -->    Nasalisation sign. Traditioinally placed in absence of vowels, but also after vowels. In some
 *	             special cases it can be placed before a vowel, so this is also permitted
 *     Candrabindu ->  Forms of the Anusvara with different glyphs (and different in identity) which can be placed
 *	             without vowel or after the vowel, but never before. Cannot combine with Anusvara.
 *     Stress marks -> Marks placed above or below a syllable, affecting the whole syllable. They are combining
 *	             marks, so they have to be attached to a specific stack. The are using to emphasise a syllable.
 *
 *     Digits ->       Digits are not considered as non-combining characters because there are a few characters which
 *	             combine with them, so they have to be considered independently.
 *     Digit combining marks -> dependent marks that combine with digits.
 *
 *     TODO
 *     There are a number of characters in the CJK block that are used in Tibetan script, two of these are symbols
 *     are used as bases for combining glyphs, and have not been encoded in Tibetan. As these characters are outside
 *     of the tibetan block, they have not been treated in this program.
*/


enum TibetanCharClassValues
{
	CC_RESERVED             =  0, /* Non Combining Characters*/
	CC_BASE                 =  1, /* Base Consonants, Base Consonants with Subjoined attached in code point, Sanskrit base marks*/
	CC_SUBJOINED            =  2, /* Subjoined Consonats, combination of more than Subjoined Consonants in the code point*/
	CC_TSA_PHRU             =  3, /* Tsa-Phru character 0F39*/
	CC_A_CHUNG              =  4, /* Vowel Lenthening a-chung mark 0F71*/
	CC_COMP_SANSKRIT        =  5, /* Precomposed Sanskrit vowels including Subjoined characters and vowels*/
	CC_HALANTA              =  6, /* Halanta Character 0F84*/
	CC_BELOW_VOWEL          =  7, /* Subjoined vowels*/
	CC_ABOVE_VOWEL          =  8, /* Superscript vowels*/
	CC_ANUSVARA             =  9, /* Tibetan sign Rjes Su Nga Ro 0F7E*/
	CC_CANDRABINDU          = 10, /* Tibetan sign Sna Ldan and Nyi Zla Naa Da 0F82, 0F83*/
	CC_VISARGA              = 11, /* Tibetan sign Rnam Bcad (0F7F)*/
	CC_ABOVE_S_MARK         = 12, /* Stress Marks placed above the text*/
	CC_BELOW_S_MARK         = 13, /* Stress Marks placed below the text*/
	CC_DIGIT                = 14, /* Dzongkha Digits*/
	CC_PRE_DIGIT_MARK       = 15, /* Mark placed before the digit*/
	CC_POST_BELOW_DIGIT_M   = 16, /* Mark placed below or after the digit*/
	CC_COUNT                = 17  /* This is the number of character classes*/
};


enum TibetanCharClassFlags
{
	CF_CLASS_MASK    = 0x0000FFFF,

	CF_DOTTED_CIRCLE = 0x04000000,  /* add a dotted circle if a character with this flag is the first in a syllable*/
	CF_DIGIT         = 0x01000000,  /* flag to speed up comparaisson*/
	CF_PREDIGIT      = 0x02000000,  /* flag to detect pre-digit marks for reordering*/

	/* position flags*/
	CF_POS_BEFORE    = 0x00080000,
	CF_POS_BELOW     = 0x00040000,
	CF_POS_ABOVE     = 0x00020000,
	CF_POS_AFTER     = 0x00010000,
	CF_POS_MASK      = 0x000f0000
};


/* Characters that get refrered to by name */
enum TibetanChar
{
  C_DOTTED_CIRCLE = 0x25CC,
  C_PRE_NUMBER_MARK = 0x0F3F
};


enum
{
    /* simple classes, they are used in the statetable (in this file) to control the length of a syllable
     * they are also used to know where a character should be placed (location in reference to the base character)
     * and also to know if a character, when independtly displayed, should be displayed with a dotted-circle to
     * indicate error in syllable construction
     */
    _xx = CC_RESERVED,
    _ba = CC_BASE,
    _sj = CC_SUBJOINED | CF_DOTTED_CIRCLE | CF_POS_BELOW,
    _tp = CC_TSA_PHRU  | CF_DOTTED_CIRCLE | CF_POS_ABOVE,
    _ac = CC_A_CHUNG |  CF_DOTTED_CIRCLE | CF_POS_BELOW,
    _cs = CC_COMP_SANSKRIT | CF_DOTTED_CIRCLE | CF_POS_BELOW,
    _ha = CC_HALANTA | CF_DOTTED_CIRCLE | CF_POS_BELOW,
    _bv = CC_BELOW_VOWEL | CF_DOTTED_CIRCLE | CF_POS_BELOW,
    _av = CC_ABOVE_VOWEL | CF_DOTTED_CIRCLE | CF_POS_ABOVE,
    _an = CC_ANUSVARA | CF_DOTTED_CIRCLE | CF_POS_ABOVE,
    _cb = CC_CANDRABINDU | CF_DOTTED_CIRCLE | CF_POS_ABOVE,
    _vs = CC_VISARGA | CF_DOTTED_CIRCLE| CF_POS_AFTER,
    _as = CC_ABOVE_S_MARK | CF_DOTTED_CIRCLE | CF_POS_ABOVE,
    _bs = CC_BELOW_S_MARK | CF_DOTTED_CIRCLE | CF_POS_BELOW,
    _di = CC_DIGIT | CF_DIGIT,
    _pd = CC_PRE_DIGIT_MARK | CF_DOTTED_CIRCLE | CF_PREDIGIT | CF_POS_BEFORE ,
    _bd = CC_POST_BELOW_DIGIT_M | CF_DOTTED_CIRCLE | CF_POS_AFTER
};


/* Character class: a character class value
 * ORed with character class flags.
 */
typedef glong TibetanCharClass;
/*_xx Non Combining characters*/
/*_ba Base Consonants*/
/*_sj Subjoined consonants*/
/*_tp Tsa - phru*/
/*_ac A-chung, Vowel Lengthening mark*/
/*_cs Precomposed Sanskrit vowel + subjoined consonants*/
/*_ha Halanta/Virama*/
/*_bv Below vowel*/
/*_av above vowel*/
/*_an Anusvara*/
/*_cb Candrabindu*/
/*_vs Visaraga/Post mark*/
/*_as Upper Stress marks*/
/*_bs Lower Stress marks*/
/*_di Digit*/
/*_pd Number pre combining, Needs reordering*/
/*_bd Other number combining marks*/


static const TibetanCharClass tibetanCharClasses[] =
{
  /* 0    1    2    3    4    5    6    7    8    9   a     b   c    d     e   f*/
    _xx, _ba, _xx, _xx, _ba, _ba, _xx, _xx, _xx, _xx, _xx, _xx, _xx, _xx, _xx, _xx, /* 0F00 - 0F0F 0*/
    _xx, _xx, _xx, _xx, _xx, _xx, _xx, _xx, _bd, _bd, _xx, _xx, _xx, _xx, _xx, _xx, /* 0F10 - 0F1F 1*/
    _di, _di, _di, _di, _di, _di, _di, _di, _di, _di, _xx, _xx, _xx, _xx, _xx, _xx, /* 0F20 - 0F2F 2*/
    _xx, _xx, _xx, _xx, _xx, _bs, _xx, _bs, _xx, _tp, _xx, _xx, _xx, _xx, _bd, _pd, /* 0F30 - 0F3F 3*/
    _ba, _ba, _ba, _ba, _ba, _ba, _ba, _ba, _xx, _ba, _ba, _ba, _ba, _ba, _ba, _ba, /* 0F40 - 0F4F 4*/
    _ba, _ba, _ba, _ba, _ba, _ba, _ba, _ba, _ba, _ba, _ba, _ba, _ba, _ba, _ba, _ba, /* 0F50 - 0F5F 5*/
    _ba, _ba, _ba, _ba, _ba, _ba, _ba, _ba, _ba, _ba, _ba, _xx, _xx, _xx, _xx, _xx, /* 0F60 - 0F6F 6*/
    _xx, _ac, _av, _cs, _bv, _bv, _cs, _cs, _cs, _cs, _av, _av, _av, _av, _an, _vs, /* 0F70 - 0F7F 7*/
    _av, _cs, _cb, _cb, _ha, _xx, _as, _as, _ba, _ba, _ba, _ba, _xx, _xx, _xx, _xx, /* 0F80 - 0F8F 8*/
    _sj, _sj, _sj, _sj, _sj, _sj, _sj, _sj, _xx, _sj, _sj, _sj, _sj, _sj, _sj, _sj, /* 0F90 - 0F9F 9*/
    _sj, _sj, _sj, _sj, _sj, _sj, _sj, _sj, _sj, _sj, _sj, _sj, _sj, _sj, _sj, _sj, /* 0FA0 - 0FAF a*/
    _sj, _sj, _sj, _sj, _sj, _sj, _sj, _sj, _sj, _sj, _sj, _sj, _sj, _xx, _sj, _sj, /* 0FB0 - 0FBF b*/
    _xx, _xx, _xx, _xx, _xx, _xx, _bs, _xx, _xx, _xx, _xx, _xx, _xx, _xx, _xx, _xx, /* 0FC0 - 0FCF c*/
    _xx, _xx, _xx, _xx, _xx, _xx, _xx, _xx, _xx, _xx, _xx, _xx, _xx, _xx, _xx, _xx, /* 0FD0 - 0FDF  d*/
    _xx, _xx, _xx, _xx, _xx, _xx, _xx, _xx, _xx, _xx, _xx, _xx, _xx, _xx, _xx, _xx, /* 0FE0 - 0FEF e*/
    _xx, _xx, _xx, _xx, _xx, _xx, _xx, _xx, _xx, _xx, _xx, _xx, _xx, _xx, _xx, _xx, /* 0FF0 - 0FFF f*/
};

/* this define must reflect the range of tibetanCharClasses */
/*First Tibetan Character*/
#define firstChar 0x0F00
/*Last Tibetan Character*/
#define lastChar 0x0FFF

/* The stateTable is used to calculate the end (the length) of a well
 * formed Tibetan Stack
 *
 * Each horizontal line is ordered exactly the same way as the values in TibetanClassTable
 * CharClassValues.This coincidence of values allows the follow up of the table.
 *
 * Each line corresponds to a state, which does not necessarily need to be a type
 * of component... for example, state 2 is a base, with is always a first character
 * in the Stack but the state could be produced a consonant of any type when
 * it is the first character that is analysed (in ground state).
 */

static const gint8 tibetanStateTable[][CC_COUNT] =
{
    /*Dzongkha state table*/
    /*xx  ba  sj  tp  ac  cs  ha  bv  av  an  cb  vs  as  bs  di  pd  bd*/
    { 1,  2,  4,  3,  8,  7,  9, 10, 14, 13, 17, 18, 19, 19, 20, 21, 21,}, /*  0 - ground state*/
    {-1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1,}, /*  1 - exit state (or sign to the right of the syllable)*/
    {-1, -1,  4,  3,  8,  7,  9, 10, 14, 13, 17, 18, 19, 19, -1, -1, -1,}, /*  2 - Base consonant*/
    {-1, -1,  5, -1,  8,  7, -1, 10, 14, 13, 17, 18, 19, 19, -1, -1, -1,}, /*  3 - Tsa phru after base*/
    {-1, -1,  4,  6,  8,  7,  9, 10, 14, 13, 17, 18, 19, 19, -1, -1, -1,}, /*  4 - Subjoined consonant after base*/
    {-1, -1,  5, -1,  8,  7, -1, 10, 14, 13, 17, 18, 19, 19, -1, -1, -1,}, /*  5 - Subjoined consonant after tsa phru*/
    {-1, -1, -1, -1,  8,  7, -1, 10, 14, 13, 17, 18, 19, 19, -1, -1, -1,}, /*  6 - Tsa phru after subjoined consonant*/
    {-1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, 19, 19, -1, -1, -1,}, /*  7 - Pre Composed Sanskrit*/
    {-1, -1, -1, -1, -1, -1, -1, 10, 14, 13, 17, 18, 19, 19, -1, -1, -1,}, /*  8 - A-chung*/
    {-1, -1, -1, -1, -1, -1, -1, -1, 14, 13, 17, -1, 19, 19, -1, -1, -1,}, /*  9 - Halanta*/
    {-1, -1, -1, -1, -1, -1, -1, 11, 14, 13, 17, 18, 19, 19, -1, -1, -1,}, /* 10 - below vowel 1*/
    {-1, -1, -1, -1, -1, -1, -1, 12, 14, 13, 17, 18, 19, 19, -1, -1, -1,}, /* 11 - below vowel 2*/
    {-1, -1, -1, -1, -1, -1, -1, -1, 14, 13, 17, 18, 19, 19, -1, -1, -1,}, /* 12 - below vowel 3*/
    {-1, -1, -1, -1, -1, -1, -1, -1, 14, 17, 17, 18, 19, 19, -1, -1, -1,}, /* 13 - Anusvara before vowel*/
    {-1, -1, -1, -1, -1, -1, -1, -1, 15, 17, 17, 18, 19, 19, -1, -1, -1,}, /* 14 - above vowel 1*/
    {-1, -1, -1, -1, -1, -1, -1, -1, 16, 17, 17, 18, 19, 19, -1, -1, -1,}, /* 15 - above vowel 2*/
    {-1, -1, -1, -1, -1, -1, -1, -1, -1, 17, 17, 18, 19, 19, -1, -1, -1,}, /* 16 - above vowel 3*/
    {-1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, 18, 19, 19, -1, -1, -1,}, /* 17 - Anusvara or Candrabindu after vowel*/
    {-1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, 19, 19, -1, -1, -1,}, /* 18 - Visarga*/
    {-1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1,}, /* 19 - strss mark*/
    {-1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, 21, 21,}, /* 20 - digit*/
    {-1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1,}, /* 21 - digit mark*/
};


enum property_flags
{
  abvf = 0x0001,
  pref = 0x0002,
  pstf = 0x0004,
  blwf = 0x0008,

  pres = 0x0010,
  blws = 0x0020,
  abvs = 0x0040,
  psts = 0x0080,
  clig = 0x0100,

  dist = 0x0200,
  blwm = 0x0400,
  abvm = 0x0800,
};


enum properties
{
  blwf_p    = /*(blwf | blws | clig | dist | blwm)*/ (abvf | pref | pstf | pres | abvs | psts | abvm),
  pstf_p    = /*(blwf | blws | pref | pres | pstf | psts | clig | dist | blwm)*/ (abvf | abvs | abvm),
  abvf_p    = /*(abvf | abvs | clig | dist | abvm)*/ (pref | pstf | blwf | pres | blws | psts | blwm),
  pref_p    = /*(pref | pres | clig | dist)*/ (abvf | pstf | blwf | blws | abvs | psts | blwm | abvm),
  default_p = /*(pres | blws | clig | dist | abvm | blwm)*/ (pref | blwf |abvf | pstf | abvs | psts)
};


/* Below we define how a character in the input string is either in the tibetanCharClasses table
 * (in which case we get its type back), or an unknown object in which case we get _xx (CC_RESERVED) back
 */
static TibetanCharClass
get_char_class (gunichar ch)
{

  if (ch < firstChar || ch > lastChar)
    return CC_RESERVED;

  return tibetanCharClasses[ch - firstChar];
}


/* Given an input string of characters and a location in which to start looking
 * calculate, using the state table, which one is the last character of the syllable
 * that starts in the starting position.
 */
static glong
find_syllable (const gunichar *chars,
	       glong           start,
	       glong           char_count)
{
  glong cursor = start;
  gint8 state = 0;
  TibetanCharClass charClass;

  while (cursor < char_count)
    {
      charClass = get_char_class (chars[cursor]) & CF_CLASS_MASK;
      state = tibetanStateTable[state][charClass];

      if (state < 0)
	break;

      cursor += 1;
    }

  return cursor;
}

static const PangoOTFeatureMap gsub_features[] =
{
  {"ccmp", PANGO_OT_ALL_GLYPHS},
  {"locl", PANGO_OT_ALL_GLYPHS},
  {"blws", blws},
  {"abvs", abvs},
  {"psts", psts},
  {"clig", clig},
  {"calt", PANGO_OT_ALL_GLYPHS}
};

static const PangoOTFeatureMap gpos_features[] =
{
  {"dist", dist},
  {"blwm", blwm},
  {"abvm", abvm},
  {"kern", PANGO_OT_ALL_GLYPHS},
  {"mark", PANGO_OT_ALL_GLYPHS},
  {"mkmk", PANGO_OT_ALL_GLYPHS}
};

static PangoGlyph
get_index (PangoFcFont *fc_font, gunichar wc)
{
  PangoGlyph index = pango_fc_font_get_glyph (fc_font, wc);
  if (!index)
    index = PANGO_GET_UNKNOWN_GLYPH ( wc);
  return index;
}


static void
tibetan_engine_shape (PangoEngineShape *engine,
		    PangoFont        *font,
		    const char       *text,
		    int               length,
		    const PangoAnalysis *analysis,
		    PangoGlyphString *glyphs)
{
  PangoFcFont *fc_font;
  FT_Face face;
  PangoOTRulesetDescription desc;
  const PangoOTRuleset *ruleset;
  PangoOTBuffer *buffer;
  glong n_chars;
  gunichar *wcs;
  const char *p;
  int i;
  glong syllable;
  TibetanCharClass charClass;
  glong cursor = 0;

  g_return_if_fail (font != NULL);
  g_return_if_fail (text != NULL);
  g_return_if_fail (length >= 0);
  g_return_if_fail (analysis != NULL);

  fc_font = PANGO_FC_FONT (font);
  face = pango_fc_font_lock_face (fc_font);
  if (!face)
    return;

  buffer = pango_ot_buffer_new (fc_font);

  wcs = g_utf8_to_ucs4_fast (text, length, &n_chars);

  p = text;
  /* This loop only exits when we reach the end of a run, which may contain
   * several syllables.
   */
  while (cursor < n_chars)
    {
      syllable = find_syllable (wcs, cursor, n_chars);

      /* shall we add a dotted circle?
      * If in the position in which the base should be (first char in the string) there is
      * a character that has the Dotted circle flag (a character that cannot be a base)
      * then write a dotted circle
      */
      if (get_char_class (wcs[cursor]) & CF_DOTTED_CIRCLE)
	{
	  pango_ot_buffer_add_glyph (buffer, get_index (fc_font, C_DOTTED_CIRCLE), default_p, p - text);
	}

      /* If it encounters a digit followed by number pre combining mark, then reorder the two characters
      * coeng Ro if they are present
      */
      for (i = cursor; i < syllable; i += 1)
	{
	  charClass = get_char_class (wcs[i]);

	  if ((charClass & CF_DIGIT )
	      && ( get_char_class (wcs[i+1]) & CF_PREDIGIT))
	   {
			 pango_ot_buffer_add_glyph (buffer, get_index (fc_font, C_PRE_NUMBER_MARK), pref_p, p - text);
			 p = g_utf8_next_char (p);
			 pango_ot_buffer_add_glyph (buffer, get_index (fc_font, wcs[i]), pref_p, p - text);
			       i += 1;
	 } else {
	  switch (charClass & CF_POS_MASK)
	    {
	      case CF_POS_ABOVE :
			     pango_ot_buffer_add_glyph (buffer, get_index (fc_font, wcs[i]), abvf_p, p - text);
			     break;

	      case CF_POS_AFTER :
			     pango_ot_buffer_add_glyph (buffer, get_index (fc_font, wcs[i]), pstf_p, p - text);
			     break;

	      case CF_POS_BELOW :
			     pango_ot_buffer_add_glyph (buffer, get_index (fc_font, wcs[i]), blwf_p, p - text);
			     break;

	      default:
		   /* default - any other characters  */
		  pango_ot_buffer_add_glyph (buffer, get_index (fc_font, wcs[i]), default_p, p - text);
		  break;
	    } /* switch */
	   }

	  p = g_utf8_next_char (p);
	} /* for */

      cursor = syllable; /* move the pointer to the start of next syllable */
    } /* while */

  desc.script = analysis->script;
  desc.language = analysis->language;

  desc.n_static_gsub_features = G_N_ELEMENTS (gsub_features);
  desc.static_gsub_features = gsub_features;
  desc.n_static_gpos_features = G_N_ELEMENTS (gpos_features);
  desc.static_gpos_features = gpos_features;

  /* TODO populate other_features from analysis->extra_attrs */
  desc.n_other_features = 0;
  desc.other_features = NULL;

  ruleset = pango_ot_ruleset_get_for_description (pango_ot_info_get (face), &desc);

  pango_ot_ruleset_substitute (ruleset, buffer);
  pango_ot_ruleset_position (ruleset, buffer);
  pango_ot_buffer_output (buffer, glyphs);

  g_free (wcs);
  pango_ot_buffer_destroy (buffer);

  pango_fc_font_unlock_face (fc_font);
}


static void
tibetan_engine_fc_class_init (PangoEngineShapeClass *class)
{
  class->script_shape = tibetan_engine_shape;
}

PANGO_ENGINE_SHAPE_DEFINE_TYPE (TibetanEngineFc, tibetan_engine_fc,
				tibetan_engine_fc_class_init, NULL)


void
PANGO_MODULE_ENTRY(init) (GTypeModule *module)
{
  tibetan_engine_fc_register_type (module);
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
    return g_object_new (tibetan_engine_fc_type, NULL);
  else
    return NULL;
}
