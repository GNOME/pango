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

typedef struct _PangoFont PangoFont;
typedef struct _PangoRectangle PangoRectangle;

/* Dummy typedef - internally it's a 'const char *' */
typedef struct _PangoLanguage PangoLanguage;

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

#define PANGO_SCALE 1024
#define PANGO_PIXELS(d) (((d) >= 0) ?                             \
                         ((d) + PANGO_SCALE / 2) / PANGO_SCALE :  \
                         ((d) - PANGO_SCALE / 2) / PANGO_SCALE)

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
 *   same as PANGO_DIRECTION_LTR
 * @PANGO_DIRECTION_WEAK_LTR: A weak left-to-right direction
 * @PANGO_DIRECTION_WEAK_RTL: A weak right-to-left direction
 * @PANGO_DIRECTION_NEUTRAL: No direction specified
 * 
 * The #PangoDirection type represents a direction in the
 * Unicode bidirectional algorithm; not every value in this
 * enumeration makes sense for every usage of #PangoDirection;
 * for example, the return value of pango_unichar_direction()
 * and pango_find_base_direction() cannot be %PANGO_DIRECTION_WEAK_LTR
 * or %PANGO_DIRECTION_WEAK_RTL, since every character is either
 * neutral or has a strong direction; on the other hand
 * %PANGO_DIRECTION_NEUTRAL doesn't make sense to pass
 * to pango_log2vis_get_embedding_levels().
 *
 * The %PANGO_DIRECTION_TTB_LTR, %PANGO_DIRECTION_TTB_RTL
 * values come from an earlier interpretation of this
 * enumeration as the writing direction of a block of
 * text and are no longer used; See the Text module of the
 * CSS3 spec for how vertical text is planned to be handled
 * in a future version of Pango. The explanation of why
 * %PANGO_DIRECTION_TTB_LTR is treated as %PANGO_DIRECTION_RTL
 * can be found there as well.
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

#define PANGO_TYPE_LANGUAGE (pango_language_get_type ())

GType          pango_language_get_type    (void);
PangoLanguage *pango_language_from_string (const char *language);
#define pango_language_to_string(language) ((const char *)language)

gboolean      pango_language_matches  (PangoLanguage *language,
				       const char *range_list);

gboolean       pango_get_mirror_char        (gunichar     ch,
					     gunichar    *mirrored_ch);
PangoDirection pango_unichar_direction      (gunichar     ch);
PangoDirection pango_find_base_dir          (const gchar *text,
					     gint         length);

G_END_DECLS

#endif /* __PANGO_TYPES_H__ */

