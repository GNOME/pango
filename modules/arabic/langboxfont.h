/* pango-arabic module 
 * 
 * (C) 2000 K. Koehler  <koehler@or.uni-bonn.de>
 *
 * This file provides a mapping unicode <- langboxfont
 */
#ifndef __lboxfont_h_
#define __lboxfont_h_
#include "pangox.h"
#include "arconv.h"

/* 
 *  create an arabic_fontstruct for the langbox-module
 *  returns: NULL on failure
 */
ArabicFontInfo*
arabic_lboxinit(PangoFont  *font);


/* glyph2 is the next glyph in line; if there is none, put in NULL 
 */
void
arabic_lbox_recode(PangoXSubfont* subfont,gunichar* glyph,int* glyph2,
                   PangoXSubfont* lboxfonts);


#endif

