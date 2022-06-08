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

#include <pango/pango-types.h>

#include <glib-object.h>

G_BEGIN_DECLS

/**
 * PangoFontMetrics:
 *
 * A `PangoFontMetrics` structure holds the overall metric information
 * for a font.
 *
 * The information in a `PangoFontMetrics` structure may be restricted
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
typedef struct _PangoFontMetrics PangoFontMetrics;

#define PANGO_TYPE_FONT_METRICS  (pango_font_metrics_get_type ())

PANGO_AVAILABLE_IN_ALL
GType             pango_font_metrics_get_type                    (void) G_GNUC_CONST;
PANGO_AVAILABLE_IN_ALL
PangoFontMetrics *pango_font_metrics_ref                         (PangoFontMetrics *metrics);
PANGO_AVAILABLE_IN_ALL
void              pango_font_metrics_unref                       (PangoFontMetrics *metrics);
PANGO_AVAILABLE_IN_ALL
int               pango_font_metrics_get_ascent                  (PangoFontMetrics *metrics) G_GNUC_PURE;
PANGO_AVAILABLE_IN_ALL
int               pango_font_metrics_get_descent                 (PangoFontMetrics *metrics) G_GNUC_PURE;
PANGO_AVAILABLE_IN_ALL
int               pango_font_metrics_get_height                  (PangoFontMetrics *metrics) G_GNUC_PURE;
PANGO_AVAILABLE_IN_ALL
int               pango_font_metrics_get_approximate_char_width  (PangoFontMetrics *metrics) G_GNUC_PURE;
PANGO_AVAILABLE_IN_ALL
int               pango_font_metrics_get_approximate_digit_width (PangoFontMetrics *metrics) G_GNUC_PURE;
PANGO_AVAILABLE_IN_ALL
int               pango_font_metrics_get_underline_position      (PangoFontMetrics *metrics) G_GNUC_PURE;
PANGO_AVAILABLE_IN_ALL
int               pango_font_metrics_get_underline_thickness     (PangoFontMetrics *metrics) G_GNUC_PURE;
PANGO_AVAILABLE_IN_ALL
int               pango_font_metrics_get_strikethrough_position  (PangoFontMetrics *metrics) G_GNUC_PURE;
PANGO_AVAILABLE_IN_ALL
int               pango_font_metrics_get_strikethrough_thickness (PangoFontMetrics *metrics) G_GNUC_PURE;


G_END_DECLS
