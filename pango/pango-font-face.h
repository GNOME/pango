/* Pango
 *
 * Copyright (C) 2000 Red Hat Software
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Library General Public
 * License as published by the Free Software Foundation; either
 * version 2 of the License, or (at your option) any later version.
 *
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Library General Public License for more details.
 *
 * You should have received a copy of the GNU Library General Public
 * License along with this library; if not, write to the
 * Free Software Foundation, Inc., 59 Temple Place - Suite 330,
 * Boston, MA 02111-1307, USA.
 */

#pragma once

#include <pango/pango-types.h>

#include <glib-object.h>

G_BEGIN_DECLS


#define PANGO_TYPE_FONT_FACE              (pango_font_face_get_type ())
#define PANGO_FONT_FACE(object)           (G_TYPE_CHECK_INSTANCE_CAST ((object), PANGO_TYPE_FONT_FACE, PangoFontFace))
#define PANGO_IS_FONT_FACE(object)        (G_TYPE_CHECK_INSTANCE_TYPE ((object), PANGO_TYPE_FONT_FACE))


PANGO_AVAILABLE_IN_ALL
GType                   pango_font_face_get_type       (void) G_GNUC_CONST;

PANGO_AVAILABLE_IN_ALL
PangoFontDescription *  pango_font_face_describe       (PangoFontFace  *face);
PANGO_AVAILABLE_IN_ALL
const char *            pango_font_face_get_face_name  (PangoFontFace  *face) G_GNUC_PURE;
PANGO_AVAILABLE_IN_1_4
void                    pango_font_face_list_sizes     (PangoFontFace  *face,
                                                        int           **sizes,
                                                        int            *n_sizes);
PANGO_AVAILABLE_IN_1_18
gboolean                pango_font_face_is_synthesized (PangoFontFace  *face) G_GNUC_PURE;

PANGO_AVAILABLE_IN_1_46
PangoFontFamily *       pango_font_face_get_family     (PangoFontFace  *face);


G_DEFINE_AUTOPTR_CLEANUP_FUNC(PangoFontFace, g_object_unref)

G_END_DECLS
