/* Pango
 * shape.c: Convert characters into glyphs.
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

#include "pango-impl-utils.h"
#include "pango-glyph.h"
#include "pango-engine-private.h"

/**
 * pango_shape:
 * @text:      the text to process
 * @length:    the length (in bytes) of @text
 * @analysis:  #PangoAnalysis structure from pango_itemize()
 * @glyphs:    glyph string in which to store results
 *
 * Given a segment of text and the corresponding 
 * #PangoAnalysis structure returned from pango_itemize(),
 * convert the characters into glyphs. You may also pass
 * in only a substring of the item from pango_itemize().
 */
void
pango_shape (const gchar      *text, 
             gint              length, 
             const PangoAnalysis *analysis,
             PangoGlyphString *glyphs)
{
  int i;
  int last_cluster = -1;

  if (G_LIKELY (PANGO_IS_ENGINE_SHAPE (analysis->shape_engine) && PANGO_IS_FONT (analysis->font)))
    {
      _pango_engine_shape_shape (analysis->shape_engine, analysis->font,
				 text, length, analysis, glyphs);

      if (G_UNLIKELY (glyphs->num_glyphs == 0))
        {
	  /* If a font has been correctly chosen, but no glyphs are output,
	   * there's probably something wrong with the shaper.  Trying to be
	   * informative, we print out the font description, but to not 
	   * flood the terminal with zillions of the message, we set a flag
	   * on the font to only err once per font.
	   */
	  static GQuark warned_quark = 0;

	  if (!warned_quark)
	    warned_quark = g_quark_from_static_string ("pango-shaper-warned");
	  
	  if (!g_object_get_qdata (G_OBJECT (analysis->font), warned_quark))
	    {
	      PangoFontDescription *desc;
	      char *s;

	      desc = pango_font_describe (analysis->font);
	      s = pango_font_description_to_string (desc);
	      pango_font_description_free (desc);

	      /* TODO: Write out the beginning excerpt of text here? */
	      g_warning ("shape engine failure, expect ugly output. the offending font is '%s'", s);

	      g_free (s);

	      g_object_set_qdata_full (G_OBJECT (analysis->font), warned_quark,
				       GINT_TO_POINTER (1), NULL);
	    }
	}
    }
  else
    {
      if (!PANGO_IS_ENGINE_SHAPE (analysis->shape_engine) &&
	  !_pango_warning_history.shape_shape_engine)
        {
	  _pango_warning_history.shape_shape_engine = TRUE;
	  g_warning ("pango_shape called with bad shape_engine, expect ugly output");
	}
      if (!PANGO_IS_FONT (analysis->font) &&
	  !_pango_warning_history.shape_font)
        {
	  _pango_warning_history.shape_font = TRUE;
	  g_warning ("pango_shape called with bad font, expect ugly output");
	}

      glyphs->num_glyphs = 0;
    }

  if (!glyphs->num_glyphs)
    {
      PangoEngineShape *fallback_engine = _pango_get_fallback_shaper ();

      _pango_engine_shape_shape (fallback_engine, analysis->font,
				 text, length, analysis, glyphs);
    }

  /* Set glyphs[i].attr.is_cluster_start based on log_clusters[]
   */
  for (i = 0; i < glyphs->num_glyphs; i++)
    {
      if (glyphs->log_clusters[i] != last_cluster)
	{
	  glyphs->glyphs[i].attr.is_cluster_start = TRUE;
	  last_cluster = glyphs->log_clusters[i];
	}
      else
	glyphs->glyphs[i].attr.is_cluster_start = FALSE;
    }
}
