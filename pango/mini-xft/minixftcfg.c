/*
 * $XFree86: xc/lib/MiniXft/xftcfg.c,v 1.9 2001/03/31 01:57:20 keithp Exp $
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
#include <stdio.h>
#include "minixftint.h"

static char	*MiniXftConfigDefaultDirs[] = {
    "/usr/X11R6/lib/X11/fonts/Type1",
    0
};

char		**MiniXftConfigDirs = MiniXftConfigDefaultDirs;
static int	MiniXftConfigNdirs;

char		MiniXftConfigDefaultCache[] = "~/.xftcache";
char		*MiniXftConfigCache = 0;

static MiniXftSubst	*MiniXftSubsts;
/* #define  XFT_DEBUG_EDIT */

Bool
MiniXftConfigAddDir (char *d)
{
    char    **dirs;
    char    *dir;
    char    *h;

    if (*d == '~')
    {
	h = getenv ("HOME");
	if (!h)
	    return False;
	dir = (char *) malloc (strlen (h) + strlen (d));
	if (!dir)
	    return False;
	strcpy (dir, h);
	strcat (dir, d+1);
    }
    else
    {
	dir = (char *) malloc (strlen (d) + 1);
	if (!dir)
	    return False;
	strcpy (dir, d);
    }
    dirs = (char **) malloc ((MiniXftConfigNdirs + 2) * sizeof (char *));
    if (!dirs)
    {
	free (dir);
	return False;
    }
    if (MiniXftConfigNdirs)
    {
	memcpy (dirs, MiniXftConfigDirs, MiniXftConfigNdirs * sizeof (char *));
    }
    dirs[MiniXftConfigNdirs] = dir;
    MiniXftConfigNdirs++;
    dirs[MiniXftConfigNdirs] = 0;
    if (MiniXftConfigDirs != MiniXftConfigDefaultDirs)
	free (MiniXftConfigDirs);
    MiniXftConfigDirs = dirs;
    return True;
}

Bool
MiniXftConfigSetCache (char *c)
{
    char    *new;
    char    *h;

    if (*c == '~')
    {
	h = getenv ("HOME");
	if (!h)
	    return False;
	new = (char *) malloc (strlen (h) + strlen (c));
	if (!new)
	    return False;
	strcpy (new, h);
	strcat (new, c+1);
    }
    else
    {
	new = _MiniXftSaveString (c);
    }
    if (MiniXftConfigCache)
	free (MiniXftConfigCache);
    MiniXftConfigCache = new;
    return True;
}

char *
MiniXftConfigGetCache (void)
{
    if (!MiniXftConfigCache)
	MiniXftConfigSetCache (MiniXftConfigDefaultCache);
    return MiniXftConfigCache;
}

static int MiniXftSubstsMaxObjects;

Bool
MiniXftConfigAddEdit (MiniXftTest *test, MiniXftEdit *edit)
{
    MiniXftSubst	*subst, **prev;
    MiniXftTest	*t;
    int		num;

    subst = (MiniXftSubst *) malloc (sizeof (MiniXftSubst));
    if (!subst)
	return False;
    for (prev = &MiniXftSubsts; *prev; prev = &(*prev)->next);
    *prev = subst;
    subst->next = 0;
    subst->test = test;
    subst->edit = edit;
#ifdef XFT_DEBUG_EDIT
    printf ("Add Subst ");
    MiniXftSubstPrint (subst);
#endif
    num = 0;
    for (t = test; t; t = t->next)
	num++;
    if (MiniXftSubstsMaxObjects < num)
	MiniXftSubstsMaxObjects = num;
    return True;
}

typedef struct _MiniXftSubState {
    MiniXftPatternElt   *elt;
    MiniXftValueList    *value;
} MiniXftSubState;

static MiniXftMatrix    MiniXftIdentityMatrix = { 1, 0, 0, 1 };

static MiniXftValue
_MiniXftConfigPromote (MiniXftValue v, MiniXftValue u)
{
    if (v.type == MiniXftTypeInteger)
    {
	v.type = MiniXftTypeDouble;
	v.u.d = (double) v.u.i;
    }
    if (v.type == MiniXftTypeVoid && u.type == MiniXftTypeMatrix)
    {
	v.u.m = &MiniXftIdentityMatrix;
	v.type = MiniXftTypeMatrix;
    }
    return v;
}

