/* Pango2
 * pango-attributes.c: Attributed text
 *
 * Copyright (C) 2000-2002 Red Hat Software
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
#include <string.h>

#include "pango-attributes-private.h"
#include "pango-attr-private.h"
#include "pango-impl-utils.h"

/* {{{ Attribute value types */

static inline Pango2Attribute *
pango2_attr_init (Pango2AttrType type)
{
  Pango2Attribute *attr;

  attr = g_slice_new (Pango2Attribute);
  attr->type = type;
  attr->start_index = PANGO2_ATTR_INDEX_FROM_TEXT_BEGINNING;
  attr->end_index = PANGO2_ATTR_INDEX_TO_TEXT_END;

  return attr;
}

static inline Pango2Attribute *
pango2_attr_string_new (Pango2AttrType  type,
                        const char     *value)
{
  Pango2Attribute *attr;

  g_return_val_if_fail (PANGO2_ATTR_TYPE_VALUE_TYPE (type) == PANGO2_ATTR_VALUE_STRING, NULL);

  attr = pango2_attr_init (type);
  attr->str_value = g_strdup (value);

  return attr;
}

static inline Pango2Attribute *
pango2_attr_int_new (Pango2AttrType type,
                     int            value)
{
  Pango2Attribute *attr;

  g_return_val_if_fail (PANGO2_ATTR_TYPE_VALUE_TYPE (type) == PANGO2_ATTR_VALUE_INT, NULL);

  attr = pango2_attr_init (type);
  attr->int_value = value;

  return attr;
}

static inline Pango2Attribute *
pango2_attr_boolean_new (Pango2AttrType type,
                         gboolean       value)
{
  Pango2Attribute *attr;

  g_return_val_if_fail (PANGO2_ATTR_TYPE_VALUE_TYPE (type) == PANGO2_ATTR_VALUE_BOOLEAN, NULL);

  attr = pango2_attr_init (type);
  attr->boolean_value = value;

  return attr;
}

static inline Pango2Attribute *
pango2_attr_float_new (Pango2AttrType type,
                       double         value)
{
  Pango2Attribute *attr;

  g_return_val_if_fail (PANGO2_ATTR_TYPE_VALUE_TYPE (type) == PANGO2_ATTR_VALUE_FLOAT, NULL);

  attr = pango2_attr_init (type);
  attr->double_value = value;

  return attr;
}

static inline Pango2Attribute *
pango2_attr_color_new (Pango2AttrType  type,
                       Pango2Color    *color)
{
  Pango2Attribute *attr;

  g_return_val_if_fail (PANGO2_ATTR_TYPE_VALUE_TYPE (type) == PANGO2_ATTR_VALUE_COLOR, NULL);

  attr = pango2_attr_init (type);
  attr->color_value = *color;

  return attr;
}

static inline Pango2Attribute *
pango2_attr_lang_new (Pango2AttrType  type,
                      Pango2Language *value)
{
  Pango2Attribute *attr;

  g_return_val_if_fail (PANGO2_ATTR_TYPE_VALUE_TYPE (type) == PANGO2_ATTR_VALUE_LANGUAGE, NULL);

  attr = pango2_attr_init (type);
  attr->lang_value  = value;

  return attr;
}

static inline Pango2Attribute *
pango2_attr_font_description_new (Pango2AttrType               type,
                                  const Pango2FontDescription *value)
{
  Pango2Attribute *attr;

  g_return_val_if_fail (PANGO2_ATTR_TYPE_VALUE_TYPE (type) == PANGO2_ATTR_VALUE_FONT_DESC, NULL);

  attr = pango2_attr_init (type);
  attr->font_value  = pango2_font_description_copy (value);

  return attr;
}

/* }}} */
/* {{{ Attribute types */

/**
 * pango2_attr_family_new:
 * @family: the family or comma-separated list of families
 *
 * Create a new font family attribute.
 *
 * Return value: (transfer full): the newly allocated
 *   `Pango2Attribute`, which should be freed with
 *   [method@Pango2.Attribute.destroy]
 */
Pango2Attribute *
pango2_attr_family_new (const char *family)
{
  return pango2_attr_string_new (PANGO2_ATTR_FAMILY, family);
}

/**
 * pango2_attr_language_new:
 * @language: language tag
 *
 * Create a new language tag attribute.
 *
 * Return value: (transfer full): the newly allocated
 *   `Pango2Attribute`, which should be freed with
 *   [method@Pango2.Attribute.destroy]
 */
