/*
 * Copyright 2021 Red Hat, Inc.
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

#include "pango-hbfont.h"
#include "pango-hbface.h"
#include "pango-font-private.h"

#include <hb.h>


typedef struct _HexBoxInfo HexBoxInfo;
struct _HexBoxInfo
{
  PangoFont *font;
  int rows;
  double digit_width;
  double digit_height;
  double pad_x;
  double pad_y;
  double line_width;
  double box_descent;
  double box_height;
};

struct _PangoHbFont
{
  PangoFont parent_instance;

  PangoHbFace *face;
  int size; /* point size, scaled by PANGO_SCALE */
  float dpi;
  hb_feature_t *features;
  unsigned int n_features;
  hb_variation_t *variations;
  unsigned int n_variations;
  PangoGravity gravity;
  PangoMatrix matrix;

  HexBoxInfo *hex_box_info;
};
