/* This is part of Pango - Arabic shaping module 
 *
 * (C) 2000 Karl Koehler <koehler@or.uni-bonn.de>
 * (C) 2001 Roozbeh Pournader <roozbeh@sharif.edu>
 *
 */

#include "arconv.h"
/*  #define DEBUG  */
#ifdef DEBUG
#include <stdio.h>
#endif

typedef struct
{
  gunichar basechar;
  int count;
  gunichar charshape[4];
}
shapestruct;

typedef struct
{
  gunichar basechar;
  gunichar mark1;               /* has to be initialized to zero */
  gunichar vowel;
  signed char lignum;           /* is a ligature with lignum aditional characters */
  char numshapes;
}
charstruct;

#define connects_to_left(a)   ((a).numshapes > 2)

/* The Unicode order is always 'isolated, final, initial, medial'. */

/* *INDENT-OFF* */
static shapestruct chartable[] = {
  {0x0621, 1, {0xFE80}}, /* HAMZA */
  {0x0622, 2, {0xFE81, 0xFE82}}, /* ALEF WITH MADDA ABOVE */
  {0x0623, 2, {0xFE83, 0xFE84}}, /* ALEF WITH HAMZA ABOVE */
  {0x0624, 2, {0xFE85, 0xFE86}}, /* WAW WITH HAMZA ABOVE */
  {0x0625, 2, {0xFE87, 0xFE88}}, /* ALEF WITH HAMZA BELOW */
  {0x0626, 4, {0xFE89, 0xFE8A, 0xFE8B, 0xFE8C}}, /* YEH WITH HAMZA ABOVE */
  {0x0627, 2, {0xFE8D, 0xFE8E}}, /* ALEF */
  {0x0628, 4, {0xFE8F, 0xFE90, 0xFE91, 0xFE92}}, /* BEH */
  {0x0629, 2, {0xFE93, 0xFE94}}, /* TEH MARBUTA */
  {0x062A, 4, {0xFE95, 0xFE96, 0xFE97, 0xFE98}}, /* TEH */
  {0x062B, 4, {0xFE99, 0xFE9A, 0xFE9B, 0xFE9C}}, /* THEH */
  {0x062C, 4, {0xFE9D, 0xFE9E, 0xFE9F, 0xFEA0}}, /* JEEM */
  {0x062D, 4, {0xFEA1, 0xFEA2, 0xFEA3, 0xFEA4}}, /* HAH */
  {0x062E, 4, {0xFEA5, 0xFEA6, 0xFEA7, 0xFEA8}}, /* KHAH */
  {0x062F, 2, {0xFEA9, 0xFEAA}}, /* DAL */
  {0x0630, 2, {0xFEAB, 0xFEAC}}, /* THAL */
  {0x0631, 2, {0xFEAD, 0xFEAE}}, /* REH */
  {0x0632, 2, {0xFEAF, 0xFEB0}}, /* ZAIN */
  {0x0633, 4, {0xFEB1, 0xFEB2, 0xFEB3, 0xFEB4}}, /* SEEN */
  {0x0634, 4, {0xFEB5, 0xFEB6, 0xFEB7, 0xFEB8}}, /* SHEEN */
  {0x0635, 4, {0xFEB9, 0xFEBA, 0xFEBB, 0xFEBC}}, /* SAD */
  {0x0636, 4, {0xFEBD, 0xFEBE, 0xFEBF, 0xFEC0}}, /* DAD */
  {0x0637, 4, {0xFEC1, 0xFEC2, 0xFEC3, 0xFEC4}}, /* TAH */
  {0x0638, 4, {0xFEC5, 0xFEC6, 0xFEC7, 0xFEC8}}, /* ZAH */
  {0x0639, 4, {0xFEC9, 0xFECA, 0xFECB, 0xFECC}}, /* AIN */
  {0x063A, 4, {0xFECD, 0xFECE, 0xFECF, 0xFED0}}, /* GHAIN */
  {0x0640, 4, {0x0640, 0x0640, 0x0640, 0x0640}}, /* TATWEEL */
  {0x0641, 4, {0xFED1, 0xFED2, 0xFED3, 0xFED4}}, /* FEH */
  {0x0642, 4, {0xFED5, 0xFED6, 0xFED7, 0xFED8}}, /* QAF */
  {0x0643, 4, {0xFED9, 0xFEDA, 0xFEDB, 0xFEDC}}, /* KAF */
  {0x0644, 4, {0xFEDD, 0xFEDE, 0xFEDF, 0xFEE0}}, /* LAM */
  {0x0645, 4, {0xFEE1, 0xFEE2, 0xFEE3, 0xFEE4}}, /* MEEM */
  {0x0646, 4, {0xFEE5, 0xFEE6, 0xFEE7, 0xFEE8}}, /* NOON */
  {0x0647, 4, {0xFEE9, 0xFEEA, 0xFEEB, 0xFEEC}}, /* HEH */
  {0x0648, 2, {0xFEED, 0xFEEE}}, /* WAW */
  {0x0649, 4, {0xFEEF, 0xFEF0, 0xFBE8, 0xFBE9}}, /* ALEF MAKSURA */
  {0x064A, 4, {0xFEF1, 0xFEF2, 0xFEF3, 0xFEF4}}, /* YEH */
  {0x0671, 2, {0xFB50, 0xFB51}}, /* ALEF WASLA */
  {0x0679, 4, {0xFB66, 0xFB67, 0xFB68, 0xFB69}}, /* TTEH */
  {0x067A, 4, {0xFB5E, 0xFB5F, 0xFB60, 0xFB61}}, /* TTEHEH */
  {0x067B, 4, {0xFB52, 0xFB53, 0xFB54, 0xFB55}}, /* BEEH */
  {0x067E, 4, {0xFB56, 0xFB57, 0xFB58, 0xFB59}}, /* PEH */
  {0x067F, 4, {0xFB62, 0xFB63, 0xFB64, 0xFB65}}, /* TEHEH */
  {0x0680, 4, {0xFB5A, 0xFB5B, 0xFB5C, 0xFB5D}}, /* BEHEH */
  {0x0683, 4, {0xFB76, 0xFB77, 0xFB78, 0xFB79}}, /* NYEH */
  {0x0684, 4, {0xFB72, 0xFB73, 0xFB74, 0xFB75}}, /* DYEH */
  {0x0686, 4, {0xFB7A, 0xFB7B, 0xFB7C, 0xFB7D}}, /* TCHEH */
  {0x0687, 4, {0xFB7E, 0xFB7F, 0xFB80, 0xFB81}}, /* TCHEHEH */
  {0x0688, 2, {0xFB88, 0xFB89}}, /* DDAL */
  {0x068C, 2, {0xFB84, 0xFB85}}, /* DAHAL */
  {0x068D, 2, {0xFB82, 0xFB83}}, /* DDAHAL */
  {0x068E, 2, {0xFB86, 0xFB87}}, /* DUL */
  {0x0691, 2, {0xFB8C, 0xFB8D}}, /* RREH */
  {0x0698, 2, {0xFB8A, 0xFB8B}}, /* JEH */
  {0x06A4, 4, {0xFB6A, 0xFB6B, 0xFB6C, 0xFB6D}}, /* VEH */
  {0x06A6, 4, {0xFB6E, 0xFB6F, 0xFB70, 0xFB71}}, /* PEHEH */
  {0x06A9, 4, {0xFB8E, 0xFB8F, 0xFB90, 0xFB91}}, /* KEHEH */
  {0x06AD, 4, {0xFBD3, 0xFBD4, 0xFBD5, 0xFBD6}}, /* NG */
  {0x06AF, 4, {0xFB92, 0xFB93, 0xFB94, 0xFB95}}, /* GAF */
  {0x06B1, 4, {0xFB9A, 0xFB9B, 0xFB9C, 0xFB9D}}, /* NGOEH */
  {0x06B3, 4, {0xFB96, 0xFB97, 0xFB98, 0xFB99}}, /* GUEH */
  {0x06BB, 4, {0xFBA0, 0xFBA1, 0xFBA2, 0xFBA3}}, /* RNOON */
  {0x06BE, 4, {0xFBAA, 0xFBAB, 0xFBAC, 0xFBAD}}, /* HEH DOACHASHMEE */
  {0x06C0, 2, {0xFBA4, 0xFBA5}}, /* HEH WITH YEH ABOVE */
  {0x06C1, 4, {0xFBA6, 0xFBA7, 0xFBA8, 0xFBA9}}, /* HEH GOAL */
  {0x06C5, 2, {0xFBE0, 0xFBE1}}, /* KIRGHIZ OE */
  {0x06C6, 2, {0xFBD9, 0xFBDA}}, /* OE */
  {0x06C7, 2, {0xFBD7, 0xFBD8}}, /* U */
  {0x06C8, 2, {0xFBDB, 0xFBDC}}, /* YU */
  {0x06C9, 2, {0xFBE2, 0xFBE3}}, /* KIRGHIZ YU */
  {0x06CB, 2, {0xFBDE, 0xFBDF}}, /* VE */
  {0x06CC, 4, {0xFBFC, 0xFBFD, 0xFBFE, 0xFBFF}}, /* FARSI YEH */
  {0x06D0, 4, {0xFBE4, 0xFBE5, 0xFBE6, 0xFBE7}}, /* E */
  {0x06D2, 2, {0xFBAE, 0xFBAF}}, /* YEH BARREE */
  {0x06D3, 2, {0xFBB0, 0xFBB1}}, /* YEH BARREE WITH HAMZA ABOVE */
};
/* *INDENT-ON* */

