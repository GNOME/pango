#include <string.h>
#include <locale.h>

#include <pango/pango.h>
#include <pango/pangocairo-fc.h>
#include <pango/pangofc-fontmap.h>
#include "test-common.h"

static void
install_fonts (const char *dir)
{
  FcConfig *config;
  PangoFontMap *map;
  char *path;
  gsize len;
  char *conf;

  map = g_object_new (PANGO_TYPE_CAIRO_FC_FONT_MAP, NULL);

  config = FcConfigCreate ();

  path = g_build_filename (dir, "fonts.conf", NULL);
  g_file_get_contents (path, &conf, &len, NULL);

  if (!FcConfigParseAndLoadFromMemory (config, (const FcChar8 *) conf, TRUE))
    g_error ("Failed to parse fontconfig configuration");

  g_free (conf);
  g_free (path);

  FcConfigAppFontAddDir (config, (const FcChar8 *) dir);
  pango_fc_font_map_set_config (PANGO_FC_FONT_MAP (map), config);
  FcConfigDestroy (config);

  pango_cairo_font_map_set_default (PANGO_CAIRO_FONT_MAP (map));

  g_object_unref (map);
}

static gboolean
append_one (PangoFontset *fonts,
            PangoFont    *font,
            gpointer      data)
{
  PangoFontDescription *desc;
  GString *str = data;
  char *s;

  desc = pango_font_describe (font);
  s = pango_font_description_to_string (desc);

  g_string_append_printf (str, "%s\n", s);

  g_free (s);
  pango_font_description_free (desc);

  return FALSE;
}

static char *
list_fonts (const char *contents)
{
  char *p, *s;
  PangoFontDescription *desc;
  PangoContext *context;
  PangoFontset *fonts;
  GString *str;

  p = strchr (contents, '\n');
  if (p)
    s = g_strndup (contents, p - contents);
  else
    s = g_strdup (contents);

  desc = pango_font_description_from_string (s);

  context = pango_font_map_create_context (pango_cairo_font_map_get_default ());
  fonts = pango_context_load_fontset (context, desc, pango_language_get_default ());

  str = g_string_new (s);
  g_string_append (str, "\n\n");

  pango_fontset_foreach (fonts, append_one, str);

  g_object_unref (fonts);
  g_object_unref (context);
  pango_font_description_free (desc);

  g_free (s);

  return g_string_free (str, FALSE);
}

static void
test_fontset (gconstpointer d)
{
  const char *filename = d;
  GError *error = NULL;
  char *diff;
  GBytes *bytes;
  char *contents;
  gsize length;
  char *s;
  GBytes *orig;

  if (!PANGO_IS_FC_FONT_MAP (pango_cairo_font_map_get_default ()))
    {
      g_test_skip ("Not an fc fontmap. Skipping...");
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
      return;
    }

  g_file_get_contents (filename, &contents, &length, &error);
  g_assert_no_error (error);

  s = list_fonts (contents);

  orig = g_bytes_new_take (contents, length);
  bytes = g_bytes_new_take (s, strlen (s));

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

int
main (int argc, char *argv[])
{
  GDir *dir;
  GError *error = NULL;
  char *opt_fonts = NULL;
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

  if (opt_fonts)
    {
      install_fonts (opt_fonts);
      g_free (opt_fonts);
    }

 /* allow to easily generate expected output for new test cases */
  if (argc > 1 && argv[1][0] != '-')
    {
      char *contents;
      gsize length;
      GError *error = NULL;
      char *s;

      g_file_get_contents (argv[1], &contents, &length, &error);
      g_assert_no_error (error);

      s = list_fonts (contents);

      g_print ("%s", s);

      g_free (s);
      g_free (contents);

      return 0;
    }

  g_test_init (&argc, &argv, NULL);

  if (!opt_fonts)
    {
      path = g_test_build_filename (G_TEST_DIST, "fonts", NULL);
      install_fonts (path);
      g_free (path);
    }

  path = g_test_build_filename (G_TEST_DIST, "fontsets", NULL);
  dir = g_dir_open (path, 0, &error);
  g_free (path);
  g_assert_no_error (error);
  while ((name = g_dir_read_name (dir)) != NULL)
    {
      path = g_strdup_printf ("/fontsets/%s", name);
      g_test_add_data_func_full (path, g_test_build_filename (G_TEST_DIST, "fontsets", name, NULL),
                                 test_fontset, g_free);
      g_free (path);
    }
  g_dir_close (dir);

  return g_test_run ();
}
