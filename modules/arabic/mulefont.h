/* pango-arabic module 
 * 
 * (C) 2000 K. Koehler  <koehler@or.uni-bonn.de>
 *
 * This file provides a mapping unicode <- mulefont
 */
#ifndef __mulefont_h_
#define __mulefont_h_
#include "pango.h"
#include "pangox.h"

/* mulefonts must be an array with at least three entries */

int
arabic_muleinit(PangoFont  *font,PangoXSubfont* mulefonts);
/* a return value of 0 means this has failed */

void 
arabic_mule_recode(PangoXSubfont* subfont,int* glyph,PangoXSubfont* mulefonts);


#endif
