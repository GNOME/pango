/* Pango
 * break.c:
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

#include "pango.h"

/**
 * pango_break:
 * @text:      the text to process
 * @length:    the length (in bytes) of @text
 * @analysis:  #PangoAnalysis structure from PangoItemize
 * @attrs:     an array to store character information in
 *
 * Determines possible line, word, and character breaks
 * for a string of Unicode text.
 */
void pango_break (const gchar   *text, 
		  gint           length, 
		  PangoAnalysis *analysis, 
		  PangoLogAttr  *attrs)
{
  /* Pseudo-implementation */

  const gchar *cur = text;
  gint i = 0;
  gunichar wc;
  
  while (*cur && cur - text < length)
    {
      wc = g_utf8_get_char (cur);
      if (wc == (gunichar)-1)
	break;			/* FIXME: ERROR */

      attrs[i].is_white = (wc == ' ' || wc == '\t' || wc == '\n' || wc == 0x200b) ? 1 : 0;
      attrs[i].is_break = i == 0 || attrs[i-1].is_white || attrs[i].is_white;
      attrs[i].is_char_stop = 1;
      attrs[i].is_word_stop = (i == 0) || attrs[i-1].is_white;
      
      i++;
      cur = g_utf8_next_char (cur);
    }
}
