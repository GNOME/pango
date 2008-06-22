/* Pango
 * syriac-ot.h: Determine what OpenType features to apply to characters based
 *              on the rules for Syriac from the OpenType standard.
 *
 * Copyright (C) 2004 Emil Soleyman-Zomalan
 * Author: Emil Soleyman-Zomalan <emil@soleyman.com>
 *
 * This file is based on the Arabic shaping code from FreeType 1 tree; original
 * copyright notice:
 *
 *  The FreeType project -- a free and portable quality TrueType renderer.
 *
 *  Copyright 1996-2000 by
 *  D. Turner, R.Wilhelm, and W. Lemberg
 *
 * The code, like the FreeType code it is derived from is dual-licensed
 * under the GNU General Public License and the FreeType license. See
 * pango/opentype/COPYING for full details of this licensing scheme.
 */
#include "config.h"
#include "syriac-ot.h"

/*     Here a table of the joining classes for characters in the range
 *     U+0700 - U+074F.
 *
 *     The following character also has a joining class:
 *
 *     U+200C  ZERO WIDTH NON-JOINER  -> causing
 *
 *     All other characters are given the joining class `none'.
 */
static const JoiningClass syriac[] =
{
  /* U+0700 */
  none, none, none, none,
  none, none, none, none,
  none, none, none, none,
  none, none, none, transparent,

  /* U+0710 */
  right, none, dual, dual,
  dual, right, right, right,
  right, right, dual, dual,
  dual, dual, right, dual,

  /* U+0720 */
  dual, dual, dual, dual,
  dual, dual, dual, dual,
  right, dual, right, dual,
  right, none, none, none,

  /* U+0730 */
  transparent, transparent, transparent, transparent,
  transparent, transparent, transparent, transparent,
  transparent, transparent, transparent, transparent,
  transparent, transparent, transparent, transparent,

  /* U+0740 */
  transparent, transparent, transparent, transparent,
  transparent, transparent, transparent, transparent,
  transparent, transparent, transparent, none,
  none, right, dual, dual
};

/* `direction' can be -1, 0, or 1 to indicate the last non-transparent
 * glyph, the current glyph, and the next non-transparent glyph,
 * respectively.
 */
static JoiningClass
Get_Joining_Class (gunichar*   string,
		   int         pos,
		   int         length,
		   int         direction)
{
  JoiningClass  j;

  while (1)
    {
      if (pos == 0 && direction < 0)
	return none;

      pos += direction;

      if (pos >= length)
	return none;

      if (string[pos] < 0x0700 ||
	  string[pos] >= 0x074F)
	{
	  if (string[pos] == 0x200C)
	    return causing;
	  else
	    return none;
	}
      else
	j =  syriac[string[pos] - 0x0700];

      if (!direction || j != transparent)
	return j;
    }
}


/* The rules here are roughly based on the Arabic rules from the Unicode
 * 2.0 standard (which differ from the Unicode-4.0 rules), augmented
 * with the Syriac rules from the Unicode-4.0 standard. The numbers
 * R1...R11  below do not correspond to either the Arabic or the Syriac
 * rule numbering from the Unicode standard.
 *
 * Characters are here specified as appearing in the byte stream, i.e.
 * *not* in visual order. Joining classes are given in angle brackets,
 * glyph forms in square brackets.  Glyphs affected by a specific rule are
 * enclosed with vertical bars.
 *
 *
 * Glyphs: 0x0715 (Dalath), 0x0716 (Dalath Rish), 0x072A (Rish),
 *         0x0722 (Nun), 0x071F (Kaph)
 *
 *
 *   R1: <anything1> <transparent> <anything2>
 *
 *       apply joining rules for
 *       <anything1> <anything2> -> [shape1] [shape2]
 *       -> [shape1] [isolated] [shape2]
 *
 *
 *   R2: <causing|right> <0x0722|0x071F> <!(causing|right|dual)>
 *       -> [isolated]
 *
 *       The Nun and Kaph characters each have 3 different glyphs
 *       with two of those glyphs coming at the final position.
 *       However, one of those final glyphs should really be of the
 *       isolated glyph form where the preceding character cannot be
 *       joined to and there is no next character.
 *
 *       This rule exists to combine similar exception for both
 *       characters without increasing the complexity in the other
 *       rules.
 *
 *
 *   R3: <causing|right|dual> && <!(0x0715|0x0716|0x072A)> |<alaph>|
 *
 *	 -> [final2]
 *
 *	 If the preceding glyph cannot be joined to the current
 *	 glyph and the preceding character is not a Dalath, Rish,
 *	 or Dotless Dalath Rish, then the Alaph takes this contextual
 *	 position.
 *
 *	 The [final2] joining rule is placed ahead of the [final] to
 *	 give it greater precedence when choosing the correct glyph.
 *	 If it comes after the [final] rule, the incorrect glyph is
 *	 inserted into position.
 *
 *
 *   R4: <0x0715|0x0715|0x072A> |<alaph>|
 *
 *	 -> [final3]
 *
 *	 If the previous glyph is a Dalath, Rish, or Dotless Dalath
 *	 Rish, then the Alaph takes this contextual position.
 *
 *	 The [final3] joining rule is placed ahead of the [final] to
 *	 give it greater precedence when choosing the correct glyph.
 *	 If it comes after the [final] rule, the incorrect glyph is
 *	 inserted into position.
 *
 *
 *   R5: <causing|right|dual> |<right>|
 *
 *	 -> [final]
 *
 *
 *   R6: <causing|right|dual> |<dual>| <!(causing|right|dual)>
 *
 *	 -> [final]
 *
 *
 *   R7: <causing|left|dual> |<dual>| <causing|right|dual>
 *
 *	 -> [medial]
 *
 *
 *   R8: <causing|right> |<alaph>| <causing|right|dual>
 *
 *	 -> [medial2]
 *
 *       If the Alaph glyph falls in the middle of a Syriac word and
 *       the preceding character cannot be joined to, then the Alaph
 *       takes this contextual position.
 *
 *
 *   R9: |<left>| <causing|right|dual>
 *
 *	 -> [initial]
 *
 *
 *   R10: <!(causing|left|dual)> |<dual>| <causing|right|dual>
 *
 *       -> [initial]
 *
 *
 *   R11: <anything> -> [isolated]
 *
 *        This joining rule is placed at the end of these features
 *        because when it is placed at the beginning of all of them
 *        it tends to break the cursive nature of Syriac writing --
 *        it inserts the isolated glyph of each character into that
 *        position with no joining occurring all throughout a text
 *        document.
 */

