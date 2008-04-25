/* Pango
 * testboundaries_ucd.c: Test text boundary algorithms with test data from
 *                       Unicode Character Database.
 *
 * Copyright (C) 2003 Noah Levitt
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Library General Public
 * License as published by the Free Software Foundation; either
 * version 2 of the License, or (at your option) any later version.
 *
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Library General Public License for more details.
 *
 * You should have received a copy of the GNU Library General Public
 * License along with this library; if not, write to the
 * Free Software Foundation, Inc., 59 Temple Place - Suite 330,
 * Boston, MA 02111-1307, USA.
 */

#include <pango/pango.h>
#include <stdlib.h>
#include <string.h>
#include <locale.h>

static gboolean failed = FALSE;

/* PangoLogAttr has to be the same size as guint or this hack breaks */
typedef union
{
  PangoLogAttr attr;
  guint bits;
}
AttrBits;

/* counts the number of multiplication and divison signs up to the first
 * '#' or null character */
static gint
count_attrs (gchar *line)
{
  gunichar ch;
  gchar *p = line;
  gint count = 0;

  for (;;)
    {
      ch = g_utf8_get_char (p);

      switch (ch)
        {
        /* MULTIPLICATION SIGN, DIVISION SIGN */
        case 0x00d7: case 0x00f7:
          count++;
          break;

        /* null char, NUMBER SIGN */
        case 0x0000: case 0x0023:
          return count;

        default:
          break;
        }

      p = g_utf8_next_char (p);
    }
  /* not reached */
}

static gboolean
parse_line (gchar *line,
            AttrBits bits,
            gchar **str_return,
            PangoLogAttr **attr_return,
            gint *num_attrs)
{
  GString *gs;
  gunichar ch, character;
  gchar *p, *q;
  gint i;
  AttrBits temp_attr;

  *num_attrs = count_attrs (line);
  *attr_return = g_new (PangoLogAttr, *num_attrs);

  p = line;
  i = 0;
  gs = g_string_new (NULL);

  for (;;)
    {
      temp_attr.bits = 0;

      /* skip white space */
      do
        {
          ch = g_utf8_get_char (p);
          p = g_utf8_next_char (p);
        }
      while (g_unichar_isspace (ch));

      switch (ch)
        {
        case 0x00f7: /* DIVISION SIGN: boundary here */
          temp_attr.bits |= bits.bits;
          /* fall through */

        case 0x00d7: /* MULTIPLICATION SIGN: no boundary here */
          break;

        case 0x0000:
        case 0x0023: 
          *str_return = g_string_free (gs, FALSE);
          return TRUE;

        default: /* unexpected character */
          g_free (*attr_return);
          return FALSE;
        }

      (*attr_return)[i] = temp_attr.attr;

      /* skip white space */
      do
        {
          ch = g_utf8_get_char (p);
          p = g_utf8_next_char (p);
        }
      while (g_unichar_isspace (ch));
      p = g_utf8_prev_char (p);

      if (ch == 0x0023 || ch == 0x0000)
        {
          *str_return = g_string_free (gs, FALSE);
          return TRUE;
        }

      character = strtoul (p, &q, 16);
      if (q < p + 4 || q > p + 6 || character > 0x10ffff)
        {
          g_free (*attr_return);
          return FALSE;
        }

      p = q;

      gs = g_string_append_unichar (gs, character);

      i++;
    }
}

static gboolean
attrs_equal (PangoLogAttr *attrs1,
             PangoLogAttr *attrs2,
             gint len,
             AttrBits bits)
{
  AttrBits a, b;
  gint i;

  for (i = 0;  i < len;  i++)
    {
      a.bits = 0;
      a.attr = attrs1[i];

      b.bits = 0;
      b.attr = attrs2[i];

      /* can't do a straight comparison because the bitmask may have
       * multiple bits set, and as long as attr&bitmask is not zero, it
       * counts as being set */
      if (((a.bits & bits.bits) && !(b.bits & bits.bits)) ||
          !(a.bits & bits.bits) && (b.bits & bits.bits))
        return FALSE;
    }

  return TRUE;
}

