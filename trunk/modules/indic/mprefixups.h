/*
 * mprefixups.c: Handle left matra placement
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

#ifndef __MPREFIXUPS_H
#define __MPREFIXUPS_H

#include <pango/pango-types.h>
#include <pango/pango-glyph.h>
#include <pango/pango-ot.h>

G_BEGIN_DECLS

typedef struct _FixupData FixupData;

struct _MPreFixups {
  glong		fFixupCount;
  FixupData	*fFixupData;
};

typedef struct _MPreFixups MPreFixups;

MPreFixups *indic_mprefixups_new(glong char_count);
void        indic_mprefixups_free(MPreFixups *mprefixups);
void        indic_mprefixups_add(MPreFixups *mprefixups, glong baseIndex, glong mpreIndex);
void        indic_mprefixups_apply(MPreFixups *mprefixups, PangoOTBuffer *buffer);


G_END_DECLS

#endif

