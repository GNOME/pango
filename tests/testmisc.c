/* Pango
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
#include <pango/pangocairo.h>

/* Test that itemizing a string with 0 characters works
 */
static void
test_itemize_empty_crash (void)
{
  PangoContext *context;

  context = pango_font_map_create_context (pango_cairo_font_map_get_default ());
  pango_itemize (context, PANGO_DIRECTION_LTR, "", 0, 1, NULL);

  g_object_unref (context);
}

static void
test_itemize_utf8 (void)
{
  PangoContext *context;
  GList *result = NULL;

  context = pango_font_map_create_context (pango_cairo_font_map_get_default ());
  result = pango_itemize (context, PANGO_DIRECTION_LTR, "\xc3\xa1\na", 3, 1, NULL);
  g_assert (result != NULL);

  g_list_free_full (result, (GDestroyNotify)pango_item_free);
  g_object_unref (context);
}

/* Test that pango_layout_set_text (layout, "short", 200)
 * does not lead to a crash. (pidgin does this)
 */
static void
test_short_string_crash (void)
{
  PangoContext *context;
  PangoLayout *layout;
  PangoRectangle ext;

  context = pango_font_map_create_context (pango_cairo_font_map_get_default ());
  layout = pango_layout_new (context);
  pango_layout_set_text (layout, "short text", 200);
  pango_lines_get_extents (pango_layout_get_lines (layout), &ext, &ext);

  g_object_unref (layout);
  g_object_unref (context);
}

static void
test_language_emoji_crash (void)
{
  PangoLanguage *lang;
  const GUnicodeScript *scripts;
  int num;

  lang = pango_language_from_string ("und-zsye");
  scripts = pango_language_get_scripts (lang, &num);

  g_assert (num >= 0);
  g_assert (scripts == NULL || num > 0);
}

static void
test_line_height (void)
{
  PangoContext *context;
  PangoLayout *layout;
  PangoLine *line;
  PangoRectangle ext;

  context = pango_font_map_create_context (pango_cairo_font_map_get_default ());
  layout = pango_layout_new (context);
  pango_layout_set_text (layout, "one\ttwo", -1);
  line = pango_lines_get_lines (pango_layout_get_lines (layout))[0];
  pango_line_get_extents (line, NULL, &ext);

  g_assert_cmpint (ext.height, >, 0);

  g_object_unref (layout);
  g_object_unref (context);
}

static void
test_line_height2 (void)
{
  PangoContext *context;
  PangoLayout *layout;
  PangoLine *line;
  PangoRectangle ext1, ext2;

  context = pango_font_map_create_context (pango_cairo_font_map_get_default ());
  layout = pango_layout_new (context);
  pango_layout_set_text (layout, "one", -1);

  line = pango_lines_get_lines (pango_layout_get_lines (layout))[0];
  g_assert_nonnull (line);
  pango_line_get_extents (line, NULL, &ext1);

  pango_layout_set_text (layout, "", -1);

  line = pango_lines_get_lines (pango_layout_get_lines (layout))[0];
  g_assert_nonnull (line);
  pango_line_get_extents (line, NULL, &ext2);

  g_assert_cmpint (ext1.height, ==, ext2.height);

  g_object_unref (layout);
  g_object_unref (context);
}

static void
test_line_height3 (void)
{
  PangoContext *context;
  PangoLayout *layout;
  PangoLine *line;
  PangoAttrList *attrs;
  PangoRectangle ext1;
  PangoRectangle ext2;

  context = pango_font_map_create_context (pango_cairo_font_map_get_default ());
  layout = pango_layout_new (context);
  pango_layout_set_text (layout, "one", -1);
  attrs = pango_attr_list_new ();
  pango_attr_list_insert (attrs, pango_attr_line_height_new (2.0));
  pango_layout_set_attributes (layout, attrs);
  pango_attr_list_unref (attrs);

  line = pango_lines_get_lines (pango_layout_get_lines (layout))[0];
  g_assert_cmpint (pango_lines_get_line_count (pango_layout_get_lines (layout)), ==, 1);
  pango_line_get_extents (line, NULL, &ext1);

  pango_layout_set_text (layout, "", -1);

  g_assert_cmpint (pango_lines_get_line_count (pango_layout_get_lines (layout)), ==, 1);
  line = pango_lines_get_lines (pango_layout_get_lines (layout))[0];
  pango_line_get_extents (line, NULL, &ext2);

  g_assert_cmpint (ext1.height, ==, ext2.height);

  g_object_unref (layout);
  g_object_unref (context);
}

