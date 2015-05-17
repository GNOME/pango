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
pango_font_map_init (PangoFontMap *fontmap G_GNUC_UNUSED)
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
 * Return value: (transfer full): the newly allocated #PangoContext,
 *               which should be freed with g_object_unref().
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
 * Returns: (transfer full) (nullable): the newly allocated #PangoFont
 *          loaded, or %NULL if no font matched.
 **/
PangoFont *
pango_font_map_load_font  (PangoFontMap               *fontmap,
			   PangoContext               *context,
			   const PangoFontDescription *desc)
{
  g_return_val_if_fail (fontmap != NULL, NULL);

  return PANGO_FONT_MAP_GET_CLASS (fontmap)->load_font (fontmap, context, desc);
}

/**
 * pango_font_map_list_families:
 * @fontmap: a #PangoFontMap
 * @families: (out) (array length=n_families) (transfer container): location to store a pointer to an array of #PangoFontFamily *.
 *            This array should be freed with g_free().
 * @n_families: (out): location to store the number of elements in @families
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
 * Returns: (transfer full) (nullable): the newly allocated
 *          #PangoFontset loaded, or %NULL if no font matched.
 **/
PangoFontset *
pango_font_map_load_fontset (PangoFontMap                 *fontmap,
			     PangoContext                 *context,
			     const PangoFontDescription   *desc,
			     PangoLanguage                *language)
{
  g_return_val_if_fail (fontmap != NULL, NULL);

  return PANGO_FONT_MAP_GET_CLASS (fontmap)->load_fontset (fontmap, context, desc, language);
}

static void
pango_font_map_fontset_add_fonts (PangoFontMap          *fontmap,
				  PangoContext          *context,
				  PangoFontsetSimple    *fonts,
				  PangoFontDescription  *desc,
				  const char            *family)
{
  PangoFont *font;

  pango_font_description_set_family_static (desc, family);
  font = pango_font_map_load_font (fontmap, context, desc);
  if (font)
    pango_fontset_simple_append (fonts, font);
}

static PangoFontset *
pango_font_map_real_load_fontset (PangoFontMap               *fontmap,
				  PangoContext               *context,
				  const PangoFontDescription *desc,
				  PangoLanguage              *language)
{
  PangoFontDescription *tmp_desc = pango_font_description_copy_static (desc);
  const char *family;
  char **families;
  int i;
  PangoFontsetSimple *fonts;
  static GHashTable *warned_fonts = NULL; /* MT-safe */
  G_LOCK_DEFINE_STATIC (warned_fonts);

  family = pango_font_description_get_family (desc);
  families = g_strsplit (family ? family : "", ",", -1);

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

      G_LOCK (warned_fonts);
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
      G_UNLOCK (warned_fonts);
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

      G_LOCK (warned_fonts);
      if (!warned_fonts || !g_hash_table_lookup (warned_fonts, ctmp1))
	{
	  g_hash_table_insert (warned_fonts, g_strdup (ctmp1), GINT_TO_POINTER (1));

	  ctmp2 = pango_font_description_to_string (tmp_desc);

	  g_warning ("couldn't load font \"%s\", falling back to \"%s\", "
		     "expect ugly output.", ctmp1, ctmp2);
	  g_free (ctmp2);
	}
      G_UNLOCK (warned_fonts);
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
 * Deprecated: 1.38
 **/
const char *
pango_font_map_get_shape_engine_type (PangoFontMap *fontmap)
{
  g_return_val_if_fail (PANGO_IS_FONT_MAP (fontmap), NULL);

  return PANGO_FONT_MAP_GET_CLASS (fontmap)->shape_engine_type;
}

/**
 * pango_font_map_get_serial:
 * @fontmap: a #PangoFontMap
 *
 * Returns the current serial number of @fontmap.  The serial number is
 * initialized to an small number larger than zero when a new fontmap
 * is created and is increased whenever the fontmap is changed. It may
 * wrap, but will never have the value 0. Since it can wrap, never compare
 * it with "less than", always use "not equals".
 *
 * The fontmap can only be changed using backend-specific API, like changing
 * fontmap resolution.
 *
 * This can be used to automatically detect changes to a #PangoFontMap, like
 * in #PangoContext.
 *
 * Return value: The current serial number of @fontmap.
 *
 * Since: 1.32.4
 **/
guint
pango_font_map_get_serial (PangoFontMap *fontmap)
{
  g_return_val_if_fail (PANGO_IS_FONT_MAP (fontmap), 0);

  if (PANGO_FONT_MAP_GET_CLASS (fontmap)->get_serial)
    return PANGO_FONT_MAP_GET_CLASS (fontmap)->get_serial (fontmap);
  else
    return 1;
}

/**
 * pango_font_map_changed:
 * @fontmap: a #PangoFontMap
 *
 * Forces a change in the context, which will cause any #PangoContext
 * using this fontmap to change.
 *
 * This function is only useful when implementing a new backend
 * for Pango, something applications won't do. Backends should
 * call this function if they have attached extra data to the context
 * and such data is changed.
 *
 * Since: 1.34
 **/
void
pango_font_map_changed (PangoFontMap *fontmap)
{
  g_return_if_fail (PANGO_IS_FONT_MAP (fontmap));

  if (PANGO_FONT_MAP_GET_CLASS (fontmap)->changed)
    PANGO_FONT_MAP_GET_CLASS (fontmap)->changed (fontmap);
}
