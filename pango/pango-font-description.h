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

#include <pango/pango-types.h>

#include <glib-object.h>
#include <hb.h>

G_BEGIN_DECLS

/**
 * PangoFontDescription:
 *
 * A `PangoFontDescription` describes a font in an implementation-independent
 * manner.
 *
 * `PangoFontDescription` structures are used both to list what fonts are
 * available on the system and also for specifying the characteristics of
 * a font to load.
 */
typedef struct _PangoFontDescription PangoFontDescription;

/**
 * PangoStyle:
 * @PANGO_STYLE_NORMAL: the font is upright.
 * @PANGO_STYLE_OBLIQUE: the font is slanted, but in a roman style.
 * @PANGO_STYLE_ITALIC: the font is slanted in an italic style.
 *
 * An enumeration specifying the various slant styles possible for a font.
 **/
typedef enum {
  PANGO_STYLE_NORMAL,
  PANGO_STYLE_OBLIQUE,
  PANGO_STYLE_ITALIC
} PangoStyle;

/**
 * PangoVariant:
 * @PANGO_VARIANT_NORMAL: A normal font.
 * @PANGO_VARIANT_SMALL_CAPS: A font with the lower case characters
 *   replaced by smaller variants of the capital characters.
 * @PANGO_VARIANT_ALL_SMALL_CAPS: A font with all characters
 *   replaced by smaller variants of the capital characters. Since: 1.50
 * @PANGO_VARIANT_PETITE_CAPS: A font with the lower case characters
 *   replaced by smaller variants of the capital characters.
 *   Petite Caps can be even smaller than Small Caps. Since: 1.50
 * @PANGO_VARIANT_ALL_PETITE_CAPS: A font with all characters
 *   replaced by smaller variants of the capital characters.
 *   Petite Caps can be even smaller than Small Caps. Since: 1.50
 * @PANGO_VARIANT_UNICASE: A font with the upper case characters
 *   replaced by smaller variants of the capital letters. Since: 1.50
 * @PANGO_VARIANT_TITLE_CAPS: A font with capital letters that
 *   are more suitable for all-uppercase titles. Since: 1.50
 *
 * An enumeration specifying capitalization variant of the font.
 */
typedef enum {
  PANGO_VARIANT_NORMAL,
  PANGO_VARIANT_SMALL_CAPS,
  PANGO_VARIANT_ALL_SMALL_CAPS,
  PANGO_VARIANT_PETITE_CAPS,
  PANGO_VARIANT_ALL_PETITE_CAPS,
  PANGO_VARIANT_UNICASE,
  PANGO_VARIANT_TITLE_CAPS
} PangoVariant;

/**
 * PangoWeight:
 * @PANGO_WEIGHT_THIN: the thin weight (= 100) Since: 1.24
 * @PANGO_WEIGHT_ULTRALIGHT: the ultralight weight (= 200)
 * @PANGO_WEIGHT_LIGHT: the light weight (= 300)
 * @PANGO_WEIGHT_SEMILIGHT: the semilight weight (= 350) Since: 1.36.7
 * @PANGO_WEIGHT_BOOK: the book weight (= 380) Since: 1.24)
 * @PANGO_WEIGHT_NORMAL: the default weight (= 400)
 * @PANGO_WEIGHT_MEDIUM: the normal weight (= 500) Since: 1.24
 * @PANGO_WEIGHT_SEMIBOLD: the semibold weight (= 600)
 * @PANGO_WEIGHT_BOLD: the bold weight (= 700)
 * @PANGO_WEIGHT_ULTRABOLD: the ultrabold weight (= 800)
 * @PANGO_WEIGHT_HEAVY: the heavy weight (= 900)
 * @PANGO_WEIGHT_ULTRAHEAVY: the ultraheavy weight (= 1000) Since: 1.24
 *
 * An enumeration specifying the weight (boldness) of a font.
 *
 * Weight is specified as a numeric value ranging from 100 to 1000.
 * This enumeration simply provides some common, predefined values.
 */
typedef enum {
  PANGO_WEIGHT_THIN = 100,
  PANGO_WEIGHT_ULTRALIGHT = 200,
  PANGO_WEIGHT_LIGHT = 300,
  PANGO_WEIGHT_SEMILIGHT = 350,
  PANGO_WEIGHT_BOOK = 380,
  PANGO_WEIGHT_NORMAL = 400,
  PANGO_WEIGHT_MEDIUM = 500,
  PANGO_WEIGHT_SEMIBOLD = 600,
  PANGO_WEIGHT_BOLD = 700,
  PANGO_WEIGHT_ULTRABOLD = 800,
  PANGO_WEIGHT_HEAVY = 900,
  PANGO_WEIGHT_ULTRAHEAVY = 1000
} PangoWeight;

