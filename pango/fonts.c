/* Pango
 * fonts.c:
 *
 * Copyright (C) 1999 Red Hat Software
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

#include <stdlib.h>
#include <ctype.h>

#include "pango.h"

/**
 * pango_font_description_copy:
 * @desc: a #PangoFontDescription
 * 
 * Make a copy of a #PangoFontDescription.
 * 
 * Return value: a newly allocated #PangoFontDescription. This value
 *               must be freed using pango_font_description_free().
 **/
PangoFontDescription *
pango_font_description_copy  (const PangoFontDescription  *desc)
{
  PangoFontDescription *result = g_new (PangoFontDescription, 1);

  *result = *desc;

  result->family_name = g_strdup (result->family_name);

  return result;
}

/**
 * pango_font_description_compare:
 * @desc1: a #PangoFontDescription
 * @desc2: another #PangoFontDescription
 * 
 * Compare two font descriptions for equality.
 * 
 * Return value: %TRUE if the two font descriptions are proveably
 *               identical. (Two font descriptions may result in
 *               identical fonts being loaded, but still compare
 *                %FALSE.)
 **/
gboolean
pango_font_description_compare (const PangoFontDescription  *desc1,
				const PangoFontDescription  *desc2)
{
  g_return_val_if_fail (desc1 != NULL, FALSE);
  g_return_val_if_fail (desc2 != NULL, FALSE);

  return (desc1->style == desc2->style &&
	  desc1->variant == desc2->variant &&
	  desc1->weight == desc2->weight &&
	  desc1->stretch == desc2->stretch &&
	  desc1->size == desc2->size &&
	  g_strcasecmp (desc1->family_name, desc2->family_name));
}

/**
 * pango_font_description_free:
 * @desc: a #PangoFontDescription
 * 
 * Free a font description returned from pango_font_describe()
 * or pango_font_description_copy().
 **/
void pango_font_description_free  (PangoFontDescription  *desc)
{
  if (desc)
    {
      if (desc->family_name)
	g_free (desc->family_name);

      g_free (desc);
    }
}

/**
 * pango_font_descriptions_free:
 * @descs: a pointer to an array of #PangoFontDescription
 * @n_descs: number of font descriptions in @descs
 * 
 * Free a list of font descriptions from pango_font_map_list_fonts()
 **/
void
pango_font_descriptions_free (PangoFontDescription **descs,
			      int                    n_descs)
{
  int i;

  if (descs)
    {
      for (i = 0; i<n_descs; i++)
	pango_font_description_free (descs[i]);
      g_free (descs);
    }
}

typedef struct
{
  int value;
  const char *str;
} FieldMap;

FieldMap style_map[] = {
  { PANGO_STYLE_NORMAL, NULL },
  { PANGO_STYLE_OBLIQUE, "Oblique" },
  { PANGO_STYLE_ITALIC, "Italic" }
};

FieldMap variant_map[] = {
  { PANGO_VARIANT_NORMAL, NULL },
  { PANGO_VARIANT_SMALL_CAPS, "Small-Caps" }
};

FieldMap weight_map[] = {
  { 300, "Light" },
  { PANGO_WEIGHT_NORMAL, NULL },
  { 500, "Medium" },
  { 600, "Semi-Bold" },
  { PANGO_WEIGHT_BOLD, "Bold" }
};

FieldMap stretch_map[] = {
  { PANGO_STRETCH_ULTRA_CONDENSED, "Ultra-Condensed" },
  { PANGO_STRETCH_EXTRA_CONDENSED, "Extra-Condensed" },
  { PANGO_STRETCH_CONDENSED,       "Condensed" },
  { PANGO_STRETCH_SEMI_CONDENSED,  "Semi-Condensed" },
  { PANGO_STRETCH_NORMAL,          NULL },
  { PANGO_STRETCH_SEMI_EXPANDED,   "Semi-Expanded" },
  { PANGO_STRETCH_EXPANDED,        "Expanded" },
  { PANGO_STRETCH_EXTRA_EXPANDED,  "Extra-Expanded" },
  { PANGO_STRETCH_ULTRA_EXPANDED,  "Ultra-Expanded" }
};