static void
test_run_height (void)
{
  PangoContext *context;
  PangoLayout *layout;
  PangoLineIter *iter;
  PangoRectangle logical1, logical2;

  if (strcmp (G_OBJECT_TYPE_NAME (pango_cairo_font_map_get_default ()), "PangoCairoCoreTextFontMap") == 0)
    {
      g_test_skip ("This test fails on macOS and needs debugging");
      return;
    }

  context = pango_font_map_create_context (pango_cairo_font_map_get_default ());
  layout = pango_layout_new (context);
  pango_layout_set_text (layout, "one", -1);

  iter = pango_lines_get_iter (pango_layout_get_lines (layout));
  pango_line_iter_get_run_extents (iter, NULL, &logical1);
  pango_line_iter_free (iter);

  pango_layout_set_text (layout, "", -1);

  iter = pango_lines_get_iter (pango_layout_get_lines (layout));
  pango_line_iter_get_run_extents (iter, NULL, &logical2);
  pango_line_iter_free (iter);

  g_assert_cmpint (logical1.height, ==, logical2.height);

  g_object_unref (layout);
  g_object_unref (context);
}

static void
test_cursor_height (void)
{
  PangoContext *context;
  PangoLayout *layout;
  PangoRectangle strong;

  context = pango_font_map_create_context (pango_cairo_font_map_get_default ());
  layout = pango_layout_new (context);

  pango_layout_set_text (layout, "one\ttwo", -1);
  pango_lines_get_cursor_pos (pango_layout_get_lines (layout), NULL, 0, &strong, NULL);
  g_assert_cmpint (strong.height, >, 0);

  pango_layout_set_text (layout, "", -1);
  pango_lines_get_cursor_pos (pango_layout_get_lines (layout), NULL, 0, &strong, NULL);
  g_assert_cmpint (strong.height, >, 0);

  g_object_unref (layout);
  g_object_unref (context);
}

static void
test_attr_list_update (void)
{
  PangoAttribute *weight_attr;
  PangoAttribute *fg_attr;
  PangoAttrList *list;

  weight_attr = pango_attr_weight_new (700);
  weight_attr->start_index = 4;
  weight_attr->end_index = 6;

  fg_attr = pango_attr_foreground_new (&(PangoColor){0, 0, 65535});
  fg_attr->start_index = PANGO_ATTR_INDEX_FROM_TEXT_BEGINNING;
  fg_attr->end_index = PANGO_ATTR_INDEX_TO_TEXT_END;

  list = pango_attr_list_new();
  pango_attr_list_insert (list, weight_attr);
  pango_attr_list_insert (list, fg_attr);

  g_assert_cmpuint (weight_attr->start_index, ==, 4);
  g_assert_cmpuint (weight_attr->end_index, ==, 6);
  g_assert_cmpuint (fg_attr->start_index, ==, PANGO_ATTR_INDEX_FROM_TEXT_BEGINNING);
  g_assert_cmpuint (fg_attr->end_index, ==, PANGO_ATTR_INDEX_TO_TEXT_END);

  // Delete 1 byte at position 2
  pango_attr_list_update (list, 2, 1, 0);

  g_assert_cmpuint (weight_attr->start_index, ==, 3);
  g_assert_cmpuint (weight_attr->end_index, ==, 5);
  g_assert_cmpuint (fg_attr->start_index, ==, PANGO_ATTR_INDEX_FROM_TEXT_BEGINNING);
  g_assert_cmpuint (fg_attr->end_index, ==, PANGO_ATTR_INDEX_TO_TEXT_END);

  pango_attr_list_unref (list);
}