/**
 * PangoStretch:
 * @PANGO_STRETCH_ULTRA_CONDENSED: ultra condensed width
 * @PANGO_STRETCH_EXTRA_CONDENSED: extra condensed width
 * @PANGO_STRETCH_CONDENSED: condensed width
 * @PANGO_STRETCH_SEMI_CONDENSED: semi condensed width
 * @PANGO_STRETCH_NORMAL: the normal width
 * @PANGO_STRETCH_SEMI_EXPANDED: semi expanded width
 * @PANGO_STRETCH_EXPANDED: expanded width
 * @PANGO_STRETCH_EXTRA_EXPANDED: extra expanded width
 * @PANGO_STRETCH_ULTRA_EXPANDED: ultra expanded width
 *
 * An enumeration specifying the width of the font relative to other designs
 * within a family.
 */
typedef enum {
  PANGO_STRETCH_ULTRA_CONDENSED,
  PANGO_STRETCH_EXTRA_CONDENSED,
  PANGO_STRETCH_CONDENSED,
  PANGO_STRETCH_SEMI_CONDENSED,
  PANGO_STRETCH_NORMAL,
  PANGO_STRETCH_SEMI_EXPANDED,
  PANGO_STRETCH_EXPANDED,
  PANGO_STRETCH_EXTRA_EXPANDED,
  PANGO_STRETCH_ULTRA_EXPANDED
} PangoStretch;

/**
 * PangoFontMask:
 * @PANGO_FONT_MASK_FAMILY: the font family is specified.
 * @PANGO_FONT_MASK_STYLE: the font style is specified.
 * @PANGO_FONT_MASK_VARIANT: the font variant is specified.
 * @PANGO_FONT_MASK_WEIGHT: the font weight is specified.
 * @PANGO_FONT_MASK_STRETCH: the font stretch is specified.
 * @PANGO_FONT_MASK_SIZE: the font size is specified.
 * @PANGO_FONT_MASK_GRAVITY: the font gravity is specified (Since: 1.16.)
 * @PANGO_FONT_MASK_VARIATIONS: OpenType font variations are specified (Since: 1.42)
 *
 * The bits in a `PangoFontMask` correspond to the set fields in a
 * `PangoFontDescription`.
 */
typedef enum {
  PANGO_FONT_MASK_FAMILY  = 1 << 0,
  PANGO_FONT_MASK_STYLE   = 1 << 1,
  PANGO_FONT_MASK_VARIANT = 1 << 2,
  PANGO_FONT_MASK_WEIGHT  = 1 << 3,
  PANGO_FONT_MASK_STRETCH = 1 << 4,
  PANGO_FONT_MASK_SIZE    = 1 << 5,
  PANGO_FONT_MASK_GRAVITY = 1 << 6,
  PANGO_FONT_MASK_VARIATIONS = 1 << 7,
} PangoFontMask;

/* CSS scale factors (1.2 factor between each size) */
/**
 * PANGO_SCALE_XX_SMALL:
 *
 * The scale factor for three shrinking steps (1 / (1.2 * 1.2 * 1.2)).
 */
/**
 * PANGO_SCALE_X_SMALL:
 *
 * The scale factor for two shrinking steps (1 / (1.2 * 1.2)).
 */
/**
 * PANGO_SCALE_SMALL:
 *
 * The scale factor for one shrinking step (1 / 1.2).
 */
/**
 * PANGO_SCALE_MEDIUM:
 *
 * The scale factor for normal size (1.0).
 */
/**
 * PANGO_SCALE_LARGE:
 *
 * The scale factor for one magnification step (1.2).
 */
/**
 * PANGO_SCALE_X_LARGE:
 *
 * The scale factor for two magnification steps (1.2 * 1.2).
 */
/**
 * PANGO_SCALE_XX_LARGE:
 *
 * The scale factor for three magnification steps (1.2 * 1.2 * 1.2).
 */
#define PANGO_SCALE_XX_SMALL ((double)0.5787037037037)
#define PANGO_SCALE_X_SMALL  ((double)0.6944444444444)
#define PANGO_SCALE_SMALL    ((double)0.8333333333333)
#define PANGO_SCALE_MEDIUM   ((double)1.0)
#define PANGO_SCALE_LARGE    ((double)1.2)
#define PANGO_SCALE_X_LARGE  ((double)1.44)
#define PANGO_SCALE_XX_LARGE ((double)1.728)


#define PANGO_TYPE_FONT_DESCRIPTION (pango_font_description_get_type ())

PANGO_AVAILABLE_IN_ALL
GType                   pango_font_description_get_type          (void) G_GNUC_CONST;
PANGO_AVAILABLE_IN_ALL
PangoFontDescription *  pango_font_description_new               (void);
PANGO_AVAILABLE_IN_ALL
PangoFontDescription *  pango_font_description_copy              (const PangoFontDescription  *desc);
PANGO_AVAILABLE_IN_ALL
PangoFontDescription *  pango_font_description_copy_static       (const PangoFontDescription  *desc);
PANGO_AVAILABLE_IN_ALL
guint                   pango_font_description_hash              (const PangoFontDescription  *desc) G_GNUC_PURE;
PANGO_AVAILABLE_IN_ALL
gboolean                pango_font_description_equal             (const PangoFontDescription  *desc1,
                                                                  const PangoFontDescription  *desc2) G_GNUC_PURE;