static gboolean
find_field (FieldMap *map, int n_elements, const char *str, int len, int *val)
{
  int i;
  
  for (i=0; i<n_elements; i++)
    {
      if (map[i].str && g_strncasecmp (map[i].str, str, len) == 0)
	{
	  if (val)
	    *val = map[i].value;
	  return TRUE;
	}
    }

  return FALSE;
}

static gboolean
find_field_any (const char *str, int len, PangoFontDescription *desc)
{
  return (g_strcasecmp (str, "Normal") == 0 ||
	  find_field (style_map, G_N_ELEMENTS (style_map), str, len,
		      desc ? (int *)&desc->style : NULL) ||
	  find_field (variant_map, G_N_ELEMENTS (variant_map), str, len,
		      desc ? (int *)&desc->variant : NULL) ||
	  find_field (weight_map, G_N_ELEMENTS (weight_map), str, len,
		      desc ? (int *)&desc->weight : NULL) || 
	  find_field (stretch_map, G_N_ELEMENTS (stretch_map), str, len,
		      desc ? (int *)&desc->stretch : NULL));
}

static const char *
getword (const char *str, const char *last, size_t *wordlen)
{
  const char *result;
  
  while (last > str && isspace (*(last - 1)))
    last--;

  result = last;
  while (result > str && !isspace (*(result - 1)))
    result--;

  *wordlen = last - result;
  
  return result;
}

/**
 * pango_font_description_from_string:
 * @str: string reprentation of a font description.
 * 
 * Create a new font description from a string representation in the
 * form "[FAMILY-LIST] [STYLE-OPTIONS] [SIZE]", where FAMILY-LIST is a
 * comma separated list of families optionally terminated by a comma,
 * STYLE_OPTIONS is a whitespace separated list of words where each
 * WORD describes one of style, variant, weight, or stretch, and SIZE
 * is an decimal number (size in points). Any one of the options may
 * be absent.  If FAMILY-LIST is absent, then the family_name field of
 * the resulting font description will be initialized to NULL.  If
 * STYLE-OPTIONS is missing, then all style options will be set to the
 * default values. If SIZE is missing, the size in the resulting font
 * description will be set to 0.
 * 
 * Return value: a new #PangoFontDescription, or %NULL if the 
 * string could not be parsed. 
 **/
PangoFontDescription *
pango_font_description_from_string (const char *str)
{
  PangoFontDescription *desc;
  const char *p, *last;
  size_t len, wordlen;

  g_return_val_if_fail (str != NULL, NULL);

  desc = g_new (PangoFontDescription, 1);
  
  desc->family_name = NULL;
  desc->style = PANGO_STYLE_NORMAL;
  desc->weight = PANGO_WEIGHT_NORMAL;
  desc->variant = PANGO_VARIANT_NORMAL;
  desc->stretch = PANGO_STRETCH_NORMAL;
  
  desc->size = 0;

  len = strlen (str);
  last = str + len;
  p = getword (str, last, &wordlen);

  /* Look for a size at the end of the string
   */
  if (wordlen != 0)
    {
      char *end;
      double size = strtod (p, &end);
      if (end - p == wordlen) /* word is a valid float */
	{
	  desc->size = (int)(size * 1000 + 0.5);
	  last = p;
	}
    }

  /* Now parse style words
   */
  p = getword (str, last, &wordlen);
  while (wordlen != 0)
    {
      if (!find_field_any (p, wordlen, desc))
	break;
      else
	{
	  last = p;
	  p = getword (str, last, &wordlen);
	}
    }

  /* Remainder (str => p) is family list. Trim off trailing commas and leading and trailing white space
   */

  while (last > str && isspace (*(last - 1)))
    last--;

  if (last > str && *(last - 1) == ',')
    last--;

  while (last > str && isspace (*(last - 1)))
    last--;

  while (isspace (*str))
    str++;

  if (str != last)
    desc->family_name = g_strndup (str, last - str);

  return desc;
}

