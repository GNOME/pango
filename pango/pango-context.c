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

#include <string.h>

#include <fribidi/fribidi.h>
#include <pango/pango-context.h>
#include "iconv.h"

#include "pango-modules.h"

struct _PangoContext
{
  GObject parent_instance;

  char *lang;
  PangoDirection base_dir;
  PangoFontDescription *font_desc;

  GSList *font_maps;
};

struct _PangoContextClass
{
  GObjectClass parent_class;
  
};

static void add_engines (PangoContext      *context,
			 const gchar       *text, 
			 gint               length,
			 PangoAttrList     *attrs,
			 PangoEngineShape **shape_engines,
			 PangoEngineLang  **lang_engines,
			 PangoFont        **fonts,
			 GSList           **extra_attr_lists);

static void pango_context_init        (PangoContext      *context);
static void pango_context_class_init  (PangoContextClass *klass);
static void pango_context_finalize    (GObject           *object);

static gpointer parent_class;

GType
pango_context_get_type (void)
{
  static GType object_type = 0;

  if (!object_type)
    {
      static const GTypeInfo object_info =
      {
        sizeof (PangoContextClass),
        (GBaseInitFunc) NULL,
        (GBaseFinalizeFunc) NULL,
        (GClassInitFunc) pango_context_class_init,
        NULL,           /* class_finalize */
        NULL,           /* class_data */
        sizeof (PangoContext),
        0,              /* n_preallocs */
        (GInstanceInitFunc) pango_context_init,
      };
      
      object_type = g_type_register_static (G_TYPE_OBJECT,
                                            "PangoContext",
                                            &object_info);
    }
  
  return object_type;
}

static void
pango_context_init (PangoContext *context)
{
  PangoFontDescription desc;  

  context->base_dir = PANGO_DIRECTION_LTR;
  context->lang = NULL;
  context->font_maps = NULL;

  desc.family_name = "serif";
  desc.style = PANGO_STYLE_NORMAL;
  desc.variant = PANGO_VARIANT_NORMAL;
  desc.weight = PANGO_WEIGHT_NORMAL;
  desc.stretch = PANGO_STRETCH_NORMAL;
  desc.size = 12 * PANGO_SCALE;

  context->font_desc = pango_font_description_copy (&desc);
}

static void
pango_context_class_init (PangoContextClass *klass)
{
  GObjectClass *object_class = G_OBJECT_CLASS (klass);
  
  parent_class = g_type_class_peek_parent (klass);
  
  object_class->finalize = pango_context_finalize;
}

static void
pango_context_finalize (GObject *object)
{
  PangoContext *context;

  context = PANGO_CONTEXT (object);

  if (context->lang)
    g_free (context->lang);
  
  g_slist_foreach (context->font_maps, (GFunc)g_object_unref, NULL);
  g_slist_free (context->font_maps);

  pango_font_description_free (context->font_desc);
  
  G_OBJECT_CLASS (parent_class)->finalize (object);
}


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
  PangoContext *context;

  g_type_init ();
  
  context = PANGO_CONTEXT (g_type_create_instance (pango_context_get_type ()));
  
  return context;
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
  
  g_object_ref (G_OBJECT (font_map));
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
 * 
 * Loads the font in one of the fontmaps in the context
 * that is the closest match for @desc.
 *
 * Returns the font loaded, or %NULL if no font matched.
 **/
PangoFont *
pango_context_load_font (PangoContext               *context,
			 const PangoFontDescription *desc)
{
  GSList *tmp_list;

  g_return_val_if_fail (context != NULL, NULL);

  tmp_list = context->font_maps;
  while (tmp_list)
    {
      PangoFont *font;
      
      font = pango_font_map_load_font (tmp_list->data, desc);
      if (font)
	return font;
      
      tmp_list = tmp_list->next;
    }

  return NULL;
}

/**
 * pango_context_set_font_description:
 * @context: a #PangoContext
 * @desc: the new pango font description
 * 
 * Set the default font description for the context
 **/
void
pango_context_set_font_description (PangoContext               *context,
				    const PangoFontDescription *desc)
{
  g_return_if_fail (context != NULL);
  g_return_if_fail (desc != NULL);

  pango_font_description_free (context->font_desc);
  context->font_desc = pango_font_description_copy (desc);
}

/**
 * pango_context_get_font_description:
 * @context: a #PangoContext
 * 
 * Retrieve the default font description for the context.
 * 
 * Return value: a pointer to the context's default font description.
 *               This value must not be modified or freed.
 **/
