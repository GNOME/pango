/* Pango
 * pango-fontmap.c: Font handling
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

#include "config.h"
#include "pango-fontmap.h"
#include "pango-impl-utils.h"
#include <stdlib.h>

static PangoFontset *pango_font_map_real_load_fontset (PangoFontMap               *fontmap,
						       PangoContext               *context,
						       const PangoFontDescription *desc,
						       PangoLanguage              *language);


G_DEFINE_ABSTRACT_TYPE (PangoFontMap, pango_font_map, G_TYPE_OBJECT)

static void
pango_font_map_class_init (PangoFontMapClass *class)
{
  class->load_fontset = pango_font_map_real_load_fontset;
}

static void
pango_font_map_init (PangoFontMap *fontmap)
{
}

/**
 * pango_font_map_create_context:
 * @fontmap: a #PangoFontMap
 *
 * Creates a #PangoContext connected to @fontmap.  This is equivalent
 * to pango_context_new() followed by pango_context_set_font_map().
 *
 * If you are using Pango as part of a higher-level system,
 * that system may have it's own way of create a #PangoContext.
 * For instance, the GTK+ toolkit has, among others,
 * gdk_pango_context_get_for_screen(), and
 * gtk_widget_get_pango_context().  Use those instead.
 *
 * Return value: the newly allocated #PangoContext, which should
 *               be freed with g_object_unref().
 *
 * Since: 1.22
 **/
PangoContext *
pango_font_map_create_context (PangoFontMap *fontmap)
{
  PangoContext *context;

  g_return_val_if_fail (fontmap != NULL, NULL);

  context = pango_context_new ();
  pango_context_set_font_map (context, fontmap);

  return context;
}

/**
 * pango_font_map_load_font:
 * @fontmap: a #PangoFontMap
 * @context: the #PangoContext the font will be used with
 * @desc: a #PangoFontDescription describing the font to load
 *
 * Load the font in the fontmap that is the closest match for @desc.
 *
 * Returns: the font loaded, or %NULL if no font matched.
 **/
PangoFont *
pango_font_map_load_font  (PangoFontMap               *fontmap,
			   PangoContext               *context,
			   const PangoFontDescription *desc)
{
  g_return_val_if_fail (fontmap != NULL, NULL);
  g_return_val_if_fail (pango_font_description_get_family (desc) != NULL, NULL);

  return PANGO_FONT_MAP_GET_CLASS (fontmap)->load_font (fontmap, context, desc);
}

/**
 * pango_font_map_list_families:
 * @fontmap: a #PangoFontMap
 * @families: location to store a pointer to an array of #PangoFontFamily *.
 *            This array should be freed with g_free().
 * @n_families: location to store the number of elements in @families
 *
 * List all families for a fontmap.
 **/
void
pango_font_map_list_families (PangoFontMap      *fontmap,
			      PangoFontFamily ***families,
			      int               *n_families)
{
  g_return_if_fail (fontmap != NULL);

  PANGO_FONT_MAP_GET_CLASS (fontmap)->list_families (fontmap, families, n_families);
}

/**
 * pango_font_map_load_fontset:
 * @fontmap: a #PangoFontMap
 * @context: the #PangoContext the font will be used with
 * @desc: a #PangoFontDescription describing the font to load
 * @language: a #PangoLanguage the fonts will be used for
 *
 * Load a set of fonts in the fontmap that can be used to render
 * a font matching @desc.
 *
 * Returns: the fontset, or %NULL if no font matched.
 **/
PangoFontset *
pango_font_map_load_fontset (PangoFontMap                 *fontmap,
			     PangoContext                 *context,
			     const PangoFontDescription   *desc,
			     PangoLanguage                *language)
{
  g_return_val_if_fail (fontmap != NULL, NULL);
  g_return_val_if_fail (pango_font_description_get_family (desc) != NULL, NULL);

  return PANGO_FONT_MAP_GET_CLASS (fontmap)->load_fontset (fontmap, context, desc, language);
}

static void
pango_font_map_fontset_add_fonts (PangoFontMap          *fontmap,
				  PangoContext          *context,
				  PangoFontsetSimple    *fonts,
				  PangoFontDescription  *desc,
				  const char            *family)
{
  char **aliases;
  int n_aliases;
  int j;
  PangoFont *font;

  pango_lookup_aliases (family,
			&aliases,
			&n_aliases);

  if (n_aliases)
    {
      for (j = 0; j < n_aliases; j++)
	{
	  pango_font_description_set_family_static (desc, aliases[j]);
	  font = pango_font_map_load_font (fontmap, context, desc);
	  if (font)
	    pango_fontset_simple_append (fonts, font);
	}
    }
  else
    {
      pango_font_description_set_family_static (desc, family);
      font = pango_font_map_load_font (fontmap, context, desc);
      if (font)
	pango_fontset_simple_append (fonts, font);
    }
}

