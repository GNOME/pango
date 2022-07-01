/*
 * Copyright (C) 1999 Red Hat Software
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

#include <glib.h>
#include <glib-object.h>

#include <pango2/pango-version-macros.h>

G_BEGIN_DECLS

typedef struct _Pango2LogAttr Pango2LogAttr;

typedef struct _Pango2FontDescription Pango2FontDescription;

typedef struct _Pango2Font Pango2Font;
typedef struct _Pango2FontFace Pango2FontFace;
typedef struct _Pango2FontFamily Pango2FontFamily;

typedef struct _Pango2FontMap Pango2FontMap;

typedef struct _Pango2Rectangle Pango2Rectangle;

typedef struct _Pango2Context Pango2Context;

typedef struct _Pango2Language Pango2Language;

/* A index of a glyph into a font. Rendering system dependent */
/**
 * Pango2Glyph:
 *
 * A `Pango2Glyph` represents a single glyph in the output form of a string.
 */
typedef guint32 Pango2Glyph;

typedef struct _Pango2Lines Pango2Lines;
typedef struct _Pango2Run Pango2Run;
typedef struct _Pango2Line Pango2Line;
typedef struct _Pango2LineIter Pango2LineIter;

/**
 * PANGO2_SCALE:
 *
 * The scale between dimensions used for Pango distances and device units.
 *
 * The definition of device units is dependent on the output device; it will
 * typically be pixels for a screen, and points for a printer. %PANGO2_SCALE is
 * currently 1024, but this may be changed in the future.
 *
 * When setting font sizes, device units are always considered to be
 * points (as in "12 point font"), rather than pixels.
 */
/**
 * PANGO2_PIXELS:
 * @d: a dimension in Pango units.
 *
 * Converts a dimension to device units by rounding.
 *
 * Return value: rounded dimension in device units.
 */
/**
 * PANGO2_PIXELS_FLOOR:
 * @d: a dimension in Pango units.
 *
 * Converts a dimension to device units by flooring.
 *
 * Return value: floored dimension in device units.
 */
/**
 * PANGO2_PIXELS_CEIL:
 * @d: a dimension in Pango units.
 *
 * Converts a dimension to device units by ceiling.
 *
 * Return value: ceiled dimension in device units.
 */
#define PANGO2_SCALE 1024
#define PANGO2_PIXELS(d) (((int)(d) + 512) >> 10)
#define PANGO2_PIXELS_FLOOR(d) (((int)(d)) >> 10)
#define PANGO2_PIXELS_CEIL(d) (((int)(d) + 1023) >> 10)
/* The above expressions are just slightly wrong for floating point d;
 * For example we'd expect PANGO2_PIXELS(-512.5) => -1 but instead we get 0.
 * That's unlikely to matter for practical use and the expression is much
 * more compact and faster than alternatives that work exactly for both
 * integers and floating point.
 *
 * PANGO2_PIXELS also behaves differently for +512 and -512.
 */

/**
 * PANGO2_UNITS_FLOOR:
 * @d: a dimension in Pango units.
 *
 * Rounds a dimension down to whole device units, but does not
 * convert it to device units.
 *
 * Return value: rounded down dimension in Pango units.
 */
#define PANGO2_UNITS_FLOOR(d)                \
  ((d) & ~(PANGO2_SCALE - 1))

/**
 * PANGO2_UNITS_CEIL:
 * @d: a dimension in Pango units.
 *
 * Rounds a dimension up to whole device units, but does not
 * convert it to device units.
 *
 * Return value: rounded up dimension in Pango units.
 */
#define PANGO2_UNITS_CEIL(d)                 \
  (((d) + (PANGO2_SCALE - 1)) & ~(PANGO2_SCALE - 1))

/**
 * PANGO2_UNITS_ROUND:
 * @d: a dimension in Pango units.
 *
 * Rounds a dimension to whole device units, but does not
 * convert it to device units.
 *
 * Return value: rounded dimension in Pango units.
 */
#define PANGO2_UNITS_ROUND(d)                           \
  (((d) + (PANGO2_SCALE >> 1)) & ~(PANGO2_SCALE - 1))


PANGO2_AVAILABLE_IN_ALL
int    pango2_units_from_double (double d) G_GNUC_CONST;
PANGO2_AVAILABLE_IN_ALL
double pango2_units_to_double (int i) G_GNUC_CONST;



/**
 * Pango2Rectangle:
 * @x: X coordinate of the left side of the rectangle.
 * @y: Y coordinate of the the top side of the rectangle.
 * @width: width of the rectangle.
 * @height: height of the rectangle.
 *
 * The `Pango2Rectangle` structure represents a rectangle.
 *
 * `Pango2Rectangle` is frequently used to represent the logical or ink
 * extents of a single glyph or section of text. (See, for instance,
 * [method@Pango2.Font.get_glyph_extents].)
 */