PangoFontDescription *
pango_context_get_font_description (PangoContext *context)
{
  g_return_val_if_fail (context != NULL, NULL);
  
  return context->font_desc;
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

/**
 * pango_itemize:
 * @context:   a structure holding information that affects
               the itemization process.
 * @text:      the text to itemize.
 * @length:    the number of bytes (not characters) in text.
 *             This must be >= 0.
 * @attrs:     the set of attributes that apply to @text.
 *
 * Breaks a piece of text into segments with consistent
 * directional level and shaping engine.
 *
 * Returns a GList of PangoItem structures.
 */
GList *
pango_itemize (PangoContext   *context, 
	       const char     *text, 
	       int             length,
	       PangoAttrList  *attrs)
{
  gunichar *text_ucs4;
  int n_chars, i;
  guint8 *embedding_levels;
  FriBidiCharType base_dir;
  PangoItem *item;
  const char *p;
  const char *next;
  GList *result = NULL;
  
  PangoEngineShape **shape_engines;
  PangoEngineLang  **lang_engines;
  GSList           **extra_attr_lists;
  PangoFont        **fonts;

  g_return_val_if_fail (context != NULL, NULL);
  g_return_val_if_fail (length >= 0, NULL);
  g_return_val_if_fail (length == 0 || text != NULL, NULL);

  if (length == 0)
    return NULL;

  if (context->base_dir == PANGO_DIRECTION_RTL)
    base_dir = FRIBIDI_TYPE_RTL;
  else
    base_dir = FRIBIDI_TYPE_LTR;
  
  if (length == 0)
    return NULL;

  /* First, apply the bidirectional algorithm to break
   * the text into directional runs.
   */
  text_ucs4 = g_utf8_to_ucs4 (text, length);
  if (!text_ucs4)
    return NULL;

  n_chars = g_utf8_strlen (text, length);
  embedding_levels = g_new (guint8, n_chars);

  fribidi_log2vis_get_embedding_levels (text_ucs4, n_chars, &base_dir,
					embedding_levels);

  /* Storing these as ranges would be a lot more efficient,
   * but also more complicated... we take the simple
   * approach for now.
   */
  shape_engines = g_new0 (PangoEngineShape *, n_chars);
  lang_engines = g_new0 (PangoEngineLang *, n_chars);
  fonts = g_new0 (PangoFont *, n_chars);
  extra_attr_lists = g_new0 (GSList *, n_chars);

  /* Now, fill in the appropriate shapers, language engines and fonts for
   * each character.
   */

  add_engines (context, text, length, attrs, shape_engines, lang_engines, fonts,
	       extra_attr_lists);


  /* Make a GList of PangoItems out of the above results
   */

  item = NULL;
  p = text;
  for (i=0; i<n_chars; i++)
    {
      next = g_utf8_next_char (p);
      
      if (i == 0 ||
	  text_ucs4[i] == '\t' || text_ucs4[i-1] == '\t' ||
	  embedding_levels[i] != embedding_levels[i-1] ||
	  shape_engines[i] != shape_engines[i-1] ||
	  lang_engines[i] != lang_engines[i-1] ||
	  fonts[i] != fonts[i-1] ||
	  extra_attr_lists[i] != extra_attr_lists[i-1])
	{
	  item = g_new (PangoItem, 1);
	  item->offset = p - text;
	  item->num_chars = 0;
	  item->analysis.level = embedding_levels[i];
	  
	  item->analysis.shape_engine = shape_engines[i];
	  item->analysis.lang_engine = lang_engines[i];

	  item->analysis.font = fonts[i];

	  /* Copy the extra attribute list if necessary */
	  if (extra_attr_lists[i] && i != 0 && extra_attr_lists[i] == extra_attr_lists[i-1])
	    {
	      GSList *tmp_list = extra_attr_lists[i];
	      GSList *new_list = NULL;
	      while (tmp_list)
		{
		  new_list = g_slist_prepend (new_list,
					      pango_attribute_copy (tmp_list->data));
		  tmp_list = tmp_list->next;
		}
	      item->extra_attrs = g_slist_reverse (new_list);
	    }
	  else
	    item->extra_attrs = extra_attr_lists[i];

	  result = g_list_prepend (result, item);
	}
      else
	g_object_unref (G_OBJECT (fonts[i]));

      item->length = (next - text) - item->offset;
      item->num_chars++;
      p = next;
    }  

  g_free (embedding_levels);
  g_free (shape_engines);
  g_free (lang_engines);
  g_free (fonts);
  g_free (extra_attr_lists);
  
  g_free (text_ucs4);
  
  return g_list_reverse (result);
}

static PangoFont *
get_font (PangoFont     **fonts,
	  PangoCoverage **coverages,
	  int             n_families,
	  gunichar        wc)
{
  PangoFont *result = NULL;
  PangoCoverageLevel best_level = PANGO_COVERAGE_NONE;

  int i;
  
  for (i=0; i<n_families; i++)
    {
      if (fonts[i])
	{
	  PangoCoverageLevel level = pango_coverage_get (coverages[i], wc);
	  
	  if (!result || level > best_level)
	    {
	      result = fonts[i];
	      best_level = level;
	    }
	}
    }

  if (result)
    g_object_ref (G_OBJECT (result));
      
  return result;
}
	  
static void
add_engines (PangoContext      *context,
	     const gchar       *text, 
	     gint               length,
	     PangoAttrList     *attrs,
	     PangoEngineShape **shape_engines,
	     PangoEngineLang  **lang_engines,
	     PangoFont        **fonts,
	     GSList           **extra_attr_lists)
{
  const char *pos;
  char *lang = NULL;
  int next_index = 0;
  GSList *extra_attrs = NULL;
  gint n_chars;
  PangoMap *lang_map = NULL;
  PangoFontDescription current_desc = { 0 };

  /* FIXME: Remove this artificial limit */

  int n_families = 0;
#define MAX_FAMILIES 16
  PangoFont *current_fonts[MAX_FAMILIES];
  PangoCoverage *current_coverages[MAX_FAMILIES];
  
  PangoAttrIterator *iterator;
  PangoAttribute *attr;
  
  gunichar wc;
  int i, j;

  n_chars = g_utf8_strlen (text, length);

  iterator = pango_attr_list_get_iterator (attrs);

  pos = text;
  for (i=0; i<n_chars; i++)
    {
      if (pos - text == next_index)
	{
	  char *next_lang;
	  PangoFontDescription next_desc;
	  
	  attr = pango_attr_iterator_get (iterator, PANGO_ATTR_LANG);
	  if (attr)
	    next_lang = ((PangoAttrString *)attr)->value;
	  else
	    next_lang = context->lang;

	  if (i == 0 ||
	      (lang != next_lang &&
	       (lang == NULL || next_lang == NULL || strcmp (lang, next_lang) != 0)))
	    {
	      static guint engine_type_id = 0;
	      static guint render_type_id = 0;
	      
	      lang = next_lang;

	      if (engine_type_id == 0)
		{
		  engine_type_id = g_quark_from_static_string (PANGO_ENGINE_TYPE_LANG);
		  render_type_id = g_quark_from_static_string (PANGO_RENDER_TYPE_NONE);
		}
	       
	      lang_map = pango_find_map (next_lang,
					 engine_type_id, render_type_id);
	    }

	  pango_attr_iterator_get_font (iterator, context->font_desc, &next_desc, &extra_attrs);

	  if (i == 0 ||
	      !pango_font_description_compare (&current_desc, &next_desc))
	    {
	      char **families;
	      
	      current_desc = next_desc;

	      for (j=0; j<n_families; j++)
		{
		  if (current_fonts[j])
		    {
		      g_object_unref (G_OBJECT (current_fonts[j]));
		      pango_coverage_unref (current_coverages[j]);
		    }
		}

	      families = g_strsplit (current_desc.family_name, ",", -1);
	      n_families = 0;
	      while (families[n_families])
		n_families++;

	      if (n_families > MAX_FAMILIES)
		n_families = MAX_FAMILIES;

	      for (j=0; j<n_families; j++)
		{
		  next_desc.family_name = families[j];
		  current_fonts[j] = pango_context_load_font (context, &next_desc);

		  if (current_fonts[j])
		    current_coverages[j] = pango_font_get_coverage (current_fonts[j], lang);
		  else
		    {
		      char *ctmp;

		      ctmp = pango_font_description_to_string (&next_desc);
		      g_warning ("Couldn't load font \"%s\"", ctmp);
		      g_free (ctmp);
		    }
		}

	      g_strfreev (families);
	    }

	  pango_attr_iterator_range (iterator, NULL, &next_index);
	  pango_attr_iterator_next (iterator);
	}

      wc = g_utf8_get_char (pos);
      pos = g_utf8_next_char (pos);
	  
      lang_engines[i] = (PangoEngineLang *)pango_map_get_engine (lang_map, wc);
      fonts[i] = get_font (current_fonts, current_coverages, n_families, wc);

      /* FIXME: handle reference counting properly on the shapers */
      if (fonts[i])
	shape_engines[i] = pango_font_find_shaper (fonts[i], lang, wc);
      else
	shape_engines[i] = NULL;

      extra_attr_lists[i] = extra_attrs;
    }
  
  for (j=0; j<n_families; j++)
    {
      if (current_fonts[j])
	{
	  g_object_unref (G_OBJECT (current_fonts[j]));
	  pango_coverage_unref (current_coverages[j]);
	}
    }

  pango_attr_iterator_destroy (iterator);
}