static gchar *
make_test_string (gchar *string, 
                  PangoLogAttr *attrs, 
                  AttrBits bits)
{
  GString *gs = g_string_new (NULL);
  gint i = 0;
  AttrBits a;
  gchar *p = string;
  gunichar ch;

  for (;;)
    {
      a.bits = 0;
      a.attr = attrs[i];
      if ((a.bits & bits.bits) != 0)
        gs = g_string_append_unichar (gs, 0x00f7);
      else
        gs = g_string_append_unichar (gs, 0x00d7);

      g_string_append_c (gs, ' ');

      if (*p == '\0')
        break;

      ch = g_utf8_get_char (p);
      g_string_append_printf (gs, "%04X ", ch);

      p = g_utf8_next_char (p);
      i++;
    }

  return g_string_free (gs, FALSE);
}

static void
do_test (gchar *filename,
         AttrBits bits,
	 gboolean fixup_broken_linebreaktest)
{
  GIOChannel *channel;
  GIOStatus status;
  gchar *line;
  gsize length, terminator_pos;
  GError *error;
  gchar *string;
  PangoLogAttr *expected_attrs;
  gint num_attrs;
  gint i;

  error = NULL;
  channel = g_io_channel_new_file (filename, "r", &error);
  if (!channel)
    {
      if (error->domain == G_FILE_ERROR && error->code == G_FILE_ERROR_NOENT)
        {
	  g_print ("%s not found.  Skipping test.\n", filename);
	  goto done;
	}
      else
        {
	  g_printerr ("%s: %s\n", filename, error->message);
	  exit (1);
	}
    }

  i = 1;
  for (;;)
    {
      error = NULL;
      status = g_io_channel_read_line (channel, &line, &length, &terminator_pos, &error);

      switch (status)
        {
          case G_IO_STATUS_ERROR:
            g_printerr ("%s: %s\n", filename, error->message);
            exit (1);

          case G_IO_STATUS_EOF:
	    goto done;

          case G_IO_STATUS_AGAIN:
            continue;

          case G_IO_STATUS_NORMAL:
            line[terminator_pos] = '\0';
            break;
        }

      if (! parse_line (line, bits, &string, &expected_attrs, &num_attrs))
        {
          g_printerr ("%s: error parsing line %d: %s\n", filename, i, line);
          exit (1);
        }
      
      if (num_attrs > 0)
        {
          PangoLogAttr *attrs = g_new (PangoLogAttr, num_attrs);
          pango_get_log_attrs (string, -1, 0, pango_language_from_string ("C"), attrs, num_attrs);

	  /* LineBreakTest.txt from Unicode 5.1.0 has this bug that it says
	   * breaking is allowed at the beginning of the strings, while the
	   * algorithm says it's not.  Fix that up. */
	  if (fixup_broken_linebreaktest)
	    memset (expected_attrs, 0, sizeof (expected_attrs[0]));

          if (! attrs_equal (attrs, expected_attrs, num_attrs, bits))
            {
              gchar *str = make_test_string (string, attrs, bits);
              gchar *comments = strchr (line, '#');
              if (comments) /* don't print the # comment in the error message.  print it separately */
	        {
		  *comments = '\0';
		  comments++;
		}
	      else
	        {
		  comments = "";
		}

              g_printerr ("%s: line %d failed\n"
                          "   expected: %s\n"
                          "   returned: %s\n"
			  "   comments: %s\n\n",
                          filename, i, line, str, comments);

              g_free (str);
              failed = TRUE;
            }
          g_free (attrs);
        }
      g_free (string);
      g_free (expected_attrs);

      i++;
    }

done:
  if (channel)
    g_io_channel_unref (channel);
  if (error)
    g_error_free (error);
  g_free (filename);
}

gint
main (gint argc,
      gchar **argv)
{
  gchar *srcdir;
  gchar *filename;
  AttrBits bits;

  setlocale (LC_ALL, "");

  srcdir = getenv ("srcdir");
  if (!srcdir)
    srcdir = ".";

  filename = g_strdup_printf ("%s/GraphemeBreakTest.txt", srcdir);
  bits.bits = 0;
  bits.attr.is_cursor_position = 1;
  do_test (filename, bits, FALSE);

  filename = g_strdup_printf ("%s/WordBreakTest.txt", srcdir);
  bits.bits = 0;
  bits.attr.is_word_boundary = 1;
  do_test (filename, bits, FALSE);

  filename = g_strdup_printf ("%s/SentenceBreakTest.txt", srcdir);
  bits.bits = 0;
  bits.attr.is_sentence_boundary = 1;
  do_test (filename, bits, FALSE);

  filename = g_strdup_printf ("%s/LineBreakTest.txt", srcdir);
  bits.bits = 0;
  bits.attr.is_line_break = 1;
  bits.attr.is_mandatory_break = 1;
  do_test (filename, bits, TRUE);

  exit (failed);
}
