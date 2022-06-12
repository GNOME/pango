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

#include "pango-fontmap-private.h"
#include "pango-hbfamily-private.h"
#include "pango-generic-family-private.h"
#include "pango-hbface-private.h"
#include "pango-hbfont-private.h"
#include "pango-fontset-cached-private.h"
#include "pango-userface-private.h"
#include "pango-userfont-private.h"
#include "pango-fontset.h"
#include "pango-font-face-private.h"
#include "pango-trace-private.h"
#include "pango-context.h"

#ifdef HAVE_CORE_TEXT
#include "pangocoretext-fontmap.h"
#endif

#ifdef HAVE_DIRECT_WRITE
#include "pangodwrite-fontmap.h"
#endif

#ifdef HAVE_FONTCONFIG
#include "pangofc-fontmap.h"
#endif

#ifdef HAVE_CAIRO
#include <cairo.h>
#include <pangocairo-private.h>
#endif

#include <hb-ot.h>


/**
 * PangoFontMap:
 *
 * `PangoFontMap` is the base class for font enumeration.
 * It also handles caching and lookup of faces and fonts.
 *
 * Subclasses populate the fontmap using backend-specific APIs
 * to enumerate the available fonts on the sytem, but it is
 * also possible to create an instance of `PangoFontMap` and
 * populate it manually using [method@Pango.FontMap.add_file]
 * and [method@Pango.FontMap.add_face].
 *
 * Note that to be fully functional, a fontmap needs to provide
 * generic families for monospace and sans-serif. These can
 * be added using [method@Pango.FontMap.add_family] and
 * [ctor@Pango.GenericFamily.new].
 *
 * `PangoFontMap` implements the [iface@Gio.ListModel] interface,
 * to provide a list of font families.
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


/* {{{ GListModel implementation */

static GType
pango_font_map_get_item_type (GListModel *list)
{
  return PANGO_TYPE_FONT_FAMILY;
}

static guint
pango_font_map_get_n_items (GListModel *list)
{
  PangoFontMap *self = PANGO_FONT_MAP (list);

  return self->families->len;
}

static gpointer
pango_font_map_get_item (GListModel *list,
                         guint       position)
{
  PangoFontMap *self = PANGO_FONT_MAP (list);

  if (position < self->families->len)
    return g_object_ref (g_ptr_array_index (self->families, position));

  return NULL;
}

static void
pango_font_map_list_model_init (GListModelInterface *iface)
{
  iface->get_item_type = pango_font_map_get_item_type;
  iface->get_n_items = pango_font_map_get_n_items;
  iface->get_item = pango_font_map_get_item;
}

/* }}} */
/* {{{ Fontset caching */

/* The number of fontsets we keep in the fontset cache */
#define FONTSET_CACHE_SIZE 256

#define FNV_32_PRIME ((guint32)0x01000193)
#define FNV1_32_INIT ((guint32)0x811c9dc5)

static guint32
hash_bytes_fnv (unsigned char *buffer,
                int            len,
                guint32        hval)
{
  while (len--)
    {
      hval *= FNV_32_PRIME;
      hval ^= *buffer++;
    }

  return hval;
}

static guint
pango_fontset_cached_hash (const PangoFontsetCached *fontset)
{
  guint32 hash = FNV1_32_INIT;

  if (fontset->ctm)
    hash = hash_bytes_fnv ((unsigned char *)(fontset->ctm), sizeof (double) * 4, hash);

  return (hash ^
          GPOINTER_TO_UINT (fontset->language) ^
#ifdef HAVE_CAIRO
          cairo_font_options_hash (fontset->font_options) ^
#endif
          pango_font_description_hash (fontset->description));
}

static gboolean
pango_fontset_cached_equal (const PangoFontsetCached *a,
                            const PangoFontsetCached *b)
{
  return a->language == b->language &&
#ifdef HAVE_CAIRO
         cairo_font_options_equal (a->font_options, b->font_options) &&
#endif
         (a->ctm == b->ctm ||
          (a->ctm && b->ctm && memcmp (a->ctm, b->ctm, 4 * sizeof (double)) == 0)) &&
         pango_font_description_equal (a->description, b->description);
}

static void
pango_fontset_cache (PangoFontsetCached *fontset,
                     PangoFontMap       *self)
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

