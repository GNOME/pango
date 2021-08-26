/* Pango
 *
 * Copyright (C) 1999 Red Hat Software
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

#include "config.h"

#include "validate-log-attrs.h"
#include "pango.h"
#include <string.h>

/* {{{ Validation */

G_DEFINE_QUARK(pango-validate-error-quark, pango_validate_error)

typedef gboolean (* CharForeachFunc) (int                  pos,
                                      gunichar             wc,
                                      gunichar             prev_wc,
                                      gunichar             next_wc,
                                      GUnicodeType         type,
                                      GUnicodeType         prev_type,
                                      GUnicodeType         next_type,
                                      const PangoLogAttr  *attr,
                                      const PangoLogAttr  *prev_attr,
                                      const PangoLogAttr  *next_attr,
                                      gboolean            *after_zws,
                                      GError             **error);

static gboolean
log_attr_foreach (const char          *text,
                  int                  length,
                  const PangoLogAttr  *attrs,
                  int                  attrs_len,
                  CharForeachFunc      func,
                  GError             **error)
{
  const gchar *next = text;
  const gchar *end = text + length;
  gint i = 0;
  gunichar prev_wc;
  gunichar next_wc;
  GUnicodeType prev_type;
  GUnicodeType next_type;
  gboolean after_zws;

  if (next == end)
    goto done;

  prev_type = (GUnicodeType) -1;
  prev_wc = 0;

  next_wc = g_utf8_get_char (next);
  next_type = g_unichar_type (next_wc);

  after_zws = FALSE;

  while (next_wc != 0)
    {
      GUnicodeType type;
      gunichar wc;

      wc = next_wc;
      type = next_type;

      next = g_utf8_next_char (next);

      if (next >= end)
        next_wc = 0;
      else
        next_wc = g_utf8_get_char (next);

      if (next_wc)
        next_type = g_unichar_type (next_wc);

      if (!func (i,
                 wc, prev_wc, next_wc,
                 type, prev_type, next_type,
                 &attrs[i],
                 i != 0 ? &attrs[i - 1] : NULL,
                 &attrs[i + 1],
                 &after_zws,
                 error))
        return FALSE;

      prev_type = type;
      prev_wc = wc;
      i++;
    }

done:
  return TRUE;
}

