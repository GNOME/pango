/* Pango
 * bench-itemize.c: Benchmark pango_itemize with cold fontconfig caches
 *
 * Copyright (C) 2026 Red Hat, Inc
 *
 * SPDX-License-Identifier: LGPL-2.1-or-later
 *
 * Measures the cost of the first itemize call (cold font caches) for
 * different script mixes.  The number of system fonts (n) is what
 * drives fontconfig sort cost; the script mix controls how many
 * positions we extract from the sorted set (k).
 */

#include <glib.h>
#include <pango/pangocairo.h>
#include <locale.h>
#include <string.h>
#include <stdio.h>

static const char *texts[] = {
  /* Pure Latin — single font, k=1 */
  "The quick brown fox jumps over the lazy dog.",

  /* Latin + Cyrillic — 2 fonts */
  "Hello \xd0\x9f\xd1\x80\xd0\xb8\xd0\xb2\xd0\xb5\xd1\x82 world",

  /* Latin + CJK + Arabic — 3 fonts */
  "Hello \xe4\xbd\xa0\xe5\xa5\xbd \xd9\x85\xd8\xb1\xd8\xad\xd8\xa8\xd8\xa7 world",

  /* Many scripts — exercises deep extraction from the sorted set */
  "Hello "                                                      /* Latin */
  "\xd0\x9f\xd1\x80\xd0\xb8\xd0\xb2\xd0\xb5\xd1\x82 "         /* Cyrillic */
  "\xe4\xbd\xa0\xe5\xa5\xbd "                                   /* CJK */
  "\xe0\xa4\xa8\xe0\xa4\xae\xe0\xa4\xb8\xe0\xa5\x8d\xe0\xa4\xa4\xe0\xa5\x87 "  /* Devanagari */
  "\xd7\xa9\xd7\x9c\xd7\x95\xd7\x9d "                          /* Hebrew */
  "\xec\x95\x88\xeb\x85\x95 "                                   /* Korean */
  "\xe0\xb8\xaa\xe0\xb8\xa7\xe0\xb8\xb1\xe0\xb8\xaa\xe0\xb8\x94\xe0\xb8\xb5 " /* Thai */
  "\xd9\x85\xd8\xb1\xd8\xad\xd8\xa8\xd8\xa7",                 /* Arabic */
};

static const char *labels[] = {
  "latin-only",
  "latin+cyrillic",
  "latin+cjk+arabic",
  "8-scripts",
};

static double
bench_one_cold (const char *text)
{
  PangoFontMap *fontmap;
  PangoContext *context;
  PangoFontDescription *desc;
  gint64 start, end;
  GList *items;

  fontmap = pango_cairo_font_map_new ();
  context = pango_font_map_create_context (fontmap);
  desc = pango_font_description_from_string ("sans 12");
  pango_context_set_font_description (context, desc);

  start = g_get_monotonic_time ();

  items = pango_itemize (context, text, 0, strlen (text), NULL, NULL);

  end = g_get_monotonic_time ();

  g_list_free_full (items, (GDestroyNotify)pango_item_free);
  pango_font_description_free (desc);
  g_object_unref (context);
  g_object_unref (fontmap);

  return (double)(end - start);
}

int
main (int argc, char *argv[])
{
  int iterations = 20;
  int i, j;

  setlocale (LC_ALL, "");

  printf ("%-22s %6s %8s %8s %8s\n", "test", "items", "min µs", "med µs", "mean µs");
  printf ("%-22s %6s %8s %8s %8s\n", "----", "-----", "------", "------", "-------");

  for (i = 0; i < (int)G_N_ELEMENTS (texts); i++)
    {
      double times[20];
      double sum = 0;
      double min_t;
      int n_items;

      /* Count items (warm, doesn't matter for count) */
      {
        PangoFontMap *fontmap = pango_cairo_font_map_get_default ();
        PangoContext *context = pango_font_map_create_context (fontmap);
        PangoFontDescription *desc = pango_font_description_from_string ("sans 12");
        GList *items;

        pango_context_set_font_description (context, desc);
        items = pango_itemize (context, texts[i], 0, strlen (texts[i]), NULL, NULL);
        n_items = g_list_length (items);
        g_list_free_full (items, (GDestroyNotify)pango_item_free);
        pango_font_description_free (desc);
        g_object_unref (context);
      }

      for (j = 0; j < iterations; j++)
        {
          times[j] = bench_one_cold (texts[i]);
          sum += times[j];
        }

      /* Sort for min and median */
      for (j = 0; j < iterations - 1; j++)
        for (int k = j + 1; k < iterations; k++)
          if (times[k] < times[j])
            { double t = times[j]; times[j] = times[k]; times[k] = t; }

      min_t = times[0];

      printf ("%-22s %6d %8.0f %8.0f %8.0f\n",
              labels[i], n_items, min_t,
              times[iterations / 2],
              sum / iterations);
    }

  return 0;
}
