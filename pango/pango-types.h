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

/* Include this here, since we don't depend on GLib 1.3 yet */

#ifndef G_N_ELEMENTS
#define G_N_ELEMENTS(arr)		(sizeof(arr) / sizeof((arr)[0]))
#endif

typedef struct _PangoLangRange PangoLangRange;
typedef struct _PangoLogAttr PangoLogAttr;

typedef struct _PangoEngineLang PangoEngineLang;
typedef struct _PangoEngineShape PangoEngineShape;

typedef struct _PangoFont PangoFont;
typedef struct _PangoRectangle PangoRectangle;

/* A index of a glyph into a font. Rendering system dependent
 */
typedef guint32 PangoGlyph;

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

#define PANGO_SCALE 1000

/* Macros to translate from extents rectangles to ascent/descent/lbearing/rbearing
 */
#define PANGO_ASCENT(rect) (-(rect).y)
#define PANGO_DESCENT(rect) ((rect).y + (rect).height)
#define PANGO_LBEARING(rect) ((rect).x)
#define PANGO_RBEARING(rect) ((rect).x + (rect).width)

/* Information about a segment of text with a consistent
 * shaping/language engine and bidirectional level
 */

typedef enum {
  PANGO_DIRECTION_LTR,
  PANGO_DIRECTION_RTL,
  PANGO_DIRECTION_TTB_LTR,
  PANGO_DIRECTION_TTB_RTL
} PangoDirection;

/* Language tagging information
 */
struct _PangoLangRange
{
  gint start;
  gint length;
  gchar *lang;
};

#ifdef __cplusplus
extern "C" {
#endif /* __cplusplus */

#ifdef __cplusplus
}
#endif /* __cplusplus */


#endif /* __PANGO_TYPES_H__ */

