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
 * Pango2FontDescription:
 *
 * A `Pango2FontDescription` describes a font in an implementation-independent
 * manner.
 *
 * `Pango2FontDescription` structures are used both to list what fonts are
 * available on the system and also for specifying the characteristics of
 * a font to load.
 */

/**
 * Pango2Style:
 * @PANGO2_STYLE_NORMAL: the font is upright.
 * @PANGO2_STYLE_OBLIQUE: the font is slanted, but in a roman style.
 * @PANGO2_STYLE_ITALIC: the font is slanted in an italic style.
 *
 * An enumeration specifying the various slant styles possible for a font.
 **/
typedef enum {
  PANGO2_STYLE_NORMAL,
  PANGO2_STYLE_OBLIQUE,
  PANGO2_STYLE_ITALIC
} Pango2Style;

/**
 * Pango2Variant:
 * @PANGO2_VARIANT_NORMAL: A normal font.
 * @PANGO2_VARIANT_SMALL_CAPS: A font with the lower case characters
 *   replaced by smaller variants of the capital characters.
 * @PANGO2_VARIANT_ALL_SMALL_CAPS: A font with all characters
 *   replaced by smaller variants of the capital characters.
 * @PANGO2_VARIANT_PETITE_CAPS: A font with the lower case characters
 *   replaced by smaller variants of the capital characters.
 *   Petite Caps can be even smaller than Small Caps.
 * @PANGO2_VARIANT_ALL_PETITE_CAPS: A font with all characters
 *   replaced by smaller variants of the capital characters.
 *   Petite Caps can be even smaller than Small Caps.
 * @PANGO2_VARIANT_UNICASE: A font with the upper case characters
 *   replaced by smaller variants of the capital letters.
 * @PANGO2_VARIANT_TITLE_CAPS: A font with capital letters that
 *   are more suitable for all-uppercase titles.
 *
 * An enumeration specifying capitalization variant of the font.
 */
typedef enum {
  PANGO2_VARIANT_NORMAL,
  PANGO2_VARIANT_SMALL_CAPS,
  PANGO2_VARIANT_ALL_SMALL_CAPS,
  PANGO2_VARIANT_PETITE_CAPS,
  PANGO2_VARIANT_ALL_PETITE_CAPS,
  PANGO2_VARIANT_UNICASE,
  PANGO2_VARIANT_TITLE_CAPS
} Pango2Variant;

/**
 * Pango2Weight:
 * @PANGO2_WEIGHT_THIN: the thin weight (= 100)
 * @PANGO2_WEIGHT_ULTRALIGHT: the ultralight weight (= 200)
 * @PANGO2_WEIGHT_LIGHT: the light weight (= 300)
 * @PANGO2_WEIGHT_SEMILIGHT: the semilight weight (= 350)
 * @PANGO2_WEIGHT_BOOK: the book weight (= 380)
 * @PANGO2_WEIGHT_NORMAL: the default weight (= 400)
 * @PANGO2_WEIGHT_MEDIUM: the normal weight (= 500)
 * @PANGO2_WEIGHT_SEMIBOLD: the semibold weight (= 600)
 * @PANGO2_WEIGHT_BOLD: the bold weight (= 700)
 * @PANGO2_WEIGHT_ULTRABOLD: the ultrabold weight (= 800)
 * @PANGO2_WEIGHT_HEAVY: the heavy weight (= 900)
 * @PANGO2_WEIGHT_ULTRAHEAVY: the ultraheavy weight (= 1000)
 *
 * A `Pango2Weight` specifes the weight (boldness) of a font.
 *
 * Weight is specified as a numeric value ranging from 100 to 1000.
 * This enumeration simply provides some common, predefined values.
 */
