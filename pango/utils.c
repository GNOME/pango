/* Pango
 * utils.c:
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

#include "utils.h"
#include <iconv.h>
#include <errno.h>
#include <unicode.h>

gboolean
_pango_utf8_iterate (const char *cur, const char **next, GUChar4 *wc_out)
{
  const guchar *p = (guchar *)cur;
  guchar c = *p;
  GUChar4 wc;
  gint length;

  if ((c & 0x80) == 0)
    {
      length = 1;
      wc = c;
    }
  else if ((c & 0xc0) == 0x80)
    {
      return FALSE;
    }
  else if ((c & 0xe0) == 0xc0)
    {
      length = 2;
      wc = c & 0x1f;
    }
  else if ((c & 0xf0) == 0xe0)
    {
      length = 3;
      wc = c & 0x0f;
    }
  else
    return FALSE;

  p++;
  while (--length > 0)
    {
      if (*p == 0)		/* Incomplete character */
	{
	  if (next)
	    *next = cur;
	  if (wc_out)
	    *wc_out = 0;
	  return TRUE;
	}

      if ((*p & 0xc0) != 0x80)
	return FALSE;

      wc <<= 6;
      wc |= (*p) & 0x3f;

      p++;
    }

  if (wc_out)
    *wc_out = wc;
  if (next)
    *next = (const char *)p;

  return TRUE;
}

int
_pango_utf8_len (const char *str, int limit)
{
  const char *cur = str;
  const char *next;
  int len = 0;
  
  while (*cur)
    {
      if (!_pango_utf8_iterate (cur, &next, NULL))
	return -1;
      if (cur == next)
	break;
      if (limit >= 0 && (next - str) > limit)
	return len;
      cur = next;
      len++;
    }

  return len;
}

#if G_BYTE_ORDER == G_LITTLE_ENDIAN
#define UCS2_CHARSET "UNICODELITTLE"
#else
#define UCS2_CHARSET "UNICODE"
#endif

GUChar2 *
_pango_utf8_to_ucs2 (const char *str, int len)
{
  iconv_t cd;
  char *outbuf, *result;
  const char *inbuf;
  size_t inbytesleft;
  size_t outbytesleft;
  gint outlen;

  gint count;

  cd = iconv_open (UCS2_CHARSET, "UTF8");
  
  if (cd == (iconv_t)-1)
    g_error ("No converter from UTF8 to " UCS2_CHARSET);

  if (len < 0)
    len = strlen (str);

  outlen = unicode_strlen (str, len) * sizeof(GUChar2);
  result = g_malloc (outlen);
      
  inbuf = str;
  inbytesleft = len;
  outbuf = result;
  outbytesleft = outlen;
  
  count = iconv (cd, &inbuf, &inbytesleft, &outbuf, &outbytesleft);

  if (count < 0 && (errno != E2BIG))
    {
      g_free (result);
      result = NULL;
    }

  iconv_close (cd);

  return (GUChar2 *)result;

}

/**
 * _pango_guchar4_to_utf8:
 * @ch: a ISO10646 character code
 * @out: output buffer, must have at least 6 bytes of space.
 * 
 * Convert a single character to utf8
 * 
 * Return value: number of bytes written
 **/
int
_pango_guchar4_to_utf8 (GUChar4 c, char *outbuf)
{
  size_t len = 0;
  int first;
  int i;

  if (c < 0x80)
    {
      first = 0;
      len = 1;
    }
  else if (c < 0x800)
    {
      first = 0xc0;
      len = 2;
    }
  else if (c < 0x10000)
    {
      first = 0xe0;
      len = 3;
    }
   else if (c < 0x200000)
    {
      first = 0xf0;
      len = 4;
    }
  else if (c < 0x4000000)
    {
      first = 0xf8;
      len = 5;
    }
  else
    {
      first = 0xfc;
      len = 6;
    }

  for (i = len - 1; i > 0; --i)
    {
      outbuf[i] = (c & 0x3f) | 0x80;
      c >>= 6;
    }
  outbuf[0] = c | first;

  return len;
}
