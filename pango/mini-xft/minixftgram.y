/*
 * $XFree86: xc/lib/MiniXft/xftgram.y,v 1.5 2001/05/16 10:32:54 keithp Exp $
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

%{

#include <stdlib.h>
#include <stdio.h>
#include "minixftint.h"

static MiniXftMatrix   matrix;
    
%}

%union {
    int		ival;
    double	dval;
    char	*sval;
    MiniXftExpr	*eval;
    MiniXftPattern	*pval;
    MiniXftValue	vval;
    MiniXftEdit	*Eval;
    MiniXftOp	oval;
    MiniXftQual	qval;
    MiniXftTest	*tval;
}

%token <ival>	INTEGER
%token <dval>	DOUBLE
%token <sval>	STRING NAME
%token <ival>	ANY ALL
%token <ival>	DIR CACHE INCLUDE INCLUDEIF MATCH EDIT
%token <ival>	TOK_TRUE TOK_FALSE TOK_NIL
%token <ival>	EQUAL SEMI OS CS

%type  <eval>	expr
%type  <vval>	value
%type  <sval>	field
%type  <Eval>	edit
%type  <Eval>	edits
%type  <oval>	eqop
%type  <qval>	qual
%type  <oval>	compare
%type  <tval>	tests test
%type  <dval>	number

%right <ival>	QUEST COLON
%left <ival>	OROR
%left <ival>	ANDAND
%left <ival>	EQEQ NOTEQ
%left <ival>	LESS LESSEQ MORE MOREEQ
%left <ival>	PLUS MINUS
%left <ival>	TIMES DIVIDE
%right <ival>	NOT

%%
configs	:   configs config
	|
	;
config	:   DIR STRING
		{ MiniXftConfigAddDir ($2); }
	|   CACHE STRING
		{ MiniXftConfigSetCache ($2); }
	|   INCLUDE STRING
		{ MiniXftConfigPushInput ($2, True); }
	|   INCLUDEIF STRING
		{ MiniXftConfigPushInput ($2, False); }
	|   MATCH tests EDIT edits
		{ MiniXftConfigAddEdit ($2, $4); }
	;
tests	:   test tests
		{ $1->next = $2; $$ = $1; }
	|
		{ $$ = 0; }
	;
test	:   qual field compare value
		{ $$ = MiniXftTestCreate ($1, $2, $3, $4); }
	;
qual	:   ANY
	    { $$ = MiniXftQualAny; }
	|   ALL
	    { $$ = MiniXftQualAll; }
	|
	    { $$ = MiniXftQualAny; }
	;
field	:   NAME
		{ 
		    $$ = MiniXftConfigSaveField ($1); 
		}
	;
compare	:   EQUAL
		{ $$ = MiniXftOpEqual; }
	|   EQEQ
		{ $$ = MiniXftOpEqual; }
	|   NOTEQ
		{ $$ = MiniXftOpNotEqual; }
	|   LESS
		{ $$ = MiniXftOpLess; }
	|   LESSEQ
		{ $$ = MiniXftOpLessEqual; }
	|   MORE
		{ $$ = MiniXftOpMore; }
	|   MOREEQ
		{ $$ = MiniXftOpMoreEqual; }
	;
value	:   INTEGER
		{
		    $$.type = MiniXftTypeInteger;
		    $$.u.i = $1;
		}
	|   DOUBLE		
		{
		    $$.type = MiniXftTypeDouble;
		    $$.u.d = $1;
		}
	|   STRING
		{
		    $$.type = MiniXftTypeString;
		    $$.u.s = $1;
		}
	|   TOK_TRUE
		{
		    $$.type = MiniXftTypeBool;
		    $$.u.b = True;
		}
	|   TOK_FALSE
		{
		    $$.type = MiniXftTypeBool;
		    $$.u.b = False;
		}
	|   TOK_NIL
		{
		    $$.type = MiniXftTypeVoid;
		}
	|   matrix
		{
		    $$.type = MiniXftTypeMatrix;
		    $$.u.m = &matrix;
		}
	;
matrix	:   OS number number number number CS
		{
		    matrix.xx = $2;
		    matrix.xy = $3;
		    matrix.yx = $4;
		    matrix.__REALLY_YY__ = $5;
		}
number	:   INTEGER
		{ $$ = (double) $1; }
	|   DOUBLE
	;
edits	:   edit edits
	    { $1->next = $2; $$ = $1; }
	|
	    { $$ = 0; }
	;
edit	:   field eqop expr SEMI
	    { $$ = MiniXftEditCreate ($1, $2, $3); }
	;
eqop	:   EQUAL
	    { $$ = MiniXftOpAssign; }
	|   PLUS EQUAL
	    { $$ = MiniXftOpPrepend; }
	|   EQUAL PLUS
	    { $$ = MiniXftOpAppend; }
	;
expr	:   INTEGER
	    { $$ = MiniXftExprCreateInteger ($1); }
	|   DOUBLE
	    { $$ = MiniXftExprCreateDouble ($1); }
	|   STRING
	    { $$ = MiniXftExprCreateString ($1); }
	|   TOK_TRUE
	    { $$ = MiniXftExprCreateBool (True); }
	|   TOK_FALSE
	    { $$ = MiniXftExprCreateBool (False); }
	|   TOK_NIL
	    { $$ = MiniXftExprCreateNil (); }
	|   matrix
	    { $$ = MiniXftExprCreateMatrix (&matrix); }
	|   NAME
	    { $$ = MiniXftExprCreateField ($1); }
	|   expr OROR expr
	    { $$ = MiniXftExprCreateOp ($1, MiniXftOpOr, $3); }
	|   expr ANDAND expr
	    { $$ = MiniXftExprCreateOp ($1, MiniXftOpAnd, $3); }
	|   expr EQEQ expr
	    { $$ = MiniXftExprCreateOp ($1, MiniXftOpEqual, $3); }
	|   expr NOTEQ expr
	    { $$ = MiniXftExprCreateOp ($1, MiniXftOpNotEqual, $3); }
	|   expr LESS expr
	    { $$ = MiniXftExprCreateOp ($1, MiniXftOpLess, $3); }
	|   expr LESSEQ expr
	    { $$ = MiniXftExprCreateOp ($1, MiniXftOpLessEqual, $3); }
	|   expr MORE expr
	    { $$ = MiniXftExprCreateOp ($1, MiniXftOpMore, $3); }
	|   expr MOREEQ expr
	    { $$ = MiniXftExprCreateOp ($1, MiniXftOpMoreEqual, $3); }
	|   expr PLUS expr
	    { $$ = MiniXftExprCreateOp ($1, MiniXftOpPlus, $3); }
	|   expr MINUS expr
	    { $$ = MiniXftExprCreateOp ($1, MiniXftOpMinus, $3); }
	|   expr TIMES expr
	    { $$ = MiniXftExprCreateOp ($1, MiniXftOpTimes, $3); }
	|   expr DIVIDE expr
	    { $$ = MiniXftExprCreateOp ($1, MiniXftOpDivide, $3); }
	|   NOT expr
	    { $$ = MiniXftExprCreateOp ($2, MiniXftOpNot, (MiniXftExpr *) 0); }
	|   expr QUEST expr COLON expr
	    { $$ = MiniXftExprCreateOp ($1, MiniXftOpQuest, MiniXftExprCreateOp ($3, MiniXftOpQuest, $5)); }
	;
%%

int
MiniXftConfigwrap (void)
{
    return 1;
}

void
MiniXftConfigerror (char *fmt, ...)
{
    va_list	args;

    fprintf (stderr, "\"%s\": line %d, ", MiniXftConfigFile, MiniXftConfigLineno);
    va_start (args, fmt);
    vfprintf (stderr, fmt, args);
    va_end (args);
    fprintf (stderr, "\n");
}

MiniXftTest *
MiniXftTestCreate (MiniXftQual qual, const char *field, MiniXftOp compare, MiniXftValue value)
{
    MiniXftTest	*test = (MiniXftTest *) malloc (sizeof (MiniXftTest));;

    if (test)
    {
	test->next = 0;
	test->qual = qual;
	test->field = field;	/* already saved in grammar */
	test->op = compare;
	if (value.type == MiniXftTypeString)
	    value.u.s = _MiniXftSaveString (value.u.s);
	else if (value.type == MiniXftTypeMatrix)
	    value.u.m = _MiniXftSaveMatrix (value.u.m);
	test->value = value;
    }
    return test;
}

