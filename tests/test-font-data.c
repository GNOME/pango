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

#include "test-common.h"

#undef DEBUG

static PangoFont *
load_font_with_font_options (PangoFontMap         *fontmap,
                             PangoFontDescription *desc,
                             cairo_hint_style_t    hint_style,
                             cairo_hint_metrics_t  hint_metrics)
{
  PangoContext *context;
  cairo_font_options_t *options;
  PangoFont *font;

  context = pango_font_map_create_context (fontmap);

  options = cairo_font_options_create ();
  cairo_font_options_set_hint_style (options, hint_style);
  cairo_font_options_set_hint_metrics (options, hint_metrics);
  pango_cairo_context_set_font_options (context, options);
  cairo_font_options_destroy (options);

  font = pango_context_load_font (context, desc);

  g_object_unref (context);

  return font;
}

static PangoFontMap *
get_font_map_with_boxes (void)
{
  PangoFontMap *fontmap;
  char *path;
  GError *error = NULL;
  PangoFontDescription *desc;
  PangoFont *font;

  fontmap = pango_cairo_font_map_new ();
  path = g_test_build_filename (G_TEST_DIST, "fonts", "boxes.ttf", NULL);

  pango_font_map_add_font_file (fontmap, path, &error);
  if (error)
    {
      g_clear_error (&error);
      g_clear_object (&fontmap);
      return NULL;
    }

  desc = pango_font_description_from_string ("Boxes 1px");
  font = load_font_with_font_options (fontmap, desc, CAIRO_HINT_STYLE_NONE, CAIRO_HINT_METRICS_OFF);

  if (strcmp (pango_font_family_get_name (pango_font_face_get_family (pango_font_get_face (font))),  "Boxes") != 0)
    {
      g_clear_object (&fontmap);
    }

  g_object_unref (font);
  pango_font_description_free (desc);
  g_free (path);

  return fontmap;
}

static PangoFontMetrics boxes_unhinted_metrics = {
  .ascent = 204800,
  .descent = 102400,
  .height = 307200,
  .approximate_char_width = 49273,
  .approximate_digit_width = 58368,
  /* Font doesn't have these set, so we get pango defaults */
  .underline_position = -1024,
  .underline_thickness = 1024,
  .strikethrough_position = 102400,
  .strikethrough_thickness = 1024,
};

#ifdef DEBUG
static void
print_metrics (PangoFont *font)
{
  PangoFontMetrics *metrics;

  metrics = pango_font_get_metrics (font, pango_language_from_string ("en"));

  g_print ("ascent %d\n", metrics->ascent);
  g_print ("descent %d\n", metrics->descent);
  g_print ("height %d\n", metrics->height);
  g_print ("char width %d\n", metrics->approximate_char_width);
  g_print ("digit width %d\n", metrics->approximate_digit_width);
  g_print ("underline %d %d\n", metrics->underline_position, metrics->underline_thickness);
  g_print ("strikethrough %d %d\n", metrics->strikethrough_position, metrics->strikethrough_thickness);

  pango_font_metrics_unref (metrics);
}
#endif

static void
test_boxes_font_metrics (void)
{
  PangoFontMap *fontmap;
  PangoFontDescription *desc;
  PangoFont *font;
  PangoFontMetrics *metrics;

  fontmap = get_font_map_with_boxes ();
  if (!fontmap)
    {
      g_test_fail_printf ("Failed to get a fontmap with Boxes");
      return;
    }

  desc = pango_font_description_from_string ("Boxes 100px");
  font = load_font_with_font_options (fontmap, desc, CAIRO_HINT_STYLE_NONE, CAIRO_HINT_METRICS_OFF);

#ifdef DEBUG
  print_metrics (font);
  return;
#endif

  metrics = pango_font_get_metrics (font, pango_language_from_string ("en"));

  g_assert_cmpint (metrics->ascent, ==, boxes_unhinted_metrics.ascent);
  g_assert_cmpint (metrics->descent, ==, boxes_unhinted_metrics.descent);
  g_assert_cmpint (metrics->approximate_char_width, ==, boxes_unhinted_metrics.approximate_char_width);
  g_assert_cmpint (metrics->approximate_digit_width, ==, boxes_unhinted_metrics.approximate_digit_width);
  g_assert_cmpint (metrics->underline_position, ==, boxes_unhinted_metrics.underline_position);
  g_assert_cmpint (metrics->underline_thickness, ==, boxes_unhinted_metrics.underline_thickness);
  g_assert_cmpint (metrics->strikethrough_position, ==, boxes_unhinted_metrics.strikethrough_position);
  g_assert_cmpint (metrics->strikethrough_thickness, ==, boxes_unhinted_metrics.strikethrough_thickness);

  pango_font_metrics_unref (metrics);

  g_object_unref (font);
  pango_font_description_free (desc);
  g_object_unref (fontmap);
}

