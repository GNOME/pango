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

static const char *
script_name (GUnicodeScript s)
{
  const char *names[] = {
    "Zyyy", "Zinh", "Arab", "Armn", "Beng", "Bopo", "Cher",
    "Copt", "Cyrl", "Dsrt", "Deva", "Ethi", "Geor", "Goth",
    "Grek", "Gujr", "Guru", "Hani", "Hang", "Hebr", "Hira",
    "Knda", "Kana", "Khmr", "Laoo", "Latn", "Mlym", "Mong",
    "Mymr", "Ogam", "Ital", "Orya", "Runr", "Sinh", "Syrc",
    "Taml", "Telu", "Thaa", "Thai", "Tibt", "Cans", "Yiii",
    "Tglg", "Hano", "Buhd", "Tagb", "Brai", "Cprt", "Limb",
    "Osma", "Shaw", "Linb", "Tale", "Ugar", "Talu", "Bugi",
    "Glag", "Tfng", "Sylo", "Xpeo", "Khar", "Zzzz", "Bali",
    "Xsux", "Phnx", "Phag", "Nkoo", "Kali", "Lepc", "Rjng",
    "Sund", "Saur", "Cham", "Olck", "Vaii", "Cari", "Lyci",
    "Lydi", "Avst", "Bamu", "Egyp", "Armi", "Phli", "Prti",
    "Java", "Kthi", "Lisu", "Mtei", "Sarb", "Orkh", "Samr",
    "Lana", "Tavt", "Batk", "Brah", "Mand", "Cakm", "Merc",
    "Mero", "Plrd", "Shrd", "Sora", "Takr", "Bass", "Aghb",
    "Dupl", "Elba", "Gran", "Khoj", "Sind", "Lina", "Mahj",
    "Mani", "Mend", "Modi", "Mroo", "Nbat", "Narb", "Perm",
    "Hmng", "Palm", "Pauc", "Phlp", "Sidd", "Tirh", "Wara",
    "Ahom", "Hluw", "Hatr", "Mult", "Hung", "Sgnw", "Adlm",
    "Bhks", "Marc", "Newa", "Osge", "Tang", "Gonm", "Nshu",
    "Soyo", "Zanb", "Dogr", "Gong", "Rohg", "Maka", "Medf",
    "Sogo", "Sogd", "Elym", "Nand", "Rohg", "Wcho"
  };
  return names[s];
}

static void
append_text (GString    *s,
             const char *text,
             int         len)
{
  char *p;

  for (p = text; p < text + len; p = g_utf8_next_char (p))
    {
      gunichar ch = g_utf8_get_char (p);
      if (ch == 0x0A || ch == 0x2028 || !g_unichar_isprint (ch))
        g_string_append_printf (s, "[%#04x]", ch);
      else
        g_string_append_unichar (s, ch);
    }
}
static void
test_file (const gchar *filename, GString *string)
{
  gchar *contents;
  gsize  length;
  GError *error = NULL;
  GString *s1, *s2, *s3, *s4, *s5;
  char *test;
  char *text;
  PangoAttrList *attrs;
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

  length = strlen (text);
  if (text[length - 1] == '\n')
    length--;

  items = pango_itemize (context, text, 0, length, attrs, NULL);

  for (l = items; l; l = l->next)
    {
      PangoItem *item = l->data;
      PangoFontDescription *desc;
      char *font;
      int m;

      desc = pango_font_describe (item->analysis.font);
      font = pango_font_description_to_string (desc);

      if (l != items)
        sep = "|";
      g_string_append (s1, sep);
      append_text (s1, text + item->offset, item->length);

      g_string_append_printf (s2, "%s%s", sep, font);
      g_string_append_printf (s3, "%s%s", sep, script_name (item->analysis.script));
      g_string_append_printf (s4, "%s%s", sep, pango_language_to_string (item->analysis.language));
      g_string_append_printf (s5, "%s%d", sep, item->analysis.level);

      g_free (font);
      pango_font_description_free (desc);

      m = MAX (MAX (MAX (s1->len, s2->len), MAX (s3->len, s4->len)), s5->len);

      g_string_append_printf (s1, "%*s", (int)(m - s1->len), "");
      g_string_append_printf (s2, "%*s", (int)(m - s2->len), "");
      g_string_append_printf (s3, "%*s", (int)(m - s3->len), "");
      g_string_append_printf (s4, "%*s", (int)(m - s4->len), "");
      g_string_append_printf (s5, "%*s", (int)(m - s5->len), "");
    }

  g_string_append_printf (string, "%s\n", test);
  g_string_append_printf (string, "%s\n", s1->str);
  g_string_append_printf (string, "%s\n", s2->str);
  g_string_append_printf (string, "%s\n", s3->str);
  g_string_append_printf (string, "%s\n", s4->str);
  g_string_append_printf (string, "%s\n", s5->str);

  g_string_free (s1, TRUE);
  g_string_free (s2, TRUE);
  g_string_free (s3, TRUE);
  g_string_free (s4, TRUE);
  g_string_free (s5, TRUE);

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
