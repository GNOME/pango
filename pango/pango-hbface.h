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

#include <pango/pango-types.h>
#include <pango/pango-font.h>

G_BEGIN_DECLS

#define PANGO_TYPE_HB_FACE      (pango_hb_face_get_type ())

PANGO_AVAILABLE_IN_1_52
G_DECLARE_FINAL_TYPE (PangoHbFace, pango_hb_face, PANGO, HB_FACE, PangoFontFace)

PANGO_AVAILABLE_IN_1_52
PangoHbFace *   pango_hb_face_new_from_hb_face  (hb_face_t                  *face,
                                                 int                         instance_id,
                                                 const char                 *name,
                                                 const PangoFontDescription *description);

PANGO_AVAILABLE_IN_1_52
PangoHbFace *   pango_hb_face_new_from_file     (const char                 *file,
                                                 unsigned int                index,
                                                 int                         instance_id,
                                                 const char                 *name,
                                                 const PangoFontDescription *description);

PANGO_AVAILABLE_IN_1_52
PangoHbFace *   pango_hb_face_new_synthetic     (PangoHbFace                *face,
                                                 const PangoMatrix          *transform,
                                                 gboolean                    embolden,
                                                 const char                 *name,
                                                 const PangoFontDescription *description);

PANGO_AVAILABLE_IN_1_52
PangoHbFace *   pango_hb_face_new_instance      (PangoHbFace                *face,
                                                 const hb_variation_t       *variations,
                                                 unsigned int                n_variations,
                                                 const char                 *name,
                                                 const PangoFontDescription *description);

PANGO_AVAILABLE_IN_1_52
hb_face_t *     pango_hb_face_get_hb_face       (PangoHbFace            *self);

PANGO_AVAILABLE_IN_1_52
const char *    pango_hb_face_get_file          (PangoHbFace            *self);

PANGO_AVAILABLE_IN_1_52
unsigned int    pango_hb_face_get_face_index    (PangoHbFace            *self);

PANGO_AVAILABLE_IN_1_52
int             pango_hb_face_get_instance_id   (PangoHbFace            *self);

PANGO_AVAILABLE_IN_1_52
const hb_variation_t *
                pango_hb_face_get_variations   (PangoHbFace            *self,
                                                unsigned int           *n_variations);

PANGO_AVAILABLE_IN_1_52
gboolean        pango_hb_face_get_embolden      (PangoHbFace            *self);

PANGO_AVAILABLE_IN_1_52
const PangoMatrix *
                pango_hb_face_get_transform     (PangoHbFace            *self);

G_END_DECLS