static gunichar unshapetableFB[] = {
  0x0671, 0x0671, 0x067B, 0x067B, 0x067B, 0x067B, 0x067E, 0x067E,
  0x067E, 0x067E, 0x0680, 0x0680, 0x0680, 0x0680, 0x067A, 0x067A,
  0x067A, 0x067A, 0x067F, 0x067F, 0x067F, 0x067F, 0x0679, 0x0679,
  0x0679, 0x0679, 0x06A4, 0x06A4, 0x06A4, 0x06A4, 0x06A6, 0x06A6,
  0x06A6, 0x06A6, 0x0684, 0x0684, 0x0684, 0x0684, 0x0683, 0x0683,
  0x0683, 0x0683, 0x0686, 0x0686, 0x0686, 0x0686, 0x0687, 0x0687,
  0x0687, 0x0687, 0x068D, 0x068D, 0x068C, 0x068C, 0x068E, 0x068E,
  0x0688, 0x0688, 0x0698, 0x0698, 0x0691, 0x0691, 0x06A9, 0x06A9,
  0x06A9, 0x06A9, 0x06AF, 0x06AF, 0x06AF, 0x06AF, 0x06B3, 0x06B3,
  0x06B3, 0x06B3, 0x06B1, 0x06B1, 0x06B1, 0x06B1, 0x0000, 0x0000,
  0x06BB, 0x06BB, 0x06BB, 0x06BB, 0x06C0, 0x06C0, 0x06C1, 0x06C1,
  0x06C1, 0x06C1, 0x06BE, 0x06BE, 0x06BE, 0x06BE, 0x06D2, 0x06D2,
  0x06D3, 0x06D3, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000,
  0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000,
  0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000,
  0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000,
  0x0000, 0x0000, 0x0000, 0x06AD, 0x06AD, 0x06AD, 0x06AD, 0x06C7,
  0x06C7, 0x06C6, 0x06C6, 0x06C8, 0x06C8, 0x0000, 0x06CB, 0x06CB,
  0x06C5, 0x06C5, 0x06C9, 0x06C9, 0x06D0, 0x06D0, 0x06D0, 0x06D0,
  0x0649, 0x0649, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000,
  0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000,
  0x0000, 0x0000, 0x0000, 0x0000, 0x06CC, 0x06CC, 0x06CC, 0x06CC
};

