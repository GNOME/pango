// Copyright 2018 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

%%{
  machine emoji_presentation;
  alphtype unsigned char;
  write data noerror nofinal;
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

emoji_combining_enclosing_circle_backslash_sequence = any_emoji
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
 emoji_combining_enclosing_circle_backslash_sequence;

emoji_run = emoji_presentation;

text_emoji = EMOJI_TEXT_PRESENTATION;
text_presentation_emoji = any_emoji VS15;
text_run = any;

text_and_emoji_run := |*
# In order to give the the VS15 sequences higher priority than detecting
# emoji sequences they are listed first as scanner token here.
text_presentation_emoji => { *state = EMOJI_PRESENTATION_TEXT; *explicit = TRUE; return te; };
text_emoji => { *state = EMOJI_PRESENTATION_TEXT; *explicit = FALSE; return te; };
emoji_presentation_sequence => { *state = EMOJI_PRESENTATION_EMOJI; *explicit = TRUE; return te; };
emoji_run => { *state = EMOJI_PRESENTATION_EMOJI; *explicit = FALSE; return te; };
text_run => { *state = EMOJI_PRESENTATION_NONE; *explicit = TRUE; return te; };
*|;

}%%

static emoji_text_iter_t
scan_emoji_presentation (emoji_text_iter_t p,
    const emoji_text_iter_t pe,
    EmojiPresentation *state,
    gboolean *explicit)
{
  emoji_text_iter_t ts, te;
  const emoji_text_iter_t eof = pe;

  unsigned act;
  int cs;

  %%{
    write init;
    write exec;
  }%%

  /* Should not be reached. */
  *state = EMOJI_PRESENTATION_NONE;
  *explicit = FALSE;
  return pe;
}
