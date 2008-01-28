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
 */

#define PANGO_ENABLE_ENGINE
#include <pango/pango-ot.h>

#undef VERBOSE

#define ASSERT(stmt) G_STMT_START {					\
    if (stmt) { }							\
    else								\
      {									\
	g_warning ("%s:%d (%s): assertion '%s' failed",			\
		 __FILE__, __LINE__, G_STRFUNC, #stmt);			\
	exit (1);							\
      }									\
} G_STMT_END

static void
test_script_tags (void)
{
  gunichar ch;
  PangoScript i, max_script;

  /* we need to know what the maximum script number is.  but we don't
   * provide an api for that.  instead of looking into internal tables,
   * we'll go over all chars and see what their script is, taking the max!
   */

  max_script = PANGO_SCRIPT_INVALID_CODE;
  for (ch = 0; ch <= 0x10FFFF; ch++)
    max_script = MAX (max_script, pango_script_for_unichar (ch));

  for (i = PANGO_SCRIPT_COMMON; i <= max_script; i++)
    {
      PangoOTTag tag = pango_ot_tag_from_script (i);
      PangoScript j  = pango_ot_tag_to_script (tag);

      if (i <= PANGO_SCRIPT_INHERITED || i == PANGO_SCRIPT_UNKNOWN)
        {
	  ASSERT (tag == PANGO_OT_TAG_DEFAULT_SCRIPT);
	  ASSERT (j == PANGO_SCRIPT_COMMON);
        }
      else if (tag == FT_MAKE_TAG ('k', 'a', 'n', 'a'))
        {
	  /* Hiragana and Katakana both map to tag 'kana' */
	 ASSERT (i == PANGO_SCRIPT_HIRAGANA || i == PANGO_SCRIPT_KATAKANA);
	 ASSERT (j == PANGO_SCRIPT_HIRAGANA || j == PANGO_SCRIPT_KATAKANA);
	}
      else
        {
	  if (j != i)
	    g_error ("Got back %d for script %d (OT tag '%c%c%c%c')", j, i,
		     tag>>24, (tag>>16)&255, (tag>>8)&255, tag&255);
	}
    }

  ASSERT (pango_ot_tag_to_script (FT_MAKE_TAG ('X', 'Y', 'Z', ' ')) == PANGO_SCRIPT_UNKNOWN);
}

static void
test_language_tags (void)
{
  /* just test it for a few known languages to make sure it's working */
  const char languages[][6] = {
    "xy", /* hopefully nonexistent */
    "aa",
    "az_IR",
    "en",
    "en_US",
    "fa",
    "fa_IR",
    "fr",
    "zh_CN",
    "zu"
  };
  unsigned int i;

  for (i = 0; i < G_N_ELEMENTS (languages); i++)
    {
      PangoLanguage *l = pango_language_from_string (languages[i]);
      PangoOTTag tag   = pango_ot_tag_from_language (l);
      PangoLanguage *m = pango_ot_tag_to_language (tag);

      if (i == 0)
        {
	  ASSERT (tag == PANGO_OT_TAG_DEFAULT_LANGUAGE);
	  ASSERT (strcmp (pango_language_to_string (m), "xx") == 0);
	}
      else
        {
	  if (tag == PANGO_OT_TAG_DEFAULT_LANGUAGE)
	    g_error ("Got PANGO_OT_TAG_DEFAULT_LANGUAGE for language '%s'", pango_language_to_string (l));

	  if (!pango_language_matches (l, pango_language_to_string (m)))
	    g_error ("Got back %s for language %s (OT tag '%c%c%c%c')",
		     pango_language_to_string (m), pango_language_to_string (l),
		     tag>>24, (tag>>16)&255, (tag>>8)&255, tag&255);
	}
    }
}

int
main (int argc, char **argv)
{
  g_setenv ("PANGO_RC_FILE", "./pangorc", TRUE);

  test_script_tags ();
  test_language_tags ();

  return 0;
}
