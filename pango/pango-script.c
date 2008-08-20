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

#include "config.h"
#include <stdlib.h>
#include <string.h>

#include "pango-script.h"

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

/**
 * pango_script_for_unichar:
 * @ch: a Unicode character
 *
 * Looks up the #PangoScript for a particular character (as defined by
 * Unicode Standard Annex #24). No check is made for @ch being a
 * valid Unicode character; if you pass in invalid character, the
 * result is undefined.
 *
 * As of Pango 1.18, this function simply returns the return value of
 * g_unichar_get_script().
 *
 * Return value: the #PangoScript for the character.
 *
 * Since: 1.4
 **/
PangoScript
pango_script_for_unichar (gunichar ch)
{
  return g_unichar_get_script (ch);
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

/* duplicated in pango-language.c */
#define REAL_SCRIPT(script) \
  ((script) > PANGO_SCRIPT_INHERITED && (script) != PANGO_SCRIPT_UNKNOWN)

#define SAME_SCRIPT(script1, script2) \
  (!REAL_SCRIPT (script1) || !REAL_SCRIPT (script2) || (script1) == (script2))

#define IS_OPEN(pair_index) (((pair_index) & 1) == 0)

/**
 * pango_script_iter_next:
 * @iter: a #PangoScriptIter
 *
 * Advances a #PangoScriptIter to the next range. If @iter
 * is already at the end, it is left unchanged and %FALSE
 * is returned.
 *
 * Return value: %TRUE if @iter was successfully advanced.
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
