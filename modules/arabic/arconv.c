/* This is part of Pango - Arabic shaping module 
 *
 * (C) 2000 Karl Koehler<koehler@or.uni-bonn.de>
 *
 * Note : The book  "The Unicode Standard Version 3.0"  is not very helpful
 *        regarding arabic, so I implemented this according to my own best
 *        knowledge. Bad examples from the book are 'ALEF.LAM'-ligature,
 *        ( one also sais fi-ligature, not if-ligature ) and HAMZA handling.
 *        There is only _one_ letter HAMZA, and so four (!) forms of HAMZA in
 *        the basic section seem .. strange ( maybe I just have not understood
 *        the sense of them, though ).
 */

#include "arconv.h"
/*  #define DEBUG */
#ifdef DEBUG
#include <stdio.h>
#endif

typedef struct {
    GUChar4 basechar;
    GUChar4 charstart;
    int count;
} shapestruct;

typedef struct {
    GUChar4      basechar;
    GUChar4      mark1;  /* has to be initialized to zero */
    GUChar4      vowel;  /*  */
    char         connecttoleft;
    signed char  lignum; /* is a ligature with lignum aditional characters */
    char         numshapes; 
} charstruct;

/* The Unicode order is always: Standalone End Beginning Middle */

static shapestruct chartable [] = 
{
    {0x621,      0xFE80,4}, /* HAMZA; handle seperately !!! */
    {0x622,      0xFE81,2}, /* ALIF MADDA   */
    {0x623,      0xFE83,2}, /* ALIF HAMZA   */
    {0x624,      0xFE85,2}, /* WAW  HAMZA   */
    {0x625,      0xFE87,2}, /* ALIF IHAMZA  */
    {0x626,      0xFE89,4}, /* YA   HAMZA   */
    {0x627,      0xFE8D,2}, /* ALIF         */
    {0x628,      0xFE8F,4}, /* BA           */
    {0x629,      0xFE93,2}, /* TA MARBUTA   */
    {0x62A,      0xFE95,4}, /* TA           */
    {0x62B,      0xFE99,4}, /* THA          */
    {0x62C,      0xFE9D,4}, /* DJIM         */
    {0x62D,      0xFEA1,4}, /* HA           */
    {0x62E,      0xFEA5,4}, /* CHA          */
    {0x62F,      0xFEA9,2}, /* DAL          */
    {0x630,      0xFEAB,2}, /* THAL         */
    {0x631,      0xFEAD,2}, /* RA           */
    {0x632,      0xFEAF,2}, /* ZAY          */
    {0x633,      0xFEB1,4}, /* SIN          */
    {0x634,      0xFEB5,4}, /* SHIN         */
    {0x635,      0xFEB9,4}, /* SAAD         */
    {0x636,      0xFEBD,4}, /* DAAD         */
    {0x637,      0xFEC1,4}, /* .TA          */
    {0x638,      0xFEC5,4}, /* .ZA          */
    {0x639,      0xFEC9,4}, /* AIN          */
    {0x63A,      0xFECD,4}, /* RAIN         */
    {0x63B,      0x0000,0}, /*  :           */
    {0x63C,      0x0000,0}, /* epmty for    */
    {0x63D,      0x0000,0}, /* simple       */
    {0x63E,      0x0000,0}, /* indexing     */
    {0x63F,      0x0000,0}, /*  :           */
    {0x640,      0x0640,1}, /*              */
    {0x641,      0xFED1,4}, /* FA           */
    {0x642,      0xFED5,4}, /* QAF          */
    {0x643,      0xFED9,4}, /* KAF          */
    {0x644,      0xFEDD,4}, /* LAM          */
    {0x645,      0xFEE1,4}, /* MIM          */
    {0x646,      0xFEE5,4}, /* NUN          */
    {0x647,      0xFEE9,4}, /* HA           */
    {0x648,      0xFEED,2}, /* WAW          */
    {0x649,      0xFEEF,2}, /* ALIF MAQSURA */
    {0x64A,      0xFEF1,4}, /* YA          */
    /* The following are letters are not plain arabic  */
    /* some of the coding does not preserve order ... */
    {0x679,      0xFB66,4}, /* Urdu:TTEH                         */
    {0x67B,      0xFB52,4}, /* Sindhi:                           */
    {0x67E,      0xFB56,4}, /* PEH: latin compatibility          */
    {0x680,      0xFB62,4}, /* Sindhi:                           */
    {0x683,      0xFB86,4}, /*     "                             */
    {0x684,      0xFB72,4}, /*     "                             */
    {0x686,      0xFB7A,4}, /* Persian: Tcheh                    */
    {0x687,      0xFB7E,4}, /* Sindhi:                           */
    {0x68C,      0xFB84,2}, /* Sindhi: DAHAL                     */
    {0x68D,      0xFB82,2}, /* Sindhi                            */             
    {0x68E,      0xFB86,2}, /*                                   */
    {0x691,      0xFB8C,2}, /* Urdu                              */             
    {0x698,      0xFB8A,2}, /* Persian: JEH                      */
    {0x6A4,      0xFB6A,4}, /* VEH: latin compatibility          */             
    {0x6A6,      0xFB6E,4}, /* Sindhi                            */             
    {0x6A9,      0xFB8E,4}, /* Persan K                          */
    {0x6AA,      0xFB8E,4}, /* extrawide KAF-> Persian KAF       */             
    {0x6AF,      0xFB92,4}, /* Persian: GAF                      */             
    {0x6B1,      0xFB9A,4}, /* Sindhi:                           */
    {0x6B3,      0xFB97,4}, /*                                   */
    {0x6BA,      0xFB9E,2}, /* Urdu:NUN GHUNNA                   */
    {0x6BB,      0xFBA0,4}, /* Sindhi:                           */             
    {0x6BE,      0xFBAA,4}, /* HA special                        */
    {0x6C0,      0xFBA4,2}, /* izafet: HA HAMZA                  */             
    {0x6C1,      0xFBA6,4}, /* Urdu:                             */
    {0x6D2,      0xFBAE,2}, /* YA barree                         */
    {0x6D3,      0xFBB0,2}, /* YA BARREE HAMZA                   */             

    {0xFEF5,     0xFEF5,2}, /* Lam-Alif Madda  */
    {0xFEF7,     0xFEF7,2}, /* Lam-Alif Hamza  */
    {0xFEF9,     0xFEF9,2}, /* Lam-Alif iHamza */
    {0xFEFB,     0xFEFB,2}  /* Lam-Alif        */
};             
#define ALIF       0x627
#define ALIFHAMZA  0x623
#define ALIFIHAMZA 0x625
#define ALIFMADDA  0x622
#define LAM        0x644
#define HAMZA      0x621

