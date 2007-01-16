/* Pango
 * test-color.c: Test program for pango_color_parse()
 *
 * Copyright (C) 2002 Matthias Clasen
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
#include <pango/pango.h>

typedef struct _ColorSpec {
  const gchar *spec;
  gboolean valid;
  guint16 red;
  guint16 green;
  guint16 blue;
} ColorSpec;

static gboolean test_color (ColorSpec *spec)
{
  PangoColor color;
  gboolean accepted;

  accepted = pango_color_parse (&color, spec->spec);

  if (accepted == spec->valid &&
      (!accepted ||
      (color.red == spec->red &&
       color.green == spec->green &&
       color.blue == spec->blue)))
    return TRUE;
  else
    return FALSE;
}


ColorSpec specs [] = {
  { "#abc",          1, 0xaaaa, 0xbbbb, 0xcccc },
  { "#aabbcc",       1, 0xaaaa, 0xbbbb, 0xcccc },
  { "#aaabbbccc",    1, 0xaaaa, 0xbbbb, 0xcccc },
  { "#100100100",    1, 0x1001, 0x1001, 0x1001 },
  { "#aaaabbbbcccc", 1, 0xaaaa, 0xbbbb, 0xcccc },
  { "#fff",          1, 0xffff, 0xffff, 0xffff },
  { "#ffffff",       1, 0xffff, 0xffff, 0xffff },
  { "#fffffffff",    1, 0xffff, 0xffff, 0xffff },
  { "#ffffffffffff", 1, 0xffff, 0xffff, 0xffff },
  { "#000",          1, 0x0000, 0x0000, 0x0000 },
  { "#000000",       1, 0x0000, 0x0000, 0x0000 },
  { "#000000000",    1, 0x0000, 0x0000, 0x0000 },
  { "#000000000000", 1, 0x0000, 0x0000, 0x0000 },
  { "#AAAABBBBCCCC", 1, 0xaaaa, 0xbbbb, 0xcccc },
  { "#aa bb cc ",    0, 0, 0, 0 },
  { "#aa bb ccc",    0, 0, 0, 0 },
  { "#ab",           0, 0, 0, 0 },
  { "#aabb",         0, 0, 0, 0 },
  { "#aaabb",        0, 0, 0, 0 },
  { "aaabb",         0, 0, 0, 0 },
  { "",              0, 0, 0, 0 },
  { "#",             0, 0, 0, 0 },
  { "##fff",         0, 0, 0, 0 },
  { "#0000ff+",      0, 0, 0, 0 },
  { "#0000f+",       0, 0, 0, 0 },
  { "#0x00x10x2",    0, 0, 0, 0 },
  { NULL,            0, 0, 0, 0 }
};

int
main (int argc, char *argv[])
{
  gboolean success;
  ColorSpec *spec;

  g_setenv ("PANGO_RC_FILE", "./pangorc", TRUE);

  success = TRUE;
  for (spec = specs; spec->spec; spec++)
    success &= test_color (spec);

  return !success;
}