Bool
_MiniXftConfigCompareValue (MiniXftValue    m,
			MiniXftOp	    op,
			MiniXftValue    v)
{
    Bool    ret;
    
    if (m.type == MiniXftTypeVoid)
	return True;
    m = _MiniXftConfigPromote (m, v);
    v = _MiniXftConfigPromote (v, m);
    if (m.type == v.type) 
    {
	ret = False;
	switch (m.type) {
	case MiniXftTypeDouble:
	    switch (op) {
	    case MiniXftOpEqual:    
		ret = m.u.d == v.u.d;
		break;
	    case MiniXftOpNotEqual:    
		ret = m.u.d != v.u.d;
		break;
	    case MiniXftOpLess:    
		ret = m.u.d < v.u.d;
		break;
	    case MiniXftOpLessEqual:    
		ret = m.u.d <= v.u.d;
		break;
	    case MiniXftOpMore:    
		ret = m.u.d > v.u.d;
		break;
	    case MiniXftOpMoreEqual:    
		ret = m.u.d >= v.u.d;
		break;
	    default:
		break;
	    }
	    break;
	case MiniXftTypeBool:
	    switch (op) {
	    case MiniXftOpEqual:    
		ret = m.u.b == v.u.b;
		break;
	    case MiniXftOpNotEqual:    
		ret = m.u.b != v.u.b;
		break;
	    default:
		break;
	    }
	    break;
	case MiniXftTypeString:
	    switch (op) {
	    case MiniXftOpEqual:    
		ret = _MiniXftStrCmpIgnoreCase (m.u.s, v.u.s) == 0;
		break;
	    case MiniXftOpNotEqual:    
		ret = _MiniXftStrCmpIgnoreCase (m.u.s, v.u.s) != 0;
		break;
	    default:
		break;
	    }
	    break;
	default:
	    break;
	}
    }
    else
    {
	if (op == MiniXftOpNotEqual)
	    ret = True;
	else
	    ret = False;
    }
    return ret;
}

static MiniXftValueList *
_MiniXftConfigMatchValueList (MiniXftTest	*t,
			  MiniXftValueList  *v)
{
    MiniXftValueList    *ret = 0;
    
    for (; v; v = v->next)
    {
	if (_MiniXftConfigCompareValue (v->value, t->op, t->value))
	{
	    if (!ret)
		ret = v;
	}
	else
	{
	    if (t->qual == MiniXftQualAll)
	    {
		ret = 0;
		break;
	    }
	}
    }
    return ret;
}

