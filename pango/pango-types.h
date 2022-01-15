/* Pango
 * pango-types.h:
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
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.	 See the GNU
 * Library General Public License for more details.
 *
 * You should have received a copy of the GNU Library General Public
 * License along with this library; if not, write to the
 * Free Software Foundation, Inc., 59 Temple Place - Suite 330,
 * Boston, MA 02111-1307, USA.
 */

#ifndef __PANGO_TYPES_H__
#define __PANGO_TYPES_H__

#include <glib.h>
#include <glib-object.h>

#include <pango/pango-version-macros.h>

G_BEGIN_DECLS

typedef struct _PangoLogAttr PangoLogAttr;

typedef struct _PangoFontDescription PangoFontDescription;

typedef struct _PangoFont PangoFont;
typedef struct _PangoFontFace PangoFontFace;
typedef struct _PangoFontFamily PangoFontFamily;

typedef struct _PangoFontMap PangoFontMap;

typedef struct _PangoRectangle PangoRectangle;

typedef struct _PangoContext PangoContext;

typedef struct _PangoLanguage PangoLanguage;

/* A index of a glyph into a font. Rendering system dependent */
/**
 * PangoGlyph:
 *
 * A `PangoGlyph` represents a single glyph in the output form of a string.
 */
typedef guint32 PangoGlyph;



/**
 * PANGO_SCALE:
 *
 * The scale between dimensions used for Pango distances and device units.
 *
 * The definition of device units is dependent on the output device; it will
 * typically be pixels for a screen, and points for a printer. %PANGO_SCALE is
 * currently 1024, but this may be changed in the future.
 *
 * When setting font sizes, device units are always considered to be
 * points (as in "12 point font"), rather than pixels.
 */
/**
 * PANGO_PIXELS:
 * @d: a dimension in Pango units.
 *
 * Converts a dimension to device units by rounding.
 *
 * Return value: rounded dimension in device units.
 */
/**
 * PANGO_PIXELS_FLOOR:
 * @d: a dimension in Pango units.
 *
 * Converts a dimension to device units by flooring.
 *
 * Return value: floored dimension in device units.
 * Since: 1.14
 */
/**
 * PANGO_PIXELS_CEIL:
 * @d: a dimension in Pango units.
 *
 * Converts a dimension to device units by ceiling.
 *
 * Return value: ceiled dimension in device units.
 * Since: 1.14
 */
#define PANGO_SCALE 1024
#define PANGO_PIXELS(d) (((int)(d) + 512) >> 10)
#define PANGO_PIXELS_FLOOR(d) (((int)(d)) >> 10)
#define PANGO_PIXELS_CEIL(d) (((int)(d) + 1023) >> 10)
/* The above expressions are just slightly wrong for floating point d;
 * For example we'd expect PANGO_PIXELS(-512.5) => -1 but instead we get 0.
 * That's unlikely to matter for practical use and the expression is much
 * more compact and faster than alternatives that work exactly for both
 * integers and floating point.
 *
 * PANGO_PIXELS also behaves differently for +512 and -512.
 */

/**
 * PANGO_UNITS_FLOOR:
 * @d: a dimension in Pango units.
 *
 * Rounds a dimension down to whole device units, but does not
 * convert it to device units.
 *
 * Return value: rounded down dimension in Pango units.
 * Since: 1.50
 */
#define PANGO_UNITS_FLOOR(d)                \
  ((d) & ~(PANGO_SCALE - 1))

/**
 * PANGO_UNITS_CEIL:
 * @d: a dimension in Pango units.
 *
 * Rounds a dimension up to whole device units, but does not
 * convert it to device units.
 *
 * Return value: rounded up dimension in Pango units.
 * Since: 1.50
 */
#define PANGO_UNITS_CEIL(d)                 \
  (((d) + (PANGO_SCALE - 1)) & ~(PANGO_SCALE - 1))

/**
 * PANGO_UNITS_ROUND:
 * @d: a dimension in Pango units.
 *
 * Rounds a dimension to whole device units, but does not
 * convert it to device units.
 *
 * Return value: rounded dimension in Pango units.
 * Since: 1.18
 */
#define PANGO_UNITS_ROUND(d)				\
  (((d) + (PANGO_SCALE >> 1)) & ~(PANGO_SCALE - 1))


PANGO_AVAILABLE_IN_1_16
int    pango_units_from_double (double d) G_GNUC_CONST;
PANGO_AVAILABLE_IN_1_16
double pango_units_to_double (int i) G_GNUC_CONST;



/**
 * PangoRectangle:
 * @x: X coordinate of the left side of the rectangle.
 * @y: Y coordinate of the the top side of the rectangle.
 * @width: width of the rectangle.
 * @height: height of the rectangle.
 *
 * The `PangoRectangle` structure represents a rectangle.
 *
 * `PangoRectangle` is frequently used to represent the logical or ink
 * extents of a single glyph or section of text. (See, for instance,
 * [method@Pango.Font.get_glyph_extents].)
 */
struct _PangoRectangle
{
  int x;
  int y;
  int width;
  int height;
};

/* Macros to translate from extents rectangles to ascent/descent/lbearing/rbearing
 */
