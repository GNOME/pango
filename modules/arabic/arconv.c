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
/*  #define DEBUG  */
#ifdef DEBUG
#include <stdio.h>
#endif

typedef struct {
    gunichar basechar;
    gunichar charstart;
    int count;
} shapestruct;

typedef struct {
    gunichar      basechar;
    gunichar      mark1;  /* has to be initialized to zero */
    gunichar      vowel;  /*  */
    char         connecttoleft;
    signed char  lignum; /* is a ligature with lignum aditional characters */
    char         numshapes; 
} charstruct;

/* The Unicode order is always: Standalone End Beginning Middle */

static shapestruct chartable [] = 
{
    {0x621,      0xFE80,1}, /* HAMZA; handle seperately !!! */
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
    {0x640,      0x0640,4}, /* tatweel      */
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
    {0x6CC,      0xFBFC,4}, /* farsi ya                          */
    {0x6C0,      0xFBA4,2}, /* izafet: HA HAMZA                  */
    {0x6C1,      0xFBA6,4}, /* Urdu:                             */
    {0x6D2,      0xFBAE,2}, /* YA barree                         */
    {0x6D3,      0xFBB0,2}, /* YA BARREE HAMZA                   */

    {0x200D,     0x200D,4}, /* Zero-width joiner */
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
#define TATWEEL    0x640
#define JOINER    0x200D

/* Hamza below ( saves Kasra and special cases ), Hamza above ( always joins ).
 * As I don't know what sHAMZA is good for I don't handle it.
 */
#define aHAMZA      0x654
#define iHAMZA      0x655
#define sHAMZA      0x674

#define WAW        0x648
#define WAWHAMZA   0x624

#define SHADDA     0x651
#define KASRA      0x650
#define FATHA      0x64E
#define DAMMA      0x64F
#define MADDA      0x653


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
    s->numshapes      = 1;
}
 
void
copycstostring(gunichar* string,int* i,charstruct* s,arabic_level level)
{ /* s is a shaped charstruct; i is the index into the string */
    if (s->basechar == 0) return;

    string[*i] = s->basechar;  (*i)++; (s->lignum)--;
    if (s->mark1 != 0)
        {     
            if ( !(level & ar_novowel) )
                {
                    string[*i] = s->mark1;  (*i)++; (s->lignum)--;
                } 
            else 
                {
                    (s->lignum)--;
                }
        }
    if (s->vowel != 0)
        {    
            if (! (level & ar_novowel) )
                {
                    string[*i] = s->vowel;  (*i)++; (s->lignum)--;
                }
            else 
                { /* vowel elimination */ 
                    (s->lignum)--;
                }
        }
    while (s->lignum  > 0 )
        { /* NULL-insertion for Langbox-font */
            string[*i] = 0;  (*i)++; (s->lignum)--;
        }
}

int 
arabic_isvowel(gunichar s)
{ /* is this 'joining HAMZA' ( strange but has to be handled like a vowel )
   *  Kasra, Fatha, Damma, Sukun ? 
   */
    if ((s >= 0x64B) && (s <= 0x655)) return 1;
    if ((s >= 0xFC5E) && (s <= 0xFC63)) return 1;
    if ((s >= 0xFE70) && (s <= 0xFE7F)) return 1;
    return 0;
}

static gunichar 
unshape(gunichar s)
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

static gunichar 
charshape(gunichar s,short which)
{ /* which 0=alone 1=end 2=start 3=middle */
    int j = 0;
    if ((s >= chartable[1].basechar)  && (s <= 0x64A) && ( s != TATWEEL) && ( s != JOINER)) 
        {   /* basic character */ 
            return chartable[s-chartable[0].basechar].charstart+which;
        } 
    else if ( (s >= chartable[1].basechar)  && ( s <= 0xFEFB ) 
              && (s != TATWEEL) && ( s != JOINER) && ( s!= 0x6CC))
        {   /* special char or  Lam-Alif */
            while ( chartable[j].basechar < s) j++;
            return chartable[j].charstart+which;
        } 
    else if (s == HAMZA)
        {
            if (which < 2) return s;
            else return 0xFE8B+(which-2); /* The Hamza-'pod' */
        } 
    else if (s == 0x6CC)
        { /* farsi ya --> map to Alif maqsura and Ya, depending on form */
            switch (which){
            case 0: return 0xFEEF; 
            case 1: return 0xFEF0;
            case 2: return 0xFEF3;
            case 3: return 0xFEF4;
            }
        }
    else 
        {
            return s;
        }
}


