#pragma once

#include <glib-object.h>

#include <pango/pango-types.h>
#include <pango/pango-run.h>

G_BEGIN_DECLS

PANGO_AVAILABLE_IN_ALL
GType                   pango_line_get_type      (void) G_GNUC_CONST;

PANGO_AVAILABLE_IN_ALL
PangoLine *             pango_line_copy          (PangoLine       *line);

PANGO_AVAILABLE_IN_ALL
void                    pango_line_free          (PangoLine       *line);

PANGO_AVAILABLE_IN_ALL
PangoLine *             pango_line_justify       (PangoLine       *line,
                                                  int                    width);

PANGO_AVAILABLE_IN_ALL
int                     pango_line_get_run_count (PangoLine       *line);

PANGO_AVAILABLE_IN_ALL
PangoRun **             pango_line_get_runs      (PangoLine       *line);

PANGO_AVAILABLE_IN_ALL
const char *            pango_line_get_text      (PangoLine       *line,
                                                  int             *start_index,
                                                  int             *length);

PANGO_AVAILABLE_IN_ALL
int                     pango_line_get_start_index (PangoLine     *line);

PANGO_AVAILABLE_IN_ALL
int                     pango_line_get_length      (PangoLine     *line);

PANGO_AVAILABLE_IN_ALL
const PangoLogAttr *    pango_line_get_log_attrs (PangoLine       *line,
                                                  int             *start_offset,
                                                  int             *n_attrs);

PANGO_AVAILABLE_IN_ALL
gboolean                pango_line_is_wrapped    (PangoLine       *line);

PANGO_AVAILABLE_IN_ALL
gboolean                pango_line_is_ellipsized (PangoLine       *line);

PANGO_AVAILABLE_IN_ALL
gboolean                pango_line_is_hyphenated (PangoLine       *line);

PANGO_AVAILABLE_IN_ALL
gboolean                pango_line_is_justified  (PangoLine       *line);

PANGO_AVAILABLE_IN_ALL
gboolean                pango_line_is_paragraph_start
                                                 (PangoLine       *line);

PANGO_AVAILABLE_IN_ALL
gboolean                pango_line_is_paragraph_end
                                                 (PangoLine       *line);

PANGO_AVAILABLE_IN_ALL
PangoDirection          pango_line_get_resolved_direction
                                                 (PangoLine       *line);

PANGO_AVAILABLE_IN_ALL
void                    pango_line_get_extents   (PangoLine       *line,
                                                  PangoRectangle  *ink_rect,
                                                  PangoRectangle  *logical_rect);

PANGO_AVAILABLE_IN_ALL
void                    pango_line_get_trimmed_extents
                                                 (PangoLine        *line,
                                                  PangoLeadingTrim  trim,
                                                  PangoRectangle   *logical_rect);

PANGO_AVAILABLE_IN_ALL
void                    pango_line_index_to_pos  (PangoLine       *line,
                                                  int              idx,
                                                  PangoRectangle  *pos);

PANGO_AVAILABLE_IN_ALL
void                    pango_line_index_to_x    (PangoLine       *line,
                                                  int              idx,
                                                  int              trailing,
                                                  int             *x_pos);

PANGO_AVAILABLE_IN_ALL
gboolean                pango_line_x_to_index    (PangoLine       *line,
                                                  int              x,
                                                  int             *idx,
                                                  int             *trailing);

PANGO_AVAILABLE_IN_ALL
void                    pango_line_get_cursor_pos (PangoLine       *line,
                                                   int              idx,
                                                   PangoRectangle  *strong_pos,
                                                   PangoRectangle  *weak_pos);

PANGO_AVAILABLE_IN_ALL
void                    pango_line_get_caret_pos (PangoLine       *line,
                                                  int              idx,
                                                  PangoRectangle  *strong_pos,
                                                  PangoRectangle  *weak_pos);

G_END_DECLS
