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
#include <pango-attributes.h>

#ifdef __cplusplus
extern "C" {
#endif /* __cplusplus */

/* Sort of like a GC - application set information about how
 * to handle scripts
 */
typedef struct _PangoContext PangoContext;

PangoContext *pango_context_new           (void);
void          pango_context_ref           (PangoContext                 *context);
void          pango_context_unref         (PangoContext                 *context);
void          pango_context_add_font_map  (PangoContext                 *context,
					   PangoFontMap                 *font_map);
void          pango_context_list_fonts    (PangoContext                 *context,
					   const char                   *family,
					   PangoFontDescription       ***descs,
					   int                          *n_descs);
void          pango_context_list_families (PangoContext                 *context,
					   gchar                      ***families,
					   int                          *n_families);
PangoFont *   pango_context_load_font     (PangoContext                 *context,
					   const PangoFontDescription   *desc,
					   gdouble                       size);

void                  pango_context_set_font_description (PangoContext               *context,
							  const PangoFontDescription *desc);
PangoFontDescription *pango_context_get_font_description (PangoContext               *context);
char *                pango_context_get_lang             (PangoContext               *context);
void                  pango_context_set_lang             (PangoContext               *context,
							  const char                 *lang);
int                   pango_context_get_size             (PangoContext               *context);
void                  pango_context_set_size             (PangoContext               *context,
							  int                         size);
void                  pango_context_set_base_dir         (PangoContext               *context,
							  PangoDirection              direction);
PangoDirection        pango_context_get_base_dir         (PangoContext               *context);

/* Break a string of Unicode characters into segments with
 * consistent shaping/language engine and bidrectional level.
 * Returns a GList of PangoItem's
 */
GList *pango_itemize (PangoContext   *context, 
		      gchar          *text, 
		      gint            length,
		      PangoAttrList  *attrs);

#ifdef __cplusplus
}
#endif /* __cplusplus */

#endif __PANGO_CONTEXT_H__
