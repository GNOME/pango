/* Pango
 * pango-context.c: Contexts for itemization and shaping
 *
 * Copyright (C) 2000, 2006 Red Hat Software
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Library General Public
 * License as published by the Free Software Foundation; either
 * version 2 of the License, or (at your option) any later version.
 *
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Library General Public License for more details.
 *
 * You should have received a copy of the GNU Library General Public
 * License along with this library; if not, write to the
 * Free Software Foundation, Inc., 59 Temple Place - Suite 330,
 * Boston, MA 02111-1307, USA.
 */

#include "config.h"
#include <string.h>
#include <stdlib.h>

#include "pango-context.h"
#include "pango-impl-utils.h"

#include "pango-font-private.h"
#include "pango-fontset-private.h"
#include "pango-fontmap-private.h"
#include "pango-script-private.h"
#include "pango-emoji-private.h"

/**
 * PangoContext:
 *
 * A `PangoContext` stores global information used to control the
 * itemization process.
 *
 * The information stored by `PangoContext includes the fontmap used
 * to look up fonts, and default values such as the default language,
 * default gravity, or default font.
 *
 * To obtain a `PangoContext`, use [method@Pango.FontMap.create_context].
 */
struct _PangoContext
{
  GObject parent_instance;
  guint serial;
  guint fontmap_serial;

  PangoLanguage *set_language;
  PangoLanguage *language;
  PangoDirection base_dir;
  PangoGravity base_gravity;
  PangoGravity resolved_gravity;
  PangoGravityHint gravity_hint;

  PangoFontDescription *font_desc;

  PangoMatrix *matrix;

  PangoFontMap *font_map;

  PangoFontMetrics *metrics;

  gboolean round_glyph_positions;
};

struct _PangoContextClass
{
  GObjectClass parent_class;

};

static void pango_context_finalize    (GObject       *object);
static void context_changed           (PangoContext  *context);

G_DEFINE_TYPE (PangoContext, pango_context, G_TYPE_OBJECT)

static void
pango_context_init (PangoContext *context)
{
  context->base_dir = PANGO_DIRECTION_WEAK_LTR;
  context->resolved_gravity = context->base_gravity = PANGO_GRAVITY_SOUTH;
  context->gravity_hint = PANGO_GRAVITY_HINT_NATURAL;

  context->serial = 1;
  context->set_language = NULL;
  context->language = pango_language_get_default ();
  context->font_map = NULL;
  context->round_glyph_positions = TRUE;

  context->font_desc = pango_font_description_new ();
  pango_font_description_set_family_static (context->font_desc, "serif");
  pango_font_description_set_style (context->font_desc, PANGO_STYLE_NORMAL);
  pango_font_description_set_variant (context->font_desc, PANGO_VARIANT_NORMAL);
  pango_font_description_set_weight (context->font_desc, PANGO_WEIGHT_NORMAL);
  pango_font_description_set_stretch (context->font_desc, PANGO_STRETCH_NORMAL);
  pango_font_description_set_size (context->font_desc, 12 * PANGO_SCALE);
}

static void
pango_context_class_init (PangoContextClass *klass)
{
  GObjectClass *object_class = G_OBJECT_CLASS (klass);

  object_class->finalize = pango_context_finalize;
}

static void
pango_context_finalize (GObject *object)
{
  PangoContext *context;

  context = PANGO_CONTEXT (object);

  if (context->font_map)
    g_object_unref (context->font_map);

  pango_font_description_free (context->font_desc);
  if (context->matrix)
    pango_matrix_free (context->matrix);

  if (context->metrics)
    pango_font_metrics_unref (context->metrics);

  G_OBJECT_CLASS (pango_context_parent_class)->finalize (object);
}

/**
 * pango_context_new:
 *
 * Creates a new `PangoContext` initialized to default values.
 *
 * This function is not particularly useful as it should always
 * be followed by a [method@Pango.Context.set_font_map] call, and the
 * function [method@Pango.FontMap.create_context] does these two steps
 * together and hence users are recommended to use that.
 *
 * If you are using Pango as part of a higher-level system,
 * that system may have it's own way of create a `PangoContext`.
 * For instance, the GTK toolkit has, among others,
 * `gtk_widget_get_pango_context()`. Use those instead.
 *
 * Return value: the newly allocated `PangoContext`, which should
 *   be freed with g_object_unref().
 */
PangoContext *
pango_context_new (void)
{
  PangoContext *context;

  context = g_object_new (PANGO_TYPE_CONTEXT, NULL);

  return context;
}

static void
update_resolved_gravity (PangoContext *context)
{
  if (context->base_gravity == PANGO_GRAVITY_AUTO)
    context->resolved_gravity = pango_gravity_get_for_matrix (context->matrix);
  else
    context->resolved_gravity = context->base_gravity;
}

/**
 * pango_context_set_matrix:
 * @context: a `PangoContext`
 * @matrix: (nullable): a `PangoMatrix`, or %NULL to unset any existing
 * matrix. (No matrix set is the same as setting the identity matrix.)
 *
 * Sets the transformation matrix that will be applied when rendering
 * with this context.
 *
 * Note that reported metrics are in the user space coordinates before
 * the application of the matrix, not device-space coordinates after the
 * application of the matrix. So, they don't scale with the matrix, though
 * they may change slightly for different matrices, depending on how the
 * text is fit to the pixel grid.
 *
 * Since: 1.6
 */
void
pango_context_set_matrix (PangoContext      *context,
                          const PangoMatrix *matrix)
{
  g_return_if_fail (PANGO_IS_CONTEXT (context));

  if (context->matrix || matrix)
    context_changed (context);

  if (context->matrix)
    pango_matrix_free (context->matrix);
  if (matrix)
    context->matrix = pango_matrix_copy (matrix);
  else
    context->matrix = NULL;

  update_resolved_gravity (context);
}

/**
 * pango_context_get_matrix:
 * @context: a `PangoContext`
 *
 * Gets the transformation matrix that will be applied when
 * rendering with this context.
 *
 * See [method@Pango.Context.set_matrix].
 *
 * Return value: (nullable): the matrix, or %NULL if no matrix has
 *   been set (which is the same as the identity matrix). The returned
 *   matrix is owned by Pango and must not be modified or freed.
 *
 * Since: 1.6
 */
const PangoMatrix *
pango_context_get_matrix (PangoContext *context)
{
  g_return_val_if_fail (PANGO_IS_CONTEXT (context), NULL);

  return context->matrix;
}

/**
 * pango_context_set_font_map:
 * @context: a `PangoContext`
 * @font_map: the `PangoFontMap` to set.
 *
 * Sets the font map to be searched when fonts are looked-up
 * in this context.
 *
 * This is only for internal use by Pango backends, a `PangoContext`
 * obtained via one of the recommended methods should already have a
 * suitable font map.
 */
void
pango_context_set_font_map (PangoContext *context,
                            PangoFontMap *font_map)
{
  g_return_if_fail (PANGO_IS_CONTEXT (context));
  g_return_if_fail (!font_map || PANGO_IS_FONT_MAP (font_map));

  if (font_map == context->font_map)
    return;

  context_changed (context);

  if (font_map)
    g_object_ref (font_map);

  if (context->font_map)
    g_object_unref (context->font_map);

  context->font_map = font_map;
  context->fontmap_serial = pango_font_map_get_serial (font_map);
}

