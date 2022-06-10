/*
 * Copyright (C) 2021 Red Hat, Inc
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

#include <pango/pango.h>
#include <fontconfig/fontconfig.h>

G_BEGIN_DECLS

#define PANGO_TYPE_FC_HB_FONT_MAP      (pango_fc_hb_font_map_get_type ())

PANGO_AVAILABLE_IN_ALL
PANGO_DECLARE_INTERNAL_TYPE (PangoFcHbFontMap, pango_fc_hb_font_map, PANGO, FC_HB_FONT_MAP, PangoHbFontMap)

PANGO_AVAILABLE_IN_ALL
PangoFcHbFontMap *      pango_fc_hb_font_map_new         (void);

PANGO_AVAILABLE_IN_ALL
void                    pango_fc_hb_font_map_set_config  (PangoFcHbFontMap *self,
                                                          FcConfig         *config);

PANGO_AVAILABLE_IN_ALL
FcConfig *              pango_fc_hb_font_map_get_config  (PangoFcHbFontMap *self);

G_END_DECLS
