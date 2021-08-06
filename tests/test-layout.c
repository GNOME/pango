/* Pango
 * test-layout.c: Test Pango Layout
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

#ifndef G_OS_WIN32
#include <unistd.h>
#endif

#include "config.h"
#include <pango/pangocairo.h>
#include "test-common.h"


static PangoContext *context;

static gboolean opt_show_font;

static const gchar *
enum_value_nick (GType type, gint value)
{
  GEnumClass *eclass;
  GEnumValue *ev;

  eclass = g_type_class_ref (type);
  ev = g_enum_get_value (eclass, value);
  g_type_class_unref (eclass);

  if (ev)
    return ev->value_nick;
  else
    return "?";
}

static const gchar *
direction_name (PangoDirection dir)
{
  return enum_value_nick (PANGO_TYPE_DIRECTION, dir);
}

static const gchar *
gravity_name (PangoGravity gravity)
{
  return enum_value_nick (PANGO_TYPE_GRAVITY, gravity);
}

static const gchar *
script_name (PangoScript script)
{
  return enum_value_nick (PANGO_TYPE_SCRIPT, script);
}

static gchar *
font_name (PangoFont *font)
{
  PangoFontDescription *desc;
  gchar *name;

  desc = pango_font_describe (font);
  name = pango_font_description_to_string (desc);
  pango_font_description_free (desc);

  return name;
}

static void
dump_lines (PangoLayout *layout, GString *string)
{
  PangoLayoutIter *iter;
  const gchar *text;
  gint index, index2;
  gboolean has_more;
  gchar *char_str;
  gint i;
  PangoLayoutLine *line;

  text = pango_layout_get_text (layout);
  iter = pango_layout_get_iter (layout);

  has_more = TRUE;
  index = pango_layout_iter_get_index (iter);
  index2 = 0;
  i = 0;
  while (has_more)
    {
      line = pango_layout_iter_get_line (iter);
      has_more = pango_layout_iter_next_line (iter);
      i++;

      if (has_more)
        {
          index2 = pango_layout_iter_get_index (iter);
          char_str = g_strndup (text + index, index2 - index);
        }
      else
        {
          char_str = g_strdup (text + index);
        }

      g_string_append_printf (string, "i=%d, index=%d, paragraph-start=%d, dir=%s '%s'\n",
                              i, index, line->is_paragraph_start, direction_name (line->resolved_dir),
                              char_str);
      g_free (char_str);

      index = index2;
    }
  pango_layout_iter_free (iter);
}

static void
dump_runs (PangoLayout *layout, GString *string)
{
  PangoLayoutIter *iter;
  PangoLayoutRun *run;
  PangoItem *item;
  const gchar *text;
  gint index;
  gboolean has_more;
  gchar *char_str;
  gint i;
  gchar *font = 0;

  text = pango_layout_get_text (layout);
  iter = pango_layout_get_iter (layout);

  has_more = TRUE;
  i = 0;
  while (has_more)
    {
      run = pango_layout_iter_get_run (iter);
      index = pango_layout_iter_get_index (iter);
      has_more = pango_layout_iter_next_run (iter);
      i++;

      if (run)
        {
          item = ((PangoGlyphItem*)run)->item;
          char_str = g_strndup (text + item->offset, item->length);
          font = font_name (item->analysis.font);
          g_string_append_printf (string, "i=%d, index=%d, chars=%d, level=%d, gravity=%s, flags=%d, font=%s, script=%s, language=%s, '%s'\n",
                                  i, index, item->num_chars, item->analysis.level,
                                  gravity_name (item->analysis.gravity),
                                  item->analysis.flags,
                                  opt_show_font ? font : "OMITTED", /* for some reason, this fails on build.gnome.org, so leave it out */
                                  script_name (item->analysis.script),
                                  pango_language_to_string (item->analysis.language),
                                  char_str);
          print_attributes (item->analysis.extra_attrs, string);
          g_free (font);
          g_free (char_str);
        }
      else
        {
          g_string_append_printf (string, "i=%d, index=%d, no run, line end\n",
                                  i, index);
        }
    }
  pango_layout_iter_free (iter);
}

