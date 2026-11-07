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

#include "config.h"

#include <string.h>
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
test_large_wrapped_item (gconstpointer data)
{
  PangoContext *context;
  PangoLayout *layout;
  PangoFontDescription *font;
  PangoAttrList *attrs;
  char *text;
  GSList *lines;
  int char_width;
  int offset = 0;
  const int n_chars = GPOINTER_TO_INT (data);
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
  if (n_chars == 500000)
    g_assert_cmpint ((gint64) char_width * n_chars, >, G_MAXINT);

  text = g_strnfill (n_chars, 'a');

  pango_layout_set_text (layout, text, n_chars);
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

  g_free (text);
  g_object_unref (layout);
  g_object_unref (context);
}

static void
assert_rectangle_equal (const PangoRectangle *a,
                        const PangoRectangle *b)
{
  g_assert_cmpint (a->x, ==, b->x);
  g_assert_cmpint (a->y, ==, b->y);
  g_assert_cmpint (a->width, ==, b->width);
  g_assert_cmpint (a->height, ==, b->height);
}

static void
compare_layouts (PangoLayout *actual,
                 PangoLayout *reference,
                 gboolean     compare_cursors)
{
  GSList *a, *b;
  PangoRectangle ai, al, bi, bl;
  const char *text = pango_layout_get_text (actual);

  pango_layout_get_extents (actual, &ai, &al);
  pango_layout_get_extents (reference, &bi, &bl);
  assert_rectangle_equal (&ai, &bi);
  assert_rectangle_equal (&al, &bl);
  g_assert_cmpint (pango_layout_get_line_count (actual), ==,
                   pango_layout_get_line_count (reference));

  for (a = pango_layout_get_lines_readonly (actual),
       b = pango_layout_get_lines_readonly (reference); a; a = a->next, b = b->next)
    {
      PangoLayoutLine *la = a->data;
      PangoLayoutLine *lb = b->data;
      GSList *ra, *rb;

      g_assert_cmpint (la->start_index, ==, lb->start_index);
      g_assert_cmpint (la->length, ==, lb->length);
      g_assert_cmpint (la->resolved_dir, ==, lb->resolved_dir);
      pango_layout_line_get_extents (la, &ai, &al);
      pango_layout_line_get_extents (lb, &bi, &bl);
      assert_rectangle_equal (&ai, &bi);
      assert_rectangle_equal (&al, &bl);
      g_assert_cmpint (g_slist_length (la->runs), ==, g_slist_length (lb->runs));
      for (ra = la->runs, rb = lb->runs; ra; ra = ra->next, rb = rb->next)
        {
          PangoGlyphItem *ga = ra->data;
          PangoGlyphItem *gb = rb->data;

          g_assert_cmpint (ga->item->offset, ==, gb->item->offset);
          g_assert_cmpint (ga->item->length, ==, gb->item->length);
          g_assert_cmpint (ga->glyphs->num_glyphs, ==, gb->glyphs->num_glyphs);
          for (int i = 0; i < ga->glyphs->num_glyphs; i++)
            {
              PangoGlyphInfo *ia = &ga->glyphs->glyphs[i];
              PangoGlyphInfo *ib = &gb->glyphs->glyphs[i];

              g_assert_cmpuint (ia->glyph, ==, ib->glyph);
              g_assert_cmpint (ga->glyphs->log_clusters[i], ==, gb->glyphs->log_clusters[i]);
              g_assert_cmpint (ia->geometry.width, ==, ib->geometry.width);
              g_assert_cmpint (ia->geometry.x_offset, ==, ib->geometry.x_offset);
              g_assert_cmpint (ia->geometry.y_offset, ==, ib->geometry.y_offset);
              g_assert (ia->attr.unsafe_to_break || !ib->attr.unsafe_to_break);
              g_assert (ia->attr.unsafe_to_concat || !ib->attr.unsafe_to_concat);
            }
        }
    }

  if (!compare_cursors)
    return;

  for (const char *p = text; ; p = g_utf8_next_char (p))
    {
      int index = p - text;
      int ax, bx, aline, bline, aindex, bindex, atrailing, btrailing;
      gboolean ar, br;

      pango_layout_get_cursor_pos (actual, index, &ai, &al);
      pango_layout_get_cursor_pos (reference, index, &bi, &bl);
      assert_rectangle_equal (&ai, &bi);
      assert_rectangle_equal (&al, &bl);
      pango_layout_index_to_pos (actual, index, &ai);
      pango_layout_index_to_pos (reference, index, &bi);
      assert_rectangle_equal (&ai, &bi);
      for (int trailing = 0; trailing <= 1; trailing++)
        {
          pango_layout_index_to_line_x (actual, index, trailing, &aline, &ax);
          pango_layout_index_to_line_x (reference, index, trailing, &bline, &bx);
          g_assert_cmpint (aline, ==, bline);
          g_assert_cmpint (ax, ==, bx);
        }
      ar = pango_layout_xy_to_index (actual, ai.x, ai.y, &aindex, &atrailing);
      br = pango_layout_xy_to_index (reference, bi.x, bi.y, &bindex, &btrailing);
      g_assert_cmpint (ar, ==, br);
      g_assert_cmpint (aindex, ==, bindex);
      g_assert_cmpint (atrailing, ==, btrailing);
      if (!*p)
        break;
    }
}

