/* Pango
 * mapping.c:
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

/* The initial implementation here is script independent,
 * but it might actually need to be virtualized into the
 * rendering modules. Otherwise, we probably will end up
 * enforcing unnatural cursor behavior for some languages.
 *
 * The only distinction that Uniscript makes is whether
 * cursor positioning is allowed within clusters or not.
 */

#include "pango-glyph.h"

/**
 * pango_glyph_string_index_to_x:
 * @glyphs:    the glyphs return from pango_shape()
 * @text:      the text for the run
 * @length:    the number of bytes (not characters) in @text.
 * @analysis:  the analysis information return from pango_itemize()
 * @index_:    the byte index within @text
 * @trailing:  whether we should compute the result for the beginning
 *             or end of the character.
 * @x_pos:     location to store result
 *
 * Converts from character position to x position. (X position
 * is measured from the left edge of the run). Character positions
 * are computed by dividing up each cluster into equal portions.
 */

void
pango_glyph_string_index_to_x (PangoGlyphString *glyphs,
			       char             *text,
			       int               length,
			       PangoAnalysis    *analysis,
			       int               index,
			       gboolean          trailing,
			       int              *x_pos)
{
  int i;
  int start_xpos = 0;
  int end_xpos = 0;
  int width = 0;

  int start_index = -1;
  int end_index = -1;

  int cluster_chars = 0;
  int cluster_offset = 0;

  char *p;
  
  g_return_if_fail (glyphs != NULL);
  g_return_if_fail (length >= 0);
  g_return_if_fail (length == 0 || text != NULL);

  if (!x_pos) /* Allow the user to do the useless */
    return;
  
  if (glyphs->num_glyphs == 0)
    {
      *x_pos = 0;
      return;
    }

  /* Calculate the starting and ending character positions
   * and x positions for the cluster
   */
  if (analysis->level % 2) /* Right to left */
    {
      for (i = glyphs->num_glyphs - 1; i >= 0; i--)
	width += glyphs->glyphs[i].geometry.width;

      for (i = glyphs->num_glyphs - 1; i >= 0; i--)
	{
	  if (glyphs->log_clusters[i] > index)
	    {
	      end_index = glyphs->log_clusters[i];
	      end_xpos = width;
	      break;
	    }

	  if (glyphs->log_clusters[i] != start_index)
	    {
	      start_index = glyphs->log_clusters[i];
	      start_xpos = width;
	    }

	  width -= glyphs->glyphs[i].geometry.width;
	}
    }
  else /* Left to right */
    {
      for (i = 0; i < glyphs->num_glyphs; i++)
	{
	  if (glyphs->log_clusters[i] > index)
	    {
	      end_index = glyphs->log_clusters[i];
	      end_xpos = width;
	      break;
	    }

	  if (glyphs->log_clusters[i] != start_index)
	    {
	      start_index = glyphs->log_clusters[i];
	      start_xpos = width;
	    }
	  
	  width += glyphs->glyphs[i].geometry.width;
	}
    }

  if (end_index == -1)
    {
      end_index = length;
      end_xpos = (analysis->level % 2) ? 0 : width;
    }

  /* Calculate offset of character within cluster */

  p = text + start_index;
  while (p < text + end_index)
    {
      if (p < text + index)
	cluster_offset++;
      cluster_chars++;
      p = g_utf8_next_char (p);
    }
  
  if (trailing)
    cluster_offset += 1;

  *x_pos = ((cluster_chars - cluster_offset) * start_xpos +
	    cluster_offset * end_xpos) / cluster_chars;
}

/**
 * pango_glyph_string_x_to_index:
 * @glyphs:    the glyphs return from pango_shape()
 * @text:      the text for the run
 * @length:    the number of bytes (not characters) in text.
 * @analysis:  the analysis information return from pango_itemize()
 * @x_pos:     the x offset (in #PangoGlyphUnit)
 * @index_:    location to store calculated byte index within @text
 * @trailing:  location to store a integer indicating where
 *             whether the user clicked on the leading or trailing
 *             edge of the character.
 *
 * Convert from x offset to character position. Character positions
 * are computed by dividing up each cluster into equal portions.
 * In scripts where positioning within a cluster is not allowed
 * (such as Thai), the returned value may not be a valid cursor
 * position; the caller must combine the result with the logical
 * attributes for the text to compute the valid cursor position.
 */
