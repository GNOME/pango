/* Pango
 * khmer-fc.c: Shaper for Khmer script
 *
 * Copyright (C) 2004 Open Forum of Cambodia (www.forum.org.kh / www.khmeros.info)
 * Authors: Jens Herden <jens@khmeros.info> and Javier Sola <javier@khmeros.info>
 *
 * Based on code from other shapers
 * Copyright (C) 1999-2004 Red Hat Software
 * Author: Owen Taylor <otaylor@redhat.com>

 * Partially based on Indic shaper
 * Copyright (C) 2001, 2002 IBM Corporation
 * Author: Eric Mader <mader@jtcsv.com>
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
 *  * Permission is hereby granted, free of charge, to any person
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


#define SCRIPT_ENGINE_NAME "KhmerScriptEngineFc"
#define RENDER_TYPE PANGO_RENDER_TYPE_FC


typedef PangoEngineShape      KhmerEngineFc;
typedef PangoEngineShapeClass KhmerEngineFcClass ;


static PangoEngineScriptInfo khmer_scripts[] =
{
  { PANGO_SCRIPT_KHMER, "*" }
};

static PangoEngineInfo script_engines[] =
{
  {
    SCRIPT_ENGINE_NAME,
    PANGO_ENGINE_TYPE_SHAPE,
    RENDER_TYPE,
    khmer_scripts, G_N_ELEMENTS (khmer_scripts)
  }
};


/* Vocabulary
 *     Base ->         A consonant or an independent vowel in its full (not subscript) form. It is the
 *                     center of the syllable, it can be surrounded by coeng (subscript) consonants, vowels,
 *                     split vowels, signs... but there is only one base in a syllable, it has to be coded as
 *                     the first character of the syllable.
 *     split vowel --> vowel that has two parts placed separately (e.g. Before and after the consonant).
 *                     Khmer language has five of them. Khmer split vowels either have one part before the
 *                     base and one after the base or they have a part before the base and a part above the base.
 *                     The first part of all Khmer split vowels is the same character, identical to
 *                     the glyph of Khmer dependent vowel SRA EI
 *     coeng -->  modifier used in Khmer to construct coeng (subscript) consonants
 *                Differently than indian languages, the coeng modifies the consonant that follows it,
 *                not the one preceding it  Each consonant has two forms, the base form and the subscript form
 *                the base form is the normal one (using the consonants code-point), the subscript form is
 *                displayed when the combination coeng + consonant is encountered.
 *     Consonant of type 1 -> A consonant which has subscript for that only occupies space under a base consonant
 *     Consonant of type 2.-> Its subscript form occupies space under and before the base (only one, RO)
 *     Consonant of Type 3 -> Its subscript form occupies space under and after the base (KHO, CHHO, THHO, BA, YO, SA)
 *     Consonant shifter -> Khmer has to series of consonants. The same dependent vowel has different sounds
 *                          if it is attached to a consonant of the first series or a consonant of the second series
 *                          Most consonants have an equivalent in the other series, but some of theme exist only in
 *                          one series (for example SA). If we want to use the consonant SA with a vowel sound that
 *                          can only be done with a vowel sound that corresponds to a vowel accompanying a consonant
 *                          of the other series, then we need to use a consonant shifter: TRIISAP or MUSIKATOAN
 *                          x17C9 y x17CA. TRIISAP changes a first series consonant to second series sound and
 *                          MUSIKATOAN a second series consonant to have a first series vowel sound.
 *                          Consonant shifter are both normally supercript marks, but, when they are followed by a
 *                          superscript, they change shape and take the form of subscript dependent vowel SRA U.
 *                          If they are in the same syllable as a coeng consonant, Unicode 3.0 says that they
 *                          should be typed before the coeng. Unicode 4.0 breaks the standard and says that it should
 *                          be placed after the coeng consonant.
 *     Dependent vowel ->   In khmer dependent vowels can be placed above, below, before or after the base
 *                          Each vowel has its own position. Only one vowel per syllable is allowed.
 *     Signs            ->  Khmer has above signs and post signs. Only one above sign and/or one post sign are
 *                          Allowed in a syllable.
 *
 *
 *  order is important here! This order must be the same that is found in each horizontal
 *  line in the statetable for Khmer (see khmerStateTable) .
 */
