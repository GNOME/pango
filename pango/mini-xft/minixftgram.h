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


extern YYSTYPE MiniXftConfiglval;
