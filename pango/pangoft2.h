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

#include <glib.h>
#include <pango/pango.h>

#ifdef __cplusplus
extern "C" {
#endif /* __cplusplus */

#include <freetype/freetype.h>

#define PANGO_RENDER_TYPE_FT2 "PangoRenderFT2"

/* Calls for applications
 */
PangoContext  *pango_ft2_get_context        (void);

PangoFont     *pango_ft2_load_font          (PangoFontMap     *fontmap,
					     FT_Open_Args    **open_args,
					     FT_Long	      *face_indices,
					     int               n_fonts,
					     int               size);
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


/* API for rendering modules
 */
typedef guint16 PangoFT2Subfont;

#define PANGO_FT2_MAKE_GLYPH(subfont,index) ((subfont)<<16 | (index))
#define PANGO_FT2_GLYPH_SUBFONT(glyph) ((glyph)>>16)
#define PANGO_FT2_GLYPH_INDEX(glyph) ((glyph) & 0xFFFF)

int        pango_ft2_n_subfonts        (PangoFont          *font);
gboolean   pango_ft2_has_glyph         (PangoFont          *font,
				        PangoGlyph          glyph);
PangoGlyph pango_ft2_get_unknown_glyph (PangoFont          *font);
int        pango_ft2_font_get_kerning  (PangoFont          *font,
					PangoGlyph          left,
					PangoGlyph          right);

/* API for libraries that want to use PangoFT2 mixed with classic
 * FT2 fonts.
 */
typedef struct _PangoFT2FontCache PangoFT2FontCache;

PangoFT2FontCache *pango_ft2_font_cache_new          (FT_Library         library);
void               pango_ft2_font_cache_free         (PangoFT2FontCache *cache);
FT_Face            pango_ft2_font_cache_load         (PangoFT2FontCache *cache,
						      FT_Open_Args      *args,
						      FT_Long            face_index);
void               pango_ft2_font_cache_unload       (PangoFT2FontCache *cache,
						      FT_Face            face);
PangoFontMap      *pango_ft2_font_map_for_display    (void);
void               pango_ft2_shutdown_display        (void);
PangoFT2FontCache *pango_ft2_font_map_get_font_cache (PangoFontMap      *font_map);
void               pango_ft2_font_subfont_open_args  (PangoFont         *font,
						      PangoFT2Subfont    subfont_id,
						      FT_Open_Args     **open_args,
						      FT_Long           *face_index);


/* Debugging.
 */
void     pango_ft2_fontmap_dump (int           indent,
				 PangoFontMap *fontmap);

#ifdef __cplusplus
}
#endif /* __cplusplus */


#endif /* __PANGOFT2_H__ */