static void
append_field (GString *str, FieldMap *map, int n_elements, int val)
{
  int i;
  for (i=0; i<n_elements; i++)
    {
      if (map[i].value == val)
	{
	  if (map[i].str)
	    {
	      if (str->len != 0)
		g_string_append_c (str, ' ');
	      g_string_append (str, map[i].str);
	    }
	  return;
	}
    }
  
  if (str->len != 0)
    g_string_append_c (str, ' ');
  g_string_sprintfa (str, "%d", val);
}

/**
 * pango_font_description_to_string:
 * @desc: a #PangoFontDescription
 * 
 * Create a string representation of a font description. See
 * pango_font_description_from_string() for a description of the
 * format of the string representation. The family list in the
 * string description will only have a terminating comma if the
 * last word of the list is a valid style option.
 * 
 * Return value: a new string that must be freed with g_free().
 **/
char *
pango_font_description_to_string (const PangoFontDescription  *desc)
{
  GString *result = g_string_new (NULL);
  char *str;

  if (desc->family_name)
    {
      const char *p;
      size_t wordlen;

      g_string_append (result, desc->family_name);
      
      p = getword (desc->family_name, desc->family_name + strlen(desc->family_name), &wordlen);
      if (wordlen != 0 && find_field_any (p, wordlen, NULL))
	g_string_append_c (result, ',');
    }

  append_field (result, weight_map, G_N_ELEMENTS (weight_map), desc->weight);
  append_field (result, style_map, G_N_ELEMENTS (style_map), desc->style);
  append_field (result, stretch_map, G_N_ELEMENTS (stretch_map), desc->stretch);
  append_field (result, variant_map, G_N_ELEMENTS (variant_map), desc->variant);

  if (result->len == 0)
    g_string_append (result, "Normal");

  if (desc->size > 0)
    {
      if (result->len == 0)
	g_string_append_c (result, ' ');

      g_string_sprintfa (result, "%f", desc->size / 1000.);
    }
  
  str = result->str;
  g_string_free (result, FALSE);
  return str;
}

/**
 * pango_font_init:
 * @font:    a #PangoFont
 *
 * Initialize a #PangoFont structure. This should
 * only be called from the "new" routine of code which
 * is implementing  a "subclass" of #PangoFont
 */
void 
pango_font_init (PangoFont *font)
{
  g_return_if_fail (font != NULL);

  g_datalist_init (&font->data);
  font->ref_count = 1;
}

/**
 * pango_font_ref:
 * @font:    a #PangoFont
 *
 * Increase the reference count of a #PangoFont.
 */
void 
pango_font_ref (PangoFont *font)
{
  g_return_if_fail (font != NULL);

  font->ref_count++;
}


/**
 * pango_font_unref:
 * @font: a #PangoFont
 *
 * Decrease the reference count of a #PangoFont.
 * if the result is zero, destroy the font
 * and free the associated memory.
 */
void 
pango_font_unref (PangoFont *font)
{
  g_return_if_fail (font != NULL);
  g_return_if_fail (font->ref_count > 0);
  
  font->ref_count--;
  if (font->ref_count == 0)
    {
      g_datalist_clear (&font->data);
      font->klass->destroy (font);
    }
}

/**
 * pango_font_set_data:
 * @font:         a #PangoFont
 * @key:          a string identifying the type of user data.
 * @data:         the data to store. If %NULL, the current
 *                data for the key will be removed.
 * @destroy_func: a function to call when the data is no
 *                longer stored, either because the font has
 *                been destroyed, or because the data has
 *                been replaced. This can be %NULL, in which
 *                case no function will be called.
 *
 * Associate user data, tagged with a string id, with a particular
 * font. 
 */
void
pango_font_set_data (PangoFont   *font,
		     gchar         *key,
		     gpointer       data,
		     GDestroyNotify destroy_func)
{
  g_datalist_set_data_full (&font->data, key, data, destroy_func);
}

/**
 * pango_font_get_data:
 * @font:    a #PangoFont
 * @key:     a string identifying the type of user data.
 *
 * Look up user data tagged with a particular key.
 * 
 * Returns the data, or NULL if that key does not exist.
 */
