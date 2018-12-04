/* Pango
 * break.c:
 *
 * Copyright (C) 1999 Red Hat Software
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

#include "pango-break.h"
#include "pango-engine-private.h"
#include "pango-script-private.h"
#include "pango-emoji-private.h"
#include "pango-impl-utils.h"
#include <string.h>

#define PARAGRAPH_SEPARATOR 0x2029
#define PARAGRAPH_SEPARATOR_STRING "\xE2\x80\xA9"

/* See http://www.unicode.org/unicode/reports/tr14/ if you hope
 * to understand the line breaking code.
 */

typedef enum
{
  BREAK_ALREADY_HANDLED,   /* didn't use the table */
  BREAK_PROHIBITED, /* no break, even if spaces intervene */
  BREAK_IF_SPACES,  /* "indirect break" (only if there are spaces) */
  BREAK_ALLOWED     /* "direct break" (can always break here) */
  /* TR 14 has two more break-opportunity classes,
   * "indirect break opportunity for combining marks following a space"
   * and "prohibited break for combining marks"
   * but we handle that inline in the code.
   */
} BreakOpportunity;

/* need to sync the break range to glib/gunicode.h . */
#define BREAK_TYPE_SAFE(btype)            \
	 ((btype) <= G_UNICODE_BREAK_ZERO_WIDTH_JOINER ? (btype) : G_UNICODE_BREAK_UNKNOWN)


/*
 * Hangul Conjoining Jamo handling.
 *
 * The way we implement it is just a bit different from TR14,
 * but produces the same results.
 * The same algorithm is also used in TR29 for cluster boundaries.
 *
 */


/* An enum that works as the states of the Hangul syllables system.
 **/
typedef enum
{
  JAMO_L,	/* G_UNICODE_BREAK_HANGUL_L_JAMO */
  JAMO_V,	/* G_UNICODE_BREAK_HANGUL_V_JAMO */
  JAMO_T,	/* G_UNICODE_BREAK_HANGUL_T_JAMO */
  JAMO_LV,	/* G_UNICODE_BREAK_HANGUL_LV_SYLLABLE */
  JAMO_LVT,	/* G_UNICODE_BREAK_HANGUL_LVT_SYLLABLE */
  NO_JAMO	/* Other */
} JamoType;

/* There are Hangul syllables encoded as characters, that act like a
 * sequence of Jamos. For each character we define a JamoType
 * that the character starts with, and one that it ends with.  This
 * decomposes JAMO_LV and JAMO_LVT to simple other JAMOs.  So for
 * example, a character with LineBreak type
 * G_UNICODE_BREAK_HANGUL_LV_SYLLABLE has start=JAMO_L and end=JAMO_V.
 */
typedef struct _CharJamoProps
{
  JamoType start, end;
} CharJamoProps;

/* Map from JamoType to CharJamoProps that hold only simple
 * JamoTypes (no LV or LVT) or none.
 */
static const CharJamoProps HangulJamoProps[] = {
  {JAMO_L, JAMO_L},	/* JAMO_L */
  {JAMO_V, JAMO_V},	/* JAMO_V */
  {JAMO_T, JAMO_T},	/* JAMO_T */
  {JAMO_L, JAMO_V},	/* JAMO_LV */
  {JAMO_L, JAMO_T},	/* JAMO_LVT */
  {NO_JAMO, NO_JAMO}	/* NO_JAMO */
};

/* A character forms a syllable with the previous character if and only if:
 * JamoType(this) is not NO_JAMO and:
 *
 * HangulJamoProps[JamoType(prev)].end and
 * HangulJamoProps[JamoType(this)].start are equal,
 * or the former is one less than the latter.
 */

#define IS_JAMO(btype)              \
	((btype >= G_UNICODE_BREAK_HANGUL_L_JAMO) && \
	 (btype <= G_UNICODE_BREAK_HANGUL_LVT_SYLLABLE))
#define JAMO_TYPE(btype)      \
	(IS_JAMO(btype) ? (btype - G_UNICODE_BREAK_HANGUL_L_JAMO) : NO_JAMO)

/* Types of Japanese characters */
#define JAPANESE(wc) ((wc) >= 0x2F00 && (wc) <= 0x30FF)
#define KANJI(wc)    ((wc) >= 0x2F00 && (wc) <= 0x2FDF)
#define HIRAGANA(wc) ((wc) >= 0x3040 && (wc) <= 0x309F)
#define KATAKANA(wc) ((wc) >= 0x30A0 && (wc) <= 0x30FF)

#define LATIN(wc) (((wc) >= 0x0020 && (wc) <= 0x02AF) || ((wc) >= 0x1E00 && (wc) <= 0x1EFF))
#define CYRILLIC(wc) (((wc) >= 0x0400 && (wc) <= 0x052F))
#define GREEK(wc) (((wc) >= 0x0370 && (wc) <= 0x3FF) || ((wc) >= 0x1F00 && (wc) <= 0x1FFF))
#define KANA(wc) ((wc) >= 0x3040 && (wc) <= 0x30FF)
#define HANGUL(wc) ((wc) >= 0xAC00 && (wc) <= 0xD7A3)
#define BACKSPACE_DELETES_CHARACTER(wc) (!LATIN (wc) && !CYRILLIC (wc) && !GREEK (wc) && !KANA(wc) && !HANGUL(wc))

/* Previously "123foo" was two words. But in UAX 29 of Unicode, 
 * we know don't break words between consecutive letters and numbers
 */
typedef enum
{
  WordNone,
  WordLetters,
  WordNumbers
} WordType;


/**
 * pango_default_break:
 * @text: text to break
 * @length: length of text in bytes (may be -1 if @text is nul-terminated)
 * @analysis: (nullable): a #PangoAnalysis for the @text
 * @attrs: logical attributes to fill in
 * @attrs_len: size of the array passed as @attrs
 *
 * This is the default break algorithm, used if no language
 * engine overrides it. Normally you should use pango_break()
 * instead. Unlike pango_break(),
 * @analysis can be %NULL, but only do that if you know what
 * you're doing. If you need an analysis to pass to pango_break(),
 * you need to pango_itemize().  In most cases however you should
 * simply use pango_get_log_attrs().
 **/
