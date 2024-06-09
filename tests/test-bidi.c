/* Pango
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
#include <pango/pango.h>
#include <pango/pangocairo.h>

G_GNUC_BEGIN_IGNORE_DEPRECATIONS

static void
test_mirror_char (void)
{
  /* just some samples */
  struct {
    gunichar a;
    gunichar b;
  } tests[] = {
    { '(', ')' },
    { '<', '>' },
    { '[', ']' },
    { '{', '}' },
    { 0x00ab, 0x00bb },
    { 0x2045, 0x2046 },
    { 0x226e, 0x226f },
  };

  for (int i = 0; i < G_N_ELEMENTS (tests); i++)
    {
      gboolean ret;
      gunichar ch;

      ret = pango_get_mirror_char (tests[i].a, &ch);
      g_assert_true (ret);
      g_assert_true (ch == tests[i].b);
      ret = pango_get_mirror_char (tests[i].b, &ch);
      g_assert_true (ret);
      g_assert_true (ch == tests[i].a);
    }
}

static void
test_bidi_type_for_unichar (void)
{
  /* one representative from each class we support */
  g_assert_true (pango_bidi_type_for_unichar ('a') == PANGO_BIDI_TYPE_L);
  g_assert_true (pango_bidi_type_for_unichar (0x202a) == PANGO_BIDI_TYPE_LRE);
  g_assert_true (pango_bidi_type_for_unichar (0x202d) == PANGO_BIDI_TYPE_LRO);
  g_assert_true (pango_bidi_type_for_unichar (0x05d0) == PANGO_BIDI_TYPE_R);
  g_assert_true (pango_bidi_type_for_unichar (0x0627) == PANGO_BIDI_TYPE_AL);
  g_assert_true (pango_bidi_type_for_unichar (0x202b) == PANGO_BIDI_TYPE_RLE);
  g_assert_true (pango_bidi_type_for_unichar (0x202e) == PANGO_BIDI_TYPE_RLO);
  g_assert_true (pango_bidi_type_for_unichar (0x202c) == PANGO_BIDI_TYPE_PDF);
  g_assert_true (pango_bidi_type_for_unichar ('0') == PANGO_BIDI_TYPE_EN);
  g_assert_true (pango_bidi_type_for_unichar ('+') == PANGO_BIDI_TYPE_ES);
  g_assert_true (pango_bidi_type_for_unichar ('#') == PANGO_BIDI_TYPE_ET);
  g_assert_true (pango_bidi_type_for_unichar (0x601) == PANGO_BIDI_TYPE_AN);
  g_assert_true (pango_bidi_type_for_unichar (',') == PANGO_BIDI_TYPE_CS);
  g_assert_true (pango_bidi_type_for_unichar (0x0301) == PANGO_BIDI_TYPE_NSM);
  g_assert_true (pango_bidi_type_for_unichar (0x200d) == PANGO_BIDI_TYPE_BN);
  g_assert_true (pango_bidi_type_for_unichar (0x2029) == PANGO_BIDI_TYPE_B);
  g_assert_true (pango_bidi_type_for_unichar (0x000b) == PANGO_BIDI_TYPE_S);
  g_assert_true (pango_bidi_type_for_unichar (' ') == PANGO_BIDI_TYPE_WS);
  g_assert_true (pango_bidi_type_for_unichar ('!') == PANGO_BIDI_TYPE_ON);
  /* these are new */
  g_assert_true (pango_bidi_type_for_unichar (0x2066) == PANGO_BIDI_TYPE_LRI);
  g_assert_true (pango_bidi_type_for_unichar (0x2067) == PANGO_BIDI_TYPE_RLI);
  g_assert_true (pango_bidi_type_for_unichar (0x2068) == PANGO_BIDI_TYPE_FSI);
  g_assert_true (pango_bidi_type_for_unichar (0x2069) == PANGO_BIDI_TYPE_PDI);
}

static void
test_unichar_direction (void)
{
  struct {
    gunichar ch;
    PangoDirection dir;
  } tests[] = {
    { 'a', PANGO_DIRECTION_LTR },
    { '0', PANGO_DIRECTION_NEUTRAL },
    { '.', PANGO_DIRECTION_NEUTRAL },
    { '(', PANGO_DIRECTION_NEUTRAL },
    { 0x05d0, PANGO_DIRECTION_RTL },
  };

  for (int i = 0; i < G_N_ELEMENTS (tests); i++)
    {
      g_assert_true (pango_unichar_direction (tests[i].ch) == tests[i].dir);
    }
}

