/* -*- mode: C; c-file-style: "gnu" -*- */
/* Pango
 * pango-glyph-item.h: Pair of PangoItem and a glyph string
 *
 * Copyright (C) 2002 Red Hat Software
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Library General Public
 * License as published by the Free Software Foundation; either
 * version 2 of the License, or (at your option) any later version.
 *
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.	 See the GNU
 * Library General Public License for more details.
 *
 * You should have received a copy of the GNU Library General Public
 * License along with this library; if not, write to the
 * Free Software Foundation, Inc., 59 Temple Place - Suite 330,
 * Boston, MA 02111-1307, USA.
 */

#ifndef __PANGO_GLYPH_ITEM_H__
#define __PANGO_GLYPH_ITEM_H__

#include <pango/pango-attributes.h>
#include <pango/pango-item.h>
#include <pango/pango-glyph.h>

G_BEGIN_DECLS

typedef struct _PangoGlyphItem PangoGlyphItem;

struct _PangoGlyphItem
{
  PangoItem        *item;
  PangoGlyphString *glyphs;
};

PangoGlyphItem *pango_glyph_item_split       (PangoGlyphItem *orig,
					      const char     *text,
					      int             split_index);
GSList *        pango_glyph_item_apply_attrs (PangoGlyphItem *glyph_item,
					      const char     *text,
					      PangoAttrList  *list);

G_END_DECLS

#endif /* __PANGO_GLYPH_ITEM_H__ */
