/* Pango
 *
 * Copyright (C) 2022 Matthias Clasen
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
#include "pango-hbface.h"
#include "pango-generic-family.h"
#include "pango-hbfamily-private.h"


struct _PangoGenericFamily
{
  PangoFontFamily parent_instance;

  PangoFontMap *map;
  char *name;
  GPtrArray *families;
};


void                    pango_generic_family_set_font_map       (PangoGenericFamily      *self,
                                                                 PangoFontMap            *map);

PangoHbFace *           pango_generic_family_find_face          (PangoGenericFamily     *self,
                                                                 PangoFontDescription   *description,
                                                                 PangoLanguage          *language,
                                                                 gunichar                wc);
