/* Pango
 * test-font.c: Test fonts and font maps
 *
 * Copyright (C) 2014 Red Hat, Inc
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
#include <pango/pango.h>
#include <pango/pango-item-private.h>
#include <pango/pango-font-private.h>

#include "test-common.h"

static PangoContext *context;

static void
test_parse (void)
{
  PangoFontDescription **descs;
  PangoFontDescription *desc;

  descs = g_new (PangoFontDescription *, 2);

  descs[0] = desc = pango_font_description_from_string ("Cantarell 14");

  g_assert_cmpstr (pango_font_description_get_family (desc), ==, "Cantarell");
  g_assert (!pango_font_description_get_size_is_absolute (desc));
  g_assert_cmpint (pango_font_description_get_size (desc), ==, 14 * PANGO_SCALE);
  g_assert_cmpint (pango_font_description_get_style (desc), ==, PANGO_STYLE_NORMAL);
  g_assert_cmpint (pango_font_description_get_variant (desc), ==, PANGO_VARIANT_NORMAL);
  g_assert_cmpint (pango_font_description_get_weight (desc), ==, PANGO_WEIGHT_NORMAL);
  g_assert_cmpint (pango_font_description_get_stretch (desc), ==, PANGO_STRETCH_NORMAL);
  g_assert_cmpint (pango_font_description_get_gravity (desc), ==, PANGO_GRAVITY_SOUTH);
  g_assert_cmpint (pango_font_description_get_set_fields (desc), ==, PANGO_FONT_MASK_FAMILY | PANGO_FONT_MASK_STYLE | PANGO_FONT_MASK_VARIANT | PANGO_FONT_MASK_WEIGHT | PANGO_FONT_MASK_STRETCH | PANGO_FONT_MASK_SIZE);

  descs[1] = desc = pango_font_description_from_string ("Sans Bold Italic Condensed 22.5px");

  g_assert_cmpstr (pango_font_description_get_family (desc), ==, "Sans");
  g_assert (pango_font_description_get_size_is_absolute (desc)); 
  g_assert_cmpint (pango_font_description_get_size (desc), ==, 225 * PANGO_SCALE / 10);
  g_assert_cmpint (pango_font_description_get_style (desc), ==, PANGO_STYLE_ITALIC);
  g_assert_cmpint (pango_font_description_get_variant (desc), ==, PANGO_VARIANT_NORMAL); 
  g_assert_cmpint (pango_font_description_get_weight (desc), ==, PANGO_WEIGHT_BOLD);
  g_assert_cmpint (pango_font_description_get_stretch (desc), ==, PANGO_STRETCH_CONDENSED); 
  g_assert_cmpint (pango_font_description_get_gravity (desc), ==, PANGO_GRAVITY_SOUTH);  g_assert_cmpint (pango_font_description_get_set_fields (desc), ==, PANGO_FONT_MASK_FAMILY | PANGO_FONT_MASK_STYLE | PANGO_FONT_MASK_VARIANT | PANGO_FONT_MASK_WEIGHT | PANGO_FONT_MASK_STRETCH | PANGO_FONT_MASK_SIZE);

  pango_font_description_free (descs[0]);
  pango_font_description_free (descs[1]);
}

static void
test_roundtrip (void)
{
  PangoFontDescription *desc;
  char *str;

  desc = pango_font_description_from_string ("Cantarell 14");
  str = pango_font_description_to_string (desc);
  g_assert_cmpstr (str, ==, "Cantarell 14");
  pango_font_description_free (desc);
  g_free (str);

  desc = pango_font_description_from_string ("Sans Bold Italic Condensed 22.5px");
  str = pango_font_description_to_string (desc);
  g_assert_cmpstr (str, ==, "Sans Bold Italic Condensed 22.5px");
  pango_font_description_free (desc);
  g_free (str);
}

static void
test_variations (void)
{
  PangoFontDescription *desc1;
  PangoFontDescription *desc2;
  char *str;

  desc1 = pango_font_description_from_string ("Cantarell 14");
  g_assert_nonnull (desc1);
  g_assert_cmpint ((pango_font_description_get_set_fields (desc1) & PANGO_FONT_MASK_VARIATIONS), ==, 0);
  g_assert_cmpstr (pango_font_description_get_family (desc1), ==, "Cantarell");
  g_assert_cmpint (pango_font_description_get_size (desc1), ==, 14 * PANGO_SCALE);
  g_assert_null (pango_font_description_get_variations (desc1));

  str = pango_font_description_to_string (desc1);
  g_assert_cmpstr (str, ==, "Cantarell 14");
  g_free (str);

  desc2 = pango_font_description_from_string ("Cantarell 14 @wght=100,wdth=235");
  g_assert_nonnull (desc2);
  g_assert_cmpint ((pango_font_description_get_set_fields (desc2) & PANGO_FONT_MASK_VARIATIONS), ==, PANGO_FONT_MASK_VARIATIONS);
  g_assert_cmpstr (pango_font_description_get_family (desc2), ==, "Cantarell");
  g_assert_cmpint (pango_font_description_get_size (desc2), ==, 14 * PANGO_SCALE);
  g_assert_cmpstr (pango_font_description_get_variations (desc2), ==, "wght=100,wdth=235");

  str = pango_font_description_to_string (desc2);
  g_assert_cmpstr (str, ==, "Cantarell 14 @wght=100,wdth=235");
  g_free (str);

  g_assert_false (pango_font_description_equal (desc1, desc2));

  pango_font_description_set_variations (desc1, "wght=100,wdth=235");
  g_assert_cmpint ((pango_font_description_get_set_fields (desc1) & PANGO_FONT_MASK_VARIATIONS), ==, PANGO_FONT_MASK_VARIATIONS);
  g_assert_cmpstr (pango_font_description_get_variations (desc1), ==, "wght=100,wdth=235");

  g_assert_true (pango_font_description_equal (desc1, desc2));

  pango_font_description_free (desc1);
  pango_font_description_free (desc2);
}

static void
test_empty_variations (void)
{
  PangoFontDescription *desc;
  char *str;

  desc = pango_font_description_from_string ("Cantarell 14");
  g_assert_nonnull (desc);
  g_assert_cmpint ((pango_font_description_get_set_fields (desc) & PANGO_FONT_MASK_VARIATIONS), ==, 0);
  g_assert_null (pango_font_description_get_variations (desc));

  str = pango_font_description_to_string (desc);
  g_assert_cmpstr (str, ==, "Cantarell 14");
  g_free (str);

  pango_font_description_set_variations (desc, "");
  g_assert_cmpint ((pango_font_description_get_set_fields (desc) & PANGO_FONT_MASK_VARIATIONS), ==, PANGO_FONT_MASK_VARIATIONS);
  g_assert_cmpstr (pango_font_description_get_variations (desc), ==, "");

  str = pango_font_description_to_string (desc);
  g_assert_cmpstr (str, ==, "Cantarell 14");
  g_free (str);

  pango_font_description_free (desc);
}

static void
test_metrics (void)
{
  PangoFontDescription *desc;
  PangoFontMetrics *metrics;
  char *str;

  desc = pango_font_description_from_string ("Cantarell 11");

  str = pango_font_description_to_string (desc);

  metrics = pango_context_get_metrics (context, desc, pango_language_get_default ());

  g_test_message ("%s metrics", str);
  g_test_message ("\tascent: %d", pango_font_metrics_get_ascent (metrics));
  g_test_message ("\tdescent: %d", pango_font_metrics_get_descent (metrics));
  g_test_message ("\theight: %d", pango_font_metrics_get_height (metrics));
  g_test_message ("\tchar width: %d",
                  pango_font_metrics_get_approximate_char_width (metrics));
  g_test_message ("\tdigit width: %d",
                  pango_font_metrics_get_approximate_digit_width (metrics));
  g_test_message ("\tunderline position: %d",
                  pango_font_metrics_get_underline_position (metrics));
  g_test_message ("\tunderline thickness: %d",
                  pango_font_metrics_get_underline_thickness (metrics));
  g_test_message ("\tstrikethrough position: %d",
                  pango_font_metrics_get_strikethrough_position (metrics));
  g_test_message ("\tstrikethrough thickness: %d",
                  pango_font_metrics_get_strikethrough_thickness (metrics));

  pango_font_metrics_free (metrics);
  g_free (str);
  pango_font_description_free (desc);
}

static void
test_extents (void)
{
  const char *str = "Composer";
  GList *items;
  PangoItem *item;
  PangoGlyphString *glyphs;
  PangoRectangle ink, log;
  PangoContext *context;
  PangoFontDescription *desc;
  PangoDirection dir;

  context = pango_context_new ();
  desc = pango_font_description_from_string("Cantarell 11");
  pango_context_set_font_description (context, desc);
  pango_font_description_free (desc);

  dir = pango_context_get_base_dir (context);
  items = pango_itemize (context, dir, str, 0, strlen (str), NULL);
  glyphs = pango_glyph_string_new ();
  item = items->data;
  pango_shape (str, strlen (str), NULL, 0, &item->analysis, glyphs, PANGO_SHAPE_NONE);
  pango_glyph_string_extents (glyphs, item->analysis.font, &ink, &log);

  g_assert_cmpint (ink.width, >=, 0);
  g_assert_cmpint (ink.height, >=, 0);
  g_assert_cmpint (log.width, >=, 0);
  g_assert_cmpint (log.height, >=, 0);

  pango_glyph_string_free (glyphs);
  g_list_free_full (items, (GDestroyNotify)pango_item_free);
  g_object_unref (context);
}

static void
test_fontmap_enumerate (void)
{
  PangoFontMap *fontmap;
  PangoContext *context;
  PangoFontFamily *family;
  int i;
  PangoFontFace *face;
  PangoFontDescription *desc;
  PangoFont *font;
  gboolean found_face;

  fontmap = pango_font_map_get_default ();
  context = pango_context_new_with_font_map (fontmap);

  g_assert_cmpint (g_list_model_get_n_items (G_LIST_MODEL (fontmap)), >, 0);

  for (i = 0; i < g_list_model_get_n_items (G_LIST_MODEL (fontmap)); i++)
    {
      PangoFontFamily *fam = g_list_model_get_item (G_LIST_MODEL (fontmap), i);
      family = pango_font_map_get_family (fontmap, pango_font_family_get_name (fam));
      g_assert_true (family == fam);
      g_object_unref (fam);
    }

  family = g_list_model_get_item (G_LIST_MODEL (fontmap), 0);
  g_object_unref (family);

  g_assert_cmpint (g_list_model_get_n_items (G_LIST_MODEL (family)), >, 0);
  for (i = 0; i < g_list_model_get_n_items (G_LIST_MODEL (family)); i++)
    {
      PangoFontFace *f = g_list_model_get_item (G_LIST_MODEL (family), i);
      face = pango_font_family_get_face (family, pango_font_face_get_name (f));
      g_assert_cmpstr (pango_font_face_get_name (face), ==, pango_font_face_get_name (f));
      g_object_unref (f);
    }

  desc = pango_font_description_new ();
  pango_font_description_set_family (desc, pango_font_family_get_name (family));
  pango_font_description_set_size (desc, 10*PANGO_SCALE);

  font = pango_font_map_load_font (fontmap, context, desc);
  face = pango_font_get_face (font);
  found_face = FALSE;
  for (i = 0; i < g_list_model_get_n_items (G_LIST_MODEL (family)); i++)
    {
      PangoFontFace *f = g_list_model_get_item (G_LIST_MODEL (family), i);
      g_object_unref (f);
      if (face == f)
        {
          found_face = TRUE;
          break;
        }
    }
  g_assert_true (found_face);

  g_object_unref (font);
  pango_font_description_free (desc);
  g_object_unref (context);
}

static void
test_roundtrip_plain (void)
{
  PangoContext *context;
  PangoFontDescription *desc, *desc2, *desc3;
  PangoFont *font, *font2;

#ifdef HAVE_CARBON
  desc = pango_font_description_from_string ("Helvetica 11");
#else
  desc = pango_font_description_from_string ("Cantarell 11");
#endif

  context = pango_context_new ();

  font = pango_context_load_font (context, desc);
  desc2 = pango_font_describe (font);

  font2 = pango_context_load_font (context, desc2);
  desc3 = pango_font_describe (font2);

  g_assert_true (pango_font_description_equal (desc2, desc3));
  //g_assert_true (font == font2);

  pango_font_description_free (desc2);
  g_object_unref (font2);
  pango_font_description_free (desc3);
  g_object_unref (font);
  pango_font_description_free (desc);
  g_object_unref (context);
}

static void
test_roundtrip_small_caps (void)
{
  PangoContext *context;
  PangoFontDescription *desc, *desc2;
  PangoFont *font;
  hb_feature_t features[32];
  guint num = 0;

  context = pango_context_new ();

  desc = pango_font_description_from_string ("Cantarell Small-Caps 11");
  g_assert_true (pango_font_description_get_variant (desc) == PANGO_VARIANT_SMALL_CAPS);

  font = pango_context_load_font (context, desc);
  desc2 = pango_font_describe (font);

  num = 0;
  pango_font_get_features (font, features, G_N_ELEMENTS (features), &num);
  g_assert_true (num == 1);
  g_assert_true (features[0].tag == HB_TAG ('s', 'm', 'c', 'p'));
  g_assert_true (features[0].value == 1);
  g_assert_true (pango_font_description_get_variant (desc2) == PANGO_VARIANT_SMALL_CAPS);
  /* We need to unset faceid since desc doesn't have one */
  pango_font_description_unset_fields (desc2, PANGO_FONT_MASK_FACEID);
  g_assert_true (pango_font_description_equal (desc2, desc));

  pango_font_description_free (desc2);
  g_object_unref (font);
  pango_font_description_free (desc);

  desc = pango_font_description_from_string ("Cantarell All-Small-Caps 11");
  g_assert_true (pango_font_description_get_variant (desc) == PANGO_VARIANT_ALL_SMALL_CAPS);

  font = pango_context_load_font (context, desc);
  desc2 = pango_font_describe (font);

  num = 0;
  pango_font_get_features (font, features, G_N_ELEMENTS (features), &num);
  g_assert_true (num == 2);
  g_assert_true (features[0].tag == HB_TAG ('s', 'm', 'c', 'p'));
  g_assert_true (features[0].value == 1);
  g_assert_true (features[1].tag == HB_TAG ('c', '2', 's', 'c'));
  g_assert_true (features[1].value == 1);
  g_assert_true (pango_font_description_get_variant (desc2) == PANGO_VARIANT_ALL_SMALL_CAPS);
  pango_font_description_unset_fields (desc2, PANGO_FONT_MASK_FACEID);
  g_assert_true (pango_font_description_equal (desc2, desc));

  pango_font_description_free (desc2);
  g_object_unref (font);
  pango_font_description_free (desc);

  desc = pango_font_description_from_string ("Cantarell Unicase 11");
  g_assert_true (pango_font_description_get_variant (desc) == PANGO_VARIANT_UNICASE);

  font = pango_context_load_font (context, desc);
  desc2 = pango_font_describe (font);

  num = 0;
  pango_font_get_features (font, features, G_N_ELEMENTS (features), &num);
  g_assert_true (num == 1);
  g_assert_true (features[0].tag == HB_TAG ('u', 'n', 'i', 'c'));
  g_assert_true (features[0].value == 1);
  g_assert_true (pango_font_description_get_variant (desc2) == PANGO_VARIANT_UNICASE);
  pango_font_description_unset_fields (desc2, PANGO_FONT_MASK_FACEID);
  g_assert_true (pango_font_description_equal (desc2, desc));

  pango_font_description_free (desc2);
  g_object_unref (font);
  pango_font_description_free (desc);

  g_object_unref (context);
}