/* Hmaza below ( saves Kasra and special cases ), Hamza above ( always joins ).
 * As I don't know what sHAMZA is good for I don't handle it.
 */
#define iHAMZA      0x654
#define aHAMZA      0x655
#define sHAMZA      0x674

#define WAW        0x648
#define WAWHAMZA   0x624

#define SHADDA     0x651
#define KASRA      0x650
#define FATHA      0x64E
#define DAMMA      0x64F


#define LAM_ALIF        0xFEFB
#define LAM_ALIFHAMZA   0xFEF7
#define LAM_ALIFIHAMZA  0xFEF9
#define LAM_ALIFMADDA   0xFEF5


void 
charstruct_init(charstruct* s)
{
    s->basechar       = 0;
    s->mark1          = 0;
    s->vowel          = 0;  
    s->connecttoleft  = 0;
    s->lignum         = 0;
    s->numshapes      = 0;
}
 
void
copycstostring(GUChar4* string,int* i,charstruct* s,int level)
{ /* s is a shaped charstruct; i is the index into the string */
    if (s->basechar == 0) return;

    string[*i] = s->basechar;  (*i)--; (s->lignum)--;
    if (s->mark1 != 0)
        {     
            string[*i] = s->mark1;  (*i)--; (s->lignum)--;
        }
    if (s->vowel != 0)
        {    
            if (level > 1)
                {
                    string[*i] = s->vowel;  (*i)--; (s->lignum)--;
                }
            else 
                { /* vowel elimination */
                    string[*i] = 0;  (*i)--; (s->lignum)--;
                }
        }
    while (s->lignum > 0 )
        {
            string[*i] = 0;  (*i)--; (s->lignum)--;
        }
#ifdef DEBUG
    if (*i < -1){
        fprintf(stderr,"you are in trouble ! i = %i, the last char is %x, "
                "lignum  = %i",
                *i,s->basechar,s->lignum);
    }
#endif
}

