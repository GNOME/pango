/* Pango
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
 * PangoFont:
 *
 * A `PangoFont` is used to represent a font in a
 * rendering-system-independent manner.
 */

/* }}} */
/* {{{ PangoFont implementation */

enum {
  PROP_FACE = 1,
  PROP_HB_FONT,
  PROP_SIZE,
  PROP_DPI,
  PROP_GRAVITY,
  N_PROPERTIES
};

static GParamSpec *properties[N_PROPERTIES] = { NULL, };

G_DEFINE_ABSTRACT_TYPE (PangoFont, pango_font, G_TYPE_OBJECT)

static void
pango_font_finalize (GObject *object)
{
  PangoFont *font = PANGO_FONT (object);

  g_object_unref (font->face);
  hb_font_destroy (font->hb_font);

  G_OBJECT_CLASS (pango_font_parent_class)->finalize (object);
}

static void
pango_font_get_property (GObject    *object,
                         guint       property_id,
                         GValue     *value,
                         GParamSpec *pspec)
{
  PangoFont *font = PANGO_FONT (object);

  switch (property_id)
    {
    case PROP_FACE:
      g_value_set_object (value, font->face);
      break;

    case PROP_HB_FONT:
      g_value_set_boxed (value, pango_font_get_hb_font (font));
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
pango_font_default_is_hinted (PangoFont *font)
{
  return FALSE;
}

static void
pango_font_default_get_scale_factors (PangoFont *font,
                                      double    *x_scale,
                                      double    *y_scale)
{
  *x_scale = *y_scale = 1.0;
}

static void
pango_font_default_get_transform (PangoFont   *font,
                                  PangoMatrix *matrix)
{
  *matrix = (PangoMatrix) PANGO_MATRIX_INIT;
}

static void
pango_font_class_init (PangoFontClass *class G_GNUC_UNUSED)
{
  GObjectClass *object_class = G_OBJECT_CLASS (class);

  object_class->finalize = pango_font_finalize;
  object_class->get_property = pango_font_get_property;

  class->is_hinted = pango_font_default_is_hinted;
  class->get_scale_factors = pango_font_default_get_scale_factors;
  class->get_transform = pango_font_default_get_transform;

  /**
   * PangoFont:face: (attributes org.gtk.Property.get=pango_font_get_face)
   *
   * The face to which the font belongs.
   */
  properties[PROP_FACE] =
      g_param_spec_object ("face", NULL, NULL, PANGO_TYPE_FONT_FACE,
                           G_PARAM_READABLE | G_PARAM_STATIC_STRINGS);

  /**
   * PangoFont:hb-font: (attributes org.gtk.Property.get=pango_font_get_hb_font)
   *
   * A `hb_font_t` object backing this font.
   */
  properties[PROP_HB_FONT] =
      g_param_spec_boxed ("hb-font", NULL, NULL, HB_GOBJECT_TYPE_FONT,
                          G_PARAM_READABLE | G_PARAM_STATIC_STRINGS);

  /**
   * PangoFont:size: (attributes org.gtk.Property.get=pango_font_get_size)
   *
   * The size of the font, scaled by `PANGO_SCALE`.
   */
  properties[PROP_SIZE] =
      g_param_spec_int ("size", NULL, NULL, 0, G_MAXINT, 0,
                        G_PARAM_READABLE | G_PARAM_STATIC_STRINGS);

  /**
   * PangoFont:dpi:
   *
   * The resolution at which the font is rendered.
   *
   * The pixel size of the font is computed as
   *
   *     size * dpi / 72.
   */
  properties[PROP_DPI] =
      g_param_spec_float ("dpi", NULL, NULL, 0, G_MAXFLOAT, 96.0,
                          G_PARAM_READABLE | G_PARAM_STATIC_STRINGS);

  /**
   * PangoFont:gravity: (attributes org.gtk.Property.get=pango_font_get_gravity)
   *
   * The gravity of the font.
   */
  properties[PROP_GRAVITY] =
      g_param_spec_enum ("gravity", NULL, NULL, PANGO_TYPE_GRAVITY,
                         PANGO_GRAVITY_AUTO,
                         G_PARAM_READABLE | G_PARAM_STATIC_STRINGS);

  g_object_class_install_properties (object_class, N_PROPERTIES, properties);
}

static void
pango_font_init (PangoFont *font)
{
  font->gravity = PANGO_GRAVITY_AUTO;
  font->ctm = (PangoMatrix) PANGO_MATRIX_INIT;
}

/* }}} */
/* {{{ Private API */

/*< private >
 * pango_font_get_transform:
 * @font: a `PangoFont`
 * @matrix: the matrix to fill in
 *
 * Gets the matrix for the transformation from 'font space' to 'user space'.
 */
void
pango_font_get_transform (PangoFont   *font,
                          PangoMatrix *matrix)
{
  PANGO_FONT_GET_CLASS (font)->get_transform (font, matrix);
}

/*< private >
 * pango_font_is_hinted:
 * @font: a `PangoFont`
 *
 * Gets whether this font is hinted.
 *
 * Returns: %TRUE if @font is hinted
 */
gboolean
pango_font_is_hinted (PangoFont *font)
{
  return PANGO_FONT_GET_CLASS (font)->is_hinted (font);
}

/*< private >
 * pango_font_get_scale_factors:
 * @font: a `PangoFont`
 * @x_scale: return location for X scale
 * @y_scale: return location for Y scale
 *
 * Gets the font scale factors of the ctm for this font.
 *
 * The ctm is the matrix set on the context that this font was
 * loaded for.
 */
void
pango_font_get_scale_factors (PangoFont *font,
                              double    *x_scale,
                              double    *y_scale)
{
  PANGO_FONT_GET_CLASS (font)->get_scale_factors (font, x_scale, y_scale);
}

/* }}} */
/* {{{ Public API */

/**
 * pango_font_describe:
 * @font: a `PangoFont`
 *
 * Returns a description of the font, with font size set in points.
 *
 * Use [method@Pango.Font.describe_with_absolute_size] if you want
 * the font size in device units.
 *
 * Return value: a newly-allocated `PangoFontDescription` object.
 */
PangoFontDescription *
pango_font_describe (PangoFont *font)
{
  g_return_val_if_fail (font != NULL, NULL);

  return PANGO_FONT_GET_CLASS (font)->describe (font);
}

/**
 * pango_font_describe_with_absolute_size:
 * @font: a `PangoFont`
 *
 * Returns a description of the font, with absolute font size set
 * in device units.
 *
 * Use [method@Pango.Font.describe] if you want the font size in points.
 *
 * Return value: a newly-allocated `PangoFontDescription` object.
 */
PangoFontDescription *
pango_font_describe_with_absolute_size (PangoFont *font)
{
  PangoFontDescription *desc;

  g_return_val_if_fail (font != NULL, NULL);

  desc = pango_font_describe (font);
  pango_font_description_set_absolute_size (desc, font->size * font->dpi / 72.);

  return desc;
}

/**
 * pango_font_get_glyph_extents:
 * @font: (nullable): a `PangoFont`
 * @glyph: the glyph index
 * @ink_rect: (out) (optional): rectangle used to store the extents of the glyph as drawn
 * @logical_rect: (out) (optional): rectangle used to store the logical extents of the glyph
 *
 * Gets the logical and ink extents of a glyph within a font.
 *
 * The coordinate system for each rectangle has its origin at the
 * base line and horizontal origin of the character with increasing
 * coordinates extending to the right and down. The macros PANGO_ASCENT(),
 * PANGO_DESCENT(), PANGO_LBEARING(), and PANGO_RBEARING() can be used to convert
 * from the extents rectangle to more traditional font metrics. The units
 * of the rectangles are in 1/PANGO_SCALE of a device unit.
 *
 * If @font is %NULL, this function gracefully sets some sane values in the
 * output variables and returns.
 */
void
pango_font_get_glyph_extents (PangoFont      *font,
                              PangoGlyph      glyph,
                              PangoRectangle *ink_rect,
                              PangoRectangle *logical_rect)
{
  if (G_UNLIKELY (!font))
    {
      if (ink_rect)
        {
          ink_rect->x = PANGO_SCALE;
          ink_rect->y = - (PANGO_UNKNOWN_GLYPH_HEIGHT - 1) * PANGO_SCALE;
          ink_rect->height = (PANGO_UNKNOWN_GLYPH_HEIGHT - 2) * PANGO_SCALE;
          ink_rect->width = (PANGO_UNKNOWN_GLYPH_WIDTH - 2) * PANGO_SCALE;
        }
      if (logical_rect)
        {
          logical_rect->x = 0;
          logical_rect->y = - PANGO_UNKNOWN_GLYPH_HEIGHT * PANGO_SCALE;
          logical_rect->height = PANGO_UNKNOWN_GLYPH_HEIGHT * PANGO_SCALE;
          logical_rect->width = PANGO_UNKNOWN_GLYPH_WIDTH * PANGO_SCALE;
        }
      return;
    }

  PANGO_FONT_GET_CLASS (font)->get_glyph_extents (font, glyph, ink_rect, logical_rect);
}

/**
 * pango_font_get_metrics:
 * @font: (nullable): a `PangoFont`
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
 * Return value: a `PangoFontMetrics` object. The caller must call
 *   [method@Pango.FontMetrics.free] when finished using the object.
 */
PangoFontMetrics *
pango_font_get_metrics (PangoFont     *font,
                        PangoLanguage *language)
{
  if (G_UNLIKELY (!font))
    {
      PangoFontMetrics *metrics = pango_font_metrics_new ();

      metrics->ascent = PANGO_SCALE * PANGO_UNKNOWN_GLYPH_HEIGHT;
      metrics->descent = 0;
      metrics->height = 0;
      metrics->approximate_char_width = PANGO_SCALE * PANGO_UNKNOWN_GLYPH_WIDTH;
      metrics->approximate_digit_width = PANGO_SCALE * PANGO_UNKNOWN_GLYPH_WIDTH;
      metrics->underline_position = -PANGO_SCALE;
      metrics->underline_thickness = PANGO_SCALE;
      metrics->strikethrough_position = PANGO_SCALE * PANGO_UNKNOWN_GLYPH_HEIGHT / 2;
      metrics->strikethrough_thickness = PANGO_SCALE;

      return metrics;
    }

  return PANGO_FONT_GET_CLASS (font)->get_metrics (font, language);
}

/**
 * pango_font_get_face:
 * @font: a `PangoFont`
 *
 * Gets the `PangoFontFace` to which @font belongs.
 *
 * Returns: (transfer none): the `PangoFontFace`
 */
PangoFontFace *
pango_font_get_face (PangoFont *font)
{
  g_return_val_if_fail (PANGO_IS_FONT (font), NULL);

  return font->face;
}

/**
 * pango_font_get_hb_font:
 * @font: a `PangoFont`
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
pango_font_get_hb_font (PangoFont *font)
{
  g_return_val_if_fail (PANGO_IS_FONT (font), NULL);

  if (!font->hb_font)
    {
      font->hb_font = PANGO_FONT_GET_CLASS (font)->create_hb_font (font);
      hb_font_make_immutable (font->hb_font);
    }

  return font->hb_font;
}

/**
 * pango_font_get_size:
 * @font: a `PangoFont`
 *
 * Returns the size of the font, scaled by `PANGO_SCALE`.
 *
 * Return value: the size of the font
 */
int
pango_font_get_size (PangoFont *font)
{
  g_return_val_if_fail (PANGO_IS_FONT (font), 0);

  return font->size;
}

/**
 * pango_font_get_absolute_size:
 * @font: a `PangoFont`
 *
 * Returns the pixel size of the font, scaled by `PANGO_SCALE`.
 *
 * Return value: the pixel size of the font
 */
double
pango_font_get_absolute_size (PangoFont *font)
{
  g_return_val_if_fail (PANGO_IS_FONT (font), 0.);

  return font->size * font->dpi / 72.;
}

/**
 * pango_font_get_gravity:
 * @font: a `PangoFont`
 *
 * Returns the gravity of the font.
 *
 * Return value: the gravity of the font
 */
PangoGravity
pango_font_get_gravity (PangoFont *font)
{
  g_return_val_if_fail (PANGO_IS_FONT (font), PANGO_GRAVITY_SOUTH);

  return font->gravity;
}

/* }}} */

/* vim:set foldmethod=marker expandtab: */
