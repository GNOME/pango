/* Pango2
 * testmisc.c: Test program for miscellaneous things
 *
 * Copyright (C) 2020 Matthias Clasen
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
#include <glib.h>
#include <pango2/pangocairo.h>
#include <pango2/pango-item-private.h>

/* Test that itemizing a string with 0 characters works
 */
static void
test_itemize_empty_crash (void)
{
  Pango2Context *context;

  context = pango2_context_new ();
  pango2_itemize (context, PANGO2_DIRECTION_LTR, "", 0, 1, NULL);

  g_object_unref (context);
}

static void
test_itemize_utf8 (void)
{
  Pango2Context *context;
  GList *result = NULL;

  context = pango2_context_new ();
  result = pango2_itemize (context, PANGO2_DIRECTION_LTR, "\xc3\xa1\na", 3, 1, NULL);
  g_assert (result != NULL);

  g_list_free_full (result, (GDestroyNotify)pango2_item_free);
  g_object_unref (context);
}

/* Test that pango2_layout_set_text (layout, "short", 200)
 * does not lead to a crash. (pidgin does this)
 */
static void
test_short_string_crash (void)
{
  Pango2Context *context;
  Pango2Layout *layout;
  Pango2Rectangle ext;

  context = pango2_context_new ();
  layout = pango2_layout_new (context);
  pango2_layout_set_text (layout, "short text", 200);
  pango2_lines_get_extents (pango2_layout_get_lines (layout), &ext, &ext);

  g_object_unref (layout);
  g_object_unref (context);
}

static void
test_language_emoji_crash (void)
{
  Pango2Language *lang;
  const GUnicodeScript *scripts;
  int num;

  lang = pango2_language_from_string ("und-zsye");
  scripts = pango2_language_get_scripts (lang, &num);

  g_assert (num >= 0);
  g_assert (scripts == NULL || num > 0);
}

static void
test_line_height (void)
{
  Pango2Context *context;
  Pango2Layout *layout;
  Pango2Line *line;
  Pango2Rectangle ext;

  context = pango2_context_new ();
  layout = pango2_layout_new (context);
  pango2_layout_set_text (layout, "one\ttwo", -1);
  line = pango2_lines_get_lines (pango2_layout_get_lines (layout))[0];
  pango2_line_get_extents (line, NULL, &ext);

  g_assert_cmpint (ext.height, >, 0);

  g_object_unref (layout);
  g_object_unref (context);
}

static void
test_line_height2 (void)
{
  Pango2Context *context;
  Pango2Layout *layout;
  Pango2Line *line;
  Pango2Rectangle ext1, ext2;

  context = pango2_context_new ();
  layout = pango2_layout_new (context);
  pango2_layout_set_text (layout, "one", -1);

  line = pango2_lines_get_lines (pango2_layout_get_lines (layout))[0];
  g_assert_nonnull (line);
  pango2_line_get_extents (line, NULL, &ext1);

  pango2_layout_set_text (layout, "", -1);

  line = pango2_lines_get_lines (pango2_layout_get_lines (layout))[0];
  g_assert_nonnull (line);
  pango2_line_get_extents (line, NULL, &ext2);

  g_assert_cmpint (ext1.height, ==, ext2.height);

  g_object_unref (layout);
  g_object_unref (context);
}

static void
test_line_height3 (void)
{
  Pango2Context *context;
  Pango2Layout *layout;
  Pango2Line *line;
  Pango2AttrList *attrs;
  Pango2Rectangle ext1;
  Pango2Rectangle ext2;

  context = pango2_context_new ();
  layout = pango2_layout_new (context);
  pango2_layout_set_text (layout, "one", -1);
  attrs = pango2_attr_list_new ();
  pango2_attr_list_insert (attrs, pango2_attr_line_height_new (2.0));
  pango2_layout_set_attributes (layout, attrs);
  pango2_attr_list_unref (attrs);

  line = pango2_lines_get_lines (pango2_layout_get_lines (layout))[0];
  g_assert_cmpint (pango2_lines_get_line_count (pango2_layout_get_lines (layout)), ==, 1);
  pango2_line_get_extents (line, NULL, &ext1);

  pango2_layout_set_text (layout, "", -1);

  g_assert_cmpint (pango2_lines_get_line_count (pango2_layout_get_lines (layout)), ==, 1);
  line = pango2_lines_get_lines (pango2_layout_get_lines (layout))[0];
  pango2_line_get_extents (line, NULL, &ext2);

  g_assert_cmpint (ext1.height, ==, ext2.height);

  g_object_unref (layout);
  g_object_unref (context);
}

