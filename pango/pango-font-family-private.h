/*
 * Copyright 2022 Red Hat, Inc.
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

#include <pango/pango-font-family.h>


typedef struct _PangoFontFamilyClass PangoFontFamilyClass;

struct _PangoFontFamily
{
  GObject parent_instance;
};

struct _PangoFontFamilyClass
{
  GObjectClass parent_class;

  const char *    (* get_name)     (PangoFontFamily *family);
  gboolean        (* is_generic)   (PangoFontFamily *family);
  gboolean        (* is_monospace) (PangoFontFamily *family);
  gboolean        (* is_variable)  (PangoFontFamily *family);

  PangoFontFace * (* get_face)     (PangoFontFamily *family,
                                    const char      *name);

};

#define PANGO_FONT_FAMILY_CLASS(klass)      (G_TYPE_CHECK_CLASS_CAST ((klass), PANGO_TYPE_FONT_FAMILY, PangoFontFamilyClass))
#define PANGO_FONT_FAMILY_GET_CLASS(obj)    (G_TYPE_INSTANCE_GET_CLASS ((obj), PANGO_TYPE_FONT_FAMILY, PangoFontFamilyClass))