MiniXftExpr *
MiniXftExprCreateInteger (int i)
{
    MiniXftExpr *e = (MiniXftExpr *) malloc (sizeof (MiniXftExpr));

    if (e)
    {
	e->op = MiniXftOpInteger;
	e->u.ival = i;
    }
    return e;
}

MiniXftExpr *
MiniXftExprCreateDouble (double d)
{
    MiniXftExpr *e = (MiniXftExpr *) malloc (sizeof (MiniXftExpr));

    if (e)
    {
	e->op = MiniXftOpDouble;
	e->u.dval = d;
    }
    return e;
}

MiniXftExpr *
MiniXftExprCreateString (const char *s)
{
    MiniXftExpr *e = (MiniXftExpr *) malloc (sizeof (MiniXftExpr));

    if (e)
    {
	e->op = MiniXftOpString;
	e->u.sval = _MiniXftSaveString (s);
    }
    return e;
}

MiniXftExpr *
MiniXftExprCreateMatrix (const MiniXftMatrix *m)
{
    MiniXftExpr *e = (MiniXftExpr *) malloc (sizeof (MiniXftExpr));

    if (e)
    {
	e->op = MiniXftOpMatrix;
	e->u.mval = _MiniXftSaveMatrix (m);
    }
    return e;
}

MiniXftExpr *
MiniXftExprCreateBool (Bool b)
{
    MiniXftExpr *e = (MiniXftExpr *) malloc (sizeof (MiniXftExpr));

    if (e)
    {
	e->op = MiniXftOpBool;
	e->u.bval = b;
    }
    return e;
}

