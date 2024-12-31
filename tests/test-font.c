/* Pango2
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
#include <pango2/pango.h>
#include <pango2/pango-item-private.h>
#include <pango2/pango-font-private.h>
#include <pango2/pangocairo.h>

#include "test-common.h"

static void
test_parse (void)
{
  Pango2FontDescription **descs;
  Pango2FontDescription *desc;

  descs = g_new (Pango2FontDescription *, 2);

  descs[0] = desc = pango2_font_description_from_string ("Cantarell 14");

  g_assert_cmpstr (pango2_font_description_get_family (desc), ==, "Cantarell");
  g_assert_false (pango2_font_description_get_size_is_absolute (desc));
  g_assert_cmpint (pango2_font_description_get_size (desc), ==, 14 * PANGO2_SCALE);
  g_assert_cmpint (pango2_font_description_get_style (desc), ==, PANGO2_STYLE_NORMAL);
  g_assert_cmpint (pango2_font_description_get_variant (desc), ==, PANGO2_VARIANT_NORMAL);
  g_assert_cmpint (pango2_font_description_get_weight (desc), ==, PANGO2_WEIGHT_NORMAL);
  g_assert_cmpint (pango2_font_description_get_stretch (desc), ==, PANGO2_STRETCH_NORMAL);
  g_assert_cmpint (pango2_font_description_get_gravity (desc), ==, PANGO2_GRAVITY_SOUTH);
  g_assert_cmpint (pango2_font_description_get_set_fields (desc), ==, PANGO2_FONT_MASK_FAMILY | PANGO2_FONT_MASK_STYLE | PANGO2_FONT_MASK_VARIANT | PANGO2_FONT_MASK_WEIGHT | PANGO2_FONT_MASK_STRETCH | PANGO2_FONT_MASK_SIZE);

  descs[1] = desc = pango2_font_description_from_string ("Sans Bold Italic Condensed 22.5px");

  g_assert_cmpstr (pango2_font_description_get_family (desc), ==, "Sans");
  g_assert_true (pango2_font_description_get_size_is_absolute (desc));
  g_assert_cmpint (pango2_font_description_get_size (desc), ==, 225 * PANGO2_SCALE / 10);
  g_assert_cmpint (pango2_font_description_get_style (desc), ==, PANGO2_STYLE_ITALIC);
  g_assert_cmpint (pango2_font_description_get_variant (desc), ==, PANGO2_VARIANT_NORMAL); 
  g_assert_cmpint (pango2_font_description_get_weight (desc), ==, PANGO2_WEIGHT_BOLD);
  g_assert_cmpint (pango2_font_description_get_stretch (desc), ==, PANGO2_STRETCH_CONDENSED); 
  g_assert_cmpint (pango2_font_description_get_gravity (desc), ==, PANGO2_GRAVITY_SOUTH);  g_assert_cmpint (pango2_font_description_get_set_fields (desc), ==, PANGO2_FONT_MASK_FAMILY | PANGO2_FONT_MASK_STYLE | PANGO2_FONT_MASK_VARIANT | PANGO2_FONT_MASK_WEIGHT | PANGO2_FONT_MASK_STRETCH | PANGO2_FONT_MASK_SIZE);

  pango2_font_description_free (descs[0]);
  pango2_font_description_free (descs[1]);
}

static void
test_roundtrip (void)
{
  Pango2FontDescription *desc;
  char *str;

  desc = pango2_font_description_from_string ("Cantarell 14");
  str = pango2_font_description_to_string (desc);
  g_assert_cmpstr (str, ==, "Cantarell 14");
  pango2_font_description_free (desc);
  g_free (str);

  desc = pango2_font_description_from_string ("Sans Bold Italic Condensed 22.5px");
  str = pango2_font_description_to_string (desc);
  g_assert_cmpstr (str, ==, "Sans Bold Italic Condensed 22.5px");
  pango2_font_description_free (desc);
  g_free (str);
}

static void
test_variations (void)
{
  Pango2FontDescription *desc1;
  Pango2FontDescription *desc2;
  char *str;

  desc1 = pango2_font_description_from_string ("Cantarell 14");
  g_assert_nonnull (desc1);
  g_assert_cmpint ((pango2_font_description_get_set_fields (desc1) & PANGO2_FONT_MASK_VARIATIONS), ==, 0);
  g_assert_cmpstr (pango2_font_description_get_family (desc1), ==, "Cantarell");
  g_assert_cmpint (pango2_font_description_get_size (desc1), ==, 14 * PANGO2_SCALE);
  g_assert_null (pango2_font_description_get_variations (desc1));

  str = pango2_font_description_to_string (desc1);
  g_assert_cmpstr (str, ==, "Cantarell 14");
  g_free (str);

  desc2 = pango2_font_description_from_string ("Cantarell 14 @wght=100,wdth=235");
  g_assert_nonnull (desc2);
  g_assert_cmpint ((pango2_font_description_get_set_fields (desc2) & PANGO2_FONT_MASK_VARIATIONS), ==, PANGO2_FONT_MASK_VARIATIONS);
  g_assert_cmpstr (pango2_font_description_get_family (desc2), ==, "Cantarell");
  g_assert_cmpint (pango2_font_description_get_size (desc2), ==, 14 * PANGO2_SCALE);
  g_assert_cmpstr (pango2_font_description_get_variations (desc2), ==, "wght=100,wdth=235");

  str = pango2_font_description_to_string (desc2);
  g_assert_cmpstr (str, ==, "Cantarell 14 @wght=100,wdth=235");
  g_free (str);

  g_assert_false (pango2_font_description_equal (desc1, desc2));

  pango2_font_description_set_variations (desc1, "wght=100,wdth=235");
  g_assert_cmpint ((pango2_font_description_get_set_fields (desc1) & PANGO2_FONT_MASK_VARIATIONS), ==, PANGO2_FONT_MASK_VARIATIONS);
  g_assert_cmpstr (pango2_font_description_get_variations (desc1), ==, "wght=100,wdth=235");

  g_assert_true (pango2_font_description_equal (desc1, desc2));

  pango2_font_description_free (desc1);
  pango2_font_description_free (desc2);
}

static void
test_empty_variations (void)
{
  Pango2FontDescription *desc;
  char *str;

  desc = pango2_font_description_from_string ("Cantarell 14");
  g_assert_nonnull (desc);
  g_assert_cmpint ((pango2_font_description_get_set_fields (desc) & PANGO2_FONT_MASK_VARIATIONS), ==, 0);
  g_assert_null (pango2_font_description_get_variations (desc));

  str = pango2_font_description_to_string (desc);
  g_assert_cmpstr (str, ==, "Cantarell 14");
  g_free (str);

  pango2_font_description_set_variations (desc, "");
  g_assert_cmpint ((pango2_font_description_get_set_fields (desc) & PANGO2_FONT_MASK_VARIATIONS), ==, PANGO2_FONT_MASK_VARIATIONS);
  g_assert_cmpstr (pango2_font_description_get_variations (desc), ==, "");

  str = pango2_font_description_to_string (desc);
  g_assert_cmpstr (str, ==, "Cantarell 14");
  g_free (str);

  pango2_font_description_free (desc);
}

static void
test_metrics (void)
{
  Pango2FontMap *fontmap;
  Pango2Context *context;
  Pango2FontDescription *desc;
  Pango2FontMetrics *metrics;
  char *str;

  fontmap = pango2_font_map_new_default ();
  context = pango2_context_new_with_font_map (fontmap);

  desc = pango2_font_description_from_string ("Cantarell 11");

  str = pango2_font_description_to_string (desc);

  metrics = pango2_context_get_metrics (context, desc, pango2_language_get_default ());

  g_test_message ("%s metrics", str);
  g_test_message ("\tascent: %d", pango2_font_metrics_get_ascent (metrics));
  g_test_message ("\tdescent: %d", pango2_font_metrics_get_descent (metrics));
  g_test_message ("\theight: %d", pango2_font_metrics_get_height (metrics));
  g_test_message ("\tchar width: %d",
                  pango2_font_metrics_get_approximate_char_width (metrics));
  g_test_message ("\tdigit width: %d",
                  pango2_font_metrics_get_approximate_digit_width (metrics));
  g_test_message ("\tunderline position: %d",
                  pango2_font_metrics_get_underline_position (metrics));
  g_test_message ("\tunderline thickness: %d",
                  pango2_font_metrics_get_underline_thickness (metrics));
  g_test_message ("\tstrikethrough position: %d",
                  pango2_font_metrics_get_strikethrough_position (metrics));
  g_test_message ("\tstrikethrough thickness: %d",
                  pango2_font_metrics_get_strikethrough_thickness (metrics));

  pango2_font_metrics_free (metrics);
  g_free (str);
  pango2_font_description_free (desc);

  g_object_unref (context);
  g_object_unref (fontmap);
}

static void
test_extents (void)
{
  const char *str = "Composer";
  GList *items;
  Pango2Item *item;
  Pango2GlyphString *glyphs;
  Pango2Rectangle ink, log;
  Pango2Context *context;
  Pango2FontDescription *desc;
  Pango2Direction dir;

  context = pango2_context_new ();
  desc = pango2_font_description_from_string("Cantarell 11");
  pango2_context_set_font_description (context, desc);
  pango2_font_description_free (desc);

  dir = pango2_context_get_base_dir (context);
  items = pango2_itemize (context, dir, str, 0, strlen (str), NULL);
  glyphs = pango2_glyph_string_new ();
  item = items->data;
  pango2_shape (str, strlen (str), NULL, 0, &item->analysis, glyphs, PANGO2_SHAPE_NONE);
  pango2_glyph_string_extents (glyphs, item->analysis.font, &ink, &log);

  g_assert_cmpint (ink.width, >=, 0);
  g_assert_cmpint (ink.height, >=, 0);
  g_assert_cmpint (log.width, >=, 0);
  g_assert_cmpint (log.height, >=, 0);

  pango2_glyph_string_free (glyphs);
  g_list_free_full (items, (GDestroyNotify)pango2_item_free);
  g_object_unref (context);
}

static void
test_fontmap_enumerate (void)
{
  Pango2FontMap *fontmap;
  Pango2Context *context;
  Pango2FontFamily *family;
  int i;
  Pango2FontFace *face;
  Pango2FontDescription *desc;
  Pango2Font *font;
  gboolean found_face;

  fontmap = pango2_font_map_new_default ();
  context = pango2_context_new_with_font_map (fontmap);

  g_assert_cmpint (g_list_model_get_n_items (G_LIST_MODEL (fontmap)), >, 0);

  for (i = 0; i < g_list_model_get_n_items (G_LIST_MODEL (fontmap)); i++)
    {
      Pango2FontFamily *fam = g_list_model_get_item (G_LIST_MODEL (fontmap), i);
      family = pango2_font_map_get_family (fontmap, pango2_font_family_get_name (fam));
      g_assert_true (family == fam);
      g_object_unref (fam);
    }

  family = g_list_model_get_item (G_LIST_MODEL (fontmap), 0);
  g_object_unref (family);

  g_assert_cmpint (g_list_model_get_n_items (G_LIST_MODEL (family)), >, 0);
  for (i = 0; i < g_list_model_get_n_items (G_LIST_MODEL (family)); i++)
    {
      Pango2FontFace *f = g_list_model_get_item (G_LIST_MODEL (family), i);
      face = pango2_font_family_get_face (family, pango2_font_face_get_name (f));
      g_assert_cmpstr (pango2_font_face_get_name (face), ==, pango2_font_face_get_name (f));
      g_object_unref (f);
    }

  desc = pango2_font_description_new ();
  pango2_font_description_set_family (desc, pango2_font_family_get_name (family));
  pango2_font_description_set_size (desc, 10*PANGO2_SCALE);

  font = pango2_font_map_load_font (fontmap, context, desc);
  face = pango2_font_get_face (font);
  found_face = FALSE;
  for (i = 0; i < g_list_model_get_n_items (G_LIST_MODEL (family)); i++)
    {
      Pango2FontFace *f = g_list_model_get_item (G_LIST_MODEL (family), i);
      g_object_unref (f);
      if (face == f)
        {
          found_face = TRUE;
          break;
        }
    }
  g_assert_true (found_face);

  g_object_unref (font);
  pango2_font_description_free (desc);
  g_object_unref (context);
  g_object_unref (fontmap);
}

static void
test_roundtrip_plain (void)
{
  Pango2Context *context;
  Pango2FontDescription *desc, *desc2, *desc3;
  Pango2Font *font, *font2;

  desc = pango2_font_description_from_string ("Cantarell 11");

  context = pango2_context_new ();

  font = pango2_context_load_font (context, desc);
  desc2 = pango2_font_describe (font);

  font2 = pango2_context_load_font (context, desc2);
  desc3 = pango2_font_describe (font2);

  g_assert_true (pango2_font_description_equal (desc2, desc3));
  //g_assert_true (font == font2);

  pango2_font_description_free (desc2);
  g_object_unref (font2);
  pango2_font_description_free (desc3);
  g_object_unref (font);
  pango2_font_description_free (desc);
  g_object_unref (context);
}

static void
test_roundtrip_absolute (void)
{
  Pango2FontMap *fontmap;
  Pango2Context *context;
  Pango2FontDescription *desc, *desc2;
  Pango2Font *font;

  desc = pango2_font_description_from_string ("Cantarell 11");

  pango2_font_description_set_absolute_size (desc, 11 * 1024.0);

  fontmap = get_font_map_with_cantarell ();
  context = pango2_context_new_with_font_map (fontmap);

  font = pango2_context_load_font (context, desc);
  desc2 = pango2_font_describe_with_absolute_size (font);

  pango2_font_description_set_faceid_static (desc, pango2_font_description_get_faceid (desc2));

  g_assert_true (pango2_font_description_equal (desc2, desc));

  pango2_font_description_free (desc2);
  g_object_unref (font);
  pango2_font_description_free (desc);
  g_object_unref (context);
  g_object_unref (fontmap);
}

static void
test_roundtrip_small_caps (void)
{
  Pango2Context *context;
  Pango2FontDescription *desc, *desc2;
  Pango2Font *font;
  const hb_feature_t *features;
  guint num = 0;

  context = pango2_context_new ();

  desc = pango2_font_description_from_string ("Cantarell Small-Caps 11");
  g_assert_true (pango2_font_description_get_variant (desc) == PANGO2_VARIANT_SMALL_CAPS);

  font = pango2_context_load_font (context, desc);
  desc2 = pango2_font_describe (font);

  num = 0;
  features = pango2_hb_font_get_features (PANGO2_HB_FONT (font), &num);
  g_assert_true (num == 1);
  g_assert_true (features[0].tag == HB_TAG ('s', 'm', 'c', 'p'));
  g_assert_true (features[0].value == 1);
  g_assert_true (pango2_font_description_get_variant (desc2) == PANGO2_VARIANT_SMALL_CAPS);
  /* We need to unset faceid since desc doesn't have one */
  pango2_font_description_unset_fields (desc2, PANGO2_FONT_MASK_FACEID);
  g_assert_true (pango2_font_description_equal (desc2, desc));

  pango2_font_description_free (desc2);
  g_object_unref (font);
  pango2_font_description_free (desc);

  desc = pango2_font_description_from_string ("Cantarell All-Small-Caps 11");
  g_assert_true (pango2_font_description_get_variant (desc) == PANGO2_VARIANT_ALL_SMALL_CAPS);

  font = pango2_context_load_font (context, desc);
  desc2 = pango2_font_describe (font);

  num = 0;
  features = pango2_hb_font_get_features (PANGO2_HB_FONT (font), &num);
  g_assert_true (num == 2);
  g_assert_true (features[0].tag == HB_TAG ('s', 'm', 'c', 'p'));
  g_assert_true (features[0].value == 1);
  g_assert_true (features[1].tag == HB_TAG ('c', '2', 's', 'c'));
  g_assert_true (features[1].value == 1);
  g_assert_true (pango2_font_description_get_variant (desc2) == PANGO2_VARIANT_ALL_SMALL_CAPS);
  pango2_font_description_unset_fields (desc2, PANGO2_FONT_MASK_FACEID);
  g_assert_true (pango2_font_description_equal (desc2, desc));

  pango2_font_description_free (desc2);
  g_object_unref (font);
  pango2_font_description_free (desc);

  desc = pango2_font_description_from_string ("Cantarell Unicase 11");
  g_assert_true (pango2_font_description_get_variant (desc) == PANGO2_VARIANT_UNICASE);

  font = pango2_context_load_font (context, desc);
  desc2 = pango2_font_describe (font);

  num = 0;
  features = pango2_hb_font_get_features (PANGO2_HB_FONT (font), &num);
  g_assert_true (num == 1);
  g_assert_true (features[0].tag == HB_TAG ('u', 'n', 'i', 'c'));
  g_assert_true (features[0].value == 1);
  g_assert_true (pango2_font_description_get_variant (desc2) == PANGO2_VARIANT_UNICASE);
  pango2_font_description_unset_fields (desc2, PANGO2_FONT_MASK_FACEID);
  g_assert_true (pango2_font_description_equal (desc2, desc));

  pango2_font_description_free (desc2);
  g_object_unref (font);
  pango2_font_description_free (desc);

  g_object_unref (context);
}

