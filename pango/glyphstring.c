/* Pango
 * glyphstring.c:
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

#include <glib.h>
#include <pango-glyph.h>
#include <pango-font.h>

/**
 * pango_glyph_string_new:
 *
 * Create a new PangoGlyphString.
 *
 * Returns the new PangoGlyphString
 */
PangoGlyphString *
pango_glyph_string_new (void)
{
  PangoGlyphString *string = g_new (PangoGlyphString, 1);

  string->num_glyphs = 0;
  string->space = 0;
  string->glyphs = NULL;
  string->log_clusters = NULL;

  return string;
}

/**
 * pango_glyph_string_set_size:
 * @string:    a PangoGlyphString.
 * @new_len:   the new length of the string.
 *
 * Resize a glyph string to the given length.
 */
void
pango_glyph_string_set_size (PangoGlyphString *string, gint new_len)
{
  g_return_if_fail (new_len >= 0);
  
  while (new_len > string->space)
    {
      if (string->space == 0)
	string->space = 1;
      else
	string->space *= 2;
      
      if (string->space < 0)
	string->space = G_MAXINT;
    }
  
  string->glyphs = g_realloc (string->glyphs, string->space * sizeof (PangoGlyphInfo));
  string->log_clusters = g_realloc (string->log_clusters, string->space * sizeof (gint));
  string->num_glyphs = new_len;
}

/**
 * pango_glyph_string_free:
 * @string:    a PangoGlyphString.
 *
 *  Free a glyph string and associated storage.
 */
void
pango_glyph_string_free (PangoGlyphString *string)
{
  g_free (string->glyphs);
  g_free (string->log_clusters);
  g_free (string);
}

/**
 * pango_glyph_string_extents:
 * @glyphs:   a #PangoGlyphString
 * @font:     a #PangoFont
 * @ink_rect: rectangle used to store the extents of the glyph string as drawn
 *            or %NULL to indicate that the result is not needed.
 * @logical_rect: rectangle used to store the logical extents of the glyph string
 *            or %NULL to indicate that the result is not needed.
 * 
 * Compute the logical and ink extents of a glyph string. See the documentation
 * for pango_font_get_glyph_extents() for details about the interpretation
 * of the rectangles.
 */
void 
pango_glyph_string_extents (PangoGlyphString *glyphs,
			    PangoFont        *font,
			    PangoRectangle   *ink_rect,
			    PangoRectangle   *logical_rect)
{
  int x_pos = 0;
  int i;

  if (glyphs->num_glyphs == 0)
    {
      if (ink_rect)
	{
	  ink_rect->x = 0;
	  ink_rect->y = 0;
	  ink_rect->width = 0;
	  ink_rect->height = 0;
	}
      
      if (logical_rect)
	{
	  logical_rect->x = 0;
	  logical_rect->y = 0;
	  logical_rect->width = 0;
	  logical_rect->height = 0;
	}

      return;
    }
  
  for (i=0; i<glyphs->num_glyphs; i++)
    {
      PangoRectangle glyph_ink;
      PangoRectangle glyph_logical;
      
      PangoGlyphGeometry *geometry = &glyphs->glyphs[i].geometry;

      if (i == 0)
	{
	  pango_font_get_glyph_extents (font, glyphs->glyphs[i].glyph, ink_rect, logical_rect);
	}
      else
	{
	  int new_pos;
	  
	  pango_font_get_glyph_extents (font, glyphs->glyphs[i].glyph,
					ink_rect ? &glyph_ink : NULL,
					logical_rect ? &glyph_logical : NULL);

	  if (ink_rect)
	    {
	      new_pos = MIN (ink_rect->x, x_pos + glyph_ink.x + geometry->x_offset);
	      ink_rect->width = MAX (ink_rect->x + ink_rect->width,
				     x_pos + glyph_ink.x + glyph_ink.width + geometry->x_offset) - ink_rect->x;
	      ink_rect->x = new_pos;

	      new_pos = MIN (ink_rect->y, glyph_ink.y + geometry->y_offset);
	      ink_rect->height = MAX (ink_rect->y + ink_rect->height,
				      glyph_ink.y + glyph_ink.height + geometry->y_offset) - ink_rect->y;
	      ink_rect->y = new_pos;
	    }

	  if (logical_rect)
	    {
	      new_pos = MIN (logical_rect->x, x_pos + glyph_logical.x + geometry->x_offset);
	      logical_rect->width = MAX (logical_rect->x + logical_rect->width,
				     x_pos + glyph_logical.x + glyph_logical.width + geometry->x_offset) - logical_rect->x;
	      logical_rect->x = new_pos;

	      new_pos = MIN (logical_rect->y, glyph_logical.y + geometry->y_offset);
	      logical_rect->height = MAX (logical_rect->y + logical_rect->height,
				      glyph_logical.y + glyph_logical.height + geometry->y_offset) - logical_rect->y;
	      logical_rect->y = new_pos;
	    }
	}

      x_pos += geometry->width;
    }
}

