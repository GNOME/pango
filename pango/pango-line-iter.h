#pragma once

#include <glib-object.h>

#include <pango/pango-types.h>
#include <pango/pango-lines.h>
#include <pango/pango-glyph-item.h>

G_BEGIN_DECLS

PANGO_AVAILABLE_IN_ALL
GType                   pango_line_iter_get_type           (void) G_GNUC_CONST;

PANGO_AVAILABLE_IN_ALL
PangoLineIter *         pango_line_iter_copy                (PangoLineIter *iter);

PANGO_AVAILABLE_IN_ALL
void                    pango_line_iter_free                (PangoLineIter *iter);

PANGO_AVAILABLE_IN_ALL
PangoLines *            pango_line_iter_get_lines           (PangoLineIter *iter);

PANGO_AVAILABLE_IN_ALL
PangoLine *             pango_line_iter_get_line            (PangoLineIter *iter);

PANGO_AVAILABLE_IN_ALL
gboolean                pango_line_iter_at_last_line        (PangoLineIter *iter);

PANGO_AVAILABLE_IN_ALL
PangoLayoutRun *        pango_line_iter_get_run             (PangoLineIter *iter);

PANGO_AVAILABLE_IN_ALL
int                     pango_line_iter_get_index           (PangoLineIter *iter);

PANGO_AVAILABLE_IN_ALL
gboolean                pango_line_iter_next_line           (PangoLineIter *iter);

PANGO_AVAILABLE_IN_ALL
gboolean                pango_line_iter_next_run            (PangoLineIter *iter);

PANGO_AVAILABLE_IN_ALL
gboolean                pango_line_iter_next_cluster        (PangoLineIter *iter);

PANGO_AVAILABLE_IN_ALL
gboolean                pango_line_iter_next_char           (PangoLineIter *iter);

PANGO_AVAILABLE_IN_ALL
void                    pango_line_iter_get_layout_extents  (PangoLineIter  *iter,
                                                             PangoRectangle *ink_rect,
                                                             PangoRectangle *logical_rect);

PANGO_AVAILABLE_IN_ALL
void                    pango_line_iter_get_line_extents    (PangoLineIter  *iter,
                                                             PangoRectangle *ink_rect,
                                                             PangoRectangle *logical_rect);

PANGO_AVAILABLE_IN_ALL
void                    pango_line_iter_get_trimmed_line_extents
                                                            (PangoLineIter    *iter,
                                                             PangoLeadingTrim  trim,
                                                             PangoRectangle   *logical_rect);

PANGO_AVAILABLE_IN_ALL
void                    pango_line_iter_get_run_extents     (PangoLineIter  *iter,
                                                             PangoRectangle *ink_rect,
                                                             PangoRectangle *logical_rect);

PANGO_AVAILABLE_IN_ALL
void                    pango_line_iter_get_cluster_extents (PangoLineIter  *iter,
                                                             PangoRectangle *ink_rect,
                                                             PangoRectangle *logical_rect);

PANGO_AVAILABLE_IN_ALL
void                    pango_line_iter_get_char_extents    (PangoLineIter  *iter,
                                                             PangoRectangle *logical_rect);

PANGO_AVAILABLE_IN_ALL
int                     pango_line_iter_get_line_baseline   (PangoLineIter  *iter);

PANGO_AVAILABLE_IN_ALL
int                     pango_line_iter_get_run_baseline    (PangoLineIter  *iter);


G_END_DECLS
