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
test_serialize_attr_list (void)
{
  const char *valid[] = {
    "5 16 style italic",
    "0 10 foreground red, 5 15 weight bold, 0 200 font-desc \"Sans Small-Caps 10\"",
    "0 10 foreground red\n5 15 weight bold\n0 200 font-desc \"Sans Small-Caps 10\"",
    "  0   10   fallback  false,\n 5 15 weight semilight\n\n \n \n",
    "0 100 font-desc \"Cantarell, Sans, Italic Ultra-Light 64\", 10 11 weight 100",
    "0 -1 size 10",
    "0 1 weight 700, 2 4 weight book",
    "0 200 rise 100\n5 15 family Times\n10 11 size 10240\n11 100 fallback 0\n30 60 stretch 2\n",
    ""
  };
  const char *roundtripped[] = {
    "5 16 style italic",
    "0 10 foreground #ffff00000000\n5 15 weight bold\n0 200 font-desc \"Sans Small-Caps 10\"",
    "0 10 foreground #ffff00000000\n5 15 weight bold\n0 200 font-desc \"Sans Small-Caps 10\"",
    "0 10 fallback false\n5 15 weight semilight",
    "0 100 font-desc \"Cantarell,Sans Ultra-Light Italic 64\"\n10 11 weight thin",
    "0 4294967295 size 10",
    "0 1 weight bold\n2 4 weight book",
    "0 200 rise 100\n5 15 family Times\n10 11 size 10240\n11 100 fallback false\n30 60 stretch condensed",
    ""
  };
  const char *invalid[] = {
    "not an attr list",
    "0 -1 FOREGROUND xyz",
    ",,bla.wewq",
  };

  for (int i = 0; i < G_N_ELEMENTS (valid); i++)
    {
      PangoAttrList *attrs;
      char *str;

      attrs = pango_attr_list_from_string (valid[i]);
      g_assert_nonnull (attrs);
      str = pango_attr_list_to_string (attrs);
      g_assert_cmpstr (str, ==, roundtripped[i]);
      g_free (str);
      pango_attr_list_unref (attrs);
    }

  for (int i = 0; i < G_N_ELEMENTS (invalid); i++)
    {
      PangoAttrList *attrs;

      attrs = pango_attr_list_from_string (invalid[i]);
      g_assert_null (attrs);
    }
}

int
main (int argc, char *argv[])
{
  g_test_init (&argc, &argv, NULL);

  g_test_add_func ("/serialize/attr-list", test_serialize_attr_list);

  return g_test_run ();
}
