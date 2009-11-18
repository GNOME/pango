/* Pango
 * pango-ot-buffer.c: Buffer of glyphs for shaping/positioning
 *
 * Copyright (C) 2004 Red Hat Software
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

#include "config.h"

#include "pango-ot-private.h"
#include "pangofc-private.h"
#include "pango-impl-utils.h"

/* cache a single hb_buffer_t */
static hb_buffer_t *cached_buffer = NULL;
G_LOCK_DEFINE_STATIC (cached_buffer);

static hb_buffer_t *
acquire_buffer (gboolean *free_buffer)
{
  hb_buffer_t *buffer;

  if (G_LIKELY (G_TRYLOCK (cached_buffer)))
    {
      if (G_UNLIKELY (!cached_buffer))
	cached_buffer = hb_buffer_create (64);

      buffer = cached_buffer;
      *free_buffer = FALSE;
    }
  else
    {
      buffer = hb_buffer_create (32);
      *free_buffer = TRUE;
    }

  return buffer;
}

static void
release_buffer (hb_buffer_t *buffer, gboolean free_buffer)
{
  if (G_LIKELY (!free_buffer) && hb_buffer_get_reference_count (buffer) == 1)
    {
      hb_buffer_clear (buffer);
      G_UNLOCK (cached_buffer);
    }
  else
    hb_buffer_destroy (buffer);
}

/**
 * pango_ot_buffer_new
 * @font: a #PangoFcFont
 *
 * Creates a new #PangoOTBuffer for the given OpenType font.
 *
 * Return value: the newly allocated #PangoOTBuffer, which should
 *               be freed with pango_ot_buffer_destroy().
 *
 * Since: 1.4
 **/
PangoOTBuffer *
pango_ot_buffer_new (PangoFcFont *font)
{
  PangoOTBuffer *buffer = g_slice_new (PangoOTBuffer);

  buffer->buffer = acquire_buffer (&buffer->should_free_hb_buffer);
  buffer->font = g_object_ref (font);
  buffer->applied_gpos = FALSE;
  buffer->rtl = FALSE;
  buffer->zero_width_marks = FALSE;

  return buffer;
}

/**
 * pango_ot_buffer_destroy
 * @buffer: a #PangoOTBuffer
 *
 * Destroys a #PangoOTBuffer and free all associated memory.
 *
 * Since: 1.4
 **/
void
pango_ot_buffer_destroy (PangoOTBuffer *buffer)
{
  release_buffer (buffer->buffer, buffer->should_free_hb_buffer);
  g_object_unref (buffer->font);
  g_slice_free (PangoOTBuffer, buffer);
}

/**
 * pango_ot_buffer_clear
 * @buffer: a #PangoOTBuffer
 *
 * Empties a #PangoOTBuffer, make it ready to add glyphs to.
 *
 * Since: 1.4
 **/
void
pango_ot_buffer_clear (PangoOTBuffer *buffer)
{
  hb_buffer_clear (buffer->buffer);
  buffer->applied_gpos = FALSE;
}

/**
 * pango_ot_buffer_add_glyph
 * @buffer: a #PangoOTBuffer
 * @glyph: the glyph index to add, like a #PangoGlyph
 * @properties: the glyph properties
 * @cluster: the cluster that this glyph belongs to
 *
 * Appends a glyph to a #PangoOTBuffer, with @properties identifying which
 * features should be applied on this glyph.  See pango_ruleset_add_feature().
 *
 * Since: 1.4
 **/
void
pango_ot_buffer_add_glyph (PangoOTBuffer *buffer,
			   guint          glyph,
			   guint          properties,
			   guint          cluster)
{
  hb_buffer_add_glyph (buffer->buffer,
			glyph, properties, cluster);
}

/**
 * pango_ot_buffer_set_rtl
 * @buffer: a #PangoOTBuffer
 * @rtl: %TRUE for right-to-left text
 *
 * Sets whether glyphs will be rendered right-to-left.  This setting
 * is needed for proper horizontal positioning of right-to-left scripts.
 *
 * Since: 1.4
 **/
void
pango_ot_buffer_set_rtl (PangoOTBuffer *buffer,
			 gboolean       rtl)
{
  buffer->rtl = rtl != FALSE;
  hb_buffer_set_direction (buffer->buffer,
			   buffer->rtl ? HB_DIRECTION_RTL : HB_DIRECTION_LTR);
}

/**
 * pango_ot_buffer_set_zero_width_marks
 * @buffer: a #PangoOTBuffer
 * @zero_width_marks: %TRUE if characters with a mark class should
 *  be forced to zero width.
 *
 * Sets whether characters with a mark class should be forced to zero width.
 * This setting is needed for proper positioning of Arabic accents,
 * but will produce incorrect results with standard OpenType Indic
 * fonts.
 *
 * Since: 1.6
 **/
