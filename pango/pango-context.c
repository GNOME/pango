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
#include <stdlib.h>

#include "pango/pango-context.h"
#include "pango/pango-utils.h"

#include "pango-modules.h"

struct _PangoContext
{
  GObject parent_instance;

  PangoLanguage *language;
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
                         gint               start_index,
			 gint               length,
			 PangoAttrList     *attrs,
                         PangoAttrIterator *cached_iter,
                         gint               n_chars,
			 PangoAnalysis     *analyses);

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
                                            &object_info, 0);
    }
  
  return object_type;
}

static void
pango_context_init (PangoContext *context)
{
  PangoFontDescription desc;  

  context->base_dir = PANGO_DIRECTION_LTR;
  context->language = NULL;
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
 * pango_context_set_language:
 * @context: a #PangoContext
 * @language: the new language tag.
 * 
 * Sets the global language tag for the context.
 **/
void
pango_context_set_language (PangoContext *context,
			    PangoLanguage    *language)
{
  g_return_if_fail (context != NULL);

  context->language = language;
}

/**
 * pango_context_get_language:
 * @context: a #PangoContext
 * 
 * Retrieves the global language tag for the context.
 * 
 * Return value: the global language tag.
 **/
PangoLanguage *
pango_context_get_language (PangoContext *context)
{
  g_return_val_if_fail (context != NULL, NULL);

  return context->language;
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
 * @start_index: first byte in @text to process
 * @length:    the number of bytes (not characters) to process
 *             after @start_index.
 *             This must be >= 0.
 * @attrs:     the set of attributes that apply to @text.
 * @cached_iter:      Cached attribute iterator, or NULL
 *
 * Breaks a piece of text into segments with consistent
 * directional level and shaping engine. Each byte of @text will
 * be contained in exactly one of the items in the returned list;
 * the generated list of items will be in logical order (the start
 * offsets of the items are ascending).
 *
 * @cached_iter should be an iterator over @attrs currently positioned at a
 * range before or containing @start_index; @cached_iter will be advanced to
 * the range covering the position just after @start_index + @length.
 * (i.e. if itemizing in a loop, just keep passing in the same @cached_iter).
 *
 * Return value: a GList of PangoItem structures.
 */
GList *
pango_itemize (PangoContext      *context, 
	       const char        *text,
               int                start_index,
	       int                length,
	       PangoAttrList     *attrs,
               PangoAttrIterator *cached_iter)
{
  gunichar *text_ucs4;
  long n_chars, i;
  guint8 *embedding_levels;
  PangoDirection base_dir;
  PangoItem *item;
  const char *p;
  const char *next;
  GList *result = NULL;

  PangoAnalysis *analyses;

  g_return_val_if_fail (context != NULL, NULL);
  g_return_val_if_fail (start_index >= 0, NULL);
  g_return_val_if_fail (length >= 0, NULL);
  g_return_val_if_fail (length == 0 || text != NULL, NULL);

  if (length == 0)
    return NULL;

  base_dir = context->base_dir;

  if (length == 0)
    return NULL;

  /* First, apply the bidirectional algorithm to break
   * the text into directional runs.
   */
  text_ucs4 = g_utf8_to_ucs4_fast (text + start_index, length, &n_chars);

  embedding_levels = g_new (guint8, n_chars);

  pango_log2vis_get_embedding_levels (text_ucs4, n_chars, &base_dir,
                                      embedding_levels);

  /* Storing ranges would be more efficient, but also more
   * complicated... we take the simple approach for now.
   */

  analyses = g_new0 (PangoAnalysis, n_chars);

  /* Now, fill in the appropriate shapers, language engines and fonts for
   * each character.
   */

  add_engines (context, text, start_index, length, attrs,
               cached_iter,
               n_chars,
	       analyses);

  /* Make a GList of PangoItems out of the above results
   */

  item = NULL;
  p = text + start_index;
  for (i=0; i<n_chars; i++)
    {
      PangoAnalysis *analysis = &analyses[i];
      PangoAnalysis *last_analysis = i > 0 ? &analyses[i-1] : 0;
      
      next = g_utf8_next_char (p);
      
      if (i == 0 ||
	  text_ucs4[i] == '\t' || text_ucs4[i-1] == '\t' ||
	  embedding_levels[i] != embedding_levels[i-1] ||
	  analysis->shape_engine != last_analysis->shape_engine ||
	  analysis->lang_engine != last_analysis->lang_engine ||
	  analysis->font != last_analysis->font ||
	  analysis->language != last_analysis->language ||
	  analysis->extra_attrs != last_analysis->extra_attrs)
	{
          /* assert that previous item got at least one char */
          g_assert (item == NULL || item->length > 0);
          g_assert (item == NULL || item->num_chars > 0);
          
	  item = pango_item_new ();
	  item->offset = p - text;
	  item->num_chars = 0;
	  item->analysis.level = embedding_levels[i];
	  
	  item->analysis.shape_engine = analysis->shape_engine;
	  item->analysis.lang_engine = analysis->lang_engine;

	  item->analysis.font = analysis->font;
	  item->analysis.language = analysis->language;

	  /* Copy the extra attribute list if necessary */
	  if (analysis->extra_attrs && i != 0 && analysis->extra_attrs == last_analysis->extra_attrs)
	    {
	      GSList *tmp_list = analysis->extra_attrs;
	      GSList *new_list = NULL;
	      while (tmp_list)
		{
		  new_list = g_slist_prepend (new_list,
					      pango_attribute_copy (tmp_list->data));
		  tmp_list = tmp_list->next;
		}
	      item->analysis.extra_attrs = g_slist_reverse (new_list);
	    }
	  else
	    item->analysis.extra_attrs = analysis->extra_attrs;

	  result = g_list_prepend (result, item);
	}
      else
	g_object_unref (analysis->font);

      item->length = (next - text) - item->offset;
      item->num_chars++;
      p = next;
    }  

  g_free (analyses);
  g_free (embedding_levels);
  g_free (text_ucs4);
  
  return g_list_reverse (result);
}

static void 
fallback_engine_shape (PangoFont        *font,
                       const char       *text,
                       gint              length,
                       PangoAnalysis    *analysis,
                       PangoGlyphString *glyphs)
{
  int n_chars;
  int i;
  const char *p;
  
  g_return_if_fail (font != NULL);
  g_return_if_fail (text != NULL);
  g_return_if_fail (length >= 0);
  g_return_if_fail (analysis != NULL);

  n_chars = g_utf8_strlen (text, length);
  pango_glyph_string_set_size (glyphs, n_chars);
  
  p = text;
  i = 0;
  while (i < n_chars)
    {
      glyphs->glyphs[i].glyph = 0;
      
      glyphs->glyphs[i].geometry.x_offset = 0;
      glyphs->glyphs[i].geometry.y_offset = 0;
      glyphs->glyphs[i].geometry.width = 0;

      glyphs->log_clusters[i] = p - text;
      
      ++i;
      p = g_utf8_next_char (p);
    }
}

static PangoCoverage*
fallback_engine_get_coverage (PangoFont      *font,
                              PangoLanguage  *lang)
{
  PangoCoverage *result = pango_coverage_new ();

  /* We return an empty coverage (this function won't get
   * called, but if it is, empty coverage will keep
   * it from being used).
   */
  
  return result;
}

static PangoEngineShape fallback_shaper = {
  {
    "FallbackScriptEngine",
    PANGO_ENGINE_TYPE_SHAPE,
    sizeof (PangoEngineShape)
  },
  fallback_engine_shape,
  fallback_engine_get_coverage
};

/* FIXME: Remove this artificial limit */
#define MAX_FAMILIES 16

typedef struct _FontSet FontSet;

struct _FontSet
{
  int n_families;
  PangoFont *fonts[MAX_FAMILIES];
  PangoCoverage *coverages[MAX_FAMILIES];  
};

#define FONT_SET_INITIALIZER { 0, }

static gint
font_set_get_font (FontSet  *font_set,
		   gunichar  wc)
{
  PangoCoverageLevel best_level = PANGO_COVERAGE_NONE;

  int result = -1;
  int i;
  
  for (i=0; i < font_set->n_families; i++)
    {
      if (font_set->fonts[i])
	{
	  PangoCoverageLevel level = pango_coverage_get (font_set->coverages[i], wc);
	  
	  if (result == -1 || level > best_level)
	    {
	      result = i;
	      best_level = level;
	    }
	}
    }

  return result;
}

static void
font_set_free (FontSet *font_set)
{
  int j;
  
  for (j=0; j < font_set->n_families; j++)
    {
      if (font_set->fonts[j])
	{
	  g_object_unref (font_set->fonts[j]);
	  pango_coverage_unref (font_set->coverages[j]);
	}
    }

  font_set->n_families = 0;
}

static void
font_set_load (FontSet              *font_set,
	       PangoContext         *context,
	       PangoLanguage        *language,
	       PangoFontDescription *desc)
{
  char **families;
  char *orig_family;
  int j;

  font_set_free (font_set);

  orig_family = desc->family_name;
  families = g_strsplit (orig_family, ",", -1);

  font_set->n_families = 0;
  for (j=0; families[j] && font_set->n_families < MAX_FAMILIES; j++)
    {
      desc->family_name = families[j];
      font_set->fonts[font_set->n_families] = pango_context_load_font (context, desc);
      
      if (font_set->fonts[font_set->n_families])
	{
	  font_set->coverages[font_set->n_families] = pango_font_get_coverage (font_set->fonts[font_set->n_families], language);
	  (font_set->n_families)++;
	}
    }
  
  g_strfreev (families);

  /* The font description was completely unloadable, try with
   * family == "Sans"
   */
  if (font_set->n_families == 0)
    {
      char *ctmp1, *ctmp2;
      
      desc->family_name = orig_family;
      
      ctmp1 = pango_font_description_to_string (desc);
      desc->family_name = "Sans";
      ctmp2 = pango_font_description_to_string (desc);
      
      g_warning ("Couldn't load font \"%s\" falling back to \"%s\"", ctmp1, ctmp2);
      g_free (ctmp1);
      g_free (ctmp2);
      
      desc->family_name = "Sans";
      
      font_set->fonts[0] = pango_context_load_font (context, desc);
      if (font_set->fonts[0])
	{
	  font_set->coverages[0] = pango_font_get_coverage (font_set->fonts[0], language);
	  font_set->n_families = 1;
	}
    }
  
  /* We couldn't try with Sans and the specified style. Try Sans Normal
   */
  if (font_set->n_families == 0)
    {
      char *ctmp1, *ctmp2;
      
      ctmp1 = pango_font_description_to_string (desc);
      desc->style = PANGO_STYLE_NORMAL;
      desc->weight = PANGO_WEIGHT_NORMAL;
      desc->variant = PANGO_VARIANT_NORMAL;
      desc->stretch = PANGO_STRETCH_NORMAL;
      ctmp2 = pango_font_description_to_string (desc);
      
      g_warning ("Couldn't load font \"%s\" falling back to \"%s\"", ctmp1, ctmp2);
      g_free (ctmp1);
      g_free (ctmp2);
      
      font_set->fonts[0] = pango_context_load_font (context, desc);
      if (font_set->fonts[0])
	{
	  font_set->coverages[0] = pango_font_get_coverage (font_set->fonts[0], language);
	  font_set->n_families = 1;
	}
    }

  /* Everything failed, we are screwed, there is no way to continue
   */
  if (font_set->n_families == 0)
    {
      g_warning ("All font failbacks failed!!!!");
      exit (1);
    }
}

static gboolean
advance_iterator_to (PangoAttrIterator *iterator,
                     int                start_index)
{
  int start_range, end_range;
  
  pango_attr_iterator_range (iterator, &start_range, &end_range);

  while (start_index >= end_range)
    {
      if (!pango_attr_iterator_next (iterator))
        return FALSE;
      pango_attr_iterator_range (iterator, &start_range, &end_range);
    }

  if (start_range > start_index)
    g_warning ("In pango_itemize(), the cached iterator passed in "
               "had already moved beyond the start_index");

  return TRUE;
}

static void
add_engines (PangoContext      *context,
	     const gchar       *text,
             gint               start_index,
	     gint               length,
	     PangoAttrList     *attrs,
             PangoAttrIterator *cached_iter,
             gint               n_chars,
	     PangoAnalysis     *analyses)
{
  const char *pos;
  PangoLanguage *language = NULL;
  int next_index;
  GSList *extra_attrs = NULL;
  PangoMap *lang_map = NULL;
  PangoFontDescription current_desc = { 0 };
  FontSet current_fonts = FONT_SET_INITIALIZER;
  PangoAttrIterator *iterator;
  gboolean first_iteration = TRUE;
  gunichar wc;
  int i = 0;
  int font_index;

  if (cached_iter)
    iterator = cached_iter;
  else
    iterator = pango_attr_list_get_iterator (attrs);

  advance_iterator_to (iterator, start_index);
  
  pango_attr_iterator_range (iterator, NULL, &next_index);
  
  pos = text + start_index;
  for (i=0; i<n_chars; i++)
    {
      PangoAnalysis *analysis = &analyses[i];
      
      if (first_iteration || pos - text == next_index)
	{
	  PangoLanguage *next_language;
	  PangoFontDescription next_desc;

          first_iteration = FALSE;
          
          /* Only advance the iterator if we've exhausted a range,
           * not on the first iteration.
           */
          if (pos - text == next_index)
            {
              pango_attr_iterator_next (iterator);
              pango_attr_iterator_range (iterator, NULL, &next_index);
            }
          
	  pango_attr_iterator_get_font (iterator, context->font_desc,
                                        &next_desc, &next_language, &extra_attrs);

          if (!next_language)
	    next_language = context->language;

	  if (i == 0 || language != next_language)
	    {
	      static guint engine_type_id = 0;
	      static guint render_type_id = 0;
	      
	      language = next_language;

	      if (engine_type_id == 0)
		{
		  engine_type_id = g_quark_from_static_string (PANGO_ENGINE_TYPE_LANG);
		  render_type_id = g_quark_from_static_string (PANGO_RENDER_TYPE_NONE);
		}
	       
	      lang_map = pango_find_map (next_language,
					 engine_type_id, render_type_id);
	    }

	  if (i == 0 ||
	      !pango_font_description_equal (&current_desc, &next_desc))
	    {
	      current_desc = next_desc;

	      font_set_load (&current_fonts, context, language, &current_desc);
	    }
        }

      wc = g_utf8_get_char (pos);
      pos = g_utf8_next_char (pos);
      
      analysis->lang_engine = (PangoEngineLang *)pango_map_get_engine (lang_map, wc);
      font_index = font_set_get_font (&current_fonts, wc);
      if (font_index != -1)
	{
	  analysis->font = current_fonts.fonts[font_index];
	  g_object_ref (analysis->font);
	}
      else
	analysis->font = NULL;
      analysis->language = language;
      
      /* FIXME: handle reference counting properly on the shapers */
      if (analysis->font)
	analysis->shape_engine = pango_font_find_shaper (analysis->font, language, wc);
      else
	analysis->shape_engine = NULL;
      
      if (analysis->shape_engine == NULL)
        analysis->shape_engine = &fallback_shaper;
      
      analysis->extra_attrs = extra_attrs;
    }

  g_assert (pos - text == start_index + length);

  font_set_free (&current_fonts);

  if (iterator != cached_iter)
    pango_attr_iterator_destroy (iterator);
}

/**
 * pango_context_get_metrics:
 * @context: a #PangoContext
 * @desc: a #PangoFontDescription structure
 * @language: language tag used to determine which script to get the metrics
 *            for, or %NULL to indicate to get the metrics for the entire
 *            font.
 * @metrics: Structure to fill in with the metrics of the font
 * 
 * Get overall metric information for a font particular font
 * description.  Since the metrics may be substantially different for
 * different scripts, a language tag can be provided to indicate that
 * the metrics should be retrieved that correspond to the script(s)
 * used by that language.
 *
 * The #PangoFontDescription is interpreted in the same way as
 * by pango_itemize(), and the family name may be a comma separated
 * list of figures. If characters from multiple of these families
 * would be used to render the string, then the returned fonts would
 * be a composite of the metrics for the fonts loaded for the
 * individual families.
 **/
void
pango_context_get_metrics (PangoContext                 *context,
			   const PangoFontDescription   *desc,
			   PangoLanguage                *language,
			   PangoFontMetrics             *metrics)
{
  FontSet current_fonts = FONT_SET_INITIALIZER;
  PangoFontMetrics raw_metrics[MAX_FAMILIES];
  gboolean have_metrics[MAX_FAMILIES];
  PangoFontDescription tmp_desc = *desc;
  const char *sample_str;
  const char *p;
  int i;

  g_return_if_fail (PANGO_IS_CONTEXT (context));
  g_return_if_fail (desc != NULL);
  g_return_if_fail (metrics != NULL);

  sample_str = pango_language_get_sample_string (language);

  font_set_load (&current_fonts, context, language, &tmp_desc);

  for (i=0; i < MAX_FAMILIES; i++)
    have_metrics[i] = FALSE;
  
  if (current_fonts.n_families == 1)
    pango_font_get_metrics (current_fonts.fonts[0], language, metrics);
  else
    {
      int count = 0;

      p = sample_str;
      while (*p)
	{
	  gunichar wc = g_utf8_get_char (p);
	  int index = font_set_get_font (&current_fonts, wc);
	  if (!have_metrics[index])
	    {
	      pango_font_get_metrics (current_fonts.fonts[index], language, &raw_metrics[index]);
	      have_metrics[index] = TRUE;
	    }

	  if (count == 0)
	    *metrics = raw_metrics[index];
	  else
	    {
	      metrics->ascent = MAX (metrics->ascent, raw_metrics[index].ascent);
	      metrics->descent = MAX (metrics->descent, raw_metrics[index].descent);
	      metrics->approximate_char_width += raw_metrics[index].approximate_char_width;
	      metrics->approximate_digit_width += raw_metrics[index].approximate_digit_width;
	    }
	  
	  p = g_utf8_next_char (p);
	  count++;
	}

      metrics->approximate_char_width /= count;
      metrics->approximate_digit_width /= count;
    }
      
  font_set_free (&current_fonts);
}
