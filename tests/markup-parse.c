/* Pango
 * markup-parse.c: Test Pango markup
 *
 * Copyright (C) 2014 Red Hat, Inc
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

#ifdef G_OS_WIN32
# include <io.h>
#else
# include <unistd.h>
#endif

#include <locale.h>

#include <pango/pangocairo.h>
#include "test-common.h"

static void
test_file (const gchar *filename, GString *string)
{
  gchar *contents;
  gsize  length;
  GError *error = NULL;
  gchar *text;
  PangoAttrList *attrs;
  PangoAttrIterator *iter;
  PangoFontDescription *desc;
  PangoLanguage *lang;
  gboolean ret;
  char *str;
  int start, end;
  gunichar accel = 0;

  g_file_get_contents (filename, &contents, &length, &error);
  g_assert_no_error (error);

  ret = pango_parse_markup (contents, length, '_', &attrs, &text, &accel, &error);

  if (ret)
    {
      g_assert_no_error (error);
      g_string_append (string, text);
      g_string_append (string, "\n\n---\n\n");
      print_attr_list (attrs, string);
      g_string_append (string, "\n\n---\n\n");
      desc = pango_font_description_new ();
      iter = pango_attr_list_get_iterator (attrs);
      do {
        pango_attr_iterator_range (iter, &start, &end);
        pango_attr_iterator_get_font (iter, desc, &lang, NULL);
        str = pango_font_description_to_string (desc);
        g_string_append_printf (string, "[%d:%d] %s %s\n", start, end, (char *)lang, str);
        g_free (str);
      } while (pango_attr_iterator_next (iter));
      pango_attr_iterator_destroy (iter);
      pango_attr_list_unref (attrs);
      pango_font_description_free (desc);
      g_free (text);

      if (accel)
        {
          g_string_append (string, "\n\n---\n\n");
          g_string_append_unichar (string, accel);
        }

      /* Check that all optional arguments can be NULL */
      ret = pango_parse_markup (contents, length, '_', NULL, NULL, NULL, NULL);
      g_assert_true (ret);
    }
  else
    {
      g_string_append_printf (string, "ERROR: %s", error->message);
      g_error_free (error);
    }

  g_free (contents);
}

static gchar *
get_expected_filename (const gchar *filename)
{
  gchar *f, *p, *expected;

  f = g_strdup (filename);
  p = strstr (f, ".markup");
  if (p)
    *p = 0;
  expected = g_strconcat (f, ".expected", NULL);

  g_free (f);

  return expected;
}

static void
test_parse (gconstpointer d)
{
  const gchar *filename = d;
  gchar *expected_file;
  GError *error = NULL;
  GString *string;
  char *diff;

  expected_file = get_expected_filename (filename);

  string = g_string_sized_new (0);

  test_file (filename, string);

  diff = diff_with_file (expected_file, string->str, string->len, &error);
  if (g_error_matches (error, G_SPAWN_ERROR, G_SPAWN_ERROR_NOENT))
    {
      g_test_skip ("diff binary not found");

      g_string_free (string, TRUE);
      g_free (expected_file);

      return;
    }

  g_assert_no_error (error);

  if (diff && diff[0])
    {
      g_test_message ("Resulting output doesn't match reference:\n%s", diff);
      g_test_fail ();
    }
  g_free (diff);

  g_string_free (string, TRUE);
  g_free (expected_file);
}

