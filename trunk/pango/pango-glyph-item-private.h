/* Pango
 * pango-glyph-item-private.h: Pair of PangoItem and a glyph string; private
 *   functionality
 *
 * Copyright (C) 2004 Red Hat Software
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

#ifndef __PANGO_GLYPH_ITEM_PRIVATE_H__
#define __PANGO_GLYPH_ITEM_PRIVATE_H__

#include <pango-glyph-item.h>

G_BEGIN_DECLS

/* Structure holding state when we're iterating over a GlyphItem.
 * start_index/cluster_end (and range_start/range_end in
 * apply_attrs()) are offsets into the text, so note the difference
 * of glyph_item->item->offset between them and clusters in the
 * log_clusters[] array.
 */
typedef struct _PangoGlyphItemIter PangoGlyphItemIter;

struct _PangoGlyphItemIter
{
  PangoGlyphItem *glyph_item;
  const gchar *text;

  int start_glyph;
  int start_index;
  int start_char;

  int end_glyph;
  int end_index;
  int end_char;
};

gboolean _pango_glyph_item_iter_init_start   (PangoGlyphItemIter *iter,
					      PangoGlyphItem     *glyph_item,
					      const char         *text);
gboolean _pango_glyph_item_iter_init_end     (PangoGlyphItemIter *iter,
					      PangoGlyphItem     *glyph_item,
					      const char         *text);
gboolean _pango_glyph_item_iter_next_cluster (PangoGlyphItemIter *iter);
gboolean _pango_glyph_item_iter_prev_cluster (PangoGlyphItemIter *iter);

G_END_DECLS

#endif /* __PANGO_GLYPH_ITEM_PRIVATE_H__ */
