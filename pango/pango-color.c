/* pango
 * pango-color.c: Color handling
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
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "pango-attributes.h"
#include "pango-impl-utils.h"

GType
pango_color_get_type (void)
{
  static GType our_type = 0;

  if (G_UNLIKELY (our_type == 0))
    our_type = g_boxed_type_register_static (I_("PangoColor"),
					     (GBoxedCopyFunc) pango_color_copy,
					     (GBoxedFreeFunc) pango_color_free);

  return our_type;
}

/**
 * pango_color_copy:
 * @src: color to copy, may be %NULL
 *
 * Creates a copy of @src, which should be freed with
 * pango_color_free(). Primarily used by language bindings,
 * not that useful otherwise (since colors can just be copied
 * by assignment in C).
 *
 * Return value: the newly allocated #PangoColor, which should
 *               be freed with pango_color_free(), or %NULL
 *               if @src was %NULL.
 **/
PangoColor*
pango_color_copy (const PangoColor *src)
{
  PangoColor *ret;

  if (src == NULL)
    return NULL;

  ret = g_slice_new (PangoColor);

  *ret = *src;

  return ret;
}

/**
 * pango_color_free:
 * @color: an allocated #PangoColor, may be %NULL
 *
 * Frees a color allocated by pango_color_copy().
 **/
void
pango_color_free (PangoColor *color)
{
  if (color == NULL)
    return;

  g_slice_free (PangoColor, color);
}

/**
 * pango_color_to_string:
 * @color: a #PangoColor
 *
 * Returns a textual specification of @color in the hexadecimal form
 * <literal>&num;rrrrggggbbbb</literal>, where <literal>r</literal>,
 * <literal>g</literal> and <literal>b</literal> are hex digits representing
 * the red, green, and blue components respectively.
 *
 * Return value: a newly-allocated text string that must be freed with g_free().
 *
 * Since: 1.16
 **/
gchar *
pango_color_to_string (const PangoColor *color)
{
  g_return_val_if_fail (color != NULL, NULL);

  return g_strdup_printf ("#%04x%04x%04x", color->red, color->green, color->blue);
}

/* Color parsing
 */

/* The following 2 routines (parse_color, find_color) come from Tk, via the Win32
 * port of GDK. The licensing terms on these (longer than the functions) is:
 *
 * This software is copyrighted by the Regents of the University of
 * California, Sun Microsystems, Inc., and other parties.  The following
 * terms apply to all files associated with the software unless explicitly
 * disclaimed in individual files.
 *
 * The authors hereby grant permission to use, copy, modify, distribute,
 * and license this software and its documentation for any purpose, provided
 * that existing copyright notices are retained in all copies and that this
 * notice is included verbatim in any distributions. No written agreement,
 * license, or royalty fee is required for any of the authorized uses.
 * Modifications to this software may be copyrighted by their authors
 * and need not follow the licensing terms described here, provided that
 * the new terms are clearly indicated on the first page of each file where
 * they apply.
 *
 * IN NO EVENT SHALL THE AUTHORS OR DISTRIBUTORS BE LIABLE TO ANY PARTY
 * FOR DIRECT, INDIRECT, SPECIAL, INCIDENTAL, OR CONSEQUENTIAL DAMAGES
 * ARISING OUT OF THE USE OF THIS SOFTWARE, ITS DOCUMENTATION, OR ANY
 * DERIVATIVES THEREOF, EVEN IF THE AUTHORS HAVE BEEN ADVISED OF THE
 * POSSIBILITY OF SUCH DAMAGE.
 *
 * THE AUTHORS AND DISTRIBUTORS SPECIFICALLY DISCLAIM ANY WARRANTIES,
 * INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE, AND NON-INFRINGEMENT.  THIS SOFTWARE
 * IS PROVIDED ON AN "AS IS" BASIS, AND THE AUTHORS AND DISTRIBUTORS HAVE
 * NO OBLIGATION TO PROVIDE MAINTENANCE, SUPPORT, UPDATES, ENHANCEMENTS, OR
 * MODIFICATIONS.
 *
 * GOVERNMENT USE: If you are acquiring this software on behalf of the
 * U.S. government, the Government shall have only "Restricted Rights"
 * in the software and related documentation as defined in the Federal
 * Acquisition Regulations (FARs) in Clause 52.227.19 (c) (2).  If you
 * are acquiring the software on behalf of the Department of Defense, the
 * software shall be classified as "Commercial Computer Software" and the
 * Government shall have only "Restricted Rights" as defined in Clause
 * 252.227-7013 (c) (1) of DFARs.  Notwithstanding the foregoing, the
 * authors grant the U.S. Government and others acting in its behalf
 * permission to use and distribute the software in accordance with the
 * terms specified in this license.
 */

#include "pango-color-table.h"

static int
compare_xcolor_entries (const void *a, const void *b)
{
  return g_ascii_strcasecmp ((const char *) a, color_names + ((const ColorEntry *) b)->name_offset);
}

static gboolean
find_color(const char *name,
	   PangoColor *color)
{
  ColorEntry *found;

  found = bsearch (name, color_entries, G_N_ELEMENTS (color_entries),
		   sizeof (ColorEntry),
		   compare_xcolor_entries);
  if (found == NULL)
    return FALSE;

  if (color)
    {
      color->red = (found->red * 65535) / 255;
      color->green = (found->green * 65535) / 255;
      color->blue = (found->blue * 65535) / 255;
    }

  return TRUE;
}

static gboolean
hex (const char *spec,
    int len,
    unsigned int *c)
{
  const char *end;
  *c = 0;
  for (end = spec + len; spec != end; spec++)
    if (g_ascii_isxdigit (*spec))
      *c = (*c << 4) | g_ascii_xdigit_value (*spec);
    else
      return FALSE;
  return TRUE;
}

/**
 * pango_color_parse:
 * @color: a #PangoColor structure in which to store the result
 * @spec: a string specifying the new color
 *
 * Fill in the fields of a color from a string specification. The
 * string can either one of a large set of standard names. (Taken
 * from the X11 <filename>rgb.txt</filename> file), or it can be a hex value in the
 * form '&num;rgb' '&num;rrggbb' '&num;rrrgggbbb' or '&num;rrrrggggbbbb' where
 * 'r', 'g' and 'b' are hex digits of the red, green, and blue
 * components of the color, respectively. (White in the four
 * forms is '&num;fff' '&num;ffffff' '&num;fffffffff' and '&num;ffffffffffff')
 *
 * Return value: %TRUE if parsing of the specifier succeeded,
 *   otherwise false.
 **/
gboolean
pango_color_parse (PangoColor *color,
		   const char *spec)
{
  g_return_val_if_fail (spec != NULL, FALSE);

  if (spec[0] == '#')
    {
      size_t len;
      unsigned int r, g, b;

      spec++;
      len = strlen (spec);
      if (len % 3 || len < 3 || len > 12)
	return FALSE;

      len /= 3;

      if (!hex (spec, len, &r) ||
	  !hex (spec + len, len, &g) ||
	  !hex (spec + len * 2, len, &b))
	return FALSE;

      if (color)
	{
	  int bits = len * 4;
	  r <<= 16 - bits;
	  g <<= 16 - bits;
	  b <<= 16 - bits;
	  while (bits < 16)
	    {
	      r |= (r >> bits);
	      g |= (g >> bits);
	      b |= (b >> bits);
	      bits *= 2;
	    }
	  color->red   = r;
	  color->green = g;
	  color->blue  = b;
	}
    }
  else
    {
      if (!find_color (spec, color))
	return FALSE;
    }
  return TRUE;
}