Pango2Attribute *
pango2_attr_language_new (Pango2Language *language)
{
  return pango2_attr_lang_new (PANGO2_ATTR_LANGUAGE, language);
}

/**
 * pango2_attr_foreground_new:
 * @color: the color
 *
 * Create a new foreground color attribute.
 *
 * Return value: (transfer full): the newly allocated
 *   `Pango2Attribute`, which should be freed with
 *   [method@Pango2.Attribute.destroy]
 */
Pango2Attribute *
pango2_attr_foreground_new (Pango2Color *color)
{
  return pango2_attr_color_new (PANGO2_ATTR_FOREGROUND, color);
}

/**
 * pango2_attr_background_new:
 * @color: the color
 *
 * Create a new background color attribute.
 *
 * Return value: (transfer full): the newly allocated
 *   `Pango2Attribute`, which should be freed with
 *   [method@Pango2.Attribute.destroy]
 */
Pango2Attribute *
pango2_attr_background_new (Pango2Color *color)
{
  return pango2_attr_color_new (PANGO2_ATTR_BACKGROUND, color);
}

/**
 * pango2_attr_size_new:
 * @size: the font size, in %PANGO2_SCALE-ths of a point
 *
 * Create a new font-size attribute in fractional points.
 *
 * Return value: (transfer full): the newly allocated
 *   `Pango2Attribute`, which should be freed with
 *   [method@Pango2.Attribute.destroy]
 */
Pango2Attribute *
pango2_attr_size_new (int value)
{
  return pango2_attr_int_new (PANGO2_ATTR_SIZE, value);
}


/**
 * pango2_attr_size_new_absolute:
 * @size: the font size, in %PANGO2_SCALE-ths of a device unit
 *
 * Create a new font-size attribute in device units.
 *
 * Return value: (transfer full): the newly allocated
 *   `Pango2Attribute`, which should be freed with
 *   [method@Pango2.Attribute.destroy]
 */
Pango2Attribute *
pango2_attr_size_new_absolute (int size)
{
  return pango2_attr_int_new (PANGO2_ATTR_ABSOLUTE_SIZE, size);
}

/**
 * pango2_attr_style_new:
 * @style: the slant style
 *
 * Create a new font slant style attribute.
 *
 * Return value: (transfer full): the newly allocated
 *   `Pango2Attribute`, which should be freed with
 *   [method@Pango2.Attribute.destroy]
 */
Pango2Attribute *
pango2_attr_style_new (Pango2Style style)
{
  return pango2_attr_int_new (PANGO2_ATTR_STYLE, (int)style);
}

/**
 * pango2_attr_weight_new:
 * @weight: the weight
 *
 * Create a new font weight attribute.
 *
 * Return value: (transfer full): the newly allocated
 *   `Pango2Attribute`, which should be freed with
 *   [method@Pango2.Attribute.destroy]
 */
Pango2Attribute *
pango2_attr_weight_new (Pango2Weight weight)
{
  return pango2_attr_int_new (PANGO2_ATTR_WEIGHT, (int)weight);
}

/**
 * pango2_attr_variant_new:
 * @variant: the variant
 *
 * Create a new font variant attribute (normal or small caps).
 *
 * Return value: (transfer full): the newly allocated `Pango2Attribute`,
 *   which should be freed with [method@Pango2.Attribute.destroy].
 */
Pango2Attribute *
pango2_attr_variant_new (Pango2Variant variant)
{
  return pango2_attr_int_new (PANGO2_ATTR_VARIANT, (int)variant);
}

/**
 * pango2_attr_stretch_new:
 * @stretch: the stretch
 *
 * Create a new font stretch attribute.
 *
 * Return value: (transfer full): the newly allocated
 *   `Pango2Attribute`, which should be freed with
 *   [method@Pango2.Attribute.destroy]
 */
Pango2Attribute *
pango2_attr_stretch_new (Pango2Stretch stretch)
{
  return pango2_attr_int_new (PANGO2_ATTR_STRETCH, (int)stretch);
}

/**
 * pango2_attr_font_desc_new:
 * @desc: the font description
 *
 * Create a new font description attribute.
 *
 * This attribute allows setting family, style, weight, variant,
 * stretch, and size simultaneously.
 *
 * Return value: (transfer full): the newly allocated
 *   `Pango2Attribute`, which should be freed with
 *   [method@Pango2.Attribute.destroy]
 */
