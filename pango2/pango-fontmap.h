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
#include <pango2/pango-fontset.h>
#include <pango2/pango-hbface.h>

G_BEGIN_DECLS

#define PANGO2_TYPE_FONT_MAP   (pango2_font_map_get_type ())

PANGO2_AVAILABLE_IN_ALL
PANGO2_DECLARE_INTERNAL_TYPE (Pango2FontMap, pango2_font_map, PANGO2, FONT_MAP, GObject)

PANGO2_AVAILABLE_IN_ALL
Pango2Font *             pango2_font_map_load_font                (Pango2FontMap               *self,
                                                                   Pango2Context               *context,
                                                                   const Pango2FontDescription *desc);
PANGO2_AVAILABLE_IN_ALL
Pango2Fontset *          pango2_font_map_load_fontset             (Pango2FontMap               *self,
                                                                   Pango2Context               *context,
                                                                   const Pango2FontDescription *desc,
                                                                   Pango2Language              *language);
PANGO2_AVAILABLE_IN_ALL
guint                    pango2_font_map_get_serial               (Pango2FontMap               *self);

PANGO2_AVAILABLE_IN_ALL
Pango2FontFamily *       pango2_font_map_get_family               (Pango2FontMap               *self,
                                                                   const char                  *name);

PANGO2_AVAILABLE_IN_ALL
Pango2FontMap *          pango2_font_map_new                      (void);

PANGO2_AVAILABLE_IN_ALL
void                     pango2_font_map_add_file                 (Pango2FontMap               *self,
                                                                   const char                  *file);

PANGO2_AVAILABLE_IN_ALL
void                     pango2_font_map_add_face                 (Pango2FontMap               *self,
                                                                   Pango2FontFace              *face);

PANGO2_AVAILABLE_IN_ALL
void                     pango2_font_map_remove_face              (Pango2FontMap               *self,
                                                                   Pango2FontFace              *face);

PANGO2_AVAILABLE_IN_ALL
void                     pango2_font_map_add_family               (Pango2FontMap               *self,
                                                                   Pango2FontFamily            *family);

PANGO2_AVAILABLE_IN_ALL
void                     pango2_font_map_remove_family            (Pango2FontMap               *self,
                                                                   Pango2FontFamily            *family);

PANGO2_AVAILABLE_IN_ALL
void                     pango2_font_map_set_fallback             (Pango2FontMap               *self,
                                                                   Pango2FontMap               *fallback);

PANGO2_AVAILABLE_IN_ALL
Pango2FontMap *          pango2_font_map_get_fallback             (Pango2FontMap               *self);

PANGO2_AVAILABLE_IN_ALL
float                    pango2_font_map_get_resolution           (Pango2FontMap               *self);

PANGO2_AVAILABLE_IN_ALL
void                     pango2_font_map_set_resolution           (Pango2FontMap               *self,
                                                                   float                        dpi);

PANGO2_AVAILABLE_IN_ALL
Pango2FontMap *          pango2_font_map_new_default              (void);
PANGO2_AVAILABLE_IN_ALL
Pango2FontMap *          pango2_font_map_get_default              (void);
PANGO2_AVAILABLE_IN_ALL
void                     pango2_font_map_set_default              (Pango2FontMap               *fontmap);

G_END_DECLS