static void
test_roundtrip_emoji (void)
{
  Pango2Context *context;
  Pango2FontDescription *desc, *desc2;
  Pango2Font *font;

  context = pango2_context_new ();

  /* This is how pango2_itemize creates the emoji font desc */
  desc = pango2_font_description_from_string ("DejaVu Sans Book 11");
  pango2_font_description_set_family_static (desc, "emoji");

  font = pango2_context_load_font (context, desc);
  desc2 = pango2_font_describe (font);

  /* We can't expect the family name to match, since we go in with
   * a generic family
   * And we need to unset faceid, since desc doesn't have one.
   */
  pango2_font_description_unset_fields (desc, PANGO2_FONT_MASK_FAMILY);
  pango2_font_description_unset_fields (desc2, PANGO2_FONT_MASK_FAMILY|PANGO2_FONT_MASK_FACEID);
  g_assert_true (pango2_font_description_equal (desc2, desc));

  pango2_font_description_free (desc2);
  g_object_unref (font);
  pango2_font_description_free (desc);
  g_object_unref (context);
}

static void
test_fontmap_models (void)
{
  Pango2FontMap *map = pango2_font_map_new_default ();
  int n_families = 0;

  g_assert_true (g_list_model_get_item_type (G_LIST_MODEL (map)) == PANGO2_TYPE_FONT_FAMILY);

  for (guint i = 0; i < g_list_model_get_n_items (G_LIST_MODEL (map)); i++)
    {
      GObject *obj = g_list_model_get_item (G_LIST_MODEL (map), i);

      g_assert_true (PANGO2_IS_FONT_FAMILY (obj));

      g_assert_true (g_list_model_get_item_type (G_LIST_MODEL (obj)) == PANGO2_TYPE_FONT_FACE);

      n_families++;

      for (guint j = 0; j < g_list_model_get_n_items (G_LIST_MODEL (obj)); j++)
        {
          GObject *obj2 = g_list_model_get_item (G_LIST_MODEL (obj), j);

          g_assert_true (PANGO2_IS_FONT_FACE (obj2));

          g_object_unref (obj2);
        }

      g_object_unref (obj);
    }

  g_print ("# %d font families\n", n_families);
  g_object_unref (map);
}

