/*
 * Copyright (C) 2000 Red Hat Software
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

#include <glib-object.h>

G_BEGIN_DECLS


#define PANGO2_TYPE_FONT_FACE              (pango2_font_face_get_type ())

PANGO2_AVAILABLE_IN_ALL
PANGO2_DECLARE_INTERNAL_TYPE (Pango2FontFace, pango2_font_face, PANGO2, FONT_FACE, GObject)

PANGO2_AVAILABLE_IN_ALL
Pango2FontDescription * pango2_font_face_describe       (Pango2FontFace  *face);
PANGO2_AVAILABLE_IN_ALL
const char *            pango2_font_face_get_name       (Pango2FontFace  *face) G_GNUC_PURE;
PANGO2_AVAILABLE_IN_ALL
gboolean                pango2_font_face_is_synthesized (Pango2FontFace  *face) G_GNUC_PURE;
PANGO2_AVAILABLE_IN_ALL
gboolean                pango2_font_face_is_monospace   (Pango2FontFace  *face);
PANGO2_AVAILABLE_IN_ALL
gboolean                pango2_font_face_is_variable    (Pango2FontFace  *face);

PANGO2_AVAILABLE_IN_ALL
Pango2FontFamily *      pango2_font_face_get_family     (Pango2FontFace  *face);

PANGO2_AVAILABLE_IN_ALL
gboolean                pango2_font_face_supports_language
                                                        (Pango2FontFace  *face,
                                                         Pango2Language  *language);

PANGO2_AVAILABLE_IN_ALL
Pango2Language **       pango2_font_face_get_languages  (Pango2FontFace  *face);

PANGO2_AVAILABLE_IN_ALL
gboolean                pango2_font_face_has_char       (Pango2FontFace  *face,
                                                         gunichar         wc);


G_END_DECLS
