/* Pango
 *
 * Copyright (C) 2021 Matthias Clasen
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Library General Public
 * License as published by the Free Software Foundation; either
 * version 2 of the License, or (at your option) any later version.
 *
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the GNU
 * Library General Public License for more details.
 *
 * You should have received a copy of the GNU Library General Public
 * License along with this library; if not, write to the
 * Free Software Foundation, Inc., 59 Temple Place - Suite 330,
 * Boston, MA 02111-1307, USA.
 */

#include "config.h"
#include <math.h>

#include <gio/gio.h>

#include "pango-hbfontmap-private.h"
#include "pango-hbfamily-private.h"
#include "pango-generic-family-private.h"
#include "pango-hbface-private.h"
#include "pango-hbfont-private.h"
#include "pango-fontset-private.h"
#include "pango-userface-private.h"
#include "pango-userfont-private.h"
#include "pango-fontset.h"
#include "pango-font-face-private.h"
#include "pango-trace-private.h"
#include "pango-context.h"

#include <hb-ot.h>


/**
 * PangoHbFontMap:
 *
 * `PangoHbFontMap` is a `PangoFontMap` subclass for use with
 * `PangoHbFace` and `PangoHbFont`. It handles caching and
 * lookup of faces and fonts.
 *
 * Subclasses populate the fontmap using backend-specific APIs
 * to enumerate the available fonts on the sytem, but it is
 * also possible to create an instance of `PangoHbFontMap` and
 * populate it manually using [method@Pango.HbFontMap.add_file]
 * and [method@Pango.HbFontMap.add_face].
 *
 * Note that to be fully functional, a fontmap needs to provide
 * generic families for monospace and sans-serif. These can
 * be added using [method@Pango.HbFontMap.add_family] and
 * [ctor@Pango.GenericFamily.new].
 */


/* The central api is load_fontset, which takes a font description
 * and language, finds the matching faces, and creates a PangoFontset
 * for them. To speed this operation up, the font map maintains a
 * cache of fontsets (in the form of a GQueue) and has a hash table
 * for looking up existing fontsets.
 *
 * The PangoFontsetCached object is the fontset subclass that is used
 * here, and it contains the necessary data for the cashing and hashing.
 * PangoFontsetCached also caches the character-to-font mapping that is
 * used when itemizing text.
 */

/* The number of fontsets we keep in the fontset cache */

#define FONTSET_CACHE_SIZE 256


/* {{{ GListModel implementation */

static GType
pango_hb_font_map_get_item_type (GListModel *list)
{
  return PANGO_TYPE_FONT_FAMILY;
}

static guint
pango_hb_font_map_get_n_items (GListModel *list)
{
  PangoHbFontMap *self = PANGO_HB_FONT_MAP (list);

  return self->families->len;
}

static gpointer
pango_hb_font_map_get_item (GListModel *list,
                            guint       position)
{
  PangoHbFontMap *self = PANGO_HB_FONT_MAP (list);

  if (position < self->families->len)
    return g_object_ref (g_ptr_array_index (self->families, position));

  return NULL;
}

static void
pango_hb_font_map_list_model_init (GListModelInterface *iface)
{
  iface->get_item_type = pango_hb_font_map_get_item_type;
  iface->get_n_items = pango_hb_font_map_get_n_items;
  iface->get_item = pango_hb_font_map_get_item;
}

/* }}} */
/* {{{ Fontset implementation */

typedef struct _PangoFontsetCached PangoFontsetCached;

struct _PangoFontsetCached
{
  PangoFontset parent_instance;

  GPtrArray *items; /* contains PangoHbFont or PangoGenericFamily */
  PangoLanguage *language;
  PangoFontDescription *description;
  float dpi;
  const PangoMatrix *matrix;
  GList cache_link;
  GHashTable *cache;
};

typedef PangoFontsetClass PangoFontsetCachedClass;

GType pango_fontset_cached_get_type (void) G_GNUC_CONST;

