
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
  {0x66D,      0xFEF5,2}  /* star,LAM-ALIF */
}; 	       

static GUChar4 unshape(GUChar4 s)
{
      int j = 0;
      if ( (s > 0xFE80)  && ( s < 0xFEFF )){ /* arabic shaped Glyph */
	    while ( chartable[j+1].charstart <= s) j++;
	    return chartable[j].basechar;
      } else {
	    return s;
      };
}

static GUChar4 charshape(GUChar4 s,short which)
{ /* which 0=alone 1=end 2=start 3=middle */
      int j = 0;
      if ( (s >= 0x622)  && ( s <= 0x64A )){ /* arabic base char */
	    while ( chartable[j].basechar < s) j++;
	    return chartable[j].charstart+which;
      } else {
	    return s;
      };
}


static short shapecount(GUChar4 s)
{
      int j = 0;
      if ( (s >= 0x622)  && ( s <= 0x64A )){ /* arabic base char */
	    while ( chartable[j].basechar < s) j++;
	    return chartable[j].count;
      } else {
	    return 1;
      };
}

void shape(int len,GUChar4* string)
{
      /* The string must be in visual order already.
      ** This routine does the basic arabic reshaping.
      */
      int   j;
      short nc;            /* #shapes of next char */
      short sc;            /* #shapes of current char */
      int   prevstate = 0; /* */
      int   which;

      sc = shapecount(string[len-1]);
      for ( j = len-1; j >= 0; j-- ){
	    if (j > 0){
		  nc = shapecount(string[j-1]);
	    } else {
		  nc = 1;
	    };

	    if ( prevstate & 2 ){ /* use and end or Middle-form */
		  if (nc == 1){
			which = 1; /* end */
		  } else {
			which = 3; /* middle */
		  }
	    } else { /* use a stand-alone or beginning-form */
		  if (nc == 1){
			which = 0; /* basic */
		  } else {
			which = 2; /* beginning */
		  }
	    };
	    which = which % sc; /* middle->end,beginning->basic */
	    string[j] = charshape(string[j],which);
	    prevstate = which;
	    sc        = nc;    /* no need to recalculate */
      }
}

void reshape(int len,GUChar4* string)
{
  int i;
  for ( i = 0; i < len; i++){
    string[i] = unshape(string[i]);
  }
  shape(len,string);
}

/* Lam-Alif beginns at F5,F7,F9,FB ( MADDA,HAMZA,IHAMZA, . respectively ) */

