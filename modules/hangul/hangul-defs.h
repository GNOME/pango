/* Pango
 * hangul-defs.h:
 *
 * Copyright (C) 2002 Changwoo Ryu
 * Author: Changwoo Ryu <cwryu@debian.org>
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

/*
 * From 3.10 of the Unicode 2.0 Book; used for combining Jamos.
 */
#define SBASE 0xAC00
#define LBASE 0x1100
#define VBASE 0x1161
#define TBASE 0x11A7

#define LEND 0x115F
#define VEND 0x11A6
#define TEND 0x11FF

#define SCOUNT 11172
#define LCOUNT 19
#define VCOUNT 21
#define TCOUNT 28
#define NCOUNT (VCOUNT * TCOUNT)

/*
 * Unicode 2.0 doesn't define the fill for trailing consonants, but
 * I'll use 0x11A7 as that purpose internally.
 */
#define LFILL 0x115F
#define VFILL 0x1160
#define TFILL 0x11A7

#define IS_L(wc) (wc >= LBASE && wc <= LEND)
#define IS_V(wc) (wc >= VFILL && wc <= VEND)
#define IS_T(wc) (wc >= TBASE && wc <= TEND)

/* jamo which can be composited as a Hangul syllable */
#define IS_L_S(wc) (wc >= LBASE && wc < LBASE + LCOUNT)
#define IS_V_S(wc) (wc >= VBASE && wc < VBASE + VCOUNT)
#define IS_T_S(wc) (wc >= TBASE && wc < TBASE + TCOUNT)

#define S_FROM_LVT(l,v,t)	(SBASE + (((l) - LBASE) * VCOUNT + ((v) - VBASE)) * TCOUNT + ((t) - TBASE))
#define S_FROM_LV(l,v)		(SBASE + (((l) - LBASE) * VCOUNT + ((v) - VBASE)) * TCOUNT)
#define L_FROM_S(s)		(LBASE + (((s) - SBASE) / NCOUNT))
#define V_FROM_S(s)		(VBASE + (((s) - SBASE) % NCOUNT) / TCOUNT)
#define T_FROM_S(s)		(TBASE + (((s) - SBASE) % TCOUNT))
