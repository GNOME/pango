
/*  A Bison parser, made from minixftgram.y
    by GNU Bison version 1.28  */

#define YYBISON 1  /* Identify Bison output.  */

#define	INTEGER	257
#define	DOUBLE	258
#define	STRING	259
#define	NAME	260
#define	ANY	261
#define	ALL	262
#define	DIR	263
#define	CACHE	264
#define	INCLUDE	265
#define	INCLUDEIF	266
#define	MATCH	267
#define	EDIT	268
#define	TOK_TRUE	269
#define	TOK_FALSE	270
#define	TOK_NIL	271
#define	EQUAL	272
#define	SEMI	273
#define	OS	274
#define	CS	275
#define	QUEST	276
#define	COLON	277
#define	OROR	278
#define	ANDAND	279
#define	EQEQ	280
#define	NOTEQ	281
#define	LESS	282
#define	LESSEQ	283
#define	MORE	284
#define	MOREEQ	285
#define	PLUS	286
#define	MINUS	287
#define	TIMES	288
#define	DIVIDE	289
#define	NOT	290

#line 25 "minixftgram.y"


#include <stdlib.h>
#include <stdio.h>
#include "minixftint.h"

static MiniXftMatrix   matrix;
    

#line 35 "minixftgram.y"
typedef union {
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
} YYSTYPE;
#include <stdio.h>

#ifndef __cplusplus
#ifndef __STDC__
#define const
#endif
#endif



#define	YYFINAL		95
#define	YYFLAG		-32768
#define	YYNTBASE	37

#define YYTRANSLATE(x) ((unsigned)(x) <= 290 ? MiniXftConfigtranslate[x] : 51)

static const char MiniXftConfigtranslate[] = {     0,
     2,     2,     2,     2,     2,     2,     2,     2,     2,     2,
     2,     2,     2,     2,     2,     2,     2,     2,     2,     2,
     2,     2,     2,     2,     2,     2,     2,     2,     2,     2,
     2,     2,     2,     2,     2,     2,     2,     2,     2,     2,
     2,     2,     2,     2,     2,     2,     2,     2,     2,     2,
     2,     2,     2,     2,     2,     2,     2,     2,     2,     2,
     2,     2,     2,     2,     2,     2,     2,     2,     2,     2,
     2,     2,     2,     2,     2,     2,     2,     2,     2,     2,
     2,     2,     2,     2,     2,     2,     2,     2,     2,     2,
     2,     2,     2,     2,     2,     2,     2,     2,     2,     2,
     2,     2,     2,     2,     2,     2,     2,     2,     2,     2,
     2,     2,     2,     2,     2,     2,     2,     2,     2,     2,
     2,     2,     2,     2,     2,     2,     2,     2,     2,     2,
     2,     2,     2,     2,     2,     2,     2,     2,     2,     2,
     2,     2,     2,     2,     2,     2,     2,     2,     2,     2,
     2,     2,     2,     2,     2,     2,     2,     2,     2,     2,
     2,     2,     2,     2,     2,     2,     2,     2,     2,     2,
     2,     2,     2,     2,     2,     2,     2,     2,     2,     2,
     2,     2,     2,     2,     2,     2,     2,     2,     2,     2,
     2,     2,     2,     2,     2,     2,     2,     2,     2,     2,
     2,     2,     2,     2,     2,     2,     2,     2,     2,     2,
     2,     2,     2,     2,     2,     2,     2,     2,     2,     2,
     2,     2,     2,     2,     2,     2,     2,     2,     2,     2,
     2,     2,     2,     2,     2,     2,     2,     2,     2,     2,
     2,     2,     2,     2,     2,     2,     2,     2,     2,     2,
     2,     2,     2,     2,     2,     1,     3,     4,     5,     6,
     7,     8,     9,    10,    11,    12,    13,    14,    15,    16,
    17,    18,    19,    20,    21,    22,    23,    24,    25,    26,
    27,    28,    29,    30,    31,    32,    33,    34,    35,    36
};

#if YYDEBUG != 0
static const short MiniXftConfigprhs[] = {     0,
     0,     3,     4,     7,    10,    13,    16,    21,    24,    25,
    30,    32,    34,    35,    37,    39,    41,    43,    45,    47,
    49,    51,    53,    55,    57,    59,    61,    63,    65,    72,
    74,    76,    79,    80,    85,    87,    90,    93,    95,    97,
    99,   101,   103,   105,   107,   109,   113,   117,   121,   125,
   129,   133,   137,   141,   145,   149,   153,   157,   160
};

static const short MiniXftConfigrhs[] = {    37,
    38,     0,     0,     9,     5,     0,    10,     5,     0,    11,
     5,     0,    12,     5,     0,    13,    39,    14,    47,     0,
    40,    39,     0,     0,    41,    42,    43,    44,     0,     7,
     0,     8,     0,     0,     6,     0,    18,     0,    26,     0,
    27,     0,    28,     0,    29,     0,    30,     0,    31,     0,
     3,     0,     4,     0,     5,     0,    15,     0,    16,     0,
    17,     0,    45,     0,    20,    46,    46,    46,    46,    21,
     0,     3,     0,     4,     0,    48,    47,     0,     0,    42,
    49,    50,    19,     0,    18,     0,    32,    18,     0,    18,
    32,     0,     3,     0,     4,     0,     5,     0,    15,     0,
    16,     0,    17,     0,    45,     0,     6,     0,    50,    24,
    50,     0,    50,    25,    50,     0,    50,    26,    50,     0,
    50,    27,    50,     0,    50,    28,    50,     0,    50,    29,
    50,     0,    50,    30,    50,     0,    50,    31,    50,     0,
    50,    32,    50,     0,    50,    33,    50,     0,    50,    34,
    50,     0,    50,    35,    50,     0,    36,    50,     0,    50,
    22,    50,    23,    50,     0
};

#endif

#if YYDEBUG != 0
static const short MiniXftConfigrline[] = { 0,
    77,    78,    80,    82,    84,    86,    88,    91,    93,    96,
    99,   101,   103,   106,   111,   113,   115,   117,   119,   121,
   123,   126,   131,   136,   141,   146,   151,   155,   161,   168,
   170,   172,   174,   177,   180,   182,   184,   187,   189,   191,
   193,   195,   197,   199,   201,   203,   205,   207,   209,   211,
   213,   215,   217,   219,   221,   223,   225,   227,   229
};
#endif


#if YYDEBUG != 0 || defined (YYERROR_VERBOSE)