gpointer
pango_font_get_data (PangoFont *font,
			gchar       *key)
{
  return g_datalist_get_data (&font->data, key);
}

/**
 * pango_font_get_coverage:
 * @font: a #PangoFont
 * @lang: the language tag (in the form of "en_US")
 * 
 * Compute the coverage map for a given font and language tag.
 * 
 * Return value: a newly allocated #PangoContext object.
 **/
PangoCoverage *
pango_font_get_coverage (PangoFont      *font,
			 const char     *lang)
{
  g_return_val_if_fail (font != NULL, NULL);
  g_return_val_if_fail (lang != NULL, NULL);

  return font->klass->get_coverage (font, lang);
}

/**
 * pango_font_find_shaper:
 * @font: a #PangoFont
 * @lang: the language tag (in the form of "en_US")
 * @ch: the ISO-10646 character code.
 * 
 * Find the best matching shaper for a font for a particular
 * language tag and character point.
 * 
 * Return value: the best matching shaper.
 **/
PangoEngineShape *
pango_font_find_shaper (PangoFont      *font,
			const char     *lang,
			guint32         ch)
{
  g_return_val_if_fail (font != NULL, NULL);
  g_return_val_if_fail (lang != NULL, NULL);

  return font->klass->find_shaper (font, lang, ch);
}

/**
 * pango_font_get_glyph_extents:
 * @font: a #PangoFont
 * @glyph: the glyph index
 * @ink_rect: rectangle used to store the extents of the glyph as drawn
 *            or %NULL to indicate that the result is not needed.
 * @logical_rect: rectangle used to store the logical extents of the glyph
 *            or %NULL to indicate that the result is not needed.
 * 
 * Get the logical and ink extents of a glyph within a font. The
 * coordinate system for each rectangle has its origin at the
 * base line and horizontal origin of the character with increasing
 * coordinates extending to the right and down. The macros PANGO_ASCENT(),
 * PANGO_DESCENT(), PANGO_LBEARING(), and PANGO_RBEARING can be used to convert
 * from the extents rectangle to more traditional font metrics. The units
 * of the rectangles are in 1000ths of a device unit.
 **/
void
pango_font_get_glyph_extents  (PangoFont      *font,
			       PangoGlyph      glyph,
			       PangoRectangle *ink_rect,
			       PangoRectangle *logical_rect)
{
  g_return_if_fail (font != NULL);

  font->klass->get_glyph_extents (font, glyph, ink_rect, logical_rect);
}
     
/**
 * pango_font_map_init:
 * @fontmap:    a #PangoFontMap
 *
 * Initialize a #PangoFontMap structure. This should
 * only be called from the "new" routine of code which
 * is implementing  a "subclass" of #PangoFontMap
 */
void 
pango_font_map_init (PangoFontMap *fontmap)
{
  g_return_if_fail (fontmap != NULL);

  fontmap->ref_count = 1;
}

/**
 * pango_font_map_ref:
 * @fontmap:    a #PangoFontMap
 *
 * Increase the reference count of a #PangoFontMap.
 */
void 
pango_font_map_ref (PangoFontMap *fontmap)
{
  g_return_if_fail (fontmap != NULL);

  fontmap->ref_count++;
}


/**
 * pango_font_map_unref:
 * @fontmap: a #PangoFontMap
 *
 * Decrease the reference count of a #PangoFontMap.
 * if the result is zero, destroy the font
 * and free the associated memory.
 */
void 
pango_font_map_unref (PangoFontMap *fontmap)
{
  g_return_if_fail (fontmap != NULL);
  g_return_if_fail (fontmap->ref_count > 0);
  
  fontmap->ref_count--;
  if (fontmap->ref_count == 0)
    fontmap->klass->destroy (fontmap);
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

  return fontmap->klass->load_font (fontmap, desc);
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

  fontmap->klass->list_fonts (fontmap, family, descs, n_descs);
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

  fontmap->klass->list_families (fontmap, families, n_families);
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
