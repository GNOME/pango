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

#include <pango/pango-types.h>
#include <pango/pango-userface.h>
#include <hb.h>

G_BEGIN_DECLS

#define PANGO_TYPE_USER_FONT      (pango_user_font_get_type ())

PANGO_AVAILABLE_IN_ALL
PANGO_DECLARE_INTERNAL_TYPE (PangoUserFont, pango_user_font, PANGO, USER_FONT, PangoFont)

PANGO_AVAILABLE_IN_ALL
PangoUserFont * pango_user_font_new                 (PangoUserFace              *face,
                                                     int                         size,
                                                     PangoGravity                gravity,
                                                     float                       dpi,
                                                     const PangoMatrix          *matrix);

PANGO_AVAILABLE_IN_ALL
PangoUserFont * pango_user_font_new_for_description (PangoUserFace              *face,
                                                     const PangoFontDescription *description,
                                                     float                       dpi,
                                                     const PangoMatrix          *matrix);

G_END_DECLS
