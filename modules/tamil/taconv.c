/* taconv.h
 * Author: Sivaraj D (sivaraj@tamil.net)
 * Date  : 4-Jan-2000
 */

/* Warning: These routines are very inefficient.  
 */

#include <stdio.h>
#include "taconv.h"
#include "tadefs.h"

/* tsc_find_char: Search the string for given character, 
 *                and return its position */
int tsc_find_char(unsigned char *str, unsigned char c)
{
	int i=0;
	while(str[i]) {
		if (c==str[i])
			return i;
		i++;
	}
	return -1;             /* character not found */
}

/* u_find_char: Search the string of integers for given unicode character
 *              and return its position */
int u_find_char(unsigned int *str, unsigned int c)
{
	int i=0;
	while(str[i]) {
		if (c==str[i])
			return i;
		i++;
	}
	return -1;             /* character not found */
}

int tsc2uni(unsigned char *tsc_str, unsigned int *uni_str, 
		int *num_tsc, int *num_uni,
		int num_in_left, int num_out_left)
{
	int i = 0, pos;
	unsigned char c1, c2, c3;
	
	/* default is 1 char */
	*num_tsc = 1;
	*num_uni = 1;

	if (num_in_left > 0)
		c1 = tsc_str[0];
	else
		return TA_INSUF_IN;

	if (num_in_left > 1)
		c2 = tsc_str[1];
	else
		c2 = TA_NULL;

	if (num_in_left > 2)
		c3 = tsc_str[2];
	else
		c3 = TA_NULL;

	if (num_out_left < 3)
		return TA_INSUF_OUT;           /* Need atleast 3 chars */

	/* us-ascii characters */
	if (c1 < 0x80) {
		uni_str[i] = (int)c1;
		return TA_SUCCESS;
	}

	/* direct one to one translation - uyirs, aaytham, quotes, 
	 * copyright & numbers */
	if((pos = tsc_find_char(tsc_uyir, c1)) >= 0 ) {
		uni_str[i] = u_uyir[pos];
		return TA_SUCCESS;
	}

	/* mey is always amey + puLLI in unicode */
	if (is_tsc_mey(c1)) {
		if (c1 == 0x8C) {
			uni_str[i++] = 0x0B95;
			uni_str[i++] = 0x0BCD;
		}
		pos = tsc_find_char(tsc_mey, c1);
		uni_str[i++] = u_amey[pos];
		uni_str[i] = U_PULLI;
		*num_uni = i+1;
		return TA_SUCCESS;
	}

	/* ukaram = amey + umodi1 */
	if ((c1 >= 0xCC && c1 <= 0xDB) || 
	     c1 == 0x99 || c1 == 0x9A ) {
		pos = tsc_find_char(tsc_ukaram, c1);
		uni_str[i++] = u_amey[pos];
		uni_str[i] = U_UMODI1;
		*num_uni = i+1;
		return TA_SUCCESS;
	}

	/* uukaram = amey + umodi2 */
	if ((c1 >= 0xDC && c1 <= 0xEB) || 
	     c1 == 0x9B || c1 == 0x9C ) {
		pos = tsc_find_char(tsc_uukaaram, c1);
		uni_str[i++] = u_amey[pos];
		uni_str[i] = U_UMODI2;
		*num_uni = i+1;
		return TA_SUCCESS;
	}

	/* TI */
	if (c1 == TSC_TI) {
		uni_str[i++] = 0x0B9F;  /* TA */
		uni_str[i] = U_KOKKI1;
		*num_uni = i+1;
		return TA_SUCCESS;
	}

	/* TII */
	if (c1 == TSC_TI) {
		uni_str[i++] = 0x0B9F;  /* TA */
		uni_str[i] = U_KOKKI2;
		*num_uni = i+1;
		return TA_SUCCESS;
	}
		
	/* characters starting with akarameriya meys */		
	if (is_tsc_amey(c1)) {
	    if (c1 == 0x87) {           /* KSHA = KA+puLLi+SHA in unicode */
		uni_str[i++] = 0x0B95;  /* KA */
		uni_str[i++] = 0x0BCD;  /* puLLi */
	    }
	    pos = tsc_find_char(tsc_amey, c1);
	    switch(c2) {
		case TSC_KAAL:
			uni_str[i++] = u_amey[pos];
			uni_str[i] = U_KAAL;
			*num_tsc = 2;
			*num_uni = i+1;
			return TA_SUCCESS;
		case TSC_KOKKI1:
			pos = tsc_find_char(tsc_amey, c1);
			uni_str[i++] = u_amey[pos];
			uni_str[i] = U_KOKKI1;
			*num_tsc = 2;
			*num_uni = i+1;
			return TA_SUCCESS;
		case TSC_KOKKI2:
			pos = tsc_find_char(tsc_amey, c1);
			uni_str[i++] = u_amey[pos];
			uni_str[i] = U_KOKKI2;
			*num_tsc = 2;
			*num_uni = i+1;
			return TA_SUCCESS;
		case TSC_UMODI1:           /* ok, I know this is not correct */
			pos = tsc_find_char(tsc_amey, c1);
			uni_str[i++] = u_amey[pos];
			uni_str[i] = U_UMODI1;
			*num_tsc = 2;
			*num_uni = i+1;
			return TA_SUCCESS;
		case TSC_UMODI2:
			pos = tsc_find_char(tsc_amey, c1);
			uni_str[i++] = u_amey[pos];
			uni_str[i] = U_UMODI2;
			*num_tsc = 2;
			*num_uni = i+1;
			return TA_SUCCESS;
		default:
			pos = tsc_find_char(tsc_amey, c1);
			uni_str[i] = u_amey[pos];
			*num_uni = i+1;
			return TA_SUCCESS;
	    }
	}

	/* ekaram, okaram & aukaaram */
	if (c1 == TSC_KOMBU1) {
		if (((c2 >= 0xB8 && c2 <= 0xC9)  ||
		     (c2 >= 0x93 && c2 <= 0x96)) &&
	     	     num_in_left > 2) { 
		    pos = tsc_find_char(tsc_amey, c2);
		    switch(c3) {
			case TSC_KAAL:
				uni_str[i++] = u_amey[pos];
				uni_str[i] = U_OMODI1;
				*num_tsc = 3;
				*num_uni = i+1;
				return TA_SUCCESS;
			case TSC_AUMODI:
				uni_str[i++] = u_amey[pos];
				uni_str[i] = U_AUMODI;
				*num_tsc = 3;
				*num_uni = i+1;
				return TA_SUCCESS;
			default:   /* it is ekaram */
				uni_str[i++] = u_amey[pos];
				uni_str[i] = U_KOMBU1;
				*num_tsc = 2;
				*num_uni = i+1;
				return TA_SUCCESS;
		    }	/* switch */
		}	/* c2 */
		else {
			/* if the sequence is illegal, handle it gracefully */
			uni_str[i++] = U_ZWSP;   /* zero width space */
			uni_str[i] = U_KOMBU1;
			*num_uni = i+1;
			return TA_ILL_SEQ;
		}	/* c2 */
	}		/* c1 */


	/* eekaaram, ookaaram */
	if (c1 == TSC_KOMBU2) {
		if ((c2 >= 0xB8 && c2 <= 0xC9) ||
		    (c2 >= 0x93 && c2 <= 0x96)) {
		    switch(c3) {
			case TSC_KAAL:
				pos = tsc_find_char(tsc_amey, c2);
				uni_str[i++] = u_amey[pos];
				uni_str[i] = U_OMODI2;
				*num_tsc = 3;
				*num_uni = i+1;
				return TA_SUCCESS;
			default:   /* it is eekaaram */
				pos = tsc_find_char(tsc_amey, c2);
				uni_str[i++] = u_amey[pos];
				uni_str[i] = U_KOMBU2;
				*num_tsc = 2;
				*num_uni = i+1;
				return TA_SUCCESS;
		    }		/* switch */
		}		/* c2 */
		else {
			/* if the sequence is illegal, handle it gracefully */
			uni_str[i++] = U_ZWSP;   /* zero width space */
			uni_str[i] = U_KOMBU2;
			*num_uni = i+1;
			return TA_ILL_SEQ;
		}       	/* c2 */
	}			/* c1 */
		
	/* aikaaram */
	if (c1 == TSC_AIMODI) {
		if ((c2 >= 0xB8 && c2 <= 0xC9) ||
		    (c2 >= 0x93 && c2 <= 0x96)) {
			pos = tsc_find_char(tsc_amey, c2);
			uni_str[i++] = u_amey[pos];
			uni_str[i] = U_AIMODI;
			*num_tsc = 2;
			*num_uni = i+1;
			return TA_SUCCESS;
		}	/* c2 */
		else {
			/* if the sequence is illegal, handle it gracefully */
			uni_str[i++] = U_ZWSP;   /* zero width space */
			uni_str[i] = U_AIMODI;
			*num_uni = i+1;
			return TA_ILL_SEQ;
		}	/* c2 */
	}		/* c1 */

	/* It is illegal in the language for the modifiers to appear alone.
	 * However in practice, they might appear alone, for example, when
         * teaching the language.  We will precede those with a zero width
	 * space to avoid combining them improperly */
	if (c1 == TSC_KAAL   || c1 == TSC_AUMODI ||
            c1 == TSC_KOKKI1 || c1 == TSC_KOKKI2 ||
	    c1 == TSC_UMODI1 || c1 == TSC_UMODI2 ) {
		pos = tsc_find_char(tsc_modi, c1);
		uni_str[i++] = U_ZWSP;
		uni_str[i] = u_modi[pos];
		*num_uni = i+1;
		return TA_ILL_SEQ;
	}

	/* These two characters were undefined in TSCII */
	if (c1 == 0xFE || c1 == 0xFF ) {
		uni_str[i++] = U_ZWSP;
		return TA_NOT_DEFINED;
	}

	/* For everything else, display a space */
	uni_str[i++] = U_SPACE;
	return TA_ILL_SEQ;
}


