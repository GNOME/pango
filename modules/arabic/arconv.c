
#include "arconv.h"

/* This belongs to my arabic-shaping-module */
typedef struct {
  GUChar4 basechar;
  GUChar4 charstart;
  int count;
} shapestruct;

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
  {0x628,      0xFE8F,4}, /* BA		  */
  {0x629,      0xFE93,2}, /* TA MARBUTA	  */
  {0x62A,      0xFE95,4}, /* TA		  */
  {0x62B,      0xFE99,4}, /* THA	  */
  {0x62C,      0xFE9D,4}, /* DJIM         */
  {0x62D,      0xFEA1,4}, /* HA 	  */
  {0x62E,      0xFEA5,4}, /* CHA	  */
  {0x62F,      0xFEA9,2}, /* DAL	  */
  {0x630,      0xFEAB,2}, /* THAL	  */
  {0x631,      0xFEAD,2}, /* RA           */
  {0x632,      0xFEAF,2}, /* ZAY	  */
  {0x633,      0xFEB1,4}, /* SIN	  */
  {0x634,      0xFEB5,4}, /* SHIN	  */
  {0x635,      0xFEB9,4}, /* SAAD	  */
  {0x636,      0xFEBD,4}, /* DAAD	  */
  {0x637,      0xFEC1,4}, /* .TA	  */
  {0x638,      0xFEC5,4}, /* .ZA	  */
  {0x639,      0xFEC9,4}, /* AIN	  */
  {0x63A,      0xFECD,4}, /* RAIN	  */
  {0x641,      0xFED1,4}, /* FA		  */
  {0x642,      0xFED5,4}, /* QAF	  */
  {0x643,      0xFED9,4}, /* KAF	  */
  {0x644,      0xFEDD,4}, /* LAM	  */
  {0x645,      0xFEE1,4}, /* MIM	  */
  {0x646,      0xFEE5,4}, /* NUN          */
  {0x647,      0xFEE9,4}, /* HA           */
  {0x648,      0xFEED,2}, /* WAW          */
  {0x649,      0xFEEF,2}, /* ALIF MAQSORA */
  {0x64A,      0xFEF1,4},  /* YA           */
  {0xFEF5,     0xFEF5,2},  /* Lam-Alif Madda */
  {0xFEF7,     0xFEF7,2},  /* Lam-Alif Hamza*/
  {0xFEF9,     0xFEF9,2},  /* Lam-Alif iHamza*/
  {0xFEFB,     0xFEFB,2}   /* Lam-Alif */
}; 	       
#define ALIF       0x627
#define ALIFHAMZA  0x623
#define ALIFIHAMZA 0x625
#define ALIFMADDA  0x622
#define LAM        0x644
#define HAMZA      0x621
#define WAW        0x648
#define WAWHAMZA   0x624


#define LAM_ALIF        0xFEFB
#define LAM_ALIFHAMZA   0xFEF7
#define LAM_ALIFIHAMZA  0xFEF9
#define LAM_ALIFMADDA   0xFEF5

#define LAM_ALIF_f        0xFEFC
#define LAM_ALIFHAMZA_f   0xFEF8
#define LAM_ALIFIHAMZA_f  0xFEFA
#define LAM_ALIFMADDA_f   0xFEF6
 
int arabic_isvowel(GUChar4 s)
{
      if ((s >= 0x64B) && (s <= 0x653)) return 1;
      if ((s >= 0xFE70) && (s <= 0xFE7F)) return 1;
      return 0;
}

static GUChar4 unshape(GUChar4 s)
{
      int j = 0;
      if ( (s > 0xFE80)  && ( s < 0xFEFF )){ /* arabic shaped Glyph , not HAMZA */
	    while ( chartable[j+1].charstart <= s) j++;
	    return chartable[j].basechar;
      } else if ((s == 0xFE8B)||(s == 0xFE8C)){
	  return HAMZA;
      } else {
	    return s;
      };
}

