/*
 * Copyright (C) 2001 Red Hat Software
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

#include <pango2/pango-types.h>
#include <pango2/pango-font-metrics.h>

#include <glib-object.h>

G_BEGIN_DECLS

#define PANGO2_TYPE_FONTSET              (pango2_fontset_get_type ())

PANGO2_AVAILABLE_IN_ALL
PANGO2_DECLARE_INTERNAL_TYPE (Pango2Fontset, pango2_fontset, PANGO2, FONTSET, GObject)

/**
 * Pango2FontsetForeachFunc:
 * @fontset: a `Pango2Fontset`
 * @font: a font from @fontset
 * @user_data: callback data
 *
 * Callback used when enumerating fonts in a fontset.
 *
 * See [method@Pango2.Fontset.foreach].
 *
 * Returns: if %TRUE, stop iteration and return immediately.
 */
typedef gboolean (*Pango2FontsetForeachFunc) (Pango2Fontset *fontset,
                                              Pango2Font    *font,
                                              gpointer       user_data);

PANGO2_AVAILABLE_IN_ALL
Pango2Font *             pango2_fontset_get_font          (Pango2Fontset                   *fontset,
                                                           guint                            wc);
PANGO2_AVAILABLE_IN_ALL
Pango2FontMetrics *      pango2_fontset_get_metrics       (Pango2Fontset                   *fontset);
PANGO2_AVAILABLE_IN_ALL
void                     pango2_fontset_foreach           (Pango2Fontset                   *fontset,
                                                           Pango2FontsetForeachFunc         func,
                                                           gpointer                         data);

G_END_DECLS
