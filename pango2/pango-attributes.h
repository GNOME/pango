/*
 * Copyright (C) 2000 Red Hat Software
 *
 * SPDX-License-Identifier: LGPL-2.1-or-later
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public
 * License as published by the Free Software Foundation; either
 * version 2.1 of the License, or (at your option) any later version.
 *
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public
 * License along with this library; if not, see <http://www.gnu.org/licenses/>.
 */

#pragma once

#include <pango2/pango-font.h>
#include <pango2/pango-color.h>
#include <pango2/pango-attr.h>

G_BEGIN_DECLS

#ifndef __GI_SCANNER__

#define PANGO2_ATTR_TYPE(value, affects, merge) (PANGO2_ATTR_VALUE_##value | (PANGO2_ATTR_AFFECTS_##affects << 8) | (PANGO2_ATTR_MERGE_##merge << 12) | (__COUNTER__ << 16))

#endif

/**
 * Pango2AttrType:
 * @PANGO2_ATTR_INVALID: does not happen
 * @PANGO2_ATTR_LANGUAGE: language
 * @PANGO2_ATTR_FAMILY: font family name
 * @PANGO2_ATTR_STYLE: font style
 * @PANGO2_ATTR_WEIGHT: font weight
 * @PANGO2_ATTR_VARIANT: font variant
 * @PANGO2_ATTR_STRETCH: font stretch
 * @PANGO2_ATTR_SIZE: font size in points scaled by `PANGO2_SCALE`
 * @PANGO2_ATTR_FONT_DESC: font description
 * @PANGO2_ATTR_FOREGROUND: foreground color
 * @PANGO2_ATTR_BACKGROUND: background color
 * @PANGO2_ATTR_UNDERLINE: underline style
 * @PANGO2_ATTR_UNDERLINE_POSITION: underline position
 * @PANGO2_ATTR_STRIKETHROUGH: whether the text is struck-through
 * @PANGO2_ATTR_RISE: baseline displacement
 * @PANGO2_ATTR_SCALE: font size scale factor
 * @PANGO2_ATTR_FALLBACK: whether font fallback is enabled
 * @PANGO2_ATTR_LETTER_SPACING: letter spacing in Pango units
 * @PANGO2_ATTR_UNDERLINE_COLOR: underline color
 * @PANGO2_ATTR_STRIKETHROUGH_COLOR: strikethrough color
 * @PANGO2_ATTR_ABSOLUTE_SIZE: font size in pixels scaled by `PANGO2_SCALE`
 * @PANGO2_ATTR_GRAVITY: base text gravity
 * @PANGO2_ATTR_GRAVITY_HINT: gravity hint
 * @PANGO2_ATTR_FONT_FEATURES: OpenType font features
 * @PANGO2_ATTR_PALETTE: Color palette name
 * @PANGO2_ATTR_ALLOW_BREAKS: whether line breaks are allowed
 * @PANGO2_ATTR_SHOW: how to render invisible characters
 * @PANGO2_ATTR_INSERT_HYPHENS: whether to insert hyphens at intra-word line breaks
 * @PANGO2_ATTR_OVERLINE: whether the text has an overline
 * @PANGO2_ATTR_OVERLINE_COLOR: overline color
 * @PANGO2_ATTR_LINE_HEIGHT: line height factor
 * @PANGO2_ATTR_ABSOLUTE_LINE_HEIGHT: line height in Pango units
 * @PANGO2_ATTR_WORD: mark the range of the attribute as a single word
 * @PANGO2_ATTR_SENTENCE: mark the range of the attribute as a single sentence
 * @PANGO2_ATTR_PARAGRAPH: mark the range of the attribute as a single paragraph
 * @PANGO2_ATTR_BASELINE_SHIFT: baseline displacement
 * @PANGO2_ATTR_FONT_SCALE: font-relative size change
 * @PANGO2_ATTR_LINE_SPACING: space to add to the leading from the
 *   font metrics (if not overridden by a line height attribute)
 * @PANGO2_ATTR_SHAPE: override glyph shapes (requires renderer support)
 * @PANGO2_ATTR_EMOJI_PRESENTATION: override Emoji presentation
 *
 * Predefined attribute types.
 *
 * Along with the predefined values, it is possible to allocate additional
 * values for custom attributes using [func@AttrType.register].
 */
