/*
 * $XFree86: xc/lib/MiniXft/minixftint.h,v 1.26 2001/07/13 18:16:10 keithp Exp $
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

#ifndef _XFTINT_H_
#define _XFTINT_H_

#include "MiniXftFreetype.h"

typedef struct _MiniXftMatcher {
    char    *object;
    double  (*compare) (char *object, MiniXftValue value1, MiniXftValue value2);
} MiniXftMatcher;

typedef struct _MiniXftSymbolic {
    const char	*name;
    int		value;
} MiniXftSymbolic;

#define XFT_DRAW_N_SRC	    2

#define XFT_DRAW_SRC_TEXT   0
#define XFT_DRAW_SRC_RECT   1

typedef struct _MiniXftDisplayInfo {
    struct _MiniXftDisplayInfo  *next;
    Display		    *display;
	/*    XExtCodes		    *codes; */
    MiniXftPattern		    *defaults;
	/*    MiniXftFontSet		    *coreFonts; */
    Bool		    hasRender;
} MiniXftDisplayInfo;

extern MiniXftDisplayInfo	*_MiniXftDisplayInfo;
extern MiniXftFontSet	*_MiniXftGlobalFontSet;
extern char		**MiniXftConfigDirs;
extern MiniXftFontSet	*_MiniXftFontSet;

#define XFT_NMISSING	256

extern char *mini_xft_get_default_path (void);

#define XFT_DBG_OPEN	1
#define XFT_DBG_OPENV	2
#define XFT_DBG_RENDER	4
#define XFT_DBG_DRAW	8
#define XFT_DBG_REF	16
#define XFT_DBG_GLYPH	32
#define XFT_DBG_GLYPHV	64
#define XFT_DBG_CACHE	128
#define XFT_DBG_CACHEV	256
#define XFT_DBG_MATCH	512
#define XFT_DBG_MATCHV	1024

typedef enum _MiniXftOp {
    MiniXftOpInteger, MiniXftOpDouble, MiniXftOpString, MiniXftOpMatrix, MiniXftOpBool, MiniXftOpNil,
    MiniXftOpField,
    MiniXftOpAssign, MiniXftOpPrepend, MiniXftOpAppend,
    MiniXftOpQuest,
    MiniXftOpOr, MiniXftOpAnd, MiniXftOpEqual, MiniXftOpNotEqual,
    MiniXftOpLess, MiniXftOpLessEqual, MiniXftOpMore, MiniXftOpMoreEqual,
    MiniXftOpPlus, MiniXftOpMinus, MiniXftOpTimes, MiniXftOpDivide,
    MiniXftOpNot
} MiniXftOp;

typedef struct _MiniXftExpr {
    MiniXftOp   op;
    union {
	int	    ival;
	double	    dval;
	char	    *sval;
	MiniXftMatrix   *mval;
	Bool	    bval;
	char	    *field;
	struct {
	    struct _MiniXftExpr *left, *right;
	} tree;
    } u;
} MiniXftExpr;

typedef enum _MiniXftQual {
    MiniXftQualAny, MiniXftQualAll
} MiniXftQual;

typedef struct _MiniXftTest {
    struct _MiniXftTest	*next;
    MiniXftQual		qual;
    const char		*field;
    MiniXftOp		op;
    MiniXftValue		value;
} MiniXftTest;

typedef struct _MiniXftEdit {
    struct _MiniXftEdit *next;
    const char	    *field;
    MiniXftOp	    op;
    MiniXftExpr	    *expr;
} MiniXftEdit;

typedef struct _MiniXftSubst {
    struct _MiniXftSubst	*next;
    MiniXftTest		*test;
    MiniXftEdit		*edit;
} MiniXftSubst;

/*
 * I tried this with functions that took va_list* arguments
 * but portability concerns made me change these functions
 * into macros (sigh).
 */

#define _MiniXftPatternVapBuild(result, orig, va)			    \
{								    \
    MiniXftPattern	*__p__ = (orig);				    \
    const char	*__o__;						    \
    MiniXftValue	__v__;						    \
								    \
    if (!__p__)							    \
    {								    \
	__p__ = MiniXftPatternCreate ();				    \
	if (!__p__)		    				    \
	    goto _MiniXftPatternVapBuild_bail0;			    \
    }				    				    \
    for (;;)			    				    \
    {				    				    \
	__o__ = va_arg (va, const char *);			    \
	if (!__o__)		    				    \
	    break;		    				    \
	__v__.type = va_arg (va, MiniXftType);			    \
	switch (__v__.type) {	    				    \
	case MiniXftTypeVoid:					    \
	    goto _MiniXftPatternVapBuild_bail1;       		    \
	case MiniXftTypeInteger:	    				    \
	    __v__.u.i = va_arg (va, int);			    \
	    break;						    \
	case MiniXftTypeDouble:					    \
	    __v__.u.d = va_arg (va, double);			    \
	    break;						    \
	case MiniXftTypeString:					    \
	    __v__.u.s = va_arg (va, char *);			    \
	    break;						    \
	case MiniXftTypeBool:					    \
	    __v__.u.b = va_arg (va, Bool);			    \
	    break;						    \
	case MiniXftTypeMatrix:					    \
	    __v__.u.m = va_arg (va, MiniXftMatrix *);		    \
	    break;						    \
	}							    \
	if (!MiniXftPatternAdd (__p__, __o__, __v__, True))		    \
	    goto _MiniXftPatternVapBuild_bail1;			    \
    }								    \
    result = __p__;						    \
    goto _MiniXftPatternVapBuild_return;				    \
								    \
_MiniXftPatternVapBuild_bail1:					    \
    if (!orig)							    \
	MiniXftPatternDestroy (__p__);				    \
_MiniXftPatternVapBuild_bail0:					    \
    result = 0;							    \
								    \
_MiniXftPatternVapBuild_return:					    \
    ;								    \
}


