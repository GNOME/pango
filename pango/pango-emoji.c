/* Pango
 * pango-emoji.c: Emoji handling
 *
 * Copyright (C) 2017 Google, Inc.
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Library General Public
 * License as published by the Free Software Foundation; either
 * version 2 of the License, or (at your option) any later version.
 *
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.	 See the GNU
 * Library General Public License for more details.
 *
 * You should have received a copy of the GNU Library General Public
 * License along with this library; if not, write to the
 * Free Software Foundation, Inc., 59 Temple Place - Suite 330,
 * Boston, MA 02111-1307, USA.
 *
 * Implementation of pango_emoji_iter is derived from Chromium:
 *
 * https://cs.chromium.org/chromium/src/third_party/WebKit/Source/platform/fonts/FontFallbackPriority.h
 * https://cs.chromium.org/chromium/src/third_party/WebKit/Source/platform/text/CharacterEmoji.cpp
 * https://cs.chromium.org/chromium/src/third_party/WebKit/Source/platform/fonts/SymbolsIterator.cpp
 *
 * // Copyright 2015 The Chromium Authors. All rights reserved.
 * // Use of this source code is governed by a BSD-style license that can be
 * // found in the LICENSE file.
 */

#include "config.h"
#include <stdlib.h>
#include <string.h>

#include "pango-emoji-private.h"
#include "pango-emoji-table.h"


static int
interval_compare (const void *key, const void *elt)
{
  gunichar c = GPOINTER_TO_UINT (key);
  struct Interval *interval = (struct Interval *)elt;

  if (c < interval->start)
    return -1;
  if (c > interval->end)
    return +1;

  return 0;
}

