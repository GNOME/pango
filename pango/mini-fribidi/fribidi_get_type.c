/* FriBidi - Library of BiDi algorithm
 * Copyright (C) 1999,2000 Dov Grobgeld, and
 * Copyright (C) 2001 Behdad Esfahbod. 
 * 
 * This library is free software; you can redistribute it and/or 
 * modify it under the terms of the GNU Lesser General Public 
 * License as published by the Free Software Foundation; either 
 * version 2.1 of the License, or (at your option) any later version. 
 * 
 * This library is distributed in the hope that it will be useful, 
 * but WITHOUT ANY WARRANTY; without even the implied warranty of 
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU 
 * Lesser General Public License for more details. 
 * 
 * You should have received a copy of the GNU Lesser General Public License 
 * along with this library, in a file named COPYING.LIB; if not, write to the 
 * Free Software Foundation, Inc., 59 Temple Place, Suite 330, 
 * Boston, MA 02111-1307, USA  
 * 
 * For licensing issues, contact <dov@imagic.weizmann.ac.il> and 
 * <fwpg@sharif.edu>. 
 */

#include <stdio.h>
#include <stdlib.h>
#include <glib.h>
#include "pango/pango-utils.h"
#include "fribidi_types.h"
#include "fribidi_tables.i"

#ifdef MEM_OPTIMIZED
extern FriBidiCharType prop_to_type[];
#endif

/*======================================================================
 *  fribidi_get_type() returns the bidi type of a character.
 *----------------------------------------------------------------------*/
FriBidiCharType
_pango_fribidi_get_type(FriBidiChar uch)
{
  int i = uch % 256, j = uch / 256;
  FriBidiPropCharType *block = FriBidiPropertyBlocks[j];
  if (block)
#ifdef MEM_OPTIMIZED
    return prop_to_type[block[i]];
#else
    return block[i];
#endif
  else
    {
      switch (j)
	{
	case 0x05:
	  if (i >= 0x90)
	    return FRIBIDI_TYPE_RTL;
	  else
	    break;

	case 0xFB:
	  if (i >= 0x50)
	    return FRIBIDI_TYPE_AL;
	  else if (i >= 0x1D)
	    return FRIBIDI_TYPE_RTL;
	  else
	    break;
	case 0x06:
	case 0xFC:
	case 0xFD:
	  return FRIBIDI_TYPE_AL;
	case 0x07:
	  if (i <= 0xBF)
	    return FRIBIDI_TYPE_AL;
	  else
	    break;
	case 0xFE:
	  if (i >= 0x70)
	    return FRIBIDI_TYPE_AL;
	  else
	    break;
	}
      return FRIBIDI_TYPE_LTR;
    }
}

gboolean
pango_get_mirror_char (	/* Input */
		       FriBidiChar ch,
		       /* Output */
		       FriBidiChar * mirrored_ch)
{
  int pos, step;
  gboolean found;

  pos = step = (nFriBidiMirroredChars / 2) + 1;

  while (step > 1)
    {
      FriBidiChar cmp_ch = FriBidiMirroredChars[pos].ch;
      step = (step + 1) / 2;

      if (cmp_ch < ch)
	{
	  pos += step;
	  if (pos > nFriBidiMirroredChars - 1)
	    pos = nFriBidiMirroredChars - 1;
	}
      else if (cmp_ch > ch)
	{
	  pos -= step;
	  if (pos < 0)
	    pos = 0;
	}
      else
	break;
    }
  if (FriBidiMirroredChars[pos].ch == ch)
    {
      *mirrored_ch = FriBidiMirroredChars[pos].mirrored_ch;
      found = TRUE;
    }
  else
    {
      *mirrored_ch = ch;
      found = FALSE;
    }
  return found;
}
