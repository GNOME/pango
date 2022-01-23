#include "config.h"

#include "pango-layout-run-private.h"
#include "pango-item-private.h"
#include "pango-impl-utils.h"

#include <math.h>

/**
 * PangoLayoutRun:
 *
 * A `PangoLayoutRun` represents a single run within a `PangoLayoutLine`.
 *
 * A run is a range of text with uniform script, font and attributes that
 * is shaped as a unit.
 *
 * Script, font and attributes of a run can be accessed via
 * [method@Pango.LayoutRun.get_item]. The glyphs that result from shaping
 * the text of the run can be obtained via [method@Pango.LayoutRun.get_glyphs].
 */

/**
 * pango_layout_run_get_item:
 * @run: a `PangoLayoutRun`
 *
 * Gets the `PangoItem` for the run.
 *
 * Returns: (transfer none): the `PangoItem` of @run
 */
PangoItem *
pango_layout_run_get_item (PangoLayoutRun *run)
{
  return run->item;
}

/**
 * pango_layout_run_get_glyphs:
 * @run: a `PangoLayoutRun`
 *
 * Gets the `PangoGlyphString` for the run.
 *
 * Returns: (transfer none): the `PangoGlyphString` of @run
 */
PangoGlyphString *
pango_layout_run_get_glyphs (PangoLayoutRun *run)
{
  return run->glyphs;
}

/**
 * pango_layout_run_get_extents:
 * @run: a `PangoLayoutRun`
 * @trim: `PangoLeadingTrim` flags
 * @ink_rect: (out caller-allocates) (optional): return location
 *   for the ink extents
 * @logical_rect: (out caller-allocates) (optional): return location
 *   for the logical extents
 *
 * Gets the extents of a `PangoLayoutRun`.
 *
 * The @trim flags specify if line-height attributes are taken
 * into consideration for determining the logical height. See the
 * [CSS inline layout](https://www.w3.org/TR/css-inline-3/#inline-height)
 * specification for details.
 */
void
pango_layout_run_get_extents (PangoLayoutRun   *run,
                              PangoLeadingTrim  trim,
                              PangoRectangle   *ink_rect,
                              PangoRectangle   *logical_rect)
{
  PangoGlyphItem *glyph_item = run;
  ItemProperties properties;
  gboolean has_underline;
  gboolean has_overline;
  PangoRectangle logical;
  PangoFontMetrics *metrics = NULL;
  int y_offset;

  pango_item_get_properties (glyph_item->item, &properties);

  has_underline = properties.uline_single || properties.uline_double ||
                  properties.uline_low || properties.uline_error;
  has_overline = properties.oline_single;

  if (!logical_rect && (glyph_item->item->analysis.flags & PANGO_ANALYSIS_FLAG_CENTERED_BASELINE))
    logical_rect = &logical;

  if (!logical_rect && (has_underline || has_overline || properties.strikethrough))
    logical_rect = &logical;

  if (properties.shape_set)
    _pango_shape_get_extents (glyph_item->item->num_chars,
                              properties.shape_ink_rect, properties.shape_logical_rect,
                              ink_rect, logical_rect);
  else
    pango_glyph_string_extents (glyph_item->glyphs, glyph_item->item->analysis.font,
                                ink_rect, logical_rect);

  if (ink_rect && (has_underline || has_overline || properties.strikethrough))
    {
      int underline_thickness;
      int underline_position;
      int strikethrough_thickness;
      int strikethrough_position;
      int new_pos;

      if (!metrics)
        metrics = pango_font_get_metrics (glyph_item->item->analysis.font,
                                          glyph_item->item->analysis.language);

      underline_thickness = pango_font_metrics_get_underline_thickness (metrics);
      underline_position = pango_font_metrics_get_underline_position (metrics);
      strikethrough_thickness = pango_font_metrics_get_strikethrough_thickness (metrics);
      strikethrough_position = pango_font_metrics_get_strikethrough_position (metrics);

      /* the underline/strikethrough takes x, width of logical_rect.
       * Reflect that into ink_rect.
       */
      new_pos = MIN (ink_rect->x, logical_rect->x);
      ink_rect->width = MAX (ink_rect->x + ink_rect->width, logical_rect->x + logical_rect->width) - new_pos;
      ink_rect->x = new_pos;

      /* We should better handle the case of height==0 in the following cases.
       * If ink_rect->height == 0, we should adjust ink_rect->y appropriately.
       */

      if (properties.strikethrough)
        {
          if (ink_rect->height == 0)
            {
              ink_rect->height = strikethrough_thickness;
              ink_rect->y = -strikethrough_position;
            }
        }

      if (properties.oline_single)
        {
          ink_rect->y -= underline_thickness;
          ink_rect->height += underline_thickness;
        }

      if (properties.uline_low)
        ink_rect->height += 2 * underline_thickness;
      if (properties.uline_single)
        ink_rect->height = MAX (ink_rect->height,
                               underline_thickness - underline_position - ink_rect->y);
      if (properties.uline_double)
        ink_rect->height = MAX (ink_rect->height,
                                 3 * underline_thickness - underline_position - ink_rect->y);
      if (properties.uline_error)
        ink_rect->height = MAX (ink_rect->height,
                               3 * underline_thickness - underline_position - ink_rect->y);
    }

  y_offset = glyph_item->y_offset;

  if (glyph_item->item->analysis.flags & PANGO_ANALYSIS_FLAG_CENTERED_BASELINE)
    {
      gboolean is_hinted = (logical_rect->y & logical_rect->height & (PANGO_SCALE - 1)) == 0;
      int adjustment = logical_rect->y + logical_rect->height / 2;

      if (is_hinted)
        adjustment = PANGO_UNITS_ROUND (adjustment);

      y_offset += adjustment;
    }

  if (ink_rect)
    ink_rect->y -= y_offset;

  if (logical_rect)
    logical_rect->y -= y_offset;

  if (logical_rect && trim != PANGO_LEADING_TRIM_BOTH)
    {
      int leading;

      if (properties.absolute_line_height != 0 || properties.line_height != 0.0)
        {
          int line_height;

          line_height = MAX (properties.absolute_line_height, ceilf (properties.line_height * logical_rect->height));
          leading = (line_height - logical_rect->height);
        }
      else
        {
          /* line-height 'normal' in the CSS inline layout spec */
          if (!metrics)
            metrics = pango_font_get_metrics (glyph_item->item->analysis.font,
                                              glyph_item->item->analysis.language);
          leading = MAX (metrics->height - (metrics->ascent + metrics->descent), 0);
        }
      if ((trim & PANGO_LEADING_TRIM_START) == 0)
        logical_rect->y -= leading / 2;
      if (trim == PANGO_LEADING_TRIM_NONE)
        logical_rect->height += leading;
      else
        logical_rect->height += (leading - leading / 2);
    }

  if (metrics)
    pango_font_metrics_unref (metrics);
}
