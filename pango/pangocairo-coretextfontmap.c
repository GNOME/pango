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
pango_cairo_core_text_font_map_get_resolution_cairo (PangoCairoFontMap *cfontmap)
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
  iface->get_resolution = pango_cairo_core_text_font_map_get_resolution_cairo;
  iface->get_font_type  = pango_cairo_core_text_font_map_get_font_type;
}

G_DEFINE_TYPE_WITH_CODE (PangoCairoCoreTextFontMap, pango_cairo_core_text_font_map, PANGO_TYPE_CORE_TEXT_FONT_MAP,
    { G_IMPLEMENT_INTERFACE (PANGO_TYPE_CAIRO_FONT_MAP, cairo_font_map_iface_init) });


static PangoCoreTextFont *
pango_cairo_core_text_font_map_create_font (PangoCoreTextFontMap       *fontmap,
                                            PangoCoreTextFontKey       *key)

{
  return _pango_cairo_core_text_font_new (PANGO_CAIRO_CORE_TEXT_FONT_MAP (fontmap),
                                          key);
}

static void
pango_cairo_core_text_font_map_finalize (GObject *object)
{
  G_OBJECT_CLASS (pango_cairo_core_text_font_map_parent_class)->finalize (object);
}

static double
pango_cairo_core_text_font_map_get_resolution_core_text (PangoCoreTextFontMap *ctfontmap,
                                                         PangoContext         *context)
{
  PangoCairoCoreTextFontMap *cafontmap = PANGO_CAIRO_CORE_TEXT_FONT_MAP (ctfontmap);
  double dpi;

  if (context)
    {
      dpi = pango_cairo_context_get_resolution (context);

      if (dpi <= 0)
        dpi = cafontmap->dpi;
    }
  else
    dpi = cafontmap->dpi;

  return dpi;
}

static gconstpointer
pango_cairo_core_text_font_map_context_key_get (PangoCoreTextFontMap *fontmap G_GNUC_UNUSED,
                                                PangoContext         *context)
{
  return _pango_cairo_context_get_merged_font_options (context);
}

static gpointer
pango_cairo_core_text_font_map_context_key_copy (PangoCoreTextFontMap *fontmap G_GNUC_UNUSED,
                                                 gconstpointer         key)
{
  return cairo_font_options_copy (key);
}

static void
pango_cairo_core_text_font_map_context_key_free (PangoCoreTextFontMap *fontmap G_GNUC_UNUSED,
                                                 gpointer              key)
{
  cairo_font_options_destroy (key);
}

static guint32
pango_cairo_core_text_font_map_context_key_hash (PangoCoreTextFontMap *fontmap G_GNUC_UNUSED,
                                                 gconstpointer         key)
{
  return (guint32)cairo_font_options_hash (key);
}

static gboolean
pango_cairo_core_text_font_map_context_key_equal (PangoCoreTextFontMap *fontmap G_GNUC_UNUSED,
                                                  gconstpointer         key_a,
                                                  gconstpointer         key_b)
{
  return cairo_font_options_equal (key_a, key_b);
}

static void
pango_cairo_core_text_font_map_class_init (PangoCairoCoreTextFontMapClass *class)
{
  PangoCoreTextFontMapClass *ctfontmapclass = (PangoCoreTextFontMapClass *)class;
  GObjectClass *object_class = (GObjectClass *)class;

  object_class->finalize = pango_cairo_core_text_font_map_finalize;

  ctfontmapclass->get_resolution = pango_cairo_core_text_font_map_get_resolution_core_text;
  ctfontmapclass->create_font = pango_cairo_core_text_font_map_create_font;
  ctfontmapclass->context_key_get = pango_cairo_core_text_font_map_context_key_get;
  ctfontmapclass->context_key_copy = pango_cairo_core_text_font_map_context_key_copy;
  ctfontmapclass->context_key_free = pango_cairo_core_text_font_map_context_key_free;
  ctfontmapclass->context_key_hash = pango_cairo_core_text_font_map_context_key_hash;
  ctfontmapclass->context_key_equal = pango_cairo_core_text_font_map_context_key_equal;
}

static void
pango_cairo_core_text_font_map_init (PangoCairoCoreTextFontMap *cafontmap)
{
  cafontmap->dpi = 72.;
}
