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

#include "pango-language.h"


#define PANGO2_TYPE_LANGUAGE_SET (pango2_language_set_get_type ())

G_DECLARE_DERIVABLE_TYPE (Pango2LanguageSet, pango2_language_set, PANGO2, LANGUAGE_SET, GObject)

struct _Pango2LanguageSetClass
{
  GObjectClass parent_class;

  gboolean               (* matches_language)   (Pango2LanguageSet       *set,
                                                 Pango2Language          *language);

  Pango2Language **      (* get_languages)      (Pango2LanguageSet       *set);
};

gboolean                pango2_language_set_matches_language     (Pango2LanguageSet *set,
                                                                  Pango2Language    *language);

Pango2Language **       pango2_language_set_get_languages        (Pango2LanguageSet *set);

char *                  pango2_language_set_to_string            (Pango2LanguageSet *set);
