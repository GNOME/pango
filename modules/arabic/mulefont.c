/* pango-arabic module 
 * 
 * (C) 2000 K. Koehler  <koehler@or.uni-bonn.de>
 *
 * This file provides a mapping unicode <- mulefont
 */


#include <stdio.h>
#include <glib.h>
#include "pangox.h"


/*  #define DEBUG   */
#ifdef DEBUG
#include <stdio.h>
#endif
#include "mulefont.h"

ArabicFontInfo*
arabic_muleinit(PangoFont  *font)
{ 
    static char *mule_charsets0[] = {
        "mulearabic-0",
    };

    static char *mule_charsets1[] = {
        "mulearabic-1",
    };

    static char *mule_charsets2[] = {
        "mulearabic-2",
    };

    ArabicFontInfo *fs = NULL;
    PangoXSubfont  *subfonts;
    int            *subfont_charsets;
    int             n_subfonts;
    PangoXSubfont   mulefonts[3];

    n_subfonts = pango_x_list_subfonts (font,mule_charsets0, 
                                        1, &subfonts, &subfont_charsets);
    if (n_subfonts > 0)
        {
            mulefonts[0] = subfonts[0];
        }
    g_free (subfonts);
    g_free (subfont_charsets);

    n_subfonts = pango_x_list_subfonts (font,mule_charsets1, 
                                        1, &subfonts, &subfont_charsets);
    if (n_subfonts > 0)
        {
            mulefonts[1] = subfonts[0];
        }
    g_free (subfonts);
    g_free (subfont_charsets);

    n_subfonts = pango_x_list_subfonts (font,mule_charsets2, 
                                        1, &subfonts, &subfont_charsets);
    if (n_subfonts > 0)
        {
            mulefonts[2] = subfonts[0];
        }
    g_free (subfonts);
    g_free (subfont_charsets);

    if (( mulefonts[0] != 0)&&(mulefonts[1] != 0)&&(mulefonts[2] != 0))
        {
            fs              = g_new (ArabicFontInfo,1);
            fs->level       = ar_novowel | ar_mulefont;
            fs->subfonts[0] = mulefonts[0];
            fs->subfonts[1] = mulefonts[1];
            fs->subfonts[2] = mulefonts[2];
        }
    return fs;
}


typedef struct {
    gunichar  unicodechar;
    int       fontindex;
    int       charindex;
} fontentry;


