#pragma once

#include <glib-object.h>

#include <pango/pango-types.h>
#include <pango/pango-layout-run.h>

G_BEGIN_DECLS

#define PANGO_TYPE_LAYOUT_LINE pango_layout_line_get_type ()

PANGO_AVAILABLE_IN_ALL
G_DECLARE_FINAL_TYPE (PangoLayoutLine, pango_layout_line, PANGO, LAYOUT_LINE, GObject);

PANGO_AVAILABLE_IN_ALL
PangoLayoutLine *       pango_layout_line_justify       (PangoLayoutLine       *line,
                                                         int                    width);

PANGO_AVAILABLE_IN_ALL
GSList *                pango_layout_line_get_runs      (PangoLayoutLine       *line);

PANGO_AVAILABLE_IN_ALL
const char *            pango_layout_line_get_text      (PangoLayoutLine       *line,
                                                         int                   *start_index,
                                                         int                   *length);

PANGO_AVAILABLE_IN_ALL
int                     pango_layout_line_get_start_index (PangoLayoutLine     *line);

PANGO_AVAILABLE_IN_ALL
int                     pango_layout_line_get_length      (PangoLayoutLine     *line);

PANGO_AVAILABLE_IN_ALL
const PangoLogAttr *    pango_layout_line_get_log_attrs (PangoLayoutLine       *line,
                                                         int                   *start_offset,
                                                         int                   *n_attrs);

PANGO_AVAILABLE_IN_ALL
gboolean                pango_layout_line_is_wrapped    (PangoLayoutLine       *line);

PANGO_AVAILABLE_IN_ALL
gboolean                pango_layout_line_is_ellipsized (PangoLayoutLine       *line);

PANGO_AVAILABLE_IN_ALL
gboolean                pango_layout_line_is_hyphenated (PangoLayoutLine       *line);

PANGO_AVAILABLE_IN_ALL
gboolean                pango_layout_line_is_justified  (PangoLayoutLine       *line);

PANGO_AVAILABLE_IN_ALL
gboolean                pango_layout_line_is_paragraph_start
                                                        (PangoLayoutLine       *line);

PANGO_AVAILABLE_IN_ALL
gboolean                pango_layout_line_is_paragraph_end
                                                        (PangoLayoutLine       *line);

PANGO_AVAILABLE_IN_ALL
PangoDirection          pango_layout_line_get_resolved_direction
                                                        (PangoLayoutLine       *line);

PANGO_AVAILABLE_IN_ALL
void                    pango_layout_line_get_extents   (PangoLayoutLine       *line,
                                                         PangoRectangle        *ink_rect,
                                                         PangoRectangle        *logical_rect);

PANGO_AVAILABLE_IN_ALL
void                    pango_layout_line_get_trimmed_extents
                                                        (PangoLayoutLine       *line,
                                                         PangoLeadingTrim       trim,
                                                         PangoRectangle        *logical_rect);

PANGO_AVAILABLE_IN_ALL
void                    pango_layout_line_index_to_pos  (PangoLayoutLine       *line,
                                                         int                    idx,
                                                         PangoRectangle        *pos);

PANGO_AVAILABLE_IN_ALL
void                    pango_layout_line_index_to_x    (PangoLayoutLine       *line,
                                                         int                    idx,
                                                         int                    trailing,
                                                         int                   *x_pos);

PANGO_AVAILABLE_IN_ALL
gboolean                pango_layout_line_x_to_index    (PangoLayoutLine       *line,
                                                         int                    x,
                                                         int                   *idx,
                                                         int                   *trailing);

PANGO_AVAILABLE_IN_ALL
void                    pango_layout_line_get_cursor_pos
                                                        (PangoLayoutLine       *line,
                                                         int                    idx,
                                                         PangoRectangle        *strong_pos,
                                                         PangoRectangle        *weak_pos);

PANGO_AVAILABLE_IN_ALL
void                    pango_layout_line_get_caret_pos (PangoLayoutLine       *line,
                                                         int                    idx,
                                                         PangoRectangle        *strong_pos,
                                                         PangoRectangle        *weak_pos);

G_END_DECLS
