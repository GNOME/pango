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

#include <pango/pango-types.h>
#include <pango/pango-font-metrics.h>

#include <glib-object.h>

G_BEGIN_DECLS

/*
 * PangoFontset
 */

#define PANGO_TYPE_FONTSET              (pango_fontset_get_type ())
#define PANGO_FONTSET(object)           (G_TYPE_CHECK_INSTANCE_CAST ((object), PANGO_TYPE_FONTSET, PangoFontset))
#define PANGO_IS_FONTSET(object)        (G_TYPE_CHECK_INSTANCE_TYPE ((object), PANGO_TYPE_FONTSET))


PANGO_AVAILABLE_IN_ALL
GType pango_fontset_get_type (void) G_GNUC_CONST;

typedef struct _PangoFontset        PangoFontset;

/**
 * PangoFontsetForeachFunc:
 * @fontset: a `PangoFontset`
 * @font: a font from @fontset
 * @user_data: callback data
 *
 * Callback used when enumerating fonts in a fontset.
 *
 * See [method@Pango.Fontset.foreach].
 *
 * Returns: if %TRUE, stop iteration and return immediately.
 */
typedef gboolean (*PangoFontsetForeachFunc) (PangoFontset  *fontset,
                                             PangoFont     *font,
                                             gpointer       user_data);

/**
 * PangoFontset:
 *
 * A `PangoFontset` represents a set of `PangoFont` to use when rendering text.
 *
 * A `PangoFontset` is the result of resolving a `PangoFontDescription`
 * against a particular `PangoContext`. It has operations for finding the
 * component font for a particular Unicode character, and for finding a
 * composite set of metrics for the entire fontset.
 */

PANGO_AVAILABLE_IN_ALL
PangoFont *             pango_fontset_get_font          (PangoFontset                   *fontset,
                                                         guint                           wc);
PANGO_AVAILABLE_IN_ALL
PangoFontMetrics *      pango_fontset_get_metrics       (PangoFontset                   *fontset);
PANGO_AVAILABLE_IN_ALL
void                    pango_fontset_foreach           (PangoFontset                   *fontset,
                                                         PangoFontsetForeachFunc         func,
                                                         gpointer                        data);
PANGO_AVAILABLE_IN_ALL
PangoLanguage *         pango_fontset_get_language      (PangoFontset                   *fontset);


G_END_DECLS