int 
arabic_isvowel(GUChar4 s)
{ /* is this 'joining HAMZA' ( strange but has to be handled like a vowel )
   *  Kasra, Fatha, Damma, Sukun ? 
   */
    if ((s >= 0x64B) && (s <= 0x655)) return 1;
    if ((s >= 0xFC5E) && (s <= 0xFC63)) return 1;
    if ((s >= 0xFE70) && (s <= 0xFE7F)) return 1;
    return 0;
}

static GUChar4 
unshape(GUChar4 s)
{
    int j = 0;
    if ( (s > 0xFE80)  && ( s < 0xFEFF ))
        {   /* arabic shaped Glyph , not HAMZA */
            while ( chartable[j+1].charstart <= s) j++;
            return chartable[j].basechar;
        } 
    else if ((s == 0xFE8B)||(s == 0xFE8C))
        {
            return HAMZA;
        } 
    else 
        {
            return s;
        }
}

static GUChar4 
charshape(GUChar4 s,short which)
{ /* which 0=alone 1=end 2=start 3=middle */
    int j = 0;
    if ((s >= chartable[1].basechar)  && (s <= 0x64A)) 
        {   /* basic character */ 
            return chartable[s-chartable[0].basechar].charstart+which;
        } 
    else if ( (s >= chartable[1].basechar)  && ( s <= 0xFEFB ))
        {   /* special char or  Lam-Alif */
            while ( chartable[j].basechar < s) j++;
            return chartable[j].charstart+which;
        } 
    else if (s == HAMZA)
        {
            if (which < 2) return s;
            else return 0xFE8B+(which-2); /* The Hamza-'pod' */
        } 
    else 
        {
            return s;
        }
}


static short 
shapecount(GUChar4 s)
{
    int j = 0;
    if (arabic_isvowel(s))
        { /* correct trailing wovels */
            return 1;
        } 
    else if ((s >= chartable[1].basechar)  && ( s <= 0x64A ))  
        {   /* basic character */ 
            return chartable[s-chartable[0].basechar].count;
        } 
    else if ( (s >= chartable[0].basechar)  && ( s <= 0xFEFB ))
        { 
            /* arabic base char or ligature */
            while ( chartable[j].basechar < s) j++;
            return chartable[j].count;
        } 
    else 
        {
            return 1;
        }
}

