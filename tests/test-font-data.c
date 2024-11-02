/* Pango
 * test-font.c: Test PangoFontDescription
 *
 * Copyright (C) 2024 Red Hat, Inc
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
#include <string.h>
#include <locale.h>

#include <gio/gio.h>
#include <pango/pangocairo.h>
#ifdef G_OS_WIN32
#include <pango/pangowin32.h>
#endif

struct {
  PangoGlyph glyph;
  PangoRectangle ink_rect;
  PangoRectangle logical_rect;
} glyph_data[] = {
};

static PangoFontMetrics cantarell_unhinted_metrics = {
  .ascent = 14764,
  .descent = 3259,
  .height = 18023,
  .approximate_char_width = 6679,
  .approximate_digit_width = 9216,
  .underline_position = -1502,
  .underline_thickness = 751,
  .strikethrough_position = 4340,
  .strikethrough_thickness = 751,
};

static PangoFontMetrics cantarell_hinted_metrics = {
  .ascent = 15360,
  .descent = 4096,
  .height = 18432,
  .approximate_char_width = 6679,
  .approximate_digit_width = 9216,
  .underline_position = -1024,
  .underline_thickness = 1024,
  .strikethrough_position = 5120,
  .strikethrough_thickness = 1024,
};

#if 0
static void
print_metrics (PangoFontMetrics *metrics)
{
  g_print ("ascent %d\n", metrics->ascent);
  g_print ("descent %d\n", metrics->descent);
  g_print ("height %d\n", metrics->height);
  g_print ("char width %d\n", metrics->approximate_char_width);
  g_print ("digit width %d\n", metrics->approximate_digit_width);
  g_print ("underline %d %d\n", metrics->underline_position, metrics->underline_thickness);
  g_print ("strikethrough %d %d\n", metrics->strikethrough_position, metrics->strikethrough_thickness);
}
#endif

static void
test_cantarell_metrics (void)
{
  PangoFontDescription *desc;
  PangoFontMap *fontmap;
  PangoContext *context;
  PangoFont *font;
  PangoFontMetrics *metrics;
  cairo_font_options_t *options;

  desc = pango_font_description_from_string ("Cantarell 11");

  fontmap = pango_cairo_font_map_new ();

#ifdef G_OS_WIN32
  if (strcmp (G_OBJECT_TYPE_NAME (fontmap), "PangoWin32FontMap") == 0)
    {
      GError *error = NULL;
      char *path;

      path = g_test_build_filename (G_TEST_DIST, "tests", "fonts", "Cantarell-VF.otf", NULL);
      pango_win32_font_map_add_font_file (fontmap, path, &error);
      g_assert_no_error (error);
      g_free (path);
    }
#endif

  context = pango_font_map_create_context (fontmap);

  options = cairo_font_options_create ();
  cairo_font_options_set_hint_style (options, CAIRO_HINT_STYLE_NONE);
  cairo_font_options_set_hint_metrics (options, CAIRO_HINT_METRICS_OFF);
  pango_cairo_context_set_font_options (context, options);
  cairo_font_options_destroy (options);

  font = pango_context_load_font (context, desc);
  metrics = pango_font_get_metrics (font, pango_language_from_string ("en"));

  g_assert_cmpint (metrics->ascent, ==, cantarell_unhinted_metrics.ascent);
  g_assert_cmpint (metrics->descent, ==, cantarell_unhinted_metrics.descent);
  g_assert_cmpint (metrics->approximate_char_width, ==, cantarell_unhinted_metrics.approximate_char_width);
  g_assert_cmpint (metrics->approximate_digit_width, ==, cantarell_unhinted_metrics.approximate_digit_width);
  g_assert_cmpint (metrics->underline_position, ==, cantarell_unhinted_metrics.underline_position);
  g_assert_cmpint (metrics->underline_thickness, ==, cantarell_unhinted_metrics.underline_thickness);
  g_assert_cmpint (metrics->strikethrough_position, ==, cantarell_unhinted_metrics.strikethrough_position);
  g_assert_cmpint (metrics->strikethrough_thickness, ==, cantarell_unhinted_metrics.strikethrough_thickness);

  pango_font_metrics_unref (metrics);
  g_object_unref (font);

  options = cairo_font_options_create ();
  cairo_font_options_set_hint_style (options, CAIRO_HINT_STYLE_FULL);
  cairo_font_options_set_hint_metrics (options, CAIRO_HINT_METRICS_ON);
  pango_cairo_context_set_font_options (context, options);
  cairo_font_options_destroy (options);

  font = pango_context_load_font (context, desc);
  metrics = pango_font_get_metrics (font, pango_language_from_string ("en"));

  g_assert_cmpint (metrics->ascent, ==, cantarell_hinted_metrics.ascent);
  g_assert_cmpint (metrics->descent, ==, cantarell_hinted_metrics.descent);
  g_assert_cmpint (metrics->approximate_char_width, ==, cantarell_hinted_metrics.approximate_char_width);
  g_assert_cmpint (metrics->approximate_digit_width, ==, cantarell_hinted_metrics.approximate_digit_width);
  g_assert_cmpint (metrics->underline_position, ==, cantarell_hinted_metrics.underline_position);
  g_assert_cmpint (metrics->underline_thickness, ==, cantarell_hinted_metrics.underline_thickness);
  g_assert_cmpint (metrics->strikethrough_position, ==, cantarell_hinted_metrics.strikethrough_position);
  g_assert_cmpint (metrics->strikethrough_thickness, ==, cantarell_hinted_metrics.strikethrough_thickness);

  pango_font_metrics_unref (metrics);
  g_object_unref (font);

  pango_font_description_free (desc);
  g_object_unref (context);
  g_object_unref (fontmap);
}

int
main (int argc, char *argv[])
{
  setlocale (LC_ALL, "");

  g_test_init (&argc, &argv, NULL);

  g_test_add_func ("/pango/font/cantarell/metrics", test_cantarell_metrics);

  return g_test_run ();
}
