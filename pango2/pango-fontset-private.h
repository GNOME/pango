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


#define PANGO2_FONTSET_CLASS(klass)      (G_TYPE_CHECK_CLASS_CAST ((klass), PANGO2_TYPE_FONTSET, Pango2FontsetClass))
#define PANGO2_FONTSET_GET_CLASS(obj)    (G_TYPE_INSTANCE_GET_CLASS ((obj), PANGO2_TYPE_FONTSET, Pango2FontsetClass))


typedef struct _Pango2FontsetClass   Pango2FontsetClass;

struct _Pango2Fontset
{
  GObject parent_instance;
};

struct _Pango2FontsetClass
{
  GObjectClass parent_class;

  Pango2Font *            (* get_font)      (Pango2Fontset            *fontset,
                                             guint                     wc);

  Pango2FontMetrics *     (* get_metrics)   (Pango2Fontset            *fontset);
  Pango2Language *        (* get_language)  (Pango2Fontset            *fontset);
  void                    (* foreach)       (Pango2Fontset            *fontset,
                                             Pango2FontsetForeachFunc  func,
                                             gpointer                  data);
};

Pango2Language * pango2_fontset_get_language (Pango2Fontset *fontset);

