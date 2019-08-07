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

static PangoContext *context;

/* Test that ellipsization does not change the
 * height of a layout.
 */
static void
test_ellipsize (void)
{
  PangoLayout *layout;
  int height1, height2;
  PangoFontDescription *desc;

  layout = pango_layout_new (context);

  desc = pango_font_description_from_string ("Fixed 7");
  pango_layout_set_font_description (layout, desc);
  pango_font_description_free (desc);

  pango_layout_set_text (layout, "some text that should be ellipsized", -1);
  g_assert_cmpint (pango_layout_get_line_count (layout), ==, 1);
  pango_layout_get_pixel_size (layout, NULL, &height1);

  pango_layout_set_width (layout, 100 * PANGO_SCALE);
  pango_layout_set_ellipsize (layout, PANGO_ELLIPSIZE_END);

  g_assert_cmpint (pango_layout_get_line_count (layout), ==, 1);
  g_assert_cmpint (pango_layout_is_ellipsized (layout), ==, 1);
  pango_layout_get_pixel_size (layout, NULL, &height2);

  g_assert_cmpint (height1, ==, height2);

  g_object_unref (layout);
}

int
main (int argc, char *argv[])
{
  PangoFontMap *fontmap;

  fontmap = pango_cairo_font_map_get_default ();
  context = pango_font_map_create_context (fontmap);

  g_test_init (&argc, &argv, NULL);

  g_test_add_func ("/layout/ellipsize", test_ellipsize);

  return g_test_run ();
}
