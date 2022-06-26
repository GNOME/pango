/* Pango2
 * pango-font-metrics.c:
 *
 * Copyright (C) 1999 Red Hat Software
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

#include "pango-font-metrics-private.h"


G_DEFINE_BOXED_TYPE (Pango2FontMetrics, pango2_font_metrics,
                     pango2_font_metrics_copy,
                     pango2_font_metrics_free);

/**
 * pango2_font_metrics_new:
 *
 * Creates a new `Pango2FontMetrics` structure.
 *
 * This is only for internal use by Pango backends and there is
 * no public way to set the fields of the structure.
 *
 * Return value: a newly-created `Pango2FontMetrics` structure
 */
Pango2FontMetrics *
pango2_font_metrics_new (void)
{
  Pango2FontMetrics *metrics = g_slice_new0 (Pango2FontMetrics);

  return metrics;
}

/**
 * pango2_font_metrics_copy:
 * @metrics: (nullable): a `Pango2FontMetrics` structure, may be %NULL
 *
 * Create a copy of @metrics.
 *
 * Return value: (nullable): @metrics
 */
Pango2FontMetrics *
pango2_font_metrics_copy (Pango2FontMetrics *metrics)
{
  return g_slice_dup (Pango2FontMetrics, metrics);
}

/**
 * pango2_font_metrics_free:
 * @metrics: (nullable): a `Pango2FontMetrics` structure, may be %NULL
 *
 * Free the @metrics.
 */
void
pango2_font_metrics_free (Pango2FontMetrics *metrics)
{
  g_slice_free (Pango2FontMetrics, metrics);
}

/**
 * pango2_font_metrics_get_ascent:
 * @metrics: a `Pango2FontMetrics` structure
 *
 * Gets the ascent from a font metrics structure.
 *
 * The ascent is the distance from the baseline to the logical top
 * of a line of text. (The logical top may be above or below the top
 * of the actual drawn ink. It is necessary to lay out the text to
 * figure where the ink will be.)
 *
 * Return value: the ascent, in Pango units.
 */
int
pango2_font_metrics_get_ascent (Pango2FontMetrics *metrics)
{
  g_return_val_if_fail (metrics != NULL, 0);

  return metrics->ascent;
}

/**
 * pango2_font_metrics_get_descent:
 * @metrics: a `Pango2FontMetrics` structure
 *
 * Gets the descent from a font metrics structure.
 *
 * The descent is the distance from the baseline to the logical bottom
 * of a line of text. (The logical bottom may be above or below the
 * bottom of the actual drawn ink. It is necessary to lay out the text
 * to figure where the ink will be.)
 *
 * Return value: the descent, in Pango units.
 */
int
pango2_font_metrics_get_descent (Pango2FontMetrics *metrics)
{
  g_return_val_if_fail (metrics != NULL, 0);

  return metrics->descent;
}

/**
 * pango2_font_metrics_get_height:
 * @metrics: a `Pango2FontMetrics` structure
 *
 * Gets the line height from a font metrics structure.
 *
 * The line height is the recommended distance between successive
 * baselines in wrapped text using this font.
 *
 * If the line height is not available, 0 is returned.
 *
 * Return value: the height, in Pango units
 */
int
pango2_font_metrics_get_height (Pango2FontMetrics *metrics)
{
  g_return_val_if_fail (metrics != NULL, 0);

  return metrics->height;
}

/**
 * pango2_font_metrics_get_approximate_char_width:
 * @metrics: a `Pango2FontMetrics` structure
 *
 * Gets the approximate character width for a font metrics structure.
 *
 * This is merely a representative value useful, for example, for
 * determining the initial size for a window. Actual characters in
 * text will be wider and narrower than this.
 *
 * Return value: the character width, in Pango units.
 */
int
pango2_font_metrics_get_approximate_char_width (Pango2FontMetrics *metrics)
{
  g_return_val_if_fail (metrics != NULL, 0);

  return metrics->approximate_char_width;
}

/**
 * pango2_font_metrics_get_approximate_digit_width:
 * @metrics: a `Pango2FontMetrics` structure
 *
 * Gets the approximate digit width for a font metrics structure.
 *
 * This is merely a representative value useful, for example, for
 * determining the initial size for a window. Actual digits in
 * text can be wider or narrower than this, though this value
 * is generally somewhat more accurate than the result of
 * pango2_font_metrics_get_approximate_char_width() for digits.
 *
 * Return value: the digit width, in Pango units.
 */
int
pango2_font_metrics_get_approximate_digit_width (Pango2FontMetrics *metrics)
{
  g_return_val_if_fail (metrics != NULL, 0);

  return metrics->approximate_digit_width;
}

/**
 * pango2_font_metrics_get_underline_position:
 * @metrics: a `Pango2FontMetrics` structure
 *
 * Gets the suggested position to draw the underline.
 *
 * The value returned is the distance *above* the baseline of the top
 * of the underline. Since most fonts have underline positions beneath
 * the baseline, this value is typically negative.
 *
 * Return value: the suggested underline position, in Pango units.
 */
int
pango2_font_metrics_get_underline_position (Pango2FontMetrics *metrics)
{
  g_return_val_if_fail (metrics != NULL, 0);

  return metrics->underline_position;
}

/**
 * pango2_font_metrics_get_underline_thickness:
 * @metrics: a `Pango2FontMetrics` structure
 *
 * Gets the suggested thickness to draw for the underline.
 *
 * Return value: the suggested underline thickness, in Pango units.
 */
int
pango2_font_metrics_get_underline_thickness (Pango2FontMetrics *metrics)
{
  g_return_val_if_fail (metrics != NULL, 0);

  return metrics->underline_thickness;
}

/**
 * pango2_font_metrics_get_strikethrough_position:
 * @metrics: a `Pango2FontMetrics` structure
 *
 * Gets the suggested position to draw the strikethrough.
 *
 * The value returned is the distance *above* the
 * baseline of the top of the strikethrough.
 *
 * Return value: the suggested strikethrough position, in Pango units.
 */
int
pango2_font_metrics_get_strikethrough_position (Pango2FontMetrics *metrics)
{
  g_return_val_if_fail (metrics != NULL, 0);

  return metrics->strikethrough_position;
}

/**
 * pango2_font_metrics_get_strikethrough_thickness:
 * @metrics: a `Pango2FontMetrics` structure
 *
 * Gets the suggested thickness to draw for the strikethrough.
 *
 * Return value: the suggested strikethrough thickness, in Pango units.
 */
int
pango2_font_metrics_get_strikethrough_thickness (Pango2FontMetrics *metrics)
{
  g_return_val_if_fail (metrics != NULL, 0);

  return metrics->strikethrough_thickness;
}
