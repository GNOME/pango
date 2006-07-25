/* -*- mode: C; c-file-style: "gnu" -*- */
/* Pango
 * pango-script.c: Script tag handling
 *
 * Copyright (C) 2002 Red Hat Software
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
 * Implementation of pango_script_iter is derived from ICU:
 *
 *  icu/sources/common/usc_impl.c
 *
 **********************************************************************
 *   Copyright (C) 1999-2002, International Business Machines
 *   Corporation and others.  All Rights Reserved.
 **********************************************************************
 *
 * Permission is hereby granted, free of charge, to any person obtaining a
 * copy of this software and associated documentation files (the
 * "Software"), to deal in the Software without restriction, including
 * without limitation the rights to use, copy, modify, merge, publish,
 * distribute, and/or sell copies of the Software, and to permit persons
 * to whom the Software is furnished to do so, provided that the above
 * copyright notice(s) and this permission notice appear in all copies of
 * the Software and that both the above copyright notice(s) and this
 * permission notice appear in supporting documentation.
 * 
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS
 * OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF
 * MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT
 * OF THIRD PARTY RIGHTS. IN NO EVENT SHALL THE COPYRIGHT HOLDER OR
 * HOLDERS INCLUDED IN THIS NOTICE BE LIABLE FOR ANY CLAIM, OR ANY SPECIAL
 * INDIRECT OR CONSEQUENTIAL DAMAGES, OR ANY DAMAGES WHATSOEVER RESULTING
 * FROM LOSS OF USE, DATA OR PROFITS, WHETHER IN AN ACTION OF CONTRACT,
 * NEGLIGENCE OR OTHER TORTIOUS ACTION, ARISING OUT OF OR IN CONNECTION
 * WITH THE USE OR PERFORMANCE OF THIS SOFTWARE.
 * 
 * Except as contained in this notice, the name of a copyright holder
 * shall not be used in advertising or otherwise to promote the sale, use
 * or other dealings in this Software without prior written authorization
 * of the copyright holder.
 */

#include <config.h>
#include <stdlib.h>
#include <string.h>

#include "pango-script.h"
#include "pango-script-table.h"

#define PAREN_STACK_DEPTH 128

typedef struct _ParenStackEntry ParenStackEntry;

struct _ParenStackEntry
{
  int pair_index;
  PangoScript script_code;
};

struct _PangoScriptIter
{
  const gchar *text_start;
  const gchar *text_end;

  const gchar *script_start;
  const gchar *script_end;
  PangoScript script_code;

  ParenStackEntry paren_stack[PAREN_STACK_DEPTH];
  int paren_sp;
};

#define PANGO_SCRIPT_TABLE_MIDPOINT (G_N_ELEMENTS (pango_script_table) / 2)

static inline PangoScript
pango_script_for_unichar_bsearch (gunichar ch)
{
  int lower = 0;
  int upper = G_N_ELEMENTS (pango_script_table) - 1;
  static int saved_mid = PANGO_SCRIPT_TABLE_MIDPOINT;
  int mid = saved_mid;


  do 
    {
      if (ch < pango_script_table[mid].start)
	upper = mid - 1;
      else if (ch >= pango_script_table[mid].start + pango_script_table[mid].chars)
	lower = mid + 1;
      else
	return pango_script_table[saved_mid = mid].script;

      mid = (lower + upper) / 2;
    }
  while (lower <= upper);

  return PANGO_SCRIPT_UNKNOWN;
}

/**
 * pango_script_for_unichar:
 * @ch: a Unicode character
 * 
 * Looks up the #PangoScript for a particular character (as defined by
 * Unicode Standard Annex #24). No check is made for @ch being a
 * valid Unicode character; if you pass in invalid character, the
 * result is undefined.
 * 
 * Return value: the #PangoScript for the character.
 *
 * Since: 1.4
 **/
PangoScript
pango_script_for_unichar (gunichar ch)
{
  if (ch < PANGO_EASY_SCRIPTS_RANGE)
    return pango_script_easy_table[ch];
  else 
    return pango_script_for_unichar_bsearch (ch); 
}

/**********************************************************************/