static const char * const MiniXftConfigtname[] = {   "$","error","$undefined.","INTEGER",
"DOUBLE","STRING","NAME","ANY","ALL","DIR","CACHE","INCLUDE","INCLUDEIF","MATCH",
"EDIT","TOK_TRUE","TOK_FALSE","TOK_NIL","EQUAL","SEMI","OS","CS","QUEST","COLON",
"OROR","ANDAND","EQEQ","NOTEQ","LESS","LESSEQ","MORE","MOREEQ","PLUS","MINUS",
"TIMES","DIVIDE","NOT","configs","config","tests","test","qual","field","compare",
"value","matrix","number","edits","edit","eqop","expr", NULL
};
#endif

static const short MiniXftConfigr1[] = {     0,
    37,    37,    38,    38,    38,    38,    38,    39,    39,    40,
    41,    41,    41,    42,    43,    43,    43,    43,    43,    43,
    43,    44,    44,    44,    44,    44,    44,    44,    45,    46,
    46,    47,    47,    48,    49,    49,    49,    50,    50,    50,
    50,    50,    50,    50,    50,    50,    50,    50,    50,    50,
    50,    50,    50,    50,    50,    50,    50,    50,    50
};

static const short MiniXftConfigr2[] = {     0,
     2,     0,     2,     2,     2,     2,     4,     2,     0,     4,
     1,     1,     0,     1,     1,     1,     1,     1,     1,     1,
     1,     1,     1,     1,     1,     1,     1,     1,     6,     1,
     1,     2,     0,     4,     1,     2,     2,     1,     1,     1,
     1,     1,     1,     1,     1,     3,     3,     3,     3,     3,
     3,     3,     3,     3,     3,     3,     3,     2,     5
};

static const short MiniXftConfigdefact[] = {     2,
     0,     0,     0,     0,     0,     9,     1,     3,     4,     5,
     6,    11,    12,     0,     9,     0,    33,     8,    14,     0,
     0,     7,    33,    15,    16,    17,    18,    19,    20,    21,
     0,    35,     0,     0,    32,    22,    23,    24,    25,    26,
    27,     0,    10,    28,    37,    36,    38,    39,    40,    45,
    41,    42,    43,     0,    44,     0,    30,    31,     0,    58,
    34,     0,     0,     0,     0,     0,     0,     0,     0,     0,
     0,     0,     0,     0,     0,     0,    46,    47,    48,    49,
    50,    51,    52,    53,    54,    55,    56,    57,     0,     0,
     0,    59,    29,     0,     0
};

static const short MiniXftConfigdefgoto[] = {     1,
     7,    14,    15,    16,    21,    31,    43,    55,    59,    22,
    23,    34,    56
};

static const short MiniXftConfigpact[] = {-32768,
    39,    32,    52,    94,    95,    -1,-32768,-32768,-32768,-32768,
-32768,-32768,-32768,    15,    -1,    79,    79,-32768,-32768,    80,
     8,-32768,    79,-32768,-32768,-32768,-32768,-32768,-32768,-32768,
    27,    69,    84,    18,-32768,-32768,-32768,-32768,-32768,-32768,
-32768,    24,-32768,-32768,-32768,-32768,-32768,-32768,-32768,-32768,
-32768,-32768,-32768,    18,-32768,    34,-32768,-32768,    24,-32768,
-32768,    18,    18,    18,    18,    18,    18,    18,    18,    18,
    18,    18,    18,    18,    24,    48,    87,    97,   105,   105,
   -31,   -31,   -31,   -31,    11,    11,-32768,-32768,    24,    18,
    82,    62,-32768,   104,-32768
};

static const short MiniXftConfigpgoto[] = {-32768,
-32768,    90,-32768,-32768,   125,-32768,-32768,   111,   -34,   120,
-32768,-32768,   -54
};


#define	YYLAST		143


static const short MiniXftConfigtable[] = {    60,
    71,    72,    73,    74,   -13,    12,    13,    76,    77,    78,
    79,    80,    81,    82,    83,    84,    85,    86,    87,    88,
    47,    48,    49,    50,    75,    32,    57,    58,    17,    36,
    37,    38,    51,    52,    53,    92,     8,    42,    94,    33,
    89,    39,    40,    41,    73,    74,    42,     2,     3,     4,
     5,     6,    61,    54,    91,    62,     9,    63,    64,    65,
    66,    67,    68,    69,    70,    71,    72,    73,    74,    62,
    90,    63,    64,    65,    66,    67,    68,    69,    70,    71,
    72,    73,    74,    62,    19,    63,    64,    65,    66,    67,
    68,    69,    70,    71,    72,    73,    74,    24,    10,    11,
    45,    46,    93,    95,    18,    25,    26,    27,    28,    29,
    30,    64,    65,    66,    67,    68,    69,    70,    71,    72,
    73,    74,    65,    66,    67,    68,    69,    70,    71,    72,
    73,    74,    67,    68,    69,    70,    71,    72,    73,    74,
    20,    44,    35
};

static const short MiniXftConfigcheck[] = {    54,
    32,    33,    34,    35,     6,     7,     8,    62,    63,    64,
    65,    66,    67,    68,    69,    70,    71,    72,    73,    74,
     3,     4,     5,     6,    59,    18,     3,     4,    14,     3,
     4,     5,    15,    16,    17,    90,     5,    20,     0,    32,
    75,    15,    16,    17,    34,    35,    20,     9,    10,    11,
    12,    13,    19,    36,    89,    22,     5,    24,    25,    26,
    27,    28,    29,    30,    31,    32,    33,    34,    35,    22,
    23,    24,    25,    26,    27,    28,    29,    30,    31,    32,
    33,    34,    35,    22,     6,    24,    25,    26,    27,    28,
    29,    30,    31,    32,    33,    34,    35,    18,     5,     5,
    32,    18,    21,     0,    15,    26,    27,    28,    29,    30,
    31,    25,    26,    27,    28,    29,    30,    31,    32,    33,
    34,    35,    26,    27,    28,    29,    30,    31,    32,    33,
    34,    35,    28,    29,    30,    31,    32,    33,    34,    35,
    16,    31,    23
};
/* -*-C-*-  Note some compilers choke on comments on `#line' lines.  */
#line 3 "/usr/lib/bison.simple"
/* This file comes from bison-1.28.  */

/* Skeleton output parser for bison,
   Copyright (C) 1984, 1989, 1990 Free Software Foundation, Inc.

   This program is free software; you can redistribute it and/or modify
   it under the terms of the GNU General Public License as published by
   the Free Software Foundation; either version 2, or (at your option)
   any later version.

   This program is distributed in the hope that it will be useful,
   but WITHOUT ANY WARRANTY; without even the implied warranty of
   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
   GNU General Public License for more details.

   You should have received a copy of the GNU General Public License
   along with this program; if not, write to the Free Software
   Foundation, Inc., 59 Temple Place - Suite 330,
   Boston, MA 02111-1307, USA.  */