static guint
pango_family_hash (const PangoFontFamily *key)
{
  const char *p;
  guint32 h = 5381;

  for (p = (const char *)key->name; *p != '\0'; p++)
    h = (h << 5) + h + g_ascii_tolower (*p);

  return h;
}

static gboolean
pango_family_equal (const PangoFontFamily *a,
                    const PangoFontFamily *b)
{
  return g_ascii_strcasecmp (a->name, b->name) == 0;
}

static PangoFontFamily *
find_family (PangoFontMap *self,
             const char   *family_name)
{
  PangoFontFamily lookup;
  PangoFontFamily *family;

  lookup.name = (char *)family_name;

  family = PANGO_FONT_FAMILY (g_hash_table_lookup (self->families_hash, &lookup));

  return family;
}

static void
clear_caches (PangoFontMap *self)
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
synthesize_bold_and_italic_faces (PangoFontMap *map)
{
  for (int i = 0; i < map->families->len; i++)
    {
      PangoFontFamily *family = g_ptr_array_index (map->families, i);
      PangoFontFace *regular_face = NULL;
      int regular_dist = G_MAXINT;
      int bold_dist = G_MAXINT;
      gboolean has_italic = FALSE;
      gboolean has_bold = FALSE;
      gboolean has_bold_italic = FALSE;

      if (PANGO_IS_GENERIC_FAMILY (family))
        continue;

      for (int j = 0; j < g_list_model_get_n_items (G_LIST_MODEL (family)); j++)
        {
          PangoFontFace *face = g_list_model_get_item (G_LIST_MODEL (family), j);
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
            add_style_variation (PANGO_HB_FAMILY (family),
                                 PANGO_HB_FACE (regular_face),
                                 PANGO_STYLE_ITALIC,
                                 PANGO_WEIGHT_NORMAL);

          if (!has_bold)
            add_style_variation (PANGO_HB_FAMILY (family),
                                 PANGO_HB_FACE (regular_face),
                                 PANGO_STYLE_NORMAL,
                                 PANGO_WEIGHT_BOLD);

          if (!has_bold_italic)
            add_style_variation (PANGO_HB_FAMILY (family),
                                 PANGO_HB_FACE (regular_face),
                                 PANGO_STYLE_ITALIC,
                                 PANGO_WEIGHT_BOLD);
        }
    }
}

/* }}} */
/* {{{ PangoFontMap implementation */

enum {
  PROP_RESOLUTION = 1,
  PROP_ITEM_TYPE,
  PROP_N_ITEMS,
  N_PROPERTIES
};

static GParamSpec *properties[N_PROPERTIES] = { NULL };

G_DEFINE_TYPE_WITH_CODE (PangoFontMap, pango_font_map, G_TYPE_OBJECT,
                         G_IMPLEMENT_INTERFACE (G_TYPE_LIST_MODEL, pango_font_map_list_model_init))


static void
pango_font_map_init (PangoFontMap *self)
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
pango_font_map_finalize (GObject *object)
{
  PangoFontMap *self = PANGO_FONT_MAP (object);

  g_ptr_array_unref (self->added_faces);
  g_ptr_array_unref (self->added_families);
  g_hash_table_unref (self->families_hash);
  g_ptr_array_unref (self->families);
  g_hash_table_unref (self->fontsets);

  G_OBJECT_CLASS (pango_font_map_parent_class)->finalize (object);
}

static void
pango_font_map_set_property (GObject      *object,
                             guint         property_id,
                             const GValue *value,
                             GParamSpec   *pspec)
{
  PangoFontMap *map = PANGO_FONT_MAP (object);

  switch (property_id)
    {
    case PROP_RESOLUTION:
      pango_font_map_set_resolution (map, g_value_get_float (value));
      break;

    default:
      G_OBJECT_WARN_INVALID_PROPERTY_ID (object, property_id, pspec);
    }
}

static void
pango_font_map_get_property (GObject    *object,
                             guint       property_id,
                             GValue     *value,
                             GParamSpec *pspec)
{
  PangoFontMap *map = PANGO_FONT_MAP (object);

  switch (property_id)
    {
    case PROP_RESOLUTION:
      g_value_set_float (value, map->dpi);
      break;

    case PROP_ITEM_TYPE:
      g_value_set_gtype (value, PANGO_TYPE_FONT_FAMILY);
      break;

    case PROP_N_ITEMS:
      g_value_set_uint (value, pango_font_map_get_n_items (G_LIST_MODEL (object)));
      break;

    default:
      G_OBJECT_WARN_INVALID_PROPERTY_ID (object, property_id, pspec);
    }
}

