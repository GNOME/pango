/* Pango
 * pango-intset.c: Integer set
 *
 * Copyright (C) 2000 SuSE Linux Ltd
 *
 * Author: Robert Brady <rwb197@zepler.org>
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Library General Public
 * Licence as published by the Free Software Foundation; either
 * version 2 of the Licence, or (at your option) any later version.
 *
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Library General Public Licence for more details.
 *
 * You should have received a copy of the GNU Library General Public
 * Licence along with this library; if not, write to the
 * Free Software Foundation, Inc., 59 Temple Place - Suite 330,
 * Boston, MA 02111-1307, USA.
 */


#include <string.h>
#include <stdlib.h>
#include <math.h>
#include <glib.h>

#include "pango-intset.h"

#define ELEMENT_BITS (sizeof (guint) * CHAR_BIT)

#define OFF_MASK (ELEMENT_BITS-1)
#define SEG_MASK (~(OFF_MASK))

PangoIntSet *
pango_int_set_new (void) 
{
  PangoIntSet *p = g_new (PangoIntSet, 1);
  p->start = 0;
  p->size = 0;
  p->bits = 0;
  return p;
}

static void
pango_int_set_expand (PangoIntSet *g, int value)
{
  if (!g->bits) 
    {
      g->bits = g_new (guint, 1);
      g->bits[0] = 0;
      g->start = (value & SEG_MASK);
      g->size = 1;
      return;
    }

  if (value < g->start) 
    {
      int extra_space = ((g->start - value) & OFF_MASK) + 1;
      int new_start = (value & SEG_MASK);
      guint *new_bits = g_new (guint, extra_space + g->size);
      memcpy (new_bits + extra_space, g->bits, g->size * sizeof (guint));
      g_free (g->bits);
      g->bits = new_bits;
      memset (new_bits, 0, extra_space * sizeof (guint));
      g->start = new_start;
      g->size += extra_space;
      return;
    }

  if (value >= (g->start + g->size * ELEMENT_BITS))
    {
      int extra_space = (((value - (g->start + g->size * ELEMENT_BITS)) & 
			  OFF_MASK)) + 1;
      g->bits = g_realloc (g->bits, (g->size + extra_space) * sizeof (guint));
      memset (g->bits + g->size, 0, extra_space * sizeof (guint));
      g->size += extra_space;
      return;
    }
}

void
pango_int_set_add (PangoIntSet *g, int value)
{
  int offset;
  pango_int_set_expand (g, value);
  offset = value - g->start;
  g->bits[offset / ELEMENT_BITS] |= (1 << (offset & (OFF_MASK)));
}

void 
pango_int_set_add_range (PangoIntSet *g, int start, int end)
{
  int i;

  pango_int_set_add (g, start);

  if (start != end) 
    pango_int_set_add (g, end);

  if ((end - start) != 1)
    for (i = start; i < end; i++)
      pango_int_set_add (g, i);
}

gboolean 
pango_int_set_contains (PangoIntSet *g, int member)
{
  if (!g->bits)
    return 0;

  if (member < g->start)
    return 0;

  if (member >= (g->start + (g->size * ELEMENT_BITS)))
    return 0;
  
  return (g->bits[((member - g->start) / ELEMENT_BITS)] >> 
         (member & (ELEMENT_BITS-1))) & 1;
}

void
pango_int_set_destroy (PangoIntSet *g) 
{
  g_free (g->bits);
  g_free (g);
}