/**
 * pango_script_iter_new:
 * @text: a UTF-8 string
 * @length: length of @text, or -1 if @text is nul-terminated.
 * 
 * Create a new #PangoScriptIter, used to break a string of
 * Unicode into runs by text. No copy is made of @text, so
 * the caller needs to make sure it remains valid until
 * the iterator is freed with pango_script_iter_free ().x
 * 
 * Return value: the new script iterator, initialized
 *  to point at the first range in the text, which should be
 *  freed with pango_script_iter_free(). If the string is
 *  empty, it will point at an empty range.
 *
 * Since: 1.4
 **/
PangoScriptIter *
pango_script_iter_new (const char *text,
		       int         length)
{
  PangoScriptIter *iter = g_slice_new (PangoScriptIter);

  iter->text_start = text;
  if (length >= 0)
    iter->text_end = text + length;
  else
    iter->text_end = text + strlen (text);

  iter->script_start = text;
  iter->script_end = text;
  iter->script_code = PANGO_SCRIPT_COMMON;
  
  iter->paren_sp = -1;

  pango_script_iter_next (iter);

  return iter;
}

/**
 * pango_script_iter_free:
 * @iter: a #PangoScriptIter
 * 
 * Frees a #PangoScriptIter created with pango_script_iter_new().
 *
 * Since: 1.4
 **/
void
pango_script_iter_free (PangoScriptIter *iter)
{
  g_slice_free (PangoScriptIter, iter);
}

/**
 * pango_script_iter_get_range:
 * @iter: a #PangoScriptIter
 * @start: location to store start position of the range, or %NULL
 * @end: location to store end position of the range, or %NULL
 * @script: location to store script for range, or %NULL
 * 
 * Gets information about the range to which @iter currently points.
 * The range is the set of locations p where *start <= p < *end.
 * (That is, it doesn't include the character stored at *end)
 *
 * Since: 1.4
 **/
void
pango_script_iter_get_range (PangoScriptIter      *iter,
			     G_CONST_RETURN char **start,
			     G_CONST_RETURN char **end,
			     PangoScript          *script)
{
  if (start)
    *start = iter->script_start;
  if (end)
    *end = iter->script_end;
  if (script)
    *script = iter->script_code;
}

static const gunichar paired_chars[] = {
  0x0028, 0x0029, /* ascii paired punctuation */
  0x003c, 0x003e,
  0x005b, 0x005d,
  0x007b, 0x007d,
  0x00ab, 0x00bb, /* guillemets */
  0x2018, 0x2019, /* general punctuation */
  0x201c, 0x201d,
  0x2039, 0x203a,
  0x3008, 0x3009, /* chinese paired punctuation */
  0x300a, 0x300b,
  0x300c, 0x300d,
  0x300e, 0x300f,
  0x3010, 0x3011,
  0x3014, 0x3015,
  0x3016, 0x3017,
  0x3018, 0x3019,
  0x301a, 0x301b
};

static int
get_pair_index (gunichar ch)
{
  int lower = 0;
  int upper = G_N_ELEMENTS (paired_chars) - 1;

  while (lower <= upper)
    {
      int mid = (lower + upper) / 2;
      
      if (ch < paired_chars[mid])
	upper = mid - 1;
      else if (ch > paired_chars[mid])
	lower = mid + 1;
      else
	return mid;
    }

  return -1;
}

#define REAL_SCRIPT(script) \
  ((script) > PANGO_SCRIPT_INHERITED)

#define SAME_SCRIPT(script1, script2) \
  (!REAL_SCRIPT (script1) || !REAL_SCRIPT (script2) || (script1) == (script2))

#define IS_OPEN(pair_index) (((pair_index) & 1) == 0)

/**
 * pango_script_iter_next:
 * @iter: a #PangoScriptIter
 * 
 * Advances a #PangoScriptIter to the next range. If the iter
 * is already at the end, it is left unchanged and %FALSE
 * is returned.
 * 
 * Return value: %TRUE if the iter was succesfully advanced.
 *
 * Since: 1.4
 **/
