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
#ifndef FRIBIDI_TYPES_H
#define FRIBIDI_TYPES_H

#ifndef FRIBIDI_USE_MINI_GLIB
#include <glib.h>
#else
#include "fribidi_mini_glib.h"
#endif

typedef guint32 FriBidiChar;

typedef guint16 FriBidiStrIndex;

/* Define some bit masks, that character types are based on, each one has
   only one bit on. */
typedef enum
{
  FRIBIDI_MASK_RTL = 0x00000001,	/* Is right to left */
  FRIBIDI_MASK_ARABIC = 0x00000002,	/* Is arabic */

  /* Each char can be only one of the three following. */
  FRIBIDI_MASK_STRONG = 0x00000010,	/* Is strong */
  FRIBIDI_MASK_WEAK = 0x00000020,	/* Is weak */
  FRIBIDI_MASK_NEUTRAL = 0x00000040,	/* Is neutral */
  FRIBIDI_MASK_SENTINEL = 0x00000080,	/* Is sentinel: SOT, EOT */
  /* Sentinels are not valid chars, just identify the start and end of strings. */

  /* Each char can be only one of the five following. */
  FRIBIDI_MASK_LETTER = 0x00000100,	/* Is letter: L, R, AL */
  FRIBIDI_MASK_NUMBER = 0x00000200,	/* Is number: EN, AN */
  FRIBIDI_MASK_NUMSEPTER = 0x00000400,	/* Is number separator or terminator: ES, ET, CS */
  FRIBIDI_MASK_SPACE = 0x00000800,	/* Is space: BN, BS, SS, WS */
  FRIBIDI_MASK_EXPLICIT = 0x00001000,	/* Is expilict mark: LRE, RLE, LRO, RLO, PDF */

  /* Can be on only if FRIBIDI_MASK_SPACE is also on. */
  FRIBIDI_MASK_SEPARATOR = 0x00002000,	/* Is test separator: BS, SS */

  /* Can be on only if FRIBIDI_MASK_EXPLICIT is also on. */
  FRIBIDI_MASK_OVERRIDE = 0x00004000,	/* Is explicit override: LRO, RLO */

  /* The following must be to make types pairwise different, some of them can
     be removed but are here because of efficiency (make queries faster). */

  FRIBIDI_MASK_ES = 0x00010000,
  FRIBIDI_MASK_ET = 0x00020000,
  FRIBIDI_MASK_CS = 0x00040000,

  FRIBIDI_MASK_NSM = 0x00080000,
  FRIBIDI_MASK_BN = 0x00100000,

  FRIBIDI_MASK_BS = 0x00200000,
  FRIBIDI_MASK_SS = 0x00400000,
  FRIBIDI_MASK_WS = 0x00800000
}
FriBidiMaskType;