static gboolean
check_line_char (int                  pos,
                 gunichar             wc,
                 gunichar             prev_wc,
                 gunichar             next_wc,
                 GUnicodeType         type,
                 GUnicodeType         prev_type,
                 GUnicodeType         next_type,
                 const PangoLogAttr  *attr,
                 const PangoLogAttr  *prev_attr,
                 const PangoLogAttr  *next_attr,
                 gboolean            *after_zws,
                 GError             **error)
{
  GUnicodeBreakType break_type;
  GUnicodeBreakType prev_break_type;

  break_type = g_unichar_break_type (wc);

  if (prev_wc)
    prev_break_type = g_unichar_break_type (prev_wc);
  else
    prev_break_type = G_UNICODE_BREAK_UNKNOWN;

  if (prev_break_type == G_UNICODE_BREAK_ZERO_WIDTH_SPACE ||
      (prev_break_type == G_UNICODE_BREAK_SPACE && *after_zws))
    *after_zws = TRUE;
  else
    *after_zws = FALSE;

  if (wc == '\n' && prev_wc == '\r')
    {
      if (attr->is_line_break)
        {
          g_set_error (error,
                       PANGO_VALIDATE_ERROR, PANGO_VALIDATE_ERROR_BREAK,
                       "char %#x %d: Do not break between \\r and \\n (LB5)", wc, pos);
          return FALSE;
        }
    }

  if (prev_wc == 0 && wc != 0)
    {
      if (attr->is_line_break)
        {
          g_set_error (error,
                       PANGO_VALIDATE_ERROR, PANGO_VALIDATE_ERROR_BREAK,
                       "char %#x %d: Do not break before first char (LB2)", wc, pos);
          return FALSE;
        }
    }

  if (next_wc == 0)
    {
      if (!next_attr->is_line_break)
        {
          g_set_error (error,
                       PANGO_VALIDATE_ERROR, PANGO_VALIDATE_ERROR_BREAK,
                       "char %#x %d: Always break after the last char (LB3)", wc, pos);
          return FALSE;
        }
    }

  if (prev_break_type == G_UNICODE_BREAK_MANDATORY)
    {
      if (!attr->is_mandatory_break)
        {
          g_set_error (error,
                       PANGO_VALIDATE_ERROR, PANGO_VALIDATE_ERROR_BREAK,
                       "char %#x %d: Always break after hard line breaks (LB4)", wc, pos);
          return FALSE;
        }
    }

  if (prev_break_type == G_UNICODE_BREAK_CARRIAGE_RETURN ||
      prev_break_type == G_UNICODE_BREAK_LINE_FEED ||
      prev_break_type == G_UNICODE_BREAK_NEXT_LINE)
    {
      if (!attr->is_mandatory_break)
        {
          g_set_error (error,
                       PANGO_VALIDATE_ERROR, PANGO_VALIDATE_ERROR_BREAK,
                       "char %#x %d: Always break after CR, LF and NL (LB5)", wc, pos);
          return FALSE;
        }
    }

  if (break_type == G_UNICODE_BREAK_MANDATORY ||
      break_type == G_UNICODE_BREAK_CARRIAGE_RETURN ||
      break_type == G_UNICODE_BREAK_LINE_FEED ||
      break_type == G_UNICODE_BREAK_NEXT_LINE)
    {
          if (attr->is_line_break)
            {
              g_set_error (error,
                           PANGO_VALIDATE_ERROR, PANGO_VALIDATE_ERROR_BREAK,
                           "char %#x %d: Do not break before hard line beaks (LB6)", wc, pos);
              return FALSE;
            }
    }

  if (break_type == G_UNICODE_BREAK_SPACE ||
      break_type == G_UNICODE_BREAK_ZERO_WIDTH_SPACE)
    {
      if (attr->is_line_break && prev_attr != NULL &&
          !attr->is_mandatory_break &&
          !(next_wc && g_unichar_break_type (next_wc) == G_UNICODE_BREAK_COMBINING_MARK))
        {
          g_set_error (error,
                       PANGO_VALIDATE_ERROR, PANGO_VALIDATE_ERROR_BREAK,
                       "char %#x %d: Can't break before a space unless mandatory precedes or combining mark follows (LB7)", wc, pos);
          return FALSE;
        }
    }

  if (break_type != G_UNICODE_BREAK_ZERO_WIDTH_SPACE &&
      break_type != G_UNICODE_BREAK_SPACE &&
      *after_zws)
    {
      if (!attr->is_line_break)
        {
          g_set_error (error,
                       PANGO_VALIDATE_ERROR, PANGO_VALIDATE_ERROR_BREAK,
                       "char %#x %d: Break before a char following ZWS, even if spaces intervene (LB8)", wc, pos);
          return FALSE;
        }
    }

  if (break_type == G_UNICODE_BREAK_ZERO_WIDTH_JOINER)
    {
      if (attr->is_line_break)
        {
          g_set_error (error,
                       PANGO_VALIDATE_ERROR, PANGO_VALIDATE_ERROR_BREAK,
                       "char %#x %d: Do not break after ZWJ (LB8a)", wc, pos);
          return FALSE;
        }
    }

  /* TODO: check LB9 */

  if (prev_break_type == G_UNICODE_BREAK_WORD_JOINER ||
      break_type == G_UNICODE_BREAK_WORD_JOINER)
    {
      if (attr->is_line_break)
        {
          g_set_error (error,
                       PANGO_VALIDATE_ERROR, PANGO_VALIDATE_ERROR_BREAK,
                       "char %#x %d: Do not break before or after WJ (LB11)", wc, pos);
          return FALSE;
        }
    }

  if (prev_break_type == G_UNICODE_BREAK_NON_BREAKING_GLUE)
    {
          g_set_error (error,
                       PANGO_VALIDATE_ERROR, PANGO_VALIDATE_ERROR_BREAK,
                       "char %#x %d: Do not break after GL (LB12)", wc, pos);
          return FALSE;
    }

  /* internal consistency */

  if (attr->is_mandatory_break && !attr->is_line_break)
    {
      g_set_error (error,
                   PANGO_VALIDATE_ERROR, PANGO_VALIDATE_ERROR_BREAK,
                   "char %#x %d: Mandatory breaks must also be marked as regular breaks", wc, pos);
      return FALSE;
    }

  return TRUE;
}

static gboolean
check_line_invariants (const char          *text,
                       int                  length,
                       const PangoLogAttr  *attrs,
                       int                  attrs_len,
                       GError             **error)
{
  return log_attr_foreach (text, length,
                           attrs, attrs_len,
                           check_line_char, error);
}

static gboolean
check_grapheme_invariants (const char          *text,
                           int                  length,
                           const PangoLogAttr  *attrs,
                           int                  attrs_len,
                           GError             **error)
{
  return TRUE;
}

static gboolean
check_word_invariants (const char          *text,
                       int                  length,
                       const PangoLogAttr  *attrs,
                       int                  attrs_len,
                       GError             **error)
{
  enum {
    AFTER_START,
    AFTER_END
  } state = AFTER_END;

  for (int i = 0; i < attrs_len; i++)
    {
      /* Check that word starts and ends are alternating */
      switch (state)
        {
        case AFTER_END:
          if (attrs[i].is_word_start)
            {
              if (attrs[i].is_word_end)
                state = AFTER_END;
              else
                state = AFTER_START;
              break;
            }
          if (attrs[i].is_word_end)
            {
              g_set_error (error,
                           PANGO_VALIDATE_ERROR, PANGO_VALIDATE_ERROR_WORD,
                           "char %d: Unexpected word end", i);
              return FALSE;
            }
          break;

        case AFTER_START:
          if (attrs[i].is_word_end)
            {
              if (attrs[i].is_word_start)
                state = AFTER_START;
              else
                state = AFTER_END;
              break;
           }
          if (attrs[i].is_word_start)
            {
              g_set_error (error,
                           PANGO_VALIDATE_ERROR, PANGO_VALIDATE_ERROR_WORD,
                           "char %d: Unexpected word start", i);
              return FALSE;
            }
          break;

        default:
          g_assert_not_reached ();
        }

      /* Check that words don't end in the middle of graphemes */
      if (attrs[i].is_word_boundary && !attrs[i].is_cursor_position)
        {
          g_set_error (error,
                       PANGO_VALIDATE_ERROR, PANGO_VALIDATE_ERROR_SENTENCE,
                       "char %d: Word ends inside a grapheme", i);
          return FALSE;
        }
    }

  return TRUE;
}