Pango2Attribute *
pango2_attr_font_desc_new (const Pango2FontDescription *desc)
{
  return pango2_attr_font_description_new (PANGO2_ATTR_FONT_DESC, desc);
}

/**
 * pango2_attr_underline_new:
 * @style: the line style
 *
 * Create a new underline attribute.
 *
 * Return value: (transfer full): the newly allocated
 *   `Pango2Attribute`, which should be freed with
 *   [method@Pango2.Attribute.destroy]
 */
Pango2Attribute *
pango2_attr_underline_new (Pango2LineStyle style)
{
  return pango2_attr_int_new (PANGO2_ATTR_UNDERLINE, (int)style);
}

/**
 * pango2_attr_underline_color_new:
 * @color: the color
 *
 * Create a new underline color attribute.
 *
 * This attribute modifies the color of underlines.
 * If not set, underlines will use the foreground color.
 *
 * Return value: (transfer full): the newly allocated
 *   `Pango2Attribute`, which should be freed with
 *   [method@Pango2.Attribute.destroy]
 */
Pango2Attribute *
pango2_attr_underline_color_new (Pango2Color *color)
{
  return pango2_attr_color_new (PANGO2_ATTR_UNDERLINE_COLOR, color);
}

/**
 * pango2_attr_underline_position_new:
 * @position: the underline position
 *
 * Create a new underline position attribute.
 *
 * Return value: (transfer full): the newly allocated
 *   `Pango2Attribute`, which should be freed with
 *   [method@Pango2.Attribute.destroy]
 */
Pango2Attribute *
pango2_attr_underline_position_new (Pango2UnderlinePosition position)
{
  return pango2_attr_int_new (PANGO2_ATTR_UNDERLINE_POSITION, (int)position);
}
/**
 * pango2_attr_strikethrough_new:
 * @style: the line style
 *
 * Create a new strike-through attribute.
 *
 * Return value: (transfer full): the newly allocated
 *   `Pango2Attribute`, which should be freed with
 *   [method@Pango2.Attribute.destroy]
 */
Pango2Attribute *
pango2_attr_strikethrough_new (Pango2LineStyle style)
{
  return pango2_attr_int_new (PANGO2_ATTR_STRIKETHROUGH, (int)style);
}

/**
 * pango2_attr_strikethrough_color_new:
 * @color: the color
 *
 * Create a new strikethrough color attribute.
 *
 * This attribute modifies the color of strikethrough lines.
 * If not set, strikethrough lines will use the foreground color.
 *
 * Return value: (transfer full): the newly allocated
 *   `Pango2Attribute`, which should be freed with
 *   [method@Pango2.Attribute.destroy]
 */
Pango2Attribute *
pango2_attr_strikethrough_color_new (Pango2Color *color)
{
  return pango2_attr_color_new (PANGO2_ATTR_STRIKETHROUGH_COLOR, color);
}

/**
 * pango2_attr_rise_new:
 * @rise: the amount that the text should be displaced vertically,
 *   in Pango units. Positive values displace the text upwards.
 *
 * Create a new baseline displacement attribute.
 *
 * Return value: (transfer full): the newly allocated
 *   `Pango2Attribute`, which should be freed with
 *   [method@Pango2.Attribute.destroy]
 */
Pango2Attribute *
pango2_attr_rise_new (int rise)
{
  return pango2_attr_int_new (PANGO2_ATTR_RISE, (int)rise);
}

/**
 * pango2_attr_baseline_shift_new:
 * @shift: either a `Pango2BaselineShift` enumeration value or an absolute value (> 1024)
 *   in Pango units, relative to the baseline of the previous run.
 *   Positive values displace the text upwards.
 *
 * Create a new baseline displacement attribute.
 *
 * The effect of this attribute is to shift the baseline of a run,
 * relative to the run of preceding run.
 *
 * <picture>
 *   <source srcset="baseline-shift-dark.png" media="(prefers-color-scheme: dark)">
 *   <img alt="Baseline Shift" src="baseline-shift-light.png">
 * </picture>

 * Return value: (transfer full): the newly allocated
 *   `Pango2Attribute`, which should be freed with
 *   [method@Pango2.Attribute.destroy]
 */
Pango2Attribute *
pango2_attr_baseline_shift_new (int rise)
{
  return pango2_attr_int_new (PANGO2_ATTR_BASELINE_SHIFT, (int)rise);
}

