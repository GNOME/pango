/* Pango
 *
 * Copyright (C) 2021 Matthias Clasen
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

static void
test_roundtrip (void)
{
  PangoContext *context;
  PangoLayout *layout;
  PangoLayout *layout2;
  PangoFontDescription *desc;
  GBytes *bytes;

  context = pango_font_map_create_context (pango_cairo_font_map_get_default ());

  layout = pango_layout_new (context);
  pango_layout_set_text (layout, "Lets see if this goes through", -1);
  desc = pango_font_description_from_string ("Sans 23");
  pango_layout_set_font_description (layout, desc);
  pango_font_description_free (desc);
  pango_layout_set_alignment (layout, PANGO_ALIGN_RIGHT);
  pango_layout_set_wrap (layout, PANGO_WRAP_WORD_CHAR);
  pango_layout_set_ellipsize (layout, PANGO_ELLIPSIZE_END);
  pango_layout_set_justify (layout, TRUE);
  pango_layout_set_justify_last_line (layout, TRUE);
  pango_layout_set_single_paragraph_mode (layout, FALSE);
  pango_layout_set_auto_dir (layout, FALSE);
  pango_layout_set_width (layout, 98000);
  pango_layout_set_height (layout, 47000);
  pango_layout_set_indent (layout, 2000);
  pango_layout_set_spacing (layout, 10);

  bytes = pango_layout_serialize (layout);
  //g_print ("%s\n", (const char *)g_bytes_get_data (bytes, NULL));
  layout2 = pango_layout_deserialize (context, bytes);

  g_assert_true (PANGO_IS_LAYOUT (layout2));
  g_assert_cmpstr (pango_layout_get_text (layout), ==, pango_layout_get_text (layout2));
  g_assert_true (pango_font_description_equal (pango_layout_get_font_description (layout),
                                               pango_layout_get_font_description (layout2)));

  g_assert_cmpint (pango_layout_get_alignment (layout), ==, pango_layout_get_alignment (layout2));
  g_assert_cmpint (pango_layout_get_wrap (layout), ==, pango_layout_get_wrap (layout2));
  g_assert_cmpint (pango_layout_get_ellipsize (layout), ==, pango_layout_get_ellipsize (layout2));
  g_assert_cmpint (pango_layout_get_justify (layout), ==, pango_layout_get_justify (layout2));
  g_assert_cmpint (pango_layout_get_justify_last_line (layout), ==, pango_layout_get_justify_last_line (layout2));
  g_assert_cmpint (pango_layout_get_single_paragraph_mode (layout), ==, pango_layout_get_single_paragraph_mode (layout2));
  g_assert_cmpint (pango_layout_get_auto_dir (layout), ==, pango_layout_get_auto_dir (layout2));
  g_assert_cmpint (pango_layout_get_width (layout), ==, pango_layout_get_width (layout2));
  g_assert_cmpint (pango_layout_get_height (layout), ==, pango_layout_get_height (layout2));
  g_assert_cmpint (pango_layout_get_indent (layout), ==, pango_layout_get_indent (layout2));
  g_assert_cmpint (pango_layout_get_spacing (layout), ==, pango_layout_get_spacing (layout2));

  g_object_unref (layout);
  g_object_unref (layout2);
  g_object_unref (context);
}

int
main (int argc, char *argv[])
{
  g_test_init (&argc, &argv, NULL);

  g_test_add_func ("/layout/roundtrip", test_roundtrip);

  return g_test_run ();
}