static void
dump_directions (PangoLayout *layout, GString *string)
{
  const char *text, *p;

  text = pango_layout_get_text (layout);
  for (p = text; *p; p = g_utf8_next_char (p))
    {
      g_string_append_printf (string, "%d ", pango_layout_get_direction (layout, p - text));
    }
  g_string_append (string, "\n");
}

static void
dump_cursor_positions (PangoLayout *layout, GString *string)
{
  const char *text;
  int index, trailing;

  text = pango_layout_get_text (layout);

  index = 0;
  trailing = 0;

  while (index < G_MAXINT)
    {
      g_string_append_printf (string, "%d(%d) ", index, trailing);

      while (trailing--)
        index = g_utf8_next_char (text + index) - text;

      pango_layout_move_cursor_visually (layout, TRUE, index, 0, 1, &index, &trailing);
    }

  g_string_append (string, "\n");
}

typedef struct {
  int width;
  int height;
  int indent;
  int spacing;
  float line_spacing;
  PangoEllipsizeMode ellipsize;
  PangoWrapMode wrap;
  PangoAlignment alignment;
  gboolean justify;
  gboolean auto_dir;
  gboolean single_paragraph;
  PangoTabArray *tabs;
  PangoGravity gravity;
} LayoutParams;

static void
init_params (LayoutParams *params)
{
  params->width = -1;
  params->height = -1;
  params->indent = 0;
  params->spacing = 0;
  params->line_spacing = 0.0;
  params->ellipsize = PANGO_ELLIPSIZE_NONE;
  params->wrap = PANGO_WRAP_WORD;
  params->alignment = PANGO_ALIGN_LEFT;
  params->justify = FALSE;
  params->auto_dir = TRUE;
  params->single_paragraph = FALSE;
  params->tabs = NULL;
  params->gravity = PANGO_GRAVITY_AUTO;
}

static void
parse_params (const char   *str,
              LayoutParams *params)
{
  char **strings;
  int i;
  GEnumClass *eclass;
  GEnumValue *ev;

  strings = g_strsplit (str, ",", -1);
  for (i = 0; strings[i]; i++)
    {
      char **str2 = g_strsplit (strings[i], "=", -1);
      if (strcmp (str2[0], "width") == 0)
        {
          params->width = (int) g_ascii_strtoll (str2[1], NULL, 10);
        }
      else if (strcmp (str2[0], "height") == 0)
        {
          params->height = (int) g_ascii_strtoll (str2[1], NULL, 10);
        }
      else if (strcmp (str2[0], "indent") == 0)
        {
          params->indent = (int) g_ascii_strtoll (str2[1], NULL, 10);
        }
      else if (strcmp (str2[0], "spacing") == 0)
        {
          params->spacing = (int) g_ascii_strtoll (str2[1], NULL, 10);
        }
      else if (strcmp (str2[0], "line_spacing") == 0)
        {
          params->line_spacing = (float) g_ascii_strtod (str2[1], NULL);
        }
      else if (strcmp (str2[0], "ellipsize") == 0)
        {
          eclass = g_type_class_ref (PANGO_TYPE_ELLIPSIZE_MODE);
          ev = g_enum_get_value_by_name (eclass, str2[1]);
          if (!ev)
            ev = g_enum_get_value_by_nick (eclass, str2[1]);
          if (ev)
            params->ellipsize = ev->value;
          g_type_class_unref (eclass);
        }
      else if (strcmp (str2[0], "wrap") == 0)
        {
          eclass = g_type_class_ref (PANGO_TYPE_WRAP_MODE);
          ev = g_enum_get_value_by_name (eclass, str2[1]);
          if (!ev)
            ev = g_enum_get_value_by_nick (eclass, str2[1]);
          if (ev)
            params->wrap = ev->value;
          g_type_class_unref (eclass);
        }
      else if (strcmp (str2[0], "alignment") == 0)
        {
          eclass = g_type_class_ref (PANGO_TYPE_ALIGNMENT);
          ev = g_enum_get_value_by_name (eclass, str2[1]);
          if (!ev)
            ev = g_enum_get_value_by_nick (eclass, str2[1]);
          if (ev)
            params->alignment = ev->value;
          g_type_class_unref (eclass);
        }
      else if (strcmp (str2[0], "justify") == 0)
        {
          params->justify = g_str_equal (str2[1], "true");
        }
      else if (strcmp (str2[0], "auto_dir") == 0)
        {
          params->auto_dir = g_str_equal (str2[1], "true");
        }
      else if (strcmp (str2[0], "single_paragraph") == 0)
        {
          params->single_paragraph = g_str_equal (str2[1], "true");
        }
      else if (strcmp (str2[0], "tabs") == 0)
        {
          char **str3 = g_strsplit (strings[i], " ", -1);
          params->tabs = pango_tab_array_new (g_strv_length (str3), TRUE);
          for (int j = 0; str3[j]; j++)
            {
              int tab = (int) g_ascii_strtoll (str3[j], NULL, 10);
              pango_tab_array_set_tab (params->tabs, j, PANGO_TAB_LEFT, tab);
            }
          g_strfreev (str3);
        }
      else if (strcmp (str2[0], "gravity") == 0)
        {
          eclass = g_type_class_ref (PANGO_TYPE_GRAVITY);
          ev = g_enum_get_value_by_name (eclass, str2[1]);
          if (!ev)
            ev = g_enum_get_value_by_nick (eclass, str2[1]);
          if (ev)
            params->gravity = ev->value;
          g_type_class_unref (eclass);
        }

      g_strfreev (str2);
    }
  g_strfreev (strings);
}

