#ifndef HEBREW_SHAPER_H
#define HEBREW_SHAPER_H

char *
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