static void
test_roundtrip_emoji (void)
{
  PangoContext *context;
  PangoFontDescription *desc, *desc2;
  PangoFont *font;

  context = pango_context_new ();

  /* This is how pango_itemize creates the emoji font desc */
  desc = pango_font_description_from_string ("DejaVu Sans Book 11");
  pango_font_description_set_family_static (desc, "emoji");

  font = pango_context_load_font (context, desc);
  desc2 = pango_font_describe (font);

  /* We can't expect the family name to match, since we go in with
   * a generic family
   * And we need to unset faceid, since desc doesn't have one.
   */
  pango_font_description_unset_fields (desc, PANGO_FONT_MASK_FAMILY);
  pango_font_description_unset_fields (desc2, PANGO_FONT_MASK_FAMILY|PANGO_FONT_MASK_FACEID);
  g_assert_true (pango_font_description_equal (desc2, desc));

  pango_font_description_free (desc2);
  g_object_unref (font);
  pango_font_description_free (desc);
  g_object_unref (context);
}

static void
test_fontmap_models (void)
{
  PangoFontMap *map = pango_font_map_get_default ();
  int n_families = 0;

  g_assert_true (g_list_model_get_item_type (G_LIST_MODEL (map)) == PANGO_TYPE_FONT_FAMILY);

  for (guint i = 0; i < g_list_model_get_n_items (G_LIST_MODEL (map)); i++)
    {
      GObject *obj = g_list_model_get_item (G_LIST_MODEL (map), i);

      g_assert_true (PANGO_IS_FONT_FAMILY (obj));

      g_assert_true (g_list_model_get_item_type (G_LIST_MODEL (obj)) == PANGO_TYPE_FONT_FACE);

      n_families++;

      for (guint j = 0; j < g_list_model_get_n_items (G_LIST_MODEL (obj)); j++)
        {
          GObject *obj2 = g_list_model_get_item (G_LIST_MODEL (obj), j);

          g_assert_true (PANGO_IS_FONT_FACE (obj2));

          g_object_unref (obj2);
        }

      g_object_unref (obj);
    }

  g_print ("# %d font families\n", n_families);
}

