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
#include "config.h"
#include <cairo-ft.h>

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
  PangoCairoFcFontMap *cffontmap = (PangoCairoFcFontMap *) (cfontmap);

  cffontmap->dpi = dpi;

  pango_fc_font_map_cache_clear ((PangoFcFontMap *) (cfontmap));
}

static double
pango_cairo_fc_font_map_get_resolution_cairo (PangoCairoFontMap *cfontmap)
{
  PangoCairoFcFontMap *cffontmap = (PangoCairoFcFontMap *) (cfontmap);

  return cffontmap->dpi;
}

static cairo_font_type_t
pango_cairo_fc_font_map_get_font_type (PangoCairoFontMap *cfontmap)
{
  return CAIRO_FONT_TYPE_FT;
}

static void
cairo_font_map_iface_init (PangoCairoFontMapIface *iface)
{
  iface->set_resolution = pango_cairo_fc_font_map_set_resolution;
  iface->get_resolution = pango_cairo_fc_font_map_get_resolution_cairo;
  iface->get_font_type  = pango_cairo_fc_font_map_get_font_type;
}

G_DEFINE_TYPE_WITH_CODE (PangoCairoFcFontMap, pango_cairo_fc_font_map, PANGO_TYPE_FC_FONT_MAP,
    { G_IMPLEMENT_INTERFACE (PANGO_TYPE_CAIRO_FONT_MAP, cairo_font_map_iface_init) })

static void
pango_cairo_fc_font_map_finalize (GObject *object)
{
  PangoCairoFcFontMap *cffontmap = (PangoCairoFcFontMap *) (object);

  FT_Done_FreeType (cffontmap->library);

  G_OBJECT_CLASS (pango_cairo_fc_font_map_parent_class)->finalize (object);
}

static void
pango_cairo_fc_font_map_context_substitute (PangoFcFontMap *fcfontmap,
					    PangoContext   *context,
					    FcPattern      *pattern)
{
  FcConfigSubstitute (NULL, pattern, FcMatchPattern);

  if (context)
    cairo_ft_font_options_substitute (_pango_cairo_context_get_merged_font_options (context),
				      pattern);

  FcDefaultSubstitute (pattern);
}

static double
pango_cairo_fc_font_map_get_resolution_fc (PangoFcFontMap *fcfontmap,
					   PangoContext   *context)
{
  PangoCairoFcFontMap *cffontmap = (PangoCairoFcFontMap *) (fcfontmap);
  double dpi;

  if (context)
    {
      dpi = pango_cairo_context_get_resolution (context);

      if (dpi <= 0)
	dpi = cffontmap->dpi;
    }
  else
    dpi = cffontmap->dpi;

  return dpi;
}

static gconstpointer
pango_cairo_fc_font_map_context_key_get (PangoFcFontMap *fcfontmap,
					 PangoContext   *context)
{
  return _pango_cairo_context_get_merged_font_options (context);
}

static gpointer
pango_cairo_fc_font_map_context_key_copy (PangoFcFontMap *fcfontmap,
					  gconstpointer   key)
{
  return cairo_font_options_copy (key);
}

static void
pango_cairo_fc_font_map_context_key_free (PangoFcFontMap *fcfontmap,
					  gpointer        key)
{
  cairo_font_options_destroy (key);
}


static guint32
pango_cairo_fc_font_map_context_key_hash (PangoFcFontMap *fcfontmap,
					  gconstpointer        key)
{
  return (guint32)cairo_font_options_hash (key);
}

static gboolean
pango_cairo_fc_font_map_context_key_equal (PangoFcFontMap *fcfontmap,
					   gconstpointer   key_a,
					   gconstpointer   key_b)
{
  return cairo_font_options_equal (key_a, key_b);
}

static PangoFcFont *
pango_cairo_fc_font_map_create_font (PangoFcFontMap             *fcfontmap,
				     PangoContext               *context,
				     const PangoFontDescription *desc,
				     FcPattern                  *pattern)
{
  return _pango_cairo_fc_font_new ((PangoCairoFcFontMap *) (fcfontmap),
				   context, desc, pattern);
}

static void
pango_cairo_fc_font_map_class_init (PangoCairoFcFontMapClass *class)
{
  GObjectClass *gobject_class = G_OBJECT_CLASS (class);
  PangoFcFontMapClass *fcfontmap_class = PANGO_FC_FONT_MAP_CLASS (class);

  gobject_class->finalize  = pango_cairo_fc_font_map_finalize;

  fcfontmap_class->context_substitute = pango_cairo_fc_font_map_context_substitute;
  fcfontmap_class->get_resolution = pango_cairo_fc_font_map_get_resolution_fc;

  fcfontmap_class->context_key_get = pango_cairo_fc_font_map_context_key_get;
  fcfontmap_class->context_key_copy = pango_cairo_fc_font_map_context_key_copy;
  fcfontmap_class->context_key_free = pango_cairo_fc_font_map_context_key_free;
  fcfontmap_class->context_key_hash = pango_cairo_fc_font_map_context_key_hash;
  fcfontmap_class->context_key_equal = pango_cairo_fc_font_map_context_key_equal;

  fcfontmap_class->create_font = pango_cairo_fc_font_map_create_font;
}

static void
pango_cairo_fc_font_map_init (PangoCairoFcFontMap *cffontmap)
{
  FT_Error error;

  cffontmap->library = NULL;
  error = FT_Init_FreeType (&cffontmap->library);
  if (error != FT_Err_Ok)
    g_critical ("pango_cairo_font_map_init: Could not initialize freetype");

  cffontmap->dpi   = 96.0;
}

FT_Library
_pango_cairo_fc_font_map_get_library (PangoCairoFcFontMap *fontmap)
{
  g_return_val_if_fail (PANGO_IS_CAIRO_FC_FONT_MAP (fontmap), NULL);

  return fontmap->library;
}