static void
test_glyph_extents (void)
{
  Pango2Rectangle ink, logical;

  pango2_font_get_glyph_extents (NULL, 0, &ink, &logical);

  /* We are promised 'sane values', so lets check that
   * we are between 1 and 100 pixels in both dimensions.
   */
  g_assert_cmpint (ink.height, >=, PANGO2_SCALE);
  g_assert_cmpint (ink.height, <=, 100 * PANGO2_SCALE);
  g_assert_cmpint (ink.width, >=, PANGO2_SCALE);
  g_assert_cmpint (ink.width, <=, 100 * PANGO2_SCALE);
  g_assert_cmpint (logical.height, >=, PANGO2_SCALE);
  g_assert_cmpint (logical.height, <=, 100 * PANGO2_SCALE);
  g_assert_cmpint (logical.width, >=, PANGO2_SCALE);
  g_assert_cmpint (logical.width, <=, 100 * PANGO2_SCALE);
}

static void
test_font_metrics (void)
{
  Pango2FontMetrics *metrics;

  metrics = pango2_font_get_metrics (NULL, NULL);

  g_assert_cmpint (pango2_font_metrics_get_approximate_char_width (metrics), ==, PANGO2_SCALE * PANGO2_UNKNOWN_GLYPH_WIDTH);
  g_assert_cmpint (pango2_font_metrics_get_approximate_digit_width (metrics), ==, PANGO2_SCALE * PANGO2_UNKNOWN_GLYPH_WIDTH);

  pango2_font_metrics_free (metrics);
}

