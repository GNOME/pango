/* Pango
 * pangox.h:
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

#ifndef __PANGOX_H__
#define __PANGOX_H__

#include <glib.h>
#include <pango.h>

#ifdef __cplusplus
extern "C" {
#endif /* __cplusplus */

#include <X11/Xlib.h>

#define PANGO_RENDER_TYPE_X "PangoRenderX"

/* Calls for applications
 */
PangoFont * pango_x_load_font     (Display          *display,
				   char             *spec);
void        pango_x_render        (Display          *display,
				   Drawable          d,
				   GC                gc,
				   PangoFont        *font,
				   PangoGlyphString *glyphs,
				   int               x,
				   int               y);
void        pango_x_extents       (PangoFont        *font,
				   PangoGlyphString *glyphs,
				   int              *lbearing,
				   int              *rbearing,
				   int              *width,
				   int              *ascent,
				   int              *descent,
				   int              *logical_ascent,
				   int              *logical_descent);
void        pango_x_glyph_extents (PangoFont        *font,
				   PangoGlyph        glyph,
				   int              *lbearing,
				   int              *rbearing,
				   int              *width,
				   int              *ascent,
				   int              *descent,
				   int              *logical_ascent,
				   int              *logical_descent);

/* API for rendering modules
 */
typedef guint16 PangoXSubfontID;

#define PANGO_X_MAKE_GLYPH(subfont,index) (subfont<<16 | index)
#define PANGO_X_GLYPH_SUBFONT(glyph) (glyph>>16)
#define PANGO_X_GLYPH_INDEX(glyph) (glyph & 0xffff)

void          pango_x_list_charsets (PangoFont      *font,
				     char          **charsets,
				     int             n_charsets,
				     int             charsets);

PangoXCharset pango_x_find_charset (PangoFont       *font,
				    char           *charset);
gboolean      pango_x_has_glyph    (PangoFont       *font,
				    PangoGlyph       glyph);

#ifdef __cplusplus
}
#endif /* __cplusplus */


#endif /* __PANGOX_H__ */