enum KhmerCharClassValues
{
  CC_RESERVED             =  0,
  CC_CONSONANT            =  1, /* Consonant of type 1 or independent vowel */
  CC_CONSONANT2           =  2, /* Consonant of type 2 */
  CC_CONSONANT3           =  3, /* Consonant of type 3 */
  CC_ZERO_WIDTH_NJ_MARK   =  4, /* Zero Width non joiner character (0x200C) */
  CC_CONSONANT_SHIFTER    =  5,
  CC_ROBAT                =  6, /* Khmer special diacritic accent -treated differently in state table */
  CC_COENG                =  7, /* Subscript consonant combining character */
  CC_DEPENDENT_VOWEL      =  8,
  CC_SIGN_ABOVE           =  9,
  CC_SIGN_AFTER           = 10,
  CC_ZERO_WIDTH_J_MARK    = 11, /* Zero width joiner character */
  CC_COUNT                = 12  /* This is the number of character classes */
};


enum KhmerCharClassFlags
{
  CF_CLASS_MASK    = 0x0000FFFF,

  CF_CONSONANT     = 0x01000000,  /* flag to speed up comparing */
  CF_SPLIT_VOWEL   = 0x02000000,  /* flag for a split vowel -> the first part is added in front of the syllable */
  CF_DOTTED_CIRCLE = 0x04000000,  /* add a dotted circle if a character with this flag is the first in a syllable */
  CF_COENG         = 0x08000000,  /* flag to speed up comparing */
  CF_SHIFTER       = 0x10000000,  /* flag to speed up comparing */
  CF_ABOVE_VOWEL   = 0x20000000,  /* flag to speed up comparing */

  /* position flags */
  CF_POS_BEFORE    = 0x00080000,
  CF_POS_BELOW     = 0x00040000,
  CF_POS_ABOVE     = 0x00020000,
  CF_POS_AFTER     = 0x00010000,
  CF_POS_MASK      = 0x000f0000
};


/* Characters that get refrered to by name */
enum KhmerChar
{
  C_SIGN_ZWNJ     = 0x200C,
  C_SIGN_ZWJ      = 0x200D,
  C_DOTTED_CIRCLE = 0x25CC,
  C_RO            = 0x179A,
  C_VOWEL_AA      = 0x17B6,
  C_SIGN_NIKAHIT  = 0x17C6,
  C_VOWEL_E       = 0x17C1,
  C_COENG         = 0x17D2
};


enum
{
  /* simple classes, they are used in the state table (in this file) to control the length of a syllable
   * they are also used to know where a character should be placed (location in reference to the base character)
   * and also to know if a character, when independently displayed, should be displayed with a dotted-circle to
   * indicate error in syllable construction
   */
  _xx = CC_RESERVED,
  _sa = CC_SIGN_ABOVE | CF_DOTTED_CIRCLE | CF_POS_ABOVE,
  _sp = CC_SIGN_AFTER | CF_DOTTED_CIRCLE| CF_POS_AFTER,
  _c1 = CC_CONSONANT | CF_CONSONANT,
  _c2 = CC_CONSONANT2 | CF_CONSONANT,
  _c3 = CC_CONSONANT3 | CF_CONSONANT,
  _rb = CC_ROBAT | CF_POS_ABOVE | CF_DOTTED_CIRCLE,
  _cs = CC_CONSONANT_SHIFTER | CF_DOTTED_CIRCLE | CF_SHIFTER,
  _dl = CC_DEPENDENT_VOWEL | CF_POS_BEFORE | CF_DOTTED_CIRCLE,
  _db = CC_DEPENDENT_VOWEL | CF_POS_BELOW | CF_DOTTED_CIRCLE,
  _da = CC_DEPENDENT_VOWEL | CF_POS_ABOVE | CF_DOTTED_CIRCLE | CF_ABOVE_VOWEL,
  _dr = CC_DEPENDENT_VOWEL | CF_POS_AFTER | CF_DOTTED_CIRCLE,
  _co = CC_COENG | CF_COENG | CF_DOTTED_CIRCLE,