/**
 * pango2_attr_font_scale_new:
 * @scale: a `Pango2FontScale` value, which indicates font size change relative
 *   to the size of the previous run.
 *
 * Create a new font scale attribute.
 *
 * The effect of this attribute is to change the font size of a run,
 * relative to the size of preceding run.
 *
 * Return value: (transfer full): the newly allocated
 *   `Pango2Attribute`, which should be freed with
 *   [method@Pango2.Attribute.destroy]
 */
Pango2Attribute *
pango2_attr_font_scale_new (Pango2FontScale scale)
{
  return pango2_attr_int_new (PANGO2_ATTR_FONT_SCALE, (int)scale);
}

/**
 * pango2_attr_scale_new:
 * @scale_factor: factor to scale the font
 *
 * Create a new font size scale attribute.
 *
 * The base font for the affected text will have
 * its size multiplied by @scale_factor.
 *
 * Return value: (transfer full): the newly allocated
 *   `Pango2Attribute`, which should be freed with
 *   [method@Pango2.Attribute.destroy]
 */
Pango2Attribute*
pango2_attr_scale_new (double scale_factor)
{
  return pango2_attr_float_new (PANGO2_ATTR_SCALE, scale_factor);
}

/**
 * pango2_attr_fallback_new:
 * @enable_fallback: %TRUE if we should fall back on other fonts
 *   for characters the active font is missing
 *
 * Create a new font fallback attribute.
 *
 * If fallback is disabled, characters will only be
 * used from the closest matching font on the system.
 * No fallback will be done to other fonts on the system
 * that might contain the characters in the text.
 *
 * Return value: (transfer full): the newly allocated
 *   `Pango2Attribute`, which should be freed with
 *   [method@Pango2.Attribute.destroy]
 */
Pango2Attribute *
pango2_attr_fallback_new (gboolean enable_fallback)
{
  return pango2_attr_boolean_new (PANGO2_ATTR_FALLBACK, enable_fallback);
}

/**
 * pango2_attr_letter_spacing_new:
 * @letter_spacing: amount of extra space to add between
 *   graphemes of the text, in Pango units
 *
 * Create a new letter-spacing attribute.
 *
 * Return value: (transfer full): the newly allocated
 *   `Pango2Attribute`, which should be freed with
 *   [method@Pango2.Attribute.destroy]
 */
Pango2Attribute *
pango2_attr_letter_spacing_new (int letter_spacing)
{
  return pango2_attr_int_new (PANGO2_ATTR_LETTER_SPACING, letter_spacing);
}

/**
 * pango2_attr_gravity_new:
 * @gravity: the gravity value; should not be %PANGO2_GRAVITY_AUTO
 *
 * Create a new gravity attribute.
 *
 * Return value: (transfer full): the newly allocated
 *   `Pango2Attribute`, which should be freed with
 *   [method@Pango2.Attribute.destroy]
 */
Pango2Attribute *
pango2_attr_gravity_new (Pango2Gravity gravity)
{
  g_return_val_if_fail (gravity != PANGO2_GRAVITY_AUTO, NULL);

  return pango2_attr_int_new (PANGO2_ATTR_GRAVITY, (int)gravity);
}

/**
 * pango2_attr_gravity_hint_new:
 * @hint: the gravity hint value
 *
 * Create a new gravity hint attribute.
 *
 * Return value: (transfer full): the newly allocated
 *   `Pango2Attribute`, which should be freed with
 *   [method@Pango2.Attribute.destroy]
 */
Pango2Attribute *
pango2_attr_gravity_hint_new (Pango2GravityHint hint)
{
  return pango2_attr_int_new (PANGO2_ATTR_GRAVITY_HINT, (int)hint);
}

/**
 * pango2_attr_font_features_new:
 * @features: a string with OpenType font features, with the syntax of the [CSS
 * font-feature-settings property](https://www.w3.org/TR/css-fonts-4/#font-rend-desc)
 *
 * Create a new font features tag attribute.
 *
 * You can use this attribute to select OpenType font features like small-caps,
 * alternative glyphs, ligatures, etc. for fonts that support them.
 *
 * Return value: (transfer full): the newly allocated
 *   `Pango2Attribute`, which should be freed with
 *   [method@Pango2.Attribute.destroy]
 */
Pango2Attribute *
pango2_attr_font_features_new (const char *features)
{
  g_return_val_if_fail (features != NULL, NULL);

  return pango2_attr_string_new (PANGO2_ATTR_FONT_FEATURES, features);
}

