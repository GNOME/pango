/*
 * $XFree86: xc/lib/MiniXft/MiniXft.h,v 1.19 2001/04/29 03:21:17 keithp Exp $
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

#ifndef _XFT_H_
#define _XFT_H_

#include "Xemu.h"

#include <stdarg.h>

typedef unsigned char	MiniXftChar8;
typedef unsigned short	MiniXftChar16;
typedef unsigned int	MiniXftChar32;

#define XFT_FAMILY	    "family"	/* String */
#define XFT_STYLE	    "style"	/* String */
#define XFT_SLANT	    "slant"	/* Int */
#define XFT_WEIGHT	    "weight"	/* Int */
#define XFT_SIZE	    "size"	/* Double */
#define XFT_PIXEL_SIZE	    "pixelsize"	/* Double */
#define XFT_ENCODING	    "encoding"	/* String */
#define XFT_SPACING	    "spacing"	/* Int */
#define XFT_FOUNDRY	    "foundry"	/* String */
#define XFT_CORE	    "core"	/* Bool */
#define XFT_ANTIALIAS	    "antialias"	/* Bool */
#define XFT_XLFD	    "xlfd"	/* String */
#define XFT_FILE	    "file"	/* String */
#define XFT_INDEX	    "index"	/* Int */
#define XFT_RASTERIZER	    "rasterizer"/* String */
#define XFT_OUTLINE	    "outline"	/* Bool */
#define XFT_SCALABLE	    "scalable"	/* Bool */
#define XFT_RGBA	    "rgba"	/* Int */

/* defaults from resources */
#define XFT_SCALE	    "scale"	/* double */
#define XFT_RENDER	    "render"	/* Bool */
#define XFT_MINSPACE	    "minspace"	/* Bool use minimum line spacing */
#define XFT_DPI		    "dpi"	/* double */

/* specific to FreeType rasterizer */
#define XFT_CHAR_WIDTH	    "charwidth"	/* Int */
#define XFT_CHAR_HEIGHT	    "charheight"/* Int */
#define XFT_MATRIX	    "matrix"    /* MiniXftMatrix */

#define XFT_WEIGHT_LIGHT	0
#define XFT_WEIGHT_MEDIUM	100
#define XFT_WEIGHT_DEMIBOLD	180
#define XFT_WEIGHT_BOLD		200
#define XFT_WEIGHT_BLACK	210

#define XFT_SLANT_ROMAN		0
#define XFT_SLANT_ITALIC	100
#define XFT_SLANT_OBLIQUE	110

#define XFT_PROPORTIONAL    0
#define XFT_MONO	    100
#define XFT_CHARCELL	    110

#define XFT_RGBA_NONE	    0
#define XFT_RGBA_RGB	    1
#define XFT_RGBA_BGR	    2
#define XFT_RGBA_VRGB	    3
#define XFT_RGBA_VBGR	    4

typedef enum _MiniXftType {
    MiniXftTypeVoid, 
    MiniXftTypeInteger, 
    MiniXftTypeDouble, 
    MiniXftTypeString, 
    MiniXftTypeBool,
    MiniXftTypeMatrix
} MiniXftType;

typedef struct _MiniXftMatrix {
    double xx, xy, yx, yy;
} MiniXftMatrix;

#define MiniXftMatrixInit(m)	((m)->xx = (m)->yy = 1, \
				 (m)->xy = (m)->yx = 0)

typedef enum _MiniXftResult {
    MiniXftResultMatch, MiniXftResultNoMatch, MiniXftResultTypeMismatch, MiniXftResultNoId
} MiniXftResult;

typedef struct _MiniXftValue {
    MiniXftType	type;
    union {
	char    *s;
	int	i;
	Bool	b;
	double	d;
	MiniXftMatrix *m;
    } u;
} MiniXftValue;

typedef struct _MiniXftValueList {
    struct _MiniXftValueList    *next;
    MiniXftValue		    value;
} MiniXftValueList;

typedef struct _MiniXftPatternElt {
    const char	    *object;
    MiniXftValueList    *values;
} MiniXftPatternElt;

typedef struct _MiniXftPattern {
    int		    num;
    int		    size;
    MiniXftPatternElt   *elts;
} MiniXftPattern;

typedef struct _MiniXftFontSet {
    int		nfont;
    int		sfont;
    MiniXftPattern	**fonts;
} MiniXftFontSet;

typedef struct _MiniXftObjectSet {
    int		nobject;
    int		sobject;
    const char	**objects;
} MiniXftObjectSet;

_XFUNCPROTOBEGIN

/* xftcfg.c */
Bool
MiniXftConfigSubstitute (MiniXftPattern *p);

/* xftdbg.c */
void
MiniXftValuePrint (MiniXftValue v);

void
MiniXftValueListPrint (MiniXftValueList *l);

void
MiniXftPatternPrint (MiniXftPattern *p);

void
MiniXftFontSetPrint (MiniXftFontSet *s);

/* xftdir.c */
/* xftdpy.c */
void
MiniXftSetDPI (double dpi);

Bool
MiniXftDefaultHasRender (Display *dpy);
    
