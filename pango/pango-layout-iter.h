#pragma once

#include <glib-object.h>

#include <pango/pango-types.h>
#include <pango/pango-lines.h>
#include <pango/pango-glyph-item.h>

G_BEGIN_DECLS

PANGO_AVAILABLE_IN_ALL
GType                   pango_layout_iter_get_type           (void) G_GNUC_CONST;

PANGO_AVAILABLE_IN_ALL
PangoLayoutIter *       pango_layout_iter_copy                (PangoLayoutIter *iter);

PANGO_AVAILABLE_IN_ALL
void                    pango_layout_iter_free                (PangoLayoutIter *iter);

PANGO_AVAILABLE_IN_ALL
PangoLines *            pango_layout_iter_get_lines           (PangoLayoutIter *iter);

PANGO_AVAILABLE_IN_ALL
PangoLine *             pango_layout_iter_get_line            (PangoLayoutIter *iter);

PANGO_AVAILABLE_IN_ALL
gboolean                pango_layout_iter_at_last_line        (PangoLayoutIter *iter);

PANGO_AVAILABLE_IN_ALL
PangoRun *              pango_layout_iter_get_run             (PangoLayoutIter *iter);

PANGO_AVAILABLE_IN_ALL
int                     pango_layout_iter_get_index           (PangoLayoutIter *iter);

PANGO_AVAILABLE_IN_ALL
gboolean                pango_layout_iter_next_line           (PangoLayoutIter *iter);

PANGO_AVAILABLE_IN_ALL
gboolean                pango_layout_iter_next_run            (PangoLayoutIter *iter);

PANGO_AVAILABLE_IN_ALL
gboolean                pango_layout_iter_next_cluster        (PangoLayoutIter *iter);

PANGO_AVAILABLE_IN_ALL
gboolean                pango_layout_iter_next_char           (PangoLayoutIter *iter);

PANGO_AVAILABLE_IN_ALL
void                    pango_layout_iter_get_layout_extents  (PangoLayoutIter  *iter,
                                                               PangoRectangle   *ink_rect,
                                                               PangoRectangle   *logical_rect);

PANGO_AVAILABLE_IN_ALL
void                    pango_layout_iter_get_line_extents    (PangoLayoutIter  *iter,
                                                               PangoRectangle   *ink_rect,
                                                               PangoRectangle   *logical_rect);

PANGO_AVAILABLE_IN_ALL
void                    pango_layout_iter_get_trimmed_line_extents
                                                              (PangoLayoutIter    *iter,
                                                               PangoLeadingTrim    trim,
                                                               PangoRectangle     *logical_rect);

PANGO_AVAILABLE_IN_ALL
void                    pango_layout_iter_get_run_extents     (PangoLayoutIter  *iter,
                                                               PangoRectangle   *ink_rect,
                                                               PangoRectangle   *logical_rect);

PANGO_AVAILABLE_IN_ALL
void                    pango_layout_iter_get_cluster_extents (PangoLayoutIter  *iter,
                                                               PangoRectangle   *ink_rect,
                                                               PangoRectangle   *logical_rect);

PANGO_AVAILABLE_IN_ALL
void                    pango_layout_iter_get_char_extents    (PangoLayoutIter  *iter,
                                                               PangoRectangle   *logical_rect);

PANGO_AVAILABLE_IN_ALL
int                     pango_layout_iter_get_line_baseline   (PangoLayoutIter  *iter);

PANGO_AVAILABLE_IN_ALL
int                     pango_layout_iter_get_run_baseline    (PangoLayoutIter  *iter);


G_END_DECLS