G_DEFINE_TYPE (PangoFontsetCached, pango_fontset_cached, PANGO_TYPE_FONTSET);

static void
pango_fontset_cached_init (PangoFontsetCached *fontset)
{
  fontset->items = g_ptr_array_new_with_free_func (g_object_unref);
  fontset->cache = g_hash_table_new_full (NULL, NULL, NULL, g_object_unref);
  fontset->language = NULL;
}

static void
pango_fontset_cached_finalize (GObject *object)
{
  PangoFontsetCached *self = (PangoFontsetCached *)object;

  g_ptr_array_free (self->items, TRUE);
  g_hash_table_unref (self->cache);
  pango_font_description_free (self->description);

  G_OBJECT_CLASS (pango_fontset_cached_parent_class)->finalize (object);
}

static PangoFont *
pango_fontset_cached_get_font (PangoFontset *fontset,
                               guint         wc)
{
  PangoFontsetCached *self = (PangoFontsetCached *)fontset;
  PangoFont *retval;
  int i;

  retval = g_hash_table_lookup (self->cache, GUINT_TO_POINTER (wc));
  if (retval)
    return g_object_ref (retval);

  for (i = 0; i < self->items->len; i++)
    {
      gpointer item = g_ptr_array_index (self->items, i);

      if (PANGO_IS_FONT (item))
        {
          PangoFont *font = PANGO_FONT (item);
          if (pango_font_has_char (font, wc))
            {
              retval = g_object_ref (font);
              break;
            }
        }
      else if (PANGO_IS_GENERIC_FAMILY (item))
        {
          PangoGenericFamily *family = PANGO_GENERIC_FAMILY (item);
          PangoFontFace *face;

          /* Here is where we implement delayed picking for generic families.
           * If a face does not cover the character and its family is generic,
           * choose a different face from the same family and create a font to use.
           */
          face = pango_generic_family_find_face (family,
                                                 self->description,
                                                 self->language,
                                                 wc);
          if (face)
            {
              retval = pango_font_face_create_font (face,
                                                    self->description,
                                                    self->dpi,
                                                    self->matrix);
              break;
            }
        }
    }

  if (retval)
    g_hash_table_insert (self->cache, GUINT_TO_POINTER (wc), g_object_ref (retval));

  return retval;
}

static PangoFont *
pango_fontset_cached_get_first_font (PangoFontsetCached *self)
{
  gpointer item;

  item = g_ptr_array_index (self->items, 0);

  if (PANGO_IS_FONT (item))
    return g_object_ref (PANGO_FONT (item));
  else if (PANGO_IS_GENERIC_FAMILY (item))
    {
      PangoFontFace *face = pango_generic_family_find_face (PANGO_GENERIC_FAMILY (item), self->description, self->language, 0);
      return pango_font_face_create_font (face, self->description, self->dpi, self->matrix);
    }

  return NULL;
}

static PangoFontMetrics *
pango_fontset_cached_get_metrics (PangoFontset *fontset)
{
  PangoFontsetCached *self = (PangoFontsetCached *)fontset;

  if (self->items->len == 1)
    {
      PangoFont *font = pango_fontset_cached_get_first_font (self);
      PangoFontMetrics *ret;

      ret = pango_font_get_metrics (font, self->language);
      g_object_unref (font);

      return ret;
    }

  return PANGO_FONTSET_CLASS (pango_fontset_cached_parent_class)->get_metrics (fontset);
}

static PangoLanguage *
pango_fontset_cached_get_language (PangoFontset *fontset)
{
  PangoFontsetCached *self = (PangoFontsetCached *)fontset;

  return self->language;
}

