/* Pango
 *
 * Copyright (C) 2022 Matthias Clasen
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Library General Public
 * License as published by the Free Software Foundation; either
 * version 2 of the License, or (at your option) any later version.
 *
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the GNU
 * Library General Public License for more details.
 *
 * You should have received a copy of the GNU Library General Public
 * License along with this library; if not, write to the
 * Free Software Foundation, Inc., 59 Temple Place - Suite 330,
 * Boston, MA 02111-1307, USA.
 */

#pragma once

#include <pango/pango-types.h>
#include <pango/pango-userface.h>
#include <hb.h>

G_BEGIN_DECLS

#define PANGO_TYPE_USER_FONT      (pango_user_font_get_type ())

PANGO_AVAILABLE_IN_1_52
G_DECLARE_FINAL_TYPE (PangoUserFont, pango_user_font, PANGO, USER_FONT, PangoFont)

PANGO_AVAILABLE_IN_1_52
PangoUserFont * pango_user_font_new                 (PangoUserFace              *face,
                                                     int                         size,
                                                     PangoGravity                gravity,
                                                     float                       dpi,
                                                     const PangoMatrix          *matrix);

PANGO_AVAILABLE_IN_1_52
PangoUserFont * pango_user_font_new_for_description (PangoUserFace              *face,
                                                     const PangoFontDescription *description,
                                                     float                       dpi,
                                                     const PangoMatrix          *matrix);

G_END_DECLS
