/* pango-arabic module 
 * 
 * (C) 2000 K. Koehler  <koehler@or.uni-bonn.de>
 *
 * This file provides a mapping unicode <- naqshfont
 */


#include <stdio.h>
#include <glib.h>
#include "pango.h"
#include "pangox.h"

/*  #define DEBUG   */
#ifdef DEBUG
#include <stdio.h>
#endif


int
urdu_naqshinit(PangoFont  *font,PangoXSubfont* nqfont)
{ 
    static char *nq_charsets0[] = {
        "symbol-0",
    };

    PangoXSubfont *subfonts;
    int           *subfont_charsets;
    int            n_subfonts;

    n_subfonts = pango_x_list_subfonts (font,nq_charsets0, 
                                        1, &subfonts, &subfont_charsets);
    if (n_subfonts > 0)
        {
            nqfont[0] = subfonts[0];
            g_free (subfonts);
            g_free (subfont_charsets);
        }
    else
        {
            g_free (subfonts);
            g_free (subfont_charsets);
            return 0;
        }

    return 1;
}



typedef struct {
    gunichar   unicodechar;
    int       charindex;
} fontentry;


static fontentry charmap [] = 
{
    { 0xFE80,0x4A }, /* HAMZA; handle seperately !!! */
    { 0xFE81,0x22  }, /* ALIF MADDA   */
    { 0xFE82,0xAD  }, 
    { 0xFE83,0x22  }, /* ALIF HAMZA   */
    { 0xFE84,0xAD  }, 
    { 0xFE85,0x46  }, /* WAW  HAMZA   */
    { 0xFE86,0xD2  }, 
    { 0xFE87,0x22  }, /* ALIF IHAMZA  */
    { 0xFE88,0xAD  }, 
    { 0xFE89,0x4C  }, /* YA   HAMZA   */
    { 0xFE8A,0xD7  }, 

    { 0xFE8B,0x6C  }, /* HMAZA-'pod' */ 
    { 0xFE8C,0xAB  }, 
    { 0xFE8D,0x22  }, /* ALIF         */
    { 0xFE8E,0xAD  }, 
    { 0xFE8F,0x23  }, /* BA               */
    { 0xFE90,0xAE  }, 
    { 0xFE91,0x4E  }, 
    { 0xFE92,0x6E  }, 
    { 0xFE93,0x48  }, /* TA MARBUTA       */
    { 0xFE94,0xD5  }, 
    { 0xFE95,0x25  }, /* TA               */
    { 0xFE96,0xB0  }, 
    { 0xFE97,0x50  }, 
    { 0xFE98,0x70  }, 
    { 0xFE99,0x27  }, /* THA      */
    { 0xFE9A,0xB2  }, 
    { 0xFE9B,0x52  }, 
    { 0xFE9C,0x72  }, 
    { 0xFE9D,0x28  }, /* DJIM         */
    { 0xFE9E,0xB3  }, 
    { 0xFE9F,0x53  }, 
    { 0xFEA0,0x73  }, 
    { 0xFEA1,0x2A  }, /* .HA       */
    { 0xFEA2,0xB5  }, 
    { 0xFEA3,0x55  }, 
    { 0xFEA4,0x75  }, 
    { 0xFEA5,0x2B  }, /* CHA      */
    { 0xFEA6,0xB6  }, 
    { 0xFEA7,0x56  }, 
    { 0xFEA8,0x76  }, 
    { 0xFEA9,0x2C  }, /* DAL      */
    { 0xFEAA,0xB8  }, 
    { 0xFEAB,0x2E  }, /* THAL     */
    { 0xFEAC,0xBA  }, 
    { 0xFEAD,0x2F  }, /* RA           */
    { 0xFEAE,0xBB  }, 
    { 0xFEAF,0x31  }, /* ZAY      */
    { 0xFEB0,0xBD  }, 

    { 0xFEB1,0x33  }, /* SIN      */
    { 0xFEB2,0xBF  }, 
    { 0xFEB3,0x57  }, 
    { 0xFEB4,0x77  }, 
    { 0xFEB5,0x34  }, /* SHIN     */
    { 0xFEB2,0xC0  }, 
    { 0xFEB3,0x58  }, 
    { 0xFEB4,0x78  }, 
    { 0xFEB9,0x35  }, /* SAAD     */
    { 0xFEBA,0xC1  }, 
    { 0xFEBB,0x59  }, 
    { 0xFEBC,0x79  }, 
    { 0xFEBD,0x36  }, /* DAAD     */
    { 0xFEBE,0xC2  }, 
    { 0xFEBF,0x5A  }, 
    { 0xFEC0,0x7A  }, 
    { 0xFEC1,0x37  }, /* .TA      */
    { 0xFEC2,0xC3  }, 
    { 0xFEC3,0x5B  }, 
    { 0xFEC4,0x7B  }, 
    { 0xFEC5,0x38  }, /* .ZA      */
    { 0xFEC6,0xC4  }, 
    { 0xFEC7,0x5C  }, 
    { 0xFEC8,0x7C  }, 
    { 0xFEC9,0x39  }, /* AIN      */
    { 0xFECA,0xC5  }, 
    { 0xFECB,0x5D  }, 
    { 0xFECC,0x7D  }, 
    { 0xFECD,0x3A  }, /* RAIN     */
    { 0xFECE,0xC6  }, 
    { 0xFECF,0x5E  }, 
    { 0xFED0,0x7E  }, 
    { 0xFED1,0x3B  }, /* FA               */
    { 0xFED2,0xC7  }, 
    { 0xFED3,0x5F  }, 
    { 0xFED4,0xA1  }, 

    { 0xFED5,0x3D  }, /* QAF      */
    { 0xFED6,0xC9  }, 
    { 0xFED7,0x61  }, 
    { 0xFEB8,0xA3  }, 
    { 0xFED9,0x3E  }, /* KAF      */
    { 0xFEDA,0xCA  }, 
    { 0xFEDB,0x62  }, 
    { 0xFEDC,0xA4  }, 
    { 0xFEDD,0x41  }, /* LAM      */
    { 0xFEDE,0xCD  }, 
    { 0xFEDF,0x66  }, 
    { 0xFEE0,0xA6  }, 
    { 0xFEE1,0x43  }, /* MIM      */
    { 0xFEE2,0xCF  }, 
    { 0xFEE3,0x67  }, 
    { 0xFEE4,0xA7  }, 
    { 0xFEE5,0x44  }, /* NUN          */
    { 0xFEE6,0xD0  }, 
    { 0xFEE7,0x68  }, 
    { 0xFEE8,0xA8  }, 

    { 0xFEE9,0x47  }, /* HA           */
    { 0xFEEA,0xD4  }, 
    { 0xFEEB,0x6B  }, 
    { 0xFEEC,0xAA  }, 
    { 0xFEED,0x46  }, /* WAW          */
    { 0xFEEE,0xD2  }, 
    { 0xFEEF,0x4B  }, /* ALIF MAQSORA */
    { 0xFEF0,0xD6  }, 
    { 0xFEF1,0x4C  },  /* YA           */
    { 0xFEF2,0xD7  }, 
    { 0xFEF3,0x6D  }, 
    { 0xFEF4,0xAC  } 
};             