static void
test_run_height (void)
{
  Pango2Context *context;
  Pango2Layout *layout;
  Pango2LineIter *iter;
  Pango2Rectangle logical1, logical2;

  context = pango2_context_new ();
  layout = pango2_layout_new (context);
  pango2_layout_set_text (layout, "one", -1);

  iter = pango2_lines_get_iter (pango2_layout_get_lines (layout));
  pango2_line_iter_get_run_extents (iter, NULL, &logical1);
  pango2_line_iter_free (iter);

  pango2_layout_set_text (layout, "", -1);

  iter = pango2_lines_get_iter (pango2_layout_get_lines (layout));
  pango2_line_iter_get_run_extents (iter, NULL, &logical2);
  pango2_line_iter_free (iter);

  g_assert_cmpint (logical1.height, ==, logical2.height);

  g_object_unref (layout);
  g_object_unref (context);
}

static void
test_cursor_height (void)
{
  Pango2Context *context;
  Pango2Layout *layout;
  Pango2Rectangle strong;

  context = pango2_context_new ();
  layout = pango2_layout_new (context);

  pango2_layout_set_text (layout, "one\ttwo", -1);
  pango2_lines_get_cursor_pos (pango2_layout_get_lines (layout), NULL, 0, &strong, NULL);
  g_assert_cmpint (strong.height, >, 0);

  pango2_layout_set_text (layout, "", -1);
  pango2_lines_get_cursor_pos (pango2_layout_get_lines (layout), NULL, 0, &strong, NULL);
  g_assert_cmpint (strong.height, >, 0);

  g_object_unref (layout);
  g_object_unref (context);
}

static void
test_attr_list_update (void)
{
  Pango2Attribute *weight_attr;
  Pango2Attribute *fg_attr;
  Pango2AttrList *list;
  guint start, end;

  weight_attr = pango2_attr_weight_new (700);
  pango2_attribute_set_range (weight_attr, 4, 6);

  fg_attr = pango2_attr_foreground_new (&(Pango2Color){0, 0, 65535});
  pango2_attribute_set_range (fg_attr, PANGO2_ATTR_INDEX_FROM_TEXT_BEGINNING, PANGO2_ATTR_INDEX_TO_TEXT_END);

  list = pango2_attr_list_new();
  pango2_attr_list_insert (list, weight_attr);
  pango2_attr_list_insert (list, fg_attr);

  pango2_attribute_get_range (weight_attr, &start, &end);
  g_assert_cmpuint (start, ==, 4);
  g_assert_cmpuint (end, ==, 6);
  pango2_attribute_get_range (fg_attr, &start, &end);
  g_assert_cmpuint (start, ==, PANGO2_ATTR_INDEX_FROM_TEXT_BEGINNING);
  g_assert_cmpuint (end, ==, PANGO2_ATTR_INDEX_TO_TEXT_END);

  // Delete 1 byte at position 2
  pango2_attr_list_update (list, 2, 1, 0);

  pango2_attribute_get_range (weight_attr, &start, &end);
  g_assert_cmpuint (start, ==, 3);
  g_assert_cmpuint (end, ==, 5);
  pango2_attribute_get_range (fg_attr, &start, &end);
  g_assert_cmpuint (start, ==, PANGO2_ATTR_INDEX_FROM_TEXT_BEGINNING);
  g_assert_cmpuint (end, ==, PANGO2_ATTR_INDEX_TO_TEXT_END);

  pango2_attr_list_unref (list);
}

static void
test_version_info (void)
{
  char *str;

  g_assert_null (pango2_version_check (1, 0, 0));
  g_assert_null (pango2_version_check (PANGO2_VERSION_MAJOR, PANGO2_VERSION_MINOR, PANGO2_VERSION_MICRO));
  g_assert_nonnull (pango2_version_check (2, 0, 0));

  str = g_strdup_printf ("%d.%d.%d", PANGO2_VERSION_MAJOR, PANGO2_VERSION_MINOR, PANGO2_VERSION_MICRO);
  g_assert_cmpstr (str, ==, pango2_version_string ());
  g_free (str);
}

