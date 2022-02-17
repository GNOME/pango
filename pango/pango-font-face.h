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

#include <pango/pango-types.h>

#include <glib-object.h>

G_BEGIN_DECLS


#define PANGO_TYPE_FONT_FACE              (pango_font_face_get_type ())

PANGO_AVAILABLE_IN_ALL
PANGO_DECLARE_INTERNAL_TYPE (PangoFontFace, pango_font_face, PANGO, FONT_FACE, GObject)

PANGO_AVAILABLE_IN_ALL
PangoFontDescription *  pango_font_face_describe       (PangoFontFace  *face);
PANGO_AVAILABLE_IN_ALL
const char *            pango_font_face_get_face_name  (PangoFontFace  *face) G_GNUC_PURE;
PANGO_AVAILABLE_IN_1_18
gboolean                pango_font_face_is_synthesized (PangoFontFace  *face) G_GNUC_PURE;
PANGO_AVAILABLE_IN_ALL
gboolean                pango_font_face_is_monospace   (PangoFontFace  *face);
PANGO_AVAILABLE_IN_ALL
gboolean                pango_font_face_is_variable    (PangoFontFace  *face);

PANGO_AVAILABLE_IN_1_46
PangoFontFamily *       pango_font_face_get_family     (PangoFontFace  *face);

PANGO_AVAILABLE_IN_ALL
gboolean              pango_font_face_supports_language (PangoFontFace *face,
                                                         PangoLanguage *language);

PANGO_AVAILABLE_IN_ALL
PangoLanguage **      pango_font_face_get_languages     (PangoFontFace *face);


G_END_DECLS
