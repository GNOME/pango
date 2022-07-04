/*
 * Copyright 2022 Red Hat, Inc.
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

#include "pango-font-family.h"


typedef struct _Pango2FontFamilyClass Pango2FontFamilyClass;

struct _Pango2FontFamily
{
  GObject parent_instance;

  Pango2FontMap *map;
  char *name;
};

struct _Pango2FontFamilyClass
{
  GObjectClass parent_class;

  Pango2FontFace * (* get_face)     (Pango2FontFamily *family,
                                    const char      *name);
};

#define PANGO2_FONT_FAMILY_CLASS(klass)      (G_TYPE_CHECK_CLASS_CAST ((klass), PANGO2_TYPE_FONT_FAMILY, Pango2FontFamilyClass))
#define PANGO2_FONT_FAMILY_GET_CLASS(obj)    (G_TYPE_INSTANCE_GET_CLASS ((obj), PANGO2_TYPE_FONT_FAMILY, Pango2FontFamilyClass))

static inline void
pango2_font_family_set_name (Pango2FontFamily *family,
                             const char       *name)
{
  family->name = g_strdup (name);
}

static inline void
pango2_font_family_set_font_map (Pango2FontFamily *family,
                                 Pango2FontMap    *map)
{
  if (family->map)
    g_object_remove_weak_pointer (G_OBJECT (family->map),
                                  (gpointer *)&family->map);

  family->map = map;

  if (family->map)
    g_object_add_weak_pointer (G_OBJECT (family->map),
                               (gpointer *)&family->map);
}
