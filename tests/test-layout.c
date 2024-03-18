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
#include <pango/pangocairo-fc.h>
#include <pango/pangofc-fontmap.h>
#include "test-common.h"

static char *opt_fonts = NULL;

static PangoFontMap *
generate_font_map (void)
{
  FcConfig *config;
  PangoFontMap *map;
  char *path;
  gsize len;
  char *conf;

  map = g_object_new (PANGO_TYPE_CAIRO_FC_FONT_MAP, NULL);

  config = FcConfigCreate ();

  path = g_build_filename (opt_fonts, "fonts.conf", NULL);
  g_file_get_contents (path, &conf, &len, NULL);

  if (!FcConfigParseAndLoadFromMemory (config, (const FcChar8 *) conf, TRUE))
    g_error ("Failed to parse fontconfig configuration");

  g_free (conf);
  g_free (path);

  FcConfigAppFontAddDir (config, (const FcChar8 *) opt_fonts);
  pango_fc_font_map_set_config (PANGO_FC_FONT_MAP (map), config);
  FcConfigDestroy (config);

  return map;
}

static void
test_layout (gconstpointer d)
{
  const char *filename = d;
  PangoFontMap *fontmap;
  GError *error = NULL;
  char *diff;
  GBytes *bytes;
  char *contents;
  gsize length;
  GBytes *orig;
  PangoContext *context;
  PangoLayout *layout;

  fontmap = generate_font_map ();
  if (!PANGO_IS_FC_FONT_MAP (fontmap))
    {
      g_test_skip ("Not an fc fontmap. Skipping...");
      g_object_unref (fontmap);
      return;
    }

  char *old_locale = g_strdup (setlocale (LC_ALL, NULL));
  setlocale (LC_ALL, "en_US.UTF-8");
  if (strstr (setlocale (LC_ALL, NULL), "en_US") == NULL)
    {
      char *msg = g_strdup_printf ("Locale en_US.UTF-8 not available, skipping layout %s", filename);
      g_test_skip (msg);
      g_free (msg);
      g_free (old_locale);
      g_object_unref (fontmap);
      return;
    }

  g_file_get_contents (filename, &contents, &length, &error);
  g_assert_no_error (error);
  orig = g_bytes_new_take (contents, length);

  context = pango_font_map_create_context (fontmap);
  layout = pango_layout_deserialize (context, orig, PANGO_LAYOUT_DESERIALIZE_CONTEXT, &error);
  g_assert_no_error (error);

  bytes = pango_layout_serialize (layout, PANGO_LAYOUT_SERIALIZE_CONTEXT | PANGO_LAYOUT_SERIALIZE_OUTPUT);

  g_object_unref (layout);
  g_object_unref (context);
  g_object_unref (fontmap);

  diff = diff_bytes (orig, bytes, &error);
  g_assert_no_error (error);

  g_bytes_unref (bytes);
  g_bytes_unref (orig);

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
}

static void
generate_expected_output (const char *path)
{
  PangoFontMap *fontmap;
  char *contents;
  gsize length;
  GError *error = NULL;
  GBytes *orig;
  GBytes *bytes;
  PangoContext *context;
  PangoLayout *layout;

  fontmap = generate_font_map ();

  g_file_get_contents (path, &contents, &length, &error);
  g_assert_no_error (error);
  orig = g_bytes_new_take (contents, length);
  context = pango_font_map_create_context (fontmap);
  layout = pango_layout_deserialize (context, orig, PANGO_LAYOUT_DESERIALIZE_CONTEXT, &error);
  g_assert_no_error (error);

  bytes = pango_layout_serialize (layout, PANGO_LAYOUT_SERIALIZE_CONTEXT | PANGO_LAYOUT_SERIALIZE_OUTPUT);

  g_object_unref (layout);
  g_object_unref (context);

  g_print ("%s", (const char *)g_bytes_get_data (bytes, NULL));

  g_bytes_unref (bytes);
  g_bytes_unref (orig);
  g_object_unref (fontmap);
}

int
main (int argc, char *argv[])
{
  int result = 0;
  GDir *dir;
  GError *error = NULL;
  const gchar *name;
  char *path;
  GOptionContext *option_context;
  GOptionEntry entries[] = {
    { "fonts", 0, 0, G_OPTION_ARG_FILENAME, &opt_fonts, "Fonts to use", "DIR" },
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

  /* allow to easily generate expected output for new test cases */
  if (argc > 1 && argv[1][0] != '-')
    {
      generate_expected_output (argv[1]);
      return 0;
    }

  g_test_init (&argc, &argv, NULL);

  if (!opt_fonts)
    opt_fonts = g_test_build_filename (G_TEST_DIST, "fonts", NULL);

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

  result = g_test_run ();

  g_free (opt_fonts);

  return result;
}