typedef enum {
  PANGO2_WEIGHT_THIN = 100,
  PANGO2_WEIGHT_ULTRALIGHT = 200,
  PANGO2_WEIGHT_LIGHT = 300,
  PANGO2_WEIGHT_SEMILIGHT = 350,
  PANGO2_WEIGHT_BOOK = 380,
  PANGO2_WEIGHT_NORMAL = 400,
  PANGO2_WEIGHT_MEDIUM = 500,
  PANGO2_WEIGHT_SEMIBOLD = 600,
  PANGO2_WEIGHT_BOLD = 700,
  PANGO2_WEIGHT_ULTRABOLD = 800,
  PANGO2_WEIGHT_HEAVY = 900,
  PANGO2_WEIGHT_ULTRAHEAVY = 1000
} Pango2Weight;

/**
 * Pango2Stretch:
 * @PANGO2_STRETCH_ULTRA_CONDENSED: ultra-condensed width (= 500)
 * @PANGO2_STRETCH_EXTRA_CONDENSED: extra-condensed width (= 625)
 * @PANGO2_STRETCH_CONDENSED: condensed width (= 750)
 * @PANGO2_STRETCH_SEMI_CONDENSED: semi-condensed width (= 875)
 * @PANGO2_STRETCH_NORMAL: the normal width (= 1000)
 * @PANGO2_STRETCH_SEMI_EXPANDED: semi-expanded width (= 1125)
 * @PANGO2_STRETCH_EXPANDED: expanded width (= 1250)
 * @PANGO2_STRETCH_EXTRA_EXPANDED: extra-expanded width (= 1500)
 * @PANGO2_STRETCH_ULTRA_EXPANDED: ultra-expanded width (= 2000)
 *
 * A `Pango2Stretch` specifes the width of the font relative
 * to other designs within a family.
 *
 * Stretch is specified as a numeric value ranging from 500 to 2000.
 * This enumeration simply provides some common, predefined values.
 */
typedef enum {
  PANGO2_STRETCH_ULTRA_CONDENSED =  500,
  PANGO2_STRETCH_EXTRA_CONDENSED =  625,
  PANGO2_STRETCH_CONDENSED       =  750,
  PANGO2_STRETCH_SEMI_CONDENSED  =  875,
  PANGO2_STRETCH_NORMAL          = 1000,
  PANGO2_STRETCH_SEMI_EXPANDED   = 1125,
  PANGO2_STRETCH_EXPANDED        = 1250,
  PANGO2_STRETCH_EXTRA_EXPANDED  = 1500,
  PANGO2_STRETCH_ULTRA_EXPANDED  = 2000
} Pango2Stretch;

/**
 * Pango2FontMask:
 * @PANGO2_FONT_MASK_FAMILY: the font family is specified.
 * @PANGO2_FONT_MASK_STYLE: the font style is specified.
 * @PANGO2_FONT_MASK_VARIANT: the font variant is specified.
 * @PANGO2_FONT_MASK_WEIGHT: the font weight is specified.
 * @PANGO2_FONT_MASK_STRETCH: the font stretch is specified.
 * @PANGO2_FONT_MASK_SIZE: the font size is specified.
 * @PANGO2_FONT_MASK_GRAVITY: the font gravity is specified
 * @PANGO2_FONT_MASK_VARIATIONS: OpenType font variations are specified
 * @PANGO2_FONT_MASK_FACEID: the face ID is specified
 *
 * The bits in a `Pango2FontMask` correspond to the set fields in a
 * `Pango2FontDescription`.
 */
typedef enum {
  PANGO2_FONT_MASK_FAMILY     = 1 << 0,
  PANGO2_FONT_MASK_STYLE      = 1 << 1,
  PANGO2_FONT_MASK_VARIANT    = 1 << 2,
  PANGO2_FONT_MASK_WEIGHT     = 1 << 3,
  PANGO2_FONT_MASK_STRETCH    = 1 << 4,
  PANGO2_FONT_MASK_SIZE       = 1 << 5,
  PANGO2_FONT_MASK_GRAVITY    = 1 << 6,
  PANGO2_FONT_MASK_VARIATIONS = 1 << 7,
  PANGO2_FONT_MASK_FACEID     = 1 << 8,
} Pango2FontMask;

