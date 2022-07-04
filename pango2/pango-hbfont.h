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
#include <pango2/pango-font.h>
#include <pango2/pango-hbface.h>
#include <hb.h>

G_BEGIN_DECLS

#define PANGO2_TYPE_HB_FONT      (pango2_hb_font_get_type ())

PANGO2_AVAILABLE_IN_ALL
PANGO2_DECLARE_INTERNAL_TYPE (Pango2HbFont, pango2_hb_font, PANGO2, HB_FONT, Pango2Font)

PANGO2_AVAILABLE_IN_ALL
Pango2HbFont *          pango2_hb_font_new                      (Pango2HbFace                    *face,
                                                                 int                              size,
                                                                 hb_feature_t                    *features,
                                                                 unsigned int                     n_features,
                                                                 hb_variation_t                  *variations,
                                                                 unsigned int                     n_variations,
                                                                 Pango2Gravity                    gravity,
                                                                 float                            dpi,
                                                                 const Pango2Matrix              *ctm);

PANGO2_AVAILABLE_IN_ALL
Pango2HbFont *          pango2_hb_font_new_for_description      (Pango2HbFace                    *face,
                                                                 const Pango2FontDescription     *description,
                                                                 float                            dpi,
                                                                 const Pango2Matrix              *ctm);

PANGO2_AVAILABLE_IN_ALL
const hb_feature_t *    pango2_hb_font_get_features             (Pango2HbFont                    *self,
                                                                 unsigned int                    *n_features);

PANGO2_AVAILABLE_IN_ALL
const hb_variation_t *  pango2_hb_font_get_variations           (Pango2HbFont                    *self,
                                                                 unsigned int                    *n_variations);

G_END_DECLS