void
pango_default_break (const gchar   *text,
		     gint           length,
		     PangoAnalysis *analysis G_GNUC_UNUSED,
		     PangoLogAttr  *attrs,
		     int            attrs_len G_GNUC_UNUSED)
{
  /* The rationale for all this is in section 5.15 of the Unicode 3.0 book,
   * the line breaking stuff is also in TR14 on unicode.org
   */

  /* This is a default break implementation that should work for nearly all
   * languages. Language engines can override it optionally.
   */

  /* FIXME one cheesy optimization here would be to memset attrs to 0
   * before we start, and then never assign %FALSE to anything
   */

  const gchar *next;
  gint i;

  gunichar prev_wc;
  gunichar next_wc;

  JamoType prev_jamo;

  GUnicodeBreakType next_break_type;
  GUnicodeBreakType prev_break_type;
  GUnicodeBreakType prev_prev_break_type;

  /* See Grapheme_Cluster_Break Property Values table of UAX#29 */
  typedef enum
  {
    GB_Other,
    GB_ControlCRLF,
    GB_Extend,
    GB_ZWJ,
    GB_Prepend,
    GB_SpacingMark,
    GB_InHangulSyllable, /* Handles all of L, V, T, LV, LVT rules */
    /* Use state machine to handle emoji sequence */
    /* Rule GB12 and GB13 */
    GB_RI_Odd, /* Meets odd number of RI */
    GB_RI_Even, /* Meets even number of RI */
  } GraphemeBreakType;
  GraphemeBreakType prev_GB_type = GB_Other;
  gboolean met_Extended_Pictographic = FALSE;

  /* See Word_Break Property Values table of UAX#29 */
  typedef enum
  {
    WB_Other,
    WB_NewlineCRLF,
    WB_ExtendFormat,
    WB_Katakana,
    WB_Hebrew_Letter,
    WB_ALetter,
    WB_MidNumLet,
    WB_MidLetter,
    WB_MidNum,
    WB_Numeric,
    WB_ExtendNumLet,
    WB_RI_Odd,
    WB_RI_Even,
    WB_WSegSpace,
  } WordBreakType;
  WordBreakType prev_prev_WB_type = WB_Other, prev_WB_type = WB_Other;
  gint prev_WB_i = -1;

  /* See Sentence_Break Property Values table of UAX#29 */
  typedef enum
  {
    SB_Other,
    SB_ExtendFormat,
    SB_ParaSep,
    SB_Sp,
    SB_Lower,
    SB_Upper,
    SB_OLetter,
    SB_Numeric,
    SB_ATerm,
    SB_SContinue,
    SB_STerm,
    SB_Close,
    /* Rules SB8 and SB8a */
    SB_ATerm_Close_Sp,
    SB_STerm_Close_Sp,
  } SentenceBreakType;
  SentenceBreakType prev_prev_SB_type = SB_Other, prev_SB_type = SB_Other;
  gint prev_SB_i = -1;

  /* Rule LB25 with Example 7 of Customization */
  typedef enum
  {
    LB_Other,
    LB_Numeric,
    LB_Numeric_Close,
    LB_RI_Odd,
    LB_RI_Even,
  } LineBreakType;
  LineBreakType prev_LB_type = LB_Other;

  WordType current_word_type = WordNone;
  gunichar last_word_letter = 0;
  gunichar base_character = 0;

  gint last_sentence_start = -1;
  gint last_non_space = -1;

  gboolean almost_done = FALSE;
  gboolean done = FALSE;

  g_return_if_fail (length == 0 || text != NULL);
  g_return_if_fail (attrs != NULL);

  next = text;

  prev_break_type = G_UNICODE_BREAK_UNKNOWN;
  prev_prev_break_type = G_UNICODE_BREAK_UNKNOWN;
  prev_wc = 0;
  prev_jamo = NO_JAMO;

  if (length == 0 || *text == '\0')
    {
      next_wc = PARAGRAPH_SEPARATOR;
      almost_done = TRUE;
    }
  else
    next_wc = g_utf8_get_char (next);

  next_break_type = g_unichar_break_type (next_wc);
  next_break_type = BREAK_TYPE_SAFE (next_break_type);

  for (i = 0; !done ; i++)
    {
      GUnicodeType type;
      gunichar wc;
      GUnicodeBreakType break_type;
      GUnicodeBreakType row_break_type;
      BreakOpportunity break_op;
      JamoType jamo;
      gboolean makes_hangul_syllable;

      /* UAX#29 boundaries */
      gboolean is_grapheme_boundary;
      gboolean is_word_boundary;
      gboolean is_sentence_boundary;

      /* Emoji extended pictographics */
      gboolean is_Extended_Pictographic;


      wc = next_wc;
      break_type = next_break_type;

      if (almost_done)
	{
	  /*
	   * If we have already reached the end of @text g_utf8_next_char()
	   * may not increment next
	   */
	  next_wc = 0;
	  next_break_type = G_UNICODE_BREAK_UNKNOWN;
	  done = TRUE;
	}
      else
	{
	  next = g_utf8_next_char (next);

	  if ((length >= 0 && next >= text + length) || *next == '\0')
	    {
	      /* This is how we fill in the last element (end position) of the
	       * attr array - assume there's a paragraph separators off the end
	       * of @text.
	       */
	      next_wc = PARAGRAPH_SEPARATOR;
	      almost_done = TRUE;
	    }
	  else
	    next_wc = g_utf8_get_char (next);

	  next_break_type = g_unichar_break_type (next_wc);
	  next_break_type = BREAK_TYPE_SAFE (next_break_type);
	}

      type = g_unichar_type (wc);
      jamo = JAMO_TYPE (break_type);

      /* Determine wheter this forms a Hangul syllable with prev. */
      if (jamo == NO_JAMO)
	makes_hangul_syllable = FALSE;
      else
	{
	  JamoType prev_end   = HangulJamoProps[prev_jamo].end  ;
	  JamoType this_start = HangulJamoProps[     jamo].start;

	  /* See comments before IS_JAMO */
	  makes_hangul_syllable = (prev_end == this_start) || (prev_end + 1 == this_start);
	}

      switch (type)
        {
        case G_UNICODE_SPACE_SEPARATOR:
        case G_UNICODE_LINE_SEPARATOR:
        case G_UNICODE_PARAGRAPH_SEPARATOR:
          attrs[i].is_white = TRUE;
          break;
        default:
          if (wc == '\t' || wc == '\n' || wc == '\r' || wc == '\f')
            attrs[i].is_white = TRUE;
          else
            attrs[i].is_white = FALSE;
          break;
        }

      /* Just few spaces have variable width. So explicitly mark them.
       */
      attrs[i].is_expandable_space = (0x0020 == wc || 0x00A0 == wc);
      is_Extended_Pictographic =
	_pango_Is_Emoji_Extended_Pictographic (wc);


      /* ---- UAX#29 Grapheme Boundaries ---- */
      {
	GraphemeBreakType GB_type;

        /* Find the GraphemeBreakType of wc */
	GB_type = GB_Other;
	switch ((int) type)
	  {
	  case G_UNICODE_FORMAT:
	    if (G_UNLIKELY (wc == 0x200C))
	      {
		GB_type = GB_Extend;
		break;
	      }
	    if (G_UNLIKELY (wc == 0x200D))
	      {
		GB_type = GB_ZWJ;
		break;
	      }
            if (G_UNLIKELY((wc >= 0x600 && wc <= 0x605) ||
                            wc == 0x6DD ||
                            wc == 0x70F ||
                            wc == 0x8E2 ||
                            wc == 0xD4E ||
                            wc == 0x110BD ||
                            (wc >= 0x111C2 && wc <= 0x111C3)))
              {
                GB_type = GB_Prepend;
                break;
              }
	    /* fall through */
	  case G_UNICODE_CONTROL:
	  case G_UNICODE_LINE_SEPARATOR:
	  case G_UNICODE_PARAGRAPH_SEPARATOR:
	  case G_UNICODE_SURROGATE:
	    GB_type = GB_ControlCRLF;
	    break;

	  case G_UNICODE_UNASSIGNED:
	    /* Unassigned default ignorables */
	    if ((wc >= 0xFFF0 && wc <= 0xFFF8) ||
		(wc >= 0xE0000 && wc <= 0xE0FFF))
	      {
		GB_type = GB_ControlCRLF;
		break;
	      }

	  case G_UNICODE_OTHER_LETTER:
	    if (makes_hangul_syllable)
	      GB_type = GB_InHangulSyllable;
	    break;

	  case G_UNICODE_MODIFIER_LETTER:
	    if (wc >= 0xFF9E && wc <= 0xFF9F)
	      GB_type = GB_Extend; /* Other_Grapheme_Extend */
	    break;

	  case G_UNICODE_SPACING_MARK:
	    GB_type = GB_SpacingMark; /* SpacingMark */
	    if (wc >= 0x0900)
	      {
		if (wc == 0x09BE || wc == 0x09D7 ||
		    wc == 0x0B3E || wc == 0x0B57 || wc == 0x0BBE || wc == 0x0BD7 ||
		    wc == 0x0CC2 || wc == 0x0CD5 || wc == 0x0CD6 ||
		    wc == 0x0D3E || wc == 0x0D57 || wc == 0x0DCF || wc == 0x0DDF ||
		    wc == 0x1D165 || (wc >= 0x1D16E && wc <= 0x1D172))
		  GB_type = GB_Extend; /* Other_Grapheme_Extend */
	      }
	    break;

	  case G_UNICODE_ENCLOSING_MARK:
	  case G_UNICODE_NON_SPACING_MARK:
	    GB_type = GB_Extend; /* Grapheme_Extend */
	    break;

          case G_UNICODE_OTHER_SYMBOL:
            if (G_UNLIKELY(wc >=0x1F1E6 && wc <=0x1F1FF))
              {
                if (prev_GB_type == GB_RI_Odd)
                  GB_type = GB_RI_Even;
                else if (prev_GB_type == GB_RI_Even)
                  GB_type = GB_RI_Odd;
                else
                  GB_type = GB_RI_Odd;
                break;
              }
            break;

          case G_UNICODE_MODIFIER_SYMBOL:
            if (wc >= 0x1F3FB && wc <= 0x1F3FF)
              GB_type = GB_Extend;
            break;
	  }

	/* Rule GB11 */
	if (met_Extended_Pictographic)
	  {
	    if (GB_type == GB_Extend)
	      met_Extended_Pictographic = TRUE;
	    else if (_pango_Is_Emoji_Extended_Pictographic (prev_wc) &&
		     GB_type == GB_ZWJ)
	      met_Extended_Pictographic = TRUE;
	    else if (prev_GB_type == GB_Extend && GB_type == GB_ZWJ)
	      met_Extended_Pictographic = TRUE;
	    else if (prev_GB_type == GB_ZWJ && is_Extended_Pictographic)
	      met_Extended_Pictographic = TRUE;
	    else
	      met_Extended_Pictographic = FALSE;
	  }

	/* Grapheme Cluster Boundary Rules */
	is_grapheme_boundary = TRUE; /* Rule GB999 */

	/* We apply Rules GB1 and GB2 at the end of the function */
	if (wc == '\n' && prev_wc == '\r')
          is_grapheme_boundary = FALSE; /* Rule GB3 */
	else if (prev_GB_type == GB_ControlCRLF || GB_type == GB_ControlCRLF)
	  is_grapheme_boundary = TRUE; /* Rules GB4 and GB5 */
	else if (GB_type == GB_InHangulSyllable)
	  is_grapheme_boundary = FALSE; /* Rules GB6, GB7, GB8 */
	else if (GB_type == GB_Extend)
          {
	    is_grapheme_boundary = FALSE; /* Rule GB9 */
          }
        else if (GB_type == GB_ZWJ)
	  is_grapheme_boundary = FALSE; /* Rule GB9 */
	else if (GB_type == GB_SpacingMark)
	  is_grapheme_boundary = FALSE; /* Rule GB9a */
	else if (prev_GB_type == GB_Prepend)
	  is_grapheme_boundary = FALSE; /* Rule GB9b */
	else if (is_Extended_Pictographic)
	  { /* Rule GB11 */
	    if (prev_GB_type == GB_ZWJ && met_Extended_Pictographic)
	      is_grapheme_boundary = FALSE;
	  }
	else if (prev_GB_type == GB_RI_Odd && GB_type == GB_RI_Even)
	  is_grapheme_boundary = FALSE; /* Rule GB12 and GB13 */

	if (is_Extended_Pictographic)
	  met_Extended_Pictographic = TRUE;

	attrs[i].is_cursor_position = is_grapheme_boundary;
	/* If this is a grapheme boundary, we have to decide if backspace
	 * deletes a character or the whole grapheme cluster */
	if (is_grapheme_boundary)
          {
	    attrs[i].backspace_deletes_character = BACKSPACE_DELETES_CHARACTER (base_character);
          }
	else
	  attrs[i].backspace_deletes_character = FALSE;

	prev_GB_type = GB_type;
      }

      /* ---- UAX#29 Word Boundaries ---- */
      {
	is_word_boundary = FALSE;
	if (is_grapheme_boundary ||
	    G_UNLIKELY(wc >=0x1F1E6 && wc <=0x1F1FF)) /* Rules WB3 and WB4 */
	  {
	    PangoScript script;
	    WordBreakType WB_type;

	    script = g_unichar_get_script (wc);

	    /* Find the WordBreakType of wc */
	    WB_type = WB_Other;

	    if (script == PANGO_SCRIPT_KATAKANA)
	      WB_type = WB_Katakana;

	    if (script == PANGO_SCRIPT_HEBREW && type == G_UNICODE_OTHER_LETTER)
	      WB_type = WB_Hebrew_Letter;

	    if (WB_type == WB_Other)
	      switch (wc >> 8)
	        {
		case 0x30:
		  if (wc == 0x3031 || wc == 0x3032 || wc == 0x3033 || wc == 0x3034 || wc == 0x3035 ||
		      wc == 0x309b || wc == 0x309c || wc == 0x30a0 || wc == 0x30fc)
		    WB_type = WB_Katakana; /* Katakana exceptions */
		  break;
		case 0xFF:
		  if (wc == 0xFF70)
		    WB_type = WB_Katakana; /* Katakana exceptions */
		  else if (wc >= 0xFF9E && wc <= 0xFF9F)
		    WB_type = WB_ExtendFormat; /* Other_Grapheme_Extend */
		  break;
		case 0x05:
		  if (wc == 0x05F3)
		    WB_type = WB_ALetter; /* ALetter exceptions */
		  break;
		}

	    if (WB_type == WB_Other)
	      switch ((int) break_type)
	        {
		case G_UNICODE_BREAK_NUMERIC:
		  if (wc != 0x066C)
		    WB_type = WB_Numeric; /* Numeric */
		  break;
		case G_UNICODE_BREAK_INFIX_SEPARATOR:
		  if (wc != 0x003A && wc != 0xFE13 && wc != 0x002E)
		    WB_type = WB_MidNum; /* MidNum */
		  break;
		}

	    if (WB_type == WB_Other)
	      switch ((int) type)
		{
		case G_UNICODE_CONTROL:
		  if (wc != 0x000D && wc != 0x000A && wc != 0x000B && wc != 0x000C && wc != 0x0085)
		    break;
		  /* fall through */
		case G_UNICODE_LINE_SEPARATOR:
		case G_UNICODE_PARAGRAPH_SEPARATOR:
		  WB_type = WB_NewlineCRLF; /* CR, LF, Newline */
		  break;

		case G_UNICODE_FORMAT:
		case G_UNICODE_SPACING_MARK:
		case G_UNICODE_ENCLOSING_MARK:
		case G_UNICODE_NON_SPACING_MARK:
		  WB_type = WB_ExtendFormat; /* Extend, Format */
		  break;

		case G_UNICODE_CONNECT_PUNCTUATION:
		  WB_type = WB_ExtendNumLet; /* ExtendNumLet */
		  break;

		case G_UNICODE_INITIAL_PUNCTUATION:
		case G_UNICODE_FINAL_PUNCTUATION:
		  if (wc == 0x2018 || wc == 0x2019)
		    WB_type = WB_MidNumLet; /* MidNumLet */
		  break;
		case G_UNICODE_OTHER_PUNCTUATION:
		  if (wc == 0x0027 || wc == 0x002e || wc == 0x2024 ||
		      wc == 0xfe52 || wc == 0xff07 || wc == 0xff0e)
		    WB_type = WB_MidNumLet; /* MidNumLet */
		  else if (wc == 0x00b7 || wc == 0x05f4 || wc == 0x2027 || wc == 0x003a || wc == 0x0387 ||
			   wc == 0xfe13 || wc == 0xfe55 || wc == 0xff1a)
		    WB_type = WB_MidLetter; /* WB_MidLetter */
		  else if (wc == 0x066c ||
			   wc == 0xfe50 || wc == 0xfe54 || wc == 0xff0c || wc == 0xff1b)
		    WB_type = WB_MidNum; /* MidNum */
		  break;

		case G_UNICODE_OTHER_SYMBOL:
		  if (wc >= 0x24B6 && wc <= 0x24E9) /* Other_Alphabetic */
		    goto Alphabetic;

		  if (G_UNLIKELY(wc >=0x1F1E6 && wc <=0x1F1FF))
		    {
			  if (prev_WB_type == WB_RI_Odd)
			   WB_type = WB_RI_Even;
			  else if (prev_WB_type == WB_RI_Even)
			   WB_type = WB_RI_Odd;
			  else
			   WB_type = WB_RI_Odd;
		    }

		  break;

		case G_UNICODE_OTHER_LETTER:
		case G_UNICODE_LETTER_NUMBER:
		  if (wc == 0x3006 || wc == 0x3007 ||
		      (wc >= 0x3021 && wc <= 0x3029) ||
		      (wc >= 0x3038 && wc <= 0x303A) ||
		      (wc >= 0x3400 && wc <= 0x4DB5) ||
		      (wc >= 0x4E00 && wc <= 0x9FC3) ||
		      (wc >= 0xF900 && wc <= 0xFA2D) ||
		      (wc >= 0xFA30 && wc <= 0xFA6A) ||
		      (wc >= 0xFA70 && wc <= 0xFAD9) ||
		      (wc >= 0x20000 && wc <= 0x2A6D6) ||
		      (wc >= 0x2F800 && wc <= 0x2FA1D))
		    break; /* ALetter exceptions: Ideographic */
		  goto Alphabetic;

		case G_UNICODE_LOWERCASE_LETTER:
		case G_UNICODE_MODIFIER_LETTER:
		case G_UNICODE_TITLECASE_LETTER:
		case G_UNICODE_UPPERCASE_LETTER:
		Alphabetic:
		  if (break_type != G_UNICODE_BREAK_COMPLEX_CONTEXT && script != PANGO_SCRIPT_HIRAGANA)
		    WB_type = WB_ALetter; /* ALetter */
		  break;
		}

	    if (WB_type == WB_Other)
	      {
		if (type == G_UNICODE_SPACE_SEPARATOR &&
		    break_type != G_UNICODE_BREAK_NON_BREAKING_GLUE)
		  WB_type = WB_WSegSpace;
	      }

	    /* Word Cluster Boundary Rules */

	    /* We apply Rules WB1 and WB2 at the end of the function */

	    if (prev_wc == 0x3031 && wc == 0x41)
	      g_debug ("Y %d %d", prev_WB_type, WB_type);
	    if (prev_WB_type == WB_NewlineCRLF && prev_WB_i + 1 == i)
	      {
	        /* The extra check for prev_WB_i is to correctly handle sequences like
		 * Newline ÷ Extend × Extend
		 * since we have not skipped ExtendFormat yet.
		 */
	        is_word_boundary = TRUE; /* Rule WB3a */
	      }
	    else if (WB_type == WB_NewlineCRLF)
	      is_word_boundary = TRUE; /* Rule WB3b */
	    else if (prev_wc == 0x200D && is_Extended_Pictographic)
	      is_word_boundary = FALSE; /* Rule WB3c */
	    else if (prev_WB_type == WB_WSegSpace &&
		     WB_type == WB_WSegSpace && prev_WB_i + 1 == i)
	      is_word_boundary = FALSE; /* Rule WB3d */
	    else if (WB_type == WB_ExtendFormat)
	      is_word_boundary = FALSE; /* Rules WB4? */
	    else if ((prev_WB_type == WB_ALetter  ||
                  prev_WB_type == WB_Hebrew_Letter ||
                  prev_WB_type == WB_Numeric) &&
                 (WB_type == WB_ALetter  ||
                  WB_type == WB_Hebrew_Letter ||
                  WB_type == WB_Numeric))
	      is_word_boundary = FALSE; /* Rules WB5, WB8, WB9, WB10 */
	    else if (prev_WB_type == WB_Katakana && WB_type == WB_Katakana)
	      is_word_boundary = FALSE; /* Rule WB13 */
	    else if ((prev_WB_type == WB_ALetter ||
                  prev_WB_type == WB_Hebrew_Letter ||
                  prev_WB_type == WB_Numeric ||
                  prev_WB_type == WB_Katakana ||
                  prev_WB_type == WB_ExtendNumLet) &&
                 WB_type == WB_ExtendNumLet)
	      is_word_boundary = FALSE; /* Rule WB13a */
	    else if (prev_WB_type == WB_ExtendNumLet &&
                 (WB_type == WB_ALetter ||
                  WB_type == WB_Hebrew_Letter ||
                  WB_type == WB_Numeric ||
                  WB_type == WB_Katakana))
	      is_word_boundary = FALSE; /* Rule WB13b */
	    else if (((prev_prev_WB_type == WB_ALetter ||
                   prev_prev_WB_type == WB_Hebrew_Letter) &&
                  (WB_type == WB_ALetter ||
                   WB_type == WB_Hebrew_Letter)) &&
		     (prev_WB_type == WB_MidLetter ||
              prev_WB_type == WB_MidNumLet ||
              prev_wc == 0x0027))
	      {
		attrs[prev_WB_i].is_word_boundary = FALSE; /* Rule WB6 */
		is_word_boundary = FALSE; /* Rule WB7 */
	      }
	    else if (prev_WB_type == WB_Hebrew_Letter && wc == 0x0027)
          is_word_boundary = FALSE; /* Rule WB7a */
	    else if (prev_prev_WB_type == WB_Hebrew_Letter && prev_wc == 0x0022 &&
                 WB_type == WB_Hebrew_Letter) {
          attrs[prev_WB_i].is_word_boundary = FALSE; /* Rule WB7b */
          is_word_boundary = FALSE; /* Rule WB7c */
        }
	    else if ((prev_prev_WB_type == WB_Numeric && WB_type == WB_Numeric) &&
                 (prev_WB_type == WB_MidNum || prev_WB_type == WB_MidNumLet ||
                  prev_wc == 0x0027))
	      {
		is_word_boundary = FALSE; /* Rule WB11 */
		attrs[prev_WB_i].is_word_boundary = FALSE; /* Rule WB12 */
	      }
	    else if (prev_WB_type == WB_RI_Odd && WB_type == WB_RI_Even)
	      is_word_boundary = FALSE; /* Rule WB15 and WB16 */
	    else
	      is_word_boundary = TRUE; /* Rule WB999 */

	    if (WB_type != WB_ExtendFormat)
	      {
		prev_prev_WB_type = prev_WB_type;
		prev_WB_type = WB_type;
		prev_WB_i = i;
	      }
	  }

	attrs[i].is_word_boundary = is_word_boundary;
      }

      /* ---- UAX#29 Sentence Boundaries ---- */
      {
	is_sentence_boundary = FALSE;
	if (is_word_boundary ||
	    wc == '\r' || wc == '\n') /* Rules SB3 and SB5 */
	  {
	    SentenceBreakType SB_type;

	    /* Find the SentenceBreakType of wc */
	    SB_type = SB_Other;

	    if (break_type == G_UNICODE_BREAK_NUMERIC)
	      SB_type = SB_Numeric; /* Numeric */

	    if (SB_type == SB_Other)
	      switch ((int) type)
		{
		case G_UNICODE_CONTROL:
		  if (wc == '\r' || wc == '\n')
		    SB_type = SB_ParaSep;
		  else if (wc == 0x0009 || wc == 0x000B || wc == 0x000C)
		    SB_type = SB_Sp;
		  else if (wc == 0x0085)
		    SB_type = SB_ParaSep;
		  break;

		case G_UNICODE_SPACE_SEPARATOR:
		  if (wc == 0x0020 || wc == 0x00A0 || wc == 0x1680 ||
		      (wc >= 0x2000 && wc <= 0x200A) ||
		      wc == 0x202F || wc == 0x205F || wc == 0x3000)
		    SB_type = SB_Sp;
		  break;

		case G_UNICODE_LINE_SEPARATOR:
		case G_UNICODE_PARAGRAPH_SEPARATOR:
		  SB_type = SB_ParaSep;
		  break;

		case G_UNICODE_FORMAT:
		case G_UNICODE_SPACING_MARK:
		case G_UNICODE_ENCLOSING_MARK:
		case G_UNICODE_NON_SPACING_MARK:
		  SB_type = SB_ExtendFormat; /* Extend, Format */
		  break;

		case G_UNICODE_MODIFIER_LETTER:
		  if (wc >= 0xFF9E && wc <= 0xFF9F)
		    SB_type = SB_ExtendFormat; /* Other_Grapheme_Extend */
		  break;

		case G_UNICODE_TITLECASE_LETTER:
		  SB_type = SB_Upper;
		  break;

		case G_UNICODE_DASH_PUNCTUATION:
		  if (wc == 0x002D ||
		      (wc >= 0x2013 && wc <= 0x2014) ||
		      (wc >= 0xFE31 && wc <= 0xFE32) ||
		      wc == 0xFE58 ||
		      wc == 0xFE63 ||
		      wc == 0xFF0D)
		    SB_type = SB_SContinue;
		  break;

		case G_UNICODE_OTHER_PUNCTUATION:
		  if (wc == 0x05F3)
		    SB_type = SB_OLetter;
		  else if (wc == 0x002E || wc == 0x2024 ||
		      wc == 0xFE52 || wc == 0xFF0E)
		    SB_type = SB_ATerm;

		  if (wc == 0x002C ||
		      wc == 0x003A ||
		      wc == 0x055D ||
		      (wc >= 0x060C && wc <= 0x060D) ||
		      wc == 0x07F8 ||
		      wc == 0x1802 ||
		      wc == 0x1808 ||
		      wc == 0x3001 ||
		      (wc >= 0xFE10 && wc <= 0xFE11) ||
		      wc == 0xFE13 ||
		      (wc >= 0xFE50 && wc <= 0xFE51) ||
		      wc == 0xFE55 ||
		      wc == 0xFF0C ||
		      wc == 0xFF1A ||
		      wc == 0xFF64)
		    SB_type = SB_SContinue;

		  if (wc == 0x0021 ||
		      wc == 0x003F ||
		      wc == 0x0589 ||
		      wc == 0x061F ||
		      wc == 0x06D4 ||
		      (wc >= 0x0700 && wc <= 0x0702) ||
		      wc == 0x07F9 ||
		      (wc >= 0x0964 && wc <= 0x0965) ||
		      (wc >= 0x104A && wc <= 0x104B) ||
		      wc == 0x1362 ||
		      (wc >= 0x1367 && wc <= 0x1368) ||
		      wc == 0x166E ||
		      (wc >= 0x1735 && wc <= 0x1736) ||
		      wc == 0x1803 ||
		      wc == 0x1809 ||
		      (wc >= 0x1944 && wc <= 0x1945) ||
		      (wc >= 0x1AA8 && wc <= 0x1AAB) ||
		      (wc >= 0x1B5A && wc <= 0x1B5B) ||
		      (wc >= 0x1B5E && wc <= 0x1B5F) ||
		      (wc >= 0x1C3B && wc <= 0x1C3C) ||
		      (wc >= 0x1C7E && wc <= 0x1C7F) ||
		      (wc >= 0x203C && wc <= 0x203D) ||
		      (wc >= 0x2047 && wc <= 0x2049) ||
		      wc == 0x2E2E ||
		      wc == 0x2E3C ||
		      wc == 0x3002 ||
		      wc == 0xA4FF ||
		      (wc >= 0xA60E && wc <= 0xA60F) ||
		      wc == 0xA6F3 ||
		      wc == 0xA6F7 ||
		      (wc >= 0xA876 && wc <= 0xA877) ||
		      (wc >= 0xA8CE && wc <= 0xA8CF) ||
		      wc == 0xA92F ||
		      (wc >= 0xA9C8 && wc <= 0xA9C9) ||
		      (wc >= 0xAA5D && wc <= 0xAA5F) ||
		      (wc >= 0xAAF0 && wc <= 0xAAF1) ||
		      wc == 0xABEB ||
		      (wc >= 0xFE56 && wc <= 0xFE57) ||
		      wc == 0xFF01 ||
		      wc == 0xFF1F ||
		      wc == 0xFF61 ||
		      (wc >= 0x10A56 && wc <= 0x10A57) ||
		      (wc >= 0x11047 && wc <= 0x11048) ||
		      (wc >= 0x110BE && wc <= 0x110C1) ||
		      (wc >= 0x11141 && wc <= 0x11143) ||
		      (wc >= 0x111C5 && wc <= 0x111C6) ||
		      wc == 0x111CD ||
		      (wc >= 0x111DE && wc <= 0x111DF) ||
		      (wc >= 0x11238 && wc <= 0x11239) ||
		      (wc >= 0x1123B && wc <= 0x1123C) ||
		      wc == 0x112A9 ||
		      (wc >= 0x1144B && wc <= 0x1144C) ||
		      (wc >= 0x115C2 && wc <= 0x115C3) ||
		      (wc >= 0x115C9 && wc <= 0x115D7) ||
		      (wc >= 0x11641 && wc <= 0x11642) ||
		      (wc >= 0x1173C && wc <= 0x1173E) ||
		      (wc >= 0x11C41 && wc <= 0x11C42) ||
		      (wc >= 0x16A6E && wc <= 0x16A6F) ||
		      wc == 0x16AF5 ||
		      (wc >= 0x16B37 && wc <= 0x16B38) ||
		      wc == 0x16B44 ||
		      wc == 0x1BC9F ||
		      wc == 0x1DA88)
		    SB_type = SB_STerm;

		  break;
		}

	    if (SB_type == SB_Other)
	      {
		if (g_unichar_islower(wc))
		  SB_type = SB_Lower;
		else if (g_unichar_isupper(wc))
		  SB_type = SB_Upper;
		else if (g_unichar_isalpha(wc))
		  SB_type = SB_OLetter;

		if (type == G_UNICODE_OPEN_PUNCTUATION ||
		    type == G_UNICODE_CLOSE_PUNCTUATION ||
		    break_type == G_UNICODE_BREAK_QUOTATION)
		  SB_type = SB_Close;
	      }

	    /* Sentence Boundary Rules */

	    /* We apply Rules SB1 and SB2 at the end of the function */

#define IS_OTHER_TERM(SB_type)						\
	    /* not in (OLetter | Upper | Lower | ParaSep | SATerm) */	\
	      !(SB_type == SB_OLetter ||				\
		SB_type == SB_Upper || SB_type == SB_Lower ||		\
		SB_type == SB_ParaSep ||				\
		SB_type == SB_ATerm || SB_type == SB_STerm ||		\
		SB_type == SB_ATerm_Close_Sp ||				\
		SB_type == SB_STerm_Close_Sp)


	    if (wc == '\n' && prev_wc == '\r')
	      is_sentence_boundary = FALSE; /* Rule SB3 */
	    else if (prev_SB_type == SB_ParaSep && prev_SB_i + 1 == i)
	      {
		/* The extra check for prev_SB_i is to correctly handle sequences like
		 * ParaSep ÷ Extend × Extend
		 * since we have not skipped ExtendFormat yet.
		 */

		is_sentence_boundary = TRUE; /* Rule SB4 */
	      }
	    else if (SB_type == SB_ExtendFormat)
	      is_sentence_boundary = FALSE; /* Rule SB5? */
	    else if (prev_SB_type == SB_ATerm && SB_type == SB_Numeric)
	      is_sentence_boundary = FALSE; /* Rule SB6 */
	    else if ((prev_prev_SB_type == SB_Upper ||
		      prev_prev_SB_type == SB_Lower) &&
		     prev_SB_type == SB_ATerm &&
		     SB_type == SB_Upper)
	      is_sentence_boundary = FALSE; /* Rule SB7 */
	    else if (prev_SB_type == SB_ATerm && SB_type == SB_Close)
		SB_type = SB_ATerm;
	    else if (prev_SB_type == SB_STerm && SB_type == SB_Close)
	      SB_type = SB_STerm;
	    else if (prev_SB_type == SB_ATerm && SB_type == SB_Sp)
	      SB_type = SB_ATerm_Close_Sp;
	    else if (prev_SB_type == SB_STerm && SB_type == SB_Sp)
	      SB_type = SB_STerm_Close_Sp;
	    /* Rule SB8 */
	    else if ((prev_SB_type == SB_ATerm ||
		      prev_SB_type == SB_ATerm_Close_Sp) &&
		     SB_type == SB_Lower)
	      is_sentence_boundary = FALSE;
	    else if ((prev_prev_SB_type == SB_ATerm ||
		      prev_prev_SB_type == SB_ATerm_Close_Sp) &&
		     IS_OTHER_TERM(prev_SB_type) &&
		     SB_type == SB_Lower)
	      attrs[prev_SB_i].is_sentence_boundary = FALSE;
	    else if ((prev_SB_type == SB_ATerm ||
		      prev_SB_type == SB_ATerm_Close_Sp ||
		      prev_SB_type == SB_STerm ||
		      prev_SB_type == SB_STerm_Close_Sp) &&
		     (SB_type == SB_SContinue ||
		      SB_type == SB_ATerm || SB_type == SB_STerm))
	      is_sentence_boundary = FALSE; /* Rule SB8a */
	    else if ((prev_SB_type == SB_ATerm ||
		      prev_SB_type == SB_STerm) &&
		     (SB_type == SB_Close || SB_type == SB_Sp ||
		      SB_type == SB_ParaSep))
	      is_sentence_boundary = FALSE; /* Rule SB9 */
	    else if ((prev_SB_type == SB_ATerm ||
		      prev_SB_type == SB_ATerm_Close_Sp ||
		      prev_SB_type == SB_STerm ||
		      prev_SB_type == SB_STerm_Close_Sp) &&
		     (SB_type == SB_Sp || SB_type == SB_ParaSep))
	      is_sentence_boundary = FALSE; /* Rule SB10 */
	    else if ((prev_SB_type == SB_ATerm ||
		      prev_SB_type == SB_ATerm_Close_Sp ||
		      prev_SB_type == SB_STerm ||
		      prev_SB_type == SB_STerm_Close_Sp) &&
		     SB_type != SB_ParaSep)
	      is_sentence_boundary = TRUE; /* Rule SB11 */
	    else
	      is_sentence_boundary = FALSE; /* Rule SB998 */

	    if (SB_type != SB_ExtendFormat &&
		!((prev_prev_SB_type == SB_ATerm ||
		   prev_prev_SB_type == SB_ATerm_Close_Sp) &&
		  IS_OTHER_TERM(prev_SB_type) &&
		  IS_OTHER_TERM(SB_type)))
              {
                prev_prev_SB_type = prev_SB_type;
                prev_SB_type = SB_type;
                prev_SB_i = i;
              }

#undef IS_OTHER_TERM

	  }

	if (i == 0 || done)
	  is_sentence_boundary = TRUE; /* Rules SB1 and SB2 */

	attrs[i].is_sentence_boundary = is_sentence_boundary;
      }

      /* ---- Line breaking ---- */

      break_op = BREAK_ALREADY_HANDLED;

      row_break_type = prev_break_type == G_UNICODE_BREAK_SPACE ?
	prev_prev_break_type : prev_break_type;
      g_assert (row_break_type != G_UNICODE_BREAK_SPACE);

      attrs[i].is_char_break = FALSE;
      attrs[i].is_line_break = FALSE;
      attrs[i].is_mandatory_break = FALSE;

      /* Rule LB1:
	 assign a line breaking class to each code point of the input. */
      switch (break_type)
	{
	case G_UNICODE_BREAK_AMBIGUOUS:
	case G_UNICODE_BREAK_SURROGATE:
	case G_UNICODE_BREAK_UNKNOWN:
	  break_type = G_UNICODE_BREAK_ALPHABETIC;
	  break;

	case G_UNICODE_BREAK_COMPLEX_CONTEXT:
	  if (type == G_UNICODE_NON_SPACING_MARK ||
	      type == G_UNICODE_SPACING_MARK)
	    break_type = G_UNICODE_BREAK_COMBINING_MARK;
	  else
	    break_type = G_UNICODE_BREAK_ALPHABETIC;
	  break;

	case G_UNICODE_BREAK_CONDITIONAL_JAPANESE_STARTER:
	  break_type = G_UNICODE_BREAK_NON_STARTER;
	  break;

	default:
	  ;
	}

      /* If it's not a grapheme boundary, it's not a line break either */
      if (attrs[i].is_cursor_position ||
	  break_type == G_UNICODE_BREAK_COMBINING_MARK ||
	  break_type == G_UNICODE_BREAK_ZERO_WIDTH_JOINER ||
	  break_type == G_UNICODE_BREAK_HANGUL_L_JAMO ||
	  break_type == G_UNICODE_BREAK_HANGUL_V_JAMO ||
	  break_type == G_UNICODE_BREAK_HANGUL_T_JAMO ||
	  break_type == G_UNICODE_BREAK_HANGUL_LV_SYLLABLE ||
	  break_type == G_UNICODE_BREAK_HANGUL_LVT_SYLLABLE ||
	  break_type == G_UNICODE_BREAK_EMOJI_MODIFIER ||
	  break_type == G_UNICODE_BREAK_REGIONAL_INDICATOR)
	{
	  LineBreakType LB_type;

	  /* Find the LineBreakType of wc */
	  LB_type = LB_Other;

	  if (break_type == G_UNICODE_BREAK_NUMERIC)
	    LB_type = LB_Numeric;

	  if (break_type == G_UNICODE_BREAK_SYMBOL ||
	      break_type == G_UNICODE_BREAK_INFIX_SEPARATOR)
	    {
	      if (!(prev_LB_type == LB_Numeric))
		LB_type = LB_Other;
	    }

	  if (break_type == G_UNICODE_BREAK_CLOSE_PUNCTUATION ||
	      break_type == G_UNICODE_BREAK_CLOSE_PARANTHESIS)
	    {
	      if (prev_LB_type == LB_Numeric)
		LB_type = LB_Numeric_Close;
	      else
		LB_type = LB_Other;
	    }

	  if (break_type == G_UNICODE_BREAK_REGIONAL_INDICATOR)
	    {
	      if (prev_LB_type == LB_RI_Odd)
		LB_type = LB_RI_Even;
	      else if (prev_LB_type == LB_RI_Even)
		LB_type = LB_RI_Odd;
	      else
		LB_type = LB_RI_Odd;
	    }

	  attrs[i].is_line_break = TRUE; /* Rule LB31 */
	  /* Unicode doesn't specify char wrap;
	     we wrap around all chars currently. */
	  if (attrs[i].is_cursor_position)
	    attrs[i].is_char_break = TRUE;

	  /* Make any necessary replacements first */
	  if (row_break_type == G_UNICODE_BREAK_UNKNOWN)
	    row_break_type = G_UNICODE_BREAK_ALPHABETIC;

	  /* add the line break rules in reverse order to override
	     the lower priority rules. */

	  /* Rule LB30 */
	  if ((prev_break_type == G_UNICODE_BREAK_ALPHABETIC ||
	       prev_break_type == G_UNICODE_BREAK_HEBREW_LETTER ||
	       prev_break_type == G_UNICODE_BREAK_NUMERIC) &&
	      break_type == G_UNICODE_BREAK_OPEN_PUNCTUATION)
	    break_op = BREAK_PROHIBITED;

	  if (prev_break_type == G_UNICODE_BREAK_CLOSE_PARANTHESIS &&
	      (break_type == G_UNICODE_BREAK_ALPHABETIC ||
	       break_type == G_UNICODE_BREAK_HEBREW_LETTER ||
	       break_type == G_UNICODE_BREAK_NUMERIC))
	    break_op = BREAK_PROHIBITED;

	  /* Rule LB30a */
	  if (prev_LB_type == LB_RI_Odd && LB_type == LB_RI_Even)
	    break_op = BREAK_PROHIBITED;

	  /* Rule LB30b */
	  if (prev_break_type == G_UNICODE_BREAK_EMOJI_BASE &&
	      break_type == G_UNICODE_BREAK_EMOJI_MODIFIER)
	    break_op = BREAK_PROHIBITED;

	  /* Rule LB29 */
	  if (prev_break_type == G_UNICODE_BREAK_INFIX_SEPARATOR &&
	      (break_type == G_UNICODE_BREAK_ALPHABETIC ||
	       break_type == G_UNICODE_BREAK_HEBREW_LETTER))
	    break_op = BREAK_PROHIBITED;

	  /* Rule LB28 */
	  if ((prev_break_type == G_UNICODE_BREAK_ALPHABETIC ||
	       prev_break_type == G_UNICODE_BREAK_HEBREW_LETTER) &&
	      (break_type == G_UNICODE_BREAK_ALPHABETIC ||
	       break_type == G_UNICODE_BREAK_HEBREW_LETTER))
	    break_op = BREAK_PROHIBITED;

	  /* Rule LB27 */
	  if ((prev_break_type == G_UNICODE_BREAK_HANGUL_L_JAMO ||
	       prev_break_type == G_UNICODE_BREAK_HANGUL_V_JAMO ||
	       prev_break_type == G_UNICODE_BREAK_HANGUL_T_JAMO ||
	       prev_break_type == G_UNICODE_BREAK_HANGUL_LV_SYLLABLE ||
	       prev_break_type == G_UNICODE_BREAK_HANGUL_LVT_SYLLABLE) &&
	      (break_type == G_UNICODE_BREAK_INSEPARABLE ||
	       break_type == G_UNICODE_BREAK_POSTFIX))
	    break_op = BREAK_PROHIBITED;

	  if (prev_break_type == G_UNICODE_BREAK_PREFIX &&
	      (break_type == G_UNICODE_BREAK_HANGUL_L_JAMO ||
	       break_type == G_UNICODE_BREAK_HANGUL_V_JAMO ||
	       break_type == G_UNICODE_BREAK_HANGUL_T_JAMO ||
	       break_type == G_UNICODE_BREAK_HANGUL_LV_SYLLABLE ||
	       break_type == G_UNICODE_BREAK_HANGUL_LVT_SYLLABLE))
	    break_op = BREAK_PROHIBITED;

	  /* Rule LB26 */
	  if (prev_break_type == G_UNICODE_BREAK_HANGUL_L_JAMO &&
	      (break_type == G_UNICODE_BREAK_HANGUL_L_JAMO ||
	       break_type == G_UNICODE_BREAK_HANGUL_V_JAMO ||
	       break_type == G_UNICODE_BREAK_HANGUL_LV_SYLLABLE ||
	       break_type == G_UNICODE_BREAK_HANGUL_LVT_SYLLABLE))
	    break_op = BREAK_PROHIBITED;

	  if ((prev_break_type == G_UNICODE_BREAK_HANGUL_V_JAMO ||
	       prev_break_type == G_UNICODE_BREAK_HANGUL_LV_SYLLABLE) &&
	      (break_type == G_UNICODE_BREAK_HANGUL_V_JAMO ||
	       break_type == G_UNICODE_BREAK_HANGUL_T_JAMO))
	    break_op = BREAK_PROHIBITED;

	  if ((prev_break_type == G_UNICODE_BREAK_HANGUL_T_JAMO ||
	       prev_break_type == G_UNICODE_BREAK_HANGUL_LVT_SYLLABLE) &&
	      break_type == G_UNICODE_BREAK_HANGUL_T_JAMO)
	    break_op = BREAK_PROHIBITED;

	  /* Rule LB25 with Example 7 of Customization */
	  if ((prev_break_type == G_UNICODE_BREAK_PREFIX ||
	       prev_break_type == G_UNICODE_BREAK_POSTFIX) &&
	      break_type == G_UNICODE_BREAK_NUMERIC)
	    break_op = BREAK_PROHIBITED;

	  if ((prev_break_type == G_UNICODE_BREAK_PREFIX ||
	       prev_break_type == G_UNICODE_BREAK_POSTFIX) &&
	      (break_type == G_UNICODE_BREAK_OPEN_PUNCTUATION ||
	       break_type == G_UNICODE_BREAK_HYPHEN) &&
	      next_break_type == G_UNICODE_BREAK_NUMERIC)
	    break_op = BREAK_PROHIBITED;

	  if ((prev_break_type == G_UNICODE_BREAK_OPEN_PUNCTUATION ||
	       prev_break_type == G_UNICODE_BREAK_HYPHEN) &&
	      break_type == G_UNICODE_BREAK_NUMERIC)
	    break_op = BREAK_PROHIBITED;

	  if (prev_break_type == G_UNICODE_BREAK_NUMERIC &&
	      (break_type == G_UNICODE_BREAK_NUMERIC ||
	       break_type == G_UNICODE_BREAK_SYMBOL ||
	       break_type == G_UNICODE_BREAK_INFIX_SEPARATOR))
	    break_op = BREAK_PROHIBITED;

	  if (prev_LB_type == LB_Numeric &&
	      (break_type == G_UNICODE_BREAK_NUMERIC ||
	       break_type == G_UNICODE_BREAK_SYMBOL ||
	       break_type == G_UNICODE_BREAK_INFIX_SEPARATOR ||
	       break_type == G_UNICODE_BREAK_CLOSE_PUNCTUATION ||
	       break_type == G_UNICODE_BREAK_CLOSE_PARANTHESIS))
	    break_op = BREAK_PROHIBITED;

	  if ((prev_LB_type == LB_Numeric ||
	       prev_LB_type == LB_Numeric_Close) &&
	      (break_type == G_UNICODE_BREAK_POSTFIX ||
	       break_type == G_UNICODE_BREAK_PREFIX))
	    break_op = BREAK_PROHIBITED;

	  /* Rule LB24 */
	  if ((prev_break_type == G_UNICODE_BREAK_PREFIX ||
	       prev_break_type == G_UNICODE_BREAK_POSTFIX) &&
	      (break_type == G_UNICODE_BREAK_ALPHABETIC ||
	       break_type == G_UNICODE_BREAK_HEBREW_LETTER))
	    break_op = BREAK_PROHIBITED;

	  if ((prev_break_type == G_UNICODE_BREAK_ALPHABETIC ||
	       prev_break_type == G_UNICODE_BREAK_HEBREW_LETTER) &&
	      (break_type == G_UNICODE_BREAK_PREFIX ||
	       break_type == G_UNICODE_BREAK_POSTFIX))
	    break_op = BREAK_PROHIBITED;

	  /* Rule LB23 */
	  if ((prev_break_type == G_UNICODE_BREAK_ALPHABETIC ||
	       prev_break_type == G_UNICODE_BREAK_HEBREW_LETTER) &&
	      break_type == G_UNICODE_BREAK_NUMERIC)
	    break_op = BREAK_PROHIBITED;

	  if (prev_break_type == G_UNICODE_BREAK_NUMERIC &&
	      (break_type == G_UNICODE_BREAK_ALPHABETIC ||
	       break_type == G_UNICODE_BREAK_HEBREW_LETTER))
	    break_op = BREAK_PROHIBITED;

	  /* Rule LB23a */
	  if (prev_break_type == G_UNICODE_BREAK_PREFIX &&
	      (break_type == G_UNICODE_BREAK_IDEOGRAPHIC ||
	       break_type == G_UNICODE_BREAK_EMOJI_BASE ||
	       break_type == G_UNICODE_BREAK_EMOJI_MODIFIER))
	    break_op = BREAK_PROHIBITED;

	  if ((prev_break_type == G_UNICODE_BREAK_IDEOGRAPHIC ||
	       prev_break_type == G_UNICODE_BREAK_EMOJI_BASE ||
	       prev_break_type == G_UNICODE_BREAK_EMOJI_MODIFIER) &&
	      break_type == G_UNICODE_BREAK_POSTFIX)
	    break_op = BREAK_PROHIBITED;

	  /* Rule LB22 */
	  if (break_type == G_UNICODE_BREAK_INSEPARABLE)
	    {
	      if (prev_break_type == G_UNICODE_BREAK_ALPHABETIC ||
		  prev_break_type == G_UNICODE_BREAK_HEBREW_LETTER)
		break_op = BREAK_PROHIBITED;

	      if (prev_break_type == G_UNICODE_BREAK_EXCLAMATION)
		break_op = BREAK_PROHIBITED;

	      if (prev_break_type == G_UNICODE_BREAK_IDEOGRAPHIC ||
		  prev_break_type == G_UNICODE_BREAK_EMOJI_BASE ||
		  prev_break_type == G_UNICODE_BREAK_EMOJI_MODIFIER)
		break_op = BREAK_PROHIBITED;

	      if (prev_break_type == G_UNICODE_BREAK_INSEPARABLE)
		break_op = BREAK_PROHIBITED;

	      if (prev_break_type == G_UNICODE_BREAK_NUMERIC)
		break_op = BREAK_PROHIBITED;
	    }

	  if (break_type == G_UNICODE_BREAK_AFTER ||
	      break_type == G_UNICODE_BREAK_HYPHEN ||
	      break_type == G_UNICODE_BREAK_NON_STARTER ||
	      prev_break_type == G_UNICODE_BREAK_BEFORE)
	    break_op = BREAK_PROHIBITED; /* Rule LB21 */

	  if (prev_prev_break_type == G_UNICODE_BREAK_HEBREW_LETTER &&
	      (prev_break_type == G_UNICODE_BREAK_HYPHEN ||
	       prev_break_type == G_UNICODE_BREAK_AFTER))
	    break_op = BREAK_PROHIBITED; /* Rule LB21a */

	  if (prev_break_type == G_UNICODE_BREAK_SYMBOL &&
	      break_type == G_UNICODE_BREAK_HEBREW_LETTER)
	    break_op = BREAK_PROHIBITED; /* Rule LB21b */

	  if (prev_break_type == G_UNICODE_BREAK_CONTINGENT ||
	      break_type == G_UNICODE_BREAK_CONTINGENT)
	    break_op = BREAK_ALLOWED; /* Rule LB20 */

	  if (prev_break_type == G_UNICODE_BREAK_QUOTATION ||
	      break_type == G_UNICODE_BREAK_QUOTATION)
	    break_op = BREAK_PROHIBITED; /* Rule LB19 */

	  /* handle related rules for Space as state machine here,
	     and override the pair table result. */
	  if (prev_break_type == G_UNICODE_BREAK_SPACE) /* Rule LB18 */
	    break_op = BREAK_ALLOWED;

	  if (row_break_type == G_UNICODE_BREAK_BEFORE_AND_AFTER &&
	      break_type == G_UNICODE_BREAK_BEFORE_AND_AFTER)
	    break_op = BREAK_PROHIBITED; /* Rule LB17 */

	  if ((row_break_type == G_UNICODE_BREAK_CLOSE_PUNCTUATION ||
	       row_break_type == G_UNICODE_BREAK_CLOSE_PARANTHESIS) &&
	      break_type == G_UNICODE_BREAK_NON_STARTER)
	    break_op = BREAK_PROHIBITED; /* Rule LB16 */

	  if (row_break_type == G_UNICODE_BREAK_QUOTATION &&
	      break_type == G_UNICODE_BREAK_OPEN_PUNCTUATION)
	    break_op = BREAK_PROHIBITED; /* Rule LB15 */

	  if (row_break_type == G_UNICODE_BREAK_OPEN_PUNCTUATION)
	    break_op = BREAK_PROHIBITED; /* Rule LB14 */

	  /* Rule LB13 with Example 7 of Customization */
	  if (break_type == G_UNICODE_BREAK_EXCLAMATION)
	    break_op = BREAK_PROHIBITED;

	  if (prev_break_type != G_UNICODE_BREAK_NUMERIC &&
	      (break_type == G_UNICODE_BREAK_CLOSE_PUNCTUATION ||
	       break_type == G_UNICODE_BREAK_CLOSE_PARANTHESIS ||
	       break_type == G_UNICODE_BREAK_INFIX_SEPARATOR ||
	       break_type == G_UNICODE_BREAK_SYMBOL))
	    break_op = BREAK_PROHIBITED;

	  if (prev_break_type == G_UNICODE_BREAK_NON_BREAKING_GLUE)
	    break_op = BREAK_PROHIBITED; /* Rule LB12 */

	  if (break_type == G_UNICODE_BREAK_NON_BREAKING_GLUE &&
	      (prev_break_type != G_UNICODE_BREAK_SPACE &&
	       prev_break_type != G_UNICODE_BREAK_AFTER &&
	       prev_break_type != G_UNICODE_BREAK_HYPHEN))
	    break_op = BREAK_PROHIBITED; /* Rule LB12a */

	  if (prev_break_type == G_UNICODE_BREAK_WORD_JOINER ||
	      break_type == G_UNICODE_BREAK_WORD_JOINER)
	    break_op = BREAK_PROHIBITED; /* Rule LB11 */


	  /* Rule LB9 */
	  if (break_type == G_UNICODE_BREAK_COMBINING_MARK ||
              break_type == G_UNICODE_BREAK_ZERO_WIDTH_JOINER)
	    {
	      if (!(prev_break_type == G_UNICODE_BREAK_MANDATORY ||
		    prev_break_type == G_UNICODE_BREAK_CARRIAGE_RETURN ||
		    prev_break_type == G_UNICODE_BREAK_LINE_FEED ||
		    prev_break_type == G_UNICODE_BREAK_NEXT_LINE ||
		    prev_break_type == G_UNICODE_BREAK_SPACE ||
		    prev_break_type == G_UNICODE_BREAK_ZERO_WIDTH_SPACE))
		break_op = BREAK_PROHIBITED;
	    }

	  if (row_break_type == G_UNICODE_BREAK_ZERO_WIDTH_SPACE)
	    break_op = BREAK_ALLOWED; /* Rule LB8 */

	  if (prev_wc == 0x200D)
	    break_op = BREAK_PROHIBITED; /* Rule LB8a */

	  if (break_type == G_UNICODE_BREAK_SPACE ||
	      break_type == G_UNICODE_BREAK_ZERO_WIDTH_SPACE)
	    break_op = BREAK_PROHIBITED; /* Rule LB7 */

	  /* Rule LB6 */
	  if (break_type == G_UNICODE_BREAK_MANDATORY ||
	      break_type == G_UNICODE_BREAK_CARRIAGE_RETURN ||
	      break_type == G_UNICODE_BREAK_LINE_FEED ||
	      break_type == G_UNICODE_BREAK_NEXT_LINE)
	    break_op = BREAK_PROHIBITED;

	  /* Rules LB4 and LB5 */
	  if (prev_break_type == G_UNICODE_BREAK_MANDATORY ||
	      (prev_break_type == G_UNICODE_BREAK_CARRIAGE_RETURN &&
	       wc != '\n') ||
	      prev_break_type == G_UNICODE_BREAK_LINE_FEED ||
	      prev_break_type == G_UNICODE_BREAK_NEXT_LINE)
	    {
	      attrs[i].is_mandatory_break = TRUE;
	      break_op = BREAK_ALLOWED;
	    }

	  switch (break_op)
	    {
	    case BREAK_PROHIBITED:
	      /* can't break here */
	      attrs[i].is_line_break = FALSE;
	      break;

	    case BREAK_IF_SPACES:
	      /* break if prev char was space */
	      if (prev_break_type != G_UNICODE_BREAK_SPACE)
		attrs[i].is_line_break = FALSE;
	      break;

	    case BREAK_ALLOWED:
	      attrs[i].is_line_break = TRUE;
	      break;

	    case BREAK_ALREADY_HANDLED:
	      break;

	    default:
	      g_assert_not_reached ();
	      break;
	    }

	  /* Rule LB9 */
	  if (!(break_type == G_UNICODE_BREAK_COMBINING_MARK ||
		break_type == G_UNICODE_BREAK_ZERO_WIDTH_JOINER))
	    {
	      /* Rule LB25 with Example 7 of Customization */
	      if (break_type == G_UNICODE_BREAK_NUMERIC ||
		  break_type == G_UNICODE_BREAK_SYMBOL ||
		  break_type == G_UNICODE_BREAK_INFIX_SEPARATOR)
		{
		  if (prev_LB_type != LB_Numeric)
		    prev_LB_type = LB_type;
		  /* else don't change the prev_LB_type */
		}
	      else
		{
		  prev_LB_type = LB_type;
		}
	    }
	  /* else don't change the prev_LB_type for Rule LB9 */
	}

      if (break_type != G_UNICODE_BREAK_SPACE)
	{
	  /* Rule LB9 */
	  if (break_type == G_UNICODE_BREAK_COMBINING_MARK ||
	      break_type == G_UNICODE_BREAK_ZERO_WIDTH_JOINER)
	    {
	      if (i == 0 /* start of text */ ||
		  prev_break_type == G_UNICODE_BREAK_MANDATORY ||
		  prev_break_type == G_UNICODE_BREAK_CARRIAGE_RETURN ||
		  prev_break_type == G_UNICODE_BREAK_LINE_FEED ||
		  prev_break_type == G_UNICODE_BREAK_NEXT_LINE ||
		  prev_break_type == G_UNICODE_BREAK_SPACE ||
		  prev_break_type == G_UNICODE_BREAK_ZERO_WIDTH_SPACE)
		prev_break_type = G_UNICODE_BREAK_ALPHABETIC; /* Rule LB10 */
	      /* else don't change the prev_break_type for Rule LB9 */
	    }
	  else
	    {
	      prev_prev_break_type = prev_break_type;
	      prev_break_type = break_type;
	    }

	  prev_jamo = jamo;
	}
      else
	{
	  if (prev_break_type != G_UNICODE_BREAK_SPACE)
	    {
	      prev_prev_break_type = prev_break_type;
	      prev_break_type = break_type;
	    }
	  /* else don't change the prev_break_type */
	}

      /* ---- Word breaks ---- */

      /* default to not a word start/end */
      attrs[i].is_word_start = FALSE;
      attrs[i].is_word_end = FALSE;

      if (current_word_type != WordNone)
	{
	  /* Check for a word end */
	  switch ((int) type)
	    {
	    case G_UNICODE_SPACING_MARK:
	    case G_UNICODE_ENCLOSING_MARK:
	    case G_UNICODE_NON_SPACING_MARK:
	    case G_UNICODE_FORMAT:
	      /* nothing, we just eat these up as part of the word */
	      break;

	    case G_UNICODE_LOWERCASE_LETTER:
	    case G_UNICODE_MODIFIER_LETTER:
	    case G_UNICODE_OTHER_LETTER:
	    case G_UNICODE_TITLECASE_LETTER:
	    case G_UNICODE_UPPERCASE_LETTER:
	      if (current_word_type == WordLetters)
		{
		  /* Japanese special cases for ending the word */
		  if (JAPANESE (last_word_letter) ||
		      JAPANESE (wc))
		    {
		      if ((HIRAGANA (last_word_letter) &&
			   !HIRAGANA (wc)) ||
			  (KATAKANA (last_word_letter) &&
			   !(KATAKANA (wc) || HIRAGANA (wc))) ||
			  (KANJI (last_word_letter) &&
			   !(HIRAGANA (wc) || KANJI (wc))) ||
			  (JAPANESE (last_word_letter) &&
			   !JAPANESE (wc)) ||
			  (!JAPANESE (last_word_letter) &&
			   JAPANESE (wc)))
			attrs[i].is_word_end = TRUE;
		    }
		}
	      last_word_letter = wc;
	      break;

	    case G_UNICODE_DECIMAL_NUMBER:
	    case G_UNICODE_LETTER_NUMBER:
	    case G_UNICODE_OTHER_NUMBER:
	      last_word_letter = wc;
	      break;

	    default:
	      /* Punctuation, control/format chars, etc. all end a word. */
	      attrs[i].is_word_end = TRUE;
	      current_word_type = WordNone;
	      break;
	    }
	}
      else
	{
	  /* Check for a word start */
	  switch ((int) type)
	    {
	    case G_UNICODE_LOWERCASE_LETTER:
	    case G_UNICODE_MODIFIER_LETTER:
	    case G_UNICODE_OTHER_LETTER:
	    case G_UNICODE_TITLECASE_LETTER:
	    case G_UNICODE_UPPERCASE_LETTER:
	      current_word_type = WordLetters;
	      last_word_letter = wc;
	      attrs[i].is_word_start = TRUE;
	      break;

	    case G_UNICODE_DECIMAL_NUMBER:
	    case G_UNICODE_LETTER_NUMBER:
	    case G_UNICODE_OTHER_NUMBER:
	      current_word_type = WordNumbers;
	      last_word_letter = wc;
	      attrs[i].is_word_start = TRUE;
	      break;

	    default:
	      /* No word here */
	      break;
	    }
	}

      /* ---- Sentence breaks ---- */
      {

	/* default to not a sentence start/end */
	attrs[i].is_sentence_start = FALSE;
	attrs[i].is_sentence_end = FALSE;

	/* maybe start sentence */
	if (last_sentence_start == -1 && !is_sentence_boundary)
	  last_sentence_start = i - 1;

	/* remember last non space character position */
	if (i > 0 && !attrs[i - 1].is_white)
	  last_non_space = i;

	/* meets sentence end, mark both sentence start and end */
	if (last_sentence_start != -1 && is_sentence_boundary) {
	  if (last_non_space != -1) {
	    attrs[last_sentence_start].is_sentence_start = TRUE;
	    attrs[last_non_space].is_sentence_end = TRUE;
	  }

	  last_sentence_start = -1;
	  last_non_space = -1;
	}

	/* meets space character, move sentence start */
	if (last_sentence_start != -1 &&
	    last_sentence_start == i - 1 &&
	    attrs[i - 1].is_white)
	    last_sentence_start++;

      }

      prev_wc = wc;

      /* wc might not be a valid Unicode base character, but really all we
       * need to know is the last non-combining character */
      if (type != G_UNICODE_SPACING_MARK &&
	  type != G_UNICODE_ENCLOSING_MARK &&
	  type != G_UNICODE_NON_SPACING_MARK)
	base_character = wc;
    }

  i--;

  attrs[i].is_cursor_position = TRUE;  /* Rule GB2 */
  attrs[0].is_cursor_position = TRUE;  /* Rule GB1 */

  attrs[i].is_word_boundary = TRUE;  /* Rule WB2 */
  attrs[0].is_word_boundary = TRUE;  /* Rule WB1 */

  attrs[i].is_line_break = TRUE;  /* Rule LB3 */
  attrs[0].is_line_break = FALSE; /* Rule LB2 */

}

