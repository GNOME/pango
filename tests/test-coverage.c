/* Pango
 * test-coverage.c: Test coverage
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

static void
test_coverage_basic (void)
{
  PangoCoverage *coverage;
  int i;

  coverage = pango_coverage_new ();

  for (i = 0; i < 100; i++)
    g_assert_cmpint (pango_coverage_get (coverage, i), ==, PANGO_COVERAGE_NONE);

  for (i = 0; i < 100; i++)
    pango_coverage_set (coverage, i, PANGO_COVERAGE_EXACT);

  for (i = 0; i < 100; i++)
    g_assert_cmpint (pango_coverage_get (coverage, i), ==, PANGO_COVERAGE_EXACT);

  for (i = 0; i < 100; i++)
    pango_coverage_set (coverage, i, PANGO_COVERAGE_NONE);

  for (i = 0; i < 100; i++)
    g_assert_cmpint (pango_coverage_get (coverage, i), ==, PANGO_COVERAGE_NONE);

  g_object_unref (coverage);
}

static void
test_coverage_copy (void)
{
  PangoCoverage *coverage;
  PangoCoverage *coverage2;
  int i;

  coverage = pango_coverage_new ();

  for (i = 0; i < 100; i++)
    pango_coverage_set (coverage, i, PANGO_COVERAGE_EXACT);

  coverage2 = pango_coverage_copy (coverage);

  for (i = 0; i < 50; i++)
    pango_coverage_set (coverage, i, PANGO_COVERAGE_NONE);

  for (i = 0; i < 100; i++)
    g_assert_cmpint (pango_coverage_get (coverage2, i), ==, PANGO_COVERAGE_EXACT);

  g_object_unref (coverage);
  g_object_unref (coverage2);
}

static void
test_coverage_decomposition (void)
{
  PangoCoverage *coverage;

  coverage = pango_coverage_new ();

  pango_coverage_set (coverage, 0x004f, PANGO_COVERAGE_EXACT); /* Capital Letter O */
  pango_coverage_set (coverage, 0x0303, PANGO_COVERAGE_EXACT); /* Combining Tilde */
  pango_coverage_set (coverage, 0x0304, PANGO_COVERAGE_EXACT); /* Combining Macron */

  g_assert_cmpint (pango_coverage_get (coverage, 0x0d5), ==, PANGO_COVERAGE_EXACT); /* Capital Letter O with Tilde */
  g_assert_cmpint (pango_coverage_get (coverage, 0x014c), ==, PANGO_COVERAGE_EXACT); /* Capital Letter O with Macron */
  g_assert_cmpint (pango_coverage_get (coverage, 0x022c), ==, PANGO_COVERAGE_EXACT); /* Capital Letter O with Tilde and Macron */

  g_object_unref (coverage);
}

int
main (int argc, char *argv[])
{
  g_test_init (&argc, &argv, NULL);

  g_test_add_func ("/coverage/basic", test_coverage_basic);
  g_test_add_func ("/coverage/copy", test_coverage_copy);
  g_test_add_func ("/coverage/decomposition", test_coverage_decomposition);

  return g_test_run ();
}
