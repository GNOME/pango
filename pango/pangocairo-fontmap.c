/* Pango
 * pangocairo-fontmap.c: Cairo font handling
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

#include "pangocairo.h"
#include "pangocairo-private.h"
#include "pango-impl-utils.h"

#if defined (HAVE_CAIRO_ATSUI)
#  include "pangocairo-atsui.h"
#endif
#if defined (HAVE_CAIRO_WIN32)
#  include "pangocairo-win32.h"
#endif
#if defined (HAVE_CAIRO_FREETYPE)
#  include "pangocairo-fc.h"
#endif

GType
pango_cairo_font_map_get_type (void)
{
  static GType cairo_font_map_type = 0;

  if (! cairo_font_map_type)
    {
      const GTypeInfo cairo_font_map_info =
      {
	sizeof (PangoCairoFontMapIface), /* class_size */
	NULL,           /* base_init */
	NULL,		/* base_finalize */
	NULL,
	NULL,		/* class_finalize */
	NULL,		/* class_data */
	0,
	0,
	NULL,
	NULL
      };

      cairo_font_map_type =
	g_type_register_static (G_TYPE_INTERFACE, I_("PangoCairoFontMap"),
				&cairo_font_map_info, 0);

      g_type_interface_add_prerequisite (cairo_font_map_type, PANGO_TYPE_FONT_MAP);
    }

  return cairo_font_map_type;
}

/**
 * pango_cairo_font_map_new:
 *
 * Creates a new #PangoCairoFontMap object; a fontmap is used
 * to cache information about available fonts, and holds
 * certain global parameters such as the resolution.
 * In most cases, you can use pango_cairo_font_map_get_default()
 * instead.
 *
 * Note that the type of the returned object will depend
 * on the particular font backend Cairo was compiled to use;
 * You generally should only use the #PangoFontMap and
 * #PangoCairoFontMap interfaces on the returned object.
 *
 * Return value: the newly allocated #PangoFontMap, which should
 *               be freed with g_object_unref().
 *
 * Since: 1.10
 **/
PangoFontMap *
pango_cairo_font_map_new (void)
{
  /* Make sure that the type system is initialized */
  g_type_init ();

#if defined(HAVE_CAIRO_ATSUI)
  return g_object_new (PANGO_TYPE_CAIRO_ATSUI_FONT_MAP, NULL);
#elif defined(HAVE_CAIRO_WIN32)
  return g_object_new (PANGO_TYPE_CAIRO_WIN32_FONT_MAP, NULL);
#elif defined(HAVE_CAIRO_FREETYPE)
  return g_object_new (PANGO_TYPE_CAIRO_FC_FONT_MAP, NULL);
#else
  g_assert_not_reached ();
  return NULL;
#endif
}

/**
 * pango_cairo_font_map_new_for_font_type:
 * @fonttype: desired #cairo_font_type_t
 *
 * Creates a new #PangoCairoFontMap object of the type suitable
 * to be used with cairo font backend of type @fonttype.
 *
 * In most cases one should simply use @pango_cairo_font_map_new(),
 * or in fact in most of those cases, just use
 * @pango_cairo_font_map_get_default().
 *
 * Return value: the newly allocated #PangoFontMap of suitable type
 *               which should be freed with g_object_unref(),
 *               or %NULL if the requested cairo font backend is
 *               not supported / compiled in.
 *
 * Since: 1.18
 **/
PangoFontMap *
pango_cairo_font_map_new_for_font_type (cairo_font_type_t fonttype)
{
  /* Make sure that the type system is initialized */
  g_type_init ();

  switch (fonttype)
  {
#if defined(HAVE_CAIRO_ATSUI)
    case CAIRO_FONT_TYPE_QUARTZ:
      return g_object_new (PANGO_TYPE_CAIRO_ATSUI_FONT_MAP, NULL);
#endif
#if defined(HAVE_CAIRO_WIN32)
    case CAIRO_FONT_TYPE_WIN32:
      return g_object_new (PANGO_TYPE_CAIRO_WIN32_FONT_MAP, NULL);
#endif
#if defined(HAVE_CAIRO_FREETYPE)
    case CAIRO_FONT_TYPE_FT:
      return g_object_new (PANGO_TYPE_CAIRO_FC_FONT_MAP, NULL);
#endif
    default:
      return NULL;
  }
}

static PangoFontMap *default_font_map = NULL;