typedef enum
{
  FRIBIDI_TYPE_LTR =		/* Strong left to right */
    FRIBIDI_MASK_STRONG + FRIBIDI_MASK_LETTER,
  FRIBIDI_TYPE_RTL =		/* Right to left characters */
    FRIBIDI_MASK_STRONG + FRIBIDI_MASK_LETTER + FRIBIDI_MASK_RTL,
  FRIBIDI_TYPE_AL =		/* Arabic characters */
    FRIBIDI_MASK_STRONG + FRIBIDI_MASK_LETTER +
    FRIBIDI_MASK_RTL + FRIBIDI_MASK_ARABIC,
  FRIBIDI_TYPE_LRE =		/* Left-To-Right embedding */
    FRIBIDI_MASK_STRONG + FRIBIDI_MASK_EXPLICIT,
  FRIBIDI_TYPE_RLE =		/* Right-To-Left embedding */
    FRIBIDI_MASK_STRONG + FRIBIDI_MASK_EXPLICIT + FRIBIDI_MASK_RTL,
  FRIBIDI_TYPE_LRO =		/* Left-To-Right override */
    FRIBIDI_MASK_STRONG + FRIBIDI_MASK_EXPLICIT + FRIBIDI_MASK_OVERRIDE,
  FRIBIDI_TYPE_RLO =		/* Right-To-Left override */
    FRIBIDI_MASK_STRONG + FRIBIDI_MASK_EXPLICIT +
    FRIBIDI_MASK_RTL + FRIBIDI_MASK_OVERRIDE,

  FRIBIDI_TYPE_PDF =		/* Pop directional override */
    FRIBIDI_MASK_WEAK + FRIBIDI_MASK_EXPLICIT,
  FRIBIDI_TYPE_EN =		/* European digit */
    FRIBIDI_MASK_WEAK + FRIBIDI_MASK_NUMBER,
  FRIBIDI_TYPE_AN =		/* Arabic digit */
    FRIBIDI_MASK_WEAK + FRIBIDI_MASK_NUMBER + FRIBIDI_MASK_ARABIC,
  FRIBIDI_TYPE_ES =		/* European number separator */
    FRIBIDI_MASK_WEAK + FRIBIDI_MASK_NUMSEPTER + FRIBIDI_MASK_ES,
  FRIBIDI_TYPE_ET =		/* European number terminator */
    FRIBIDI_MASK_WEAK + FRIBIDI_MASK_NUMSEPTER + FRIBIDI_MASK_ET,
  FRIBIDI_TYPE_CS =		/* Common Separator */
    FRIBIDI_MASK_WEAK + FRIBIDI_MASK_NUMSEPTER + FRIBIDI_MASK_CS,
  FRIBIDI_TYPE_NSM =		/* Non spacing mark */
    FRIBIDI_MASK_WEAK + FRIBIDI_MASK_NSM,
  FRIBIDI_TYPE_BN =		/* Boundary neutral */
    FRIBIDI_MASK_WEAK + FRIBIDI_MASK_SPACE + FRIBIDI_MASK_BN,

  FRIBIDI_TYPE_BS =		/* Block separator */
    FRIBIDI_MASK_NEUTRAL + FRIBIDI_MASK_SPACE +
    FRIBIDI_MASK_SEPARATOR + FRIBIDI_MASK_BS,
  FRIBIDI_TYPE_SS =		/* Segment separator */
    FRIBIDI_MASK_NEUTRAL + FRIBIDI_MASK_SPACE +
    FRIBIDI_MASK_SEPARATOR + FRIBIDI_MASK_SS,
  FRIBIDI_TYPE_WS =		/* Whitespace */
    FRIBIDI_MASK_NEUTRAL + FRIBIDI_MASK_SPACE + FRIBIDI_MASK_WS,
  FRIBIDI_TYPE_ON =		/* Other Neutral */
    FRIBIDI_MASK_NEUTRAL,

  /* The following are used to identify the paragraph direction,
     types L, R, N are not used internally anymore, and recommended to use
     LTR, RTL and ON instead, didn't removed because of compatability. */
  FRIBIDI_TYPE_L = FRIBIDI_TYPE_LTR,
  FRIBIDI_TYPE_R = FRIBIDI_TYPE_RTL,
  FRIBIDI_TYPE_N = FRIBIDI_TYPE_ON,
  FRIBIDI_TYPE_WL =		/* Weak left to right */
    FRIBIDI_MASK_WEAK,
  FRIBIDI_TYPE_WR =		/* Weak right to left */
    FRIBIDI_MASK_WEAK + FRIBIDI_MASK_RTL,

  /* The following are only used internally */
  FRIBIDI_TYPE_SOT =		/* Start of text */
    FRIBIDI_MASK_SENTINEL,
  FRIBIDI_TYPE_EOT =		/* End of text */
    FRIBIDI_MASK_SENTINEL + FRIBIDI_MASK_RTL
}
FriBidiCharType;

/* Defining macros for needed queries, It is fully dependent on the 
   implementation of FriBidiCharType. */

