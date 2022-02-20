/* Pango
 * testhbfont.c: Test program for PangoHbFont etc
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
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Library General Public License for more details.
 *
 * You should have received a copy of the GNU Library General Public
 * License along with this library; if not, write to the
 * Free Software Foundation, Inc., 59 Temple Place - Suite 330,
 * Boston, MA 02111-1307, USA.
 */

#include "config.h"
#include <glib.h>
#include <gio/gio.h>
#include <pango/pangocairo.h>
#include <pango/pangofc-hbfontmap.h>

#include <hb-ot.h>


/* Verify that the variable and monospace properties work as expected
 * for PangoHbFamily
 */
static void
test_hbfont_monospace (void)
{
  PangoHbFontMap *map;
  PangoFontFamily *family;
  char *path;

  map = pango_hb_font_map_new ();

  path = g_test_build_filename (G_TEST_DIST, "fonts", "Cantarell-VF.otf", NULL);
  pango_hb_font_map_add_file (map, path);

  family = pango_font_map_get_family (PANGO_FONT_MAP (map), "Cantarell");

  g_assert_nonnull (family);
  g_assert_false (pango_font_family_is_variable (family));
  g_assert_false (pango_font_family_is_monospace (family));

  pango_hb_font_map_add_face (map, PANGO_FONT_FACE (pango_hb_face_new_from_file (path, 0, -2, NULL, NULL)));

  g_assert_true (pango_font_family_is_variable (family));

  g_free (path);

  path = g_test_build_filename (G_TEST_DIST, "fonts", "DejaVuSansMono.ttf", NULL);
  pango_hb_font_map_add_file (map, path);
  g_free (path);

  family = pango_font_map_get_family (PANGO_FONT_MAP (map), "DejaVu Sans Mono");

  g_assert_nonnull (family);
  g_assert_false (pango_font_family_is_variable (family));
  g_assert_true (pango_font_family_is_monospace (family));

  g_object_unref (map);
}

/* Verify that a description -> face -> description roundtrip works for
 * PangoHbFaces created with pango_hb_face_new_synthetic or pango_hb_face_new_instance
 */