static void
test_wrapping_broken_syllables (void)
{
  const char *text = "ভ া  কষষ ষভষারভপ ষংংংাা";
  PangoContext *context;
  PangoLayout *layout;
  PangoLayout *reference;
  PangoFontDescription *desc;
  PangoFontDescription *loaded_desc;
  PangoFont *font;
  PangoAttrList *attrs;
  PangoLayoutLine *line;
  PangoGlyphItem *run;
  PangoGlyphString *glyphs;
  PangoLogAttr *log_attrs;
  int char_offset;

  context = pango_font_map_create_context (pango_cairo_font_map_get_default ());
  pango_cairo_context_set_resolution (context, 96);
  desc = pango_font_description_from_string ("Noto Sans Bengali 12");
  font = pango_context_load_font (context, desc);
  loaded_desc = font ? pango_font_describe (font) : NULL;
  if (!loaded_desc ||
      g_strcmp0 (pango_font_description_get_family (loaded_desc), "Noto Sans Bengali") != 0)
    {
      g_test_skip ("Noto Sans Bengali is required for this test");
      g_clear_pointer (&loaded_desc, pango_font_description_free);
      g_clear_object (&font);
      pango_font_description_free (desc);
      g_object_unref (context);
      return;
    }
  pango_font_description_free (loaded_desc);
  g_object_unref (font);

  layout = pango_layout_new (context);
  pango_layout_set_font_description (layout, desc);
  pango_font_description_free (desc);
  pango_layout_set_text (layout, text, -1);
  pango_layout_set_width (layout, 187 * PANGO_SCALE);
  pango_layout_set_wrap (layout, PANGO_WRAP_WORD_CHAR);
  attrs = pango_attr_list_new ();
  pango_attr_list_insert (attrs, pango_attr_insert_hyphens_new (FALSE));
  pango_layout_set_attributes (layout, attrs);
  pango_attr_list_unref (attrs);

  reference = pango_layout_copy (layout);
  g_object_set_data (G_OBJECT (reference), "pango-no-reuse", GINT_TO_POINTER (1));
  compare_layouts (layout, reference, TRUE);

  g_assert_cmpint (pango_layout_get_line_count (layout), ==, 2);
  line = pango_layout_get_line_readonly (layout, 1);
  g_assert_cmpuint (g_slist_length (line->runs), ==, 1);
  run = line->runs->data;
  g_assert_cmpstr (text + run->item->offset, ==, "ষংংংাা");

  glyphs = pango_glyph_string_new ();
  pango_layout_get_log_attrs (layout, &log_attrs, NULL);
  char_offset = g_utf8_pointer_to_offset (text, text + run->item->offset);
  pango_shape_item (run->item, text, strlen (text), log_attrs + char_offset,
                    glyphs, PANGO_SHAPE_ROUND_POSITIONS);
  g_assert_cmpint (run->glyphs->num_glyphs, ==, glyphs->num_glyphs);
  for (int i = 0; i < glyphs->num_glyphs; i++)
    {
      g_assert_cmpuint (run->glyphs->glyphs[i].glyph, ==, glyphs->glyphs[i].glyph);
      g_assert_cmpint (run->glyphs->log_clusters[i], ==, glyphs->log_clusters[i]);
      g_assert_cmpint (run->glyphs->glyphs[i].geometry.width, ==, glyphs->glyphs[i].geometry.width);
      g_assert_cmpint (run->glyphs->glyphs[i].geometry.x_offset, ==, glyphs->glyphs[i].geometry.x_offset);
      g_assert_cmpint (run->glyphs->glyphs[i].geometry.y_offset, ==, glyphs->glyphs[i].geometry.y_offset);
    }

  pango_glyph_string_free (glyphs);
  g_free (log_attrs);
  g_object_unref (reference);
  g_object_unref (layout);
  g_object_unref (context);
}

