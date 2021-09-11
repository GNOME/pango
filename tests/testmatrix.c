/* Pango
 * testmatrix.c: Test program for PangoMatrix
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
#include <math.h>

#define matrix_equal(m1, m2) \
  (G_APPROX_VALUE ((m1)->xx, (m2)->xx, 0.0001) && \
   G_APPROX_VALUE ((m1)->xy, (m2)->xy, 0.0001) && \
   G_APPROX_VALUE ((m1)->yx, (m2)->yx, 0.0001) && \
   G_APPROX_VALUE ((m1)->yy, (m2)->yy, 0.0001) && \
   G_APPROX_VALUE ((m1)->x0, (m2)->x0, 0.0001) && \
   G_APPROX_VALUE ((m1)->y0, (m2)->y0, 0.0001))

static void
test_matrix_translate (void)
{
  PangoMatrix m = PANGO_MATRIX_INIT;
  PangoMatrix m2 = PANGO_MATRIX_INIT;

  pango_matrix_translate (&m, 10, 10);
  pango_matrix_translate (&m, -10, -10);

  g_assert_true (matrix_equal (&m, &m2));
}

static void
test_matrix_rotate (void)
{
  PangoMatrix m = PANGO_MATRIX_INIT;
  PangoMatrix m2 = PANGO_MATRIX_INIT;

  pango_matrix_rotate (&m, 90);
  pango_matrix_rotate (&m, 90);
  pango_matrix_rotate (&m, 90);
  pango_matrix_rotate (&m, 90);

  g_assert_true (matrix_equal (&m, &m2));
}

static void
test_matrix_scale(void)
{
  PangoMatrix m = PANGO_MATRIX_INIT;
  PangoMatrix m2 = PANGO_MATRIX_INIT;

  pango_matrix_scale (&m, 4, 5);
  m2.xx = 4;
  m2.yy = 5;

  g_assert_true (matrix_equal (&m, &m2));
}

static void
test_matrix_concat (void)
{
  PangoMatrix m = PANGO_MATRIX_INIT;
  PangoMatrix id = PANGO_MATRIX_INIT;
  PangoMatrix *m2;

  pango_matrix_rotate (&m, 120);
  m2 = pango_matrix_copy (&m);
  pango_matrix_concat (m2, &m);
  pango_matrix_concat (m2, &m);

  g_assert_true (matrix_equal (&id, m2));

  pango_matrix_free (m2);
}

static void
test_matrix_font_scale (void)
{
  PangoMatrix m = PANGO_MATRIX_INIT;
  double x, y;

  pango_matrix_scale (&m, 2, 3);

  pango_matrix_get_font_scale_factors (&m, &x, &y);
  g_assert_cmpfloat (x, ==, 2);
  g_assert_cmpfloat (y, ==, 3);
  g_assert_cmpfloat (pango_matrix_get_font_scale_factor (&m), ==, 3);

  pango_matrix_rotate (&m, 90);
  pango_matrix_get_font_scale_factors (&m, &x, &y);
  g_assert_cmpfloat (x, ==, 3);
  g_assert_cmpfloat (y, ==, 2);
}

static void
test_matrix_transform_point (void)
{
  PangoMatrix m = PANGO_MATRIX_INIT;
  double x, y;

  pango_matrix_translate (&m, 1, 1);
  pango_matrix_scale (&m, 2, 4);
  pango_matrix_rotate (&m, -90);

  x = 1;
  y = 0;

  pango_matrix_transform_point (&m, &x, &y);

  g_assert_cmpfloat_with_epsilon (x, 1, 0.001);
  g_assert_cmpfloat_with_epsilon (y, 5, 0.001);
}

static void
test_matrix_transform_distance (void)
{
  PangoMatrix m = PANGO_MATRIX_INIT;
  double x, y;

  pango_matrix_translate (&m, 1, 1);
  pango_matrix_scale (&m, 2, 4);
  pango_matrix_rotate (&m, -90);

  x = 1;
  y = 0;

  pango_matrix_transform_distance (&m, &x, &y);

  g_assert_cmpfloat_with_epsilon (x, 0, 0.001);
  g_assert_cmpfloat_with_epsilon (y, 4, 0.001);
}

static void
test_matrix_transform_rect (void)
{
  PangoMatrix m = PANGO_MATRIX_INIT;
  PangoRectangle rect;

  pango_matrix_rotate (&m, 45);

  rect.x = 0;
  rect.y = 0;
  rect.width = 1 * PANGO_SCALE;
  rect.height = 1 * PANGO_SCALE;

  pango_matrix_transform_rectangle (&m, &rect);

  g_assert_cmpfloat_with_epsilon (rect.x, 0, 0.5);
  g_assert_cmpfloat_with_epsilon (rect.y, - G_SQRT2  / 2 * PANGO_SCALE, 0.5);
  g_assert_cmpfloat_with_epsilon (rect.width, G_SQRT2 * PANGO_SCALE, 0.5);
  g_assert_cmpfloat_with_epsilon (rect.height, G_SQRT2 * PANGO_SCALE, 0.5);
}

static void
test_matrix_transform_pixel_rect (void)
{
  PangoMatrix m = PANGO_MATRIX_INIT;
  PangoRectangle rect;

  pango_matrix_rotate (&m, 45);

  rect.x = 0;
  rect.y = 0;
  rect.width = 1;
  rect.height = 1;

  pango_matrix_transform_pixel_rectangle (&m, &rect);

  g_assert_cmpfloat_with_epsilon (rect.x, 0, 0.1);
  g_assert_cmpfloat_with_epsilon (rect.y, -1, 0.1);
  g_assert_cmpfloat_with_epsilon (rect.width, 2, 0.1);
  g_assert_cmpfloat_with_epsilon (rect.height, 2, 0.1);
}

static void
test_matrix_slant_ratio (void)
{
  PangoMatrix m = (PangoMatrix) { 1, 0, 0.2, 1, 0, 0 };
  PangoMatrix m2 = (PangoMatrix) { 1, 0.4, 0, 1, 0, 0 };
  double r;

  r = pango_matrix_get_slant_ratio (&m);
  g_assert_cmphex (r, ==, 0.2);

  pango_matrix_rotate (&m, 45);

  r = pango_matrix_get_slant_ratio (&m);
  g_assert_cmphex (r, ==, 0.2);

  pango_matrix_scale (&m, 2, 3);

  r = pango_matrix_get_slant_ratio (&m);
  g_assert_cmphex (r, ==, 0.2);

  r = pango_matrix_get_slant_ratio (&m2);
  g_assert_cmphex (r, ==, 0.4);
}

int
main (int argc, char *argv[])
{
  g_test_init (&argc, &argv, NULL);

  g_test_add_func ("/matrix/translate", test_matrix_translate);
  g_test_add_func ("/matrix/rotate", test_matrix_rotate);
  g_test_add_func ("/matrix/scale", test_matrix_scale);
  g_test_add_func ("/matrix/concat", test_matrix_concat);
  g_test_add_func ("/matrix/font-scale", test_matrix_font_scale);
  g_test_add_func ("/matrix/transform-point", test_matrix_transform_point);
  g_test_add_func ("/matrix/transform-distance", test_matrix_transform_distance);
  g_test_add_func ("/matrix/transform-rect", test_matrix_transform_rect);
  g_test_add_func ("/matrix/transform-pixel-rect", test_matrix_transform_pixel_rect);
  g_test_add_func ("/matrix/slant-ratio", test_matrix_slant_ratio);

  return g_test_run ();
}