static void
test_set_gravity (void)
{
  Pango2FontDescription *desc;

  desc = pango2_font_description_from_string ("Futura Medium Italic 14");
  pango2_font_description_set_gravity (desc, PANGO2_GRAVITY_SOUTH);
  g_assert_true (pango2_font_description_get_set_fields (desc) & PANGO2_FONT_MASK_GRAVITY);

  pango2_font_description_set_gravity (desc, PANGO2_GRAVITY_AUTO);
  g_assert_false (pango2_font_description_get_set_fields (desc) & PANGO2_FONT_MASK_GRAVITY);

  pango2_font_description_free (desc);
}

static void
test_faceid (void)
{
  const char *test = "Cantarell Bold Italic 32 @faceid=Cantarell-Regular:0:-1:0 @wght=600";
  Pango2FontDescription *desc;
  char *s;

  desc = pango2_font_description_from_string (test);
  g_assert_cmpint (pango2_font_description_get_set_fields (desc), ==, PANGO2_FONT_MASK_FAMILY|
                                                                     PANGO2_FONT_MASK_STYLE|
                                                                     PANGO2_FONT_MASK_WEIGHT|
                                                                     PANGO2_FONT_MASK_VARIANT|
                                                                     PANGO2_FONT_MASK_STRETCH|
                                                                     PANGO2_FONT_MASK_SIZE|
                                                                     PANGO2_FONT_MASK_FACEID|
                                                                     PANGO2_FONT_MASK_VARIATIONS);
  g_assert_cmpstr (pango2_font_description_get_family (desc), ==, "Cantarell");
  g_assert_cmpint (pango2_font_description_get_size (desc), ==, 32 * PANGO2_SCALE);
  g_assert_cmpint (pango2_font_description_get_style (desc), ==, PANGO2_STYLE_ITALIC);
  g_assert_cmpint (pango2_font_description_get_variant (desc), ==, PANGO2_VARIANT_NORMAL);
  g_assert_cmpint (pango2_font_description_get_weight (desc), ==, PANGO2_WEIGHT_BOLD);
  g_assert_cmpint (pango2_font_description_get_stretch (desc), ==, PANGO2_STRETCH_NORMAL);
  g_assert_cmpstr (pango2_font_description_get_faceid (desc), ==, "Cantarell-Regular:0:-1:0");
  g_assert_cmpstr (pango2_font_description_get_variations (desc), ==, "wght=600");

  s = pango2_font_description_to_string (desc);
  g_assert_cmpstr (s, ==, test);
  g_free (s);

  pango2_font_description_free (desc);
}

