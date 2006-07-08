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

#include <config.h>
#include <glib.h>
#include "pango-glyph.h"
#include "pango-font.h"
#include "pango-impl-utils.h"

/**
 * pango_glyph_string_new:
 *
 * Create a new PangoGlyphString.
 *
 * Return value: the newly allocated #PangoGlyphString, which
 *               should be freed with pango_glyph_string_free().
 */
PangoGlyphString *
pango_glyph_string_new (void)
{
  PangoGlyphString *string = g_slice_new (PangoGlyphString);

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
        {
	  g_warning ("glyph string length overflows maximum integer size, truncated");
	  new_len = string->space = G_MAXINT - 8;
	}
    }
  
  string->glyphs = g_realloc (string->glyphs, string->space * sizeof (PangoGlyphInfo));
  string->log_clusters = g_realloc (string->log_clusters, string->space * sizeof (gint));
  string->num_glyphs = new_len;
}

GType
pango_glyph_string_get_type (void)
{
  static GType our_type = 0;
  
  if (our_type == 0)
    our_type = g_boxed_type_register_static (I_("PangoGlyphString"),
					     (GBoxedCopyFunc)pango_glyph_string_copy,
					     (GBoxedFreeFunc)pango_glyph_string_free);

  return our_type;
}

/**
 * pango_glyph_string_copy:
 * @string: a PangoGlyphString.
 *
 *  Copy a glyph string and associated storage.
 *
 * Return value: the newly allocated #PangoGlyphString, which
 *               should be freed with pango_glyph_string_free().
 */
PangoGlyphString *
pango_glyph_string_copy (PangoGlyphString *string)
{
  PangoGlyphString *new_string = g_slice_new (PangoGlyphString);

  *new_string = *string;

  new_string->glyphs = g_memdup (string->glyphs,
				 string->space * sizeof (PangoGlyphInfo));
  new_string->log_clusters = g_memdup (string->log_clusters,
				       string->space * sizeof (gint));

  return new_string;
}

/**
 * pango_glyph_string_free:
 * @string:    a PangoGlyphString.
 *
 * Free a glyph string and associated storage.
 */
void
pango_glyph_string_free (PangoGlyphString *string)
{
  g_free (string->glyphs);
  g_free (string->log_clusters);
  g_slice_free (PangoGlyphString, string);
}

/**
 * pango_glyph_string_extents_range:
 * @glyphs:   a #PangoGlyphString
 * @start:    start index
 * @end:      end index (the range is the set of bytes with
              indices such that start <= index < end)
 * @font:     a #PangoFont
 * @ink_rect: rectangle used to store the extents of the glyph string range as drawn
 *            or %NULL to indicate that the result is not needed.
 * @logical_rect: rectangle used to store the logical extents of the glyph string range
 *            or %NULL to indicate that the result is not needed.
 *
 * Computes the extents of a sub-portion of a glyph string. The extents are
 * relative to the start of the glyph string range (the origin of their
 * coordinate system is at the start of the range, not at the start of the entire
 * glyph string).
 **/