static MiniXftValue
_MiniXftConfigEvaluate (MiniXftPattern *p, MiniXftExpr *e)
{
    MiniXftValue	v, vl, vr;
    MiniXftResult	r;
    
    switch (e->op) {
    case MiniXftOpInteger:
	v.type = MiniXftTypeInteger;
	v.u.i = e->u.ival;
	break;
    case MiniXftOpDouble:
	v.type = MiniXftTypeDouble;
	v.u.d = e->u.dval;
	break;
    case MiniXftOpString:
	v.type = MiniXftTypeString;
	v.u.s = e->u.sval;
	break;
    case MiniXftOpMatrix:
	v.type = MiniXftTypeMatrix;
	v.u.m = e->u.mval;
	break;
    case MiniXftOpBool:
	v.type = MiniXftTypeBool;
	v.u.b = e->u.bval;
	break;
    case MiniXftOpField:
	r = MiniXftPatternGet (p, e->u.field, 0, &v);
	if (r != MiniXftResultMatch)
	    v.type = MiniXftTypeVoid;
	break;
    case MiniXftOpQuest:
	vl = _MiniXftConfigEvaluate (p, e->u.tree.left);
	if (vl.type == MiniXftTypeBool)
	{
	    if (vl.u.b)
		v = _MiniXftConfigEvaluate (p, e->u.tree.right->u.tree.left);
	    else
		v = _MiniXftConfigEvaluate (p, e->u.tree.right->u.tree.right);
	}
	else
	    v.type = MiniXftTypeVoid;
	break;
    case MiniXftOpOr:
    case MiniXftOpAnd:
    case MiniXftOpEqual:
    case MiniXftOpNotEqual:
    case MiniXftOpLess:
    case MiniXftOpLessEqual:
    case MiniXftOpMore:
    case MiniXftOpMoreEqual:
    case MiniXftOpPlus:
    case MiniXftOpMinus:
    case MiniXftOpTimes:
    case MiniXftOpDivide:
	vl = _MiniXftConfigEvaluate (p, e->u.tree.left);
	vr = _MiniXftConfigEvaluate (p, e->u.tree.right);
	vl = _MiniXftConfigPromote (vl, vr);
	vr = _MiniXftConfigPromote (vr, vl);
	if (vl.type == vr.type)
	{
	    switch (vl.type) {
	    case MiniXftTypeDouble:
		switch (e->op) {
		case MiniXftOpPlus:	   
		    v.type = MiniXftTypeDouble;
		    v.u.d = vl.u.d + vr.u.d; 
		    break;
		case MiniXftOpMinus:
		    v.type = MiniXftTypeDouble;
		    v.u.d = vl.u.d - vr.u.d; 
		    break;
		case MiniXftOpTimes:
		    v.type = MiniXftTypeDouble;
		    v.u.d = vl.u.d * vr.u.d; 
		    break;
		case MiniXftOpDivide:
		    v.type = MiniXftTypeDouble;
		    v.u.d = vl.u.d / vr.u.d; 
		    break;
		case MiniXftOpEqual:    
		    v.type = MiniXftTypeBool; 
		    v.u.b = vl.u.d == vr.u.d;
		    break;
		case MiniXftOpNotEqual:    
		    v.type = MiniXftTypeBool; 
		    v.u.b = vl.u.d != vr.u.d;
		    break;
		case MiniXftOpLess:    
		    v.type = MiniXftTypeBool; 
		    v.u.b = vl.u.d < vr.u.d;
		    break;
		case MiniXftOpLessEqual:    
		    v.type = MiniXftTypeBool; 
		    v.u.b = vl.u.d <= vr.u.d;
		    break;
		case MiniXftOpMore:    
		    v.type = MiniXftTypeBool; 
		    v.u.b = vl.u.d > vr.u.d;
		    break;
		case MiniXftOpMoreEqual:    
		    v.type = MiniXftTypeBool; 
		    v.u.b = vl.u.d >= vr.u.d;
		    break;
		default:
		    v.type = MiniXftTypeVoid; 
		    break;
		}
		if (v.type == MiniXftTypeDouble &&
		    v.u.d == (double) (int) v.u.d)
		{
		    v.type = MiniXftTypeInteger;
		    v.u.i = (int) v.u.d;
		}
		break;
	    case MiniXftTypeBool:
		switch (e->op) {
		case MiniXftOpOr:
		    v.type = MiniXftTypeBool;
		    v.u.b = vl.u.b || vr.u.b;
		    break;
		case MiniXftOpAnd:
		    v.type = MiniXftTypeBool;
		    v.u.b = vl.u.b && vr.u.b;
		    break;
		case MiniXftOpEqual:
		    v.type = MiniXftTypeBool;
		    v.u.b = vl.u.b == vr.u.b;
		    break;
		case MiniXftOpNotEqual:
		    v.type = MiniXftTypeBool;
		    v.u.b = vl.u.b != vr.u.b;
		    break;
		default:
		    v.type = MiniXftTypeVoid; 
		    break;
		}
		break;
	    case MiniXftTypeString:
		switch (e->op) {
		case MiniXftOpEqual:
		    v.type = MiniXftTypeBool;
		    v.u.b = _MiniXftStrCmpIgnoreCase (vl.u.s, vr.u.s) == 0;
		    break;
		case MiniXftOpNotEqual:
		    v.type = MiniXftTypeBool;
		    v.u.b = _MiniXftStrCmpIgnoreCase (vl.u.s, vr.u.s) != 0;
		    break;
		case MiniXftOpPlus:
		    v.type = MiniXftTypeString;
		    v.u.s = malloc (strlen (vl.u.s) + strlen (vr.u.s) + 1);
		    if (v.u.s)
		    {
			strcpy (v.u.s, vl.u.s);
			strcat (v.u.s, vr.u.s);
		    }
		    else
			v.type = MiniXftTypeVoid;
		    break;
		default:
		    v.type = MiniXftTypeVoid;
		    break;
		}
	    case MiniXftTypeMatrix:
		switch (e->op) {
		case MiniXftOpEqual:
		    v.type = MiniXftTypeBool;
		    v.u.b = MiniXftMatrixEqual (vl.u.m, vr.u.m) == 0;
		    break;
		case MiniXftOpNotEqual:
		    v.type = MiniXftTypeBool;
		    v.u.b = MiniXftMatrixEqual (vl.u.m, vr.u.m) != 0;
		    break;
		case MiniXftOpTimes:
		    v.type = MiniXftTypeMatrix;
		    v.u.m = malloc (sizeof (MiniXftMatrix));
		    MiniXftMatrixMultiply (v.u.m, vl.u.m, vr.u.m);
		    break;
		default:
		    v.type = MiniXftTypeVoid;
		    break;
		}
		break;
	    default:
		v.type = MiniXftTypeVoid;
		break;
	    }
	}
	else
	    v.type = MiniXftTypeVoid;
	break;
    case MiniXftOpNot:
	vl = _MiniXftConfigEvaluate (p, e->u.tree.left);
	switch (vl.type) {
	case MiniXftTypeBool:
	    v.type = MiniXftTypeBool;
	    v.u.b = !vl.u.b;
	    break;
	default:
	    v.type = MiniXftTypeVoid;
	    break;
	}
	break;
    default:
	v.type = MiniXftTypeVoid;
	break;
    }
    return v;
}

