/*
 * $XFree86: xc/lib/MiniXft/xftpat.c,v 1.6 2001/03/30 18:50:18 keithp Exp $
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
#include <string.h>
#include "minixftint.h"

MiniXftPattern *
MiniXftPatternCreate (void)
{
    MiniXftPattern	*p;

    p = (MiniXftPattern *) malloc (sizeof (MiniXftPattern));
    if (!p)
	return 0;
    p->num = 0;
    p->size = 0;
    p->elts = 0;
    return p;
}

void
MiniXftValueDestroy (MiniXftValue v)
{
    if (v.type == MiniXftTypeString)
	free (v.u.s);
    if( v.type == MiniXftTypeMatrix)
	free (v.u.m);
}

void
MiniXftValueListDestroy (MiniXftValueList *l)
{
    MiniXftValueList    *next;
    for (; l; l = next)
    {
	if (l->value.type == MiniXftTypeString)
	    free (l->value.u.s);
	if (l->value.type == MiniXftTypeMatrix)
	    free (l->value.u.m);
	next = l->next;
	free (l);
    }
}

void
MiniXftPatternDestroy (MiniXftPattern *p)
{
    int		    i;
    
    for (i = 0; i < p->num; i++)
	MiniXftValueListDestroy (p->elts[i].values);

    if (p->elts)
    {
	free (p->elts);
	p->elts = 0;
    }
    p->num = p->size = 0;
    free (p);
}

MiniXftPatternElt *
MiniXftPatternFind (MiniXftPattern *p, const char *object, Bool insert)
{
    int		    i;
    int		    s;
    MiniXftPatternElt   *e;
    
    /* match existing */
    for (i = 0; i < p->num; i++)
    {
	if (!_MiniXftStrCmpIgnoreCase (object, p->elts[i].object))
	    return &p->elts[i];
    }

    if (!insert)
	return 0;

    /* grow array */
    if (i == p->size)
    {
	s = p->size + 16;
	if (p->elts)
	    e = (MiniXftPatternElt *) realloc (p->elts, s * sizeof (MiniXftPatternElt));
	else
	    e = (MiniXftPatternElt *) malloc (s * sizeof (MiniXftPatternElt));
	if (!e)
	    return False;
	p->elts = e;
	while (p->size < s)
	{
	    p->elts[p->size].object = 0;
	    p->elts[p->size].values = 0;
	    p->size++;
	}
    }
    
    /* bump count */
    p->num++;
    
    return &p->elts[i];
}

Bool
MiniXftPatternAdd (MiniXftPattern *p, const char *object, MiniXftValue value, Bool append)
{
    MiniXftPatternElt   *e;
    MiniXftValueList    *new, **prev;

    new = (MiniXftValueList *) malloc (sizeof (MiniXftValueList));
    if (!new)
	goto bail0;

    /* dup string */
    if (value.type == MiniXftTypeString)
    {
	value.u.s = _MiniXftSaveString (value.u.s);
	if (!value.u.s)
	    goto bail1;
    }
    else if (value.type == MiniXftTypeMatrix)
    {
	value.u.m = _MiniXftSaveMatrix (value.u.m);
	if (!value.u.m)
	    goto bail1;
    }
    new->value = value;
    new->next = 0;
    
    e = MiniXftPatternFind (p, object, True);
    if (!e)
	goto bail2;
    
    e->object = object;
    if (append)
    {
	for (prev = &e->values; *prev; prev = &(*prev)->next);
	*prev = new;
    }
    else
    {
	new->next = e->values;
	e->values = new;
    }
    
    return True;

bail2:    
    if (value.type == MiniXftTypeString)
        free (value.u.s);
    else if (value.type == MiniXftTypeMatrix)
	free (value.u.m);
bail1:
    free (new);
bail0:
    return False;
}

Bool
MiniXftPatternDel (MiniXftPattern *p, const char *object)
{
    MiniXftPatternElt   *e;
    int		    i;

    e = MiniXftPatternFind (p, object, False);
    if (!e)
	return False;

    i = e - p->elts;
    
    /* destroy value */
    MiniXftValueListDestroy (e->values);
    
    /* shuffle existing ones down */
    memmove (e, e+1, (p->elts + p->num - (e + 1)) * sizeof (MiniXftPatternElt));
    p->num--;
    p->elts[p->num].object = 0;
    p->elts[p->num].values = 0;
    return True;
}

Bool
MiniXftPatternAddInteger (MiniXftPattern *p, const char *object, int i)
{
    MiniXftValue	v;

    v.type = MiniXftTypeInteger;
    v.u.i = i;
    return MiniXftPatternAdd (p, object, v, True);
}

Bool
MiniXftPatternAddDouble (MiniXftPattern *p, const char *object, double d)
{
    MiniXftValue	v;

    v.type = MiniXftTypeDouble;
    v.u.d = d;
    return MiniXftPatternAdd (p, object, v, True);
}


