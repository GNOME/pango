/* Pango
 * pango-glyph.h: Glyph storage
 *
 * Copyright (C) 2000 Red Hat Software
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

#ifndef __PANGO_GLYPH_H__
#define __PANGO_GLYPH_H__

#ifdef __cplusplus
extern "C" {
#endif /* __cplusplus */

typedef struct _PangoGlyphGeometry PangoGlyphGeometry;
typedef struct _PangoGlyphVisAttr PangoGlyphVisAttr;
typedef struct _PangoGlyphInfo PangoGlyphInfo;
typedef struct _PangoGlyphString PangoGlyphString;

/* 64'ths of a point - 1/4608 in, 5.51 * 10^-5 in. */
typedef guint32 PangoGlyphUnit;

/* A index of a glyph into a font. Rendering system dependent
 */
typedef guint32 PangoGlyph;

/* Positioning information about a glyph
 */
struct _PangoGlyphGeometry
{
  PangoGlyphUnit width;
  PangoGlyphUnit x_offset;  
  PangoGlyphUnit y_offset;
};

/* Visual attributes of a glyph
 */
struct _PangoGlyphVisAttr
{
  guint is_cluster_start : 1;
};

/* A single glyph 
 */
struct _PangoGlyphInfo
{
  PangoGlyph    glyph;
  PangoGlyphGeometry geometry;
  PangoGlyphVisAttr  attr;
};

/* A string of glyphs with positional information and visual attributes -
 * ready for drawing
 */
struct _PangoGlyphString {
  gint num_glyphs;

  PangoGlyphInfo *glyphs;

  /* This is a memory inefficient way of representing the
   * information here - each value gives the character index
   * of the start of the cluster to which the glyph belongs.
   */
  gint *log_clusters;

  /*< private >*/
  gint space;
};

PangoGlyphString *pango_glyph_string_new      (void);
void              pango_glyph_string_set_size (PangoGlyphString *string,
					       gint              new_len);
void              pango_glyph_string_free     (PangoGlyphString *string);

void pango_cp_to_x (gchar            *text,
		    gint              length,
		    PangoAnalysis    *analysis,
		    PangoGlyphString *glyphs,
		    gint              char_pos,
		    gboolean          trailing,
		    gint             *x_pos);
void pango_x_to_cp (gchar            *text,
		    gint              length,
		    PangoAnalysis    *analysis,
		    PangoGlyphString *glyphs,
		    gint              x_pos,
		    gint             *char_pos,
		    gint             *trailing);

void pango_justify (PangoGlyphString *glyphs,
                    gint              new_line_width,
                    gint              min_kashida_width);

#ifdef __cplusplus
}
#endif /* __cplusplus */

#endif __PANGO_GLYPH_H__
