/* Pango
 * pango.h:
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

#ifndef __PANGO_H__
#define __PANGO_H__

#include <glib.h>

#ifdef __cplusplus
extern "C" {
#endif /* __cplusplus */

#include <pango/pango-attributes.h>
#include <pango/pango-context.h>
#include <pango/pango-coverage.h>
#include <pango/pango-engine.h>
#include <pango/pango-font.h>
#include <pango/pango-fontmap.h>
#include <pango/pango-glyph.h>
#include <pango/pango-item.h>
#include <pango/pango-layout.h>
#include <pango/pango-types.h>

/* Logical attributes of a character
 */
struct _PangoLogAttr
{
  guint is_break : 1;  /* Break in front of character */
  guint is_white : 1;
  guint is_char_stop : 1;
  guint is_word_stop : 1;
};

/* Determine information about cluster/word/line breaks in a string
 * of Unicode text.
 */
void pango_break (const gchar   *text, 
		  gint           length, 
		  PangoAnalysis *analysis, 
		  PangoLogAttr  *attrs);

void pango_get_log_attrs (const char    *text,
                          int            length,
                          int            level,
                          const char    *language,
                          PangoLogAttr  *log_attrs);

/* Turn a string of characters into a string of glyphs
 */
void pango_shape (const gchar      *text,
		  gint              length,
		  PangoAnalysis    *analysis,
		  PangoGlyphString *glyphs);

GList *pango_reorder_items (GList *logical_items);

#ifdef __cplusplus
}
#endif /* __cplusplus */


#endif /* __PANGO_H__ */
