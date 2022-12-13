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

#include "pango-types.h"
#include "pango-fontset-private.h"
#include "pango-generic-family.h"
#include <glib-object.h>

#ifdef HAVE_CAIRO
#include <cairo.h>
#endif

#define PANGO2_TYPE_FONTSET_CACHED (pango2_fontset_cached_get_type ())

G_DECLARE_FINAL_TYPE (Pango2FontsetCached, pango2_fontset_cached, PANGO2, FONTSET_CACHED, Pango2Fontset)

struct _Pango2FontsetCached
{
  Pango2Fontset parent_instance;

  GPtrArray *items; /* contains Pango2HbFont or Pango2GenericFamily */
  Pango2Language *language;
  Pango2FontDescription *description;
  float dpi;
  Pango2Matrix *ctm;
  GList cache_link;
  GHashTable *cache;

#ifdef HAVE_CAIRO
  cairo_font_options_t *font_options;
#endif
};

Pango2FontsetCached * pango2_fontset_cached_new            (const Pango2FontDescription *description,
                                                            Pango2Language              *language,
                                                            float                        dpi,
                                                            const Pango2Matrix          *ctm);

void                 pango2_fontset_cached_add_face        (Pango2FontsetCached         *self,
                                                            Pango2FontFace              *face);
void                 pango2_fontset_cached_add_family      (Pango2FontsetCached         *self,
                                                            Pango2GenericFamily         *family);
int                  pango2_fontset_cached_size            (Pango2FontsetCached         *self);
Pango2Font *          pango2_fontset_cached_get_first_font (Pango2FontsetCached         *self);

void                 pango2_fontset_cached_append          (Pango2FontsetCached         *self,
                                                            Pango2FontsetCached         *other);
