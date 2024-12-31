/* Pango2
 * test-bidi.c: Test bidi apis
 *
 * Copyright (C) 2021 Red Hat, Inc.
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

#include <locale.h>
#include <pango2/pango.h>

#if 0
static void
test_bidi_embedding_levels (void)
{
  /* Examples taken from https://www.w3.org/International/articles/inline-bidi-markup/uba-basics */
  struct {
    const char *text;
    Pango2Direction dir;
    const char *levels;
    Pango2Direction out_dir;
  } tests[] = {
    { "bahrain مصر kuwait", PANGO2_DIRECTION_LTR, "\0\0\0\0\0\0\0\0\1\1\1\0\0\0\0\0\0\0", PANGO2_DIRECTION_LTR },
    { "bahrain مصر kuwait", PANGO2_DIRECTION_WEAK_LTR, "\0\0\0\0\0\0\0\0\1\1\1\0\0\0\0\0\0\0", PANGO2_DIRECTION_LTR },
    { "bahrain مصر kuwait", PANGO2_DIRECTION_RTL, "\2\2\2\2\2\2\2\1\1\1\1\1\2\2\2\2\2\2", PANGO2_DIRECTION_RTL },
    { "bahrain مصر kuwait", PANGO2_DIRECTION_WEAK_RTL, "\0\0\0\0\0\0\0\0\1\1\1\0\0\0\0\0\0\0", PANGO2_DIRECTION_LTR },
    { "The title is مفتاح معايير الويب in Arabic.", PANGO2_DIRECTION_LTR, "\0\0\0\0\0\0\0\0\0\0\0\0\0\1\1\1\1\1\1\1\1\1\1\1\1\1\1\1\1\1\1\0\0\0\0\0\0\0\0\0\0\0", PANGO2_DIRECTION_LTR },
    { "The title is مفتاح معايير الويب, in Arabic.", PANGO2_DIRECTION_LTR, "\0\0\0\0\0\0\0\0\0\0\0\0\0\1\1\1\1\1\1\1\1\1\1\1\1\1\1\1\1\1\1\0\0\0\0\0\0\0\0\0\0\0\0", PANGO2_DIRECTION_LTR },
    { "The title is مفتاح معايير الويب⁧!⁩ in Arabic.", PANGO2_DIRECTION_LTR, "\0\0\0\0\0\0\0\0\0\0\0\0\0\1\1\1\1\1\1\1\1\1\1\1\1\1\1\1\1\1\1\0\1\0\0\0\0\0\0\0\0\0\0\0", PANGO2_DIRECTION_LTR }, // FIXME 
    { "one two ثلاثة 1234 خمسة", PANGO2_DIRECTION_LTR, "\0\0\0\0\0\0\0\0\1\1\1\1\1\1\2\2\2\2\1\1\1\1\1", PANGO2_DIRECTION_LTR },
    { "one two ثلاثة ١٢٣٤ خمسة", PANGO2_DIRECTION_LTR, "\0\0\0\0\0\0\0\0\1\1\1\1\1\1\2\2\2\2\1\1\1\1\1", PANGO2_DIRECTION_LTR },
    { "abאב12cd", PANGO2_DIRECTION_LTR, "\0\0\1\1\2\2\0\0" },
    { "abאב‪xy‬cd", PANGO2_DIRECTION_LTR, "\0\0\1\1\1\2\2\2\0\0" },

  };

  for (int i = 0; i < G_N_ELEMENTS (tests); i++)
    {
      const char *text = tests[i].text;
      Pango2Direction dir = tests[i].dir;
      guint8 *levels;
      gsize len;

      levels = pango2_log2vis_get_embedding_levels (text, -1, &dir);

      len = g_utf8_strlen (text, -1);

      if (memcmp (levels, tests[i].levels, sizeof (guint8) * len) != 0)
        {
        for (int j = 0; j < len; j++)
          g_print ("\\%d", levels[j]);
        g_print ("\n");
        }
      g_assert_true (memcmp (levels, tests[i].levels, sizeof (guint8) * len) == 0);
      g_assert_true (dir == tests[i].out_dir);

      g_free (levels);
    }
}
#endif