static void
test_hbface_roundtrip (void)
{
  char *path;
  PangoHbFace *face;
  PangoHbFace *face2;
  PangoFontDescription *desc;
  const int NO_FACEID = ~PANGO_FONT_MASK_FACEID;
  hb_variation_t v;

  path = g_test_build_filename (G_TEST_DIST, "fonts", "Cantarell-VF.otf", NULL);

  face = pango_hb_face_new_from_file (path, 0, -1, NULL, NULL);
  g_assert_true (PANGO_IS_HB_FACE (face));
  g_assert_cmpstr (pango_font_face_get_face_name (PANGO_FONT_FACE (face)), ==, "Regular");
  desc = pango_font_face_describe (PANGO_FONT_FACE (face));
  g_assert_cmpint (pango_font_description_get_set_fields (desc) & NO_FACEID, ==, PANGO_FONT_MASK_FAMILY |
                                                                                 PANGO_FONT_MASK_STYLE |
                                                                                 PANGO_FONT_MASK_WEIGHT |
                                                                                 PANGO_FONT_MASK_STRETCH);
  g_assert_cmpstr (pango_font_description_get_family (desc), ==, "Cantarell");
  g_assert_cmpint (pango_font_description_get_style (desc), ==, PANGO_STYLE_NORMAL);
  g_assert_cmpint (pango_font_description_get_weight (desc), ==, PANGO_WEIGHT_NORMAL);
  g_assert_cmpint (pango_font_description_get_stretch (desc), ==, PANGO_STRETCH_NORMAL);
  pango_font_description_free (desc);

  desc = pango_font_description_new ();
  pango_font_description_set_style (desc, PANGO_STYLE_OBLIQUE);
  face2 = pango_hb_face_new_synthetic (face, &(PangoMatrix){ 1, 0.2, 0, 1, 0, 0 }, FALSE, NULL, desc);
  pango_font_description_free (desc);

  g_assert_true (PANGO_IS_HB_FACE (face2));
  g_assert_cmpstr (pango_font_face_get_face_name (PANGO_FONT_FACE (face2)), ==, "Oblique");
  desc = pango_font_face_describe (PANGO_FONT_FACE (face2));
  g_assert_cmpint (pango_font_description_get_set_fields (desc) & NO_FACEID, ==, PANGO_FONT_MASK_FAMILY |
                                                                                 PANGO_FONT_MASK_STYLE |
                                                                                 PANGO_FONT_MASK_WEIGHT |
                                                                                 PANGO_FONT_MASK_STRETCH);
  g_assert_cmpstr (pango_font_description_get_family (desc), ==, "Cantarell");
  g_assert_cmpint (pango_font_description_get_style (desc), ==, PANGO_STYLE_OBLIQUE);
  g_assert_cmpint (pango_font_description_get_weight (desc), ==, PANGO_WEIGHT_NORMAL);
  g_assert_cmpint (pango_font_description_get_stretch (desc), ==, PANGO_STRETCH_NORMAL);
  pango_font_description_free (desc);
  g_object_unref (face2);

  desc = pango_font_description_new ();
  pango_font_description_set_weight (desc, PANGO_WEIGHT_BOLD);
  face2 = pango_hb_face_new_synthetic (face, NULL, TRUE, NULL, desc);
  pango_font_description_free (desc);

  g_assert_true (PANGO_IS_HB_FACE (face2));
  g_assert_cmpstr (pango_font_face_get_face_name (PANGO_FONT_FACE (face2)), ==, "Bold");
  desc = pango_font_face_describe (PANGO_FONT_FACE (face2));
  g_assert_cmpint (pango_font_description_get_set_fields (desc) & NO_FACEID, ==, PANGO_FONT_MASK_FAMILY |
                                                                                 PANGO_FONT_MASK_STYLE |
                                                                                 PANGO_FONT_MASK_WEIGHT |
                                                                                 PANGO_FONT_MASK_STRETCH);
  g_assert_cmpstr (pango_font_description_get_family (desc), ==, "Cantarell");
  g_assert_cmpint (pango_font_description_get_style (desc), ==, PANGO_STYLE_NORMAL);
  g_assert_cmpint (pango_font_description_get_weight (desc), ==, PANGO_WEIGHT_BOLD);
  g_assert_cmpint (pango_font_description_get_stretch (desc), ==, PANGO_STRETCH_NORMAL);
  pango_font_description_free (desc);
  g_object_unref (face2);

  desc = pango_font_description_new ();
  pango_font_description_set_family (desc, "Cantarellagain");
  face2 = pango_hb_face_new_synthetic (face, NULL, FALSE, NULL, desc);
  pango_font_description_free (desc);

  g_assert_true (PANGO_IS_HB_FACE (face2));
  g_assert_cmpstr (pango_font_face_get_face_name (PANGO_FONT_FACE (face2)), ==, "Regular");
  desc = pango_font_face_describe (PANGO_FONT_FACE (face2));
  g_assert_cmpint (pango_font_description_get_set_fields (desc) & NO_FACEID, ==, PANGO_FONT_MASK_FAMILY |
                                                                                 PANGO_FONT_MASK_STYLE |
                                                                                 PANGO_FONT_MASK_WEIGHT |
                                                                                 PANGO_FONT_MASK_STRETCH);
  g_assert_cmpstr (pango_font_description_get_family (desc), ==, "Cantarellagain");
  g_assert_cmpint (pango_font_description_get_style (desc), ==, PANGO_STYLE_NORMAL);
  g_assert_cmpint (pango_font_description_get_weight (desc), ==, PANGO_WEIGHT_NORMAL);
  g_assert_cmpint (pango_font_description_get_stretch (desc), ==, PANGO_STRETCH_NORMAL);
  pango_font_description_free (desc);
  g_object_unref (face2);

  desc = pango_font_description_new ();
  pango_font_description_set_family (desc, "Cat");
  pango_font_description_set_weight (desc, PANGO_WEIGHT_ULTRABOLD);

  v.tag = HB_OT_TAG_VAR_AXIS_WEIGHT;
  v.value = 768.;

  face2 = pango_hb_face_new_instance (face, &v, 1, "Fat", desc);
  pango_font_description_free (desc);

  g_assert_true (PANGO_IS_HB_FACE (face2));
  g_assert_cmpstr (pango_font_face_get_face_name (PANGO_FONT_FACE (face2)), ==, "Fat");
  desc = pango_font_face_describe (PANGO_FONT_FACE (face2));
  g_assert_cmpint (pango_font_description_get_set_fields (desc) & NO_FACEID, ==, PANGO_FONT_MASK_FAMILY |
                                                                                 PANGO_FONT_MASK_STYLE |
                                                                                 PANGO_FONT_MASK_WEIGHT |
                                                                                 PANGO_FONT_MASK_VARIATIONS |
                                                                                 PANGO_FONT_MASK_STRETCH);
  g_assert_cmpstr (pango_font_description_get_family (desc), ==, "Cat");
  g_assert_cmpint (pango_font_description_get_style (desc), ==, PANGO_STYLE_NORMAL);
  g_assert_cmpint (pango_font_description_get_weight (desc), ==, PANGO_WEIGHT_ULTRABOLD);
  g_assert_cmpint (pango_font_description_get_stretch (desc), ==, PANGO_STRETCH_NORMAL);
  g_assert_cmpstr (pango_font_description_get_variations (desc), ==, "wght=768");
  pango_font_description_free (desc);
  g_object_unref (face2);

  g_object_unref (face);
  g_free (path);
}