/**
 * pango_cairo_font_map_get_default:
 *
 * Gets a default #PangoCairoFontMap to use with Cairo.
 *
 * Note that the type of the returned object will depend
 * on the particular font backend Cairo was compiled to use;
 * You generally should only use the #PangoFontMap and
 * #PangoCairoFontMap interfaces on the returned object.
 *
 * The default Cairo fontmap can be changed by using
 * pango_cairo_font_map_set_default().  This can be used to
 * change the Cairo font backend that the default fontmap
 * uses for example.
 *
 * Return value: the default Cairo fontmap for Pango. This
 *  object is owned by Pango and must not be freed.
 *
 * Since: 1.10
 **/
PangoFontMap *
pango_cairo_font_map_get_default (void)
{
  if (G_UNLIKELY (!default_font_map))
    default_font_map = pango_cairo_font_map_new ();

  return default_font_map;
}

/**
 * pango_cairo_font_map_set_default:
 * @fontmap: The new default font map, or %NULL
 *
 * Sets a default #PangoCairoFontMap to use with Cairo.
 *
 * This can be used to change the Cairo font backend that the
 * default fontmap uses for example.  The old default font map
 * is unreffed and the new font map referenced.
 *
 * A value of %NULL for @fontmap will cause the current default
 * font map to be released and a new default font
 * map to be created on demand, using pango_cairo_font_map_new().
 *
 * Since: 1.22
 **/
void
pango_cairo_font_map_set_default (PangoCairoFontMap *fontmap)
{
  g_return_if_fail (fontmap == NULL || PANGO_IS_CAIRO_FONT_MAP (fontmap));

  if ((PangoFontMap *) fontmap == default_font_map)
    return;

  if (default_font_map)
    g_object_unref (default_font_map);

  if (fontmap)
    g_object_ref (fontmap);
  default_font_map = fontmap;
}

/**
 * pango_cairo_font_map_set_resolution:
 * @fontmap: a #PangoCairoFontMap
 * @dpi: the resolution in "dots per inch". (Physical inches aren't actually
 *   involved; the terminology is conventional.)
 *
 * Sets the resolution for the fontmap. This is a scale factor between
 * points specified in a #PangoFontDescription and Cairo units. The
 * default value is 96, meaning that a 10 point font will be 13
 * units high. (10 * 96. / 72. = 13.3).
 *
 * Since: 1.10
 **/
void
pango_cairo_font_map_set_resolution (PangoCairoFontMap *fontmap,
				     double             dpi)
{
  g_return_if_fail (PANGO_IS_CAIRO_FONT_MAP (fontmap));

  (* PANGO_CAIRO_FONT_MAP_GET_IFACE (fontmap)->set_resolution) (fontmap, dpi);
}

/**
 * pango_cairo_font_map_get_resolution:
 * @fontmap: a #PangoCairoFontMap
 *
 * Gets the resolution for the fontmap. See pango_cairo_font_map_set_resolution()
 *
 * Return value: the resolution in "dots per inch"
 *
 * Since: 1.10
 **/
double
pango_cairo_font_map_get_resolution (PangoCairoFontMap *fontmap)
{
  g_return_val_if_fail (PANGO_IS_CAIRO_FONT_MAP (fontmap), 96.);

  return (* PANGO_CAIRO_FONT_MAP_GET_IFACE (fontmap)->get_resolution) (fontmap);
}

/**
 * pango_cairo_font_map_create_context:
 * @fontmap: a #PangoCairoFontMap
 *
 * Create a #PangoContext for the given fontmap.
 *
 * Return value: the newly created context; free with g_object_unref().
 *
 * Since: 1.10
 *
 * Deprecated: 1.22: Use pango_font_map_create_context() instead.
 **/
PangoContext *
pango_cairo_font_map_create_context (PangoCairoFontMap *fontmap)
{
  g_return_val_if_fail (PANGO_IS_CAIRO_FONT_MAP (fontmap), NULL);

  return pango_font_map_create_context (PANGO_FONT_MAP (fontmap));
}

/**
 * pango_cairo_font_map_get_font_type:
 * @fontmap: a #PangoCairoFontMap
 *
 * Gets the type of Cairo font backend that @fontmap uses.  
 *
 * Return value: the #cairo_font_type_t cairo font backend type
 *
 * Since: 1.18
 **/
cairo_font_type_t
pango_cairo_font_map_get_font_type (PangoCairoFontMap *fontmap)
{
  g_return_val_if_fail (PANGO_IS_CAIRO_FONT_MAP (fontmap), CAIRO_FONT_TYPE_TOY);

  return (* PANGO_CAIRO_FONT_MAP_GET_IFACE (fontmap)->get_font_type) (fontmap);
}