/* Some basic tests for pango2_layout_move_cursor inside
 * a single Pango2Line:
 * - check that we actually move the cursor in the right direction
 * - check that we get through the line with at most n steps
 * - check that we don't skip legitimate cursor positions
 */
static void
test_move_cursor_line (void)
{
  const char *tests[] = {
    "abc😂️def",
    "abcאבגdef",
    "אבabcב",
    "aאב12b",
    "pa­ra­graph", // soft hyphens
  };
  Pango2Layout *layout;
  gboolean fail = FALSE;
  Pango2FontMap *fontmap;
  Pango2Context *context;

  fontmap = pango2_font_map_new_default ();
  context = pango2_context_new_with_font_map (fontmap);
  layout = pango2_layout_new (context);

  for (int i = 0; i < G_N_ELEMENTS (tests); i++)
    {
      const char *text;
      gsize n_chars;
      int index;
      int start_index;
      gboolean trailing;
      Pango2Rectangle s_pos, old_s_pos;
      Pango2Rectangle w_pos, old_w_pos;
      Pango2Lines *lines;
      Pango2Line *line;
      Pango2Line *new_line;
      struct {
        int direction;
        gboolean strong;
      } params[] = {
        { 1, TRUE },
        { 1, FALSE },
        { -1, TRUE },
        { -1, FALSE },
      };
      int *strong_cursor;
      int *weak_cursor;
      gboolean *met_cursor;
      const Pango2LogAttr *attrs;
      int n_attrs;
      int j;
      const char *p;

      pango2_layout_set_text (layout, tests[i], -1);

      text = pango2_layout_get_text (layout);
      lines = pango2_layout_get_lines (layout);
      line = pango2_lines_get_lines (lines)[0];

      n_chars = g_utf8_strlen (text, -1);

      attrs = pango2_layout_get_log_attrs (layout, &n_attrs);
      strong_cursor = g_new (int, n_attrs);
      weak_cursor = g_new (int, n_attrs);
      met_cursor = g_new (gboolean, n_attrs);
      for (j = 0, p = text; j < n_attrs; j++, p = g_utf8_next_char (p))
        {
          if (attrs[j].is_cursor_position)
            {
              pango2_lines_get_cursor_pos (lines, NULL, p - text, &s_pos, &w_pos);
              strong_cursor[j] = s_pos.x;
              weak_cursor[j] = w_pos.x;
            }
          else
            strong_cursor[j] = weak_cursor[j] = -1;
        }

      for (j = 0; j < G_N_ELEMENTS (params); j++)
        {
          gboolean ok;

          if (g_test_verbose ())
            g_print ("'%s' %s %s :\t",
                     text,
                     params[j].direction > 0 ? "->" : "<-",
                     params[j].strong ? "strong" : "weak");

          if ((pango2_line_get_resolved_direction (line) == PANGO2_DIRECTION_LTR) == (params[j].direction > 0))
            start_index = 0;
          else
            start_index = strlen (text);

          index = start_index;

          memset (met_cursor, 0, sizeof (gboolean) * n_attrs);

          pango2_lines_get_cursor_pos (lines, NULL, index, &s_pos, &w_pos);
          for (int l = 0; l < n_attrs; l++)
            {
              if ((params[j].strong && strong_cursor[l] == s_pos.x) ||
                  (!params[j].strong && weak_cursor[l] == w_pos.x))
                met_cursor[l] = TRUE;
            }

          ok = TRUE;

          for (int k = 0; k <= n_chars; k++)
            {
              old_s_pos = s_pos;
              old_w_pos = w_pos;
              pango2_lines_move_cursor (lines, params[j].strong,
                                       NULL,
                                       index, 0,
                                       params[j].direction,
                                       &new_line,
                                       &index, &trailing);

              while (trailing--)
                index = g_utf8_next_char (text + index) - text;

              g_assert_true (index == -1 || index == G_MAXINT ||
                             (0 <= index && index <= strlen (tests[i])));

              if (index == -1 || index == G_MAXINT)
                break;

              pango2_lines_get_cursor_pos (lines, NULL, index, &s_pos, &w_pos);
              for (int l = 0; l < n_attrs; l++)
                {
                  if ((params[j].strong && strong_cursor[l] == s_pos.x) ||
                      (!params[j].strong && weak_cursor[l] == w_pos.x))
                    met_cursor[l] = TRUE;
                }

              if ((params[j].direction > 0 && params[j].strong && old_s_pos.x >= s_pos.x) ||
                  (params[j].direction < 0 && params[j].strong && old_s_pos.x <= s_pos.x) ||
                  (params[j].direction > 0 && !params[j].strong && old_w_pos.x >= w_pos.x) ||
                  (params[j].direction < 0 && !params[j].strong && old_w_pos.x <= w_pos.x))
                {
                  if (g_test_verbose ())
                    g_print ("(wrong move)\t");
                  ok = FALSE;
                  break;
                }
            }

          if (ok)
            for (int l = 0; l < n_attrs; l++)
              {
                if (!(met_cursor[l] ||
                      (params[j].strong && strong_cursor[l] == -1) ||
                      (!params[j].strong && weak_cursor[l] == -1)))
                  {
                    if (g_test_verbose ())
                      g_print ("(missed cursor)\t");
                    ok = FALSE;
                  }
              }

          if (ok)
            if (!(index >= strlen (text) || index <= 0))
              {
                if (g_test_verbose ())
                  g_print ("(got stuck)\t");
                ok = FALSE;
              }

          if (g_test_verbose ())
            g_print ("%s\n", ok ? "ok": "not ok");

          fail = fail || !ok;
        }

       g_free (strong_cursor);
       g_free (weak_cursor);
       g_free (met_cursor);
    }

  g_object_unref (layout);
  g_object_unref (context);
  g_object_unref (fontmap);

  if (fail)
    g_test_fail ();
}

