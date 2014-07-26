/* Pango
 * dump-boundaries.c: Dump text boundaries for a file
 *
 * Copyright (C) 1999-2000 Red Hat Software
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

#include <string.h>
#include <stdlib.h>
#include <stdio.h>
#include <unistd.h>

#include <pango/pango.h>

#define CHFORMAT "%0#6x"

static void fail (const char *format, ...) G_GNUC_PRINTF (1, 2) G_GNUC_NORETURN;
static void fail (const char *format, ...)
{
  char *str;

  va_list args;

  va_start (args, format);
  str = g_strdup_vprintf (format, args);
  va_end (args);

  fprintf (stderr, "Error: %s\n", str);

  g_free (str);

  exit (1);
}

static void
dump_text (const char *text)
{
  unsigned int len;
  PangoLogAttr *attrs;
  unsigned int i;
  gunichar *ucs4;

  if (!g_utf8_validate (text, -1, NULL))
    fail ("Invalid UTF-8 in file");

  len = g_utf8_strlen (text, -1);
  attrs = g_new0 (PangoLogAttr, len + 1);

  pango_get_log_attrs (text,
		       -1,
		       0,
		       pango_language_from_string ("C"),
		       attrs,
		       len + 1);

  ucs4 = g_utf8_to_ucs4 (text, -1, NULL, NULL, NULL);

  for (i = 0; i < len + 1; i++)
    {
      char buf[7] = { '\0', };
      char *loc;

      g_unichar_to_utf8 (ucs4[i], buf);

      if (*buf == '\n')
	loc = g_strdup ("\\n");
      else if (*buf == '\r')
	loc = g_strdup ("\\r");
      else
	loc = g_locale_from_utf8 (buf, -1, NULL, NULL, NULL);

      g_print (CHFORMAT " (%s):\t line_break = %d mandatory_break = %d char_break = %d\n"
	       "     \t\t white = %d cursor_position = %d\n"
	       "     \t\t word_start = %d word_end = %d\n"
	       "     \t\t sentence_boundary = %d sentence_start = %d sentence_end = %d\n",
	       ucs4[i], loc ? loc : "?",
	       attrs[i].is_line_break,
	       attrs[i].is_mandatory_break,
	       attrs[i].is_char_break,
	       attrs[i].is_white,
	       attrs[i].is_cursor_position,
	       attrs[i].is_word_start,
	       attrs[i].is_word_end,
	       attrs[i].is_sentence_boundary,
	       attrs[i].is_sentence_start,
	       attrs[i].is_sentence_end);

      g_free (loc);
    }

  g_free (ucs4);
  g_free (attrs);
}

int
main (int    argc,
      char **argv)
{
  gchar *text;

  if (argc < 2)
    fail ("must give a filename on the command line");

  if (!g_file_get_contents (argv[1], &text, NULL, NULL))
    fail ("Couldn't open sample text file");

  dump_text (text);

  g_free (text);

  return 0;
}