MiniXftExpr *
MiniXftExprCreateNil (void)
{
    MiniXftExpr *e = (MiniXftExpr *) malloc (sizeof (MiniXftExpr));

    if (e)
    {
	e->op = MiniXftOpNil;
    }
    return e;
}

MiniXftExpr *
MiniXftExprCreateField (const char *field)
{
    MiniXftExpr *e = (MiniXftExpr *) malloc (sizeof (MiniXftExpr));

    if (e)
    {
	e->op = MiniXftOpField;
	e->u.field = _MiniXftSaveString (field);
    }
    return e;
}

MiniXftExpr *
MiniXftExprCreateOp (MiniXftExpr *left, MiniXftOp op, MiniXftExpr *right)
{
    MiniXftExpr *e = (MiniXftExpr *) malloc (sizeof (MiniXftExpr));

    if (e)
    {
	e->op = op;
	e->u.tree.left = left;
	e->u.tree.right = right;
    }
    return e;
}

void
MiniXftExprDestroy (MiniXftExpr *e)
{
    switch (e->op) {
    case MiniXftOpInteger:
	break;
    case MiniXftOpDouble:
	break;
    case MiniXftOpString:
	free (e->u.sval);
	break;
    case MiniXftOpMatrix:
	free (e->u.mval);
	break;
    case MiniXftOpBool:
	break;
    case MiniXftOpField:
	free (e->u.field);
	break;
    case MiniXftOpAssign:
    case MiniXftOpPrepend:
    case MiniXftOpAppend:
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
    case MiniXftOpQuest:
	MiniXftExprDestroy (e->u.tree.right);
	/* fall through */
    case MiniXftOpNot:
	MiniXftExprDestroy (e->u.tree.left);
	break;
    case MiniXftOpNil:
	break;
    }
    free (e);
}

MiniXftEdit *
MiniXftEditCreate (const char *field, MiniXftOp op, MiniXftExpr *expr)
{
    MiniXftEdit *e = (MiniXftEdit *) malloc (sizeof (MiniXftEdit));

    if (e)
    {
	e->next = 0;
	e->field = field;   /* already saved in grammar */
	e->op = op;
	e->expr = expr;
    }
    return e;
}

void
MiniXftEditDestroy (MiniXftEdit *e)
{
    if (e->next)
	MiniXftEditDestroy (e->next);
    free ((void *) e->field);
    if (e->expr)
	MiniXftExprDestroy (e->expr);
}

char *
MiniXftConfigSaveField (const char *field)
{
    return _MiniXftSaveString (field);
}