gboolean
pango_script_iter_next (PangoScriptIter *iter)
{
  int start_sp;
  
  if (iter->script_end == iter->text_end)
    return FALSE;

  start_sp = iter->paren_sp;
  iter->script_code = PANGO_SCRIPT_COMMON;
  iter->script_start = iter->script_end;

  for (; iter->script_end < iter->text_end; iter->script_end = g_utf8_next_char (iter->script_end))
    {
      gunichar ch = g_utf8_get_char (iter->script_end);
      PangoScript sc;
      int pair_index;
      
      sc = pango_script_for_unichar (ch);
      if (sc != PANGO_SCRIPT_COMMON)
	pair_index = -1;
      else
	pair_index = get_pair_index (ch);

      /*
       * Paired character handling:
       *
       * if it's an open character, push it onto the stack.
       * if it's a close character, find the matching open on the
       * stack, and use that script code. Any non-matching open
       * characters above it on the stack will be poped.
       */
      if (pair_index >= 0)
	{
	  if (IS_OPEN (pair_index))
	    {
	      /*
	       * If the paren stack is full, empty it. This
	       * means that deeply nested paired punctuation
	       * characters will be ignored, but that's an unusual
	       * case, and it's better to ignore them than to
	       * write off the end of the stack...
	       */
	      if (++iter->paren_sp >= PAREN_STACK_DEPTH)
		iter->paren_sp = 0;

	      iter->paren_stack[iter->paren_sp].pair_index = pair_index;
	      iter->paren_stack[iter->paren_sp].script_code = iter->script_code;
	    }
	  else if (iter->paren_sp >= 0)
	    {
	      int pi = pair_index & ~1;

	      while (iter->paren_sp >= 0 && iter->paren_stack[iter->paren_sp].pair_index != pi)
		iter->paren_sp--;

	      if (iter->paren_sp < start_sp)
		start_sp = iter->paren_sp;

	      if (iter->paren_sp >= 0)
		sc = iter->paren_stack[iter->paren_sp].script_code;
	    }
	}

      if (SAME_SCRIPT (iter->script_code, sc))
	{
	  if (!REAL_SCRIPT (iter->script_code) && REAL_SCRIPT (sc))
	    {
	      iter->script_code = sc;

	      /*
	       * now that we have a final script code, fix any open
	       * characters we pushed before we knew the script code.
	       */
	      while (start_sp < iter->paren_sp)
		iter->paren_stack[++start_sp].script_code = iter->script_code;
	    }

	  /*
	   * if this character is a close paired character,
	   * pop it from the stack
	   */
	  if (pair_index >= 0 && !IS_OPEN (pair_index) && iter->paren_sp >= 0)
	    {
	      iter->paren_sp--;
	      
	      if (iter->paren_sp < start_sp)
		start_sp = iter->paren_sp;
            }
	}
      else
	{
	  /* Different script, we're done */
	  break;
	}
    }

  return TRUE;
}

/**********************************************************
 * End of code from ICU
 **********************************************************/

#include "pango-script-lang-table.h"

/* The fact that this comparison function works is dependent
 * on a property of the pango_script_lang_table which accidental rather
 * than inherent.
 *
 * The property is if we take any element in the table and suffix it
 * <elem>-<suffix> then that must strcmp() between any elements
 * preceding the element in the table and any element following in the
 * table. So, if we had something like:
 *
 * 'zh'
 *' zh-cn'
 *
 * in the table we would have a problem since 'zh-tw' follows 'zh-cn'.
 * On the other hand:
 *
 * 'zh'
 * 'zha'
 *
 * Works because 'zh-tw' precedes 'zha'.
 */
static int
script_for_lang_compare (gconstpointer key,
			 gconstpointer member)
{
  PangoLanguage *lang = (PangoLanguage *)key;
  const PangoScriptForLang *script_for_lang = member;

  if (pango_language_matches (lang, script_for_lang->lang))
    return 0;
  else
    return strcmp (pango_language_to_string (lang),
		   script_for_lang->lang);
}