static PangoFontset *
pango_font_map_real_load_fontset (PangoFontMap               *fontmap,
				  PangoContext               *context,
				  const PangoFontDescription *desc,
				  PangoLanguage              *language)
{
  PangoFontDescription *tmp_desc = pango_font_description_copy_static (desc);
  char **families;
  int i;
  PangoFontsetSimple *fonts;
  static GHashTable *warned_fonts = NULL;

  families = g_strsplit (pango_font_description_get_family (desc), ",", -1);

  fonts = pango_fontset_simple_new (language);

  for (i = 0; families[i]; i++)
    pango_font_map_fontset_add_fonts (fontmap,
				      context,
				      fonts,
				      tmp_desc,
				      families[i]);

  g_strfreev (families);

  /* The font description was completely unloadable, try with
   * family == "Sans"
   */
  if (pango_fontset_simple_size (fonts) == 0)
    {
      char *ctmp1, *ctmp2;

      pango_font_description_set_family_static (tmp_desc,
						pango_font_description_get_family (desc));

      ctmp1 = pango_font_description_to_string (desc);
      pango_font_description_set_family_static (tmp_desc, "Sans");

      if (!warned_fonts || !g_hash_table_lookup (warned_fonts, ctmp1))
	{
	  if (!warned_fonts)
	    warned_fonts = g_hash_table_new (g_str_hash, g_str_equal);

	  g_hash_table_insert (warned_fonts, g_strdup (ctmp1), GINT_TO_POINTER (1));

	  ctmp2 = pango_font_description_to_string (tmp_desc);
	  g_warning ("couldn't load font \"%s\", falling back to \"%s\", "
		     "expect ugly output.", ctmp1, ctmp2);
	  g_free (ctmp2);
	}
      g_free (ctmp1);

      pango_font_map_fontset_add_fonts (fontmap,
					context,
					fonts,
					tmp_desc,
					"Sans");
    }

  /* We couldn't try with Sans and the specified style. Try Sans Normal
   */
  if (pango_fontset_simple_size (fonts) == 0)
    {
      char *ctmp1, *ctmp2;

      pango_font_description_set_family_static (tmp_desc, "Sans");
      ctmp1 = pango_font_description_to_string (tmp_desc);
      pango_font_description_set_style (tmp_desc, PANGO_STYLE_NORMAL);
      pango_font_description_set_weight (tmp_desc, PANGO_WEIGHT_NORMAL);
      pango_font_description_set_variant (tmp_desc, PANGO_VARIANT_NORMAL);
      pango_font_description_set_stretch (tmp_desc, PANGO_STRETCH_NORMAL);

      if (!warned_fonts || !g_hash_table_lookup (warned_fonts, ctmp1))
	{
	  g_hash_table_insert (warned_fonts, g_strdup (ctmp1), GINT_TO_POINTER (1));

	  ctmp2 = pango_font_description_to_string (tmp_desc);

	  g_warning ("couldn't load font \"%s\", falling back to \"%s\", "
		     "expect ugly output.", ctmp1, ctmp2);
	  g_free (ctmp2);
	}

      g_free (ctmp1);

      pango_font_map_fontset_add_fonts (fontmap,
					context,
					fonts,
					tmp_desc,
					"Sans");
    }

  pango_font_description_free (tmp_desc);

  /* Everything failed, we are screwed, there is no way to continue,
   * but lets just not crash here.
   */
  if (pango_fontset_simple_size (fonts) == 0)
      g_warning ("All font fallbacks failed!!!!");

  return PANGO_FONTSET (fonts);
}

/**
 * pango_font_map_get_shape_engine_type:
 * @fontmap: a #PangoFontMap
 *
 * Returns the render ID for shape engines for this fontmap.
 * See the <structfield>render_type</structfield> field of
 * #PangoEngineInfo.
  *
 * Return value: the ID string for shape engines for
 *  this fontmap. Owned by Pango, should not be modified
 *  or freed.
 *
 * Since: 1.4
 **/
G_CONST_RETURN char *
pango_font_map_get_shape_engine_type (PangoFontMap *fontmap)
{
  g_return_val_if_fail (PANGO_IS_FONT_MAP (fontmap), NULL);

  return PANGO_FONT_MAP_GET_CLASS (fontmap)->shape_engine_type;
}

