/* Pango
 * pango-attributes.h: Attributed text
 *
 * Copyright (C) 2000 Red Hat Software
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

#pragma once

#include <pango/pango-font.h>
#include <pango/pango-color.h>
#include <pango/pango-attr.h>

G_BEGIN_DECLS


#define PANGO_ATTR_TYPE(value, affects, merge) (PANGO_ATTR_VALUE_##value | (PANGO_ATTR_AFFECTS_##affects << 8) | (PANGO_ATTR_MERGE_##merge << 12) | (__COUNTER__ << 16))
/**
 * PangoAttrType:
 * @PANGO_ATTR_INVALID: does not happen
 * @PANGO_ATTR_LANGUAGE: language
 * @PANGO_ATTR_FAMILY: font family name
 * @PANGO_ATTR_STYLE: font style
 * @PANGO_ATTR_WEIGHT: font weight
 * @PANGO_ATTR_VARIANT: font variant
 * @PANGO_ATTR_STRETCH: font stretch
 * @PANGO_ATTR_SIZE: font size in points scaled by `PANGO_SCALE`
 * @PANGO_ATTR_FONT_DESC: font description
 * @PANGO_ATTR_FOREGROUND: foreground color
 * @PANGO_ATTR_BACKGROUND: background color
 * @PANGO_ATTR_UNDERLINE: whether the text has an underline
 * @PANGO_ATTR_STRIKETHROUGH: whether the text is struck-through
 * @PANGO_ATTR_RISE: baseline displacement
 * @PANGO_ATTR_SCALE: font size scale factor
 * @PANGO_ATTR_FALLBACK: whether font fallback is enabled
 * @PANGO_ATTR_LETTER_SPACING: letter spacing in Pango units
 * @PANGO_ATTR_UNDERLINE_COLOR: underline color
 * @PANGO_ATTR_STRIKETHROUGH_COLOR: strikethrough color
 * @PANGO_ATTR_ABSOLUTE_SIZE: font size in pixels scaled by `PANGO_SCALE`
 * @PANGO_ATTR_GRAVITY: base text gravity
 * @PANGO_ATTR_GRAVITY_HINT: gravity hint
 * @PANGO_ATTR_FONT_FEATURES: OpenType font features
 * @PANGO_ATTR_FOREGROUND_ALPHA: foreground alpha
 * @PANGO_ATTR_BACKGROUND_ALPHA: background alpha
 * @PANGO_ATTR_ALLOW_BREAKS: whether line breaks are allowed
 * @PANGO_ATTR_SHOW: how to render invisible characters
 * @PANGO_ATTR_INSERT_HYPHENS: whether to insert hyphens at intra-word line breaks
 * @PANGO_ATTR_OVERLINE: whether the text has an overline
 * @PANGO_ATTR_OVERLINE_COLOR: overline color
 * @PANGO_ATTR_LINE_HEIGHT: line height factor
 * @PANGO_ATTR_ABSOLUTE_LINE_HEIGHT: line height in Pango units
 * @PANGO_ATTR_WORD: mark the range of the attribute as a single word
 * @PANGO_ATTR_SENTENCE: mark the range of the attribute as a single sentence
 * @PANGO_ATTR_PARAGRAPH: mark the range of the attribute as a single paragraph
 * @PANGO_ATTR_BASELINE_SHIFT: baseline displacement
 * @PANGO_ATTR_FONT_SCALE: font-relative size change
 * @PANGO_ATTR_LINE_SPACING: extra space to add to the leading from the
 *   font metrics (if not overridden by line height attribute)
 *
 * The `PangoAttrType` enumeration contains predefined types for attributes.
 *
 * Along with the predefined values, it is possible to allocate additional
 * values for custom attributes using [func@AttrType.register]. The predefined
 * values are given below.
 */