static void
test_move_cursor_para (void)
{
  struct {
    const char *text;
    int width;
  } tests[] = {
    { "This paragraph should ac­tual­ly have multiple lines, unlike all the other wannabe äöü pa­ra­graph tests in this ugh test-case. Grow some lines!\n", 188 },
    { "你好 Hello שלום Γειά σας", 40 },
    { "你好 Hello שלום Γειά σας", 60 },
    { "你好 Hello שלום Γειά σας", 80 },
    { "line 1 line 2 line 3\nline 4\r\nline 5", -1 }, // various separators
    { "some text, some more text,\n\n even more text", 60 },
    { "long word", 40 },
    { "זוהי השורה הראשונה" "\n" "זוהי השורה השנייה" "\n" "זוהי השורה השלישית" , 200 },
  };
  Pango2Layout *layout;
  Pango2Rectangle pos, old_pos;
  int index;
  int trailing;
  const char *text;
  Pango2Line *line;
  Pango2Lines *lines;
  Pango2Line *new_line;
  Pango2FontMap *fontmap;
  Pango2Context *context;

  fontmap = pango2_font_map_new_default ();
  context = pango2_context_new_with_font_map (fontmap);
  layout = pango2_layout_new (context);

  for (int i = 0; i < G_N_ELEMENTS (tests); i++)
    {
      pango2_layout_set_text (layout, tests[i].text, -1);
      text = pango2_layout_get_text (layout);
      if (tests[i].width > 0)
        pango2_layout_set_width (layout, tests[i].width * PANGO2_SCALE);
      else
        pango2_layout_set_width (layout, -1);

      index = 0;
      lines = pango2_layout_get_lines (layout);
      pango2_lines_get_cursor_pos (lines, NULL, index, &pos, NULL);

      while (index < G_MAXINT)
        {
          old_pos = pos;

          pango2_lines_index_to_line (lines, index, &line, NULL, NULL, NULL);
          if (line == NULL)
            break;

          pango2_lines_move_cursor(lines, TRUE,
                                  NULL,
                                  index, 0,
                                  1,
                                  &new_line,
                                  &index, &trailing);
          while (trailing--)
            index = g_utf8_next_char (text + index) - text;

          g_assert_true (index == -1 || index == G_MAXINT ||
                         (0 <= index && index <= strlen (tests[i].text)));

          if (index == -1 || index == G_MAXINT)
            break;

          if (index >= strlen (tests[i].text) - 1)
            continue;

          pango2_lines_get_cursor_pos (lines, NULL, index, &pos, NULL);

          // assert that we are either moving to the right
          // or jumping to the next line
          g_assert_true (pos.y >= old_pos.y || pos.x > old_pos.x);
          // no invisible cursors, please
          g_assert_true (pos.height > 1024);
        }

      /* and now backwards */
      index = strlen (text);
      pango2_lines_get_cursor_pos (lines, NULL, index, &pos, NULL);

      while (index > -1)
        {
          old_pos = pos;

          line = NULL;
          pango2_lines_index_to_line (lines, index, &line, NULL, NULL, NULL);
          g_assert_nonnull (line);

          pango2_lines_move_cursor (lines, TRUE,
                                   NULL,
                                   index, 0,
                                   -1,
                                   &new_line,
                                   &index, &trailing);
          while (trailing--)
            index = g_utf8_next_char (text + index) - text;

          g_assert_true (index == -1 || index == G_MAXINT ||
                         (0 <= index && index <= strlen (tests[i].text)));

          if (index == -1 || index == G_MAXINT)
            break;

          pango2_lines_get_cursor_pos (lines, NULL, index, &pos, NULL);

          // assert that we are either moving to the left
          // or jumping to the previous line
          g_assert_true (pos.y < old_pos.y || pos.x < old_pos.x);
          // no invisible cursors, please
          g_assert_true (pos.height > 1024);
        }
    }

  g_object_unref (layout);
  g_object_unref (context);
  g_object_unref (fontmap);
}