/* xftcache.c */

char *
MiniXftFileCacheFind (char *file, int id, int *count);

void
MiniXftFileCacheDispose (void);

void
MiniXftFileCacheLoad (char *cache);

Bool
MiniXftFileCacheUpdate (char *file, int id, char *name);

Bool
MiniXftFileCacheSave (char *cache);

Bool
MiniXftFileCacheReadDir (MiniXftFontSet *set, const char *cache_file);

Bool
MiniXftFileCacheWriteDir (MiniXftFontSet *set, const char *cache_file);
    
/* xftcfg.c */
Bool
MiniXftConfigAddDir (char *d);

Bool
MiniXftConfigSetCache (char *c);

char *
MiniXftConfigGetCache (void);

Bool
MiniXftConfigAddEdit (MiniXftTest *test, MiniXftEdit *edit);

Bool
_MiniXftConfigCompareValue (MiniXftValue    m,
			MiniXftOp	    op,
			MiniXftValue    v);

/* xftdbg.c */
void
MiniXftOpPrint (MiniXftOp op);

void
MiniXftTestPrint (MiniXftTest *test);

void
MiniXftExprPrint (MiniXftExpr *expr);

void
MiniXftEditPrint (MiniXftEdit *edit);

void
MiniXftSubstPrint (MiniXftSubst *subst);

/* xftdpy.c */
int
MiniXftDefaultParseBool (char *v);

Bool
MiniXftDefaultGetBool (Display *dpy, const char *object, int screen, Bool def);

int
MiniXftDefaultGetInteger (Display *dpy, const char *object, int screen, int def);

double
MiniXftDefaultGetDouble (Display *dpy, const char *object, int screen, double def);

MiniXftFontSet *
MiniXftDisplayGetFontSet (Display *dpy);

/* xftextent.c */
/* xftfont.c */
int
_MiniXftFontDebug (void);
    
/* xftfs.c */
/* xftgram.y */
int
MiniXftConfigparse (void);

int
MiniXftConfigwrap (void);
    
void
MiniXftConfigerror (char *fmt, ...);
    
char *
MiniXftConfigSaveField (const char *field);

MiniXftTest *
MiniXftTestCreate (MiniXftQual qual, const char *field, MiniXftOp compare, MiniXftValue value);

MiniXftExpr *
MiniXftExprCreateInteger (int i);

MiniXftExpr *
MiniXftExprCreateDouble (double d);

MiniXftExpr *
MiniXftExprCreateString (const char *s);

MiniXftExpr *
MiniXftExprCreateMatrix (const MiniXftMatrix *m);

MiniXftExpr *
MiniXftExprCreateBool (Bool b);

MiniXftExpr *
MiniXftExprCreateNil (void);

MiniXftExpr *
MiniXftExprCreateField (const char *field);

MiniXftExpr *
MiniXftExprCreateOp (MiniXftExpr *left, MiniXftOp op, MiniXftExpr *right);

void
MiniXftExprDestroy (MiniXftExpr *e);

MiniXftEdit *
MiniXftEditCreate (const char *field, MiniXftOp op, MiniXftExpr *expr);

void
MiniXftEditDestroy (MiniXftEdit *e);

/* xftinit.c */

/* xftlex.l */
extern int	MiniXftConfigLineno;
extern char	*MiniXftConfigFile;

int
MiniXftConfiglex (void);

Bool
MiniXftConfigLexFile(char *s);

Bool
MiniXftConfigPushInput (char *s, Bool complain);

/* xftlist.c */
Bool
MiniXftListValueCompare (MiniXftValue	v1,
		     MiniXftValue	v2);

Bool
MiniXftListValueListCompare (MiniXftValueList	*v1orig,
			 MiniXftValueList	*v2orig,
			 MiniXftQual	qual);

Bool
MiniXftListMatch (MiniXftPattern    *p,
	      MiniXftPattern    *font,
	      MiniXftQual	    qual);

Bool
MiniXftListAppend (MiniXftFontSet   *s,
	       MiniXftPattern   *font,
	       MiniXftObjectSet *os);


/* xftmatch.c */

/* xftname.c */
Bool
MiniXftNameConstant (char *string, int *result);

/* xftpat.c */

/* xftrender.c */

/* xftmatrix.c */
MiniXftMatrix *
_MiniXftSaveMatrix (const MiniXftMatrix *mat);

/* xftstr.c */
char *
_MiniXftSaveString (const char *s);

const char *
_MiniXftGetInt(const char *ptr, int *val);

char *
_MiniXftSplitStr (const char *field, char *save);

char *
_MiniXftDownStr (const char *field, char *save);

const char *
_MiniXftSplitField (const char *field, char *save);

const char *
_MiniXftSplitValue (const char *field, char *save);

int
_MiniXftMatchSymbolic (MiniXftSymbolic *s, int n, const char *name, int def);

int
_MiniXftStrCmpIgnoreCase (const char *s1, const char *s2);
    
/* xftxlfd.c */
Bool
MiniXftCoreAddFonts (MiniXftFontSet *set, Display *dpy, Bool ignore_scalable);

#endif /* _XFT_INT_H_ */