static void
test_wrapping_arabic_context (void)
{
  const char *text = "\u0644\u0652\u061c\u064e\u064e\u00ad\u0639";
  g_autoptr(PangoContext) context = NULL;
  g_autoptr(PangoFontDescription) desc = NULL;
  g_autoptr(PangoFontDescription) loaded_desc = NULL;
  g_autoptr(PangoFont) font = NULL;
  g_autoptr(PangoLayout) layout = NULL;
  g_autoptr(PangoLayout) reference = NULL;
  g_autoptr(PangoGlyphString) glyphs = NULL;
  g_autofree PangoLogAttr *log_attrs = NULL;
  PangoLayoutLine *line;
  PangoGlyphItem *run;
  PangoRectangle rect;
  int char_offset;

  context = pango_font_map_create_context (pango_cairo_font_map_get_default ());
  pango_cairo_context_set_resolution (context, 96);
  desc = pango_font_description_from_string ("Droid Arabic Kufi 12");
  font = pango_context_load_font (context, desc);
  loaded_desc = font ? pango_font_describe (font) : NULL;
  if (!loaded_desc ||
      g_strcmp0 (pango_font_description_get_family (loaded_desc), "Droid Arabic Kufi") != 0)
    {
      g_test_skip ("Droid Arabic Kufi is required for this test");
      return;
    }

  layout = pango_layout_new (context);
  pango_layout_set_font_description (layout, desc);
  pango_layout_set_text (layout, text, -1);
  pango_layout_set_width (layout, 5 * PANGO_SCALE);
  pango_layout_set_wrap (layout, PANGO_WRAP_WORD);

  reference = pango_layout_copy (layout);
  g_object_set_data (G_OBJECT (reference), "pango-no-reuse", GINT_TO_POINTER (1));
  compare_layouts (layout, reference, TRUE);

  g_assert_cmpint (pango_layout_get_line_count (layout), ==, 2);
  line = pango_layout_get_line_readonly (layout, 1);
  g_assert_cmpuint (g_slist_length (line->runs), ==, 1);
  run = line->runs->data;
  g_assert_cmpstr (text + run->item->offset, ==, "\u0639");

  glyphs = pango_glyph_string_new ();
  pango_layout_get_log_attrs (layout, &log_attrs, NULL);
  char_offset = g_utf8_pointer_to_offset (text, text + run->item->offset);
  pango_shape_item (run->item, text, strlen (text), log_attrs + char_offset, glyphs, PANGO_SHAPE_ROUND_POSITIONS);
  g_assert_cmpint (glyphs->num_glyphs, ==, 1);
  g_assert_cmpint (run->glyphs->num_glyphs, ==, 1);
  g_assert_cmpuint (run->glyphs->glyphs[0].glyph, ==, glyphs->glyphs[0].glyph);
  g_assert_cmpint (run->glyphs->log_clusters[0], ==, glyphs->log_clusters[0]);
  g_assert_cmpint (run->glyphs->glyphs[0].geometry.width, ==, glyphs->glyphs[0].geometry.width);
  g_assert_cmpint (run->glyphs->glyphs[0].geometry.x_offset, ==, glyphs->glyphs[0].geometry.x_offset);
  g_assert_cmpint (run->glyphs->glyphs[0].geometry.y_offset, ==, glyphs->glyphs[0].geometry.y_offset);
  pango_layout_line_get_extents (line, NULL, &rect);
  g_assert_cmpint (rect.width, ==, pango_glyph_string_get_width (glyphs));
}

