/* Pango
 * test-common.c: Common test code
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
#include <stdlib.h>
#include <string.h>

#include <locale.h>

#ifdef G_OS_WIN32
#include <io.h>
#else
#include <unistd.h>
#endif

#include <pango/pangocairo.h>

#ifdef G_OS_WIN32
#include <pango/pangowin32.h>
#endif

#ifdef HAVE_FREETYPE
#include <pango/pangoft2.h>
#include <pango/pangocairo-fc.h>
#endif

#include "test-common.h"


char *
diff_with_file (const char  *file,
                char        *text,
                gssize       len,
                GError     **error)
{
#ifdef G_OS_WIN32
  const char *command[] = { "diff", "-u", "-i", "--strip-trailing-cr", file, NULL, NULL };
  const int tmp_file_idx = 5;
#else
  const char *command[] = { "diff", "-u", "-i", file, NULL, NULL };
  const int tmp_file_idx = 4;
#endif

  char *diff, *tmpfile;
  int fd;

  diff = NULL;

  if (len < 0)
    len = strlen (text);

  /* write the text buffer to a temporary file */
  fd = g_file_open_tmp (NULL, &tmpfile, error);
  if (fd < 0)
    return NULL;

  if (write (fd, text, len) != (int) len)
    {
      close (fd);
      g_set_error (error,
                   G_FILE_ERROR, G_FILE_ERROR_FAILED,
                   "Could not write data to temporary file '%s'", tmpfile);
      goto done;
    }
  close (fd);
  command[tmp_file_idx] = tmpfile;

  /* run diff command */
  g_spawn_sync (NULL,
                (char **) command,
                NULL,
                G_SPAWN_SEARCH_PATH,
                NULL, NULL,
                &diff,
                NULL, NULL,
                error);

done:
  unlink (tmpfile);
  g_free (tmpfile);

  return diff;
}

char *
diff_bytes (GBytes  *b1,
            GBytes  *b2,
            GError **error)
{
  const char *command[] = { "diff", "-u", "-i", NULL, NULL, NULL };
  char *diff = NULL, *tmpfile = NULL, *tmpfile2 = NULL;
  int fd;
  const char *text;
  gsize len;

  /* write the text buffer to a temporary file */
  fd = g_file_open_tmp (NULL, &tmpfile, error);
  if (fd < 0)
    return NULL;

  text = (const char *) g_bytes_get_data (b1, &len);
  if (write (fd, text, len) != (int) len)
    {
      close (fd);
      g_set_error (error,
                   G_FILE_ERROR, G_FILE_ERROR_FAILED,
                   "Could not write data to temporary file '%s'", tmpfile);
      goto done;
    }
  close (fd);

  fd = g_file_open_tmp (NULL, &tmpfile2, error);
  if (fd < 0)
    return NULL;

  text = (const char *) g_bytes_get_data (b2, &len);
  if (write (fd, text, len) != (int) len)
    {
      close (fd);
      g_set_error (error,
                   G_FILE_ERROR, G_FILE_ERROR_FAILED,
                   "Could not write data to temporary file '%s'", tmpfile2);
      goto done;
    }
  close (fd);

  command[3] = tmpfile;
  command[4] = tmpfile2;

  /* run diff command */
  g_spawn_sync (NULL,
                (char **) command,
                NULL,
                G_SPAWN_SEARCH_PATH,
                NULL, NULL,
                &diff,
                NULL, NULL,
                error);

done:
  unlink (tmpfile);
  g_free (tmpfile);
  unlink (tmpfile2);
  g_free (tmpfile2);

  return diff;
}

gboolean
file_has_prefix (const char  *filename,
                 const char  *str,
                 GError     **error)
{
  char *contents;
  gsize len;
  gboolean ret;

  if (!g_file_get_contents (filename, &contents, &len, error))
    return FALSE;

  ret = g_str_has_prefix (contents, str);

  g_free (contents);

  return ret;
}

void
print_attribute (PangoAttribute *attr, GString *string)
{
  PangoAttrList *l = pango_attr_list_new ();
  char *s;

  pango_attr_list_insert (l, pango_attribute_copy (attr));
  s = pango_attr_list_to_string (l);
  g_string_append (string, s);
  g_free (s);
  pango_attr_list_unref (l);
}

void
print_attr_list (PangoAttrList *attrs, GString *string)
{
  PangoAttrIterator *iter;

  if (!attrs)
    return;

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
        print_attribute (attr, string);
        g_string_append (string, "\n");
      }
    g_slist_free_full (list, (GDestroyNotify)pango_attribute_destroy);
  } while (pango_attr_iterator_next (iter));

  pango_attr_iterator_destroy (iter);
}

void
print_attributes (GSList *attrs, GString *string)
{
  GSList *l;

  for (l = attrs; l; l = l->next)
    {
      PangoAttribute *attr = l->data;

      print_attribute (attr, string);
      g_string_append (string, "\n");
    }
}

const char *
get_script_name (GUnicodeScript s)
{
  GEnumClass *class;
  GEnumValue *value;
  const char *nick;

  class = g_type_class_ref (g_unicode_script_get_type ());
  value = g_enum_get_value (class, s);
  nick = value->value_nick;
  g_type_class_unref (class);
  return nick;
}

/* Create a fontmap that will return our included Cantarell-VF.otf
 * for "Cantarell"
 *
 * FIXME: Make this work for macOS
 */
PangoFontMap *
get_font_map_with_cantarell (void)
{
  PangoFontMap *fontmap;
  GError *error = NULL;
  char *path;

  fontmap = pango_cairo_font_map_new ();

  /* FIXME: the coretext backend doesn't implement add_font_file */
  if (strcmp (G_OBJECT_TYPE_NAME (fontmap), "PangoCairoCoreTextFontMap") != 0)
    {
      path = g_test_build_filename (G_TEST_DIST, "fonts", "Cantarell-VF.otf", NULL);
      if (g_test_verbose ())
        g_test_message ("adding %s to font map", path);
      pango_font_map_add_font_file (fontmap, path, &error);
      g_assert_no_error (error);
      g_free (path);
    }

  g_assert_true (pango_cairo_font_map_get_resolution (PANGO_CAIRO_FONT_MAP (fontmap)) == 96.0);

  return fontmap;
}