void 
pango_glyph_string_x_to_index (PangoGlyphString *glyphs,
			       char             *text,
			       int               length,
			       PangoAnalysis    *analysis,
			       int               x_pos,
			       int              *index, 
			       gboolean         *trailing)
{
  int i;
  int start_xpos = 0;
  int end_xpos = 0;
  int width = 0;

  int start_index = -1;
  int end_index = -1;

  int cluster_chars = 0;
  char *p;

  gboolean found = FALSE;

  /* Find the cluster containing the position */

  width = 0;

  if (analysis->level % 2) /* Right to left */
    {
      for (i = glyphs->num_glyphs - 1; i >= 0; i--)
	width += glyphs->glyphs[i].geometry.width;

      for (i = glyphs->num_glyphs - 1; i >= 0; i--)
	{
	  if (glyphs->log_clusters[i] != start_index)
	    {
	      if (found)
		{
		  end_index = glyphs->log_clusters[i];
		  end_xpos = width;
		  break;
		}
	      else
		{
		  start_index = glyphs->log_clusters[i];
		  start_xpos = width;
		}
	    }

	  width -= glyphs->glyphs[i].geometry.width;

	  if (width <= x_pos && x_pos < width + glyphs->glyphs[i].geometry.width)
	    found = TRUE;
	}
    }
  else /* Left to right */
    {
      for (i = 0; i < glyphs->num_glyphs; i++)
	{
	  if (glyphs->log_clusters[i] != start_index)
	    {
	      if (found)
		{
		  end_index = glyphs->log_clusters[i];
		  end_xpos = width;
		  break;
		}
	      else
		{
		  start_index = glyphs->log_clusters[i];
		  start_xpos = width;
		}
	    }
	  
	  if (width <= x_pos && x_pos < width + glyphs->glyphs[i].geometry.width)
	    found = TRUE;
	  
	  width += glyphs->glyphs[i].geometry.width;
	}
    }

  if (end_index == -1)
    {
      end_index = length;
      end_xpos = (analysis->level % 2) ? 0 : width;
    }

  /* Calculate number of chars within cluster */
  p = text + start_index;
  while (p < text + end_index)
    {
      p = g_utf8_next_char (p);
      cluster_chars++;
    }
  
  if (start_xpos == end_xpos)
    {
      if (index)
	*index = start_index;
      if (trailing)
	*trailing = 0;
    }
  else
    {
      double cp = ((double)(x_pos - start_xpos) * cluster_chars) / (end_xpos - start_xpos);

      /* LTR and right-to-left have to be handled separately
       * here because of the edge condition when we are exactly
       * at a pixel boundary; end_xpos goes with the next
       * character for LTR, with the previous character for RTL.
       */
      if (start_xpos < end_xpos) /* Left-to-right */
	{
	  if (index)
	    {
	      char *p = text + start_index;
	      int i = 0;
	      
	      while (i + 1 <= cp)
		{
		  p = g_utf8_next_char (p);
		  i++;
		}
	      
	      *index = (p - text);
	    }
	  
	  if (trailing)
	    *trailing = (cp - (int)cp >= 0.5) ? 1 : 0;
	}
      else /* Right-to-left */
	{
	  if (index)
	    {
	      char *p = text + start_index;
	      int i = 0;
	      
	      while (i + 1 < cp)
		{
		  p = g_utf8_next_char (p);
		  i++;
		}
	      
	      *index = (p - text);
	    }
	  
	  if (trailing)
	    {
	      double cp_flip = cluster_chars - cp;
	      *trailing = (cp_flip - (int)cp_flip >= 0.5) ? 0 : 1;
	    }
	}
    }
}