static void
test_wrapping_reuse (void)
{
  const char *texts[] = {
    "one two three   four five six seven eight nine ten ",
    "office affinity ffi ffl AVATAR Toffee efficient official",
    "áâä éêë क्‍षि क्षत्रिय संस्कृत भाषा हिन्दी परीक्षण",
    "السَّلَامُ عَلَيْكُمْ العربية لا لا العربية كلمات طويلة",
    "English עברית العربية 123 ffi हिन्दी English again",
    "extraordinary incomprehensibility soft\302\255hyphen co\302\255operate",
    "abc\t12.34\tffi def\txyz longer words and more words",
    "one two\nthree four\342\200\250five six\nseven eight",
    "extraordinarily incomprehensibility internationalization counterintuitiveness "
    "extraordinarily incomprehensibility internationalization counterintuitiveness",
    "office\302\255affinity ffi\302\255ffl áêï office\302\255affinity ffi\302\255ffl",

  };
  const int widths[] = { 0, 1, 17, 39, 80, 143, 300, -1 };
  PangoContext *context;

  context = pango_font_map_create_context (pango_cairo_font_map_get_default ());
  for (int t = 0; t < G_N_ELEMENTS (texts); t++)
    for (int w = 0; w < G_N_ELEMENTS (widths); w++)
      for (int wrap = PANGO_WRAP_WORD; wrap <= PANGO_WRAP_WORD_CHAR; wrap++)
        for (int variant = 0; variant < 11; variant++)
          {
            PangoLayout *actual;
            PangoLayout *reference;
            PangoAttrList *attrs = pango_attr_list_new ();
            PangoTabArray *tabs;
            PangoRectangle rect = { 0, -10 * PANGO_SCALE, 10 * PANGO_SCALE, 12 * PANGO_SCALE };

            pango_context_set_base_gravity (context, variant == 7 ? PANGO_GRAVITY_EAST : PANGO_GRAVITY_SOUTH);
            pango_context_set_round_glyph_positions (context, variant != 9);
            actual = pango_layout_new (context);
            pango_layout_set_text (actual, texts[t], -1);
            pango_layout_set_width (actual, widths[w] < 0 ? -1 : widths[w] * PANGO_SCALE);
            pango_layout_set_wrap (actual, wrap);
            switch (variant)
              {
              case 0:
                pango_attr_list_insert (attrs, pango_attr_insert_hyphens_new (FALSE));
                break;

              case 2:
                pango_attr_list_insert (attrs, pango_attr_letter_spacing_new (PANGO_SCALE));
                break;

              case 3:
                pango_attr_list_insert (attrs, pango_attr_text_transform_new (PANGO_TEXT_TRANSFORM_UPPERCASE));
                break;

              case 4:
                pango_attr_list_insert (attrs, pango_attr_shape_new (&rect, &rect));
                break;

              case 5:
                tabs = pango_tab_array_new (3, TRUE);
                pango_tab_array_set_tab (tabs, 0, PANGO_TAB_RIGHT, 50);
                pango_tab_array_set_tab (tabs, 1, PANGO_TAB_CENTER, 100);
                pango_tab_array_set_tab (tabs, 2, PANGO_TAB_DECIMAL, 150);
                pango_layout_set_tabs (actual, tabs);
                pango_tab_array_free (tabs);
                break;

              case 6:
                pango_layout_set_height (actual, -2);
                pango_layout_set_ellipsize (actual, PANGO_ELLIPSIZE_END);
                break;

              case 8:
                pango_layout_set_justify (actual, TRUE);
                break;

              case 10:
                {
                  PangoAttribute *attr = pango_attr_weight_new (PANGO_WEIGHT_BOLD);

                  attr->start_index = g_utf8_offset_to_pointer (texts[t], 4) - texts[t];
                  attr->end_index = g_utf8_offset_to_pointer (texts[t], 11) - texts[t];
                  pango_attr_list_insert (attrs, attr);
                }
                break;

              default:
                break;
              }
            pango_layout_set_attributes (actual, attrs);
            pango_attr_list_unref (attrs);
            reference = pango_layout_copy (actual);
            g_object_set_data (G_OBJECT (reference), "pango-no-reuse", GINT_TO_POINTER (1));
            g_test_message ("text %d width %d wrap %d variant %d", t, widths[w], wrap, variant);
            compare_layouts (actual, reference, variant != 4 || (t != 3 && t != 4));
            g_object_unref (reference);
            g_object_unref (actual);
          }
  g_object_unref (context);
}

int
main (int argc, char *argv[])
{
  g_test_init (&argc, &argv, NULL);

  g_test_add_func ("/layout/bad.layout", test_bad_layout);
  g_test_add_data_func ("/layout/large-wrapped-item", GINT_TO_POINTER (500000), test_large_wrapped_item);
  g_test_add_data_func ("/layout/wrapped-item-125000", GINT_TO_POINTER (125000), test_large_wrapped_item);
  g_test_add_data_func ("/layout/wrapped-item-250000", GINT_TO_POINTER (250000), test_large_wrapped_item);

  g_test_add_func ("/layout/wrapping-reuse", test_wrapping_reuse);
  g_test_add_func ("/layout/wrapping-broken-syllables", test_wrapping_broken_syllables);
  g_test_add_func ("/layout/wrapping-arabic-context", test_wrapping_arabic_context);

  return g_test_run ();
}