typedef enum
{
  PANGO_ATTR_INVALID,
  PANGO_ATTR_LANGUAGE             = PANGO_ATTR_TYPE (LANGUAGE, ITEMIZATION, OVERRIDES),
  PANGO_ATTR_FAMILY               = PANGO_ATTR_TYPE (STRING, ITEMIZATION, OVERRIDES),
  PANGO_ATTR_STYLE                = PANGO_ATTR_TYPE (INT, ITEMIZATION, OVERRIDES),
  PANGO_ATTR_WEIGHT               = PANGO_ATTR_TYPE (INT, ITEMIZATION, OVERRIDES),
  PANGO_ATTR_VARIANT              = PANGO_ATTR_TYPE (INT, ITEMIZATION, OVERRIDES),
  PANGO_ATTR_STRETCH              = PANGO_ATTR_TYPE (INT, ITEMIZATION, OVERRIDES),
  PANGO_ATTR_SIZE                 = PANGO_ATTR_TYPE (INT, ITEMIZATION, OVERRIDES),
  PANGO_ATTR_FONT_DESC            = PANGO_ATTR_TYPE (FONT_DESC, ITEMIZATION, ACCUMULATES),
  PANGO_ATTR_FOREGROUND           = PANGO_ATTR_TYPE (COLOR, RENDERING, OVERRIDES),
  PANGO_ATTR_BACKGROUND           = PANGO_ATTR_TYPE (COLOR, RENDERING, OVERRIDES),
  PANGO_ATTR_UNDERLINE            = PANGO_ATTR_TYPE (INT, RENDERING, OVERRIDES),
  PANGO_ATTR_STRIKETHROUGH        = PANGO_ATTR_TYPE (BOOLEAN, RENDERING, OVERRIDES),
  PANGO_ATTR_RISE                 = PANGO_ATTR_TYPE (INT, ITEMIZATION, OVERRIDES),
  PANGO_ATTR_SCALE                = PANGO_ATTR_TYPE (FLOAT, ITEMIZATION, OVERRIDES),
  PANGO_ATTR_FALLBACK             = PANGO_ATTR_TYPE (BOOLEAN, ITEMIZATION, OVERRIDES),
  PANGO_ATTR_LETTER_SPACING       = PANGO_ATTR_TYPE (INT, ITEMIZATION, OVERRIDES),
  PANGO_ATTR_UNDERLINE_COLOR      = PANGO_ATTR_TYPE (COLOR, RENDERING, OVERRIDES),
  PANGO_ATTR_STRIKETHROUGH_COLOR  = PANGO_ATTR_TYPE (COLOR, RENDERING, OVERRIDES),
  PANGO_ATTR_ABSOLUTE_SIZE        = PANGO_ATTR_TYPE (INT, ITEMIZATION, OVERRIDES),
  PANGO_ATTR_GRAVITY              = PANGO_ATTR_TYPE (INT, ITEMIZATION, OVERRIDES),
  PANGO_ATTR_GRAVITY_HINT         = PANGO_ATTR_TYPE (INT, ITEMIZATION, OVERRIDES),
  PANGO_ATTR_FONT_FEATURES        = PANGO_ATTR_TYPE (STRING, SHAPING, ACCUMULATES),
  PANGO_ATTR_FOREGROUND_ALPHA     = PANGO_ATTR_TYPE (INT, RENDERING, OVERRIDES),
  PANGO_ATTR_BACKGROUND_ALPHA     = PANGO_ATTR_TYPE (INT, RENDERING, OVERRIDES),
  PANGO_ATTR_ALLOW_BREAKS         = PANGO_ATTR_TYPE (BOOLEAN, BREAKING, OVERRIDES),
  PANGO_ATTR_SHOW                 = PANGO_ATTR_TYPE (INT, SHAPING, OVERRIDES),
  PANGO_ATTR_INSERT_HYPHENS       = PANGO_ATTR_TYPE (BOOLEAN, SHAPING, OVERRIDES),
  PANGO_ATTR_OVERLINE             = PANGO_ATTR_TYPE (INT, RENDERING, OVERRIDES),
  PANGO_ATTR_OVERLINE_COLOR       = PANGO_ATTR_TYPE (COLOR, RENDERING, OVERRIDES),
  PANGO_ATTR_LINE_HEIGHT          = PANGO_ATTR_TYPE (FLOAT, ITEMIZATION, OVERRIDES),
  PANGO_ATTR_ABSOLUTE_LINE_HEIGHT = PANGO_ATTR_TYPE (INT, ITEMIZATION, OVERRIDES),
  PANGO_ATTR_TEXT_TRANSFORM       = PANGO_ATTR_TYPE (INT, ITEMIZATION, OVERRIDES),
  PANGO_ATTR_WORD                 = PANGO_ATTR_TYPE (BOOLEAN, BREAKING, OVERRIDES),
  PANGO_ATTR_SENTENCE             = PANGO_ATTR_TYPE (BOOLEAN, BREAKING, OVERRIDES),
  PANGO_ATTR_PARAGRAPH            = PANGO_ATTR_TYPE (BOOLEAN, BREAKING, OVERRIDES),
  PANGO_ATTR_BASELINE_SHIFT       = PANGO_ATTR_TYPE (INT, ITEMIZATION, ACCUMULATES),
  PANGO_ATTR_FONT_SCALE           = PANGO_ATTR_TYPE (INT, ITEMIZATION, ACCUMULATES),
  PANGO_ATTR_LINE_SPACING         = PANGO_ATTR_TYPE (INT, ITEMIZATION, OVERRIDES),
} PangoAttrType;

