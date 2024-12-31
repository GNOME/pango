/* Pango2
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

#include <pango2/pango.h>
#include "test-common.h"

#include <hb-ot.h>

static void
test_layout (gconstpointer d)
{
  const char *filename = d;
  GError *error = NULL;
  char *diff;
  GBytes *bytes;
  char *contents;
  gsize length;
  GBytes *orig;
  Pango2Context *context;
  Pango2Layout *layout;

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
  orig = g_bytes_new_take (contents, length);

  context = pango2_context_new ();
  layout = pango2_layout_deserialize (context, orig, PANGO2_LAYOUT_DESERIALIZE_CONTEXT, &error);
  g_assert_no_error (error);

  bytes = pango2_layout_serialize (layout, PANGO2_LAYOUT_SERIALIZE_CONTEXT |
                                                 PANGO2_LAYOUT_SERIALIZE_OUTPUT);

  g_object_unref (layout);
  g_object_unref (context);

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
      GBytes *orig;
      GBytes *bytes;
      Pango2Context *context;
      Pango2Layout *layout;

      g_file_get_contents (argv[1], &contents, &length, &error);
      g_assert_no_error (error);
      orig = g_bytes_new_take (contents, length);
      context = pango2_context_new ();
      layout = pango2_layout_deserialize (context, orig, PANGO2_LAYOUT_DESERIALIZE_CONTEXT, &error);
      g_assert_no_error (error);

      bytes = pango2_layout_serialize (layout, PANGO2_LAYOUT_SERIALIZE_CONTEXT |
                                              PANGO2_LAYOUT_SERIALIZE_OUTPUT);

      g_object_unref (layout);
      g_object_unref (context);

      g_print ("%s", (const char *)g_bytes_get_data (bytes, NULL));

      g_bytes_unref (bytes);
      g_bytes_unref (orig);

      return 0;
    }

  g_test_init (&argc, &argv, NULL);
  g_test_set_nonfatal_assertions ();

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
