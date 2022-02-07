/* Pango
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

static inline PangoAttribute *
pango_attr_init (PangoAttrType type)
{
  PangoAttribute *attr;

  attr = g_slice_new (PangoAttribute);
  attr->type = type;
  attr->start_index = PANGO_ATTR_INDEX_FROM_TEXT_BEGINNING;
  attr->end_index = PANGO_ATTR_INDEX_TO_TEXT_END;

  return attr;
}

static inline PangoAttribute *
pango_attr_string_new (PangoAttrType  type,
                       const char    *value)
{
  PangoAttribute *attr;

  g_return_val_if_fail (PANGO_ATTR_TYPE_VALUE_TYPE (type) == PANGO_ATTR_VALUE_STRING, NULL);

  attr = pango_attr_init (type);
  attr->str_value = g_strdup (value);

  return attr;
}

static inline PangoAttribute *
pango_attr_int_new (PangoAttrType type,
                    int           value)
{
  PangoAttribute *attr;

  g_return_val_if_fail (PANGO_ATTR_TYPE_VALUE_TYPE (type) == PANGO_ATTR_VALUE_INT, NULL);

  attr = pango_attr_init (type);
  attr->int_value = value;

  return attr;
}

static inline PangoAttribute *
pango_attr_boolean_new (PangoAttrType type,
                        gboolean      value)
{
  PangoAttribute *attr;

  g_return_val_if_fail (PANGO_ATTR_TYPE_VALUE_TYPE (type) == PANGO_ATTR_VALUE_BOOLEAN, NULL);

  attr = pango_attr_init (type);
  attr->boolean_value = value;

  return attr;
}

static inline PangoAttribute *
pango_attr_float_new (PangoAttrType type,
                      double        value)
{
  PangoAttribute *attr;

  g_return_val_if_fail (PANGO_ATTR_TYPE_VALUE_TYPE (type) == PANGO_ATTR_VALUE_FLOAT, NULL);

  attr = pango_attr_init (type);
  attr->double_value = value;

  return attr;
}

static inline PangoAttribute *
pango_attr_color_new (PangoAttrType  type,
                      PangoColor    *color)
{
  PangoAttribute *attr;

  g_return_val_if_fail (PANGO_ATTR_TYPE_VALUE_TYPE (type) == PANGO_ATTR_VALUE_COLOR, NULL);

  attr = pango_attr_init (type);
  attr->color_value = *color;

  return attr;
}

static inline PangoAttribute *
pango_attr_lang_new (PangoAttrType  type,
                     PangoLanguage *value)
{
  PangoAttribute *attr;

  g_return_val_if_fail (PANGO_ATTR_TYPE_VALUE_TYPE (type) == PANGO_ATTR_VALUE_LANGUAGE, NULL);

  attr = pango_attr_init (type);
  attr->lang_value  = value;

  return attr;
}

static inline PangoAttribute *
pango_attr_font_description_new (PangoAttrType               type,
                                 const PangoFontDescription *value)
{
  PangoAttribute *attr;

  g_return_val_if_fail (PANGO_ATTR_TYPE_VALUE_TYPE (type) == PANGO_ATTR_VALUE_FONT_DESC, NULL);

  attr = pango_attr_init (type);
  attr->font_value  = pango_font_description_copy (value);

  return attr;
}

/* }}} */
/* {{{ Attribute types */

/**
 * pango_attr_family_new:
 * @family: the family or comma-separated list of families
 *
 * Create a new font family attribute.
 *
 * Return value: (transfer full): the newly allocated
 *   `PangoAttribute`, which should be freed with
 *   [method@Pango.Attribute.destroy]
 */
PangoAttribute *
pango_attr_family_new (const char *family)
{
  return pango_attr_string_new (PANGO_ATTR_FAMILY, family);
}

/**
 * pango_attr_language_new:
 * @language: language tag
 *
 * Create a new language tag attribute.
 *
 * Return value: (transfer full): the newly allocated
 *   `PangoAttribute`, which should be freed with
 *   [method@Pango.Attribute.destroy]
 */