/* As a special exception, when this file is copied by Bison into a
   Bison output file, you may use that output file without restriction.
   This special exception was added by the Free Software Foundation
   in version 1.24 of Bison.  */

/* This is the parser code that is written into each bison parser
  when the %semantic_parser declaration is not specified in the grammar.
  It was written by Richard Stallman by simplifying the hairy parser
  used when %semantic_parser is specified.  */

#ifndef YYSTACK_USE_ALLOCA
#ifdef alloca
#define YYSTACK_USE_ALLOCA
#else /* alloca not defined */
#ifdef __GNUC__
#define YYSTACK_USE_ALLOCA
#define alloca __builtin_alloca
#else /* not GNU C.  */
#if (!defined (__STDC__) && defined (sparc)) || defined (__sparc__) || defined (__sparc) || defined (__sgi) || (defined (__sun) && defined (__i386))
#define YYSTACK_USE_ALLOCA
#include <alloca.h>
#else /* not sparc */
/* We think this test detects Watcom and Microsoft C.  */
/* This used to test MSDOS, but that is a bad idea
   since that symbol is in the user namespace.  */
#if (defined (_MSDOS) || defined (_MSDOS_)) && !defined (__TURBOC__)
#if 0 /* No need for malloc.h, which pollutes the namespace;
	 instead, just don't use alloca.  */
#include <malloc.h>
#endif
#else /* not MSDOS, or __TURBOC__ */
#if defined(_AIX)
/* I don't know what this was needed for, but it pollutes the namespace.
   So I turned it off.   rms, 2 May 1997.  */
/* #include <malloc.h>  */
 #pragma alloca
#define YYSTACK_USE_ALLOCA
#else /* not MSDOS, or __TURBOC__, or _AIX */
#if 0
#ifdef __hpux /* haible@ilog.fr says this works for HPUX 9.05 and up,
		 and on HPUX 10.  Eventually we can turn this on.  */
#define YYSTACK_USE_ALLOCA
#define alloca __builtin_alloca
#endif /* __hpux */
#endif
#endif /* not _AIX */
#endif /* not MSDOS, or __TURBOC__ */
#endif /* not sparc */
#endif /* not GNU C */
#endif /* alloca not defined */
#endif /* YYSTACK_USE_ALLOCA not defined */

#ifdef YYSTACK_USE_ALLOCA
#define YYSTACK_ALLOC alloca
#else
#define YYSTACK_ALLOC malloc
#endif

/* Note: there must be only one dollar sign in this file.
   It is replaced by the list of actions, each action
   as one case of the switch.  */

#define MiniXftConfigerrok		(MiniXftConfigerrstatus = 0)
#define MiniXftConfigclearin	(MiniXftConfigchar = YYEMPTY)
#define YYEMPTY		-2
#define YYEOF		0
#define YYACCEPT	goto MiniXftConfigacceptlab
#define YYABORT 	goto MiniXftConfigabortlab
#define YYERROR		goto MiniXftConfigerrlab1
/* Like YYERROR except do call MiniXftConfigerror.
   This remains here temporarily to ease the
   transition to the new meaning of YYERROR, for GCC.
   Once GCC version 2 has supplanted version 1, this can go.  */
#define YYFAIL		goto MiniXftConfigerrlab
#define YYRECOVERING()  (!!MiniXftConfigerrstatus)
#define YYBACKUP(token, value) \
do								\
  if (MiniXftConfigchar == YYEMPTY && MiniXftConfiglen == 1)				\
    { MiniXftConfigchar = (token), MiniXftConfiglval = (value);			\
      MiniXftConfigchar1 = YYTRANSLATE (MiniXftConfigchar);				\
      YYPOPSTACK;						\
      goto MiniXftConfigbackup;						\
    }								\
  else								\
    { MiniXftConfigerror ("syntax error: cannot back up"); YYERROR; }	\
while (0)

#define YYTERROR	1
#define YYERRCODE	256

#ifndef YYPURE
#define YYLEX		MiniXftConfiglex()
#endif

#ifdef YYPURE
#ifdef YYLSP_NEEDED
#ifdef YYLEX_PARAM
#define YYLEX		MiniXftConfiglex(&MiniXftConfiglval, &MiniXftConfiglloc, YYLEX_PARAM)
#else
#define YYLEX		MiniXftConfiglex(&MiniXftConfiglval, &MiniXftConfiglloc)
#endif
#else /* not YYLSP_NEEDED */
#ifdef YYLEX_PARAM
#define YYLEX		MiniXftConfiglex(&MiniXftConfiglval, YYLEX_PARAM)
#else
#define YYLEX		MiniXftConfiglex(&MiniXftConfiglval)
#endif
#endif /* not YYLSP_NEEDED */
#endif

/* If nonreentrant, generate the variables here */

#ifndef YYPURE

int	MiniXftConfigchar;			/*  the lookahead symbol		*/
YYSTYPE	MiniXftConfiglval;			/*  the semantic value of the		*/
				/*  lookahead symbol			*/

#ifdef YYLSP_NEEDED
YYLTYPE MiniXftConfiglloc;			/*  location data for the lookahead	*/
				/*  symbol				*/
#endif

int MiniXftConfignerrs;			/*  number of parse errors so far       */
#endif  /* not YYPURE */

#if YYDEBUG != 0
int MiniXftConfigdebug;			/*  nonzero means print parse trace	*/
/* Since this is uninitialized, it does not stop multiple parsers
   from coexisting.  */
#endif

/*  YYINITDEPTH indicates the initial size of the parser's stacks	*/

#ifndef	YYINITDEPTH
#define YYINITDEPTH 200
#endif

/*  YYMAXDEPTH is the maximum size the stacks can grow to
    (effective only if the built-in stack extension method is used).  */

#if YYMAXDEPTH == 0
#undef YYMAXDEPTH
#endif

#ifndef YYMAXDEPTH
#define YYMAXDEPTH 10000
#endif

/* Define __MiniXftConfig_memcpy.  Note that the size argument
   should be passed with type unsigned int, because that is what the non-GCC
   definitions require.  With GCC, __builtin_memcpy takes an arg
   of type size_t, but it can handle unsigned int.  */

#if __GNUC__ > 1		/* GNU C and GNU C++ define this.  */
#define __MiniXftConfig_memcpy(TO,FROM,COUNT)	__builtin_memcpy(TO,FROM,COUNT)
#else				/* not GNU C or C++ */
#ifndef __cplusplus

/* This is the most reliable way to avoid incompatibilities
   in available built-in functions on various systems.  */
static void
__MiniXftConfig_memcpy (to, from, count)
     char *to;
     char *from;
     unsigned int count;
{
  register char *f = from;
  register char *t = to;
  register int i = count;

  while (i-- > 0)
    *t++ = *f++;
}

