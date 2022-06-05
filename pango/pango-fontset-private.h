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

#include "pango-fontset.h"


#define PANGO_FONTSET_CLASS(klass)      (G_TYPE_CHECK_CLASS_CAST ((klass), PANGO_TYPE_FONTSET, PangoFontsetClass))
#define PANGO_FONTSET_GET_CLASS(obj)    (G_TYPE_INSTANCE_GET_CLASS ((obj), PANGO_TYPE_FONTSET, PangoFontsetClass))


typedef struct _PangoFontsetClass   PangoFontsetClass;

struct _PangoFontset
{
  GObject parent_instance;
};

struct _PangoFontsetClass
{
  GObjectClass parent_class;

  PangoFont *       (*get_font)     (PangoFontset     *fontset,
                                     guint             wc);

  PangoFontMetrics *(*get_metrics)  (PangoFontset     *fontset);
  PangoLanguage *   (*get_language) (PangoFontset     *fontset);
  void              (*foreach)      (PangoFontset           *fontset,
                                     PangoFontsetForeachFunc func,
                                     gpointer                data);
};
