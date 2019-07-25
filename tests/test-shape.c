/* Pango
 * test-shape.c: Test Pango shaping
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

static void
append_text (GString    *s,
             const char *text,
             int         len)
{
  const char *p;

  for (p = text; p < text + len; p = g_utf8_next_char (p))
    {
      gunichar ch = g_utf8_get_char (p);
      if (ch == ' ')
        g_string_append (s, "[ ]");
      if (ch == 0x0A || ch == 0x2028 || !g_unichar_isprint (ch))
        g_string_append_printf (s, "[%#04x]", ch);
      else
        g_string_append_unichar (s, ch);
    }
}

static void
parse_params (const char *str,
              gboolean *insert_hyphen)
{
  char **strings;
  int i;

  strings = g_strsplit (str, ",", -1);
  for (i = 0; strings[i]; i++)
    {
      char **str2 = g_strsplit (strings[i], "=", -1);
      if (strcmp (str2[0], "insert-hyphen") == 0)
        *insert_hyphen = strcmp (str2[1], "true") == 0;
      g_strfreev (str2);
    }
  g_strfreev (strings);
}

static gboolean
affects_itemization (PangoAttribute *attr,
                     gpointer        data)
{
  switch (attr->klass->type)
    {
    /* These affect font selection */
    case PANGO_ATTR_LANGUAGE:
    case PANGO_ATTR_FAMILY:
    case PANGO_ATTR_STYLE:
    case PANGO_ATTR_WEIGHT:
    case PANGO_ATTR_VARIANT:
    case PANGO_ATTR_STRETCH:
    case PANGO_ATTR_SIZE:
    case PANGO_ATTR_FONT_DESC:
    case PANGO_ATTR_SCALE:
    case PANGO_ATTR_FALLBACK:
    case PANGO_ATTR_ABSOLUTE_SIZE:
    case PANGO_ATTR_GRAVITY:
    case PANGO_ATTR_GRAVITY_HINT:
    /* These are part of ItemProperties, so need to break runs */
    case PANGO_ATTR_SHAPE:
    case PANGO_ATTR_RISE:
    case PANGO_ATTR_UNDERLINE:
    case PANGO_ATTR_STRIKETHROUGH:
    case PANGO_ATTR_LETTER_SPACING:
      return TRUE;
    default:
      return FALSE;
    }
}

static gboolean
affects_break_or_shape (PangoAttribute *attr,
                        gpointer        data)
{
  switch (attr->klass->type)
    {
    /* Affects breaks */
    case PANGO_ATTR_ALLOW_BREAKS:
    /* Affects shaping */
    case PANGO_ATTR_FONT_FEATURES:
    case PANGO_ATTR_SHOW:
      return TRUE;
    default:
      return FALSE;
    }
}

static void
apply_attributes_to_items (GList         *items,
                           PangoAttrList *attrs)
{
  GList *l;
  PangoAttrIterator *iter;

  if (!attrs)
    return;

  iter = pango_attr_list_get_iterator (attrs);

  for (l = items; l; l = l->next)
    {
      PangoItem *item = l->data;
      pango_item_apply_attrs (item, iter);
    }

  pango_attr_iterator_destroy (iter);
}