typedef enum
{
  PANGO2_ATTR_INVALID,
  PANGO2_ATTR_LANGUAGE             = PANGO2_ATTR_TYPE (LANGUAGE, ITEMIZATION, OVERRIDES),
  PANGO2_ATTR_FAMILY               = PANGO2_ATTR_TYPE (STRING, ITEMIZATION, OVERRIDES),
  PANGO2_ATTR_STYLE                = PANGO2_ATTR_TYPE (INT, ITEMIZATION, OVERRIDES),
  PANGO2_ATTR_WEIGHT               = PANGO2_ATTR_TYPE (INT, ITEMIZATION, OVERRIDES),
  PANGO2_ATTR_VARIANT              = PANGO2_ATTR_TYPE (INT, ITEMIZATION, OVERRIDES),
  PANGO2_ATTR_STRETCH              = PANGO2_ATTR_TYPE (INT, ITEMIZATION, OVERRIDES),
  PANGO2_ATTR_SIZE                 = PANGO2_ATTR_TYPE (INT, ITEMIZATION, OVERRIDES),
  PANGO2_ATTR_FONT_DESC            = PANGO2_ATTR_TYPE (FONT_DESC, ITEMIZATION, ACCUMULATES),
  PANGO2_ATTR_FOREGROUND           = PANGO2_ATTR_TYPE (COLOR, RENDERING, OVERRIDES),
  PANGO2_ATTR_BACKGROUND           = PANGO2_ATTR_TYPE (COLOR, RENDERING, OVERRIDES),
  PANGO2_ATTR_UNDERLINE            = PANGO2_ATTR_TYPE (INT, RENDERING, OVERRIDES),
  PANGO2_ATTR_UNDERLINE_POSITION   = PANGO2_ATTR_TYPE (INT, RENDERING, OVERRIDES),
  PANGO2_ATTR_STRIKETHROUGH        = PANGO2_ATTR_TYPE (INT, RENDERING, OVERRIDES),
  PANGO2_ATTR_RISE                 = PANGO2_ATTR_TYPE (INT, ITEMIZATION, OVERRIDES),
  PANGO2_ATTR_SCALE                = PANGO2_ATTR_TYPE (FLOAT, ITEMIZATION, OVERRIDES),
  PANGO2_ATTR_FALLBACK             = PANGO2_ATTR_TYPE (BOOLEAN, ITEMIZATION, OVERRIDES),
  PANGO2_ATTR_LETTER_SPACING       = PANGO2_ATTR_TYPE (INT, ITEMIZATION, OVERRIDES),
  PANGO2_ATTR_UNDERLINE_COLOR      = PANGO2_ATTR_TYPE (COLOR, RENDERING, OVERRIDES),
  PANGO2_ATTR_STRIKETHROUGH_COLOR  = PANGO2_ATTR_TYPE (COLOR, RENDERING, OVERRIDES),
  PANGO2_ATTR_ABSOLUTE_SIZE        = PANGO2_ATTR_TYPE (INT, ITEMIZATION, OVERRIDES),
  PANGO2_ATTR_GRAVITY              = PANGO2_ATTR_TYPE (INT, ITEMIZATION, OVERRIDES),
  PANGO2_ATTR_GRAVITY_HINT         = PANGO2_ATTR_TYPE (INT, ITEMIZATION, OVERRIDES),
  PANGO2_ATTR_FONT_FEATURES        = PANGO2_ATTR_TYPE (STRING, SHAPING, ACCUMULATES),
  PANGO2_ATTR_PALETTE              = PANGO2_ATTR_TYPE (STRING, ITEMIZATION, OVERRIDES),
  PANGO2_ATTR_ALLOW_BREAKS         = PANGO2_ATTR_TYPE (BOOLEAN, BREAKING, OVERRIDES),
  PANGO2_ATTR_SHOW                 = PANGO2_ATTR_TYPE (INT, SHAPING, OVERRIDES),
  PANGO2_ATTR_INSERT_HYPHENS       = PANGO2_ATTR_TYPE (BOOLEAN, SHAPING, OVERRIDES),
  PANGO2_ATTR_OVERLINE             = PANGO2_ATTR_TYPE (INT, RENDERING, OVERRIDES),
  PANGO2_ATTR_OVERLINE_COLOR       = PANGO2_ATTR_TYPE (COLOR, RENDERING, OVERRIDES),
  PANGO2_ATTR_LINE_HEIGHT          = PANGO2_ATTR_TYPE (FLOAT, ITEMIZATION, OVERRIDES),
  PANGO2_ATTR_ABSOLUTE_LINE_HEIGHT = PANGO2_ATTR_TYPE (INT, ITEMIZATION, OVERRIDES),
  PANGO2_ATTR_TEXT_TRANSFORM       = PANGO2_ATTR_TYPE (INT, ITEMIZATION, OVERRIDES),
  PANGO2_ATTR_WORD                 = PANGO2_ATTR_TYPE (BOOLEAN, BREAKING, OVERRIDES),
  PANGO2_ATTR_SENTENCE             = PANGO2_ATTR_TYPE (BOOLEAN, BREAKING, OVERRIDES),
  PANGO2_ATTR_PARAGRAPH            = PANGO2_ATTR_TYPE (BOOLEAN, BREAKING, OVERRIDES),
  PANGO2_ATTR_BASELINE_SHIFT       = PANGO2_ATTR_TYPE (INT, ITEMIZATION, ACCUMULATES),
  PANGO2_ATTR_FONT_SCALE           = PANGO2_ATTR_TYPE (INT, ITEMIZATION, ACCUMULATES),
  PANGO2_ATTR_LINE_SPACING         = PANGO2_ATTR_TYPE (INT, ITEMIZATION, OVERRIDES),
  PANGO2_ATTR_SHAPE                = PANGO2_ATTR_TYPE (POINTER, ITEMIZATION, OVERRIDES),
  PANGO2_ATTR_EMOJI_PRESENTATION   = PANGO2_ATTR_TYPE (INT, ITEMIZATION, OVERRIDES),
} Pango2AttrType;

