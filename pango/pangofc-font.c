/* Pango
 * pangofc-font.c: Shared interfaces for fontconfig-based backends
 *
 * Copyright (C) 2003 Red Hat Software
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

#include "pangofc-font.h"

GType
pango_fc_font_get_type (void)
{
  static GType object_type = 0;

  if (!object_type)
    {
      static const GTypeInfo object_info =
      {
        sizeof (PangoFcFontClass),
        (GBaseInitFunc) NULL,
        (GBaseFinalizeFunc) NULL,
        NULL,           /* class_init */
        NULL,           /* class_finalize */
        NULL,           /* class_data */
        sizeof (PangoFcFont),
        0,              /* n_preallocs */
        NULL            /* init */
      };
      
      object_type = g_type_register_static (PANGO_TYPE_FONT,
                                            "PangoFcFont",
                                            &object_info, 0);
    }
  
  return object_type;
}
 
/**
 * pango_fc_font_lock_face:
 * @font: a #PangoFcFont.
 *
 * Gets the FreeType FT_Face associated with a font,
 * This face will be kept around until you call
 * pango_fc_font_unlock_face().
 *
 * Returns: the FreeType FT_Face associated with @font.
 *
 * Since: 1.4
 **/
FT_Face
pango_fc_font_lock_face (PangoFcFont *font)
{
  g_return_val_if_fail (PANGO_IS_FC_FONT (font), NULL);

  return PANGO_FC_FONT_GET_CLASS (font)->lock_face (font);
}

/**
 * pango_fc_font_unlock_face:
 * @font: a #PangoFcFont.
 *
 * Releases a font previously obtained with
 * pango_fc_font_lock_face().
 *
 * Since: 1.4
 **/
void
pango_fc_font_unlock_face (PangoFcFont *font)
{
  g_return_if_fail (PANGO_IS_FC_FONT (font));
  
  PANGO_FC_FONT_GET_CLASS (font)->unlock_face (font);
}

/**
 * pango_fc_font_has_char:
 * @font: a #PangoFcFont
 * @wc: Unicode codepoint to look up
 * 
 * Determines whether @font has a glyph for the codepoint @wc.
 * 
 * Return value: %TRUE if @font has the requested codepoint.
 *
 * Since: 1.4
 **/
gboolean
pango_fc_font_has_char (PangoFcFont *font,
			gunichar     wc)
{
  g_return_val_if_fail (PANGO_IS_FC_FONT (font), FALSE);

  return PANGO_FC_FONT_GET_CLASS (font)->has_char (font, wc);
}

/**
 * pango_fc_font_get_glyph:
 * @font: a #PangoFcFont
 * @wc: Unicode codepoint to look up
 * 
 * Gets the glyph index for a given unicode codepoint
 * for @font. If you only want to determine
 * whether the font has the glyph, use pango_fc_font_has_char().
 * 
 * Return value: the glyph index, or 0, if the unicode
 *  codepoint doesn't exist in the font.
 *
 * Since: 1.4
 **/
PangoGlyph
pango_fc_font_get_glyph (PangoFcFont *font,
			 gunichar     wc)
{
  g_return_val_if_fail (PANGO_IS_FC_FONT (font), 0);

  return PANGO_FC_FONT_GET_CLASS (font)->get_glyph (font, wc);
}


/**
 * pango_fc_font_get_unknown_glyph:
 * @font: a #PangoFcFont
 * @wc: the Unicode character for which a glyph is needed.
 *
 * Returns the index of a glyph suitable for drawing @wc as an
 * unknown character.
 *
 * Return value: a glyph index into @font.
 *
 * Since: 1.4
 **/
PangoGlyph
pango_fc_font_get_unknown_glyph (PangoFcFont *font,
				 gunichar     wc)
{
  g_return_val_if_fail (PANGO_IS_FC_FONT (font), 0);

  return PANGO_FC_FONT_GET_CLASS (font)->get_unknown_glyph (font, wc);
}

/**
 * pango_fc_font_get_kerning:
 * @font: a #PangoFcFont.
 * @left: the left #PangoGlyph
 * @right: the right #PangoGlyph
 * 
 * Retrieves kerning information for a combination of two glyphs.
 * 
 * Return value: The amount of kerning (in Pango units) to apply for 
 * the given combination of glyphs.
 *
 * Since: 1.4
 **/
int
pango_fc_font_get_kerning (PangoFcFont *font,
			   PangoGlyph   left,
			   PangoGlyph   right)
{
  g_return_val_if_fail (PANGO_IS_FC_FONT (font), 0);

  return PANGO_FC_FONT_GET_CLASS (font)->get_kerning (font, left, right);
}