static gboolean
check_sentence_invariants (const char          *text,
                           int                  length,
                           const PangoLogAttr  *attrs,
                           int                  attrs_len,
                           GError             **error)
{
  enum {
    AFTER_START,
    AFTER_END
  } state = AFTER_END;

  for (int i = 0; i < attrs_len; i++)
    {
      /* Check that word starts and ends are alternating */
      switch (state)
        {
        case AFTER_END:
          if (attrs[i].is_sentence_start)
            {
              if (attrs[i].is_sentence_end)
                state = AFTER_END;
              else
                state = AFTER_START;
              break;
            }
          if (attrs[i].is_sentence_end)
            {
              g_set_error (error,
                           PANGO_VALIDATE_ERROR, PANGO_VALIDATE_ERROR_SENTENCE,
                           "char %d: Unexpected sentence end", i);
              return FALSE;
            }
          break;

        case AFTER_START:
          if (attrs[i].is_sentence_end)
            {
              if (attrs[i].is_sentence_start)
                state = AFTER_START;
              else
                state = AFTER_END;
              break;
            }
          if (attrs[i].is_sentence_start)
            {
              g_set_error (error,
                           PANGO_VALIDATE_ERROR, PANGO_VALIDATE_ERROR_SENTENCE,
                           "char %d: Unexpected sentence start", i);
              return FALSE;
            }
          break;

        default:
          g_assert_not_reached ();
        }
    }

  return TRUE;
}

static gboolean
check_space_invariants (const char          *text,
                        int                  length,
                        const PangoLogAttr  *log_attrs,
                        int                  attrs_len,
                        GError             **error)
{
  for (int i = 0; i < attrs_len; i++)
    {
      if (log_attrs[i].is_expandable_space && !log_attrs[i].is_white)
        {
          g_set_error (error,
                       PANGO_VALIDATE_ERROR, PANGO_VALIDATE_ERROR_SPACE,
                       "char %d: Expandable space must be space", i);
          return FALSE;
        }
    }

  return TRUE;
}

/* }}} */
/* {{{ Public API */

/*
 * pango_validate_log_attrs:
 * @text: text to which @log_attrs belong
 * @length: length of @text
 * @log_attrs: `PangoLogAttr` array to validate
 * @attrs_len: length of @log_attrs
 *
 * Apply sanity checks to @log_attrs.
 *
 * This function checks some conditions that Pango
 * relies on. It is not guaranteed to be an exhaustive
 * validity test. Currentlty, it checks that
 *
 * - There's no break before the first char
 * - Mandatory breaks are line breaks
 * - Line breaks are char breaks
 * - Lines aren't broken between \\r and \\n
 * - Lines aren't broken before a space (unless the break
 *   is mandatory, or the space precedes a combining mark)
 * - Lines aren't broken between two open punctuation
 *   or between two close punctuation characters
 * - Lines aren't broken between a letter and a quotation mark
 * - Word starts and ends alternate
 * - Sentence starts and ends alternate
 * - Expandable spaces are spaces
 * - Words don't end in the middle of graphemes
 * - Sentences don't end in the middle of words
 *
 * Returns: %TRUE if @log_attrs are valid
 */
gboolean
pango_validate_log_attrs (const char          *text,
                          int                  length,
                          const PangoLogAttr  *log_attrs,
                          int                  attrs_len,
                          GError             **error)
{
  int n_chars;

  n_chars = g_utf8_strlen (text, length);
  if (attrs_len != n_chars + 1)
    {
      g_set_error_literal (error,
                           PANGO_VALIDATE_ERROR, PANGO_VALIDATE_ERROR_FAILED,
                           "Array has wrong length");
      return FALSE;
    }

  if (!check_line_invariants (text, length, log_attrs, attrs_len, error))
    return FALSE;

  if (!check_grapheme_invariants (text, length, log_attrs, attrs_len, error))
    return FALSE;

  if (!check_word_invariants (text, length, log_attrs, attrs_len, error))
    return FALSE;

  if (!check_sentence_invariants (text, length, log_attrs, attrs_len, error))
    return FALSE;

  if (!check_space_invariants (text, length, log_attrs, attrs_len, error))
    return FALSE;

  return TRUE;
}

 /* }}} */

/* vim:set foldmethod=marker expandtab: */