static void
test_file (const gchar *filename, GString *string)
{
  gchar *contents;
  gsize  length;
  GError *error = NULL;
  char *test;
  char *text;
  PangoAttrList *attrs;
  PangoAttrList *itemize_attrs;
  PangoAttrList *shape_attrs;
  GList *items, *l;
  GString *s1, *s2, *s3, *s4, *s5, *s6, *s7;
  gboolean insert_hyphen = FALSE;
  char *p, *p1;
  const char *sep = "";

  if (!g_file_get_contents (filename, &contents, &length, &error))
    {
      fprintf (stderr, "%s\n", error->message);
      g_error_free (error);
      return;
    }

  test = contents;

  /* Skip initial comments */
  while (test[0] == '#')
    test = strchr (test, '\n') + 1;

  p = strchr (test, '\n');
  *p = '\0';

  parse_params (test, &insert_hyphen);
  test  = p + 1;

  if (!pango_parse_markup (test, -1, 0, &attrs, &text, NULL, &error))
    {
      fprintf (stderr, "%s\n", error->message);
      g_error_free (error);
      return;
    }

  s1 = g_string_new ("Text:      ");
  s2 = g_string_new ("Glyphs:    ");
  s3 = g_string_new ("Cluster:   ");
  s4 = g_string_new ("Width:     ");
  s5 = g_string_new ("Direction: ");
  s6 = g_string_new ("Item:      ");
  s7 = g_string_new ("Offset:    ");

  length = strlen (text);
  if (text[length - 1] == '\n')
    length--;

  itemize_attrs = pango_attr_list_filter (attrs, affects_itemization, NULL);
  shape_attrs = pango_attr_list_filter (attrs, affects_break_or_shape, NULL);

  items = pango_itemize (context, text, 0, length, itemize_attrs, NULL);
  apply_attributes_to_items (items, shape_attrs);

  pango_attr_list_unref (itemize_attrs);
  pango_attr_list_unref (shape_attrs);

  pango_attr_list_unref (attrs);

  for (l = items; l; l = l->next)
    {
      PangoItem *item = l->data;
      PangoGlyphString *glyphs;
      gboolean rtl = item->analysis.level % 2;
      PangoGlyphItem glyph_item;
      int i;

      glyphs = pango_glyph_string_new ();
      pango_shape_full (text + item->offset, item->length, text, length, &item->analysis, glyphs);

      glyph_item.item = item;
      glyph_item.glyphs = glyphs;
      pango_glyph_item_apply_attrs (&glyph_item, text, attrs);

      g_string_append (s1, sep);
      g_string_append (s2, sep);
      g_string_append (s3, sep);
      g_string_append (s4, sep);
      g_string_append (s5, sep);
      g_string_append (s6, sep);
      g_string_append (s7, sep);
      sep = "|";

      g_string_append_printf (s6, "%d(%d)", item->num_chars, item->length);

      for (i = 0; i < glyphs->num_glyphs; i++)
        {
          int len;
          PangoGlyphInfo *gi = &glyphs->glyphs[i];

          char *p;
          p = text + item->offset + glyphs->log_clusters[i];
          if (rtl)
            {
              if (i > 0)
                p1 = text + item->offset + glyphs->log_clusters[i - 1];
              else
                p1 = g_utf8_next_char (p);
            }
          else
            {
              if (i + 1 < glyphs->num_glyphs)
                p1 = text + item->offset + glyphs->log_clusters[i + 1];
              else
                p1 = g_utf8_next_char (p);
            }
          append_text (s1, p, p1 - p);
          g_string_append_printf (s2, "[%d]", gi->glyph);
          g_string_append_printf (s4, "%d ", gi->geometry.width);
          g_string_append_printf (s7, "%d,%d ", gi->geometry.x_offset, gi->geometry.y_offset);
          if (gi->attr.is_cluster_start)
            g_string_append_printf (s3, "%d ", item->offset + glyphs->log_clusters[i]);
          g_string_append (s5, rtl ? "<" : ">");
          len = 0;
          len = MAX (len, g_utf8_strlen (s1->str, s1->len));
          len = MAX (len, g_utf8_strlen (s2->str, s2->len));
          len = MAX (len, g_utf8_strlen (s3->str, s3->len));
          len = MAX (len, g_utf8_strlen (s4->str, s4->len));
          len = MAX (len, g_utf8_strlen (s5->str, s5->len));
          len = MAX (len, g_utf8_strlen (s6->str, s6->len));
          len = MAX (len, g_utf8_strlen (s7->str, s7->len));
          g_string_append_printf (s1, "%*s", len - (int)g_utf8_strlen (s1->str, s1->len), "");
          g_string_append_printf (s4, "%*s", len - (int)g_utf8_strlen (s4->str, s4->len), "");
          g_string_append_printf (s2, "%*s", len - (int)g_utf8_strlen (s2->str, s2->len), "");
          g_string_append_printf (s3, "%*s", len - (int)g_utf8_strlen (s3->str, s3->len), "");
          g_string_append_printf (s5, "%*s", len - (int)g_utf8_strlen (s5->str, s5->len), "");
          g_string_append_printf (s6, "%*s", len - (int)g_utf8_strlen (s6->str, s6->len), "");
          g_string_append_printf (s7, "%*s", len - (int)g_utf8_strlen (s7->str, s7->len), "");
        }

      pango_glyph_string_free (glyphs);
    }

  g_string_append_printf (string, "%s\n", test);
  g_string_append_printf (string, "%s\n", s6->str);
  g_string_append_printf (string, "%s\n", s1->str);
  g_string_append_printf (string, "%s\n", s2->str);
  g_string_append_printf (string, "%s\n", s4->str);
  g_string_append_printf (string, "%s\n", s7->str);
  g_string_append_printf (string, "%s\n", s3->str);
  g_string_append_printf (string, "%s\n", s5->str);

  g_string_free (s1, TRUE);
  g_string_free (s2, TRUE);
  g_string_free (s3, TRUE);
  g_string_free (s4, TRUE);
  g_string_free (s5, TRUE);
  g_string_free (s6, TRUE);
  g_string_free (s7, TRUE);

  g_list_free_full (items, (GDestroyNotify)pango_item_free);
  g_free (contents);
  g_free (text);
}

static gchar *
get_expected_filename (const gchar *filename)
{
  gchar *f, *p, *expected;

  f = g_strdup (filename);
  p = strstr (f, ".shape");
  if (p)
    *p = 0;
  expected = g_strconcat (f, ".expected", NULL);

  g_free (f);

  return expected;
}

static void
test_itemize (gconstpointer d)
{
  const gchar *filename = d;
  gchar *expected_file;
  GError *error = NULL;
  GString *dump;
  gchar *diff;

  expected_file = get_expected_filename (filename);

  dump = g_string_sized_new (0);

  test_file (filename, dump);

  diff = diff_with_file (expected_file, dump->str, dump->len, &error);
  g_assert_no_error (error);

  if (diff && diff[0])
    {
      g_printerr ("Contents don't match expected contents:\n%s", diff);
      g_test_fail ();
      g_free (diff);
    }

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

  g_setenv ("LC_ALL", "en_US.UTF-8", TRUE);
  setlocale (LC_ALL, "");

  g_test_init (&argc, &argv, NULL);

  context = pango_font_map_create_context (pango_cairo_font_map_get_default ());

  /* allow to easily generate expected output for new test cases */
  if (argc > 1)
    {
      GString *string;

      string = g_string_sized_new (0);
      test_file (argv[1], string);
      printf ("%s", string->str);

      return 0;
    }

  path = g_test_build_filename (G_TEST_DIST, "shape", NULL);
  dir = g_dir_open (path, 0, &error);
  g_free (path);

  if (g_error_matches (error, G_FILE_ERROR, G_FILE_ERROR_NOENT))
    return 0;

  g_assert_no_error (error);
  while ((name = g_dir_read_name (dir)) != NULL)
    {
      if (!strstr (name, "items"))
        continue;

      path = g_strdup_printf ("/shape/%s", name);
      g_test_add_data_func_full (path, g_test_build_filename (G_TEST_DIST, "shape", name, NULL),
                                 test_itemize, g_free);
      g_free (path);
    }
  g_dir_close (dir);

  return g_test_run ();
}
