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

#include "pango-language-set-private.h"


#define PANGO_TYPE_LANGUAGE_SET_SIMPLE (pango_language_set_simple_get_type ())

G_DECLARE_FINAL_TYPE (PangoLanguageSetSimple, pango_language_set_simple, PANGO, LANGUAGE_SET_SIMPLE, PangoLanguageSet)

PangoLanguageSetSimple *pango_language_set_simple_new           (void);

void                    pango_language_set_simple_add_language  (PangoLanguageSetSimple *self,
                                                                 PangoLanguage          *language);