typedef struct
{
  const char *name;
  PangoGlyph id;
  PangoRectangle ink;
  PangoRectangle logical;
} GlyphMetrics;

static GlyphMetrics boxes_unhinted_glyphs[] = {
  { .name = "A",
    .id = 1,
    .ink = { 0, -100, 100, 100 },
    .logical = { 0, -204800, 102400, 307200 }
  },
  { .name = "B",
    .id = 2,
    .ink = { 0, -200, 200, 200 },
    .logical = { 0, -204800, 102400, 307200 }
  },
  { .name = "C",
    .id = 3,
    .ink = { 0, -400, 400, 400 },
    .logical = { 0, -204800, 102400, 307200 }
  },
  { .name = "D",
    .id = 4,
    .ink = { 0, -800, 800, 800 },
    .logical = { 0, -204800, 102400, 307200 }
  },
  { .name = "E",
    .id = 5,
    .ink = { 0, -1600, 1600, 1600 },
    .logical = { 0, -204800, 102400, 307200 }
  },
  { .name = "F",
    .id = 6,
    .ink = { 0, -3200, 3200, 3200 },
    .logical = { 0, -204800, 102400, 307200 }
  },
  { .name = "G",
    .id = 7,
    .ink = { 0, -6400, 6400, 6400 },
    .logical = { 0, -204800, 102400, 307200 }
  },
  { .name = "H",
    .id = 8,
    .ink = { 0, -12800, 12800, 12800 },
    .logical = { 0, -204800, 102400, 307200 }
  },
  { .name = "I",
    .id = 9,
    .ink = { 0, -25600, 25600, 25600 },
    .logical = { 0, -204800, 102400, 307200 }
  },
  { .name = "J",
    .id = 10,
    .ink = { 0, -51200, 51200, 51200 },
    .logical = { 0, -204800, 102400, 307200 }
  },
  { .name = "K",
    .id = 11,
    .ink = { 0, -102400, 102400, 102400 },
    .logical = { 0, -204800, 102400, 307200 }
  },
};

#ifdef DEBUG
static void
print_glyph_metrics (PangoFont  *font,
                     gunichar    ch)
{
  hb_font_t *hb_font;
  hb_codepoint_t glyph;
  char name[6];

  g_unichar_to_utf8 (ch, name);

  hb_font = pango_font_get_hb_font (font);

  if (!hb_font_get_glyph (hb_font, ch, 0, &glyph))
    {
      g_print ("Not a glyph: %s\n", name);
    }
  else
    {
      PangoRectangle ink, logical;

      pango_font_get_glyph_extents (font, glyph, &ink, &logical);

      g_print ("{ .name = \"%s\",\n", name);
      g_print ("  .id = %u,\n", glyph);
      g_print ("  .ink = { %d, %d, %d, %d },\n", ink.x, ink.y, ink.width, ink.height);
      g_print ("  .logical = { %d, %d, %d, %d }\n", logical.x, logical.y, logical.width, logical.height);
      g_print ("},\n");
    }
}
#endif

static gboolean
pango_rectangle_equal (const PangoRectangle *a,
                       const PangoRectangle *b)
{
  return a->x == b->x &&
         a->y == b->y &&
         a->width == b->width &&
         a->height == b->height;
}

#define assert_rectangle_equal(s, a, b) \
G_STMT_START { \
  const PangoRectangle *ar = (a); \
  const PangoRectangle *br = (b); \
  if (!pango_rectangle_equal (ar, br)) \
    { \
      char msg[1024]; \
      g_snprintf (msg, sizeof (msg), "assertion failed for %s ( { %d %d %d %d } == { %d %d %d %d } )", \
                  s, \
                  ar->x, ar->y, ar->width, ar->height, \
                  br->x, br->y, br->width, br->height); \
      g_assertion_message (G_LOG_DOMAIN, __FILE__, __LINE__, G_STRFUNC, msg); \
    } \
} G_STMT_END