/**
 * pango_context_get_font_map:
 * @context: a `PangoContext`
 *
 * Gets the `PangoFontMap` used to look up fonts for this context.
 *
 * Return value: (transfer none): the font map for the `PangoContext`.
 *   This value is owned by Pango and should not be unreferenced.
 *
 * Since: 1.6
 */
PangoFontMap *
pango_context_get_font_map (PangoContext *context)
{
  g_return_val_if_fail (PANGO_IS_CONTEXT (context), NULL);

  return context->font_map;
}

/**
 * pango_context_list_families:
 * @context: a `PangoContext`
 * @families: (out) (array length=n_families) (transfer container): location
 *   to store a pointer to an array of `PangoFontFamily`. This array should
 *   be freed with g_free().
 * @n_families: (out): location to store the number of elements in @descs
 *
 * List all families for a context.
 */
void
pango_context_list_families (PangoContext      *context,
                             PangoFontFamily ***families,
                             int               *n_families)
{
  g_return_if_fail (context != NULL);
  g_return_if_fail (families == NULL || n_families != NULL);

  if (n_families == NULL)
    return;

  if (context->font_map == NULL)
    {
      *n_families = 0;
      if (families)
        *families = NULL;

      return;
    }
  else
    pango_font_map_list_families (context->font_map, families, n_families);
}

/**
 * pango_context_load_font:
 * @context: a `PangoContext`
 * @desc: a `PangoFontDescription` describing the font to load
 *
 * Loads the font in one of the fontmaps in the context
 * that is the closest match for @desc.
 *
 * Returns: (transfer full) (nullable): the newly allocated `PangoFont`
 *   that was loaded, or %NULL if no font matched.
 */
PangoFont *
pango_context_load_font (PangoContext               *context,
                         const PangoFontDescription *desc)
{
  g_return_val_if_fail (context != NULL, NULL);
  g_return_val_if_fail (context->font_map != NULL, NULL);

  return pango_font_map_load_font (context->font_map, context, desc);
}

/**
 * pango_context_load_fontset:
 * @context: a `PangoContext`
 * @desc: a `PangoFontDescription` describing the fonts to load
 * @language: a `PangoLanguage` the fonts will be used for
 *
 * Load a set of fonts in the context that can be used to render
 * a font matching @desc.
 *
 * Returns: (transfer full) (nullable): the newly allocated
 *   `PangoFontset` loaded, or %NULL if no font matched.
 */
PangoFontset *
pango_context_load_fontset (PangoContext               *context,
                            const PangoFontDescription *desc,
                            PangoLanguage             *language)
{
  g_return_val_if_fail (context != NULL, NULL);

  return pango_font_map_load_fontset (context->font_map, context, desc, language);
}

/**
 * pango_context_set_font_description:
 * @context: a `PangoContext`
 * @desc: the new pango font description
 *
 * Set the default font description for the context
 */
void
pango_context_set_font_description (PangoContext               *context,
                                    const PangoFontDescription *desc)
{
  g_return_if_fail (context != NULL);
  g_return_if_fail (desc != NULL);

  if (desc != context->font_desc &&
      (!desc || !context->font_desc || !pango_font_description_equal(desc, context->font_desc)))
    {
      context_changed (context);

      pango_font_description_free (context->font_desc);
      context->font_desc = pango_font_description_copy (desc);
    }
}

/**
 * pango_context_get_font_description:
 * @context: a `PangoContext`
 *
 * Retrieve the default font description for the context.
 *
 * Return value: (transfer none): a pointer to the context's default font
 *   description. This value must not be modified or freed.
 */
PangoFontDescription *
pango_context_get_font_description (PangoContext *context)
{
  g_return_val_if_fail (context != NULL, NULL);

  return context->font_desc;
}

/**
 * pango_context_set_language:
 * @context: a `PangoContext`
 * @language: the new language tag.
 *
 * Sets the global language tag for the context.
 *
 * The default language for the locale of the running process
 * can be found using [type_func@Pango.Language.get_default].
 */
void
pango_context_set_language (PangoContext  *context,
                            PangoLanguage *language)
{
  g_return_if_fail (context != NULL);

  if (language != context->language)
    context_changed (context);

  context->set_language = language;
  if (language)
    context->language = language;
  else
    context->language = pango_language_get_default ();
}

/**
 * pango_context_get_language:
 * @context: a `PangoContext`
 *
 * Retrieves the global language tag for the context.
 *
 * Return value: the global language tag.
 */
PangoLanguage *
pango_context_get_language (PangoContext *context)
{
  g_return_val_if_fail (context != NULL, NULL);

  return context->set_language;
}

/**
 * pango_context_set_base_dir:
 * @context: a `PangoContext`
 * @direction: the new base direction
 *
 * Sets the base direction for the context.
 *
 * The base direction is used in applying the Unicode bidirectional
 * algorithm; if the @direction is %PANGO_DIRECTION_LTR or
 * %PANGO_DIRECTION_RTL, then the value will be used as the paragraph
 * direction in the Unicode bidirectional algorithm. A value of
 * %PANGO_DIRECTION_WEAK_LTR or %PANGO_DIRECTION_WEAK_RTL is used only
 * for paragraphs that do not contain any strong characters themselves.
 */
void
pango_context_set_base_dir (PangoContext   *context,
                            PangoDirection  direction)
{
  g_return_if_fail (context != NULL);

  if (direction != context->base_dir)
    context_changed (context);

  context->base_dir = direction;
}

/**
 * pango_context_get_base_dir:
 * @context: a `PangoContext`
 *
 * Retrieves the base direction for the context.
 *
 * See [method@Pango.Context.set_base_dir].
 *
 * Return value: the base direction for the context.
 */
PangoDirection
pango_context_get_base_dir (PangoContext *context)
{
  g_return_val_if_fail (context != NULL, PANGO_DIRECTION_LTR);

  return context->base_dir;
}

/**
 * pango_context_set_base_gravity:
 * @context: a `PangoContext`
 * @gravity: the new base gravity
 *
 * Sets the base gravity for the context.
 *
 * The base gravity is used in laying vertical text out.
 *
 * Since: 1.16
 */
void
pango_context_set_base_gravity (PangoContext *context,
                                PangoGravity  gravity)
{
  g_return_if_fail (context != NULL);

  if (gravity != context->base_gravity)
    context_changed (context);

  context->base_gravity = gravity;

  update_resolved_gravity (context);
}

/**
 * pango_context_get_base_gravity:
 * @context: a `PangoContext`
 *
 * Retrieves the base gravity for the context.
 *
 * See [method@Pango.Context.set_base_gravity].
 *
 * Return value: the base gravity for the context.
 *
 * Since: 1.16
 */
PangoGravity
pango_context_get_base_gravity (PangoContext *context)
{
  g_return_val_if_fail (context != NULL, PANGO_GRAVITY_SOUTH);

  return context->base_gravity;
}

