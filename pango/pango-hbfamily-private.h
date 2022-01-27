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

#include "pango-font.h"

#define PANGO_TYPE_HB_FAMILY (pango_hb_family_get_type ())

PANGO_AVAILABLE_IN_1_52
G_DECLARE_FINAL_TYPE (PangoHbFamily, pango_hb_family, PANGO, HB_FAMILY, PangoFontFamily)

struct _PangoHbFamily
{
  PangoFontFamily parent_instance;

  PangoFontMap *map;
  char *name;
  GPtrArray *faces;
};

PANGO_AVAILABLE_IN_1_52
PangoHbFamily *         pango_hb_family_new             (const char             *name);

void                    pango_hb_family_set_font_map    (PangoHbFamily          *self,
                                                         PangoFontMap           *map);

void                    pango_hb_family_add_face        (PangoHbFamily          *self,
                                                         PangoFontFace          *face);

void                    pango_hb_family_remove_face     (PangoHbFamily          *self,
                                                         PangoFontFace          *face);

PangoFontFace *         pango_hb_family_find_face       (PangoHbFamily          *self,
                                                         PangoFontDescription   *description,
                                                         PangoLanguage          *language,
                                                         gunichar                wc);
