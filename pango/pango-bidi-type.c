/* Pango
 * pango-bidi-type.c: Bidirectional Character Types
 *
 * Copyright (C) 2008 Jürg Billeter <j@bitron.ch>
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

#include <string.h>

#include "pango-bidi-type.h"

#include "mini-fribidi/fribidi.h"



/**
 * pango_bidi_type_for_unichar
 * @ch: a Unicode character
 *
 * Determines the normative bidirectional character type of a
 * character, as specified in the Unicode Character Database.
 *
 * A simplified version of this function is available as
 * pango_unichar_get_direction().
 *
 * Return value: the bidirectional character type, as used in the
 * Unicode bidirectional algorithm.
 *
 * Since: 1.22
 */
PangoBidiType
pango_bidi_type_for_unichar (gunichar ch)
{
  FriBidiCharType fribidi_ch_type = fribidi_get_type (ch);

  switch (fribidi_ch_type)
    {
    case FRIBIDI_TYPE_LTR:  return PANGO_BIDI_TYPE_L;
    case FRIBIDI_TYPE_LRE:  return PANGO_BIDI_TYPE_LRE;
    case FRIBIDI_TYPE_LRO:  return PANGO_BIDI_TYPE_LRO;
    case FRIBIDI_TYPE_RTL:  return PANGO_BIDI_TYPE_R;
    case FRIBIDI_TYPE_AL:   return PANGO_BIDI_TYPE_AL;
    case FRIBIDI_TYPE_RLE:  return PANGO_BIDI_TYPE_RLE;
    case FRIBIDI_TYPE_RLO:  return PANGO_BIDI_TYPE_RLO;
    case FRIBIDI_TYPE_PDF:  return PANGO_BIDI_TYPE_PDF;
    case FRIBIDI_TYPE_EN:   return PANGO_BIDI_TYPE_EN;
    case FRIBIDI_TYPE_ES:   return PANGO_BIDI_TYPE_ES;
    case FRIBIDI_TYPE_ET:   return PANGO_BIDI_TYPE_ET;
    case FRIBIDI_TYPE_AN:   return PANGO_BIDI_TYPE_AN;
    case FRIBIDI_TYPE_CS:   return PANGO_BIDI_TYPE_CS;
    case FRIBIDI_TYPE_NSM:  return PANGO_BIDI_TYPE_NSM;
    case FRIBIDI_TYPE_BN:   return PANGO_BIDI_TYPE_BN;
    case FRIBIDI_TYPE_BS:   return PANGO_BIDI_TYPE_B;
    case FRIBIDI_TYPE_SS:   return PANGO_BIDI_TYPE_S;
    case FRIBIDI_TYPE_WS:   return PANGO_BIDI_TYPE_WS;
    case FRIBIDI_TYPE_ON:   return PANGO_BIDI_TYPE_ON;
    default:
      g_assert_not_reached ();
      return PANGO_BIDI_TYPE_ON;
    }
}

/* Some bidi-related functions */

/**
 * pango_log2vis_get_embedding_levels:
 * @text:      the text to itemize.
 * @length:    the number of bytes (not characters) to process, or -1
 *             if @text is nul-terminated and the length should be calculated.
 * @pbase_dir: input base direction, and output resolved direction.
 *
 * This will return the bidirectional embedding levels of the input paragraph
 * as defined by the Unicode Bidirectional Algorithm available at:
 *
 *   http://www.unicode.org/reports/tr9/
 *
 * If the input base direction is a weak direction, the direction of the
 * characters in the text will determine the final resolved direction.
 *
 * Return value: a newly allocated array of embedding levels, one item per
 *               character (not byte), that should be freed using g_free.
 *
 * Since: 1.4
 */
guint8 *
pango_log2vis_get_embedding_levels (const gchar    *text,
				    int             length,
				    PangoDirection *pbase_dir)
{
  FriBidiCharType fribidi_base_dir;
  guint8 *embedding_levels_list;

  switch (*pbase_dir)
    {
    case PANGO_DIRECTION_LTR:
    case PANGO_DIRECTION_TTB_RTL:
      fribidi_base_dir = FRIBIDI_TYPE_L;
      break;
    case PANGO_DIRECTION_RTL:
    case PANGO_DIRECTION_TTB_LTR:
      fribidi_base_dir = FRIBIDI_TYPE_R;
      break;
    case PANGO_DIRECTION_WEAK_RTL:
      fribidi_base_dir = FRIBIDI_TYPE_WR;
      break;
    case PANGO_DIRECTION_WEAK_LTR:
    case PANGO_DIRECTION_NEUTRAL:
    default:
      fribidi_base_dir = FRIBIDI_TYPE_WL;
      break;
    }

#ifdef FRIBIDI_HAVE_UTF8
  {
    if (length < 0)
      length = strlen (text);
    embedding_levels_list = fribidi_log2vis_get_embedding_levels_new_utf8 (text, length, &fribidi_base_dir);
  }
#else
  {
    gunichar *text_ucs4;
    int n_chars;
    text_ucs4 = g_utf8_to_ucs4_fast (text, length, &n_chars);
    embedding_levels_list = g_new (guint8, n_chars);
    fribidi_log2vis_get_embedding_levels ((FriBidiChar*)text_ucs4, n_chars,
					  &fribidi_base_dir,
					  (FriBidiLevel*)embedding_levels_list);
    g_free (text_ucs4);
  }
#endif

  *pbase_dir = (fribidi_base_dir == FRIBIDI_TYPE_L) ?  PANGO_DIRECTION_LTR : PANGO_DIRECTION_RTL;

  return embedding_levels_list;
}

/**
 * pango_unichar_direction:
 * @ch: a Unicode character
 *
 * Determines the inherent direction of a character; either
 * %PANGO_DIRECTION_LTR, %PANGO_DIRECTION_RTL, or
 * %PANGO_DIRECTION_NEUTRAL.
 *
 * This function is useful to categorize characters into left-to-right
 * letters, right-to-left letters, and everything else.  If full
 * Unicode bidirectional type of a character is needed,
 * pango_bidi_type_for_gunichar() can be used instead.
 *
 * Return value: the direction of the character.
 */
PangoDirection
pango_unichar_direction (gunichar ch)
{
  FriBidiCharType fribidi_ch_type = fribidi_get_type (ch);

  if (!FRIBIDI_IS_STRONG (fribidi_ch_type))
    return PANGO_DIRECTION_NEUTRAL;
  else if (FRIBIDI_IS_RTL (fribidi_ch_type))
    return PANGO_DIRECTION_RTL;
  else
    return PANGO_DIRECTION_LTR;
}

/**
 * pango_get_mirror_char:
 * @ch: a Unicode character
 * @mirrored_ch: location to store the mirrored character
 *
 * If @ch has the Unicode mirrored property and there is another Unicode
 * character that typically has a glyph that is the mirror image of @ch's
 * glyph, puts that character in the address pointed to by @mirrored_ch.
 *
 * Use g_unichar_get_mirror_char() instead; the docs for that function
 * provide full details.
 *
 * Return value: %TRUE if @ch has a mirrored character and @mirrored_ch is
 * filled in, %FALSE otherwise
 **/
gboolean
pango_get_mirror_char (gunichar        ch,
		       gunichar       *mirrored_ch)
{
  return g_unichar_get_mirror_char (ch, mirrored_ch);
}

