/* pango-arabic module 
 * 
 * (C) 2000 K. Koehler  <koehler@or.uni-bonn.de>
 *
 * This file provides a mapping unicode <- mulefont
 */
#ifndef __mulefont_h_
#define __mulefont_h_
#include "pango-layout.h"
#include "pangox.h"
#include "arconv.h"

/* 
 *  create an arabic_fontstruct for the mulefont-module
 *  returns: NULL on failure
 */
ArabicFontInfo*
arabic_muleinit(PangoFont  *font);

void 
arabic_mule_recode(PangoXSubfont* subfont,gunichar* glyph,PangoXSubfont* mulefonts);

#endif