static void
pango_fontset_cached_foreach (PangoFontset            *fontset,
                              PangoFontsetForeachFunc  func,
                              gpointer                 data)
{
  PangoFontsetCached *self = (PangoFontsetCached *)fontset;
  unsigned int i;

  for (i = 0; i < self->items->len; i++)
    {
      gpointer item = g_ptr_array_index (self->items, i);
      PangoFont *font = NULL;

      if (PANGO_IS_FONT (item))
        {
          font = g_object_ref (PANGO_FONT (item));
        }
      else if (PANGO_IS_GENERIC_FAMILY (item))
        {
          PangoFontFace *face = pango_generic_family_find_face (PANGO_GENERIC_FAMILY (item), self->description, self->language, 0);
          font = pango_font_face_create_font (face, self->description, self->dpi, self->matrix);
        }

      if ((*func) (fontset, font, data))
        {
          g_object_unref (font);
          return;
        }
      g_object_unref (font);
    }
}

static void
pango_fontset_cached_class_init (PangoFontsetCachedClass *class)
{
  GObjectClass *object_class = G_OBJECT_CLASS (class);
  PangoFontsetClass *fontset_class = PANGO_FONTSET_CLASS (class);

  object_class->finalize = pango_fontset_cached_finalize;

  fontset_class->get_font = pango_fontset_cached_get_font;
  fontset_class->get_metrics = pango_fontset_cached_get_metrics;
  fontset_class->get_language = pango_fontset_cached_get_language;
  fontset_class->foreach = pango_fontset_cached_foreach;
}

static PangoFontsetCached *
pango_fontset_cached_new (const PangoFontDescription *description,
                          PangoLanguage              *language,
                          float                       dpi,
                          const PangoMatrix          *matrix)
{
  PangoFontsetCached *self;

  self = g_object_new (pango_fontset_cached_get_type (), NULL);
  self->language = language;
  self->description = pango_font_description_copy (description);
  self->dpi = dpi;
  self->matrix = matrix;

  return self;
}

static void
pango_fontset_cached_add_face (PangoFontsetCached *self,
                               PangoFontFace      *face)
{
  g_ptr_array_add (self->items,
                   pango_font_face_create_font (face,
                                                self->description,
                                                self->dpi,
                                                self->matrix));
}

static void
pango_fontset_cached_add_family (PangoFontsetCached *self,
                                 PangoGenericFamily *family)
{
  g_ptr_array_add (self->items, g_object_ref (family));
}

static int
pango_fontset_cached_size (PangoFontsetCached *self)
{
  return self->items->len;
}

#define FNV1_32_INIT ((guint32)0x811c9dc5)

static guint
pango_fontset_cached_hash (const PangoFontsetCached *fontset)
{
  guint32 hash = FNV1_32_INIT;

  return (hash ^
          GPOINTER_TO_UINT (fontset->language) ^
          pango_font_description_hash (fontset->description));
}

static gboolean
pango_fontset_cached_equal (const PangoFontsetCached *a,
                            const PangoFontsetCached *b)
{
  return a->language == b->language &&
         pango_font_description_equal (a->description, b->description);
}

static void
pango_fontset_cache (PangoFontsetCached *fontset,
                     PangoHbFontMap     *self)
{
  GQueue *cache = &self->fontset_cache;
  GList *link = &fontset->cache_link;

  if (link->data == fontset)
    {
      /* Already in cache, move to head */
      if (link == cache->head)
        return;

      g_queue_unlink (cache, link);
    }
  else
    {
      /* Not in cache yet. Make room... */
      if (cache->length == FONTSET_CACHE_SIZE)
        {
          GList *old = g_queue_pop_tail_link (cache);
          g_hash_table_remove (self->fontsets, old->data);
          old->data = NULL;
        }
    }

  link->data = fontset;
  link->prev = NULL;
  link->next = NULL;
  g_queue_push_head_link (cache, link);
}

/* }}} */
/* {{{ Utilities */

typedef struct {
  PangoFontFamily family;
  PangoFontMap *map;
  const char *name;
} FamilyKey;

static guint
pango_family_hash (const FamilyKey *key)
{
  const char *p;
  guint32 h = 5381;

  for (p = (const char *)key->name; *p != '\0'; p++)
    h = (h << 5) + h + g_ascii_tolower (*p);

  return h;
}

