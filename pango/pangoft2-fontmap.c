/* Pango
 * pangoft2-fontmap.c:
 *
 * Copyright (C) 2000 Red Hat Software
 * Copyright (C) 2000 Tor Lillqvist
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

#include <glib.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <errno.h>

#include <fontconfig/fontconfig.h>

#include "pango-impl-utils.h"
#include "pangoft2-private.h"
#include "pangofc-fontmap.h"

typedef struct _PangoFT2Family       PangoFT2Family;
typedef struct _PangoFT2FontMapClass PangoFT2FontMapClass;

/**
 * PangoFT2FontMap:
 *
 * The `PangoFT2FontMap` is the `PangoFontMap` implementation for FreeType fonts.
 */
struct _PangoFT2FontMap
{
  PangoFcFontMap parent_instance;

  FT_Library library;

  guint serial;
  double dpi_x;
  double dpi_y;

  PangoRenderer *renderer;
};

struct _PangoFT2FontMapClass
{
  PangoFcFontMapClass parent_class;
};

static void          pango_ft2_font_map_finalize            (GObject              *object);
static PangoFcFont * pango_ft2_font_map_new_font            (PangoFcFontMap       *fcfontmap,
							     FcPattern            *pattern);
static double        pango_ft2_font_map_get_resolution      (PangoFcFontMap       *fcfontmap,
							     PangoContext         *context);
static guint         pango_ft2_font_map_get_serial          (PangoFontMap         *fontmap);
static void          pango_ft2_font_map_changed             (PangoFontMap         *fontmap);

G_DEFINE_TYPE (PangoFT2FontMap, pango_ft2_font_map, PANGO_TYPE_FC_FONT_MAP)

static void
pango_ft2_font_map_class_init (PangoFT2FontMapClass *class)
{
  GObjectClass *gobject_class = G_OBJECT_CLASS (class);
  PangoFontMapClass *fontmap_class = PANGO_FONT_MAP_CLASS (class);
  PangoFcFontMapClass *fcfontmap_class = PANGO_FC_FONT_MAP_CLASS (class);

  gobject_class->finalize = pango_ft2_font_map_finalize;
  fontmap_class->get_serial = pango_ft2_font_map_get_serial;
  fontmap_class->changed = pango_ft2_font_map_changed;
  fcfontmap_class->default_substitute = _pango_ft2_font_map_default_substitute;
  fcfontmap_class->new_font = pango_ft2_font_map_new_font;
  fcfontmap_class->get_resolution = pango_ft2_font_map_get_resolution;
}

static void
pango_ft2_font_map_init (PangoFT2FontMap *fontmap)
{
  FT_Error error;

  fontmap->serial = 1;
  fontmap->library = NULL;
  fontmap->dpi_x   = 72.0;
  fontmap->dpi_y   = 72.0;

  error = FT_Init_FreeType (&fontmap->library);
  if (error != FT_Err_Ok)
    g_critical ("pango_ft2_font_map_init: Could not initialize freetype");
}

static void
pango_ft2_font_map_finalize (GObject *object)
{
  PangoFT2FontMap *ft2fontmap = PANGO_FT2_FONT_MAP (object);

  if (ft2fontmap->renderer)
    g_object_unref (ft2fontmap->renderer);

  G_OBJECT_CLASS (pango_ft2_font_map_parent_class)->finalize (object);

  FT_Done_FreeType (ft2fontmap->library);
}

/**
 * pango_ft2_font_map_new:
 *
 * Create a new `PangoFT2FontMap` object.
 *
 * A fontmap is used to cache information about available fonts,
 * and holds certain global parameters such as the resolution and
 * the default substitute function (see
 * [method@PangoFT2.FontMap.set_default_substitute]).
 *
 * Return value: the newly created fontmap object. Unref
 * with g_object_unref() when you are finished with it.
 *
 * Since: 1.2
 **/
PangoFontMap *
pango_ft2_font_map_new (void)
{
  return (PangoFontMap *) g_object_new (PANGO_TYPE_FT2_FONT_MAP, NULL);
}

static guint
pango_ft2_font_map_get_serial (PangoFontMap *fontmap)
{
  PangoFT2FontMap *ft2fontmap = PANGO_FT2_FONT_MAP (fontmap);

  return ft2fontmap->serial;
}

static void
pango_ft2_font_map_changed (PangoFontMap *fontmap)
{
  PangoFT2FontMap *ft2fontmap = PANGO_FT2_FONT_MAP (fontmap);

  ft2fontmap->serial++;
  if (ft2fontmap->serial == 0)
    ft2fontmap->serial++;
}

/**
 * pango_ft2_font_map_set_resolution:
 * @fontmap: a `PangoFT2FontMap`
 * @dpi_x: dots per inch in the X direction
 * @dpi_y: dots per inch in the Y direction
 *
 * Sets the horizontal and vertical resolutions for the fontmap.
 *
 * Since: 1.2
 **/
void
pango_ft2_font_map_set_resolution (PangoFT2FontMap *fontmap,
				   double           dpi_x,
				   double           dpi_y)
{
  g_return_if_fail (PANGO_FT2_IS_FONT_MAP (fontmap));

  fontmap->dpi_x = dpi_x;
  fontmap->dpi_y = dpi_y;

  pango_fc_font_map_substitute_changed (PANGO_FC_FONT_MAP (fontmap));
}

FT_Library
_pango_ft2_font_map_get_library (PangoFontMap *fontmap)
{
  PangoFT2FontMap *ft2fontmap = (PangoFT2FontMap *)fontmap;

  return ft2fontmap->library;
}


/**
 * _pango_ft2_font_map_get_renderer:
 * @fontmap: a `PangoFT2FontMap`
 *
 * Gets the singleton PangoFT2Renderer for this fontmap.
 *
 * Return value: the renderer.
 **/
PangoRenderer *
_pango_ft2_font_map_get_renderer (PangoFT2FontMap *ft2fontmap)
{
  if (!ft2fontmap->renderer)
    ft2fontmap->renderer = g_object_new (PANGO_TYPE_FT2_RENDERER, NULL);

  return ft2fontmap->renderer;
}

void
_pango_ft2_font_map_default_substitute (PangoFcFontMap *fcfontmap,
				       FcPattern      *pattern)
{
  PangoFT2FontMap *ft2fontmap = PANGO_FT2_FONT_MAP (fcfontmap);
  FcValue v;

  FcConfigSubstitute (NULL, pattern, FcMatchPattern);

  if (fcfontmap->substitute_func)
    fcfontmap->substitute_func (pattern, fcfontmap->substitute_data);

  if (FcPatternGet (pattern, FC_DPI, 0, &v) == FcResultNoMatch)
    FcPatternAddDouble (pattern, FC_DPI, ft2fontmap->dpi_y);
  FcDefaultSubstitute (pattern);
}

static double
pango_ft2_font_map_get_resolution (PangoFcFontMap       *fcfontmap,
				   PangoContext         *context G_GNUC_UNUSED)
{
  return ((PangoFT2FontMap *)fcfontmap)->dpi_y;
}

static PangoFcFont *
pango_ft2_font_map_new_font (PangoFcFontMap  *fcfontmap,
			     FcPattern       *pattern)
{
  return (PangoFcFont *)_pango_ft2_font_new (PANGO_FT2_FONT_MAP (fcfontmap), pattern);
}
