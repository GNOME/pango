/* pango-arabic module 
 * 
 * (C) 2000 K. Koehler  <koehler@or.uni-bonn.de>
 *
 * This file provides a mapping unicode <- langboxfont
 */


#include <stdio.h>
#include <glib.h>
#include "pangox.h"

/*  #define DEBUG   */
#ifdef DEBUG
#include <stdio.h>
#endif
#include "langboxfont.h"


ArabicFontInfo* 
arabic_lboxinit(PangoFont  *font)
{ 
    static char *lbox_charsets0[] = {
        "iso8859-6.8x",
    };
    
    ArabicFontInfo    *fs = NULL;
    PangoXSubfont     *subfonts;
    int               *subfont_charsets;
    int                n_subfonts;

    n_subfonts = pango_x_list_subfonts (font,lbox_charsets0, 
                                        1, &subfonts, &subfont_charsets);
    if (n_subfonts > 0)
        {
            fs              = g_new (ArabicFontInfo,1);
            fs->level       = ar_standard | ar_composedtashkeel | ar_lboxfont;
            fs->subfonts[0] = subfonts[0];
        }

    g_free (subfonts);
    g_free (subfont_charsets);
    return fs;
}


typedef struct {
    gunichar  unicodechar;
    int       charindex;
} fontentry;


static fontentry charmap [] = 
{
    { 0xFE80,0xC1 }, /* HAMZA; handle seperately !!! */
    { 0xFE81,0xC2  }, /* ALIF MADDA   */
    { 0xFE82,0xC2  }, 
    { 0xFE83,0xC3  }, /* ALIF HAMZA   */
    { 0xFE84,0xC3  }, 
    { 0xFE85,0xC4  }, /* WAW  HAMZA   */
    { 0xFE86,0xC4  }, 
    { 0xFE87,0xC5  }, /* ALIF IHAMZA  */
    { 0xFE88,0xC5  }, 
    { 0xFE89,0x9F  }, /* YA   HAMZA   */
    { 0xFE8A,0xC6  }, 
    { 0xFE8B,0xC0  }, /* HMAZA-'pod' */ 
    { 0xFE8C,0xC0  }, 
    { 0xFE8D,0xC7  }, /* ALIF         */
    { 0xFE8E,0xC7  }, 
    { 0xFE8F,0xC8  }, /* BA               */
    { 0xFE90,0xC8  }, 
    { 0xFE91,0xEB  }, 
    { 0xFE92,0xEB  }, 
    { 0xFE93,0xC9  }, /* TA MARBUTA       */
    { 0xFE94,0x8E  }, 
    { 0xFE95,0xCA  }, /* TA               */
    { 0xFE96,0xCA  }, 
    { 0xFE97,0xEC  }, 
    { 0xFE98,0xEC  }, 
    { 0xFE99,0xCB  }, /* THA      */
    { 0xFE9A,0xCB  }, 
    { 0xFE9B,0xED  }, 
    { 0xFE9C,0xED  }, 
    { 0xFE9D,0xCC  }, /* DJIM         */
    { 0xFE9E,0xCC  }, 
    { 0xFE9F,0xEE  }, 
    { 0xFEA0,0xEE  }, 
    { 0xFEA1,0xCD  }, /* .HA       */
    { 0xFEA2,0xCD  }, 
    { 0xFEA3,0xEF  }, 
    { 0xFEA4,0xEF  }, 
    { 0xFEA5,0xCE  }, /* CHA      */
    { 0xFEA6,0xCE  }, 
    { 0xFEA7,0xF0  }, 
    { 0xFEA8,0xF0  }, 
    { 0xFEA9,0xCF  }, /* DAL      */
    { 0xFEAA,0xCF  }, 
    { 0xFEAB,0xD0  }, /* THAL     */
    { 0xFEAC,0xD0  }, 
    { 0xFEAD,0xD1  }, /* RA           */
    { 0xFEAE,0xD1  }, 
    { 0xFEAF,0xD2  }, /* ZAY      */
    { 0xFEB0,0xD2  }, 
    { 0xFEB1,0xD3  }, /* SIN      */
    { 0xFEB2,0x8F  }, 
    { 0xFEB3,0xF1  }, 
    { 0xFEB4,0xF1  }, 
    { 0xFEB5,0xD4  }, /* SHIN     */
    { 0xFEB2,0x90  }, 
    { 0xFEB3,0xF2  }, 
    { 0xFEB4,0xF2  }, 
    { 0xFEB9,0xD5  }, /* SAAD     */
    { 0xFEBA,0x91  }, 
    { 0xFEBB,0xF3  }, 
    { 0xFEBC,0xF3  }, 
    { 0xFEBD,0xD6  }, /* DAAD     */
    { 0xFEBE,0x92  }, 
    { 0xFEBF,0xF4  }, 
    { 0xFEC0,0xF4  }, 
    { 0xFEC1,0xD7  }, /* .TA      */
    { 0xFEC2,0xD7  }, 
    { 0xFEC3,0x93  }, 
    { 0xFEC4,0x93  }, 
    { 0xFEC5,0xD8  }, /* .ZA      */
    { 0xFEC6,0xD8  }, 
    { 0xFEC7,0x94  }, 
    { 0xFEC8,0x94  }, 
    { 0xFEC9,0xD9  }, /* AIN      */
    { 0xFECA,0x96  }, 
    { 0xFECB,0xF5  }, 
    { 0xFECC,0x95  }, 
    { 0xFECD,0xDA  }, /* RAIN     */
    { 0xFECE,0x98  }, 
    { 0xFECF,0xF6  }, 
    { 0xFED0,0x97  }, 
    { 0xFED1,0xE1  }, /* FA               */
    { 0xFED2,0xE1  }, 
    { 0xFED3,0xF7  }, 
    { 0xFED4,0x99  }, 
    { 0xFED5,0xE2  }, /* QAF      */
    { 0xFED6,0xE2  }, 
    { 0xFED7,0xF8  }, 
    { 0xFEB8,0x9A  }, 
    { 0xFED9,0xE3  }, /* KAF      */
    { 0xFEDA,0xE3  }, 
    { 0xFEDB,0xF9  }, 
    { 0xFEDC,0x9B  }, 
    { 0xFEDD,0xE4  }, /* LAM      */
    { 0xFEDE,0xE4  }, 
    { 0xFEDF,0xFA  }, 
    { 0xFEE0,0xFA  }, 
    { 0xFEE1,0xE5  }, /* MIM      */
    { 0xFEE2,0xE5  }, 
    { 0xFEE3,0xFB  }, 
    { 0xFEE4,0xFB  }, 
    { 0xFEE5,0xE6  }, /* NUN          */
    { 0xFEE6,0xE6  }, 
    { 0xFEE7,0xFC  }, 
    { 0xFEE8,0xFC  }, 
    { 0xFEE9,0xE7  }, /* HA           */
    { 0xFEEA,0x9D  }, 
    { 0xFEEB,0xFD  }, 
    { 0xFEEC,0x9C  }, 
    { 0xFEED,0xE8  }, /* WAW          */
    { 0xFEEE,0xE8  }, 
    { 0xFEEF,0x8D  }, /* ALIF MAQSORA */
    { 0xFEF0,0xE9  }, 
    { 0xFEF1,0x9E  },  /* YA           */
    { 0xFEF2,0xEA  }, 
    { 0xFEF3,0xFE  }, 
    { 0xFEF4,0xFE  } 
};             