PangoAttribute *
pango_attr_language_new (PangoLanguage *language)
{
  return pango_attr_lang_new (PANGO_ATTR_LANGUAGE, language);
}

/**
 * pango_attr_foreground_new:
 * @color: the color
 *
 * Create a new foreground color attribute.
 *
 * Return value: (transfer full): the newly allocated
 *   `PangoAttribute`, which should be freed with
 *   [method@Pango.Attribute.destroy]
 */
PangoAttribute *
pango_attr_foreground_new (PangoColor *color)
{
  return pango_attr_color_new (PANGO_ATTR_FOREGROUND, color);
}

/**
 * pango_attr_background_new:
 * @color: the color
 *
 * Create a new background color attribute.
 *
 * Return value: (transfer full): the newly allocated
 *   `PangoAttribute`, which should be freed with
 *   [method@Pango.Attribute.destroy]
 */
PangoAttribute *
pango_attr_background_new (PangoColor *color)
{
  return pango_attr_color_new (PANGO_ATTR_BACKGROUND, color);
}

/**
 * pango_attr_size_new:
 * @size: the font size, in %PANGO_SCALE-ths of a point
 *
 * Create a new font-size attribute in fractional points.
 *
 * Return value: (transfer full): the newly allocated
 *   `PangoAttribute`, which should be freed with
 *   [method@Pango.Attribute.destroy]
 */
PangoAttribute *
pango_attr_size_new (int value)
{
  return pango_attr_int_new (PANGO_ATTR_SIZE, value);
}


/**
 * pango_attr_size_new_absolute:
 * @size: the font size, in %PANGO_SCALE-ths of a device unit
 *
 * Create a new font-size attribute in device units.
 *
 * Return value: (transfer full): the newly allocated
 *   `PangoAttribute`, which should be freed with
 *   [method@Pango.Attribute.destroy]
 *
 * Since: 1.8
 */
PangoAttribute *
pango_attr_size_new_absolute (int size)
{
  return pango_attr_int_new (PANGO_ATTR_ABSOLUTE_SIZE, size);
}

/**
 * pango_attr_style_new:
 * @style: the slant style
 *
 * Create a new font slant style attribute.
 *
 * Return value: (transfer full): the newly allocated
 *   `PangoAttribute`, which should be freed with
 *   [method@Pango.Attribute.destroy]
 */
PangoAttribute *
pango_attr_style_new (PangoStyle style)
{
  return pango_attr_int_new (PANGO_ATTR_STYLE, (int)style);
}

/**
 * pango_attr_weight_new:
 * @weight: the weight
 *
 * Create a new font weight attribute.
 *
 * Return value: (transfer full): the newly allocated
 *   `PangoAttribute`, which should be freed with
 *   [method@Pango.Attribute.destroy]
 */
PangoAttribute *
pango_attr_weight_new (PangoWeight weight)
{
  return pango_attr_int_new (PANGO_ATTR_WEIGHT, (int)weight);
}

/**
 * pango_attr_variant_new:
 * @variant: the variant
 *
 * Create a new font variant attribute (normal or small caps).
 *
 * Return value: (transfer full): the newly allocated `PangoAttribute`,
 *   which should be freed with [method@Pango.Attribute.destroy].
 */
PangoAttribute *
pango_attr_variant_new (PangoVariant variant)
{
  return pango_attr_int_new (PANGO_ATTR_VARIANT, (int)variant);
}

/**
 * pango_attr_stretch_new:
 * @stretch: the stretch
 *
 * Create a new font stretch attribute.
 *
 * Return value: (transfer full): the newly allocated
 *   `PangoAttribute`, which should be freed with
 *   [method@Pango.Attribute.destroy]
 */
PangoAttribute *
pango_attr_stretch_new (PangoStretch stretch)
{
  return pango_attr_int_new (PANGO_ATTR_STRETCH, (int)stretch);
}

/**
 * pango_attr_font_desc_new:
 * @desc: the font description
 *
 * Create a new font description attribute.
 *
 * This attribute allows setting family, style, weight, variant,
 * stretch, and size simultaneously.
 *
 * Return value: (transfer full): the newly allocated
 *   `PangoAttribute`, which should be freed with
 *   [method@Pango.Attribute.destroy]
 */