static gboolean
pango_family_equal (const FamilyKey *a,
                    const FamilyKey *b)
{
  return g_ascii_strcasecmp (a->name, b->name) == 0;
}

static PangoFontFamily *
find_family (PangoHbFontMap *self,
             const char     *family_name)
{
  FamilyKey lookup;
  PangoFontFamily *family;

  lookup.name = family_name;

  family = PANGO_FONT_FAMILY (g_hash_table_lookup (self->families_hash, &lookup));

  return family;
}

static void
clear_caches (PangoHbFontMap *self)
{
  g_hash_table_remove_all (self->fontsets);
  g_queue_init (&self->fontset_cache);
}

static void
add_style_variation (PangoHbFamily *family,
                     PangoHbFace   *face,
                     PangoStyle     style,
                     PangoWeight    weight)
{
  PangoMatrix italic_matrix = { 1, 0.2, 0, 1, 0, 0 };
  PangoFontDescription *desc;
  PangoHbFace *variation;

  desc = pango_font_description_new ();
  pango_font_description_set_family (desc, pango_font_family_get_name (PANGO_FONT_FAMILY (family)));
  pango_font_description_set_style (desc, style);
  pango_font_description_set_weight (desc, weight);

  variation = pango_hb_face_new_synthetic (face,
                                           style == PANGO_STYLE_ITALIC ? &italic_matrix : NULL,
                                           weight == PANGO_WEIGHT_BOLD,
                                           NULL,
                                           desc);
  pango_hb_family_add_face (family, PANGO_FONT_FACE (variation));

  pango_font_description_free (desc);
}

static void
synthesize_bold_and_italic_faces (PangoHbFontMap *map)
{
  for (int i = 0; i < map->families->len; i++)
    {
      PangoFontFamily *family = g_ptr_array_index (map->families, i);
      PangoHbFace *regular_face = NULL;
      int regular_dist = G_MAXINT;
      int bold_dist = G_MAXINT;
      gboolean has_italic = FALSE;
      gboolean has_bold = FALSE;
      gboolean has_bold_italic = FALSE;

      if (PANGO_IS_GENERIC_FAMILY (family))
        continue;

      for (int j = 0; j < g_list_model_get_n_items (G_LIST_MODEL (family)); j++)
        {
          PangoHbFace *face = g_list_model_get_item (G_LIST_MODEL (family), j);
          int weight;
          PangoStyle style;
          int dist;

          if (!PANGO_IS_HB_FACE (face))
            continue;

          weight = pango_font_description_get_weight (face->description);
          style = pango_font_description_get_style (face->description);

          if (style == PANGO_STYLE_NORMAL)
            {
              dist = abs (weight - (int)PANGO_WEIGHT_NORMAL);

              if (dist < regular_dist)
                {
                  regular_dist = dist;
                  if (dist < 150)
                    regular_face = face;
                }

              dist = abs (weight - (int)PANGO_WEIGHT_BOLD);

              if (dist < bold_dist)
                {
                  bold_dist = dist;
                  has_bold = dist < 150;
                }
            }
          else
            {
              if (weight < PANGO_WEIGHT_SEMIBOLD)
                has_italic = TRUE;
              else
                has_bold_italic = TRUE;
            }

          g_object_unref (face);
        }

      if (regular_face)
        {
          if (!has_italic)
            add_style_variation (PANGO_HB_FAMILY (family), regular_face, PANGO_STYLE_ITALIC, PANGO_WEIGHT_NORMAL);

          if (!has_bold)
            add_style_variation (PANGO_HB_FAMILY (family), regular_face, PANGO_STYLE_NORMAL, PANGO_WEIGHT_BOLD);

          if (!has_bold_italic)
            add_style_variation (PANGO_HB_FAMILY (family), regular_face, PANGO_STYLE_ITALIC, PANGO_WEIGHT_BOLD);
        }
    }
}

/* }}} */
/* {{{ PangoFontMap implementation */