#undef PANGO_ATTR_TYPE


PANGO_AVAILABLE_IN_ALL
PangoAttribute *        pango_attr_language_new                 (PangoLanguage              *language);
PANGO_AVAILABLE_IN_ALL
PangoAttribute *        pango_attr_family_new                   (const char                 *family);
PANGO_AVAILABLE_IN_ALL
PangoAttribute *        pango_attr_foreground_new               (PangoColor                 *color);
PANGO_AVAILABLE_IN_ALL
PangoAttribute *        pango_attr_background_new               (PangoColor                 *color);
PANGO_AVAILABLE_IN_ALL
PangoAttribute *        pango_attr_size_new                     (int                         size);
PANGO_AVAILABLE_IN_1_8
PangoAttribute *        pango_attr_size_new_absolute            (int                         size);
PANGO_AVAILABLE_IN_ALL
PangoAttribute *        pango_attr_style_new                    (PangoStyle                  style);
PANGO_AVAILABLE_IN_ALL
PangoAttribute *        pango_attr_weight_new                   (PangoWeight                 weight);
PANGO_AVAILABLE_IN_ALL
PangoAttribute *        pango_attr_variant_new                  (PangoVariant                variant);
PANGO_AVAILABLE_IN_ALL
PangoAttribute *        pango_attr_stretch_new                  (PangoStretch                stretch);
PANGO_AVAILABLE_IN_ALL
PangoAttribute *        pango_attr_font_desc_new                (const PangoFontDescription *desc);

/**
 * PangoUnderline:
 * @PANGO_UNDERLINE_NONE: no underline should be drawn
 * @PANGO_UNDERLINE_SINGLE: a single underline should be drawn
 * @PANGO_UNDERLINE_DOUBLE: a double underline should be drawn
 * @PANGO_UNDERLINE_LOW: a single underline should be drawn at a
 *   position beneath the ink extents of the text being
 *   underlined. This should be used only for underlining
 *   single characters, such as for keyboard accelerators.
 *   %PANGO_UNDERLINE_SINGLE should be used for extended
 *   portions of text.
 * @PANGO_UNDERLINE_ERROR: an underline indicating an error should
 *   be drawn below. The exact style of rendering is up to the
 *   `PangoRenderer` in use, but typical styles include wavy
 *   or dotted lines.
 *   This underline is typically used to indicate an error such
 *   as a possible mispelling; in some cases a contrasting color
 *   may automatically be used. This type of underlining is
 *   available since Pango 1.4.
 * @PANGO_UNDERLINE_SINGLE_LINE: Like @PANGO_UNDERLINE_SINGLE, but
 *   drawn continuously across multiple runs. This type
 *   of underlining is available since Pango 1.46.
 * @PANGO_UNDERLINE_DOUBLE_LINE: Like @PANGO_UNDERLINE_DOUBLE, but
 *   drawn continuously across multiple runs. This type
 *   of underlining is available since Pango 1.46.
 * @PANGO_UNDERLINE_ERROR_LINE: Like @PANGO_UNDERLINE_ERROR, but
 *   drawn continuously across multiple runs. This type
 *   of underlining is available since Pango 1.46.
 *
 * The `PangoUnderline` enumeration is used to specify whether text
 * should be underlined, and if so, the type of underlining.
 */