FT_Error
syriac_assign_properties (gunichar    *string,
			  gulong      *properties,
			  int          length)
{
  JoiningClass previous, current, next;
  int i;

  if (!string || !properties || length == 0)
    return FT_Err_Invalid_Argument;

  for (i = 0; i < length; i++)
    {
      previous = Get_Joining_Class (string, i, length, -1);
      current  = Get_Joining_Class (string, i, length,  0);
      next     = Get_Joining_Class (string, i, length,  1);

      /* R1 */

      if (current == transparent)
	{
	  properties[i] |= isolated_p;
	  continue;
	}

      /* R2 */

      if (string[i] == 0x0722 ||
	  string[i] == 0x071F)
	if (previous == causing ||
	    previous == right)
	  if (!(next == causing ||
		next == right ||
		next == dual))
	  {
	    properties[i] |= isolated_p;
	    continue;
	  }

      /* R3 */

      if (string[i] == 0x0710)
	if (previous == causing ||
	    previous == right)
	  if (!(string[i - 1] == 0x0715 ||
		string[i - 1] == 0x0716 ||
		string[i - 1] == 0x072A))
	  {
	    properties[i] |= final2_p;
	    continue;
	  }

      /* R4 */

      if (string[i] == 0x0710)
	if (previous == causing ||
	    previous == right)
	 if (string[i - 1] == 0x0715 ||
	      string[i - 1] == 0x0716 ||
	      string[i - 1] == 0x072A)
	  {
	    properties[i] |= final3_p;
	    continue;
	  }

      /* R5 */

      if (previous == causing ||
	  previous == right   ||
	  previous == dual)
	if (current == right)
	    {
	      properties[i] |= final_p;
	      continue;
	    }

      /* R6 */

      if (previous == causing ||
	  previous == right   ||
	  previous == dual)
	if (current == dual)
	  if (!(next == causing ||
		    next == right   ||
		    next == dual   ))
	      {
		properties[i] |= final_p;
		continue;
	      }

      /* R7 */

      if (previous == causing ||
	  previous == left    ||
	  previous == dual)
	if (current == dual)
	  if (next == causing ||
	      next == right   ||
	      next == dual   )
	      {
		properties[i] |= medial_p;
		continue;
	      }

      /* R8 */

      if (string[i] == 0x0710)
	if (previous == causing ||
	    previous == right)
	    if (next == causing ||
		next == right ||
		next == dual)
	    {
	      properties[i] |= medial2_p;
	      continue;
	    }

      /* R9 */

      if (current == left)
	if (next == causing ||
	    next == right   ||
	    next == dual)
	    {
	      properties[i] |= initial_p;
	      continue;
	    }

      /* R10 */

      if (!(previous == causing ||
	    previous == left ||
	    previous == dual   ))
	if (current == dual)
	  if (next == causing ||
	      next == right   ||
	      next == dual)
	      {
		properties[i] |= initial_p;
		continue;
	      }

      /* R11 */

      properties[i] |= isolated_p;
    }

  return FT_Err_Ok;
}
