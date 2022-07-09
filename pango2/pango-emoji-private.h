/*
 * Copyright (C) 2017 Google, Inc.
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

#include <glib.h>
#include "pango-types.h"

gboolean
_pango2_Is_Emoji_Base_Character (gunichar ch);

gboolean
_pango2_Is_Emoji_Extended_Pictographic (gunichar ch);

typedef struct _Pango2EmojiIter Pango2EmojiIter;

typedef enum {
  EMOJI_PRESENTATION_NONE,
  EMOJI_PRESENTATION_TEXT,
  EMOJI_PRESENTATION_EMOJI,
} EmojiPresentation;

struct _Pango2EmojiIter
{
  const char *text_start;
  const char *text_end;
  const char *start;
  const char *end;

  gboolean explicit;
  EmojiPresentation state;

  unsigned char *types;
  unsigned int n_chars;
  unsigned int cursor;
};

Pango2EmojiIter *
pango2_emoji_iter_init (Pango2EmojiIter *iter,
                        const char      *text,
                        int              length);

gboolean
pango2_emoji_iter_next (Pango2EmojiIter *iter);


void
pango2_emoji_iter_fini (Pango2EmojiIter *iter);

EmojiPresentation
pango2_emoji_iter_get (Pango2EmojiIter         *iter,
                       Pango2EmojiPresentation  preferred);