static void
test_version_info (void)
{
  char *str;

  g_assert_null (pango_version_check (1, 0, 0));
  g_assert_null (pango_version_check (PANGO_VERSION_MAJOR, PANGO_VERSION_MINOR, PANGO_VERSION_MICRO));
  g_assert_nonnull (pango_version_check (2, 0, 0));

  str = g_strdup_printf ("%d.%d.%d", PANGO_VERSION_MAJOR, PANGO_VERSION_MINOR, PANGO_VERSION_MICRO);
  g_assert_cmpstr (str, ==, pango_version_string ());
  g_free (str);
}

static void
test_is_zero_width (void)
{
  g_assert_true (pango_is_zero_width (0x00ad));
  g_assert_true (pango_is_zero_width (0x034f));
  g_assert_false (pango_is_zero_width ('a'));
  g_assert_false (pango_is_zero_width ('c'));

  g_assert_true (pango_is_zero_width (0x2066));
  g_assert_true (pango_is_zero_width (0x2067));
  g_assert_true (pango_is_zero_width (0x2068));
  g_assert_true (pango_is_zero_width (0x2069));

  g_assert_true (pango_is_zero_width (0x202a));
  g_assert_true (pango_is_zero_width (0x202b));
  g_assert_true (pango_is_zero_width (0x202c));
  g_assert_true (pango_is_zero_width (0x202d));
  g_assert_true (pango_is_zero_width (0x202e));
}

static void
test_gravity_to_rotation (void)
{
  g_assert_true (pango_gravity_to_rotation (PANGO_GRAVITY_SOUTH) == 0);
  g_assert_true (pango_gravity_to_rotation (PANGO_GRAVITY_NORTH) == G_PI);
  g_assert_true (pango_gravity_to_rotation (PANGO_GRAVITY_EAST) == -G_PI_2);
  g_assert_true (pango_gravity_to_rotation (PANGO_GRAVITY_WEST) == G_PI_2);
}

static void
test_gravity_from_matrix (void)
{
  PangoMatrix m = PANGO_MATRIX_INIT;

  g_assert_true (pango_gravity_get_for_matrix (&m) == PANGO_GRAVITY_SOUTH);

  pango_matrix_rotate (&m, 90);
  g_assert_true (pango_gravity_get_for_matrix (&m) == PANGO_GRAVITY_WEST);

  pango_matrix_rotate (&m, 90);
  g_assert_true (pango_gravity_get_for_matrix (&m) == PANGO_GRAVITY_NORTH);

  pango_matrix_rotate (&m, 90);
  g_assert_true (pango_gravity_get_for_matrix (&m) == PANGO_GRAVITY_EAST);
}

