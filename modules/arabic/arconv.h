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
 *   level: 1 : font with basic arabic characters, no vowels
 *          2 : with vowels
 *          3 : with composed vowels : Shadda+(Fatha,Damma,Kasra)
 *          4 : with some extra Ligatures
 *
 */
void arabic_reshape(int* len,gunichar* string,int level);
int  arabic_isvowel(gunichar s);

#endif