Bool
MiniXftDefaultSet (Display *dpy, MiniXftPattern *defaults);

void
MiniXftDefaultSubstitute (Display *dpy, int screen, MiniXftPattern *pattern);

/* xftfont.c */
MiniXftPattern *
MiniXftFontMatch (Display *dpy, int screen, MiniXftPattern *pattern, MiniXftResult *result);

/* xftfreetype.c */
/* xftfs.c */

MiniXftFontSet *
MiniXftFontSetCreate (void);

void
MiniXftFontSetDestroy (MiniXftFontSet *s);

Bool
MiniXftFontSetAdd (MiniXftFontSet *s, MiniXftPattern *font);

/* xftglyphs.c */
/* see MiniXftFreetype.h */

/* xftgram.y */

/* xftinit.c */
extern MiniXftFontSet  *_MiniXftFontSet;

Bool
MiniXftInit (char *config);
    
/* xftlex.l */

/* xftlist.c */
MiniXftObjectSet *
MiniXftObjectSetCreate (void);

Bool
MiniXftObjectSetAdd (MiniXftObjectSet *os, const char *object);

void
MiniXftObjectSetDestroy (MiniXftObjectSet *os);

MiniXftObjectSet *
MiniXftObjectSetVaBuild (const char *first, va_list va);

MiniXftObjectSet *
MiniXftObjectSetBuild (const char *first, ...);

MiniXftFontSet *
MiniXftListFontSets (MiniXftFontSet	**sets,
		 int		nsets,
		 MiniXftPattern	*p,
		 MiniXftObjectSet	*os);

MiniXftFontSet *
MiniXftListFontsPatternObjects (Display	    *dpy,
			    int		    screen,
			    MiniXftPattern	    *pattern,
			    MiniXftObjectSet    *os);

MiniXftFontSet *
MiniXftListFonts (Display	*dpy,
	      int	screen,
	      ...);

/* xftmatch.c */
MiniXftPattern *
MiniXftFontSetMatch (MiniXftFontSet	**sets, 
		 int		nsets, 
		 MiniXftPattern	*p, 
		 MiniXftResult	*result);

/* xftmatrix.c */
int
MiniXftMatrixEqual (const MiniXftMatrix *mat1, const MiniXftMatrix *mat2);

void
MiniXftMatrixMultiply (MiniXftMatrix *result, MiniXftMatrix *a, MiniXftMatrix *b);

void
MiniXftMatrixRotate (MiniXftMatrix *m, double c, double s);

void
MiniXftMatrixScale (MiniXftMatrix *m, double sx, double sy);

void
MiniXftMatrixShear (MiniXftMatrix *m, double sh, double sv);

/* xftname.c */
MiniXftPattern *
MiniXftNameParse (const char *name);

Bool
MiniXftNameUnparse (MiniXftPattern *pat, char *dest, int len);

/* xftpat.c */
MiniXftPattern *
MiniXftPatternCreate (void);

MiniXftPattern *
MiniXftPatternDuplicate (MiniXftPattern *p);

void
MiniXftValueDestroy (MiniXftValue v);

void
MiniXftValueListDestroy (MiniXftValueList *l);
    
void
MiniXftPatternDestroy (MiniXftPattern *p);

MiniXftPatternElt *
MiniXftPatternFind (MiniXftPattern *p, const char *object, Bool insert);

Bool
MiniXftPatternAdd (MiniXftPattern *p, const char *object, MiniXftValue value, Bool append);
    
MiniXftResult
MiniXftPatternGet (MiniXftPattern *p, const char *object, int id, MiniXftValue *v);
    
Bool
MiniXftPatternDel (MiniXftPattern *p, const char *object);

Bool
MiniXftPatternAddInteger (MiniXftPattern *p, const char *object, int i);

Bool
MiniXftPatternAddDouble (MiniXftPattern *p, const char *object, double d);

Bool
MiniXftPatternAddString (MiniXftPattern *p, const char *object, const char *s);

Bool
MiniXftPatternAddMatrix (MiniXftPattern *p, const char *object, const MiniXftMatrix *s);

Bool
MiniXftPatternAddBool (MiniXftPattern *p, const char *object, Bool b);

MiniXftResult
MiniXftPatternGetInteger (MiniXftPattern *p, const char *object, int n, int *i);

MiniXftResult
MiniXftPatternGetDouble (MiniXftPattern *p, const char *object, int n, double *d);

MiniXftResult
MiniXftPatternGetString (MiniXftPattern *p, const char *object, int n, char **s);

MiniXftResult
MiniXftPatternGetMatrix (MiniXftPattern *p, const char *object, int n, MiniXftMatrix **s);

MiniXftResult
MiniXftPatternGetBool (MiniXftPattern *p, const char *object, int n, Bool *b);

MiniXftPattern *
MiniXftPatternVaBuild (MiniXftPattern *orig, va_list va);
    
MiniXftPattern *
MiniXftPatternBuild (MiniXftPattern *orig, ...);

/* xftrender.c */
/* see MiniXftFreetype.h */

/* xftstr.c */
_XFUNCPROTOEND

#endif /* _XFT_H_ */
