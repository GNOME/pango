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


#define PANGO2_TYPE_LANGUAGE_SET_SIMPLE (pango2_language_set_simple_get_type ())

G_DECLARE_FINAL_TYPE (Pango2LanguageSetSimple, pango2_language_set_simple, PANGO2, LANGUAGE_SET_SIMPLE, Pango2LanguageSet)

Pango2LanguageSetSimple *pango2_language_set_simple_new           (void);

void                     pango2_language_set_simple_add_language  (Pango2LanguageSetSimple *self,
                                                                   Pango2Language          *language);
