/*
 * Copyright (C) 2002 Red Hat Software
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

#include <glib-object.h>

#include <pango2/pango-language.h>
#include <pango2/pango-version-macros.h>

G_BEGIN_DECLS

/**
 * Pango2ScriptIter:
 *
 * A `Pango2ScriptIter` is used to iterate through a string
 * and identify ranges in different scripts.
 **/
typedef struct _Pango2ScriptIter Pango2ScriptIter;

PANGO2_AVAILABLE_IN_ALL
GType                   pango2_script_iter_get_type             (void) G_GNUC_CONST;

PANGO2_AVAILABLE_IN_ALL
Pango2ScriptIter *      pango2_script_iter_new                  (const char        *text,
                                                                 int                length);
PANGO2_AVAILABLE_IN_ALL
void                    pango2_script_iter_get_range            (Pango2ScriptIter  *iter,
                                                                 const char       **start,
                                                                 const char       **end,
                                                                 GUnicodeScript    *script);
PANGO2_AVAILABLE_IN_ALL
gboolean                pango2_script_iter_next                 (Pango2ScriptIter  *iter);
PANGO2_AVAILABLE_IN_ALL
void                    pango2_script_iter_free                 (Pango2ScriptIter  *iter);

PANGO2_AVAILABLE_IN_ALL
Pango2Language *        pango2_script_get_sample_language       (GUnicodeScript     script) G_GNUC_PURE;

G_DEFINE_AUTOPTR_CLEANUP_FUNC(Pango2ScriptIter, pango2_script_iter_free)

G_END_DECLS
