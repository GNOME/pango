/*
 * $XFree86: xc/lib/MiniXft/xftdbg.c,v 1.3 2001/03/31 01:57:20 keithp Exp $
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

#include "minixftint.h"
#include <stdio.h>

void
MiniXftValuePrint (MiniXftValue v)
{
    switch (v.type) {
    case MiniXftTypeVoid:
	printf (" <void>");
	break;
    case MiniXftTypeInteger:
	printf (" %d", v.u.i);
	break;
    case MiniXftTypeDouble:
	printf (" %g", v.u.d);
	break;
    case MiniXftTypeString:
	printf (" \"%s\"", v.u.s);
	break;
    case MiniXftTypeBool:
	printf (" %s", v.u.b ? "True" : "False");
	break;
    case MiniXftTypeMatrix:
	printf (" (%f %f; %f %f)", v.u.m->xx, v.u.m->xy, v.u.m->yx, v.u.m->yy);
	break;
    }
}

void
MiniXftValueListPrint (MiniXftValueList *l)
{
    for (; l; l = l->next)
	MiniXftValuePrint (l->value);
}

void
MiniXftPatternPrint (MiniXftPattern *p)
{
    int		    i;
    MiniXftPatternElt   *e;
    
    printf ("Pattern %d of %d\n", p->num, p->size);
    for (i = 0; i < p->num; i++)
    {
	e = &p->elts[i];
	printf ("\t%s:", e->object);
	MiniXftValueListPrint (e->values);
	printf ("\n");
    }
    printf ("\n");
}

void
MiniXftOpPrint (MiniXftOp op)
{
    switch (op) {
    case MiniXftOpInteger: printf ("Integer"); break;
    case MiniXftOpDouble: printf ("Double"); break;
    case MiniXftOpString: printf ("String"); break;
    case MiniXftOpMatrix: printf ("Matrix"); break;
    case MiniXftOpBool: printf ("Bool"); break;
    case MiniXftOpField: printf ("Field"); break;
    case MiniXftOpAssign: printf ("Assign"); break;
    case MiniXftOpPrepend: printf ("Prepend"); break;
    case MiniXftOpAppend: printf ("Append"); break;
    case MiniXftOpQuest: printf ("Quest"); break;
    case MiniXftOpOr: printf ("Or"); break;
    case MiniXftOpAnd: printf ("And"); break;
    case MiniXftOpEqual: printf ("Equal"); break;
    case MiniXftOpNotEqual: printf ("NotEqual"); break;
    case MiniXftOpLess: printf ("Less"); break;
    case MiniXftOpLessEqual: printf ("LessEqual"); break;
    case MiniXftOpMore: printf ("More"); break;
    case MiniXftOpMoreEqual: printf ("MoreEqual"); break;
    case MiniXftOpPlus: printf ("Plus"); break;
    case MiniXftOpMinus: printf ("Minus"); break;
    case MiniXftOpTimes: printf ("Times"); break;
    case MiniXftOpDivide: printf ("Divide"); break;
    case MiniXftOpNot: printf ("Not"); break;
    case MiniXftOpNil: printf ("Nil"); break;
    }
}

void
MiniXftTestPrint (MiniXftTest *test)
{
    switch (test->qual) {
    case MiniXftQualAny:
	printf ("any ");
	break;
    case MiniXftQualAll:
	printf ("all ");
	break;
    }
    printf ("%s ", test->field);
    MiniXftOpPrint (test->op);
    printf (" ");
    MiniXftValuePrint (test->value);
    printf ("\n");
}

void
MiniXftExprPrint (MiniXftExpr *expr)
{
    switch (expr->op) {
    case MiniXftOpInteger: printf ("%d", expr->u.ival); break;
    case MiniXftOpDouble: printf ("%g", expr->u.dval); break;
    case MiniXftOpString: printf ("\"%s\"", expr->u.sval); break;
    case MiniXftOpMatrix: printf ("[%g %g %g %g]",
			      expr->u.mval->xx,
			      expr->u.mval->xy,
			      expr->u.mval->yx,
			      expr->u.mval->yy);
    case MiniXftOpBool: printf ("%s", expr->u.bval ? "true" : "false"); break;
    case MiniXftOpField: printf ("%s", expr->u.field); break;
    case MiniXftOpQuest:
	MiniXftExprPrint (expr->u.tree.left);
	printf (" quest ");
	MiniXftExprPrint (expr->u.tree.right->u.tree.left);
	printf (" colon ");
	MiniXftExprPrint (expr->u.tree.right->u.tree.right);
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
	MiniXftExprPrint (expr->u.tree.left);
	printf (" ");
	switch (expr->op) {
	case MiniXftOpOr: printf ("Or"); break;
	case MiniXftOpAnd: printf ("And"); break;
	case MiniXftOpEqual: printf ("Equal"); break;
	case MiniXftOpNotEqual: printf ("NotEqual"); break;
	case MiniXftOpLess: printf ("Less"); break;
	case MiniXftOpLessEqual: printf ("LessEqual"); break;
	case MiniXftOpMore: printf ("More"); break;
	case MiniXftOpMoreEqual: printf ("MoreEqual"); break;
	case MiniXftOpPlus: printf ("Plus"); break;
	case MiniXftOpMinus: printf ("Minus"); break;
	case MiniXftOpTimes: printf ("Times"); break;
	case MiniXftOpDivide: printf ("Divide"); break;
	default: break;
	}
	printf (" ");
	MiniXftExprPrint (expr->u.tree.right);
	break;
    case MiniXftOpNot:
	printf ("Not ");
	MiniXftExprPrint (expr->u.tree.left);
	break;
    default:
	break;
    }
}

void
MiniXftEditPrint (MiniXftEdit *edit)
{
    printf ("Edit %s ", edit->field);
    MiniXftOpPrint (edit->op);
    printf (" ");
    MiniXftExprPrint (edit->expr);
}

void
MiniXftSubstPrint (MiniXftSubst *subst)
{
    MiniXftEdit	*e;
    MiniXftTest	*t;
    
    printf ("match\n");
    for (t = subst->test; t; t = t->next)
    {
	printf ("\t");
	MiniXftTestPrint (t);
    }
    printf ("edit\n");
    for (e = subst->edit; e; e = e->next)
    {
	printf ("\t");
	MiniXftEditPrint (e);
	printf (";\n");
    }
    printf ("\n");
}

void
MiniXftFontSetPrint (MiniXftFontSet *s)
{
    int	    i;

    printf ("FontSet %d of %d\n", s->nfont, s->sfont);
    for (i = 0; i < s->nfont; i++)
    {
	printf ("Font %d ", i);
	MiniXftPatternPrint (s->fonts[i]);
    }
}
