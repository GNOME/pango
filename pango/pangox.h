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

typedef struct _PangoXCFont PangoXCFont;

struct _PangoXCFont {
  /*< private >*/
  PangoCFont font;

  /*< public >*/
  Display *display;
  XFontStruct *font_struct;
};

/* Calls for applications
 */
PangoFont *pango_x_load_font     (Display          *display,
				  gchar            *spec);
void       pango_x_render        (Display          *display,
				  Drawable          d,
				  GC                gc,
				  PangoGlyphString *glyphs,
				  gint              x,
				  gint              y);
void       pango_x_extents       (PangoGlyphString *glyphs,
				  gint             *lbearing,
				  gint             *rbearing,
				  gint             *width,
				  gint             *ascent,
				  gint             *descent,
				  gint             *logical_ascent,
				  gint             *logical_descent);
void       pango_x_glyph_extents (PangoGlyph       *glyph,
				  gint             *lbearing,
				  gint             *rbearing,
				  gint             *width,
				  gint             *ascent,
				  gint             *descent,
				  gint             *logical_ascent,
				  gint             *logical_descent);


/* Calls for rendering modules
 */
PangoCFont *pango_x_find_cfont      (PangoFont   *font,
				     gchar       *charset);
void        pango_x_list_cfonts     (PangoFont   *font,
				     gchar      **charsets,
				     gint         n_charsets,
				     gchar     ***xlfds,
				     gint        *n_xlfds);
gboolean    pango_x_xlfd_get_ranges (PangoFont   *font,
				     gchar       *xlfd,
				     gint       **ranges,
				     gint        *n_ranges);
PangoCFont *pango_x_load_xlfd       (PangoFont   *font,
				     gchar       *xlfd);
			   
#ifdef __cplusplus
}
#endif /* __cplusplus */


#endif /* __PANGOX_H__ */