/**
 * pango2_attr_allow_breaks_new:
 * @allow_breaks: %TRUE if we line breaks are allowed
 *
 * Create a new allow-breaks attribute.
 *
 * If breaks are disabled, the range will be kept in a
 * single run, as far as possible.
 *
 * Return value: (transfer full): the newly allocated
 *   `Pango2Attribute`, which should be freed with
 *   [method@Pango2.Attribute.destroy]
 */
Pango2Attribute *
pango2_attr_allow_breaks_new (gboolean allow_breaks)
{
  return pango2_attr_boolean_new (PANGO2_ATTR_ALLOW_BREAKS, allow_breaks);
}

/**
 * pango2_attr_insert_hyphens_new:
 * @insert_hyphens: %TRUE if hyphens should be inserted
 *
 * Create a new insert-hyphens attribute.
 *
 * Pango will insert hyphens when breaking lines in
 * the middle of a word. This attribute can be used
 * to suppress the hyphen.
 *
 * Return value: (transfer full): the newly allocated
 *   `Pango2Attribute`, which should be freed with
 *   [method@Pango2.Attribute.destroy]
 */
Pango2Attribute *
pango2_attr_insert_hyphens_new (gboolean insert_hyphens)
{
  return pango2_attr_boolean_new (PANGO2_ATTR_INSERT_HYPHENS, insert_hyphens);
}

/**
 * pango2_attr_show_new:
 * @flags: `Pango2ShowFlags` to apply
 *
 * Create a new attribute that influences how invisible
 * characters are rendered.
 *
 * Return value: (transfer full): the newly allocated
 *   `Pango2Attribute`, which should be freed with
 *   [method@Pango2.Attribute.destroy]
 **/
Pango2Attribute *
pango2_attr_show_new (Pango2ShowFlags flags)
{
  return pango2_attr_int_new (PANGO2_ATTR_SHOW, (int)flags);
}

/**
 * pango2_attr_word_new:
 *
 * Create a new attribute that marks its range
 * as a single word.
 *
 * Note that this may require adjustments to word and
 * sentence classification around the range.
 *
 * Return value: (transfer full): the newly allocated
 *   `Pango2Attribute`, which should be freed with
 *   [method@Pango2.Attribute.destroy]
 */
Pango2Attribute *
pango2_attr_word_new (void)
{
  return pango2_attr_boolean_new (PANGO2_ATTR_WORD, TRUE);
}

/**
 * pango2_attr_sentence_new:
 *
 * Create a new attribute that marks its range
 * as a single sentence.
 *
 * Note that this may require adjustments to word and
 * sentence classification around the range.
 *
 * Return value: (transfer full): the newly allocated
 *   `Pango2Attribute`, which should be freed with
 *   [method@Pango2.Attribute.destroy]
 */
Pango2Attribute *
pango2_attr_sentence_new (void)
{
  return pango2_attr_boolean_new (PANGO2_ATTR_SENTENCE, TRUE);
}

/**
 * pango2_attr_paragraph_new:
 *
 * Create a new attribute that marks its range as a single paragraph.
 *
 * Newlines and similar characters in the range of the attribute
 * will not be treated as paragraph separators.
 *
 * Return value: (transfer full): the newly allocated
 *   `Pango2Attribute`, which should be freed with
 *   [method@Pango2.Attribute.destroy]
 */
Pango2Attribute *
pango2_attr_paragraph_new (void)
{
  return pango2_attr_boolean_new (PANGO2_ATTR_SENTENCE, TRUE);
}

/**
 * pango2_attr_overline_new:
 * @style: the line style
 *
 * Create a new overline-style attribute.
 *
 * Return value: (transfer full): the newly allocated
 *   `Pango2Attribute`, which should be freed with
 *   [method@Pango2.Attribute.destroy]
 */
Pango2Attribute *
pango2_attr_overline_new (Pango2LineStyle style)
{
  return pango2_attr_int_new (PANGO2_ATTR_OVERLINE, (int)style);
}

/**
 * pango2_attr_overline_color_new:
 * @color: the color
 *
 * Create a new overline color attribute.
 *
 * This attribute modifies the color of overlines.
 * If not set, overlines will use the foreground color.
 *
 * Return value: (transfer full): the newly allocated
 *   `Pango2Attribute`, which should be freed with
 *   [method@Pango2.Attribute.destroy]
 */
Pango2Attribute *
pango2_attr_overline_color_new (Pango2Color *color)
{
  return pango2_attr_color_new (PANGO2_ATTR_OVERLINE_COLOR, color);
}

