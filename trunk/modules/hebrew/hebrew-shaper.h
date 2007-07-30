/* Pango
 * hebrew-shaper.h: Hebrew shaper internal functions
 *
 * Copyright (c) 2001 by Sun Microsystems, Inc.
 * Author: Chookij Vanatham <Chookij.Vanatham@Eng.Sun.COM>
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

#ifndef HEBREW_SHAPER_H
#define HEBREW_SHAPER_H

const char *
hebrew_shaper_get_next_cluster(const char	*text,
			       gint		length,
			       gunichar       *cluster,
			       gint		*num_chrs);

void
hebrew_shaper_get_cluster_kerning(gunichar            *cluster,
				  gint                cluster_length,
				  PangoRectangle      ink_rect[],

				  /* input and output */
				  gint                width[],
				  gint                x_offset[],
				  gint                y_offset[]);

void
hebrew_shaper_swap_range (PangoGlyphString *glyphs,
			  int               start,
			  int               end);

void
hebrew_shaper_bidi_reorder(PangoGlyphString *glyphs);

#endif
