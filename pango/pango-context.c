/* Pango
 * pango-context.c: Contexts for itemization and shaping
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

#include <pango-context.h>
#include <fribidi/fribidi.h>
#include <unicode.h>
#include "iconv.h"
#include "utils.h"
#include "modules.h"


struct _PangoContext
{
  gint ref_count;

  char *lang;
  PangoDirection base_dir;

  GSList *font_maps;
};

/**
 * pango_context_new:
 * 
 * Creates a new #PangoContext initialized to default value.
 * 
 * Return value: the new #PangoContext
 **/
PangoContext *
pango_context_new (void)
{
  PangoContext *result = g_new (PangoContext, 1);
  result->ref_count = 1;
  result->base_dir = PANGO_DIRECTION_LTR;
  result->lang = NULL;
  result->font_maps = NULL;

  return result;
}

/**
 * pango_context_ref:
 * @context: a #PangoContext
 * 
 * Increases the reference count of a #PangoContext.
 **/
void
pango_context_ref (PangoContext *context)
{
  g_return_if_fail (context != NULL);

  context->ref_count++;
}


/**
 * pango_context_unref:
 * @context: a #PangoContext
 *
 * Decreases the reference count of a #PangoContext.
 * if the result is zero, destroy the context
 * and free the associated memory.
 */
void
pango_context_unref (PangoContext *context)
{
  g_return_if_fail (context != NULL);

  context->ref_count--;
  if (context->ref_count == 0)
    {
      if (context->lang)
	g_free (context->lang);

      g_slist_foreach (context->font_maps, (GFunc)pango_font_map_unref, NULL);
      g_slist_free (context->font_maps);
    }
}

/**
 * pango_context_add_font_map:
 * @context: a #PangoContext
 * @font_map: the #PangoFontMap to add.
 * 
 * Add a font map to the list of font maps that are searched for fonts
 * when fonts are looked-up in this context.
 **/
void
pango_context_add_font_map (PangoContext *context,
			    PangoFontMap *font_map)
{
  g_return_if_fail (context != NULL);
  g_return_if_fail (font_map != NULL);
  
  context->font_maps =  g_slist_append (context->font_maps, font_map);
}

/**
 * pango_context_list_fonts:
 * @context: a #PangoContext
 * @family: the family for which to list the fonts, or %NULL
 *          to list fonts in all families.
 * @descs: location to store a pointer to an array of pointers to
 *         #PangoFontDescription. This array should be freed
 *         with pango_font_descriptions_free()
 * @n_descs: location to store the number of elements in @descs
 * 
 * Lists all fonts in all fontmaps for this context, or all
 * fonts in a particular family.
 **/
void
pango_context_list_fonts (PangoContext           *context,
			  const char             *family,
			  PangoFontDescription ***descs,
			  int                    *n_descs)
{
  int n_maps;
  
  g_return_if_fail (context != NULL);
  g_return_if_fail (descs == NULL || n_descs != NULL);

  if (n_descs == 0)
    return;
  
  n_maps = g_slist_length (context->font_maps);
  
  if (n_maps == 0)
    {
      *n_descs = 0;
      if (descs)
	*descs = NULL;
      return;
    }
  else if (n_maps == 1)
    pango_font_map_list_fonts (context->font_maps->data, family, descs, n_descs);
  else
    {
      /* FIXME: This does not properly suppress duplicate fonts! */
      
      PangoFontDescription ***tmp_descs;
      int *tmp_n_descs;
      int total_n_descs = 0;
      GSList *tmp_list;
      int i;

      tmp_descs = g_new (PangoFontDescription **, n_maps);
      tmp_n_descs = g_new (int, n_maps);

      *n_descs = 0;

      tmp_list = context->font_maps;
      for (i = 0; i<n_maps; i++)
	{
	  pango_font_map_list_fonts (tmp_list->data, family, &tmp_descs[i], &tmp_n_descs[i]);
	  *n_descs += tmp_n_descs[i];

	  tmp_list = tmp_list->next;
	}

      if (descs)
	{
	  *descs = g_new (PangoFontDescription *, *n_descs);
	  
	  total_n_descs = 0;
	  for (i = 0; i<n_maps; i++)
	    {
	      memcpy (&(*descs)[total_n_descs], tmp_descs[i], tmp_n_descs[i] * sizeof (PangoFontDescription *));
	      total_n_descs += tmp_n_descs[i];
	      pango_font_descriptions_free (tmp_descs[i], tmp_n_descs[i]);
	    }
	}
      else
	{
	  for (i = 0; i<n_maps; i++)
	    pango_font_descriptions_free (tmp_descs[i], tmp_n_descs[i]);
	}
	  
      g_free (tmp_descs);
      g_free (tmp_n_descs);
    }
}