/* Load a font from the first matching family */
static PangoFont *
pango_font_map_default_load_font (PangoFontMap               *self,
                                  PangoContext               *context,
                                  const PangoFontDescription *description)
{
  PangoFontsetCached *fontset;
  PangoLanguage *language;
  PangoFont *font = NULL;

  if (self->families->len == 0)
    return NULL;

  if (context)
    language = pango_context_get_language (context);
  else
    language = NULL;

  fontset = (PangoFontsetCached *)pango_font_map_load_fontset (self, context, description, language);
  if (pango_fontset_cached_size (fontset) > 0)
    font = pango_fontset_cached_get_first_font (fontset);
  g_object_unref (fontset);

  return font;
}

/* Add one font for each family we find */
static PangoFontset *
pango_font_map_default_load_fontset (PangoFontMap               *self,
                                     PangoContext               *context,
                                     const PangoFontDescription *description,
                                     PangoLanguage              *language)
{
  PangoFontsetCached lookup;
  PangoFontsetCached *fontset;
  const PangoMatrix *ctm;
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
  ctm = pango_context_get_matrix (context);

  lookup.language = language;
  lookup.description = (PangoFontDescription *)description;
  lookup.ctm = ctm;
#ifdef HAVE_CAIRO
  lookup.font_options = (cairo_font_options_t *)pango_cairo_context_get_merged_font_options (context);
#endif
  fontset = g_hash_table_lookup (self->fontsets, &lookup);

  if (fontset)
    goto done;

  fontset = pango_fontset_cached_new (description, language, self->dpi, ctm);
#ifdef HAVE_CAIRO
  fontset->font_options = cairo_font_options_copy (pango_cairo_context_get_merged_font_options (context));
#endif

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
pango_font_map_default_get_family (PangoFontMap *self,
                           const char   *name)
{
  return PANGO_FONT_FAMILY (find_family (self, name));
}

static void
pango_font_map_default_populate (PangoFontMap *self)
{
}

static guint
pango_font_map_default_get_serial (PangoFontMap *self)
{
  return self->serial;
}

static void
pango_font_map_default_changed (PangoFontMap *self)
{
  self->serial++;
  if (self->serial == 0)
    self->serial++;
}

static void
pango_font_map_class_init (PangoFontMapClass *class)
{
  GObjectClass *object_class = G_OBJECT_CLASS (class);

  object_class->finalize = pango_font_map_finalize;
  object_class->set_property = pango_font_map_set_property;
  object_class->get_property = pango_font_map_get_property;

  class->load_font = pango_font_map_default_load_font;
  class->load_fontset = pango_font_map_default_load_fontset;
  class->get_serial = pango_font_map_default_get_serial;
  class->changed = pango_font_map_default_changed;
  class->get_family = pango_font_map_default_get_family;
  class->populate = pango_font_map_default_populate;

  /**
   * PangoFontMap:resolution: (attributes org.gtk.Property.get=pango_font_map_get_resolution org.gtk.Property.set=pango_font_map_set_resolution)
   *
   * The resolution for the fontmap.
   *
   * This is a scale factor between points specified in a
   * `PangoFontDescription` and Cairo units. The default value
   * is 96, meaning that a 10 point font will be 13 units high.
   * (10 * 96. / 72. = 13.3).
   */
  properties[PROP_RESOLUTION] =
      g_param_spec_float ("resolution", NULL, NULL, 0, G_MAXFLOAT, 96.0,
                          G_PARAM_READWRITE | G_PARAM_STATIC_STRINGS);

  /**
   * PangoFontMap:item-type:
   *
   * The type of items contained in this list.
   */
  properties[PROP_ITEM_TYPE] =
    g_param_spec_gtype ("item-type", "", "", G_TYPE_OBJECT,
                        G_PARAM_READABLE | G_PARAM_STATIC_STRINGS);

  /**
   * PangoFontMap:n-items:
   *
   * The number of items contained in this list.
   */
  properties[PROP_N_ITEMS] =
    g_param_spec_uint ("n-items", "", "", 0, G_MAXUINT, 0,
                       G_PARAM_READABLE | G_PARAM_STATIC_STRINGS);

  g_object_class_install_properties (object_class, N_PROPERTIES, properties);
}

/* }}} */
/* {{{ Private API */

/*< private >
 * pango_font_map_repopulate:
 * @self: a `PangoFontMap`
 * @add_synthetic: if `TRUE`, missing bold and italic faces will be synthesized
 *
 * Clear all cached information and repopulate the fontmap with
 * families and faces.
 *
 * Subclasses should call this when their configuration changes
 * in a way that requires recreating families and faces.
 */
void
pango_font_map_repopulate (PangoFontMap *self,
                           gboolean      add_synthetic)
{
  int removed, added;

  clear_caches (self);

  removed = self->families->len;

  g_hash_table_remove_all (self->families_hash);
  g_ptr_array_set_size (self->families, 0);

  self->in_populate = TRUE;

  PANGO_FONT_MAP_GET_CLASS (self)->populate (self);

  if (add_synthetic)
    synthesize_bold_and_italic_faces (self);

  for (int i = 0; i < self->added_faces->len; i++)
    {
      PangoFontFace *face = g_ptr_array_index (self->added_faces, i);
      pango_font_map_add_face (self, face);
    }

  for (int i = 0; i < self->added_families->len; i++)
    {
      PangoFontFamily *family = PANGO_FONT_FAMILY (g_ptr_array_index (self->added_families, i));
      pango_font_map_add_family (self, family);
    }

  self->in_populate = FALSE;

  added = self->families->len;

  g_list_model_items_changed (G_LIST_MODEL (self), 0, removed, added);
  g_object_notify_by_pspec (G_OBJECT (self), properties[PROP_N_ITEMS]);
  pango_font_map_changed (self);
}

/*< private >
 * pango_font_map_changed:
 * @self: a `PangoFontMap`
 *
 * Forces a change in the context, which will cause any `PangoContext`
 * using this fontmap to change.
 *
 * This function is only useful when implementing a new backend
 * for Pango, something applications won't do. Backends should
 * call this function if they have attached extra data to the
 * context and such data is changed.
 */
void
pango_font_map_changed (PangoFontMap *self)
{
  g_return_if_fail (PANGO_IS_FONT_MAP (self));

  PANGO_FONT_MAP_GET_CLASS (self)->changed (self);
}

/* }}} */
/* {{{ Public API */

/**
 * pango_font_map_create_context:
 * @self: a `PangoFontMap`
 *
 * Creates a `PangoContext` connected to @fontmap.
 *
 * This is equivalent to [ctor@Pango.Context.new] followed by
 * [method@Pango.Context.set_font_map].
 *
 * If you are using Pango as part of a higher-level system,
 * that system may have it's own way of create a `PangoContext`.
 * For instance, the GTK toolkit has, among others,
 * gtk_widget_get_pango_context(). Use those instead.
 *
 * Return value: (transfer full): the newly allocated `PangoContext`,
 *   which should be freed with g_object_unref().
 */
PangoContext *
pango_font_map_create_context (PangoFontMap *self)
{
  PangoContext *context;

  g_return_val_if_fail (PANGO_IS_FONT_MAP (self), NULL);

  context = pango_context_new ();
  pango_context_set_font_map (context, self);

  return context;
}


/**
 * pango_font_map_load_font:
 * @self: a `PangoFontMap`
 * @context: the `PangoContext` the font will be used with
 * @desc: a `PangoFontDescription` describing the font to load
 *
 * Load the font in the fontmap that is the closest match for @desc.
 *
 * Returns: (transfer full) (nullable): the newly allocated `PangoFont`
 *   loaded, or %NULL if no font matched.
 */
PangoFont *
pango_font_map_load_font (PangoFontMap               *self,
                          PangoContext               *context,
                          const PangoFontDescription *desc)
{
  g_return_val_if_fail (PANGO_IS_FONT_MAP (self), NULL);

  return PANGO_FONT_MAP_GET_CLASS (self)->load_font (self, context, desc);
}

/**
 * pango_font_map_load_fontset:
 * @self: a `PangoFontMap`
 * @context: the `PangoContext` the font will be used with
 * @desc: a `PangoFontDescription` describing the font to load
 * @language: a `PangoLanguage` the fonts will be used for
 *
 * Load a set of fonts in the fontmap that can be used to render
 * a font matching @desc.
 *
 * Returns: (transfer full) (nullable): the newly allocated
 *   `PangoFontset` loaded, or %NULL if no font matched.
 */
PangoFontset *
pango_font_map_load_fontset (PangoFontMap               *self,
                             PangoContext               *context,
                             const PangoFontDescription *desc,
                             PangoLanguage              *language)
{
  g_return_val_if_fail (PANGO_IS_FONT_MAP (self), NULL);

  return PANGO_FONT_MAP_GET_CLASS (self)->load_fontset (self, context, desc, language);
}

/**
 * pango_font_map_get_serial:
 * @self: a `PangoFontMap`
 *
 * Returns the current serial number of @self.
 *
 * The serial number is initialized to an small number larger than zero
 * when a new fontmap is created and is increased whenever the fontmap
 * is changed. It may wrap, but will never have the value 0. Since it can
 * wrap, never compare it with "less than", always use "not equals".
 *
 * The fontmap can only be changed using backend-specific API, like changing
 * fontmap resolution.
 *
 * This can be used to automatically detect changes to a `PangoFontMap`,
 * like in `PangoContext`.
 *
 * Return value: The current serial number of @fontmap.
 */
guint
pango_font_map_get_serial (PangoFontMap *self)
{
  g_return_val_if_fail (PANGO_IS_FONT_MAP (self), 0);

  return PANGO_FONT_MAP_GET_CLASS (self)->get_serial (self);
}

/**
 * pango_font_map_get_family:
 * @self: a `PangoFontMap`
 * @name: a family name
 *
 * Gets a font family by name.
 *
 * Returns: (transfer none): the `PangoFontFamily`
 */
PangoFontFamily *
pango_font_map_get_family (PangoFontMap *self,
                           const char   *name)
{
  g_return_val_if_fail (PANGO_IS_FONT_MAP (self), NULL);

  return PANGO_FONT_MAP_GET_CLASS (self)->get_family (self, name);
}

/**
 * pango_font_map_new:
 *
 * Creates a new `PangoFontMap`.
 *
 * Returns: A newly created `PangoFontMap
 */
PangoFontMap *
pango_font_map_new (void)
{
  return g_object_new (PANGO_TYPE_FONT_MAP, NULL);
}

/**
 * pango_font_map_add_face:
 * @self: a `PangoFontMap`
 * @face: (transfer full): a `PangoFontFace`
 *
 * Adds @face to the `PangoFontMap`.
 *
 * This is most useful for creating transformed faces or aliases.
 * See [method@Pango.HbFace.new_synthetic] and [method@Pango.HbFace.new_instance].
 */
void
pango_font_map_add_face (PangoFontMap  *self,
                         PangoFontFace *face)
{
  const char *family_name;
  PangoHbFamily *family;
  const PangoFontDescription *description;

  g_return_if_fail (PANGO_IS_FONT_MAP (self));
  g_return_if_fail (PANGO_IS_HB_FACE (face) || PANGO_IS_USER_FACE (face));

  description = face->description;

  if (pango_font_description_get_set_fields (description) &
      (PANGO_FONT_MASK_VARIANT | PANGO_FONT_MASK_GRAVITY))
    g_warning ("Font description for PangoFontFace includes things that it shouldn't");

  if (!self->in_populate)
    g_ptr_array_add (self->added_faces, g_object_ref (face));

  family_name = pango_font_description_get_family (description);
  family = PANGO_HB_FAMILY (pango_font_map_get_family (self, family_name));
  if (!family)
    {
      family = pango_hb_family_new (family_name);
      pango_font_map_add_family (self, PANGO_FONT_FAMILY (family));
    }

  pango_hb_family_add_face (family, face);

  pango_font_map_changed (self);

  clear_caches (self);
}

/**
 * pango_font_map_remove_face:
 * @self: a `PangoFontMap`
 * @face: a `PangoFontFace` that belongs to @map
 *
 * Removes @face from the `PangoFontMap`.
 *
 * @face must have been added with [method@Pango.FontMap.add_face].
 */
void
pango_font_map_remove_face (PangoFontMap  *self,
                            PangoFontFace *face)
{
  PangoHbFamily *family;
  unsigned int position;

  g_return_if_fail (PANGO_IS_FONT_MAP (self));
  g_return_if_fail (PANGO_IS_HB_FACE (face) || PANGO_IS_USER_FACE (face));

  if (!g_ptr_array_find (self->added_faces, face, &position))
    return;

  family = PANGO_HB_FAMILY (pango_font_face_get_family (face));

  pango_hb_family_remove_face (family, face);

  if (family->faces->len == 0)
    pango_font_map_remove_family (self, PANGO_FONT_FAMILY (family));

  pango_font_map_changed (self);

  clear_caches (self);

  g_ptr_array_remove_index (self->added_faces, position);
}

/**
 * pango_font_map_add_file:
 * @self: a `PangoFontMap`
 * @file: font filename
 *
 * Creates a new `PangoHbFace` and adds it.
 *
 * If you need to specify an instance id or other
 * parameters, use [ctor@Pango.HbFace.new_from_file].
 */
void
pango_font_map_add_file (PangoFontMap *self,
                         const char   *file)
{
  PangoHbFace *face;

  face = pango_hb_face_new_from_file (file, 0, -1, NULL, NULL);
  pango_font_map_add_face (self, PANGO_FONT_FACE (face));
}

/**
 * pango_font_map_add_family:
 * @self: a `PangoFontMap`
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
pango_font_map_add_family (PangoFontMap    *self,
                           PangoFontFamily *family)
{
  const char *name;
  int position;

  g_return_if_fail (PANGO_IS_FONT_MAP (self));
  g_return_if_fail (PANGO_IS_HB_FAMILY (family) || PANGO_IS_GENERIC_FAMILY (family));
  g_return_if_fail (family->map == NULL);

  if (!self->in_populate)
    g_ptr_array_add (self->added_families, g_object_ref (family));

  name = family->name;

  position = 0;
  while (position < self->families->len)
    {
      PangoFontFamily *f = g_ptr_array_index (self->families, position);
      if (g_ascii_strcasecmp (name, f->name) < 0)
        break;
      position++;
    }

  pango_font_family_set_font_map (family, self);

  g_ptr_array_insert (self->families, position, family);
  g_hash_table_add (self->families_hash, family);

  if (!self->in_populate)
    {
      g_list_model_items_changed (G_LIST_MODEL (self), position, 0, 1);
      g_object_notify_by_pspec (G_OBJECT (self), properties[PROP_N_ITEMS]);
    }

  pango_font_map_changed (self);
}

/**
 * pango_font_map_remove_family:
 * @self: a `PangoFontMap`
 * @family: a `PangoFontFamily` that belongs to @self
 *
 * Removes @family from a `PangoFontMap`
 */
void
pango_font_map_remove_family (PangoFontMap    *self,
                              PangoFontFamily *family)
{
  unsigned int position;

  g_return_if_fail (PANGO_IS_FONT_MAP (self));
  g_return_if_fail (PANGO_IS_HB_FAMILY (family) || PANGO_IS_GENERIC_FAMILY (family));
  g_return_if_fail (family->map == self);

  if (!g_ptr_array_find (self->added_families, family, &position))
    return;

  g_hash_table_remove (self->families_hash, family);
  g_ptr_array_remove_index (self->families, position);

  pango_font_family_set_font_map (family, NULL);

  if (!self->in_populate)
    {
      g_list_model_items_changed (G_LIST_MODEL (self), position, 1, 0);
      g_object_notify_by_pspec (G_OBJECT (self), properties[PROP_N_ITEMS]);
    }

  pango_font_map_changed (self);

  g_ptr_array_remove_index (self->added_families, position);
}

/**
 * pango_font_map_set_resolution:
 * @self: a `PangoFontMap`
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
pango_font_map_set_resolution (PangoFontMap *self,
                               float         dpi)
{
  g_return_if_fail (PANGO_IS_FONT_MAP (self));
  g_return_if_fail (dpi > 0);

  if (self->dpi == dpi)
    return;

  self->dpi = dpi;

  clear_caches (self);
  pango_font_map_changed (self);

  g_object_notify_by_pspec (G_OBJECT (self), properties[PROP_RESOLUTION]);
}

/**
 * pango_font_map_get_resolution:
 * @self: a `PangoFontMap`
 *
 * Returns the resolution for the fontmap.
 *
 * See [method@Pango.FontMap.set_resolution].
 *
 * Return value: the resolution for the fontmap
 */
float
pango_font_map_get_resolution (PangoFontMap *self)
{
  g_return_val_if_fail (PANGO_IS_FONT_MAP (self), 0);

  return self->dpi;
}

/* }}} */
/* {{{ Default handling */

static GPrivate default_font_map = G_PRIVATE_INIT (g_object_unref); /* MT-safe */

/**
 * pango_font_map_new_default:
 *
 * Creates a new `PangoFontMap` object.
 *
 * A fontmap is used to cache information about available fonts,
 * and holds certain global parameters such as the resolution.
 * In most cases, you can use [func@Pango.FontMap.get_default]
 * instead.
 *
 * Note that the type of the returned object will depend
 * on the platform that Pango is used on. If you want to
 * explicitly create an instance of `PangoFontMap` itself
 * (and not a platform-specific subclass), see [ctor@Pango.FontMap.new].
 *
 * You can override the type of backend returned by using an
 * environment variable %PANGOCAIRO_BACKEND. Supported types,
 * based on your build, are fontconfig, win32, and coretext.
 *
 * If requested type is not available, `NULL` is returned.
 * This is only useful for testing, when at least two backends
 * are compiled in.
 *
 * Return value: (transfer full): the newly allocated `PangoFontMap`,
 *   which should be freed with g_object_unref().
 */
PangoFontMap *
pango_font_map_new_default (void)
{
  const char *backend;

  backend = getenv ("PANGOCAIRO_BACKEND");
  if (backend && !*backend)
    backend = NULL;

#if defined (HAVE_CORE_TEXT)
  if (!backend || 0 == strcmp (backend, "coretext"))
    return PANGO_FONT_MAP (pango_core_text_font_map_new ());
#endif

#if defined (HAVE_DIRECT_WRITE)
  if (!backend || 0 == strcmp (backend, "win32"))
    return PANGO_FONT_MAP (pango_direct_write_font_map_new ());
#endif

#if defined (HAVE_FONTCONFIG)
  if (!backend || 0 == strcmp (backend, "fc")
               || 0 == strcmp (backend, "fontconfig"))
    return PANGO_FONT_MAP (pango_fc_font_map_new ());
#endif

  {
    const char backends[] = ""
#if defined (HAVE_CORE_TEXT)
      " coretext"
#endif
#if defined (HAVE_DIRECT_WRITE)
      " win32"
#endif
#if defined (HAVE_FNTCONFIG)
      " fontconfig"
#endif
      ;

    g_critical ("Unknown PANGOCAIRO_BACKEND value.\n"
                "Available backends are:%s", backends);
  }

  return NULL;
}

/**
 * pango_font_map_get_default:
 *
 * Gets a default `PangoFontMap`.
 *
 * Note that the type of the returned object will depend on the
 * platform that Pango is used on.
 *
 * The default fontmap can be changed by using
 * [method@Pango.FontMap.set_default].
 *
 * Note that the default fontmap is per-thread. Each thread gets
 * its own default fontmap. In this way, Pango can be used safely
 * from multiple threads.
 *
 * Return value: (transfer none): the default fontmap
 *  for the current thread. This object is owned by Pango and must
 *  not be freed.
 */
PangoFontMap *
pango_font_map_get_default (void)
{
  PangoFontMap *fontmap = g_private_get (&default_font_map);

  if (G_UNLIKELY (!fontmap))
    {
      fontmap = pango_font_map_new_default ();
      g_private_replace (&default_font_map, fontmap);
    }

  return fontmap;
}

/**
 * pango_font_map_set_default:
 * @fontmap: (nullable): The new default font map
 *
 * Sets a default `PangoFontMap`.
 *
 * The old default font map is unreffed and the new font map referenced.
 *
 * Note that the default fontmap is per-thread.
 * This function only changes the default fontmap for
 * the current thread. Default fontmaps of existing threads
 * are not changed. Default fontmaps of any new threads will
 * still be created using [ctor@Pango.FontMap.new_default].
 *
 * A value of %NULL for @fontmap will cause the current default
 * font map to be released and a new default font map to be created
 * on demand, using [ctor@Pango.FontMap.new_default].
 */
void
pango_font_map_set_default (PangoFontMap *fontmap)
{
  g_return_if_fail (fontmap == NULL || PANGO_IS_FONT_MAP (fontmap));

  if (fontmap)
    g_object_ref (fontmap);

  g_private_replace (&default_font_map, fontmap);
}

/* }}} */

/* vim:set foldmethod=marker expandtab: */