static gboolean
tailor_break (const gchar   *text,
	     gint           length,
	     PangoAnalysis *analysis,
	     PangoLogAttr  *attrs,
	     int            attrs_len)
{
  if (analysis->lang_engine && PANGO_ENGINE_LANG_GET_CLASS (analysis->lang_engine)->script_break)
    {
      if (length < 0)
	length = strlen (text);
      else if (text == NULL)
	text = "";

      PANGO_ENGINE_LANG_GET_CLASS (analysis->lang_engine)->script_break (analysis->lang_engine, text, length, analysis, attrs, attrs_len);
      return TRUE;
    }
  return FALSE;
}

/**
 * pango_break:
 * @text:      the text to process
 * @length:    length of @text in bytes (may be -1 if @text is nul-terminated)
 * @analysis:  #PangoAnalysis structure from pango_itemize()
 * @attrs:     (array length=attrs_len): an array to store character
 *             information in
 * @attrs_len: size of the array passed as @attrs
 *
 * Determines possible line, word, and character breaks
 * for a string of Unicode text with a single analysis.  For most
 * purposes you may want to use pango_get_log_attrs().
 */
void
pango_break (const gchar   *text,
	     gint           length,
	     PangoAnalysis *analysis,
	     PangoLogAttr  *attrs,
	     int            attrs_len)
{
  g_return_if_fail (analysis != NULL);
  g_return_if_fail (attrs != NULL);

  pango_default_break (text, length, analysis, attrs, attrs_len);
  tailor_break        (text, length, analysis, attrs, attrs_len);
}