G_DEFINE_TYPE_WITH_CODE (PangoHbFontMap, pango_hb_font_map, PANGO_TYPE_FONT_MAP,
                         G_IMPLEMENT_INTERFACE (G_TYPE_LIST_MODEL, pango_hb_font_map_list_model_init))


static void
pango_hb_font_map_init (PangoHbFontMap *self)
{
  self->added_faces = g_ptr_array_new_with_free_func (g_object_unref);
  self->added_families = g_ptr_array_new_with_free_func (g_object_unref);
  self->families = g_ptr_array_new_with_free_func (g_object_unref);
  self->families_hash = g_hash_table_new_full ((GHashFunc) pango_family_hash,
                                               (GEqualFunc) pango_family_equal,
                                               NULL,
                                               NULL);
  self->fontsets = g_hash_table_new_full ((GHashFunc) pango_fontset_cached_hash,
                                          (GEqualFunc) pango_fontset_cached_equal,
                                          NULL,
                                          (GDestroyNotify) g_object_unref);
  g_queue_init (&self->fontset_cache);
  self->dpi = 96.;
}

static void
pango_hb_font_map_finalize (GObject *object)
{
  PangoHbFontMap *self = PANGO_HB_FONT_MAP (object);

  g_ptr_array_unref (self->added_faces);
  g_ptr_array_unref (self->added_families);
  g_hash_table_unref (self->families_hash);
  g_ptr_array_unref (self->families);
  g_hash_table_unref (self->fontsets);

  G_OBJECT_CLASS (pango_hb_font_map_parent_class)->finalize (object);
}

/* Load a font from the first matching family */
static PangoFont *
pango_hb_font_map_load_font (PangoFontMap               *map,
                             PangoContext               *context,
                             const PangoFontDescription *description)
{
  PangoHbFontMap *self = PANGO_HB_FONT_MAP (map);
  PangoFontsetCached *fontset;
  PangoLanguage *language;
  PangoFont *font = NULL;

  if (self->families->len == 0)
    return NULL;

  if (context)
    language = pango_context_get_language (context);
  else
    language = NULL;

  fontset = (PangoFontsetCached *)pango_font_map_load_fontset (map, context, description, language);
  if (pango_fontset_cached_size (fontset) > 0)
    font = pango_fontset_cached_get_first_font (fontset);
  g_object_unref (fontset);

  return font;
}

/* Add one font for each family we find */
static PangoFontset *
pango_hb_font_map_load_fontset (PangoFontMap               *map,
                                PangoContext               *context,
                                const PangoFontDescription *description,
                                PangoLanguage              *language)
{
  PangoHbFontMap *self = PANGO_HB_FONT_MAP (map);
  PangoFontsetCached lookup;
  PangoFontsetCached *fontset;
  const PangoMatrix *matrix;
  const char *family_name;
  char **families;
  PangoFontDescription *copy;
  PangoFontFamily *family;
  PangoFontFace *face;
  gboolean has_generic = FALSE;
  gint64 before G_GNUC_UNUSED;

  before = PANGO_TRACE_CURRENT_TIME;

  if (!language)
    language = pango_context_get_language (context);

  family_name = pango_font_description_get_family (description);

  lookup.language = language;
  lookup.description = (PangoFontDescription *)description;
  fontset = g_hash_table_lookup (self->fontsets, &lookup);

  if (fontset)
    goto done;

  matrix = pango_context_get_matrix (context);

  fontset = pango_fontset_cached_new (description, language, self->dpi, matrix);

  if (self->families->len == 0)
    {
      g_warning ("Font map contains no fonts!!!!");
      goto done_no_cache;
    }

  families = g_strsplit (family_name ? family_name : "", ",", -1);

  /* Unset gravity and variant since PangoHbFace does not have these fields */
  copy = pango_font_description_copy_static (description);
  pango_font_description_unset_fields (copy, PANGO_FONT_MASK_VARIATIONS |
                                             PANGO_FONT_MASK_GRAVITY |
                                             PANGO_FONT_MASK_VARIANT);

  for (int i = 0; families[i]; i++)
    {
      family = find_family (self, families[i]);
      if (!family)
        continue;

      if (PANGO_IS_GENERIC_FAMILY (family))
        {
          pango_fontset_cached_add_family (fontset, PANGO_GENERIC_FAMILY (family));
          has_generic = TRUE;
        }
      else
        {
          face = pango_hb_family_find_face (PANGO_HB_FAMILY (family), copy, language, 0);
          if (face)
            pango_fontset_cached_add_face (fontset, face);
        }
    }

  g_strfreev (families);

  /* Returning an empty fontset leads to bad outcomes.
   *
   * We always include a generic family in order
   * to produce fontsets with good coverage.
   */
  if (!has_generic)
    {
      family = find_family (self, "sans-serif");
      if (PANGO_IS_GENERIC_FAMILY (family))
        pango_fontset_cached_add_family (fontset, PANGO_GENERIC_FAMILY (family));
    }

  pango_font_description_free (copy);

  g_hash_table_add (self->fontsets, fontset);

done:
  pango_fontset_cache (fontset, self);

  if (pango_fontset_cached_size (fontset) == 0)
    {
      char *s = pango_font_description_to_string (description);
      g_warning ("All font fallbacks failed for \"%s\", in %s !!!!", s, pango_language_to_string (language));
      g_free (s);
    }

done_no_cache:
  pango_trace_mark (before, "pango_hb_fontmap_load_fontset", "%s", family_name);

  return g_object_ref (PANGO_FONTSET (fontset));
}