  /* split vowel */
  _va = _da | CF_SPLIT_VOWEL,
  _vr = _dr | CF_SPLIT_VOWEL
};


/* Character class: a character class value
 * ORed with character class flags.
 */
typedef glong KhmerCharClass;


/* Character class tables
 * _xx character does not combine into syllable, such as numbers, puntuation marks, non-Khmer signs...
 * _sa Sign placed above the base
 * _sp Sign placed after the base
 * _c1 Consonant of type 1 or independent vowel (independent vowels behave as type 1 consonants)
 * _c2 Consonant of type 2 (only RO)
 * _c3 Consonant of type 3
 * _rb Khmer sign robat u17CC. combining mark for subscript consonants
 * _cd Consonant-shifter
 * _dl Dependent vowel placed before the base (left of the base)
 * _db Dependent vowel placed below the base
 * _da Dependent vowel placed above the base
 * _dr Dependent vowel placed behind the base (right of the base)
 * _co Khmer combining mark COENG u17D2, combines with the consonant or independent vowel following
 *     it to create a subscript consonant or independent vowel
 * _va Khmer split vowel in wich the first part is before the base and the second one above the base
 * _vr Khmer split vowel in wich the first part is before the base and the second one behind (right of) the base
 */
static const KhmerCharClass khmerCharClasses[] =
{
  _c1, _c1, _c1, _c3, _c1, _c1, _c1, _c1, _c3, _c1, _c1, _c1, _c1, _c3, _c1, _c1, /* 1780 - 178F */
  _c1, _c1, _c1, _c1, _c3, _c1, _c1, _c1, _c1, _c3, _c2, _c1, _c1, _c1, _c3, _c3, /* 1790 - 179F */
  _c1, _c3, _c1, _c1, _c1, _c1, _c1, _c1, _c1, _c1, _c1, _c1, _c1, _c1, _c1, _c1, /* 17A0 - 17AF */
  _c1, _c1, _c1, _c1, _dr, _dr, _dr, _da, _da, _da, _da, _db, _db, _db, _va, _vr, /* 17B0 - 17BF */
  _vr, _dl, _dl, _dl, _vr, _vr, _sa, _sp, _sp, _cs, _cs, _sa, _rb, _sa, _sa, _sa, /* 17C0 - 17CF */
  _sa, _sa, _co, _sa, _xx, _xx, _xx, _xx, _xx, _xx, _xx, _xx, _xx, _sa, _xx, _xx, /* 17D0 - 17DF */
};

/* this define must reflect the range of khmerCharClasses */
#define firstChar 0x1780
#define lastChar 0x17df