#undef PANGO2_ATTR_TYPE

PANGO2_AVAILABLE_IN_ALL
Pango2Attribute *        pango2_attr_language_new                 (Pango2Language              *language);
PANGO2_AVAILABLE_IN_ALL
Pango2Attribute *        pango2_attr_family_new                   (const char                  *family);
PANGO2_AVAILABLE_IN_ALL
Pango2Attribute *        pango2_attr_foreground_new               (Pango2Color                 *color);
PANGO2_AVAILABLE_IN_ALL
Pango2Attribute *        pango2_attr_background_new               (Pango2Color                 *color);
PANGO2_AVAILABLE_IN_ALL
Pango2Attribute *        pango2_attr_size_new                     (int                          size);
PANGO2_AVAILABLE_IN_ALL
Pango2Attribute *        pango2_attr_size_new_absolute            (int                          size);
PANGO2_AVAILABLE_IN_ALL
Pango2Attribute *        pango2_attr_style_new                    (Pango2Style                  style);
PANGO2_AVAILABLE_IN_ALL
Pango2Attribute *        pango2_attr_weight_new                   (Pango2Weight                 weight);
PANGO2_AVAILABLE_IN_ALL
Pango2Attribute *        pango2_attr_variant_new                  (Pango2Variant                variant);
PANGO2_AVAILABLE_IN_ALL
Pango2Attribute *        pango2_attr_stretch_new                  (Pango2Stretch                stretch);
PANGO2_AVAILABLE_IN_ALL
Pango2Attribute *        pango2_attr_font_desc_new                (const Pango2FontDescription *desc);

/**
 * Pango2LineStyle:
 * @PANGO2_LINE_STYLE_NONE: No visible line
 * @PANGO2_LINE_STYLE_SOLID: A single line
 * @PANGO2_LINE_STYLE_DOUBLE: A double line
 * @PANGO2_LINE_STYLE_DASHED: A dashed line
 * @PANGO2_LINE_STYLE_DOTTED: A dotted line
 * @PANGO2_LINE_STYLE_WAVY: A wavy line
 *
 * Specifies how lines should be drawn.
 */
