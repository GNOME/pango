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

G_BEGIN_DECLS

typedef struct _PangoLogAttr PangoLogAttr;

typedef struct _PangoEngineLang PangoEngineLang;
typedef struct _PangoEngineShape PangoEngineShape;

typedef struct _PangoFont    PangoFont;
typedef struct _PangoFontMap PangoFontMap;

typedef struct _PangoMatrix    PangoMatrix;
typedef struct _PangoRectangle PangoRectangle;



/* A index of a glyph into a font. Rendering system dependent */
typedef guint32 PangoGlyph;



#define PANGO_SCALE 1024
#define PANGO_PIXELS(d) (((int)(d) + 512) >> 10)
#define PANGO_PIXELS_FLOOR(d) (((int)(d)) >> 10)
#define PANGO_PIXELS_CEIL(d) (((int)(d) + 1023) >> 10)
/* The above expressions are just slightly wrong for floating point d;
 * For example we'd expect PANGO_PIXELS(-512.5) => -1 but instead we get 0.
 * That's unlikely to matter for practical use and the expression is much
 * more compact and faster than alternatives that work exactly for both
 * integers and floating point.
 */

/* Hint line position and thickness.
 */
void pango_quantize_line_geometry (int *thickness,
				   int *position);



/* Dummy typedef - internally it's a 'const char *' */
typedef struct _PangoLanguage PangoLanguage;

#define PANGO_TYPE_LANGUAGE (pango_language_get_type ())

GType          pango_language_get_type    (void);
PangoLanguage *pango_language_from_string (const char *language);
#define pango_language_to_string(language) ((const char *)language)

G_CONST_RETURN char *pango_language_get_sample_string (PangoLanguage *language);

gboolean      pango_language_matches  (PangoLanguage *language,
				       const char *range_list);



/* A rectangle. Used to store logical and physical extents of glyphs,
 * runs, strings, etc.
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
#define PANGO_ASCENT(rect) (-(rect).y)
#define PANGO_DESCENT(rect) ((rect).y + (rect).height)
#define PANGO_LBEARING(rect) ((rect).x)
#define PANGO_RBEARING(rect) ((rect).x + (rect).width)



/**
 * PangoDirection:
 * @PANGO_DIRECTION_LTR: A strong left-to-right direction
 * @PANGO_DIRECTION_RTL: A strong right-to-left direction
 * @PANGO_DIRECTION_TTB_LTR: Deprecated value; treated the
 *   same as %PANGO_DIRECTION_RTL.
 * @PANGO_DIRECTION_TTB_RTL: Deprecated value; treated the
 *   same as %PANGO_DIRECTION_LTR
 * @PANGO_DIRECTION_WEAK_LTR: A weak left-to-right direction
 * @PANGO_DIRECTION_WEAK_RTL: A weak right-to-left direction
 * @PANGO_DIRECTION_NEUTRAL: No direction specified
 * 
 * The #PangoDirection type represents a direction in the
 * Unicode bidirectional algorithm; not every value in this
 * enumeration makes sense for every usage of #PangoDirection;
 * for example, the return value of pango_unichar_direction()
 * and pango_find_base_dir() cannot be %PANGO_DIRECTION_WEAK_LTR
 * or %PANGO_DIRECTION_WEAK_RTL, since every character is either
 * neutral or has a strong direction; on the other hand
 * %PANGO_DIRECTION_NEUTRAL doesn't make sense to pass
 * to pango_itemize_with_base_dir().
 *
 * The %PANGO_DIRECTION_TTB_LTR, %PANGO_DIRECTION_TTB_RTL
 * values come from an earlier interpretation of this
 * enumeration as the writing direction of a block of
 * text and are no longer used; See #PangoGravity for how
 * vertical text is handled in Pango.
 **/			  
typedef enum {
  PANGO_DIRECTION_LTR,
  PANGO_DIRECTION_RTL,
  PANGO_DIRECTION_TTB_LTR,
  PANGO_DIRECTION_TTB_RTL,
  PANGO_DIRECTION_WEAK_LTR,
  PANGO_DIRECTION_WEAK_RTL,
  PANGO_DIRECTION_NEUTRAL
} PangoDirection;