int uni2tsc(unsigned int *uni_str, unsigned char *tsc_str, 
		int *num_uni, int *num_tsc,
		int num_in_left, int num_out_left)
{
	int i = 0, pos;
	unsigned int c1, c2, c3;

	/* default is 1 char */
	*num_uni = 1;
	*num_tsc = 1;

	if (num_in_left > 0)
		c1 = uni_str[0];
	else
		return TA_INSUF_IN;

	if (num_in_left > 1)
		c2 = uni_str[1];
	else
		c2 = TA_NULL;

	if (num_in_left > 2)
		c3 = uni_str[2];
	else
		c3 = TA_NULL;

	if (num_out_left < 3)
		return TA_INSUF_OUT;      /* Need atleast three chars */

	if (c1 < 0x80) {
		tsc_str[i] = (char)c1;
		return TA_SUCCESS;
	}

	if (c1 < 0x0B80 || c1 > 0x0BFF) {
		tsc_str[i] = SPACE;
		return TA_OUT_OF_RANGE;
	}

	if (c1 < 0x0B82)
		return TA_ILL_SEQ;
	
	if (c1 == 0x0B82) {
		tsc_str[i] = SPACE;
		return TA_SUCCESS;      /* Don't know any TAMIL SIGN ANUSVARA */
	}

	/* uyir, aaytham & numbers */
	if ((c1 >= 0x0B83 && c1 <= 0x0B94) ||
	    (c1 >= 0x0BE7 && c1 <= 0x0BF2)) {
		if ((pos = u_find_char(u_uyir, c1)) < 0) {
			tsc_str[i] = SPACE;
			return TA_NOT_DEFINED;
		}
		tsc_str[i] = tsc_uyir[pos];
		return TA_SUCCESS;
	}

	/* akarameriya mey */
	if (c1 >= 0x0B95 && c1 <= 0x0BB9) {
		if ((pos = u_find_char(u_amey, c1)) < 0) {
			tsc_str[i] = SPACE;
			return TA_NOT_DEFINED;
		}
		switch(c2) {
			case U_PULLI:
				tsc_str[i] = tsc_mey[pos];		
				*num_uni = 2;
				return TA_SUCCESS;
			case U_KAAL: 
				tsc_str[i++] = tsc_amey[pos];		
				tsc_str[i] = TSC_KAAL;
				*num_tsc = 2;
				*num_uni = 2;
				return TA_SUCCESS;
			case U_KOKKI1:
				/* TI & TII case */
                                if (c1 == 0x0B9f) {
                                  tsc_str[i] = TSC_TI;
                                  *num_uni = 2;
                                  return TA_SUCCESS;
                                }
				tsc_str[i++] = tsc_amey[pos];		
				tsc_str[i] = TSC_KOKKI1;
				*num_tsc = 2;
				*num_uni = 2;
				return TA_SUCCESS;
			case U_KOKKI2:
				/* TI & TII case */
                                if (c1 == 0x0B9f) {
                                  tsc_str[i] = TSC_TII;
                                  *num_uni = 2;
                                  return TA_SUCCESS;
                                }
				tsc_str[i++] = tsc_amey[pos];		
				tsc_str[i] = TSC_KOKKI2;
				*num_tsc = 2;
				*num_uni = 2;
				return TA_SUCCESS;
			case U_UMODI1:
				/* If it is a grantha add a hook, otherwise
				 * we have separate chars in TSCII */
				if (u_find_char(u_grantha, c1) < 0) {
					tsc_str[i] = tsc_ukaram[pos];
					*num_uni = 2;
					return TA_SUCCESS;
				}
				else {
					tsc_str[i++] = tsc_amey[pos];
					tsc_str[i] = TSC_UMODI1;
					*num_tsc = 2;
					*num_uni = 2;
					return TA_SUCCESS;
				}
			case U_UMODI2:
				if (u_find_char(u_grantha, c1) < 0) {
					tsc_str[i] = tsc_uukaaram[pos];
					*num_uni = 2;
					return TA_SUCCESS;
				}
				else {
					tsc_str[i++] = tsc_amey[pos];
					tsc_str[i] = TSC_UMODI2;
					*num_tsc = 2;
					*num_uni = 2;
					return TA_SUCCESS;
				}
			case U_KOMBU1:
				/* Unicode seems to allow double modifiers for
				 * okaram, ookaaram & aukaaram.  This is 
				 * somewhat unnecessary.  But we will handle
				 * that condition too.
				 */
				switch(c3) {
				    case U_KAAL:
					tsc_str[i++] = TSC_KOMBU1;
					tsc_str[i++] = tsc_amey[pos];		
					tsc_str[i] = TSC_KAAL;
					*num_tsc = 3;
					*num_uni = 3;
					return TA_SUCCESS;
				    case U_AUMARK:
					tsc_str[i++] = TSC_KOMBU1;
					tsc_str[i++] = tsc_amey[pos];		
					tsc_str[i] = TSC_AUMODI;
					*num_tsc = 3;
					*num_uni = 3;
					return TA_SUCCESS;
				    default:
					tsc_str[i++] = TSC_KOMBU1;
					tsc_str[i] = tsc_amey[pos];		
					*num_tsc = 2;
					*num_uni = 2;
					return TA_SUCCESS;
				}	
			case U_KOMBU2:
				if (c3 == U_KAAL) {
					tsc_str[i++] = TSC_KOMBU2;
					tsc_str[i++] = tsc_amey[pos];		
					tsc_str[i] = TSC_KAAL;
					*num_tsc = 3;
					*num_uni = 3;
					return TA_SUCCESS;
				}
				else {
					tsc_str[i++] = TSC_KOMBU2;
					tsc_str[i] = tsc_amey[pos];		
					*num_tsc = 2;
					*num_uni = 2;
					return TA_SUCCESS;
				}	
			case U_AIMODI:
				tsc_str[i++] = TSC_AIMODI;
				tsc_str[i] = tsc_amey[pos];		
				*num_tsc = 2;
				*num_uni = 2;
				return TA_SUCCESS;
			case U_OMODI1:
				tsc_str[i++] = TSC_KOMBU1;
				tsc_str[i++] = tsc_amey[pos];		
				tsc_str[i] = TSC_KAAL;
				*num_tsc = 3;
				*num_uni = 2;
				return TA_SUCCESS;
			case U_OMODI2:
				tsc_str[i++] = TSC_KOMBU2;
				tsc_str[i++] = tsc_amey[pos];		
				tsc_str[i] = TSC_KAAL;
				*num_tsc = 3;
				*num_uni = 2;
				return TA_SUCCESS;
			case U_AUMODI:
				tsc_str[i++] = TSC_KOMBU1;
				tsc_str[i++] = tsc_amey[pos];		
				tsc_str[i] = TSC_AUMODI;
				*num_tsc = 3;
				*num_uni = 2;
				return TA_SUCCESS;
			default:
				tsc_str[i] = tsc_amey[pos];		
				return TA_SUCCESS;
		}
	}

	/* modifiers - illegal sequence */
	if (c1 >= 0x0BBE && c1 <= 0x0BD7) {
		if ((pos = u_find_char(u_modi, c1)) < 0) {
			tsc_str[i] = SPACE;
			return TA_NOT_DEFINED;
		}
		tsc_str[i] = tsc_modi[pos];
		return TA_ILL_SEQ;	
	}
	tsc_str[i] = SPACE;
	return TA_NOT_DEFINED;
}