static void
test_is_zero_width (void)
{
  g_assert_true (pango2_is_zero_width (0x00ad));
  g_assert_true (pango2_is_zero_width (0x034f));
  g_assert_false (pango2_is_zero_width ('a'));
  g_assert_false (pango2_is_zero_width ('c'));

  g_assert_true (pango2_is_zero_width (0x2066));
  g_assert_true (pango2_is_zero_width (0x2067));
  g_assert_true (pango2_is_zero_width (0x2068));
  g_assert_true (pango2_is_zero_width (0x2069));

  g_assert_true (pango2_is_zero_width (0x202a));
  g_assert_true (pango2_is_zero_width (0x202b));
  g_assert_true (pango2_is_zero_width (0x202c));
  g_assert_true (pango2_is_zero_width (0x202d));
  g_assert_true (pango2_is_zero_width (0x202e));
}

static void
test_gravity_to_rotation (void)
{
  g_assert_true (pango2_gravity_to_rotation (PANGO2_GRAVITY_SOUTH) == 0);
  g_assert_true (pango2_gravity_to_rotation (PANGO2_GRAVITY_NORTH) == G_PI);
  g_assert_true (pango2_gravity_to_rotation (PANGO2_GRAVITY_EAST) == -G_PI_2);
  g_assert_true (pango2_gravity_to_rotation (PANGO2_GRAVITY_WEST) == G_PI_2);
}

static void
test_gravity_from_matrix (void)
{
  Pango2Matrix m = PANGO2_MATRIX_INIT;

  g_assert_true (pango2_gravity_get_for_matrix (&m) == PANGO2_GRAVITY_SOUTH);

  pango2_matrix_rotate (&m, 90);
  g_assert_true (pango2_gravity_get_for_matrix (&m) == PANGO2_GRAVITY_WEST);

  pango2_matrix_rotate (&m, 90);
  g_assert_true (pango2_gravity_get_for_matrix (&m) == PANGO2_GRAVITY_NORTH);

  pango2_matrix_rotate (&m, 90);
  g_assert_true (pango2_gravity_get_for_matrix (&m) == PANGO2_GRAVITY_EAST);
}

static void
test_gravity_for_script (void)
{
  struct {
    GUnicodeScript script;
    Pango2Gravity gravity;
    Pango2Gravity gravity_natural;
    Pango2Gravity gravity_line;
  } tests[] = {
    { G_UNICODE_SCRIPT_ARABIC, PANGO2_GRAVITY_SOUTH, PANGO2_GRAVITY_SOUTH, PANGO2_GRAVITY_NORTH },
    { G_UNICODE_SCRIPT_BOPOMOFO, PANGO2_GRAVITY_EAST, PANGO2_GRAVITY_SOUTH, PANGO2_GRAVITY_SOUTH },
    { G_UNICODE_SCRIPT_LATIN, PANGO2_GRAVITY_SOUTH, PANGO2_GRAVITY_SOUTH, PANGO2_GRAVITY_SOUTH },
    { G_UNICODE_SCRIPT_HANGUL, PANGO2_GRAVITY_EAST, PANGO2_GRAVITY_SOUTH, PANGO2_GRAVITY_SOUTH },
    { G_UNICODE_SCRIPT_MONGOLIAN, PANGO2_GRAVITY_WEST, PANGO2_GRAVITY_SOUTH },
    { G_UNICODE_SCRIPT_OGHAM, PANGO2_GRAVITY_WEST, PANGO2_GRAVITY_NORTH, PANGO2_GRAVITY_SOUTH },
    { G_UNICODE_SCRIPT_TIBETAN, PANGO2_GRAVITY_SOUTH, PANGO2_GRAVITY_SOUTH, PANGO2_GRAVITY_SOUTH },
  };

  for (int i = 0; i < G_N_ELEMENTS (tests); i++)
    {
      g_assert_true (pango2_gravity_get_for_script (tests[i].script, PANGO2_GRAVITY_AUTO, PANGO2_GRAVITY_HINT_STRONG) == tests[i].gravity);

      g_assert_true (pango2_gravity_get_for_script_and_width (tests[i].script, FALSE, PANGO2_GRAVITY_EAST, PANGO2_GRAVITY_HINT_NATURAL) == tests[i].gravity_natural);
      g_assert_true (pango2_gravity_get_for_script_and_width (tests[i].script, FALSE, PANGO2_GRAVITY_EAST, PANGO2_GRAVITY_HINT_STRONG) == PANGO2_GRAVITY_EAST);
      g_assert_true (pango2_gravity_get_for_script_and_width (tests[i].script, FALSE, PANGO2_GRAVITY_EAST, PANGO2_GRAVITY_HINT_LINE) == tests[i].gravity_line);
    }
}