PangoAttribute *
pango_attr_font_desc_new (const PangoFontDescription *desc)
{
  return pango_attr_font_description_new (PANGO_ATTR_FONT_DESC, desc);
}

/**
 * pango_attr_underline_new:
 * @style: the line style
 *
 * Create a new underline attribute.
 *
 * Return value: (transfer full): the newly allocated
 *   `PangoAttribute`, which should be freed with
 *   [method@Pango.Attribute.destroy]
 */
PangoAttribute *
pango_attr_underline_new (PangoLineStyle style)
{
  return pango_attr_int_new (PANGO_ATTR_UNDERLINE, (int)style);
}

/**
 * pango_attr_underline_color_new:
 * @color: the color
 *
 * Create a new underline color attribute.
 *
 * This attribute modifies the color of underlines.
 * If not set, underlines will use the foreground color.
 *
 * Return value: (transfer full): the newly allocated
 *   `PangoAttribute`, which should be freed with
 *   [method@Pango.Attribute.destroy]
 *
 * Since: 1.8
 */
PangoAttribute *
pango_attr_underline_color_new (PangoColor *color)
{
  return pango_attr_color_new (PANGO_ATTR_UNDERLINE_COLOR, color);
}

PangoAttribute *
pango_attr_underline_position_new (PangoUnderlinePosition position)
{
  return pango_attr_int_new (PANGO_ATTR_UNDERLINE_POSITION, (int)position);
}
/**
 * pango_attr_strikethrough_new:
 * @strikethrough: %TRUE if the text should be struck-through
 *
 * Create a new strike-through attribute.
 *
 * Return value: (transfer full): the newly allocated
 *   `PangoAttribute`, which should be freed with
 *   [method@Pango.Attribute.destroy]
 */
PangoAttribute *
pango_attr_strikethrough_new (gboolean strikethrough)
{
  return pango_attr_boolean_new (PANGO_ATTR_STRIKETHROUGH, (int)strikethrough);
}

/**
 * pango_attr_strikethrough_color_new:
 * @color: the color
 *
 * Create a new strikethrough color attribute.
 *
 * This attribute modifies the color of strikethrough lines.
 * If not set, strikethrough lines will use the foreground color.
 *
 * Return value: (transfer full): the newly allocated
 *   `PangoAttribute`, which should be freed with
 *   [method@Pango.Attribute.destroy]
 *
 * Since: 1.8
 */
PangoAttribute *
pango_attr_strikethrough_color_new (PangoColor *color)
{
  return pango_attr_color_new (PANGO_ATTR_STRIKETHROUGH_COLOR, color);
}

/**
 * pango_attr_rise_new:
 * @rise: the amount that the text should be displaced vertically,
 *   in Pango units. Positive values displace the text upwards.
 *
 * Create a new baseline displacement attribute.
 *
 * Return value: (transfer full): the newly allocated
 *   `PangoAttribute`, which should be freed with
 *   [method@Pango.Attribute.destroy]
 */
PangoAttribute *
pango_attr_rise_new (int rise)
{
  return pango_attr_int_new (PANGO_ATTR_RISE, (int)rise);
}

/**
 * pango_attr_baseline_shift_new:
 * @shift: either a `PangoBaselineShift` enumeration value or an absolute value (> 1024)
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
 *   `PangoAttribute`, which should be freed with
 *   [method@Pango.Attribute.destroy]
 *
 * Since: 1.50
 */
PangoAttribute *
pango_attr_baseline_shift_new (int rise)
{
  return pango_attr_int_new (PANGO_ATTR_BASELINE_SHIFT, (int)rise);
}

/**
 * pango_attr_font_scale_new:
 * @scale: a `PangoFontScale` value, which indicates font size change relative
 *   to the size of the previous run.
 *
 * Create a new font scale attribute.
 *
 * The effect of this attribute is to change the font size of a run,
 * relative to the size of preceding run.
 *
 * Return value: (transfer full): the newly allocated
 *   `PangoAttribute`, which should be freed with
 *   [method@Pango.Attribute.destroy]
 *
 * Since: 1.50
 */
