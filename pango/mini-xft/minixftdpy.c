/*
 * $XFree86: xc/lib/MiniXft/xftdpy.c,v 1.7 2001/04/29 03:21:17 keithp Exp $
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

#include <stdio.h>
#include <stdlib.h>
#include <ctype.h>
#include "minixftint.h"

MiniXftDisplayInfo	*_MiniXftDisplayInfo;

static MiniXftDisplayInfo *
_MiniXftDisplayInfoGet (Display *dpy)
{
    MiniXftDisplayInfo  *info, **prev;

    for (prev = &_MiniXftDisplayInfo; (info = *prev); prev = &(*prev)->next)
    {
	if (info->display == dpy)
	{
	    /*
	     * MRU the list
	     */
	    if (prev != &_MiniXftDisplayInfo)
	    {
		*prev = info->next;
		info->next = _MiniXftDisplayInfo;
		_MiniXftDisplayInfo = info;
	    }
	    return info;
	}
    }
    info = (MiniXftDisplayInfo *) malloc (sizeof (MiniXftDisplayInfo));
    if (!info)
	goto bail0;
    
    info->display = dpy;
    info->defaults = 0;
    
    info->next = _MiniXftDisplayInfo;
    _MiniXftDisplayInfo = info;
    return info;
    
bail0:
    if (_MiniXftFontDebug () & XFT_DBG_RENDER)
    {
	printf ("MiniXftDisplayInfoGet failed to initialize, MiniXft unhappy\n");
    }
    return 0;
}

Bool
MiniXftDefaultHasRender (Display *dpy)
{
    return True;
}

Bool
MiniXftDefaultSet (Display *dpy, MiniXftPattern *defaults)
{
    MiniXftDisplayInfo  *info = _MiniXftDisplayInfoGet (dpy);

    if (!info)
	return False;
    if (info->defaults)
	MiniXftPatternDestroy (info->defaults);
    info->defaults = defaults;
    return True;
}

int
MiniXftDefaultParseBool (char *v)
{
    unsigned char    c0, c1;

    c0 = *v;
    if (isupper (c0))
	c0 = tolower (c0);
    if (c0 == 't' || c0 == 'y' || c0 == '1')
	return 1;
    if (c0 == 'f' || c0 == 'n' || c0 == '0')
	return 0;
    if (c0 == 'o')
    {
	c1 = v[1];
	if (isupper (c1))
	    c1 = tolower (c1);
	if (c1 == 'n')
	    return 1;
	if (c1 == 'f')
	    return 0;
    }
    return -1;
}

static Bool
_MiniXftDefaultInitBool (Display *dpy, MiniXftPattern *pat, char *option)
{
    return True;
}

static Bool
_MiniXftDefaultInitDouble (Display *dpy, MiniXftPattern *pat, char *option)
{
    return True;
}

static Bool
_MiniXftDefaultInitInteger (Display *dpy, MiniXftPattern *pat, char *option)
{
    return True;
}

static MiniXftPattern *
_MiniXftDefaultInit (Display *dpy)
{
    MiniXftPattern	*pat;

    pat = MiniXftPatternCreate ();
    if (!pat)
	goto bail0;

    if (!_MiniXftDefaultInitBool (dpy, pat, XFT_CORE))
	goto bail1;
    if (!_MiniXftDefaultInitDouble (dpy, pat, XFT_SCALE))
	goto bail1;
    if (!_MiniXftDefaultInitDouble (dpy, pat, XFT_DPI))
	goto bail1;
    if (!_MiniXftDefaultInitBool (dpy, pat, XFT_RENDER))
	goto bail1;
    if (!_MiniXftDefaultInitInteger (dpy, pat, XFT_RGBA))
	goto bail1;
    if (!_MiniXftDefaultInitBool (dpy, pat, XFT_ANTIALIAS))
	goto bail1;
    if (!_MiniXftDefaultInitBool (dpy, pat, XFT_MINSPACE))
	goto bail1;
    
    return pat;
    
bail1:
    MiniXftPatternDestroy (pat);
bail0:
    return 0;
}

static MiniXftResult
_MiniXftDefaultGet (Display *dpy, const char *object, int screen, MiniXftValue *v)
{
    MiniXftDisplayInfo  *info = _MiniXftDisplayInfoGet (dpy);
    MiniXftResult	    r;

    if (!info)
	return MiniXftResultNoMatch;
    
    if (!info->defaults)
    {
	info->defaults = _MiniXftDefaultInit (dpy);
	if (!info->defaults)
	    return MiniXftResultNoMatch;
    }
    r = MiniXftPatternGet (info->defaults, object, screen, v);
    if (r == MiniXftResultNoId && screen > 0)
	r = MiniXftPatternGet (info->defaults, object, 0, v);
    return r;
}

Bool
MiniXftDefaultGetBool (Display *dpy, const char *object, int screen, Bool def)
{
    MiniXftResult	    r;
    MiniXftValue	    v;

    r = _MiniXftDefaultGet (dpy, object, screen, &v);
    if (r != MiniXftResultMatch || v.type != MiniXftTypeBool)
	return def;
    return v.u.b;
}

