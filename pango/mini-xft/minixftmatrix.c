/*
 * $XFree86: xc/lib/MiniXft/xftmatrix.c,v 1.1 2001/03/30 18:50:18 keithp Exp $
 *
 * Copyright © 2000 Tuomas J. Lukka
 *
 * Permission to use, copy, modify, distribute, and sell this software and its
 * documentation for any purpose is hereby granted without fee, provided that
 * the above copyright notice appear in all copies and that both that
 * copyright notice and this permission notice appear in supporting
 * documentation, and that the name of Tuomas Lukka not be used in
 * advertising or publicity pertaining to distribution of the software without
 * specific, written prior permission.  Tuomas Lukka makes no
 * representations about the suitability of this software for any purpose.  It
 * is provided "as is" without express or implied warranty.
 *
 * TUOMAS LUKKA DISCLAIMS ALL WARRANTIES WITH REGARD TO THIS SOFTWARE,
 * INCLUDING ALL IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS, IN NO
 * EVENT SHALL TUOMAS LUKKA BE LIABLE FOR ANY SPECIAL, INDIRECT OR
 * CONSEQUENTIAL DAMAGES OR ANY DAMAGES WHATSOEVER RESULTING FROM LOSS OF USE,
 * DATA OR PROFITS, WHETHER IN AN ACTION OF CONTRACT, NEGLIGENCE OR OTHER
 * TORTIOUS ACTION, ARISING OUT OF OR IN CONNECTION WITH THE USE OR
 * PERFORMANCE OF THIS SOFTWARE.
 */

#include <math.h>
#include <stdlib.h>
#include <ctype.h>
#include "minixftint.h"

MiniXftMatrix *
_MiniXftSaveMatrix (const MiniXftMatrix *mat) 
{
    MiniXftMatrix *r;
    if(!mat) 
	return 0;
    r = (MiniXftMatrix *) malloc (sizeof (*r) );
    if (!r)
	return 0;
    *r = *mat;
    return r;
}

int
MiniXftMatrixEqual (const MiniXftMatrix *mat1, const MiniXftMatrix *mat2)
{
    if(mat1 == mat2) return True;
    if(mat1 == 0 || mat2 == 0) return False;
    return mat1->xx == mat2->xx && 
	   mat1->xy == mat2->xy &&
	   mat1->yx == mat2->yx &&
	   mat1->yy == mat2->yy;
}

void
MiniXftMatrixMultiply (MiniXftMatrix *result, MiniXftMatrix *a, MiniXftMatrix *b)
{
    MiniXftMatrix	r;

    r.xx = a->xx * b->xx + a->xy * b->yx;
    r.xy = a->xx * b->xy + a->xy * b->yy;
    r.yx = a->yx * b->xx + a->yy * b->yx;
    r.yy = a->yx * b->xy + a->yy * b->yy;
    *result = r;
}

void
MiniXftMatrixRotate (MiniXftMatrix *m, double c, double s)
{
    MiniXftMatrix	r;

    /*
     * X Coordinate system is upside down, swap to make
     * rotations counterclockwise
     */
    r.xx = c;
    r.xy = -s;
    r.yx = s;
    r.yy = c;
    MiniXftMatrixMultiply (m, &r, m);
}

void
MiniXftMatrixScale (MiniXftMatrix *m, double sx, double sy)
{
    MiniXftMatrix	r;

    r.xx = sx;
    r.xy = 0;
    r.yx = 0;
    r.yy = sy;
    MiniXftMatrixMultiply (m, &r, m);
}

void
MiniXftMatrixShear (MiniXftMatrix *m, double sh, double sv)
{
    MiniXftMatrix	r;

    r.xx = 1;
    r.xy = sh;
    r.yx = sv;
    r.yy = 1;
    MiniXftMatrixMultiply (m, &r, m);
}