PangoAttribute *
pango_attr_font_scale_new (PangoFontScale scale)
{
  return pango_attr_int_new (PANGO_ATTR_FONT_SCALE, (int)scale);
}

/**
 * pango_attr_scale_new:
 * @scale_factor: factor to scale the font
 *
 * Create a new font size scale attribute.
 *
 * The base font for the affected text will have
 * its size multiplied by @scale_factor.
 *
 * Return value: (transfer full): the newly allocated
 *   `PangoAttribute`, which should be freed with
 *   [method@Pango.Attribute.destroy]
 */
PangoAttribute*
pango_attr_scale_new (double scale_factor)
{
  return pango_attr_float_new (PANGO_ATTR_SCALE, scale_factor);
}

/**
 * pango_attr_fallback_new:
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
 *   `PangoAttribute`, which should be freed with
 *   [method@Pango.Attribute.destroy]
 *
 * Since: 1.4
 */
PangoAttribute *
pango_attr_fallback_new (gboolean enable_fallback)
{
  return pango_attr_boolean_new (PANGO_ATTR_FALLBACK, enable_fallback);
}

/**
 * pango_attr_letter_spacing_new:
 * @letter_spacing: amount of extra space to add between
 *   graphemes of the text, in Pango units
 *
 * Create a new letter-spacing attribute.
 *
 * Return value: (transfer full): the newly allocated
 *   `PangoAttribute`, which should be freed with
 *   [method@Pango.Attribute.destroy]
 *
 * Since: 1.6
 */
PangoAttribute *
pango_attr_letter_spacing_new (int letter_spacing)
{
  return pango_attr_int_new (PANGO_ATTR_LETTER_SPACING, letter_spacing);
}

/**
 * pango_attr_gravity_new:
 * @gravity: the gravity value; should not be %PANGO_GRAVITY_AUTO
 *
 * Create a new gravity attribute.
 *
 * Return value: (transfer full): the newly allocated
 *   `PangoAttribute`, which should be freed with
 *   [method@Pango.Attribute.destroy]
 *
 * Since: 1.16
 */
PangoAttribute *
pango_attr_gravity_new (PangoGravity gravity)
{
  g_return_val_if_fail (gravity != PANGO_GRAVITY_AUTO, NULL);

  return pango_attr_int_new (PANGO_ATTR_GRAVITY, (int)gravity);
}

/**
 * pango_attr_gravity_hint_new:
 * @hint: the gravity hint value
 *
 * Create a new gravity hint attribute.
 *
 * Return value: (transfer full): the newly allocated
 *   `PangoAttribute`, which should be freed with
 *   [method@Pango.Attribute.destroy]
 *
 * Since: 1.16
 */
PangoAttribute *
pango_attr_gravity_hint_new (PangoGravityHint hint)
{
  return pango_attr_int_new (PANGO_ATTR_GRAVITY_HINT, (int)hint);
}

/**
 * pango_attr_font_features_new:
 * @features: a string with OpenType font features, with the syntax of the [CSS
 * font-feature-settings property](https://www.w3.org/TR/css-fonts-4/#font-rend-desc)
 *
 * Create a new font features tag attribute.
 *
 * You can use this attribute to select OpenType font features like small-caps,
 * alternative glyphs, ligatures, etc. for fonts that support them.
 *
 * Return value: (transfer full): the newly allocated
 *   `PangoAttribute`, which should be freed with
 *   [method@Pango.Attribute.destroy]
 *
 * Since: 1.38
 */
PangoAttribute *
pango_attr_font_features_new (const char *features)
{
  g_return_val_if_fail (features != NULL, NULL);

  return pango_attr_string_new (PANGO_ATTR_FONT_FEATURES, features);
}

/**
 * pango_attr_foreground_alpha_new:
 * @alpha: the alpha value, between 1 and 65536
 *
 * Create a new foreground alpha attribute.
 *
 * Return value: (transfer full): the newly allocated
 *   `PangoAttribute`, which should be freed with
 *   [method@Pango.Attribute.destroy]
 *
 * Since: 1.38
 */
