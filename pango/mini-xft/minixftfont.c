/*
 * $XFree86: xc/lib/MiniXft/xftfont.c,v 1.8 2000/12/20 00:20:48 keithp Exp $
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
#include "minixftint.h"

MiniXftPattern *
MiniXftFontMatch (Display *dpy, int screen, MiniXftPattern *pattern, MiniXftResult *result)
{
    MiniXftPattern	*new;
    MiniXftPattern	*match;
    MiniXftFontSet	*sets[2];
    int		nsets;
    Bool	render, core;

    if (!MiniXftInit (0))
	return 0;
    
    new = MiniXftPatternDuplicate (pattern);
    if (!new)
	return 0;

    if (_MiniXftFontDebug () & XFT_DBG_OPENV)
    {
	printf ("MiniXftFontMatch pattern ");
	MiniXftPatternPrint (new);
    }
    MiniXftConfigSubstitute (new);
    if (_MiniXftFontDebug () & XFT_DBG_OPENV)
    {
	printf ("MiniXftFontMatch after MiniXftConfig substitutions ");
	MiniXftPatternPrint (new);
    }
    MiniXftDefaultSubstitute (dpy, screen, new);
    if (_MiniXftFontDebug () & XFT_DBG_OPENV)
    {
	printf ("MiniXftFontMatch after X resource substitutions ");
	MiniXftPatternPrint (new);
    }
    nsets = 0;
    
    render = True;
    core = False;
    (void) MiniXftPatternGetBool (new, XFT_RENDER, 0, &render);
    (void) MiniXftPatternGetBool (new, XFT_CORE, 0, &core);
    if (_MiniXftFontDebug () & XFT_DBG_OPENV)
    {
	printf ("MiniXftFontMatch: use core fonts \"%s\", use render fonts \"%s\"\n",
		core ? "True" : "False", render ? "True" : "False");
    }

    if (render)
    {
	if (MiniXftInitFtLibrary ())
	{
	    sets[nsets] = _MiniXftFontSet;
	    if (sets[nsets])
		nsets++;
	}
    }
    
    match = MiniXftFontSetMatch (sets, nsets, new, result);
    MiniXftPatternDestroy (new);
    return match;
}

int
_MiniXftFontDebug (void)
{
    static int	initialized;
    static int	debug;

    if (!initialized)
    {
	char	*e;
	
	initialized = 1;
	e = getenv ("XFT_DEBUG");
	if (e)
	{
	    printf ("XFT_DEBUG=%s\n", e);
	    debug = atoi (e);
	    if (debug <= 0)
		debug = 1;
	}
    }
    return debug;
}

