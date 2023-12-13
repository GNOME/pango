/* Pango
 * pangocairo-win32fontmap.c: Cairo font handling, Win32 backend
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

#include "pangowin32-private.h"
#include "pangocairo.h"
#include "pangocairo-private.h"
#include "pangocairo-win32.h"

typedef struct _PangoCairoWin32FontMapClass PangoCairoWin32FontMapClass;

struct _PangoCairoWin32FontMapClass
{
  PangoWin32FontMapClass parent_class;
};

static guint
pango_cairo_win32_font_map_get_serial (PangoFontMap *fontmap)
{
  PangoCairoWin32FontMap *cwfontmap = PANGO_CAIRO_WIN32_FONT_MAP (fontmap);

  return cwfontmap->serial;
}

static void
pango_cairo_win32_font_map_changed (PangoFontMap *fontmap)
{
  PangoCairoWin32FontMap *cwfontmap = PANGO_CAIRO_WIN32_FONT_MAP (fontmap);

  cwfontmap->serial++;
  if (cwfontmap->serial == 0)
    cwfontmap->serial++;

  pango_win32_font_map_cache_clear (fontmap);
}

static void
pango_cairo_win32_font_map_set_resolution (PangoCairoFontMap *cfontmap,
					   double             dpi)
{
  PangoCairoWin32FontMap *cwfontmap = PANGO_CAIRO_WIN32_FONT_MAP (cfontmap);

  cwfontmap->serial++;
  if (cwfontmap->serial == 0)
    cwfontmap->serial++;
  cwfontmap->dpi = dpi;
}

static double
pango_cairo_win32_font_map_get_resolution (PangoCairoFontMap *cfontmap)
{
  PangoCairoWin32FontMap *cwfontmap = PANGO_CAIRO_WIN32_FONT_MAP (cfontmap);

  return cwfontmap->dpi;
}

static cairo_font_type_t
pango_cairo_win32_font_map_get_font_type (PangoCairoFontMap *cfontmap)
{
  return CAIRO_FONT_TYPE_WIN32;
}

static void
cairo_font_map_iface_init (PangoCairoFontMapIface *iface)
{
  iface->set_resolution = pango_cairo_win32_font_map_set_resolution;
  iface->get_resolution = pango_cairo_win32_font_map_get_resolution;
  iface->get_font_type  = pango_cairo_win32_font_map_get_font_type;
}

G_DEFINE_TYPE_WITH_CODE (PangoCairoWin32FontMap, pango_cairo_win32_font_map, PANGO_TYPE_WIN32_FONT_MAP,
    { G_IMPLEMENT_INTERFACE (PANGO_TYPE_CAIRO_FONT_MAP, cairo_font_map_iface_init) });

static void
pango_cairo_win32_font_map_finalize (GObject *object)
{
  G_OBJECT_CLASS (pango_cairo_win32_font_map_parent_class)->finalize (object);
}

static PangoFont *
pango_cairo_win32_font_map_find_font (PangoWin32FontMap          *fontmap,
				      PangoContext               *context,
				      PangoWin32Face             *face,
				      const PangoFontDescription *desc)
{
  return _pango_cairo_win32_font_new (PANGO_CAIRO_WIN32_FONT_MAP (fontmap),
				      context, face, desc);
}

static void
pango_cairo_win32_font_map_class_init (PangoCairoWin32FontMapClass *class)
{
  GObjectClass *gobject_class = G_OBJECT_CLASS (class);
  PangoFontMapClass *fontmap_class = PANGO_FONT_MAP_CLASS (class);
  PangoWin32FontMapClass *win32fontmap_class = PANGO_WIN32_FONT_MAP_CLASS (class);

  gobject_class->finalize  = pango_cairo_win32_font_map_finalize;
  fontmap_class->get_serial = pango_cairo_win32_font_map_get_serial;
  fontmap_class->changed = pango_cairo_win32_font_map_changed;
  win32fontmap_class->find_font = pango_cairo_win32_font_map_find_font;
}

static void
pango_cairo_win32_font_map_init (PangoCairoWin32FontMap *cwfontmap)
{
  cwfontmap->serial = 1;
  cwfontmap->dpi = GetDeviceCaps (_pango_win32_get_display_dc (), LOGPIXELSY);
}
