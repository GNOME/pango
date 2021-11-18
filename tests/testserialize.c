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
#include <gio/gio.h>

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

static void
test_serialize_tab_array (void)
{
  const char *valid[] = {
    "0 10 100 200 400",
    "0px 10px 100px 200px 400px",
    "  0    10   ",
    "20 10",
    ""
  };
  const char *roundtripped[] = {
    "0 10 100 200 400",
    "0px 10px 100px 200px 400px",
    "0 10",
    "20 10",
    ""
  };
  const char *invalid[] = {
    "not a tabarray",
    "-10\n-20",
    "10ps 20pu",
    "10, 20",
    "10 20px 30",
  };

  for (int i = 0; i < G_N_ELEMENTS (valid); i++)
    {
      PangoTabArray *tabs;
      char *str;

      tabs = pango_tab_array_from_string (valid[i]);
      g_assert_nonnull (tabs);
      str = pango_tab_array_to_string (tabs);
      g_assert_cmpstr (str, ==, roundtripped[i]);
      g_free (str);
      pango_tab_array_free (tabs);
    }

  for (int i = 0; i < G_N_ELEMENTS (invalid); i++)
    {
      PangoTabArray *tabs;

      tabs = pango_tab_array_from_string (invalid[i]);
      g_assert_null (tabs);
    }
}

static void
test_serialize_layout_minimal (void)
{
  const char *test =
    "{\n"
    "  \"text\" : \"Almost nothing\"\n"
    "}";

  PangoContext *context;
  GBytes *bytes;
  PangoLayout *layout;
  GError *error = NULL;
  GBytes *out_bytes;
  const char *str;

  context = pango_font_map_create_context (pango_cairo_font_map_get_default ());

  bytes = g_bytes_new_static (test, -1);

  layout = pango_layout_deserialize (context, bytes, &error);
  g_assert_no_error (error);
  g_assert_true (PANGO_IS_LAYOUT (layout));
  g_assert_cmpstr (pango_layout_get_text (layout), ==, "Almost nothing");
  g_assert_null (pango_layout_get_attributes (layout));
  g_assert_null (pango_layout_get_tabs (layout));
  g_assert_null (pango_layout_get_font_description (layout));
  g_assert_cmpint (pango_layout_get_alignment (layout), ==, PANGO_ALIGN_LEFT);
  g_assert_cmpint (pango_layout_get_width (layout), ==, -1);

  out_bytes = pango_layout_serialize (layout);
  str = g_bytes_get_data (out_bytes, NULL);

  g_assert_cmpstr (str, ==, test);

  g_bytes_unref (out_bytes);

  g_object_unref (layout);
  g_bytes_unref (bytes);
  g_object_unref (context);
}

