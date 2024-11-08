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


#ifdef HAVE_FREETYPE
#include <pango/pangoft2.h>
#include <pango/pangocairo-fc.h>
#include <fontconfig/fontconfig.h>
#endif
#include <math.h>

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

#if 1
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

static void
test_cantarell_font_metrics (void)
{
  PangoFontMap *fontmap;
  PangoFontDescription *desc;
  PangoFont *font;
  PangoFontMetrics *metrics;

  desc = pango_font_description_from_string ("Cantarell 11");
  fontmap = get_font_map_with_cantarell ();
  font = load_font_with_font_options (fontmap, desc, CAIRO_HINT_STYLE_NONE, CAIRO_HINT_METRICS_OFF);

  if (strcmp (pango_font_family_get_name (pango_font_face_get_family (pango_font_get_face (font))), "Cantarell") != 0)
    {
      g_test_skip ("Fontmap has no Cantarell");
      return;
    }

  print_metrics (font);

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

  font = load_font_with_font_options (fontmap, desc, CAIRO_HINT_STYLE_FULL, CAIRO_HINT_METRICS_ON);

  print_metrics (font);

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
  g_object_unref (fontmap);
}

#if 0
typedef struct
{
  char name[2];
  PangoGlyph id;
  PangoRectangle ink;
  PangoRectangle logical;
} GlyphMetrics;

static GlyphMetrics cantarell_unhinted_glyphs[] = {
  { .name = "a",
    .id = 244,
    .ink = (PangoRectangle) { 706, -7389, 5857, 7540 },
    .logical = (PangoRectangle) { 0, -14764, 7690, 18023 }
  },
  { .name = "b",
    .id = 272,
    .ink = (PangoRectangle) { 1247, -11099, 6533, 11249 },
    .logical = (PangoRectangle) { 0, -14764, 8561, 18023 }
  },
  { .name = "c",
    .id = 273,
    .ink = (PangoRectangle) { 781, -7389, 5677, 7540 },
    .logical = (PangoRectangle) { 0, -14764, 7014, 18023 }
  },
  { .name = "d",
    .id = 280,
    .ink = (PangoRectangle) { 781, -11099, 6533, 11249 },
    .logical = (PangoRectangle) { 0, -14764, 8561, 18023 }
  },
  { .name = "e",
    .id = 287,
    .ink = (PangoRectangle) { 781, -7389, 6368, 7540 },
    .logical = (PangoRectangle) { 0, -14764, 7930, 18023 }
  },
  { .name = "f",
    .id = 311,
    .ink = (PangoRectangle) { 406, -11249, 5076, 11249 },
    .logical = (PangoRectangle) { 0, -14764, 5106, 18023 }
  },
  { .name = "g",
    .id = 312,
    .ink = (PangoRectangle) { 781, -7389, 6533, 10799 },
    .logical = (PangoRectangle) { 0, -14764, 8561, 18023 }
  },
  { .name = "h",
    .id = 319,
    .ink = (PangoRectangle) { 1247, -11099, 6143, 11099 },
    .logical = (PangoRectangle) { 0, -14764, 8516, 18023 }
  },
  { .name = "A",
    .id = 1,
    .ink = (PangoRectangle) { 105, -10423, 9192, 10423 },
    .logical = (PangoRectangle) { 0, -14764, 9402, 18023 }
  },
  { .name = "B",
    .id = 29,
    .ink = (PangoRectangle) { 1382, -10423, 7449, 10423 },
    .logical = (PangoRectangle) { 0, -14764, 9462, 18023 }
  },
  { .name = "C",
    .id = 30,
    .ink = (PangoRectangle) { 811, -10558, 8245, 10709 },
    .logical = (PangoRectangle) { 0, -14764, 9687, 18023 }
  },
  { .name = "D",
    .id = 37,
    .ink = (PangoRectangle) { 1382, -10423, 8486, 10423 },
    .logical = (PangoRectangle) { 0, -14764, 10679, 18023 }
  },
  { .name = "E",
    .id = 45,
    .ink = (PangoRectangle) { 1382, -10423, 6548, 10423 },
    .logical = (PangoRectangle) { 0, -14764, 8771, 18023 }
  },
  { .name = "F",
    .id = 68,
    .ink = (PangoRectangle) { 1382, -10423, 6473, 10423 },
    .logical = (PangoRectangle) { 0, -14764, 8456, 18023 }
  },
  { .name = "G",
    .id = 69,
    .ink = (PangoRectangle) { 811, -10558, 9026, 10694 },
    .logical = (PangoRectangle) { 0, -14764, 10739, 18023 }
  },
  { .name = "H",
    .id = 76,
    .ink = (PangoRectangle) { 1382, -10423, 8065, 10423 },
    .logical = (PangoRectangle) { 0, -14764, 10829, 18023 }
  }
};

