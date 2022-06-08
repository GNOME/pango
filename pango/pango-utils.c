/* Pango
 * pango-utils.c: Utilities for internal functions and modules
 *
 * Copyright (C) 2000 Red Hat Software
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
#include <errno.h>
#include <string.h>
#include <stdlib.h>
#include <math.h>
#include <locale.h>

#include "pango-font.h"
#include "pango-features.h"
#include "pango-impl-utils.h"
#include "pango-utils-internal.h"

#include <glib/gstdio.h>

#ifndef HAVE_FLOCKFILE
#  define flockfile(f) (void)1
#  define funlockfile(f) (void)1
#  define getc_unlocked(f) getc(f)
#endif /* !HAVE_FLOCKFILE */

#ifdef G_OS_WIN32

#include <sys/types.h>

#define STRICT
#include <windows.h>

#endif

/**
 * pango_version:
 *
 * Returns the encoded version of Pango available at run-time.
 *
 * This is similar to the macro %PANGO_VERSION except that the macro
 * returns the encoded version available at compile-time. A version
 * number can be encoded into an integer using PANGO_VERSION_ENCODE().
 *
 * Returns: The encoded version of Pango library available at run time.
 *
 * Since: 1.16
 */
int
pango_version (void)
{
  return PANGO_VERSION;
}

/**
 * pango_version_string:
 *
 * Returns the version of Pango available at run-time.
 *
 * This is similar to the macro %PANGO_VERSION_STRING except that the
 * macro returns the version available at compile-time.
 *
 * Returns: A string containing the version of Pango library available
 *   at run time. The returned string is owned by Pango and should not
 *   be modified or freed.
 *
 * Since: 1.16
 */
const char *
pango_version_string (void)
{
  return PANGO_VERSION_STRING;
}

/**
 * pango_version_check:
 * @required_major: the required major version
 * @required_minor: the required minor version
 * @required_micro: the required major version
 *
 * Checks that the Pango library in use is compatible with the
 * given version.
 *
 * Generally you would pass in the constants %PANGO_VERSION_MAJOR,
 * %PANGO_VERSION_MINOR, %PANGO_VERSION_MICRO as the three arguments
 * to this function; that produces a check that the library in use at
 * run-time is compatible with the version of Pango the application or
 * module was compiled against.
 *
 * Compatibility is defined by two things: first the version
 * of the running library is newer than the version
 * @required_major.required_minor.@required_micro. Second
 * the running library must be binary compatible with the
 * version @required_major.required_minor.@required_micro
 * (same major version.)
 *
 * For compile-time version checking use PANGO_VERSION_CHECK().
 *
 * Return value: (nullable): %NULL if the Pango library is compatible
 *   with the given version, or a string describing the version
 *   mismatch.  The returned string is owned by Pango and should not
 *   be modified or freed.
 *
 * Since: 1.16
 */
const gchar*
pango_version_check (int required_major,
		     int required_minor,
		     int required_micro)
{
  gint pango_effective_micro = 100 * PANGO_VERSION_MINOR + PANGO_VERSION_MICRO;
  gint required_effective_micro = 100 * required_minor + required_micro;

  if (required_major > PANGO_VERSION_MAJOR)
    return "Pango version too old (major mismatch)";
  if (required_major < PANGO_VERSION_MAJOR)
    return "Pango version too new (major mismatch)";
  if (required_effective_micro < pango_effective_micro - PANGO_BINARY_AGE)
    return "Pango version too new (micro mismatch)";
  if (required_effective_micro > pango_effective_micro)
    return "Pango version too old (micro mismatch)";
  return NULL;
}

char *
_pango_trim_string (const char *str)
{
  int len;

  g_return_val_if_fail (str != NULL, NULL);

  while (*str && g_ascii_isspace (*str))
    str++;

  len = strlen (str);
  while (len > 0 && g_ascii_isspace (str[len-1]))
    len--;

  return g_strndup (str, len);
}

gboolean
_pango_scan_int (const char **pos, int *out)
{
  char *end;
  long temp;

  errno = 0;
  temp = strtol (*pos, &end, 10);
  if (errno == ERANGE)
    {
      errno = 0;
      return FALSE;
    }

  *out = (int)temp;
  if ((long)(*out) != temp)
    {
      return FALSE;
    }

  *pos = end;

  return TRUE;
}

