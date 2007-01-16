/* Pango
 * testiter.c: Test pangolayoutiter.c
 *
 * Copyright (C) 2005 Amit Aronovitch
 * Copyright (C) 2005 Red Hat, Inc
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

#undef G_DISABLE_ASSERT
#undef G_LOG_DOMAIN

#include <stdarg.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include <glib.h>

#include <pango/pangocairo.h>

#undef VERBOSE

static void verbose (const char *format, ...) G_GNUC_PRINTF (1, 2);
static void
verbose (const char *format, ...)
{
#ifdef VERBOSE
  va_list vap;

  va_start (vap, format);
  vfprintf (stderr, format, vap);
  va_end (vap);
#endif
}

#define LAYOUT_WIDTH (80 * PANGO_SCALE)

/* Note: The test expects that any newline sequence is of length 1
 * use \n (not \r\n) in the test texts.
 * I think the iterator itself should support \r\n without trouble,
 * but there are comments in layout-iter.c suggesting otherwise.
 */
const char *test_texts[] =
  {
    /* English with embedded RTL runs (from ancient-hebrew.org) */
    "The Hebrew word \xd7\x90\xd7\x93\xd7\x9d\xd7\x94 (adamah) is the feminine form of \xd7\x90\xd7\x93\xd7\x9d meaning \"ground\"\n",
    /* Arabic, with vowel marks (from Sura Al Fatiha) */
    "\xd8\xa8\xd9\x90\xd8\xb3\xd9\x92\xd9\x85\xd9\x90 \xd8\xa7\xd9\x84\xd9\x84\xd9\x91\xd9\x87\xd9\x90 \xd8\xa7\xd9\x84\xd8\xb1\xd9\x91\xd9\x8e\xd8\xad\xd9\x92\xd9\x85\xd9\x80\xd9\x8e\xd9\x86\xd9\x90 \xd8\xa7\xd9\x84\xd8\xb1\xd9\x91\xd9\x8e\xd8\xad\xd9\x90\xd9\x8a\xd9\x85\xd9\x90\n\xd8\xa7\xd9\x84\xd9\x92\xd8\xad\xd9\x8e\xd9\x85\xd9\x92\xd8\xaf\xd9\x8f \xd9\x84\xd9\x84\xd9\x91\xd9\x87\xd9\x90 \xd8\xb1\xd9\x8e\xd8\xa8\xd9\x91\xd9\x90 \xd8\xa7\xd9\x84\xd9\x92\xd8\xb9\xd9\x8e\xd8\xa7\xd9\x84\xd9\x8e\xd9\x85\xd9\x90\xd9\x8a\xd9\x86\xd9\x8e\n",
    /* Arabic, with embedded LTR runs (from a Linux guide) */
    "\xd8\xa7\xd9\x84\xd9\x85\xd8\xaa\xd8\xba\xd9\x8a\xd8\xb1 LC_ALL \xd9\x8a\xd8\xba\xd9\x8a\xd9\x8a\xd8\xb1 \xd9\x83\xd9\x84 \xd8\xa7\xd9\x84\xd9\x85\xd8\xaa\xd8\xba\xd9\x8a\xd8\xb1\xd8\xa7\xd8\xaa \xd8\xa7\xd9\x84\xd8\xaa\xd9\x8a \xd8\xaa\xd8\xa8\xd8\xaf\xd8\xa3 \xd8\xa8\xd8\xa7\xd9\x84\xd8\xb1\xd9\x85\xd8\xb2 LC.",
    /* Hebrew, with vowel marks (from Genesis) */
    "\xd7\x91\xd6\xbc\xd6\xb0\xd7\xa8\xd6\xb5\xd7\x90\xd7\xa9\xd7\x81\xd6\xb4\xd7\x99\xd7\xaa, \xd7\x91\xd6\xbc\xd6\xb8\xd7\xa8\xd6\xb8\xd7\x90 \xd7\x90\xd6\xb1\xd7\x9c\xd6\xb9\xd7\x94\xd6\xb4\xd7\x99\xd7\x9d, \xd7\x90\xd6\xb5\xd7\xaa \xd7\x94\xd6\xb7\xd7\xa9\xd6\xbc\xd7\x81\xd6\xb8\xd7\x9e\xd6\xb7\xd7\x99\xd6\xb4\xd7\x9d, \xd7\x95\xd6\xb0\xd7\x90\xd6\xb5\xd7\xaa \xd7\x94\xd6\xb8\xd7\x90\xd6\xb8\xd7\xa8\xd6\xb6\xd7\xa5",
    /* Hebrew, with embedded LTR runs (from a Linux guide) */
    "\xd7\x94\xd7\xa7\xd7\x9c\xd7\x93\xd7\x94 \xd7\xa2\xd7\x9c \xd7\xa9\xd7\xa0\xd7\x99 \xd7\x94 SHIFT\xd7\x99\xd7\x9d (\xd7\x99\xd7\x9e\xd7\x99\xd7\x9f \xd7\x95\xd7\xa9\xd7\x9e\xd7\x90\xd7\x9c \xd7\x91\xd7\x99\xd7\x97\xd7\x93) \xd7\x90\xd7\x9e\xd7\x95\xd7\xa8\xd7\x99\xd7\x9d \xd7\x9c\xd7\x94\xd7\x93\xd7\x9c\xd7\x99\xd7\xa7 \xd7\x90\xd7\xaa \xd7\xa0\xd7\x95\xd7\xa8\xd7\xaa \xd7\x94 Scroll Lock , \xd7\x95\xd7\x9c\xd7\x94\xd7\xa2\xd7\x91\xd7\x99\xd7\xa8 \xd7\x90\xd7\x95\xd7\xaa\xd7\xa0\xd7\x95 \xd7\x9c\xd7\x9e\xd7\xa6\xd7\x91 \xd7\x9b\xd7\xaa\xd7\x99\xd7\x91\xd7\x94 \xd7\x91\xd7\xa2\xd7\x91\xd7\xa8\xd7\x99\xd7\xaa.",
    /* Different line terminators */
    "AAAA\nBBBB\nCCCC\n",
    "DDDD\rEEEE\rFFFF\r",
    "GGGG\r\nHHHH\r\nIIII\r\n",
    NULL
  };