int
MiniXftDefaultGetInteger (Display *dpy, const char *object, int screen, int def)
{
    MiniXftResult	    r;
    MiniXftValue	    v;

    r = _MiniXftDefaultGet (dpy, object, screen, &v);
    if (r != MiniXftResultMatch || v.type != MiniXftTypeInteger)
	return def;
    return v.u.i;
}

double
MiniXftDefaultGetDouble (Display *dpy, const char *object, int screen, double def)
{
    MiniXftResult	    r;
    MiniXftValue	    v;

    r = _MiniXftDefaultGet (dpy, object, screen, &v);
    if (r != MiniXftResultMatch || v.type != MiniXftTypeDouble)
	return def;
    return v.u.d;
}

MiniXftFontSet *
MiniXftDisplayGetFontSet (Display *dpy)
{
    return 0;
}

static double default_dpi = 0.0;

void
MiniXftSetDPI (double dpi)
{
  default_dpi = dpi;
}

void
MiniXftDefaultSubstitute (Display *dpy, int screen, MiniXftPattern *pattern)
{
    MiniXftValue	v;
    double	size;
    double	scale;

    if (MiniXftPatternGet (pattern, XFT_STYLE, 0, &v) == MiniXftResultNoMatch)
    {
	if (MiniXftPatternGet (pattern, XFT_WEIGHT, 0, &v) == MiniXftResultNoMatch )
	{
	    MiniXftPatternAddInteger (pattern, XFT_WEIGHT, XFT_WEIGHT_MEDIUM);
	}
	if (MiniXftPatternGet (pattern, XFT_SLANT, 0, &v) == MiniXftResultNoMatch)
	{
	    MiniXftPatternAddInteger (pattern, XFT_SLANT, XFT_SLANT_ROMAN);
	}
    }
    if (MiniXftPatternGet (pattern, XFT_ENCODING, 0, &v) == MiniXftResultNoMatch)
	MiniXftPatternAddString (pattern, XFT_ENCODING, "iso8859-1");
    if (MiniXftPatternGet (pattern, XFT_RENDER, 0, &v) == MiniXftResultNoMatch)
    {
	MiniXftPatternAddBool (pattern, XFT_RENDER,
			   MiniXftDefaultGetBool (dpy, XFT_RENDER, screen, 
					      MiniXftDefaultHasRender (dpy)));
    }
    if (MiniXftPatternGet (pattern, XFT_CORE, 0, &v) == MiniXftResultNoMatch)
    {
	MiniXftPatternAddBool (pattern, XFT_CORE,
			   MiniXftDefaultGetBool (dpy, XFT_CORE, screen, 
					      !MiniXftDefaultHasRender (dpy)));
    }
    if (MiniXftPatternGet (pattern, XFT_ANTIALIAS, 0, &v) == MiniXftResultNoMatch)
    {
	MiniXftPatternAddBool (pattern, XFT_ANTIALIAS,
			   MiniXftDefaultGetBool (dpy, XFT_ANTIALIAS, screen,
					      True));
    }
    if (MiniXftPatternGet (pattern, XFT_RGBA, 0, &v) == MiniXftResultNoMatch)
    {
	MiniXftPatternAddInteger (pattern, XFT_RGBA,
			      MiniXftDefaultGetInteger (dpy, XFT_RGBA, screen, 
						    XFT_RGBA_NONE));
    }
    if (MiniXftPatternGet (pattern, XFT_MINSPACE, 0, &v) == MiniXftResultNoMatch)
    {
	MiniXftPatternAddBool (pattern, XFT_MINSPACE,
			   MiniXftDefaultGetBool (dpy, XFT_MINSPACE, screen,
					      False));
    }
    if (MiniXftPatternGet (pattern, XFT_PIXEL_SIZE, 0, &v) == MiniXftResultNoMatch)
    {
	double	dpi;

	if (MiniXftPatternGet (pattern, XFT_SIZE, 0, &v) != MiniXftResultMatch)
	{
	    size = 12.0;
	    MiniXftPatternAddDouble (pattern, XFT_SIZE, size);
	}
	else
	{
	    switch (v.type) {
	    case MiniXftTypeInteger:
		size = (double) v.u.i;
		break;
	    case MiniXftTypeDouble:
		size = v.u.d;
		break;
	    default:
		size = 12.0;
		break;
	    }
	}
	scale = MiniXftDefaultGetDouble (dpy, XFT_SCALE, screen, 1.0);
	size *= scale;
	if (default_dpi > 0.0)
	  dpi = default_dpi;
	else
	  dpi = 72.0;
	dpi = MiniXftDefaultGetDouble (dpy, XFT_DPI, screen, dpi);
	size = size * dpi / 72.0;
	MiniXftPatternAddDouble (pattern, XFT_PIXEL_SIZE, size);
    }
}

