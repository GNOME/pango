/* Pango
 * test-bidi.c: Test bidi apis
 *
 * Copyright (C) 2021 Red Hat, Inc.
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
 */

#include <pango/pango.h>

G_GNUC_BEGIN_IGNORE_DEPRECATIONS

static void
test_mirror_char (void)
{
  /* just some samples */
  struct {
    gunichar a;
    gunichar b;
  } tests[] = {
    { '(', ')' },
    { '<', '>' },
    { '[', ']' },
    { '{', '}' },
    { 0x00ab, 0x00bb },
    { 0x2045, 0x2046 },
    { 0x226e, 0x226f },
  };

  for (int i = 0; i < G_N_ELEMENTS (tests); i++)
    {
      gboolean ret;
      gunichar ch;

      ret = pango_get_mirror_char (tests[i].a, &ch);
      g_assert_true (ret);
      g_assert_true (ch == tests[i].b);
      ret = pango_get_mirror_char (tests[i].b, &ch);
      g_assert_true (ret);
      g_assert_true (ch == tests[i].a);
    }
}

static void
test_bidi_type_for_unichar (void)
{
  /* one representative from each class we support */
  g_assert_true (pango_bidi_type_for_unichar ('a') == PANGO_BIDI_TYPE_L);
  g_assert_true (pango_bidi_type_for_unichar (0x202a) == PANGO_BIDI_TYPE_LRE);
  g_assert_true (pango_bidi_type_for_unichar (0x202d) == PANGO_BIDI_TYPE_LRO);
  g_assert_true (pango_bidi_type_for_unichar (0x05d0) == PANGO_BIDI_TYPE_R);
  g_assert_true (pango_bidi_type_for_unichar (0x0627) == PANGO_BIDI_TYPE_AL);
  g_assert_true (pango_bidi_type_for_unichar (0x202b) == PANGO_BIDI_TYPE_RLE);
  g_assert_true (pango_bidi_type_for_unichar (0x202e) == PANGO_BIDI_TYPE_RLO);
  g_assert_true (pango_bidi_type_for_unichar (0x202c) == PANGO_BIDI_TYPE_PDF);
  g_assert_true (pango_bidi_type_for_unichar ('0') == PANGO_BIDI_TYPE_EN);
  g_assert_true (pango_bidi_type_for_unichar ('+') == PANGO_BIDI_TYPE_ES);
  g_assert_true (pango_bidi_type_for_unichar ('#') == PANGO_BIDI_TYPE_ET);
  g_assert_true (pango_bidi_type_for_unichar (0x601) == PANGO_BIDI_TYPE_AN);
  g_assert_true (pango_bidi_type_for_unichar (',') == PANGO_BIDI_TYPE_CS);
  g_assert_true (pango_bidi_type_for_unichar (0x0301) == PANGO_BIDI_TYPE_NSM);
  g_assert_true (pango_bidi_type_for_unichar (0x200d) == PANGO_BIDI_TYPE_BN);
  g_assert_true (pango_bidi_type_for_unichar (0x2029) == PANGO_BIDI_TYPE_B);
  g_assert_true (pango_bidi_type_for_unichar (0x000b) == PANGO_BIDI_TYPE_S);
  g_assert_true (pango_bidi_type_for_unichar (' ') == PANGO_BIDI_TYPE_WS);
  g_assert_true (pango_bidi_type_for_unichar ('!') == PANGO_BIDI_TYPE_ON);
  /* these are new */
  g_assert_true (pango_bidi_type_for_unichar (0x2066) == PANGO_BIDI_TYPE_LRI);
  g_assert_true (pango_bidi_type_for_unichar (0x2067) == PANGO_BIDI_TYPE_RLI);
  g_assert_true (pango_bidi_type_for_unichar (0x2068) == PANGO_BIDI_TYPE_FSI);
  g_assert_true (pango_bidi_type_for_unichar (0x2069) == PANGO_BIDI_TYPE_PDI);
}

static void
test_unichar_direction (void)
{
  struct {
    gunichar ch;
    PangoDirection dir;
  } tests[] = {
    { 'a', PANGO_DIRECTION_LTR },
    { '0', PANGO_DIRECTION_NEUTRAL },
    { '.', PANGO_DIRECTION_NEUTRAL },
    { '(', PANGO_DIRECTION_NEUTRAL },
    { 0x05d0, PANGO_DIRECTION_RTL },
  };

  for (int i = 0; i < G_N_ELEMENTS (tests); i++)
    {
      g_assert_true (pango_unichar_direction (tests[i].ch) == tests[i].dir);
    }
}

