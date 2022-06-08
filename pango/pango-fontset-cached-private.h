/*
 * Copyright (C) 2021 Red Hat, Inc.
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
#include <pango/pango-generic-family.h>
#include <glib-object.h>


typedef struct _PangoFontsetCached PangoFontsetCached;
typedef struct _PangoFontsetCachedClass  PangoFontsetCachedClass;

struct _PangoFontsetCached
{
  PangoFontset parent_instance;

  GPtrArray *items; /* contains PangoHbFont or PangoGenericFamily */
  PangoLanguage *language;
  PangoFontDescription *description;
  float dpi;
  const PangoMatrix *matrix;
  GList cache_link;
  GHashTable *cache;
};

struct _PangoFontsetCachedClass
{
  PangoFontsetClass parent_class;
};


GType                pango_fontset_cached_get_type       (void) G_GNUC_CONST;

PangoFontsetCached * pango_fontset_cached_new            (const PangoFontDescription *description,
                                                          PangoLanguage              *language,
                                                          float                       dpi,
                                                          const PangoMatrix          *matrix);

void                 pango_fontset_cached_add_face       (PangoFontsetCached         *self,
                                                          PangoFontFace              *face);
void                 pango_fontset_cached_add_family     (PangoFontsetCached         *self,
                                                          PangoGenericFamily         *family);
int                  pango_fontset_cached_size           (PangoFontsetCached         *self);
PangoFont *          pango_fontset_cached_get_first_font (PangoFontsetCached         *self);