/* Verify that face -> font -> description works as expected for PangoHbFont */
static void
test_hbfont_roundtrip (void)
{
  char *path;
  PangoHbFace *face;
  PangoHbFont *font;
  PangoFontDescription *desc;
  hb_feature_t features[10];
  unsigned int n_features;

  path = g_test_build_filename (G_TEST_DIST, "fonts", "Cantarell-VF.otf", NULL);

  face = pango_hb_face_new_from_file (path, 0, -1, NULL, NULL);
  g_assert_true (PANGO_IS_HB_FACE (face));

  font = pango_hb_font_new (face, 11 * PANGO_SCALE, NULL, 0, NULL, 0, PANGO_GRAVITY_AUTO, 96., NULL);
  g_assert_true (PANGO_IS_HB_FONT (font));
  g_assert_true (pango_font_get_face (PANGO_FONT (font)) == PANGO_FONT_FACE (face));
  pango_font_get_features (PANGO_FONT (font), features, G_N_ELEMENTS (features), &n_features);
  g_assert_cmpint (n_features, ==, 0);

  desc = pango_font_describe (PANGO_FONT (font));
  g_assert_cmpstr (pango_font_description_get_family (desc), ==, "Cantarell");
  g_assert_cmpint (pango_font_description_get_style (desc), ==, PANGO_STYLE_NORMAL);
  g_assert_cmpint (pango_font_description_get_weight (desc), ==, PANGO_WEIGHT_NORMAL);
  g_assert_cmpint (pango_font_description_get_stretch (desc), ==, PANGO_STRETCH_NORMAL);
  g_assert_cmpint (pango_font_description_get_size (desc), ==, 11 * PANGO_SCALE);
  pango_font_description_free (desc);

  g_object_unref (font);
  g_object_unref (face);
  g_free (path);
}

/* Verify that pango_font_describe and pango_font_describe_with_absolute_size
 * work as expected with PangoHbFont
 */
static void
test_hbfont_describe (void)
{
  char *path;
  PangoHbFace *face;
  PangoHbFont *font;
  PangoFontDescription *desc;

  path = g_test_build_filename (G_TEST_DIST, "fonts", "Cantarell-VF.otf", NULL);

  face = pango_hb_face_new_from_file (path, 0, -1, NULL, NULL);
  g_assert_true (PANGO_IS_HB_FACE (face));

  font = pango_hb_font_new (face, 11 * PANGO_SCALE, NULL, 0, NULL, 0, PANGO_GRAVITY_AUTO, 96., NULL);
  g_assert_true (PANGO_IS_HB_FONT (font));

  desc = pango_font_describe (PANGO_FONT (font));
  g_assert_cmpstr (pango_font_description_get_family (desc), ==, "Cantarell");
  g_assert_cmpint (pango_font_description_get_size (desc), ==, 11 * PANGO_SCALE);
  g_assert_true (!pango_font_description_get_size_is_absolute (desc));
  pango_font_description_free (desc);

  desc = pango_font_describe_with_absolute_size (PANGO_FONT (font));
  g_assert_cmpstr (pango_font_description_get_family (desc), ==, "Cantarell");
  g_assert_cmpint (pango_font_description_get_size (desc), ==, 11 * PANGO_SCALE * 96./72.);
  g_assert_true (pango_font_description_get_size_is_absolute (desc));
  pango_font_description_free (desc);

  g_object_unref (font);
  g_object_unref (face);
  g_free (path);
}