static PangoFontFamily *
pango_hb_font_map_get_family (PangoFontMap *map,
                              const char   *name)
{
  PangoHbFontMap *self = PANGO_HB_FONT_MAP (map);

  return PANGO_FONT_FAMILY (find_family (self, name));
}

static void
pango_hb_font_map_populate (PangoHbFontMap *self)
{
}

static guint
pango_hb_font_map_get_serial (PangoFontMap *map)
{
  PangoHbFontMap *self = PANGO_HB_FONT_MAP (map);

  return self->serial;
}

static void
pango_hb_font_map_changed (PangoFontMap *map)
{
  PangoHbFontMap *self = PANGO_HB_FONT_MAP (map);

  self->serial++;
  if (self->serial == 0)
    self->serial++;
}

static void
pango_hb_font_map_class_init (PangoHbFontMapClass *class)
{
  GObjectClass *object_class = G_OBJECT_CLASS (class);
  PangoFontMapClass *fontmap_class = PANGO_FONT_MAP_CLASS (class);

  object_class->finalize = pango_hb_font_map_finalize;

  fontmap_class->load_font = pango_hb_font_map_load_font;
  fontmap_class->load_fontset = pango_hb_font_map_load_fontset;
  fontmap_class->get_family = pango_hb_font_map_get_family;
  fontmap_class->get_serial = pango_hb_font_map_get_serial;
  fontmap_class->changed = pango_hb_font_map_changed;

  class->populate = pango_hb_font_map_populate;
}

/* }}} */
/* {{{ Private API */

/*< private >
 * pango_hb_font_map_repopulate:
 * @self: a `PangoHbFontMap`
 * @add_synthetic: if `TRUE`, missing bold and italic faces will be synthesized
 *
 * Clear all cached information and repopulate the fontmap with
 * families and faces.
 *
 * Subclasses should call this when their configuration changes
 * in a way that requires recreating families and faces.
 */
