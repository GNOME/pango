/*
 * $XFree86: xc/lib/MiniXft/xftlist.c,v 1.2 2000/12/07 23:57:28 keithp Exp $
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

#include <stdlib.h>
#include "minixftint.h"

MiniXftObjectSet *
MiniXftObjectSetCreate (void)
{
    MiniXftObjectSet    *os;

    os = (MiniXftObjectSet *) malloc (sizeof (MiniXftObjectSet));
    if (!os)
	return 0;
    os->nobject = 0;
    os->sobject = 0;
    os->objects = 0;
    return os;
}

Bool
MiniXftObjectSetAdd (MiniXftObjectSet *os, const char *object)
{
    int		s;
    const char	**objects;
    
    if (os->nobject == os->sobject)
    {
	s = os->sobject + 4;
	if (os->objects)
	    objects = (const char **) realloc ((void *) os->objects,
					       s * sizeof (const char **));
	else
	    objects = (const char **) malloc (s * sizeof (const char **));
	if (!objects)
	    return False;
	os->objects = objects;
	os->sobject = s;
    }
    os->objects[os->nobject++] = object;
    return True;
}

void
MiniXftObjectSetDestroy (MiniXftObjectSet *os)
{
    if (os->objects)
	free ((void *) os->objects);
    free (os);
}

#define _MiniXftObjectSetVapBuild(__ret__, __first__, __va__) 		\
{									\
    MiniXftObjectSet    *__os__;						\
    const char	    *__ob__;						\
									\
    __ret__ = 0;						    	\
    __os__ = MiniXftObjectSetCreate ();					\
    if (!__os__)							\
	goto _MiniXftObjectSetVapBuild_bail0;				\
    __ob__ = __first__;							\
    while (__ob__)							\
    {									\
	if (!MiniXftObjectSetAdd (__os__, __ob__))				\
	    goto _MiniXftObjectSetVapBuild_bail1;				\
	__ob__ = va_arg (__va__, const char *);				\
    }									\
    __ret__ = __os__;							\
									\
_MiniXftObjectSetVapBuild_bail1:						\
    if (!__ret__ && __os__)					    	\
	MiniXftObjectSetDestroy (__os__);					\
_MiniXftObjectSetVapBuild_bail0:						\
    ;									\
}

MiniXftObjectSet *
MiniXftObjectSetVaBuild (const char *first, va_list va)
{
    MiniXftObjectSet    *ret;

    _MiniXftObjectSetVapBuild (ret, first, va);
    return ret;
}

MiniXftObjectSet *
MiniXftObjectSetBuild (const char *first, ...)
{
    va_list	    va;
    MiniXftObjectSet    *os;

    va_start (va, first);
    _MiniXftObjectSetVapBuild (os, first, va);
    va_end (va);
    return os;
}

Bool
MiniXftListValueCompare (MiniXftValue	v1,
		     MiniXftValue	v2)
{
    return _MiniXftConfigCompareValue (v1, MiniXftOpEqual, v2);
}

Bool
MiniXftListValueListCompare (MiniXftValueList	*v1orig,
			 MiniXftValueList	*v2orig,
			 MiniXftQual	qual)
{
    MiniXftValueList    *v1, *v2;

    for (v1 = v1orig; v1; v1 = v1->next)
    {
	for (v2 = v2orig; v2; v2 = v2->next)
	{
	    if (_MiniXftConfigCompareValue (v1->value, MiniXftOpEqual, v2->value))
	    {
		if (qual == MiniXftQualAny)
		    return True;
		else
		    break;
	    }
	}
	if (qual == MiniXftQualAll)
	{
	    if (!v2)
		return False;
	}
    }
    if (qual == MiniXftQualAll)
	return True;
    else
	return False;
}

/*
 * True iff all objects in "p" match "font"
 */
Bool
MiniXftListMatch (MiniXftPattern    *p,
	      MiniXftPattern    *font,
	      MiniXftQual	    qual)
{
    int		    i;
    MiniXftPatternElt   *e;

    for (i = 0; i < p->num; i++)
    {
	e = MiniXftPatternFind (font, p->elts[i].object, False);
	if (!e)
	{
	    if (qual == MiniXftQualAll)
		continue;
	    else
		return False;
	}
	if (!MiniXftListValueListCompare (p->elts[i].values, e->values, qual))
	    return False;
    }
    return True;
}

Bool
MiniXftListAppend (MiniXftFontSet   *s,
	       MiniXftPattern   *font,
	       MiniXftObjectSet *os)
{
    int		    f;
    int		    o;
    MiniXftPattern	    *l;
    MiniXftPatternElt   *e;
    MiniXftValueList    *v;

    for (f = 0; f < s->nfont; f++)
    {
	l = s->fonts[f];
	if (MiniXftListMatch (l, font, MiniXftQualAll))
	    return True;
    }
    l = MiniXftPatternCreate ();
    if (!l)
	goto bail0;
    for (o = 0; o < os->nobject; o++)
    {
	e = MiniXftPatternFind (font, os->objects[o], False);
	if (e)
	{
	    for (v = e->values; v; v = v->next)
	    {
		if (!MiniXftPatternAdd (l, os->objects[o], v->value, True))
		    goto bail1;
	    }
	}
    }
    if (!MiniXftFontSetAdd (s, l))
	goto bail1;
    return True;
bail1:
    MiniXftPatternDestroy (l);
bail0:
    return False;
}

MiniXftFontSet *
MiniXftListFontSets (MiniXftFontSet	**sets,
		 int		nsets,
		 MiniXftPattern	*p,
		 MiniXftObjectSet	*os)
{
    MiniXftFontSet	*ret;
    MiniXftFontSet	*s;
    int		f;
    int		set;

    ret = MiniXftFontSetCreate ();
    if (!ret)
	goto bail0;
    for (set = 0; set < nsets; set++)
    {
	s = sets[set];
	for (f = 0; f < s->nfont; f++)
	{
	    if (MiniXftListMatch (p, s->fonts[f], MiniXftQualAny))
	    {
		if (!MiniXftListAppend (ret, s->fonts[f], os))
		    goto bail1;
	    }
	}
    }
    return ret;
bail1:
    MiniXftFontSetDestroy (ret);
bail0:
    return 0;
}

MiniXftFontSet *
MiniXftListFontsPatternObjects (Display	    *dpy,
			    int		    screen,
			    MiniXftPattern	    *pattern,
			    MiniXftObjectSet    *os)
{
    MiniXftFontSet	*sets[2];
    int		nsets = 0;

    if (!MiniXftInit (0))
	return 0;

    if (MiniXftInitFtLibrary ())
      {
	sets[nsets] = _MiniXftFontSet;
	if (sets[nsets])
	  nsets++;
      }
    return MiniXftListFontSets (sets, nsets, pattern, os);
}

MiniXftFontSet *
MiniXftListFonts (Display	*dpy,
	      int	screen,
	      ...)
{
    va_list	    va;
    MiniXftFontSet	    *fs;
    MiniXftObjectSet    *os;
    MiniXftPattern	    *pattern;
    const char	    *first;

    va_start (va, screen);
    
    _MiniXftPatternVapBuild (pattern, 0, va);
    
    first = va_arg (va, const char *);
    _MiniXftObjectSetVapBuild (os, first, va);
    
    va_end (va);
    
    fs = MiniXftListFontsPatternObjects (dpy, screen, pattern, os);
    MiniXftPatternDestroy (pattern);
    MiniXftObjectSetDestroy (os);
    return fs;
}
