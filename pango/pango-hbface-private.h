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

#include "pango-hbface.h"
#include "pango-font-face-private.h"
#include "pango-fontmap.h"
#include "pango-language-set-private.h"
#include <hb.h>

struct _Pango2HbFace
{
  Pango2FontFace parent_instance;

  char *faceid;
  unsigned int index;
  int instance_id;
  char *file;
  hb_face_t *face;
  hb_variation_t *variations;
  unsigned int n_variations;
  Pango2Matrix *transform;
  double x_scale, y_scale;
  Pango2LanguageSet *languages;
  gboolean embolden;
  gboolean synthetic;
};

Pango2LanguageSet *     pango2_hb_face_get_language_set  (Pango2HbFace          *self);

void                    pango2_hb_face_set_language_set  (Pango2HbFace          *self,
                                                          Pango2LanguageSet     *languages);

void                    pango2_hb_face_set_matrix        (Pango2HbFace          *self,
                                                          const Pango2Matrix    *matrix);