static gunichar unshapetableFE[] = {
  0x0621, 0x0622, 0x0622, 0x0623, 0x0623, 0x0624, 0x0624, 0x0625,
  0x0625, 0x0626, 0x0626, 0x0626, 0x0626, 0x0627, 0x0627, 0x0628,
  0x0628, 0x0628, 0x0628, 0x0629, 0x0629, 0x062A, 0x062A, 0x062A,
  0x062A, 0x062B, 0x062B, 0x062B, 0x062B, 0x062C, 0x062C, 0x062C,
  0x062C, 0x062D, 0x062D, 0x062D, 0x062D, 0x062E, 0x062E, 0x062E,
  0x062E, 0x062F, 0x062F, 0x0630, 0x0630, 0x0631, 0x0631, 0x0632,
  0x0632, 0x0633, 0x0633, 0x0633, 0x0633, 0x0634, 0x0634, 0x0634,
  0x0634, 0x0635, 0x0635, 0x0635, 0x0635, 0x0636, 0x0636, 0x0636,
  0x0636, 0x0637, 0x0637, 0x0637, 0x0637, 0x0638, 0x0638, 0x0638,
  0x0638, 0x0639, 0x0639, 0x0639, 0x0639, 0x063A, 0x063A, 0x063A,
  0x063A, 0x0641, 0x0641, 0x0641, 0x0641, 0x0642, 0x0642, 0x0642,
  0x0642, 0x0643, 0x0643, 0x0643, 0x0643, 0x0644, 0x0644, 0x0644,
  0x0644, 0x0645, 0x0645, 0x0645, 0x0645, 0x0646, 0x0646, 0x0646,
  0x0646, 0x0647, 0x0647, 0x0647, 0x0647, 0x0648, 0x0648, 0x0649,
  0x0649, 0x064A, 0x064A, 0x064A, 0x064A
};