/**
 * pango2_attr_line_height_new:
 * @factor: the scaling factor to apply to the logical height
 *
 * Create a new attribute that modifies the height
 * of logical line extents by a factor.
 *
 * This affects the values returned by
 * [method@Pango2.Line.get_extents] and
 * [method@Pango2.LineIter.get_line_extents].
 */
Pango2Attribute *
pango2_attr_line_height_new (double factor)
{
  return pango2_attr_float_new (PANGO2_ATTR_LINE_HEIGHT, factor);
}

/**
 * pango2_attr_line_height_new_absolute:
 * @height: the line height, in %PANGO2_SCALE-ths of a point
 *
 * Create a new attribute that overrides the height
 * of logical line extents to be @height.
 *
 * This affects the values returned by
 * [method@Pango2.Line.get_extents],
 * [method@Pango2.LineIter.get_line_extents].
 */
Pango2Attribute *
pango2_attr_line_height_new_absolute (int height)
{
  return pango2_attr_int_new (PANGO2_ATTR_ABSOLUTE_LINE_HEIGHT, height);
}

/**
 * pango2_attr_line_spacing_new:
 * @spacing: the line spacing, in %PANGO2_SCALE-ths of a point
 *
 * Create a new attribute that adds space to the
 * leading from font metrics, if not overridden
 * by line spacing attributes.
 *
 * This affects the values returned by
 * [method@Pango2.Line.get_extents],
 * [method@Pango2.LineIter.get_line_extents].
 */
Pango2Attribute *
pango2_attr_line_spacing_new (int spacing)
{
  return pango2_attr_int_new (PANGO2_ATTR_LINE_SPACING, spacing);
}

/**
 * pango2_attr_text_transform_new:
 * @transform: `Pango2TextTransform` to apply
 *
 * Create a new attribute that influences how characters
 * are transformed during shaping.
 *
 * Return value: (transfer full): the newly allocated
 *   `Pango2Attribute`, which should be freed with
 *   [method@Pango2.Attribute.destroy]
 */
Pango2Attribute *
pango2_attr_text_transform_new (Pango2TextTransform transform)
{
  return pango2_attr_int_new (PANGO2_ATTR_TEXT_TRANSFORM, transform);
}

/**
 * pango2_attr_shape_new:
 * @ink_rect: ink rectangle to use for each character
 * @logical_rect: logical rectangle to use for each character
 * @data: user data
 * @copy: (nullable): function to copy @data when the attribute
 *   is copied
 * @destroy: (nullable): function to free @data when the attribute
 *   is freed
 *
 * Creates a new shape attribute.
 *
 * Shape attributes override the extents for a glyph and can
 * trigger custom rendering in a `Pango2Renderer`. This might
 * be used, for instance, for embedding a picture or a widget
 * inside a `Pango2Layout`.
 *
 * Return value: (transfer full): the newly allocated
 *   `Pango2Attribute`, which should be freed with
 *   [method@Pango2.Attribute.destroy]
 */
Pango2Attribute *
pango2_attr_shape_new (Pango2Rectangle        *ink_rect,
                       Pango2Rectangle        *logical_rect,
                       gpointer                data,
                       Pango2AttrDataCopyFunc  copy,
                       GDestroyNotify          destroy)
{
  Pango2Attribute *attr;
  ShapeData *shape_data;

  shape_data = g_new0 (ShapeData, 1);
  shape_data->ink_rect = *ink_rect;
  shape_data->logical_rect = *logical_rect;
  shape_data->data = data;
  shape_data->copy = copy;
  shape_data->destroy = destroy;

  attr = pango2_attr_init (PANGO2_ATTR_SHAPE);
  attr->pointer_value = shape_data;

  return attr;
}

/* }}} */
/* {{{ Private API */

gboolean
pango2_attribute_affects_itemization (Pango2Attribute *attr,
                                      gpointer         data)
{
  return (PANGO2_ATTR_AFFECTS (attr) & PANGO2_ATTR_AFFECTS_ITEMIZATION) != 0;
}

gboolean
pango2_attribute_affects_break_or_shape (Pango2Attribute *attr,
                                         gpointer         data)
{
  return (PANGO2_ATTR_AFFECTS (attr) & (PANGO2_ATTR_AFFECTS_BREAKING |
                                       PANGO2_ATTR_AFFECTS_SHAPING)) != 0;
}

/* }}} */

/* vim:set foldmethod=marker expandtab: */
