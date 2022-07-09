/*
 * Copyright (C) 2000 Red Hat Software
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
#include <pango2/pango-font.h>
#include <pango2/pango-fontmap.h>
#include <pango2/pango-attributes.h>
#include <pango2/pango-direction.h>

G_BEGIN_DECLS

#define PANGO2_TYPE_CONTEXT              (pango2_context_get_type ())

PANGO2_AVAILABLE_IN_ALL
G_DECLARE_FINAL_TYPE (Pango2Context, pango2_context, PANGO2, CONTEXT, GObject);

PANGO2_AVAILABLE_IN_ALL
Pango2Context *          pango2_context_new                       (void);
PANGO2_AVAILABLE_IN_ALL
Pango2Context *          pango2_context_new_with_font_map         (Pango2FontMap                 *font_map);

PANGO2_AVAILABLE_IN_ALL
void                     pango2_context_changed                   (Pango2Context                 *context);
PANGO2_AVAILABLE_IN_ALL
void                     pango2_context_set_font_map              (Pango2Context                 *context,
                                                                   Pango2FontMap                 *font_map);
PANGO2_AVAILABLE_IN_ALL
Pango2FontMap *          pango2_context_get_font_map              (Pango2Context                 *context);
PANGO2_AVAILABLE_IN_ALL
guint                    pango2_context_get_serial                (Pango2Context                 *context);
PANGO2_AVAILABLE_IN_ALL
Pango2Font *             pango2_context_load_font                 (Pango2Context                 *context,
                                                                   const Pango2FontDescription   *desc);
PANGO2_AVAILABLE_IN_ALL
Pango2Fontset *          pango2_context_load_fontset              (Pango2Context                 *context,
                                                                   const Pango2FontDescription   *desc,
                                                                   Pango2Language                *language);

PANGO2_AVAILABLE_IN_ALL
Pango2FontMetrics *      pango2_context_get_metrics               (Pango2Context                 *context,
                                                                   const Pango2FontDescription   *desc,
                                                                   Pango2Language                *language);

PANGO2_AVAILABLE_IN_ALL
void                     pango2_context_set_font_description      (Pango2Context                 *context,
                                                                  const Pango2FontDescription    *desc);
PANGO2_AVAILABLE_IN_ALL
Pango2FontDescription *  pango2_context_get_font_description      (Pango2Context                 *context);
PANGO2_AVAILABLE_IN_ALL
Pango2Language *         pango2_context_get_language              (Pango2Context                 *context);
PANGO2_AVAILABLE_IN_ALL
void                     pango2_context_set_language              (Pango2Context                 *context,
                                                                   Pango2Language                *language);
PANGO2_AVAILABLE_IN_ALL
void                     pango2_context_set_base_dir              (Pango2Context                 *context,
                                                                   Pango2Direction                direction);
PANGO2_AVAILABLE_IN_ALL
Pango2Direction          pango2_context_get_base_dir              (Pango2Context                 *context);
PANGO2_AVAILABLE_IN_ALL
void                     pango2_context_set_base_gravity          (Pango2Context                 *context,
                                                                   Pango2Gravity                  gravity);
PANGO2_AVAILABLE_IN_ALL
Pango2Gravity            pango2_context_get_base_gravity          (Pango2Context                 *context);
PANGO2_AVAILABLE_IN_ALL
Pango2Gravity            pango2_context_get_gravity               (Pango2Context                 *context);
PANGO2_AVAILABLE_IN_ALL
void                     pango2_context_set_gravity_hint          (Pango2Context                 *context,
                                                                   Pango2GravityHint              hint);
PANGO2_AVAILABLE_IN_ALL
Pango2GravityHint        pango2_context_get_gravity_hint          (Pango2Context                 *context);

PANGO2_AVAILABLE_IN_ALL
void                     pango2_context_set_matrix                (Pango2Context                 *context,
                                                                   const Pango2Matrix            *matrix);
PANGO2_AVAILABLE_IN_ALL
const Pango2Matrix *     pango2_context_get_matrix                (Pango2Context                 *context);

PANGO2_AVAILABLE_IN_ALL
void                     pango2_context_set_round_glyph_positions (Pango2Context                 *context,
                                                                   gboolean                       round_positions);
PANGO2_AVAILABLE_IN_ALL
gboolean                 pango2_context_get_round_glyph_positions (Pango2Context                 *context);

PANGO2_AVAILABLE_IN_ALL
void                     pango2_context_set_palette               (Pango2Context                 *context,
                                                                   const char                    *palette);
PANGO2_AVAILABLE_IN_ALL
const char *             pango2_context_get_palette               (Pango2Context                 *context);

PANGO2_AVAILABLE_IN_ALL
void                     pango2_context_set_emoji_presentation    (Pango2Context                 *context,
                                                                   Pango2EmojiPresentation        presentation);
PANGO2_AVAILABLE_IN_ALL
Pango2EmojiPresentation  pango2_context_get_emoji_presentation    (Pango2Context                 *context);


G_END_DECLS
