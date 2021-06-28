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

/* test that we don't crash in shape_tab when the layout
 * is such that we don't have effective attributes
 */
static void
test_shape_tab_crash (void)
{
  PangoContext *context;
  PangoLayout *layout;

  context = pango_font_map_create_context (pango_cairo_font_map_get_default ());
  layout = pango_layout_new (context);
  pango_layout_set_text (layout, "one\ttwo", -1);
  pango_layout_is_ellipsized (layout);

  g_object_unref (layout);
  g_object_unref (context);
}

/* Test that itemizing a string with 0 characters works
 */
static void
test_itemize_empty_crash (void)
{
  PangoContext *context;

  context = pango_font_map_create_context (pango_cairo_font_map_get_default ());
  pango_itemize_with_base_dir (context, PANGO_DIRECTION_LTR, "", 0, 1, NULL, NULL);

  g_object_unref (context);
}

static void
test_itemize_utf8 (void)
{
  PangoContext *context;
  GList *result = NULL;

  context = pango_font_map_create_context (pango_cairo_font_map_get_default ());
  result = pango_itemize_with_base_dir (context, PANGO_DIRECTION_LTR, "\xc3\xa1\na", 3, 1, NULL, NULL);
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
  int width, height;

  context = pango_font_map_create_context (pango_cairo_font_map_get_default ());
  layout = pango_layout_new (context);
  pango_layout_set_text (layout, "short text", 200);
  pango_layout_get_pixel_size (layout, &width, &height);

  g_object_unref (layout);
  g_object_unref (context);
}

static void
test_language_emoji_crash (void)
{
  PangoLanguage *lang;
  const PangoScript *scripts;
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
  PangoLayoutLine *line;
  int height = 0;

  context = pango_font_map_create_context (pango_cairo_font_map_get_default ());
  layout = pango_layout_new (context);
  pango_layout_set_text (layout, "one\ttwo", -1);
  line = pango_layout_get_line_readonly (layout, 0);
  pango_layout_line_get_height (line, &height);

  g_assert_cmpint (height, >, 0);

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

  fg_attr = pango_attr_foreground_new (0, 0, 65535);
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
    PangoScript script;
    PangoGravity gravity;
    PangoGravity gravity_natural;
    PangoGravity gravity_line;
  } tests[] = {
    { PANGO_SCRIPT_ARABIC, PANGO_GRAVITY_SOUTH, PANGO_GRAVITY_SOUTH, PANGO_GRAVITY_NORTH },
    { PANGO_SCRIPT_BOPOMOFO, PANGO_GRAVITY_EAST, PANGO_GRAVITY_SOUTH, PANGO_GRAVITY_SOUTH },
    { PANGO_SCRIPT_LATIN, PANGO_GRAVITY_SOUTH, PANGO_GRAVITY_SOUTH, PANGO_GRAVITY_SOUTH },
    { PANGO_SCRIPT_HANGUL, PANGO_GRAVITY_EAST, PANGO_GRAVITY_SOUTH, PANGO_GRAVITY_SOUTH },
    { PANGO_SCRIPT_MONGOLIAN, PANGO_GRAVITY_WEST, PANGO_GRAVITY_SOUTH },
    { PANGO_SCRIPT_OGHAM, PANGO_GRAVITY_WEST, PANGO_GRAVITY_NORTH, PANGO_GRAVITY_SOUTH },
    { PANGO_SCRIPT_TIBETAN, PANGO_GRAVITY_SOUTH, PANGO_GRAVITY_SOUTH, PANGO_GRAVITY_SOUTH },
  };

  for (int i = 0; i < G_N_ELEMENTS (tests); i++)
    {
      g_assert_true (pango_gravity_get_for_script (tests[i].script, PANGO_GRAVITY_AUTO, PANGO_GRAVITY_HINT_STRONG) == tests[i].gravity);

      g_assert_true (pango_gravity_get_for_script_and_width (tests[i].script, FALSE, PANGO_GRAVITY_EAST, PANGO_GRAVITY_HINT_NATURAL) == tests[i].gravity_natural);
      g_assert_true (pango_gravity_get_for_script_and_width (tests[i].script, FALSE, PANGO_GRAVITY_EAST, PANGO_GRAVITY_HINT_STRONG) == PANGO_GRAVITY_EAST);
      g_assert_true (pango_gravity_get_for_script_and_width (tests[i].script, FALSE, PANGO_GRAVITY_EAST, PANGO_GRAVITY_HINT_LINE) == tests[i].gravity_line);
    }
}

G_GNUC_BEGIN_IGNORE_DEPRECATIONS

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
test_bidi_mirror_char (void)
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

G_GNUC_END_IGNORE_DEPRECATIONS

int
main (int argc, char *argv[])
{
  g_test_init (&argc, &argv, NULL);

  g_test_add_func ("/layout/shape-tab-crash", test_shape_tab_crash);
  g_test_add_func ("/layout/itemize-empty-crash", test_itemize_empty_crash);
  g_test_add_func ("/layout/itemize-utf8", test_itemize_utf8);
  g_test_add_func ("/layout/short-string-crash", test_short_string_crash);
  g_test_add_func ("/language/emoji-crash", test_language_emoji_crash);
  g_test_add_func ("/layout/line-height", test_line_height);
  g_test_add_func ("/attr-list/update", test_attr_list_update);
  g_test_add_func ("/version-info", test_version_info);
  g_test_add_func ("/is-zerowidth", test_is_zero_width);
  g_test_add_func ("/gravity/to-rotation", test_gravity_to_rotation);
  g_test_add_func ("/gravity/from-matrix", test_gravity_from_matrix);
  g_test_add_func ("/gravity/for-script", test_gravity_for_script);
  g_test_add_func ("/bidi/type-for-unichar", test_bidi_type_for_unichar);
  g_test_add_func ("/bidi/mirror-char", test_bidi_mirror_char);

  return g_test_run ();
}
