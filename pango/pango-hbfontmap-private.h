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

#include <pango/pango-hbfontmap.h>
#include <pango/pango-hbfamily-private.h>
#include <pango/pango-fontmap-private.h>


struct _PangoHbFontMap
{
  PangoFontMap parent_instance;

  GPtrArray *added_faces;
  GPtrArray *added_families;
  GHashTable *families_hash;
  GPtrArray *families;
  GHashTable *fontsets;
  GQueue fontset_cache;

  double dpi;
  gboolean in_populate;
  guint serial;
};

/**
 * PangoHbFontMapClass:
 * @populate: Subclasses should call pango_hb_font_map_add_face to populate
 *   the map with faces and families in this vfunc.
 */
struct _PangoHbFontMapClass
{
  PangoFontMapClass parent_class;

  void (* populate) (PangoHbFontMap *self);

  gpointer reserved[10];
};

void                    pango_hb_font_map_repopulate    (PangoHbFontMap *self,
                                                         gboolean        add_synthetic);