int 
ligature(GUChar4* string,int si,int len,charstruct* oldchar)
{ /* no ligature possible --> return 0; 1 == vowel; 2 = two chars */
    int     retval = 0;
    GUChar4 newchar = string[si];
    if (arabic_isvowel(newchar))
        {
            retval = 1;
            switch(newchar)
                {
                case SHADDA: oldchar->mark1 = newchar; break;
                case iHAMZA: 
                    switch(oldchar->basechar)
                        {
                        case ALIF:
                            oldchar->basechar = ALIFIHAMZA;
                            retval = 2; break;
                        case LAM_ALIF:
                            oldchar->basechar = LAM_ALIFIHAMZA;
                            retval = 2; break;
                        default: oldchar->mark1 = newchar; break;
                        }
                    break;
                case aHAMZA: 
                    switch(oldchar->basechar)
                        {
                        case ALIF:
                            oldchar->basechar = ALIFHAMZA;
                            retval = 2; break;
                        case LAM_ALIF:
                            oldchar->basechar = LAM_ALIFHAMZA;
                            retval = 2; break;
                        case WAW:
                            oldchar->basechar = WAWHAMZA;
                            retval = 2; break;
                        default: /* whatever sense this may make .. */
                            oldchar->mark1 = newchar; break;
                        }
                    break;
                case KASRA:  
                    switch(oldchar->basechar)
                        {
                        case ALIFHAMZA:
                            oldchar->basechar = ALIFIHAMZA;
                            retval = 2; break;
                        case LAM_ALIFHAMZA:
                            oldchar->basechar = LAM_ALIFIHAMZA;
                            retval = 2; break;
                        default: oldchar->vowel = newchar; break;
                        }
                    break;
                default: oldchar->vowel = newchar; break;
                }
            oldchar->lignum++;
            return retval;
        }
    if (oldchar->vowel != 0)
        { /* if we already joined a vowel, we can't join a Hamza */
            return 0;
        }

    switch(oldchar->basechar)
        {
        case LAM:
            switch (newchar)
                {
                case ALIF:      oldchar->basechar = LAM_ALIF; 
                    oldchar->numshapes = 2;  retval = 3; break;
                case ALIFHAMZA: oldchar->basechar = LAM_ALIFHAMZA; 
                    oldchar->numshapes = 2;  retval = 3; break;
                case ALIFIHAMZA:oldchar->basechar = LAM_ALIFIHAMZA;
                    oldchar->numshapes = 2;  retval = 3; break;
                case ALIFMADDA: oldchar->basechar = LAM_ALIFMADDA ;
                    oldchar->numshapes = 2;  retval = 3; break;
                }
            break;
        case ALIF:
            switch (newchar)
                {
                case ALIF: oldchar->basechar = ALIFMADDA; retval = 2; break;
                case HAMZA:
                    if (si == len-2) /* HAMZA is 2nd char */
                        {
                            oldchar->basechar = ALIFHAMZA; retval = 2; 
                        }
                    break;
                }       
            break;
        case WAW:
            switch (newchar)
                {
                case HAMZA:oldchar->basechar = WAWHAMZA; retval = 2; break;
                }
            break;
        case LAM_ALIF:
            switch (newchar)
                {
                case HAMZA:
                    if (si == len-4) /* ! We assume the string has been split
                                        into words. This is AL-A.. I hope */
                        {
                            oldchar->basechar = LAM_ALIFHAMZA; retval = 2;
                        }
                    break;
                }
            break;
        case 0:
            oldchar->basechar  = newchar;
            oldchar->numshapes = shapecount(newchar);
            retval = 1;
            break;
        }
    if (retval)
        {
            oldchar->lignum++;
#ifdef DEBUG
            fprintf(stderr,"[ar]   ligature : added %x to make %x\n",
                    newchar,oldchar->basechar);
#endif      
        }
    return retval;
}

static void 
shape(int olen,int* len,GUChar4* string,int level)
{
    /* The string must be in visual order already.
    ** This routine does the basic arabic reshaping.
    ** olen is the memory lenght, *len the number of non-null characters.
    */
    charstruct oldchar,curchar;
    int        si  = (olen)-1;
    int        j   = (olen)-1;
    int        join;
    int        which;

    *len = olen;
    charstruct_init(&oldchar);
    charstruct_init(&curchar);
    while (si >= 0)
        {
            join = ligature(string,si,olen,&curchar);
            if (!join)
                { /* shape curchar */
                    int nc = shapecount(string[si]);
                    if (nc == 1)
                        {
                            which = 0; /* end  or basic */
                        } 
                    else 
                        { 
                            which = 2; /* middle or beginning */
                        }
                    if (oldchar.connecttoleft) which++; 
                    which = which % (curchar.numshapes);
                    curchar.basechar = charshape(curchar.basechar,which);
                    if (curchar.numshapes > 2)
                        curchar.connecttoleft = 1;

                    /* get rid of oldchar */
                    copycstostring(string,&j,&oldchar,level);
                    oldchar = curchar; /* new vlues in oldchar */

                    /* init new curchar */
                    charstruct_init(&curchar);
                    curchar.basechar  = string[si];
                    curchar.numshapes = nc;
                    curchar.lignum++;
                }
            else if ( ( join == 2 )
		      ||((join == 3)&&(level != 2))  ) 
                { /*  Lam-Alif in Langbox-font is no ligature */
                    (*len)--;
                }
            si--;
        }
    /* Handle last char */
    
    if (oldchar.connecttoleft) 
        which = 1;
    else 
        which = 0;
    which = which % (curchar.numshapes);
    curchar.basechar = charshape(curchar.basechar,which);
    /* get rid of oldchar */
    copycstostring(string,&j,&oldchar,level);
    copycstostring(string,&j,&curchar,level);
#ifdef DEBUG
    fprintf(stderr,"[ar] shape statistic: %i chars -> %i glyphs \n",
            olen,*len);
#endif
}

