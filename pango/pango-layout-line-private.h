#pragma once

#include "pango-layout-line.h"
#include "pango-break.h"
#include "pango-attributes.h"
#include "pango-glyph-item.h"

typedef struct _LineData LineData;
struct _LineData {
  char *text;
  int length;
  int n_chars;
  PangoDirection direction;

  PangoAttrList *attrs;
  PangoLogAttr *log_attrs;
};

LineData *      line_data_new           (void);
LineData *      line_data_ref           (LineData *data);
void            line_data_unref         (LineData *data);
void            line_data_clear         (LineData *data);

struct _PangoLayoutLine
{
  GObject parent_instance;

  PangoContext *context;
  LineData *data;

  int start_index;
  int length;
  int start_offset;
  int n_chars;
  GSList *runs;

  guint wrapped             : 1;
  guint ellipsized          : 1;
  guint hyphenated          : 1;
  guint justified           : 1;
  guint starts_paragraph    : 1;
  guint ends_paragraph      : 1;
  guint has_extents         : 1;

  PangoDirection direction;

  PangoRectangle ink_rect;
  PangoRectangle logical_rect;
};

PangoLayoutLine * pango_layout_line_new       (PangoContext       *context,
                                               LineData           *data);

void              pango_layout_line_ellipsize (PangoLayoutLine    *line,
                                               PangoContext       *context,
                                               PangoEllipsizeMode  ellipsize,
                                               int                 goal_width);

void              pango_layout_line_index_to_run (PangoLayoutLine  *line,
                                                  int              idx,
                                                  PangoLayoutRun **run);

void              pango_layout_line_get_empty_extents (PangoLayoutLine  *line,
                                                       PangoLeadingTrim  trim,
                                                       PangoRectangle   *logical_rect);

void              pango_layout_line_check_invariants (PangoLayoutLine *line);