Bool
MiniXftPatternAddString (MiniXftPattern *p, const char *object, const char *s)
{
    MiniXftValue	v;

    v.type = MiniXftTypeString;
    v.u.s = (char *) s;
    return MiniXftPatternAdd (p, object, v, True);
}

Bool
MiniXftPatternAddMatrix (MiniXftPattern *p, const char *object, const MiniXftMatrix *s)
{
    MiniXftValue	v;

    v.type = MiniXftTypeMatrix;
    v.u.m = (MiniXftMatrix *) s;
    return MiniXftPatternAdd (p, object, v, True);
}


Bool
MiniXftPatternAddBool (MiniXftPattern *p, const char *object, Bool b)
{
    MiniXftValue	v;

    v.type = MiniXftTypeBool;
    v.u.b = b;
    return MiniXftPatternAdd (p, object, v, True);
}

MiniXftResult
MiniXftPatternGet (MiniXftPattern *p, const char *object, int id, MiniXftValue *v)
{
    MiniXftPatternElt   *e;
    MiniXftValueList    *l;

    e = MiniXftPatternFind (p, object, False);
    if (!e)
	return MiniXftResultNoMatch;
    for (l = e->values; l; l = l->next)
    {
	if (!id)
	{
	    *v = l->value;
	    return MiniXftResultMatch;
	}
	id--;
    }
    return MiniXftResultNoId;
}

MiniXftResult
MiniXftPatternGetInteger (MiniXftPattern *p, const char *object, int id, int *i)
{
    MiniXftValue	v;
    MiniXftResult	r;

    r = MiniXftPatternGet (p, object, id, &v);
    if (r != MiniXftResultMatch)
	return r;
    switch (v.type) {
    case MiniXftTypeDouble:
	*i = (int) v.u.d;
	break;
    case MiniXftTypeInteger:
	*i = v.u.i;
	break;
    default:
        return MiniXftResultTypeMismatch;
    }
    return MiniXftResultMatch;
}

MiniXftResult
MiniXftPatternGetDouble (MiniXftPattern *p, const char *object, int id, double *d)
{
    MiniXftValue	v;
    MiniXftResult	r;

    r = MiniXftPatternGet (p, object, id, &v);
    if (r != MiniXftResultMatch)
	return r;
    switch (v.type) {
    case MiniXftTypeDouble:
	*d = v.u.d;
	break;
    case MiniXftTypeInteger:
	*d = (double) v.u.i;
	break;
    default:
        return MiniXftResultTypeMismatch;
    }
    return MiniXftResultMatch;
}

MiniXftResult
MiniXftPatternGetString (MiniXftPattern *p, const char *object, int id, char **s)
{
    MiniXftValue	v;
    MiniXftResult	r;

    r = MiniXftPatternGet (p, object, id, &v);
    if (r != MiniXftResultMatch)
	return r;
    if (v.type != MiniXftTypeString)
        return MiniXftResultTypeMismatch;
    *s = v.u.s;
    return MiniXftResultMatch;
}

MiniXftResult
MiniXftPatternGetMatrix (MiniXftPattern *p, const char *object, int id, MiniXftMatrix **m)
{
    MiniXftValue	v;
    MiniXftResult	r;

    r = MiniXftPatternGet (p, object, id, &v);
    if (r != MiniXftResultMatch)
	return r;
    if (v.type != MiniXftTypeMatrix)
        return MiniXftResultTypeMismatch;
    *m = v.u.m;
    return MiniXftResultMatch;
}


MiniXftResult
MiniXftPatternGetBool (MiniXftPattern *p, const char *object, int id, Bool *b)
{
    MiniXftValue	v;
    MiniXftResult	r;

    r = MiniXftPatternGet (p, object, id, &v);
    if (r != MiniXftResultMatch)
	return r;
    if (v.type != MiniXftTypeBool)
        return MiniXftResultTypeMismatch;
    *b = v.u.b;
    return MiniXftResultMatch;
}

MiniXftPattern *
MiniXftPatternDuplicate (MiniXftPattern *orig)
{
    MiniXftPattern	    *new;
    int		    i;
    MiniXftValueList    *l;

    new = MiniXftPatternCreate ();
    if (!new)
	goto bail0;

    for (i = 0; i < orig->num; i++)
    {
	for (l = orig->elts[i].values; l; l = l->next)
	    if (!MiniXftPatternAdd (new, orig->elts[i].object, l->value, True))
		goto bail1;
    }

    return new;

bail1:
    MiniXftPatternDestroy (new);
bail0:
    return 0;
}

MiniXftPattern *
MiniXftPatternVaBuild (MiniXftPattern *orig, va_list va)
{
    MiniXftPattern	*ret;
    
    _MiniXftPatternVapBuild (ret, orig, va);
    return ret;
}

MiniXftPattern *
MiniXftPatternBuild (MiniXftPattern *orig, ...)
{
    va_list	va;
    
    va_start (va, orig);
    _MiniXftPatternVapBuild (orig, orig, va);
    va_end (va);
    return orig;
}