static void
test_sinhala_cursor (void)
{
  const char *text = "ර් á ";
  Pango2Layout *layout;
  Pango2Lines *lines;
  Pango2FontMap *fontmap;
  Pango2Context *context;

  fontmap = pango2_font_map_new_default ();
  context = pango2_context_new_with_font_map (fontmap);
  layout = pango2_layout_new (context);

  pango2_layout_set_text (layout, text, -1);
  lines = pango2_layout_get_lines (layout);

  if (pango2_lines_get_unknown_glyphs_count (lines) > 0)
    {
      g_object_unref (layout);
      g_object_unref (context);
      g_object_unref (fontmap);
      g_test_skip ("missing Sinhala fonts");
      return;
    }

  for (int i = 0; i < pango2_lines_get_line_count (lines); i++)
    {
      Pango2Line *line;
      const Pango2LogAttr *attrs;
      Pango2Rectangle strong, weak;
      int start, n_attrs;
      const char *text, *text_start, *text_end, *p;
      int start_index, length, j;

      line = pango2_lines_get_lines (lines)[i];
      text = pango2_line_get_text (line, &start_index, &length);
      text_start = text + start_index;
      text_end = text_start + length;
      attrs = pango2_line_get_log_attrs (line, &start, &n_attrs);
      for (p = text_start, j = 0; p < text_end; p = g_utf8_next_char (p), j++)
        {
          if (!attrs[start + j].is_cursor_position)
            continue;

          //g_assert_true (pango2_layout_get_direction (layout, index) == PANGO_DIRECTION_LTR);

          pango2_line_get_cursor_pos (line, p - text, &strong, &weak);
          g_assert_true (strong.x == weak.x);
          g_assert_true (strong.width == weak.width);
        }
    }

  g_object_unref (layout);
  g_object_unref (context);
  g_object_unref (fontmap);
}

static struct
{
  const char *text;
  int chars[20];
  int n_chars;
  int n_runs;
  int offsets[20];
  int levels[20];
  GUnicodeScript scripts[20];
} nested_tests[] = {
  {
    /* The example from docs/pango_bidi.md */
    .text = "abאב12αβ",
    .chars = { 0, 1, 4, 5, 3, 2, 6, 7, -1, },
    .n_chars = 8,
    .n_runs = 5,
    .offsets = { 0, 4, 2, 6, -1, },
    .levels = { 0, 2, 1, 0, -1, },
    .scripts = {
      G_UNICODE_SCRIPT_LATIN,  /* ab */
      G_UNICODE_SCRIPT_HEBREW, /* 12 */
      G_UNICODE_SCRIPT_HEBREW, /* אב */
      G_UNICODE_SCRIPT_GREEK,  /* αβ */
      -1,                      /* NULL run */
    },
  },
  {
    /* https://gitlab.gnome.org/GNOME/pango/-/issues/794 */
    .text = "abאב12",
    .chars = { 0, 1, 4, 5, 3, 2, -1, },
    .n_chars = 6,
    .n_runs = 4,
    .offsets = { 0, 4, 2, -1, },
    .levels = { 0, 2, 1, -1, },
    .scripts = {
      G_UNICODE_SCRIPT_LATIN,  /* ab */
      G_UNICODE_SCRIPT_HEBREW, /* 12 */
      G_UNICODE_SCRIPT_HEBREW, /* אב */
      -1,                      /* NULL run */
    },
  },
};

