/*
 * Copyright 2022 Red Hat, Inc.
 *
 * SPDX-License-Identifier: LGPL-2.1-or-later
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public
 * License as published by the Free Software Foundation; either
 * version 2.1 of the License, or (at your option) any later version.
 *
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public
 * License along with this library; if not, see <http://www.gnu.org/licenses/>.
 */

#pragma once

#include "pango-line.h"
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

struct _PangoLine
{
  PangoContext *context;
  LineData *data;

  int start_index;
  int length;
  int start_offset;
  int n_chars;
  GSList *runs;
  PangoRun **run_array;
  int n_runs;

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

PangoLine * pango_line_new               (PangoContext       *context,
                                          LineData           *data);

void        pango_line_ellipsize         (PangoLine          *line,
                                          PangoContext       *context,
                                          PangoEllipsizeMode  ellipsize,
                                          int                 goal_width);

void        pango_line_index_to_run      (PangoLine          *line,
                                          int                 idx,
                                          PangoRun          **run);

void        pango_line_get_empty_extents (PangoLine          *line,
                                          PangoLeadingTrim    trim,
                                          PangoRectangle     *logical_rect);

void        pango_line_check_invariants  (PangoLine          *line);
