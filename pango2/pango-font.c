/* Pango2
 * fonts.c:
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
#include <stdlib.h>
#include <math.h>
#include <string.h>

#include <gio/gio.h>

#include "pango-types.h"
#include "pango-font-private.h"
#include "pango-font-metrics-private.h"
#include "pango-fontmap-private.h"
#include "pango-impl-utils.h"

#include <hb-gobject.h>

/**
 * Pango2Font:
 *
 * A `Pango2Font` is used to represent a font in a
 * rendering-system-independent manner.
 */

/* }}} */
/* {{{ Pango2Font implementation */

enum {
  PROP_FACE = 1,
  PROP_HB_FONT,
  PROP_SIZE,
  PROP_DPI,
  PROP_GRAVITY,
  N_PROPERTIES
};

static GParamSpec *properties[N_PROPERTIES] = { NULL, };

G_DEFINE_ABSTRACT_TYPE (Pango2Font, pango2_font, G_TYPE_OBJECT)

static void
pango2_font_finalize (GObject *object)
{
  Pango2Font *font = PANGO2_FONT (object);

  g_object_unref (font->face);
  hb_font_destroy (font->hb_font);

  G_OBJECT_CLASS (pango2_font_parent_class)->finalize (object);
}

static void
pango2_font_get_property (GObject    *object,
                          guint       property_id,
                          GValue     *value,
                          GParamSpec *pspec)
{
  Pango2Font *font = PANGO2_FONT (object);

  switch (property_id)
    {
    case PROP_FACE:
      g_value_set_object (value, font->face);
      break;

    case PROP_HB_FONT:
      g_value_set_boxed (value, pango2_font_get_hb_font (font));
      break;

    case PROP_SIZE:
      g_value_set_int (value, font->size);
      break;

    case PROP_DPI:
      g_value_set_float (value, font->dpi);
      break;

    case PROP_GRAVITY:
      g_value_set_enum (value, font->gravity);
      break;

    default:
      G_OBJECT_WARN_INVALID_PROPERTY_ID (object, property_id, pspec);
    }
}

static gboolean
pango2_font_default_is_hinted (Pango2Font *font)
{
  return FALSE;
}

static void
pango2_font_default_get_scale_factors (Pango2Font *font,
                                       double     *x_scale,
                                       double     *y_scale)
{
  *x_scale = *y_scale = 1.0;
}

static void
pango2_font_default_get_transform (Pango2Font   *font,
                                   Pango2Matrix *matrix)
{
  *matrix = (Pango2Matrix) PANGO2_MATRIX_INIT;
}

static void
pango2_font_class_init (Pango2FontClass *class G_GNUC_UNUSED)
{
  GObjectClass *object_class = G_OBJECT_CLASS (class);

  object_class->finalize = pango2_font_finalize;
  object_class->get_property = pango2_font_get_property;

  class->is_hinted = pango2_font_default_is_hinted;
  class->get_scale_factors = pango2_font_default_get_scale_factors;
  class->get_transform = pango2_font_default_get_transform;

  /**
   * Pango2Font:face: (attributes org.gtk.Property.get=pango2_font_get_face)
   *
   * The face to which the font belongs.
   */
  properties[PROP_FACE] =
      g_param_spec_object ("face", NULL, NULL,
                           PANGO2_TYPE_FONT_FACE,
                           G_PARAM_READABLE | G_PARAM_STATIC_STRINGS);

  /**
   * Pango2Font:hb-font: (attributes org.gtk.Property.get=pango2_font_get_hb_font)
   *
   * A `hb_font_t` object backing this font.
   */
  properties[PROP_HB_FONT] =
      g_param_spec_boxed ("hb-font", NULL, NULL,
                          HB_GOBJECT_TYPE_FONT,
                          G_PARAM_READABLE | G_PARAM_STATIC_STRINGS);

  /**
   * Pango2Font:size: (attributes org.gtk.Property.get=pango2_font_get_size)
   *
   * The size of the font, scaled by `PANGO2_SCALE`.
   */
  properties[PROP_SIZE] =
      g_param_spec_int ("size", NULL, NULL,
                        0, G_MAXINT, 0,
                        G_PARAM_READABLE | G_PARAM_STATIC_STRINGS);

  /**
   * Pango2Font:dpi:
   *
   * The resolution at which the font is rendered.
   *
   * The pixel size of the font is computed as
   *
   *     size * dpi / 72.
   */
  properties[PROP_DPI] =
      g_param_spec_float ("dpi", NULL, NULL,
                          0, G_MAXFLOAT, 96.0,
                          G_PARAM_READABLE | G_PARAM_STATIC_STRINGS);

  /**
   * Pango2Font:gravity: (attributes org.gtk.Property.get=pango2_font_get_gravity)
   *
   * The gravity of the font.
   */
  properties[PROP_GRAVITY] =
      g_param_spec_enum ("gravity", NULL, NULL,
                         PANGO2_TYPE_GRAVITY,
                         PANGO2_GRAVITY_AUTO,
                         G_PARAM_READABLE | G_PARAM_STATIC_STRINGS);

  g_object_class_install_properties (object_class, N_PROPERTIES, properties);
}

