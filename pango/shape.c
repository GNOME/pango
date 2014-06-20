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

/**
 * SECTION:glyphs
 * @short_description:Structures for storing information about glyphs
 * @title:Glyph Storage
 *
 * pango_shape() produces a string of glyphs which
 * can be measured or drawn to the screen. The following
 * structures are used to store information about
 * glyphs.
 */
#include "config.h"

#include "pango-impl-utils.h"
#include "pango-glyph.h"
#include "pango-engine-private.h"

#include <string.h>

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
 *
 * It is recommended that you use pango_shape_full() instead, since
 * that API allows for shaping interaction happening across text item
 * boundaries.
 */
void
pango_shape (const gchar      *text,
	     gint              length,
	     const PangoAnalysis *analysis,
	     PangoGlyphString *glyphs)
{
  pango_shape_full (text, length, text, length, analysis, glyphs);
}

/**
 * pango_shape_full:
 * @item_text:        valid UTF-8 text to shape.
 * @item_length:      the length (in bytes) of @item_text. -1 means nul-terminated text.
 * @paragraph_text: (allow-none): text of the paragraph (see details).  May be %NULL.
 * @paragraph_length: the length (in bytes) of @paragraph_text. -1 means nul-terminated text.
 * @analysis:  #PangoAnalysis structure from pango_itemize().
 * @glyphs:    glyph string in which to store results.
 *
 * Given a segment of text and the corresponding
 * #PangoAnalysis structure returned from pango_itemize(),
 * convert the characters into glyphs. You may also pass
 * in only a substring of the item from pango_itemize().
 *
 * This is similar to pango_shape(), except it also can optionally take
 * the full paragraph text as input, which will then be used to perform
 * certain cross-item shaping interactions.  If you have access to the broader
 * text of which @item_text is part of, provide the broader text as
 * @paragraph_text.  If @paragraph_text is %NULL, item text is used instead.
 *
 * Since: 1.32
 */
void
pango_shape_full (const gchar      *item_text,
		  gint              item_length,
		  const gchar      *paragraph_text,
		  gint              paragraph_length,
		  const PangoAnalysis *analysis,
		  PangoGlyphString *glyphs)
{
  int i;
  int last_cluster;

  glyphs->num_glyphs = 0;

  if (item_length == -1)
    item_length = strlen (item_text);

  if (!paragraph_text)
    {
      paragraph_text = item_text;
      paragraph_length = item_length;
    }
  if (paragraph_length == -1)
    paragraph_length = strlen (paragraph_text);

  g_return_if_fail (paragraph_text <= item_text);
  g_return_if_fail (paragraph_text + paragraph_length >= item_text + item_length);

  if (G_LIKELY (analysis->shape_engine && analysis->font))
    {
      _pango_engine_shape_shape (analysis->shape_engine, analysis->font,
				 item_text, item_length,
				 paragraph_text, paragraph_length,
				 analysis, glyphs);

      if (G_UNLIKELY (glyphs->num_glyphs == 0))
	{
	  /* If a font has been correctly chosen, but no glyphs are output,
	   * there's probably something wrong with the shaper, or the font.
	   *
	   * Trying to be informative, we print out the font description,
	   * shaper name, and the text, but to not flood the terminal with
	   * zillions of the message, we set a flag to only err once per
	   * font/engine pair.
	   *
	   * To do the flag fast, we use the engine qname to qflag the font,
	   * but also the font description to flag the engine.  This is
	   * supposed to be fast to check, but also avoid writing out
	   * duplicate warnings when a new PangoFont is created.
	   */
	  GType engine_type = G_OBJECT_TYPE (analysis->shape_engine);
	  GQuark warned_quark = g_type_qname (engine_type);

	  if (!g_object_get_qdata (G_OBJECT (analysis->font), warned_quark))
	    {
	      PangoFontDescription *desc;
	      char *font_name;
	      const char *engine_name;

	      desc = pango_font_describe (analysis->font);
	      font_name = pango_font_description_to_string (desc);
	      pango_font_description_free (desc);

	      if (!g_object_get_data (G_OBJECT (analysis->shape_engine), font_name))
	        {
		  engine_name = g_type_name (engine_type);
		  if (!engine_name)
		    engine_name = "(unknown)";

		  g_warning ("shaping failure, expect ugly output. shape-engine='%s', font='%s', text='%.*s'",
			     engine_name, font_name, item_length, item_text);

		  g_object_set_data_full (G_OBJECT (analysis->shape_engine), font_name,
					  GINT_TO_POINTER (1), NULL);
	        }

	      g_free (font_name);

	      g_object_set_qdata_full (G_OBJECT (analysis->font), warned_quark,
				       GINT_TO_POINTER (1), NULL);
	    }
	}
    }
  else
    glyphs->num_glyphs = 0;

  if (G_UNLIKELY (!glyphs->num_glyphs))
    {
      PangoEngineShape *fallback_engine = _pango_get_fallback_shaper ();

      _pango_engine_shape_shape (fallback_engine, analysis->font,
				 item_text, item_length,
				 paragraph_text, paragraph_length,
				 analysis, glyphs);
      if (G_UNLIKELY (!glyphs->num_glyphs))
        return;
    }

  /* make sure last_cluster is invalid */
  last_cluster = glyphs->log_clusters[0] - 1;
  for (i = 0; i < glyphs->num_glyphs; i++)
    {
      /* Set glyphs[i].attr.is_cluster_start based on log_clusters[] */
      if (glyphs->log_clusters[i] != last_cluster)
	{
	  glyphs->glyphs[i].attr.is_cluster_start = TRUE;
	  last_cluster = glyphs->log_clusters[i];
	}
      else
	glyphs->glyphs[i].attr.is_cluster_start = FALSE;


      /* Shift glyph if width is negative, and negate width.
       * This is useful for rotated font matrices and shouldn't
       * harm in normal cases.
       */
      if (glyphs->glyphs[i].geometry.width < 0)
	{
	  glyphs->glyphs[i].geometry.width = -glyphs->glyphs[i].geometry.width;
	  glyphs->glyphs[i].geometry.x_offset += glyphs->glyphs[i].geometry.width;
	}
    }

  /* Make sure glyphstring direction conforms to analysis->level */
  if (G_UNLIKELY ((analysis->level & 1) &&
		  glyphs->log_clusters[0] < glyphs->log_clusters[glyphs->num_glyphs - 1]))
    {
      /* Warn once per shaper */
      static GQuark warned_quark = 0; /* MT-safe */

      if (!warned_quark)
	warned_quark = g_quark_from_static_string ("pango-shape-warned");

      if (analysis->shape_engine && !g_object_get_qdata (G_OBJECT (analysis->shape_engine), warned_quark))
	{
	  GType engine_type = G_OBJECT_TYPE (analysis->shape_engine);
	  const char *engine_name = g_type_name (engine_type);
	  if (!engine_name)
	    engine_name = "(unknown)";

	  g_warning ("Expected RTL run but shape-engine='%s' returned LTR. Fixing.", engine_name);

	  g_object_set_qdata_full (G_OBJECT (analysis->shape_engine), warned_quark,
				   GINT_TO_POINTER (1), NULL);
	}

      /* *Fix* it so we don't crash later */
      pango_glyph_string_reverse_range (glyphs, 0, glyphs->num_glyphs);
    }
}