static void
iter_char_test (PangoLayout *layout)
{
  PangoRectangle   extents, run_extents;
  PangoLayoutIter *iter;
  PangoLayoutRun  *run;
  int              num_chars;
  int              i, index, offset;
  int              leading_x, trailing_x, x0, x1;
  gboolean         iter_next_ok, rtl;
  const char      *text, *ptr;

  text = pango_layout_get_text (layout);
  num_chars = g_utf8_strlen (text, -1);

  iter = pango_layout_get_iter (layout);
  iter_next_ok = TRUE;

  for (i = 0 ; i < num_chars; ++i)
    {
      gchar *char_str;
      g_assert (iter_next_ok);

      index = pango_layout_iter_get_index (iter);
      ptr = text + index;
      char_str = g_strndup (ptr, g_utf8_next_char (ptr) - ptr);
      verbose ("i=%d (visual), index = %d '%s':\n",
	       i, index, char_str);
      g_free (char_str);

      pango_layout_iter_get_char_extents (iter, &extents);
      verbose ("  char extents: x=%d,y=%d w=%d,h=%d\n",
	       extents.x, extents.y,
	       extents.width, extents.height);

      run = pango_layout_iter_get_run (iter);

      if (run)
	{
	  /* Get needed data for the GlyphString */
	  pango_layout_iter_get_run_extents(iter, NULL, &run_extents);
	  offset = run->item->offset;
	  rtl = run->item->analysis.level%2;
	  verbose ("  (current run: offset=%d,x=%d,len=%d,rtl=%d)\n",
		   offset, run_extents.x, run->item->length, rtl);

	  /* Calculate expected x result using index_to_x */
	  pango_glyph_string_index_to_x (run->glyphs,
					 (char *)(text + offset), run->item->length,
					 &run->item->analysis,
					 index - offset, FALSE, &leading_x);
	  pango_glyph_string_index_to_x (run->glyphs,
					 (char *)(text + offset), run->item->length,
					 &run->item->analysis,
					 index - offset, TRUE, &trailing_x);

	  x0 = run_extents.x + MIN (leading_x, trailing_x);
	  x1 = run_extents.x + MAX (leading_x, trailing_x);

	  verbose ("  (index_to_x ind=%d: expected x=%d, width=%d)\n",
		   index - offset, x0, x1 - x0);

	  g_assert (extents.x == x0);
	  g_assert (extents.width == x1 - x0);
	}
      else
	{
	  /* We're on a line terminator */
	}

      iter_next_ok = pango_layout_iter_next_char (iter);
      verbose ("more to go? %d\n", iter_next_ok);
    }

  /* There should be one character position iterator for each character in the
   * input string */
  g_assert (!iter_next_ok);

  pango_layout_iter_free (iter);
}

