/* taconv.h:
 * Author: Sivaraj D (sivaraj@tamil.net)
 * Date  : 4-Jan-2000
 */

/* Return codes */
#define TA_SUCCESS	 	0
#define TA_ILL_SEQ 		1 /* Sequence is illegal in language */
#define TA_INSUF_IN  		2
#define TA_INSUF_OUT 		3 /* Need atleast three chars */
#define TA_OUT_OF_RANGE 	4 /* Char outside 0x00-0x7f or 0xb80-0xbff */
#define TA_NOT_DEFINED		5 /* Within 0xb80-0xbff but not defined 
                                   * by unicode*/

#define TA_NULL			0x00
#define TA_TRUE			1
#define TA_FALSE		0

/* tsc2uni: Get the first TSCII Tamil character in tsc_str and convert it to 
 *          corresponding unicode character in uni_str.  
 * tsc_str:      TSCII string (in)
 * uni_str:      Unicode string (out)
 * num_tsc:      Number of TSCII characters processed (out)
 * num_uni:      Number of Unicode characters returned (out)
 * num_in_left:  Number of characters left in input buffer (in)
 * num_out_left: Number of characters left in output buffer (in)
 */
int tsc2uni(unsigned char *tsc_str, unsigned int *uni_str, 
		int *num_tsc, int *num_uni,
		int num_in_left, int num_out_left);

/* uni2tsc: Get the first Unicode character in uni_str and convert it to 
 *          corresponding TSCII Tamil character in tsc_str.  
 * uni_str:      Unicode string (out)
 * tsc_str:      TSCII string (in)
 * num_uni:      Number of Unicode characters returned (out)
 * num_tsc:      Number of TSCII characters processed (out)
 * num_in_left:  Number of characters left in input buffer (in)
 * num_out_left: Number of characters left in output buffer (in)
 */
int uni2tsc(unsigned int *uni_str, unsigned char *tsc_str, 
		int *num_uni, int *num_tsc,
		int num_in_left, int num_out_left);

int is_tsc_uyir(unsigned char c);    /* Returns 1 if c is a vowel, else 0 */
int is_tsc_modi(unsigned char c);    /* Returns 1 if c is a modifier */
int is_tsc_amey(unsigned char c);    /* Returns 1 if c is a akara mey */
int is_tsc_mey(unsigned char c);     /* Returns 1 if c is a consonant */
int is_tsc_number(unsigned char c);  /* Returns 1 if c is a number */

int is_uni_uyir(unsigned int c);     /* Returns 1 if c is a vowel, else 0 */
int is_uni_modi(unsigned int c);     /* Returns 1 if c is a modifier */
int is_uni_amey(unsigned int c);     /* Returns 1 if c is a akara mey */
int is_uni_numb(unsigned int c);     /* Returns 1 if c is a number */

