/* Pango2
 * testmatrix.c: Test program for Pango2Matrix
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
  Pango2Matrix m = PANGO2_MATRIX_INIT;
  Pango2Matrix m2 = PANGO2_MATRIX_INIT;

  pango2_matrix_translate (&m, 10, 10);
  pango2_matrix_translate (&m, -10, -10);

  g_assert_true (matrix_equal (&m, &m2));
}

static void
test_matrix_rotate (void)
{
  Pango2Matrix m = PANGO2_MATRIX_INIT;
  Pango2Matrix m2 = PANGO2_MATRIX_INIT;

  pango2_matrix_rotate (&m, 90);
  pango2_matrix_rotate (&m, 90);
  pango2_matrix_rotate (&m, 90);
  pango2_matrix_rotate (&m, 90);

  g_assert_true (matrix_equal (&m, &m2));
}

static void
test_matrix_scale(void)
{
  Pango2Matrix m = PANGO2_MATRIX_INIT;
  Pango2Matrix m2 = PANGO2_MATRIX_INIT;

  pango2_matrix_scale (&m, 4, 5);
  m2.xx = 4;
  m2.yy = 5;

  g_assert_true (matrix_equal (&m, &m2));
}

static void
test_matrix_concat (void)
{
  Pango2Matrix m = PANGO2_MATRIX_INIT;
  Pango2Matrix id = PANGO2_MATRIX_INIT;
  Pango2Matrix *m2;

  pango2_matrix_rotate (&m, 120);
  m2 = pango2_matrix_copy (&m);
  pango2_matrix_concat (m2, &m);
  pango2_matrix_concat (m2, &m);

  g_assert_true (matrix_equal (&id, m2));

  pango2_matrix_free (m2);
}

static void
test_matrix_font_scale (void)
{
  Pango2Matrix m = PANGO2_MATRIX_INIT;
  double x, y;

  pango2_matrix_scale (&m, 2, 3);

  pango2_matrix_get_font_scale_factors (&m, &x, &y);
  g_assert_cmpfloat (x, ==, 2);
  g_assert_cmpfloat (y, ==, 3);
  g_assert_cmpfloat (pango2_matrix_get_font_scale_factor (&m), ==, 3);

  pango2_matrix_rotate (&m, 90);
  pango2_matrix_get_font_scale_factors (&m, &x, &y);
  g_assert_cmpfloat (x, ==, 3);
  g_assert_cmpfloat (y, ==, 2);
}

static void
test_matrix_transform_point (void)
{
  Pango2Matrix m = PANGO2_MATRIX_INIT;
  double x, y;

  pango2_matrix_translate (&m, 1, 1);
  pango2_matrix_scale (&m, 2, 4);
  pango2_matrix_rotate (&m, -90);

  x = 1;
  y = 0;

  pango2_matrix_transform_point (&m, &x, &y);

  g_assert_cmpfloat_with_epsilon (x, 1, 0.001);
  g_assert_cmpfloat_with_epsilon (y, 5, 0.001);
}

static void
test_matrix_transform_distance (void)
{
  Pango2Matrix m = PANGO2_MATRIX_INIT;
  double x, y;

  pango2_matrix_translate (&m, 1, 1);
  pango2_matrix_scale (&m, 2, 4);
  pango2_matrix_rotate (&m, -90);

  x = 1;
  y = 0;

  pango2_matrix_transform_distance (&m, &x, &y);

  g_assert_cmpfloat_with_epsilon (x, 0, 0.001);
  g_assert_cmpfloat_with_epsilon (y, 4, 0.001);
}

static void
test_matrix_transform_rect (void)
{
  Pango2Matrix m = PANGO2_MATRIX_INIT;
  Pango2Rectangle rect;

  pango2_matrix_rotate (&m, 45);

  rect.x = 0;
  rect.y = 0;
  rect.width = 1 * PANGO2_SCALE;
  rect.height = 1 * PANGO2_SCALE;

  pango2_matrix_transform_rectangle (&m, &rect);

  g_assert_cmpfloat_with_epsilon (rect.x, 0, 0.5);
  g_assert_cmpfloat_with_epsilon (rect.y, - G_SQRT2  / 2 * PANGO2_SCALE, 0.5);
  g_assert_cmpfloat_with_epsilon (rect.width, G_SQRT2 * PANGO2_SCALE, 0.5);
  g_assert_cmpfloat_with_epsilon (rect.height, G_SQRT2 * PANGO2_SCALE, 0.5);
}

static void
test_matrix_transform_pixel_rect (void)
{
  Pango2Matrix m = PANGO2_MATRIX_INIT;
  Pango2Rectangle rect;

  pango2_matrix_rotate (&m, 45);

  rect.x = 0;
  rect.y = 0;
  rect.width = 1;
  rect.height = 1;

  pango2_matrix_transform_pixel_rectangle (&m, &rect);

  g_assert_cmpfloat_with_epsilon (rect.x, 0, 0.1);
  g_assert_cmpfloat_with_epsilon (rect.y, -1, 0.1);
  g_assert_cmpfloat_with_epsilon (rect.width, 2, 0.1);
  g_assert_cmpfloat_with_epsilon (rect.height, 2, 0.1);
}

static void
pango2_matrix_postrotate (Pango2Matrix *m,
                         double       angle)
{
  Pango2Matrix rot = (Pango2Matrix) PANGO2_MATRIX_INIT;

  pango2_matrix_rotate (&rot, angle);
  pango2_matrix_concat (&rot, m);
  *m = rot;
}

static void
test_matrix_slant_ratio (void)
{
  Pango2Matrix m = (Pango2Matrix) { 1, 0.2, 0, 1, 0, 0 };
  Pango2Matrix m2 = (Pango2Matrix) { 1, 0.4, 0, 1, 0, 0 };
  Pango2Matrix m3 = (Pango2Matrix) { 1, 0.3, 0, 2, 0, 0 };
  double a;
  double sx, sy;
  double r;

  a = pango2_matrix_get_rotation (&m);
  g_assert_cmpfloat_with_epsilon (a, 0, 0.001);

  pango2_matrix_get_font_scale_factors (&m, &sx, &sy);
  g_assert_cmpfloat_with_epsilon (sx, 1, 0.001);
  g_assert_cmpfloat_with_epsilon (sy, 1, 0.001);

  r = pango2_matrix_get_slant_ratio (&m);
  g_assert_cmpfloat_with_epsilon (r, 0.2, 0.001);

  pango2_matrix_postrotate (&m, 45);

  a = pango2_matrix_get_rotation (&m);
  g_assert_cmpfloat_with_epsilon (a, 45, 0.001);

  pango2_matrix_postrotate (&m, -a);

  pango2_matrix_get_font_scale_factors (&m, &sx, &sy);
  g_assert_cmpfloat_with_epsilon (sx, 1, 0.001);
  g_assert_cmpfloat_with_epsilon (sy, 1, 0.001);

  r = pango2_matrix_get_slant_ratio (&m);
  g_assert_cmpfloat_with_epsilon (r, 0.2, 0.001);

  pango2_matrix_scale (&m, 2, 3);

  a = pango2_matrix_get_rotation (&m);
  g_assert_cmpfloat_with_epsilon (a, 0, 0.001);

  pango2_matrix_get_font_scale_factors (&m, &sx, &sy);
  g_assert_cmpfloat_with_epsilon (sx, 2, 0.001);
  g_assert_cmpfloat_with_epsilon (sy, 3, 0.001);

  pango2_matrix_scale (&m, 1/sx, 1/sy);

  r = pango2_matrix_get_slant_ratio (&m);
  g_assert_cmpfloat_with_epsilon (r, 0.2, 0.001);

  r = pango2_matrix_get_slant_ratio (&m2);
  g_assert_cmpfloat_with_epsilon (r, 0.4, 0.001);

  r = pango2_matrix_get_slant_ratio (&m3);

  g_assert_cmpfloat_with_epsilon (r, 0.15, 0.001);
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