void
pango_glyph_string_extents_range (PangoGlyphString *glyphs,
                                  int               start,
                                  int               end,
                                  PangoFont        *font,
                                  PangoRectangle   *ink_rect,
                                  PangoRectangle   *logical_rect)
{
  int x_pos = 0;
  int i;

  /* Note that the handling of empty rectangles for ink
   * and logical rectangles is different. A zero-height ink
   * rectangle makes no contribution to the overall ink rect,
   * while a zero-height logical rect still reserves horizontal
   * width. Also, we may return zero-width, positive height
   * logical rectangles, while we'll never do that for the
   * ink rect.
   */
  g_return_if_fail (start <= end);

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
  
  for (i = start; i < end; i++)
    {
      PangoRectangle glyph_ink;
      PangoRectangle glyph_logical;
      
      PangoGlyphGeometry *geometry = &glyphs->glyphs[i].geometry;

      pango_font_get_glyph_extents (font, glyphs->glyphs[i].glyph,
				    ink_rect ? &glyph_ink : NULL,
				    logical_rect ? &glyph_logical : NULL);

      if (ink_rect && glyph_ink.width != 0 && glyph_ink.height != 0)
	{
	  if (ink_rect->width == 0 || ink_rect->height == 0)
	    {
	      ink_rect->x = x_pos + glyph_ink.x + geometry->x_offset;
	      ink_rect->width = glyph_ink.width;
	      ink_rect->y = glyph_ink.y + geometry->y_offset;
	      ink_rect->height = glyph_ink.height;
	    }
	  else
	    {
	      int new_x, new_y;
	      
	      new_x = MIN (ink_rect->x, x_pos + glyph_ink.x + geometry->x_offset);
	      ink_rect->width = MAX (ink_rect->x + ink_rect->width,
				     x_pos + glyph_ink.x + glyph_ink.width + geometry->x_offset) - new_x;
	      ink_rect->x = new_x;
	      
	      new_y = MIN (ink_rect->y, glyph_ink.y + geometry->y_offset);
	      ink_rect->height = MAX (ink_rect->y + ink_rect->height,
				      glyph_ink.y + glyph_ink.height + geometry->y_offset) - new_y;
	      ink_rect->y = new_y;
	    }
	}

      if (logical_rect)
	{
	  logical_rect->width += geometry->width;

	  if (i == start)
	    {
	      logical_rect->y = glyph_logical.y;
	      logical_rect->height = glyph_logical.height;
	    }
	  else
	    {
	      int new_y = MIN (logical_rect->y, glyph_logical.y);
	      logical_rect->height = MAX (logical_rect->y + logical_rect->height,
					  glyph_logical.y + glyph_logical.height) - new_y;
	      logical_rect->y = new_y;
	    }
	}

      x_pos += geometry->width;
    }
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
  pango_glyph_string_extents_range (glyphs, 0, glyphs->num_glyphs,
                                    font, ink_rect, logical_rect);
}

/**
 * pango_glyph_string_get_width:
 * @glyphs:   a #PangoGlyphString
 * 
 * Computes the logical width of the glyph string as can also be computed
 * using pango_glyph_string_extents().  However, since this only computes the
 * width, it's much faster.  This is in fact only a convenience function that
 * computes the sum of geometry.width for each glyph in the @glyphs.
 *
 * Return value: the logical width of the glyph string.
 *
 * Since: 1.14
 */
int
pango_glyph_string_get_width (PangoGlyphString *glyphs)
{
  int i;
  int width = 0;

  for (i = 0; i < glyphs->num_glyphs; i++)
    width += glyphs->glyphs[i].geometry.width;

  return width;
}

/**
 * pango_glyph_string_get_logical_widths:
 * @glyphs: a #PangoGlyphString
 * @text: the text corresponding to the glyphs
 * @length: the length of @text, in bytes
 * @embedding_level: the embedding level of the string
 * @logical_widths: an array whose length is g_utf8_strlen (text, length)
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
  const char *p = text;		/* Points to start of current cluster */
  
  for (i=0; i<=glyphs->num_glyphs; i++)
    {
      int glyph_index = (embedding_level % 2 == 0) ? i : glyphs->num_glyphs - i - 1;

      /* If this glyph belongs to a new cluster, or we're at the end, find
       * the start of the next cluster, and assign the widths for this cluster.
       */
      if (i == glyphs->num_glyphs || p != text + glyphs->log_clusters[glyph_index])
	{
	  int next_cluster = last_cluster;
	  
	  if (i < glyphs->num_glyphs)
	    {
	      while (p < text + glyphs->log_clusters[glyph_index])
		{
		  next_cluster++;
		  p = g_utf8_next_char (p);
		}
	    }
	  else
	    {
	      while (p < text + length)
		{
		  next_cluster++;
		  p = g_utf8_next_char (p);
		}
	    }
	  
	  for (j = last_cluster; j < next_cluster; j++)
	    logical_widths[j] = (width - last_cluster_width) / (next_cluster - last_cluster);
	  
	  last_cluster = next_cluster;
	  last_cluster_width = width;
	}
      
      if (i < glyphs->num_glyphs)
	width += glyphs->glyphs[glyph_index].geometry.width;
    }
}