/* CSS scale factors (1.2 factor between each size) */
/**
 * PANGO2_SCALE_XX_SMALL:
 *
 * The scale factor for three shrinking steps (1 / (1.2 * 1.2 * 1.2)).
 */
/**
 * PANGO2_SCALE_X_SMALL:
 *
 * The scale factor for two shrinking steps (1 / (1.2 * 1.2)).
 */
/**
 * PANGO2_SCALE_SMALL:
 *
 * The scale factor for one shrinking step (1 / 1.2).
 */
/**
 * PANGO2_SCALE_MEDIUM:
 *
 * The scale factor for normal size (1.0).
 */
/**
 * PANGO2_SCALE_LARGE:
 *
 * The scale factor for one magnification step (1.2).
 */
/**
 * PANGO2_SCALE_X_LARGE:
 *
 * The scale factor for two magnification steps (1.2 * 1.2).
 */
/**
 * PANGO2_SCALE_XX_LARGE:
 *
 * The scale factor for three magnification steps (1.2 * 1.2 * 1.2).
 */
#define PANGO2_SCALE_XX_SMALL ((double)0.5787037037037)
#define PANGO2_SCALE_X_SMALL  ((double)0.6944444444444)
#define PANGO2_SCALE_SMALL    ((double)0.8333333333333)
#define PANGO2_SCALE_MEDIUM   ((double)1.0)
#define PANGO2_SCALE_LARGE    ((double)1.2)
#define PANGO2_SCALE_X_LARGE  ((double)1.44)
#define PANGO2_SCALE_XX_LARGE ((double)1.728)


#define PANGO2_TYPE_FONT_DESCRIPTION (pango2_font_description_get_type ())

PANGO2_AVAILABLE_IN_ALL
GType                   pango2_font_description_get_type          (void) G_GNUC_CONST;
PANGO2_AVAILABLE_IN_ALL
Pango2FontDescription * pango2_font_description_new               (void);
PANGO2_AVAILABLE_IN_ALL
Pango2FontDescription * pango2_font_description_copy              (const Pango2FontDescription  *desc);
PANGO2_AVAILABLE_IN_ALL
Pango2FontDescription * pango2_font_description_copy_static       (const Pango2FontDescription  *desc);
PANGO2_AVAILABLE_IN_ALL
guint                   pango2_font_description_hash              (const Pango2FontDescription  *desc) G_GNUC_PURE;
PANGO2_AVAILABLE_IN_ALL
gboolean                pango2_font_description_equal             (const Pango2FontDescription  *desc1,
                                                                   const Pango2FontDescription  *desc2) G_GNUC_PURE;
PANGO2_AVAILABLE_IN_ALL
void                    pango2_font_description_free              (Pango2FontDescription        *desc);

PANGO2_AVAILABLE_IN_ALL
void                    pango2_font_description_set_family        (Pango2FontDescription *desc,
                                                                   const char           *family);
PANGO2_AVAILABLE_IN_ALL
void                    pango2_font_description_set_family_static (Pango2FontDescription *desc,
                                                                   const char           *family);
PANGO2_AVAILABLE_IN_ALL
const char *            pango2_font_description_get_family        (const Pango2FontDescription *desc) G_GNUC_PURE;
PANGO2_AVAILABLE_IN_ALL
void                    pango2_font_description_set_style         (Pango2FontDescription *desc,
                                                                   Pango2Style            style);
PANGO2_AVAILABLE_IN_ALL
Pango2Style             pango2_font_description_get_style         (const Pango2FontDescription *desc) G_GNUC_PURE;
PANGO2_AVAILABLE_IN_ALL
void                    pango2_font_description_set_variant       (Pango2FontDescription *desc,
                                                                   Pango2Variant          variant);
PANGO2_AVAILABLE_IN_ALL
Pango2Variant           pango2_font_description_get_variant       (const Pango2FontDescription *desc) G_GNUC_PURE;
PANGO2_AVAILABLE_IN_ALL
void                    pango2_font_description_set_weight        (Pango2FontDescription *desc,
                                                                   Pango2Weight           weight);
