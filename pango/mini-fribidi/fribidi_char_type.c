/* FriBidi - Library of BiDi algorithm
 * Copyright (C) 2001,2002 Behdad Esfahbod. 
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
 * along with this library, in a file named COPYING; if not, write to the 
 * Free Software Foundation, Inc., 59 Temple Place, Suite 330, 
 * Boston, MA 02111-1307, USA  
 * 
 * For licensing issues, contact <fwpg@sharif.edu>. 
 */

#include <glib.h>
#include "fribidi_types.h"

/*======================================================================
 *  _pango_fribidi_get_type() returns the bidi type of a character.
 *----------------------------------------------------------------------*/
FriBidiCharType _pango_fribidi_get_type_internal (FriBidiChar uch);

FriBidiCharType
_pango_fribidi_get_type (FriBidiChar uch)
{
  return _pango_fribidi_get_type_internal (uch);
}

#include "fribidi_tab_char_type_2.i"