G_GNUC_END_IGNORE_DEPRECATIONS

static void
test_bidi_embedding_levels (void)
{
  /* Examples taken from https://www.w3.org/International/articles/inline-bidi-markup/uba-basics */
  struct {
    const char *text;
    PangoDirection dir;
    const char *levels;
    PangoDirection out_dir;
  } tests[] = {
    { "bahrain مصر kuwait", PANGO_DIRECTION_LTR, "\0\0\0\0\0\0\0\0\1\1\1\0\0\0\0\0\0\0", PANGO_DIRECTION_LTR },
    { "bahrain مصر kuwait", PANGO_DIRECTION_WEAK_LTR, "\0\0\0\0\0\0\0\0\1\1\1\0\0\0\0\0\0\0", PANGO_DIRECTION_LTR },
    { "bahrain مصر kuwait", PANGO_DIRECTION_RTL, "\2\2\2\2\2\2\2\1\1\1\1\1\2\2\2\2\2\2", PANGO_DIRECTION_RTL },
    { "bahrain مصر kuwait", PANGO_DIRECTION_WEAK_RTL, "\0\0\0\0\0\0\0\0\1\1\1\0\0\0\0\0\0\0", PANGO_DIRECTION_LTR },
    { "The title is مفتاح معايير الويب in Arabic.", PANGO_DIRECTION_LTR, "\0\0\0\0\0\0\0\0\0\0\0\0\0\1\1\1\1\1\1\1\1\1\1\1\1\1\1\1\1\1\1\0\0\0\0\0\0\0\0\0\0\0", PANGO_DIRECTION_LTR },
    { "The title is مفتاح معايير الويب, in Arabic.", PANGO_DIRECTION_LTR, "\0\0\0\0\0\0\0\0\0\0\0\0\0\1\1\1\1\1\1\1\1\1\1\1\1\1\1\1\1\1\1\0\0\0\0\0\0\0\0\0\0\0\0", PANGO_DIRECTION_LTR },
    { "The title is مفتاح معايير الويب⁧!⁩ in Arabic.", PANGO_DIRECTION_LTR, "\0\0\0\0\0\0\0\0\0\0\0\0\0\1\1\1\1\1\1\1\1\1\1\1\1\1\1\1\1\1\1\0\1\0\0\0\0\0\0\0\0\0\0\0", PANGO_DIRECTION_LTR }, // FIXME 
    { "one two ثلاثة 1234 خمسة", PANGO_DIRECTION_LTR, "\0\0\0\0\0\0\0\0\1\1\1\1\1\1\2\2\2\2\1\1\1\1\1", PANGO_DIRECTION_LTR },
    { "one two ثلاثة ١٢٣٤ خمسة", PANGO_DIRECTION_LTR, "\0\0\0\0\0\0\0\0\1\1\1\1\1\1\2\2\2\2\1\1\1\1\1", PANGO_DIRECTION_LTR },

  };

  for (int i = 0; i < G_N_ELEMENTS (tests); i++)
    {
      const char *text = tests[i].text;
      PangoDirection dir = tests[i].dir;
      guint8 *levels;
      gsize len;

      levels = pango_log2vis_get_embedding_levels (text, -1, &dir);

      len = g_utf8_strlen (text, -1);

      if (memcmp (levels, tests[i].levels, sizeof (guint8) * len) != 0)
        {
        for (int j = 0; j < len; j++)
          g_print ("\\%d", levels[j]);
        g_print ("\n");
        }
      g_assert_true (memcmp (levels, tests[i].levels, sizeof (guint8) * len) == 0);
      g_assert_true (dir == tests[i].out_dir);

      g_free (levels);
    }
}

int
main (int argc, char *argv[])
{
  g_test_init (&argc, &argv, NULL);

  g_test_add_func ("/bidi/mirror-char", test_mirror_char);
  g_test_add_func ("/bidi/type-for-unichar", test_bidi_type_for_unichar);
  g_test_add_func ("/bidi/unichar-direction", test_unichar_direction);
  g_test_add_func ("/bidi/embedding-levels", test_bidi_embedding_levels);

  return g_test_run ();
}