#define ALEF           0x0627
#define ALEFHAMZA      0x0623
#define ALEFHAMZABELOW 0x0625
#define ALEFMADDA      0x0622
#define LAM            0x0644
#define HAMZA          0x0621
#define TATWEEL        0x0640
#define ZWJ            0x200D

#define HAMZAABOVE  0x0654
#define HAMZABELOW  0x0655

#define WAWHAMZA    0x0624
#define YEHHAMZA    0x0626
#define WAW         0x0648
#define ALEFMAKSURA 0x0649
#define YEH         0x064A
#define FARSIYEH    0x06CC

#define SHADDA      0x0651
#define KASRA       0x0650
#define FATHA       0x064E
#define DAMMA       0x064F
#define MADDA       0x0653

#define LAM_ALEF           0xFEFB
#define LAM_ALEFHAMZA      0xFEF7
#define LAM_ALEFHAMZABELOW 0xFEF9
#define LAM_ALEFMADDA      0xFEF5

static void
charstruct_init (charstruct * s)
{
  s->basechar = 0;
  s->mark1 = 0;
  s->vowel = 0;
  s->lignum = 0;
  s->numshapes = 1;
}

static void
copycstostring (gunichar * string, int *i, charstruct * s, arabic_level level)
/* s is a shaped charstruct; i is the index into the string */
{
  if (s->basechar == 0)
    return;

  string[*i] = s->basechar;
  (*i)++;
  (s->lignum)--;
  if (s->mark1 != 0)
    {
      if (!(level & ar_novowel))
        {
          string[*i] = s->mark1;
          (*i)++;
          (s->lignum)--;
        }
      else
        {
          (s->lignum)--;
        }
    }
  if (s->vowel != 0)
    {
      if (!(level & ar_novowel))
        {
          string[*i] = s->vowel;
          (*i)++;
          (s->lignum)--;
        }
      else
        {                       /* vowel elimination */
          (s->lignum)--;
        }
    }
  while (s->lignum > 0)
    {                           /* NULL-insertion for Langbox-font */
      string[*i] = 0;
      (*i)++;
      (s->lignum)--;
    }
}

int
arabic_isvowel (gunichar s)
/* is this a combining mark? */
{
  if (((s >= 0x064B) && (s <= 0x0655)) || (s == 0x0670))
    {
      return 1;
    }
  else
    {
      return 0;
    }
}

static gunichar
unshape (gunichar s)
{
  gunichar r;

  if ((s >= 0xFE80) && (s <= 0xFEF4)) /* Arabic Presentation Forms-B */
    {
      return unshapetableFE[s - 0xFE80];
    }
  else if ((s >= 0xFEF5) && (s <= 0xFEFC)) /* Lam+Alef ligatures */
    {
      return ((s % 2) ? s : (s - 1));
    }
  else if ((s >= 0xFB50) && (s <= 0xFBFF)) /* Arabic Presentation Forms-A */
    {
      return ((r = unshapetableFB[s - 0xFB50]) ? r : s);
    }
  else
    {
      return s;
    }
}

