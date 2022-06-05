/*
 * Copyright (C) 2001 Red Hat Software
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

#include <pango/pango-coverage.h>
#include <pango/pango-types.h>
#include <pango/pango-fontset.h>

#include <glib-object.h>

G_BEGIN_DECLS

/**
 * PangoFontsetSimple:
 *
 * `PangoFontsetSimple` is a implementation of the abstract
 * `PangoFontset` base class as an array of fonts.
 *
 * When creating a `PangoFontsetSimple`, you have to provide
 * the array of fonts that make up the fontset.
 */
#define PANGO_TYPE_FONTSET_SIMPLE       (pango_fontset_simple_get_type ())
#define PANGO_FONTSET_SIMPLE(object)    (G_TYPE_CHECK_INSTANCE_CAST ((object), PANGO_TYPE_FONTSET_SIMPLE, PangoFontsetSimple))
#define PANGO_IS_FONTSET_SIMPLE(object) (G_TYPE_CHECK_INSTANCE_TYPE ((object), PANGO_TYPE_FONTSET_SIMPLE))

typedef struct _PangoFontsetSimple  PangoFontsetSimple;

PANGO_AVAILABLE_IN_ALL
GType                   pango_fontset_simple_get_type (void) G_GNUC_CONST;

PANGO_AVAILABLE_IN_ALL
PangoFontsetSimple *    pango_fontset_simple_new    (PangoLanguage      *language);
PANGO_AVAILABLE_IN_ALL
void                    pango_fontset_simple_append (PangoFontsetSimple *fontset,
                                                     PangoFont          *font);
PANGO_AVAILABLE_IN_ALL
int                     pango_fontset_simple_size   (PangoFontsetSimple *fontset);


G_END_DECLS
