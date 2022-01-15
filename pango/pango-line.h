#pragma once

#include <glib-object.h>

#include <pango/pango-types.h>
#include <pango/pango-layout-run.h>

G_BEGIN_DECLS

typedef PangoGlyphItem PangoLayoutRun;

#define PANGO_TYPE_LINE pango_line_get_type ()

PANGO_AVAILABLE_IN_ALL
G_DECLARE_FINAL_TYPE (PangoLine, pango_line, PANGO, LINE, GObject);

PANGO_AVAILABLE_IN_ALL
PangoLine *             pango_line_justify              (PangoLine             *line,
                                                         int                    width);

PANGO_AVAILABLE_IN_ALL
GSList *                pango_line_get_runs             (PangoLine             *line);

PANGO_AVAILABLE_IN_ALL
const char *            pango_line_get_text             (PangoLine             *line,
                                                         int                   *start_index,
                                                         int                   *length);

PANGO_AVAILABLE_IN_ALL
int                     pango_line_get_start_index      (PangoLine             *line);

PANGO_AVAILABLE_IN_ALL
int                     pango_line_get_length           (PangoLine             *line);

PANGO_AVAILABLE_IN_ALL
const PangoLogAttr *    pango_line_get_log_attrs        (PangoLine             *line,
                                                         int                   *start_offset,
                                                         int                   *n_attrs);

PANGO_AVAILABLE_IN_ALL
gboolean                pango_line_wrapped              (PangoLine             *line);

PANGO_AVAILABLE_IN_ALL
gboolean                pango_line_ellipsized           (PangoLine             *line);

PANGO_AVAILABLE_IN_ALL
gboolean                pango_line_hyphenated           (PangoLine             *line);

PANGO_AVAILABLE_IN_ALL
gboolean                pango_line_justified            (PangoLine             *line);

PANGO_AVAILABLE_IN_ALL
gboolean                pango_line_starts_paragraph     (PangoLine             *line);

PANGO_AVAILABLE_IN_ALL
gboolean                pango_line_ends_paragraph       (PangoLine             *line);

PANGO_AVAILABLE_IN_ALL
PangoDirection          pango_line_get_resolved_direction
                                                        (PangoLine             *line);

PANGO_AVAILABLE_IN_ALL
void                    pango_line_get_extents          (PangoLine             *line,
                                                         PangoRectangle        *ink_rect,
                                                         PangoRectangle        *logical_rect);

PANGO_AVAILABLE_IN_ALL
void                    pango_line_get_trimmed_extents  (PangoLine             *line,
                                                         PangoLeadingTrim       trim,
                                                         PangoRectangle        *logical_rect);

PANGO_AVAILABLE_IN_ALL
void                    pango_line_index_to_pos         (PangoLine             *line,
                                                         int                    idx,
                                                         PangoRectangle        *pos);

PANGO_AVAILABLE_IN_ALL
void                    pango_line_index_to_x           (PangoLine             *line,
                                                         int                    idx,
                                                         int                    trailing,
                                                         int                   *x_pos);

PANGO_AVAILABLE_IN_ALL
gboolean                pango_line_x_to_index           (PangoLine             *line,
                                                         int                    x,
                                                         int                   *idx,
                                                         int                   *trailing);

PANGO_AVAILABLE_IN_ALL
void                    pango_line_get_cursor_pos       (PangoLine             *line,
                                                         int                    idx,
                                                         PangoRectangle        *strong_pos,
                                                         PangoRectangle        *weak_pos);

PANGO_AVAILABLE_IN_ALL
void                    pango_line_get_caret_pos        (PangoLine             *line,
                                                         int                    idx,
                                                         PangoRectangle        *strong_pos,
                                                         PangoRectangle        *weak_pos);

G_END_DECLS