/* Return the direction of the level number, FRIBIDI_TYPE_LTR for even and
   FRIBIDI_TYPE_RTL for odds. */
#define FRIBIDI_LEVEL_TO_DIR(lev) (FRIBIDI_TYPE_LTR | (lev & 1))

/* Return the minimum level of the direction, 0 for FRIBIDI_TYPE_LTR and
   1 for FRIBIDI_TYPE_RTL and FRIBIDI_TYPE_AL. */
#define FRIBIDI_DIR_TO_LEVEL(dir) (dir & 1)

/* Is right to left? */
#define FRIBIDI_IS_RTL(p)      ((P) & FRIBIDI_MASK_RTL)
/* Is arabic? */
#define FRIBIDI_IS_ARABIC(p)   ((p) & FRIBIDI_MASK_ARABIC)

/* Is strong? */
#define FRIBIDI_IS_STRONG(p)   ((p) & FRIBIDI_MASK_STRONG)
/* Is weak? */
#define FRIBIDI_IS_WEAK(p)     ((p) & FRIBIDI_MASK_WEAK)
/* Is neutral? */
#define FRIBIDI_IS_NEUTRAL(p)  ((p) & FRIBIDI_MASK_NEUTRAL)
/* Is sentinel? */
#define FRIBIDI_IS_SENTINEL(p) ((p) & FRIBIDI_MASK_SENTINEL)

/* Is letter: L, R, AL? */
#define FRIBIDI_IS_LETTER(p)   ((p) & FRIBIDI_MASK_LETTER)
/* Is number: EN, AN? */
#define FRIBIDI_IS_NUMBER(p)   ((p) & FRIBIDI_MASK_NUMBER)
/* Is number separator or terminator: ES, ET, CS? */
#define FRIBIDI_IS_NUMBER_SEPARATOR_OR_TERMINATOR(p) \
                       ((p) & FRIBIDI_MASK_NUMSEPTER)
/* Is space: BN, BS, SS, WS? */
#define FRIBIDI_IS_SPACE(p)    ((p) & FRIBIDI_MASK_SPACE)
/* Is explicit mark: LRE, RLE, LRO, RLO, PDF? */
#define FRIBIDI_IS_EXPLICIT(p) ((p) & FRIBIDI_MASK_EXPLICIT)

/* Is test separator: BS, SS? */
#define FRIBIDI_IS_SEPARATOR(p) ((p) & FRIBIDI_MASK_SEPARATOR)

/* Is explicit override: LRO, RLO? */
#define FRIBIDI_IS_OVERRIDE(p) ((p) & FRIBIDI_MASK_OVERRIDE)

/* Some more: */

/* Is left to right letter: LTR? */
#define FRIBIDI_IS_LTR_LETTER(p) \
    ((p) & (FRIBIDI_MASK_LETTER | FRIBIDI_MASK_RTL) == FRIBIDI_MASK_LETTER)

/* Is right to left letter: RTL, AL? */
#define FRIBIDI_IS_RTL_LETTER(p) \
    ((p) & (FRIBIDI_MASK_LETTER | FRIBIDI_MASK_RTL) == \
    (FRIBIDI_MASK_LETTER | FRIBIDI_MASK_RTL))

/* Is ES or CS: ES, CS? */
#define FRIBIDI_IS_ES_OR_CS(p) \
    ((p) & (FRIBIDI_MASK_ES | FRIBIDI_MASK_CS))

/* Is explicit or BN: LRE, RLE, LRO, RLO, PDF, BN? */
#define FRIBIDI_IS_EXPLICIT_OR_BN(p) \
    ((p) & (FRIBIDI_MASK_EXPLICIT | FRIBIDI_MASK_BN))

