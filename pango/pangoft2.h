/* Pango
 * pangoft2.h:
 *
 * Copyright (C) 1999 Red Hat Software
 * Copyright (C) 2000 Tor Lillqvist
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

#ifndef __PANGOFT2_H__
#define __PANGOFT2_H__

#include <pango/pango-layout.h>

#include <freetype/freetype.h>

G_BEGIN_DECLS

#define PANGO_RENDER_TYPE_FT2 "PangoRenderFT2"

/* Calls for applications */
PangoContext  *pango_ft2_get_context        (double dpi_x, double dpi_y);

void           pango_ft2_render             (FT_Bitmap        *bitmap,
					     PangoFont        *font,
					     PangoGlyphString *glyphs,
					     gint              x,
					     gint              y);
void           pango_ft2_render_layout_line (FT_Bitmap        *bitmap,
					     PangoLayoutLine  *line,
					     int               x,
					     int               y);
void           pango_ft2_render_layout      (FT_Bitmap        *bitmap,
					     PangoLayout      *layout,
					     int               x, 
					     int               y);

PangoFontMap      *pango_ft2_font_map_for_display    (void);
void               pango_ft2_shutdown_display        (void);

/* API for rendering modules
 */
PangoGlyph     pango_ft2_get_unknown_glyph (PangoFont       *font);
int            pango_ft2_font_get_kerning  (PangoFont       *font,
					    PangoGlyph       left,
					    PangoGlyph       right);
FT_Face        pango_ft2_font_get_face     (PangoFont       *font);
PangoCoverage *pango_ft2_font_get_coverage (PangoFont       *font,
					    PangoLanguage   *language);

G_END_DECLS

#endif /* __PANGOFT2_H__ */
