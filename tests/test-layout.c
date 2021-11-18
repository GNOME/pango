/* Pango
 * test-layout.c: Test Pango Layout
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
#include <locale.h>

#ifndef G_OS_WIN32
#include <unistd.h>
#endif

#include "config.h"
#include <pango/pangocairo.h>
#include "test-common.h"


static PangoContext *context;

static gboolean opt_show_font;

static const gchar *
enum_value_nick (GType type, gint value)
{
  GEnumClass *eclass;
  GEnumValue *ev;

  eclass = g_type_class_ref (type);
  ev = g_enum_get_value (eclass, value);
  g_type_class_unref (eclass);

  if (ev)
    return ev->value_nick;
  else
    return "?";
}

static const gchar *
direction_name (PangoDirection dir)
{
  return enum_value_nick (PANGO_TYPE_DIRECTION, dir);
}

static const gchar *
gravity_name (PangoGravity gravity)
{
  return enum_value_nick (PANGO_TYPE_GRAVITY, gravity);
}

static const gchar *
script_name (PangoScript script)
{
  return enum_value_nick (PANGO_TYPE_SCRIPT, script);
}

static gchar *
font_name (PangoFont *font)
{
  PangoFontDescription *desc;
  gchar *name;

  desc = pango_font_describe (font);
  name = pango_font_description_to_string (desc);
  pango_font_description_free (desc);

  return name;
}

static void
dump_lines (PangoLayout *layout, GString *string)
{
  PangoLayoutIter *iter;
  const gchar *text;
  gint index, index2;
  gboolean has_more;
  gchar *char_str;
  gint i;
  PangoLayoutLine *line;

  text = pango_layout_get_text (layout);
  iter = pango_layout_get_iter (layout);

  has_more = TRUE;
  index = pango_layout_iter_get_index (iter);
  index2 = 0;
  i = 0;
  while (has_more)
    {
      line = pango_layout_iter_get_line (iter);
      has_more = pango_layout_iter_next_line (iter);
      i++;

      if (has_more)
        {
          index2 = pango_layout_iter_get_index (iter);
          char_str = g_strndup (text + index, index2 - index);
        }
      else
        {
          char_str = g_strdup (text + index);
        }

      g_string_append_printf (string, "i=%d, index=%d, paragraph-start=%d, dir=%s '%s'\n",
                              i, index, line->is_paragraph_start, direction_name (line->resolved_dir),
                              char_str);
      g_free (char_str);

      index = index2;
    }
  pango_layout_iter_free (iter);
}

#define ANALYSIS_FLAGS (PANGO_ANALYSIS_FLAG_CENTERED_BASELINE | \
                        PANGO_ANALYSIS_FLAG_IS_ELLIPSIS | \
                        PANGO_ANALYSIS_FLAG_NEED_HYPHEN)

static void
dump_runs (PangoLayout *layout, GString *string)
{
  PangoLayoutIter *iter;
  PangoLayoutRun *run;
  PangoItem *item;
  const gchar *text;
  gint index;
  gboolean has_more;
  gchar *char_str;
  gint i;
  gchar *font = 0;

  text = pango_layout_get_text (layout);
  iter = pango_layout_get_iter (layout);

  has_more = TRUE;
  i = 0;
  while (has_more)
    {
      run = pango_layout_iter_get_run (iter);
      index = pango_layout_iter_get_index (iter);
      has_more = pango_layout_iter_next_run (iter);
      i++;

      if (run)
        {
          item = ((PangoGlyphItem*)run)->item;
          char_str = g_strndup (text + item->offset, item->length);
          font = font_name (item->analysis.font);
          g_string_append_printf (string, "i=%d, index=%d, chars=%d, level=%d, gravity=%s, flags=%d, font=%s, script=%s, language=%s, '%s'\n",
                                  i, index, item->num_chars, item->analysis.level,
                                  gravity_name (item->analysis.gravity),
                                  item->analysis.flags & ANALYSIS_FLAGS,
                                  opt_show_font ? font : "OMITTED", /* for some reason, this fails on build.gnome.org, so leave it out */
                                  script_name (item->analysis.script),
                                  pango_language_to_string (item->analysis.language),
                                  char_str);
          print_attributes (item->analysis.extra_attrs, string);
          g_free (font);
          g_free (char_str);
        }
      else
        {
          g_string_append_printf (string, "i=%d, index=%d, no run, line end\n",
                                  i, index);
        }
    }
  pango_layout_iter_free (iter);
}

static void
dump_directions (PangoLayout *layout, GString *string)
{
  const char *text, *p;

  text = pango_layout_get_text (layout);
  for (p = text; *p; p = g_utf8_next_char (p))
    {
      g_string_append_printf (string, "%d ", pango_layout_get_direction (layout, p - text));
    }
  g_string_append (string, "\n");
}

static void
dump_cursor_positions (PangoLayout *layout, GString *string)
{
  const char *text;
  int index, trailing;

  text = pango_layout_get_text (layout);

  index = 0;
  trailing = 0;

  while (index < G_MAXINT)
    {
      g_string_append_printf (string, "%d(%d) ", index, trailing);

      while (trailing--)
        index = g_utf8_next_char (text + index) - text;

      pango_layout_move_cursor_visually (layout, TRUE, index, 0, 1, &index, &trailing);
    }

  g_string_append (string, "\n");
}

