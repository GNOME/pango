/*
 * Copyright (C) 2000 Red Hat Software
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

#include <glib-object.h>

G_BEGIN_DECLS

/**
 * Pango2FontMetrics:
 *
 * A `Pango2FontMetrics` structure holds the overall metric information
 * for a font.
 *
 * The information in a `Pango2FontMetrics` structure may be restricted
 * to a script. The fields of this structure are private to implementations
 * of a font backend. See the documentation of the corresponding getters
 * for documentation of their meaning.
 *
 * For an overview of the most important metrics, see:
 *
 * <picture>
 *   <source srcset="fontmetrics-dark.png" media="(prefers-color-scheme: dark)">
 *   <img alt="Font metrics" src="fontmetrics-light.png">
 * </picture>

 */
typedef struct _Pango2FontMetrics Pango2FontMetrics;

#define PANGO2_TYPE_FONT_METRICS  (pango2_font_metrics_get_type ())

PANGO2_AVAILABLE_IN_ALL
GType             pango2_font_metrics_get_type                    (void) G_GNUC_CONST;
PANGO2_AVAILABLE_IN_ALL
Pango2FontMetrics *pango2_font_metrics_copy                        (Pango2FontMetrics *metrics);
PANGO2_AVAILABLE_IN_ALL
void              pango2_font_metrics_free                        (Pango2FontMetrics *metrics);
PANGO2_AVAILABLE_IN_ALL
int               pango2_font_metrics_get_ascent                  (Pango2FontMetrics *metrics) G_GNUC_PURE;
PANGO2_AVAILABLE_IN_ALL
int               pango2_font_metrics_get_descent                 (Pango2FontMetrics *metrics) G_GNUC_PURE;
PANGO2_AVAILABLE_IN_ALL
int               pango2_font_metrics_get_height                  (Pango2FontMetrics *metrics) G_GNUC_PURE;
PANGO2_AVAILABLE_IN_ALL
int               pango2_font_metrics_get_approximate_char_width  (Pango2FontMetrics *metrics) G_GNUC_PURE;
PANGO2_AVAILABLE_IN_ALL
int               pango2_font_metrics_get_approximate_digit_width (Pango2FontMetrics *metrics) G_GNUC_PURE;
PANGO2_AVAILABLE_IN_ALL
int               pango2_font_metrics_get_underline_position      (Pango2FontMetrics *metrics) G_GNUC_PURE;
PANGO2_AVAILABLE_IN_ALL
int               pango2_font_metrics_get_underline_thickness     (Pango2FontMetrics *metrics) G_GNUC_PURE;
PANGO2_AVAILABLE_IN_ALL
int               pango2_font_metrics_get_strikethrough_position  (Pango2FontMetrics *metrics) G_GNUC_PURE;
PANGO2_AVAILABLE_IN_ALL
int               pango2_font_metrics_get_strikethrough_thickness (Pango2FontMetrics *metrics) G_GNUC_PURE;

G_DEFINE_AUTOPTR_CLEANUP_FUNC(Pango2FontMetrics, pango2_font_metrics_free)

G_END_DECLS
