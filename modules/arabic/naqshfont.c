/* pango-arabic module 
 * 
 * (C) 2000 K. Koehler  <koehler@or.uni-bonn.de>
 * (c) 2000 Pablo Saratxaga <pablo@mandrakesoft.com>
 *
 * This file provides a mapping unicode <- naqshfont
 */


#include <stdio.h>
#include <glib.h>
#include "pangox.h"

/*  #define DEBUG    */
#ifdef DEBUG
#include <stdio.h>
#endif
#include "naqshfont.h"


ArabicFontInfo*
urdu_naqshinit(PangoFont  *font)
{ 
    static char *nq_charsets0[] = {
        "symbol-0",
/*          "urdunaqsh-0" */
    };

    ArabicFontInfo  *fs = NULL;
    PangoXSubfont   *subfonts;
    int             *subfont_charsets;
    int              n_subfonts;

    n_subfonts = pango_x_list_subfonts (font,nq_charsets0, 
                                        1, &subfonts, &subfont_charsets);
    if (n_subfonts > 0)
        {
            fs              = g_new (ArabicFontInfo,1);
            fs->level       = ar_standard | ar_naqshfont;
            fs->subfonts[0] = subfonts[0];
        }
    g_free (subfonts);
    g_free (subfont_charsets);
    return fs;
}




typedef struct {
    gunichar   unicodechar;
    int        charindex;
} fontentry;