/* Test that describing fonts and faces works with variations */
static void
test_hbfont_describe_variation (void)
{
  char *path;
  PangoHbFace *face, *face2;
  PangoHbFont *font;
  PangoFontDescription *desc;
  hb_variation_t v;

  path = g_test_build_filename (G_TEST_DIST, "fonts", "Cantarell-VF.otf", NULL);

  face = pango_hb_face_new_from_file (path, 0, -1, NULL, NULL);
  g_assert_true (PANGO_IS_HB_FACE (face));

  font = pango_hb_font_new (face, 11 * PANGO_SCALE, NULL, 0, NULL, 0, PANGO_GRAVITY_AUTO, 96., NULL);
  g_assert_true (PANGO_IS_HB_FONT (font));

  desc = pango_font_describe (PANGO_FONT (font));
  g_assert_true ((pango_font_description_get_set_fields (desc) & PANGO_FONT_MASK_VARIATIONS) == 0);
  pango_font_description_free (desc);
  g_object_unref (font);

  v.tag = HB_OT_TAG_VAR_AXIS_WEIGHT;
  v.value = 768.;
  font = pango_hb_font_new (face, 11 * PANGO_SCALE, NULL, 0, &v, 1, PANGO_GRAVITY_AUTO, 96., NULL);

  desc = pango_font_describe (PANGO_FONT (font));
  g_assert_true ((pango_font_description_get_set_fields (desc) & PANGO_FONT_MASK_VARIATIONS) != 0);
  g_assert_cmpstr (pango_font_description_get_variations (desc), ==, "wght=768");
  pango_font_description_free (desc);
  g_object_unref (font);

  desc = pango_font_description_new ();
  pango_font_description_set_family (desc, "Cantarellagain");

  v.tag = HB_OT_TAG_VAR_AXIS_WEIGHT;
  v.value = 512.;
  face2 = pango_hb_face_new_instance (face, &v, 1, "Medium", desc);
  g_assert_true (PANGO_IS_HB_FACE (face));
  pango_font_description_free (desc);

  desc = pango_font_face_describe (PANGO_FONT_FACE (face2));
  g_assert_true ((pango_font_description_get_set_fields (desc) & PANGO_FONT_MASK_VARIATIONS) != 0);
  g_assert_cmpstr (pango_font_description_get_variations (desc), ==, "wght=512");
  pango_font_description_free (desc);

  font = pango_hb_font_new (face2, 11 * PANGO_SCALE, NULL, 0, NULL, 0, PANGO_GRAVITY_AUTO, 96., NULL);
  g_assert_true (PANGO_IS_HB_FONT (font));

  desc = pango_font_describe (PANGO_FONT (font));
  g_assert_true ((pango_font_description_get_set_fields (desc) & PANGO_FONT_MASK_VARIATIONS) != 0);
  g_assert_cmpstr (pango_font_description_get_variations (desc), ==, "wght=512");
  pango_font_description_free (desc);
  g_object_unref (font);

  v.tag = HB_OT_TAG_VAR_AXIS_WEIGHT;
  v.value = 768.;
  font = pango_hb_font_new (face2, 11 * PANGO_SCALE, NULL, 0, &v, 1, PANGO_GRAVITY_AUTO, 96., NULL);

  desc = pango_font_describe (PANGO_FONT (font));
  g_assert_true ((pango_font_description_get_set_fields (desc) & PANGO_FONT_MASK_VARIATIONS) != 0);
  g_assert_cmpstr (pango_font_description_get_variations (desc), ==, "wght=768");
  pango_font_description_free (desc);
  g_object_unref (font);

  g_object_unref (face2);
  g_object_unref (face);
  g_free (path);
}

/* Test that we get different faceids for the different named instances
 * or variants of Cantarell.
 */
