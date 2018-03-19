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

static void
print_attr (PangoAttribute *attr, GString *string)
{
  g_string_append_printf (string, "[%d %d] ", attr->start_index, attr->end_index);
  switch (attr->klass->type)
    {
    case PANGO_ATTR_LANGUAGE:
      g_string_append_printf (string,"language %s\n", pango_language_to_string (((PangoAttrLanguage *)attr)->value));
      break;
    case PANGO_ATTR_FAMILY:
      g_string_append_printf (string,"family %s\n", ((PangoAttrString *)attr)->value);
      break;
    case PANGO_ATTR_STYLE:
      g_string_append_printf (string,"style %d\n", ((PangoAttrInt *)attr)->value);
      break;
    case PANGO_ATTR_WEIGHT:
      g_string_append_printf (string,"weight %d\n", ((PangoAttrInt *)attr)->value);
      break;
    case PANGO_ATTR_VARIANT:
      g_string_append_printf (string,"variant %d\n", ((PangoAttrInt *)attr)->value);
      break;
    case PANGO_ATTR_STRETCH:
      g_string_append_printf (string,"stretch %d\n", ((PangoAttrInt *)attr)->value);
      break;
    case PANGO_ATTR_SIZE:
      g_string_append_printf (string,"size %d\n", ((PangoAttrSize *)attr)->size);
      break;
    case PANGO_ATTR_FONT_DESC:
      g_string_append_printf (string,"font %s\n", pango_font_description_to_string (((PangoAttrFontDesc *)attr)->desc));
      break;
    case PANGO_ATTR_FOREGROUND:
      g_string_append_printf (string,"foreground %s\n", pango_color_to_string (&((PangoAttrColor *)attr)->color));
      break;
    case PANGO_ATTR_BACKGROUND:
      g_string_append_printf (string,"background %s\n", pango_color_to_string (&((PangoAttrColor *)attr)->color));
      break;
    case PANGO_ATTR_UNDERLINE:
      g_string_append_printf (string,"underline %d\n", ((PangoAttrInt *)attr)->value);
      break;
    case PANGO_ATTR_STRIKETHROUGH:
      g_string_append_printf (string,"strikethrough %d\n", ((PangoAttrInt *)attr)->value);
      break;
    case PANGO_ATTR_RISE:
      g_string_append_printf (string,"rise %d\n", ((PangoAttrInt *)attr)->value);
      break;
    case PANGO_ATTR_SHAPE:
      g_string_append_printf (string,"shape\n");
      break;
    case PANGO_ATTR_SCALE:
      g_string_append_printf (string,"scale %f\n", ((PangoAttrFloat *)attr)->value);
      break;
    case PANGO_ATTR_FALLBACK:
      g_string_append_printf (string,"fallback %d\n", ((PangoAttrInt *)attr)->value);
      break;
    case PANGO_ATTR_LETTER_SPACING:
      g_string_append_printf (string,"letter-spacing %d\n", ((PangoAttrInt *)attr)->value);
      break;
    case PANGO_ATTR_UNDERLINE_COLOR:
      g_string_append_printf (string,"underline-color %s\n", pango_color_to_string (&((PangoAttrColor *)attr)->color));
      break;
    case PANGO_ATTR_STRIKETHROUGH_COLOR:
      g_string_append_printf (string,"strikethrough-color %s\n", pango_color_to_string (&((PangoAttrColor *)attr)->color));
      break;
    case PANGO_ATTR_ABSOLUTE_SIZE:
      g_string_append_printf (string,"absolute-size %d\n", ((PangoAttrSize *)attr)->size);
      break;
    case PANGO_ATTR_GRAVITY:
      g_string_append_printf (string,"gravity %d\n", ((PangoAttrInt *)attr)->value);
      break;
    case PANGO_ATTR_GRAVITY_HINT:
      g_string_append_printf (string,"gravity-hint %d\n", ((PangoAttrInt *)attr)->value);
      break;
    case PANGO_ATTR_FONT_FEATURES:
      g_string_append_printf (string,"font-features %s\n", ((PangoAttrString *)attr)->value);
      break;
    case PANGO_ATTR_FOREGROUND_ALPHA:
      g_string_append_printf (string,"foreground-alpha %04x\n", ((PangoAttrInt *)attr)->value);
      break;
    case PANGO_ATTR_BACKGROUND_ALPHA:
      g_string_append_printf (string,"background-alpha %04x\n", ((PangoAttrInt *)attr)->value);
      break;
    default:
      g_assert_not_reached ();
      break;
    }
}

static void
attr_list_dump (PangoAttrList *attrs, GString *string)
{
  PangoAttrIterator *iter;

  iter = pango_attr_list_get_iterator (attrs);
  do {
    gint start, end;
    GSList *list, *l;

    pango_attr_iterator_range (iter, &start, &end);
    g_string_append_printf (string, "range %d %d\n", start, end);
    list = pango_attr_iterator_get_attrs (iter);
    for (l = list; l; l = l->next)
      {
        PangoAttribute *attr = l->data;
        print_attr (attr, string);
      }
    g_slist_free_full (list, (GDestroyNotify)pango_attribute_destroy);
  } while (pango_attr_iterator_next (iter));

  pango_attr_iterator_destroy (iter);
}

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

  if (!g_file_get_contents (filename, &contents, &length, &error))
    {
      fprintf (stderr, "%s\n", error->message);
      g_error_free (error);
      return;
    }

  ret = pango_parse_markup (contents, length, 0, &attrs, &text, NULL, &error);
  g_free (contents);

  if (ret)
    {
      g_assert_no_error (error);
      g_string_append (string, text);
      g_string_append (string, "\n\n---\n\n");
      attr_list_dump (attrs, string);
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
    }
  else
    {
      g_string_append_printf (string, "ERROR: %s", error->message);
      g_error_free (error);
    }
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

static char *
diff_with_file (const char  *file1,
                GString     *string,
                GError     **error)
{
  const char *command[] = { "diff", "-u", file1, NULL, NULL };
  char *diff, *tmpfile;
  int fd;

  diff = NULL;

  /* write the text buffer to a temporary file */
  fd = g_file_open_tmp (NULL, &tmpfile, error);
  if (fd < 0)
    return NULL;

  if (write (fd, string->str, string->len) != (int) string->len)
    {
      close (fd);
      g_set_error (error,
                   G_FILE_ERROR, G_FILE_ERROR_FAILED,
                   "Could not write data to temporary file '%s'", tmpfile);
      goto done;
    }
  close (fd);
  command[3] = tmpfile;

  /* run diff command */
  g_spawn_sync (NULL, (char **)command, NULL, G_SPAWN_SEARCH_PATH, NULL, NULL, &diff, NULL, NULL, error);

done:
  unlink (tmpfile);
  g_free (tmpfile);

  return diff;
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

  diff = diff_with_file (expected_file, string, &error);
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

int
main (int argc, char *argv[])
{
  GDir *dir;
  GError *error = NULL;
  const gchar *name;
  gchar *path;

  g_setenv ("LC_ALL", "C", TRUE);
  setlocale (LC_ALL, "");

  g_test_init (&argc, &argv, NULL);

  /* allow to easily generate expected output for new test cases */
  if (argc > 1)
    {
      GString *string;

      string = g_string_sized_new (0);
      test_file (argv[1], string);
      g_print ("%s", string->str);

      return 0;
    }

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
    }
  g_dir_close (dir);

  return g_test_run ();
}
