/* Pango2
 * testcontext.c: Test program for Pango2Context
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
#include <pango2/pango.h>

static void
test_set_language (void)
{
  Pango2Context *context;

  context = pango2_context_new ();

  pango2_context_set_language (context, pango2_language_from_string ("de-de"));
  g_assert_true (pango2_context_get_language (context) == pango2_language_from_string ("de-de"));

  pango2_context_set_language (context, NULL);
  g_assert_null (pango2_context_get_language (context));

  g_object_unref (context);
}

static void
test_set_base_dir (void)
{
  Pango2Context *context;

  context = pango2_context_new ();

  pango2_context_set_base_dir (context, PANGO2_DIRECTION_RTL);
  g_assert_true (pango2_context_get_base_dir (context) == PANGO2_DIRECTION_RTL);

  pango2_context_set_base_dir (context, PANGO2_DIRECTION_WEAK_LTR);
  g_assert_true (pango2_context_get_base_dir (context) == PANGO2_DIRECTION_WEAK_LTR);

  g_object_unref (context);
}

static void
test_set_base_gravity (void)
{
  Pango2Context *context;

  context = pango2_context_new ();

  pango2_context_set_base_gravity (context, PANGO2_GRAVITY_SOUTH);
  g_assert_true (pango2_context_get_base_gravity (context) == PANGO2_GRAVITY_SOUTH);
  g_assert_true (pango2_context_get_gravity (context) == PANGO2_GRAVITY_SOUTH);

  pango2_context_set_base_gravity (context, PANGO2_GRAVITY_AUTO);
  g_assert_true (pango2_context_get_base_gravity (context) == PANGO2_GRAVITY_AUTO);
  g_assert_true (pango2_context_get_gravity (context) == PANGO2_GRAVITY_SOUTH);

  g_object_unref (context);
}

static void
test_set_gravity_hint (void)
{
  Pango2Context *context;

  context = pango2_context_new ();

  pango2_context_set_gravity_hint (context, PANGO2_GRAVITY_HINT_NATURAL);
  g_assert_true (pango2_context_get_gravity_hint (context) == PANGO2_GRAVITY_HINT_NATURAL);

  pango2_context_set_gravity_hint (context, PANGO2_GRAVITY_HINT_STRONG);
  g_assert_true (pango2_context_get_gravity_hint (context) == PANGO2_GRAVITY_HINT_STRONG);

  g_object_unref (context);
}

static void
test_set_round_glyph_positions (void)
{
  Pango2Context *context;

  context = pango2_context_new ();

  pango2_context_set_round_glyph_positions (context, TRUE);
  g_assert_true (pango2_context_get_round_glyph_positions (context));

  pango2_context_set_round_glyph_positions (context, FALSE);
  g_assert_false (pango2_context_get_round_glyph_positions (context));

  g_object_unref (context);
}

int
main (int argc, char *argv[])
{
  g_test_init (&argc, &argv, NULL);

  g_test_add_func ("/context/set-language", test_set_language);
  g_test_add_func ("/context/set-base-dir", test_set_base_dir);
  g_test_add_func ("/context/set-base-gravity", test_set_base_gravity);
  g_test_add_func ("/context/set-gravity-hint", test_set_gravity_hint);
  g_test_add_func ("/context/set-round-glyph-positions", test_set_round_glyph_positions);

  return g_test_run ();
}
