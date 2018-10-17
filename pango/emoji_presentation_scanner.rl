// Copyright 2018 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

%%{
  machine emoji_presentation;
  alphtype unsigned char;
  write data noerror nofinal noentry;
}%%

%%{

EMOJI = 0;
EMOJI_TEXT_PRESENTATION = 1;
EMOJI_EMOJI_PRESENTATION = 2;
EMOJI_MODIFIER_BASE = 3;
EMOJI_MODIFIER = 4;
EMOJI_VS_BASE = 5;
REGIONAL_INDICATOR = 6;
KEYCAP_BASE = 7;
COMBINING_ENCLOSING_KEYCAP = 8;
COMBINING_ENCLOSING_CIRCLE_BACKSLASH = 9;
ZWJ = 10;
VS15 = 11;
VS16 = 12;
TAG_BASE = 13;
TAG_SEQUENCE = 14;
TAG_TERM = 15;

any_emoji =  EMOJI_TEXT_PRESENTATION | EMOJI_EMOJI_PRESENTATION |  KEYCAP_BASE |
  EMOJI_MODIFIER_BASE | TAG_BASE | EMOJI;

emoji_combining_encloding_circle_backslash_sequence = any_emoji
  COMBINING_ENCLOSING_CIRCLE_BACKSLASH;

# This could be sharper than any_emoji by restricting this only to valid
# variation sequences:
# https://www.unicode.org/Public/emoji/11.0/emoji-variation-sequences.txt
# However, implementing
# https://www.unicode.org/reports/tr51/#def_emoji_presentation_sequence is
# sufficient for our purposes here.
emoji_presentation_sequence = any_emoji VS16;

emoji_modifier_sequence = EMOJI_MODIFIER_BASE EMOJI_MODIFIER;

emoji_flag_sequence = REGIONAL_INDICATOR REGIONAL_INDICATOR;

# Here we only allow the valid tag sequences
# https://www.unicode.org/reports/tr51/#valid-emoji-tag-sequences, instead of
# all well-formed ones defined in
# https://www.unicode.org/reports/tr51/#def_emoji_tag_sequence
emoji_tag_sequence = TAG_BASE TAG_SEQUENCE+ TAG_TERM;

emoji_keycap_sequence = KEYCAP_BASE VS16 COMBINING_ENCLOSING_KEYCAP;

emoji_zwj_element =  emoji_presentation_sequence | emoji_modifier_sequence | any_emoji;

emoji_zwj_sequence = emoji_zwj_element ( ZWJ emoji_zwj_element )+;

emoji_presentation = EMOJI_EMOJI_PRESENTATION | TAG_BASE | EMOJI_MODIFIER_BASE |
  emoji_presentation_sequence | emoji_modifier_sequence | emoji_flag_sequence |
  emoji_tag_sequence | emoji_keycap_sequence | emoji_zwj_sequence |
  emoji_combining_encloding_circle_backslash_sequence;

emoji_run = emoji_presentation+;

text_presentation_emoji = any_emoji VS15;
text_run = text_presentation_emoji | any;

text_and_emoji_run := |*
emoji_run => { found_emoji_presentation_sequence };
text_run => { found_text_presentation_sequence };
*|;

}%%

static gboolean
scan_emoji_presentation (const unsigned char* buffer,
                         unsigned buffer_size,
                         unsigned cursor,
                         unsigned* last,
                         unsigned* end)
{
  const unsigned char *p = buffer + cursor;
  const unsigned char *pe, *eof, *ts, *te;
  unsigned act;
  int cs;
  pe = eof = buffer + buffer_size;

  %%{
    write init;
    write exec;
  }%%
  return FALSE;
}