typedef enum {
  PANGO2_LINE_STYLE_NONE,
  PANGO2_LINE_STYLE_SOLID,
  PANGO2_LINE_STYLE_DOUBLE,
  PANGO2_LINE_STYLE_DASHED,
  PANGO2_LINE_STYLE_DOTTED,
  PANGO2_LINE_STYLE_WAVY,
} Pango2LineStyle;

PANGO2_AVAILABLE_IN_ALL
Pango2Attribute *        pango2_attr_underline_new                (Pango2LineStyle              style);

PANGO2_AVAILABLE_IN_ALL
Pango2Attribute *        pango2_attr_underline_color_new          (Pango2Color                 *color);

/**
 * Pango2UnderlinePosition:
 * @PANGO2_UNDERLINE_POSITION_NORMAL: As specified by font metrics
 * @PANGO2_UNDERLINE_POSITION_UNDER: Below the ink extents of the run
 *
 * Specifies where underlines should be drawn.
 */
typedef enum {
  PANGO2_UNDERLINE_POSITION_NORMAL,
  PANGO2_UNDERLINE_POSITION_UNDER
} Pango2UnderlinePosition;

PANGO2_AVAILABLE_IN_ALL
Pango2Attribute *        pango2_attr_underline_position_new       (Pango2UnderlinePosition      position);

PANGO2_AVAILABLE_IN_ALL
Pango2Attribute *        pango2_attr_strikethrough_new            (Pango2LineStyle              style);
PANGO2_AVAILABLE_IN_ALL
Pango2Attribute *        pango2_attr_strikethrough_color_new      (Pango2Color                 *color);
PANGO2_AVAILABLE_IN_ALL
Pango2Attribute *        pango2_attr_rise_new                     (int                          rise);

/**
 * Pango2BaselineShift:
 * @PANGO2_BASELINE_SHIFT_NONE: Leave the baseline unchanged
 * @PANGO2_BASELINE_SHIFT_SUPERSCRIPT: Shift the baseline to the superscript position,
 *   relative to the previous run
 * @PANGO2_BASELINE_SHIFT_SUBSCRIPT: Shift the baseline to the subscript position,
 *   relative to the previous run
 *
 * Influences how baselines are changed between runs.
 */
typedef enum {
  PANGO2_BASELINE_SHIFT_NONE,
  PANGO2_BASELINE_SHIFT_SUPERSCRIPT,
  PANGO2_BASELINE_SHIFT_SUBSCRIPT,
} Pango2BaselineShift;

PANGO2_AVAILABLE_IN_ALL
Pango2Attribute *        pango2_attr_baseline_shift_new           (int                          shift);

/**
 * Pango2FontScale:
 * @PANGO2_FONT_SCALE_NONE: Leave the font size unchanged
 * @PANGO2_FONT_SCALE_SUPERSCRIPT: Change the font to a size suitable for superscripts
 * @PANGO2_FONT_SCALE_SUBSCRIPT: Change the font to a size suitable for subscripts
 * @PANGO2_FONT_SCALE_SMALL_CAPS: Change the font to a size suitable for Small Caps
 *
 * Influences the font size of a run.
 */
typedef enum {
  PANGO2_FONT_SCALE_NONE,
  PANGO2_FONT_SCALE_SUPERSCRIPT,
  PANGO2_FONT_SCALE_SUBSCRIPT,
  PANGO2_FONT_SCALE_SMALL_CAPS,
} Pango2FontScale;

PANGO2_AVAILABLE_IN_ALL
Pango2Attribute *        pango2_attr_font_scale_new               (Pango2FontScale              scale);
PANGO2_AVAILABLE_IN_ALL
Pango2Attribute *        pango2_attr_scale_new                    (double                       scale_factor);
PANGO2_AVAILABLE_IN_ALL
Pango2Attribute *        pango2_attr_fallback_new                 (gboolean                     enable_fallback);
PANGO2_AVAILABLE_IN_ALL
Pango2Attribute *        pango2_attr_letter_spacing_new           (int                          letter_spacing);
PANGO2_AVAILABLE_IN_ALL
Pango2Attribute *        pango2_attr_gravity_new                  (Pango2Gravity                gravity);
PANGO2_AVAILABLE_IN_ALL
Pango2Attribute *        pango2_attr_gravity_hint_new             (Pango2GravityHint            hint);
PANGO2_AVAILABLE_IN_ALL
Pango2Attribute *        pango2_attr_font_features_new            (const char                  *features);
PANGO2_AVAILABLE_IN_ALL
Pango2Attribute *        pango2_attr_palette_new                  (const char                  *palette);
PANGO2_AVAILABLE_IN_ALL
Pango2Attribute *        pango2_attr_allow_breaks_new             (gboolean                     allow_breaks);
PANGO2_AVAILABLE_IN_ALL
Pango2Attribute *        pango2_attr_word_new                     (void);
PANGO2_AVAILABLE_IN_ALL
Pango2Attribute *        pango2_attr_sentence_new                 (void);
PANGO2_AVAILABLE_IN_ALL
Pango2Attribute *        pango2_attr_paragraph_new                (void);

