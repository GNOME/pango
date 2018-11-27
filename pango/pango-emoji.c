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
 * Implementation of pango_emoji_iter is based on Chromium's Ragel-based
 * parser:
 *
 * https://chromium-review.googlesource.com/c/chromium/src/+/1264577
 *
 * The grammar file emoji_presentation_scanner.rl was just modified to
 * adapt the function signature and variables to our usecase.  The
 * grammar itself was NOT modified:
 *
 * https://chromium-review.googlesource.com/c/chromium/src/+/1264577/3/third_party/blink/renderer/platform/fonts/emoji_presentation_scanner.rl
 *
 * The emoji_presentation_scanner.c is generated from .rl file by
 * running ragel on it.
 *
 * The categorization is also based on:
 *
 * https://chromium-review.googlesource.com/c/chromium/src/+/1264577/3/third_party/blink/renderer/platform/fonts/utf16_ragel_iterator.h
 *
 * The iterator next() is based on:
 *
 * https://chromium-review.googlesource.com/c/chromium/src/+/1264577/3/third_party/blink/renderer/platform/fonts/symbols_iterator.cc
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
DEFINE_pango_Is_(Extended_Pictographic)

gboolean
_pango_Is_Emoji_Extended_Pictographic (gunichar ch)
{
	return _pango_Is_Extended_Pictographic (ch);
}

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
const gunichar kVariationSelector15Character = 0xFE0E;
const gunichar kVariationSelector16Character = 0xFE0F;
const gunichar kZeroWidthJoinerCharacter = 0x200D;

enum PangoEmojiScannerCategory {
  EMOJI = 0,
  EMOJI_TEXT_PRESENTATION = 1,
  EMOJI_EMOJI_PRESENTATION = 2,
  EMOJI_MODIFIER_BASE = 3,
  EMOJI_MODIFIER = 4,
  EMOJI_VS_BASE = 5,
  REGIONAL_INDICATOR = 6,
  KEYCAP_BASE = 7,
  COMBINING_ENCLOSING_KEYCAP = 8,
  COMBINING_ENCLOSING_CIRCLE_BACKSLASH = 9,
  ZWJ = 10,
  VS15 = 11,
  VS16 = 12,
  TAG_BASE = 13,
  TAG_SEQUENCE = 14,
  TAG_TERM = 15,
  kMaxEmojiScannerCategory = 16
};

static unsigned char
_pango_EmojiSegmentationCategory (gunichar codepoint)
{
  /* Specific ones first. */
  if (codepoint == kCombiningEnclosingKeycapCharacter)
    return COMBINING_ENCLOSING_KEYCAP;
  if (codepoint == kCombiningEnclosingCircleBackslashCharacter)
    return COMBINING_ENCLOSING_CIRCLE_BACKSLASH;
  if (codepoint == kZeroWidthJoinerCharacter)
    return ZWJ;
  if (codepoint == kVariationSelector15Character)
    return VS15;
  if (codepoint == kVariationSelector16Character)
    return VS16;
  if (codepoint == 0x1F3F4)
    return TAG_BASE;
  if ((codepoint >= 0xE0030 && codepoint <= 0xE0039) ||
      (codepoint >= 0xE0061 && codepoint <= 0xE007A))
    return TAG_SEQUENCE;
  if (codepoint == 0xE007F)
    return TAG_TERM;
  if (_pango_Is_Emoji_Modifier_Base (codepoint))
    return EMOJI_MODIFIER_BASE;
  if (_pango_Is_Emoji_Modifier (codepoint))
    return EMOJI_MODIFIER;
  if (_pango_Is_Regional_Indicator (codepoint))
    return REGIONAL_INDICATOR;
  if (_pango_Is_Emoji_Keycap_Base (codepoint))
    return KEYCAP_BASE;

  if (_pango_Is_Emoji_Emoji_Default (codepoint))
    return EMOJI_EMOJI_PRESENTATION;
  if (_pango_Is_Emoji_Text_Default (codepoint))
    return EMOJI_TEXT_PRESENTATION;
  if (_pango_Is_Emoji (codepoint))
    return EMOJI;

  /* Ragel state machine will interpret unknown category as "any". */
  return kMaxEmojiScannerCategory;
}

#define found_text_presentation_sequence
#define found_emoji_presentation_sequence                                \
  {                                                                      \
    if (0) g_print ("emoji %ld..%ld\n", ts - buffer, te - buffer);       \
    *last = ts - buffer;                                                 \
    *end = te - buffer;                                                  \
    return TRUE;                                                         \
  }

#include "emoji_presentation_scanner.c"


PangoEmojiIter *
_pango_emoji_iter_init (PangoEmojiIter *iter,
			const char     *text,
			int             length)
{
  unsigned int n_chars = g_utf8_strlen (text, length);
  unsigned char *types = g_malloc (n_chars);
  unsigned int i;
  const char *p;

  p = text;
  for (i = 0; i < n_chars; i++)
  {
    types[i] = _pango_EmojiSegmentationCategory (g_utf8_get_char (p));
    p = g_utf8_next_char (p);
  }

  iter->text_start = iter->start = iter->end = iter->token_start = iter->token_end = text;
  if (length >= 0)
    iter->text_end = text + length;
  else
    iter->text_end = text + strlen (text);
  iter->is_emoji = FALSE;

  iter->types = types;
  iter->n_chars = n_chars;
  iter->cursor = 0;

  _pango_emoji_iter_next (iter);

  return iter;
}

void
_pango_emoji_iter_fini (PangoEmojiIter *iter)
{
  g_free (iter->types);
}

gboolean
_pango_emoji_iter_next (PangoEmojiIter *iter)
{
  if (iter->end >= iter->text_end)
    return FALSE;

  iter->start = iter->end;

  /* The scan_emoji_presentation scanner function returns false when it reaches
   * the end of the buffer and has not discovered any emoji runs in between. For
   * Emoji runs, it returns true, and token_start_ and token_end_ are set to the
   * start and end of the emoji sequence. This means, it may skip over text runs
   * in between, see below. */
  if (iter->start >= iter->token_end)
    {
      /* We need to scan furhter. */
      unsigned int token_start, token_end;
      if (!scan_emoji_presentation (iter->types, iter->n_chars, iter->cursor,
				    &token_start, &token_end))
	{
	  /* The scanner returned false, which means it has reached the end of the
	   * buffer without discovering any emoji segments in between. */
	  iter->end = iter->text_end;
	  iter->is_emoji = FALSE;

	  return TRUE;
	};
      /* Ugly... */
      g_assert (iter->cursor <= token_start && token_start < token_end && token_end <= iter->n_chars);
      iter->token_start = g_utf8_offset_to_pointer (iter->token_end, token_start - iter->cursor);
      iter->token_end   = g_utf8_offset_to_pointer (iter->token_end, token_end   - iter->cursor);
      iter->cursor = token_end;
    }

  if (iter->start < iter->token_start)
    {
      /* The scanner function has progressed to the next emoji segment, but we
       * need to return the text segment over which it had skipped. */
      iter->end = iter->token_start;
      iter->is_emoji = FALSE;
      return TRUE;
    }

  if (iter->start >= iter->token_start && iter->start < iter->token_end)
    {
      /* Now our cursor has reached the emoji segment, and we can return it. */
      iter->end = iter->token_end;
      iter->is_emoji = TRUE;
      return TRUE;
    }

  g_assert_not_reached ();
  return FALSE;
}


/**********************************************************
 * End of code from Chromium
 **********************************************************/