static GlyphMetrics cantarell_hinted_glyphs[] = {
  { .name = "a",
    .id = 244,
    .ink = (PangoRectangle) { 0, -8192, 7168, 8192 },
    .logical = (PangoRectangle) { 0, -15360, 8192, 19456 }
  },
  { .name = "b",
    .id = 272,
    .ink = (PangoRectangle) { 1024, -11264, 7168, 11264 },
    .logical = (PangoRectangle) { 0, -15360, 8192, 19456 }
  },
  { .name = "c",
    .id = 273,
    .ink = (PangoRectangle) { 0, -8192, 7168, 8192 },
    .logical = (PangoRectangle) { 0, -15360, 7168, 19456 }
  },
  { .name = "d",
    .id = 280,
    .ink = (PangoRectangle) { 0, -11264, 8192, 11264 },
    .logical = (PangoRectangle) { 0, -15360, 8192, 19456 }
  },
  { .name = "e",
    .id = 287,
    .ink = (PangoRectangle) { 0, -8192, 7168, 8192 },
    .logical = (PangoRectangle) { 0, -15360, 8192, 19456 }
  },
  { .name = "f",
    .id = 311,
    .ink = (PangoRectangle) { 0, -11264, 6144, 11264 },
    .logical = (PangoRectangle) { 0, -15360, 5120, 19456 }
  },
  { .name = "g",
    .id = 312,
    .ink = (PangoRectangle) { 0, -8192, 8192, 12288 },
    .logical = (PangoRectangle) { 0, -15360, 8192, 19456 }
  },
  { .name = "h",
    .id = 319,
    .ink = (PangoRectangle) { 1024, -11264, 7168, 11264 },
    .logical = (PangoRectangle) { 0, -15360, 8192, 19456 }
  },
  { .name = "A",
    .id = 1,
    .ink = (PangoRectangle) { 0, -11264, 10240, 11264 },
    .logical = (PangoRectangle) { 0, -15360, 9216, 19456 }
  },
  { .name = "B",
    .id = 29,
    .ink = (PangoRectangle) { 1024, -11264, 8192, 11264 },
    .logical = (PangoRectangle) { 0, -15360, 9216, 19456 }
  },
  { .name = "C",
    .id = 30,
    .ink = (PangoRectangle) { 0, -11264, 9216, 11264 },
    .logical = (PangoRectangle) { 0, -15360, 9216, 19456 }
  },
  { .name = "D",
    .id = 37,
    .ink = (PangoRectangle) { 1024, -11264, 9216, 11264 },
    .logical = (PangoRectangle) { 0, -15360, 10240, 19456 }
  },
  { .name = "E",
    .id = 45,
    .ink = (PangoRectangle) { 1024, -11264, 7168, 11264 },
    .logical = (PangoRectangle) { 0, -15360, 9216, 19456 }
  },
  { .name = "F",
    .id = 68,
    .ink = (PangoRectangle) { 1024, -11264, 7168, 11264 },
    .logical = (PangoRectangle) { 0, -15360, 8192, 19456 }
  },
  { .name = "G",
    .id = 69,
    .ink = (PangoRectangle) { 0, -11264, 10240, 11264 },
    .logical = (PangoRectangle) { 0, -15360, 10240, 19456 }
  },
  { .name = "H",
    .id = 76,
    .ink = (PangoRectangle) { 1024, -11264, 9216, 11264 },
    .logical = (PangoRectangle) { 0, -15360, 11264, 19456 }
  },
};
#endif