#define DEFINE_pango_Is_(name) \
static gboolean \
_pango_Is_##name (gunichar ch) \
{ \
  /* bsearch() is declared attribute(nonnull(1)) so we can't validly search \
   * for a NULL key */ \
  /* \
  if (G_UNLIKELY (ch == 0)) \
    return FALSE; \
   */ \
 \
  if (bsearch (GUINT_TO_POINTER (ch), \
               _pango_##name##_table, \
               G_N_ELEMENTS (_pango_##name##_table), \
               sizeof _pango_##name##_table[0], \
	       interval_compare)) \
    return TRUE; \
 \
  return FALSE; \
}

DEFINE_pango_Is_(Emoji)
DEFINE_pango_Is_(Emoji_Presentation)
DEFINE_pango_Is_(Emoji_Modifier)
DEFINE_pango_Is_(Emoji_Modifier_Base)


static gboolean
_pango_Is_Emoji_Text_Default (gunichar ch)
{
  return _pango_Is_Emoji (ch) && !_pango_Is_Emoji_Presentation (ch);
}

static gboolean
_pango_Is_Emoji_Emoji_Default (gunichar ch)
{
  return _pango_Is_Emoji_Presentation (ch);
}

static gboolean
_pango_Is_Emoji_Keycap_Base (gunichar ch)
{
  return (ch >= '0' && ch <= '9') || ch == '#' || ch == '*';
}

static gboolean
_pango_Is_Regional_Indicator (gunichar ch)
{
  return (ch >= 0x1F1E6 && ch <= 0x1F1FF);
}


const gunichar kCombiningEnclosingCircleBackslashCharacter = 0x20E0;
const gunichar kCombiningEnclosingKeycapCharacter = 0x20E3;
const gunichar kEyeCharacter = 0x1F441;
const gunichar kFemaleSignCharacter = 0x2640;
const gunichar kLeftSpeechBubbleCharacter = 0x1F5E8;
const gunichar kMaleSignCharacter = 0x2642;
const gunichar kRainbowCharacter = 0x1F308;
const gunichar kStaffOfAesculapiusCharacter = 0x2695;
const gunichar kVariationSelector15Character = 0xFE0E;
const gunichar kVariationSelector16Character = 0xFE0F;
const gunichar kWavingWhiteFlagCharacter = 0x1F3F3;
const gunichar kZeroWidthJoinerCharacter = 0x200D;


typedef enum {
  PANGO_EMOJI_TYPE_INVALID,
  PANGO_EMOJI_TYPE_TEXT, /* For regular non-symbols text */
  PANGO_EMOJI_TYPE_EMOJI_TEXT, /* For emoji in text presentaiton */
  PANGO_EMOJI_TYPE_EMOJI_EMOJI /* For emoji in emoji presentation */
} PangoEmojiType;

static PangoEmojiType
_pango_get_emoji_type (gunichar codepoint)
{
  /* Those should only be Emoji presentation as combinations of two. */
  if (_pango_Is_Emoji_Keycap_Base (codepoint) ||
      _pango_Is_Regional_Indicator (codepoint))
    return PANGO_EMOJI_TYPE_TEXT;

  if (codepoint == kCombiningEnclosingKeycapCharacter)
    return PANGO_EMOJI_TYPE_EMOJI_EMOJI;

  if (_pango_Is_Emoji_Emoji_Default (codepoint) ||
      _pango_Is_Emoji_Modifier_Base (codepoint) ||
      _pango_Is_Emoji_Modifier (codepoint))
    return PANGO_EMOJI_TYPE_EMOJI_EMOJI;

  if (_pango_Is_Emoji_Text_Default (codepoint))
    return PANGO_EMOJI_TYPE_EMOJI_TEXT;

  return PANGO_EMOJI_TYPE_TEXT;
}


PangoEmojiIter *
_pango_emoji_iter_init (PangoEmojiIter *iter,
			const char     *text,
			int             length)
{
  iter->text_start = text;
  if (length >= 0)
    iter->text_end = text + length;
  else
    iter->text_end = text + strlen (text);

  iter->start = text;
  iter->end = text;
  iter->is_emoji = (gboolean) 2; /* HACK */

  _pango_emoji_iter_next (iter);

  return iter;
}

void
_pango_emoji_iter_fini (PangoEmojiIter *iter)
{
}

#define PANGO_EMOJI_TYPE_IS_EMOJI(typ) ((typ) == PANGO_EMOJI_TYPE_EMOJI_EMOJI)

gboolean
_pango_emoji_iter_next (PangoEmojiIter *iter)
{
  PangoEmojiType current_emoji_type = PANGO_EMOJI_TYPE_INVALID;

  if (iter->end == iter->text_end)
    return FALSE;

  iter->start = iter->end;

  for (; iter->end < iter->text_end; iter->end = g_utf8_next_char (iter->end))
    {
      gunichar ch = g_utf8_get_char (iter->end);

    /* Except at the beginning, ZWJ just carries over the emoji or neutral
     * text type, VS15 & VS16 we just carry over as well, since we already
     * resolved those through lookahead. Also, don't downgrade to text
     * presentation for emoji that are part of a ZWJ sequence, example
     * U+1F441 U+200D U+1F5E8, eye (text presentation) + ZWJ + left speech
     * bubble, see below. */
    if ((!(ch == kZeroWidthJoinerCharacter && !iter->is_emoji) &&
	 ch != kVariationSelector15Character &&
	 ch != kVariationSelector16Character &&
	 ch != kCombiningEnclosingCircleBackslashCharacter &&
	 !_pango_Is_Regional_Indicator(ch) &&
	 !((ch == kLeftSpeechBubbleCharacter ||
	    ch == kRainbowCharacter ||
	    ch == kMaleSignCharacter ||
	    ch == kFemaleSignCharacter ||
	    ch == kStaffOfAesculapiusCharacter) &&
	   !iter->is_emoji)) ||
	current_emoji_type == PANGO_EMOJI_TYPE_INVALID) {
      current_emoji_type = _pango_get_emoji_type (ch);
    }

    if (g_utf8_next_char (iter->end) < iter->text_end) /* Optimize. */
    {
      gunichar peek_char = g_utf8_get_char (g_utf8_next_char (iter->end));

      /* Variation Selectors */
      if (current_emoji_type ==
	      PANGO_EMOJI_TYPE_EMOJI_EMOJI &&
	  peek_char == kVariationSelector15Character) {
	current_emoji_type = PANGO_EMOJI_TYPE_EMOJI_TEXT;
      }

      if ((current_emoji_type ==
	       PANGO_EMOJI_TYPE_EMOJI_TEXT ||
	   _pango_Is_Emoji_Keycap_Base(ch)) &&
	  peek_char == kVariationSelector16Character) {
	current_emoji_type = PANGO_EMOJI_TYPE_EMOJI_EMOJI;
      }

      /* Combining characters Keycap... */
      if (_pango_Is_Emoji_Keycap_Base(ch) &&
	  peek_char == kCombiningEnclosingKeycapCharacter) {
	current_emoji_type = PANGO_EMOJI_TYPE_EMOJI_EMOJI;
      };

      /* Regional indicators */
      if (_pango_Is_Regional_Indicator(ch) &&
	  _pango_Is_Regional_Indicator(peek_char)) {
	current_emoji_type = PANGO_EMOJI_TYPE_EMOJI_EMOJI;
      }

      /* Upgrade text presentation emoji to emoji presentation when followed by
       * ZWJ, Example U+1F441 U+200D U+1F5E8, eye + ZWJ + left speech bubble. */
      if ((ch == kEyeCharacter ||
	   ch == kWavingWhiteFlagCharacter) &&
	  peek_char == kZeroWidthJoinerCharacter) {
	current_emoji_type = PANGO_EMOJI_TYPE_EMOJI_EMOJI;
      }
    }

    if (iter->is_emoji == (gboolean) 2)
      iter->is_emoji = !PANGO_EMOJI_TYPE_IS_EMOJI (current_emoji_type);
    if (iter->is_emoji == PANGO_EMOJI_TYPE_IS_EMOJI (current_emoji_type))
    {
      iter->is_emoji = !PANGO_EMOJI_TYPE_IS_EMOJI (current_emoji_type);
      return TRUE;
    }
  }

  iter->is_emoji = PANGO_EMOJI_TYPE_IS_EMOJI (current_emoji_type);

  return TRUE;
}


/**********************************************************
 * End of code from Chromium
 **********************************************************/