/**
 * pango_find_paragraph_boundary:
 * @text: UTF-8 text
 * @length: length of @text in bytes, or -1 if nul-terminated
 * @paragraph_delimiter_index: (out): return location for index of
 *   delimiter
 * @next_paragraph_start: (out): return location for start of next
 *   paragraph
 *
 * Locates a paragraph boundary in @text. A boundary is caused by
 * delimiter characters, such as a newline, carriage return, carriage
 * return-newline pair, or Unicode paragraph separator character.  The
 * index of the run of delimiters is returned in
 * @paragraph_delimiter_index. The index of the start of the paragraph
 * (index after all delimiters) is stored in @next_paragraph_start.
 *
 * If no delimiters are found, both @paragraph_delimiter_index and
 * @next_paragraph_start are filled with the length of @text (an index one
 * off the end).
 **/
void
pango_find_paragraph_boundary (const gchar *text,
			       gint         length,
			       gint        *paragraph_delimiter_index,
			       gint        *next_paragraph_start)
{
  const gchar *p = text;
  const gchar *end;
  const gchar *start = NULL;
  const gchar *delimiter = NULL;

  /* Only one character has type G_UNICODE_PARAGRAPH_SEPARATOR in
   * Unicode 5.0; update the following code if that changes.
   */

  /* prev_sep is the first byte of the previous separator.  Since
   * the valid separators are \r, \n, and PARAGRAPH_SEPARATOR, the
   * first byte is enough to identify it.
   */
  gchar prev_sep;


  if (length < 0)
    length = strlen (text);

  end = text + length;

  if (paragraph_delimiter_index)
    *paragraph_delimiter_index = length;

  if (next_paragraph_start)
    *next_paragraph_start = length;

  if (length == 0)
    return;

  prev_sep = 0;

  while (p < end)
    {
      if (prev_sep == '\n' ||
	  prev_sep == PARAGRAPH_SEPARATOR_STRING[0])
	{
	  g_assert (delimiter);
	  start = p;
	  break;
	}
      else if (prev_sep == '\r')
	{
	  /* don't break between \r and \n */
	  if (*p != '\n')
	    {
	      g_assert (delimiter);
	      start = p;
	      break;
	    }
	}

      if (*p == '\n' ||
	   *p == '\r' ||
	   !strncmp(p, PARAGRAPH_SEPARATOR_STRING,
		    strlen(PARAGRAPH_SEPARATOR_STRING)))
	{
	  if (delimiter == NULL)
	    delimiter = p;
	  prev_sep = *p;
	}
      else
	prev_sep = 0;

      p = g_utf8_next_char (p);
    }

  if (delimiter && paragraph_delimiter_index)
    *paragraph_delimiter_index = delimiter - text;

  if (start && next_paragraph_start)
    *next_paragraph_start = start - text;
}

