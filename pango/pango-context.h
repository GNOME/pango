/* Pango
 * pango-context.h: Rendering contexts
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

#ifndef __PANGO_CONTEXT_H__
#define __PANGO_CONTEXT_H__

#include <pango-font.h>

#ifdef __cplusplus
extern "C" {
#endif /* __cplusplus */

/* Sort of like a GC - application set information about how
 * to handle scripts
 */
typedef struct _PangoContext PangoContext;

PangoContext * pango_context_new          (void);
void           pango_context_ref          (PangoContext           *context);
void           pango_context_unref        (PangoContext           *context);

void	       pango_context_add_font_map (PangoContext           *context,
					   PangoFontMap           *font_map);

void           pango_context_list_fonts   (PangoContext           *context,
					   PangoFontDescription ***descs,
					   int                    *n_descs);
PangoFont *    pango_context_load_font    (PangoContext           *context,
					   PangoFontDescription   *desc,
					   gdouble                 size);

void           pango_context_set_lang     (PangoContext           *context,
					   const char             *lang);
char *         pango_context_get_lang     (PangoContext           *context);
void           pango_context_set_base_dir (PangoContext           *context,
					   PangoDirection          direction);
PangoDirection pango_context_get_base_dir (PangoContext           *context);


#ifdef __cplusplus
}
#endif /* __cplusplus */

#endif __PANGO_CONTEXT_H__