void
urdu_naqsh_recode(PangoXSubfont* subfont,int* glyph,int* glyph2,
                   PangoXSubfont* nqfont)
{
    int letter=*glyph;
    
    *subfont = nqfont[0];
    if ((letter >= 0x661)&&(letter <= 0x664)) /* indic numeral */
        {
            *glyph   = letter - 0x661 + 0xD9;
        }
    if ((letter >= 0x6F1)&&(letter <= 0x6F3)) /* indic numeral */
        {
            *glyph   = letter - 0x6F1 + 0xD9;
        }
    else if ((letter >= 0xFE80)&&(letter <= 0xFEF4))
        { 
            *glyph   = charmap[letter-0xFE80].charindex;
        }
    else if ((letter >= 0xFEF5)&&(letter <= 0xFEFC))
        { /* Lam-Alif. Also solved interestingly ... 
           * At least, the Lam-Alif itself is not split ...
           */
#ifdef DEBUG
            fprintf(stderr,"[ar] nq-recoding chars %x",
                    letter);
#endif
            if (!(letter % 2))
                {
                    *glyph = 0xCE;
                }
            else 
                {
                    *glyph = 0x42;
                }
            switch (letter)
                {
                case 0xFEF5 :
                case 0xFEF6 : *glyph2 = 0xF3; break; /* Lam-Alif Madda */
                case 0xFEF7 :
                case 0xFEF8 : *glyph2 = 0xF6; break; /* Lam-Alif Hamza */
                case 0xFEF9 : 
                case 0xFEFA : *glyph2 = 0xF5; break; /* Lam-Alif iHamza */
                }
        }
    else switch(letter)
        {
        case 0x665: *glyph   = 0xDE; break; /* indic numerals */
        case 0x666: *glyph   = 0xE0; break;
        case 0x667: *glyph   = 0xE1; break;
        case 0x668: *glyph   = 0xE3; break;
        case 0x669: *glyph   = 0xE4; break;
        case 0x660: *glyph   = 0xE5; break;
            /* farsi numerals */
        case 0x6F4: *glyph   = 0xDD; break;
        case 0x6F5: *glyph   = 0xDF; break;
        case 0x6F6: *glyph   = 0xE0; break;
        case 0x6F7: *glyph   = 0xE1; break;
        case 0x6F8: *glyph   = 0xE3; break;
        case 0x6F9: *glyph   = 0xE4; break;
        case 0x6F0: *glyph   = 0xE5; break;
            /* tashkeel */
        case 0x64B: *glyph   = 0xF8; break;
        case 0x64C: *glyph   = 0xF7; break;
        case 0x64D: *glyph   = 0xF9; break;
        case 0x64E: *glyph   = 0xFD; break;
        case 0x64F: *glyph   = 0xFE; break;
        case 0x650: *glyph   = 0xFF; break;
        case 0x651: *glyph   = 0xFC; break;
        case 0x652: *glyph   = 0xEC; break;
        case 0x653: *glyph   = 0xF3; break;
        case 0x670: *glyph   = 0xF4; break;
            /* urdu letters */
        case 0xFB56: *glyph  = 0x24;
        case 0xFB57: *glyph  = 0x24;
        case 0xFB58: *glyph  = 0x24;
        case 0xFB59: *glyph  = 0x24;

        case 0xFB66: *glyph  = 0x26;
        case 0xFB67: *glyph  = 0xB1;
        case 0xFB68: *glyph  = 0x51;
        case 0xFB69: *glyph  = 0x71;

        case 0xFB7A: *glyph  = 0x29;
        case 0xFB7B: *glyph  = 0xB4;
        case 0xFB7C: *glyph  = 0x54;
        case 0xFB7D: *glyph  = 0x74;

        case 0xFB88: *glyph  = 0x2D;
        case 0xFB89: *glyph  = 0xB9;

        case 0xFB8C: *glyph  = 0x30;
        case 0xFB8D: *glyph  = 0xBC;

        case 0xFB8A: *glyph  = 0x32;
        case 0xFB8B: *glyph  = 0xBE;
            
        case 0xFB6A: *glyph  = 0x3C;
        case 0xFB6B: *glyph  = 0xC8;
        case 0xFB6C: *glyph  = 0x60;
        case 0xFB6D: *glyph  = 0xA2;
            
        case 0xFB8E: *glyph  = 0x3F;
        case 0xFB8F: *glyph  = 0xCB;
        case 0xFB90: *glyph  = 0x62;
        case 0xFB91: *glyph  = 0xA4;
            
        case 0xFB92: *glyph  = 0x40;
        case 0xFB93: *glyph  = 0xCC;
        case 0xFB94: *glyph  = 0x63;
        case 0xFB95: *glyph  = 0xA5;

        case 0xFBAA: *glyph  = 0x49;
        case 0xFBAB: *glyph  = 0xD4;
        case 0xFBAC: *glyph  = 0x6B;
        case 0xFBAD: *glyph  = 0xAA;

        case 0xFBAE: *glyph  = 0x4D;
        case 0xFBAF: *glyph  = 0xD8;
            
        case 0xFBA6: *glyph  = 0x47;
        case 0xFBA7: *glyph  = 0xD3;
        case 0xFBA8: *glyph  = 0x69;
        case 0xFBA9: *glyph  = 0xA9;
            

            /* specials */
        case 0x621: *glyph   = 0x4A; break;
        case 0x640: *glyph   = 0xD3; break; /* tatweel ? */
        case 0x61F: *glyph   = 0xE9; break; /* Question mark */
            
        default: *glyph = 0xE5; break; /* don't know what to do */
        }
}





