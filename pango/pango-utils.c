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
 * pango_units_from_double:
 * @d: double floating-point value
 *
 * Converts a floating-point number to Pango units.
 *
 * The conversion is done by multiplying @d by %PANGO_SCALE and
 * rounding the result to nearest integer.
 *
 * Return value: the value in Pango units.
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