G_GNUC_END_IGNORE_DEPRECATIONS

static void
test_bidi_embedding_levels (void)
{
  /* Examples taken from https://www.w3.org/International/articles/inline-bidi-markup/uba-basics */
  struct {
    const char *text;
    PangoDirection dir;
    const char *levels;
    PangoDirection out_dir;
  } tests[] = {
    { "bahrain مصر kuwait", PANGO_DIRECTION_LTR, "\0\0\0\0\0\0\0\0\1\1\1\0\0\0\0\0\0\0", PANGO_DIRECTION_LTR },
    { "bahrain مصر kuwait", PANGO_DIRECTION_WEAK_LTR, "\0\0\0\0\0\0\0\0\1\1\1\0\0\0\0\0\0\0", PANGO_DIRECTION_LTR },
    { "bahrain مصر kuwait", PANGO_DIRECTION_RTL, "\2\2\2\2\2\2\2\1\1\1\1\1\2\2\2\2\2\2", PANGO_DIRECTION_RTL },
    { "bahrain مصر kuwait", PANGO_DIRECTION_WEAK_RTL, "\0\0\0\0\0\0\0\0\1\1\1\0\0\0\0\0\0\0", PANGO_DIRECTION_LTR },
    { "The title is مفتاح معايير الويب in Arabic.", PANGO_DIRECTION_LTR, "\0\0\0\0\0\0\0\0\0\0\0\0\0\1\1\1\1\1\1\1\1\1\1\1\1\1\1\1\1\1\1\0\0\0\0\0\0\0\0\0\0\0", PANGO_DIRECTION_LTR },
    { "The title is مفتاح معايير الويب, in Arabic.", PANGO_DIRECTION_LTR, "\0\0\0\0\0\0\0\0\0\0\0\0\0\1\1\1\1\1\1\1\1\1\1\1\1\1\1\1\1\1\1\0\0\0\0\0\0\0\0\0\0\0\0", PANGO_DIRECTION_LTR },
    { "The title is مفتاح معايير الويب⁧!⁩ in Arabic.", PANGO_DIRECTION_LTR, "\0\0\0\0\0\0\0\0\0\0\0\0\0\1\1\1\1\1\1\1\1\1\1\1\1\1\1\1\1\1\1\0\1\0\0\0\0\0\0\0\0\0\0\0", PANGO_DIRECTION_LTR }, // FIXME 
    { "one two ثلاثة 1234 خمسة", PANGO_DIRECTION_LTR, "\0\0\0\0\0\0\0\0\1\1\1\1\1\1\2\2\2\2\1\1\1\1\1", PANGO_DIRECTION_LTR },
    { "one two ثلاثة ١٢٣٤ خمسة", PANGO_DIRECTION_LTR, "\0\0\0\0\0\0\0\0\1\1\1\1\1\1\2\2\2\2\1\1\1\1\1", PANGO_DIRECTION_LTR },
    { "abאב12cd", PANGO_DIRECTION_LTR, "\0\0\1\1\2\2\0\0" },
    { "abאב‪xy‬cd", PANGO_DIRECTION_LTR, "\0\0\1\1\1\2\2\2\0\0" },

  };

  for (int i = 0; i < G_N_ELEMENTS (tests); i++)
    {
      const char *text = tests[i].text;
      PangoDirection dir = tests[i].dir;
      guint8 *levels;
      gsize len;

      levels = pango_log2vis_get_embedding_levels (text, -1, &dir);

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

/* Some basic tests for pango_layout_move_cursor_visually inside
 * a single PangoLayoutLine:
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
  PangoFontMap *fontmap;
  PangoContext *context;
  PangoLayout *layout;
  gboolean fail = FALSE;

  fontmap = pango_cairo_font_map_new ();
  context = pango_font_map_create_context (fontmap);
  layout = pango_layout_new (context);

  for (int i = 0; i < G_N_ELEMENTS (tests); i++)
    {
      const char *text;
      gsize n_chars;
      int index;
      int start_index;
      gboolean trailing;
      PangoRectangle s_pos, old_s_pos;
      PangoRectangle w_pos, old_w_pos;
      PangoLayoutLine *line;
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
      const PangoLogAttr *attrs;
      int n_attrs;
      int j;
      const char *p;

      pango_layout_set_text (layout, tests[i], -1);

      text = pango_layout_get_text (layout);
      line = pango_layout_get_line_readonly (layout, 0);

      n_chars = g_utf8_strlen (text, -1);

      attrs = pango_layout_get_log_attrs_readonly (layout, &n_attrs);
      strong_cursor = g_new (int, n_attrs);
      weak_cursor = g_new (int, n_attrs);
      met_cursor = g_new (gboolean, n_attrs);
      for (j = 0, p = text; j < n_attrs; j++, p = g_utf8_next_char (p))
        {
          if (attrs[j].is_cursor_position)
            {
              pango_layout_get_cursor_pos (layout, p - text, &s_pos, &w_pos);
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

          if ((pango_layout_line_get_resolved_direction (line) == PANGO_DIRECTION_LTR) == (params[j].direction > 0))
            start_index = 0;
          else
            start_index = strlen (text);

          index = start_index;

          memset (met_cursor, 0, sizeof (gboolean) * n_attrs);

          pango_layout_get_cursor_pos (layout, index, &s_pos, &w_pos);
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
              pango_layout_move_cursor_visually (layout, params[j].strong,
                                                 index, 0,
                                                 params[j].direction,
                                                 &index, &trailing);

              while (trailing--)
                index = g_utf8_next_char (text + index) - text;

              g_assert_true (index == -1 || index == G_MAXINT ||
                             (0 <= index && index <= strlen (tests[i])));

              if (index == -1 || index == G_MAXINT)
                break;

              pango_layout_get_cursor_pos (layout, index, &s_pos, &w_pos);
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
  PangoFontMap *fontmap;
  PangoContext *context;
  PangoLayout *layout;
  PangoRectangle pos, old_pos;
  int index;
  int trailing;
  const char *text;
  int line_no;
  PangoLayoutLine *line;
  PangoRectangle ext;
  PangoLayoutIter *iter;

  fontmap = pango_cairo_font_map_new ();
  context = pango_font_map_create_context (fontmap);
  layout = pango_layout_new (context);

  for (int i = 0; i < G_N_ELEMENTS (tests); i++)
    {
      pango_layout_set_text (layout, tests[i].text, -1);
      text = pango_layout_get_text (layout);
      if (tests[i].width > 0)
        pango_layout_set_width (layout, tests[i].width * PANGO_SCALE);
      else
        pango_layout_set_width (layout, -1);

      index = 0;
      pango_layout_get_cursor_pos (layout, index, &pos, NULL);

      while (index < G_MAXINT)
        {
          old_pos = pos;

          pango_layout_index_to_line_x (layout, index, FALSE, &line_no, NULL);
          line = pango_layout_get_line (layout, line_no);
          iter = pango_layout_get_iter (layout);
          while (pango_layout_iter_get_line (iter) != line)
            pango_layout_iter_next_line (iter);
          pango_layout_iter_get_line_extents (iter, NULL, &ext);
          pango_layout_iter_free (iter);

          pango_layout_move_cursor_visually (layout, TRUE,
                                             index, 0,
                                             1,
                                             &index, &trailing);
          while (trailing--)
            index = g_utf8_next_char (text + index) - text;

          g_assert_true (index == -1 || index == G_MAXINT ||
                         (0 <= index && index <= strlen (tests[i].text)));

          if (index == -1 || index == G_MAXINT)
            break;

          if (index >= strlen (tests[i].text) - 1)
            continue;

          pango_layout_get_cursor_pos (layout, index, &pos, NULL);

          // assert that we are either moving to the right
          // or jumping to the next line
          g_assert_true (pos.y >= ext.y + ext.height || pos.x > old_pos.x);
          // no invisible cursors, please
          g_assert_true (pos.height > 1024);
        }

      /* and now backwards */
      index = strlen (text);
      pango_layout_get_cursor_pos (layout, index, &pos, NULL);

      while (index > -1)
        {
          old_pos = pos;

          pango_layout_index_to_line_x (layout, index, FALSE, &line_no, NULL);
          line = pango_layout_get_line (layout, line_no);
          iter = pango_layout_get_iter (layout);
          while (pango_layout_iter_get_line (iter) != line)
            pango_layout_iter_next_line (iter);
          pango_layout_iter_get_line_extents (iter, NULL, &ext);
          pango_layout_iter_free (iter);

          pango_layout_move_cursor_visually (layout, TRUE,
                                             index, 0,
                                             -1,
                                             &index, &trailing);
          while (trailing--)
            index = g_utf8_next_char (text + index) - text;

          g_assert_true (index == -1 || index == G_MAXINT ||
                         (0 <= index && index <= strlen (tests[i].text)));

          if (index == -1 || index == G_MAXINT)
            break;

          pango_layout_get_cursor_pos (layout, index, &pos, NULL);

          // assert that we are either moving to the left
          // or jumping to the previous line
          g_assert_true (pos.y < ext.y || pos.x < old_pos.x);
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
  PangoFontMap *fontmap;
  PangoContext *context;
  PangoLayout *layout;
  const char *p;
  const PangoLogAttr *attrs;
  int n, i;

  fontmap = pango_cairo_font_map_new ();
  context = pango_font_map_create_context (fontmap);
  layout = pango_layout_new (context);

  pango_layout_set_text (layout, text, -1);

  if (pango_layout_get_unknown_glyphs_count (layout) > 0)
    {
      g_object_unref (layout);
      g_object_unref (context);
      g_object_unref (fontmap);
      g_test_skip ("missing Sinhala fonts");
      return;
    }

  attrs = pango_layout_get_log_attrs_readonly (layout, &n);

  for (i = 0, p = text; *p; i++, p = g_utf8_next_char (p))
    {
      int index = p - text;
      PangoRectangle strong, weak;

      if (!attrs[i].is_cursor_position)
        continue;

      g_assert_true (pango_layout_get_direction (layout, index) == PANGO_DIRECTION_LTR);

      pango_layout_get_cursor_pos (layout, index, &strong, &weak);
      g_assert_true (strong.x == weak.x);
      g_assert_true (strong.width == weak.width);
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
      PangoFontMap *fontmap;
      PangoContext *context;
      PangoLayout *layout;
      PangoLayoutIter *iter;

      codes = g_utf8_to_ucs4_fast (text, -1, &n_chars);
      g_assert_nonnull (codes);
      g_assert_true (n_chars == nested_tests[i].n_chars);

      fontmap = pango_cairo_font_map_new ();
      context = pango_font_map_create_context (fontmap);
      layout = pango_layout_new (context);
      pango_layout_set_text (layout, text, -1);

      if (pango_layout_get_unknown_glyphs_count (layout) > 0)
        {
          g_object_unref (layout);
          g_object_unref (context);
          g_object_unref (fontmap);
          g_test_skip ("missing fonts");
          return;
        }

      /* Iterate chars in visual order */
      iter = pango_layout_get_iter (layout);
      for (int j = 0; j < nested_tests[i].n_chars; j++)
        {
          int index = pango_layout_iter_get_index (iter);
          int offset = g_utf8_pointer_to_offset (text, text + index);
          gboolean moved = pango_layout_iter_next_char (iter);

          g_assert_true (offset == nested_tests[i].chars[j]);
          g_assert_true (moved == (j + 1 < n_chars));
        }
      pango_layout_iter_free (iter);

      /* Iterate runs in visual order */
      iter = pango_layout_get_iter (layout);
      for (int j = 0; j < nested_tests[i].n_runs; j++)
        {
          PangoLayoutRun *run = pango_layout_iter_get_run_readonly (iter);
          gboolean moved = pango_layout_iter_next_run (iter);

          if (j + 1 < nested_tests[i].n_runs)
            {
              int offset;

              g_assert_nonnull (run);

              offset = g_utf8_pointer_to_offset (text, text + run->item->offset);
              g_assert_true (offset == nested_tests[i].offsets[j]);
              g_assert_true (offset == pango_item_get_char_offset (run->item));
              g_assert_true (run->item->analysis.level == nested_tests[i].levels[j]);
              g_assert_true (run->item->analysis.script == nested_tests[i].scripts[j]);
              g_assert_true (moved == (j + 1 < nested_tests[i].n_runs));

              g_assert_true (moved);
            }
          else
            {
              g_assert_null (run);

              g_assert_false (moved);
            }
        }
      pango_layout_iter_free (iter);

      /* Lines */
      iter = pango_layout_get_iter (layout);
      g_assert_true (pango_layout_iter_at_last_line (iter));
      g_assert_false (pango_layout_iter_next_line (iter));
      pango_layout_iter_free (iter);

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

  g_test_add_func ("/bidi/mirror-char", test_mirror_char);
  g_test_add_func ("/bidi/type-for-unichar", test_bidi_type_for_unichar);
  g_test_add_func ("/bidi/unichar-direction", test_unichar_direction);
  g_test_add_func ("/bidi/embedding-levels", test_bidi_embedding_levels);
  g_test_add_func ("/bidi/move-cursor-line", test_move_cursor_line);
  g_test_add_func ("/bidi/move-cursor-para", test_move_cursor_para);
  g_test_add_func ("/bidi/sinhala-cursor", test_sinhala_cursor);
  g_test_add_func ("/bidi/nested", test_bidi_nested);

  return g_test_run ();
}