static void
test_gravity_for_script (void)
{
  struct {
    GUnicodeScript script;
    PangoGravity gravity;
    PangoGravity gravity_natural;
    PangoGravity gravity_line;
  } tests[] = {
    { G_UNICODE_SCRIPT_ARABIC, PANGO_GRAVITY_SOUTH, PANGO_GRAVITY_SOUTH, PANGO_GRAVITY_NORTH },
    { G_UNICODE_SCRIPT_BOPOMOFO, PANGO_GRAVITY_EAST, PANGO_GRAVITY_SOUTH, PANGO_GRAVITY_SOUTH },
    { G_UNICODE_SCRIPT_LATIN, PANGO_GRAVITY_SOUTH, PANGO_GRAVITY_SOUTH, PANGO_GRAVITY_SOUTH },
    { G_UNICODE_SCRIPT_HANGUL, PANGO_GRAVITY_EAST, PANGO_GRAVITY_SOUTH, PANGO_GRAVITY_SOUTH },
    { G_UNICODE_SCRIPT_MONGOLIAN, PANGO_GRAVITY_WEST, PANGO_GRAVITY_SOUTH },
    { G_UNICODE_SCRIPT_OGHAM, PANGO_GRAVITY_WEST, PANGO_GRAVITY_NORTH, PANGO_GRAVITY_SOUTH },
    { G_UNICODE_SCRIPT_TIBETAN, PANGO_GRAVITY_SOUTH, PANGO_GRAVITY_SOUTH, PANGO_GRAVITY_SOUTH },
  };

  for (int i = 0; i < G_N_ELEMENTS (tests); i++)
    {
      g_assert_true (pango_gravity_get_for_script (tests[i].script, PANGO_GRAVITY_AUTO, PANGO_GRAVITY_HINT_STRONG) == tests[i].gravity);

      g_assert_true (pango_gravity_get_for_script_and_width (tests[i].script, FALSE, PANGO_GRAVITY_EAST, PANGO_GRAVITY_HINT_NATURAL) == tests[i].gravity_natural);
      g_assert_true (pango_gravity_get_for_script_and_width (tests[i].script, FALSE, PANGO_GRAVITY_EAST, PANGO_GRAVITY_HINT_STRONG) == PANGO_GRAVITY_EAST);
      g_assert_true (pango_gravity_get_for_script_and_width (tests[i].script, FALSE, PANGO_GRAVITY_EAST, PANGO_GRAVITY_HINT_LINE) == tests[i].gravity_line);
    }
}

static void
test_fallback_shape (void)
{
  PangoContext *context;
  const char *text;
  GList *items, *l;
  PangoDirection dir;

  context = pango_font_map_create_context (pango_cairo_font_map_get_default ());
  dir = pango_context_get_base_dir (context);

  text = "Some text to sha​pe ﺄﻧﺍ ﻕﺍﺩﺭ ﻊﻟﻯ ﺄﻜﻟ ﺎﻟﺰﺟﺎﺟ ﻭ ﻩﺫﺍ ﻻ ﻱﺆﻠﻤﻨﻳ";
  items = pango_itemize (context, dir, text, 0, strlen (text), NULL);
  for (l = items; l; l = l->next)
    {
      PangoItem *item = l->data;
      PangoGlyphString *glyphs;

      /* We want to test fallback shaping, which happens when we don't have a font */
      g_clear_object (&item->analysis.font);

      glyphs = pango_glyph_string_new ();
      pango_shape (text + item->offset, item->length, NULL, 0, &item->analysis, glyphs, PANGO_SHAPE_NONE);

      for (int i = 0; i < glyphs->num_glyphs; i++)
        {
          PangoGlyph glyph = glyphs->glyphs[i].glyph;
          g_assert_true (glyph == PANGO_GLYPH_EMPTY || (glyph & PANGO_GLYPH_UNKNOWN_FLAG));
        }

      pango_glyph_string_free (glyphs);
    }

  g_list_free_full (items, (GDestroyNotify)pango_item_free);

  g_object_unref (context);
}