void 
arabic_lbox_recode(PangoXSubfont* subfont,gunichar* glyph,int* glyph2,
                   PangoXSubfont* lboxfonts)
{
    int letter=*glyph;
    
    *subfont = lboxfonts[0];
    if ((letter >= 0x660)&&(letter <= 0x669)) /* indic numeral */
        {
            *glyph   = letter - 0x660 + 0xB0;
        }
    else if ((letter >= 0xFE80)&&(letter <= 0xFEF4))
        { 
            *glyph   = charmap[letter-0xFE80].charindex;
        }
    else if ((letter >= 0x64B)&&(letter <= 0x652))
        { /* a vowel */
            *glyph   = letter - 0x64B + 0xA8;
        }
    else if ((letter >= 0xFEF5)&&(letter <= 0xFEFC)&&(glyph2)&&(*glyph2==0))
        { /* Lam-Alif. Langbox solved the problem in their own way ... */
            if (!(letter % 2))
                {
                    *glyph = 0xA6;
                }
            else 
                {
                    *glyph = 0xA5;
                }
            switch (letter)
                {
                case 0xFEF5 :
                case 0xFEF6 : *glyph2 = 0xA2; break; /* Lam-Alif Madda */
                case 0xFEF7 :
                case 0xFEF8 : *glyph2 = 0xA3; break; /* Lam-Alif Hamza */
                case 0xFEF9 : 
                case 0xFEFA : *glyph2 = 0xA4; break; /* Lam-Alif iHamza */
                case 0xFEFB :
                case 0xFEFC : *glyph2 = 0xA1; break; /* Lam-Alif */
                }
        }
    else if (letter < 0xB0 )
        {
            *glyph   = letter;
        }
    else switch(letter)
        { 
	    /* extra vowels */
        case 0xFC5E: *glyph  = 0x82; break;
        case 0xFC5F: *glyph  = 0x83; break;
        case 0xFC60: *glyph  = 0x84; break;
        case 0xFC61: *glyph  = 0x85; break;
        case 0xFC62: *glyph  = 0x86; break;
        case 0xFC63: *glyph  = 0xAE; break; /* This is not in the font */

        case 0x621: *glyph   = charmap[0].charindex; break; /* hamza */
        case 0x640: *glyph   = 0xE0; break; /* tatweel */
        case 0x61F: *glyph   = 0xBF; break; /* question mark */
	  
	    /* farsi ye */
	case 0xFBFC: *glyph = 0x8D; break;
	case 0xFBFD: *glyph = 0xE9; break; 
	case 0xFBFE: *glyph = 0xFE; break;
	case 0xFBFF: *glyph = 0xFE; break;
	    /* Gaf -- the font does not have it, but this is better than nothing */
	case 0xFB92: *glyph = 0xE3; break;
	case 0xFB93: *glyph = 0xE3; break;
	case 0xFB94: *glyph = 0xF9; break;
	case 0xFB95: *glyph = 0x9B; break;

        default:
            *glyph   = 0x20; /* we don't have this thing -- use a space */
            /* This has to be something that does not print anything !! */
            break;
        }
}