static gunichar
charshape (gunichar s, short which)
/* which 0=isolated 1=final 2=initial 3=medial */
{
  int l, r, m;
  if ((s >= 0x0621) && (s <= 0x06D3))
    {
      l = 0;
      r = sizeof (chartable) / sizeof (shapestruct);
      while (l <= r)
        {
          m = (l + r) / 2;
          if (s == chartable[m].basechar)
            {
              return chartable[m].charshape[which];
            }
          else if (s < chartable[m].basechar)
            {
              r = m - 1;
            }
          else
            {
              l = m + 1;
            }
        }
    }
  else if ((s >= 0xFEF5) && (s <= 0xFEFB))
    {                           /* Lam+Alef */
      return s + which;
    }

  return s;
}


static short
shapecount (gunichar s)
{
  int l, r, m;
  if ((s >= 0x0621) && (s <= 0x06D3) && !arabic_isvowel (s))
    {
      l = 0;
      r = sizeof (chartable) / sizeof (shapestruct);
      while (l <= r)
        {
          m = (l + r) / 2;
          if (s == chartable[m].basechar)
            {
              return chartable[m].count;
            }
          else if (s < chartable[m].basechar)
            {
              r = m - 1;
            }
          else
            {
              l = m + 1;
            }
        }
    }
  else if (s == ZWJ)
    {
      return 4;
    }
  return 1;
}

static int
unligature (charstruct * curchar, arabic_level level)
{
  int result = 0;
  if (level & ar_naqshfont)
    {
      /* decompose Alef Madda ... */
      switch (curchar->basechar)
        {
        case ALEFHAMZA:
          curchar->basechar = ALEF;
          curchar->mark1 = HAMZAABOVE;
          result++;
          break;
        case ALEFHAMZABELOW:
          curchar->basechar = ALEF;
          curchar->mark1 = HAMZABELOW;
          result++;
          break;
        case WAWHAMZA:
          curchar->basechar = WAW;
          curchar->mark1 = HAMZAABOVE;
          result++;
          break;
        case ALEFMADDA:
          curchar->basechar = ALEF;
          curchar->vowel = MADDA;
          result++;
          break;
        }
    }
  return result;
}

static int
ligature (gunichar newchar, charstruct * oldchar)
/* 0 == no ligature possible; 1 == vowel; 2 == two chars; 3 == Lam+Alef */
{
  int retval = 0;

  if (!(oldchar->basechar))
    return 0;
  if (arabic_isvowel (newchar))
    {
      retval = 1;
      if ((oldchar->vowel != 0) && (newchar != SHADDA))
        {
          retval = 2;           /* we eliminate the old vowel .. */
        }
      switch (newchar)
        {
        case SHADDA:
          if (oldchar->mark1 == 0)
            {
              oldchar->mark1 = SHADDA;
            }
          else
            {
              return 0;         /* no ligature possible */
            }
          break;
        case HAMZABELOW:
          switch (oldchar->basechar)
            {
            case ALEF:
              oldchar->basechar = ALEFHAMZABELOW;
              retval = 2;
              break;
            case LAM_ALEF:
              oldchar->basechar = LAM_ALEFHAMZABELOW;
              retval = 2;
              break;
            default:
              oldchar->mark1 = HAMZABELOW;
              break;
            }
          break;
        case HAMZAABOVE:
          switch (oldchar->basechar)
            {
            case ALEF:
              oldchar->basechar = ALEFHAMZA;
              retval = 2;
              break;
            case LAM_ALEF:
              oldchar->basechar = LAM_ALEFHAMZA;
              retval = 2;
              break;
            case WAW:
              oldchar->basechar = WAWHAMZA;
              retval = 2;
              break;
            case YEH:
            case ALEFMAKSURA:
            case FARSIYEH:
              oldchar->basechar = YEHHAMZA;
              retval = 2;
              break;
            default:           /* whatever sense this may make .. */
              oldchar->mark1 = HAMZAABOVE;
              break;
            }
          break;
        case MADDA:
          switch (oldchar->basechar)
            {
            case ALEF:
              oldchar->basechar = ALEFMADDA;
              retval = 2;
              break;
            }
          break;
        default:
          oldchar->vowel = newchar;
          break;
        }
      if (retval == 1)
        {
          oldchar->lignum++;
        }
      return retval;
    }
  if (oldchar->vowel != 0)
    {                           /* if we already joined a vowel, we can't join a Hamza */
      return 0;
    }

  switch (oldchar->basechar)
    {
    case LAM:
      switch (newchar)
        {
        case ALEF:
          oldchar->basechar = LAM_ALEF;
          oldchar->numshapes = 2;
          retval = 3;
          break;
        case ALEFHAMZA:
          oldchar->basechar = LAM_ALEFHAMZA;
          oldchar->numshapes = 2;
          retval = 3;
          break;
        case ALEFHAMZABELOW:
          oldchar->basechar = LAM_ALEFHAMZABELOW;
          oldchar->numshapes = 2;
          retval = 3;
          break;
        case ALEFMADDA:
          oldchar->basechar = LAM_ALEFMADDA;
          oldchar->numshapes = 2;
          retval = 3;
          break;
        }
      break;
    case 0:
      oldchar->basechar = newchar;
      oldchar->numshapes = shapecount (newchar);
      retval = 1;
      break;
    }
  return retval;
}

