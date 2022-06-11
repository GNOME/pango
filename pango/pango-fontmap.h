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
#include <pango/pango-fontset.h>
#include <pango/pango-hbface.h>

G_BEGIN_DECLS

#define PANGO_TYPE_FONT_MAP   (pango_font_map_get_type ())

PANGO_AVAILABLE_IN_ALL
PANGO_DECLARE_INTERNAL_TYPE (PangoFontMap, pango_font_map, PANGO, FONT_MAP, GObject)

PANGO_AVAILABLE_IN_ALL
PangoContext *          pango_font_map_create_context           (PangoFontMap               *self);
PANGO_AVAILABLE_IN_ALL
PangoFont *             pango_font_map_load_font                (PangoFontMap               *self,
                                                                 PangoContext               *context,
                                                                 const PangoFontDescription *desc);
PANGO_AVAILABLE_IN_ALL
PangoFontset *          pango_font_map_load_fontset             (PangoFontMap               *self,
                                                                 PangoContext               *context,
                                                                 const PangoFontDescription *desc,
                                                                 PangoLanguage              *language);
PANGO_AVAILABLE_IN_ALL
guint                   pango_font_map_get_serial               (PangoFontMap               *self);

PANGO_AVAILABLE_IN_ALL
PangoFontFamily *       pango_font_map_get_family               (PangoFontMap               *self,
                                                                 const char                 *name);

PANGO_AVAILABLE_IN_ALL
PangoFontMap *          pango_font_map_new                      (void);

PANGO_AVAILABLE_IN_ALL
void                    pango_font_map_add_file                 (PangoFontMap               *self,
                                                                 const char                 *file);

PANGO_AVAILABLE_IN_ALL
void                    pango_font_map_add_face                 (PangoFontMap               *self,
                                                                 PangoFontFace              *face);

PANGO_AVAILABLE_IN_ALL
void                    pango_font_map_remove_face              (PangoFontMap               *self,
                                                                 PangoFontFace              *face);

PANGO_AVAILABLE_IN_ALL
void                    pango_font_map_add_family               (PangoFontMap               *self,
                                                                 PangoFontFamily            *family);
PANGO_AVAILABLE_IN_ALL
void                    pango_font_map_remove_family            (PangoFontMap               *self,
                                                                 PangoFontFamily            *family);

PANGO_AVAILABLE_IN_ALL
float                   pango_font_map_get_resolution           (PangoFontMap               *self);

PANGO_AVAILABLE_IN_ALL
void                    pango_font_map_set_resolution           (PangoFontMap               *self,
                                                                 float                       dpi);

PANGO_AVAILABLE_IN_ALL
PangoFontMap *          pango_font_map_new_default              (void);
PANGO_AVAILABLE_IN_ALL
PangoFontMap *          pango_font_map_get_default              (void);
PANGO_AVAILABLE_IN_ALL
void                    pango_font_map_set_default              (PangoFontMap *fontmap);

G_END_DECLS