static void
test_file_incrementally (const gchar *filename, GString *string)
{
  gchar *contents;
  gsize  length;
  GError *error = NULL;
  gchar *text;
  PangoAttrList *attrs;
  PangoAttrIterator *iter;
  PangoFontDescription *desc;
  PangoLanguage *lang;
  gboolean ret;
  char *str;
  int start, end;
  gunichar accel = 0;
  GMarkupParseContext *ctx;

  g_file_get_contents (filename, &contents, &length, &error);
  g_assert_no_error (error);

  ctx = pango_markup_parser_new ('_');

  ret = TRUE;
  for (int i = 0; i < length; i++)
    {
      ret = g_markup_parse_context_parse (ctx, &contents[i], 1, &error);
      if (!ret)
        break;
    }

  g_free (contents);

  if (ret)
    {
      pango_markup_parser_finish (ctx, &attrs, &text, &accel, &error);

      g_assert_no_error (error);
      g_string_append (string, text);
      g_string_append (string, "\n\n---\n\n");
      print_attr_list (attrs, string);
      g_string_append (string, "\n\n---\n\n");
      desc = pango_font_description_new ();
      iter = pango_attr_list_get_iterator (attrs);
      do {
        pango_attr_iterator_range (iter, &start, &end);
        pango_attr_iterator_get_font (iter, desc, &lang, NULL);
        str = pango_font_description_to_string (desc);
        g_string_append_printf (string, "[%d:%d] %s %s\n", start, end, (char *)lang, str);
        g_free (str);
      } while (pango_attr_iterator_next (iter));
      pango_attr_iterator_destroy (iter);
      pango_attr_list_unref (attrs);
      pango_font_description_free (desc);
      g_free (text);

      if (accel)
        {
          g_string_append (string, "\n\n---\n\n");
          g_string_append_unichar (string, accel);
        }
    }
  else
    {
      g_string_append_printf (string, "ERROR: %s", error->message);
      g_error_free (error);
    }

  g_markup_parse_context_free (ctx);
}

static void
test_parse_incrementally (gconstpointer d)
{
  const gchar *filename = d;
  gchar *expected_file;
  GError *error = NULL;
  GString *string;
  char *diff;
  gboolean ret;

  expected_file = get_expected_filename (filename);

  string = g_string_sized_new (0);

  test_file_incrementally (filename, string);

  /* incremental parsing can affect line numbers,
   * so avoid comparing the exact error strings
   */
  if (g_str_has_prefix (string->str, "ERROR:"))
    {
      ret = file_has_prefix (expected_file, "ERROR:", &error);
      g_assert_no_error (error);
      g_assert_true (ret);
    }
  else
    {
      diff = diff_with_file (expected_file, string->str, string->len, &error);
      if (g_error_matches (error, G_SPAWN_ERROR, G_SPAWN_ERROR_NOENT))
        {
          g_test_skip ("diff binary not found");

          g_string_free (string, TRUE);
          g_free (expected_file);

          return;
        }

      g_assert_no_error (error);

      if (diff && diff[0])
        {
          g_test_message ("Resulting output doesn't match reference:\n%s", diff);
          g_test_fail ();
        }
      g_free (diff);
    }

  g_string_free (string, TRUE);

  g_free (expected_file);
}

int
main (int argc, char *argv[])
{
  GDir *dir;
  GError *error = NULL;
  const gchar *name;
  gchar *path;

  g_setenv ("LC_ALL", "en_US.UTF8", TRUE);
  setlocale (LC_ALL, "");

  /* allow to easily generate expected output for new test cases */
  if (argc > 1 && argv[1][0] != '-')
    {
      GString *string;

      string = g_string_sized_new (0);
      test_file (argv[1], string);
      g_print ("%s", string->str);

      return 0;
    }

  g_test_init (&argc, &argv, NULL);

  path = g_test_build_filename (G_TEST_DIST, "markups", NULL);
  dir = g_dir_open (path, 0, &error);
  g_free (path);
  g_assert_no_error (error);
  while ((name = g_dir_read_name (dir)) != NULL)
    {
      if (!strstr (name, "markup"))
        continue;

      path = g_strdup_printf ("/markup/parse/%s", name);
      g_test_add_data_func_full (path, g_test_build_filename (G_TEST_DIST, "markups", name, NULL),
                                 test_parse, g_free);
      g_free (path);

      path = g_strdup_printf ("/markup/parse-incrementally/%s", name);
      g_test_add_data_func_full (path, g_test_build_filename (G_TEST_DIST, "markups", name, NULL),
                                 test_parse_incrementally, g_free);
      g_free (path);
    }
  g_dir_close (dir);

  return g_test_run ();
}
