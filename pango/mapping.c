/* Pango
 * itemize.c:
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

/* The initial implementation here is script independent,
 * but it might actually need to be virtualized into the
 * rendering modules. Otherwise, we probably will end up
 * enforcing unnatural cursor behavior for some languages.
 *
 * The only distinction that Uniscript makes is whether
 * cursor positioning is allowed within clusters or not.
 */

#include <pango.h>
#include <unicode.h>

/**
 * pango_cp_to_x:
 * @text:      the text for the run
 * @length:    the number of bytes (not characters) in @text.
 * @analysis:  the analysis information return from pango_itemize()
 * @glyphs:    the glyphs return from pango_shape()
 * @char_pos:  the character position
 * @trailing:  whether we should compute the result for the beginning
 *             or end of the character (or cluster - the decision
 *             for which may be script dependent).
 * @x_pos:     location to store result
 *
 * Converts from character position to x position. (X position
 * is measured from the left edge of the run)
 */

void
pango_cp_to_x (gchar           *text,
		  gint             length,
		  PangoAnalysis *analysis,
		  PangoGlyphString    *glyphs,
		  gint             char_pos,
		  gboolean         trailing,
		  gint            *x_pos)
{
  gint i;
  gint start_xpos = 0;
  gint end_xpos = 0;
  gint width = 0;

  gint start_char = -1;
  gint end_char = -1;

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
	width += glyphs->geometry[i].width;

      for (i = glyphs->num_glyphs - 1; i >= 0; i--)
	{
	  if (glyphs->log_clusters[i] > char_pos)
	    {
	      end_char = glyphs->log_clusters[i];
	      end_xpos = width;
	      break;
	    }

	  if (start_char == -1 || glyphs->log_clusters[i] != start_char)
	    {
	      start_char = glyphs->log_clusters[i];
	      start_xpos = width;
	    }

	  width -= glyphs->geometry[i].width;
	}
    }
  else /* Left to right */
    {
      for (i = 0; i < glyphs->num_glyphs; i++)
	{
	  if (glyphs->log_clusters[i] > char_pos)
	    {
	      end_char = glyphs->log_clusters[i];
	      end_xpos = width;
	      break;
	    }

	  if (start_char == -1 || glyphs->log_clusters[i] != start_char)
	    {
	      start_char = glyphs->log_clusters[i];
	      start_xpos = width;
	    }
	  
	  width += glyphs->geometry[i].width;
	}
    }

  /* We need the character index of one past the end of the
   * string, and so we have to recalculate the entire length
   * of the string...
   */
  if (end_char == -1)
    {
      end_char = unicode_strlen (text, length);
      end_xpos = (analysis->level % 2) ? 0 : width;
    }

  /* Now interpolate the result. For South Asian languages
   * we actually shouldn't iterpolate
   */

  if (trailing)
    char_pos += 1;

  *x_pos = (((double)(end_char - char_pos)) * start_xpos +
	    ((double)(char_pos - start_char)) * end_xpos) /
    (end_char - start_char);
}

/**
 * pango_x_to_cp:
 * @text:      the text for the run
 * @length:    the number of bytes (not characters) in text.
 * @analysis:  the analysis information return from pango_itemize()
 * @glyphs:    the glyphs return from pango_shape()
 * @x_pos:     location to store the returned x character position
 * @char_pos:  location to store calculated character position.
 * @trailing:  location to store a integer indicating where
 *             in the cluster the user clicked. If the script
 *             allows positioning within the cluster, it is either
 *             0 or 1; otherwise it is either 0 or the number
 *             of characters in the cluster. In either case
 *             0 represents the trailing edge of the cluster.
 *
 * Converts from x position to x character. (X position
 * is measured from the left edge of the run)
 */
void 
pango_x_to_cp (gchar           *text,
		  gint             length,
		  PangoAnalysis *analysis,
		  PangoGlyphString    *glyphs,
		  gint             x_pos,
		  gint            *char_pos, 
		  gint            *trailing)
{
  gint i;
  gint start_xpos = 0;
  gint end_xpos = 0;
  gint width = 0;

  gint start_char = -1;
  gint end_char = -1;

  gboolean found = FALSE;

  /* Find the cluster containing the position */

  width = 0;

  if (analysis->level % 2) /* Right to left */
    {
      for (i = glyphs->num_glyphs - 1; i >= 0; i--)
	width += glyphs->geometry[i].width;

      for (i = glyphs->num_glyphs - 1; i >= 0; i--)
	{
	  if (start_char == -1 || glyphs->log_clusters[i] != start_char)
	    {
	      if (found)
		{
		  end_char = glyphs->log_clusters[i];
		  end_xpos = width;
		  break;
		}
	      else
		{
		  start_char = glyphs->log_clusters[i];
		  start_xpos = width;
		}
	    }

	  width -= glyphs->geometry[i].width;

	  if (width <= x_pos && x_pos < width + glyphs->geometry[i].width)
	    found = TRUE;
	}
    }
  else /* Left to right */
    {
      for (i = 0; i < glyphs->num_glyphs; i++)
	{
	  if (start_char == -1 || glyphs->log_clusters[i] != start_char)
	    {
	      if (found)
		{
		  end_char = glyphs->log_clusters[i];
		  end_xpos = width;
		  break;
		}
	      else
		{
		  start_char = glyphs->log_clusters[i];
		  start_xpos = width;
		}
	    }
	  
	  if (width <= x_pos && x_pos < width + glyphs->geometry[i].width)
	    found = TRUE;
	  
	  width += glyphs->geometry[i].width;
	}
    }

  /* We need the character index of one past the end of the
   * string, and so we have to recalculate the entire length
   * of the string...
   */
  if (end_char == -1)
    {
      end_char = unicode_strlen (text, length);
      end_xpos = (analysis->level % 2) ? 0 : width;
    }

  if (start_xpos == end_xpos)
    {
      if (char_pos)
	*char_pos = start_char;
      if (trailing)
	trailing = 0;
    }
  else
    {
      double cp = (((double)(end_xpos - x_pos)) * start_char +
		   ((double)(x_pos - start_xpos)) * end_char) /
	(end_xpos - start_xpos); 

      if (char_pos)
	*char_pos = (int)cp;
      if (trailing)
	*trailing = (cp - (int)cp) > 0.5 ? 1 : 0;
    }
}



