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
#include <pango/pango-glyph.h>
#include <pango/pango-font.h>
#include <unicode.h>

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

	  if (logical_rect)
	    {
	      logical_rect->x = 0;
	      logical_rect->width = geometry->width;
	    }
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
				     x_pos + glyph_ink.x + glyph_ink.width + geometry->x_offset) - new_pos;
	      ink_rect->x = new_pos;

	      new_pos = MIN (ink_rect->y, glyph_ink.y + geometry->y_offset);
	      ink_rect->height = MAX (ink_rect->y + ink_rect->height,
				      glyph_ink.y + glyph_ink.height + geometry->y_offset) - new_pos;
	      ink_rect->y = new_pos;
	    }

	  if (logical_rect)
	    {
	      logical_rect->width += geometry->width;

	      new_pos = MIN (logical_rect->y, glyph_logical.y + geometry->y_offset);
	      logical_rect->height = MAX (logical_rect->y + logical_rect->height,
					  glyph_logical.y + glyph_logical.height + geometry->y_offset) - new_pos;
	      logical_rect->y = new_pos;
	    }
	}

      x_pos += geometry->width;
    }
}

/**
 * pango_glyph_string_get_logical_widths:
 * @glyphs: a #PangoGlyphString
 * @text: the text corresponding to the glyphs
 * @length: the length of @text, in bytes
 * @embedding_level: the embedding level of the string
 * @logical_widths: an array whose length is unicode_strlen (text, length)
 *                  to be filled in with the resulting character widths.
 *
 * Given a #PangoGlyphString resulting from pango_shape() and the corresponding
 * text, determine the screen width corresponding to each character. When
 * multiple characters compose a single cluster, the width of the entire
 * cluster is divided equally among the characters.
 **/
void
pango_glyph_string_get_logical_widths (PangoGlyphString *glyphs,
				       const char       *text,
				       int               length,
				       int               embedding_level,
				       int              *logical_widths)
{
  int i, j;
  int last_cluster = 0;
  int width = 0;
  int last_cluster_width = 0;
  const char *p = text;
  
  for (i=0; i<=glyphs->num_glyphs; i++)
    {
      int glyph_index = (embedding_level % 2 == 0) ? i : glyphs->num_glyphs - i - 1;
      
      if (i == glyphs->num_glyphs || p != text + glyphs->log_clusters[glyph_index])
	{
	  int next_cluster = last_cluster;
	  
	  if (glyph_index > 0 && glyph_index < glyphs->num_glyphs)
	    {
	      while (p < text + glyphs->log_clusters[glyph_index])
		{
		  next_cluster++;
		  p = unicode_next_utf8 (p);
		}
	    }
	  else
	    {
	      while (p < text + length)
		{
		  next_cluster++;
		  p = unicode_next_utf8 (p);
		}
	    }
	  
	  for (j = last_cluster; j < next_cluster; j++)
	    logical_widths[j] = (width - last_cluster_width) / (next_cluster - last_cluster);
	  
	  last_cluster = next_cluster;
	  last_cluster_width = width;
	}
      
      if (i < glyphs->num_glyphs)
	width += glyphs->glyphs[i].geometry.width;
    }
}