static void
test_hbfont_faceid (void)
{
  char *path;
  PangoHbFace *face, *face2, *face3;
  PangoFontDescription *desc, *desc2, *desc3;

  path = g_test_build_filename (G_TEST_DIST, "fonts", "Cantarell-VF.otf", NULL);

  face = pango_hb_face_new_from_file (path, 0, -1, NULL, NULL);
  face2 = pango_hb_face_new_from_file (path, 0, 2, NULL, NULL);
  desc = pango_font_description_new ();
  pango_font_description_set_weight (desc, PANGO_WEIGHT_BOLD);
  face3 = pango_hb_face_new_synthetic (face, NULL, TRUE, NULL, desc);
  pango_font_description_free (desc);

  desc = pango_font_face_describe (PANGO_FONT_FACE (face));
  g_assert_true (pango_font_description_get_set_fields (desc) & PANGO_FONT_MASK_FACEID);

  desc2 = pango_font_face_describe (PANGO_FONT_FACE (face2));
  g_assert_true (pango_font_description_get_set_fields (desc2) & PANGO_FONT_MASK_FACEID);

  desc3 = pango_font_face_describe (PANGO_FONT_FACE (face3));
  g_assert_true (pango_font_description_get_set_fields (desc3) & PANGO_FONT_MASK_FACEID);

  g_assert_cmpstr (pango_font_description_get_faceid (desc), !=, pango_font_description_get_faceid (desc2));
  g_assert_cmpstr (pango_font_description_get_faceid (desc), !=, pango_font_description_get_faceid (desc3));
  g_assert_cmpstr (pango_font_description_get_faceid (desc2), !=, pango_font_description_get_faceid (desc3));

  pango_font_description_free (desc);
  pango_font_description_free (desc2);
  pango_font_description_free (desc3);

  g_object_unref (face);
  g_object_unref (face2);
  g_object_unref (face3);

  g_free (path);
}

/* Test font -> description -> font roundtrips with a difficult family */
static void
test_hbfont_load (void)
{
  PangoHbFontMap *map;
  PangoContext *context;
  char *path;
  PangoFontDescription *desc;
  PangoHbFace *face_fat, *face_wild;
  char *s;
  PangoFont *font;

  /* Make a Cat family, with the two faces Fat and Wild */
  map = pango_hb_font_map_new ();
  context = pango_font_map_create_context (PANGO_FONT_MAP (map));

  path = g_test_build_filename (G_TEST_DIST, "fonts", "Cantarell-VF.otf", NULL);
  desc = pango_font_description_new ();
  pango_font_description_set_family (desc, "Cat");
  face_fat = pango_hb_face_new_from_file (path, 0, -1, "Fat", desc);
  pango_font_description_free (desc);
  g_free (path);

  pango_hb_font_map_add_face (map, PANGO_FONT_FACE (face_fat));

  path = g_test_build_filename (G_TEST_DIST, "fonts", "DejaVuSans.ttf", NULL);
  desc = pango_font_description_new ();
  pango_font_description_set_family (desc, "Cat");
  face_wild = pango_hb_face_new_from_file (path, 0, -1, "Wild", desc);
  pango_font_description_free (desc);

  pango_hb_font_map_add_face (map, PANGO_FONT_FACE (face_wild));

  desc = pango_font_face_describe (PANGO_FONT_FACE (face_wild));
  pango_font_description_set_size (desc, 12 * PANGO_SCALE);

  s = pango_font_description_to_string (desc);
  g_assert_cmpstr (s, ==, "Cat 12 @faceid=hb:DejaVuSans:0:-1:0:1:1:0");
  g_free (s);

  /* loading with faceid set works as expected */
  font = pango_font_map_load_font (PANGO_FONT_MAP (map), context, desc);
  g_assert_true (pango_font_get_face (font) == PANGO_FONT_FACE (face_wild));
  g_object_unref (font);

  pango_font_description_unset_fields (desc, PANGO_FONT_MASK_FACEID);

  /* ...and without doesn't */
  s = pango_font_description_to_string (desc);
  g_assert_cmpstr (s, ==, "Cat 12");
  g_free (s);

  font = pango_font_map_load_font (PANGO_FONT_MAP (map), context, desc);
  g_assert_true (pango_font_get_face (font) == PANGO_FONT_FACE (face_fat));
  g_object_unref (font);

  pango_font_description_free (desc);

  g_object_unref (context);
  g_object_unref (map);
}

