/* Pango2
 * pango-fontset.c:
 *
 * Copyright (C) 2001 Red Hat Software
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

/**
 * Pango2Fontset:
 *
 * Represents a set of fonts to use when rendering text.
 *
 * A `Pango2Fontset` is the result of resolving a `Pango2FontDescription`
 * against a particular `Pango2Context`. It has operations for finding the
 * component font for a particular Unicode character, and for finding a
 * composite set of metrics for the entire fontset.
 *
 * To obtain a `Pango2Fontset`, use [method@Pango2.Context.load_fontset] or
 * [method@Pango2.FontMap.load_fontset].
 */

#include "pango-fontset-private.h"
#include "pango-types.h"
#include "pango-font-metrics-private.h"
#include "pango-impl-utils.h"

static Pango2FontMetrics *pango2_fontset_real_get_metrics (Pango2Fontset *fontset);

G_DEFINE_ABSTRACT_TYPE (Pango2Fontset, pango2_fontset, G_TYPE_OBJECT);

static void
pango2_fontset_init (Pango2Fontset *self)
{
}

static void
pango2_fontset_class_init (Pango2FontsetClass *class)
{
  class->get_metrics = pango2_fontset_real_get_metrics;
}


/**
 * pango2_fontset_get_font:
 * @fontset: a `Pango2Fontset`
 * @wc: a Unicode character
 *
 * Returns the font in the fontset that contains the best
 * glyph for a Unicode character.
 *
 * Return value: (transfer full): a `Pango2Font`
 */
Pango2Font *
pango2_fontset_get_font (Pango2Fontset *fontset,
                         guint          wc)
{

  g_return_val_if_fail (PANGO2_IS_FONTSET (fontset), NULL);

  return PANGO2_FONTSET_GET_CLASS (fontset)->get_font (fontset, wc);
}

/**
 * pango2_fontset_get_metrics:
 * @fontset: a `Pango2Fontset`
 *
 * Get overall metric information for the fonts in the fontset.
 *
 * Return value: a `Pango2FontMetrics` object
 */
Pango2FontMetrics *
pango2_fontset_get_metrics (Pango2Fontset *fontset)
{
  g_return_val_if_fail (PANGO2_IS_FONTSET (fontset), NULL);

  return PANGO2_FONTSET_GET_CLASS (fontset)->get_metrics (fontset);
}

/*< private >
 * pango2_fontset_get_language:
 * @fontset: a `Pango2Fontset`
 *
 * Gets the language that the fontset was created for.
 *
 * Returns: the language that @fontset was created for
 */
Pango2Language *
pango2_fontset_get_language (Pango2Fontset *fontset)
{
  g_return_val_if_fail (PANGO2_IS_FONTSET (fontset), NULL);

  return PANGO2_FONTSET_GET_CLASS (fontset)->get_language (fontset);
}

/**
 * pango2_fontset_foreach:
 * @fontset: a `Pango2Fontset`
 * @func: (closure data) (scope call): Callback function
 * @data: data to pass to the callback function
 *
 * Iterates through all the fonts in a fontset, calling @func for
 * each one.
 *
 * If @func returns true, that stops the iteration.
 */
void
pango2_fontset_foreach (Pango2Fontset            *fontset,
                        Pango2FontsetForeachFunc  func,
                        gpointer                  data)
{
  g_return_if_fail (PANGO2_IS_FONTSET (fontset));
  g_return_if_fail (func != NULL);

  PANGO2_FONTSET_GET_CLASS (fontset)->foreach (fontset, func, data);
}

static gboolean
get_first_metrics_foreach (Pango2Fontset *fontset,
                           Pango2Font    *font,
                           gpointer       data)
{
  Pango2FontMetrics **fontset_metrics = data;
  Pango2Language *language = pango2_fontset_get_language (fontset);

  *fontset_metrics = pango2_font_get_metrics (font, language);

  return TRUE; /* Stops iteration */
}

static Pango2FontMetrics *
pango2_fontset_real_get_metrics (Pango2Fontset *fontset)
{
  Pango2FontMetrics *metrics, *raw_metrics;
  const char *sample_str;
  const char *p;
  int count;
  GHashTable *fonts_seen;
  Pango2Font *font;
  Pango2Language *language;

  language = pango2_fontset_get_language (fontset);
  sample_str = pango2_language_get_sample_string (language);

  count = 0;
  metrics = NULL;
  fonts_seen = g_hash_table_new_full (NULL, NULL, g_object_unref, NULL);

  /* Initialize the metrics from the first font in the fontset */
  pango2_fontset_foreach (fontset, get_first_metrics_foreach, &metrics);

  p = sample_str;
  while (*p)
    {
      gunichar wc = g_utf8_get_char (p);
      font = pango2_fontset_get_font (fontset, wc);
      if (font)
        {
          if (g_hash_table_lookup (fonts_seen, font) == NULL)
            {
              raw_metrics = pango2_font_get_metrics (font, language);
              g_hash_table_insert (fonts_seen, font, font);

              if (count == 0)
                {
                  metrics->ascent = raw_metrics->ascent;
                  metrics->descent = raw_metrics->descent;
                  metrics->approximate_char_width = raw_metrics->approximate_char_width;
                  metrics->approximate_digit_width = raw_metrics->approximate_digit_width;
                }
              else
                {
                  metrics->ascent = MAX (metrics->ascent, raw_metrics->ascent);
                  metrics->descent = MAX (metrics->descent, raw_metrics->descent);
                  metrics->approximate_char_width += raw_metrics->approximate_char_width;
                  metrics->approximate_digit_width += raw_metrics->approximate_digit_width;
                }
              count++;
              pango2_font_metrics_free (raw_metrics);
            }
          else
            g_object_unref (font);
        }

      p = g_utf8_next_char (p);
    }

  g_hash_table_destroy (fonts_seen);

  if (count)
    {
      metrics->approximate_char_width /= count;
      metrics->approximate_digit_width /= count;
    }

  return metrics;
}
