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

#if defined (HAVE_CAIRO_FREETYPE)
#  include "pangocairo-fc.h"
#elif defined (HAVE_CAIRO_WIN32)
#  include "pangocairo-win32.h"
#endif

GType
pango_cairo_font_map_get_type (void)
{
  static GType cairo_font_map_type = 0;

  if (! cairo_font_map_type)
    {
      static const GTypeInfo cairo_font_map_info =
      {
	sizeof (PangoCairoFontMapIface), /* class_size */
	NULL,           /* base_init */
	NULL,		/* base_finalize */
	NULL,
	NULL,		/* class_finalize */
	NULL,		/* class_data */
	0,
	0,
	NULL
      };

      cairo_font_map_type =
	g_type_register_static (G_TYPE_INTERFACE, "PangoCairoFontMap",
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
 * Return value: the newly created fontmap object. Free
 * with g_object_unref().
 *
 * Since: 1.10
 **/
PangoFontMap *
pango_cairo_font_map_new (void)
{
  /* Make sure that the type system is initialized */
  g_type_init ();

#if defined(HAVE_CAIRO_WIN32)
  return g_object_new (PANGO_TYPE_CAIRO_WIN32_FONT_MAP, NULL);
#elif defined(HAVE_CAIRO_FREETYPE)
  return g_object_new (PANGO_TYPE_CAIRO_FC_FONT_MAP, NULL);
#endif
}

/**
 * pango_cairo_font_map_get_default:
 * 
 * Gets a default font map to use with Cairo. 
 * 
 * Return value: the default Cairo fontmap for Pango. This
 *  object is owned by Pango and must not be freed.
 *
 * Since: 1.10
 **/
PangoFontMap *
pango_cairo_font_map_get_default (void)
{
  static PangoFontMap *default_font_map = NULL;

  if (!default_font_map)
    default_font_map = pango_cairo_font_map_new ();

  return default_font_map;
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
 * Gets the resolutions for the fontmap. See pango_cairo_font_map_set_resolution.
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
 **/
PangoContext *
pango_cairo_font_map_create_context (PangoCairoFontMap *fontmap)
{
  PangoContext *context;
  
  g_return_val_if_fail (PANGO_IS_CAIRO_FONT_MAP (fontmap), NULL);
  
  context = pango_context_new ();
  pango_context_set_font_map (context, PANGO_FONT_MAP (fontmap));

  return context;
}

/**
 * _pango_cairo_font_map_get_renderer:
 * @fontmap: a #PangoCairoFontmap
 * 
 * Gets the singleton PangoCairoRenderer for this fontmap.
 * 
 * Return value: the singleton renderer
 **/
PangoRenderer *
_pango_cairo_font_map_get_renderer (PangoCairoFontMap *fontmap)
{
  g_return_val_if_fail (PANGO_IS_CAIRO_FONT_MAP (fontmap), NULL);
  
  return (* PANGO_CAIRO_FONT_MAP_GET_IFACE (fontmap)->get_renderer) (fontmap);
}

/**
 * pango_cairo_update_context:
 * @cr: a Cairo context
 * @context: a #PangoContext, from pango_cairo_font_map_create_context()
 * 
 * Updates a #PangoContext previously created for use with Cairo to
 * match the current transformation and target surface of a Cairo
 * context. If any layouts have been created for the context,
 * it's necessary to call pango_layout_context_changed() on those
 * layouts.
 *
 * Since: 1.10
 **/
void
pango_cairo_update_context (cairo_t      *cr,
			    PangoContext *context)
{
  cairo_matrix_t *cairo_matrix;
  PangoMatrix pango_matrix;
  
  g_return_if_fail (cr != NULL);
  g_return_if_fail (PANGO_IS_CONTEXT (context));

  cairo_matrix = cairo_matrix_create ();
  cairo_current_matrix (cr, cairo_matrix);
  cairo_matrix_get_affine (cairo_matrix,
			   &pango_matrix.xx, &pango_matrix.yx,
			   &pango_matrix.xy, &pango_matrix.yy,
			   &pango_matrix.x0, &pango_matrix.y0);

  pango_context_set_matrix (context, &pango_matrix);

  cairo_matrix_destroy (cairo_matrix);
}

/**
 * pango_cairo_create_layout:
 * @cr: a Cairo context
 * 
 * Creates a layout object set up to match the current transformation
 * and target surface of the Cairo context.  This layout can then be
 * used for text measurement with functions like
 * pango_layout_get_size() or drawing with functions like
 * pango_cairo_show_layout(). If you change the transformation
 * or target surface for @cr, you need to call pango_cairo_update_layout()
 *
 * This function is the most convenient way to use Cairo with Pango,
 * however it is slightly inefficient since it creates a separate
 * #PangoContext object for each layout. This might matter in an
 * application that was laying out large amounts of text.
 * 
 * Return value: the newly created #PangoLayout. Free with
 *   g_object_unref().
 *
 * Since: 1.10
 **/
PangoLayout *
pango_cairo_create_layout  (cairo_t *cr)
{
  PangoFontMap *fontmap;
  PangoContext *context;
  PangoLayout *layout;

  g_return_val_if_fail (cr != NULL, NULL);

  fontmap = pango_cairo_font_map_get_default ();
  context = pango_cairo_font_map_create_context (PANGO_CAIRO_FONT_MAP (fontmap));
  layout = pango_layout_new (context);

  pango_cairo_update_context (cr, context);
  g_object_unref (context);

  return layout;
}

/**
 * pango_cairo_update_layout:
 * @cr: a Cairo context
 * @layout: a #PangoLayout, from pango_cairo_create_layout()
 * 
 * Updates the private #PangoContext of a #PangoLayout created with
 * pango_cairo_create_layout() to match the current transformation
 * and target surface of a Cairo context.
 *
 * Since: 1.10
 **/
void
pango_cairo_update_layout (cairo_t     *cr,
			   PangoLayout *layout)
{
  g_return_if_fail (cr != NULL);
  g_return_if_fail (PANGO_IS_LAYOUT (layout));

  pango_cairo_update_context (cr, pango_layout_get_context (layout));
  pango_layout_context_changed (layout);
}

