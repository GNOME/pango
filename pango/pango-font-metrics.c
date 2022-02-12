/* Pango
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


G_DEFINE_BOXED_TYPE (PangoFontMetrics, pango_font_metrics,
                     pango_font_metrics_ref,
                     pango_font_metrics_unref);

/**
 * pango_font_metrics_new:
 *
 * Creates a new `PangoFontMetrics` structure.
 *
 * This is only for internal use by Pango backends and there is
 * no public way to set the fields of the structure.
 *
 * Return value: a newly-created `PangoFontMetrics` structure
 *   with a reference count of 1.
 */
PangoFontMetrics *
pango_font_metrics_new (void)
{
  PangoFontMetrics *metrics = g_slice_new0 (PangoFontMetrics);
  metrics->ref_count = 1;

  return metrics;
}

/**
 * pango_font_metrics_ref:
 * @metrics: (nullable): a `PangoFontMetrics` structure, may be %NULL
 *
 * Increase the reference count of a font metrics structure by one.
 *
 * Return value: (nullable): @metrics
 */
PangoFontMetrics *
pango_font_metrics_ref (PangoFontMetrics *metrics)
{
  if (metrics == NULL)
    return NULL;

  g_atomic_int_inc ((int *) &metrics->ref_count);

  return metrics;
}

/**
 * pango_font_metrics_unref:
 * @metrics: (nullable): a `PangoFontMetrics` structure, may be %NULL
 *
 * Decrease the reference count of a font metrics structure by one.
 *
 * If the result is zero, frees the structure and any associated memory.
 */
void
pango_font_metrics_unref (PangoFontMetrics *metrics)
{
  if (metrics == NULL)
    return;

  g_return_if_fail (metrics->ref_count > 0 );

  if (g_atomic_int_dec_and_test ((int *) &metrics->ref_count))
    g_slice_free (PangoFontMetrics, metrics);
}

/**
 * pango_font_metrics_get_ascent:
 * @metrics: a `PangoFontMetrics` structure
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
pango_font_metrics_get_ascent (PangoFontMetrics *metrics)
{
  g_return_val_if_fail (metrics != NULL, 0);

  return metrics->ascent;
}

/**
 * pango_font_metrics_get_descent:
 * @metrics: a `PangoFontMetrics` structure
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
pango_font_metrics_get_descent (PangoFontMetrics *metrics)
{
  g_return_val_if_fail (metrics != NULL, 0);

  return metrics->descent;
}

/**
 * pango_font_metrics_get_height:
 * @metrics: a `PangoFontMetrics` structure
 *
 * Gets the line height from a font metrics structure.
 *
 * The line height is the recommended distance between successive
 * baselines in wrapped text using this font.
 *
 * If the line height is not available, 0 is returned.
 *
 * Return value: the height, in Pango units
 *
 * Since: 1.44
 */
int
pango_font_metrics_get_height (PangoFontMetrics *metrics)
{
  g_return_val_if_fail (metrics != NULL, 0);

  return metrics->height;
}

/**
 * pango_font_metrics_get_approximate_char_width:
 * @metrics: a `PangoFontMetrics` structure
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
pango_font_metrics_get_approximate_char_width (PangoFontMetrics *metrics)
{
  g_return_val_if_fail (metrics != NULL, 0);

  return metrics->approximate_char_width;
}

/**
 * pango_font_metrics_get_approximate_digit_width:
 * @metrics: a `PangoFontMetrics` structure
 *
 * Gets the approximate digit width for a font metrics structure.
 *
 * This is merely a representative value useful, for example, for
 * determining the initial size for a window. Actual digits in
 * text can be wider or narrower than this, though this value
 * is generally somewhat more accurate than the result of
 * pango_font_metrics_get_approximate_char_width() for digits.
 *
 * Return value: the digit width, in Pango units.
 */
int
pango_font_metrics_get_approximate_digit_width (PangoFontMetrics *metrics)
{
  g_return_val_if_fail (metrics != NULL, 0);

  return metrics->approximate_digit_width;
}

/**
 * pango_font_metrics_get_underline_position:
 * @metrics: a `PangoFontMetrics` structure
 *
 * Gets the suggested position to draw the underline.
 *
 * The value returned is the distance *above* the baseline of the top
 * of the underline. Since most fonts have underline positions beneath
 * the baseline, this value is typically negative.
 *
 * Return value: the suggested underline position, in Pango units.
 *
 * Since: 1.6
 */
int
pango_font_metrics_get_underline_position (PangoFontMetrics *metrics)
{
  g_return_val_if_fail (metrics != NULL, 0);

  return metrics->underline_position;
}

/**
 * pango_font_metrics_get_underline_thickness:
 * @metrics: a `PangoFontMetrics` structure
 *
 * Gets the suggested thickness to draw for the underline.
 *
 * Return value: the suggested underline thickness, in Pango units.
 *
 * Since: 1.6
 */
int
pango_font_metrics_get_underline_thickness (PangoFontMetrics *metrics)
{
  g_return_val_if_fail (metrics != NULL, 0);

  return metrics->underline_thickness;
}

/**
 * pango_font_metrics_get_strikethrough_position:
 * @metrics: a `PangoFontMetrics` structure
 *
 * Gets the suggested position to draw the strikethrough.
 *
 * The value returned is the distance *above* the
 * baseline of the top of the strikethrough.
 *
 * Return value: the suggested strikethrough position, in Pango units.
 *
 * Since: 1.6
 */
int
pango_font_metrics_get_strikethrough_position (PangoFontMetrics *metrics)
{
  g_return_val_if_fail (metrics != NULL, 0);

  return metrics->strikethrough_position;
}

/**
 * pango_font_metrics_get_strikethrough_thickness:
 * @metrics: a `PangoFontMetrics` structure
 *
 * Gets the suggested thickness to draw for the strikethrough.
 *
 * Return value: the suggested strikethrough thickness, in Pango units.
 *
 * Since: 1.6
 */
int
pango_font_metrics_get_strikethrough_thickness (PangoFontMetrics *metrics)
{
  g_return_val_if_fail (metrics != NULL, 0);

  return metrics->strikethrough_thickness;
}