static void
test_fallback_shape (void)
{
  Pango2Context *context;
  const char *text;
  GList *items, *l;
  Pango2Direction dir;

  context = pango2_context_new ();
  dir = pango2_context_get_base_dir (context);

  text = "Some text to sha​pe ﺄﻧﺍ ﻕﺍﺩﺭ ﻊﻟﻯ ﺄﻜﻟ ﺎﻟﺰﺟﺎﺟ ﻭ ﻩﺫﺍ ﻻ ﻱﺆﻠﻤﻨﻳ";
  items = pango2_itemize (context, dir, text, 0, strlen (text), NULL);
  for (l = items; l; l = l->next)
    {
      Pango2Item *item = l->data;
      Pango2GlyphString *glyphs;

      /* We want to test fallback shaping, which happens when we don't have a font */
      g_clear_object (&item->analysis.font);

      glyphs = pango2_glyph_string_new ();
      pango2_shape (text + item->offset, item->length, NULL, 0, &item->analysis, glyphs, PANGO2_SHAPE_NONE);

      for (int i = 0; i < glyphs->num_glyphs; i++)
        {
          Pango2Glyph glyph = glyphs->glyphs[i].glyph;
          g_assert_true (glyph == PANGO2_GLYPH_EMPTY || (glyph & PANGO2_GLYPH_UNKNOWN_FLAG));
        }

      pango2_glyph_string_free (glyphs);
    }

  g_list_free_full (items, (GDestroyNotify)pango2_item_free);

  g_object_unref (context);
}

/* https://bugzilla.gnome.org/show_bug.cgi?id=547303 */
static void
test_get_cursor_crash (void)
{
  Pango2Context *context;
  Pango2Layout *layout;
  int i;

  const char *string = "foo\n\rbar\r\nbaz\n\nqux\n\n..";

  context = pango2_context_new ();

  layout = pango2_layout_new (context);

  pango2_layout_set_text (layout, string, -1);

  for (i = 0; string[i]; i++)
    {
      Pango2Rectangle rectA, rectB;

      pango2_lines_get_cursor_pos (pango2_layout_get_lines (layout), NULL, i, &rectA, &rectB);
      g_assert_cmpint (rectA.x, ==, rectB.x);
    }

  g_object_unref (layout);
  g_object_unref (context);
}

/* Test that get_cursor returns split cursors in the
 * expected situations. In particular, this was broken
 * at the end of the string here.
 */
static void
test_get_cursor (void)
{
  const char *text = "abאב";
  Pango2Context *context;
  Pango2Layout *layout;
  Pango2Rectangle strong, weak;

  context = pango2_context_new ();

  layout = pango2_layout_new (context);
  pango2_layout_set_text (layout, text, -1);

  pango2_lines_get_cursor_pos (pango2_layout_get_lines (layout), NULL, 0, &strong, &weak);
  g_assert_cmpint (strong.x, ==, weak.x);

  pango2_lines_get_cursor_pos (pango2_layout_get_lines (layout), NULL, 1, &strong, &weak);
  g_assert_cmpint (strong.x, ==, weak.x);

  pango2_lines_get_cursor_pos (pango2_layout_get_lines (layout), NULL, 2, &strong, &weak);
  g_assert_cmpint (strong.x, !=, weak.x);

  pango2_lines_get_cursor_pos (pango2_layout_get_lines (layout), NULL, 4, &strong, &weak);
  g_assert_cmpint (strong.x, ==, weak.x);

  pango2_lines_get_cursor_pos (pango2_layout_get_lines (layout), NULL, 6, &strong, &weak);
  g_assert_cmpint (strong.x, !=, weak.x);

  g_object_unref (layout);
  g_object_unref (context);
}