PANGO2_AVAILABLE_IN_ALL
Pango2Weight            pango2_font_description_get_weight        (const Pango2FontDescription *desc) G_GNUC_PURE;
PANGO2_AVAILABLE_IN_ALL
void                    pango2_font_description_set_stretch       (Pango2FontDescription *desc,
                                                                   Pango2Stretch          stretch);
PANGO2_AVAILABLE_IN_ALL
Pango2Stretch           pango2_font_description_get_stretch       (const Pango2FontDescription *desc) G_GNUC_PURE;
PANGO2_AVAILABLE_IN_ALL
void                    pango2_font_description_set_size          (Pango2FontDescription *desc,
                                                                   int                   size);
PANGO2_AVAILABLE_IN_ALL
int                     pango2_font_description_get_size          (const Pango2FontDescription *desc) G_GNUC_PURE;
PANGO2_AVAILABLE_IN_ALL
void                    pango2_font_description_set_absolute_size (Pango2FontDescription *desc,
                                                                   double                size);
PANGO2_AVAILABLE_IN_ALL
gboolean                pango2_font_description_get_size_is_absolute (const Pango2FontDescription *desc) G_GNUC_PURE;
PANGO2_AVAILABLE_IN_ALL
void                    pango2_font_description_set_gravity       (Pango2FontDescription *desc,
                                                                   Pango2Gravity          gravity);
PANGO2_AVAILABLE_IN_ALL
Pango2Gravity           pango2_font_description_get_gravity       (const Pango2FontDescription *desc) G_GNUC_PURE;

PANGO2_AVAILABLE_IN_ALL
void                    pango2_font_description_set_variations_static (Pango2FontDescription       *desc,
                                                                       const char                 *variations);
PANGO2_AVAILABLE_IN_ALL
void                    pango2_font_description_set_variations    (Pango2FontDescription       *desc,
                                                                   const char                 *variations);
PANGO2_AVAILABLE_IN_ALL
const char *            pango2_font_description_get_variations    (const Pango2FontDescription *desc) G_GNUC_PURE;

PANGO2_AVAILABLE_IN_ALL
void                    pango2_font_description_set_faceid        (Pango2FontDescription     *desc,
                                                                   const char               *faceid);
PANGO2_AVAILABLE_IN_ALL
void                    pango2_font_description_set_faceid_static (Pango2FontDescription *desc,
                                                                   const char           *faceid);
PANGO2_AVAILABLE_IN_ALL
const char *            pango2_font_description_get_faceid        (const Pango2FontDescription *desc) G_GNUC_PURE;

PANGO2_AVAILABLE_IN_ALL
Pango2FontMask          pango2_font_description_get_set_fields    (const Pango2FontDescription *desc) G_GNUC_PURE;
PANGO2_AVAILABLE_IN_ALL
void                    pango2_font_description_unset_fields      (Pango2FontDescription       *desc,
                                                                   Pango2FontMask               to_unset);

PANGO2_AVAILABLE_IN_ALL
void                    pango2_font_description_merge             (Pango2FontDescription       *desc,
                                                                   const Pango2FontDescription *desc_to_merge,
                                                                   gboolean                    replace_existing);
PANGO2_AVAILABLE_IN_ALL
void                    pango2_font_description_merge_static      (Pango2FontDescription       *desc,
                                                                   const Pango2FontDescription *desc_to_merge,
                                                                   gboolean                    replace_existing);

PANGO2_AVAILABLE_IN_ALL
Pango2FontDescription * pango2_font_description_from_string       (const char                  *str);
PANGO2_AVAILABLE_IN_ALL
char *                  pango2_font_description_to_string         (const Pango2FontDescription  *desc);


G_DEFINE_AUTOPTR_CLEANUP_FUNC(Pango2FontDescription, pango2_font_description_free)

G_END_DECLS
