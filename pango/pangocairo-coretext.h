/*
 * Copyright (C) 2005 Imendio AB
 * Copyright (C) 2010  Kristian Rietveld  <kris@gtk.org>
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

#include "pangocoretext-private.h"
#include <pango/pangocairo.h>
#include <cairo-quartz.h>

G_BEGIN_DECLS

#define PANGO_TYPE_CAIRO_CORE_TEXT_FONT_MAP       (pango_cairo_core_text_font_map_get_type ())
#define PANGO_CAIRO_CORE_TEXT_FONT_MAP(object)    (G_TYPE_CHECK_INSTANCE_CAST ((object), PANGO_TYPE_CAIRO_CORE_TEXT_FONT_MAP, PangoCairoCoreTextFontMap))
#define PANGO_IS_CAIRO_CORE_TEXT_FONT_MAP(object) (G_TYPE_CHECK_INSTANCE_TYPE ((object), PANGO_TYPE_CAIRO_CORE_TEXT_FONT_MAP))

typedef struct _PangoCairoCoreTextFontMap PangoCairoCoreTextFontMap;

struct _PangoCairoCoreTextFontMap
{
  PangoCoreTextFontMap parent_instance;

  guint serial;
  gdouble dpi;
};

_PANGO_EXTERN
GType pango_cairo_core_text_font_map_get_type (void) G_GNUC_CONST;

PangoCoreTextFont *
_pango_cairo_core_text_font_new (PangoCairoCoreTextFontMap  *cafontmap,
                                 PangoCoreTextFontKey       *key);

G_END_DECLS