static void
test_bidi_nested (void)
{
  for (int i = 0; i < G_N_ELEMENTS (nested_tests); i++)
    {
      const char *text = nested_tests[i].text;
      gunichar *codes;
      glong n_chars;
      Pango2Layout *layout;
      Pango2Lines *lines;
      Pango2LineIter *iter;
      Pango2FontMap *fontmap;
      Pango2Context *context;

      fontmap = pango2_font_map_new_default ();
      context = pango2_context_new_with_font_map (fontmap);

      codes = g_utf8_to_ucs4_fast (text, -1, &n_chars);
      g_assert_nonnull (codes);
      g_assert_true (n_chars == nested_tests[i].n_chars);

      layout = pango2_layout_new (context);
      pango2_layout_set_text (layout, text, -1);
      lines = pango2_layout_get_lines (layout);

      if (pango2_lines_get_unknown_glyphs_count (lines) > 0)
        {
          g_object_unref (layout);
          g_object_unref (context);
          g_object_unref (fontmap);
          g_test_skip ("missing fonts");
          return;
        }

      /* Iterate chars in visual order */
      iter = pango2_lines_get_iter (lines);
      for (int j = 0; j < nested_tests[i].n_chars; j++)
        {
          int index = pango2_line_iter_get_index (iter);
          int offset = g_utf8_pointer_to_offset (text, text + index);
          gboolean moved = pango2_line_iter_next_char (iter);

          g_assert_true (offset == nested_tests[i].chars[j]);
          g_assert_true (moved == (j + 1 < n_chars));
        }
      pango2_line_iter_free (iter);

      /* Iterate runs in visual order */
      iter = pango2_lines_get_iter (lines);
      for (int j = 0; j < nested_tests[i].n_runs; j++)
        {
          Pango2Run *run = pango2_line_iter_get_run (iter);
          gboolean moved = pango2_line_iter_next_run (iter);

          if (j + 1 < nested_tests[i].n_runs)
            {
              int offset;
              Pango2Item *item;
              const Pango2Analysis *analysis;

              g_assert_nonnull (run);

              item = pango2_run_get_item (run);
              analysis = pango2_item_get_analysis (item);

              offset = g_utf8_pointer_to_offset (text, text + pango2_item_get_byte_offset (item));
              g_assert_true (offset == nested_tests[i].offsets[j]);
              g_assert_true (pango2_analysis_get_bidi_level (analysis) == nested_tests[i].levels[j]);
              g_assert_true (pango2_analysis_get_script (analysis) == nested_tests[i].scripts[j]);
              g_assert_true (moved == (j + 1 < nested_tests[i].n_runs));

              g_assert_true (moved);
            }
          else
            {
              g_assert_null (run);

              g_assert_false (moved);
            }
        }

      pango2_line_iter_free (iter);

      /* Lines */
      iter = pango2_lines_get_iter (lines);
      g_assert_true (pango2_line_iter_at_last_line (iter));
      g_assert_false (pango2_line_iter_next_line (iter));
      pango2_line_iter_free (iter);

      g_object_unref (layout);
      g_object_unref (context);
      g_object_unref (fontmap);

      g_free (codes);
    }
}

int
main (int argc, char *argv[])
{
  setlocale (LC_ALL, "");

  g_test_init (&argc, &argv, NULL);
  g_test_set_nonfatal_assertions ();

#if 0
  g_test_add_func ("/bidi/embedding-levels", test_bidi_embedding_levels);
#endif
  g_test_add_func ("/bidi/move-cursor-line", test_move_cursor_line);
  g_test_add_func ("/bidi/move-cursor-para", test_move_cursor_para);
  g_test_add_func ("/bidi/sinhala-cursor", test_sinhala_cursor);
  g_test_add_func ("/bidi/nested", test_bidi_nested);

  return g_test_run ();
}
