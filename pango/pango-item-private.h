/* Pango
 *
 * Copyright (C) 2021 Matthias Clasen
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

#ifndef __PANGO_ITEM_PRIVATE_H__
#define __PANGO_ITEM_PRIVATE_H__

#include <pango/pango-item.h>
#include <pango/pango-break.h>

G_BEGIN_DECLS

void               pango_analysis_collect_features    (const PangoAnalysis        *analysis,
                                                       hb_feature_t               *features,
                                                       guint                       length,
                                                       guint                      *num_features);

void               pango_analysis_set_size_font       (PangoAnalysis              *analysis,
                                                       PangoFont                  *font);
PangoFont *        pango_analysis_get_size_font       (const PangoAnalysis        *analysis);

GList *            pango_itemize_with_font            (PangoContext               *context,
                                                       PangoDirection              base_dir,
                                                       const char                 *text,
                                                       int                         start_index,
                                                       int                         length,
                                                       PangoAttrList              *attrs,
                                                       PangoAttrIterator          *cached_iter,
                                                       const PangoFontDescription *desc);

GList *            pango_itemize_post_process_items   (PangoContext               *context,
                                                       const char                 *text,
                                                       PangoLogAttr               *log_attrs,
                                                       GList                      *items);

void               pango_item_unsplit                 (PangoItem *orig,
                                                       int        split_index,
                                                       int        split_offset);


typedef struct _ItemProperties ItemProperties;
struct _ItemProperties
{
  guint uline_single        : 1;
  guint uline_double        : 1;
  guint uline_low           : 1;
  guint uline_error         : 1;
  guint strikethrough       : 1;
  guint oline_single        : 1;
  guint showing_space       : 1;
  guint no_paragraph_break  : 1;
  int letter_spacing;
  int line_spacing;
  int absolute_line_height;
  double line_height;
};

void               pango_item_get_properties          (PangoItem        *item,
                                                       ItemProperties   *properties);

G_END_DECLS

#endif /* __PANGO_ITEM_PRIVATE_H__ */