/* The stateTable is used to calculate the end (the length) of a well
 * formed Khmer Syllable.
 *
 * Each horizontal line is ordered exactly the same way as the values in KhmerClassTable
 * CharClassValues. This coincidence of values allows the follow up of the table.
 *
 * Each line corresponds to a state, which does not necessarily need to be a type
 * of component... for example, state 2 is a base, with is always a first character
 * in the syllable, but the state could be produced a consonant of any type when
 * it is the first character that is analysed (in ground state).
 *
 * Differentiating 3 types of consonants is necessary in order to
 * forbid the use of certain combinations, such as having a second
 * coeng after a coeng RO,
 * The inexistent possibility of having a type 3 after another type 3 is permitted,
 * eliminating it would very much complicate the table, and it does not create typing
 * problems, as the case above.
 *
 * The table is quite complex, in order to limit the number of coeng consonants
 * to 2 (by means of the table).
 *
 * There a peculiarity, as far as Unicode is concerned:
 * - The consonant-shifter is considered in two possible different
 *   locations, the one considered in Unicode 3.0 and the one considered in
 *   Unicode 4.0. (there is a backwards compatibility problem in this standard).
 *
 *
 * xx    independent character, such as a number, punctuation sign or non-khmer char
 *
 * c1    Khmer consonant of type 1 or an independent vowel
 *       that is, a letter in which the subscript for is only under the
 *       base, not taking any space to the right or to the left
 *
 * c2    Khmer consonant of type 2, the coeng form takes space under
 *       and to the left of the base (only RO is of this type)
 *
 * c3    Khmer consonant of type 3. Its subscript form takes space under
 *       and to the right of the base.
 *
 * cs    Khmer consonant shifter
 *
 * rb    Khmer robat
 *
 * co    coeng character (u17D2)
 *
 * dv    dependent vowel (including split vowels, they are treated in the same way).
 *       even if dv is not defined above, the component that is really tested for is
 *       KhmerClassTable::CC_DEPENDENT_VOWEL, which is common to all dependent vowels
 *
 * zwj   Zero Width joiner
 *
 * zwnj  Zero width non joiner
 *
 * sa    above sign
 *
 * sp    post sign
 *
 * there are lines with equal content but for an easier understanding
 * (and maybe change in the future) we did not join them
 */