PangoAttribute *
pango_attr_foreground_alpha_new (guint16 alpha)
{
  return pango_attr_int_new (PANGO_ATTR_FOREGROUND_ALPHA, (int)alpha);
}

/**
 * pango_attr_background_alpha_new:
 * @alpha: the alpha value, between 1 and 65536
 *
 * Create a new background alpha attribute.
 *
 * Return value: (transfer full): the newly allocated
 *   `PangoAttribute`, which should be freed with
 *   [method@Pango.Attribute.destroy]
 *
 * Since: 1.38
 */
PangoAttribute *
pango_attr_background_alpha_new (guint16 alpha)
{
  return pango_attr_int_new (PANGO_ATTR_BACKGROUND_ALPHA, (int)alpha);
}

/**
 * pango_attr_allow_breaks_new:
 * @allow_breaks: %TRUE if we line breaks are allowed
 *
 * Create a new allow-breaks attribute.
 *
 * If breaks are disabled, the range will be kept in a
 * single run, as far as possible.
 *
 * Return value: (transfer full): the newly allocated
 *   `PangoAttribute`, which should be freed with
 *   [method@Pango.Attribute.destroy]
 *
 * Since: 1.44
 */
PangoAttribute *
pango_attr_allow_breaks_new (gboolean allow_breaks)
{
  return pango_attr_boolean_new (PANGO_ATTR_ALLOW_BREAKS, allow_breaks);
}

/**
 * pango_attr_insert_hyphens_new:
 * @insert_hyphens: %TRUE if hyphens should be inserted
 *
 * Create a new insert-hyphens attribute.
 *
 * Pango will insert hyphens when breaking lines in
 * the middle of a word. This attribute can be used
 * to suppress the hyphen.
 *
 * Return value: (transfer full): the newly allocated
 *   `PangoAttribute`, which should be freed with
 *   [method@Pango.Attribute.destroy]
 *
 * Since: 1.44
 */
PangoAttribute *
pango_attr_insert_hyphens_new (gboolean insert_hyphens)
{
  return pango_attr_boolean_new (PANGO_ATTR_INSERT_HYPHENS, insert_hyphens);
}

/**
 * pango_attr_show_new:
 * @flags: `PangoShowFlags` to apply
 *
 * Create a new attribute that influences how invisible
 * characters are rendered.
 *
 * Return value: (transfer full): the newly allocated
 *   `PangoAttribute`, which should be freed with
 *   [method@Pango.Attribute.destroy]
 *
 * Since: 1.44
 **/
PangoAttribute *
pango_attr_show_new (PangoShowFlags flags)
{
  return pango_attr_int_new (PANGO_ATTR_SHOW, (int)flags);
}

/**
 * pango_attr_word_new:
 *
 * Create a new attribute that marks its range
 * as a single word.
 *
 * Note that this may require adjustments to word and
 * sentence classification around the range.
 *
 * Return value: (transfer full): the newly allocated
 *   `PangoAttribute`, which should be freed with
 *   [method@Pango.Attribute.destroy]
 *
 * Since: 1.50
 */
PangoAttribute *
pango_attr_word_new (void)
{
  return pango_attr_boolean_new (PANGO_ATTR_WORD, TRUE);
}

/**
 * pango_attr_sentence_new:
 *
 * Create a new attribute that marks its range
 * as a single sentence.
 *
 * Note that this may require adjustments to word and
 * sentence classification around the range.
 *
 * Return value: (transfer full): the newly allocated
 *   `PangoAttribute`, which should be freed with
 *   [method@Pango.Attribute.destroy]
 *
 * Since: 1.50
 */
PangoAttribute *
pango_attr_sentence_new (void)
{
  return pango_attr_boolean_new (PANGO_ATTR_SENTENCE, TRUE);
}

/**
 * pango_attr_paragraph_new:
 *
 * Create a new attribute that marks its range as a single paragraph.
 *
 * Newlines and similar characters in the range of the attribute
 * will not be treated as paragraph separators.
 *
 * Return value: (transfer full): the newly allocated
 *   `PangoAttribute`, which should be freed with
 *   [method@Pango.Attribute.destroy]
 */
