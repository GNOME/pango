/*
 * $XFree86: xc/lib/MiniXft/xftfreetype.c,v 1.14 2001/09/21 19:54:53 keithp Exp $
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
#include <stdio.h>
#include <string.h>
#include "minixftint.h"

FT_Library  _MiniXftFTlibrary;

typedef struct _MiniXftFtEncoding {
    const char	*name;
    FT_Encoding	encoding;
} MiniXftFtEncoding;

static MiniXftFtEncoding xftFtEncoding[] = {
    { "iso10646-1",	    ft_encoding_unicode, },
    { "iso8859-1",	    ft_encoding_unicode, },
    { "apple-roman",	    ft_encoding_apple_roman },
    { "adobe-fontspecific", ft_encoding_symbol,  },
    { "glyphs-fontspecific",ft_encoding_none,	 },
};

#define NUM_FT_ENCODINGS    (sizeof xftFtEncoding / sizeof xftFtEncoding[0])

#define FT_Matrix_Equal(a,b)	((a)->xx == (b)->xx && \
				 (a)->yy == (b)->yy && \
				 (a)->xy == (b)->xy && \
				 (a)->yx == (b)->yx)

MiniXftPattern *
MiniXftFreeTypeQuery (const char *file, int id, int *count)
{
    FT_Face	face;
    MiniXftPattern	*pat;
    int		slant;
    int		weight;
    int		i, j;
    
    if (FT_New_Face (_MiniXftFTlibrary, file, id, &face))
	return 0;

    *count = face->num_faces;
    
    pat = MiniXftPatternCreate ();
    if (!pat)
	goto bail0;


    if (!MiniXftPatternAddBool (pat, XFT_CORE, False))
	goto bail1;
    
    if (!MiniXftPatternAddBool (pat, XFT_OUTLINE,
			    (face->face_flags & FT_FACE_FLAG_SCALABLE) != 0))
	goto bail1;
    
    if (!MiniXftPatternAddBool (pat, XFT_SCALABLE,
			    (face->face_flags & FT_FACE_FLAG_SCALABLE) != 0))
	goto bail1;
    

    slant = XFT_SLANT_ROMAN;
    if (face->style_flags & FT_STYLE_FLAG_ITALIC)
	slant = XFT_SLANT_ITALIC;

    if (!MiniXftPatternAddInteger (pat, XFT_SLANT, slant))
	goto bail1;
    
    weight = XFT_WEIGHT_MEDIUM;
    if (face->style_flags & FT_STYLE_FLAG_BOLD)
	weight = XFT_WEIGHT_BOLD;
    
    if (!MiniXftPatternAddInteger (pat, XFT_WEIGHT, weight))
	goto bail1;
    
    if (!MiniXftPatternAddString (pat, XFT_FAMILY, face->family_name))
	goto bail1;

    if (!MiniXftPatternAddString (pat, XFT_STYLE, face->style_name))
	goto bail1;

    if (!MiniXftPatternAddString (pat, XFT_FILE, file))
	goto bail1;

    if (!MiniXftPatternAddInteger (pat, XFT_INDEX, id))
	goto bail1;
    
#if 0
    if ((face->face_flags & FT_FACE_FLAG_FIXED_WIDTH) != 0)
	if (!MiniXftPatternAddInteger (pat, XFT_SPACING, XFT_MONO))
	    goto bail1;
#endif
    
    if (!(face->face_flags & FT_FACE_FLAG_SCALABLE))
    {
	for (i = 0; i < face->num_fixed_sizes; i++)
	    if (!MiniXftPatternAddDouble (pat, XFT_PIXEL_SIZE,
				      (double) face->available_sizes[i].height))
		goto bail1;
    }
    
    for (i = 0; i < face->num_charmaps; i++)
    {
#if 0	
	printf ("face %s encoding %d %c%c%c%c\n",
		face->family_name, i, 
		face->charmaps[i]->encoding >> 24,
		face->charmaps[i]->encoding >> 16,
		face->charmaps[i]->encoding >> 8,
		face->charmaps[i]->encoding >> 0);
#endif
	for (j = 0; j < NUM_FT_ENCODINGS; j++)
	{
	    if (face->charmaps[i]->encoding == xftFtEncoding[j].encoding)
	    {
		if (!MiniXftPatternAddString (pat, XFT_ENCODING, 
					  xftFtEncoding[j].name))
		    goto bail1;
	    }
	}
    }

    if (!MiniXftPatternAddString (pat, XFT_ENCODING, 
			      "glyphs-fontspecific"))
	goto bail1;


    FT_Done_Face (face);
    return pat;
    
bail1:
    MiniXftPatternDestroy (pat);
bail0:
    FT_Done_Face (face);
    return 0;
}

/* #define XFT_DEBUG_FONTSET */
Bool
MiniXftInitFtLibrary (void)
{
    char    **d;
    char    *cache;
    
    if (_MiniXftFTlibrary)
	return True;
    if (FT_Init_FreeType (&_MiniXftFTlibrary))
	return False;
    _MiniXftFontSet = MiniXftFontSetCreate ();
    if (!_MiniXftFontSet)
	return False;
    cache = MiniXftConfigGetCache ();
    if (cache)
	MiniXftFileCacheLoad (cache);
    for (d = MiniXftConfigDirs; d && *d; d++)
    {
#ifdef XFT_DEBUG_FONTSET
	printf ("scan dir %s\n", *d);
#endif
	MiniXftDirScan (_MiniXftFontSet, *d, False);
    }
#ifdef XFT_DEBUG_FONTSET
    MiniXftFontSetPrint (_MiniXftFontSet);
#endif
    if (cache)
	MiniXftFileCacheSave (cache);
    MiniXftFileCacheDispose ();
    return True;
}
