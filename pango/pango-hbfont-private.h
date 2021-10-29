/* Pango
 *
 * Copyright (C) 2021 Matthias Clasen
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Library General Public
 * License as published by the Free Software Foundation; either
 * version 2 of the License, or (at your option) any later version.
 *
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the GNU
 * Library General Public License for more details.
 *
 * You should have received a copy of the GNU Library General Public
 * License along with this library; if not, write to the
 * Free Software Foundation, Inc., 59 Temple Place - Suite 330,
 * Boston, MA 02111-1307, USA.
 */

#pragma once

#include "pango-hbfont.h"
#include "pango-hbface.h"
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