static void
pango2_font_init (Pango2Font *font)
{
  font->gravity = PANGO2_GRAVITY_AUTO;
  font->ctm = (Pango2Matrix) PANGO2_MATRIX_INIT;
}

/* }}} */
/* {{{ Private API */

/*< private >
 * pango2_font_get_transform:
 * @font: a `Pango2Font`
 * @matrix: the matrix to fill in
 *
 * Gets the matrix for the transformation from 'font space' to 'user space'.
 */
void
pango2_font_get_transform (Pango2Font   *font,
                           Pango2Matrix *matrix)
{
  PANGO2_FONT_GET_CLASS (font)->get_transform (font, matrix);
}

/*< private >
 * pango2_font_is_hinted:
 * @font: a `Pango2Font`
 *
 * Gets whether this font is hinted.
 *
 * Returns: %TRUE if @font is hinted
 */
gboolean
pango2_font_is_hinted (Pango2Font *font)
{
  return PANGO2_FONT_GET_CLASS (font)->is_hinted (font);
}

/*< private >
 * pango2_font_get_scale_factors:
 * @font: a `Pango2Font`
 * @x_scale: return location for X scale
 * @y_scale: return location for Y scale
 *
 * Gets the font scale factors of the ctm for this font.
 *
 * The ctm is the matrix set on the context that this font was
 * loaded for.
 */
void
pango2_font_get_scale_factors (Pango2Font *font,
                               double     *x_scale,
                               double     *y_scale)
{
  PANGO2_FONT_GET_CLASS (font)->get_scale_factors (font, x_scale, y_scale);
}

/* }}} */
/* {{{ Public API */

/**
 * pango2_font_describe:
 * @font: a `Pango2Font`
 *
 * Returns a description of the font, with font size set in points.
 *
 * Use [method@Pango2.Font.describe_with_absolute_size] if you want
 * the font size in device units.
 *
 * Return value: a newly-allocated `Pango2FontDescription` object.
 */
Pango2FontDescription *
pango2_font_describe (Pango2Font *font)
{
  g_return_val_if_fail (font != NULL, NULL);

  return PANGO2_FONT_GET_CLASS (font)->describe (font);
}

/**
 * pango2_font_describe_with_absolute_size:
 * @font: a `Pango2Font`
 *
 * Returns a description of the font, with absolute font size set
 * in device units.
 *
 * Use [method@Pango2.Font.describe] if you want the font size in points.
 *
 * Return value: a newly-allocated `Pango2FontDescription` object.
 */
Pango2FontDescription *
pango2_font_describe_with_absolute_size (Pango2Font *font)
{
  Pango2FontDescription *desc;

  g_return_val_if_fail (font != NULL, NULL);

  desc = pango2_font_describe (font);
  pango2_font_description_set_absolute_size (desc, font->size * font->dpi / 72.);

  return desc;
}

/**
 * pango2_font_get_glyph_extents:
 * @font: (nullable): a `Pango2Font`
 * @glyph: the glyph index
 * @ink_rect: (out) (optional): rectangle used to store the extents of the glyph as drawn
 * @logical_rect: (out) (optional): rectangle used to store the logical extents of the glyph
 *
 * Gets the logical and ink extents of a glyph within a font.
 *
 * The coordinate system for each rectangle has its origin at the
 * base line and horizontal origin of the character with increasing
 * coordinates extending to the right and down. The macros PANGO2_ASCENT(),
 * PANGO2_DESCENT(), PANGO2_LBEARING(), and PANGO2_RBEARING() can be used to convert
 * from the extents rectangle to more traditional font metrics. The units
 * of the rectangles are in 1/PANGO2_SCALE of a device unit.
 *
 * If @font is %NULL, this function gracefully sets some sane values in the
 * output variables and returns.
 */
void
pango2_font_get_glyph_extents (Pango2Font      *font,
                               Pango2Glyph      glyph,
                               Pango2Rectangle *ink_rect,
                               Pango2Rectangle *logical_rect)
{
  if (G_UNLIKELY (!font))
    {
      if (ink_rect)
        {
          ink_rect->x = PANGO2_SCALE;
          ink_rect->y = - (PANGO2_UNKNOWN_GLYPH_HEIGHT - 1) * PANGO2_SCALE;
          ink_rect->height = (PANGO2_UNKNOWN_GLYPH_HEIGHT - 2) * PANGO2_SCALE;
          ink_rect->width = (PANGO2_UNKNOWN_GLYPH_WIDTH - 2) * PANGO2_SCALE;
        }
      if (logical_rect)
        {
          logical_rect->x = 0;
          logical_rect->y = - PANGO2_UNKNOWN_GLYPH_HEIGHT * PANGO2_SCALE;
          logical_rect->height = PANGO2_UNKNOWN_GLYPH_HEIGHT * PANGO2_SCALE;
          logical_rect->width = PANGO2_UNKNOWN_GLYPH_WIDTH * PANGO2_SCALE;
        }
      return;
    }

  PANGO2_FONT_GET_CLASS (font)->get_glyph_extents (font, glyph, ink_rect, logical_rect);
}