struct _Pango2Rectangle
{
  int x;
  int y;
  int width;
  int height;
};

/* Macros to translate from extents rectangles to ascent/descent/lbearing/rbearing
 */
/**
 * PANGO2_ASCENT:
 * @rect: a `Pango2Rectangle`
 *
 * Extracts the *ascent* from a `Pango2Rectangle`
 * representing glyph extents.
 *
 * The ascent is the distance from the baseline to the
 * highest point of the character. This is positive if the
 * glyph ascends above the baseline.
 */
/**
 * PANGO2_DESCENT:
 * @rect: a `Pango2Rectangle`
 *
 * Extracts the *descent* from a `Pango2Rectangle`
 * representing glyph extents.
 *
 * The descent is the distance from the baseline to the
 * lowest point of the character. This is positive if the
 * glyph descends below the baseline.
 */
/**
 * PANGO2_LBEARING:
 * @rect: a `Pango2Rectangle`
 *
 * Extracts the *left bearing* from a `Pango2Rectangle`
 * representing glyph extents.
 *
 * The left bearing is the distance from the horizontal
 * origin to the farthest left point of the character.
 * This is positive for characters drawn completely to
 * the right of the glyph origin.
 */
/**
 * PANGO2_RBEARING:
 * @rect: a `Pango2Rectangle`
 *
 * Extracts the *right bearing* from a `Pango2Rectangle`
 * representing glyph extents.
 *
 * The right bearing is the distance from the horizontal
 * origin to the farthest right point of the character.
 * This is positive except for characters drawn completely
 * to the left of the horizontal origin.
 */
#define PANGO2_ASCENT(rect) (-(rect).y)
#define PANGO2_DESCENT(rect) ((rect).y + (rect).height)
#define PANGO2_LBEARING(rect) ((rect).x)
#define PANGO2_RBEARING(rect) ((rect).x + (rect).width)

PANGO2_AVAILABLE_IN_ALL
void pango2_extents_to_pixels (Pango2Rectangle *inclusive,
                               Pango2Rectangle *nearest);


#include <pango2/pango-direction.h>
#include <pango2/pango-gravity.h>
#include <pango2/pango-language.h>
#include <pango2/pango-matrix.h>
#include <pango2/pango-script.h>

/**
 * Pango2Alignment:
 * @PANGO2_ALIGN_LEFT: Put all available space on the right
 * @PANGO2_ALIGN_CENTER: Center the line within the available space
 * @PANGO2_ALIGN_RIGHT: Put all available space on the left
 * @PANGO2_ALIGN_NATURAL: Use left or right alignment, depending
 *   on the text direction of the paragraph
 * @PANGO2_ALIGN_JUSTIFY: Justify the content to fill the available space
 *
 * `Pango2Alignment` describes how to align the lines of a `Pango2Layout`
 * within the available space.
 */
typedef enum
{
  PANGO2_ALIGN_LEFT,
  PANGO2_ALIGN_CENTER,
  PANGO2_ALIGN_RIGHT,
  PANGO2_ALIGN_NATURAL,
  PANGO2_ALIGN_JUSTIFY
} Pango2Alignment;

/**
 * Pango2WrapMode:
 * @PANGO2_WRAP_WORD: wrap lines at word boundaries.
 * @PANGO2_WRAP_CHAR: wrap lines at character boundaries.
 * @PANGO2_WRAP_WORD_CHAR: wrap lines at word boundaries, but fall back to
 *   character boundaries if there is not enough space for a full word.
 *
 * `Pango2WrapMode` describes how to wrap the lines of a `Pango2Layout`
 * to the desired width.
 *
 * For @PANGO2_WRAP_WORD, Pango uses break opportunities that are determined
 * by the Unicode line breaking algorithm. For @PANGO2_WRAP_CHAR, Pango allows
 * breaking at grapheme boundaries that are determined by the Unicode text
 * segmentation algorithm.
 */
typedef enum {
  PANGO2_WRAP_WORD,
  PANGO2_WRAP_CHAR,
  PANGO2_WRAP_WORD_CHAR
} Pango2WrapMode;

/**
 * Pango2EllipsizeMode:
 * @PANGO2_ELLIPSIZE_NONE: No ellipsization
 * @PANGO2_ELLIPSIZE_START: Omit characters at the start of the text
 * @PANGO2_ELLIPSIZE_MIDDLE: Omit characters in the middle of the text
 * @PANGO2_ELLIPSIZE_END: Omit characters at the end of the text
 *
 * `Pango2EllipsizeMode` describes what sort of ellipsization
 * should be applied to text.
 *
 * In the ellipsization process characters are removed from the
 * text in order to make it fit to a given width and replaced
 * with an ellipsis.
 */
typedef enum {
  PANGO2_ELLIPSIZE_NONE,
  PANGO2_ELLIPSIZE_START,
  PANGO2_ELLIPSIZE_MIDDLE,
  PANGO2_ELLIPSIZE_END
} Pango2EllipsizeMode;

