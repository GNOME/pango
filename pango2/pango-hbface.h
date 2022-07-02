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

#include <pango2/pango-types.h>
#include <pango2/pango-font-face.h>

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


typedef struct _Pango2HbFaceBuilder Pango2HbFaceBuilder;

#define PANGO2_TYPE_HB_FACE_BUILDER (pango2_hb_face_builder_get_type ())

PANGO2_AVAILABLE_IN_ALL
GType                   pango2_hb_face_builder_get_type         (void) G_GNUC_CONST;

PANGO2_AVAILABLE_IN_ALL
Pango2HbFaceBuilder *   pango2_hb_face_builder_copy             (const Pango2HbFaceBuilder   *src);

PANGO2_AVAILABLE_IN_ALL
void                    pango2_hb_face_builder_free             (Pango2HbFaceBuilder         *builder);

PANGO2_AVAILABLE_IN_ALL
Pango2HbFaceBuilder *   pango2_hb_face_builder_new              (Pango2HbFace                *face);

PANGO2_AVAILABLE_IN_ALL
Pango2HbFaceBuilder *   pango2_hb_face_builder_new_for_hb_face  (hb_face_t                   *hb_face);

PANGO2_AVAILABLE_IN_ALL
Pango2HbFace *          pango2_hb_face_builder_get_face         (Pango2HbFaceBuilder         *builder);

PANGO2_AVAILABLE_IN_ALL
void                    pango2_hb_face_builder_set_instance_id  (Pango2HbFaceBuilder         *builder,
                                                                 int                          instance_id);
PANGO2_AVAILABLE_IN_ALL
void                    pango2_hb_face_builder_set_name         (Pango2HbFaceBuilder         *builder,
                                                                 const char                  *name);
PANGO2_AVAILABLE_IN_ALL
void                    pango2_hb_face_builder_set_description  (Pango2HbFaceBuilder         *builder,
                                                                 const Pango2FontDescription *desc);
PANGO2_AVAILABLE_IN_ALL
void                    pango2_hb_face_builder_set_transform    (Pango2HbFaceBuilder         *builder,
                                                                 const Pango2Matrix          *transform);
PANGO2_AVAILABLE_IN_ALL
void                    pango2_hb_face_builder_set_embolden     (Pango2HbFaceBuilder         *builder,
                                                                 gboolean                     embolden);
PANGO2_AVAILABLE_IN_ALL
void                    pango2_hb_face_builder_set_variations   (Pango2HbFaceBuilder         *builder,
                                                                 const hb_variation_t        *variations,
                                                                 unsigned int                 n_variations);

G_END_DECLS