static int
tailor_segment (const char      *range_start,
		const char      *range_end,
		int              chars_broken,
		PangoAnalysis   *analysis,
		PangoLogAttr    *log_attrs)
{
  int chars_in_range;
  PangoLogAttr *start = log_attrs + chars_broken;
  PangoLogAttr attr_before = *start;

  chars_in_range = pango_utf8_strlen (range_start, range_end - range_start);

  if (tailor_break (range_start,
		    range_end - range_start,
		    analysis,
		    start,
		    chars_in_range + 1))
    {
      /* if tailored, we enforce some of the attrs from before tailoring at
       * the boundary
       */

     start->backspace_deletes_character  = attr_before.backspace_deletes_character;

     start->is_line_break      |= attr_before.is_line_break;
     start->is_mandatory_break |= attr_before.is_mandatory_break;
     start->is_cursor_position |= attr_before.is_cursor_position;
    }

  return chars_in_range;
}

/**
 * pango_get_log_attrs:
 * @text: text to process
 * @length: length in bytes of @text
 * @level: embedding level, or -1 if unknown
 * @language: language tag
 * @log_attrs: (array length=attrs_len): array with one #PangoLogAttr
 *   per character in @text, plus one extra, to be filled in
 * @attrs_len: length of @log_attrs array
 *
 * Computes a #PangoLogAttr for each character in @text. The @log_attrs
 * array must have one #PangoLogAttr for each position in @text; if
 * @text contains N characters, it has N+1 positions, including the
 * last position at the end of the text. @text should be an entire
 * paragraph; logical attributes can't be computed without context
 * (for example you need to see spaces on either side of a word to know
 * the word is a word).
 */
