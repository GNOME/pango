/* pango-arabic module 
 * 
 * (C) 2000 K. Koehler  <koehler@or.uni-bonn.de>
 *
 * general functions for arabic shaping 
 */

#ifndef __arconv_h_
#define __arconv_h_

#include <glib.h>

/*
 * arabic_reshape: reshapes string ( ordered left-to right visual order )
 *   len :  before: is the length of the string 
 *          after : number of  nun-NULL characters
 *
 */
typedef enum 
{ 
    ar_nothing  = 0x0,     ar_novowel          = 0x1, 
    ar_standard = 0x2,     ar_composedtashkeel = 0x4, 
    ar_lig      = 0x8,
    ar_mulefont = 0x10,    ar_lboxfont         = 0x20,
    ar_unifont  = 0x40,    ar_naqshfont        = 0x80
} arabic_level;

void arabic_reshape(int* len,gunichar* string,arabic_level level);
int  arabic_isvowel(gunichar s);

#endif
