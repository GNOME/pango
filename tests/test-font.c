/* Pango
 * test-font.c: Test PangoFontDescription
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

#include <pango/pangocairo.h>

static PangoContext *context;

static void
test_parse (void)
{
  PangoFontDescription *desc;

  desc = pango_font_description_from_string ("Cantarell 14");

  g_assert_cmpstr (pango_font_description_get_family (desc), ==, "Cantarell");
  g_assert (!pango_font_description_get_size_is_absolute (desc));
  g_assert_cmpint (pango_font_description_get_size (desc), ==, 14 * PANGO_SCALE);
  g_assert_cmpint (pango_font_description_get_style (desc), ==, PANGO_STYLE_NORMAL);
  g_assert_cmpint (pango_font_description_get_variant (desc), ==, PANGO_VARIANT_NORMAL);
  g_assert_cmpint (pango_font_description_get_weight (desc), ==, PANGO_WEIGHT_NORMAL);
  g_assert_cmpint (pango_font_description_get_stretch (desc), ==, PANGO_STRETCH_NORMAL);
  g_assert_cmpint (pango_font_description_get_gravity (desc), ==, PANGO_GRAVITY_SOUTH);
  g_assert_cmpint (pango_font_description_get_set_fields (desc), ==, PANGO_FONT_MASK_FAMILY | PANGO_FONT_MASK_STYLE | PANGO_FONT_MASK_VARIANT | PANGO_FONT_MASK_WEIGHT | PANGO_FONT_MASK_STRETCH | PANGO_FONT_MASK_SIZE);

  pango_font_description_free (desc); 

  desc = pango_font_description_from_string ("Sans Bold Italic Condensed 22.5px");

  g_assert_cmpstr (pango_font_description_get_family (desc), ==, "Sans");
  g_assert (pango_font_description_get_size_is_absolute (desc)); 
  g_assert_cmpint (pango_font_description_get_size (desc), ==, 225 * PANGO_SCALE / 10);
  g_assert_cmpint (pango_font_description_get_style (desc), ==, PANGO_STYLE_ITALIC);
  g_assert_cmpint (pango_font_description_get_variant (desc), ==, PANGO_VARIANT_NORMAL); 
  g_assert_cmpint (pango_font_description_get_weight (desc), ==, PANGO_WEIGHT_BOLD);
  g_assert_cmpint (pango_font_description_get_stretch (desc), ==, PANGO_STRETCH_CONDENSED); 
  g_assert_cmpint (pango_font_description_get_gravity (desc), ==, PANGO_GRAVITY_SOUTH);  g_assert_cmpint (pango_font_description_get_set_fields (desc), ==, PANGO_FONT_MASK_FAMILY | PANGO_FONT_MASK_STYLE | PANGO_FONT_MASK_VARIANT | PANGO_FONT_MASK_WEIGHT | PANGO_FONT_MASK_STRETCH | PANGO_FONT_MASK_SIZE);

  pango_font_description_free (desc); 
}

static void
test_roundtrip (void)
{
  PangoFontDescription *desc;
 gchar *str;

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
test_variation (void)
{
  PangoFontDescription *desc1;
  PangoFontDescription *desc2;
  gchar *str;

  desc1 = pango_font_description_from_string ("Cantarell 14");
  g_assert (desc1 != NULL);
  g_assert ((pango_font_description_get_set_fields (desc1) & PANGO_FONT_MASK_VARIATIONS) == 0);
  g_assert (pango_font_description_get_variations (desc1) == NULL);

  str = pango_font_description_to_string (desc1);
  g_assert_cmpstr (str, ==, "Cantarell 14");
  g_free (str);

  desc2 = pango_font_description_from_string ("Cantarell 14 @wght=100,wdth=235");
  g_assert (desc2 != NULL);
  g_assert ((pango_font_description_get_set_fields (desc2) & PANGO_FONT_MASK_VARIATIONS) != 0);
  g_assert_cmpstr (pango_font_description_get_variations (desc2), ==, "wght=100,wdth=235");

  str = pango_font_description_to_string (desc2);
  g_assert_cmpstr (str, ==, "Cantarell 14 @wght=100,wdth=235");
  g_free (str);

  g_assert (!pango_font_description_equal (desc1, desc2));

  pango_font_description_set_variations (desc1, "wght=100,wdth=235");
  g_assert ((pango_font_description_get_set_fields (desc1) & PANGO_FONT_MASK_VARIATIONS) != 0);
  g_assert_cmpstr (pango_font_description_get_variations (desc1), ==, "wght=100,wdth=235");

  g_assert (pango_font_description_equal (desc1, desc2));

  pango_font_description_free (desc1);
  pango_font_description_free (desc2);
}

static void
test_metrics (void)
{
  PangoFontDescription *desc;
  PangoFontMetrics *metrics;
  char *str;


  if (strcmp (G_OBJECT_TYPE_NAME (pango_context_get_font_map (context)), "PangoCairoWin32FontMap") == 0)
    desc = pango_font_description_from_string ("Verdana 11");
  else
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

  pango_font_metrics_unref (metrics);
  g_free (str);
  pango_font_description_free (desc);
}

static void
test_extents (void)
{
  char *str = "Composer";
  GList *items;
  PangoItem *item;
  PangoGlyphString *glyphs;
  PangoRectangle ink, log;
  PangoContext *context;

  context = pango_font_map_create_context (pango_cairo_font_map_get_default ());
  pango_context_set_font_description (context, pango_font_description_from_string ("Cantarell 11"));

  items = pango_itemize (context, str, 0, strlen (str), NULL, NULL);
  glyphs = pango_glyph_string_new ();
  item = items->data;
  pango_shape (str, strlen (str), &item->analysis, glyphs);
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
test_enumerate (void)
{
  PangoFontMap *fontmap;
  PangoContext *context;
  PangoFontFamily **families;
  PangoFontFamily *family;
  int n_families;
  int i;
  PangoFontFace **faces;
  PangoFontFace *face;
  int n_faces;
  PangoFontDescription *desc;
  PangoFont *font;
  gboolean found_face;

  fontmap = pango_cairo_font_map_get_default ();
  context = pango_font_map_create_context (fontmap);

  pango_font_map_list_families (fontmap, &families, &n_families);
  g_assert_cmpint (n_families, >, 0);

  for (i = 0; i < n_families; i++)
    {
      family = pango_font_map_get_family (fontmap, pango_font_family_get_name (families[i]));
      g_assert_true (family == families[i]);
    }

  pango_font_family_list_faces (families[0], &faces, &n_faces);
  g_assert_cmpint (n_faces, >, 0);
  for (i = 0; i < n_faces; i++)
    {
      face = pango_font_family_get_face (families[0], pango_font_face_get_face_name (faces[i]));
      g_assert_true (face == faces[i]);
    }

  desc = pango_font_description_new ();
  pango_font_description_set_family (desc, pango_font_family_get_name (families[0]));
  pango_font_description_set_size (desc, 10*PANGO_SCALE);

  font = pango_font_map_load_font (fontmap, context, desc);
  face = pango_font_get_face (font);
  found_face = FALSE;
  for (i = 0; i < n_faces; i++)
    {
      if (face == faces[i])
        {
          found_face = TRUE;
          break;
        }
    }
  g_assert_true (found_face);

  g_object_unref (font);
  pango_font_description_free (desc);
  g_free (faces);
  g_free (families);
  g_object_unref (context);
}

static void
test_roundtrip_plain (void)
{
  PangoFontMap *fontmap;
  PangoContext *context;
  PangoFontDescription *desc, *desc2;
  PangoFont *font;

#ifdef HAVE_CARBON
  /* We probably don't have the right fonts */
  g_test_skip ("Skipping font-dependent tests on OS X");
  return;