static void
test_boxes_glyph_metrics (void)
{
  PangoFontDescription *desc;
  PangoFontMap *fontmap;
  PangoFont *font;

  fontmap = get_font_map_with_boxes ();
  if (!fontmap)
    {
      g_test_fail_printf ("Failed to get a fontmap with Boxes");
      return;
    }

  desc = pango_font_description_from_string ("Boxes 100px");
  font = load_font_with_font_options (fontmap, desc, CAIRO_HINT_STYLE_NONE, CAIRO_HINT_METRICS_OFF);

#ifdef DEBUG
  print_glyph_metrics (font, 'A');
  print_glyph_metrics (font, 'B');
  print_glyph_metrics (font, 'C');
  print_glyph_metrics (font, 'D');
  print_glyph_metrics (font, 'E');
  print_glyph_metrics (font, 'F');
  print_glyph_metrics (font, 'G');
  print_glyph_metrics (font, 'H');
  print_glyph_metrics (font, 'I');
  print_glyph_metrics (font, 'J');
  print_glyph_metrics (font, 'K');
  return;
#endif

  for (int i = 0; i < G_N_ELEMENTS (boxes_unhinted_glyphs); i++)
    {
      GlyphMetrics *gm = &boxes_unhinted_glyphs[i];
      PangoRectangle ink, logical;
      char str[64];

      pango_font_get_glyph_extents (font, gm->id, &ink, &logical);
      g_snprintf (str, sizeof (str), "%s (ink)", gm->name);
      assert_rectangle_equal (str, &ink, &gm->ink);
      g_snprintf (str, sizeof (str), "%s (logical)", gm->name);
      assert_rectangle_equal (str, &logical, &gm->logical);
    }

  g_object_unref (font);
  pango_font_description_free (desc);
  g_object_unref (fontmap);
}

static GlyphMetrics boxes_unhinted_glyphs2[] = {
  { .name = "L",
    .id = 12,
    .ink = { 0, -5120, 5120, 5120 },
    .logical = { 0, -20480, 10240, 30720 }
  },
  { .name = "M",
    .id = 13,
    .ink = { 5120, -5120, 5120, 5120 },
    .logical = { 0, -20480, 10240, 30720 }
  },
  { .name = "N",
    .id = 14,
    .ink = { 5120, -10240, 5120, 5120 },
    .logical = { 0, -20480, 10240, 30720 }
  },
  { .name = "O",
    .id = 15,
    .ink = { 0, -10240, 5120, 5120 },
    .logical = { 0, -20480, 10240, 30720 }
  },
  { .name = "P",
    .id = 16,
    .ink = { 2560, -7680, 5120, 5120 },
    .logical = { 0, -20480, 10240, 30720 }
  },
};

static void
test_boxes_glyph_metrics2 (void)
{
  PangoFontDescription *desc;
  PangoFontMap *fontmap;
  PangoFont *font;

  fontmap = get_font_map_with_boxes ();
  if (!fontmap)
    {
      g_test_fail_printf ("Failed to get a fontmap with Boxes");
      return;
    }

  desc = pango_font_description_from_string ("Boxes 10px");
  font = load_font_with_font_options (fontmap, desc, CAIRO_HINT_STYLE_NONE, CAIRO_HINT_METRICS_OFF);

#ifdef DEBUG
  print_glyph_metrics (font, 'L');
  print_glyph_metrics (font, 'M');
  print_glyph_metrics (font, 'N');
  print_glyph_metrics (font, 'O');
  print_glyph_metrics (font, 'P');
  return;
#endif

  for (int i = 0; i < G_N_ELEMENTS (boxes_unhinted_glyphs2); i++)
    {
      GlyphMetrics *gm = &boxes_unhinted_glyphs2[i];
      PangoRectangle ink, logical;
      char str[64];

      pango_font_get_glyph_extents (font, gm->id, &ink, &logical);
      g_snprintf (str, sizeof (str), "%s (ink)", gm->name);
      assert_rectangle_equal (str, &ink, &gm->ink);
      g_snprintf (str, sizeof (str), "%s (ink)", gm->name);
      assert_rectangle_equal (str, &logical, &gm->logical);
    }

  g_object_unref (font);
  pango_font_description_free (desc);
  g_object_unref (fontmap);
}

int
main (int argc, char *argv[])
{
  setlocale (LC_ALL, "");

  g_test_init (&argc, &argv, NULL);
  g_test_set_nonfatal_assertions ();

  g_test_add_func ("/pango/font/boxes/font-metrics", test_boxes_font_metrics);
  g_test_add_func ("/pango/font/boxes/glyph-metrics", test_boxes_glyph_metrics);
  g_test_add_func ("/pango/font/boxes/glyph-metrics2", test_boxes_glyph_metrics2);


  return g_test_run ();
}
