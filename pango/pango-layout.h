/* Pango
 * pango-layout.h: Highlevel layout driver
 *
 * Copyright (C) 2000 Red Hat Software
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

#ifndef __PANGO_LAYOUT_H__
#define __PANGO_LAYOUT_H__

#include <pango/pango-attributes.h>
#include <pango/pango-context.h>
#include <pango/pango-glyph.h>

#ifdef __cplusplus
extern "C" {
#endif /* __cplusplus */

typedef struct _PangoLayout     PangoLayout;
typedef struct _PangoLayoutLine PangoLayoutLine;
typedef struct _PangoLayoutRun  PangoLayoutRun; 

typedef enum {
  PANGO_ALIGN_LEFT,
  PANGO_ALIGN_CENTER,
  PANGO_ALIGN_RIGHT
} PangoAlignment;

struct _PangoLayoutLine
{
  PangoLayout *layout;
  gint         length;		/* length of line in bytes*/
  GSList      *runs;
};

struct _PangoLayoutRun
{
  PangoItem        *item;
  PangoGlyphString *glyphs;
};

PangoLayout *pango_layout_new            (PangoContext   *context);
void         pango_layout_ref            (PangoLayout    *layout);
void         pango_layout_unref          (PangoLayout    *layout);


void           pango_layout_set_attributes (PangoLayout    *layout,
					    PangoAttrList  *attrs);
void           pango_layout_set_text       (PangoLayout    *layout,
					    char           *text,
					    int             length);

void           pango_layout_set_width     (PangoLayout    *layout,
					   int             width);
int            pango_layout_get_width     (PangoLayout    *layout);
void           pango_layout_set_indent    (PangoLayout    *layout,
					   int             indent);
int            pango_layout_get_indent    (PangoLayout    *layout);
void           pango_layout_set_justify   (PangoLayout    *layout,
					   gboolean        justify);
gboolean       pango_layout_get_justify   (PangoLayout    *layout);
void           pango_layout_set_alignment (PangoLayout    *layout,
					   PangoAlignment  alignment);
PangoAlignment pango_layout_get_alignment (PangoLayout    *layout);

void           pango_layout_context_changed (PangoLayout    *layout);

void     pango_layout_index_to_pos (PangoLayout     *layout,
				    int              index,
				    PangoRectangle  *pos);
gboolean pango_layout_xy_to_index  (PangoLayout     *layout,
				    int              x,
				    int              y,
				    int             *index,
				    gboolean        *trailing);
void     pango_layout_get_extents  (PangoLayout     *layout,
				    PangoRectangle  *ink_rect,
				    PangoRectangle  *logical_rect);

int              pango_layout_get_line_count       (PangoLayout    *layout);
PangoLayoutLine *pango_layout_get_line             (PangoLayout    *layout,
						    int             line);
GSList *         pango_layout_get_lines            (PangoLayout    *layout);

void     pango_layout_line_ref         (PangoLayoutLine *line);
void     pango_layout_line_unref       (PangoLayoutLine *line);
gboolean pango_layout_line_x_to_index  (PangoLayoutLine *line,
					int              x_pos,
					int             *index,
					int             *trailing);
void     pango_layout_line_index_to_x  (PangoLayoutLine *line,
					int              index,
					gboolean         trailing,
					int             *x_pos);
void     pango_layout_line_get_extents (PangoLayoutLine *line,
					PangoRectangle  *ink_rect,
					PangoRectangle  *logical_rect);

#ifdef __cplusplus
}
#endif /* __cplusplus */

#endif __PANGO_LAYOUT_H__

