/* Pango
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

#include <pango/pango.h>
#include <pango/pangocairo.h>
#include "test-common.h"

/* Some basic checks that the hb_font_t returned
 * by pango_font_get_hb_font is functional
 */
static void
test_hb_font (void)
{
  PangoFontMap *fontmap;
  PangoContext *context;
  PangoFontDescription *desc;
  PangoFont *font;
  hb_font_t *hb_font;
  hb_bool_t res;
  hb_codepoint_t glyph;

  fontmap = pango_cairo_font_map_new ();
  context = pango_font_map_create_context (fontmap);

  if (strcmp (G_OBJECT_TYPE_NAME (fontmap), "PangoCairoWin32FontMap") == 0)
    desc = pango_font_description_from_string ("Verdana 11");
  else
    desc = pango_font_description_from_string ("Cantarell 11");
  font = pango_context_load_font (context, desc);
  hb_font = pango_font_get_hb_font (font);

  g_assert_nonnull (hb_font);

  res = hb_font_get_nominal_glyph (hb_font, 0x20, &glyph);

  g_assert_true (res);
  g_assert_cmpint (glyph, !=, 0);

  g_object_unref (font);
  pango_font_description_free (desc);
  g_object_unref (context);
  g_object_unref (fontmap);
}

int
main (int argc, char *argv[])
{
  g_test_init (&argc, &argv, NULL);

  g_test_add_func ("/harfbuzz/font", test_hb_font);

  return g_test_run ();
}
