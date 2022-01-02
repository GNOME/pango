/* Pango
 *
 * Copyright (C) 2021 Matthias Clasen
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Library General Public
 * License as published by the Free Software Foundation; either
 * version 2 of the License, or (at your option) any later version.
 *
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the GNU
 * Library General Public License for more details.
 *
 * You should have received a copy of the GNU Library General Public
 * License along with this library; if not, write to the
 * Free Software Foundation, Inc., 59 Temple Place - Suite 330,
 * Boston, MA 02111-1307, USA.
 */

#pragma once

#include <pango/pango-hbfontmap.h>
#include <pango/pango-hbfamily-private.h>

G_BEGIN_DECLS

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

PANGO_AVAILABLE_IN_1_52
void                    pango_hb_font_map_repopulate    (PangoHbFontMap *self,
                                                         gboolean        add_synthetic);

G_END_DECLS