/* Test font -> description -> font roundtrips with variations */
static void
test_hbfont_load_variation (void)
{
  PangoHbFontMap *map;
  PangoContext *context;
  char *path;
  PangoFontDescription *desc;
  PangoHbFace *face_fat, *face_wild;
  char *s;
  PangoFont *font;
  hb_variation_t v;
  hb_font_t *hb_font;
  const float *coords;
  unsigned int length;

  /* Make a Cat family, with the two faces Fat and Wild */
  map = pango_hb_font_map_new ();
  context = pango_font_map_create_context (PANGO_FONT_MAP (map));

  path = g_test_build_filename (G_TEST_DIST, "fonts", "Cantarell-VF.otf", NULL);
  desc = pango_font_description_new ();
  pango_font_description_set_family (desc, "Cat");
  face_fat = pango_hb_face_new_from_file (path, 0, -1, "Fat", desc);
  pango_font_description_free (desc);
  g_free (path);

  pango_hb_font_map_add_face (map, PANGO_FONT_FACE (face_fat));

  desc = pango_font_description_new ();
  pango_font_description_set_family (desc, "Cat");
  v.tag = HB_OT_TAG_VAR_AXIS_WEIGHT;
  v.value = 624.;
  face_wild = pango_hb_face_new_instance (face_fat, &v, 1, "Wild", desc);
  pango_font_description_free (desc);

  pango_hb_font_map_add_face (map, PANGO_FONT_FACE (face_wild));

  desc = pango_font_face_describe (PANGO_FONT_FACE (face_wild));

  g_assert_cmpstr (pango_font_description_get_variations (desc), ==, "wght=624");

  pango_font_description_set_size (desc, 12 * PANGO_SCALE);

  s = pango_font_description_to_string (desc);
  g_assert_cmpstr (s, ==, "Cat 12 @faceid=hb:Cantarell-Regular:0:-1:0:1:1:0:wght_624,wght=624");
  g_free (s);

  font = pango_font_map_load_font (PANGO_FONT_MAP (map), context, desc);
  g_assert_true (pango_font_get_face (font) == PANGO_FONT_FACE (face_wild));

  hb_font = pango_font_get_hb_font (font);
  coords = hb_font_get_var_coords_design (hb_font, &length);
  g_assert_cmpint (length, ==, 1);
  g_assert_cmphex (coords[0], ==, 624.);

  g_object_unref (font);

  pango_font_description_free (desc);

  g_object_unref (context);
  g_object_unref (map);
}

/* Verify that pango_fontmap_load_fontset produces a non-empty result
 * even if the language isn't covered - our itemization code relies
 * on this.
 */
static gboolean
get_font (PangoFontset *fontset,
          PangoFont    *font,
          gpointer      data)
{
  gboolean *found = data;

  *found = TRUE;

  return TRUE;
}

static void
test_hbfontmap_language (void)
{
  PangoFontMap *map;
  PangoContext *context;
  PangoFontDescription *desc;
  PangoFontset *fonts;
  gboolean found;

  map = PANGO_FONT_MAP (pango_fc_hb_font_map_new ());
  context = pango_font_map_create_context (map);
  desc = pango_font_description_from_string ("serif 11");

  /* zz isn't assigned, so there should not be any fonts claiming to support
   * this language. We are expecting to get a nonempty fontset regardless.
   */
  fonts = pango_font_map_load_fontset (map, context, desc, pango_language_from_string ("zz"));
  g_assert_true (PANGO_IS_FONTSET (fonts));

  found = FALSE;
  pango_fontset_foreach (fonts, get_font, &found);
  g_assert_true (found);

  g_object_unref (fonts);
  pango_font_description_free (desc);
}

int
main (int argc, char *argv[])
{
  g_test_init (&argc, &argv, NULL);

  g_test_add_func ("/hbfont/monospace", test_hbfont_monospace);
  g_test_add_func ("/hbface/roundtrip", test_hbface_roundtrip);
  g_test_add_func ("/hbfont/roundtrip", test_hbfont_roundtrip);
  g_test_add_func ("/hbfont/describe", test_hbfont_describe);
  g_test_add_func ("/hbfont/describe/variation", test_hbfont_describe_variation);
  g_test_add_func ("/hbfont/faceid", test_hbfont_faceid);
  g_test_add_func ("/hbfont/load", test_hbfont_load);
  g_test_add_func ("/hbfont/load/variation", test_hbfont_load_variation);
  g_test_add_func ("/hbfontmap/language", test_hbfontmap_language);

  return g_test_run ();
}