typedef enum {
  PANGO_UNDERLINE_NONE,
  PANGO_UNDERLINE_SINGLE,
  PANGO_UNDERLINE_DOUBLE,
  PANGO_UNDERLINE_LOW,
  PANGO_UNDERLINE_ERROR,
  PANGO_UNDERLINE_SINGLE_LINE,
  PANGO_UNDERLINE_DOUBLE_LINE,
  PANGO_UNDERLINE_ERROR_LINE
} PangoUnderline;

PANGO_AVAILABLE_IN_ALL
PangoAttribute *        pango_attr_underline_new                (PangoUnderline              underline);
PANGO_AVAILABLE_IN_1_8
PangoAttribute *        pango_attr_underline_color_new          (PangoColor                 *color);
PANGO_AVAILABLE_IN_ALL
PangoAttribute *        pango_attr_strikethrough_new            (gboolean                    strikethrough);
PANGO_AVAILABLE_IN_1_8
PangoAttribute *        pango_attr_strikethrough_color_new      (PangoColor                 *color);
PANGO_AVAILABLE_IN_ALL
PangoAttribute *        pango_attr_rise_new                     (int                         rise);

/**
 * PangoBaselineShift:
 * @PANGO_BASELINE_SHIFT_NONE: Leave the baseline unchanged
 * @PANGO_BASELINE_SHIFT_SUPERSCRIPT: Shift the baseline to the superscript position,
 *   relative to the previous run
 * @PANGO_BASELINE_SHIFT_SUBSCRIPT: Shift the baseline to the subscript position,
 *   relative to the previous run
 *
 * An enumeration that affects baseline shifts between runs.
 *
 * Since: 1.50
 */
typedef enum {
  PANGO_BASELINE_SHIFT_NONE,
  PANGO_BASELINE_SHIFT_SUPERSCRIPT,
  PANGO_BASELINE_SHIFT_SUBSCRIPT,
} PangoBaselineShift;

PANGO_AVAILABLE_IN_1_50
PangoAttribute *        pango_attr_baseline_shift_new           (int                         shift);

/**
 * PangoFontScale:
 * @PANGO_FONT_SCALE_NONE: Leave the font size unchanged
 * @PANGO_FONT_SCALE_SUPERSCRIPT: Change the font to a size suitable for superscripts
 * @PANGO_FONT_SCALE_SUBSCRIPT: Change the font to a size suitable for subscripts
 * @PANGO_FONT_SCALE_SMALL_CAPS: Change the font to a size suitable for Small Caps
 *
 * An enumeration that affects font sizes for superscript
 * and subscript positioning and for (emulated) Small Caps.
 *
 * Since: 1.50
 */
typedef enum {
  PANGO_FONT_SCALE_NONE,
  PANGO_FONT_SCALE_SUPERSCRIPT,
  PANGO_FONT_SCALE_SUBSCRIPT,
  PANGO_FONT_SCALE_SMALL_CAPS,
} PangoFontScale;

PANGO_AVAILABLE_IN_1_50
PangoAttribute *        pango_attr_font_scale_new               (PangoFontScale              scale);
PANGO_AVAILABLE_IN_ALL
PangoAttribute *        pango_attr_scale_new                    (double                      scale_factor);
PANGO_AVAILABLE_IN_1_4
PangoAttribute *        pango_attr_fallback_new                 (gboolean                    enable_fallback);
PANGO_AVAILABLE_IN_1_6
PangoAttribute *        pango_attr_letter_spacing_new           (int                         letter_spacing);
PANGO_AVAILABLE_IN_1_16
PangoAttribute *        pango_attr_gravity_new                  (PangoGravity                 gravity);
PANGO_AVAILABLE_IN_1_16
PangoAttribute *        pango_attr_gravity_hint_new             (PangoGravityHint             hint);
PANGO_AVAILABLE_IN_1_38
PangoAttribute *        pango_attr_font_features_new            (const char                  *features);
PANGO_AVAILABLE_IN_1_38
PangoAttribute *        pango_attr_foreground_alpha_new         (guint16                      alpha);
PANGO_AVAILABLE_IN_1_38
PangoAttribute *        pango_attr_background_alpha_new         (guint16                      alpha);
PANGO_AVAILABLE_IN_1_44
PangoAttribute *        pango_attr_allow_breaks_new             (gboolean                     allow_breaks);
PANGO_AVAILABLE_IN_1_50
PangoAttribute *        pango_attr_word_new                     (void);
PANGO_AVAILABLE_IN_1_50
PangoAttribute *        pango_attr_sentence_new                 (void);
PANGO_AVAILABLE_IN_ALL
PangoAttribute *        pango_attr_paragraph_new                (void);