/**
 * PANGO_ASCENT:
 * @rect: a `PangoRectangle`
 *
 * Extracts the *ascent* from a `PangoRectangle`
 * representing glyph extents.
 *
 * The ascent is the distance from the baseline to the
 * highest point of the character. This is positive if the
 * glyph ascends above the baseline.
 */
/**
 * PANGO_DESCENT:
 * @rect: a `PangoRectangle`
 *
 * Extracts the *descent* from a `PangoRectangle`
 * representing glyph extents.
 *
 * The descent is the distance from the baseline to the
 * lowest point of the character. This is positive if the
 * glyph descends below the baseline.
 */
/**
 * PANGO_LBEARING:
 * @rect: a `PangoRectangle`
 *
 * Extracts the *left bearing* from a `PangoRectangle`
 * representing glyph extents.
 *
 * The left bearing is the distance from the horizontal
 * origin to the farthest left point of the character.
 * This is positive for characters drawn completely to
 * the right of the glyph origin.
 */
/**
 * PANGO_RBEARING:
 * @rect: a `PangoRectangle`
 *
 * Extracts the *right bearing* from a `PangoRectangle`
 * representing glyph extents.
 *
 * The right bearing is the distance from the horizontal
 * origin to the farthest right point of the character.
 * This is positive except for characters drawn completely
 * to the left of the horizontal origin.
 */
#define PANGO_ASCENT(rect) (-(rect).y)
#define PANGO_DESCENT(rect) ((rect).y + (rect).height)
#define PANGO_LBEARING(rect) ((rect).x)
#define PANGO_RBEARING(rect) ((rect).x + (rect).width)

PANGO_AVAILABLE_IN_1_16
void pango_extents_to_pixels (PangoRectangle *inclusive,
                             PangoRectangle *nearest);


#include <pango/pango-direction.h>
#include <pango/pango-gravity.h>
#include <pango/pango-language.h>
#include <pango/pango-matrix.h>
#include <pango/pango-script.h>

/**
 * PangoAlignment:
 * @PANGO_ALIGN_LEFT: Put all available space on the right
 * @PANGO_ALIGN_CENTER: Center the line within the available space
 * @PANGO_ALIGN_RIGHT: Put all available space on the left
 *
 * `PangoAlignment` describes how to align the lines of a `PangoLayout`
 * within the available space.
 *
 * If the `PangoLayout` is set to justify using [method@Pango.Layout.set_justify],
 * this only affects partial lines.
 *
 * See [method@Pango.Layout.set_auto_dir] for how text direction affects
 * the interpretation of `PangoAlignment` values.
 */
typedef enum {
  PANGO_ALIGN_LEFT,
  PANGO_ALIGN_CENTER,
  PANGO_ALIGN_RIGHT
} PangoAlignment;

typedef enum
{
  PANGO_ALIGNMENT_LEFT,
  PANGO_ALIGNMENT_CENTER,
  PANGO_ALIGNMENT_RIGHT,
  PANGO_ALIGNMENT_JUSTIFY,
  PANGO_ALIGNMENT_JUSTIFY_ALL,
} PangoAlignmentMode;

/**
 * PangoWrapMode:
 * @PANGO_WRAP_WORD: wrap lines at word boundaries.
 * @PANGO_WRAP_CHAR: wrap lines at character boundaries.
 * @PANGO_WRAP_WORD_CHAR: wrap lines at word boundaries, but fall back to
 *   character boundaries if there is not enough space for a full word.
 *
 * `PangoWrapMode` describes how to wrap the lines of a `PangoLayout`
 * to the desired width.
 *
 * For @PANGO_WRAP_WORD, Pango uses break opportunities that are determined
 * by the Unicode line breaking algorithm. For @PANGO_WRAP_CHAR, Pango allows
 * breaking at grapheme boundaries that are determined by the Unicode text
 * segmentation algorithm.
 */
typedef enum {
  PANGO_WRAP_WORD,
  PANGO_WRAP_CHAR,
  PANGO_WRAP_WORD_CHAR
} PangoWrapMode;

/**
 * PangoEllipsizeMode:
 * @PANGO_ELLIPSIZE_NONE: No ellipsization
 * @PANGO_ELLIPSIZE_START: Omit characters at the start of the text
 * @PANGO_ELLIPSIZE_MIDDLE: Omit characters in the middle of the text
 * @PANGO_ELLIPSIZE_END: Omit characters at the end of the text
 *
 * `PangoEllipsizeMode` describes what sort of ellipsization
 * should be applied to text.
 *
 * In the ellipsization process characters are removed from the
 * text in order to make it fit to a given width and replaced
 * with an ellipsis.
 */
typedef enum {
  PANGO_ELLIPSIZE_NONE,
  PANGO_ELLIPSIZE_START,
  PANGO_ELLIPSIZE_MIDDLE,
  PANGO_ELLIPSIZE_END
} PangoEllipsizeMode;


/*
 * PANGO_DECLARE_INTERNAL_TYPE:
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
#define PANGO_DECLARE_INTERNAL_TYPE(ModuleObjName, module_obj_name, MODULE, OBJ_NAME, ParentName) \
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

#endif /* __PANGO_TYPES_H__ */
