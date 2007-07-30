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
 * under the  GNU Public License and the FreeType license. See see
 * pango/opentype/FT-license.txt for full details of the FreeType
 * license.
 */

#ifndef __SYRIAC_OT_H__
#define __SYRIAC_OT_H__

#include <pango/pango-ot.h>

G_BEGIN_DECLS

typedef enum
{
  isolated = 1 << 0,    /* nominal        */
  final    = 1 << 1,    /* right_joining */
  initial  = 1 << 2,    /* left_joining   */
  medial   = 1 << 3,    /* double_joining */
  medial2  = 1 << 4,    /* double_joining, applies to Alaph only */
  final2   = 1 << 5,    /* right_joining, applies to Alaph only */
  final3   = 1 << 6     /* right_joining, applies to Alaph only */
} JoiningType;

/* A glyph's property value as needed by e.g. TT_GSUB_Apply_String()
   specifies which features should *not* be applied  */
typedef enum
{
  isolated_p = final    | initial | medial  | medial2 | final2  | final3,
  final_p    = isolated | initial | medial  | medial2 | final2  | final3,
  initial_p  = isolated | final   | medial  | medial2 | final2  | final3,
  medial_p   = isolated | final   | initial | medial2 | final2  | final3,
  medial2_p  = isolated | final   | initial | medial  | final2  | final3,
  final2_p   = isolated | final   | initial | medial  | medial2 | final3,
  final3_p   = isolated | final   | initial | medial  | medial2 | final2
} SyriacGlyphForm;

typedef enum
{
  right,
  left,			/* not used */
  dual,
  causing,
  none,
  transparent
} JoiningClass;

FT_Error syriac_assign_properties (gunichar *string,
				   gulong   *properties,
				   int       length);

G_END_DECLS

#endif /* __SYRIAC_OT_H__ */
