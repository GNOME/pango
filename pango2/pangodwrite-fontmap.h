/* Pango2
 * pangodwrite-fontmap.h: Fontmap using DirectWrite
 *
 * Copyright (C) 2022 the GTK team
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Library General Public
 * License as published by the Free Software Foundation; either
 * version 2 of the License, or (at your option) any later version.
 *
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.	 See the GNU
 * Library General Public License for more details.
 *
 * You should have received a copy of the GNU Library General Public
 * License along with this library; if not, write to the
 * Free Software Foundation, Inc., 59 Temple Place - Suite 330,
 * Boston, MA 02111-1307, USA.
 */

#pragma once

#define WIN32_LEAN_AND_MEAN 1

#include <windows.h>
#include <pango2/pango.h>

G_BEGIN_DECLS

#define PANGO2_TYPE_DIRECT_WRITE_FONT_MAP (pango2_direct_write_font_map_get_type ())

PANGO2_AVAILABLE_IN_ALL
PANGO2_DECLARE_INTERNAL_TYPE (Pango2DirectWriteFontMap,
                              pango2_direct_write_font_map,
                              PANGO2, DIRECT_WRITE_FONT_MAP,
                              Pango2FontMap)

PANGO2_AVAILABLE_IN_ALL
Pango2DirectWriteFontMap *pango2_direct_write_font_map_new                          (void);

PANGO2_AVAILABLE_IN_ALL
Pango2FontDescription    *pango2_direct_write_get_font_description_from_dwrite_font (gpointer       dwrite_font);

PANGO2_AVAILABLE_IN_ALL
Pango2FontDescription    *pango2_direct_write_get_font_description_from_logfontw    (LOGFONTW      *lfw,
                                                                                     Pango2FontMap *font_map);

G_END_DECLS