static fontentry charmap [] = 
{
    { 0xFE80,1,0x2d }, /* HAMZA; handle seperately !!! */
    { 0xFE81,1,0x2e  }, /* ALIF MADDA   */
    { 0xFE82,1,0x2f  }, 
    { 0xFE83,1,0x30  }, /* ALIF HAMZA   */
    { 0xFE84,1,0x31  }, 
    { 0xFE85,1,0x32  }, /* WAW  HAMZA   */
    { 0xFE86,1,0x33  }, 
    { 0xFE87,1,0x34  }, /* ALIF IHAMZA  */
    { 0xFE88,1,0x35  }, 
    { 0xFE89,2,0x21  }, /* YA   HAMZA   */
    { 0xFE8A,2,0x22  }, 
    { 0xFE8B,1,0x36  }, /* HMAZA-'pod' */ 
    { 0xFE8C,1,0x37  }, 
    { 0xFE8D,1,0x38  }, /* ALIF         */
    { 0xFE8E,1,0x39  }, 
    { 0xFE8F,2,0x23  }, /* BA               */
    { 0xFE90,2,0x24  }, 
    { 0xFE91,1,0x3A  }, 
    { 0xFE92,1,0x3B  }, 
    { 0xFE93,1,0x3C  }, /* TA MARBUTA       */
    { 0xFE94,1,0x3D  }, 
    { 0xFE95,2,0x25  }, /* TA               */
    { 0xFE96,2,0x26  }, 
    { 0xFE97,1,0x3E  }, 
    { 0xFE98,1,0x3F  }, 
    { 0xFE99,2,0x27  }, /* THA      */
    { 0xFE9A,2,0x28  }, 
    { 0xFE9B,1,0x40  }, 
    { 0xFE9C,1,0x41  }, 
    { 0xFE9D,2,0x29  }, /* DJIM         */
    { 0xFE9E,2,0x2C  }, 
    { 0xFE9F,2,0x2A  }, 
    { 0xFEA0,2,0x2B  }, 
    { 0xFEA1,2,0x2D  }, /* .HA       */
    { 0xFEA2,2,0x30  }, 
    { 0xFEA3,2,0x2E  }, 
    { 0xFEA4,2,0x2F  }, 
    { 0xFEA5,2,0x31  }, /* CHA      */
    { 0xFEA6,2,0x34  }, 
    { 0xFEA7,2,0x32  }, 
    { 0xFEA8,2,0x33  }, 
    { 0xFEA9,1,0x42  }, /* DAL      */
    { 0xFEAA,1,0x43  }, 
    { 0xFEAB,1,0x44  }, /* THAL     */
    { 0xFEAC,1,0x45  }, 
    { 0xFEAD,1,0x46  }, /* RA           */
    { 0xFEAE,1,0x47  }, 
    { 0xFEAF,1,0x48  }, /* ZAY      */
    { 0xFEB0,1,0x49  }, 
    { 0xFEB1,2,0x35  }, /* SIN      */
    { 0xFEB2,2,0x38  }, 
    { 0xFEB3,2,0x36  }, 
    { 0xFEB4,2,0x37  }, 
    { 0xFEB5,2,0x39  }, /* SHIN     */
    { 0xFEB2,2,0x3C  }, 
    { 0xFEB3,2,0x3A  }, 
    { 0xFEB4,2,0x3B  }, 
    { 0xFEB9,2,0x3D  }, /* SAAD     */
    { 0xFEBA,2,0x40  }, 
    { 0xFEBB,2,0x3E  }, 
    { 0xFEBC,2,0x3F  }, 
    { 0xFEBD,2,0x41  }, /* DAAD     */
    { 0xFEBE,2,0x44  }, 
    { 0xFEBF,2,0x42  }, 
    { 0xFEC0,2,0x43  }, 
    { 0xFEC1,2,0x45  }, /* .TA      */
    { 0xFEC2,2,0x48  }, 
    { 0xFEC3,2,0x46  }, 
    { 0xFEC4,2,0x47  }, 
    { 0xFEC5,2,0x49  }, /* .ZA      */
    { 0xFEC6,2,0x4C  }, 
    { 0xFEC7,2,0x4A  }, 
    { 0xFEC8,2,0x4B  }, 
    { 0xFEC9,2,0x4D  }, /* AIN      */
    { 0xFECA,2,0x4E  }, 
    { 0xFECB,1,0x4A  }, 
    { 0xFECC,1,0x4B  }, 
    { 0xFECD,2,0x4F  }, /* RAIN     */
    { 0xFECE,2,0x50  }, 
    { 0xFECF,1,0x4C  }, 
    { 0xFED0,1,0x4D  }, 
    { 0xFED1,2,0x51  }, /* FA               */
    { 0xFED2,2,0x52  }, 
    { 0xFED3,1,0x4E  }, 
    { 0xFED4,1,0x4F  }, 
    { 0xFED5,2,0x53  }, /* QAF      */
    { 0xFED6,2,0x54  }, 
    { 0xFED7,1,0x50  }, 
    { 0xFEB8,1,0x51  }, 
    { 0xFED9,2,0x55  }, /* KAF      */
    { 0xFEDA,2,0x58  }, 
    { 0xFEDB,2,0x56  }, 
    { 0xFEDC,2,0x57  }, 
    { 0xFEDD,2,0x59  }, /* LAM      */
    { 0xFEDE,2,0x5A  }, 
    { 0xFEDF,1,0x52  }, 
    { 0xFEE0,1,0x53  }, 
    { 0xFEE1,1,0x54  }, /* MIM      */
    { 0xFEE2,1,0x57  }, 
    { 0xFEE3,1,0x55  }, 
    { 0xFEE4,1,0x56  }, 
    { 0xFEE5,2,0x5B  }, /* NUN          */
    { 0xFEE6,2,0x5C  }, 
    { 0xFEE7,1,0x58  }, 
    { 0xFEE8,1,0x59  }, 
    { 0xFEE9,1,0x5A  }, /* HA           */
    { 0xFEEA,1,0x5D  }, 
    { 0xFEEB,1,0x5B  }, 
    { 0xFEEC,1,0x5C  }, 
    { 0xFEED,1,0x5E  }, /* WAW          */
    { 0xFEEE,1,0x5F  }, 
    { 0xFEEF,2,0x5D  }, /* ALIF MAQSORA */
    { 0xFEF0,2,0x5E  }, 
    { 0xFEF1,2,0x5F  },  /* YA           */
    { 0xFEF2,2,0x60  }, 
    { 0xFEF3,1,0x60  }, 
    { 0xFEF4,1,0x61  }, 
    { 0xFEF5,1,0x62  },  /* Lam-Alif Madda */
    { 0xFEF6,2,0x61  }, 
    { 0xFEF7,1,0x63  },  /* Lam-Alif Hamza*/
    { 0xFEF8,2,0x62  }, 
    { 0xFEF9,1,0x64  },  /* Lam-Alif iHamza*/
    { 0xFEFA,2,0x63  }, 
    { 0xFEFB,1,0x65  },   /* Lam-Alif */
    { 0xFEFC,2,0x64  } 
};             