#define assert_layout_changed(layout) \
  g_assert_cmpuint (pango_layout_get_serial (layout), !=, serial); \
  serial = pango_layout_get_serial (layout);

#define assert_rectangle_equal(r1, r2) \
  g_assert_true((r1)->x == (r2)->x && \
                (r1)->y == (r2)->y && \
                (r1)->width == (r2)->width && \
                (r1)->height == (r2)->height)

#define assert_rectangle_contained(r1, r2) \
  g_assert_true ((r1)->x >= (r2)->x && \
                 (r1)->y >= (r2)->y && \
                 (r1)->x + (r1)->width <= (r2)->x + (r2)->width && \
                 (r1)->y + (r1)->height <= (r2)->y + (r2)->height)

#define assert_rectangle_size_contained(r1, r2) \
  g_assert_true ((r1)->width <= (r2)->width && \
                 (r1)->height <= (r2)->height)

static void
test_file (const char *filename, GString *string)
{
  char *contents;
  char *markup;
  gsize  length;
  GError *error = NULL;
  PangoLayout *layout;
  char *p;
  LayoutParams params;
  PangoFontDescription *desc;
  const PangoFontDescription *desc2;
  guint serial;
  PangoRectangle ink_rect, logical_rect;
  PangoRectangle ink_rect1, logical_rect1;
  int width, height;
  int width1, height1;
  PangoTabArray *tabs;
  GSList *lines, *l;
  PangoLayoutIter *iter;
  PangoLayoutIter *iter2;

  if (context == NULL)
    context = pango_font_map_create_context (pango_cairo_font_map_get_default ());

  g_file_get_contents (filename, &contents, &length, &error);
  g_assert_no_error (error);

  p = strchr (contents, '\n');
  g_assert (p);
  markup = p + 1;
  *p = '\0';
  length = strlen (markup);

  init_params (&params);

  layout = pango_layout_new (context);

  serial = pango_layout_get_serial (layout);
  g_assert_cmpuint (serial, !=, 0);

  /* Check initial values */
  g_assert_cmpint (pango_layout_get_width (layout), ==, params.width);
  g_assert_cmpint (pango_layout_get_height (layout), ==, params.height);
  g_assert_cmpint (pango_layout_get_indent (layout), ==, params.indent);
  g_assert_cmpint (pango_layout_get_spacing (layout), ==, params.spacing);
  g_assert_cmpfloat (pango_layout_get_line_spacing (layout), ==, params.line_spacing);
  g_assert_cmpint (pango_layout_get_ellipsize (layout), ==, params.ellipsize);
  g_assert_cmpint (pango_layout_get_wrap (layout), ==, params.wrap);
  g_assert_cmpint (pango_layout_get_alignment (layout), ==, params.alignment);
  g_assert_cmpint (pango_layout_get_justify (layout), ==, params.justify);
  g_assert_cmpint (pango_layout_get_auto_dir (layout), ==, params.auto_dir);
  g_assert_cmpint (pango_layout_get_single_paragraph_mode (layout), ==, params.single_paragraph);

  g_assert_cmpstr (pango_layout_get_text (layout), ==, "");
  g_assert_null (pango_layout_get_attributes (layout));
  g_assert_null (pango_layout_get_tabs (layout));
  g_assert_null (pango_layout_get_font_description (layout));
  g_assert_cmpint (pango_layout_is_ellipsized (layout), ==, FALSE);
  g_assert_cmpint (pango_layout_is_wrapped (layout), ==, FALSE);

  desc = pango_font_description_from_string ("Cantarell 11");
  pango_layout_set_font_description (layout, desc);
  desc2 = pango_layout_get_font_description (layout);
  g_assert_true (pango_font_description_equal (desc, desc2));
  pango_font_description_free (desc);
  assert_layout_changed (layout);

  pango_layout_set_markup (layout, markup, length);
  assert_layout_changed (layout);

  parse_params (contents, &params);

  pango_context_set_base_gravity (context, params.gravity);

  pango_layout_set_width (layout, params.width > 0 ? params.width * PANGO_SCALE : -1);
  pango_layout_set_height (layout, params.height > 0 ? params.height * PANGO_SCALE : params.height);
  pango_layout_set_indent (layout, params.indent * PANGO_SCALE);
  pango_layout_set_spacing (layout, params.spacing * PANGO_SCALE);
  pango_layout_set_line_spacing (layout, params.line_spacing);
  pango_layout_set_ellipsize (layout, params.ellipsize);
  pango_layout_set_wrap (layout, params.wrap);
  pango_layout_set_alignment (layout, params.alignment);
  pango_layout_set_justify (layout, params.justify);
  pango_layout_set_auto_dir (layout, params.auto_dir);
  pango_layout_set_single_paragraph_mode (layout, params.single_paragraph);
  pango_layout_set_tabs (layout, params.tabs);

  /* Check the values we set */
  g_assert_cmpint (pango_layout_get_width (layout), ==, params.width > 0 ? params.width * PANGO_SCALE : -1);
  g_assert_cmpint (pango_layout_get_height (layout), ==, params.height > 0 ? params.height * PANGO_SCALE : params.height);
  g_assert_cmpint (pango_layout_get_indent (layout), ==, params.indent * PANGO_SCALE);
  g_assert_cmpint (pango_layout_get_spacing (layout), ==, params.spacing * PANGO_SCALE);
  g_assert_cmpfloat (pango_layout_get_line_spacing (layout), ==, params.line_spacing);
  g_assert_cmpint (pango_layout_get_ellipsize (layout), ==, params.ellipsize);
  g_assert_cmpint (pango_layout_get_wrap (layout), ==, params.wrap);
  g_assert_cmpint (pango_layout_get_alignment (layout), ==, params.alignment);
  g_assert_cmpint (pango_layout_get_justify (layout), ==, params.justify);
  g_assert_cmpint (pango_layout_get_auto_dir (layout), ==, params.auto_dir);
  g_assert_cmpint (pango_layout_get_single_paragraph_mode (layout), ==, params.single_paragraph);

  tabs = pango_layout_get_tabs (layout);
  g_assert_true ((tabs == NULL) == (params.tabs == NULL));
  if (tabs)
    pango_tab_array_free (tabs);

  g_assert_cmpint (pango_layout_get_character_count (layout), ==, g_utf8_strlen (pango_layout_get_text (layout), -1));

  /* Some checks on extents - we have to be careful here, since we
   * don't want to depend on font metrics.
   */
  pango_layout_get_size (layout, &width, &height);
  pango_layout_get_extents (layout, &ink_rect, &logical_rect);
  g_assert_cmpint (width, ==, logical_rect.width);
  g_assert_cmpint (height, ==, logical_rect.height);

  pango_extents_to_pixels (&ink_rect, NULL);
  pango_extents_to_pixels (&logical_rect, NULL);
  pango_layout_get_pixel_extents (layout, &ink_rect1, &logical_rect1);
  pango_layout_get_pixel_size (layout, &width1, &height1);

  assert_rectangle_equal (&ink_rect, &ink_rect1);
  assert_rectangle_equal (&logical_rect, &logical_rect1);
  g_assert_cmpint (width1, ==, logical_rect1.width);
  g_assert_cmpint (height1, ==, logical_rect1.height);

  lines = pango_layout_get_lines (layout);
  for (l = lines; l; l = l->next)
    {
      PangoLayoutLine *line = l->data;
      int line_height, line_width;
      int line_x;
      PangoRectangle line_ink, line_logical;
      PangoRectangle line_ink1, line_logical1;
      gboolean done;

      pango_layout_line_get_height (line, &line_height);
      g_assert_cmpint (line_height, <=, height);

      pango_layout_line_get_extents (line, &line_ink, &line_logical);
      line_x = line_logical.x;
      line_width = line_logical.width;
      pango_extents_to_pixels (&line_ink, NULL);
      pango_extents_to_pixels (&line_logical, NULL);
      pango_layout_line_get_pixel_extents (line, &line_ink1, &line_logical1);

      /* Not in layout coordinates, so just compare sizes */
      assert_rectangle_size_contained (&line_ink, &ink_rect);
      assert_rectangle_size_contained (&line_logical, &logical_rect);
      assert_rectangle_size_contained (&line_ink1, &ink_rect1);
      assert_rectangle_size_contained (&line_logical1, &logical_rect1);

      if (pango_layout_is_ellipsized (layout))
        continue;

      /* FIXME: should have a way to position iters */
      iter = pango_layout_get_iter (layout);
      while (pango_layout_iter_get_line_readonly (iter) != line)
        pango_layout_iter_next_line (iter);

      done = FALSE;
      while (!done && pango_layout_iter_get_line_readonly (iter) == line)
        {
          int prev_index, index, next_index;
          int x, index2, trailing;
          int *ranges;
          int n_ranges;
          gboolean found_range;
          PangoLayoutRun *run;

          index = pango_layout_iter_get_index (iter);
          run = pango_layout_iter_get_run_readonly (iter);

          if (!pango_layout_iter_next_cluster (iter))
            done = TRUE;

          pango_layout_line_index_to_x (line, index, 0, &x);
          pango_layout_line_x_to_index (line, x, &index2, &trailing);

#if 0
          /* FIXME: why doesn't this hold true? */
          g_assert_cmpint (index2, ==, index);
          g_assert_cmpint (trailing, ==, 0);
#endif

          g_assert_cmpint (0, <=, x);
          g_assert_cmpint (x, <=, line_width);

          g_assert_cmpint (line->start_index, <=, index2);
          g_assert_cmpint (index2, <=, line->start_index + line->length);

          if (!run)
            break;

          prev_index = run->item->offset;
          next_index = run->item->offset + run->item->length;

          {
            PangoGlyphItem *run2 = pango_glyph_item_copy (run);
            g_assert_cmpint (run2->item->offset, ==, run->item->offset);
            g_assert_cmpint (run2->item->length, ==, run->item->length);
            pango_glyph_item_free (run2);
          }

          pango_layout_line_get_x_ranges (line, prev_index, next_index, &ranges, &n_ranges);

          /* The index is within the run, so the x should be in one of the ranges */
          if (n_ranges > 0)
            {
              found_range = FALSE;
              for (int k = 0; k < n_ranges; k++)
                {
                  if (x + line_x >= ranges[2*k] && x + line_x <= ranges[2*k + 1])
                    {
                      found_range = TRUE;
                      break;
                    }
                }
            }

          g_assert_true (found_range);
          g_free (ranges);
        }

      pango_layout_iter_free (iter);
    }

  iter = pango_layout_get_iter (layout);
  g_assert_true (pango_layout_iter_get_layout (iter) == layout);
  g_assert_cmpint (pango_layout_iter_get_index (iter), ==, 0);
  pango_layout_iter_get_layout_extents (iter, &ink_rect, &logical_rect);

  iter2 = pango_layout_iter_copy (iter);

  do
    {
      PangoRectangle line_ink, line_logical;
      int baseline;
      PangoLayoutLine *line;
      PangoLayoutRun *run;

      line = pango_layout_iter_get_line (iter);

      pango_layout_iter_get_line_extents (iter, &line_ink, &line_logical);
      baseline = pango_layout_iter_get_baseline (iter);

      assert_rectangle_contained (&line_ink, &ink_rect);
      assert_rectangle_contained (&line_logical, &logical_rect);

      g_assert_cmpint (line_logical.y, <=, baseline);
      g_assert_cmpint (baseline, <=, line_logical.y + line_logical.height);

      if (pango_layout_iter_get_index (iter) == pango_layout_iter_get_index (iter2))
        {
          g_assert_cmpint (baseline, ==, pango_layout_get_baseline (layout));
          g_assert_true (line->is_paragraph_start);
        }

      if (pango_layout_iter_at_last_line (iter))
        {
          g_assert_cmpint (line->start_index + line->length, <=, strlen (pango_layout_get_text (layout)));
        }

      run = pango_layout_iter_get_run (iter);

      if (run)
        {
          const char *text;
          int *widths;
          int *widths2;

          text = pango_layout_get_text (layout);

          widths = g_new (int, run->item->num_chars);
          pango_glyph_item_get_logical_widths (run, text, widths);

          widths2 = g_new (int, run->item->num_chars);
          pango_glyph_string_get_logical_widths (run->glyphs, text + run->item->offset, run->item->length, run->item->analysis.level, widths2);

          g_assert_true (memcmp (widths, widths2, sizeof (int) * run->item->num_chars) == 0);

          g_free (widths);
          g_free (widths2);
        }
    }
  while (pango_layout_iter_next_line (iter));

  pango_layout_iter_free (iter);
  pango_layout_iter_free (iter2);

  /* generate the dumps */
  g_string_append (string, pango_layout_get_text (layout));

  g_string_append (string, "\n--- parameters\n\n");

  g_string_append_printf (string, "wrapped: %d\n", pango_layout_is_wrapped (layout));
  g_string_append_printf (string, "ellipsized: %d\n", pango_layout_is_ellipsized (layout));
  g_string_append_printf (string, "lines: %d\n", pango_layout_get_line_count (layout));
  if (params.width > 0)
    g_string_append_printf (string, "width: %d\n", pango_layout_get_width (layout));

  if (params.height > 0)
    g_string_append_printf (string, "height: %d\n", pango_layout_get_height (layout));

  if (params.indent != 0)
    g_string_append_printf (string, "indent: %d\n", pango_layout_get_indent (layout));

  g_string_append (string, "\n--- attributes\n\n");
  print_attr_list (pango_layout_get_attributes (layout), string);

  g_string_append (string, "\n--- directions\n\n");
  dump_directions (layout, string);

  g_string_append (string, "\n--- cursor positions\n\n");
  dump_cursor_positions (layout, string);

  g_string_append (string, "\n--- lines\n\n");
  dump_lines (layout, string);

  g_string_append (string, "\n--- runs\n\n");
  dump_runs (layout, string);

  g_object_unref (layout);
  g_free (contents);

  if (params.tabs)
    pango_tab_array_free (params.tabs);
}