static const gint8 khmerStateTable[][CC_COUNT] =
{
/* xx  c1  c2  c3 zwnj cs  rb  co  dv  sa  sp zwj  */
  { 1,  2,  2,  2,  1,  1,  1,  6,  1,  1,  1,  2}, /*  0 - ground state */
  {-1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1}, /*  1 - exit state (or sign to the right of the syllable) */
  {-1, -1, -1, -1,  3,  4,  5,  6, 16, 17,  1, -1}, /*  2 - Base consonant */
  {-1, -1, -1, -1, -1,  4, -1, -1, 16, -1, -1, -1}, /*  3 - First ZWNJ before a register shifter
							    It can only be followed by a shifter or a vowel */
  {-1, -1, -1, -1, 15, -1, -1,  6, 16, 17,  1, 14}, /*  4 - First register shifter */
  {-1, -1, -1, -1, -1, -1, -1, -1, 20, -1,  1, -1}, /*  5 - Robat */
  {-1,  7,  8,  9, -1, -1, -1, -1, -1, -1, -1, -1}, /*  6 - First Coeng */
  {-1, -1, -1, -1, 12, 13, -1, 10, 16, 17,  1, 14}, /*  7 - First consonant of type 1 after coeng */
  {-1, -1, -1, -1, 12, 13, -1, -1, 16, 17,  1, 14}, /*  8 - First consonant of type 2 after coeng */
  {-1, -1, -1, -1, 12, 13, -1, 10, 16, 17,  1, 14}, /*  9 - First consonant or type 3 after ceong */
  {-1, 11, 11, 11, -1, -1, -1, -1, -1, -1, -1, -1}, /* 10 - Second Coeng (no register shifter before) */
  {-1, -1, -1, -1, 15, -1, -1, -1, 16, 17,  1, 14}, /* 11 - Second coeng consonant (or ind. vowel) no register shifter before */
  {-1, -1, -1, -1, -1, 13, -1, -1, 16, -1, -1, -1}, /* 12 - Second ZWNJ before a register shifter */
  {-1, -1, -1, -1, 15, -1, -1, -1, 16, 17,  1, 14}, /* 13 - Second register shifter */
  {-1, -1, -1, -1, -1, -1, -1, -1, 16, -1, -1, -1}, /* 14 - ZWJ before vowel */
  {-1, -1, -1, -1, -1, -1, -1, -1, 16, -1, -1, -1}, /* 15 - ZWNJ before vowel */
  {-1, -1, -1, -1, -1, -1, -1, -1, -1, 17,  1, 18}, /* 16 - dependent vowel */
  {-1, -1, -1, -1, -1, -1, -1, -1, -1, -1,  1, 18}, /* 17 - sign above */
  {-1, -1, -1, -1, -1, -1, -1, 19, -1, -1, -1, -1}, /* 18 - ZWJ after vowel */
  {-1,  1, -1,  1, -1, -1, -1, -1, -1, -1, -1, -1}, /* 19 - Third coeng */
  {-1, -1, -1, -1, -1, -1, -1, -1, -1, -1,  1, -1}, /* 20 - dependent vowel after a Robat */
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


/* Below we define how a character in the input string is either in the khmerCharClasses table
 * (in which case we get its type back), a ZWJ or ZWNJ (two characters that may appear
 * within the syllable, but are not in the table) we also get their type back, or an unknown object
 * in which case we get _xx (CC_RESERVED) back
 */
static KhmerCharClass
get_char_class (gunichar ch)
{
  if (ch == C_SIGN_ZWJ)
    return CC_ZERO_WIDTH_J_MARK;

  if (ch == C_SIGN_ZWNJ)
    return CC_ZERO_WIDTH_NJ_MARK;

  if (ch < firstChar || ch > lastChar)
    return CC_RESERVED;

  return khmerCharClasses[ch - firstChar];
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
  KhmerCharClass charClass;

  while (cursor < char_count)
    {
      charClass = get_char_class (chars[cursor]) & CF_CLASS_MASK;
      state = khmerStateTable[state][charClass];

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
  {"pref", pref},
  {"blwf", blwf},
  {"abvf", abvf},
  {"pstf", pstf},
  {"pres", pres},
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
khmer_engine_shape (PangoEngineShape *engine,
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
  KhmerCharClass charClass;
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
      /* write a pre vowel or the pre part of a split vowel first
       * and look out for coeng + ro. RO is the only vowel of type 2, and
       * therefore the only one that requires saving space before the base.
       */
      glong coengRo = -1;  /* There is no Coeng Ro, if found this value will change */

      syllable = find_syllable (wcs, cursor, n_chars);

      for (i = cursor; i < syllable; i += 1)
	{
	  charClass = get_char_class (wcs[i]);

	  /* if a split vowel, write the pre part. In Khmer the pre part
	   * is the same for all split vowels, same glyph as pre vowel C_VOWEL_E
	   */
	  if (charClass & CF_SPLIT_VOWEL)
	    {
	      pango_ot_buffer_add_glyph (buffer, get_index (fc_font, C_VOWEL_E), pref_p, p - text);
	      break; /* there can be only one vowel */
	    }

	  /* if a vowel with pos before write it out */
	  if (charClass & CF_POS_BEFORE)
	    {
	      pango_ot_buffer_add_glyph (buffer, get_index (fc_font, wcs[i]), pref_p, p - text);
	      break; /* there can be only one vowel */
	    }

	  /* look for coeng + ro and remember position
	   * works because coeng + ro is always in front of a vowel (if there is a vowel)
	   * and because CC_CONSONANT2 is enough to identify it, as it is the only consonant
	   * with this flag
	   */
	  if ((charClass & CF_COENG) && (i + 1 < syllable) &&
	     ((get_char_class (wcs[i + 1]) & CF_CLASS_MASK) == CC_CONSONANT2))
	    {
	      coengRo = i;
	    }
	}

      /* write coeng + ro if found  */
      if (coengRo > -1)
	{
	  pango_ot_buffer_add_glyph (buffer, get_index (fc_font, C_COENG), pref_p, p - text);
	  pango_ot_buffer_add_glyph (buffer, get_index (fc_font, C_RO), pref_p, p - text);
	}

      /* shall we add a dotted circle?
      * If in the position in which the base should be (first char in the string) there is
      * a character that has the Dotted circle flag (a character that cannot be a base)
      * then write a dotted circle
      */
      if (get_char_class (wcs[cursor]) & CF_DOTTED_CIRCLE)
	{
	  pango_ot_buffer_add_glyph (buffer, get_index (fc_font, C_DOTTED_CIRCLE), default_p, p - text);
	}

      /* copy what is left to the output, skipping before vowels and
      * coeng Ro if they are present
      */
      for (i = cursor; i < syllable; i += 1)
	{
	  charClass = get_char_class (wcs[i]);

	  /* skip a before vowel, it was already processed */
	  if (charClass & CF_POS_BEFORE)
	    {
	      p = g_utf8_next_char (p);
	      continue;
	    }

	  /* skip coeng + ro, it was already processed */
	  if (i == coengRo)
	    {
	      p = g_utf8_next_char (p);
	      i += 1;
	      p = g_utf8_next_char (p);
	      continue;
	    }

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
		  /* assign the correct flags to a coeng consonant
		  * Consonants of type 3 are taged as Post forms and those type 1 as below forms
		  */
		if ((charClass & CF_COENG) && i + 1 < syllable)
		  {
		    if ((get_char_class (wcs[i + 1]) & CF_CLASS_MASK) == CC_CONSONANT3)
		      {
			pango_ot_buffer_add_glyph (buffer, get_index (fc_font, wcs[i]), pstf_p, p - text);
			p = g_utf8_next_char (p);
			i += 1;
			pango_ot_buffer_add_glyph (buffer, get_index (fc_font, wcs[i]), pstf_p, p - text);
			break;
		      }
		    else
		      {
			pango_ot_buffer_add_glyph (buffer, get_index (fc_font, wcs[i]), blwf_p, p - text);
			p = g_utf8_next_char (p);
			i += 1;
			pango_ot_buffer_add_glyph (buffer, get_index (fc_font, wcs[i]), blwf_p, p - text);
			break;
		      }
		  }

		  /* if a shifter is followed by an above vowel change the shifter to below form,
		  * an above vowel can have two possible positions i + 1 or i + 3
		  * (position i+1 corresponds to unicode 3, position i+3 to Unicode 4)
		  * and there is an extra rule for C_VOWEL_AA + C_SIGN_NIKAHIT also for two
		  * different positions, right after the shifter or after a vowel (Unicode 4)
		  */
		  if ((charClass & CF_SHIFTER) && (i + 1 < syllable))
		    {
		      if (get_char_class (wcs[i + 1]) & CF_ABOVE_VOWEL)
			{
			  pango_ot_buffer_add_glyph (buffer, get_index (fc_font, wcs[i]), blwf_p, p - text);
			  break;
			}
		      if (i + 2 < syllable &&
			  (wcs[i + 1] == C_VOWEL_AA) &&
			  (wcs[i + 2] == C_SIGN_NIKAHIT) )
			{
			  pango_ot_buffer_add_glyph (buffer, get_index (fc_font, wcs[i]), blwf_p, p - text);
			  break;
			}
		      if (i + 3 < syllable && (get_char_class (wcs[i + 3]) & CF_ABOVE_VOWEL) )
			{
			  pango_ot_buffer_add_glyph (buffer, get_index (fc_font, wcs[i]), blwf_p, p - text);
			  break;
			}
		      if (i + 4 < syllable &&
			  (wcs[i + 3] == C_VOWEL_AA) &&
			  (wcs[i + 4] == C_SIGN_NIKAHIT) )
			{
			  pango_ot_buffer_add_glyph (buffer, get_index (fc_font, wcs[i]), blwf_p, p - text);
			  break;
			}

		    }

		  /* default - any other characters  */
		  pango_ot_buffer_add_glyph (buffer, get_index (fc_font, wcs[i]), default_p, p - text);
		  break;
	    } /* switch */
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
khmer_engine_fc_class_init (PangoEngineShapeClass *class)
{
  class->script_shape = khmer_engine_shape;
}

PANGO_ENGINE_SHAPE_DEFINE_TYPE (KhmerEngineFc, khmer_engine_fc,
				khmer_engine_fc_class_init, NULL)


void
PANGO_MODULE_ENTRY(init) (GTypeModule *module)
{
  khmer_engine_fc_register_type (module);
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
    return g_object_new (khmer_engine_fc_type, NULL);
  else
    return NULL;
}
