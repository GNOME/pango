/*
 * $XFree86: xc/lib/MiniXft/MiniXftFreetype.h,v 1.15 2001/03/31 23:07:29 keithp Exp $
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

#ifndef _XFTFREETYPE_H_
#define _XFTFREETYPE_H_

#include "MiniXft.h"
#include <freetype/freetype.h>

extern FT_Library	_MiniXftFTlibrary;

_XFUNCPROTOBEGIN

/* xftdir.c */
Bool
MiniXftDirScan (MiniXftFontSet *set, const char *dir, Bool force);

Bool
MiniXftDirSave (MiniXftFontSet *set, const char *dir);

/* xftfreetype.c */
MiniXftPattern *
MiniXftFreeTypeQuery (const char *file, int id, int *count);

Bool
MiniXftInitFtLibrary(void);

_XFUNCPROTOEND

#endif /* _XFTFREETYPE_H_ */



