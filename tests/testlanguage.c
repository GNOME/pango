/* Pango
 * testlanguage.c: Test program for PangoLanguage
 *
 * Copyright (C) 2021 Matthias Clasen
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

#include <glib.h>
#include <pango/pango.h>

static void
test_language_to_string (void)
{
  PangoLanguage *lang;

  lang = pango_language_from_string ("ja-jp");
  g_assert_cmpstr (pango_language_to_string (lang), ==, "ja-jp");
  g_assert_cmpstr ((pango_language_to_string) (lang), ==, "ja-jp");
}

static void
test_language_env (void)
{
  if (g_test_subprocess ())
    {
      PangoLanguage **preferred;

      g_setenv ("PANGO_LANGUAGE", "de:ja", TRUE);
      g_setenv ("LANGUAGE", "fr", TRUE);

      preferred = pango_language_get_preferred ();
      g_assert_nonnull (preferred);
      g_assert_true (preferred[0] == pango_language_from_string ("de"));
      g_assert_true (preferred[1] == pango_language_from_string ("ja"));
      g_assert_null (preferred[2]);

      return;
    }

  g_test_trap_subprocess (NULL, 0, 0);
  g_test_trap_assert_passed ();
}

int
main (int argc, char *argv[])
{
  g_test_init (&argc, &argv, NULL);

  g_test_add_func ("/language/to-string", test_language_to_string);
  g_test_add_func ("/language/language-env", test_language_env);

  return g_test_run ();
}
