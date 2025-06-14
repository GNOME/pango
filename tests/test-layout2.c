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
#include <pango.h>
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

int
main (int argc, char *argv[])
{
  g_test_init (&argc, &argv, NULL);

  g_test_add_func ("/layout/bad.layout", test_bad_layout);

  return g_test_run ();
}
