/*
 * Copyright 2021 Red Hat, Inc.
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

#include "pango-fontmap.h"
#include "pango-hbfamily-private.h"
#include "pango-fontmap-private.h"


G_BEGIN_DECLS

struct _Pango2FontMap
{
  GObject parent_instance;

  GPtrArray *added_faces;
  GPtrArray *added_families;
  GHashTable *families_hash;
  GPtrArray *families;
  GHashTable *fontsets;
  GQueue fontset_cache;
  Pango2FontMap *fallback;

  float dpi;
  gboolean in_populate;
  guint serial;
};

/**
 * Pango2FontMapClass:
 * @populate: Subclasses should call pango2_font_map_add_face to populate
 *   the map with faces and families in this vfunc.
 */
struct _Pango2FontMapClass
{
  GObjectClass parent_class;

  Pango2Font *       (* load_font)     (Pango2FontMap               *self,
                                        Pango2Context               *context,
                                        const Pango2FontDescription *desc);
  Pango2Fontset *    (* load_fontset)  (Pango2FontMap               *self,
                                        Pango2Context               *context,
                                        const Pango2FontDescription *desc,
                                        Pango2Language              *language);
  guint              (* get_serial)    (Pango2FontMap               *self);
  void               (* changed)       (Pango2FontMap               *self);
  Pango2FontFamily * (* get_family)    (Pango2FontMap               *self,
                                        const char                  *name);
  void               (* populate)      (Pango2FontMap               *self);
};

void                    pango2_font_map_repopulate    (Pango2FontMap *self,
                                                       gboolean       add_synthetic);

void                    pango2_font_map_changed       (Pango2FontMap *self);

G_END_DECLS