typedef struct
{
  int n_found;
  char **families;
} ListFamiliesInfo;

static void
list_families_foreach (gpointer key, gpointer value, gpointer user_data)
{
  ListFamiliesInfo *info = user_data;

  if (info->families)
    info->families[info->n_found++] = value;

  g_free (value);
}

/**
 * pango_context_list_families:
 * @context: a #PangoContext
 * @families: location to store a pointer to an array of strings.
 *            This array should be freed with pango_font_map_free_families().
 * @n_families: location to store the number of elements in @descs
 * 
 * List all families for a context.
 **/
void
pango_context_list_families (PangoContext          *context,
			     gchar              ***families,
			     int                  *n_families)
{
  int n_maps;
  
  g_return_if_fail (context != NULL);
  g_return_if_fail (families == NULL || n_families != NULL);

  if (n_families == NULL)
    return;
  
  n_maps = g_slist_length (context->font_maps);
  
  if (n_maps == 0)
    {
      *n_families = 0;
      if (families)
	*families = NULL;
      
      return;
    }
  else if (n_maps == 1)
    pango_font_map_list_families (context->font_maps->data, families, n_families);
  else
    {
      GHashTable *family_hash;
      GSList *tmp_list;
      ListFamiliesInfo info;

      *n_families = 0;

      family_hash = g_hash_table_new (g_str_hash, g_str_equal);

      tmp_list = context->font_maps;
      while (tmp_list)
	{
	  char **tmp_families;
	  int tmp_n_families;
	  int i;
	  
	  pango_font_map_list_families (tmp_list->data, &tmp_families, &tmp_n_families);

	  for (i=0; i<*n_families; i++)
	    {
	      if (!g_hash_table_lookup (family_hash, tmp_families[i]))
		{
		  char *family = g_strdup (tmp_families[i]);
		  
		  g_hash_table_insert (family_hash, family, family);
		  (*n_families)++;
		}
	    }
	  
	  pango_font_map_free_families (tmp_families, tmp_n_families);

	  tmp_list = tmp_list->next;
	}

      info.n_found = 0;

      if (families)
	{
	  *families = g_new (char *, *n_families);
	  info.families = *families;
	}
      else
	info.families = NULL;
	  
      g_hash_table_foreach (family_hash, list_families_foreach, &info);
      g_hash_table_destroy (family_hash);
    }
}

/**
 * pango_context_load_font:
 * @context: a #PangoContext
 * @desc: a #PangoFontDescription describing the font to load
 * @size: the size at which to load the font (in points) 
 * 
 * Loads the font in one of the fontmaps in the context
 * that is the closest match for @desc.
 *
 * Returns the font loaded, or %NULL if no font matched.
 **/
PangoFont *
pango_context_load_font (PangoContext         *context,
			 PangoFontDescription *desc,
			 gdouble               size)
{
  GSList *tmp_list;

  g_return_val_if_fail (context != NULL, NULL);

  tmp_list = context->font_maps;
  while (tmp_list)
    {
      PangoFont *font;
      
      font = pango_font_map_load_font (tmp_list->data, desc, size);
      if (font)
	return font;
      
      tmp_list = tmp_list->next;
    }

  return NULL;
}

