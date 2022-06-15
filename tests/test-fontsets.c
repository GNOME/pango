#include <string.h>
#include <locale.h>

#include <pango/pango.h>
#include "test-common.h"

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

  context = pango_context_new ();
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
  const char *name;
  char *path;

  setlocale (LC_ALL, "");

  install_fonts ();

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
