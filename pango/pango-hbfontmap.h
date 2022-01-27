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
#include <pango/pango-fontmap.h>
#include <pango/pango-hbface.h>

G_BEGIN_DECLS

#define PANGO_TYPE_HB_FONT_MAP   (pango_hb_font_map_get_type ())

PANGO_AVAILABLE_IN_ALL
PANGO_DECLARE_INTERNAL_TYPE (PangoHbFontMap, pango_hb_font_map, PANGO, HB_FONT_MAP, PangoFontMap)

PANGO_AVAILABLE_IN_ALL
PangoHbFontMap *        pango_hb_font_map_new                   (void);

PANGO_AVAILABLE_IN_ALL
void                    pango_hb_font_map_add_file              (PangoHbFontMap *self,
                                                                 const char     *file);

PANGO_AVAILABLE_IN_ALL
void                    pango_hb_font_map_add_face              (PangoHbFontMap *self,
                                                                 PangoFontFace  *face);

PANGO_AVAILABLE_IN_ALL
void                    pango_hb_font_map_remove_face           (PangoHbFontMap *self,
                                                                 PangoFontFace  *face);

PANGO_AVAILABLE_IN_ALL
void                    pango_hb_font_map_add_family            (PangoHbFontMap  *self,
                                                                 PangoFontFamily *family);
PANGO_AVAILABLE_IN_ALL
void                    pango_hb_font_map_remove_family         (PangoHbFontMap  *self,
                                                                 PangoFontFamily *family);

PANGO_AVAILABLE_IN_ALL
void                    pango_hb_font_map_set_resolution        (PangoHbFontMap *self,
                                                                 double          dpi);

G_END_DECLS