/**
 * pango_context_set_lang:
 * @context: a #PangoContext
 * @lang: the new language tag.
 * 
 * Sets the global language tag for the context.
 **/
void
pango_context_set_lang (PangoContext *context,
			const char   *lang)
{
  g_return_if_fail (context != NULL);

  if (context->lang)
    g_free (context->lang);

  context->lang = g_strdup (lang);
}

/**
 * pango_context_get_lang:
 * @context: a #PangoContext
 * 
 * Retrieves the global language tag for the context.
 * 
 * Return value: the global language tag. This value must be freed with g_free().
 **/
char *
pango_context_get_lang (PangoContext *context)
{
  g_return_val_if_fail (context != NULL, NULL);

  return g_strdup (context->lang);
}

/**
 * pango_context_set_base_dir:
 * @context: a #PangoContext
 * @direction: the new base direction
 * 
 * Sets the base direction for the context.
 **/
void
pango_context_set_base_dir (PangoContext  *context,
			    PangoDirection direction)
{
  g_return_if_fail (context != NULL);

  context->base_dir = direction;
}

/**
 * pango_context_get_base_dir:
 * @context: 
 * 
 * Retrieves the base direction for the context.
 * 
 * Return value: the base direction for the context.
 **/
PangoDirection
pango_context_get_base_dir (PangoContext *context)
{
  g_return_val_if_fail (context != NULL, PANGO_DIRECTION_LTR);

  return context->base_dir;
}

static void add_engines (PangoContext     *context,
			 gchar              *text, 
			 gint                length,
			 PangoLangRange   *lang_info,
			 gint                n_langs,
			 gboolean            force_exact,
			 PangoEngineInfo **shape_engines,
			 PangoEngineInfo **lang_engines);

/**
 * pango_itemize:
 * @context:   a structure holding information that affects
               the itemization process.
 * @text:      the text to itemize.
 * @length:    the number of bytes (not characters) in text.
 *             This must be >= 0.
 * @lang_info: an array of language tagging information.
 * @n_langs:   the number of elements in @lang_info.
 *
 * Breaks a piece of text into segments with consistent
 * directional level and shaping engine.
 *
 * Returns a GList of PangoItem structures.
 */
