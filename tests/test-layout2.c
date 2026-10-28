/*
 * Copyright (C) 2025 Red Hat, Inc
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Library General Public
 * License as published by the Free Software Foundation; either
 * version 2 of the License, or (at your option) any later version.
 *
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
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

const char bad_layout[] =
  "{\n"
  "  \"text\" : \"r ﻞُﻠُﺼّﺒُﻠُﻠﺼّﺑُﺭﺭً ॣ ॣh ॣ ॣ 冗\",\n"
  "  \"wrap\" : \"word-char\",\n"
  "  \"width\" : 0\n"
  "}";

static void
test_bad_layout (void)
{
  GError *error = NULL;
  GBytes *orig;
  PangoContext *context;
  PangoLayout *layout;
  GBytes *bytes;

  orig = g_bytes_new (bad_layout, sizeof (bad_layout));

  context = pango_font_map_create_context (pango_cairo_font_map_get_default ());
  layout = pango_layout_deserialize (context, orig, PANGO_LAYOUT_DESERIALIZE_CONTEXT, &error);
  g_assert_no_error (error);

  bytes = pango_layout_serialize (layout, PANGO_LAYOUT_SERIALIZE_CONTEXT | PANGO_LAYOUT_SERIALIZE_OUTPUT);

  g_assert_nonnull (bytes);
  g_bytes_unref (bytes);

  g_object_unref (layout);
  g_object_unref (context);

  g_bytes_unref (orig);
}

static void
test_large_wrapped_item (void)
{
  PangoContext *context;
  PangoLayout *layout;
  PangoFontDescription *font;
  PangoAttrList *attrs;
  GString *text;
  GSList *lines;
  int char_width;
  int offset = 0;
  const int n_chars = 500000;
  const int chars_per_line = 20;

  context = pango_font_map_create_context (pango_cairo_font_map_get_default ());
  layout = pango_layout_new (context);
  font = pango_font_description_from_string ("Monospace 12");
  pango_layout_set_font_description (layout, font);
  pango_font_description_free (font);
  attrs = pango_attr_list_new ();
  pango_attr_list_insert (attrs, pango_attr_insert_hyphens_new (FALSE));
  pango_layout_set_attributes (layout, attrs);
  pango_attr_list_unref (attrs);

  pango_layout_set_text (layout, "a", 1);
  pango_layout_get_size (layout, &char_width, NULL);
  g_assert_cmpint ((gint64) char_width * n_chars, >, G_MAXINT);

  text = g_string_sized_new (n_chars);
  for (int i = 0; i < n_chars; i++)
    g_string_append_c (text, 'a');

  pango_layout_set_text (layout, text->str, text->len);
  pango_layout_set_wrap (layout, PANGO_WRAP_CHAR);
  pango_layout_set_width (layout, chars_per_line * char_width);
  g_assert_cmpint (pango_layout_get_line_count (layout), ==, n_chars / chars_per_line);

  lines = pango_layout_get_lines_readonly (layout);
  for (GSList *l = lines; l; l = l->next)
    {
      PangoLayoutLine *line = l->data;
      PangoRectangle rect;

      g_assert_cmpint (line->start_index, ==, offset);
      g_assert_cmpint (line->length, ==, chars_per_line);
      pango_layout_line_get_extents (line, NULL, &rect);
      g_assert_cmpint (rect.width, ==, chars_per_line * char_width);
      offset += line->length;
    }
  g_assert_cmpint (offset, ==, n_chars);

  g_string_free (text, TRUE);
  g_object_unref (layout);
  g_object_unref (context);
}

int
main (int argc, char *argv[])
{
  g_test_init (&argc, &argv, NULL);

  g_test_add_func ("/layout/bad.layout", test_bad_layout);
  g_test_add_func ("/layout/large-wrapped-item", test_large_wrapped_item);

  return g_test_run ();
}
