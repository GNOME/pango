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
#include <pango/pango-fontset-private.h>
#include <glib-object.h>


typedef struct _PangoFontsetSimple  PangoFontsetSimple;
typedef struct _PangoFontsetSimpleClass  PangoFontsetSimpleClass;

struct _PangoFontsetSimple
{
  PangoFontset parent_instance;

  GPtrArray *fonts;
  PangoLanguage *language;
};

struct _PangoFontsetSimpleClass
{
  PangoFontsetClass parent_class;
};


#define PANGO_TYPE_FONTSET_SIMPLE       (pango_fontset_simple_get_type ())
#define PANGO_FONTSET_SIMPLE(object)    (G_TYPE_CHECK_INSTANCE_CAST ((object), PANGO_TYPE_FONTSET_SIMPLE, PangoFontsetSimple))
#define PANGO_IS_FONTSET_SIMPLE(object) (G_TYPE_CHECK_INSTANCE_TYPE ((object), PANGO_TYPE_FONTSET_SIMPLE))

GType                   pango_fontset_simple_get_type (void) G_GNUC_CONST;

PangoFontsetSimple *    pango_fontset_simple_new    (PangoLanguage      *language);
void                    pango_fontset_simple_append (PangoFontsetSimple *fontset,
                                                     PangoFont          *font);
int                     pango_fontset_simple_size   (PangoFontsetSimple *fontset);