static short 
shapecount(gunichar s)
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

int unligature(charstruct* curchar,arabic_level level)
{
    int result = 0;
    if (level & ar_naqshfont){
        /* decompose Alif-Madda ... */
        switch(curchar->basechar){
        case ALIFHAMZA : curchar->basechar = ALIF; curchar->mark1 = aHAMZA; 
            result++; break;
        case ALIFIHAMZA: curchar->basechar = ALIF; curchar->mark1 = iHAMZA; 
            result++; break;
        case WAWHAMZA  : curchar->basechar = WAW; curchar->mark1 = aHAMZA; 
            result++; break;
        case ALIFMADDA :curchar->basechar = ALIF; curchar->vowel = MADDA; 
            result++; break;
        }
    }    
    return result;
}

int 
ligature(gunichar newchar,charstruct* oldchar)
{ /* no ligature possible --> return 0; 1 == vowel; 2 = two chars 
   * 3 = Lam-Alif
   */
    int     retval = 0;

    if (!(oldchar->basechar)) return 0;
    if (arabic_isvowel(newchar))
        {
            retval = 1;
            if ((oldchar->vowel != 0)&&(newchar != SHADDA)){
                retval = 2; /* we eliminate the old vowel .. */
            }
            switch(newchar)
                {
                case SHADDA: 
                    if (oldchar->mark1 == 0)
                        {
                            oldchar->mark1 = newchar; 
                        }
                    else 
                        {
                            return 0; /* no ligature possible */
                        }
                    break;
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
                case MADDA:
                    switch(oldchar->basechar)
                        {
                        case ALIFHAMZA:
                        case ALIF:
                            oldchar->basechar = ALIFMADDA;
                            retval = 2; break;
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
            if (retval == 1) 
                { 
                    oldchar->lignum++; 
                }
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
                }       
            break;
        case WAW:
            switch (newchar)
                {
                case HAMZA:oldchar->basechar = WAWHAMZA; retval = 2; break;
                }
            break;
        case 0:
            oldchar->basechar  = newchar;
            oldchar->numshapes = shapecount(newchar);
            retval = 1;
            break;
        }
    return retval;
}

static void 
shape(int* len,const char* text,gunichar* string,arabic_level level)
{
    /* string is assumed to be empty an big enough.
    ** text is the original text.
    ** This routine does the basic arabic reshaping.
    ** *len the number of non-null characters.
    */
    /* Note ! we have to unshape each character first ! */
    int         olen = *len;
    charstruct  oldchar,curchar;
    /*      int         si  = (olen)-1; */
    int         j   = 0;
    int         join;
    int         which;
    gunichar    nextletter;
    const char* p   = text;

    *len = 0 ; /* initialize for output */
    charstruct_init(&oldchar);
    charstruct_init(&curchar);
    while (p < text+olen)
        {
            nextletter = g_utf8_get_char (p);
            nextletter = unshape(nextletter);
            
            join = ligature(nextletter,&curchar);
            if (!join)
                { /* shape curchar */
                    int nc = shapecount(nextletter);
                    (*len)++;
                    if (nc == 1)
                        {
                            which = 0; /* end  or basic */
                        } 
                    else 
                        { 
                            which = 2; /* middle or beginning */
                        }
                    if (oldchar.connecttoleft)
                        {
                            which++; 
                        }
                    else if (curchar.basechar == HAMZA)
                        { /* normally, a Hamza hangs loose after an Alif.
                           *  Use the form Ya-Hamza if you want a Hamza
                           *  on a pod !
                           */
                            curchar.numshapes = 1;
                        }

                    which = which % (curchar.numshapes);
                    curchar.basechar = charshape(curchar.basechar,which);
                    if (curchar.numshapes > 2)
                        curchar.connecttoleft = 1;

                    /* get rid of oldchar */
                    copycstostring(string,&j,&oldchar,level);
                    oldchar = curchar; /* new vlues in oldchar */

                    /* init new curchar */
                    charstruct_init(&curchar);
                    curchar.basechar  = nextletter;
                    curchar.numshapes = nc;
                    curchar.lignum++;
                    (*len) += unligature(&curchar,level);
                }
            else if ((join == 3)&&(level & ar_lboxfont))
                { /* Lam-Alif extra in langbox-font */
                    (*len)++;
                    curchar.lignum++;
                }
            else if (join == 1)
                {
                    (*len)++;
                }
            else 
                {
                    (*len) += unligature(&curchar,level);
                }
            p = g_utf8_next_char (p);
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
doublelig(int* len,gunichar* string,arabic_level level)
{ /* Ok. We have presentation ligatures in our font. */
    int         olen = *len;
    int         j = 0, si = 1;
    gunichar    lapresult;

    
    while (si < olen)
        {
            lapresult = 0;
            if ( level & ar_composedtashkeel ){
                switch(string[j])
                    {
                    case SHADDA:
                        switch(string[si])
                            {
                            case KASRA: lapresult = 0xFC62; break;
                            case FATHA: lapresult = 0xFC60; break;
                            case DAMMA: lapresult = 0xFC61; break;
                            case 0x64C: lapresult = 0xFC5E; break;
                            case 0x64D: lapresult = 0xFC5F; break;
                            }
                        break;
                    case KASRA:
                        if (string[si]==SHADDA) lapresult = 0xFC62;
                        break;
                    case FATHA:
                        if (string[si]==SHADDA) lapresult = 0xFC60;
                        break;
                    case DAMMA:
                        if (string[si]==SHADDA) lapresult = 0xFC61;
                        break;
                    }
            }

            if ( level & ar_lig ){
                switch(string[j])
                    {
                    case 0xFEDF: /* LAM initial */
                        switch(string[si]){
                        case 0xFE9E : lapresult = 0xFC3F; break; /* DJEEM final*/
                        case 0xFEA0 : lapresult = 0xFCC9; break;
                        case 0xFEA2 : lapresult = 0xFC40; break; /* .HA final */ 
                        case 0xFEA4 : lapresult = 0xFCCA; break;
                        case 0xFEA6 : lapresult = 0xFCF1; break; /* CHA final */
                        case 0xFEA8 : lapresult = 0xFCCB; break;
                        case 0xFEE2 : lapresult = 0xFC42; break; /* MIM final */
                        case 0xFEE4 : lapresult = 0xFCCC; break;
                        }
                        break;
                    case 0xFE97: /* TA inital */
                        switch(string[si]){
                        case 0xFEA0 : lapresult = 0xFCA1; break; /* DJ init */
                        case 0xFEA4 : lapresult = 0xFCA2; break; /* .HA */
                        case 0xFEA8 : lapresult = 0xFCA3; break; /* CHA */
                        }
                        break;
                    case 0xFE91: /* BA inital */
                        switch(string[si]){
                        case 0xFEA0 : lapresult = 0xFC9C; break; /* DJ init */
                        case 0xFEA4 : lapresult = 0xFC9D; break; /* .HA */
                        case 0xFEA8 : lapresult = 0xFC9E; break; /* CHA */
                        }
                        break;
                    case 0xFEE7: /* NUN inital */
                        switch(string[si]){
                        case 0xFEA0 : lapresult = 0xFCD2; break; /* DJ init */
                        case 0xFEA4 : lapresult = 0xFCD3; break; /* .HA */
                        case 0xFEA8 : lapresult = 0xFCD4; break; /* CHA */
                        }
                        break;

                    case 0xFEE8: /* NUN medial */
                        switch(string[si]){
                            /* missing : nun-ra : FC8A und nun-sai : FC8B */
                        case 0xFEAE : lapresult = 0xFC8A; break; /* nun-ra  */
                        case 0xFEB0 : lapresult = 0xFC8B; break; /* nun-sai */
                        }
                        break;
                    case 0xFEE3: /* Mim initial */
                        switch(string[si]){
                        case 0xFEA0 : lapresult = 0xFCCE ; break; /* DJ init */
                        case 0xFEA4 : lapresult = 0xFCCF ; break; /* .HA init */
                        case 0xFEA8 : lapresult = 0xFCD0 ; break; /* CHA init */
                        case 0xFEE4 : lapresult = 0xFCD1 ; break; /* Mim init */
                        }
                        break;
                        
                    case 0xFED3: /* Fa initial */
                        switch(string[si]){
                        case 0xFEF2 : lapresult = 0xFC32 ; break; /* fi-ligature (!) */ 
                        }
                        break;

                    default:
                        break;
                    } /* end switch string[si] */
            }
            if (lapresult != 0)
                {
                    string[j] = lapresult; (*len)--; 
                    si++; /* jump over one character */
                    /* we'll have to change this, too. */
                }
            else 
                {
                    j++;
                    string[j] = string[si];
                    si++;
                }
        }    
}

void 
arabic_reshape(int* len,const char* text,gunichar* string,arabic_level level)
{
    shape(len,text ,string,level);
    if ( level & ( ar_composedtashkeel | ar_lig ) )
        doublelig(len,string,level);
}