static void
test_index_to_x (void)
{
  Pango2Context *context;
  const char *tests[] = {
    "ac­ual­ly", // soft hyphen
    "ac​ual​ly", // zero-width space
  };

  context = pango2_context_new ();

  for (int i = 0; i < G_N_ELEMENTS (tests); i++)
    {
      Pango2Layout *layout;
      const char *text;
      const char *p;

      layout = pango2_layout_new (context);
      pango2_layout_set_text (layout, tests[i], -1);
      text = pango2_layout_get_text (layout);

      for (p = text; *p; p = g_utf8_next_char (p))
        {
          int index = p - text;
          Pango2Line *line;
          int x;
          int index2, trailing;
          gunichar ch;

          ch = g_utf8_get_char (p);

          pango2_lines_index_to_line (pango2_layout_get_lines (layout), index, &line, NULL, NULL, NULL);
          g_assert_nonnull (line);

          pango2_line_index_to_x (line, index, 0, &x);
          pango2_line_x_to_index (line, x, &index2, &trailing);
          if (!pango2_is_zero_width (ch))
            g_assert_cmpint (index, ==, index2);
        }

      g_object_unref (layout);
    }

  g_object_unref (context);
}

static gboolean
pango2_rectangle_contains (const Pango2Rectangle *r1,
                          const Pango2Rectangle *r2)
{
  return r2->x >= r1->x &&
         r2->y >= r1->y &&
         r2->x + r2->width <= r1->x + r1->width &&
         r2->y + r2->height <= r1->y + r1->height;
}

static void
test_extents (void)
{
  Pango2Context *context;
  struct {
    const char *text;
    int width;
  } tests[] = {
#if 0
    { "Some long text that has multiple lines that are wrapped by Pango2.", 60 },
    { "This paragraph should ac­tual­ly have multiple lines, unlike all the other wannabe äöü pa­ra­graph tests in this ugh test-case. Grow some lines!\n", 188 },
    { "你好 Hello שלום Γειά σας", 60 },
#endif
    { "line 1 line 2 line 3\nline 4\r\nline 5", -1 }, // various separators
    { "abc😂️def", -1 },
    { "abcאבגdef", -1 },
    { "אבabcב",
      -1 },
    { "aאב12b", -1 },
    { "pa­ra­graph", -1 }, // soft hyphens
  };

  context = pango2_context_new ();

  for (int i = 0; i < G_N_ELEMENTS (tests); i++)
    {
      Pango2Layout *layout;
      Pango2Lines *lines;
      Pango2LineIter *iter;
      Pango2Rectangle layout_extents;
      Pango2Rectangle line_extents;
      Pango2Rectangle run_extents;
      Pango2Rectangle cluster_extents;
      Pango2Rectangle char_extents;
      Pango2Rectangle pos;
      Pango2Rectangle strong, weak;
      int index;

      layout = pango2_layout_new (context);
      pango2_layout_set_text (layout, tests[i].text, -1);
      pango2_layout_set_width (layout, tests[i].width > 0 ? tests[i].width * PANGO2_SCALE : tests[i].width);

      lines = pango2_layout_get_lines (layout);
      pango2_lines_get_extents (lines, NULL, &layout_extents);

      iter = pango2_lines_get_iter (lines);

      do
        {
          Pango2LeadingTrim trim = PANGO2_LEADING_TRIM_NONE;
          Pango2Line *line;

          line = pango2_line_iter_get_line (iter);
          if (pango2_line_is_paragraph_start (line))
            trim |= PANGO2_LEADING_TRIM_START;
          if (pango2_line_is_paragraph_end (line))
            trim |= PANGO2_LEADING_TRIM_END;

          pango2_line_iter_get_trimmed_line_extents (iter, trim, &line_extents);

          pango2_line_iter_get_run_extents (iter, NULL, &run_extents);
          pango2_line_iter_get_cluster_extents (iter, NULL, &cluster_extents);
          pango2_line_iter_get_char_extents (iter, &char_extents);
          index = pango2_line_iter_get_index (iter);
          pango2_lines_index_to_pos (lines, NULL, index, &pos);
          if (pos.width < 0)
            {
              pos.x += pos.width;
              pos.width = - pos.width;
            }
          pango2_lines_get_cursor_pos (lines, NULL, index, &strong, &weak);

          g_assert_true (pango2_rectangle_contains (&layout_extents, &line_extents));
          g_assert_true (pango2_rectangle_contains (&line_extents, &run_extents));
          g_assert_true (pango2_rectangle_contains (&run_extents, &cluster_extents));
          g_assert_true (pango2_rectangle_contains (&cluster_extents, &char_extents));

          g_assert_true (pango2_rectangle_contains (&run_extents, &pos));
          g_assert_true (pango2_rectangle_contains (&run_extents, &pos));
          g_assert_true (pango2_rectangle_contains (&line_extents, &strong));
          g_assert_true (pango2_rectangle_contains (&line_extents, &weak));
        }
      while (pango2_line_iter_next_char (iter));

      pango2_line_iter_free (iter);
      g_object_unref (layout);
    }

  g_object_unref (context);
}

