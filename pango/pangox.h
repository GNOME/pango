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
PangoContext * pango_x_get_context        (Display          *display);
PangoFont *    pango_x_load_font          (Display          *display,
					   gchar            *spec);
void           pango_x_render             (Display          *display,
					   Drawable          d,
					   GC                gc,
					   PangoFont        *font,
					   PangoGlyphString *glyphs,
					   gint              x,
					   gint              y);
void           pango_x_render_layout_line (Display          *display,
					   Drawable          drawable,
					   GC                gc,
					   PangoLayoutLine  *line,
					   int               x,
					   int               y);
void           pango_x_render_layout      (Display        *display,
					   Drawable        drawable,
					   GC              gc,
					   PangoLayout    *layout,
					   int             x, 
					   int             y);

/* API for rendering modules
 */
typedef guint16 PangoXSubfont;

#define PANGO_X_MAKE_GLYPH(subfont,index) (subfont<<16 | index)
#define PANGO_X_GLYPH_SUBFONT(glyph) (glyph>>16)
#define PANGO_X_GLYPH_INDEX(glyph) (glyph & 0xffff)

int        pango_x_list_subfonts     (PangoFont      *font,
				      char          **charsets,
				      int             n_charsets,
				      PangoXSubfont **subfont_ids,
				      int           **subfont_charsets);
gboolean   pango_x_has_glyph         (PangoFont      *font,
				      PangoGlyph      glyph);
PangoGlyph pango_x_get_unknown_glyph (PangoFont      *font);

#ifdef __cplusplus
}
#endif /* __cplusplus */


#endif /* __PANGOX_H__ */




