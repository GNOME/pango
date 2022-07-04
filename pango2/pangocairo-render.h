/*
 * Copyright (C) 1999, 2004 Red Hat, Inc.
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

#include <pango2/pango.h>
#include <cairo.h>

G_BEGIN_DECLS

PANGO2_AVAILABLE_IN_ALL
void    pango2_cairo_show_glyph_string           (cairo_t           *cr,
                                                  Pango2Font        *font,
                                                  Pango2GlyphString *glyphs);

PANGO2_AVAILABLE_IN_ALL
void    pango2_cairo_show_run                    (cairo_t           *cr,
                                                  const char        *text,
                                                  Pango2Run         *run);
PANGO2_AVAILABLE_IN_ALL
void     pango2_cairo_show_line                  (cairo_t           *cr,
                                                  Pango2Line        *line);
PANGO2_AVAILABLE_IN_ALL
void    pango2_cairo_show_lines                  (cairo_t           *cr,
                                                  Pango2Lines       *lines);
PANGO2_AVAILABLE_IN_ALL
void    pango2_cairo_show_layout                 (cairo_t           *cr,
                                                  Pango2Layout      *layout);
PANGO2_AVAILABLE_IN_ALL
void    pango2_cairo_glyph_string_path           (cairo_t           *cr,
                                                  Pango2Font        *font,
                                                  Pango2GlyphString *glyphs);
PANGO2_AVAILABLE_IN_ALL
void    pango2_cairo_run_path                    (cairo_t           *cr,
                                                  const char        *text,
                                                  Pango2Run         *run);
PANGO2_AVAILABLE_IN_ALL
void    pango2_cairo_line_path                   (cairo_t           *cr,
                                                  Pango2Line        *line);
PANGO2_AVAILABLE_IN_ALL
void    pango2_cairo_lines_path                  (cairo_t           *cr,
                                                  Pango2Lines       *lines);
PANGO2_AVAILABLE_IN_ALL
void    pango2_cairo_layout_path                 (cairo_t           *cr,
                                                  Pango2Layout      *layout);

G_END_DECLS