void
pango_ot_buffer_set_zero_width_marks (PangoOTBuffer     *buffer,
				      gboolean           zero_width_marks)
{
  buffer->zero_width_marks = zero_width_marks != FALSE;
}

/**
 * pango_ot_buffer_get_glyphs
 * @buffer: a #PangoOTBuffer
 * @glyphs: location to store the array of glyphs, or %NULL
 * @n_glyphs: location to store the number of glyphs, or %NULL
 *
 * Gets the glyph array contained in a #PangoOTBuffer.  The glyphs are
 * owned by the buffer and should not be freed, and are only valid as long
 * as buffer is not modified.
 *
 * Since: 1.4
 **/
void
pango_ot_buffer_get_glyphs (const PangoOTBuffer  *buffer,
			    PangoOTGlyph        **glyphs,
			    int                  *n_glyphs)
{
  if (glyphs)
    *glyphs = (PangoOTGlyph *) hb_buffer_get_glyph_infos (buffer->buffer);

  if (n_glyphs)
    *n_glyphs = hb_buffer_get_len (buffer->buffer);
}

static void
apply_gpos_ltr (PangoGlyphString    *glyphs,
		hb_glyph_position_t *positions,
		gboolean             scale,
		double               xscale,
		double               yscale,
		gboolean             is_hinted)
{
  int i;

  for (i = 0; i < glyphs->num_glyphs; i++)
    {
      FT_Pos x_pos = positions[i].x_pos;
      FT_Pos y_pos = positions[i].y_pos;
      int back = i;
      int j;
      int adjustment;


      adjustment = PANGO_UNITS_26_6(positions[i].x_advance);

      if (is_hinted)
	adjustment = PANGO_UNITS_ROUND (adjustment);
      if (G_UNLIKELY (scale))
	adjustment *= xscale;

      if (positions[i].new_advance)
	glyphs->glyphs[i].geometry.width  = adjustment;
      else
	glyphs->glyphs[i].geometry.width += adjustment;


      while (positions[back].back != 0)
	{
	  back  -= positions[back].back;
	  x_pos += positions[back].x_pos;
	  y_pos += positions[back].y_pos;
	}

      for (j = back; j < i; j++)
	glyphs->glyphs[i].geometry.x_offset -= glyphs->glyphs[j].geometry.width;

      if (G_UNLIKELY (scale))
        {
	  glyphs->glyphs[i].geometry.x_offset += xscale * PANGO_UNITS_26_6(x_pos);
	  glyphs->glyphs[i].geometry.y_offset -= yscale * PANGO_UNITS_26_6(y_pos);
	}
      else
        {
	  glyphs->glyphs[i].geometry.x_offset += PANGO_UNITS_26_6(x_pos);
	  glyphs->glyphs[i].geometry.y_offset -= PANGO_UNITS_26_6(y_pos);
	}
    }
}

static void
apply_gpos_rtl (PangoGlyphString    *glyphs,
		hb_glyph_position_t *positions,
		gboolean             scale,
		double               xscale,
		double               yscale,
		gboolean             is_hinted)
{
  int i;

  for (i = 0; i < glyphs->num_glyphs; i++)
    {
      int i_rev = glyphs->num_glyphs - i - 1;
      int back_rev = i_rev;
      int back;
      FT_Pos x_pos = positions[i_rev].x_pos;
      FT_Pos y_pos = positions[i_rev].y_pos;
      int j;
      int adjustment;


      adjustment = PANGO_UNITS_26_6(positions[i_rev].x_advance);

      if (is_hinted)
	adjustment = PANGO_UNITS_ROUND (adjustment);
      if (G_UNLIKELY (scale))
	adjustment *= xscale;

      if (positions[i_rev].new_advance)
	glyphs->glyphs[i].geometry.width  = adjustment;
      else
	glyphs->glyphs[i].geometry.width += adjustment;


      while (positions[back_rev].back != 0)
	{
	  back_rev -= positions[back_rev].back;
	  x_pos += positions[back_rev].x_pos;
	  y_pos += positions[back_rev].y_pos;
	}

      back = glyphs->num_glyphs - back_rev - 1;

      for (j = i; j < back; j++)
	glyphs->glyphs[i].geometry.x_offset += glyphs->glyphs[j].geometry.width;

      if (G_UNLIKELY (scale))
        {
	  glyphs->glyphs[i].geometry.x_offset += xscale * PANGO_UNITS_26_6(x_pos);
	  glyphs->glyphs[i].geometry.y_offset -= yscale * PANGO_UNITS_26_6(y_pos);
	}
      else
        {
	  glyphs->glyphs[i].geometry.x_offset += PANGO_UNITS_26_6(x_pos);
	  glyphs->glyphs[i].geometry.y_offset -= PANGO_UNITS_26_6(y_pos);
	}
    }
}

