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


G_DEFINE_ABSTRACT_TYPE (PangoFont, pango_font, G_TYPE_OBJECT)

static void
pango_font_finalize (GObject *object)
{
  PangoFont *font = PANGO_FONT (object);

  hb_font_destroy (font->hb_font);

  G_OBJECT_CLASS (pango_font_parent_class)->finalize (object);
}

static PangoLanguage **
pango_font_default_get_languages (PangoFont *font)
{
  return pango_font_face_get_languages (pango_font_get_face (font));
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

static gboolean
pango_font_default_has_char (PangoFont *font,
                             gunichar   wc)
{
  return FALSE;
}

static PangoFontFace *
pango_font_default_get_face (PangoFont *font)
{
  PangoFontMap *map = pango_font_get_font_map (font);

  return PANGO_FONT_MAP_GET_CLASS (map)->get_face (map,font);
}

static void
pango_font_default_get_matrix (PangoFont   *font,
                               PangoMatrix *matrix)
{
  *matrix = (PangoMatrix) PANGO_MATRIX_INIT;
}

static int
pango_font_default_get_absolute_size (PangoFont *font)
{
  PangoFontDescription *desc;
  int size;

  desc = pango_font_describe_with_absolute_size (font);
  size = pango_font_description_get_size (desc);
  pango_font_description_free (desc);

  return size;
}

static void
pango_font_class_init (PangoFontClass *class G_GNUC_UNUSED)
{
  GObjectClass *object_class = G_OBJECT_CLASS (class);

  object_class->finalize = pango_font_finalize;

  class->get_languages = pango_font_default_get_languages;
  class->is_hinted = pango_font_default_is_hinted;
  class->get_scale_factors = pango_font_default_get_scale_factors;
  class->has_char = pango_font_default_has_char;
  class->get_face = pango_font_default_get_face;
  class->get_matrix = pango_font_default_get_matrix;
  class->get_absolute_size = pango_font_default_get_absolute_size;
}

static void
pango_font_init (PangoFont *font G_GNUC_UNUSED)
{
}

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
  g_return_val_if_fail (font != NULL, NULL);

  if (G_UNLIKELY (!PANGO_FONT_GET_CLASS (font)->describe_absolute))
    {
      g_warning ("describe_absolute not implemented for this font class, report this as a bug");
      return pango_font_describe (font);
    }

  return PANGO_FONT_GET_CLASS (font)->describe_absolute (font);
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
pango_font_get_glyph_extents  (PangoFont      *font,
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
 *   [method@Pango.FontMetrics.unref] when finished using the object.
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
 * pango_font_get_font_map:
 * @font: (nullable): a `PangoFont`
 *
 * Gets the font map for which the font was created.
 *
 * Note that the font maintains a *weak* reference to
 * the font map, so if all references to font map are
 * dropped, the font map will be finalized even if there
 * are fonts created with the font map that are still alive.
 * In that case this function will return %NULL.
 *
 * It is the responsibility of the user to ensure that the
 * font map is kept alive. In most uses this is not an issue
 * as a `PangoContext` holds a reference to the font map.
 *
 * Return value: (transfer none) (nullable): the `PangoFontMap`
 *   for the font
 */
PangoFontMap *
pango_font_get_font_map (PangoFont *font)
{
  if (G_UNLIKELY (!font))
    return NULL;

  if (PANGO_FONT_GET_CLASS (font)->get_font_map)
    return PANGO_FONT_GET_CLASS (font)->get_font_map (font);
  else
    return NULL;
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
  return PANGO_FONT_GET_CLASS (font)->get_face (font);
}

/**
 * pango_font_get_hb_font: (skip)
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
 * pango_font_has_char:
 * @font: a `PangoFont`
 * @wc: a Unicode character
 *
 * Returns whether the font provides a glyph for this character.
 *
 * Returns: `TRUE` if @font can render @wc
 */
gboolean
pango_font_has_char (PangoFont *font,
                     gunichar   wc)
{
  return PANGO_FONT_GET_CLASS (font)->has_char (font, wc);
}

/**
 * pango_font_get_features:
 * @font: a `PangoFont`
 * @features: (out caller-allocates) (array length=len): Array to features in
 * @len: the length of @features
 * @num_features: (inout): the number of used items in @features
 *
 * Obtain the OpenType features that are provided by the font.
 *
 * These are passed to the rendering system, together with features
 * that have been explicitly set via attributes.
 *
 * Note that this does not include OpenType features which the
 * rendering system enables by default.
 */
void
pango_font_get_features (PangoFont    *font,
                         hb_feature_t *features,
                         guint         len,
                         guint        *num_features)
{
  if (PANGO_FONT_GET_CLASS (font)->get_features)
    PANGO_FONT_GET_CLASS (font)->get_features (font, features, len, num_features);
}

/**
 * pango_font_get_languages:
 * @font: a `PangoFont`
 *
 * Returns the languages that are supported by @font.
 *
 * If the font backend does not provide this information,
 * %NULL is returned. For the fontconfig backend, this
 * corresponds to the FC_LANG member of the FcPattern.
 *
 * The returned array is only valid as long as the font
 * and its fontmap are valid.
 *
 * Returns: (transfer none) (nullable) (array zero-terminated=1) (element-type PangoLanguage): an array of `PangoLanguage`
 */
PangoLanguage **
pango_font_get_languages (PangoFont *font)
{
  return PANGO_FONT_GET_CLASS (font)->get_languages (font);
}

/*< private >
 * pango_font_get_matrix:
 * @font: a `PangoFont`
 *
 * Gets the matrix for the transformation from 'font space' to 'user space'.
 */
void
pango_font_get_matrix (PangoFont   *font,
                       PangoMatrix *matrix)
{
  PANGO_FONT_GET_CLASS (font)->get_matrix (font, matrix);
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
