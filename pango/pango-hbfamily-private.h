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

#include "pango-font-family-private.h"
#include "pango-hbface.h"


#define PANGO_TYPE_HB_FAMILY (pango_hb_family_get_type ())

G_DECLARE_FINAL_TYPE (PangoHbFamily, pango_hb_family, PANGO, HB_FAMILY, PangoFontFamily)

struct _PangoHbFamily
{
  PangoFontFamily parent_instance;

  GPtrArray *faces;
};

PangoHbFamily *         pango_hb_family_new             (const char             *name);

void                    pango_hb_family_add_face        (PangoHbFamily          *self,
                                                         PangoFontFace          *face);

void                    pango_hb_family_remove_face     (PangoHbFamily          *self,
                                                         PangoFontFace          *face);

PangoFontFace *         pango_hb_family_find_face       (PangoHbFamily          *self,
                                                         PangoFontDescription   *description,
                                                         PangoLanguage          *language,
                                                         gunichar                wc);