static void 
doublelig(int olen,int* len,GUChar4* string,int level)
{ /* Ok. We have presentation ligatures in our font. */
    int        si  = (olen)-1;
    GUChar4    lapresult;

    while (si > 0)
        {
            lapresult = 0;
            switch(string[si])
                {
                case SHADDA:
                    switch(string[si-1])
                        {
                        case KASRA: lapresult = 0xFC62; break;
                        case FATHA: lapresult = 0xFC60; break;
                        case DAMMA: lapresult = 0xFC61; break;
                        case 0x64C: lapresult = 0xFC5E; break;
                        case 0x64D: lapresult = 0xFC5F; break;
                        }
                    break;
                case KASRA:
                    if (string[si-1]==SHADDA) lapresult = 0xFC62;
                    break;
                case FATHA:
                    if (string[si-1]==SHADDA) lapresult = 0xFC60;
                    break;
                case DAMMA:
                    if (string[si-1]==SHADDA) lapresult = 0xFC61;
                    break;
                case 0xFEDF: /* LAM initial */
                    if (level > 13){
                        switch(string[si-1]){
                        case 0xFE9E : lapresult = 0xFC3F; break; /* DJEEM final*/
                        case 0xFEA0 : lapresult = 0xFCC9; break;
                        case 0xFEA2 : lapresult = 0xFC40; break; /* .HA final */ 
                        case 0xFEA4 : lapresult = 0xFCCA; break;
                        case 0xFEA6 : lapresult = 0xFCF1; break; /* CHA final */
                        case 0xFEA8 : lapresult = 0xFCCB; break;
                        case 0xFEE2 : lapresult = 0xFC42; break; /* MIM final */
                        case 0xFEE4 : lapresult = 0xFCCC; break;
                        }
                    }
                    break;
                case 0xFE97: /* TA inital */
                    if (level > 13){
                        switch(string[si-1]){
                        case 0xFEA0 : lapresult = 0xFCA1; break; /* DJ init */
                        case 0xFEA4 : lapresult = 0xFCA2; break; /* .HA */
                        case 0xFEA8 : lapresult = 0xFCA3; break; /* CHA */
                        }
                    }
                    break;
                case 0xFE91: /* BA inital */
                    if (level > 13){
                        switch(string[si-1]){
                        case 0xFEA0 : lapresult = 0xFC9C; break; /* DJ init */
                        case 0xFEA4 : lapresult = 0xFC9D; break; /* .HA */
                        case 0xFEA8 : lapresult = 0xFC9E; break; /* CHA */
                        }
                    }
                    break;
                case 0xFEE7: /* NUN inital */
                    if (level > 13) {
                        switch(string[si-1]){
                        case 0xFEA0 : lapresult = 0xFCD2; break; /* DJ init */
                        case 0xFEA4 : lapresult = 0xFCD3; break; /* .HA */
                        case 0xFEA8 : lapresult = 0xFCD4; break; /* CHA */
                        }
                    }
                    break;
                default:
                    break;
                } /* end switch string[si] */
            if (lapresult != 0)
                {
                    string[si] = lapresult; (*len)--; string[si-1] = 0x0;
                }
            si--;
        }    
}

void 
arabic_reshape(int* len,GUChar4* string,int level)
{
    int i;
    int olen = *len;
    for ( i = 0; i < *len; i++){
        string[i] = unshape(string[i]);
    }
    shape(olen,len,string,level);
    if (level > 10)
        doublelig(olen,len,string,level);
}



