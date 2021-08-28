/* Pango
 *
 * Copyright (C) 2021 Matthias Clasen
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

#ifndef __PANGO_ITEM_PRIVATE_H__
#define __PANGO_ITEM_PRIVATE_H__

#include <pango/pango-item.h>

G_BEGIN_DECLS

/**
 * We have to do some extra work for adding the char_offset field
 * to PangoItem to preserve ABI in the face of pango's open-coded
 * structs.
 *
 * Internally, pango uses the PangoItemPrivate type, and we use
 * a bit in the PangoAnalysis flags to indicate whether we are
 * dealing with a PangoItemPrivate struct or not.
 */

#define PANGO_ANALYSIS_FLAG_HAS_CHAR_OFFSET (1 << 7)

typedef struct _PangoItemPrivate PangoItemPrivate;

#ifdef __x86_64__

struct _PangoItemPrivate
{
  int offset;
  int length;
  int num_chars;
  int char_offset;
  PangoAnalysis analysis;
};

#else

struct _PangoItemPrivate
{
  int offset;
  int length;
  int num_chars;
  PangoAnalysis analysis;
  int char_offset;
}

#endif

G_STATIC_ASSERT (offsetof (PangoItem, offset) == offsetof (PangoItemPrivate, offset));
G_STATIC_ASSERT (offsetof (PangoItem, length) == offsetof (PangoItemPrivate, length));
G_STATIC_ASSERT (offsetof (PangoItem, num_chars) == offsetof (PangoItemPrivate, num_chars));
G_STATIC_ASSERT (offsetof (PangoItem, analysis) == offsetof (PangoItemPrivate, analysis));

G_END_DECLS

#endif /* __PANGO_ITEM_PRIVATE_H__ */
