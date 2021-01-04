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

static gboolean
test_file (const gchar *filename, GString *string)
{
  gchar *contents;
  gsize  length;
  GError *error = NULL;
  PangoLogAttr *attrs;
  int len;
  char *p;
  int i;
  GString *s1, *s2, *s3, *s4;
  int m;
  char *test;
  char *text;
  PangoAttrList *attributes;
  PangoLayout *layout;

  g_file_get_contents (filename, &contents, &length, &error);
  g_assert_no_error (error);

  test = contents;

  /* Skip initial comments */
  while (test[0] == '#')
    test = strchr (test, '\n') + 1;

  length = strlen (test);
  len = g_utf8_strlen (test, -1) + 1;

  pango_parse_markup (test, -1, 0, &attributes, &text, NULL, &error);
  g_assert_no_error (error);

  layout = pango_layout_new (context);
  pango_layout_set_text (layout, text, length);
  pango_layout_set_attributes (layout, attributes);

  if (pango_layout_get_unknown_glyphs_count (layout) > 0)
    {
      char *msg = g_strdup_printf ("Missing glyphs - skipping %s. Maybe fonts are missing?", filename);
      g_test_skip (msg);
      g_free (msg);
      g_free (contents);
      g_object_unref (layout);
      pango_attr_list_unref (attributes);
      g_free (text);
      return FALSE;
    }

  pango_layout_get_log_attrs (layout, &attrs, &len);

  s1 = g_string_new ("Breaks: ");
  s2 = g_string_new ("Whitespace: ");
  s3 = g_string_new ("Words:");
  s4 = g_string_new ("Sentences:");

  g_string_append (string, "Text: ");

  m = MAX (MAX (s1->len, s2->len), MAX (s3->len, s4->len));

  g_string_append_printf (s1, "%*s", (int)(m - s1->len), "");
  g_string_append_printf (s2, "%*s", (int)(m - s2->len), "");
  g_string_append_printf (s3, "%*s", (int)(m - s3->len), "");
  g_string_append_printf (s4, "%*s", (int)(m - s4->len), "");
  g_string_append_printf (string, "%*s", (int)(m - strlen ("Text: ")), "");

  for (i = 0, p = text; i < len; i++, p = g_utf8_next_char (p))
    {
      PangoLogAttr log = attrs[i];
      int b = 0;
      int w = 0;
      int o = 0;
      int s = 0;

      if (log.is_mandatory_break)
        {
          g_string_append (s1, "L");
          b++;
        }
      else if (log.is_line_break)
        {
          g_string_append (s1, "l");
          b++;
        }
      if (log.is_char_break)
        {
          g_string_append (s1, "c");
          b++;
        }

      if (log.is_expandable_space)
        {
          g_string_append (s2, "x");
          w++;
        }
      else if (log.is_white)
        {
          g_string_append (s2, "w");
          w++;
        }

      if (log.is_word_boundary)
        {
          g_string_append (s3, "b");
          o++;
        }
      if (log.is_word_start)
        {
          g_string_append (s3, "s");
          o++;
        }
      if (log.is_word_end)
        {
          g_string_append (s3, "e");
          o++;
        }

      if (log.is_sentence_boundary)
        {
          g_string_append (s4, "b");
          s++;
        }
      if (log.is_sentence_start)
        {
          g_string_append (s4, "s");
          s++;
        }
      if (log.is_sentence_end)
        {
          g_string_append (s4, "e");
          s++;
        }

      m = MAX (MAX (b, w), MAX (o, s));

      g_string_append_printf (string, "%*s", m, "");
      g_string_append_printf (s1, "%*s", m - b, "");
      g_string_append_printf (s2, "%*s", m - w, "");
      g_string_append_printf (s3, "%*s", m - o, "");
      g_string_append_printf (s4, "%*s", m - s, "");

      if (i < len - 1)
        {
          gunichar ch = g_utf8_get_char (p);
          if (ch == 0x20)
            {
              g_string_append (string, "[ ]");
              g_string_append (s1, "   ");
              g_string_append (s2, "   ");
              g_string_append (s3, "   ");
              g_string_append (s4, "   ");
            }
          else if (g_unichar_isgraph (ch) &&
                   !(g_unichar_type (ch) == G_UNICODE_LINE_SEPARATOR ||
                     g_unichar_type (ch) == G_UNICODE_PARAGRAPH_SEPARATOR))
            {
              g_string_append_unichar (string, ch);
              g_string_append (s1, " ");
              g_string_append (s2, " ");
              g_string_append (s3, " ");
              g_string_append (s4, " ");
            }
          else
            {
              char *str = g_strdup_printf ("[%#04x]", ch);
              g_string_append (string, str);
              g_string_append_printf (s1, "%*s", (int)strlen (str), "");
              g_string_append_printf (s2, "%*s", (int)strlen (str), "");
              g_string_append_printf (s3, "%*s", (int)strlen (str), "");
              g_string_append_printf (s4, "%*s", (int)strlen (str), "");
              g_free (str);
            }
        }
    }
  g_string_append (string, "\n");
  g_string_append_len (string, s1->str, s1->len);
  g_string_append (string, "\n");
  g_string_append_len (string, s2->str, s2->len);
  g_string_append (string, "\n");
  g_string_append_len (string, s3->str, s3->len);
  g_string_append (string, "\n");
  g_string_append_len (string, s4->str, s4->len);
  g_string_append (string, "\n");

  g_string_free (s1, TRUE);
  g_string_free (s2, TRUE);
  g_string_free (s3, TRUE);
  g_string_free (s4, TRUE);

  g_object_unref (layout);
  g_free (attrs);
  g_free (contents);
  g_free (text);
  pango_attr_list_unref (attributes);

  return TRUE;
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

  char *old_locale = g_strdup (setlocale (LC_ALL, NULL));
  setlocale (LC_ALL, "en_US.utf8");
  if (strstr (setlocale (LC_ALL, NULL), "en_US") == NULL)
    {
      char *msg = g_strdup_printf ("Locale en_US.UTF-8 not available, skipping break %s", filename);
      g_test_skip (msg);
      g_free (msg);
      g_free (old_locale);
      return;
    }

  dump = g_string_sized_new (0);

  if (!test_file (filename, dump))
    {
      g_free (old_locale);
      g_string_free (dump, TRUE);
      return;
    }

  expected_file = get_expected_filename (filename);

  diff = diff_with_file (expected_file, dump->str, dump->len, &error);
  g_assert_no_error (error);

  setlocale (LC_ALL, old_locale);
  g_free (old_locale);

  if (diff && diff[0])
    {
      char **lines = g_strsplit (diff, "\n", -1);
      const char *line;
      int i = 0;

      g_test_message ("Contents don't match expected contents");

      for (line = lines[0]; line != NULL; line = lines[++i])
        g_test_message ("%s", line);

      g_test_fail ();

      g_strfreev (lines);
    }

  g_free (diff);
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

  g_test_init (&argc, &argv, NULL);

  setlocale (LC_ALL, "");

  context = pango_font_map_create_context (pango_cairo_font_map_get_default ());

  /* allow to easily generate expected output for new test cases */
  if (argc > 1)
    {
      if (strcmp (argv[1], "--help") == 0)
        {
          g_print ("test-break uses the following symbols for log attrs\n\n");
          g_print ("Breaks:                 Words:\n"
                   " L - mandatory break     b - word boundary\n"
                   " l - line break          s - word start\n"
                   " c - char break          e - word end\n"
                   "\n"
                   "Whitespace:             Sentences:\n"
                   " x - expandable space    s - sentence start\n"
                   " w - whitespace          e - sentence end\n");
        }
      else
        {
          GString *string;

          string = g_string_sized_new (0);
          test_file (argv[1], string);
          g_print ("%s", string->str);
        }

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

#ifndef HAVE_LIBTHAI
      /* four.break involves Thai, so only test it when we have libthai */
      if (strstr (name, "four.break"))
        continue;
#endif

      path = g_strdup_printf ("/break/%s", name);
      g_test_add_data_func_full (path, g_test_build_filename (G_TEST_DIST, "breaks", name, NULL),
                                 test_break, g_free);
      g_free (path);
    }
  g_dir_close (dir);

  return g_test_run ();
}