static void
test_glyph_extents (void)
{
  PangoRectangle ink, logical;

  pango_font_get_glyph_extents (NULL, 0, &ink, &logical);

  /* We are promised 'sane values', so lets check that
   * we are between 1 and 100 pixels in both dimensions.
   */
  g_assert_cmpint (ink.height, >=, PANGO_SCALE);
  g_assert_cmpint (ink.height, <=, 100 * PANGO_SCALE);
  g_assert_cmpint (ink.width, >=, PANGO_SCALE);
  g_assert_cmpint (ink.width, <=, 100 * PANGO_SCALE);
  g_assert_cmpint (logical.height, >=, PANGO_SCALE);
  g_assert_cmpint (logical.height, <=, 100 * PANGO_SCALE);
  g_assert_cmpint (logical.width, >=, PANGO_SCALE);
  g_assert_cmpint (logical.width, <=, 100 * PANGO_SCALE);
}

static void
test_font_metrics (void)
{
  PangoFontMetrics *metrics;

  metrics = pango_font_get_metrics (NULL, NULL);

  g_assert_cmpint (pango_font_metrics_get_approximate_char_width (metrics), ==, PANGO_SCALE * PANGO_UNKNOWN_GLYPH_WIDTH);
  g_assert_cmpint (pango_font_metrics_get_approximate_digit_width (metrics), ==, PANGO_SCALE * PANGO_UNKNOWN_GLYPH_WIDTH);

  pango_font_metrics_free (metrics);
}

