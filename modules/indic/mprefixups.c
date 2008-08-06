/*
 * mprefixups.h: Handle left matra placement
 *
 * Author: Sivaraj Doddannan
 * Ported from IBM's ICU engine.  Original copyright:
 * (C) Copyright IBM Corp. 1998-2003 - All Rights Reserved
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Library General Public
 * License as published by the Free Software Foundation; either
 * version 2 of the License, or (at your option) any later version.
 *
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Library General Public License for more details.
 *
 * You should have received a copy of the GNU Library General Public
 * License along with this library; if not, write to the
 * Free Software Foundation, Inc., 59 Temple Place - Suite 330,
 * Boston, MA 02111-1307, USA.
 */

#include "config.h"
#include <pango/pango-types.h>
#include "mprefixups.h"
#include <stdio.h>

struct _FixupData
{
    glong fBaseIndex;
    glong fMPreIndex;
};

MPreFixups *indic_mprefixups_new(glong char_count)
{
    MPreFixups *mprefixups = g_new (MPreFixups, 1);
    mprefixups->fFixupCount = 0;
    mprefixups->fFixupData = g_new (FixupData, char_count);

    return mprefixups;
}

void indic_mprefixups_free (MPreFixups *mprefixups)
{
    g_free (mprefixups->fFixupData);
    g_free (mprefixups);
}

void indic_mprefixups_add (MPreFixups *mprefixups, glong baseIndex, glong mpreIndex)
{
    /* NOTE: don't add the fixup data if the mpre is right
     * before the base consonant glyph.
     */
    if (baseIndex - mpreIndex > 1) {
       mprefixups->fFixupData[mprefixups->fFixupCount].fBaseIndex = baseIndex;
       mprefixups->fFixupData[mprefixups->fFixupCount].fMPreIndex = mpreIndex;

       mprefixups->fFixupCount += 1;
    }
}

void indic_mprefixups_apply(MPreFixups *mprefixups, PangoOTBuffer *buffer)
{
    glong fixup;

    for (fixup = 0; fixup < mprefixups->fFixupCount; fixup += 1) {
	gulong baseIndex = mprefixups->fFixupData[fixup].fBaseIndex;
	gulong mpreIndex = mprefixups->fFixupData[fixup].fMPreIndex;
	glong baseGlyph = -1;
	glong mpreGlyph = -1;
	glong mpreLimit = -1;
	glong mpreCount, moveCount, mpreDest;
	glong i;
	PangoOTGlyph *glyphs;
	int n_glyphs;
	PangoOTGlyph *mpreSave;

	/* determine post GSUB location of baseIndex and mpreIndex */

	pango_ot_buffer_get_glyphs (buffer, &glyphs, &n_glyphs);

	for (i = 0; i < n_glyphs; i++) {
	    if ((baseIndex >= glyphs[i].cluster) && (baseIndex-glyphs[i].cluster) % 2 == 0) /* bug 441654 */
		baseGlyph = i;
	    if (glyphs[i].cluster == mpreIndex) {
		    if (mpreGlyph < 0)
			    mpreGlyph = i;
		    mpreLimit = i + 1;
	    }
	}
	if (baseGlyph < 0 || mpreGlyph < 0 || mpreLimit >= baseGlyph) {
	    continue;
	}

	mpreCount  = mpreLimit - mpreGlyph;
	moveCount  = baseGlyph - mpreLimit;
	mpreDest   = baseGlyph - mpreCount;

	mpreSave    = g_new (PangoOTGlyph, mpreCount);

	for (i = 0; i < mpreCount; i += 1) {
	    mpreSave[i] = glyphs[mpreGlyph + i];
	}

	for (i = 0; i < moveCount; i += 1) {
	    glyphs[mpreGlyph + i] = glyphs[mpreLimit + i];
	}

	for (i = 0; i < mpreCount; i += 1) {
	    glyphs[mpreDest + i] = mpreSave[i];
	}

	g_free(mpreSave);
    }
}