PANGO_AVAILABLE_IN_1_44
PangoAttribute *        pango_attr_insert_hyphens_new           (gboolean                     insert_hyphens);

/**
 * PangoOverline:
 * @PANGO_OVERLINE_NONE: no overline should be drawn
 * @PANGO_OVERLINE_SINGLE: Draw a single line above the ink
 *   extents of the text being underlined.
 *
 * The `PangoOverline` enumeration is used to specify whether text
 * should be overlined, and if so, the type of line.
 *
 * Since: 1.46
 */
typedef enum {
  PANGO_OVERLINE_NONE,
  PANGO_OVERLINE_SINGLE
} PangoOverline;

PANGO_AVAILABLE_IN_1_46
PangoAttribute *        pango_attr_overline_new                 (PangoOverline               overline);
PANGO_AVAILABLE_IN_1_46
PangoAttribute *        pango_attr_overline_color_new           (PangoColor                 *color);

/**
 * PangoShowFlags:
 * @PANGO_SHOW_NONE: No special treatment for invisible characters
 * @PANGO_SHOW_SPACES: Render spaces, tabs and newlines visibly
 * @PANGO_SHOW_LINE_BREAKS: Render line breaks visibly
 * @PANGO_SHOW_IGNORABLES: Render default-ignorable Unicode
 *   characters visibly
 *
 * These flags affect how Pango treats characters that are normally
 * not visible in the output.
 *
 * Since: 1.44
 */
typedef enum {
  PANGO_SHOW_NONE        = 0,
  PANGO_SHOW_SPACES      = 1 << 0,
  PANGO_SHOW_LINE_BREAKS = 1 << 1,
  PANGO_SHOW_IGNORABLES  = 1 << 2
} PangoShowFlags;

PANGO_AVAILABLE_IN_1_44
PangoAttribute *        pango_attr_show_new                     (PangoShowFlags               flags);
PANGO_AVAILABLE_IN_1_50
PangoAttribute *        pango_attr_line_height_new              (double                       factor);
PANGO_AVAILABLE_IN_1_50
PangoAttribute *        pango_attr_line_height_new_absolute     (int                          height);
PANGO_AVAILABLE_IN_ALL
PangoAttribute *        pango_attr_line_spacing_new             (int                          spacing);

/*
 * PangoTextTransform:
 * @PANGO_TEXT_TRANSFORM_NONE: Leave text unchanged
 * @PANGO_TEXT_TRANSFORM_LOWERCASE: Display letters and numbers as lowercase
 * @PANGO_TEXT_TRANSFORM_UPPERCASE: Display letters and numbers as uppercase
 * @PANGO_TEXT_TRANSFORM_CAPITALIZE: Display the first character of a word
 *   in titlecase
 *
 * An enumeration that affects how Pango treats characters during shaping.
  */
typedef enum {
  PANGO_TEXT_TRANSFORM_NONE,
  PANGO_TEXT_TRANSFORM_LOWERCASE,
  PANGO_TEXT_TRANSFORM_UPPERCASE,
  PANGO_TEXT_TRANSFORM_CAPITALIZE,
} PangoTextTransform;

PANGO_AVAILABLE_IN_1_50
PangoAttribute *        pango_attr_text_transform_new           (PangoTextTransform transform);


G_END_DECLS