PangoDirection pango_unichar_direction      (gunichar     ch);
PangoDirection pango_find_base_dir          (const gchar *text,
					     gint         length);

#ifndef PANGO_DISABLE_DEPRECATED
gboolean       pango_get_mirror_char        (gunichar     ch,
					     gunichar    *mirrored_ch);
#endif



/**
 * PangoGravity:
 * @PANGO_GRAVITY_SOUTH: Glyphs stand upright (default)
 * @PANGO_GRAVITY_EAST: Glyphs are rotated 90 degrees clockwise
 * @PANGO_GRAVITY_NORTH: Glyphs are upside-down
 * @PANGO_GRAVITY_WEST: Glyphs are rotated 90 degrees counter-clockwise
 * @PANGO_GRAVITY_AUTO: Gravity is resolved from the context matrix
 * 
 * The #PangoGravity type represents the orientation of glyphs in a segment
 * of text.  This is useful when rendering vertical text layouts.  In
 * those situations, the layout is rotated using a non-identity PangoMatrix,
 * and then glyph orientation is controlled using #PangoGravity.
 * Not every value in this enumeration makes sense for every usage of
 * #PangoGravity; for example, %PANGO_GRAVITY_AUTO only can be passed to
 * pango_context_set_base_gravity() and can only be returned by
 * pango_context_get_base_gravity().
 *
 * Since: 1.16
 **/			  
typedef enum {
  PANGO_GRAVITY_SOUTH,
  PANGO_GRAVITY_EAST,
  PANGO_GRAVITY_NORTH,
  PANGO_GRAVITY_WEST,
  PANGO_GRAVITY_AUTO
} PangoGravity;

double pango_gravity_to_rotation (PangoGravity gravity);



/**
 * PangoMatrix:
 * @xx: 1st component of the transformation matrix
 * @xy: 2nd component of the transformation matrix
 * @yx: 3rd component of the transformation matrix
 * @yy: 4th component of the transformation matrix
 * @x0: x translation
 * @y0: y translation
 *
 * A structure specifying a transformation between user-space
 * coordinates and device coordinates. The transformation
 * is given by
 *
 * <programlisting>
 * x_device = x_user * matrix->xx + y_user * matrix->xy + matrix->x0;
 * y_device = x_user * matrix->yx + y_user * matrix->yy + matrix->y0;
 * </programlisting>
 *
 * Since: 1.6
 **/
struct _PangoMatrix
{
  double xx;
  double xy;
  double yx;
  double yy;
  double x0;
  double y0;
};

/**
 * PANGO_TYPE_MATRIX
 *
 * The GObject type for #PangoMatrix
 **/
#define PANGO_TYPE_MATRIX (pango_matrix_get_type ())

/**
 * PANGO_MATRIX_INIT
 *
 * Constant that can be used to initialize a PangoMatrix to
 * the identity transform.
 *
 * <informalexample><programlisting>
 * PangoMatrix matrix = PANGO_MATRIX_INIT;
 * pango_matrix_rotate (&amp;matrix, 45.);
 * </programlisting></informalexample>
 *
 * Since: 1.6
 **/
#define PANGO_MATRIX_INIT { 1., 0., 0., 1., 0., 0. }

GType pango_matrix_get_type (void);

PangoMatrix *pango_matrix_copy   (const PangoMatrix *matrix);
void         pango_matrix_free   (PangoMatrix *matrix);

void pango_matrix_translate (PangoMatrix *matrix,
			     double       tx,
			     double       ty);
void pango_matrix_scale     (PangoMatrix *matrix,
			     double       scale_x,
			     double       scale_y);
void pango_matrix_rotate    (PangoMatrix *matrix,
			     double       degrees);
void pango_matrix_concat    (PangoMatrix       *matrix,
			     const PangoMatrix *new_matrix);
double pango_matrix_get_font_scale_factor (const PangoMatrix *matrix);
PangoGravity pango_matrix_to_gravity (const PangoMatrix *matrix);



G_END_DECLS

#endif /* __PANGO_TYPES_H__ */