static void
test_set_gravity (void)
{
  PangoFontDescription *desc;

  desc = pango_font_description_from_string ("Futura Medium Italic 14");
  pango_font_description_set_gravity (desc, PANGO_GRAVITY_SOUTH);
  g_assert_true (pango_font_description_get_set_fields (desc) & PANGO_FONT_MASK_GRAVITY);

  pango_font_description_set_gravity (desc, PANGO_GRAVITY_AUTO);
  g_assert_false (pango_font_description_get_set_fields (desc) & PANGO_FONT_MASK_GRAVITY);

  pango_font_description_free (desc);
}

static void
test_faceid (void)
{
  const char *test = "Cantarell Bold Italic 32 @faceid=Cantarell-Regular:0:-1:0,wght=600";
  PangoFontDescription *desc;
  char *s;

  desc = pango_font_description_from_string (test);
  g_assert_cmpint (pango_font_description_get_set_fields (desc), ==, PANGO_FONT_MASK_FAMILY|
                                                                     PANGO_FONT_MASK_STYLE|
                                                                     PANGO_FONT_MASK_WEIGHT|
                                                                     PANGO_FONT_MASK_VARIANT|
                                                                     PANGO_FONT_MASK_STRETCH|
                                                                     PANGO_FONT_MASK_SIZE|
                                                                     PANGO_FONT_MASK_FACEID|
                                                                     PANGO_FONT_MASK_VARIATIONS);
  g_assert_cmpstr (pango_font_description_get_family (desc), ==, "Cantarell");
  g_assert_cmpint (pango_font_description_get_size (desc), ==, 32 * PANGO_SCALE);
  g_assert_cmpint (pango_font_description_get_style (desc), ==, PANGO_STYLE_ITALIC);
  g_assert_cmpint (pango_font_description_get_variant (desc), ==, PANGO_VARIANT_NORMAL);
  g_assert_cmpint (pango_font_description_get_weight (desc), ==, PANGO_WEIGHT_BOLD);
  g_assert_cmpint (pango_font_description_get_stretch (desc), ==, PANGO_STRETCH_NORMAL);
  g_assert_cmpstr (pango_font_description_get_faceid (desc), ==, "Cantarell-Regular:0:-1:0");
  g_assert_cmpstr (pango_font_description_get_variations (desc), ==, "wght=600");

  s = pango_font_description_to_string (desc);
  g_assert_cmpstr (s, ==, test);
  g_free (s);

  pango_font_description_free (desc);
}