static gboolean
parse_int (const char *word,
	   int        *out)
{
  char *end;
  long val;
  int i;

  if (word == NULL)
    return FALSE;

  val = strtol (word, &end, 10);
  i = val;

  if (end != word && *end == '\0' && val >= 0 && val == i)
    {
      if (out)
        *out = i;

      return TRUE;
    }

  return FALSE;
}

gboolean
_pango_parse_enum (GType       type,
		   const char *str,
 		   int        *value,
		   gboolean    warn,
		   char      **possible_values)
{
  GEnumClass *class = NULL;
  gboolean ret = TRUE;
  GEnumValue *v = NULL;

  class = g_type_class_ref (type);

  if (G_LIKELY (str))
    v = g_enum_get_value_by_nick (class, str);

  if (v)
    {
      if (G_LIKELY (value))
	*value = v->value;
    }
  else if (!parse_int (str, value))
    {
      ret = FALSE;
      if (G_LIKELY (warn || possible_values))
	{
	  int i;
	  GString *s = g_string_new (NULL);

	  for (i = 0, v = g_enum_get_value (class, i); v;
	       i++  , v = g_enum_get_value (class, i))
	    {
	      if (i)
		g_string_append_c (s, '/');
	      g_string_append (s, v->value_nick);
	    }

	  if (warn)
	    g_warning ("%s must be one of %s",
		       G_ENUM_CLASS_TYPE_NAME(class),
		       s->str);

	  if (possible_values)
	    *possible_values = s->str;

	  g_string_free (s, possible_values ? FALSE : TRUE);
	}
    }

  g_type_class_unref (class);

  return ret;
}

gboolean
pango_parse_flags (GType        type,
                   const char  *str,
                   int         *value,
                   char       **possible_values)
{
  GFlagsClass *class = NULL;
  gboolean ret = TRUE;
  GFlagsValue *v = NULL;

  class = g_type_class_ref (type);

  v = g_flags_get_value_by_nick (class, str);

  if (v)
    {
      *value = v->value;
    }
  else if (!parse_int (str, value))
    {
      char **strv = g_strsplit (str, "|", 0);
      int i;

      *value = 0;

      for (i = 0; strv[i]; i++)
        {
          strv[i] = g_strstrip (strv[i]);
          v = g_flags_get_value_by_nick (class, strv[i]);
          if (!v)
            {
              ret = FALSE;
              break;
            }
          *value |= v->value;
        }
      g_strfreev (strv);

      if (!ret && possible_values)
	{
	  int i;
	  GString *s = g_string_new (NULL);

          for (i = 0; i < class->n_values; i++)
            {
              v = &class->values[i];
              if (i)
                g_string_append_c (s, '/');
              g_string_append (s, v->value_nick);
            }

          *possible_values = s->str;

          g_string_free (s, FALSE);
	}
    }

  g_type_class_unref (class);

  return ret;
}

/**
 * pango_is_zero_width:
 * @ch: a Unicode character
 *
 * Checks if a character that should not be normally rendered.
 *
 * This includes all Unicode characters with "ZERO WIDTH" in their name,
 * as well as *bidi* formatting characters, and a few other ones.
 *
 * This is totally different from [func@GLib.unichar_iszerowidth] and is at best misnamed.
 *
 * Return value: %TRUE if @ch is a zero-width character, %FALSE otherwise
 *
 * Since: 1.10
 */