/**
 * pango_language_includes_script:
 * @language: a PangoLanguage
 * @script: a #PangoScript
 * 
 * Determines if @script is one of the scripts used to
 * write @language. The returned value is conservative;
 * if nothing is known about the language tag @language,
 * %TRUE will be returned, since, as far as Pango knows,
 * @script might be used to write @language.
 *
 * This routine is used in Pango's itemization process when
 * determining if a supplied language tag is relevant to
 * a particular section of text. It probably is not useful for
 * applications in most circumstances.
 * 
 * Return value: %TRUE if @script is one of the scripts used
 * to write @language, or if nothing is known about @language.
 *
 * Since: 1.4
 **/
gboolean
pango_language_includes_script (PangoLanguage *language,
				PangoScript    script)
{
  PangoScriptForLang *script_for_lang;
  unsigned int j;

  g_return_val_if_fail (language != NULL, FALSE);

  if (!REAL_SCRIPT (script))
    return TRUE;

  /* This bsearch could be optimized to occur only once if
   * we store the pointer to the PangoScriptForLang in the
   * same block as the string value for the PangoLanguage.
   */
  script_for_lang = bsearch (pango_language_to_string (language),
			     pango_script_for_lang,
			     G_N_ELEMENTS (pango_script_for_lang),
			     sizeof (PangoScriptForLang),
			     script_for_lang_compare);
  if (!script_for_lang)
    return TRUE;

  for (j = 0; j < G_N_ELEMENTS (script_for_lang->scripts); j++)
    if (script_for_lang->scripts[j] == script)
      return TRUE;

  return FALSE;
}

/**
 * pango_script_get_sample_language:
 * @script: a #PangoScript
 * 
 * Given a script, finds a language tag that is reasonably
 * representative of that script. This will usually be the
 * most widely spoken or used language written in that script:
 * for instance, the sample language for %PANGO_SCRIPT_CYRILLIC
 * is <literal>ru</literal> (Russian), the sample lanugage
 * for %PANGO_SCRIPT_ARABIC is <literal>ar</literal>.
 *
 * For some
 * scripts, no sample language will be returned because there
 * is no language that is sufficiently representative. The best
 * example of this is %PANGO_SCRIPT_HAN, where various different
 * variants of written Chinese, Japanese, and Korean all use
 * significantly different sets of Han characters and forms
 * of shared characters. No sample language can be provided
 * for many historical scripts as well.
 * 
 * Return value: a #PangoLanguage that is representative
 * of the script, or %NULL if no such language exists.
 *
 * Since: 1.4
 **/