static void
test_all (void)
{
  const char *test = "Cantarell Bold Italic 32 @faceid=Cantarell-Regular:0:-1:0 @wght=600";
  Pango2FontDescription *desc;
  char *s;

  desc = pango2_font_description_from_string (test);
  g_assert_cmpint (pango2_font_description_get_set_fields (desc), ==, PANGO2_FONT_MASK_FAMILY|
                                                                     PANGO2_FONT_MASK_STYLE|
                                                                     PANGO2_FONT_MASK_WEIGHT|
                                                                     PANGO2_FONT_MASK_VARIANT|
                                                                     PANGO2_FONT_MASK_STRETCH|
                                                                     PANGO2_FONT_MASK_SIZE|
                                                                     PANGO2_FONT_MASK_FACEID|
                                                                     PANGO2_FONT_MASK_VARIATIONS);
  g_assert_cmpstr (pango2_font_description_get_family (desc), ==, "Cantarell");
  g_assert_cmpint (pango2_font_description_get_size (desc), ==, 32 * PANGO2_SCALE);
  g_assert_cmpint (pango2_font_description_get_style (desc), ==, PANGO2_STYLE_ITALIC);
  g_assert_cmpint (pango2_font_description_get_variant (desc), ==, PANGO2_VARIANT_NORMAL);
  g_assert_cmpint (pango2_font_description_get_weight (desc), ==, PANGO2_WEIGHT_BOLD);
  g_assert_cmpint (pango2_font_description_get_stretch (desc), ==, PANGO2_STRETCH_NORMAL);
  g_assert_cmpstr (pango2_font_description_get_faceid (desc), ==, "Cantarell-Regular:0:-1:0");
  g_assert_cmpstr (pango2_font_description_get_variations (desc), ==, "wght=600");

  s = pango2_font_description_to_string (desc);
  g_assert_cmpstr (s, ==, test);
  g_free (s);

  pango2_font_description_free (desc);
}