void
pango_hb_font_map_repopulate (PangoHbFontMap *self,
                              gboolean        add_synthetic)
{
  int removed, added;

  clear_caches (self);

  removed = self->families->len;

  g_hash_table_remove_all (self->families_hash);
  g_ptr_array_set_size (self->families, 0);

  self->in_populate = TRUE;

  PANGO_HB_FONT_MAP_GET_CLASS (self)->populate (self);

  if (add_synthetic)
    synthesize_bold_and_italic_faces (self);

  for (int i = 0; i < self->added_faces->len; i++)
    {
      PangoFontFace *face = g_ptr_array_index (self->added_faces, i);
      pango_hb_font_map_add_face (self, face);
    }

  for (int i = 0; i < self->added_families->len; i++)
    {
      PangoFontFamily *family = PANGO_FONT_FAMILY (g_ptr_array_index (self->added_families, i));
      pango_hb_font_map_add_family (self, family);
    }

  self->in_populate = FALSE;

  added = self->families->len;

  g_list_model_items_changed (G_LIST_MODEL (self), 0, removed, added);
  pango_font_map_changed (PANGO_FONT_MAP (self));
}

/* }}} */
/* {{{ Public API */

/**
 * pango_hb_font_map_new:
 *
 * Creates a new `PangoHbFontmMap`.
 *
 * Returns: A newly created `PangoHbFontMap
 */
PangoHbFontMap *
pango_hb_font_map_new (void)
{
  return g_object_new (PANGO_TYPE_HB_FONT_MAP, NULL);
}

/**
 * pango_hb_font_map_add_face:
 * @self: a `PangoHbFontMap`
 * @face: (transfer full): a `PangoFontFace`
 *
 * Adds @face to the `PangoHbFontMap`.
 *
 * This is most useful for creating transformed faces or aliases.
 * See [ctor@Pango.HbFace.new_synthetic] and [ctor@Pango.HbFace.new_instance].
 */
void
pango_hb_font_map_add_face (PangoHbFontMap *self,
                            PangoFontFace  *face)
{
  PangoFontMap *map = PANGO_FONT_MAP (self);
  const char *family_name;
  PangoHbFamily *family;
  const PangoFontDescription *description;

  g_return_if_fail (PANGO_IS_HB_FACE (face) || PANGO_IS_USER_FACE (face));

  description = ((CommonFace *)face)->description;

  if (pango_font_description_get_set_fields (description) &
      (PANGO_FONT_MASK_VARIANT | PANGO_FONT_MASK_GRAVITY))
    g_warning ("Font description for PangoFontFace includes things that it shouldn't");

  if (!self->in_populate)
    g_ptr_array_add (self->added_faces, g_object_ref (face));

  family_name = pango_font_description_get_family (description);
  family = PANGO_HB_FAMILY (pango_font_map_get_family (map, family_name));
  if (!family)
    {
      family = pango_hb_family_new (family_name);
      pango_hb_font_map_add_family (self, PANGO_FONT_FAMILY (family));
    }

  pango_hb_family_add_face (family, face);

  pango_font_map_changed (PANGO_FONT_MAP (self));

  clear_caches (self);
}

/**
 * pango_hb_font_map_remove_face:
 * @self: a `PangoHbFontMap`
 * @face: a `PangoFontFace` that belongs to @map
 *
 * Removes @face from the `PangoHbFontMap`.
 *
 * @face must have been added with [method@Pango.HbFontMap.add_face].
 */
void
pango_hb_font_map_remove_face (PangoHbFontMap *self,
                               PangoFontFace  *face)
{
  PangoHbFamily *family;
  unsigned int position;

  g_return_if_fail (PANGO_IS_HB_FACE (face) || PANGO_IS_USER_FACE (face));

  if (!g_ptr_array_find (self->added_faces, face, &position))
    return;

  family = PANGO_HB_FAMILY (pango_font_face_get_family (face));

  pango_hb_family_remove_face (family, face);

  if (family->faces->len == 0)
    pango_hb_font_map_remove_family (self, PANGO_FONT_FAMILY (family));

  pango_font_map_changed (PANGO_FONT_MAP (self));

  clear_caches (self);

  g_ptr_array_remove_index (self->added_faces, position);
}

/**
 * pango_hb_font_map_add_file:
 * @self: a `PangoHbFontMap`
 * @file: font filename
 *
 * Creates a new `PangoHbFace` and adds it.
 *
 * If you need to specify an instance id or other
 * parameters, use [ctor@Pango.HbFace.new_from_file].
 */