PANGO2_AVAILABLE_IN_ALL
Pango2Attribute *        pango2_attr_insert_hyphens_new           (gboolean                     insert_hyphens);
PANGO2_AVAILABLE_IN_ALL
Pango2Attribute *        pango2_attr_overline_new                 (Pango2LineStyle              style);
PANGO2_AVAILABLE_IN_ALL
Pango2Attribute *        pango2_attr_overline_color_new           (Pango2Color                 *color);

/**
 * Pango2ShowFlags:
 * @PANGO2_SHOW_NONE: No special treatment for invisible characters
 * @PANGO2_SHOW_SPACES: Render spaces, tabs and newlines visibly
 * @PANGO2_SHOW_LINE_BREAKS: Render line breaks visibly
 * @PANGO2_SHOW_IGNORABLES: Render default-ignorable Unicode
 *   characters visibly
 *
 * Affects how Pango2 treats characters that are normally
 * not visible in the output.
 */
typedef enum {
  PANGO2_SHOW_NONE        = 0,
  PANGO2_SHOW_SPACES      = 1 << 0,
  PANGO2_SHOW_LINE_BREAKS = 1 << 1,
  PANGO2_SHOW_IGNORABLES  = 1 << 2
} Pango2ShowFlags;

PANGO2_AVAILABLE_IN_ALL
Pango2Attribute *        pango2_attr_show_new                     (Pango2ShowFlags              flags);
PANGO2_AVAILABLE_IN_ALL
Pango2Attribute *        pango2_attr_line_height_new              (double                       factor);
PANGO2_AVAILABLE_IN_ALL
Pango2Attribute *        pango2_attr_line_height_new_absolute     (int                          height);
PANGO2_AVAILABLE_IN_ALL
Pango2Attribute *        pango2_attr_line_spacing_new             (int                          spacing);

/**
 * Pango2TextTransform:
 * @PANGO2_TEXT_TRANSFORM_NONE: Leave text unchanged
 * @PANGO2_TEXT_TRANSFORM_LOWERCASE: Display letters and numbers as lowercase
 * @PANGO2_TEXT_TRANSFORM_UPPERCASE: Display letters and numbers as uppercase
 * @PANGO2_TEXT_TRANSFORM_CAPITALIZE: Display the first character of a word
 *   in titlecase
 *
 * Determines how to change the case of characters during shaping.
  */
typedef enum {
  PANGO2_TEXT_TRANSFORM_NONE,
  PANGO2_TEXT_TRANSFORM_LOWERCASE,
  PANGO2_TEXT_TRANSFORM_UPPERCASE,
  PANGO2_TEXT_TRANSFORM_CAPITALIZE,
} Pango2TextTransform;

PANGO2_AVAILABLE_IN_ALL
Pango2Attribute *        pango2_attr_text_transform_new           (Pango2TextTransform     transform);

PANGO2_AVAILABLE_IN_ALL
Pango2Attribute *        pango2_attr_shape_new                    (Pango2Rectangle        *ink_rect,
                                                                   Pango2Rectangle        *logical_rect,
                                                                   gpointer                data,
                                                                   Pango2AttrDataCopyFunc  copy,
                                                                   GDestroyNotify          destroy);

PANGO2_AVAILABLE_IN_ALL
Pango2Attribute *        pango2_attr_emoji_presentation_new       (Pango2EmojiPresentation presentation);

G_END_DECLS