static void
test_empty_line_height (void)
{
  Pango2Context *context;
  Pango2Layout *layout;
  Pango2FontDescription *description;
  Pango2Rectangle ext1, ext2, ext3;
  cairo_font_options_t *options;
  int hint;
  int size;

  context = pango2_context_new ();
  description = pango2_font_description_new ();

  for (size = 10; size <= 20; size++)
    {
      pango2_font_description_set_size (description, size);

      for (hint = CAIRO_HINT_METRICS_OFF; hint <= CAIRO_HINT_METRICS_ON; hint++)
        {
          options = cairo_font_options_create ();
          cairo_font_options_set_hint_metrics (options, hint);
          pango2_cairo_context_set_font_options (context, options);
          cairo_font_options_destroy (options);

          layout = pango2_layout_new (context);
          pango2_layout_set_font_description (layout, description);

          pango2_lines_get_extents (pango2_layout_get_lines (layout), NULL, &ext1);

          pango2_layout_set_text (layout, "a", 1);

          pango2_lines_get_extents (pango2_layout_get_lines (layout), NULL, &ext2);

          g_assert_cmpint (ext1.height, ==, ext2.height);

          pango2_layout_set_text (layout, "Pg", 1);

          pango2_lines_get_extents (pango2_layout_get_lines (layout), NULL, &ext3);

          g_assert_cmpint (ext2.height, ==, ext3.height);

          g_object_unref (layout);
        }
    }

  pango2_font_description_free (description);
  g_object_unref (context);
}

static void
test_gravity_metrics (void)
{
  Pango2FontMap *map;
  Pango2Context *context;
  Pango2FontDescription *desc;
  Pango2Font *font;
  Pango2Glyph glyph;
  Pango2Gravity gravity;
  Pango2Rectangle ink[4];
  Pango2Rectangle log[4];

  map = pango2_font_map_get_default ();
  context = pango2_context_new_with_font_map (map);

  desc = pango2_font_description_from_string ("Cantarell 64");

  glyph = 1; /* A */

  for (gravity = PANGO2_GRAVITY_SOUTH; gravity <= PANGO2_GRAVITY_WEST; gravity++)
    {
      pango2_font_description_set_gravity (desc, gravity);
      font = pango2_font_map_load_font (map, context, desc);
      pango2_font_get_glyph_extents (font, glyph, &ink[gravity], &log[gravity]);
      g_object_unref (font);
    }

  g_assert_cmpint (ink[PANGO2_GRAVITY_EAST].width, ==, ink[PANGO2_GRAVITY_SOUTH].height);
  g_assert_cmpint (ink[PANGO2_GRAVITY_EAST].height, ==, ink[PANGO2_GRAVITY_SOUTH].width);
  g_assert_cmpint (ink[PANGO2_GRAVITY_NORTH].width, ==, ink[PANGO2_GRAVITY_SOUTH].width);
  g_assert_cmpint (ink[PANGO2_GRAVITY_NORTH].height, ==, ink[PANGO2_GRAVITY_SOUTH].height);
  g_assert_cmpint (ink[PANGO2_GRAVITY_WEST].width, ==, ink[PANGO2_GRAVITY_SOUTH].height);
  g_assert_cmpint (ink[PANGO2_GRAVITY_WEST].height, ==, ink[PANGO2_GRAVITY_SOUTH].width);

  g_assert_cmpint (log[PANGO2_GRAVITY_SOUTH].width, ==, - log[PANGO2_GRAVITY_NORTH].width);
  g_assert_cmpint (log[PANGO2_GRAVITY_EAST].width, ==, - log[PANGO2_GRAVITY_WEST].width);

  pango2_font_description_free (desc);
  g_object_unref (context);
}