PangoAttribute *
pango_attr_paragraph_new (void)
{
  return pango_attr_boolean_new (PANGO_ATTR_SENTENCE, TRUE);
}

/**
 * pango_attr_overline_new:
 * @overline: the overline style
 *
 * Create a new overline-style attribute.
 *
 * Return value: (transfer full): the newly allocated
 *   `PangoAttribute`, which should be freed with
 *   [method@Pango.Attribute.destroy]
 *
 * Since: 1.46
 */
PangoAttribute *
pango_attr_overline_new (PangoOverline overline)
{
  return pango_attr_int_new (PANGO_ATTR_OVERLINE, (int)overline);
}

/**
 * pango_attr_overline_color_new:
 * @color: the color
 *
 * Create a new overline color attribute.
 *
 * This attribute modifies the color of overlines.
 * If not set, overlines will use the foreground color.
 *
 * Return value: (transfer full): the newly allocated
 *   `PangoAttribute`, which should be freed with
 *   [method@Pango.Attribute.destroy]
 *
 * Since: 1.46
 */
PangoAttribute *
pango_attr_overline_color_new (PangoColor *color)
{
  return pango_attr_color_new (PANGO_ATTR_OVERLINE_COLOR, color);
}

/**
 * pango_attr_line_height_new:
 * @factor: the scaling factor to apply to the logical height
 *
 * Create a new attribute that modifies the height
 * of logical line extents by a factor.
 *
 * This affects the values returned by
 * [method@Pango.Line.get_extents] and
 * [method@Pango.LineIter.get_line_extents].
 */
PangoAttribute *
pango_attr_line_height_new (double factor)
{
  return pango_attr_float_new (PANGO_ATTR_LINE_HEIGHT, factor);
}

/**
 * pango_attr_line_height_new_absolute:
 * @height: the line height, in %PANGO_SCALE-ths of a point
 *
 * Create a new attribute that overrides the height
 * of logical line extents to be @height.
 *
 * This affects the values returned by
 * [method@Pango.Line.get_extents],
 * [method@Pango.LineIter.get_line_extents].
 */
PangoAttribute *
pango_attr_line_height_new_absolute (int height)
{
  return pango_attr_int_new (PANGO_ATTR_ABSOLUTE_LINE_HEIGHT, height);
}

/**
 * pango_attr_line_spacing_new:
 * @spacing: the line spacing, in %PANGO_SCALE-ths of a point
 *
 * Create a new attribute that adds space to the
 * leading from font metrics, if not overridden
 * by line spacing attributes.
 *
 * This affects the values returned by
 * [method@Pango.Line.get_extents],
 * [method@Pango.LineIter.get_line_extents].
 */
PangoAttribute *
pango_attr_line_spacing_new (int spacing)
{
  return pango_attr_int_new (PANGO_ATTR_LINE_SPACING, spacing);
}

/**
 * pango_attr_text_transform_new:
 * @transform: `PangoTextTransform` to apply
 *
 * Create a new attribute that influences how characters
 * are transformed during shaping.
 *
 * Return value: (transfer full): the newly allocated
 *   `PangoAttribute`, which should be freed with
 *   [method@Pango.Attribute.destroy]
 *
 * Since: 1.50
 */
PangoAttribute *
pango_attr_text_transform_new (PangoTextTransform transform)
{
  return pango_attr_int_new (PANGO_ATTR_TEXT_TRANSFORM, transform);
}

/* }}} */
/* {{{ Private API */

gboolean
pango_attribute_affects_itemization (PangoAttribute *attr,
                                     gpointer        data)
{
  return (PANGO_ATTR_AFFECTS (attr) & PANGO_ATTR_AFFECTS_ITEMIZATION) != 0;
}

gboolean
pango_attribute_affects_break_or_shape (PangoAttribute *attr,
                                        gpointer        data)
{
  return (PANGO_ATTR_AFFECTS (attr) & (PANGO_ATTR_AFFECTS_BREAKING |
                                       PANGO_ATTR_AFFECTS_SHAPING)) != 0;
}

/* }}} */

/* vim:set foldmethod=marker expandtab: */
