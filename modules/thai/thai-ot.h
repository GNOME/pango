/* Pango
 * thai-ot.h:
 *
 * Copyright (C) 2004 Theppitak Karoonboonyanan
 * Author: Theppitak Karoonboonyanan <thep@linux.thai.net>
 */

#ifndef __THAI_OT_H__
#define __THAI_OT_H__

#include "pango-ot.h"

PangoOTRuleset *
thai_ot_get_ruleset (PangoFont *font);

void 
thai_ot_shape (PangoFont        *font,
               PangoGlyphString *glyphs);

#endif /* __THAI_OT_H__ */

