/* Pango
 * pango-font.h: Font handling
 *
 * Copyright (C) 2000 Red Hat Software
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

#include "pango-fontmap.h"

static void pango_font_map_init       (PangoFontMap      *fontmap);
static void pango_font_map_class_init (PangoFontMapClass *class);

GType
pango_font_map_get_type (void)
{
  static GType object_type = 0;

  if (!object_type)
    {
      static const GTypeInfo object_info =
      {
        sizeof (PangoFontMapClass),
        (GBaseInitFunc) NULL,
        (GBaseFinalizeFunc) NULL,
        (GClassInitFunc) pango_font_map_class_init,
        NULL,           /* class_finalize */
        NULL,           /* class_data */
        sizeof (PangoFontMap),
        0,              /* n_preallocs */
        (GInstanceInitFunc) pango_font_map_init,
      };
      
      object_type = g_type_register_static (G_TYPE_OBJECT,
                                            "PangoFontMap",
                                            &object_info);
    }
  
  return object_type;
}

static void 
pango_font_map_init (PangoFontMap *fontmap)
{
}

static void
pango_font_map_class_init (PangoFontMapClass *class)
{
}

/**
 * pango_font_map_load_font:
 * @fontmap: a #PangoFontMap
 * @desc: a #PangoFontDescription describing the font to load
 * 
 * Load the font in the fontmap that is the closest match for @desc.
 *
 * Returns the font loaded, or %NULL if no font matched.
 **/
PangoFont *
pango_font_map_load_font  (PangoFontMap               *fontmap,
			   const PangoFontDescription *desc)
{
  g_return_val_if_fail (fontmap != NULL, NULL);

  return PANGO_FONT_MAP_GET_CLASS (fontmap)->load_font (fontmap, desc);
}

/**
 * pango_font_map_list_fonts:
 * @fontmap: a #PangoFontMap
 * @family: the family for which to list the fonts, or %NULL
 *          to list fonts in all families.
 * @descs: location to store a pointer to an array of pointers to
 *         #PangoFontDescription. This array should be freed
 *         with pango_font_descriptions_free().
 * @n_descs: location to store the number of elements in @descs
 * 
 * List all fonts in a fontmap, or the fonts in a particular family.
 **/
void
pango_font_map_list_fonts (PangoFontMap           *fontmap,
			   const char             *family,
			   PangoFontDescription ***descs,
			   int                    *n_descs)
{
  g_return_if_fail (fontmap != NULL);

  return PANGO_FONT_MAP_GET_CLASS (fontmap)->list_fonts (fontmap, family, descs, n_descs);
}

/**
 * pango_font_map_list_families:
 * @fontmap: a #PangoFontMap
 * @families: location to store a pointer to an array of strings.
 *            This array should be freed with pango_font_map_free_families().
 * @n_families: location to store the number of elements in @descs
 * 
 * List all families for a fontmap. 
 **/
void
pango_font_map_list_families (PangoFontMap   *fontmap,
			      gchar        ***families,
			      int            *n_families)
{
  g_return_if_fail (fontmap != NULL);

  return PANGO_FONT_MAP_GET_CLASS (fontmap)->list_families (fontmap, families, n_families);
}

/**
 * pango_font_map_free_families:
 * @families: a list of families
 * @n_families: number of elements in @families
 * 
 * Free a list of families returned from pango_font_map_list_families()
 **/
void
pango_font_map_free_families (gchar       **families,
			      int           n_families)
{
  int i;
  
  g_return_if_fail (n_families == 0 || families != NULL);

  for (i=0; i<n_families; i++)
    g_free (families[i]);

  g_free (families);
}