static void
test_gravity_metrics2 (void)
{
  Pango2FontMap *map;
  Pango2Context *context;
  Pango2FontDescription *desc;
  Pango2Font *font;
  Pango2Glyph glyph;
  Pango2Gravity gravity;
  Pango2Rectangle ink[4];
  Pango2Rectangle log[4];
  char *path;

  map = pango2_font_map_new ();
  path = g_test_build_filename (G_TEST_DIST, "fonts", "Cantarell-VF.otf", NULL);
  pango2_font_map_add_file (map, path);
  g_free (path);

  context = pango2_context_new_with_font_map (PANGO2_FONT_MAP (map));

  desc = pango2_font_description_from_string ("Cantarell 64");

  glyph = 1; /* A */

  for (gravity = PANGO2_GRAVITY_SOUTH; gravity <= PANGO2_GRAVITY_WEST; gravity++)
    {
      pango2_font_description_set_gravity (desc, gravity);
      font = pango2_font_map_load_font (PANGO2_FONT_MAP (map), context, desc);
      pango2_font_get_glyph_extents (font, glyph, &ink[gravity], &log[gravity]);
      g_object_unref (font);
    }

  g_assert_cmpint (ink[PANGO2_GRAVITY_EAST].width, ==, ink[PANGO2_GRAVITY_SOUTH].height);
  g_assert_cmpint (ink[PANGO2_GRAVITY_EAST].height, ==, ink[PANGO2_GRAVITY_SOUTH].width);
  g_assert_cmpint (ink[PANGO2_GRAVITY_NORTH].width, ==, ink[PANGO2_GRAVITY_SOUTH].width);
  g_assert_cmpint (ink[PANGO2_GRAVITY_NORTH].height, ==, ink[PANGO2_GRAVITY_SOUTH].height);
  g_assert_cmpint (ink[PANGO2_GRAVITY_WEST].width, ==, ink[PANGO2_GRAVITY_SOUTH].height);
  g_assert_cmpint (ink[PANGO2_GRAVITY_WEST].height, ==, ink[PANGO2_GRAVITY_SOUTH].width);

  /* Seems that harfbuzz has some off-by-one differences in advance width
   * when fonts differ by a scale of -1.
   */
  g_assert_cmpint (log[PANGO2_GRAVITY_SOUTH].width + log[PANGO2_GRAVITY_NORTH].width, <=, 1);
  g_assert_cmpint (log[PANGO2_GRAVITY_EAST].width, ==, log[PANGO2_GRAVITY_WEST].width);

  pango2_font_description_free (desc);
  g_object_unref (context);
  g_object_unref (map);
}

static void
test_transform_rectangle (void)
{
  Pango2Matrix matrix = PANGO2_MATRIX_INIT;
  Pango2Rectangle rect;
  Pango2Rectangle rect2;

  rect = rect2 = (Pango2Rectangle) { 10 * PANGO2_SCALE, 20 * PANGO2_SCALE, 30 * PANGO2_SCALE, 40 * PANGO2_SCALE };
  pango2_matrix_transform_rectangle (&matrix, &rect2);

  g_assert_cmpint (rect2.x, ==, rect.x);
  g_assert_cmpint (rect2.y, ==, rect.y);
  g_assert_cmpint (rect2.width, ==, rect.width);
  g_assert_cmpint (rect2.height, ==, rect.height);

  matrix = (Pango2Matrix) PANGO2_MATRIX_INIT;
  pango2_matrix_translate (&matrix, 10, 20);

  pango2_matrix_transform_rectangle (&matrix, &rect2);

  g_assert_cmpint (rect2.x, ==, rect.x + 10 * PANGO2_SCALE);
  g_assert_cmpint (rect2.y, ==, rect.y + 20 * PANGO2_SCALE);
  g_assert_cmpint (rect2.width, ==, rect.width);
  g_assert_cmpint (rect2.height, ==, rect.height);

  rect2 = rect;

  matrix = (Pango2Matrix) PANGO2_MATRIX_INIT;
  pango2_matrix_rotate (&matrix, -90);

  pango2_matrix_transform_rectangle (&matrix, &rect2);

  g_assert_cmpint (rect2.x, ==, - (rect.y + rect.height));
  g_assert_cmpint (rect2.y, ==, rect.x);
  g_assert_cmpint (rect2.width, ==, rect.height);
  g_assert_cmpint (rect2.height, ==, rect.width);
}