static fontentry charmap [] = 
{ /* the basic arabic block comes first ... */
    { 0xFE8B,0x6C  }, /* HMAZA-'pod' */
    { 0xFE8C,0xAB  },
    { 0xFE8D,0x22  }, /* ALEF */
    { 0xFE8E,0xAD  },
    { 0xFE8F,0x23  }, /* BEH */
    { 0xFE90,0xAE  }, 
    { 0xFE91,0x4E  }, 
    { 0xFE92,0x6E  }, 
    { 0xFE93,0x48  }, /* TEH MARBUTA */
    { 0xFE94,0xD5  }, 
    { 0xFE95,0x25  }, /* TEH */
    { 0xFE96,0xB0  }, 
    { 0xFE97,0x50  }, 
    { 0xFE98,0x70  }, 
    { 0xFE99,0x27  }, /* THEH */
    { 0xFE9A,0xB2  }, 
    { 0xFE9B,0x52  }, 
    { 0xFE9C,0x72  }, 
    { 0xFE9D,0x28  }, /* JEEM */
    { 0xFE9E,0xB3  }, 
    { 0xFE9F,0x53  }, 
    { 0xFEA0,0x73  }, 
    { 0xFEA1,0x2A  }, /* HAH . */
    { 0xFEA2,0xB5  }, 
    { 0xFEA3,0x55  }, 
    { 0xFEA4,0x75  }, 
    { 0xFEA5,0x2B  }, /* KHAH   */
    { 0xFEA6,0xB6  }, 
    { 0xFEA7,0x56  }, 
    { 0xFEA8,0x76  }, 
    { 0xFEA9,0x2C  }, /* DAL */
    { 0xFEAA,0xB8  }, 
    { 0xFEAB,0x2E  }, /* THAL */
    { 0xFEAC,0xBA  }, 
    { 0xFEAD,0x2F  }, /* REH  */
    { 0xFEAE,0xBB  }, 
    { 0xFEAF,0x31  }, /* ZAIN (ZAY) */
    { 0xFEB0,0xBD  }, 
    { 0xFEB1,0x33  }, /* SEEN   */
    { 0xFEB2,0xBF  }, 
    { 0xFEB3,0x57  }, 
    { 0xFEB4,0x77  }, 
    { 0xFEB5,0x34  }, /* SHEEN  */
    { 0xFEB2,0xC0  }, 
    { 0xFEB3,0x58  }, 
    { 0xFEB4,0x78  }, 
    { 0xFEB9,0x35  }, /* SAD    */
    { 0xFEBA,0xC1  }, 
    { 0xFEBB,0x59  }, 
    { 0xFEBC,0x79  }, 
    { 0xFEBD,0x36  }, /* DAD    */
    { 0xFEBE,0xC2  }, 
    { 0xFEBF,0x5A  }, 
    { 0xFEC0,0x7A  }, 
    { 0xFEC1,0x37  }, /* TAH  . */
    { 0xFEC2,0xC3  }, 
    { 0xFEC3,0x5B  }, 
    { 0xFEC4,0x7B  }, 
    { 0xFEC5,0x38  }, /* ZAH .  */
    { 0xFEC6,0xC4  }, 
    { 0xFEC7,0x5C  }, 
    { 0xFEC8,0x7C  }, 
    { 0xFEC9,0x39  }, /* AIN    */
    { 0xFECA,0xC5  }, 
    { 0xFECB,0x5D  }, 
    { 0xFECC,0x7D  }, 
    { 0xFECD,0x3A  }, /* GHAIN  */
    { 0xFECE,0xC6  }, 
    { 0xFECF,0x5E  }, 
    { 0xFED0,0x7E  }, 
    { 0xFED1,0x3B  }, /* FEH    */
    { 0xFED2,0xC7  }, 
    { 0xFED3,0x5F  }, 
    { 0xFED4,0xA1  }, 
    { 0xFED5,0x3D  }, /* QAF    */
    { 0xFED6,0xC9  }, 
    { 0xFED7,0x61  }, 
    { 0xFEB8,0xA3  }, 
    { 0xFED9,0x3E  }, /* KAF    */
    { 0xFEDA,0xCA  }, 
    { 0xFEDB,0x62  }, 
    { 0xFEDC,0xA4  }, 
    { 0xFEDD,0x41  }, /* LAM    */
    { 0xFEDE,0xCD  }, 
    { 0xFEDF,0x66  }, 
    { 0xFEE0,0xA6  }, 
    { 0xFEE1,0x43  }, /* MEEM   */
    { 0xFEE2,0xCF  }, 
    { 0xFEE3,0x67  }, 
    { 0xFEE4,0xA7  }, 
    { 0xFEE5,0x44  }, /* NOON    */
    { 0xFEE6,0xD0  }, 
    { 0xFEE7,0x68  }, 
    { 0xFEE8,0xA8  }, 
    { 0xFEE9,0x47  }, /* HEH (HA) */
    { 0xFEEA,0xD4  }, 
    { 0xFEEB,0x6B  }, 
    { 0xFEEC,0xAA  }, 
    { 0xFEED,0x46  }, /* WAW     */
    { 0xFEEE,0xD2  }, 
    { 0xFEEF,0x4B  }, /* ALEF MAKSURA */
    { 0xFEF0,0xD6  }, 
    { 0xFEF1,0x4C  },  /* YEH (YA) */
    { 0xFEF2,0xD7  }, 
    { 0xFEF3,0x6D  }, 
    { 0xFEF4,0xAC  }, 


    { 0x0020,0x20  }, /* space */ /* We ought to NOT get these ! */
    { 0x0021,0xEA  }, /* ! */
    { 0x0027,0xEC  }, /* ' */
    { 0x0028,0xED  }, /* ( */
    { 0x0029,0xEE  }, /* ) */
    { 0x002E,0xE7  }, /* . */
    { 0x003A,0xEB  }, /* : */
    { 0x00A0,0xA0  }, /* non breaking space */



    { 0x060C,0xE8  }, /* arabic comma */
    { 0x061F,0xE9  }, /* arabic question mark */
    { 0x0621,0x4A  }, /* hamza */
    { 0x0640,0xE6  }, /* arabic tatweel */
    { 0x064B,0xF8  }, /* arabic fathatan, unshaped */
    { 0x064C,0xF7  }, /* arabic dammatan, unshaped */
    { 0x064D,0xF9  }, /* arabic kasratan, unshaped */
    { 0x064E,0xFE  }, /* arabic fatha, unshaped */
    { 0x064F,0xFD  }, /* araibc damma, unshaped */
    { 0x0650,0xFF  }, /* arabic kasra, unshaped */
    { 0x0651,0xFC  }, /* arabic shadda, unshaped */
    { 0x0652,0xA0  }, /* sukun -- non-existant in the font */
    { 0x0653,0xF3  }, /* arabic madda above, should occur in only one case: upon left-joined Alif */
    { 0x0654,0xF6  }, /* arabic hamza above, unshaped */
    { 0x0655,0xF5  }, /* arabic hamza below, unshaped */

    /* arabic digits */
    { 0x0660,0xE5  }, /* arabic digit 0 */
    { 0x0661,0xD9  }, /* arabic digit 1 */ 
    { 0x0662,0xDA  }, /* arabic digit 2 */
    { 0x0663,0xDB  }, /* arabic digit 3 */
    { 0x0664,0xDC  }, /* arabic digit 4 */
    { 0x0665,0xDE  }, /* arabic digit 5 */
    { 0x0666,0xE0  }, /* arabic digit 6 */
    { 0x0667,0xE1  }, /* arabic digit 7 */
    { 0x0668,0xE3  }, /* arabic digit 8 */
    { 0x0669,0xE4  }, /* arabic digit 9 */
    { 0x0670,0xF4  }, /* arabic percent sign */
    { 0x0679,0x26  }, /* some arabic letters, unshaped */
    { 0x067E,0x24  },
    { 0x0686,0x29  },
    { 0x0688,0x2D  },
    { 0x0691,0x30  },
    { 0x0698,0x32  },
    { 0x06A4,0x3C  },
    { 0x06A9,0x3F  },
    { 0x06AF,0x40  },
    { 0x06BA,0x45  },
    { 0x06BE,0x49  },
    { 0x06C1,0x47  },
    { 0x06CC,0x4B  },
    { 0x06D2,0x4D  },
    { 0x06D4,0xE7  },
    /* persian digits */
    { 0x06F0,0xE5  }, /* persian digit 0 */
    { 0x06F1,0xD9  }, /* persian digit 1 */ 
    { 0x06F2,0xDA  }, /* persian digit 2 */
    { 0x06F3,0xDB  }, /* persian digit 3 */
    { 0x06F4,0xDD  }, /* persian digit 4 */
    { 0x06F5,0xDF  }, /* persian digit 5 */
    { 0x06F6,0xE0  }, /* persian digit 6 */
    { 0x06F7,0xE2  }, /* persian digit 7 */
    { 0x06F8,0xE3  }, /* persian digit 8 */
    { 0x06F9,0xE4  }, /* persian digit 9 */

    /* shaped letters & ligatures */    
    { 0xFB56,0x24  }, /* PEH */
    { 0xFB57,0xAF  }, 
    { 0xFB58,0x4F  },
    { 0xFB59,0x6F  },
    { 0xFB66,0x26  }, /* TTEH */
    { 0xFB67,0xB1  },
    { 0xFB68,0x51  },
    { 0xFB69,0x71  },
    { 0xFB6A,0x3C  }, /* VEH */
    { 0xFB6B,0xC8  },
    { 0xFB6C,0x60  },
    { 0xFB6D,0xA2  },
    { 0xFB7A,0x29  }, /* TCHEH */
    { 0xFB7B,0xB4  },
    { 0xFB7C,0x54  },
    { 0xFB7D,0x74  },
    { 0xFB88,0x2D  }, /* DDAL */
    { 0xFB89,0xB9  },
    { 0xFB8A,0x32  }, /* JEH */
    { 0xFB8B,0xBE  },
    { 0xFB8C,0x30  }, /* RREH */
    { 0xFB8D,0xBC  },
    { 0xFB8E,0x3F  }, /* KEHEH */
    { 0xFB8F,0xCB  },
    { 0xFB90,0x62  },
    { 0xFB90,0x64  },
    { 0xFB91,0xA4  },
    { 0xFB92,0x40  }, /* GAF */
    { 0xFB93,0xCC  },
    { 0xFB94,0x63  },
    { 0xFB94,0x65  },
    { 0xFB95,0xA5  },
    { 0xFB9E,0x45  }, /* NOON GHUNNA */
    { 0xFB9F,0xD1  },
    { 0xFBA6,0x47  }, /* HEH GOAL */
    { 0xFBA7,0xD3  },
    { 0xFBA8,0x69  },
    { 0xFBA8,0x6A  },
    { 0xFBA9,0xA9  },
    { 0xFBAA,0x49  }, /* HEH DOACHASMEE */
    { 0xFBAB,0xAA  },
    { 0xFBAC,0x6B  },
    { 0xFBAD,0xAA  },
    { 0xFBAE,0x4D  }, /* YEH BAREE */
    { 0xFBAF,0xD8  },
    { 0xFBFC,0x4B  }, /* FARSI YEH */
    { 0xFBFD,0xD6  },
    { 0xFBFE,0x6D  },
    { 0xFBFF,0xAC  },
    { 0xFE80,0x4A  }, /* HAMZA */
    { 0xFE81,0x21  }, /* ALEF WITH MADDA ABOVE */ 
    { 0xFE82,0xAD  }, 

    /* fake entries (the font doesn't provide glyphs with the hamza
     * above; so the ones without hamza are used, as a best approach) */
    /* these *should never occur* */
    { 0xFE83,0x22  }, /* ALIF HAMZA   */
    { 0xFE84,0xAD  }, 
    { 0xFE85,0x46  }, /* WAW  HAMZA   */
    { 0xFE86,0xD2  }, 
    { 0xFE87,0x22  }, /* ALIF IHAMZA  */
    { 0xFE88,0xAD  }, 
    { 0xFE89,0x4C  }, /* YA   HAMZA   */
    { 0xFE8A,0xD7  }, 

    { 0xFEFB,0x42  }, /* ligature LAM+ALEF ISOLATED */
    { 0xFEFC,0xCE  }, /* ligature LAM+ALEF FINAL */

    /* I've been unable to map those font glyphs to an unicode value;
     * if you can, please tell me -- Pablo Saratxaga <pablo@mandrakesoft.com>
    { 0x????,0xB7  },
    { 0x????,0xEF  },
    { 0x????,0xF0  },
    { 0x????,0xF1  },
    { 0x????,0xF2  },
    { 0x????,0xFA  },
    { 0x????,0xFB  },
    */

    { 0x0000,0x00 }
};             


void
urdu_naqsh_recode(PangoXSubfont* subfont,gunichar* glyph,int* glyph2,
                   PangoXSubfont* nqfont)
{
    int letter=*glyph;
    int i;
    

    *subfont = nqfont[0];
    
    if ((letter >= 0xFE8B)&&(letter <= 0xFEF4))
        { 
            *glyph   = charmap[letter-0xFE8B].charindex;
        }
    else if (letter < 0xFF )
        { /* recoded in previous run */
            *glyph  = letter;
        }
    else if ((letter >= 0xFEF5)&&(letter <= 0xFEFC))
        { /* Lam-Alif. Also solved interestingly ... 
           * At least, the Lam-Alif itself is not split ...
           */
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
    else 
        {
            for (i=0 ; charmap[i].unicodechar!=0x0000 ; i++) {
                if (charmap[i].unicodechar == letter) {
                    *glyph = charmap[i].charindex;
                    break;
                }
            }
            if (charmap[i].unicodechar == 0x0000) {
                /* if the glyph wasn't on the table */
                *glyph = 0xE5; /* don't know what to do */
            }
        }
}





