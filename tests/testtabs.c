/* Pango
 * testtabs.c: Test program for PangoTabArray
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

static void
test_tabs_basic (void)
{
  PangoTabArray *tabs;
  PangoTabAlign align;
  int location;

  tabs = pango_tab_array_new (1, TRUE);

  g_assert_true (pango_tab_array_get_positions_in_pixels (tabs));
  g_assert_true (pango_tab_array_get_size (tabs) == 1);

  pango_tab_array_set_tab (tabs, 0, PANGO_TAB_LEFT, 10);
  pango_tab_array_get_tab (tabs, 0, &align, &location);
  g_assert_true (align == PANGO_TAB_LEFT);
  g_assert_true (location == 10);

  pango_tab_array_free (tabs);
}

static void
test_tabs_copy (void)
{
  PangoTabArray *tabs, *tabs2;
  PangoTabAlign *alignments;
  int *locations;

  tabs = pango_tab_array_new_with_positions (2, TRUE,
                                             PANGO_TAB_LEFT, 10,
                                             PANGO_TAB_LEFT, 20);

  tabs2 = pango_tab_array_copy (tabs);
  pango_tab_array_get_tabs (tabs2, &alignments, &locations);
  g_assert_true (alignments[0] == PANGO_TAB_LEFT);
  g_assert_true (alignments[1] == PANGO_TAB_LEFT);
  g_assert_true (locations[0] == 10);
  g_assert_true (locations[1] == 20);

  g_free (alignments);
  g_free (locations);

  pango_tab_array_free (tabs);
  pango_tab_array_free (tabs2);
}

static void
test_tabs_resize (void)
{
  PangoTabArray *tabs;
  PangoTabAlign *alignments;
  int *locations;

  tabs = pango_tab_array_new (1, TRUE);

  pango_tab_array_set_tab (tabs, 0, PANGO_TAB_LEFT, 10);

  g_assert_cmpint (pango_tab_array_get_size (tabs), ==, 1);

  pango_tab_array_resize (tabs, 2);
  g_assert_cmpint (pango_tab_array_get_size (tabs), ==, 2);

  pango_tab_array_set_tab (tabs, 1, PANGO_TAB_RIGHT, 20);
  pango_tab_array_set_tab (tabs, 2, PANGO_TAB_CENTER, 30);
  pango_tab_array_set_tab (tabs, 3, PANGO_TAB_DECIMAL, 40);

  g_assert_cmpint (pango_tab_array_get_size (tabs), ==, 4);

  pango_tab_array_get_tabs (tabs, &alignments, &locations);
  g_assert_cmpint (alignments[0], ==, PANGO_TAB_LEFT);
  g_assert_cmpint (alignments[1], ==, PANGO_TAB_RIGHT);
  g_assert_cmpint (alignments[2], ==, PANGO_TAB_CENTER);
  g_assert_cmpint (alignments[3], ==, PANGO_TAB_DECIMAL);
  g_assert_cmpint (locations[0], ==, 10);
  g_assert_cmpint (locations[1], ==, 20);
  g_assert_cmpint (locations[2], ==, 30);
  g_assert_cmpint (locations[3], ==, 40);

  g_free (alignments);
  g_free (locations);

  pango_tab_array_free (tabs);
}

int
main (int argc, char *argv[])
{
  g_test_init (&argc, &argv, NULL);

  g_test_add_func ("/tabs/basic", test_tabs_basic);
  g_test_add_func ("/tabs/copy", test_tabs_copy);
  g_test_add_func ("/tabs/resize", test_tabs_resize);

  return g_test_run ();
}
