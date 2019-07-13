/* Pango
 * pangohb-private.h: Private routines for using harfbuzz
 *
 * Copyright (C) 2019 Red Hat Software
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

#ifndef __PANGOHB_PRIVATE_H__
#define __PANGOHB_PRIVATE_H__

#include <pango-font.h>
#include <pango-item.h>
#include <pango-glyph.h>

G_BEGIN_DECLS

void pango_hb_shape (PangoFont           *font,
                     const char          *item_text,
                     unsigned int         item_length,
                     const PangoAnalysis *analysis,
                     PangoGlyphString    *glyphs,
                     const char          *paragraph_text,
                     unsigned int         paragraph_length);

G_END_DECLS

#endif /* __PANGOHB_PRIVATE_H__ */