/* https://bugzilla.gnome.org/show_bug.cgi?id=547303 */
static void
test_get_cursor_crash (void)
{
  PangoContext *context;
  PangoLayout *layout;
  int i;

  const char *string = "foo\n\rbar\r\nbaz\n\nqux\n\n..";

  context = pango_font_map_create_context (pango_cairo_font_map_get_default ());

  layout = pango_layout_new (context);

  pango_layout_set_text (layout, string, -1);

  for (i = 0; string[i]; i++)
    {
      PangoRectangle rectA, rectB;

      pango_lines_get_cursor_pos (pango_layout_get_lines (layout), NULL, i, &rectA, &rectB);
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
  PangoContext *context;
  PangoLayout *layout;
  PangoRectangle strong, weak;

  context = pango_font_map_create_context (pango_cairo_font_map_get_default ());

  layout = pango_layout_new (context);
  pango_layout_set_text (layout, text, -1);

  pango_lines_get_cursor_pos (pango_layout_get_lines (layout), NULL, 0, &strong, &weak);
  g_assert_cmpint (strong.x, ==, weak.x);

  pango_lines_get_cursor_pos (pango_layout_get_lines (layout), NULL, 1, &strong, &weak);
  g_assert_cmpint (strong.x, ==, weak.x);

  pango_lines_get_cursor_pos (pango_layout_get_lines (layout), NULL, 2, &strong, &weak);
  g_assert_cmpint (strong.x, !=, weak.x);

  pango_lines_get_cursor_pos (pango_layout_get_lines (layout), NULL, 4, &strong, &weak);
  g_assert_cmpint (strong.x, ==, weak.x);

  pango_lines_get_cursor_pos (pango_layout_get_lines (layout), NULL, 6, &strong, &weak);
  g_assert_cmpint (strong.x, !=, weak.x);

  g_object_unref (layout);
  g_object_unref (context);
}

static void
test_index_to_x (void)
{
  PangoContext *context;
  const char *tests[] = {
    "ac­ual­ly", // soft hyphen
    "ac​ual​ly", // zero-width space
  };

  context = pango_font_map_create_context (pango_cairo_font_map_get_default ());

  for (int i = 0; i < G_N_ELEMENTS (tests); i++)
    {
      PangoLayout *layout;
      const char *text;
      const char *p;

      layout = pango_layout_new (context);
      pango_layout_set_text (layout, tests[i], -1);
      text = pango_layout_get_text (layout);

      for (p = text; *p; p = g_utf8_next_char (p))
        {
          int index = p - text;
          PangoLine *line;
          int x;
          int index2, trailing;
          gunichar ch;

          ch = g_utf8_get_char (p);

          pango_lines_index_to_line (pango_layout_get_lines (layout), index, &line, NULL, NULL, NULL);
          g_assert_nonnull (line);

          pango_line_index_to_x (line, index, 0, &x);
          pango_line_x_to_index (line, x, &index2, &trailing);
          if (!pango_is_zero_width (ch))
            g_assert_cmpint (index, ==, index2);
        }

      g_object_unref (layout);
    }

  g_object_unref (context);
}

static gboolean
pango_rectangle_contains (const PangoRectangle *r1,
                          const PangoRectangle *r2)
{
  return r2->x >= r1->x &&
         r2->y >= r1->y &&
         r2->x + r2->width <= r1->x + r1->width &&
         r2->y + r2->height <= r1->y + r1->height;
}

static void
test_extents (void)
{
  PangoContext *context;
  struct {
    const char *text;
    int width;
  } tests[] = {
#if 0
    { "Some long text that has multiple lines that are wrapped by Pango.", 60 },
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

  context = pango_font_map_create_context (pango_cairo_font_map_get_default ());

  for (int i = 0; i < G_N_ELEMENTS (tests); i++)
    {
      PangoLayout *layout;
      PangoLines *lines;
      PangoLineIter *iter;
      PangoRectangle layout_extents;
      PangoRectangle line_extents;
      PangoRectangle run_extents;
      PangoRectangle cluster_extents;
      PangoRectangle char_extents;
      PangoRectangle pos;
      PangoRectangle strong, weak;
      int index;

      layout = pango_layout_new (context);
      pango_layout_set_text (layout, tests[i].text, -1);
      pango_layout_set_width (layout, tests[i].width > 0 ? tests[i].width * PANGO_SCALE : tests[i].width);

      lines = pango_layout_get_lines (layout);
      pango_lines_get_extents (lines, NULL, &layout_extents);

      iter = pango_lines_get_iter (lines);

      do
        {
          PangoLeadingTrim trim = PANGO_LEADING_TRIM_NONE;
          PangoLine *line;

          line = pango_line_iter_get_line (iter);
          if (pango_line_is_paragraph_start (line))
            trim |= PANGO_LEADING_TRIM_START;
          if (pango_line_is_paragraph_end (line))
            trim |= PANGO_LEADING_TRIM_END;

          pango_line_iter_get_trimmed_line_extents (iter, trim, &line_extents);

          pango_line_iter_get_run_extents (iter, NULL, &run_extents);
          pango_line_iter_get_cluster_extents (iter, NULL, &cluster_extents);
          pango_line_iter_get_char_extents (iter, &char_extents);
          index = pango_line_iter_get_index (iter);
          pango_lines_index_to_pos (lines, NULL, index, &pos);
          if (pos.width < 0)
            {
              pos.x += pos.width;
              pos.width = - pos.width;
            }
          pango_lines_get_cursor_pos (lines, NULL, index, &strong, &weak);

          g_assert_true (pango_rectangle_contains (&layout_extents, &line_extents));
          g_assert_true (pango_rectangle_contains (&line_extents, &run_extents));
          g_assert_true (pango_rectangle_contains (&run_extents, &cluster_extents));
          g_assert_true (pango_rectangle_contains (&cluster_extents, &char_extents));

          g_assert_true (pango_rectangle_contains (&run_extents, &pos));
          g_assert_true (pango_rectangle_contains (&run_extents, &pos));
          g_assert_true (pango_rectangle_contains (&line_extents, &strong));
          g_assert_true (pango_rectangle_contains (&line_extents, &weak));
        }
      while (pango_line_iter_next_char (iter));

      pango_line_iter_free (iter);
      g_object_unref (layout);
    }

  g_object_unref (context);
}

static void
test_empty_line_height (void)
{
  PangoContext *context;
  PangoLayout *layout;
  PangoFontDescription *description;
  PangoRectangle ext1, ext2, ext3;
  cairo_font_options_t *options;
  int hint;
  int size;

  if (strcmp (G_OBJECT_TYPE_NAME (pango_cairo_font_map_get_default ()), "PangoCairoCoreTextFontMap") == 0)
    {
      g_test_skip ("This test fails on macOS and needs debugging");
      return;
    }

  context = pango_font_map_create_context (pango_cairo_font_map_get_default ());
  description = pango_font_description_new ();

  for (size = 10; size <= 20; size++)
    {
      pango_font_description_set_size (description, size);

      for (hint = CAIRO_HINT_METRICS_OFF; hint <= CAIRO_HINT_METRICS_ON; hint++)
        {
          options = cairo_font_options_create ();
          cairo_font_options_set_hint_metrics (options, hint);
          pango_cairo_context_set_font_options (context, options);
          cairo_font_options_destroy (options);

          layout = pango_layout_new (context);
          pango_layout_set_font_description (layout, description);

          pango_lines_get_extents (pango_layout_get_lines (layout), NULL, &ext1);

          pango_layout_set_text (layout, "a", 1);

          pango_lines_get_extents (pango_layout_get_lines (layout), NULL, &ext2);

          g_assert_cmpint (ext1.height, ==, ext2.height);

          pango_layout_set_text (layout, "Pg", 1);

          pango_lines_get_extents (pango_layout_get_lines (layout), NULL, &ext3);

          g_assert_cmpint (ext2.height, ==, ext3.height);

          g_object_unref (layout);
        }
    }

  pango_font_description_free (description);
  g_object_unref (context);
}

static void
test_gravity_metrics (void)
{
  PangoFontMap *map;
  PangoContext *context;
  PangoFontDescription *desc;
  PangoFont *font;
  PangoGlyph glyph;
  PangoGravity gravity;
  PangoRectangle ink[4];
  PangoRectangle log[4];

  map = pango_cairo_font_map_get_default ();
  context = pango_font_map_create_context (map);

  desc = pango_font_description_from_string ("Cantarell 64");

  glyph = 1; /* A */

  for (gravity = PANGO_GRAVITY_SOUTH; gravity <= PANGO_GRAVITY_WEST; gravity++)
    {
      pango_font_description_set_gravity (desc, gravity);
      font = pango_font_map_load_font (map, context, desc);
      pango_font_get_glyph_extents (font, glyph, &ink[gravity], &log[gravity]);
      g_object_unref (font);
    }

  g_assert_cmpint (ink[PANGO_GRAVITY_EAST].width, ==, ink[PANGO_GRAVITY_SOUTH].height);
  g_assert_cmpint (ink[PANGO_GRAVITY_EAST].height, ==, ink[PANGO_GRAVITY_SOUTH].width);
  g_assert_cmpint (ink[PANGO_GRAVITY_NORTH].width, ==, ink[PANGO_GRAVITY_SOUTH].width);
  g_assert_cmpint (ink[PANGO_GRAVITY_NORTH].height, ==, ink[PANGO_GRAVITY_SOUTH].height);
  g_assert_cmpint (ink[PANGO_GRAVITY_WEST].width, ==, ink[PANGO_GRAVITY_SOUTH].height);
  g_assert_cmpint (ink[PANGO_GRAVITY_WEST].height, ==, ink[PANGO_GRAVITY_SOUTH].width);

  g_assert_cmpint (log[PANGO_GRAVITY_SOUTH].width, ==, - log[PANGO_GRAVITY_NORTH].width);
  g_assert_cmpint (log[PANGO_GRAVITY_EAST].width, ==, - log[PANGO_GRAVITY_WEST].width);

  pango_font_description_free (desc);
  g_object_unref (context);
}

static void
test_gravity_metrics2 (void)
{
  PangoFontMap *map;
  PangoContext *context;
  PangoFontDescription *desc;
  PangoFont *font;
  PangoGlyph glyph;
  PangoGravity gravity;
  PangoRectangle ink[4];
  PangoRectangle log[4];
  char *path;

  map = pango_font_map_new ();
  path = g_test_build_filename (G_TEST_DIST, "fonts", "Cantarell-VF.otf", NULL);
  pango_font_map_add_file (map, path);
  g_free (path);

  context = pango_font_map_create_context (PANGO_FONT_MAP (map));

  desc = pango_font_description_from_string ("Cantarell 64");

  glyph = 1; /* A */

  for (gravity = PANGO_GRAVITY_SOUTH; gravity <= PANGO_GRAVITY_WEST; gravity++)
    {
      pango_font_description_set_gravity (desc, gravity);
      font = pango_font_map_load_font (PANGO_FONT_MAP (map), context, desc);
      pango_font_get_glyph_extents (font, glyph, &ink[gravity], &log[gravity]);
      g_object_unref (font);
    }

  g_assert_cmpint (ink[PANGO_GRAVITY_EAST].width, ==, ink[PANGO_GRAVITY_SOUTH].height);
  g_assert_cmpint (ink[PANGO_GRAVITY_EAST].height, ==, ink[PANGO_GRAVITY_SOUTH].width);
  g_assert_cmpint (ink[PANGO_GRAVITY_NORTH].width, ==, ink[PANGO_GRAVITY_SOUTH].width);
  g_assert_cmpint (ink[PANGO_GRAVITY_NORTH].height, ==, ink[PANGO_GRAVITY_SOUTH].height);
  g_assert_cmpint (ink[PANGO_GRAVITY_WEST].width, ==, ink[PANGO_GRAVITY_SOUTH].height);
  g_assert_cmpint (ink[PANGO_GRAVITY_WEST].height, ==, ink[PANGO_GRAVITY_SOUTH].width);

  /* Seems that harfbuzz has some off-by-one differences in advance width
   * when fonts differ by a scale of -1.
   */
  g_assert_cmpint (log[PANGO_GRAVITY_SOUTH].width + log[PANGO_GRAVITY_NORTH].width, <=, 1);
  g_assert_cmpint (log[PANGO_GRAVITY_EAST].width, ==, log[PANGO_GRAVITY_WEST].width);

  pango_font_description_free (desc);
  g_object_unref (context);
  g_object_unref (map);
}

static void
test_transform_rectangle (void)
{
  PangoMatrix matrix = PANGO_MATRIX_INIT;
  PangoRectangle rect;
  PangoRectangle rect2;

  rect = rect2 = (PangoRectangle) { 10 * PANGO_SCALE, 20 * PANGO_SCALE, 30 * PANGO_SCALE, 40 * PANGO_SCALE };
  pango_matrix_transform_rectangle (&matrix, &rect2);

  g_assert_cmpint (rect2.x, ==, rect.x);
  g_assert_cmpint (rect2.y, ==, rect.y);
  g_assert_cmpint (rect2.width, ==, rect.width);
  g_assert_cmpint (rect2.height, ==, rect.height);

  matrix = (PangoMatrix) PANGO_MATRIX_INIT;
  pango_matrix_translate (&matrix, 10, 20);

  pango_matrix_transform_rectangle (&matrix, &rect2);

  g_assert_cmpint (rect2.x, ==, rect.x + 10 * PANGO_SCALE);
  g_assert_cmpint (rect2.y, ==, rect.y + 20 * PANGO_SCALE);
  g_assert_cmpint (rect2.width, ==, rect.width);
  g_assert_cmpint (rect2.height, ==, rect.height);

  rect2 = rect;

  matrix = (PangoMatrix) PANGO_MATRIX_INIT;
  pango_matrix_rotate (&matrix, -90);

  pango_matrix_transform_rectangle (&matrix, &rect2);

  g_assert_cmpint (rect2.x, ==, - (rect.y + rect.height));
  g_assert_cmpint (rect2.y, ==, rect.x);
  g_assert_cmpint (rect2.width, ==, rect.height);
  g_assert_cmpint (rect2.height, ==, rect.width);
}

static void
test_wrap_char (void)
{
  PangoContext *context;
  PangoLayout *layout;
  PangoRectangle ext, ext1;

  context = pango_font_map_create_context (pango_cairo_font_map_get_default ());
  layout = pango_layout_new (context);
  pango_layout_set_text (layout, "Rows can have suffix widgets", -1);
  pango_layout_set_wrap (layout, PANGO_WRAP_WORD_CHAR);

  pango_layout_set_width (layout, 0);
  pango_lines_get_extents (pango_layout_get_lines (layout), NULL, &ext);

  pango_layout_set_width (layout, ext.width);
  pango_lines_get_extents (pango_layout_get_lines (layout), NULL, &ext1);

  g_assert_cmpint (ext.width, ==, ext1.width);
  g_assert_cmpint (ext.height, >=, ext1.height);

  g_object_unref (layout);
  g_object_unref (context);
}

/* Test the crash with Small Caps in itemization from #627 */
static void
test_small_caps_crash (void)
{
  PangoContext *context;
  PangoLayout *layout;
  PangoFontDescription *desc;
  PangoRectangle ext;

  if (strcmp (G_OBJECT_TYPE_NAME (pango_cairo_font_map_get_default ()), "PangoCairoCoreTextFontMap") == 0)
    {
      g_test_skip ("This test needs a fontmap that supports Small-Caps");
      return;
    }

  context = pango_font_map_create_context (pango_cairo_font_map_get_default ());
  layout = pango_layout_new (context);
  desc = pango_font_description_from_string ("Cantarell Small-Caps 11");
  pango_layout_set_font_description (layout, desc);

  pango_layout_set_text (layout, "Pere Ràfols Soler\nEqualiser, LV2\nAudio: 1, 1\nMidi: 0, 0\nControls: 53, 2\nCV: 0, 0", -1);

  pango_lines_get_extents (pango_layout_get_lines (layout), NULL, &ext);

  pango_font_description_free (desc);
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