static gboolean
font_info_cb (Pango2UserFace     *face,
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
glyph_cb (Pango2UserFace  *face,
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
glyph_info_cb (Pango2UserFace      *face,
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
count_fonts (Pango2Fontset *fontset,
             Pango2Font    *font,
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
  Pango2FontMap *map;
  Pango2Context *context;
  Pango2UserFace *custom;
  Pango2FontDescription *desc;
  Pango2Fontset *fontset;
  Pango2Font *font;
  int count;

  map = pango2_font_map_new ();
  context = pango2_context_new_with_font_map (map);

  desc = pango2_font_description_from_string ("CustomFont");
  custom = pango2_user_face_new (font_info_cb,
                                glyph_cb,
                                glyph_info_cb,
                                NULL,
                                NULL,
                                NULL, NULL,
                                "Regular", desc);
  pango2_font_description_free (desc);

  pango2_font_map_add_face (map, PANGO2_FONT_FACE (custom));

  desc = pango2_font_description_from_string ("CustomFont 11");
  fontset = pango2_font_map_load_fontset (map, context, desc, NULL);
  g_assert_nonnull (fontset);

  font = pango2_fontset_get_font (fontset, 0x20);
  g_assert_nonnull (font);
  g_assert_true (pango2_font_get_face (font) == PANGO2_FONT_FACE (custom));
  g_object_unref (font);

  count = 0;
  pango2_fontset_foreach (fontset, count_fonts, &count);
  g_assert_true (count == 1);

  pango2_font_map_set_fallback (map, pango2_font_map_get_default ());

  desc = pango2_font_description_from_string ("CustomFont 11");
  fontset = pango2_font_map_load_fontset (map, context, desc, NULL);
  g_assert_nonnull (fontset);

  font = pango2_fontset_get_font (fontset, ' ');
  g_assert_nonnull (font);
  g_assert_true (pango2_font_get_face (font) == PANGO2_FONT_FACE (custom));
  g_object_unref (font);

  font = pango2_fontset_get_font (fontset, 'a');
  g_assert_nonnull (font);

  count = 0;
  pango2_fontset_foreach (fontset, count_fonts, &count);
  g_assert_true (count > 1);

  g_object_unref (context);
  g_object_unref (map);
}

static void
test_font_scale (void)
{
  Pango2FontMap *fontmap;
  Pango2Context *context;
  Pango2FontDescription *desc;
  Pango2Font *font;
  Pango2Font *scaled_font;
  Pango2FontDescription *scaled_desc;
  char *str;
  cairo_font_options_t *options;
  const cairo_font_options_t *options2;

  fontmap = pango2_font_map_new_default ();
  context = pango2_context_new_with_font_map (fontmap);

  options = cairo_font_options_create ();
  cairo_font_options_set_antialias (options, CAIRO_ANTIALIAS_NONE);
  cairo_font_options_set_hint_style (options, CAIRO_HINT_STYLE_FULL);
  cairo_font_options_set_hint_metrics (options, CAIRO_HINT_METRICS_ON);

  pango2_cairo_context_set_font_options (context, options);

  desc = pango2_font_description_from_string ("Cantarell 11 @wght=444");

  font = pango2_font_map_load_font (fontmap, context, desc);

  scaled_font = pango2_font_map_reload_font (fontmap, font, 1.5, NULL, NULL);

  g_assert_true (scaled_font != font);

  scaled_desc = pango2_font_describe (scaled_font);
  str = pango2_font_description_to_string (scaled_desc);
  g_assert_cmpstr (str, ==, "Cantarell 16.5 @faceid=hb:Cantarell-Regular:0:-2:0:1:1:0 @wght=444");

  /* check that we also preserve font options */
  options2 = pango2_cairo_font_get_font_options (scaled_font);
  g_assert_true (cairo_font_options_equal (options, options2));

  g_object_unref (scaled_font);
  pango2_font_description_free (scaled_desc);
  g_free (str);

  /* Try again, this time with different font options */

  cairo_font_options_set_hint_style (options, CAIRO_HINT_STYLE_NONE);
  cairo_font_options_set_hint_metrics (options, CAIRO_HINT_METRICS_OFF);

  pango2_cairo_context_set_font_options (context, options);

  scaled_font = pango2_font_map_reload_font (fontmap, font, 1, context, NULL);

  g_assert_true (scaled_font != font);

  scaled_desc = pango2_font_describe (scaled_font);
  str = pango2_font_description_to_string (scaled_desc);
  g_assert_cmpstr (str, ==, "Cantarell 11 @faceid=hb:Cantarell-Regular:0:-2:0:1:1:0 @wght=444");

  options2 = pango2_cairo_font_get_font_options (scaled_font);
  g_assert_true (cairo_font_options_equal (options, options2));

  cairo_font_options_destroy (options);

  g_object_unref (scaled_font);
  pango2_font_description_free (scaled_desc);
  g_free (str);

  /* Try again, this time with different variations */

  scaled_font = pango2_font_map_reload_font (fontmap, font, 1, NULL, "wght=666");

  g_assert_true (scaled_font != font);

  scaled_desc = pango2_font_describe (scaled_font);
  str = pango2_font_description_to_string (scaled_desc);
  g_assert_cmpstr (str, ==, "Cantarell 11 @faceid=hb:Cantarell-Regular:0:-2:0:1:1:0 @wght=666");

  g_object_unref (scaled_font);
  pango2_font_description_free (scaled_desc);
  g_free (str);

  g_object_unref (font);
  pango2_font_description_free (desc);
  g_object_unref (context);
  g_object_unref (fontmap);
}

int
main (int argc, char *argv[])
{
  setlocale (LC_ALL, "");

  g_test_init (&argc, &argv, NULL);

  install_fonts ();

  g_test_add_func ("/pango/fontdescription/parse", test_parse);
  g_test_add_func ("/pango/fontdescription/roundtrip", test_roundtrip);
  g_test_add_func ("/pango/fontdescription/variations", test_variations);
  g_test_add_func ("/pango/fontdescription/empty-variations", test_empty_variations);
  g_test_add_func ("/pango/fontdescription/set-gravity", test_set_gravity);
  g_test_add_func ("/pango/fontdescription/faceid", test_faceid);
  g_test_add_func ("/pango/fontdescription/all", test_all);
  g_test_add_func ("/pango/font/metrics", test_metrics);
  g_test_add_func ("/pango/font/extents", test_extents);
  g_test_add_func ("/pango/font/glyph-extents", test_glyph_extents);
  g_test_add_func ("/pango/font/font-metrics", test_font_metrics);
  g_test_add_func ("/pango/font/roundtrip/plain", test_roundtrip_plain);
  g_test_add_func ("/pango/font/roundtrip/small-caps", test_roundtrip_small_caps);
  g_test_add_func ("/pango/font/roundtrip/emoji", test_roundtrip_emoji);
  g_test_add_func ("/pango/font/roundtrip/absolute", test_roundtrip_absolute);
  g_test_add_func ("/pango/fontmap/enumerate", test_fontmap_enumerate);
  g_test_add_func ("/pango/fontmap/models", test_fontmap_models);
  g_test_add_func ("/pango/fontmap/fallback", test_fontmap_fallback);
  g_test_add_func ("/pango/font/scale-font", test_font_scale);

  return g_test_run ();
}