/**
 * pango_ot_buffer_output
 * @buffer: a #PangoOTBuffer
 * @glyphs: a #PangoGlyphString
 *
 * Exports the glyphs in a #PangoOTBuffer into a #PangoGlyphString.  This is
 * typically used after the OpenType layout processing is over, to convert the
 * resulting glyphs into a generic Pango glyph string.
 *
 * Since: 1.4
 **/
void
pango_ot_buffer_output (const PangoOTBuffer *buffer,
			PangoGlyphString    *glyphs)
{
  FT_Face face;
  hb_face_t *hb_face;
  unsigned int i;
  int last_cluster;

  unsigned int len;
  PangoOTGlyph *otglyphs;
  hb_glyph_position_t *positions;

  face = pango_fc_font_lock_face (buffer->font);
  g_assert (face);

  pango_ot_buffer_get_glyphs (buffer, &otglyphs, (int *) &len);

  /* Copy glyphs into output glyph string */
  pango_glyph_string_set_size (glyphs, len);

  last_cluster = -1;
  for (i = 0; i < len; i++)
    {
      PangoOTGlyph *otglyph = &otglyphs[i];

      glyphs->glyphs[i].glyph = otglyph->glyph;

      glyphs->log_clusters[i] = otglyph->cluster;
      if (glyphs->log_clusters[i] != last_cluster)
	glyphs->glyphs[i].attr.is_cluster_start = 1;
      else
	glyphs->glyphs[i].attr.is_cluster_start = 0;

      last_cluster = glyphs->log_clusters[i];
    }

  hb_face = _pango_ot_info_get_hb_face (pango_ot_info_get (face));

  /* Apply default positioning */
  for (i = 0; i < (unsigned int)glyphs->num_glyphs; i++)
    {
      if (glyphs->glyphs[i].glyph)
	{
	  PangoRectangle logical_rect;

	  if (buffer->zero_width_marks &&
	      hb_ot_layout_get_glyph_class (hb_face, glyphs->glyphs[i].glyph) == HB_OT_LAYOUT_GLYPH_CLASS_MARK)
	    {
	      glyphs->glyphs[i].geometry.width = 0;
	    }
	  else
	    {
	      pango_font_get_glyph_extents ((PangoFont *)buffer->font, glyphs->glyphs[i].glyph, NULL, &logical_rect);
	      glyphs->glyphs[i].geometry.width = logical_rect.width;
	    }
	}
      else
	glyphs->glyphs[i].geometry.width = 0;

      glyphs->glyphs[i].geometry.x_offset = 0;
      glyphs->glyphs[i].geometry.y_offset = 0;
    }

  if (buffer->rtl)
    {
      /* Swap all glyphs */
      pango_glyph_string_reverse_range (glyphs, 0, glyphs->num_glyphs);
    }

  positions = hb_buffer_get_glyph_positions (buffer->buffer);
  if (buffer->applied_gpos)
    {
      gboolean scale = FALSE;
      double xscale = 1, yscale = 1;
      PangoFcFontKey *key = _pango_fc_font_get_font_key (buffer->font);

      /* This is a kludge, and dupped in pango_fc_font_kern_glyphs().
       * Should move the scale factor to PangoFcFont layer. */
      if (key) {
	const PangoMatrix *matrix = pango_fc_font_key_get_matrix (key);
	PangoMatrix identity = PANGO_MATRIX_INIT;
	if (G_UNLIKELY (matrix && 0 != memcmp (&identity, matrix, 4 * sizeof (double))))
	  {
	    scale = TRUE;
	    pango_matrix_get_font_scale_factors (matrix, &xscale, &yscale);
	    if (xscale) xscale = 1 / xscale;
	    if (yscale) yscale = 1 / yscale;
	  }
      }

      if (buffer->rtl)
	apply_gpos_rtl (glyphs, positions, scale, xscale, yscale, buffer->font->is_hinted);
      else
	apply_gpos_ltr (glyphs, positions, scale, xscale, yscale, buffer->font->is_hinted);
    }
  else
    {
      /* FIXME we should only do this if the 'kern' feature was requested */
      pango_fc_font_kern_glyphs (buffer->font, glyphs);
    }

  pango_fc_font_unlock_face (buffer->font);
}
