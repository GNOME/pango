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

#ifdef __cplusplus
extern "C" {
#endif /* __cplusplus */

typedef struct _PangoLayout     PangoLayout;
typedef struct _PangoLayoutLine PangoLayoutLine;
typedef struct _PangoLayoutRun  PangoLayoutRun; 

struct _PangoLayoutLine
{
  PangoLayout *layout;
  gint         n_chars;		/* length of line in characters */
  gint         length;		/* length of line in bytes*/
  GSList      *runs;
};

struct _PangoLayoutRun
{
  PangoItem        *item;
  PangoGlyphString *glyphs;
};

PangoLayout *    pango_layout_new                  (void);
void             pango_layout_ref                  (PangoLayout   *layout);
void             pango_layout_unref                (PangoLayout   *layout);
void             pango_layout_set_width            (PangoLayout   *layout,
						    int            width);
void             pango_layout_set_justify          (PangoLayout   *layout,
						    gboolean       justify);
void             pango_layout_set_first_line_width (PangoLayout   *layout,
						    int            width);
void             pango_layout_set_attributes       (PangoLayout   *layout,
						    PangoAttrList *attrs);
void             pango_layout_set_text             (char          *text,
						    int            length);
int              pango_layout_get_line_count       (PangoLayout   *layout);
PangoLayoutLine *pango_layout_get_line             (PangoLayout   *layout,
						    int            line);
void             pango_layout_cp_to_line_x         (PangoLayout   *layout,
						    gint           char_pos,
						    gboolean       trailing,
						    gint          *line,
						    gint          *x_pos);

void pango_layout_line_ref         (PangoLayoutLine *line);
void pango_layout_line_unref       (PangoLayoutLine *line);
void pango_layout_line_x_to_cp     (PangoLayoutLine *line,
				    gint             x_pos,
				    gint            *char_pos,
				    gint            *trailing);
void pango_layout_line_get_extents (PangoLayoutLine *line,
				    PangoRectangle  *ink_rect,
				    PangoRectangle  *logical_rect);

#ifdef __cplusplus
}
#endif /* __cplusplus */

#endif __PANGO_LAYOUT_H__