void 
arabic_mule_recode(PangoXSubfont* subfont,gunichar* glyph,PangoXSubfont* mulefonts)
{
    int letter=*glyph;
    if ((letter >= 0x660)&&(letter <= 0x669)) /* indic numeral */
        {
            *subfont = mulefonts[0];
            *glyph   = letter - 0x660 + 0x21;
        }
    else if ((letter >= 0xFE80)&&(letter <= 0xFEFC))
        { /* now we have a mess ... a big mess ... */
            /* The mule 'idea' is that "wide" forms are in the -2 font, whereas
             * narrow one are in the -1 font.
             * to conserve space, the characters are all ordered in a big lump.
             * emacs can't handle it ...
             */
#ifdef DEBUG
            if (charmap[letter-0xFE80].unicodechar != letter)
                {
                    fprintf(stderr,"[ar] mulefont charmap table defect "
                            "%x comes out as %x ",
                            letter,charmap[letter-0xFE80].unicodechar);
                }
#endif
            *subfont = mulefonts[charmap[letter-0xFE80].fontindex];
            *glyph   = charmap[letter-0xFE80].charindex;
        }
    else if (letter == 0x621)
        {
            *subfont = mulefonts[charmap[0].fontindex];
            *glyph   = charmap[0].charindex;
        }
    else if (letter == 0x61F)
        { /* question mark */
            *subfont = mulefonts[1];
            *glyph   = 0x29; 
        }
    else
        {
	    switch(letter){
	      /* Gaf */
	    case 0xFB92: *subfont = mulefonts[2]; *glyph = 0x6B; break;
	    case 0xFB93: *subfont = mulefonts[2]; *glyph = 0x6E; break;
	    case 0xFB94: *subfont = mulefonts[2]; *glyph = 0x6C; break;
	    case 0xFB95: *subfont = mulefonts[2]; *glyph = 0x6D; break;
	      /* persian Kaf -- the first to forms are wrong ... */
	    case 0xFB8E: *subfont = mulefonts[2]; *glyph = 0x55; break;
	    case 0xFB8F: *subfont = mulefonts[2]; *glyph = 0x58; break;
	    case 0xFB90: *subfont = mulefonts[2]; *glyph = 0x56; break;
	    case 0xFB91: *subfont = mulefonts[2]; *glyph = 0x57; break;
	      /* Tcheh */
	    case 0xFB7A: *subfont = mulefonts[2]; *glyph = 0x67; break;
	    case 0xFB7B: *subfont = mulefonts[2]; *glyph = 0x6A; break;
	    case 0xFB7C: *subfont = mulefonts[2]; *glyph = 0x68; break;
	    case 0xFB7D: *subfont = mulefonts[2]; *glyph = 0x69; break;
	      /* Pe */
	    case 0xFB56: *subfont = mulefonts[2]; *glyph = 0x65; break;
	    case 0xFB57: *subfont = mulefonts[2]; *glyph = 0x66; break;
	    case 0xFB58: *subfont = mulefonts[1]; *glyph = 0x66; break;
	    case 0xFB59: *subfont = mulefonts[1]; *glyph = 0x67; break;
		/* farsi Jeh */
	    case 0xFBFC: *subfont = mulefonts[2]; *glyph = 0x5D; break;
            case 0xFBFD: *subfont = mulefonts[2]; *glyph = 0x5E; break; 
	    case 0xFBFE: *subfont = mulefonts[1]; *glyph = 0x60; break;
	    case 0xFBFF: *subfont = mulefonts[1]; *glyph = 0x61; break;
	      /* extra */
	    case 0xFB8A: *subfont = mulefonts[1]; *glyph = 0x68; break;
	    case 0xFB8B: *subfont = mulefonts[1]; *glyph = 0x69; break;

	    default:
		*subfont = mulefonts[1];
		*glyph   = 0x26; /* we don't have this thing -- use a dot */
		break;
	    }
        }
}




