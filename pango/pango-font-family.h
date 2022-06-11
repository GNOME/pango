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
#include <gio/gio.h>

G_BEGIN_DECLS


#define PANGO_TYPE_FONT_FAMILY              (pango_font_family_get_type ())

PANGO_AVAILABLE_IN_ALL
PANGO_DECLARE_INTERNAL_TYPE (PangoFontFamily, pango_font_family, PANGO, FONT_FAMILY, GObject)

PANGO_AVAILABLE_IN_ALL
PangoFontMap *          pango_font_family_get_font_map  (PangoFontFamily  *family);

PANGO_AVAILABLE_IN_ALL
const char *            pango_font_family_get_name      (PangoFontFamily  *family) G_GNUC_PURE;

PANGO_AVAILABLE_IN_ALL
PangoFontFace *         pango_font_family_get_face      (PangoFontFamily  *family,
                                                         const char       *name);

G_END_DECLS
