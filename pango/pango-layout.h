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
#include <pango/pango-tabs.h>

#ifdef __cplusplus
extern "C" {
#endif /* __cplusplus */

typedef struct _PangoLayout      PangoLayout;
typedef struct _PangoLayoutClass PangoLayoutClass;
typedef struct _PangoLayoutLine  PangoLayoutLine;
typedef struct _PangoLayoutRun   PangoLayoutRun;

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

#define PANGO_TYPE_LAYOUT              (pango_layout_get_type ())
#define PANGO_LAYOUT(object)           (G_TYPE_CHECK_INSTANCE_CAST ((object), PANGO_TYPE_LAYOUT, PangoLayout))
#define PANGO_LAYOUT_CLASS(klass)      (G_TYPE_CHECK_CLASS_CAST ((klass), PANGO_TYPE_LAYOUT, PangoLayoutClass))
#define PANGO_IS_LAYOUT(object)        (G_TYPE_CHECK_INSTANCE_TYPE ((object), PANGO_TYPE_LAYOUT))
#define PANGO_IS_LAYOUT_CLASS(klass)   (G_TYPE_CHECK_CLASS_TYPE ((klass), PANGO_TYPE_LAYOUT))
#define PANGO_LAYOUT_GET_CLASS(obj)    (G_TYPE_INSTANCE_GET_CLASS ((obj), PANGO_TYPE_LAYOUT, PangoLayoutClass))


/* The PangoLayout and PangoLayoutClass structs are private; if you
 * need to create a subclass of these, mail otaylor@redhat.com
 */

GType        pango_layout_get_type       (void) G_GNUC_CONST;
PangoLayout *pango_layout_new            (PangoContext   *context);

PangoContext  *pango_layout_get_context    (PangoLayout    *layout);

void           pango_layout_set_attributes (PangoLayout    *layout,
					    PangoAttrList  *attrs);
PangoAttrList *pango_layout_get_attributes (PangoLayout    *layout);

void           pango_layout_set_text       (PangoLayout    *layout,
					    const char     *text,
					    int             length);

void           pango_layout_set_markup     (PangoLayout    *layout,
                                            const char     *markup,
                                            int             length);

void           pango_layout_set_markup_with_accel (PangoLayout    *layout,
                                                   const char     *markup,
                                                   int             length,
                                                   gunichar        accel_marker,
                                                   gunichar       *accel_char);

void           pango_layout_set_font_description (PangoLayout                *layout,
						  const PangoFontDescription *desc);
void           pango_layout_set_width            (PangoLayout                *layout,
						  int                         width);
int            pango_layout_get_width            (PangoLayout                *layout);
void           pango_layout_set_indent           (PangoLayout                *layout,
						  int                         indent);
int            pango_layout_get_indent           (PangoLayout                *layout);
void           pango_layout_set_spacing          (PangoLayout                *layout,
						  int                         spacing);
int            pango_layout_get_spacing          (PangoLayout                *layout);
void           pango_layout_set_justify          (PangoLayout                *layout,
						  gboolean                    justify);
gboolean       pango_layout_get_justify          (PangoLayout                *layout);
void           pango_layout_set_alignment        (PangoLayout                *layout,
						  PangoAlignment              alignment);
PangoAlignment pango_layout_get_alignment        (PangoLayout                *layout);

void           pango_layout_set_tabs             (PangoLayout                *layout,
                                                  PangoTabArray              *tabs);

PangoTabArray* pango_layout_get_tabs             (PangoLayout                *layout);

void           pango_layout_context_changed (PangoLayout    *layout);

void     pango_layout_get_log_attrs (PangoLayout    *layout,
				     PangoLogAttr  **attrs,
				     gint           *n_attrs);

void     pango_layout_index_to_pos         (PangoLayout    *layout,
					    int             index,
					    PangoRectangle *pos);
void     pango_layout_get_cursor_pos       (PangoLayout    *layout,
					    int             index,
					    PangoRectangle *strong_pos,
					    PangoRectangle *weak_pos);
void     pango_layout_move_cursor_visually (PangoLayout    *layout,
					    int             old_index,
					    int             old_trailing,
					    int             direction,
					    int            *new_index,
					    int            *new_trailing);
gboolean pango_layout_xy_to_index          (PangoLayout    *layout,
					    int             x,
					    int             y,
					    int            *index,
					    gboolean       *trailing);
void     pango_layout_get_extents          (PangoLayout    *layout,
					    PangoRectangle *ink_rect,
					    PangoRectangle *logical_rect);
void     pango_layout_get_pixel_extents    (PangoLayout    *layout,
					    PangoRectangle *ink_rect,
					    PangoRectangle *logical_rect);
void     pango_layout_get_size             (PangoLayout    *layout,
					    int            *width,
					    int            *height);
void     pango_layout_get_pixel_size       (PangoLayout    *layout,
					    int            *width,
					    int            *height);

int              pango_layout_get_line_count       (PangoLayout    *layout);
PangoLayoutLine *pango_layout_get_line             (PangoLayout    *layout,
						    int             line);
GSList *         pango_layout_get_lines            (PangoLayout    *layout);

void     pango_layout_line_ref          (PangoLayoutLine  *line);
void     pango_layout_line_unref        (PangoLayoutLine  *line);
void     pango_layout_line_x_to_index   (PangoLayoutLine  *line,
					 int               x_pos,
					 int              *index,
					 int              *trailing);
void     pango_layout_line_index_to_x   (PangoLayoutLine  *line,
					 int               index,
					 gboolean          trailing,
					 int              *x_pos);
void     pango_layout_line_get_x_ranges (PangoLayoutLine  *line,
					 int               start_index,
					 int               end_index,
					 int             **ranges,
					 int              *n_ranges);
void     pango_layout_line_get_extents  (PangoLayoutLine  *line,
					 PangoRectangle   *ink_rect,
					 PangoRectangle   *logical_rect);
void     pango_layout_line_get_pixel_extents (PangoLayoutLine *layout_line,
					      PangoRectangle  *ink_rect,
					      PangoRectangle  *logical_rect);

#ifdef __cplusplus
}
#endif /* __cplusplus */

#endif /* __PANGO_LAYOUT_H__ */