static gboolean
font_info_cb (PangoUserFace     *face,
              int                size,
              hb_font_extents_t *extents,
              gpointer           user_data)
{
  extents->ascender = 0.75 * size;
  extents->descender = - 0.25 * size;
  extents->line_gap = 0;

  return TRUE;
}

static gboolean
glyph_cb (PangoUserFace  *face,
          hb_codepoint_t  unicode,
          hb_codepoint_t *glyph,
          gpointer        user_data)
{
  if (unicode == ' ')
    {
      *glyph = 0x20;
      return TRUE;
    }

  return FALSE;
}

static gboolean
glyph_info_cb (PangoUserFace      *face,
               int                 size,
               hb_codepoint_t      glyph,
               hb_glyph_extents_t *extents,
               hb_position_t      *h_advance,
               hb_position_t      *v_advance,
               gboolean           *is_color,
               gpointer            user_data)
{
  return FALSE;
}

static gboolean
count_fonts (PangoFontset *fontset,
             PangoFont    *font,
             gpointer      user_data)
{
  int *count = user_data;

  (*count)++;

  return FALSE;
}

/* test that fontmap fallback works as expected */
static void
test_fontmap_fallback (void)
{
  PangoFontMap *map;
  PangoUserFace *custom;
  PangoFontDescription *desc;
  PangoFontset *fontset;
  PangoFont *font;
  int count;

  map = pango_font_map_new ();

  desc = pango_font_description_from_string ("CustomFont");
  custom = pango_user_face_new (font_info_cb,
                                glyph_cb,
                                glyph_info_cb,
                                NULL,
                                NULL,
                                NULL, NULL,
                                "Regular", desc);
  pango_font_description_free (desc);

  pango_font_map_add_face (map, PANGO_FONT_FACE (custom));

  desc = pango_font_description_from_string ("CustomFont 11");
  fontset = pango_font_map_load_fontset (map, context, desc, NULL);
  g_assert_nonnull (fontset);

  font = pango_fontset_get_font (fontset, 0x20);
  g_assert_nonnull (font);
  g_assert_true (pango_font_get_face (font) == PANGO_FONT_FACE (custom));
  g_object_unref (font);

  count = 0;
  pango_fontset_foreach (fontset, count_fonts, &count);
  g_assert_true (count == 1);

  pango_font_map_set_fallback (map, pango_font_map_get_default ());

  desc = pango_font_description_from_string ("CustomFont 11");
  fontset = pango_font_map_load_fontset (map, context, desc, NULL);
  g_assert_nonnull (fontset);

  font = pango_fontset_get_font (fontset, ' ');
  g_assert_nonnull (font);
  g_assert_true (pango_font_get_face (font) == PANGO_FONT_FACE (custom));
  g_object_unref (font);

  font = pango_fontset_get_font (fontset, 'a');
  g_assert_nonnull (font);

  count = 0;
  pango_fontset_foreach (fontset, count_fonts, &count);
  g_assert_true (count > 1);

  g_object_unref (map);
}

