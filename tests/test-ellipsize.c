/* Pango
 * test-ellipsize.c: Test Pango harfbuzz apis
 *
 * Copyright (C) 2019 Red Hat, Inc.
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

#include <pango/pango.h>
#include <pango/pangocairo.h>
#include "test-common.h"

/* Test that ellipsization does not change the height of a layout.
 * See https://gitlab.gnome.org/GNOME/pango/issues/397
 */
static void
test_ellipsize_height (void)
{
  PangoFontMap *fontmap;
  PangoContext *context;
  PangoLayout *layout;
  int height1, height2;
  PangoFontDescription *desc;

  fontmap = pango_cairo_font_map_new ();
  context = pango_font_map_create_context (fontmap);
  layout = pango_layout_new (context);

  desc = pango_font_description_from_string ("Fixed 7");
  //pango_layout_set_font_description (layout, desc);
  pango_font_description_free (desc);

  pango_layout_set_text (layout, "some text that should be ellipsized", -1);
  g_assert_cmpint (pango_layout_get_line_count (layout), ==, 1);
  pango_layout_get_size (layout, NULL, &height1);

  pango_layout_set_width (layout, 100 * PANGO_SCALE);
  pango_layout_set_ellipsize (layout, PANGO_ELLIPSIZE_END);

  g_assert_cmpint (pango_layout_get_line_count (layout), ==, 1);
  g_assert_cmpint (pango_layout_is_ellipsized (layout), ==, 1);
  pango_layout_get_size (layout, NULL, &height2);

  g_assert_cmpint (height1, ==, height2);

  g_object_unref (layout);
  g_object_unref (context);
  g_object_unref (fontmap);
}

/* Test that ellipsization without attributes does not crash
 */
static void
test_ellipsize_crash (void)
{
  PangoFontMap *fontmap;
  PangoContext *context;
  PangoLayout *layout;

  fontmap = pango_cairo_font_map_new ();
  context = pango_font_map_create_context (fontmap);
  layout = pango_layout_new (context);

  pango_layout_set_text (layout, "some text that should be ellipsized", -1);
  g_assert_cmpint (pango_layout_get_line_count (layout), ==, 1);

  pango_layout_set_width (layout, 100 * PANGO_SCALE);
  pango_layout_set_ellipsize (layout, PANGO_ELLIPSIZE_END);

  g_assert_cmpint (pango_layout_get_line_count (layout), ==, 1);
  g_assert_cmpint (pango_layout_is_ellipsized (layout), ==, 1);

  g_object_unref (layout);
  g_object_unref (context);
  g_object_unref (fontmap);
}

/* Check that the width of a fully ellipsized paragraph
 * is the same as that of an explicit ellipsis.
 */
static void
test_ellipsize_fully (void)
{
  PangoFontMap *fontmap;
  PangoContext *context;
  PangoLayout *layout;
  PangoRectangle ink, logical;
  PangoRectangle ink2, logical2;

  fontmap = pango_cairo_font_map_new ();
  context = pango_font_map_create_context (fontmap);
  layout = pango_layout_new (context);

  pango_layout_set_text (layout, "…", -1);
  pango_layout_get_extents (layout, &ink, &logical);

  pango_layout_set_text (layout, "ellipsized", -1);

  pango_layout_set_width (layout, 10 * PANGO_SCALE);
  pango_layout_set_ellipsize (layout, PANGO_ELLIPSIZE_END);

  pango_layout_get_extents (layout, &ink2, &logical2);

  g_assert_cmpint (ink.width, ==, ink2.width);
  g_assert_cmpint (logical.width, ==, logical2.width);

  g_object_unref (layout);
  g_object_unref (context);
  g_object_unref (fontmap);
}

int
main (int argc, char *argv[])
{
  g_test_init (&argc, &argv, NULL);

  g_test_add_func ("/layout/ellipsize/height", test_ellipsize_height);
  g_test_add_func ("/layout/ellipsize/crash", test_ellipsize_crash);
  g_test_add_func ("/layout/ellipsize/fully", test_ellipsize_fully);

  return g_test_run ();
}