/* char iteration test:
 *  - Total num of iterations match number of chars
 *  - GlyphString's index_to_x positions match those returned by the Iter
 */
static void
iter_cluster_test (PangoLayout *layout)
{
  PangoRectangle   extents;
  PangoLayoutIter *iter;
  int              index;
  gboolean         iter_next_ok;
  const char      *text;
  PangoLayoutLine *last_line = NULL;
  int              expected_next_x = 0;

  text = pango_layout_get_text (layout);

  iter = pango_layout_get_iter (layout);
  iter_next_ok = TRUE;

  while (iter_next_ok)
    {
      PangoLayoutLine *line = pango_layout_iter_get_line (iter);

      /* Every cluster is part of a run */
      g_assert (pango_layout_iter_get_run (iter));

      index = pango_layout_iter_get_index (iter);

      pango_layout_iter_get_cluster_extents (iter, NULL, &extents);

      iter_next_ok = pango_layout_iter_next_cluster (iter);

      verbose ("index = %d:\n", index);
      verbose ("  cluster extents: x=%d,y=%d w=%d,h=%d\n",
	       extents.x, extents.y,
	       extents.width, extents.height);
      verbose ("more to go? %d\n", iter_next_ok);

      /* All the clusters on a line should be next to each other and occupy
       * the entire line. They advance linearly from left to right */
      g_assert (extents.width >= 0);

      if (last_line == line)
	g_assert (extents.x == expected_next_x);

      expected_next_x = extents.x + extents.width;

      last_line = line;
    }

  g_assert (!iter_next_ok);

  pango_layout_iter_free (iter);
}

int
main (int argc, char *argv[])
{
  const char  **ptext;
  PangoFontMap *fontmap;
  PangoContext *context;
  PangoLayout  *layout;

  g_setenv ("PANGO_RC_FILE", "./pangorc", TRUE);

  fontmap = pango_cairo_font_map_get_default ();
  context = pango_cairo_font_map_create_context (PANGO_CAIRO_FONT_MAP (fontmap));

  layout = pango_layout_new (context);
  pango_layout_set_width (layout, LAYOUT_WIDTH);

  for (ptext = test_texts; *ptext != NULL; ++ptext)
    {
      verbose ("--------- checking next text ----------\n");
      verbose (" <%s>\n", *ptext);
      verbose ( "len=%ld, bytes=%ld\n",
		(long)g_utf8_strlen (*ptext, -1), (long)strlen (*ptext));

      pango_layout_set_text (layout, *ptext, -1);
      iter_char_test (layout);
      iter_cluster_test (layout);
    }

  g_object_unref (layout);
  return 0;
}
