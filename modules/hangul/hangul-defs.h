/* Pango
 * hangul-defs.h:
 *
 * Copyright (C) 2002-2006 Changwoo Ryu
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

/* See 3.12 "Conjoining Jamo Behavior" in the Unicode Book for more
 * information
 */

/* The start of the Hangul Syllables (U+AC00-U+D7A3) */
#define SBASE 0xAC00
#define SCOUNT 11172

/* The starts/ends of the leading consonants (choseong), the vowels
 * (jungseong) and the trailing consonants (jongseong).
 */
#define LBASE 0x1100
#define VBASE 0x1161
#define TBASE 0x11A7
#define LEND 0x115F
#define VEND 0x11A7
#define TEND 0x11FF

/* Number of modern jamos */
#define LCOUNT 19
#define VCOUNT 21
#define TCOUNT 28
#define NCOUNT (VCOUNT * TCOUNT) /* number of syllables of a given choseong */

/* choseong and jungseong filler */
#define LFILL 0x115F
#define VFILL 0x1160

/* Old tone marks. */
#define HTONE1 0x302E
#define HTONE2 0x302F

/* Useful macros
 */

#define IS_JAMO(wc) ((wc) >= LBASE && (wc) <= TEND)
#define IS_L(wc) ((wc) >= LBASE && (wc) <= LEND)
#define IS_V(wc) ((wc) >= VFILL && (wc) <= VEND)
#define IS_T(wc) ((wc) > TBASE && (wc) <= TEND)
#define IS_M(wc) ((wc) == HTONE1 || (wc) == HTONE2)
#define IS_S(wc) (SBASE <= (wc) && (wc) < (SBASE + SCOUNT))

/* jamo which can be composited as a precomposed syllable */
#define IS_L_S(wc) ((wc) >= LBASE && (wc) < (LBASE + LCOUNT))
#define IS_V_S(wc) ((wc) >= VBASE && (wc) < (VBASE + VCOUNT))
#define IS_T_S(wc) ((wc) > TBASE && (wc) < (TBASE + TCOUNT))

/* if a syllable has a jongseong */
#define S_HAS_T(s) (((s) - SBASE) % TCOUNT)

/* non hangul */
#define IS_HANGUL(wc)	(IS_S(wc) || IS_JAMO(wc) || IS_M(wc))

/* syllable boundary condition */
#define IS_BOUNDARY(prev,next)					\
	((!IS_L(prev) && IS_S(wc)) ||				\
	 !IS_HANGUL(next) ||					\
	 (IS_S(prev) && S_HAS_T(prev) && IS_L(next)) ||		\
	 (IS_T(prev) && (IS_L(next) || IS_V(next))) ||		\
	 (IS_S(prev) && !S_HAS_T(prev) && IS_L(next)) ||	\
	 (IS_V(prev) && IS_L(next)) ||				\
	 IS_M(prev))

/* composing/decomposing */
#define S_FROM_LVT(l,v,t)	(SBASE + (((l) - LBASE) * VCOUNT + ((v) - VBASE)) * TCOUNT + ((t) - TBASE))
#define S_FROM_LV(l,v)		(SBASE + (((l) - LBASE) * VCOUNT + ((v) - VBASE)) * TCOUNT)
#define L_FROM_S(s)		(LBASE + (((s) - SBASE) / NCOUNT))
#define V_FROM_S(s)		(VBASE + (((s) - SBASE) % NCOUNT) / TCOUNT)
#define T_FROM_S(s)		(TBASE + (((s) - SBASE) % TCOUNT))