#ifdef HAVE_FREETYPE
#include "pangofc-private.h"
#endif

static void
print_glyph_metrics (PangoFont  *font,
                     const char *name)
{
  hb_font_t *hb_font;
  hb_codepoint_t glyph;
  PangoRectangle ink, logical;
  hb_glyph_extents_t extents;
  cairo_scaled_font_t *scaled_font;

  hb_font = pango_font_get_hb_font (font);

  if (!hb_font_get_glyph_from_name (hb_font, name, -1, &glyph))
    {
      g_print ("Not a glyph: %s\n", name);
      return;
    }

  pango_font_get_glyph_extents (font, glyph, &ink, &logical);

  g_print ("name = \"%s\"\n", name);
  g_print ("id = %u\n\n", glyph);
  g_print ("ink_rect:\n"
           "  .x = %d\n"
           "  .y = %d\n"
           "  .width = %d\n"
           "  .height =  %d\n"
           "\n",
           ink.x, ink.y, ink.width, ink.height);
  g_print ("logical_rect:\n"
           "  .x = %d\n"
           "  .y = %d\n"
           "  .width = %d\n"
           "  .height =  %d\n"
           "\n",
           logical.x, logical.y, logical.width, logical.height);
  g_print ("},\n");

  if (hb_font_get_glyph_extents (hb_font, glyph, &extents))
    {
      g_print ("hb_glyph_extents:\n"
               "  .x_bearing = %d\n"
               "  .y_bearing = %d\n"
               "  .width = %d\n"
               "  .height = %d\n"
               "\n",
               extents.x_bearing,
               extents.y_bearing,
               extents.width,
               extents.height);
    }
  g_print ("hb h_advance: %d\n\n",
           hb_font_get_glyph_h_advance (hb_font, glyph));

#ifdef HAVE_FREETYPE
G_GNUC_BEGIN_IGNORE_DEPRECATIONS
  FT_Face ft_face;

  ft_face = pango_fc_font_lock_face (PANGO_FC_FONT (font));

  FT_Load_Glyph (ft_face, glyph, FT_LOAD_NO_HINTING);

  g_print ("FT_Glyph_Metrics: (in 26.6)\n"
           "  .width = %ld (== %ld)\n"
           "  .height = %ld (== %ld)\n"
           "  .horiBearingX = %ld (== %ld)\n"
           "  .horiBearingY = %ld (== %ld)\n"
           "  .horiAdvance = %ld (== %ld)\n"
           "\n",
           ft_face->glyph->metrics.width,
           PANGO_UNITS_26_6 (ft_face->glyph->metrics.width),
           ft_face->glyph->metrics.height,
           PANGO_UNITS_26_6 (ft_face->glyph->metrics.height),
           ft_face->glyph->metrics.horiBearingX,
           PANGO_UNITS_26_6 (ft_face->glyph->metrics.horiBearingX),
           ft_face->glyph->metrics.horiBearingY,
           PANGO_UNITS_26_6 (ft_face->glyph->metrics.horiBearingY),
           ft_face->glyph->metrics.horiAdvance,
           PANGO_UNITS_26_6 (ft_face->glyph->metrics.horiAdvance));
  g_print ("FT linearHoriAdvance: (in 16.16)\n"
           "  %ld (== %ld)\n\n",
           ft_face->glyph->linearHoriAdvance,
           (ft_face->glyph->linearHoriAdvance * PANGO_SCALE / (1 << 16)));

  pango_fc_font_unlock_face (PANGO_FC_FONT (font));
#endif

G_GNUC_END_IGNORE_DEPRECATIONS

  cairo_glyph_t cg = { .index = glyph, .x = 0, .y = 0 };
  cairo_text_extents_t ce;
  scaled_font = pango_cairo_font_get_scaled_font (PANGO_CAIRO_FONT (font));
  cairo_scaled_font_glyph_extents (scaled_font, &cg, 1, &ce);

  g_print ("cairo_text_extents_t: (in pixels)\n"
           "  .x_bearing = %f (== %d)\n"
           "  .y_bearing = %f (== %d)\n"
           "  .width = %f (== %d)\n"
           "  .height = %f (== %d)\n"
           "  .x_advance = %f (== %d)\n"
           "  .y_advance = %f (== %d)\n"
           "\n",
           ce.x_bearing,
           (int) round (ce.x_bearing * PANGO_SCALE),
           ce.y_bearing,
           (int) round (ce.y_bearing * PANGO_SCALE),
           ce.width,
           (int) round (ce.width * PANGO_SCALE),
           ce.height,
           (int) round (ce.height * PANGO_SCALE),
           ce.x_advance,
           (int) round (ce.x_advance * PANGO_SCALE),
           ce.y_advance,
           (int) round (ce.y_advance * PANGO_SCALE));
}