#else /* __cplusplus */

/* This is the most reliable way to avoid incompatibilities
   in available built-in functions on various systems.  */
static void
__MiniXftConfig_memcpy (char *to, char *from, unsigned int count)
{
  register char *t = to;
  register char *f = from;
  register int i = count;

  while (i-- > 0)
    *t++ = *f++;
}

#endif
#endif

#line 217 "/usr/lib/bison.simple"

/* The user can define YYPARSE_PARAM as the name of an argument to be passed
   into MiniXftConfigparse.  The argument should have type void *.
   It should actually point to an object.
   Grammar actions can access the variable by casting it
   to the proper pointer type.  */

#ifdef YYPARSE_PARAM
#ifdef __cplusplus
#define YYPARSE_PARAM_ARG void *YYPARSE_PARAM
#define YYPARSE_PARAM_DECL
#else /* not __cplusplus */
#define YYPARSE_PARAM_ARG YYPARSE_PARAM
#define YYPARSE_PARAM_DECL void *YYPARSE_PARAM;
#endif /* not __cplusplus */
#else /* not YYPARSE_PARAM */
#define YYPARSE_PARAM_ARG
#define YYPARSE_PARAM_DECL
#endif /* not YYPARSE_PARAM */

/* Prevent warning if -Wstrict-prototypes.  */
#ifdef __GNUC__
#ifdef YYPARSE_PARAM
int MiniXftConfigparse (void *);
#else
int MiniXftConfigparse (void);
#endif
#endif

