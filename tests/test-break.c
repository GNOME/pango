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
#include "validate-log-attrs.h"


static gboolean opt_hex_chars;

static gboolean
test_file (const gchar *filename, GString *string)
{
  gchar *contents;
  gsize  length;
  GError *error = NULL;
  PangoFontMap *fontmap;
  PangoContext *context;
  PangoLogAttr *attrs;
  const PangoLogAttr *attrs2;
  int len;
  int len2;
  char *p;
  int i;
  GString *s1, *s2, *s3, *s4, *s5, *s6;
  int m;
  char *test;
  char *text;
  PangoAttrList *attributes;
  PangoLayout *layout;
  PangoLayout *layout2;

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

  fontmap = pango_cairo_font_map_new ();
  context = pango_font_map_create_context (fontmap);
  layout = pango_layout_new (context);
  pango_layout_set_text (layout, text, length);
  pango_layout_set_attributes (layout, attributes);

#if 0
  if (pango_layout_get_unknown_glyphs_count (layout) > 0)
    {
      char *msg = g_strdup_printf ("Missing glyphs - skipping %s. Maybe fonts are missing?", filename);
      if (g_test_initialized())
        g_test_skip (msg);
      else
        g_warning ("%s", msg);
      g_free (msg);
      g_free (contents);
      g_object_unref (layout);
      pango_attr_list_unref (attributes);
      g_free (text);
      g_object_unref (context);
      g_object_unref (fontmap);
      return FALSE;
    }
#endif

  pango_layout_get_log_attrs (layout, &attrs, &len);
  attrs2 = pango_layout_get_log_attrs_readonly (layout, &len2);

  g_assert_cmpint (len, ==, len2);
  g_assert_true (memcmp (attrs, attrs2, sizeof (PangoLogAttr) * len) == 0);
  if (!pango_validate_log_attrs (text, length, attrs, len, &error))
    {
      g_warning ("%s: Log attrs invalid: %s", filename, error->message);
//      g_assert_not_reached ();
    }

  layout2 = pango_layout_copy (layout);
  attrs2 = pango_layout_get_log_attrs_readonly (layout2, &len2);

  g_assert_cmpint (len, ==, len2);
  g_assert_true (memcmp (attrs, attrs2, sizeof (PangoLogAttr) * len) == 0);

  g_object_unref (layout2);

  s1 = g_string_new ("Breaks: ");
  s2 = g_string_new ("Whitespace: ");
  s3 = g_string_new ("Sentences:");
  s4 = g_string_new ("Words:");
  s5 = g_string_new ("Graphemes:");
  s6 = g_string_new ("Hyphens:");

  g_string_append (string, "Text: ");

  m = MAX (MAX (MAX (s1->len, s2->len), MAX (s3->len, s4->len)), s5->len);

  g_string_append_printf (s1, "%*s", (int)(m - s1->len), "");
  g_string_append_printf (s2, "%*s", (int)(m - s2->len), "");
  g_string_append_printf (s3, "%*s", (int)(m - s3->len), "");
  g_string_append_printf (s4, "%*s", (int)(m - s4->len), "");
  g_string_append_printf (s5, "%*s", (int)(m - s5->len), "");
  g_string_append_printf (s6, "%*s", (int)(m - s6->len), "");
  g_string_append_printf (string, "%*s", (int)(m - strlen ("Text: ")), "");

  for (i = 0, p = text; i < len; i++, p = g_utf8_next_char (p))
    {
      PangoLogAttr log = attrs[i];
      int b = 0;
      int w = 0;
      int o = 0;
      int s = 0;
      int g = 0;
      int h = 0;

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

      if (log.is_sentence_boundary)
        {
          g_string_append (s3, "b");
          s++;
        }
      if (log.is_sentence_start)
        {
          g_string_append (s3, "s");
          s++;
        }
      if (log.is_sentence_end)
        {
          g_string_append (s3, "e");
          s++;
        }

      if (log.is_word_boundary)
        {
          g_string_append (s4, "b");
          o++;
        }
      if (log.is_word_start)
        {
          g_string_append (s4, "s");
          o++;
        }
      if (log.is_word_end)
        {
          g_string_append (s4, "e");
          o++;
        }

      if (log.is_cursor_position)
        {
          g_string_append (s5, "b");
          g++;
        }

      if (log.break_removes_preceding)
        {
          g_string_append (s6, "r");
          h++;
        }
      if (log.break_inserts_hyphen)
        {
          g_string_append (s6, "i");
          h++;
        }

      m = MAX (MAX (MAX (b, w), MAX (o, s)), MAX (g, h));

      g_string_append_printf (string, "%*s", m, "");
      g_string_append_printf (s1, "%*s", m - b, "");
      g_string_append_printf (s2, "%*s", m - w, "");
      g_string_append_printf (s3, "%*s", m - s, "");
      g_string_append_printf (s4, "%*s", m - o, "");
      g_string_append_printf (s5, "%*s", m - g, "");
      g_string_append_printf (s6, "%*s", m - h, "");

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
              g_string_append (s5, "   ");
              g_string_append (s6, "   ");
            }
          else if (!opt_hex_chars &&
                   g_unichar_isgraph (ch) &&
                   !(g_unichar_type (ch) == G_UNICODE_LINE_SEPARATOR ||
                     g_unichar_type (ch) == G_UNICODE_PARAGRAPH_SEPARATOR))
            {
              g_string_append_unichar (string, 0x2066); // LRI
              g_string_append_unichar (string, ch);
              g_string_append_unichar (string, 0x2069); // PDI
              g_string_append (s1, " ");
              g_string_append (s2, " ");
              g_string_append (s3, " ");
              g_string_append (s4, " ");
              g_string_append (s5, " ");
              g_string_append (s6, " ");
            }
          else
            {
              char *str = g_strdup_printf ("[%#04x]", ch);
              g_string_append (string, str);
              g_string_append_printf (s1, "%*s", (int)strlen (str), "");
              g_string_append_printf (s2, "%*s", (int)strlen (str), "");
              g_string_append_printf (s3, "%*s", (int)strlen (str), "");
              g_string_append_printf (s4, "%*s", (int)strlen (str), "");
              g_string_append_printf (s5, "%*s", (int)strlen (str), "");
              g_string_append_printf (s6, "%*s", (int)strlen (str), "");
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
  g_string_append_len (string, s5->str, s5->len);
  g_string_append (string, "\n");
  g_string_append_len (string, s6->str, s6->len);
  g_string_append (string, "\n");

  g_string_free (s1, TRUE);
  g_string_free (s2, TRUE);
  g_string_free (s3, TRUE);
  g_string_free (s4, TRUE);
  g_string_free (s5, TRUE);
  g_string_free (s6, TRUE);

  g_object_unref (layout);
  g_free (attrs);
  g_free (contents);
  g_free (text);
  pango_attr_list_unref (attributes);
  g_object_unref (context);
  g_object_unref (fontmap);

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
  setlocale (LC_ALL, "en_US.UTF-8");
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
  gboolean opt_legend = 0;
  GOptionContext *option_context;
  GOptionEntry entries[] = {
    { "hex-chars", 0, 0, G_OPTION_ARG_NONE, &opt_hex_chars, "Print all chars in hex", NULL },
    { "legend", 0, 0, G_OPTION_ARG_NONE, &opt_legend, "Explain the output", NULL },
    { NULL, 0 },
  };

  option_context = g_option_context_new ("");
  g_option_context_add_main_entries (option_context, entries, NULL);
  g_option_context_set_ignore_unknown_options (option_context, TRUE);
  if (!g_option_context_parse (option_context, &argc, &argv, &error))
    {
      g_error ("failed to parse options: %s", error->message);
      return 1;
    }
  g_option_context_free (option_context);

  setlocale (LC_ALL, "");

  if (opt_legend)
    {
      g_print ("test-break uses the following symbols for log attrs\n\n");
      g_print ("Breaks:                 Words:                  Graphemes:\n"
               " L - mandatory break     b - word boundary       b - grapheme boundary\n"
               " l - line break          s - word start\n"
               " c - char break          e - word end\n"
               "\n"
               "Whitespace:             Sentences:              Hyphens:\n"
               " x - expandable space    b - sentence boundary   i - insert hyphen\n"
               " w - whitespace          s - sentence start      r - remove preceding\n"
               "                         e - sentence end\n");
      return 0;
    }

  if (argc > 1 && argv[1][0] != '-')
    {
      GString *string;

      string = g_string_sized_new (0);
      test_file (argv[1], string);
      g_print ("%s", string->str);

      g_string_free (string, TRUE);

      return 0;
    }

  g_test_init (&argc, &argv, NULL);

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
