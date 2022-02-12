/* Pango
 * pango-font-family.h: Font handling
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
#include <gio/gio.h>

G_BEGIN_DECLS


#define PANGO_TYPE_FONT_FAMILY              (pango_font_family_get_type ())
#define PANGO_FONT_FAMILY(object)           (G_TYPE_CHECK_INSTANCE_CAST ((object), PANGO_TYPE_FONT_FAMILY, PangoFontFamily))
#define PANGO_IS_FONT_FAMILY(object)        (G_TYPE_CHECK_INSTANCE_TYPE ((object), PANGO_TYPE_FONT_FAMILY))

PANGO_AVAILABLE_IN_ALL
GType                   pango_font_family_get_type      (void) G_GNUC_CONST;

PANGO_AVAILABLE_IN_ALL
void                    pango_font_family_list_faces    (PangoFontFamily  *family,
                                                         PangoFontFace  ***faces,
                                                         int              *n_faces);
PANGO_AVAILABLE_IN_ALL
const char *            pango_font_family_get_name      (PangoFontFamily  *family) G_GNUC_PURE;
PANGO_AVAILABLE_IN_1_4
gboolean                pango_font_family_is_monospace  (PangoFontFamily  *family) G_GNUC_PURE;
PANGO_AVAILABLE_IN_1_44
gboolean                pango_font_family_is_variable   (PangoFontFamily  *family) G_GNUC_PURE;

PANGO_AVAILABLE_IN_1_46
PangoFontFace *         pango_font_family_get_face      (PangoFontFamily  *family,
                                                         const char       *name);


G_DEFINE_AUTOPTR_CLEANUP_FUNC(PangoFontFamily, g_object_unref)

G_END_DECLS
