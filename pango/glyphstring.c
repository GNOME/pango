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
#include <pango.h>

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
  string->geometry = NULL;
  string->attrs = NULL;
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
  
  string->glyphs = g_realloc (string->glyphs, string->space * sizeof (PangoGlyph));
  string->geometry = g_realloc (string->geometry, string->space * sizeof (PangoGlyphGeometry));
  string->attrs = g_realloc (string->attrs, string->space * sizeof (PangoGlyphVisAttr));
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
  int i;
  PangoCFont *last_cfont = NULL;

  for (i=0; i<string->num_glyphs; i++)
    {
      if (last_cfont && string->glyphs[i].font != last_cfont)
	{
	  pango_cfont_unref (last_cfont);
	  last_cfont = string->glyphs[i].font;
	}
    }
  
  if (last_cfont)
    pango_cfont_unref (last_cfont);

  g_free (string->glyphs);
  g_free (string->geometry);
  g_free (string->attrs);
  g_free (string->log_clusters);
  g_free (string);
}
