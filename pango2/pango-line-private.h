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
#include "pango-glyph-item-private.h"


typedef struct _LineData LineData;
struct _LineData {
  char *text;
  int length;
  int n_chars;
  Pango2Direction direction;

  Pango2AttrList *attrs;
  Pango2LogAttr *log_attrs;
};

LineData *      line_data_new           (void);
LineData *      line_data_ref           (LineData *data);
void            line_data_unref         (LineData *data);
void            line_data_clear         (LineData *data);

struct _Pango2Line
{
  Pango2Context *context;
  LineData *data;

  int start_index;
  int length;
  int start_offset;
  int n_chars;
  GSList *runs;
  Pango2Run **run_array;
  int n_runs;

  guint wrapped             : 1;
  guint ellipsized          : 1;
  guint hyphenated          : 1;
  guint justified           : 1;
  guint starts_paragraph    : 1;
  guint ends_paragraph      : 1;
  guint has_extents         : 1;

  Pango2Direction direction;

  Pango2Rectangle ink_rect;
  Pango2Rectangle logical_rect;
};

Pango2Line * pango2_line_new               (Pango2Context       *context,
                                            LineData            *data);

void         pango2_line_ellipsize         (Pango2Line          *line,
                                            Pango2Context       *context,
                                            Pango2EllipsizeMode  ellipsize,
                                            int                  goal_width);

void         pango2_line_index_to_run      (Pango2Line          *line,
                                            int                  idx,
                                            Pango2Run          **run);

void         pango2_line_get_empty_extents (Pango2Line          *line,
                                            Pango2LeadingTrim    trim,
                                            Pango2Rectangle     *logical_rect);

void         pango2_line_check_invariants  (Pango2Line          *line);
