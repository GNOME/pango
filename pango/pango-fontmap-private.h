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

#include <pango/pango-fontmap.h>
#include <pango/pango-hbfamily-private.h>
#include <pango/pango-fontmap-private.h>


struct _PangoFontMap
{
  GObject parent_instance;

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
 * PangoFontMapClass:
 * @populate: Subclasses should call pango_font_map_add_face to populate
 *   the map with faces and families in this vfunc.
 */
struct _PangoFontMapClass
{
  GObjectClass parent_class;

  PangoFont *       (* load_font)     (PangoFontMap               *self,
                                       PangoContext               *context,
                                       const PangoFontDescription *desc);
  PangoFontset *    (* load_fontset)  (PangoFontMap               *self,
                                       PangoContext               *context,
                                       const PangoFontDescription *desc,
                                       PangoLanguage              *language);
  guint             (* get_serial)    (PangoFontMap               *self);
  void              (* changed)       (PangoFontMap               *self);
  PangoFontFamily * (* get_family)    (PangoFontMap               *self,
                                       const char                 *name);
  void              (* populate)      (PangoFontMap               *self);
};

void                    pango_font_map_repopulate    (PangoFontMap *self,
                                                      gboolean      add_synthetic);

void                    pango_font_map_changed       (PangoFontMap *self);