static gchar *
get_expected_filename (const gchar *filename)
{
  gchar *f, *p, *expected;

  f = g_strdup (filename);
  p = strstr (f, ".markup");
  if (p)
    *p = 0;
  expected = g_strconcat (f, ".expected", NULL);

  g_free (f);

  return expected;
}

static void
test_layout (gconstpointer d)
{
  const gchar *filename = d;
  gchar *expected_file;
  GError *error = NULL;
  GString *dump;
  gchar *diff;
  PangoFontFamily **families;
  int n_families;
  gboolean found_cantarell;

  char *old_locale = g_strdup (setlocale (LC_ALL, NULL));
  setlocale (LC_ALL, "en_US.UTF-8");
  if (strstr (setlocale (LC_ALL, NULL), "en_US") == NULL)
    {
      char *msg = g_strdup_printf ("Locale en_US.UTF-8 not available, skipping layout %s", filename);
      g_test_skip (msg);
      g_free (msg);
      g_free (old_locale);
      return;
    }

  if (context == NULL)
    context = pango_font_map_create_context (pango_cairo_font_map_get_default ());

  found_cantarell = FALSE;
  pango_context_list_families (context, &families, &n_families);
  for (int i = 0; i < n_families; i++)
    {
      if (strcmp (pango_font_family_get_name (families[i]), "Cantarell") == 0)
        {
          found_cantarell = TRUE;
          break;
        }
    }
  g_free (families);

  if (!found_cantarell)
    {
      char *msg = g_strdup_printf ("Cantarell font not available, skipping itemization %s", filename);
      g_test_skip (msg);
      g_free (msg);
      g_free (old_locale);
      return;
    }

  expected_file = get_expected_filename (filename);

  dump = g_string_sized_new (0);

  test_file (filename, dump);

  diff = diff_with_file (expected_file, dump->str, dump->len, &error);
  g_assert_no_error (error);

  setlocale (LC_ALL, old_locale);
  g_free (old_locale);

  if (diff && diff[0])
    {
      char **lines = g_strsplit (diff, "\n", -1);
      const char *line;
      int i = 0;

      g_test_message ("Contents don't match expected contents");

      for (line = lines[0]; line != NULL; line = lines[++i])
        g_test_message ("%s", line);

      g_test_fail ();
      g_strfreev (lines);
    }

  g_free (diff);
  g_string_free (dump, TRUE);
  g_free (expected_file);
}

int
main (int argc, char *argv[])
{
  GDir *dir;
  GError *error = NULL;
  const gchar *name;
  gchar *path;

  setlocale (LC_ALL, "");

  if (g_getenv ("PANGO_TEST_SHOW_FONT"))
    opt_show_font = TRUE;

  /* allow to easily generate expected output for new test cases */
  if (argc > 1 && argv[1][0] != '-')
    {
      GString *string;

      string = g_string_sized_new (0);
      test_file (argv[1], string);
      g_print ("%s", string->str);
      g_string_free (string, TRUE);

      return 0;
    }

  g_test_init (&argc, &argv, NULL);

  path = g_test_build_filename (G_TEST_DIST, "layouts", NULL);
  dir = g_dir_open (path, 0, &error);
  g_free (path);
  g_assert_no_error (error);
  while ((name = g_dir_read_name (dir)) != NULL)
    {
      if (!strstr (name, "markup"))
        continue;

      path = g_strdup_printf ("/layout/%s", name);
      g_test_add_data_func_full (path, g_test_build_filename (G_TEST_DIST, "layouts", name, NULL),
                                 test_layout, g_free);
      g_free (path);
    }
  g_dir_close (dir);

  return g_test_run ();
}