static void
test_file (const char *filename, GString *string)
{
  char *contents;
  gsize  length;
  GBytes *bytes;
  GError *error = NULL;
  PangoLayout *layout;

  if (context == NULL)
    context = pango_font_map_create_context (pango_cairo_font_map_get_default ());

  g_file_get_contents (filename, &contents, &length, &error);
  g_assert_no_error (error);

  bytes = g_bytes_new_take (contents, length);

  layout = pango_layout_deserialize (context, bytes, &error);
  g_assert_no_error (error);

  g_bytes_unref (bytes);

  /* generate the dumps */
  g_string_append (string, pango_layout_get_text (layout));

  g_string_append (string, "\n--- parameters\n\n");

  g_string_append_printf (string, "wrapped: %d\n", pango_layout_is_wrapped (layout));
  g_string_append_printf (string, "ellipsized: %d\n", pango_layout_is_ellipsized (layout));
  g_string_append_printf (string, "lines: %d\n", pango_layout_get_line_count (layout));
  if (pango_layout_get_width (layout) > 0)
    g_string_append_printf (string, "width: %d\n", pango_layout_get_width (layout));
  if (pango_layout_get_height (layout) > 0)
    g_string_append_printf (string, "height: %d\n", pango_layout_get_height (layout));
  if (pango_layout_get_indent (layout) != 0)
    g_string_append_printf (string, "indent: %d\n", pango_layout_get_indent (layout));

  g_string_append (string, "\n--- attributes\n\n");
  print_attr_list (pango_layout_get_attributes (layout), string);

  g_string_append (string, "\n--- directions\n\n");
  dump_directions (layout, string);

  g_string_append (string, "\n--- cursor positions\n\n");
  dump_cursor_positions (layout, string);

  g_string_append (string, "\n--- lines\n\n");
  dump_lines (layout, string);

  g_string_append (string, "\n--- runs\n\n");
  dump_runs (layout, string);

  g_object_unref (layout);
}

static gchar *
get_expected_filename (const char *filename)
{
  char *f, *p, *expected;

  f = g_strdup (filename);
  p = strstr (f, ".layout");
  if (p)
    *p = 0;
  expected = g_strconcat (f, ".expected", NULL);

  g_free (f);

  return expected;
}

static void
test_layout (gconstpointer d)
{
  const char *filename = d;
  char *expected_file;
  GError *error = NULL;
  GString *dump;
  char *diff;
  PangoFontFamily **families;
  int n_families;
  gboolean found_cantarell;

  char *old_locale = g_strdup (setlocale (LC_ALL, NULL));
  setlocale (LC_ALL, "en_US.UTF-8");
  if (strstr (setlocale (LC_ALL, NULL), "en_US") == NULL)
    {
      char *msg = g_strdup_printf ("Locale en_US.UTF-8 not available, skipping layout %s", filename);
      g_test_skip (msg);
      g_free (msg);
      g_free (old_locale);
      return;
    }

  if (context == NULL)
    context = pango_font_map_create_context (pango_cairo_font_map_get_default ());

  found_cantarell = FALSE;
  pango_context_list_families (context, &families, &n_families);
  for (int i = 0; i < n_families; i++)
    {
      if (strcmp (pango_font_family_get_name (families[i]), "Cantarell") == 0)
        {
          found_cantarell = TRUE;
          break;
        }
    }
  g_free (families);

  if (!found_cantarell)
    {
      char *msg = g_strdup_printf ("Cantarell font not available, skipping itemization %s", filename);
      g_test_skip (msg);
      g_free (msg);
      g_free (old_locale);
      return;
    }

  expected_file = get_expected_filename (filename);

  dump = g_string_sized_new (0);

  test_file (filename, dump);

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
  char *path;
  GOptionContext *option_context;
  GOptionEntry entries[] = {
    { "show-fonts", '0', 0, G_OPTION_ARG_NONE, &opt_show_font, "Print font names in dumps", NULL },
    { NULL, 0 },
  };

  setlocale (LC_ALL, "");

  option_context = g_option_context_new ("");
  g_option_context_add_main_entries (option_context, entries, NULL);
  g_option_context_set_ignore_unknown_options (option_context, TRUE);
  if (!g_option_context_parse (option_context, &argc, &argv, &error))
    {
      g_error ("failed to parse options: %s", error->message);
      return 1;
    }
  g_option_context_free (option_context);

  if (g_getenv ("PANGO_TEST_SHOW_FONT"))
    opt_show_font = TRUE;

  /* allow to easily generate expected output for new test cases */
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

  path = g_test_build_filename (G_TEST_DIST, "layouts", NULL);
  dir = g_dir_open (path, 0, &error);
  g_free (path);
  g_assert_no_error (error);
  while ((name = g_dir_read_name (dir)) != NULL)
    {
      if (!g_str_has_suffix (name, ".layout"))
        continue;

      path = g_strdup_printf ("/layout/%s", name);
      g_test_add_data_func_full (path, g_test_build_filename (G_TEST_DIST, "layouts", name, NULL),
                                 test_layout, g_free);
      g_free (path);
    }
  g_dir_close (dir);

  return g_test_run ();
}
