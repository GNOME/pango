/* Pango
 * itemize.c:
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

#include "pango-context.h"
#include "modules.h"

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
	       gchar          *text, 
	       gint            length,
	       PangoLangRange *lang_info,
	       gint            n_langs)
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
  g_return_val_if_fail (length == 0 || text != NULL, NULL);

  if (length == 0)
    return NULL;

  if (context->direction == PANGO_DIRECTION_RTL)
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

  n_chars = g_utf8_strlen (text, length);
  embedding_levels = g_new (guint8, n_chars);

  /* Storing these as ranges would be a lot more efficient,
   * but also more complicated... we take the simple
   * approach for now.
   */
  shape_engines = g_new0 (PangoEngineInfo *, n_chars);
  lang_engines = g_new0 (PangoEngineInfo *, n_chars);

  pango_log2vis_get_embedding_levels (text_ucs2, n_chars, &base_dir,
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
      next = g_utf8_next_char (p);
      
      if (i == 0 ||
	  embedding_levels[i] != embedding_levels[i-1] ||
	  shape_engines[i] != shape_engines[i-1] ||
	  lang_engines[i] != lang_engines[i-1])
	{
	  if (item)
	    result = g_list_prepend (result, item);
	  item = pango_item_new ();
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

  n_chars = g_utf8_strlen (text, length);

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

      if (last_lang != lang &&
	  (last_lang == 0 || lang == 0 ||
	   strcmp (lang, last_lang) != 0))
	{
	  lang_map = pango_find_map (lang, PANGO_ENGINE_TYPE_LANG,
				      PANGO_RENDER_TYPE_NONE);
	  shape_map = pango_find_map (lang, PANGO_ENGINE_TYPE_SHAPE,
				       context->render_type);
	  last_lang = lang;
	}

      wc = g_utf8_get_char (pos);
      pos = g_utf8_next_char (pos);

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