void
pango_hb_font_map_add_file (PangoHbFontMap *self,
                            const char     *file)
{
  PangoHbFace *face;

  face = pango_hb_face_new_from_file (file, 0, -1, NULL, NULL);
  pango_hb_font_map_add_face (self, PANGO_FONT_FACE (face));
}

/**
 * pango_hb_font_map_add_family:
 * @self: a `PangoHbFontMap`
 * @family: (transfer full): a `PangoFontFamily`
 *
 * Adds @family to @self.
 *
 * The fontmap must not contain a family with the
 * same name as @family yet.
 *
 * This is mostly useful for adding generic families
 * using [ctor@Pango.GenericFamily.new].
 */
void
pango_hb_font_map_add_family (PangoHbFontMap  *self,
                              PangoFontFamily *family)
{
  const char *name;
  int position;

  g_return_if_fail (PANGO_IS_HB_FAMILY (family) || PANGO_IS_GENERIC_FAMILY (family));
  g_return_if_fail (((CommonFamily *)family)->map == NULL);

  if (!self->in_populate)
    g_ptr_array_add (self->added_families, g_object_ref (family));

  name = ((CommonFamily *)family)->name;

  position = 0;
  while (position < self->families->len)
    {
      PangoFontFamily *f = g_ptr_array_index (self->families, position);
      if (g_ascii_strcasecmp (name, ((CommonFamily *)f)->name) < 0)
        break;
      position++;
    }

  ((CommonFamily *)family)->map = PANGO_FONT_MAP (self);
  g_object_add_weak_pointer (G_OBJECT (self), (gpointer *)&((CommonFamily *)family)->map);

  g_ptr_array_insert (self->families, position, family);
  g_hash_table_add (self->families_hash, family);

  if (!self->in_populate)
    g_list_model_items_changed (G_LIST_MODEL (self), position, 0, 1);

  pango_font_map_changed (PANGO_FONT_MAP (self));
}

/**
 * pango_hb_font_map_remove_family:
 * @self: a `PangoHbFontMap`
 * @family: a `PangoFontFamily` that belongs to @self
 *
 * Removes a `PangoHbFamily` from a `PangoHbFontMap`
 */
void
pango_hb_font_map_remove_family (PangoHbFontMap  *self,
                                 PangoFontFamily *family)
{
  unsigned int position;

  g_return_if_fail (PANGO_IS_HB_FAMILY (family) || PANGO_IS_GENERIC_FAMILY (family));
  g_return_if_fail (((CommonFamily *)family)->map == (PangoFontMap *)self);

  if (!g_ptr_array_find (self->added_families, family, &position))
    return;

  g_hash_table_remove (self->families_hash, family);
  g_ptr_array_remove_index (self->families, position);

  g_object_remove_weak_pointer (G_OBJECT (self), (gpointer *)&((CommonFamily *)family)->map);
  ((CommonFamily *)family)->map = NULL;

  if (!self->in_populate)
    g_list_model_items_changed (G_LIST_MODEL (self), position, 1, 0);

  pango_font_map_changed (PANGO_FONT_MAP (self));

  g_ptr_array_remove_index (self->added_families, position);
}

/**
 * pango_hb_font_map_set_resolution:
 * @self: a `PangoHbFontMap`
 * @dpi: the new resolution, in "dots per inch"
 *
 * Sets the resolution for the fontmap.
 *
 * This is a scale factor between points specified in a
 * `PangoFontDescription` and Cairo units. The default value
 * is 96, meaning that a 10 point font will be 13 units high.
 * (10 * 96. / 72. = 13.3).
 */
void
pango_hb_font_map_set_resolution (PangoHbFontMap *self,
                                  double          dpi)
{
  self->dpi = dpi;
  clear_caches (self);
  pango_font_map_changed (PANGO_FONT_MAP (self));
}

/* }}} */

/* vim:set foldmethod=marker expandtab: */
