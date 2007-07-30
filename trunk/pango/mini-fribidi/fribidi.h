/* FriBidi - Library of BiDi algorithm
 * Copyright (C) 1999,2000 Dov Grobgeld, and
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
 * For licensing issues, contact <dov@imagic.weizmann.ac.il> and 
 * <fwpg@sharif.edu>. 
 */

#ifndef FRIBIDI_H
#define FRIBIDI_H

#ifndef NULL
#define NULL 0
#endif

#include "fribidi_config.h"
#include "fribidi_types.h"

#ifdef __cplusplus
extern "C"
{
#endif

#define FRIBIDI_HAVE_UTF8

  FRIBIDI_API FriBidiLevel *fribidi_log2vis_get_embedding_levels_new_utf8 (	/* input */
								     const char *str,
								     int bytelen,
								     FriBidiCharType
								     *pbase_dir);

/*======================================================================
 *  fribidi_get_type() returns bidi type of a character.
 *----------------------------------------------------------------------*/
  FRIBIDI_API FriBidiCharType fribidi_get_type (FriBidiChar uch);

#ifdef	__cplusplus
}
#endif

#endif				/* FRIBIDI_H */
