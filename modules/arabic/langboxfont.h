/* pango-arabic module 
 * 
 * (C) 2000 K. Koehler  <koehler@or.uni-bonn.de>
 *
 * This file provides a mapping unicode <- langboxfont
 */
#ifndef __lboxfont_h_
#define __lboxfont_h_
#include "pango.h"
#include "pangox.h"

/* 
 * lboxfont must point to valid memory for this to work.
 */
int
arabic_lboxinit(PangoFont  *font,PangoXSubfont* lboxfonts);
/* a return value of 0 means this has failed */


/* glyph2 is the next glyph in line; if there is none, put in NULL 
 */
void
arabic_lbox_recode(PangoXSubfont* subfont,int* glyph,int* glyph2,
                   PangoXSubfont* lboxfonts);


#endif

