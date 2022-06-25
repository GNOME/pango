#include "config.h"

#include "pango-run-private.h"
#include "pango-attr-private.h"
#include "pango-item-private.h"
#include "pango-impl-utils.h"
#include "pango-font-metrics-private.h"
#include "pango-attributes-private.h"

#include <math.h>

/**
 * Pango2Run:
 *
 * A `Pango2Run` represents a single run within a `Pango2Line`.
 *
 * A run is a range of text with uniform script, font and attributes that
 * is shaped as a unit.
 *
 * Script, font and attributes of a run can be accessed via
 * [method@Pango2.Run.get_item]. The glyphs that result from shaping
 * the text of the run can be obtained via [method@Pango2.Run.get_glyphs].
 */

/**
 * pango2_run_get_item:
 * @run: a `Pango2Run`
 *
 * Gets the `Pango2Item` for the run.
 *
 * Returns: (transfer none): the `Pango2Item` of @run
 */
Pango2Item *
pango2_run_get_item (Pango2Run *run)
{
  return run->glyph_item.item;
}

/**
 * pango2_run_get_glyphs:
 * @run: a `Pango2Run`
 *
 * Gets the `Pango2GlyphString` for the run.
 *
 * Returns: (transfer none): the `Pango2GlyphString` of @run
 */
Pango2GlyphString *
pango2_run_get_glyphs (Pango2Run *run)
{
  return run->glyph_item.glyphs;
}

static void
pango2_shape_get_extents (int              n_chars,
                          Pango2Attribute *attr,
                          Pango2Rectangle *ink_rect,
                          Pango2Rectangle *logical_rect)
{
  if (n_chars > 0)
    {
      ShapeData *data = (ShapeData *)attr->pointer_value;
      Pango2Rectangle *shape_ink = &data->ink_rect;
      Pango2Rectangle *shape_logical = &data->logical_rect;

      if (ink_rect)
        {
          ink_rect->x = MIN (shape_ink->x, shape_ink->x + shape_logical->width * (n_chars - 1));
          ink_rect->width = MAX (shape_ink->width, shape_ink->width + shape_logical->width * (n_chars - 1));
          ink_rect->y = shape_ink->y;
          ink_rect->height = shape_ink->height;
        }

      if (logical_rect)
        {
          logical_rect->x = MIN (shape_logical->x, shape_logical->x + shape_logical->width * (n_chars - 1));
          logical_rect->width = MAX (shape_logical->width, shape_logical->width + shape_logical->width * (n_chars - 1));
          logical_rect->y = shape_logical->y;
          logical_rect->height = shape_logical->height;
        }
    }
  else
    {
      if (ink_rect)
        {
          ink_rect->x = 0;
          ink_rect->y = 0;
          ink_rect->width = 0;
          ink_rect->height = 0;
        }

      if (logical_rect)
        {
          logical_rect->x = 0;
          logical_rect->y = 0;
          logical_rect->width = 0;
          logical_rect->height = 0;
        }
    }
}

/**
 * pango2_run_get_extents:
 * @run: a `Pango2Run`
 * @trim: `Pango2LeadingTrim` flags
 * @ink_rect: (out caller-allocates) (optional): return location
 *   for the ink extents
 * @logical_rect: (out caller-allocates) (optional): return location
 *   for the logical extents
 *
 * Gets the extents of a `Pango2Run`.
 *
 * The @trim flags specify if line-height attributes are taken
 * into consideration for determining the logical height. See the
 * [CSS inline layout](https://www.w3.org/TR/css-inline-3/#inline-height)
 * specification for details.
 */
