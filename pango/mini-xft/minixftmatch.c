/*
 * $XFree86: xc/lib/MiniXft/xftmatch.c,v 1.6 2001/09/21 19:54:53 keithp Exp $
 *
 * Copyright © 2000 Keith Packard, member of The XFree86 Project, Inc.
 *
 * Permission to use, copy, modify, distribute, and sell this software and its
 * documentation for any purpose is hereby granted without fee, provided that
 * the above copyright notice appear in all copies and that both that
 * copyright notice and this permission notice appear in supporting
 * documentation, and that the name of Keith Packard not be used in
 * advertising or publicity pertaining to distribution of the software without
 * specific, written prior permission.  Keith Packard makes no
 * representations about the suitability of this software for any purpose.  It
 * is provided "as is" without express or implied warranty.
 *
 * KEITH PACKARD DISCLAIMS ALL WARRANTIES WITH REGARD TO THIS SOFTWARE,
 * INCLUDING ALL IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS, IN NO
 * EVENT SHALL KEITH PACKARD BE LIABLE FOR ANY SPECIAL, INDIRECT OR
 * CONSEQUENTIAL DAMAGES OR ANY DAMAGES WHATSOEVER RESULTING FROM LOSS OF USE,
 * DATA OR PROFITS, WHETHER IN AN ACTION OF CONTRACT, NEGLIGENCE OR OTHER
 * TORTIOUS ACTION, ARISING OUT OF OR IN CONNECTION WITH THE USE OR
 * PERFORMANCE OF THIS SOFTWARE.
 */

#include <string.h>
#include <ctype.h>
#include "minixftint.h"
#include <stdio.h>

static double
_MiniXftCompareInteger (char *object, MiniXftValue value1, MiniXftValue value2)
{
    int	v;
    
    if (value2.type != MiniXftTypeInteger || value1.type != MiniXftTypeInteger)
	return -1.0;
    v = value2.u.i - value1.u.i;
    if (v < 0)
	v = -v;
    return (double) v;
}

static double
_MiniXftCompareString (char *object, MiniXftValue value1, MiniXftValue value2)
{
    if (value2.type != MiniXftTypeString || value1.type != MiniXftTypeString)
	return -1.0;
    return (double) _MiniXftStrCmpIgnoreCase (value1.u.s, value2.u.s) != 0;
}

static double
_MiniXftCompareBool (char *object, MiniXftValue value1, MiniXftValue value2)
{
    if (value2.type != MiniXftTypeBool || value1.type != MiniXftTypeBool)
	return -1.0;
    return (double) value2.u.b != value1.u.b;
}

static double
_MiniXftCompareSize (char *object, MiniXftValue value1, MiniXftValue value2)
{
    double  v1, v2, v;

    switch (value1.type) {
    case MiniXftTypeInteger:
	v1 = value1.u.i;
	break;
    case MiniXftTypeDouble:
	v1 = value1.u.d;
	break;
    default:
	return -1;
    }
    switch (value2.type) {
    case MiniXftTypeInteger:
	v2 = value2.u.i;
	break;
    case MiniXftTypeDouble:
	v2 = value2.u.d;
	break;
    default:
	return -1;
    }
    if (v2 == 0)
	return 0;
    v = v2 - v1;
    if (v < 0)
	v = -v;
    return v;
}

/*
 * Order is significant, it defines the precedence of
 * each value, earlier values are more significant than
 * later values
 */
static MiniXftMatcher _MiniXftMatchers [] = {
    { XFT_FOUNDRY,	_MiniXftCompareString, },
    { XFT_ENCODING,	_MiniXftCompareString, },
    { XFT_FAMILY,	_MiniXftCompareString, },
    { XFT_SPACING,	_MiniXftCompareInteger, },
    { XFT_PIXEL_SIZE,	_MiniXftCompareSize, },
    { XFT_STYLE,	_MiniXftCompareString, },
    { XFT_SLANT,	_MiniXftCompareInteger, },
    { XFT_WEIGHT,	_MiniXftCompareInteger, },
    { XFT_RASTERIZER,	_MiniXftCompareString, },
    { XFT_ANTIALIAS,	_MiniXftCompareBool, },
    { XFT_OUTLINE,	_MiniXftCompareBool, },
};

#define NUM_MATCHER (sizeof _MiniXftMatchers / sizeof _MiniXftMatchers[0])

static Bool
_MiniXftCompareValueList (const char    *object,
		      MiniXftValueList  *v1orig,	/* pattern */
		      MiniXftValueList  *v2orig,	/* target */
		      MiniXftValue	    *bestValue,
		      double	    *value,
		      MiniXftResult	    *result)
{
    MiniXftValueList    *v1, *v2;
    double    	    v, best;
    int		    j;
    int		    i;
    
    for (i = 0; i < NUM_MATCHER; i++)
    {
	if (!_MiniXftStrCmpIgnoreCase (_MiniXftMatchers[i].object, object))
	    break;
    }
    if (i == NUM_MATCHER)
    {
	if (bestValue)
	    *bestValue = v2orig->value;
	return True;
    }
    
    best = 1e99;
    j = 0;
    for (v1 = v1orig; v1; v1 = v1->next)
    {
	for (v2 = v2orig; v2; v2 = v2->next)
	{
	    v = (*_MiniXftMatchers[i].compare) (_MiniXftMatchers[i].object,
					    v1->value,
					    v2->value);
	    if (v < 0)
	    {
		*result = MiniXftResultTypeMismatch;
		return False;
	    }
	    if (_MiniXftFontDebug () & XFT_DBG_MATCHV)
		printf (" v %g j %d ", v, j);
	    v = v * 100 + j;
	    if (v < best)
	    {
		if (bestValue)
		    *bestValue = v2->value;
		best = v;
	    }
	}
	j++;
    }
    if (_MiniXftFontDebug () & XFT_DBG_MATCHV)
    {
	printf (" %s: %g ", object, best);
	MiniXftValueListPrint (v1orig);
	printf (", ");
	MiniXftValueListPrint (v2orig);
	printf ("\n");
    }
    value[i] += best;
    return True;
}

