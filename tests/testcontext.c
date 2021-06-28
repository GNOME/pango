/* Pango
 * testcontext.c: Test program for PangoContext
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

#include <glib.h>
#include <pango/pango.h>
#include <pango/pangocairo.h>

static void
test_list_families (void)
{
  PangoContext *context;
  PangoFontFamily **families = NULL;
  int n_families = 0;

  context = pango_context_new ();

  pango_context_list_families (context, &families, &n_families);
  g_assert_null (families);
  g_assert_cmpint (n_families, ==, 0);

  pango_context_set_font_map (context, pango_cairo_font_map_get_default ());

  pango_context_list_families (context, &families, &n_families);
  g_assert_nonnull (families);
  g_assert_cmpint (n_families, >, 0);

  g_free (families);

  g_object_unref (context);
}

static void
test_set_language (void)
{
  PangoContext *context;

  context = pango_context_new ();

  pango_context_set_language (context, pango_language_from_string ("de-de"));
  g_assert_true (pango_context_get_language (context) == pango_language_from_string ("de-de"));

  pango_context_set_language (context, NULL);
  g_assert_null (pango_context_get_language (context));

  g_object_unref (context);
}

static void
test_set_base_dir (void)
{
  PangoContext *context;

  context = pango_context_new ();

  pango_context_set_base_dir (context, PANGO_DIRECTION_RTL);
  g_assert_true (pango_context_get_base_dir (context) == PANGO_DIRECTION_RTL);

  pango_context_set_base_dir (context, PANGO_DIRECTION_WEAK_LTR);
  g_assert_true (pango_context_get_base_dir (context) == PANGO_DIRECTION_WEAK_LTR);

  g_object_unref (context);
}

static void
test_set_base_gravity (void)
{
  PangoContext *context;

  context = pango_context_new ();

  pango_context_set_base_gravity (context, PANGO_GRAVITY_SOUTH);
  g_assert_true (pango_context_get_base_gravity (context) == PANGO_GRAVITY_SOUTH);
  g_assert_true (pango_context_get_gravity (context) == PANGO_GRAVITY_SOUTH);

  pango_context_set_base_gravity (context, PANGO_GRAVITY_AUTO);
  g_assert_true (pango_context_get_base_gravity (context) == PANGO_GRAVITY_AUTO);
  g_assert_true (pango_context_get_gravity (context) == PANGO_GRAVITY_SOUTH);

  g_object_unref (context);
}

static void
test_set_gravity_hint (void)
{
  PangoContext *context;

  context = pango_context_new ();

  pango_context_set_gravity_hint (context, PANGO_GRAVITY_HINT_NATURAL);
  g_assert_true (pango_context_get_gravity_hint (context) == PANGO_GRAVITY_HINT_NATURAL);

  pango_context_set_gravity_hint (context, PANGO_GRAVITY_HINT_STRONG);
  g_assert_true (pango_context_get_gravity_hint (context) == PANGO_GRAVITY_HINT_STRONG);

  g_object_unref (context);
}

static void
test_set_round_glyph_positions (void)
{
  PangoContext *context;

  context = pango_context_new ();

  pango_context_set_round_glyph_positions (context, TRUE);
  g_assert_true (pango_context_get_round_glyph_positions (context));

  pango_context_set_round_glyph_positions (context, FALSE);
  g_assert_false (pango_context_get_round_glyph_positions (context));

  g_object_unref (context);
}

int
main (int argc, char *argv[])
{
  g_test_init (&argc, &argv, NULL);

  g_test_add_func ("/context/list-families", test_list_families);
  g_test_add_func ("/context/set-language", test_set_language);
  g_test_add_func ("/context/set-base-dir", test_set_base_dir);
  g_test_add_func ("/context/set-base-gravity", test_set_base_gravity);
  g_test_add_func ("/context/set-gravity-hint", test_set_gravity_hint);
  g_test_add_func ("/context/set-round-glyph-positions", test_set_round_glyph_positions);

  return g_test_run ();
}
