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

#include "pango-font-description.h"


gboolean pango_font_description_is_similar       (const PangoFontDescription *a,
                                                  const PangoFontDescription *b);

int      pango_font_description_compute_distance (const PangoFontDescription *a,
                                                  const PangoFontDescription *b);

gboolean pango_parse_style              (const char   *str,
                                         PangoStyle   *style,
                                         gboolean      warn);
gboolean pango_parse_variant            (const char   *str,
                                         PangoVariant *variant,
                                         gboolean      warn);
gboolean pango_parse_weight             (const char   *str,
                                         PangoWeight  *weight,
                                         gboolean      warn);
gboolean pango_parse_stretch            (const char   *str,
                                         PangoStretch *stretch,
                                         gboolean      warn);

