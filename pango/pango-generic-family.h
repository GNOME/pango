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

G_BEGIN_DECLS

#define PANGO_TYPE_GENERIC_FAMILY (pango_generic_family_get_type ())

PANGO_AVAILABLE_IN_1_52
G_DECLARE_FINAL_TYPE (PangoGenericFamily, pango_generic_family, PANGO, GENERIC_FAMILY, PangoFontFamily)

PANGO_AVAILABLE_IN_1_52
PangoGenericFamily *    pango_generic_family_new                (const char              *name);

PANGO_AVAILABLE_IN_1_52
void                    pango_generic_family_add_family         (PangoGenericFamily      *self,
                                                                 PangoFontFamily         *family);

G_END_DECLS
