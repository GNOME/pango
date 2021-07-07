/* -*- mode: C; c-file-style: "gnu" -*- */
/* Pango
 * testscript.c: Test cases for PangoScriptIter
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
 * The test strings here come from ICU:
 *
 *  icu/sources/test/cintltst/cucdtst.c
 *
 ********************************************************************
 * COPYRIGHT:
 * Copyright (c) 1997-2001, International Business Machines Corporation and
 * others. All Rights Reserved.
 ********************************************************************
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

#include <stdlib.h>
#include <string.h>

#include "pango/pango-script.h"

typedef struct
{
  const char *run_text_escaped;
  char *run_text;
  PangoScript run_code;
} RunTestData;

static gchar *
unescape (const char *text)
{
  gboolean escaped = FALSE;
  GString *result = g_string_new (NULL);
  const gchar *p;

  for (p = text; *p; p = g_utf8_next_char (p))
    {
      gunichar ch = g_utf8_get_char (p);

      if (escaped)
	{
	  if (ch == 'u' || ch == 'U')
	    {
	      int n_chars = ch == 'u' ? 4 : 8;
	      int i;

	      ch = 0;
	      for (i = 0; i < n_chars; i++)
		{
		  p++;
		  ch <<= 4;
		  if (*p <= '9' && *p >= '0')
		    ch += *p - '0';
		  else if (*p <= 'F' && *p >= 'A')
		    ch += 10 + *p - 'A';
		  else if (*p <= 'f' && *p >= 'a')
		    ch += 10 + *p - 'a';
		  else
		    g_assert_not_reached ();
		}
	    }
	  else if (ch == '\\')
	    {
	      ch = '\\';
	    }
	  else
	    {
	      g_assert_not_reached ();
	    }

	  escaped = FALSE;
	}
      else
	{
	  if (ch == '\\')
	    {
	      escaped = TRUE;
	      continue;
	    }
	}

      g_string_append_unichar (result, ch);
    }

  return g_string_free (result, FALSE);
}

static void
test_script_iter (void)
{
  static RunTestData test_data[] = {
    { "\\u0020\\u0946\\u0939\\u093F\\u0928\\u094D\\u0926\\u0940\\u0020", NULL, PANGO_SCRIPT_DEVANAGARI },
    { "\\u0627\\u0644\\u0639\\u0631\\u0628\\u064A\\u0629\\u0020", NULL, PANGO_SCRIPT_ARABIC },
    { "\\u0420\\u0443\\u0441\\u0441\\u043A\\u0438\\u0439\\u0020", NULL, PANGO_SCRIPT_CYRILLIC },
    { "English (", NULL, PANGO_SCRIPT_LATIN },
    { "\\u0E44\\u0E17\\u0E22", NULL, PANGO_SCRIPT_THAI },
    { ") ", NULL, PANGO_SCRIPT_LATIN },
    { "\\u6F22\\u5B75", NULL, PANGO_SCRIPT_HAN },
    { "\\u3068\\u3072\\u3089\\u304C\\u306A\\u3068", NULL, PANGO_SCRIPT_HIRAGANA },
    { "\\u30AB\\u30BF\\u30AB\\u30CA", NULL, PANGO_SCRIPT_KATAKANA },
    { "\\U00010400\\U00010401\\U00010402\\U00010403", NULL, PANGO_SCRIPT_DESERET }
  };

  PangoScriptIter *iter;
  PangoScriptIter *iter2;
  GString *all = g_string_new (FALSE);
  char *pos;
  const char *start;
  const char *end;
  PangoScript script;
  unsigned int i;

  for (i = 0; i < G_N_ELEMENTS(test_data); i++)
    {
      test_data[i].run_text = unescape (test_data[i].run_text_escaped);
      g_string_append (all, test_data[i].run_text);
    }

  iter = pango_script_iter_new (all->str, -1);

  iter2 = g_boxed_copy (pango_script_iter_get_type (), iter);

  g_test_message ("Total length: %" G_GSIZE_FORMAT "\n", all->len);

  pos = all->str;
  for (i = 0; i < G_N_ELEMENTS(test_data); i++)
    {
      char *next_pos = pos + strlen (test_data[i].run_text);
      gboolean result;

      pango_script_iter_get_range (iter, &start, &end, &script);

      g_test_message ("Range: %d-%d: %d\n",
                      (int) (start - all->str),
                      (int) (end - all->str),
                      script);

      g_assert_true (start == pos);
      g_assert_true (end == next_pos);
      g_assert_true (script == test_data[i].run_code);

      result = pango_script_iter_next (iter);
      g_assert_true (result == (i != G_N_ELEMENTS (test_data) - 1));

      pos = next_pos;
    }

  /* Check that copying the iter worked */
  pango_script_iter_get_range (iter2, &start, &end, &script);
  g_assert_true (start == all->str);
  g_assert_true (end == all->str + strlen (test_data[0].run_text));
  g_assert_true (script == test_data[0].run_code);
  pango_script_iter_free (iter2);

  pango_script_iter_free (iter);

  /*
   * Test an empty string.
   */
  iter = pango_script_iter_new (all->str, 0);

  pango_script_iter_get_range (iter, &start, &end, &script);

  g_assert_true (start == all->str);
  g_assert_true (end == all->str);
  g_assert_true (script == PANGO_SCRIPT_COMMON);
  g_assert_true (!pango_script_iter_next (iter));

  pango_script_iter_free (iter);

  /* Cleanup */

  for (i = 0; i < G_N_ELEMENTS (test_data); i++)
    g_free (test_data[i].run_text);

  g_string_free (all, TRUE);
}

int
main (int argc, char **argv)
{
  g_test_init (&argc, &argv, NULL);

  g_test_add_func ("/script/iter", test_script_iter);

  return g_test_run ();
}