int is_tsc_uyir(unsigned char c)
{
  if (c >= 0xAB && c <= 0xB7)
	return TA_TRUE;
  else
	return TA_FALSE;

}

int is_tsc_modi(unsigned char c)
{
  if ((c >= 0xA1 && c <= 0xA8) || 
       c == 0xAA )
	return TA_TRUE;
  else
	return TA_FALSE;
}

int is_tsc_amey(unsigned char c)
{
  if ((c >= 0x83 && c <= 0x87) ||
      (c >= 0xB8 && c <= 0xC9))
	return TA_TRUE;
  else
	return TA_FALSE;
}

int is_tsc_mey(unsigned char c)
{
  if ((c >= 0x88 && c <= 0x8C) ||
      (c >= 0xEC && c <= 0xFD))
	return TA_TRUE;
  else
	return TA_FALSE;
}

int is_tsc_number(unsigned char c)
{
  if ( c >= 0x81               ||
      (c >= 0x8D && c <= 0x90) ||
      (c >= 0x95 && c <= 0x98) ||
      (c >= 0x9D && c <= 0x9F))
	return TA_TRUE;
  else
	return TA_FALSE;
}

int is_uni_uyir(unsigned int c)
{
  if ((c >= 0x0B85 && c <= 0x0B8A) || 
      (c >= 0x0B8E && c <= 0x0B90) ||
      (c >= 0x0B92 && c <= 0x0B94) ||
      (c == 0x0B83))
	return TA_TRUE;
  else
	return TA_FALSE;
}

int is_uni_amey(unsigned int c)
{
  if (u_find_char(u_amey, c) < 0)
	return TA_FALSE;
  else 
	return TA_TRUE;
}

int is_uni_modi(unsigned int c)
{
  if (u_find_char(u_modi, c) < 0)
	return TA_FALSE;
  else 
	return TA_TRUE;
}

int is_uni_numb(unsigned int c)
{
  if ((c >= 0x0BE7 && c <= 0x0BF2))
	return TA_FALSE;
  else 
	return TA_TRUE;
}