PANGO_AVAILABLE_IN_ALL
void                    pango_font_description_free              (PangoFontDescription        *desc);

PANGO_AVAILABLE_IN_ALL
void                    pango_font_description_set_family        (PangoFontDescription *desc,
                                                                  const char           *family);
PANGO_AVAILABLE_IN_ALL
void                    pango_font_description_set_family_static (PangoFontDescription *desc,
                                                                  const char           *family);
PANGO_AVAILABLE_IN_ALL
const char *            pango_font_description_get_family        (const PangoFontDescription *desc) G_GNUC_PURE;
PANGO_AVAILABLE_IN_ALL
void                    pango_font_description_set_style         (PangoFontDescription *desc,
                                                                  PangoStyle            style);
PANGO_AVAILABLE_IN_ALL
PangoStyle              pango_font_description_get_style         (const PangoFontDescription *desc) G_GNUC_PURE;
PANGO_AVAILABLE_IN_ALL
void                    pango_font_description_set_variant       (PangoFontDescription *desc,
                                                                  PangoVariant          variant);
PANGO_AVAILABLE_IN_ALL
PangoVariant            pango_font_description_get_variant       (const PangoFontDescription *desc) G_GNUC_PURE;
PANGO_AVAILABLE_IN_ALL
void                    pango_font_description_set_weight        (PangoFontDescription *desc,
                                                                  PangoWeight           weight);
PANGO_AVAILABLE_IN_ALL
PangoWeight             pango_font_description_get_weight        (const PangoFontDescription *desc) G_GNUC_PURE;
PANGO_AVAILABLE_IN_ALL
void                    pango_font_description_set_stretch       (PangoFontDescription *desc,
                                                                  PangoStretch          stretch);
PANGO_AVAILABLE_IN_ALL
PangoStretch            pango_font_description_get_stretch       (const PangoFontDescription *desc) G_GNUC_PURE;
PANGO_AVAILABLE_IN_ALL
void                    pango_font_description_set_size          (PangoFontDescription *desc,
                                                                  int                   size);
PANGO_AVAILABLE_IN_ALL
int                     pango_font_description_get_size          (const PangoFontDescription *desc) G_GNUC_PURE;
PANGO_AVAILABLE_IN_1_8
void                    pango_font_description_set_absolute_size (PangoFontDescription *desc,
                                                                  double                size);
PANGO_AVAILABLE_IN_1_8
gboolean                pango_font_description_get_size_is_absolute (const PangoFontDescription *desc) G_GNUC_PURE;
PANGO_AVAILABLE_IN_1_16
void                    pango_font_description_set_gravity       (PangoFontDescription *desc,
                                                                  PangoGravity          gravity);
PANGO_AVAILABLE_IN_1_16
PangoGravity            pango_font_description_get_gravity       (const PangoFontDescription *desc) G_GNUC_PURE;

PANGO_AVAILABLE_IN_1_42
void                    pango_font_description_set_variations_static (PangoFontDescription       *desc,
                                                                      const char                 *variations);
PANGO_AVAILABLE_IN_1_42
void                    pango_font_description_set_variations    (PangoFontDescription       *desc,
                                                                  const char                 *variations);
PANGO_AVAILABLE_IN_1_42
const char *            pango_font_description_get_variations    (const PangoFontDescription *desc) G_GNUC_PURE;

PANGO_AVAILABLE_IN_ALL
PangoFontMask           pango_font_description_get_set_fields    (const PangoFontDescription *desc) G_GNUC_PURE;
PANGO_AVAILABLE_IN_ALL
void                    pango_font_description_unset_fields      (PangoFontDescription       *desc,
                                                                  PangoFontMask               to_unset);

PANGO_AVAILABLE_IN_ALL
void                    pango_font_description_merge             (PangoFontDescription       *desc,
                                                                  const PangoFontDescription *desc_to_merge,
                                                                  gboolean                    replace_existing);
PANGO_AVAILABLE_IN_ALL
void                    pango_font_description_merge_static      (PangoFontDescription       *desc,
                                                                  const PangoFontDescription *desc_to_merge,
                                                                  gboolean                    replace_existing);

PANGO_AVAILABLE_IN_ALL
gboolean                pango_font_description_better_match      (const PangoFontDescription *desc,
                                                                  const PangoFontDescription *old_match,
                                                                  const PangoFontDescription *new_match) G_GNUC_PURE;

PANGO_AVAILABLE_IN_ALL
PangoFontDescription *  pango_font_description_from_string       (const char                  *str);
PANGO_AVAILABLE_IN_ALL
char *                  pango_font_description_to_string         (const PangoFontDescription  *desc);
PANGO_AVAILABLE_IN_ALL
char *                  pango_font_description_to_filename       (const PangoFontDescription  *desc);


G_DEFINE_AUTOPTR_CLEANUP_FUNC(PangoFontDescription, pango_font_description_free)

G_END_DECLS