GList *
pango_itemize (PangoContext   *context, 
		  gchar            *text, 
		  gint              length,
		  PangoLangRange *lang_info,
		  gint              n_langs)
{
  guint16 *text_ucs2;
  gint n_chars;
  guint8 *embedding_levels;
  FriBidiCharType base_dir;
  gint i;
  PangoItem *item;
  char *p, *next;
  GList *result = NULL;
  
  PangoEngineInfo **shape_engines;
  PangoEngineInfo  **lang_engines;

  g_return_val_if_fail (context != NULL, NULL);
  g_return_val_if_fail (text != NULL, NULL);
  g_return_val_if_fail (length >= 0, NULL);


  if (context->base_dir == PANGO_DIRECTION_RTL)
    base_dir = FRIBIDI_TYPE_RTL;
  else
    base_dir = FRIBIDI_TYPE_LTR;
  
  if (length == 0)
    return NULL;
  
  /* First, apply the bidirectional algorithm to break
   * the text into directional runs.
   */
  text_ucs2 = _pango_utf8_to_ucs2 (text, length);
  if (!text_ucs2)
    return NULL;

  n_chars = unicode_strlen (text, length);
  embedding_levels = g_new (guint8, n_chars);

  /* Storing these as ranges would be a lot more efficient,
   * but also more complicated... we take the simple
   * approach for now.
   */
  shape_engines = g_new0 (PangoEngineInfo *, n_chars);
  lang_engines = g_new0 (PangoEngineInfo *, n_chars);

  fribidi_log2vis_get_embedding_levels (text_ucs2, n_chars, &base_dir,
					embedding_levels);

  /* Now, make shaping-engine affilitations for characters in
   * each run that have high-affinity. This means that there
   * is a shaping engine specific to the current
   * language/character pair.
   */

  add_engines (context, text, length, lang_info, n_langs,
  	       TRUE, shape_engines, lang_engines);

  /* Fill in low-affinity shaping-engine affiliations for
   * remainder of characters.
   */

  add_engines (context, text, length, lang_info, n_langs,
  	       FALSE, shape_engines, lang_engines);

  /* Make a GList of PangoItems out of the above results
   */

  item = NULL;
  p = text;
  for (i=0; i<n_chars; i++)
    {
      next = unicode_next_utf8 (p);
      
      if (i == 0 ||
	  embedding_levels[i] != embedding_levels[i-1] ||
	  shape_engines[i] != shape_engines[i-1] ||
	  lang_engines[i] != lang_engines[i-1])
	{
	  if (item)
	    result = g_list_prepend (result, item);
	  item = g_new (PangoItem, 1);
	  item->offset = p - text;
	  item->num_chars = 0;
	  item->analysis.level = embedding_levels[i];
	  
	  if (shape_engines[i])
	    item->analysis.shape_engine = (PangoEngineShape *)_pango_load_engine (shape_engines[i]->id);
	  else
	    item->analysis.shape_engine = NULL;
	  
	  if (lang_engines[i])
	    item->analysis.lang_engine = (PangoEngineLang *)_pango_load_engine (lang_engines[i]->id);
	  else
	    item->analysis.lang_engine = NULL;
	}

      item->length = (next - text) - item->offset;
      item->num_chars++;
      p = next;
    }  

  if (item)
    result = g_list_prepend (result, item);

  g_free (text_ucs2);
  
  return g_list_reverse (result);
}

static void
add_engines (PangoContext     *context,
	     gchar              *text, 
	     gint                length,
	     PangoLangRange   *lang_info,
	     gint                n_langs,
	     gboolean            force_exact,
	     PangoEngineInfo **shape_engines,
	     PangoEngineInfo **lang_engines)
{
  char *pos;
  char *last_lang = NULL;
  gint n_chars;
  PangoMap *shape_map = NULL;
  PangoMap *lang_map = NULL;
  GUChar4 wc;
  int i, j;

  n_chars = unicode_strlen (text, length);

  pos = text;
  last_lang = NULL;
  for (i=0, j=0; i<n_chars; i++)
    {
      char *lang;
      PangoSubmap *submap;
      PangoMapEntry *entry;

      while (j < n_langs && lang_info[j].start < pos - text)
	j++;
      
      if (j < n_langs && (pos - text) < lang_info[j].start + lang_info[j].length)
	lang = lang_info[j].lang;
      else
	lang = context->lang;

      if (last_lang != lang ||
	  last_lang == 0 || lang == 0 ||
	  strcmp (lang, last_lang) != 0)
	{
	  lang_map = _pango_find_map (lang, PANGO_ENGINE_TYPE_LANG,
					 PANGO_RENDER_TYPE_NONE);
	  shape_map = _pango_find_map (lang, PANGO_ENGINE_TYPE_SHAPE,
				       "PangoRenderX");
	  last_lang = lang;
	}

      pos  = unicode_get_utf8 (pos, &wc);

      if (!lang_engines[i])
	{
	  submap = &lang_map->submaps[wc / 256];
	  entry = submap->is_leaf ? &submap->d.entry : &submap->d.leaves[wc % 256];
	  
	  if (entry->info && (!force_exact || entry->is_exact))
	    lang_engines[i] = entry->info;
	}

      if (!shape_engines[i])
	{
	  submap = &shape_map->submaps[wc / 256];
	  entry = submap->is_leaf ? &submap->d.entry : &submap->d.leaves[wc % 256];

	  if (entry->info && (!force_exact || entry->is_exact))
	    shape_engines[i] = entry->info;
	}
    }
}

