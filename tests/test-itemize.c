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

static void
append_text (GString    *s,
             const char *text,
             int         len)
{
  const char *p;

  for (p = text; p < text + len; p = g_utf8_next_char (p))
    {
      gunichar ch = g_utf8_get_char (p);
      if (ch == 0x0A || ch == 0x2028 || !g_unichar_isprint (ch))
        g_string_append_printf (s, "[%#04x]", ch);
      else
        g_string_append_unichar (s, ch);
    }
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
 GString *s1, *s2, *s3, *s4, *s5, *s6;
  char *test;
  char *text;
  PangoAttrList *attrs;
  PangoAttrList *itemize_attrs;
  GList *items, *l;
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


  if (!pango_parse_markup (test, -1, 0, &attrs, &text, NULL, &error))
    {
      fprintf (stderr, "%s\n", error->message);
      g_error_free (error);
      return;
    }

  s1 = g_string_new ("Items:  ");
  s2 = g_string_new ("Font:   ");
  s3 = g_string_new ("Script: ");
  s4 = g_string_new ("Lang:   ");
  s5 = g_string_new ("Bidi:   ");
  s6 = g_string_new ("Attrs:  ");

  length = strlen (text);
  if (text[length - 1] == '\n')
    length--;

  itemize_attrs = pango_attr_list_filter (attrs, affects_itemization, NULL);
  items = pango_itemize (context, text, 0, length, itemize_attrs, NULL);

  apply_attributes_to_items (items, attrs);
  pango_attr_list_unref (itemize_attrs);

  for (l = items; l; l = l->next)
    {
      PangoItem *item = l->data;
      PangoFontDescription *desc;
      char *font;
      int m;
      GSList *a;

      desc = pango_font_describe (item->analysis.font);
      font = pango_font_description_to_string (desc);

      if (l != items)
        sep = "|";
      g_string_append (s1, sep);
      append_text (s1, text + item->offset, item->length);

      g_string_append_printf (s2, "%s%s", sep, font);
      g_string_append_printf (s3, "%s%s", sep, get_script_name (item->analysis.script));
      g_string_append_printf (s4, "%s%s", sep, pango_language_to_string (item->analysis.language));
      g_string_append_printf (s5, "%s%d", sep, item->analysis.level);
      g_string_append_printf (s6, "%s", sep);
      for (a = item->analysis.extra_attrs; a; a = a->next)
        {
          PangoAttribute *attr = a->data;
          if (a != item->analysis.extra_attrs)
            g_string_append (s6, ",");
          print_attribute (attr, s6);
        }

      g_free (font);
      pango_font_description_free (desc);

      m = MAX (MAX (MAX (s1->len, s2->len),
                    MAX (s3->len, s4->len)),
               MAX (s5->len, s6->len));

      g_string_append_printf (s1, "%*s", (int)(m - s1->len), "");
      g_string_append_printf (s2, "%*s", (int)(m - s2->len), "");
      g_string_append_printf (s3, "%*s", (int)(m - s3->len), "");
      g_string_append_printf (s4, "%*s", (int)(m - s4->len), "");
      g_string_append_printf (s5, "%*s", (int)(m - s5->len), "");
      g_string_append_printf (s6, "%*s", (int)(m - s6->len), "");
    }

  g_string_append_printf (string, "%s\n", test);
  g_string_append_printf (string, "%s\n", s1->str);
  g_string_append_printf (string, "%s\n", s2->str);
  g_string_append_printf (string, "%s\n", s3->str);
  g_string_append_printf (string, "%s\n", s4->str);
  g_string_append_printf (string, "%s\n", s5->str);
  g_string_append_printf (string, "%s\n", s6->str);

  g_string_free (s1, TRUE);
  g_string_free (s2, TRUE);
  g_string_free (s3, TRUE);
  g_string_free (s4, TRUE);
  g_string_free (s5, TRUE);
  g_string_free (s6, TRUE);

  g_list_free_full (items, (GDestroyNotify)pango_item_free);
  pango_attr_list_unref (attrs);
  g_free (text);
  g_free (contents);
}

static gchar *
get_expected_filename (const gchar *filename)
{
  gchar *f, *p, *expected;

  f = g_strdup (filename);
  p = strstr (f, ".items");
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

  path = g_test_build_filename (G_TEST_DIST, "itemize", NULL);
  dir = g_dir_open (path, 0, &error);
  g_free (path);
  g_assert_no_error (error);
  while ((name = g_dir_read_name (dir)) != NULL)
    {
      if (!strstr (name, "items"))
        continue;

      path = g_strdup_printf ("/itemize/%s", name);
      g_test_add_data_func_full (path, g_test_build_filename (G_TEST_DIST, "itemize", name, NULL),
                                 test_itemize, g_free);
      g_free (path);
    }
  g_dir_close (dir);

  return g_test_run ();
}
