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

#include <config.h>

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