void
pango_get_log_attrs (const char    *text,
		     int            length,
		     int            level,
		     PangoLanguage *language,
		     PangoLogAttr  *log_attrs,
		     int            attrs_len)
{
  int chars_broken;
  PangoAnalysis analysis = { NULL };
  PangoScriptIter iter;

  g_return_if_fail (length == 0 || text != NULL);
  g_return_if_fail (log_attrs != NULL);

  analysis.level = level;
  analysis.lang_engine = _pango_get_language_engine ();

  pango_default_break (text, length, &analysis, log_attrs, attrs_len);

  chars_broken = 0;

  _pango_script_iter_init (&iter, text, length);
  do
    {
      const char *run_start, *run_end;
      PangoScript script;

      pango_script_iter_get_range (&iter, &run_start, &run_end, &script);
      analysis.script = script;

      chars_broken += tailor_segment (run_start, run_end, chars_broken, &analysis, log_attrs);
    }
  while (pango_script_iter_next (&iter));
  _pango_script_iter_fini (&iter);

  if (chars_broken + 1 > attrs_len)
    g_warning ("pango_get_log_attrs: attrs_len should have been at least %d, but was %d.  Expect corrupted memory.",
	       chars_broken + 1,
	       attrs_len);
}

#include "break-arabic.c"
#include "break-indic.c"
#include "break-thai.c"