int
main (int argc, char *argv[])
{
  setlocale (LC_ALL, "");

  g_test_init (&argc, &argv, NULL);

  install_fonts ();

  context = pango_context_new ();

  g_test_add_func ("/pango/fontdescription/parse", test_parse);
  g_test_add_func ("/pango/fontdescription/roundtrip", test_roundtrip);
  g_test_add_func ("/pango/fontdescription/variations", test_variations);
  g_test_add_func ("/pango/fontdescription/empty-variations", test_empty_variations);
  g_test_add_func ("/pango/fontdescription/set-gravity", test_set_gravity);
  g_test_add_func ("/pango/fontdescription/faceid", test_faceid);
  g_test_add_func ("/pango/font/metrics", test_metrics);
  g_test_add_func ("/pango/font/extents", test_extents);
  g_test_add_func ("/pango/font/glyph-extents", test_glyph_extents);
  g_test_add_func ("/pango/font/font-metrics", test_font_metrics);
  g_test_add_func ("/pango/font/roundtrip/plain", test_roundtrip_plain);
  g_test_add_func ("/pango/font/roundtrip/small-caps", test_roundtrip_small_caps);
  g_test_add_func ("/pango/font/roundtrip/emoji", test_roundtrip_emoji);
  g_test_add_func ("/pango/fontmap/enumerate", test_fontmap_enumerate);
  g_test_add_func ("/pango/fontmap/models", test_fontmap_models);
  g_test_add_func ("/pango/fontmap/fallback", test_fontmap_fallback);

  return g_test_run ();
}
