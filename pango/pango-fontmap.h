/* Pango
 * pango-font.h: Font handling
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
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Library General Public License for more details.
 *
 * You should have received a copy of the GNU Library General Public
 * License along with this library; if not, write to the
 * Free Software Foundation, Inc., 59 Temple Place - Suite 330,
 * Boston, MA 02111-1307, USA.
 */

#ifndef __PANGO_FONTMAP_H__
#define __PANGO_FONTMAP_H__

#include <pango/pango-types.h>
#include <pango/pango-font.h>
#include <pango/pango-fontset.h>

G_BEGIN_DECLS

#define PANGO_TYPE_FONT_MAP              (pango_font_map_get_type ())
#define PANGO_FONT_MAP(object)           (G_TYPE_CHECK_INSTANCE_CAST ((object), PANGO_TYPE_FONT_MAP, PangoFontMap))
#define PANGO_IS_FONT_MAP(object)        (G_TYPE_CHECK_INSTANCE_TYPE ((object), PANGO_TYPE_FONT_MAP))


PANGO_AVAILABLE_IN_ALL
GType         pango_font_map_get_type       (void) G_GNUC_CONST;
PANGO_AVAILABLE_IN_1_22
PangoContext * pango_font_map_create_context (PangoFontMap               *fontmap);
PANGO_AVAILABLE_IN_ALL
PangoFont *   pango_font_map_load_font     (PangoFontMap                 *fontmap,
                                            PangoContext                 *context,
                                            const PangoFontDescription   *desc);
PANGO_AVAILABLE_IN_ALL
PangoFontset *pango_font_map_load_fontset  (PangoFontMap                 *fontmap,
                                            PangoContext                 *context,
                                            const PangoFontDescription   *desc,
                                            PangoLanguage                *language);
PANGO_AVAILABLE_IN_1_32
guint         pango_font_map_get_serial    (PangoFontMap                 *fontmap);
PANGO_AVAILABLE_IN_1_34
void          pango_font_map_changed       (PangoFontMap                 *fontmap);

PANGO_AVAILABLE_IN_1_46
PangoFontFamily *pango_font_map_get_family (PangoFontMap                 *fontmap,
                                            const char                   *name);


G_DEFINE_AUTOPTR_CLEANUP_FUNC(PangoFontMap, g_object_unref)

G_END_DECLS

#endif /* __PANGO_FONTMAP_H__ */