/**
* Pango2LeadingTrim:
 * @PANGO2_LEADING_TRIM_NONE: No trimming
 * @PANGO2_LEADING_TRIM_START: Trim leading at the top
 * @PANGO2_LEADING_TRIM_END: Trim leading at the bottom
 *
 * The `Pango2LeadingTrim` flags control how the line height affects
 * the extents of runs and lines.
 */
typedef enum
{
  PANGO2_LEADING_TRIM_NONE  = 0,
  PANGO2_LEADING_TRIM_START = 1 << 0,
  PANGO2_LEADING_TRIM_END   = 1 << 1,
} Pango2LeadingTrim;

/**
 * PANGO2_LEADING_TRIM_BOTH:
 *
 * Shorthand for `PANGO2_LEADING_TRIM_START|PANGO2_LEADING_TRIM_END`.
 */
#define PANGO2_LEADING_TRIM_BOTH (PANGO2_LEADING_TRIM_START|PANGO2_LEADING_TRIM_END)

/**
 * PANGO2_COLOR_PALETTE_DEFAULT:
 *
 * The name for the default color palette.
 */
#define PANGO2_COLOR_PALETTE_DEFAULT "default"

/**
 * PANGO2_COLOR_PALETTE_LIGHT:
 *
 * The name for a color palette suitable for use on
 * a light background.
 */
#define PANGO2_COLOR_PALETTE_LIGHT  "light"

/**
 * PANGO2_COLOR_PALETTE_DARK:
 *
 * The name for a color palette suitable for use on
 * a dark background.
 */
#define PANGO2_COLOR_PALETTE_DARK   "dark"

/*
 * PANGO2_DECLARE_INTERNAL_TYPE:
 * @ModuleObjName: The name of the new type, in camel case (like GtkWidget)
 * @module_obj_name: The name of the new type in lowercase, with words
 *  separated by '_' (like 'gtk_widget')
 * @MODULE: The name of the module, in all caps (like 'GTK')
 * @OBJ_NAME: The bare name of the type, in all caps (like 'WIDGET')
 * @ParentName: the name of the parent type, in camel case (like GtkWidget)
 *
 * A convenience macro for emitting the usual declarations in the
 * header file for a type which is intended to be subclassed only
 * by internal consumers.
 *
 * This macro differs from %G_DECLARE_DERIVABLE_TYPE and %G_DECLARE_FINAL_TYPE
 * by declaring a type that is only derivable internally. Internal users can
 * derive this type, assuming they have access to the instance and class
 * structures; external users will not be able to subclass this type.
 */
#define PANGO2_DECLARE_INTERNAL_TYPE(ModuleObjName, module_obj_name, MODULE, OBJ_NAME, ParentName) \
  GType module_obj_name##_get_type (void);                                                               \
  G_GNUC_BEGIN_IGNORE_DEPRECATIONS                                                                       \
  typedef struct _##ModuleObjName ModuleObjName;                                                         \
  typedef struct _##ModuleObjName##Class ModuleObjName##Class;                                           \
                                                                                                         \
  _GLIB_DEFINE_AUTOPTR_CHAINUP (ModuleObjName, ParentName)                                               \
  G_DEFINE_AUTOPTR_CLEANUP_FUNC (ModuleObjName##Class, g_type_class_unref)                               \
                                                                                                         \
  G_GNUC_UNUSED static inline ModuleObjName * MODULE##_##OBJ_NAME (gpointer ptr) {                       \
    return G_TYPE_CHECK_INSTANCE_CAST (ptr, module_obj_name##_get_type (), ModuleObjName); }             \
  G_GNUC_UNUSED static inline ModuleObjName##Class * MODULE##_##OBJ_NAME##_CLASS (gpointer ptr) {        \
    return G_TYPE_CHECK_CLASS_CAST (ptr, module_obj_name##_get_type (), ModuleObjName##Class); }         \
  G_GNUC_UNUSED static inline gboolean MODULE##_IS_##OBJ_NAME (gpointer ptr) {                           \
    return G_TYPE_CHECK_INSTANCE_TYPE (ptr, module_obj_name##_get_type ()); }                            \
  G_GNUC_UNUSED static inline gboolean MODULE##_IS_##OBJ_NAME##_CLASS (gpointer ptr) {                   \
    return G_TYPE_CHECK_CLASS_TYPE (ptr, module_obj_name##_get_type ()); }                               \
  G_GNUC_UNUSED static inline ModuleObjName##Class * MODULE##_##OBJ_NAME##_GET_CLASS (gpointer ptr) {    \
    return G_TYPE_INSTANCE_GET_CLASS (ptr, module_obj_name##_get_type (), ModuleObjName##Class); }       \
  G_GNUC_END_IGNORE_DEPRECATIONS

G_END_DECLS