void
pango2_run_get_extents (Pango2Run         *run,
                        Pango2LeadingTrim  trim,
                        Pango2Rectangle   *ink_rect,
                        Pango2Rectangle   *logical_rect)
{
  Pango2GlyphItem *glyph_item = &run->glyph_item;
  ItemProperties properties;
  gboolean has_underline;
  gboolean has_overline;
  gboolean has_strikethrough;
  Pango2Rectangle logical;
  Pango2FontMetrics *metrics = NULL;
  int y_offset;

  pango2_item_get_properties (glyph_item->item, &properties);

  has_underline = properties.uline_style != PANGO2_LINE_STYLE_NONE;
  has_overline = properties.oline_style != PANGO2_LINE_STYLE_NONE;
  has_strikethrough = properties.strikethrough_style != PANGO2_LINE_STYLE_NONE;

  if (!logical_rect && (glyph_item->item->analysis.flags & PANGO2_ANALYSIS_FLAG_CENTERED_BASELINE))
    logical_rect = &logical;

  if (!logical_rect && (has_underline || has_overline || has_strikethrough))
    logical_rect = &logical;

  if (properties.shape)
    pango2_shape_get_extents (glyph_item->item->num_chars, properties.shape,
                              ink_rect, logical_rect);
  else
    pango2_glyph_string_extents (glyph_item->glyphs, glyph_item->item->analysis.font,
                                 ink_rect, logical_rect);

  if (ink_rect && (has_underline || has_overline || has_strikethrough))
    {
      int underline_thickness;
      int underline_position;
      int strikethrough_thickness;
      int strikethrough_position;
      int new_pos;

      if (!metrics)
        metrics = pango2_font_get_metrics (glyph_item->item->analysis.font,
                                           glyph_item->item->analysis.language);

      underline_thickness = pango2_font_metrics_get_underline_thickness (metrics);
      underline_position = pango2_font_metrics_get_underline_position (metrics);
      strikethrough_thickness = pango2_font_metrics_get_strikethrough_thickness (metrics);
      strikethrough_position = pango2_font_metrics_get_strikethrough_position (metrics);

      /* the underline/strikethrough takes x, width of logical_rect.
       * Reflect that into ink_rect.
       */
      new_pos = MIN (ink_rect->x, logical_rect->x);
      ink_rect->width = MAX (ink_rect->x + ink_rect->width, logical_rect->x + logical_rect->width) - new_pos;
      ink_rect->x = new_pos;

      /* We should better handle the case of height==0 in the following cases.
       * If ink_rect->height == 0, we should adjust ink_rect->y appropriately.
       */

      if (has_strikethrough)
        {
          if (ink_rect->height == 0)
            {
              ink_rect->height = strikethrough_thickness;
              ink_rect->y = -strikethrough_position;
            }
        }

      if (properties.oline_style == PANGO2_LINE_STYLE_SOLID ||
          properties.oline_style == PANGO2_LINE_STYLE_DASHED ||
          properties.oline_style == PANGO2_LINE_STYLE_DOTTED)
        {
          ink_rect->y -= underline_thickness;
          ink_rect->height += underline_thickness;
        }
      else if (properties.oline_style == PANGO2_LINE_STYLE_DOUBLE ||
               properties.oline_style == PANGO2_LINE_STYLE_WAVY)
        {
          ink_rect->y -= 3 * underline_thickness;
          ink_rect->height += 3 * underline_thickness;
        }

      if (properties.uline_style == PANGO2_LINE_STYLE_SOLID ||
          properties.uline_style == PANGO2_LINE_STYLE_DASHED ||
          properties.uline_style == PANGO2_LINE_STYLE_DOTTED)
        {
          if (properties.uline_position == PANGO2_UNDERLINE_POSITION_UNDER)
            ink_rect->height += 2 * underline_thickness;
          else
            ink_rect->height = MAX (ink_rect->height,
                                    underline_thickness - underline_position - ink_rect->y);
        }
      else if (properties.uline_style == PANGO2_LINE_STYLE_DOUBLE ||
               properties.uline_style == PANGO2_LINE_STYLE_WAVY)
        {
          if (properties.uline_position == PANGO2_UNDERLINE_POSITION_UNDER)
            ink_rect->height += 4 * underline_thickness;
          else
            ink_rect->height = MAX (ink_rect->height,
                                    3 * underline_thickness - underline_position - ink_rect->y);
        }
    }

  y_offset = glyph_item->y_offset;

  if (glyph_item->item->analysis.flags & PANGO2_ANALYSIS_FLAG_CENTERED_BASELINE)
    {
      gboolean is_hinted = (logical_rect->y & logical_rect->height & (PANGO2_SCALE - 1)) == 0;
      int adjustment = logical_rect->y + logical_rect->height / 2;

      if (is_hinted)
        adjustment = PANGO2_UNITS_ROUND (adjustment);

      y_offset += adjustment;
    }

  if (ink_rect)
    ink_rect->y -= y_offset;

  if (logical_rect)
    logical_rect->y -= y_offset;

  if (logical_rect && trim != PANGO2_LEADING_TRIM_BOTH)
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
            metrics = pango2_font_get_metrics (glyph_item->item->analysis.font,
                                               glyph_item->item->analysis.language);
          leading = MAX (metrics->height - (metrics->ascent + metrics->descent) + properties.line_spacing, 0);
        }
      if ((trim & PANGO2_LEADING_TRIM_START) == 0)
        logical_rect->y -= leading / 2;
      if (trim == PANGO2_LEADING_TRIM_NONE)
        logical_rect->height += leading;
      else
        logical_rect->height += (leading - leading / 2);
    }

  if (metrics)
    pango2_font_metrics_free (metrics);
}
