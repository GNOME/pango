/* Pango
 * pangocairo-fc.h: Private header file for Cairo/fontconfig combination
 *
 * Copyright (C) 2005 Red Hat, Inc.
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

#ifndef __PANGOCAIRO_FC_H__
#define __PANGOCAIRO_FC_H__

#include <pango/pangofc-fontmap.h>
#include <pango/pangocairo.h>

G_BEGIN_DECLS

#define PANGO_TYPE_CAIRO_FC_FONT_MAP       (pango_cairo_fc_font_map_get_type ())
#define PANGO_CAIRO_FC_FONT_MAP(object)    (G_TYPE_CHECK_INSTANCE_CAST ((object), PANGO_TYPE_CAIRO_FC_FONT_MAP, PangoCairoFcFontMap))
#define PANGO_IS_CAIRO_FC_FONT_MAP(object) (G_TYPE_CHECK_INSTANCE_TYPE ((object), PANGO_TYPE_CAIRO_FC_FONT_MAP))

typedef struct _PangoCairoFcFontMap PangoCairoFcFontMap;

struct _PangoCairoFcFontMap
{
  PangoFcFontMap parent_instance;

  double dpi;

  FT_Library library;
};

GType pango_cairo_fc_font_map_get_type (void) G_GNUC_CONST;

PangoFcFont *_pango_cairo_fc_font_new (PangoCairoFcFontMap        *cffontmap,
				       PangoContext               *context,
				       const PangoFontDescription *desc,
				       FcPattern	          *pattern);
FT_Library   _pango_cairo_fc_font_map_get_library (PangoCairoFcFontMap *fontmap);

G_END_DECLS

#endif /* __PANGOCAIRO_FC_H__ */

