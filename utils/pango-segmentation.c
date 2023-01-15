/* Pango
 * pango-segmentation.c: Test Pango line breaking
 *
 * Copyright (C) 2021 Red Hat, Inc
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

#include <glib.h>
#include <pango/pangocairo.h>
#include <string.h>
#include <stdlib.h>
#include <locale.h>

#ifndef G_OS_WIN32
#include <unistd.h>
#endif

typedef enum {
  GRAPHEME,
  WORD,
  WORD_START_END,
  LINE,
  SENTENCE,
  SENTENCE_START_END,
} BreakKind;

static BreakKind
kind_from_string (const char *str)
{
  if (strcmp (str, "grapheme") == 0)
    return GRAPHEME;
  else if (strcmp (str, "word") == 0)
    return WORD;
  else if (strcmp (str, "words") == 0)
    return WORD_START_END;
  else if (strcmp (str, "line") == 0)
    return LINE;
  else if (strcmp (str, "sentence") == 0)
    return SENTENCE;
  else if (strcmp (str, "sentences") == 0)
    return SENTENCE_START_END;
  else
    {
      g_printerr ("Not a segmentation: %s", str);
      return 0;
    }
}

enum {
  NONE,
  BREAK,
  START,
  END
};

static gboolean
show_segmentation (const char *input,
                   BreakKind   kind)
{
  GString *string;
  PangoContext *context;
  gsize  length;
  GError *error = NULL;
  PangoLogAttr *attrs;
  int len;
  char *p;
  int i;
  char *text;
  PangoAttrList *attributes;
  PangoLayout *layout;

  context = pango_font_map_create_context (pango_cairo_font_map_get_default ());

  string = g_string_new ("");

  length = strlen (input);
  len = g_utf8_strlen (input, -1) + 1;

  pango_parse_markup (input, -1, 0, &attributes, &text, NULL, &error);
  g_assert_no_error (error);

  layout = pango_layout_new (context);
  pango_layout_set_text (layout, text, length);
  pango_layout_set_attributes (layout, attributes);

  pango_layout_get_log_attrs (layout, &attrs, &len);

  for (i = 0, p = text; i < len; i++, p = g_utf8_next_char (p))
    {
      PangoLogAttr log = attrs[i];
      int indicator = NONE;

      switch (kind)
        {
        case GRAPHEME:
          if (log.is_cursor_position)
            indicator = BREAK;
          break;
        case WORD:
          if (log.is_word_boundary)
            indicator = BREAK;
          break;
        case WORD_START_END:
          if (log.is_word_start && log.is_word_end)
            indicator = BREAK;
          else if (log.is_word_start)
            indicator = START;
          else if (log.is_word_end)
            indicator = END;
          break;
        case LINE:
          if (log.is_line_break)
            indicator = BREAK;
          break;
        case SENTENCE:
          if (log.is_sentence_boundary)
            indicator = BREAK;
          break;
        case SENTENCE_START_END:
          if (log.is_sentence_start && log.is_sentence_end)
            indicator = BREAK;
          else if (log.is_sentence_start)
            indicator = START;
          else if (log.is_sentence_end)
            indicator = END;
          break;
        default:
          g_assert_not_reached ();
        }

      switch (indicator)
        {
        case BREAK:
          g_string_append (string, "|");
          break;
        case START:
          g_string_append (string, "⌊");
          break;
        case END:
          g_string_append (string, "⌋");
          break;
        case NONE:
        default:
          break;
        }

      if (i < len - 1)
        {
          gunichar ch = g_utf8_get_char (p);
          if (ch == 0x20)
            g_string_append (string, " ");
          else if (g_unichar_isgraph (ch) ||
                   ch == '\n' ||
                   g_unichar_type (ch) == G_UNICODE_LINE_SEPARATOR ||
                   g_unichar_type (ch) == G_UNICODE_PARAGRAPH_SEPARATOR)
            g_string_append_unichar (string, ch);
          else
            g_string_append_printf (string, "[%#04x]", ch);
        }
    }

  g_object_unref (layout);
  g_free (attrs);
  g_free (text);
  pango_attr_list_unref (attributes);

  g_print ("%s\n", string->str);

  g_string_free (string, TRUE);

  return TRUE;
}

int
main (int argc, char *argv[])
{
  const char *opt_kind = "grapheme";
  const char *opt_text = NULL;
  gboolean opt_version = FALSE;
  GOptionEntry entries[] = {
    { "kind", 0, 0, G_OPTION_ARG_STRING, &opt_kind, "Boundary (grapheme/word/words/line/sentence/sentences)", "KIND" },
    { "text", 0, 0, G_OPTION_ARG_STRING, &opt_text, "Text to display", "STRING" },
    { "version", 0, 0, G_OPTION_ARG_NONE, &opt_version, "Show version" },
    { NULL, },
  };
  GOptionContext *context;
  GError *error = NULL;
  char *text;
  gsize len;

  g_set_prgname ("pango-segmentation");
  setlocale (LC_ALL, "");

  context = g_option_context_new ("[FILE]");
  g_option_context_add_main_entries (context, entries, NULL);
  g_option_context_set_description (context,
      "Show text segmentation as determined by Pango:\n"
      "  grapheme  - Cursor positions\n"
      "  word      - Word boundaries\n"
      "  words     - Word starts and ends\n"
      "  line      - Possible line breaks\n"
      "  sentence  - Sentence boundaries\n"
      "  sentences - Sentence starts and ends");
  if (!g_option_context_parse (context, &argc, &argv, &error))
    {
      g_printerr ("%s\n", error->message);
      exit (1);
    }

  if (opt_version)
    {
      g_print ("%s (%s) %s\n", g_get_prgname (), PACKAGE_NAME, PACKAGE_VERSION);
      exit (0);
    }

  if (opt_text)
    {
      text = (char *)opt_text;
    }
  else if (argc > 1)
    {
      if (!g_file_get_contents (argv[1], &text, &len, &error))
        {
          g_printerr ("%s\n", error->message);
          exit (1);
        }
    }
  else
    {
      g_printerr ("Usage: pango-segmentation [OPTIONS…] FILE\n");
      exit (1);
    }

  show_segmentation (text, kind_from_string (opt_kind));

  return 0;
}