static void
break_script (const char          *item_text,
	      unsigned int         item_length,
	      const PangoAnalysis *analysis,
	      PangoLogAttr        *attrs,
	      int                  attrs_len)
{
  switch (analysis->script)
    {
    case PANGO_SCRIPT_ARABIC:
      break_arabic (item_text, item_length, analysis, attrs, attrs_len);
      break;

    case PANGO_SCRIPT_DEVANAGARI:
    case PANGO_SCRIPT_BENGALI:
    case PANGO_SCRIPT_GURMUKHI:
    case PANGO_SCRIPT_GUJARATI:
    case PANGO_SCRIPT_ORIYA:
    case PANGO_SCRIPT_TAMIL:
    case PANGO_SCRIPT_TELUGU:
    case PANGO_SCRIPT_KANNADA:
    case PANGO_SCRIPT_MALAYALAM:
    case PANGO_SCRIPT_SINHALA:
      break_indic (item_text, item_length, analysis, attrs, attrs_len);
      break;

    case PANGO_SCRIPT_THAI:
      break_thai (item_text, item_length, analysis, attrs, attrs_len);
      break;
    }
}


/* Wrap language breaker in PangoEngineLang to pass it through old API,
 * from times when there were modules and engines. */
typedef PangoEngineLang      PangoLanguageEngine;
typedef PangoEngineLangClass PangoLanguageEngineClass;
static GType pango_language_engine_get_type (void) G_GNUC_CONST;
G_GNUC_BEGIN_IGNORE_DEPRECATIONS
G_DEFINE_TYPE (PangoLanguageEngine, pango_language_engine, PANGO_TYPE_ENGINE_LANG);
G_GNUC_END_IGNORE_DEPRECATIONS
static void
_pango_language_engine_break (PangoEngineLang *engine G_GNUC_UNUSED,
			      const char      *item_text,
			      int              item_length,
			      PangoAnalysis   *analysis,
			      PangoLogAttr    *attrs,
			      int              attrs_len)
{
  break_script (item_text, item_length, analysis, attrs, attrs_len);
}
static void
pango_language_engine_class_init (PangoEngineLangClass *class)
{
  class->script_break = _pango_language_engine_break;
}
static void
pango_language_engine_init (PangoEngineLang *object)
{
}

PangoEngineLang *
_pango_get_language_engine (void)
{
  static PangoEngineLang *engine;
  if (g_once_init_enter (&engine))
    g_once_init_leave (&engine, g_object_new (pango_language_engine_get_type(), NULL));
  return engine;
}