gboolean
pango_is_zero_width (gunichar ch)
{
/* Zero Width characters:
 *
 *  00AD  SOFT HYPHEN
 *  034F  COMBINING GRAPHEME JOINER
 *
 *  200B  ZERO WIDTH SPACE
 *  200C  ZERO WIDTH NON-JOINER
 *  200D  ZERO WIDTH JOINER
 *  200E  LEFT-TO-RIGHT MARK
 *  200F  RIGHT-TO-LEFT MARK
 *
 *  2028  LINE SEPARATOR
 *
 *  2060  WORD JOINER
 *  2061  FUNCTION APPLICATION
 *  2062  INVISIBLE TIMES
 *  2063  INVISIBLE SEPARATOR
 *
 *  2066  LEFT-TO-RIGHT ISOLATE
 *  2067  RIGHT-TO-LEFT ISOLATE
 *  2068  FIRST STRONG ISOLATE
 *  2069  POP DIRECTIONAL ISOLATE
 *
 *  202A  LEFT-TO-RIGHT EMBEDDING
 *  202B  RIGHT-TO-LEFT EMBEDDING
 *  202C  POP DIRECTIONAL FORMATTING
 *  202D  LEFT-TO-RIGHT OVERRIDE
 *  202E  RIGHT-TO-LEFT OVERRIDE
 *
 *  FEFF  ZERO WIDTH NO-BREAK SPACE
 */
  return ((ch & ~(gunichar)0x007F) == 0x2000 && (
		(ch >= 0x200B && ch <= 0x200F) ||
		(ch >= 0x202A && ch <= 0x202E) ||
		(ch >= 0x2060 && ch <= 0x2063) ||
                (ch >= 0x2066 && ch <= 0x2069) ||
		(ch == 0x2028)
	 )) || G_UNLIKELY (ch == 0x00AD
			|| ch == 0x034F
			|| ch == 0xFEFF);
}

/**
 * pango_quantize_line_geometry:
 * @thickness: (inout): pointer to the thickness of a line, in Pango units
 * @position: (inout): corresponding position
 *
 * Quantizes the thickness and position of a line to whole device pixels.
 *
 * This is typically used for underline or strikethrough. The purpose of
 * this function is to avoid such lines looking blurry.
 *
 * Care is taken to make sure @thickness is at least one pixel when this
 * function returns, but returned @position may become zero as a result
 * of rounding.
 *
 * Since: 1.12
 */
void
pango_quantize_line_geometry (int *thickness,
			      int *position)
{
  int thickness_pixels = (*thickness + PANGO_SCALE / 2) / PANGO_SCALE;
  if (thickness_pixels == 0)
    thickness_pixels = 1;

  if (thickness_pixels & 1)
    {
      int new_center = ((*position - *thickness / 2) & ~(PANGO_SCALE - 1)) + PANGO_SCALE / 2;
      *position = new_center + (PANGO_SCALE * thickness_pixels) / 2;
    }
  else
    {
      int new_center = ((*position - *thickness / 2 + PANGO_SCALE / 2) & ~(PANGO_SCALE - 1));
      *position = new_center + (PANGO_SCALE * thickness_pixels) / 2;
    }

  *thickness = thickness_pixels * PANGO_SCALE;
}

/**
 * pango_units_from_double:
 * @d: double floating-point value
 *
 * Converts a floating-point number to Pango units.
 *
 * The conversion is done by multiplying @d by %PANGO_SCALE and
 * rounding the result to nearest integer.
 *
 * Return value: the value in Pango units.
 *
 * Since: 1.16
 */
int
pango_units_from_double (double d)
{
  return (int)floor (d * PANGO_SCALE + 0.5);
}

/**
 * pango_units_to_double:
 * @i: value in Pango units
 *
 * Converts a number in Pango units to floating-point.
 *
 * The conversion is done by dividing @i by %PANGO_SCALE.
 *
 * Return value: the double value.
 *
 * Since: 1.16
 */
double
pango_units_to_double (int i)
{
  return (double)i / PANGO_SCALE;
}

/**
 * pango_extents_to_pixels:
 * @inclusive: (nullable): rectangle to round to pixels inclusively
 * @nearest: (nullable): rectangle to round to nearest pixels
 *
 * Converts extents from Pango units to device units.
 *
 * The conversion is done by dividing by the %PANGO_SCALE factor and
 * performing rounding.
 *
 * The @inclusive rectangle is converted by flooring the x/y coordinates
 * and extending width/height, such that the final rectangle completely
 * includes the original rectangle.
 *
 * The @nearest rectangle is converted by rounding the coordinates
 * of the rectangle to the nearest device unit (pixel).
 *
 * The rule to which argument to use is: if you want the resulting device-space
 * rectangle to completely contain the original rectangle, pass it in as
 * @inclusive. If you want two touching-but-not-overlapping rectangles stay
 * touching-but-not-overlapping after rounding to device units, pass them in
 * as @nearest.
 *
 * Since: 1.16
 */