PangoLanguage *
pango_script_get_sample_language (PangoScript script)
{
  /* Note that in the following, we want
   * pango_language_includes_script() for the sample language
   * to include the script, so alternate orthographies
   * (Shavian for English, Osmanya for Somali, etc), typically
   * have no sample language
   */
  const char sample_languages[][4] = {
    "",    /* PANGO_SCRIPT_COMMON */
    "",    /* PANGO_SCRIPT_INHERITED */
    "ar",  /* PANGO_SCRIPT_ARABIC */
    "hy",  /* PANGO_SCRIPT_ARMENIAN */
    "bn",  /* PANGO_SCRIPT_BENGALI */
    /* Used primarily in Taiwan, but not part of the standard
     * zh-tw orthography  */
    "",    /* PANGO_SCRIPT_BOPOMOFO */
    "chr", /* PANGO_SCRIPT_CHEROKEE */
    "cop", /* PANGO_SCRIPT_COPTIC */
    "ru",  /* PANGO_SCRIPT_CYRILLIC */
    /* Deseret was used to write English */
    "",    /* PANGO_SCRIPT_DESERET */
    "hi",  /* PANGO_SCRIPT_DEVANAGARI */
    "am",  /* PANGO_SCRIPT_ETHIOPIC */
    "ka",  /* PANGO_SCRIPT_GEORGIAN */
    "",    /* PANGO_SCRIPT_GOTHIC */
    "el",  /* PANGO_SCRIPT_GREEK */
    "gu",  /* PANGO_SCRIPT_GUJARATI */
    "pa",  /* PANGO_SCRIPT_GURMUKHI */
    "",    /* PANGO_SCRIPT_HAN */
    "ko",  /* PANGO_SCRIPT_HANGUL */
    "he",  /* PANGO_SCRIPT_HEBREW */
    "ja",  /* PANGO_SCRIPT_HIRAGANA */
    "kn",  /* PANGO_SCRIPT_KANNADA */
    "ja",  /* PANGO_SCRIPT_KATAKANA */
    "km",  /* PANGO_SCRIPT_KHMER */
    "lo",  /* PANGO_SCRIPT_LAO */
    "en",  /* PANGO_SCRIPT_LATIN */
    "ml",  /* PANGO_SCRIPT_MALAYALAM */
    "mn",  /* PANGO_SCRIPT_MONGOLIAN */
    "my",  /* PANGO_SCRIPT_MYANMAR */
    /* Ogham was used to write old Irish */
    "",    /* PANGO_SCRIPT_OGHAM */
    "",    /* PANGO_SCRIPT_OLD_ITALIC */
    "or",  /* PANGO_SCRIPT_ORIYA */
    "",    /* PANGO_SCRIPT_RUNIC */
    "si",  /* PANGO_SCRIPT_SINHALA */
    "syr", /* PANGO_SCRIPT_SYRIAC */
    "ta",  /* PANGO_SCRIPT_TAMIL */
    "te",  /* PANGO_SCRIPT_TELUGU */
    "dv",  /* PANGO_SCRIPT_THAANA */
    "th",  /* PANGO_SCRIPT_THAI */
    "bo",  /* PANGO_SCRIPT_TIBETAN */
    "iu",  /* PANGO_SCRIPT_CANADIAN_ABORIGINAL */
    "",    /* PANGO_SCRIPT_YI */
    "tl",  /* PANGO_SCRIPT_TAGALOG */
    /* There are no ISO-636 language codes for the following
     * Phillipino languages/scripts */
    "",    /* PANGO_SCRIPT_HANUNOO */
    "",    /* PANGO_SCRIPT_BUHID */
    "",    /* PANGO_SCRIPT_TAGBANWA */

    "",    /* PANGO_SCRIPT_BRAILLE */
    "",    /* PANGO_SCRIPT_CYPRIOT */
    "",    /* PANGO_SCRIPT_LIMBU */
    /* Used for Somali (so) in the past */
    "",    /* PANGO_SCRIPT_OSMANYA */
    /* The Shavian alphabet was designed for English */
    "",    /* PANGO_SCRIPT_SHAVIAN */
    "",    /* PANGO_SCRIPT_LINEAR_B */
    "",    /* PANGO_SCRIPT_TAI_LE */
    "uga", /* PANGO_SCRIPT_UGARITIC */
    
    "",    /* PANGO_SCRIPT_NEW_TAI_LUE */
    "bug", /* PANGO_SCRIPT_BUGINESE */
    /* The original script for Old Church Slavonic (chu), later
     * written with Cyrillic */
    "",    /* PANGO_SCRIPT_GLAGOLITIC */
    /* Used for for Berber (ber), but Arabic script is more common */
    "",    /* PANGO_SCRIPT_TIFINAGH */
    /* Syloti Nagri is used for Sylheti, no ISO 639 code */
    "",    /* PANGO_SCRIPT_SYLOTI_NAGRI */
    "peo", /* PANGO_SCRIPT_OLD_PERSIAN */
    "",    /* PANGO_SCRIPT_KHAROSHTHI */

    "",    /* PANGO_SCRIPT_UNKNOWN */
    "",    /* PANGO_SCRIPT_BALINESE */
    "",    /* PANGO_SCRIPT_CUNEIFORM */
    "",    /* PANGO_SCRIPT_PHOENICIAN */
    "",    /* PANGO_SCRIPT_PHAGS_PA */
    "nqo"  /* PANGO_SCRIPT_NKO */
  };
  const char *sample_language;
  
  g_return_val_if_fail (script >= 0, NULL);
  g_return_val_if_fail ((guint)script < G_N_ELEMENTS (sample_languages), NULL);
  
  sample_language = sample_languages[script];

  if (!sample_language[0])
    return NULL;
  else
    return pango_language_from_string (sample_language);
}
