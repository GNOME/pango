/* Pango
 * pangocairo-private.h: private symbols for the Cairo backend
 *
 * Copyright (C) 2000,2004 Red Hat, Inc.
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

#ifndef __PANGOCAIRO_PRIVATE_H__
#define __PANGOCAIRO_PRIVATE_H__

#include <pango/pangocairo.h>
#include <pango/pango-renderer.h>

G_BEGIN_DECLS

#define PANGO_CAIRO_FONT_MAP_GET_IFACE(obj)  (G_TYPE_INSTANCE_GET_INTERFACE ((obj), PANGO_TYPE_CAIRO_FONT_MAP, PangoCairoFontMapIface))

typedef struct _PangoCairoFontMapIface PangoCairoFontMapIface;

struct _PangoCairoFontMapIface
{
  GTypeInterface g_iface;

  void           (*set_resolution) (PangoCairoFontMap *fontmap,
			 	    double             dpi);
  double         (*get_resolution) (PangoCairoFontMap *fontmap);
  PangoRenderer *(*get_renderer)   (PangoCairoFontMap *fontmap);
};

PangoRenderer *_pango_cairo_font_map_get_renderer (PangoCairoFontMap *cfontmap);

#define PANGO_CAIRO_FONT_GET_IFACE(obj) (G_TYPE_INSTANCE_GET_INTERFACE ((obj), PANGO_TYPE_CAIRO_FONT, PangoCairoFontIface))

#define PANGO_TYPE_CAIRO_FONT       (pango_cairo_font_get_type ())
#define PANGO_CAIRO_FONT(object)    (G_TYPE_CHECK_INSTANCE_CAST ((object), PANGO_TYPE_CAIRO_FONT, PangoCairoFont))
#define PANGO_IS_CAIRO_FONT(object) (G_TYPE_CHECK_INSTANCE_TYPE ((object), PANGO_TYPE_CAIRO_FONT))

typedef struct _PangoCairoFont      PangoCairoFont;
typedef struct _PangoCairoFontIface PangoCairoFontIface;

struct _PangoCairoFontIface
{
  GTypeInterface g_iface;

  cairo_font_t *(*get_cairo_font) (PangoCairoFont *font);
};

GType pango_cairo_font_get_type (void);

cairo_font_t *_pango_cairo_font_get_cairo_font (PangoCairoFont *font);

#define PANGO_TYPE_CAIRO_RENDERER            (pango_cairo_renderer_get_type())
#define PANGO_CAIRO_RENDERER(object)         (G_TYPE_CHECK_INSTANCE_CAST ((object), PANGO_TYPE_CAIRO_RENDERER, PangoCairoRenderer))
#define PANGO_IS_CAIRO_RENDERER(object)      (G_TYPE_CHECK_INSTANCE_TYPE ((object), PANGO_TYPE_CAIRO_RENDERER))

typedef struct _PangoCairoRenderer PangoCairoRenderer;

GType pango_cairo_renderer_get_type    (void);

G_END_DECLS

#endif /* __PANGOCAIRO_PRIVATE_H__ */