/* Is explicit or separator or BN or WS: LRE, RLE, LRO, RLO, PDF, BS, SS, BN, WS? */
#define FRIBIDI_IS_EXPLICIT_OR_SEPARATOR_OR_BN_OR_WS(p) \
    ((p) & (FRIBIDI_MASK_EXPLICIT | FRIBIDI_MASK_SEPARATOR | \
            FRIBIDI_MASK_BN | FRIBIDI_MASK_WS))

/* Define some conversions. */

/* Change numbers:EN, AN  to RTL. */
#define FRIBIDI_CHANGE_NUMBER_TO_RTL(p) \
    (FRIBIDI_IS_NUMBER(p) ? FRIBIDI_TYPE_RTL : (p))

/* Override status of an explicit mark: LRO->LTR, RLO->RTL, otherwise->ON. */
#define FRIBIDI_EXPLICIT_TO_OVERRIDE_DIR(p) \
    (FRIBIDI_IS_OVERRIDE(p) ? FRIBIDI_LEVEL_TO_DIR(FRIBIDI_DIR_TO_LEVEL(p)) : \
                              FRIBIDI_TYPE_ON)


gchar fribidi_char_from_type (FriBidiCharType c);

gchar *fribidi_type_name (FriBidiCharType c);


/* Define character types that fribidi_tables.i uses. if MEM_OPTIMIZED
   defined, then define them to be 0, 1, 2, ... and then in
   fribidi_get_type.c map them on FriBidiCharType-s, else define them to
   be equal to FribidiCharType-s */
#ifdef MEM_OPTIMIZED
#define _FRIBIDI_PROP(type) FRIBIDI_PROP_TYPE_##type
typedef guint8 FriBidiPropCharType;
#else
#define _FRIBIDI_PROP(type) FRIBIDI_PROP_TYPE_##type = FRIBIDI_TYPE_##type
typedef FriBidiCharType FriBidiPropCharType;
#endif
enum
{
  _FRIBIDI_PROP (LTR),		/* Strong left to right */
  _FRIBIDI_PROP (RTL),		/* Right to left characters */
  _FRIBIDI_PROP (AL),		/* Arabic characters */
  _FRIBIDI_PROP (LRE),		/* Left-To-Right embedding */
  _FRIBIDI_PROP (RLE),		/* Right-To-Left embedding */
  _FRIBIDI_PROP (LRO),		/* Left-To-Right override */
  _FRIBIDI_PROP (RLO),		/* Right-To-Left override */
  _FRIBIDI_PROP (PDF),		/* Pop directional override */
  _FRIBIDI_PROP (EN),		/* European digit */
  _FRIBIDI_PROP (AN),		/* Arabic digit */
  _FRIBIDI_PROP (ES),		/* European number separator */
  _FRIBIDI_PROP (ET),		/* European number terminator */
  _FRIBIDI_PROP (CS),		/* Common Separator */
  _FRIBIDI_PROP (NSM),		/* Non spacing mark */
  _FRIBIDI_PROP (BN),		/* Boundary neutral */
  _FRIBIDI_PROP (BS),		/* Block separator */
  _FRIBIDI_PROP (SS),		/* Segment separator */
  _FRIBIDI_PROP (WS),		/* Whitespace */
  _FRIBIDI_PROP (ON),		/* Other Neutral */
  _FRIBIDI_PROP (WL),		/* Weak left to right */
  _FRIBIDI_PROP (WR),		/* Weak right to left */
  _FRIBIDI_PROP (SOT),		/* Start of text */
  _FRIBIDI_PROP (EOT)		/* End of text */
};
#undef _FRIBIDI_PROP

/* The following type is used by fribidi_utils */
typedef struct
{
  int length;
  void *attribute;
}
FriBidiRunType;

/* TBD: The following should be configuration parameters, once we can
   figure out how to make configure set them... */
#ifndef FRIBIDI_MAX_STRING_LENGTH
#define FRIBIDI_MAX_STRING_LENGTH 65535
#endif

FriBidiCharType _pango_fribidi_get_type(FriBidiChar uch);

#endif