int
MiniXftConfigparse(YYPARSE_PARAM_ARG)
     YYPARSE_PARAM_DECL
{
  register int MiniXftConfigstate;
  register int MiniXftConfign;
  register short *MiniXftConfigssp;
  register YYSTYPE *MiniXftConfigvsp;
  int MiniXftConfigerrstatus;	/*  number of tokens to shift before error messages enabled */
  int MiniXftConfigchar1 = 0;		/*  lookahead token as an internal (translated) token number */

  short	MiniXftConfigssa[YYINITDEPTH];	/*  the state stack			*/
  YYSTYPE MiniXftConfigvsa[YYINITDEPTH];	/*  the semantic value stack		*/

  short *MiniXftConfigss = MiniXftConfigssa;		/*  refer to the stacks thru separate pointers */
  YYSTYPE *MiniXftConfigvs = MiniXftConfigvsa;	/*  to allow MiniXftConfigoverflow to reallocate them elsewhere */

#ifdef YYLSP_NEEDED
  YYLTYPE MiniXftConfiglsa[YYINITDEPTH];	/*  the location stack			*/
  YYLTYPE *MiniXftConfigls = MiniXftConfiglsa;
  YYLTYPE *MiniXftConfiglsp;

#define YYPOPSTACK   (MiniXftConfigvsp--, MiniXftConfigssp--, MiniXftConfiglsp--)
#else
#define YYPOPSTACK   (MiniXftConfigvsp--, MiniXftConfigssp--)
#endif

  int MiniXftConfigstacksize = YYINITDEPTH;
  int MiniXftConfigfree_stacks = 0;

#ifdef YYPURE
  int MiniXftConfigchar;
  YYSTYPE MiniXftConfiglval;
  int MiniXftConfignerrs;
#ifdef YYLSP_NEEDED
  YYLTYPE MiniXftConfiglloc;
#endif
#endif

  YYSTYPE MiniXftConfigval;		/*  the variable used to return		*/
				/*  semantic values from the action	*/
				/*  routines				*/

  int MiniXftConfiglen;

#if YYDEBUG != 0
  if (MiniXftConfigdebug)
    fprintf(stderr, "Starting parse\n");
#endif

  MiniXftConfigstate = 0;
  MiniXftConfigerrstatus = 0;
  MiniXftConfignerrs = 0;
  MiniXftConfigchar = YYEMPTY;		/* Cause a token to be read.  */

  /* Initialize stack pointers.
     Waste one element of value and location stack
     so that they stay on the same level as the state stack.
     The wasted elements are never initialized.  */

  MiniXftConfigssp = MiniXftConfigss - 1;
  MiniXftConfigvsp = MiniXftConfigvs;
#ifdef YYLSP_NEEDED
  MiniXftConfiglsp = MiniXftConfigls;
#endif

/* Push a new state, which is found in  MiniXftConfigstate  .  */
/* In all cases, when you get here, the value and location stacks
   have just been pushed. so pushing a state here evens the stacks.  */
MiniXftConfignewstate:

  *++MiniXftConfigssp = MiniXftConfigstate;

  if (MiniXftConfigssp >= MiniXftConfigss + MiniXftConfigstacksize - 1)
    {
      /* Give user a chance to reallocate the stack */
      /* Use copies of these so that the &'s don't force the real ones into memory. */
      YYSTYPE *MiniXftConfigvs1 = MiniXftConfigvs;
      short *MiniXftConfigss1 = MiniXftConfigss;
#ifdef YYLSP_NEEDED
      YYLTYPE *MiniXftConfigls1 = MiniXftConfigls;
#endif

      /* Get the current used size of the three stacks, in elements.  */
      int size = MiniXftConfigssp - MiniXftConfigss + 1;

#ifdef MiniXftConfigoverflow
      /* Each stack pointer address is followed by the size of
	 the data in use in that stack, in bytes.  */
#ifdef YYLSP_NEEDED
      /* This used to be a conditional around just the two extra args,
	 but that might be undefined if MiniXftConfigoverflow is a macro.  */
      MiniXftConfigoverflow("parser stack overflow",
		 &MiniXftConfigss1, size * sizeof (*MiniXftConfigssp),
		 &MiniXftConfigvs1, size * sizeof (*MiniXftConfigvsp),
		 &MiniXftConfigls1, size * sizeof (*MiniXftConfiglsp),
		 &MiniXftConfigstacksize);
#else
      MiniXftConfigoverflow("parser stack overflow",
		 &MiniXftConfigss1, size * sizeof (*MiniXftConfigssp),
		 &MiniXftConfigvs1, size * sizeof (*MiniXftConfigvsp),
		 &MiniXftConfigstacksize);
#endif

      MiniXftConfigss = MiniXftConfigss1; MiniXftConfigvs = MiniXftConfigvs1;
#ifdef YYLSP_NEEDED
      MiniXftConfigls = MiniXftConfigls1;
#endif
#else /* no MiniXftConfigoverflow */
      /* Extend the stack our own way.  */
      if (MiniXftConfigstacksize >= YYMAXDEPTH)
	{
	  MiniXftConfigerror("parser stack overflow");
	  if (MiniXftConfigfree_stacks)
	    {
	      free (MiniXftConfigss);
	      free (MiniXftConfigvs);
#ifdef YYLSP_NEEDED
	      free (MiniXftConfigls);
#endif
	    }
	  return 2;
	}
      MiniXftConfigstacksize *= 2;
      if (MiniXftConfigstacksize > YYMAXDEPTH)
	MiniXftConfigstacksize = YYMAXDEPTH;
#ifndef YYSTACK_USE_ALLOCA
      MiniXftConfigfree_stacks = 1;
#endif
      MiniXftConfigss = (short *) YYSTACK_ALLOC (MiniXftConfigstacksize * sizeof (*MiniXftConfigssp));
      __MiniXftConfig_memcpy ((char *)MiniXftConfigss, (char *)MiniXftConfigss1,
		   size * (unsigned int) sizeof (*MiniXftConfigssp));
      MiniXftConfigvs = (YYSTYPE *) YYSTACK_ALLOC (MiniXftConfigstacksize * sizeof (*MiniXftConfigvsp));
      __MiniXftConfig_memcpy ((char *)MiniXftConfigvs, (char *)MiniXftConfigvs1,
		   size * (unsigned int) sizeof (*MiniXftConfigvsp));
#ifdef YYLSP_NEEDED
      MiniXftConfigls = (YYLTYPE *) YYSTACK_ALLOC (MiniXftConfigstacksize * sizeof (*MiniXftConfiglsp));
      __MiniXftConfig_memcpy ((char *)MiniXftConfigls, (char *)MiniXftConfigls1,
		   size * (unsigned int) sizeof (*MiniXftConfiglsp));
#endif
#endif /* no MiniXftConfigoverflow */

      MiniXftConfigssp = MiniXftConfigss + size - 1;
      MiniXftConfigvsp = MiniXftConfigvs + size - 1;
#ifdef YYLSP_NEEDED
      MiniXftConfiglsp = MiniXftConfigls + size - 1;
#endif

#if YYDEBUG != 0
      if (MiniXftConfigdebug)
	fprintf(stderr, "Stack size increased to %d\n", MiniXftConfigstacksize);
#endif

      if (MiniXftConfigssp >= MiniXftConfigss + MiniXftConfigstacksize - 1)
	YYABORT;
    }

#if YYDEBUG != 0
  if (MiniXftConfigdebug)
    fprintf(stderr, "Entering state %d\n", MiniXftConfigstate);
#endif

  goto MiniXftConfigbackup;
 MiniXftConfigbackup:

/* Do appropriate processing given the current state.  */
/* Read a lookahead token if we need one and don't already have one.  */
/* MiniXftConfigresume: */

  /* First try to decide what to do without reference to lookahead token.  */

  MiniXftConfign = MiniXftConfigpact[MiniXftConfigstate];
  if (MiniXftConfign == YYFLAG)
    goto MiniXftConfigdefault;

  /* Not known => get a lookahead token if don't already have one.  */

  /* MiniXftConfigchar is either YYEMPTY or YYEOF
     or a valid token in external form.  */

  if (MiniXftConfigchar == YYEMPTY)
    {
#if YYDEBUG != 0
      if (MiniXftConfigdebug)
	fprintf(stderr, "Reading a token: ");
#endif
      MiniXftConfigchar = YYLEX;
    }

  /* Convert token to internal form (in MiniXftConfigchar1) for indexing tables with */

  if (MiniXftConfigchar <= 0)		/* This means end of input. */
    {
      MiniXftConfigchar1 = 0;
      MiniXftConfigchar = YYEOF;		/* Don't call YYLEX any more */

#if YYDEBUG != 0
      if (MiniXftConfigdebug)
	fprintf(stderr, "Now at end of input.\n");
#endif
    }
  else
    {
      MiniXftConfigchar1 = YYTRANSLATE(MiniXftConfigchar);

#if YYDEBUG != 0
      if (MiniXftConfigdebug)
	{
	  fprintf (stderr, "Next token is %d (%s", MiniXftConfigchar, MiniXftConfigtname[MiniXftConfigchar1]);
	  /* Give the individual parser a way to print the precise meaning
	     of a token, for further debugging info.  */
#ifdef YYPRINT
	  YYPRINT (stderr, MiniXftConfigchar, MiniXftConfiglval);
#endif
	  fprintf (stderr, ")\n");
	}
#endif
    }

  MiniXftConfign += MiniXftConfigchar1;
  if (MiniXftConfign < 0 || MiniXftConfign > YYLAST || MiniXftConfigcheck[MiniXftConfign] != MiniXftConfigchar1)
    goto MiniXftConfigdefault;

  MiniXftConfign = MiniXftConfigtable[MiniXftConfign];

  /* MiniXftConfign is what to do for this token type in this state.
     Negative => reduce, -MiniXftConfign is rule number.
     Positive => shift, MiniXftConfign is new state.
       New state is final state => don't bother to shift,
       just return success.
     0, or most negative number => error.  */

  if (MiniXftConfign < 0)
    {
      if (MiniXftConfign == YYFLAG)
	goto MiniXftConfigerrlab;
      MiniXftConfign = -MiniXftConfign;
      goto MiniXftConfigreduce;
    }
  else if (MiniXftConfign == 0)
    goto MiniXftConfigerrlab;

  if (MiniXftConfign == YYFINAL)
    YYACCEPT;

  /* Shift the lookahead token.  */

#if YYDEBUG != 0
  if (MiniXftConfigdebug)
    fprintf(stderr, "Shifting token %d (%s), ", MiniXftConfigchar, MiniXftConfigtname[MiniXftConfigchar1]);
#endif

  /* Discard the token being shifted unless it is eof.  */
  if (MiniXftConfigchar != YYEOF)
    MiniXftConfigchar = YYEMPTY;

  *++MiniXftConfigvsp = MiniXftConfiglval;
#ifdef YYLSP_NEEDED
  *++MiniXftConfiglsp = MiniXftConfiglloc;
#endif

  /* count tokens shifted since error; after three, turn off error status.  */
  if (MiniXftConfigerrstatus) MiniXftConfigerrstatus--;

  MiniXftConfigstate = MiniXftConfign;
  goto MiniXftConfignewstate;

/* Do the default action for the current state.  */
MiniXftConfigdefault:

  MiniXftConfign = MiniXftConfigdefact[MiniXftConfigstate];
  if (MiniXftConfign == 0)
    goto MiniXftConfigerrlab;

/* Do a reduction.  MiniXftConfign is the number of a rule to reduce with.  */
MiniXftConfigreduce:
  MiniXftConfiglen = MiniXftConfigr2[MiniXftConfign];
  if (MiniXftConfiglen > 0)
    MiniXftConfigval = MiniXftConfigvsp[1-MiniXftConfiglen]; /* implement default value of the action */

#if YYDEBUG != 0
  if (MiniXftConfigdebug)
    {
      int i;

      fprintf (stderr, "Reducing via rule %d (line %d), ",
	       MiniXftConfign, MiniXftConfigrline[MiniXftConfign]);

      /* Print the symbols being reduced, and their result.  */
      for (i = MiniXftConfigprhs[MiniXftConfign]; MiniXftConfigrhs[i] > 0; i++)
	fprintf (stderr, "%s ", MiniXftConfigtname[MiniXftConfigrhs[i]]);
      fprintf (stderr, " -> %s\n", MiniXftConfigtname[MiniXftConfigr1[MiniXftConfign]]);
    }
#endif


  switch (MiniXftConfign) {

case 3:
#line 81 "minixftgram.y"
{ MiniXftConfigAddDir (MiniXftConfigvsp[0].sval); ;
    break;}
case 4:
#line 83 "minixftgram.y"
{ MiniXftConfigSetCache (MiniXftConfigvsp[0].sval); ;
    break;}
case 5:
#line 85 "minixftgram.y"
{ MiniXftConfigPushInput (MiniXftConfigvsp[0].sval, True); ;
    break;}
case 6:
#line 87 "minixftgram.y"
{ MiniXftConfigPushInput (MiniXftConfigvsp[0].sval, False); ;
    break;}
case 7:
#line 89 "minixftgram.y"
{ MiniXftConfigAddEdit (MiniXftConfigvsp[-2].tval, MiniXftConfigvsp[0].Eval); ;
    break;}
case 8:
#line 92 "minixftgram.y"
{ MiniXftConfigvsp[-1].tval->next = MiniXftConfigvsp[0].tval; MiniXftConfigval.tval = MiniXftConfigvsp[-1].tval; ;
    break;}
case 9:
#line 94 "minixftgram.y"
{ MiniXftConfigval.tval = 0; ;
    break;}
case 10:
#line 97 "minixftgram.y"
{ MiniXftConfigval.tval = MiniXftTestCreate (MiniXftConfigvsp[-3].qval, MiniXftConfigvsp[-2].sval, MiniXftConfigvsp[-1].oval, MiniXftConfigvsp[0].vval); ;
    break;}
case 11:
#line 100 "minixftgram.y"
{ MiniXftConfigval.qval = MiniXftQualAny; ;
    break;}
case 12:
#line 102 "minixftgram.y"
{ MiniXftConfigval.qval = MiniXftQualAll; ;
    break;}
case 13:
#line 104 "minixftgram.y"
{ MiniXftConfigval.qval = MiniXftQualAny; ;
    break;}
case 14:
#line 107 "minixftgram.y"
{ 
		    MiniXftConfigval.sval = MiniXftConfigSaveField (MiniXftConfigvsp[0].sval); 
		;
    break;}
case 15:
#line 112 "minixftgram.y"
{ MiniXftConfigval.oval = MiniXftOpEqual; ;
    break;}
case 16:
#line 114 "minixftgram.y"
{ MiniXftConfigval.oval = MiniXftOpEqual; ;
    break;}
case 17:
#line 116 "minixftgram.y"
{ MiniXftConfigval.oval = MiniXftOpNotEqual; ;
    break;}
case 18:
#line 118 "minixftgram.y"
{ MiniXftConfigval.oval = MiniXftOpLess; ;
    break;}
case 19:
#line 120 "minixftgram.y"
{ MiniXftConfigval.oval = MiniXftOpLessEqual; ;
    break;}
case 20:
#line 122 "minixftgram.y"
{ MiniXftConfigval.oval = MiniXftOpMore; ;
    break;}
case 21:
#line 124 "minixftgram.y"
{ MiniXftConfigval.oval = MiniXftOpMoreEqual; ;
    break;}
case 22:
#line 127 "minixftgram.y"
{
		    MiniXftConfigval.vval.type = MiniXftTypeInteger;
		    MiniXftConfigval.vval.u.i = MiniXftConfigvsp[0].ival;
		;
    break;}
case 23:
#line 132 "minixftgram.y"
{
		    MiniXftConfigval.vval.type = MiniXftTypeDouble;
		    MiniXftConfigval.vval.u.d = MiniXftConfigvsp[0].dval;
		;
    break;}
case 24:
#line 137 "minixftgram.y"
{
		    MiniXftConfigval.vval.type = MiniXftTypeString;
		    MiniXftConfigval.vval.u.s = MiniXftConfigvsp[0].sval;
		;
    break;}
case 25:
#line 142 "minixftgram.y"
{
		    MiniXftConfigval.vval.type = MiniXftTypeBool;
		    MiniXftConfigval.vval.u.b = True;
		;
    break;}
case 26:
#line 147 "minixftgram.y"
{
		    MiniXftConfigval.vval.type = MiniXftTypeBool;
		    MiniXftConfigval.vval.u.b = False;
		;
    break;}
case 27:
#line 152 "minixftgram.y"
{
		    MiniXftConfigval.vval.type = MiniXftTypeVoid;
		;
    break;}
case 28:
#line 156 "minixftgram.y"
{
		    MiniXftConfigval.vval.type = MiniXftTypeMatrix;
		    MiniXftConfigval.vval.u.m = &matrix;
		;
    break;}
case 29:
#line 162 "minixftgram.y"
{
		    matrix.xx = MiniXftConfigvsp[-4].dval;
		    matrix.xy = MiniXftConfigvsp[-3].dval;
		    matrix.yx = MiniXftConfigvsp[-2].dval;
		    matrix.yy = MiniXftConfigvsp[-1].dval;
		;
    break;}
case 30:
#line 169 "minixftgram.y"
{ MiniXftConfigval.dval = (double) MiniXftConfigvsp[0].ival; ;
    break;}
case 32:
#line 173 "minixftgram.y"
{ MiniXftConfigvsp[-1].Eval->next = MiniXftConfigvsp[0].Eval; MiniXftConfigval.Eval = MiniXftConfigvsp[-1].Eval; ;
    break;}
case 33:
#line 175 "minixftgram.y"
{ MiniXftConfigval.Eval = 0; ;
    break;}
case 34:
#line 178 "minixftgram.y"
{ MiniXftConfigval.Eval = MiniXftEditCreate (MiniXftConfigvsp[-3].sval, MiniXftConfigvsp[-2].oval, MiniXftConfigvsp[-1].eval); ;
    break;}
case 35:
#line 181 "minixftgram.y"
{ MiniXftConfigval.oval = MiniXftOpAssign; ;
    break;}
case 36:
#line 183 "minixftgram.y"
{ MiniXftConfigval.oval = MiniXftOpPrepend; ;
    break;}
case 37:
#line 185 "minixftgram.y"
{ MiniXftConfigval.oval = MiniXftOpAppend; ;
    break;}
case 38:
#line 188 "minixftgram.y"
{ MiniXftConfigval.eval = MiniXftExprCreateInteger (MiniXftConfigvsp[0].ival); ;
    break;}
case 39:
#line 190 "minixftgram.y"
{ MiniXftConfigval.eval = MiniXftExprCreateDouble (MiniXftConfigvsp[0].dval); ;
    break;}
case 40:
#line 192 "minixftgram.y"
{ MiniXftConfigval.eval = MiniXftExprCreateString (MiniXftConfigvsp[0].sval); ;
    break;}
case 41:
#line 194 "minixftgram.y"
{ MiniXftConfigval.eval = MiniXftExprCreateBool (True); ;
    break;}
case 42:
#line 196 "minixftgram.y"
{ MiniXftConfigval.eval = MiniXftExprCreateBool (False); ;
    break;}
case 43:
#line 198 "minixftgram.y"
{ MiniXftConfigval.eval = MiniXftExprCreateNil (); ;
    break;}
case 44:
#line 200 "minixftgram.y"
{ MiniXftConfigval.eval = MiniXftExprCreateMatrix (&matrix); ;
    break;}
case 45:
#line 202 "minixftgram.y"
{ MiniXftConfigval.eval = MiniXftExprCreateField (MiniXftConfigvsp[0].sval); ;
    break;}
case 46:
#line 204 "minixftgram.y"
{ MiniXftConfigval.eval = MiniXftExprCreateOp (MiniXftConfigvsp[-2].eval, MiniXftOpOr, MiniXftConfigvsp[0].eval); ;
    break;}
case 47:
#line 206 "minixftgram.y"
{ MiniXftConfigval.eval = MiniXftExprCreateOp (MiniXftConfigvsp[-2].eval, MiniXftOpAnd, MiniXftConfigvsp[0].eval); ;
    break;}
case 48:
#line 208 "minixftgram.y"
{ MiniXftConfigval.eval = MiniXftExprCreateOp (MiniXftConfigvsp[-2].eval, MiniXftOpEqual, MiniXftConfigvsp[0].eval); ;
    break;}
case 49:
#line 210 "minixftgram.y"
{ MiniXftConfigval.eval = MiniXftExprCreateOp (MiniXftConfigvsp[-2].eval, MiniXftOpNotEqual, MiniXftConfigvsp[0].eval); ;
    break;}
case 50:
#line 212 "minixftgram.y"
{ MiniXftConfigval.eval = MiniXftExprCreateOp (MiniXftConfigvsp[-2].eval, MiniXftOpLess, MiniXftConfigvsp[0].eval); ;
    break;}
case 51:
#line 214 "minixftgram.y"
{ MiniXftConfigval.eval = MiniXftExprCreateOp (MiniXftConfigvsp[-2].eval, MiniXftOpLessEqual, MiniXftConfigvsp[0].eval); ;
    break;}
case 52:
#line 216 "minixftgram.y"
{ MiniXftConfigval.eval = MiniXftExprCreateOp (MiniXftConfigvsp[-2].eval, MiniXftOpMore, MiniXftConfigvsp[0].eval); ;
    break;}
case 53:
#line 218 "minixftgram.y"
{ MiniXftConfigval.eval = MiniXftExprCreateOp (MiniXftConfigvsp[-2].eval, MiniXftOpMoreEqual, MiniXftConfigvsp[0].eval); ;
    break;}
case 54:
#line 220 "minixftgram.y"
{ MiniXftConfigval.eval = MiniXftExprCreateOp (MiniXftConfigvsp[-2].eval, MiniXftOpPlus, MiniXftConfigvsp[0].eval); ;
    break;}
case 55:
#line 222 "minixftgram.y"
{ MiniXftConfigval.eval = MiniXftExprCreateOp (MiniXftConfigvsp[-2].eval, MiniXftOpMinus, MiniXftConfigvsp[0].eval); ;
    break;}
case 56:
#line 224 "minixftgram.y"
{ MiniXftConfigval.eval = MiniXftExprCreateOp (MiniXftConfigvsp[-2].eval, MiniXftOpTimes, MiniXftConfigvsp[0].eval); ;
    break;}
case 57:
#line 226 "minixftgram.y"
{ MiniXftConfigval.eval = MiniXftExprCreateOp (MiniXftConfigvsp[-2].eval, MiniXftOpDivide, MiniXftConfigvsp[0].eval); ;
    break;}
case 58:
#line 228 "minixftgram.y"
{ MiniXftConfigval.eval = MiniXftExprCreateOp (MiniXftConfigvsp[0].eval, MiniXftOpNot, (MiniXftExpr *) 0); ;
    break;}
case 59:
#line 230 "minixftgram.y"
{ MiniXftConfigval.eval = MiniXftExprCreateOp (MiniXftConfigvsp[-4].eval, MiniXftOpQuest, MiniXftExprCreateOp (MiniXftConfigvsp[-2].eval, MiniXftOpQuest, MiniXftConfigvsp[0].eval)); ;
    break;}
}
   /* the action file gets copied in in place of this dollarsign */
#line 543 "/usr/lib/bison.simple"

  MiniXftConfigvsp -= MiniXftConfiglen;
  MiniXftConfigssp -= MiniXftConfiglen;
#ifdef YYLSP_NEEDED
  MiniXftConfiglsp -= MiniXftConfiglen;
#endif

#if YYDEBUG != 0
  if (MiniXftConfigdebug)
    {
      short *ssp1 = MiniXftConfigss - 1;
      fprintf (stderr, "state stack now");
      while (ssp1 != MiniXftConfigssp)
	fprintf (stderr, " %d", *++ssp1);
      fprintf (stderr, "\n");
    }
#endif

  *++MiniXftConfigvsp = MiniXftConfigval;

#ifdef YYLSP_NEEDED
  MiniXftConfiglsp++;
  if (MiniXftConfiglen == 0)
    {
      MiniXftConfiglsp->first_line = MiniXftConfiglloc.first_line;
      MiniXftConfiglsp->first_column = MiniXftConfiglloc.first_column;
      MiniXftConfiglsp->last_line = (MiniXftConfiglsp-1)->last_line;
      MiniXftConfiglsp->last_column = (MiniXftConfiglsp-1)->last_column;
      MiniXftConfiglsp->text = 0;
    }
  else
    {
      MiniXftConfiglsp->last_line = (MiniXftConfiglsp+MiniXftConfiglen-1)->last_line;
      MiniXftConfiglsp->last_column = (MiniXftConfiglsp+MiniXftConfiglen-1)->last_column;
    }
#endif

  /* Now "shift" the result of the reduction.
     Determine what state that goes to,
     based on the state we popped back to
     and the rule number reduced by.  */

  MiniXftConfign = MiniXftConfigr1[MiniXftConfign];

  MiniXftConfigstate = MiniXftConfigpgoto[MiniXftConfign - YYNTBASE] + *MiniXftConfigssp;
  if (MiniXftConfigstate >= 0 && MiniXftConfigstate <= YYLAST && MiniXftConfigcheck[MiniXftConfigstate] == *MiniXftConfigssp)
    MiniXftConfigstate = MiniXftConfigtable[MiniXftConfigstate];
  else
    MiniXftConfigstate = MiniXftConfigdefgoto[MiniXftConfign - YYNTBASE];

  goto MiniXftConfignewstate;

MiniXftConfigerrlab:   /* here on detecting error */

  if (! MiniXftConfigerrstatus)
    /* If not already recovering from an error, report this error.  */
    {
      ++MiniXftConfignerrs;

#ifdef YYERROR_VERBOSE
      MiniXftConfign = MiniXftConfigpact[MiniXftConfigstate];

      if (MiniXftConfign > YYFLAG && MiniXftConfign < YYLAST)
	{
	  int size = 0;
	  char *msg;
	  int x, count;

	  count = 0;
	  /* Start X at -MiniXftConfign if nec to avoid negative indexes in MiniXftConfigcheck.  */
	  for (x = (MiniXftConfign < 0 ? -MiniXftConfign : 0);
	       x < (sizeof(MiniXftConfigtname) / sizeof(char *)); x++)
	    if (MiniXftConfigcheck[x + MiniXftConfign] == x)
	      size += strlen(MiniXftConfigtname[x]) + 15, count++;
	  msg = (char *) malloc(size + 15);
	  if (msg != 0)
	    {
	      strcpy(msg, "parse error");

	      if (count < 5)
		{
		  count = 0;
		  for (x = (MiniXftConfign < 0 ? -MiniXftConfign : 0);
		       x < (sizeof(MiniXftConfigtname) / sizeof(char *)); x++)
		    if (MiniXftConfigcheck[x + MiniXftConfign] == x)
		      {
			strcat(msg, count == 0 ? ", expecting `" : " or `");
			strcat(msg, MiniXftConfigtname[x]);
			strcat(msg, "'");
			count++;
		      }
		}
	      MiniXftConfigerror(msg);
	      free(msg);
	    }
	  else
	    MiniXftConfigerror ("parse error; also virtual memory exceeded");
	}
      else
#endif /* YYERROR_VERBOSE */
	MiniXftConfigerror("parse error");
    }

  goto MiniXftConfigerrlab1;
MiniXftConfigerrlab1:   /* here on error raised explicitly by an action */

  if (MiniXftConfigerrstatus == 3)
    {
      /* if just tried and failed to reuse lookahead token after an error, discard it.  */

      /* return failure if at end of input */
      if (MiniXftConfigchar == YYEOF)
	YYABORT;

#if YYDEBUG != 0
      if (MiniXftConfigdebug)
	fprintf(stderr, "Discarding token %d (%s).\n", MiniXftConfigchar, MiniXftConfigtname[MiniXftConfigchar1]);
#endif

      MiniXftConfigchar = YYEMPTY;
    }

  /* Else will try to reuse lookahead token
     after shifting the error token.  */

  MiniXftConfigerrstatus = 3;		/* Each real token shifted decrements this */

  goto MiniXftConfigerrhandle;

MiniXftConfigerrdefault:  /* current state does not do anything special for the error token. */

#if 0
  /* This is wrong; only states that explicitly want error tokens
     should shift them.  */
  MiniXftConfign = MiniXftConfigdefact[MiniXftConfigstate];  /* If its default is to accept any token, ok.  Otherwise pop it.*/
  if (MiniXftConfign) goto MiniXftConfigdefault;
#endif

MiniXftConfigerrpop:   /* pop the current state because it cannot handle the error token */

  if (MiniXftConfigssp == MiniXftConfigss) YYABORT;
  MiniXftConfigvsp--;
  MiniXftConfigstate = *--MiniXftConfigssp;
#ifdef YYLSP_NEEDED
  MiniXftConfiglsp--;
#endif

#if YYDEBUG != 0
  if (MiniXftConfigdebug)
    {
      short *ssp1 = MiniXftConfigss - 1;
      fprintf (stderr, "Error: state stack now");
      while (ssp1 != MiniXftConfigssp)
	fprintf (stderr, " %d", *++ssp1);
      fprintf (stderr, "\n");
    }
#endif

MiniXftConfigerrhandle:

  MiniXftConfign = MiniXftConfigpact[MiniXftConfigstate];
  if (MiniXftConfign == YYFLAG)
    goto MiniXftConfigerrdefault;

  MiniXftConfign += YYTERROR;
  if (MiniXftConfign < 0 || MiniXftConfign > YYLAST || MiniXftConfigcheck[MiniXftConfign] != YYTERROR)
    goto MiniXftConfigerrdefault;

  MiniXftConfign = MiniXftConfigtable[MiniXftConfign];
  if (MiniXftConfign < 0)
    {
      if (MiniXftConfign == YYFLAG)
	goto MiniXftConfigerrpop;
      MiniXftConfign = -MiniXftConfign;
      goto MiniXftConfigreduce;
    }
  else if (MiniXftConfign == 0)
    goto MiniXftConfigerrpop;

  if (MiniXftConfign == YYFINAL)
    YYACCEPT;

#if YYDEBUG != 0
  if (MiniXftConfigdebug)
    fprintf(stderr, "Shifting error token, ");
#endif

  *++MiniXftConfigvsp = MiniXftConfiglval;
#ifdef YYLSP_NEEDED
  *++MiniXftConfiglsp = MiniXftConfiglloc;
#endif

  MiniXftConfigstate = MiniXftConfign;
  goto MiniXftConfignewstate;

 MiniXftConfigacceptlab:
  /* YYACCEPT comes here.  */
  if (MiniXftConfigfree_stacks)
    {
      free (MiniXftConfigss);
      free (MiniXftConfigvs);
#ifdef YYLSP_NEEDED
      free (MiniXftConfigls);
#endif
    }
  return 0;

 MiniXftConfigabortlab:
  /* YYABORT comes here.  */
  if (MiniXftConfigfree_stacks)
    {
      free (MiniXftConfigss);
      free (MiniXftConfigvs);
#ifdef YYLSP_NEEDED
      free (MiniXftConfigls);
#endif
    }
  return 1;
}
#line 232 "minixftgram.y"


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
