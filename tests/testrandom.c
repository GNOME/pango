/* Pango
 * testmisc.c: Test program for miscellaneous things
 *
 * Copyright (C) 2021 Benjamin Otte
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

#include "config.h"

#include <locale.h>
#include <gio/gio.h>
#include <pango/pangocairo.h>

static char **ltr_words;
static gsize n_ltr_words;
static char **rtl_words;
static gsize n_rtl_words;

static const char *
random_word (PangoDirection dir)
{
  switch ((int)dir)
    {
      case PANGO_DIRECTION_LTR:
        return ltr_words[g_test_rand_int_range (0, n_ltr_words)];
      case PANGO_DIRECTION_RTL:
        return rtl_words[g_test_rand_int_range (0, n_rtl_words)];
      default:
        return random_word (g_test_rand_bit () ? PANGO_DIRECTION_LTR : PANGO_DIRECTION_RTL);
    }
}

static char *
create_random_sentence (PangoDirection dir)
{
  GString *string = g_string_new (NULL);
  gsize i, n_words;

  n_words = g_test_rand_int_range (3, 15);
  for (i = 0; i < n_words; i++)
    {
      if (i > 0)
        {
          if (g_random_int_range (0, 10) == 0)
            g_string_append_c (string, ',');
          g_string_append_c (string, ' ');
        }
      g_string_append (string, random_word (dir));
    }

  g_string_append_c (string, "!.?"[g_test_rand_int_range(0, 3)]);

  return g_string_free (string, FALSE);
}

typedef struct
{
  int set_width;
  int width;
  int height;
} Size;

static int
compare_size (gconstpointer a,
              gconstpointer b,
              gpointer      unused)
{
  return ((const Size *) a)->set_width - ((const Size *) b)->set_width;
}

static void
layout_check_size (PangoLayout *layout,
                   int          width,
                   Size        *out_size)
{
  out_size->set_width = width;
  pango_layout_set_width (layout, width);
  pango_layout_get_size (layout, &out_size->width, &out_size->height);
}

static void
test_wrap_char (gconstpointer data)
{
#define N_SENTENCES 20
  PangoDirection dir = GPOINTER_TO_UINT (data);
  PangoFontDescription *desc;
  PangoContext *context;
  PangoLayout *layout;
  char *sentence;
  Size min, max;
  Size sizes[100];
  gsize i, j;

  context = pango_font_map_create_context (pango_cairo_font_map_get_default ());
  desc = pango_font_description_from_string ("Sans 10");
  layout = pango_layout_new (context);
  pango_layout_set_font_description (layout, desc);
  pango_font_description_free (desc);

  for (j = 0; j < N_SENTENCES; j++)
    {
      sentence = create_random_sentence (dir);
      pango_layout_set_text (layout, sentence, -1);
      if (g_test_verbose ())
        g_print ("%s\n", sentence);
      g_free (sentence);
      pango_layout_set_wrap (layout, PANGO_WRAP_WORD_CHAR);

      layout_check_size (layout, -1, &max);
      layout_check_size (layout, 0, &min);
      g_assert_cmpint (min.width, <=, max.width);
      g_assert_cmpint (min.height, >=, max.height);

      for (i = 0; i < G_N_ELEMENTS (sizes); i++)
        {
          layout_check_size (layout, min.width / 2 + g_test_rand_int_range (0, max.width), &sizes[i]);
        }

      g_qsort_with_data (sizes, G_N_ELEMENTS (sizes), sizeof (Size), compare_size, NULL);

      if (sizes[0].set_width >= min.width)
        {
          g_assert_cmpint (sizes[0].width, >=, min.width);
          g_assert_cmpint (sizes[0].height, <=, min.height);
        }

      for (i = 1; i < G_N_ELEMENTS (sizes); i++)
        {
          if (sizes[i-1].set_width < min.width ||
              sizes[i].set_width < min.width)
            continue;

          g_assert_cmpint (sizes[i-1].set_width, <=, sizes[i].set_width);

          g_assert_cmpint (sizes[i].width, <=, sizes[i].set_width);

          if (g_test_verbose ())
            {
              if (sizes[i-1].width > sizes[i].width)
                {
                  g_print ("min %d\n", min.width);
                  g_print ("%d %d\n", sizes[i-1].set_width, sizes[i].set_width);
                }
            }
          g_assert_cmpint (sizes[i-1].width, <=, sizes[i].width);
          g_assert_cmpint (sizes[i-1].height, >=, sizes[i].height);
        }

      if (sizes[i-1].set_width >= max.width)
        {
          g_assert_cmpint (sizes[i-1].width, ==, max.width);
          g_assert_cmpint (sizes[i-1].height, ==, max.height);
        }
      else
        {
          g_assert_cmpint (sizes[i-1].width, <, max.width);
          g_assert_cmpint (sizes[i-1].height, >, max.height);
        }
    }

  g_object_unref (layout);
  g_object_unref (context);
}

static gboolean
load_hunspell_words (GStrvBuilder *builder,
                      const char  *language)
{
  char *path;
  GBytes *bytes;
  GFile *file;
  char **words;
  gsize i;

  path = g_strdup_printf ("/usr/share/myspell/%s.dic", language);
  file = g_file_new_for_path (path);
  g_free (path);
  bytes = g_file_load_bytes (file, NULL, NULL, NULL);
  g_object_unref (file);
  if (bytes == NULL)
    return FALSE;

  words = g_strsplit (g_bytes_get_data (bytes, NULL), "\n", -1);
  if (words == NULL || words[0] == NULL)
    {
      g_free (words);
      return FALSE;
    }

  for (i = 1; words[i]; i++)
    {
      char *slash = strchr (words[i], '/');
      if (slash)
        *slash = 0;

      g_strv_builder_add (builder, words[i]);
    }
  
  g_strfreev (words);

  return TRUE;
}

static char **
init_ltr_words (void)
{
  GStrvBuilder *builder;
  GFile *file;
  GBytes *bytes;
  char **result;

  builder = g_strv_builder_new ();

  if (!load_hunspell_words (builder, "en_US"))
    {
      file = g_file_new_for_path ("/usr/share/dict/words");
      bytes = g_file_load_bytes (file, NULL, NULL, NULL);
      if (bytes)
        {
          result = g_strsplit (g_bytes_get_data (bytes, NULL), "\n", -1);
          g_bytes_unref (bytes);
        }
      else
        result = g_strsplit ("lorem ipsum dolor sit amet consectetur adipisci elit sed eiusmod tempor incidunt labore et dolore magna aliqua ut enim ad minim veniam quis nostrud exercitation ullamco laboris nisi ut aliquid ex ea commodi consequat", " ", -1);
      g_strv_builder_addv (builder, (const char **) result);
      g_strfreev (result);
    }

  result = g_strv_builder_end (builder);
  g_strv_builder_unref (builder);

  return result;
}

static char **
init_rtl_words (void)
{
  GStrvBuilder *builder;
  char **result;

  builder = g_strv_builder_new ();

  if (!load_hunspell_words (builder, "he_IL"))
    {
      result = g_strsplit ("לורם איפסום דולור סיט אמט קונסקטטור אדיפיסינג אלית קולורס מונפרד אדנדום סילקוף מרגשי ומרגשח עמחליף לפרומי בלוף קינץ תתיח לרעח לת צשחמי צש בליא מנסוטו צמלח לביקו ננבי צמוקו בלוקריה שיצמה ברורק להאמית קרהשק סכעיט דז מא מנכם למטכין נשואי מנורךגולר מונפרר סוברט לורם שבצק יהול לכנוץ בעריר גק ליץ ושבעגט ושבעגט לבם סולגק בראיט ולחת צורק מונחף בגורמי מגמש תרבנך וסתעד לכנו סתשם השמה - לתכי מורגם בורק? לתיג ישבעס", " ", -1);
      g_strv_builder_addv (builder, (const char **) result);
      g_strfreev (result);
    }

  result = g_strv_builder_end (builder);
  g_strv_builder_unref (builder);

  return result;
}

int
main (int argc, char *argv[])
{
  int result;

  g_test_init (&argc, &argv, NULL);
  setlocale (LC_ALL, "");

  ltr_words = init_ltr_words ();
  n_ltr_words = g_strv_length (ltr_words);
  rtl_words = init_rtl_words ();
  n_rtl_words = g_strv_length (rtl_words);

  g_test_add_data_func ("/layout/ltr/wrap-char", GUINT_TO_POINTER (PANGO_DIRECTION_LTR), test_wrap_char);
  g_test_add_data_func ("/layout/rtl/wrap-char", GUINT_TO_POINTER (PANGO_DIRECTION_RTL), test_wrap_char);
  g_test_add_data_func ("/layout/any/wrap-char", GUINT_TO_POINTER (PANGO_DIRECTION_NEUTRAL), test_wrap_char);

  result = g_test_run ();

  g_strfreev (ltr_words);
  g_strfreev (rtl_words);

  return result;
}