/**
 * pango2_font_get_metrics:
 * @font: (nullable): a `Pango2Font`
 * @language: (nullable): language tag used to determine which script
 *   to get the metrics for, or %NULL to indicate to get the metrics for
 *   the entire font.
 *
 * Gets overall metric information for a font.
 *
 * Since the metrics may be substantially different for different scripts,
 * a language tag can be provided to indicate that the metrics should be
 * retrieved that correspond to the script(s) used by that language.
 *
 * If @font is %NULL, this function gracefully sets some sane values in the
 * output variables and returns.
 *
 * Return value: a `Pango2FontMetrics` object. The caller must call
 *   [method@Pango2.FontMetrics.free] when finished using the object.
 */
Pango2FontMetrics *
pango2_font_get_metrics (Pango2Font     *font,
                         Pango2Language *language)
{
  if (G_UNLIKELY (!font))
    {
      Pango2FontMetrics *metrics = pango2_font_metrics_new ();

      metrics->ascent = PANGO2_SCALE * PANGO2_UNKNOWN_GLYPH_HEIGHT;
      metrics->descent = 0;
      metrics->height = 0;
      metrics->approximate_char_width = PANGO2_SCALE * PANGO2_UNKNOWN_GLYPH_WIDTH;
      metrics->approximate_digit_width = PANGO2_SCALE * PANGO2_UNKNOWN_GLYPH_WIDTH;
      metrics->underline_position = -PANGO2_SCALE;
      metrics->underline_thickness = PANGO2_SCALE;
      metrics->strikethrough_position = PANGO2_SCALE * PANGO2_UNKNOWN_GLYPH_HEIGHT / 2;
      metrics->strikethrough_thickness = PANGO2_SCALE;

      return metrics;
    }

  return PANGO2_FONT_GET_CLASS (font)->get_metrics (font, language);
}

/**
 * pango2_font_get_face:
 * @font: a `Pango2Font`
 *
 * Gets the `Pango2FontFace` to which the font belongs.
 *
 * Returns: (transfer none): the `Pango2FontFace`
 */
Pango2FontFace *
pango2_font_get_face (Pango2Font *font)
{
  g_return_val_if_fail (PANGO2_IS_FONT (font), NULL);

  return font->face;
}

/**
 * pango2_font_get_hb_font:
 * @font: a `Pango2Font`
 *
 * Get a `hb_font_t` object backing this font.
 *
 * Note that the objects returned by this function are cached
 * and immutable. If you need to make changes to the `hb_font_t`,
 * use [hb_font_create_sub_font()](https://harfbuzz.github.io/harfbuzz-hb-font.html#hb-font-create-sub-font).
 *
 * Returns: (transfer none) (nullable): the `hb_font_t` object
 *   backing the font
 */
hb_font_t *
pango2_font_get_hb_font (Pango2Font *font)
{
  g_return_val_if_fail (PANGO2_IS_FONT (font), NULL);

  if (!font->hb_font)
    {
      font->hb_font = PANGO2_FONT_GET_CLASS (font)->create_hb_font (font);
      hb_font_make_immutable (font->hb_font);
    }

  return font->hb_font;
}

/**
 * pango2_font_get_size:
 * @font: a `Pango2Font`
 *
 * Returns the size of the font, scaled by `PANGO2_SCALE`.
 *
 * Return value: the size of the font
 */
int
pango2_font_get_size (Pango2Font *font)
{
  g_return_val_if_fail (PANGO2_IS_FONT (font), 0);

  return font->size;
}

/**
 * pango2_font_get_absolute_size:
 * @font: a `Pango2Font`
 *
 * Returns the pixel size of the font, scaled by `PANGO2_SCALE`.
 *
 * Return value: the pixel size of the font
 */
double
pango2_font_get_absolute_size (Pango2Font *font)
{
  g_return_val_if_fail (PANGO2_IS_FONT (font), 0.);

  return font->size * font->dpi / 72.;
}

/**
 * pango2_font_get_gravity:
 * @font: a `Pango2Font`
 *
 * Returns the gravity of the font.
 *
 * Return value: the gravity of the font
 */
Pango2Gravity
pango2_font_get_gravity (Pango2Font *font)
{
  g_return_val_if_fail (PANGO2_IS_FONT (font), PANGO2_GRAVITY_SOUTH);

  return font->gravity;
}

/* }}} */

/* vim:set foldmethod=marker expandtab: */
