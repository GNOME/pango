/* Pango
 * pango-intset.h: Integer set
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

#ifndef __PANGO_INTSET_H__
#define __PANGO_INTSET_H__

typedef struct _PangoIntSet        PangoIntSet;
PangoIntSet     *pango_int_set_new             (void);
void            pango_int_set_add              (PangoIntSet     *g, 
                                                int              glyph);
void            pango_int_set_destroy          (PangoIntSet     *g);
void            pango_int_set_add_range        (PangoIntSet     *g, 
                                                int              start, 
                                                int              end);
gboolean        pango_int_set_contains         (PangoIntSet     *g, 
                                                int              member);
struct _PangoIntSet 
{
  int start, size;
  guint *bits;
};

#endif