/**
 * pango_context_get_gravity:
 * @context: a `PangoContext`
 *
 * Retrieves the gravity for the context.
 *
 * This is similar to [method@Pango.Context.get_base_gravity],
 * except for when the base gravity is %PANGO_GRAVITY_AUTO for
 * which [type_func@Pango.Gravity.get_for_matrix] is used to return the
 * gravity from the current context matrix.
 *
 * Return value: the resolved gravity for the context.
 *
 * Since: 1.16
 */
PangoGravity
pango_context_get_gravity (PangoContext *context)
{
  g_return_val_if_fail (context != NULL, PANGO_GRAVITY_SOUTH);

  return context->resolved_gravity;
}

/**
 * pango_context_set_gravity_hint:
 * @context: a `PangoContext`
 * @hint: the new gravity hint
 *
 * Sets the gravity hint for the context.
 *
 * The gravity hint is used in laying vertical text out, and
 * is only relevant if gravity of the context as returned by
 * [method@Pango.Context.get_gravity] is set to %PANGO_GRAVITY_EAST
 * or %PANGO_GRAVITY_WEST.
 *
 * Since: 1.16
 */
void
pango_context_set_gravity_hint (PangoContext     *context,
                                PangoGravityHint  hint)
{
  g_return_if_fail (context != NULL);

  if (hint != context->gravity_hint)
    context_changed (context);

  context->gravity_hint = hint;
}

/**
 * pango_context_get_gravity_hint:
 * @context: a `PangoContext`
 *
 * Retrieves the gravity hint for the context.
 *
 * See [method@Pango.Context.set_gravity_hint] for details.
 *
 * Return value: the gravity hint for the context.
 *
 * Since: 1.16
 */
PangoGravityHint
pango_context_get_gravity_hint (PangoContext *context)
{
  g_return_val_if_fail (context != NULL, PANGO_GRAVITY_HINT_NATURAL);

  return context->gravity_hint;
}

/**********************************************************************/

