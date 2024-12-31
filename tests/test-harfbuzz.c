/* Pango2
 * test-harfbuzz.c: Test Pango harfbuzz apis
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

#include <pango2/pango.h>
#include <gio/gio.h>
#include "test-common.h"

/* Some basic checks that the hb_font_t returned
 * by pango2_font_get_hb_font is functional
 */
static void
test_hb_font (void)
{
  Pango2FontMap *map = pango2_font_map_get_default ();
  Pango2FontFamily *family;
  Pango2FontFace *face;
  Pango2Font *font;
  hb_font_t *hb_font;
  hb_bool_t res;
  hb_codepoint_t glyph;

  g_assert_true (g_list_model_get_n_items (G_LIST_MODEL (map)) > 0);

  family = g_list_model_get_item (G_LIST_MODEL (map), 0);
  g_assert_true (g_list_model_get_n_items (G_LIST_MODEL (family)) > 0);
  face = g_list_model_get_item (G_LIST_MODEL (family), 0);

  font = PANGO2_FONT (pango2_hb_font_new (PANGO2_HB_FACE (face),
                                        12 * PANGO2_SCALE,
                                        NULL, 0,
                                        NULL, 0,
                                        PANGO2_GRAVITY_SOUTH,
                                        96., NULL));

  hb_font = pango2_font_get_hb_font (font);

  g_assert_nonnull (hb_font);

  res = hb_font_get_nominal_glyph (hb_font, 0x20, &glyph);

  g_assert_true (res);
  g_assert_true (glyph != 0);

  g_object_unref (font);
  g_object_unref (face);
  g_object_unref (family);
}

int
main (int argc, char *argv[])
{
  g_test_init (&argc, &argv, NULL);

  g_test_add_func ("/harfbuzz/font", test_hb_font);

  return g_test_run ();
}
