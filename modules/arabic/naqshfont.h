/* pango-arabic module 
 * 
 * (C) 2000 K. Koehler  <koehler@or.uni-bonn.de>
 *
 * This file provides a mapping unicode <- urdu nq-font
 */
#ifndef __nqfont_h_
#define __nqfont_h_
#include "pangox.h"
#include "arconv.h"

/* 
 *  create an arabic_fontstruct for the urdu_naqshfont-module
 *  returns: NULL on failure
 */
ArabicFontInfo*
urdu_naqshinit(PangoFont  *font);


/* glyph2 is the next glyph in line; if there is none, put in NULL 
 */
void
urdu_naqsh_recode(PangoXSubfont* subfont,gunichar* glyph,int* glyph2,
                   PangoXSubfont* nqfont);
#endif