static Bool
_MiniXftConfigAdd (MiniXftValueList  **head,
	      MiniXftValueList  *position,
	      Bool	    append,
	      MiniXftValue	    value)
{
    MiniXftValueList    *new, **prev;
    
    new = (MiniXftValueList *) malloc (sizeof (MiniXftValueList));
    if (!new)
	goto bail0;
    
    if (value.type == MiniXftTypeString)
    {
	value.u.s = _MiniXftSaveString (value.u.s);
	if (!value.u.s)
	    goto bail1;
	
    }
    new->value = value;
    new->next = 0;

    if (append)
    {
	prev = &position->next;
    }
    else
    {
	for (prev = head; *prev; prev = &(*prev)->next)
	{
	    if (*prev == position)
		break;
	}
#ifdef XFT_DEBUG
	if (!*prev)
	    printf ("position not on list\n");
#endif
    }

#ifdef XFT_DEBUG_EDIT
    printf ("%s list before ", append ? "Append" : "Prepend");
    MiniXftValueListPrint (*head);
    printf ("\n");
#endif
    
    new->next = *prev;
    *prev = new;
    
#ifdef XFT_DEBUG_EDIT
    printf ("%s list after ", append ? "Append" : "Prepend");
    MiniXftValueListPrint (*head);
    printf ("\n");
#endif
    
    return True;
    
bail1:
    free (new);
bail0:
    return False;
}

static void
_MiniXftConfigDel (MiniXftValueList	**head,
	      MiniXftValueList	*position)
{
    MiniXftValueList    **prev;

    for (prev = head; *prev; prev = &(*prev)->next)
    {
	if (*prev == position)
	{
	    *prev = position->next;
	    position->next = 0;
	    MiniXftValueListDestroy (position);
	    break;
	}
    }
}

Bool
MiniXftConfigSubstitute (MiniXftPattern *p)
{
    MiniXftSubst	    *s;
    MiniXftSubState	    *st;
    int		    i;
    MiniXftTest	    *t;
    MiniXftEdit	    *e;
    MiniXftValue	    v;

    st = (MiniXftSubState *) malloc (MiniXftSubstsMaxObjects * sizeof (MiniXftSubState));
    if (!st && MiniXftSubstsMaxObjects)
	return False;

#ifdef XFT_DEBUG_EDIT
    printf ("MiniXftConfigSubstitute ");
    MiniXftPatternPrint (p);
#endif
    for (s = MiniXftSubsts; s; s = s->next)
    {
	for (t = s->test, i = 0; t; t = t->next, i++)
	{
#ifdef XFT_DEBUG_EDIT
	    printf ("MiniXftConfigSubstitute test ");
	    MiniXftTestPrint (t);
#endif
	    st[i].elt = MiniXftPatternFind (p, t->field, False);
	    if (!st[i].elt)
	    {
		if (t->qual == MiniXftQualAll)
		    continue;
		else
		    break;
	    }
	    st[i].value = _MiniXftConfigMatchValueList (t, st[i].elt->values);
	    if (!st[i].value)
		break;
	}
	if (t)
	{
#ifdef XFT_DEBUG_EDIT
	    printf ("No match\n");
#endif
	    continue;
	}
#ifdef XFT_DEBUG_EDIT
	printf ("Substitute ");
	MiniXftSubstPrint (s);
#endif
	for (e = s->edit; e; e = e->next)
	{
	    v = _MiniXftConfigEvaluate (p, e->expr);
	    if (v.type == MiniXftTypeVoid)
		continue;
	    for (t = s->test, i = 0; t; t = t->next, i++)
		if (!_MiniXftStrCmpIgnoreCase (t->field, e->field))
		    break;
	    switch (e->op) {
	    case MiniXftOpAssign:
		if (t)
		{
		    _MiniXftConfigAdd (&st[i].elt->values, st[i].value, True, v);
		    _MiniXftConfigDel (&st[i].elt->values, st[i].value);
		}
		else
		{
		    MiniXftPatternDel (p, e->field);
		    MiniXftPatternAdd (p, e->field, v, True);
		}
		break;
	    case MiniXftOpPrepend:
		if (t)
		    _MiniXftConfigAdd (&st[i].elt->values, st[i].value, False, v);
		else
		    MiniXftPatternAdd (p, e->field, v, False);
		break;
	    case MiniXftOpAppend:
		if (t)
		    _MiniXftConfigAdd (&st[i].elt->values, st[i].value, True, v);
		else
		    MiniXftPatternAdd (p, e->field, v, True);
		break;
	    default:
		break;
	    }
	}
#ifdef XFT_DEBUG_EDIT
	printf ("MiniXftConfigSubstitute edit");
	MiniXftPatternPrint (p);
#endif
    }
    free (st);
#ifdef XFT_DEBUG_EDIT
    printf ("MiniXftConfigSubstitute done");
    MiniXftPatternPrint (p);
#endif
    return True;
}