static void
test_wrap_char (void)
{
  Pango2Context *context;
  Pango2Layout *layout;
  Pango2Rectangle ext, ext1;

  context = pango2_context_new ();
  layout = pango2_layout_new (context);
  pango2_layout_set_text (layout, "Rows can have suffix widgets", -1);
  pango2_layout_set_wrap (layout, PANGO2_WRAP_WORD_CHAR);

  pango2_layout_set_width (layout, 0);
  pango2_lines_get_extents (pango2_layout_get_lines (layout), NULL, &ext);

  pango2_layout_set_width (layout, ext.width);
  pango2_lines_get_extents (pango2_layout_get_lines (layout), NULL, &ext1);

  g_assert_cmpint (ext.width, ==, ext1.width);
  g_assert_cmpint (ext.height, >=, ext1.height);

  g_object_unref (layout);
  g_object_unref (context);
}

/* Test the crash with Small Caps in itemization from #627 */
static void
test_small_caps_crash (void)
{
  Pango2Context *context;
  Pango2Layout *layout;
  Pango2FontDescription *desc;
  Pango2Rectangle ext;

  context = pango2_context_new ();
  layout = pango2_layout_new (context);
  desc = pango2_font_description_from_string ("Cantarell Small-Caps 11");
  pango2_layout_set_font_description (layout, desc);

  pango2_layout_set_text (layout, "Pere Ràfols Soler\nEqualiser, LV2\nAudio: 1, 1\nMidi: 0, 0\nControls: 53, 2\nCV: 0, 0", -1);

  pango2_lines_get_extents (pango2_layout_get_lines (layout), NULL, &ext);

  pango2_font_description_free (desc);
  g_object_unref (layout);
  g_object_unref (context);
}

int
main (int argc, char *argv[])
{
  g_test_init (&argc, &argv, NULL);

  g_test_add_func ("/layout/itemize-empty-crash", test_itemize_empty_crash);
  g_test_add_func ("/layout/itemize-utf8", test_itemize_utf8);
  g_test_add_func ("/layout/short-string-crash", test_short_string_crash);
  g_test_add_func ("/language/emoji-crash", test_language_emoji_crash);
  g_test_add_func ("/layout/line-height", test_line_height);
  g_test_add_func ("/layout/line-height2", test_line_height2);
  g_test_add_func ("/layout/line-height3", test_line_height3);
  g_test_add_func ("/layout/run-height", test_run_height);
  g_test_add_func ("/layout/cursor-height", test_cursor_height);
  g_test_add_func ("/attr-list/update", test_attr_list_update);
  g_test_add_func ("/misc/version-info", test_version_info);
  g_test_add_func ("/misc/is-zerowidth", test_is_zero_width);
  g_test_add_func ("/gravity/to-rotation", test_gravity_to_rotation);
  g_test_add_func ("/gravity/from-matrix", test_gravity_from_matrix);
  g_test_add_func ("/gravity/for-script", test_gravity_for_script);
  g_test_add_func ("/layout/fallback-shape", test_fallback_shape);
  g_test_add_func ("/bidi/get-cursor-crash", test_get_cursor_crash);
  g_test_add_func ("/bidi/get-cursor", test_get_cursor);
  g_test_add_func ("/layout/index-to-x", test_index_to_x);
  g_test_add_func ("/layout/extents", test_extents);
  g_test_add_func ("/layout/empty-line-height", test_empty_line_height);
  g_test_add_func ("/layout/gravity-metrics", test_gravity_metrics);
  g_test_add_func ("/layout/wrap-char", test_wrap_char);
  g_test_add_func ("/layout/gravity-metrics2", test_gravity_metrics2);
  g_test_add_func ("/matrix/transform-rectangle", test_transform_rectangle);
  g_test_add_func ("/itemize/small-caps-crash", test_small_caps_crash);

  return g_test_run ();
}