static void
shape (long *len, const char *text, gunichar * string, arabic_level level)
{
  /* string is assumed to be empty and big enough.
   * text is the original text.
   * This routine does the basic arabic reshaping.
   * *len the number of non-null characters.
   *
   * Note: We have to unshape each character first!
   */
  int olen = *len;
  charstruct oldchar, curchar;
  int j = 0;
  int join;
  int which;
  gunichar nextletter;
  const char *p = text;

  *len = 0;                     /* initialize for output */
  charstruct_init (&oldchar);
  charstruct_init (&curchar);
  while (p < text + olen)
    {
      nextletter = g_utf8_get_char (p);
      nextletter = unshape (nextletter);

      join = ligature (nextletter, &curchar);
      if (!join)
        {                       /* shape curchar */
          int nc = shapecount (nextletter);
          (*len)++;
          if (nc == 1)
            {
              which = 0;        /* final or isolated */
            }
          else
            {
              which = 2;        /* medial or initial */
            }
          if (connects_to_left (oldchar))
            {
              which++;
            }

          which = which % (curchar.numshapes);
          curchar.basechar = charshape (curchar.basechar, which);

          /* get rid of oldchar */
          copycstostring (string, &j, &oldchar, level);
          oldchar = curchar;    /* new values in oldchar */

          /* init new curchar */
          charstruct_init (&curchar);
          curchar.basechar = nextletter;
          curchar.numshapes = nc;
          curchar.lignum++;
          (*len) += unligature (&curchar, level);
        }
      else if ((join == 3) && (level & ar_lboxfont))
        {                       /* Lam+Alef extra in langbox-font */
          (*len)++;
          curchar.lignum++;
        }
      else if (join == 1)
        {
          (*len)++;
        }
      else
        {
          (*len) += unligature (&curchar, level);
        }
      p = g_utf8_next_char (p);
    }

  /* Handle last char */
  if (connects_to_left (oldchar))
    which = 1;
  else
    which = 0;
  which = which % (curchar.numshapes);
  curchar.basechar = charshape (curchar.basechar, which);

  /* get rid of oldchar */
  copycstostring (string, &j, &oldchar, level);
  copycstostring (string, &j, &curchar, level);
#ifdef DEBUG
  fprintf (stderr, "[ar] shape statistic: %i chars -> %i glyphs \n",
           olen, *len);
#endif
}

