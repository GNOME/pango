/* Pango
 *
 * Copyright (C) 2021 Matthias Clasen
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
#include <pango/pango-hbface.h>
#include <hb.h>

G_BEGIN_DECLS

#define PANGO_TYPE_HB_FONT      (pango_hb_font_get_type ())

PANGO_AVAILABLE_IN_1_52
G_DECLARE_FINAL_TYPE (PangoHbFont, pango_hb_font, PANGO, HB_FONT, PangoFont)

PANGO_AVAILABLE_IN_1_52
PangoHbFont *           pango_hb_font_new                       (PangoHbFace                    *face,
                                                                 int                             size,
                                                                 hb_feature_t                   *features,
                                                                 unsigned int                    n_features,
                                                                 hb_variation_t                 *variations,
                                                                 unsigned int                    n_variations,
                                                                 PangoGravity                    gravity,
                                                                 float                           dpi,
                                                                 const PangoMatrix              *matrix);

PANGO_AVAILABLE_IN_1_52
PangoHbFont *           pango_hb_font_new_for_description       (PangoHbFace                    *face,
                                                                 const PangoFontDescription     *description,
                                                                 float                           dpi,
                                                                 const PangoMatrix              *matrix);

G_END_DECLS
