/* pango-arabic module 
 * 
 * (C) 2000 K. Koehler  <koehler@or.uni-bonn.de>
 *
 * This file provides a mapping unicode <- mulefont
 */
#ifndef __nqfont_h_
#define __nqfont_h_
#include "pango.h"
#include "pangox.h"

/* 
 * nqfont must point to valid memory for this to work.
 */
int
urdu_naqshinit(PangoFont  *font,PangoXSubfont* nqfont);
/* a return value of 0 means this has failed */


/* glyph2 is the next glyph in line; if there is none, put in NULL 
 */
void
urdu_naqsh_recode(PangoXSubfont* subfont,int* glyph,int* glyph2,
                   PangoXSubfont* nqfont);
#endif