static void
test_serialize_layout_valid (void)
{
  const char *test =
    "{\n"
    "  \"text\" : \"Some fun with layouts!\",\n"
    "  \"attributes\" : [\n"
    "    {\n"
    "      \"end\" : 4,\n"
    "      \"type\" : \"foreground\",\n"
    "      \"value\" : \"#ffff00000000\"\n"
    "    },\n"
    "    {\n"
    "      \"start\" : 5,\n"
    "      \"end\" : 8,\n"
    "      \"type\" : \"foreground\",\n"
    "      \"value\" : \"#000080800000\"\n"
    "    },\n"
    "    {\n"
    "      \"start\" : 14,\n"
    "      \"type\" : \"foreground\",\n"
    "      \"value\" : \"#ffff0000ffff\"\n"
    "    }\n"
    "  ],\n"
    "  \"font\" : \"Sans Bold 32\",\n"
    "  \"tabs\" : {\n"
    "    \"positions-in-pixels\" : true,\n"
    "    \"positions\" : [\n"
    "      0,\n"
    "      50,\n"
    "      100\n"
    "    ]\n"
    "  },\n"
    "  \"alignment\" : \"center\",\n"
    "  \"width\" : 350000,\n"
    "  \"line-spacing\" : 1.5\n"
    "}";

  PangoContext *context;
  GBytes *bytes;
  PangoLayout *layout;
  PangoTabArray *tabs;
  GError *error = NULL;
  GBytes *out_bytes;
  const char *str;
  char *s;

  context = pango_font_map_create_context (pango_cairo_font_map_get_default ());

  bytes = g_bytes_new_static (test, -1);

  layout = pango_layout_deserialize (context, bytes, &error);
  g_assert_no_error (error);
  g_assert_true (PANGO_IS_LAYOUT (layout));
  g_assert_cmpstr (pango_layout_get_text (layout), ==, "Some fun with layouts!");
  g_assert_nonnull (pango_layout_get_attributes (layout));
  tabs = pango_layout_get_tabs (layout);
  g_assert_nonnull (tabs);
  pango_tab_array_free (tabs);
  s = pango_font_description_to_string (pango_layout_get_font_description (layout));
  g_assert_cmpstr (s, ==, "Sans Bold 32");
  g_free (s);
  g_assert_cmpint (pango_layout_get_alignment (layout), ==, PANGO_ALIGN_CENTER);
  g_assert_cmpint (pango_layout_get_width (layout), ==, 350000);
  g_assert_cmpfloat_with_epsilon (pango_layout_get_line_spacing (layout), 1.5, 0.0001);

  out_bytes = pango_layout_serialize (layout);
  str = g_bytes_get_data (out_bytes, NULL);

  g_assert_cmpstr (str, ==, test);

  g_bytes_unref (out_bytes);

  g_object_unref (layout);
  g_bytes_unref (bytes);
  g_object_unref (context);
}

static void
test_serialize_layout_invalid (void)
{
  struct {
    const char *json;
    int expected_error;
  } test[] = {
    {
      "{\n"
      "  \"attributes\" : [\n"
      "    {\n"
      "      \"type\" : \"caramba\"\n"
      "    }\n"
      "  ]\n"
      "}",
      PANGO_LAYOUT_SERIALIZE_INVALID_VALUE
    },
    {
      "{\n"
      "  \"attributes\" : [\n"
      "    {\n"
      "      \"type\" : \"weight\"\n"
      "    }\n"
      "  ]\n"
      "}",
      PANGO_LAYOUT_SERIALIZE_MISSING_VALUE
    },
    {
      "{\n"
      "  \"attributes\" : [\n"
      "    {\n"
      "      \"type\" : \"alignment\",\n"
      "      \"value\" : \"nonsense\"\n"
      "    }\n"
      "  ]\n"
      "}",
      PANGO_LAYOUT_SERIALIZE_INVALID_VALUE
    },
    {
      "{\n"
      "  \"alignment\" : \"nonsense\"\n"
      "}",
      PANGO_LAYOUT_SERIALIZE_INVALID_VALUE
    }
  };

  PangoContext *context;

  context = pango_font_map_create_context (pango_cairo_font_map_get_default ());

  for (int i = 0; i < G_N_ELEMENTS (test); i++)
    {
      GBytes *bytes;
      PangoLayout *layout;
      GError *error = NULL;

       bytes = g_bytes_new_static (test[i].json, -1);
       layout = pango_layout_deserialize (context, bytes, &error);
       g_assert_null (layout);
       g_assert_error (error, PANGO_LAYOUT_SERIALIZE_ERROR, test[i].expected_error);
       g_bytes_unref (bytes);
       g_clear_error (&error);
    }

  g_object_unref (context);
}
int
main (int argc, char *argv[])
{
  g_test_init (&argc, &argv, NULL);

  g_test_add_func ("/serialize/attr-list", test_serialize_attr_list);
  g_test_add_func ("/serialize/tab-array", test_serialize_tab_array);
  g_test_add_func ("/serialize/layout/minimal", test_serialize_layout_minimal);
  g_test_add_func ("/serialize/layout/valid", test_serialize_layout_valid);
  g_test_add_func ("/serialize/layout/invalid", test_serialize_layout_invalid);

  return g_test_run ();
}
