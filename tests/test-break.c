/* Pango
 * test-break.c: Test Pango line breaking
 *
 * Copyright (C) 2019 Red Hat, Inc
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
#include <string.h>
#include <locale.h>

#ifndef G_OS_WIN32
#include <unistd.h>
#endif

#include "config.h"
#include <pango/pangocairo.h>
#include "test-common.h"


static PangoContext *context;

static void
test_file (const gchar *filename, GString *string)
{
  gchar *contents;
  gsize  length;
  GError *error = NULL;
  PangoLogAttr *attrs;
  PangoLanguage *lang;
  int len;
  char *p;
  int i;
  GString *s2;

  if (!g_file_get_contents (filename, &contents, &length, &error))
    {
      fprintf (stderr, "%s\n", error->message);
      g_error_free (error);
      return;
    }

  len = g_utf8_strlen (contents, -1) + 1;
  attrs = g_new (PangoLogAttr, len);

  lang = pango_language_from_string ("en");

  pango_get_log_attrs (contents, length, -1, lang, attrs, len);

  s2 = g_string_new ("");

  for (i = 0, p = contents; i < len; i++, p = g_utf8_next_char (p))
    {
      PangoLogAttr log = attrs[i];
      if (log.is_mandatory_break)
        {
          g_string_append (string, " ");
          g_string_append (s2, "L");
        }
      else if (log.is_line_break)
        {
          g_string_append (string, " ");
          g_string_append (s2, "l");
        }
      if (log.is_char_break)
        {
          g_string_append (string, " ");
          g_string_append (s2, "c");
        }
      if (log.is_white)
        {
          g_string_append (string, " ");
          g_string_append (s2, "w");
        }
      if (log.is_expandable_space)
        {
          g_string_append (string, " ");
          g_string_append (s2, "x");
        }
      if (i < len - 1)
        {
          gunichar ch = g_utf8_get_char (p);
          if (ch == 0x20)
            {
              g_string_append (string, "[ ]");
              g_string_append (s2, "   ");
            }
          else if (g_unichar_isprint (ch))
            {
              g_string_append_unichar (string, ch);
              g_string_append (s2, " ");
            }
          else
            {
              g_string_append_printf (string, "[%#04x]", ch);
              g_string_append (s2, "      ");
            }
        }
    }
  g_string_append (string, "\n");
  g_string_append_len (string, s2->str, s2->len);
  g_string_append (string, "\n");
  g_string_free (s2, TRUE);

  g_free (attrs);
  g_free (contents);
}

static gchar *
get_expected_filename (const gchar *filename)
{
  gchar *f, *p, *expected;

  f = g_strdup (filename);
  p = strstr (f, ".break");
  if (p)
    *p = 0;
  expected = g_strconcat (f, ".expected", NULL);

  g_free (f);

  return expected;
}

static void
test_break (gconstpointer d)
{
  const gchar *filename = d;
  gchar *expected_file;
  GError *error = NULL;
  GString *dump;
  gchar *diff;

  expected_file = get_expected_filename (filename);

  dump = g_string_sized_new (0);

  test_file (filename, dump);

  //diff = diff_with_file (expected_file, dump->str, dump->len, &error);
  g_assert_no_error (error);

  if (diff && diff[0])
    {
      g_printerr ("Contents don't match expected contents:\n%s", diff);
      g_test_fail ();
      g_free (diff);
    }

  g_string_free (dump, TRUE);
  g_free (expected_file);
}

int
main (int argc, char *argv[])
{
  GDir *dir;
  GError *error = NULL;
  const gchar *name;
  gchar *path;

  g_setenv ("LC_ALL", "en_US.UTF-8", TRUE);
  setlocale (LC_ALL, "");

  g_test_init (&argc, &argv, NULL);

  context = pango_context_new ();

  /* allow to easily generate expected output for new test cases */
  if (argc > 1)
    {
      GString *string;

      string = g_string_sized_new (0);
      test_file (argv[1], string);
      printf ("%s", string->str);

      return 0;
    }

  path = g_test_build_filename (G_TEST_DIST, "breaks", NULL);
  dir = g_dir_open (path, 0, &error);
  g_free (path);
  g_assert_no_error (error);
  while ((name = g_dir_read_name (dir)) != NULL)
    {
      if (!strstr (name, "break"))
        continue;

      path = g_strdup_printf ("/break/%s", name);
      g_test_add_data_func_full (path, g_test_build_filename (G_TEST_DIST, "breaks", name, NULL),
                                 test_break, g_free);
      g_free (path);
    }
  g_dir_close (dir);

  return g_test_run ();
}
