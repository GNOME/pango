/* Pango
 * pangocairo-coretextfontmap.c
 *
 * Copyright (C) 2005 Imendio AB
 * Copyright (C) 2010  Kristian Rietveld  <kris@gtk.org>
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

#include "config.h"

#include "pangocoretext-private.h"
#include "pangocairo.h"
#include "pangocairo-private.h"
#include "pangocairo-coretext.h"

typedef struct _PangoCairoCoreTextFontMapClass PangoCairoCoreTextFontMapClass;

struct _PangoCairoCoreTextFontMapClass
{
  PangoCoreTextFontMapClass parent_class;
};

static void
pango_cairo_core_text_font_map_set_resolution (PangoCairoFontMap *cfontmap,
                                               double             dpi)
{
  PangoCairoCoreTextFontMap *cafontmap = PANGO_CAIRO_CORE_TEXT_FONT_MAP (cfontmap);

  cafontmap->dpi = dpi;
}

static double
pango_cairo_core_text_font_map_get_resolution (PangoCairoFontMap *cfontmap)
{
  PangoCairoCoreTextFontMap *cafontmap = PANGO_CAIRO_CORE_TEXT_FONT_MAP (cfontmap);

  return cafontmap->dpi;
}

static cairo_font_type_t
pango_cairo_core_text_font_map_get_font_type (PangoCairoFontMap *cfontmap)
{
  /* This is a bit misleading, but Cairo takes a CoreGraphics font
   * for rendering and does not use ATSUI.
   */
  return CAIRO_FONT_TYPE_ATSUI;
}

static void
cairo_font_map_iface_init (PangoCairoFontMapIface *iface)
{
  iface->set_resolution = pango_cairo_core_text_font_map_set_resolution;
  iface->get_resolution = pango_cairo_core_text_font_map_get_resolution;
  iface->get_font_type  = pango_cairo_core_text_font_map_get_font_type;
}

G_DEFINE_TYPE_WITH_CODE (PangoCairoCoreTextFontMap, pango_cairo_core_text_font_map, PANGO_TYPE_CORE_TEXT_FONT_MAP,
    { G_IMPLEMENT_INTERFACE (PANGO_TYPE_CAIRO_FONT_MAP, cairo_font_map_iface_init) });


static PangoCoreTextFont *
pango_cairo_core_text_font_map_create_font (PangoCoreTextFontMap       *fontmap,
                                            PangoContext               *context,
                                            PangoCoreTextFace          *face,
                                            const PangoFontDescription *desc)

{
  return _pango_cairo_core_text_font_new (PANGO_CAIRO_CORE_TEXT_FONT_MAP (fontmap),
                                          context, face, desc);
}

static void
pango_cairo_core_text_font_map_finalize (GObject *object)
{
  G_OBJECT_CLASS (pango_cairo_core_text_font_map_parent_class)->finalize (object);
}

static void
pango_cairo_core_text_font_map_class_init (PangoCairoCoreTextFontMapClass *class)
{
  PangoCoreTextFontMapClass *ctfontmapclass = (PangoCoreTextFontMapClass *)class;
  GObjectClass *object_class = (GObjectClass *)class;

  object_class->finalize = pango_cairo_core_text_font_map_finalize;

  ctfontmapclass->create_font = pango_cairo_core_text_font_map_create_font;
}

static void
pango_cairo_core_text_font_map_init (PangoCairoCoreTextFontMap *cafontmap)
{
  cafontmap->dpi = 72.;
}
