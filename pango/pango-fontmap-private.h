/*
 * Copyright 2000 Red Hat Software
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

#include <pango/pango-font-private.h>
#include <pango/pango-fontset.h>
#include <pango/pango-fontmap.h>


#define PANGO_FONT_MAP_CLASS(klass)      (G_TYPE_CHECK_CLASS_CAST ((klass), PANGO_TYPE_FONT_MAP, PangoFontMapClass))
#define PANGO_FONT_MAP_GET_CLASS(obj)    (G_TYPE_INSTANCE_GET_CLASS ((obj), PANGO_TYPE_FONT_MAP, PangoFontMapClass))

typedef struct _PangoFontMapClass PangoFontMapClass;

struct _PangoFontMap
{
  GObject parent_instance;
};

struct _PangoFontMapClass
{
  GObjectClass parent_class;

  PangoFont *   (*load_font)     (PangoFontMap               *fontmap,
                                  PangoContext               *context,
                                  const PangoFontDescription *desc);
  PangoFontset *(*load_fontset)  (PangoFontMap               *fontmap,
                                  PangoContext               *context,
                                  const PangoFontDescription *desc,
                                  PangoLanguage              *language);

  guint         (*get_serial)    (PangoFontMap               *fontmap);
  void          (*changed)       (PangoFontMap               *fontmap);

  PangoFontFamily * (*get_family) (PangoFontMap               *fontmap,
                                   const char                 *name);

  PangoFontFace *   (*get_face)   (PangoFontMap               *fontmap,
                                   PangoFont                  *font);
};
