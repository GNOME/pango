/*
 * Copyright (C) 2021 Matthias Clasen
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

#include <pango/pango-types.h>
#include <pango/pango-font-face.h>

#include <hb.h>

G_BEGIN_DECLS

#define PANGO2_TYPE_HB_FACE      (pango2_hb_face_get_type ())

PANGO2_AVAILABLE_IN_ALL
PANGO2_DECLARE_INTERNAL_TYPE (Pango2HbFace, pango2_hb_face, PANGO2, HB_FACE, Pango2FontFace)

PANGO2_AVAILABLE_IN_ALL
Pango2HbFace *          pango2_hb_face_new_from_hb_face  (hb_face_t                   *face,
                                                          int                          instance_id,
                                                          const char                  *name,
                                                          const Pango2FontDescription *description);

PANGO2_AVAILABLE_IN_ALL
Pango2HbFace *          pango2_hb_face_new_from_file     (const char                  *file,
                                                          unsigned int                 index,
                                                          int                          instance_id,
                                                          const char                  *name,
                                                          const Pango2FontDescription *description);

PANGO2_AVAILABLE_IN_ALL
Pango2HbFace *          pango2_hb_face_new_synthetic     (Pango2HbFace                *face,
                                                          const Pango2Matrix          *transform,
                                                          gboolean                     embolden,
                                                          const char                  *name,
                                                          const Pango2FontDescription *description);

PANGO2_AVAILABLE_IN_ALL
Pango2HbFace *          pango2_hb_face_new_instance      (Pango2HbFace                *face,
                                                          const hb_variation_t        *variations,
                                                          unsigned int                 n_variations,
                                                          const char                  *name,
                                                          const Pango2FontDescription *description);

PANGO2_AVAILABLE_IN_ALL
hb_face_t *             pango2_hb_face_get_hb_face       (Pango2HbFace                *self);

PANGO2_AVAILABLE_IN_ALL
const char *            pango2_hb_face_get_file          (Pango2HbFace                *self);

PANGO2_AVAILABLE_IN_ALL
unsigned int            pango2_hb_face_get_face_index    (Pango2HbFace                *self);

PANGO2_AVAILABLE_IN_ALL
int                     pango2_hb_face_get_instance_id   (Pango2HbFace                *self);

PANGO2_AVAILABLE_IN_ALL
const hb_variation_t * pango2_hb_face_get_variations     (Pango2HbFace                *self,
                                                          unsigned int                *n_variations);

PANGO2_AVAILABLE_IN_ALL
gboolean                pango2_hb_face_get_embolden      (Pango2HbFace                *self);

PANGO2_AVAILABLE_IN_ALL
const Pango2Matrix *    pango2_hb_face_get_transform     (Pango2HbFace                *self);

G_END_DECLS