/*
 * Return a value indicating the distance between the two lists of
 * values
 */

static Bool
_MiniXftCompare (MiniXftPattern	*pat,
	     MiniXftPattern *fnt,
	     double	*value,
	     MiniXftResult	*result)
{
    int		    i, i1, i2;
    
    for (i = 0; i < NUM_MATCHER; i++)
	value[i] = 0.0;
    
    for (i1 = 0; i1 < pat->num; i1++)
    {
	for (i2 = 0; i2 < fnt->num; i2++)
	{
	    if (!_MiniXftStrCmpIgnoreCase (pat->elts[i1].object,
				       fnt->elts[i2].object))
	    {
		if (!_MiniXftCompareValueList (pat->elts[i1].object,
					   pat->elts[i1].values,
					   fnt->elts[i2].values,
					   0,
					   value,
					   result))
		    return False;
		break;
	    }
	}
#if 0
	/*
	 * Overspecified patterns are slightly penalized in
	 * case some other font includes the requested field
	 */
	if (i2 == fnt->num)
	{
	    for (i2 = 0; i2 < NUM_MATCHER; i2++)
	    {
		if (!_MiniXftStrCmpIgnoreCase (_MiniXftMatchers[i2].object,
					   pat->elts[i1].object))
		{
		    value[i2] = 1.0;
		    break;
		}
	    }
	}
#endif
    }
    return True;
}

MiniXftPattern *
MiniXftFontSetMatch (MiniXftFontSet	**sets, 
		 int		nsets, 
		 MiniXftPattern	*p, 
		 MiniXftResult	*result)
{
    double    	    score[NUM_MATCHER], bestscore[NUM_MATCHER];
    int		    f;
    MiniXftFontSet	    *s;
    MiniXftPattern	    *best;
    MiniXftPattern	    *new;
    MiniXftPatternElt   *fe, *pe;
    MiniXftValue	    v;
    int		    i;
    int		    set;

    for (i = 0; i < NUM_MATCHER; i++)
	bestscore[i] = 0;
    best = 0;
    if (_MiniXftFontDebug () & XFT_DBG_MATCH)
    {
	printf ("Match ");
	MiniXftPatternPrint (p);
    }
    for (set = 0; set < nsets; set++)
    {
	s = sets[set];
	for (f = 0; f < s->nfont; f++)
	{
	    if (_MiniXftFontDebug () & XFT_DBG_MATCH)
	    {
		printf ("Font %d ", f);
		MiniXftPatternPrint (s->fonts[f]);
	    }
	    if (!_MiniXftCompare (p, s->fonts[f], score, result))
		return 0;
	    if (_MiniXftFontDebug () & XFT_DBG_MATCH)
	    {
		printf ("Score");
		for (i = 0; i < NUM_MATCHER; i++)
		{
		    printf (" %g", score[i]);
		}
		printf ("\n");
	    }
	    for (i = 0; i < NUM_MATCHER; i++)
	    {
		if (best && bestscore[i] < score[i])
		    break;
		if (!best || score[i] < bestscore[i])
		{
		    for (i = 0; i < NUM_MATCHER; i++)
			bestscore[i] = score[i];
		    best = s->fonts[f];
		    break;
		}
	    }
	}
    }
    if (_MiniXftFontDebug () & XFT_DBG_MATCH)
    {
	printf ("Best score");
	for (i = 0; i < NUM_MATCHER; i++)
	    printf (" %g", bestscore[i]);
	MiniXftPatternPrint (best);
    }
    if (!best)
    {
	*result = MiniXftResultNoMatch;
	return 0;
    }
    new = MiniXftPatternCreate ();
    if (!new)
	return 0;
    for (i = 0; i < best->num; i++)
    {
	fe = &best->elts[i];
	pe = MiniXftPatternFind (p, fe->object, False);
	if (pe)
	{
	    if (!_MiniXftCompareValueList (pe->object, pe->values, 
				       fe->values, &v, score, result))
	    {
		MiniXftPatternDestroy (new);
		return 0;
	    }
	}
	else
	    v = fe->values->value;
	MiniXftPatternAdd (new, fe->object, v, True);
    }
    for (i = 0; i < p->num; i++)
    {
	pe = &p->elts[i];
	fe = MiniXftPatternFind (best, pe->object, False);
	if (!fe)
	    MiniXftPatternAdd (new, pe->object, pe->values->value, True);
    }
    return new;
}
