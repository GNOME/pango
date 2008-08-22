/* Pango
 * pango-layout-private.h: Internal structures of PangoLayout
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

#ifndef __PANGO_LAYOUT_PRIVATE_H__
#define __PANGO_LAYOUT_PRIVATE_H__

#include <pango/pango-layout.h>

G_BEGIN_DECLS

struct _PangoLayout
{
  GObject parent_instance;

  /* If you add fields to PangoLayout, be sure to update both
   * the _copy function
   */

  PangoContext *context;
  PangoAttrList *attrs;
  PangoFontDescription *font_desc;

  gchar *text;
  int length;			/* length of text in bytes */
  int width;			/* wrap/ellipsize width, in device units, or -1 if not set */
  int height;			/* ellipsize width, in device units if positive, number of lines if negative */
  int indent;			/* amount by which first line should be shorter */
  int spacing;			/* spacing between lines */

  int unknown_glyphs_count;	/* number of unknown glyphs */

  guint justify : 1;
  guint alignment : 2;

  guint single_paragraph : 1;
  guint auto_dir : 1;

  guint wrap : 2;		/* PangoWrapMode */
  guint is_wrapped : 1;		/* Whether the layout has any wrapped lines */
  guint ellipsize : 2;		/* PangoEllipsizeMode */
  guint is_ellipsized : 1;	/* Whether the layout has any ellipsized lines */

  /* some caching */
  guint logical_rect_cached : 1;
  guint ink_rect_cached : 1;
  PangoRectangle logical_rect;
  PangoRectangle ink_rect;


  gint n_chars;		        /* Total number of characters in layout */
  PangoLogAttr *log_attrs;	/* Logical attributes for layout's text */

  int tab_width;		/* Cached width of a tab. -1 == not yet calculated */

  PangoTabArray *tabs;

  GSList *lines;
  guint line_count;		/* Number of lines in @lines. 0 if lines is %NULL */
};

gboolean _pango_layout_line_ellipsize (PangoLayoutLine *line,
				       PangoAttrList   *attrs,
				       int              goal_width);

G_END_DECLS

#endif /* __PANGO_LAYOUT_PRIVATE_H__ */
