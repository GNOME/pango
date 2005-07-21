/* Pango
 * basic-fc.c: Basic shaper for FreeType-based backends
 *
 * Copyright (C) 2002 Sven Neumann
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

#ifndef __BASIC_COMMON_H__
#define __BASIC_COMMON_H__

G_BEGIN_DECLS

/* Zero Width characters:
 *
 *  200B  ZERO WIDTH SPACE
 *  200C  ZERO WIDTH NON-JOINER
 *  200D  ZERO WIDTH JOINER
 *  200E  LEFT-TO-RIGHT MARK
 *  200F  RIGHT-TO-LEFT MARK
 *  202A  LEFT-TO-RIGHT EMBEDDING
 *  202B  RIGHT-TO-LEFT EMBEDDING
 *  202C  POP DIRECTIONAL FORMATTING
 *  202D  LEFT-TO-RIGHT OVERRIDE
 *  202E  RIGHT-TO-LEFT OVERRIDE
 *  FEFF  ZERO WIDTH NO-BREAK SPACE
 */

#define ZERO_WIDTH_CHAR(wc)\
(((wc) >= 0x200B && (wc) <= 0x200F) || ((wc) >= 0x202A && (wc) <= 0x202E) || ((wc) == 0xFEFF))

G_END_DECLS

#endif /* __BASIC_COMMON_H__ */