void
pango_extents_to_pixels (PangoRectangle *inclusive,
			 PangoRectangle *nearest)
{
  if (inclusive)
    {
      int orig_x = inclusive->x;
      int orig_y = inclusive->y;

      inclusive->x = PANGO_PIXELS_FLOOR (inclusive->x);
      inclusive->y = PANGO_PIXELS_FLOOR (inclusive->y);

      inclusive->width  = PANGO_PIXELS_CEIL (orig_x + inclusive->width ) - inclusive->x;
      inclusive->height = PANGO_PIXELS_CEIL (orig_y + inclusive->height) - inclusive->y;
    }

  if (nearest)
    {
      int orig_x = nearest->x;
      int orig_y = nearest->y;

      nearest->x = PANGO_PIXELS (nearest->x);
      nearest->y = PANGO_PIXELS (nearest->y);

      nearest->width  = PANGO_PIXELS (orig_x + nearest->width ) - nearest->x;
      nearest->height = PANGO_PIXELS (orig_y + nearest->height) - nearest->y;
    }
}

/**
 * pango_find_paragraph_boundary:
 * @text: UTF-8 text
 * @length: length of @text in bytes, or -1 if nul-terminated
 * @paragraph_delimiter_index: (out): return location for index of
 *   delimiter
 * @next_paragraph_start: (out): return location for start of next
 *   paragraph
 *
 * Locates a paragraph boundary in @text.
 *
 * A boundary is caused by delimiter characters, such as
 * a newline, carriage return, carriage return-newline pair,
 * or Unicode paragraph separator character.
 *
 * The index of the run of delimiters is returned in
 * @paragraph_delimiter_index. The index of the start of the
 * next paragraph (index after all delimiters) is stored n
 * @next_paragraph_start.
 *
 * If no delimiters are found, both @paragraph_delimiter_index
 * and @next_paragraph_start are filled with the length of @text
 * (an index one off the end).
 */
void
pango_find_paragraph_boundary (const char *text,
                               int         length,
                               int        *paragraph_delimiter_index,
                               int        *next_paragraph_start)
{
  const char *p = text;
  const char *end;
  const char *start = NULL;
  const char *delimiter = NULL;

  /* Only one character has type G_UNICODE_PARAGRAPH_SEPARATOR in
   * Unicode 5.0; update the following code if that changes.
   */

  /* prev_sep is the first byte of the previous separator.  Since
   * the valid separators are \r, \n, and PARAGRAPH_SEPARATOR, the
   * first byte is enough to identify it.
   */
  char prev_sep;

#define PARAGRAPH_SEPARATOR_STRING "\xE2\x80\xA9"

  if (length < 0)
    length = strlen (text);

  end = text + length;

  if (paragraph_delimiter_index)
    *paragraph_delimiter_index = length;

  if (next_paragraph_start)
    *next_paragraph_start = length;

  if (length == 0)
    return;

  prev_sep = 0;
  while (p < end)
    {
      if (prev_sep == '\n' ||
          prev_sep == PARAGRAPH_SEPARATOR_STRING[0])
        {
          g_assert (delimiter);
          start = p;
          break;
        }
      else if (prev_sep == '\r')
        {
          /* don't break between \r and \n */
          if (*p != '\n')
            {
              g_assert (delimiter);
              start = p;
              break;
            }
        }

      if (*p == '\n' ||
           *p == '\r' ||
           !strncmp(p, PARAGRAPH_SEPARATOR_STRING, strlen (PARAGRAPH_SEPARATOR_STRING)))
        {
          if (delimiter == NULL)
            delimiter = p;
          prev_sep = *p;
        }
      else
        prev_sep = 0;

      p = g_utf8_next_char (p);
    }

  if (delimiter && paragraph_delimiter_index)
    *paragraph_delimiter_index = delimiter - text;

  if (start && next_paragraph_start)
    *next_paragraph_start = start - text;
}