static GUChar4 charshape(GUChar4 s,short which)
{ /* which 0=alone 1=end 2=start 3=middle */
      int j = 0;
      if ( (s >= chartable[1].basechar)  && ( s <= 0xFEFB )){ 
	  /* arabic base char of Lam-Alif */
	    while ( chartable[j].basechar < s) j++;
	    return chartable[j].charstart+which;
      } else if (s == HAMZA){
	  if (which < 2) return s;
	  else return 0xFE8B+(which-2); /* The Hamza-'pod' */
      } else {
	    return s;
      };
}


static short shapecount(GUChar4 s)
{
      int j = 0;
      if (arabic_isvowel(s)){ /* correct trailing wovels */
	  return 1;
      } else if ( (s >= chartable[0].basechar)  && ( s <= 0xFEFB )){ 
	  /* arabic base char or ligature */
	  while ( chartable[j].basechar < s) j++;
	  return chartable[j].count;
      } else {
	  return 1;
      };
}

void shape(int* len,GUChar4* string)
{
      /* The string must be in visual order already.
      ** This routine does the basic arabic reshaping.
      */
      int   j;
      int   pcount    = 1; /* #of preceding vowels , plus one */
      short nc;            /* #shapes of next char */
      short sc;            /* #shapes of current char */
      int   prevstate = 0; /* */
      int   which;

      GUChar4 prevchar = 0;
      
      sc = shapecount(string[(*len)-1]);
      j = (*len)-1;
      while (j >= 0){
	    if (arabic_isvowel(string[j])){ 
		j--;  continue; /* don't shape vowels */
	    }
	    if (string[j] == 0){
		j--; continue;
	    };
	    if (j > 0){
		pcount = 1;
		while ( ( j-pcount >= 0)&&
			( (arabic_isvowel(string[j-pcount]))||
			  (string[j-pcount] == 0) ))
		    pcount++;
		nc = shapecount(string[j-pcount]);
	    } else {
		nc = 1;
	    };
	    
	    if (nc == 1){
		which = 0; /* end  or basic */
	    } else { 
		which = 2; /* middle or beginning */
	    };
	    if ( prevstate & 2 ){ /* use end or Middle-form */
		which += 1;
#define LIG
#ifdef LIG
		/* test if char == Alif && prev == Lam */
		if (prevchar == LAM){ 
		    switch(string[j]){
		    case ALIF: 
			(*len)--;
			string[j+1] = LAM_ALIF;
			string[j] = 0; j++; break;
		    case ALIFHAMZA: 
			(*len)--;
			string[j+1] = LAM_ALIFHAMZA;
			string[j] = 0; j++; break;
		    case ALIFIHAMZA:
			(*len)--;
			string[j+1] = LAM_ALIFIHAMZA;
			string[j] = 0; j++; break;
		    case ALIFMADDA:
			(*len)--;
			string[j+1] = LAM_ALIFMADDA;
			string[j] = 0; j++; break;
		    }
		}
#endif 
	    } else { /* use basic or beginning form */
#ifdef LIG
		if ((string[j] == HAMZA)
		    &&(prevchar)
		    &&(!arabic_isvowel(string[j-1])) ){
		    switch(prevchar){
		    case ALIF: 
			(*len)--;
			string[j+1] = ALIFHAMZA;
			string[j] = 0; 
			j++; sc = 2; break;
		    case WAW:
			(*len)--;
			string[j+1] = WAWHAMZA;
			string[j] = 0; 
			j++; sc = 2; break;
			/* case LAM_ALIF: have to hande LAM_ALIF+HAMZA.*/
			/* But LAM_ALIF is two back already ... */
		    }
		}
#endif
	    }
	    which = which % sc; /* middle->end,beginning->basic */
	    if (string[j] != 0){
		prevchar  = string[j];
		string[j] = charshape(string[j],which);
		prevstate = which;
		sc        = nc;    /* no need to recalculate */
	    }
	    j--;
      }
}

void reshape(int* len,GUChar4* string)
{
  int i;
  for ( i = 0; i < *len; i++){
    string[i] = unshape(string[i]);
  }
  shape(len,string);
}

/* Lam-Alif beginns at F5,F7,F9,FB ( MADDA,HAMZA,IHAMZA, . respectively ) */