static gboolean
advance_attr_iterator_to (PangoAttrIterator *iterator,
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

/***************************************************************************
 * We cache the results of character,fontset => font in a hash table
 ***************************************************************************/

typedef struct {
  GHashTable *hash;
} FontCache;

typedef struct {
  PangoFont *font;
} FontElement;

static void
font_cache_destroy (FontCache *cache)
{
  g_hash_table_destroy (cache->hash);
  g_slice_free (FontCache, cache);
}

static void
font_element_destroy (FontElement *element)
{
  if (element->font)
    g_object_unref (element->font);
  g_slice_free (FontElement, element);
}

static FontCache *
get_font_cache (PangoFontset *fontset)
{
  FontCache *cache;

  static GQuark cache_quark = 0; /* MT-safe */
  if (G_UNLIKELY (!cache_quark))
    cache_quark = g_quark_from_static_string ("pango-font-cache");

retry:
  cache = g_object_get_qdata (G_OBJECT (fontset), cache_quark);
  if (G_UNLIKELY (!cache))
    {
      cache = g_slice_new (FontCache);
      cache->hash = g_hash_table_new_full (g_direct_hash, NULL,
                                           NULL, (GDestroyNotify)font_element_destroy);
      if (!g_object_replace_qdata (G_OBJECT (fontset), cache_quark, NULL,
                                   cache, (GDestroyNotify)font_cache_destroy,
                                   NULL))
        {
          font_cache_destroy (cache);
          goto retry;
        }
    }

  return cache;
}

static gboolean
font_cache_get (FontCache   *cache,
                gunichar     wc,
                PangoFont  **font)
{
  FontElement *element;

  element = g_hash_table_lookup (cache->hash, GUINT_TO_POINTER (wc));
  if (element)
    {
      *font = element->font;

      return TRUE;
    }
  else
    return FALSE;
}

static void
font_cache_insert (FontCache   *cache,
                   gunichar           wc,
                   PangoFont         *font)
{
  FontElement *element = g_slice_new (FontElement);
  element->font = font ? g_object_ref (font) : NULL;

  g_hash_table_insert (cache->hash, GUINT_TO_POINTER (wc), element);
}

/**********************************************************************/

typedef enum {
  EMBEDDING_CHANGED    = 1 << 0,
  SCRIPT_CHANGED       = 1 << 1,
  LANG_CHANGED         = 1 << 2,
  FONT_CHANGED         = 1 << 3,
  DERIVED_LANG_CHANGED = 1 << 4,
  WIDTH_CHANGED        = 1 << 5,
  EMOJI_CHANGED        = 1 << 6,
} ChangedFlags;



typedef struct _PangoWidthIter PangoWidthIter;

struct _PangoWidthIter
{
        const gchar *text_start;
        const gchar *text_end;
        const gchar *start;
        const gchar *end;
        gboolean     upright;
};

typedef struct _ItemizeState ItemizeState;



struct _ItemizeState
{
  PangoContext *context;
  const char *text;
  const char *end;

  const char *run_start;
  const char *run_end;

  GList *result;
  PangoItem *item;

  guint8 *embedding_levels;
  int embedding_end_offset;
  const char *embedding_end;
  guint8 embedding;

  PangoGravity gravity;
  PangoGravityHint gravity_hint;
  PangoGravity resolved_gravity;
  PangoGravity font_desc_gravity;
  gboolean centered_baseline;

  PangoAttrIterator *attr_iter;
  gboolean free_attr_iter;
  const char *attr_end;
  PangoFontDescription *font_desc;
  PangoFontDescription *emoji_font_desc;
  PangoLanguage *lang;
  GSList *extra_attrs;
  gboolean copy_extra_attrs;

  ChangedFlags changed;

  PangoScriptIter script_iter;
  const char *script_end;
  PangoScript script;

  PangoWidthIter width_iter;
  PangoEmojiIter emoji_iter;

  PangoLanguage *derived_lang;

  PangoFontset *current_fonts;
  FontCache *cache;
  PangoFont *base_font;
  gboolean enable_fallback;
};

static void
update_embedding_end (ItemizeState *state)
{
  state->embedding = state->embedding_levels[state->embedding_end_offset];
  while (state->embedding_end < state->end &&
         state->embedding_levels[state->embedding_end_offset] == state->embedding)
    {
      state->embedding_end_offset++;
      state->embedding_end = g_utf8_next_char (state->embedding_end);
    }

  state->changed |= EMBEDDING_CHANGED;
}

static PangoAttribute *
find_attribute (GSList        *attr_list,
                PangoAttrType  type)
{
  GSList *node;

  for (node = attr_list; node; node = node->next)
    if (((PangoAttribute *) node->data)->klass->type == type)
      return (PangoAttribute *) node->data;

  return NULL;
}

static void
update_attr_iterator (ItemizeState *state)
{
  PangoLanguage *old_lang;
  PangoAttribute *attr;
  int end_index;

  pango_attr_iterator_range (state->attr_iter, NULL, &end_index);
  if (end_index < state->end - state->text)
    state->attr_end = state->text + end_index;
  else
    state->attr_end = state->end;

  if (state->emoji_font_desc)
    {
      pango_font_description_free (state->emoji_font_desc);
      state->emoji_font_desc = NULL;
    }

  old_lang = state->lang;
  if (state->font_desc)
    pango_font_description_free (state->font_desc);
  state->font_desc = pango_font_description_copy_static (state->context->font_desc);
  pango_attr_iterator_get_font (state->attr_iter, state->font_desc,
                                &state->lang, &state->extra_attrs);
  if (pango_font_description_get_set_fields (state->font_desc) & PANGO_FONT_MASK_GRAVITY)
    state->font_desc_gravity = pango_font_description_get_gravity (state->font_desc);
  else
    state->font_desc_gravity = PANGO_GRAVITY_AUTO;

  state->copy_extra_attrs = FALSE;

  if (!state->lang)
    state->lang = state->context->language;

  attr = find_attribute (state->extra_attrs, PANGO_ATTR_FALLBACK);
  state->enable_fallback = (attr == NULL || ((PangoAttrInt *)attr)->value);

  attr = find_attribute (state->extra_attrs, PANGO_ATTR_GRAVITY);
  state->gravity = attr == NULL ? PANGO_GRAVITY_AUTO : ((PangoAttrInt *)attr)->value;

  attr = find_attribute (state->extra_attrs, PANGO_ATTR_GRAVITY_HINT);
  state->gravity_hint = attr == NULL ? state->context->gravity_hint : (PangoGravityHint)((PangoAttrInt *)attr)->value;

  state->changed |= FONT_CHANGED;
  if (state->lang != old_lang)
    state->changed |= LANG_CHANGED;
}

static void
update_end (ItemizeState *state)
{
  state->run_end = state->embedding_end;
  if (state->attr_end < state->run_end)
    state->run_end = state->attr_end;
  if (state->script_end < state->run_end)
    state->run_end = state->script_end;
  if (state->width_iter.end < state->run_end)
    state->run_end = state->width_iter.end;
  if (state->emoji_iter.end < state->run_end)
    state->run_end = state->emoji_iter.end;
}

static gboolean
width_iter_is_upright (gunichar ch)
{
  /* https://www.unicode.org/Public/11.0.0/ucd/VerticalOrientation.txt
   * VO=U or Tu table generated by tools/gen-vertical-orientation-U-table.py.
   *
   * FIXME: In the future, If GLib supports VerticalOrientation, please use it.
   */
  static const gunichar upright[][2] = {
    {0x00A7, 0x00A7}, {0x00A9, 0x00A9}, {0x00AE, 0x00AE}, {0x00B1, 0x00B1},
    {0x00BC, 0x00BE}, {0x00D7, 0x00D7}, {0x00F7, 0x00F7}, {0x02EA, 0x02EB},
    {0x1100, 0x11FF}, {0x1401, 0x167F}, {0x18B0, 0x18FF}, {0x2016, 0x2016},
    {0x2020, 0x2021}, {0x2030, 0x2031}, {0x203B, 0x203C}, {0x2042, 0x2042},
    {0x2047, 0x2049}, {0x2051, 0x2051}, {0x2065, 0x2065}, {0x20DD, 0x20E0},
    {0x20E2, 0x20E4}, {0x2100, 0x2101}, {0x2103, 0x2109}, {0x210F, 0x210F},
    {0x2113, 0x2114}, {0x2116, 0x2117}, {0x211E, 0x2123}, {0x2125, 0x2125},
    {0x2127, 0x2127}, {0x2129, 0x2129}, {0x212E, 0x212E}, {0x2135, 0x213F},
    {0x2145, 0x214A}, {0x214C, 0x214D}, {0x214F, 0x2189}, {0x218C, 0x218F},
    {0x221E, 0x221E}, {0x2234, 0x2235}, {0x2300, 0x2307}, {0x230C, 0x231F},
    {0x2324, 0x2328}, {0x232B, 0x232B}, {0x237D, 0x239A}, {0x23BE, 0x23CD},
    {0x23CF, 0x23CF}, {0x23D1, 0x23DB}, {0x23E2, 0x2422}, {0x2424, 0x24FF},
    {0x25A0, 0x2619}, {0x2620, 0x2767}, {0x2776, 0x2793}, {0x2B12, 0x2B2F},
    {0x2B50, 0x2B59}, {0x2BB8, 0x2BD1}, {0x2BD3, 0x2BEB}, {0x2BF0, 0x2BFF},
    {0x2E80, 0x3007}, {0x3012, 0x3013}, {0x3020, 0x302F}, {0x3031, 0x309F},
    {0x30A1, 0x30FB}, {0x30FD, 0xA4CF}, {0xA960, 0xA97F}, {0xAC00, 0xD7FF},
    {0xE000, 0xFAFF}, {0xFE10, 0xFE1F}, {0xFE30, 0xFE48}, {0xFE50, 0xFE57},
    {0xFE5F, 0xFE62}, {0xFE67, 0xFE6F}, {0xFF01, 0xFF07}, {0xFF0A, 0xFF0C},
    {0xFF0E, 0xFF19}, {0xFF1F, 0xFF3A}, {0xFF3C, 0xFF3C}, {0xFF3E, 0xFF3E},
    {0xFF40, 0xFF5A}, {0xFFE0, 0xFFE2}, {0xFFE4, 0xFFE7}, {0xFFF0, 0xFFF8},
    {0xFFFC, 0xFFFD}, {0x10980, 0x1099F}, {0x11580, 0x115FF}, {0x11A00, 0x11AAF},
    {0x13000, 0x1342F}, {0x14400, 0x1467F}, {0x16FE0, 0x18AFF}, {0x1B000, 0x1B12F},
    {0x1B170, 0x1B2FF}, {0x1D000, 0x1D1FF}, {0x1D2E0, 0x1D37F}, {0x1D800, 0x1DAAF},
    {0x1F000, 0x1F7FF}, {0x1F900, 0x1FA6F}, {0x20000, 0x2FFFD}, {0x30000, 0x3FFFD},
    {0xF0000, 0xFFFFD}, {0x100000, 0x10FFFD}
  };
  static const int max = sizeof(upright) / sizeof(upright[0]);
  int st = 0;
  int ed = max;

  if (ch < upright[0][0])
    return FALSE;

  while (st <= ed)
    {
      int mid = (st + ed) / 2;
      if (upright[mid][0] <= ch && ch <= upright[mid][1])
        return TRUE;
      else
        if (upright[mid][0] <= ch)
          st = mid + 1;
        else
          ed = mid - 1;
    }

  return FALSE;
}

static void
width_iter_next (PangoWidthIter *iter)
{
  gboolean met_joiner = FALSE;
  iter->start = iter->end;

  if (iter->end < iter->text_end)
    {
      gunichar ch = g_utf8_get_char (iter->end);
      iter->upright = width_iter_is_upright (ch);
    }

  while (iter->end < iter->text_end)
    {
      gunichar ch = g_utf8_get_char (iter->end);

      /* for zero width joiner */
      if (ch == 0x200D)
        {
          iter->end = g_utf8_next_char (iter->end);
          met_joiner = TRUE;
          continue;
        }

      /* ignore the upright check if met joiner */
      if (met_joiner)
        {
          iter->end = g_utf8_next_char (iter->end);
          met_joiner = FALSE;
          continue;
        }

      /* for variation selector, tag and emoji modifier. */
      if (G_UNLIKELY (ch == 0xFE0EU || ch == 0xFE0FU ||
                      (ch >= 0xE0020 && ch <= 0xE007F) ||
                      (ch >= 0x1F3FB && ch <= 0x1F3FF)))
        {
          iter->end = g_utf8_next_char (iter->end);
          continue;
        }

      if (width_iter_is_upright (ch) != iter->upright)
        break;

      iter->end = g_utf8_next_char (iter->end);
    }
}

static void
width_iter_init (PangoWidthIter *iter,
                 const char     *text,
                 int             length)
{
  iter->text_start = text;
  iter->text_end = text + length;
  iter->start = iter->end = text;

  width_iter_next (iter);
}

static void
width_iter_fini (PangoWidthIter* iter)
{
}

static void
itemize_state_init (ItemizeState               *state,
                    PangoContext               *context,
                    const char                 *text,
                    PangoDirection              base_dir,
                    int                         start_index,
                    int                         length,
                    PangoAttrList              *attrs,
                    PangoAttrIterator          *cached_iter,
                    const PangoFontDescription *desc)
{
  state->context = context;
  state->text = text;
  state->end = text + start_index + length;

  state->result = NULL;
  state->item = NULL;

  state->run_start = text + start_index;
  state->changed = EMBEDDING_CHANGED | SCRIPT_CHANGED | LANG_CHANGED |
                   FONT_CHANGED | WIDTH_CHANGED | EMOJI_CHANGED;

  /* First, apply the bidirectional algorithm to break
   * the text into directional runs.
   */
  state->embedding_levels = pango_log2vis_get_embedding_levels (text + start_index, length, &base_dir);

  state->embedding_end_offset = 0;
  state->embedding_end = text + start_index;
  update_embedding_end (state);

  /* Initialize the attribute iterator
   */
  if (cached_iter)
    {
      state->attr_iter = cached_iter;
      state->free_attr_iter = FALSE;
    }
  else if (attrs)
    {
      state->attr_iter = pango_attr_list_get_iterator (attrs);
      state->free_attr_iter = TRUE;
    }
  else
    {
      state->attr_iter = NULL;
      state->free_attr_iter = FALSE;
    }

  state->emoji_font_desc = NULL;
  if (state->attr_iter)
    {
      state->font_desc = NULL;
      state->lang = NULL;

      advance_attr_iterator_to (state->attr_iter, start_index);
      update_attr_iterator (state);
    }
  else
    {
      state->font_desc = pango_font_description_copy_static (desc ? desc : state->context->font_desc);
      state->lang = state->context->language;
      state->extra_attrs = NULL;
      state->copy_extra_attrs = FALSE;

      state->attr_end = state->end;
      state->enable_fallback = TRUE;
    }

  /* Initialize the script iterator
   */
  _pango_script_iter_init (&state->script_iter, text + start_index, length);
  pango_script_iter_get_range (&state->script_iter, NULL,
                               &state->script_end, &state->script);

  width_iter_init (&state->width_iter, text + start_index, length);
  _pango_emoji_iter_init (&state->emoji_iter, text + start_index, length);

  if (state->emoji_iter.is_emoji)
    state->width_iter.end = MAX (state->width_iter.end, state->emoji_iter.end);

  update_end (state);

  if (pango_font_description_get_set_fields (state->font_desc) & PANGO_FONT_MASK_GRAVITY)
    state->font_desc_gravity = pango_font_description_get_gravity (state->font_desc);
  else
    state->font_desc_gravity = PANGO_GRAVITY_AUTO;

  state->gravity = PANGO_GRAVITY_AUTO;
  state->centered_baseline = PANGO_GRAVITY_IS_VERTICAL (state->context->resolved_gravity);
  state->gravity_hint = state->context->gravity_hint;
  state->resolved_gravity = PANGO_GRAVITY_AUTO;
  state->derived_lang = NULL;
  state->current_fonts = NULL;
  state->cache = NULL;
  state->base_font = NULL;

}

static gboolean
itemize_state_next (ItemizeState *state)
{
  if (state->run_end == state->end)
    return FALSE;

  state->changed = 0;

  state->run_start = state->run_end;

  if (state->run_end == state->embedding_end)
    {
      update_embedding_end (state);
    }

  if (state->run_end == state->attr_end)
    {
      pango_attr_iterator_next (state->attr_iter);
      update_attr_iterator (state);
    }

  if (state->run_end == state->script_end)
    {
      pango_script_iter_next (&state->script_iter);
      pango_script_iter_get_range (&state->script_iter, NULL,
                                   &state->script_end, &state->script);
      state->changed |= SCRIPT_CHANGED;
    }
  if (state->run_end == state->emoji_iter.end)
    {
      _pango_emoji_iter_next (&state->emoji_iter);
      state->changed |= EMOJI_CHANGED;

      if (state->emoji_iter.is_emoji)
        state->width_iter.end = MAX (state->width_iter.end, state->emoji_iter.end);
    }
  if (state->run_end == state->width_iter.end)
    {
      width_iter_next (&state->width_iter);
      state->changed |= WIDTH_CHANGED;
    }

  update_end (state);

  return TRUE;
}

static GSList *
copy_attr_slist (GSList *attr_slist)
{
  GSList *new_list = NULL;
  GSList *l;

  for (l = attr_slist; l; l = l->next)
    new_list = g_slist_prepend (new_list, pango_attribute_copy (l->data));

  return g_slist_reverse (new_list);
}

static void
itemize_state_fill_font (ItemizeState *state,
                         PangoFont    *font)
{
  GList *l;

  for (l = state->result; l; l = l->next)
    {
      PangoItem *item = l->data;
      if (item->analysis.font)
        break;
      if (font)
        item->analysis.font = g_object_ref (font);
    }
}

static void
itemize_state_add_character (ItemizeState *state,
                             PangoFont    *font,
                             gboolean      force_break,
                             const char   *pos)
{
  if (state->item)
    {
      if (!state->item->analysis.font && font)
        {
          itemize_state_fill_font (state, font);
        }
      else if (state->item->analysis.font && !font)
        {
          font = state->item->analysis.font;
        }

      if (!force_break &&
          state->item->analysis.font == font)
        {
          state->item->num_chars++;
          return;
        }

      state->item->length = (pos - state->text) - state->item->offset;
    }

  state->item = pango_item_new ();
  state->item->offset = pos - state->text;
  state->item->length = 0;
  state->item->num_chars = 1;

  if (font)
    g_object_ref (font);
  state->item->analysis.font = font;

  state->item->analysis.level = state->embedding;
  state->item->analysis.gravity = state->resolved_gravity;

  /* The level vs. gravity dance:
   *    - If gravity is SOUTH, leave level untouched.
   *    - If gravity is NORTH, step level one up, to
   *      not get mirrored upside-down text.
   *    - If gravity is EAST, step up to an even level, as
   *      it's a clockwise-rotated layout, so the rotated
   *      top is unrotated left.
   *    - If gravity is WEST, step up to an odd level, as
   *      it's a counter-clockwise-rotated layout, so the rotated
   *      top is unrotated right.
   *
   * A similar dance is performed in pango-layout.c:
   * line_set_resolved_dir().  Keep in synch.
   */
  switch (state->item->analysis.gravity)
    {
      case PANGO_GRAVITY_SOUTH:
      default:
        break;
      case PANGO_GRAVITY_NORTH:
        state->item->analysis.level++;
        break;
      case PANGO_GRAVITY_EAST:
        state->item->analysis.level += 1;
        state->item->analysis.level &= ~1;
        break;
      case PANGO_GRAVITY_WEST:
        state->item->analysis.level |= 1;
        break;
    }

  state->item->analysis.flags = state->centered_baseline ? PANGO_ANALYSIS_FLAG_CENTERED_BASELINE : 0;

  state->item->analysis.script = state->script;
  state->item->analysis.language = state->derived_lang;

  if (state->copy_extra_attrs)
    {
      state->item->analysis.extra_attrs = copy_attr_slist (state->extra_attrs);
    }
  else
    {
      state->item->analysis.extra_attrs = state->extra_attrs;
      state->copy_extra_attrs = TRUE;
    }

  state->result = g_list_prepend (state->result, state->item);
}

typedef struct {
  PangoLanguage *lang;
  gunichar wc;
  PangoFont *font;
} GetFontInfo;

static gboolean
get_font_foreach (PangoFontset *fontset,
                  PangoFont    *font,
                  gpointer      data)
{
  GetFontInfo *info = data;

  if (G_UNLIKELY (!font))
    return FALSE;

  if (pango_font_has_char (font, info->wc))
    {
      info->font = font;
      return TRUE;
    }

  if (!fontset)
    {
      info->font = font;
      return TRUE;
    }

  return FALSE;
}

static PangoFont *
get_base_font (ItemizeState *state)
{
  if (!state->base_font)
    state->base_font = pango_font_map_load_font (state->context->font_map,
                                                 state->context,
                                                 state->font_desc);
  return state->base_font;
}

static gboolean
get_font (ItemizeState  *state,
          gunichar       wc,
          PangoFont    **font)
{
  GetFontInfo info;

  /* We'd need a separate cache when fallback is disabled, but since lookup
   * with fallback disabled is faster anyways, we just skip caching */
  if (state->enable_fallback && font_cache_get (state->cache, wc, font))
    return TRUE;

  info.lang = state->derived_lang;
  info.wc = wc;
  info.font = NULL;

  if (state->enable_fallback)
    pango_fontset_foreach (state->current_fonts, get_font_foreach, &info);
  else
    get_font_foreach (NULL, get_base_font (state), &info);

  *font = info.font;

  /* skip caching if fallback disabled (see above) */
  if (state->enable_fallback)
    font_cache_insert (state->cache, wc, *font);

  return TRUE;
}

static PangoLanguage *
compute_derived_language (PangoLanguage *lang,
                          PangoScript    script)
{
  PangoLanguage *derived_lang;

  /* Make sure the language tag is consistent with the derived
   * script. There is no point in marking up a section of
   * Arabic text with the "en" language tag.
   */
  if (lang && pango_language_includes_script (lang, script))
    derived_lang = lang;
  else
    {
      derived_lang = pango_script_get_sample_language (script);
      /* If we don't find a sample language for the script, we
       * use a language tag that shouldn't actually be used
       * anywhere. This keeps fontconfig (for the PangoFc*
       * backend) from using the language tag to affect the
       * sort order. I don't have a reference for 'xx' being
       * safe here, though Keith Packard claims it is.
       */
      if (!derived_lang)
        derived_lang = pango_language_from_string ("xx");
    }

  return derived_lang;
}

static void
itemize_state_update_for_new_run (ItemizeState *state)
{
  /* This block should be moved to update_attr_iterator, but I'm too lazy to
   * do it right now */
  if (state->changed & (FONT_CHANGED | SCRIPT_CHANGED | WIDTH_CHANGED))
    {
      /* Font-desc gravity overrides everything */
      if (state->font_desc_gravity != PANGO_GRAVITY_AUTO)
        {
          state->resolved_gravity = state->font_desc_gravity;
        }
      else
        {
          PangoGravity gravity = state->gravity;
          PangoGravityHint gravity_hint = state->gravity_hint;

          if (G_LIKELY (gravity == PANGO_GRAVITY_AUTO))
            gravity = state->context->resolved_gravity;

          state->resolved_gravity = pango_gravity_get_for_script_and_width (state->script,
                                                                            state->width_iter.upright,
                                                                            gravity,
                                                                            gravity_hint);
        }

      if (state->font_desc_gravity != state->resolved_gravity)
        {
          pango_font_description_set_gravity (state->font_desc, state->resolved_gravity);
          state->changed |= FONT_CHANGED;
        }
    }

  if (state->changed & (SCRIPT_CHANGED | LANG_CHANGED))
    {
      PangoLanguage *old_derived_lang = state->derived_lang;
      state->derived_lang = compute_derived_language (state->lang, state->script);
      if (old_derived_lang != state->derived_lang)
        state->changed |= DERIVED_LANG_CHANGED;
    }

  if (state->changed & (EMOJI_CHANGED))
    {
      state->changed |= FONT_CHANGED;
    }

  if (state->changed & (FONT_CHANGED | DERIVED_LANG_CHANGED) &&
      state->current_fonts)
    {
      g_object_unref (state->current_fonts);
      state->current_fonts = NULL;
      state->cache = NULL;
    }

  if (!state->current_fonts)
    {
      gboolean is_emoji = state->emoji_iter.is_emoji;
      if (is_emoji && !state->emoji_font_desc)
        {
          state->emoji_font_desc = pango_font_description_copy_static (state->font_desc);
          pango_font_description_set_family_static (state->emoji_font_desc, "emoji");
        }
      state->current_fonts = pango_font_map_load_fontset (state->context->font_map,
                                                          state->context,
                                                          is_emoji ? state->emoji_font_desc : state->font_desc,
                                                          state->derived_lang);
      state->cache = get_font_cache (state->current_fonts);
    }

  if ((state->changed & FONT_CHANGED) && state->base_font)
    {
      g_object_unref (state->base_font);
      state->base_font = NULL;
    }
}

static void
itemize_state_process_run (ItemizeState *state)
{
  const char *p;
  gboolean last_was_forced_break = FALSE;

  /* Only one character has type G_UNICODE_LINE_SEPARATOR in Unicode 4.0;
   * update this if that changes. */
#define LINE_SEPARATOR 0x2028

  itemize_state_update_for_new_run (state);

  /* We should never get an empty run */
  g_assert (state->run_end != state->run_start);

  for (p = state->run_start;
       p < state->run_end;
       p = g_utf8_next_char (p))
    {
      gunichar wc = g_utf8_get_char (p);
      gboolean is_forced_break = (wc == '\t' || wc == LINE_SEPARATOR);
      PangoFont *font;
      GUnicodeType type;

      /* We don't want space characters to affect font selection; in general,
       * it's always wrong to select a font just to render a space.
       * We assume that all fonts have the ASCII space, and for other space
       * characters if they don't, HarfBuzz will compatibility-decompose them
       * to ASCII space...
       * See bugs #355987 and #701652.
       *
       * We don't want to change fonts just for variation selectors.
       * See bug #781123.
       *
       * Finally, don't change fonts for line or paragraph separators.
       */
      type = g_unichar_type (wc);
      if (G_UNLIKELY (type == G_UNICODE_CONTROL ||
                      type == G_UNICODE_FORMAT ||
                      type == G_UNICODE_SURROGATE ||
                      type == G_UNICODE_LINE_SEPARATOR ||
                      type == G_UNICODE_PARAGRAPH_SEPARATOR ||
                      (type == G_UNICODE_SPACE_SEPARATOR && wc != 0x1680u /* OGHAM SPACE MARK */) ||
                      (wc >= 0xfe00u && wc <= 0xfe0fu) ||
                      (wc >= 0xe0100u && wc <= 0xe01efu)))
        {
          font = NULL;
        }
      else
        {
          get_font (state, wc, &font);
        }

      itemize_state_add_character (state, font,
                                   is_forced_break || last_was_forced_break,
                                   p);

      last_was_forced_break = is_forced_break;
    }

  /* Finish the final item from the current segment */
  state->item->length = (p - state->text) - state->item->offset;
  if (!state->item->analysis.font)
    {
      PangoFont *font;

      if (G_UNLIKELY (!get_font (state, ' ', &font)))
        {
          /* If no font was found, warn once per fontmap/script pair */
          PangoFontMap *fontmap = state->context->font_map;
          char *script_tag = g_strdup_printf ("g-unicode-script-%d", state->script);

          if (!g_object_get_data (G_OBJECT (fontmap), script_tag))
            {
              g_warning ("failed to choose a font, expect ugly output. script='%d'",
                         state->script);

              g_object_set_data_full (G_OBJECT (fontmap), script_tag,
                                      GINT_TO_POINTER (1), NULL);
            }

          g_free (script_tag);

          font = NULL;
        }
      itemize_state_fill_font (state, font);
    }
  state->item = NULL;
}

static void
itemize_state_finish (ItemizeState *state)
{
  g_free (state->embedding_levels);
  if (state->free_attr_iter)
    pango_attr_iterator_destroy (state->attr_iter);
  _pango_script_iter_fini (&state->script_iter);
  pango_font_description_free (state->font_desc);
  pango_font_description_free (state->emoji_font_desc);
  width_iter_fini (&state->width_iter);
  _pango_emoji_iter_fini (&state->emoji_iter);

  if (state->current_fonts)
    g_object_unref (state->current_fonts);
  if (state->base_font)
    g_object_unref (state->base_font);
}

/**
 * pango_itemize_with_base_dir:
 * @context: a structure holding information that affects
 *   the itemization process.
 * @base_dir: base direction to use for bidirectional processing
 * @text: the text to itemize.
 * @start_index: first byte in @text to process
 * @length: the number of bytes (not characters) to process
 *   after @start_index. This must be >= 0.
 * @attrs: the set of attributes that apply to @text.
 * @cached_iter: (nullable): Cached attribute iterator, or %NULL
 *
 * Like `pango_itemize()`, but with an explicitly specified base direction.
 *
 * The base direction is used when computing bidirectional levels.
 * (see [method@Pango.Context.set_base_dir]). [func@itemize] gets the
 * base direction from the `PangoContext`.
 *
 * Return value: (transfer full) (element-type Pango.Item): a `GList` of
 *   [struct@Pango.Item] structures. The items should be freed using
 *   [method@Pango.Item.free] probably in combination with g_list_free_full().
 *
 * Since: 1.4
 */
GList *
pango_itemize_with_base_dir (PangoContext      *context,
                             PangoDirection     base_dir,
                             const char        *text,
                             int                start_index,
                             int                length,
                             PangoAttrList     *attrs,
                             PangoAttrIterator *cached_iter)
{
  ItemizeState state;

  g_return_val_if_fail (context != NULL, NULL);
  g_return_val_if_fail (start_index >= 0, NULL);
  g_return_val_if_fail (length >= 0, NULL);
  g_return_val_if_fail (length == 0 || text != NULL, NULL);

  if (length == 0 || g_utf8_get_char (text + start_index) == '\0')
    return NULL;

  itemize_state_init (&state, context, text, base_dir, start_index, length,
                      attrs, cached_iter, NULL);

  do
    itemize_state_process_run (&state);
  while (itemize_state_next (&state));

  itemize_state_finish (&state);

  return g_list_reverse (state.result);
}

static GList *
itemize_with_font (PangoContext               *context,
                   const char                 *text,
                   int                         start_index,
                   int                         length,
                   const PangoFontDescription *desc)
{
  ItemizeState state;

  if (length == 0)
    return NULL;

  itemize_state_init (&state, context, text, context->base_dir, start_index, length,
                      NULL, NULL, desc);

  do
    itemize_state_process_run (&state);
  while (itemize_state_next (&state));

  itemize_state_finish (&state);

  return g_list_reverse (state.result);
}

/**
 * pango_itemize:
 * @context: a structure holding information that affects
 *   the itemization process.
 * @text: the text to itemize. Must be valid UTF-8
 * @start_index: first byte in @text to process
 * @length: the number of bytes (not characters) to process
 *   after @start_index. This must be >= 0.
 * @attrs: the set of attributes that apply to @text.
 * @cached_iter: (nullable): Cached attribute iterator, or %NULL
 *
 * Breaks a piece of text into segments with consistent directional
 * level and font.
 *
 * Each byte of @text will be contained in exactly one of the items in the
 * returned list; the generated list of items will be in logical order (the
 * start offsets of the items are ascending).
 *
 * @cached_iter should be an iterator over @attrs currently positioned
 * at a range before or containing @start_index; @cached_iter will be
 * advanced to the range covering the position just after
 * @start_index + @length. (i.e. if itemizing in a loop, just keep passing
 * in the same @cached_iter).
 *
 * Return value: (transfer full) (element-type Pango.Item): a `GList` of
 *   [struct@Pango.Item] structures. The items should be freed using
 *   [method@Pango.Item.free] probably in combination with g_list_free_full().
 */
GList *
pango_itemize (PangoContext      *context,
               const char        *text,
               int                start_index,
               int                length,
               PangoAttrList     *attrs,
               PangoAttrIterator *cached_iter)
{
  g_return_val_if_fail (context != NULL, NULL);
  g_return_val_if_fail (start_index >= 0, NULL);
  g_return_val_if_fail (length >= 0, NULL);
  g_return_val_if_fail (length == 0 || text != NULL, NULL);

  return pango_itemize_with_base_dir (context, context->base_dir,
                                      text, start_index, length, attrs, cached_iter);
}

static gboolean
get_first_metrics_foreach (PangoFontset  *fontset,
                           PangoFont     *font,
                           gpointer       data)
{
  PangoFontMetrics *fontset_metrics = data;
  PangoLanguage *language = PANGO_FONTSET_GET_CLASS (fontset)->get_language (fontset);
  PangoFontMetrics *font_metrics = pango_font_get_metrics (font, language);
  guint save_ref_count;

  /* Initialize the fontset metrics to metrics of the first font in the
   * fontset; saving the refcount and restoring it is a bit of hack but avoids
   * having to update this code for each metrics addition.
   */
  save_ref_count = fontset_metrics->ref_count;
  *fontset_metrics = *font_metrics;
  fontset_metrics->ref_count = save_ref_count;

  pango_font_metrics_unref (font_metrics);

  return TRUE;                  /* Stops iteration */
}

static PangoFontMetrics *
get_base_metrics (PangoFontset *fontset)
{
  PangoFontMetrics *metrics = pango_font_metrics_new ();

  /* Initialize the metrics from the first font in the fontset */
  pango_fontset_foreach (fontset, get_first_metrics_foreach, metrics);

  return metrics;
}

static void
update_metrics_from_items (PangoFontMetrics *metrics,
                           PangoLanguage    *language,
                           const char       *text,
                           unsigned int      text_len,
                           GList            *items)

{
  GHashTable *fonts_seen = g_hash_table_new (NULL, NULL);
  PangoGlyphString *glyphs = pango_glyph_string_new ();
  GList *l;
  glong text_width;

  /* This should typically be called with a sample text string. */
  g_return_if_fail (text_len > 0);

  metrics->approximate_char_width = 0;

  for (l = items; l; l = l->next)
    {
      PangoItem *item = l->data;
      PangoFont *font = item->analysis.font;

      if (font != NULL && g_hash_table_lookup (fonts_seen, font) == NULL)
        {
          PangoFontMetrics *raw_metrics = pango_font_get_metrics (font, language);
          g_hash_table_insert (fonts_seen, font, font);

          /* metrics will already be initialized from the first font in the fontset */
          metrics->ascent = MAX (metrics->ascent, raw_metrics->ascent);
          metrics->descent = MAX (metrics->descent, raw_metrics->descent);
          metrics->height = MAX (metrics->height, raw_metrics->height);
          pango_font_metrics_unref (raw_metrics);
        }

      pango_shape_full (text + item->offset, item->length,
                        text, text_len,
                        &item->analysis, glyphs);
      metrics->approximate_char_width += pango_glyph_string_get_width (glyphs);
    }

  pango_glyph_string_free (glyphs);
  g_hash_table_destroy (fonts_seen);

  text_width = pango_utf8_strwidth (text);
  g_assert (text_width > 0);
  metrics->approximate_char_width /= text_width;
}

/**
 * pango_context_get_metrics:
 * @context: a `PangoContext`
 * @desc: (nullable): a `PangoFontDescription` structure. %NULL means that the
 *   font description from the context will be used.
 * @language: (nullable): language tag used to determine which script to get
 *   the metrics for. %NULL means that the language tag from the context
 *   will be used. If no language tag is set on the context, metrics
 *   for the default language (as determined by [type_func@Pango.Language.get_default]
 *   will be returned.
 *
 * Get overall metric information for a particular font description.
 *
 * Since the metrics may be substantially different for different scripts,
 * a language tag can be provided to indicate that the metrics should be
 * retrieved that correspond to the script(s) used by that language.
 *
 * The `PangoFontDescription` is interpreted in the same way as by [func@itemize],
 * and the family name may be a comma separated list of names. If characters
 * from multiple of these families would be used to render the string, then
 * the returned fonts would be a composite of the metrics for the fonts loaded
 * for the individual families.
 *
 * Return value: a `PangoFontMetrics` object. The caller must call
 *   [method@Pango.FontMetrics.unref] when finished using the object.
 */
PangoFontMetrics *
pango_context_get_metrics (PangoContext               *context,
                           const PangoFontDescription *desc,
                           PangoLanguage              *language)
{
  PangoFontset *current_fonts = NULL;
  PangoFontMetrics *metrics;
  const char *sample_str;
  unsigned int text_len;
  GList *items;

  g_return_val_if_fail (PANGO_IS_CONTEXT (context), NULL);

  if (!desc)
    desc = context->font_desc;

  if (!language)
    language = context->language;

  if (desc == context->font_desc &&
      language == context->language &&
      context->metrics != NULL)
    return pango_font_metrics_ref (context->metrics);

  current_fonts = pango_font_map_load_fontset (context->font_map, context, desc, language);
  metrics = get_base_metrics (current_fonts);

  sample_str = pango_language_get_sample_string (language);
  text_len = strlen (sample_str);
  items = itemize_with_font (context, sample_str, 0, text_len, desc);

  update_metrics_from_items (metrics, language, sample_str, text_len, items);

  g_list_foreach (items, (GFunc)pango_item_free, NULL);
  g_list_free (items);

  g_object_unref (current_fonts);

  if (desc == context->font_desc &&
      language == context->language)
    context->metrics = pango_font_metrics_ref (metrics);

  return metrics;
}

static void
context_changed (PangoContext *context)
{
  context->serial++;
  if (context->serial == 0)
    context->serial++;

  g_clear_pointer (&context->metrics, pango_font_metrics_unref);
}

/**
 * pango_context_changed:
 * @context: a `PangoContext`
 *
 * Forces a change in the context, which will cause any `PangoLayout`
 * using this context to re-layout.
 *
 * This function is only useful when implementing a new backend
 * for Pango, something applications won't do. Backends should
 * call this function if they have attached extra data to the context
 * and such data is changed.
 *
 * Since: 1.32.4
 **/
void
pango_context_changed (PangoContext *context)
{
  context_changed (context);
}

static void
check_fontmap_changed (PangoContext *context)
{
  guint old_serial = context->fontmap_serial;

  if (!context->font_map)
    return;

  context->fontmap_serial = pango_font_map_get_serial (context->font_map);

  if (old_serial != context->fontmap_serial)
    context_changed (context);
}

/**
 * pango_context_get_serial:
 * @context: a `PangoContext`
 *
 * Returns the current serial number of @context.
 *
 * The serial number is initialized to an small number larger than zero
 * when a new context is created and is increased whenever the context
 * is changed using any of the setter functions, or the `PangoFontMap` it
 * uses to find fonts has changed. The serial may wrap, but will never
 * have the value 0. Since it can wrap, never compare it with "less than",
 * always use "not equals".
 *
 * This can be used to automatically detect changes to a `PangoContext`,
 * and is only useful when implementing objects that need update when their
 * `PangoContext` changes, like `PangoLayout`.
 *
 * Return value: The current serial number of @context.
 *
 * Since: 1.32.4
 */
guint
pango_context_get_serial (PangoContext *context)
{
  check_fontmap_changed (context);
  return context->serial;
}

/**
 * pango_context_set_round_glyph_positions:
 * @context: a `PangoContext`
 * @round_positions: whether to round glyph positions
 *
 * Sets whether font rendering with this context should
 * round glyph positions and widths to integral positions,
 * in device units.
 *
 * This is useful when the renderer can't handle subpixel
 * positioning of glyphs.
 *
 * The default value is to round glyph positions, to remain
 * compatible with previous Pango behavior.
 *
 * Since: 1.44
 */
void
pango_context_set_round_glyph_positions (PangoContext *context,
                                         gboolean      round_positions)
{
  if (context->round_glyph_positions != round_positions)
    {
      context->round_glyph_positions = round_positions;
      context_changed (context);
    }
}

/**
 * pango_context_get_round_glyph_positions:
 * @context: a `PangoContext`
 *
 * Returns whether font rendering with this context should
 * round glyph positions and widths.
 *
 * Since: 1.44
 */
gboolean
pango_context_get_round_glyph_positions (PangoContext *context)
{
  return context->round_glyph_positions;
}
