/* Pango
 * pangocairo-fontmap.c: Cairo font handling, fontconfig backend
 *
 * Copyright (C) 2000-2005 Red Hat Software
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

#include "pangofc-fontmap.h"
#include "pangocairo.h"
#include "pangocairo-private.h"
#include "pangocairo-fc.h"

typedef struct _PangoCairoFcFontMapClass PangoCairoFcFontMapClass;

struct _PangoCairoFcFontMapClass
{
  PangoFcFontMapClass parent_class;
};

static void
pango_cairo_fc_font_map_set_resolution (PangoCairoFontMap *cfontmap,
					double             dpi)
{
  PangoCairoFcFontMap *cffontmap = PANGO_CAIRO_FC_FONT_MAP (cfontmap);
  
  cffontmap->dpi = dpi;

  pango_fc_font_map_cache_clear (PANGO_FC_FONT_MAP (cfontmap));
}

static double
pango_cairo_fc_font_map_get_resolution (PangoCairoFontMap *cfontmap)
{
  PangoCairoFcFontMap *cffontmap = PANGO_CAIRO_FC_FONT_MAP (cfontmap);

  return cffontmap->dpi;
}

static PangoRenderer *
pango_cairo_fc_font_map_get_renderer (PangoCairoFontMap *cfontmap)
{
  PangoCairoFcFontMap *cffontmap = PANGO_CAIRO_FC_FONT_MAP (cfontmap);
  
  if (!cffontmap->renderer)
    cffontmap->renderer = g_object_new (PANGO_TYPE_CAIRO_RENDERER, NULL);

  return cffontmap->renderer;
}

static void
cairo_font_map_iface_init (PangoCairoFontMapIface *iface)
{
  iface->set_resolution = pango_cairo_fc_font_map_set_resolution;
  iface->get_resolution = pango_cairo_fc_font_map_get_resolution;
  iface->get_renderer = pango_cairo_fc_font_map_get_renderer;
}

G_DEFINE_TYPE_WITH_CODE (PangoCairoFcFontMap, pango_cairo_fc_font_map, PANGO_TYPE_FC_FONT_MAP,
    { G_IMPLEMENT_INTERFACE (PANGO_TYPE_CAIRO_FONT_MAP, cairo_font_map_iface_init) });

static void
pango_cairo_fc_font_map_finalize (GObject *object)
{
  PangoCairoFcFontMap *cffontmap = PANGO_CAIRO_FC_FONT_MAP (object);
  
  if (cffontmap->renderer)
    g_object_unref (cffontmap->renderer);

  FT_Done_FreeType (cffontmap->library);

  G_OBJECT_CLASS (pango_cairo_fc_font_map_parent_class)->finalize (object);
}

static void
pango_cairo_fc_font_map_default_substitute (PangoFcFontMap *fcfontmap,
					    FcPattern      *pattern)
{
  PangoCairoFcFontMap *cffontmap = PANGO_CAIRO_FC_FONT_MAP (fcfontmap);
  FcValue v;

  FcConfigSubstitute (NULL, pattern, FcMatchPattern);

  if (FcPatternGet (pattern, FC_DPI, 0, &v) == FcResultNoMatch)
    FcPatternAddDouble (pattern, FC_DPI, cffontmap->dpi);
  FcDefaultSubstitute (pattern);
}

static gboolean
pango_cairo_fc_font_map_get_render_key (PangoFcFontMap             *fcfontmap,
					PangoContext               *context,
					const PangoFontDescription *desc,
					int                        *xsize,
					int                        *ysize,
					guint                      *flags)
{
  return _pango_cairo_fc_get_render_key (PANGO_CAIRO_FC_FONT_MAP (fcfontmap),
					 context, desc, xsize, ysize, flags);
}

static PangoFcFont *
pango_cairo_fc_font_map_create_font (PangoFcFontMap             *fcfontmap,
				     PangoContext               *context,
				     const PangoFontDescription *desc,
				     FcPattern                  *pattern)
{
  return _pango_cairo_fc_font_new (PANGO_CAIRO_FC_FONT_MAP (fcfontmap),
				   context, desc, pattern);
}

static void
pango_cairo_fc_font_map_class_init (PangoCairoFcFontMapClass *class)
{
  GObjectClass *gobject_class = G_OBJECT_CLASS (class);
  PangoFcFontMapClass *fcfontmap_class = PANGO_FC_FONT_MAP_CLASS (class);

  gobject_class->finalize  = pango_cairo_fc_font_map_finalize;
  fcfontmap_class->default_substitute = pango_cairo_fc_font_map_default_substitute;
  fcfontmap_class->get_render_key = pango_cairo_fc_font_map_get_render_key;
  fcfontmap_class->create_font = pango_cairo_fc_font_map_create_font;
}

static void
pango_cairo_fc_font_map_init (PangoCairoFcFontMap *cffontmap)
{
  FT_Error error;

  cffontmap->library = NULL;
  error = FT_Init_FreeType (&cffontmap->library);
  if (error != FT_Err_Ok)
    g_error ("pango_cairo_font_map_init: Could not initialize freetype");

  cffontmap->dpi   = 96.0;
}

FT_Library
_pango_cairo_fc_font_map_get_library (PangoCairoFcFontMap *fontmap)
{
  g_return_val_if_fail (PANGO_IS_CAIRO_FC_FONT_MAP (fontmap), NULL);

  return fontmap->library;
}