static gboolean
pango_rectangle_equal (const PangoRectangle *a,
                       const PangoRectangle *b)
{
  return a->x == b->x &&
         a->y == b->y &&
         a->width == b->width &&
         a->height == b->height;
}

static void
test_cantarell_glyph_metrics (void)
{
  PangoFontDescription *desc;
  PangoFontMap *fontmap;
  PangoFont *font;

  desc = pango_font_description_from_string ("Cantarell 11");
  fontmap = get_font_map_with_cantarell ();
  font = load_font_with_font_options (fontmap, desc, CAIRO_HINT_STYLE_NONE, CAIRO_HINT_METRICS_OFF);

  if (strcmp (pango_font_family_get_name (pango_font_face_get_family (pango_font_get_face (font))), "Cantarell") != 0)
    {
      g_test_skip ("Fontmap has no Cantarell");
      return;
    }

  print_glyph_metrics (font, "a");
  return;

#if 0
  for (int i = 0; i < G_N_ELEMENTS (cantarell_unhinted_glyphs); i++)
    {
      GlyphMetrics *gm = &cantarell_unhinted_glyphs[i];
      PangoRectangle ink, logical;

      pango_font_get_glyph_extents (font, gm->id, &ink, &logical);
      g_assert_true (pango_rectangle_equal (&ink, &gm->ink));
      g_assert_true (pango_rectangle_equal (&logical, &gm->logical));
    }
#endif

  g_object_unref (font);

  font = load_font_with_font_options (fontmap, desc, CAIRO_HINT_STYLE_FULL, CAIRO_HINT_METRICS_ON);

#if 0
  print_glyph_metrics (font, "a");
  print_glyph_metrics (font, "b");
  print_glyph_metrics (font, "c");
  print_glyph_metrics (font, "d");
  print_glyph_metrics (font, "e");
  print_glyph_metrics (font, "f");
  print_glyph_metrics (font, "g");
  print_glyph_metrics (font, "h");

  print_glyph_metrics (font, "A");
  print_glyph_metrics (font, "B");
  print_glyph_metrics (font, "C");
  print_glyph_metrics (font, "D");
  print_glyph_metrics (font, "E");
  print_glyph_metrics (font, "F");
  print_glyph_metrics (font, "G");
  print_glyph_metrics (font, "H");

  for (int i = 0; i < G_N_ELEMENTS (cantarell_hinted_glyphs); i++)
    {
      GlyphMetrics *gm = &cantarell_hinted_glyphs[i];
      PangoRectangle ink, logical;

      pango_font_get_glyph_extents (font, gm->id, &ink, &logical);
      g_assert_true (pango_rectangle_equal (&ink, &gm->ink));
      g_assert_true (pango_rectangle_equal (&logical, &gm->logical));
    }
#endif

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

  g_test_add_func ("/pango/font/cantarell/font-metrics", test_cantarell_font_metrics);
  g_test_add_func ("/pango/font/cantarell/glyph-metrics", test_cantarell_glyph_metrics);

  return g_test_run ();
}