#endif

  fontmap = pango_cairo_font_map_get_default ();
  context = pango_font_map_create_context (fontmap);

  desc = pango_font_description_from_string ("Cantarell 11");

  font = pango_context_load_font (context, desc);
  desc2 = pango_font_describe (font);

  g_assert (pango_font_description_equal (desc2, desc));

  pango_font_description_free (desc2);
  g_object_unref (font);
  pango_font_description_free (desc);
  g_object_unref (context);
}

static void
test_roundtrip_emoji (void)
{
  PangoFontMap *fontmap;
  PangoContext *context;
  PangoFontDescription *desc, *desc2;
  PangoFont *font;

#ifdef HAVE_CARBON
  /* We probably don't have the right fonts */
  g_test_skip ("Skipping font-dependent tests on OS X");
  return;
#endif

  fontmap = pango_cairo_font_map_get_default ();
  context = pango_font_map_create_context (fontmap);

  /* This is how pango_itemize creates the emoji font desc */
  desc = pango_font_description_from_string ("Cantarell 11");
  pango_font_description_set_family_static (desc, "emoji");

  font = pango_context_load_font (context, desc);
  desc2 = pango_font_describe (font);

  /* We can't expect the family name to match, since we go in with
   * a generic family
   */
  pango_font_description_unset_fields (desc, PANGO_FONT_MASK_FAMILY);
  pango_font_description_unset_fields (desc2, PANGO_FONT_MASK_FAMILY);
  g_assert (pango_font_description_equal (desc2, desc));

  pango_font_description_free (desc2);
  g_object_unref (font);
  pango_font_description_free (desc);
  g_object_unref (context);
  g_object_unref (fontmap);
}

int
main (int argc, char *argv[])
{
  g_setenv ("LC_ALL", "C", TRUE);
  setlocale (LC_ALL, "");

  g_test_init (&argc, &argv, NULL);

  context = pango_font_map_create_context (pango_cairo_font_map_get_default ());

  g_test_add_func ("/pango/font/metrics", test_metrics);
  g_test_add_func ("/pango/fontdescription/parse", test_parse);
  g_test_add_func ("/pango/fontdescription/roundtrip", test_roundtrip);
  g_test_add_func ("/pango/fontdescription/variation", test_variation);
  g_test_add_func ("/pango/font/extents", test_extents);
  g_test_add_func ("/pango/font/enumerate", test_enumerate);
  g_test_add_func ("/pango/font/roundtrip/plain", test_roundtrip_plain);
  g_test_add_func ("/pango/font/roundtrip/emoji", test_roundtrip_emoji);

  return g_test_run ();
}