static void
doublelig (long *len, gunichar * string, arabic_level level)
/* Ok. We have presentation ligatures in our font. */
{
  int olen = *len;
  int j = 0, si = 1;
  gunichar lapresult;

  while (si < olen)
    {
      lapresult = 0;
      if (level & ar_composedtashkeel)
        {
          switch (string[j])
            {
            case SHADDA:
              switch (string[si])
                {
                case KASRA:
                  lapresult = 0xFC62;
                  break;
                case FATHA:
                  lapresult = 0xFC60;
                  break;
                case DAMMA:
                  lapresult = 0xFC61;
                  break;
                case 0x064C:
                  lapresult = 0xFC5E;
                  break;
                case 0x064D:
                  lapresult = 0xFC5F;
                  break;
                }
              break;
            case KASRA:
              if (string[si] == SHADDA)
                lapresult = 0xFC62;
              break;
            case FATHA:
              if (string[si] == SHADDA)
                lapresult = 0xFC60;
              break;
            case DAMMA:
              if (string[si] == SHADDA)
                lapresult = 0xFC61;
              break;
            }
        }

      if (level & ar_lig)
        {
          switch (string[j])
            {
            case 0xFEDF:       /* LAM initial */
              switch (string[si])
                {
                case 0xFE9E:
                  lapresult = 0xFC3F;
                  break;        /* JEEM final */
                case 0xFEA0:
                  lapresult = 0xFCC9;
                  break;        /* JEEM medial */
                case 0xFEA2:
                  lapresult = 0xFC40;
                  break;        /* HAH final */
                case 0xFEA4:
                  lapresult = 0xFCCA;
                  break;        /* HAH medial */
                case 0xFEA6:
                  lapresult = 0xFC41;
                  break;        /* KHAH final */
                case 0xFEA8:
                  lapresult = 0xFCCB;
                  break;        /* KHAH medial */
                case 0xFEE2:
                  lapresult = 0xFC42;
                  break;        /* MEEM final */
                case 0xFEE4:
                  lapresult = 0xFCCC;
                  break;        /* MEEM medial */
                }
              break;
            case 0xFE97:       /* TEH inital */
              switch (string[si])
                {
                case 0xFEA0:
                  lapresult = 0xFCA1;
                  break;        /* JEEM medial */
                case 0xFEA4:
                  lapresult = 0xFCA2;
                  break;        /* HAH medial */
                case 0xFEA8:
                  lapresult = 0xFCA3;
                  break;        /* KHAH medial */
                }
              break;
            case 0xFE91:       /* BEH inital */
              switch (string[si])
                {
                case 0xFEA0:
                  lapresult = 0xFC9C;
                  break;        /* JEEM medial */
                case 0xFEA4:
                  lapresult = 0xFC9D;
                  break;        /* HAH medial */
                case 0xFEA8:
                  lapresult = 0xFC9E;
                  break;        /* KHAH medial */
                }
              break;
            case 0xFEE7:       /* NOON inital */
              switch (string[si])
                {
                case 0xFEA0:
                  lapresult = 0xFCD2;
                  break;        /* JEEM initial */
                case 0xFEA4:
                  lapresult = 0xFCD3;
                  break;        /* HAH medial */
                case 0xFEA8:
                  lapresult = 0xFCD4;
                  break;        /* KHAH medial */
                }
              break;

            case 0xFEE8:       /* NOON medial */
              switch (string[si])
                {
                case 0xFEAE:
                  lapresult = 0xFC8A;
                  break;        /* REH final  */
                case 0xFEB0:
                  lapresult = 0xFC8B;
                  break;        /* ZAIN final */
                }
              break;
            case 0xFEE3:       /* MEEM initial */
              switch (string[si])
                {
                case 0xFEA0:
                  lapresult = 0xFCCE;
                  break;        /* JEEM medial */
                case 0xFEA4:
                  lapresult = 0xFCCF;
                  break;        /* HAH medial */
                case 0xFEA8:
                  lapresult = 0xFCD0;
                  break;        /* KHAH medial */
                case 0xFEE4:
                  lapresult = 0xFCD1;
                  break;        /* MEEM medial */
                }
              break;

            case 0xFED3:       /* FEH initial */
              switch (string[si])
                {
                case 0xFEF2:
                  lapresult = 0xFC32;
                  break;        /* YEH final */
                }
              break;

            default:
              break;
            }                   /* end switch string[si] */
        }
      if (lapresult != 0)
        {
          string[j] = lapresult;
          (*len)--;
          si++;                 /* jump over one character */
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
arabic_reshape (long *len, const char *text, gunichar * string,
                arabic_level level)
{
  shape (len, text, string, level);
  if (level & (ar_composedtashkeel | ar_lig))
    doublelig (len, string, level);
}
