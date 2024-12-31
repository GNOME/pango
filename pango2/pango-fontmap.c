/* Pango2
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
#include "pango-context-private.h"

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
 * Pango2FontMap:
 *
 * `Pango2FontMap` is the base class for font enumeration.
 * It also handles caching and lookup of faces and fonts.
 * To obtain fonts from a `Pango2FontMap`, use [method@Pango2.FontMap.load_font]
 * or [method@Pango2.FontMap.load_fontset].
 *
 * Subclasses populate the fontmap using backend-specific APIs
 * to enumerate the available fonts on the sytem, but it is
 * also possible to create an instance of `Pango2FontMap` and
 * populate it manually using [method@Pango2.FontMap.add_file]
 * and [method@Pango2.FontMap.add_face].
 *
 * Fontmaps can be combined using [method@Pango2.FontMap.set_fallback].
 * This can be useful to add custom fonts to the default fonts
 * without making them available to every user of the default
 * fontmap.
 *
 * Note that to be fully functional, a fontmap needs to provide
 * generic families for monospace and sans-serif. These can
 * be added using [method@Pango2.FontMap.add_family] and
 * [ctor@Pango2.GenericFamily.new].
 *
 * `Pango2FontMap` implements the [iface@Gio.ListModel] interface,
 * to provide a list of font families.
 */


/* The central api is load_fontset, which takes a font description
 * and language, finds the matching faces, and creates a Pango2Fontset
 * for them. To speed this operation up, the font map maintains a
 * cache of fontsets (in the form of a GQueue) and has a hash table
 * for looking up existing fontsets.
 *
 * The Pango2FontsetCached object is the fontset subclass that is used
 * here, and it contains the necessary data for the cashing and hashing.
 * Pango2FontsetCached also caches the character-to-font mapping that is
 * used when itemizing text.
 */


/* {{{ GListModel implementation */

static GType
pango2_font_map_get_item_type (GListModel *list)
{
  return PANGO2_TYPE_FONT_FAMILY;
}

static guint
pango2_font_map_get_n_items (GListModel *list)
{
  Pango2FontMap *self = PANGO2_FONT_MAP (list);

  if (self->fallback)
    return self->families->len + g_list_model_get_n_items (G_LIST_MODEL (self->fallback));
  else
    return self->families->len;
}

static gpointer
pango2_font_map_get_item (GListModel *list,
                          guint       position)
{
  Pango2FontMap *self = PANGO2_FONT_MAP (list);

  if (position < self->families->len)
    return g_object_ref (g_ptr_array_index (self->families, position));
  else if (self->fallback)
    return g_list_model_get_item (G_LIST_MODEL (self->fallback), position - self->families->len);

  return NULL;
}

static void
pango2_font_map_list_model_init (GListModelInterface *iface)
{
  iface->get_item_type = pango2_font_map_get_item_type;
  iface->get_n_items = pango2_font_map_get_n_items;
  iface->get_item = pango2_font_map_get_item;
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
pango2_fontset_cached_hash (const Pango2FontsetCached *fontset)
{
  guint32 hash = FNV1_32_INIT;

  if (fontset->ctm)
    hash = hash_bytes_fnv ((unsigned char *)(fontset->ctm), sizeof (double) * 4, hash);

  return (hash ^
          GPOINTER_TO_UINT (fontset->language) ^
#ifdef HAVE_CAIRO
          cairo_font_options_hash (fontset->font_options) ^
#endif
          pango2_font_description_hash (fontset->description));
}

static gboolean
pango2_fontset_cached_equal (const Pango2FontsetCached *a,
                             const Pango2FontsetCached *b)
{
  return a->language == b->language &&
#ifdef HAVE_CAIRO
         cairo_font_options_equal (a->font_options, b->font_options) &&
#endif
         (a->ctm == b->ctm ||
          (a->ctm && b->ctm && memcmp (a->ctm, b->ctm, 4 * sizeof (double)) == 0)) &&
         pango2_font_description_equal (a->description, b->description);
}

static void
pango2_fontset_cache (Pango2FontsetCached *fontset,
                      Pango2FontMap       *self)
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
pango2_family_hash (const Pango2FontFamily *key)
{
  const char *p;
  guint32 h = 5381;

  for (p = (const char *)key->name; *p != '\0'; p++)
    h = (h << 5) + h + g_ascii_tolower (*p);

  return h;
}

static gboolean
pango2_family_equal (const Pango2FontFamily *a,
                     const Pango2FontFamily *b)
{
  return g_ascii_strcasecmp (a->name, b->name) == 0;
}

static Pango2FontFamily *
find_family (Pango2FontMap *self,
             const char    *family_name)
{
  Pango2FontFamily lookup;
  Pango2FontFamily *family;

  lookup.name = (char *)family_name;

  family = PANGO2_FONT_FAMILY (g_hash_table_lookup (self->families_hash, &lookup));

  return family;
}

static void
clear_caches (Pango2FontMap *self)
{
  g_hash_table_remove_all (self->fontsets);
  g_queue_init (&self->fontset_cache);
}

static void
add_style_variation (Pango2HbFamily *family,
                     Pango2HbFace   *face,
                     Pango2Style     style,
                     Pango2Weight    weight)
{
  Pango2Matrix italic_matrix = { 1, 0.2, 0, 1, 0, 0 };
  Pango2FontDescription *desc;
  Pango2HbFaceBuilder *builder;
  Pango2HbFace *variation;
  struct {
    Pango2Style style;
    Pango2Weight weight;
    const char *name;
  } names[] = {
    { PANGO2_STYLE_ITALIC, PANGO2_WEIGHT_BOLD, "Bold Italic" },
    { PANGO2_STYLE_ITALIC, PANGO2_WEIGHT_NORMAL, "Italic" },
    { PANGO2_STYLE_OBLIQUE, PANGO2_WEIGHT_BOLD, "Bold Oblique" },
    { PANGO2_STYLE_OBLIQUE, PANGO2_WEIGHT_NORMAL, "Oblique" },
    { PANGO2_STYLE_NORMAL, PANGO2_WEIGHT_BOLD, "Bold" },
    { PANGO2_STYLE_NORMAL, PANGO2_WEIGHT_NORMAL, "Regular" },
  };

  desc = pango2_font_description_new ();
  pango2_font_description_set_family (desc, pango2_font_family_get_name (PANGO2_FONT_FAMILY (family)));
  pango2_font_description_set_style (desc, style);
  pango2_font_description_set_weight (desc, weight);

  builder = pango2_hb_face_builder_new (face);
  pango2_hb_face_builder_set_transform (builder, style == PANGO2_STYLE_ITALIC ? &italic_matrix : NULL);
  pango2_hb_face_builder_set_embolden (builder, weight == PANGO2_WEIGHT_BOLD);
  pango2_hb_face_builder_set_description (builder, desc);
  for (int i = 0; i < G_N_ELEMENTS (names); i++)
    {
      if (names[i].style == style && names[i].weight == weight)
        {
          pango2_hb_face_builder_set_name (builder, names[i].name);
          break;
        }
    }

  variation = pango2_hb_face_builder_get_face (builder);

  pango2_hb_family_add_face (family, PANGO2_FONT_FACE (variation));

  pango2_hb_face_builder_free (builder);

  pango2_font_description_free (desc);
}

static void
synthesize_bold_and_italic_faces (Pango2FontMap *map)
{
  for (int i = 0; i < map->families->len; i++)
    {
      Pango2FontFamily *family = g_ptr_array_index (map->families, i);
      Pango2FontFace *regular_face = NULL;
      int regular_dist = G_MAXINT;
      int bold_dist = G_MAXINT;
      gboolean has_italic = FALSE;
      gboolean has_bold = FALSE;
      gboolean has_bold_italic = FALSE;

      if (PANGO2_IS_GENERIC_FAMILY (family))
        continue;

      for (int j = 0; j < g_list_model_get_n_items (G_LIST_MODEL (family)); j++)
        {
          Pango2FontFace *face = g_list_model_get_item (G_LIST_MODEL (family), j);
          int weight;
          Pango2Style style;
          int dist;

          if (!PANGO2_IS_HB_FACE (face))
            continue;

          weight = pango2_font_description_get_weight (face->description);
          style = pango2_font_description_get_style (face->description);

          if (style == PANGO2_STYLE_NORMAL)
            {
              dist = abs (weight - (int)PANGO2_WEIGHT_NORMAL);

              if (dist < regular_dist)
                {
                  regular_dist = dist;
                  if (dist < 150)
                    regular_face = face;
                }

              dist = abs (weight - (int)PANGO2_WEIGHT_BOLD);

              if (dist < bold_dist)
                {
                  bold_dist = dist;
                  has_bold = dist < 150;
                }
            }
          else
            {
              if (weight < PANGO2_WEIGHT_SEMIBOLD)
                has_italic = TRUE;
              else
                has_bold_italic = TRUE;
            }

          g_object_unref (face);
        }

      if (regular_face)
        {
          if (!has_italic)
            add_style_variation (PANGO2_HB_FAMILY (family),
                                 PANGO2_HB_FACE (regular_face),
                                 PANGO2_STYLE_ITALIC,
                                 PANGO2_WEIGHT_NORMAL);

          if (!has_bold)
            add_style_variation (PANGO2_HB_FAMILY (family),
                                 PANGO2_HB_FACE (regular_face),
                                 PANGO2_STYLE_NORMAL,
                                 PANGO2_WEIGHT_BOLD);

          if (!has_bold_italic)
            add_style_variation (PANGO2_HB_FAMILY (family),
                                 PANGO2_HB_FACE (regular_face),
                                 PANGO2_STYLE_ITALIC,
                                 PANGO2_WEIGHT_BOLD);
        }
    }
}

/* }}} */
/* {{{ Pango2FontMap implementation */

enum {
  PROP_RESOLUTION = 1,
  PROP_FALLBACK,
  PROP_ITEM_TYPE,
  PROP_N_ITEMS,
  N_PROPERTIES
};

static GParamSpec *properties[N_PROPERTIES] = { NULL };

G_DEFINE_TYPE_WITH_CODE (Pango2FontMap, pango2_font_map, G_TYPE_OBJECT,
                         G_IMPLEMENT_INTERFACE (G_TYPE_LIST_MODEL, pango2_font_map_list_model_init))


static void
pango2_font_map_init (Pango2FontMap *self)
{
  self->added_faces = g_ptr_array_new_with_free_func (g_object_unref);
  self->added_families = g_ptr_array_new_with_free_func (g_object_unref);
  self->families = g_ptr_array_new_with_free_func (g_object_unref);
  self->families_hash = g_hash_table_new_full ((GHashFunc) pango2_family_hash,
                                               (GEqualFunc) pango2_family_equal,
                                               NULL,
                                               NULL);
  self->fontsets = g_hash_table_new_full ((GHashFunc) pango2_fontset_cached_hash,
                                          (GEqualFunc) pango2_fontset_cached_equal,
                                          NULL,
                                          (GDestroyNotify) g_object_unref);
  g_queue_init (&self->fontset_cache);
  self->dpi = 96.;
}

static void
pango2_font_map_finalize (GObject *object)
{
  Pango2FontMap *self = PANGO2_FONT_MAP (object);

  g_clear_object (&self->fallback);
  g_ptr_array_unref (self->added_faces);
  g_ptr_array_unref (self->added_families);
  g_hash_table_unref (self->families_hash);
  g_ptr_array_unref (self->families);
  g_hash_table_unref (self->fontsets);

  G_OBJECT_CLASS (pango2_font_map_parent_class)->finalize (object);
}

static void
pango2_font_map_set_property (GObject      *object,
                              guint         property_id,
                              const GValue *value,
                              GParamSpec   *pspec)
{
  Pango2FontMap *map = PANGO2_FONT_MAP (object);

  switch (property_id)
    {
    case PROP_RESOLUTION:
      pango2_font_map_set_resolution (map, g_value_get_float (value));
      break;

    case PROP_FALLBACK:
      pango2_font_map_set_fallback (map, g_value_get_object (value));
      break;

    default:
      G_OBJECT_WARN_INVALID_PROPERTY_ID (object, property_id, pspec);
    }
}

static void
pango2_font_map_get_property (GObject    *object,
                              guint       property_id,
                              GValue     *value,
                              GParamSpec *pspec)
{
  Pango2FontMap *map = PANGO2_FONT_MAP (object);

  switch (property_id)
    {
    case PROP_RESOLUTION:
      g_value_set_float (value, map->dpi);
      break;

    case PROP_FALLBACK:
      g_value_set_object (value, map->fallback);
      break;

    case PROP_ITEM_TYPE:
      g_value_set_gtype (value, PANGO2_TYPE_FONT_FAMILY);
      break;

    case PROP_N_ITEMS:
      g_value_set_uint (value, pango2_font_map_get_n_items (G_LIST_MODEL (object)));
      break;

    default:
      G_OBJECT_WARN_INVALID_PROPERTY_ID (object, property_id, pspec);
    }
}

/* Load a font from the first matching family */
static Pango2Font *
pango2_font_map_default_load_font (Pango2FontMap               *self,
                                   Pango2Context               *context,
                                   const Pango2FontDescription *description)
{
  Pango2FontsetCached *fontset;
  Pango2Language *language;
  Pango2Font *font = NULL;

  if (self->families->len == 0)
    return NULL;

  if (context)
    language = pango2_context_get_language (context);
  else
    language = NULL;

  fontset = (Pango2FontsetCached *)pango2_font_map_load_fontset (self, context, description, language);
  if (pango2_fontset_cached_size (fontset) > 0)
    font = pango2_fontset_cached_get_first_font (fontset);
  g_object_unref (fontset);

  return font;
}

/* Add one font for each family we find */
static Pango2Fontset *
pango2_font_map_default_load_fontset (Pango2FontMap               *self,
                                      Pango2Context               *context,
                                      const Pango2FontDescription *description,
                                      Pango2Language              *language)
{
  Pango2FontsetCached lookup;
  Pango2FontsetCached *fontset;
  const Pango2Matrix *ctm;
  const char *family_name;
  char **families;
  Pango2FontDescription *copy;
  Pango2FontFamily *family;
  Pango2FontFace *face;
  gboolean has_generic = FALSE;
  Pango2Fontset *fallback;
  gint64 before G_GNUC_UNUSED;

  before = PANGO2_TRACE_CURRENT_TIME;

  if (!language)
    language = pango2_context_get_language (context);

  family_name = pango2_font_description_get_family (description);
  ctm = pango2_context_get_matrix (context);

  lookup.language = language;
  lookup.description = (Pango2FontDescription *)description;
  lookup.ctm = (Pango2Matrix *)ctm;

#ifdef HAVE_CAIRO
  lookup.font_options = (cairo_font_options_t *)pango2_cairo_context_get_merged_font_options (context);
#endif
  fontset = g_hash_table_lookup (self->fontsets, &lookup);

  if (fontset)
    goto done;

  fontset = pango2_fontset_cached_new (description, language, self->dpi, ctm);
#ifdef HAVE_CAIRO
  fontset->font_options = cairo_font_options_copy (pango2_cairo_context_get_merged_font_options (context));
#endif

  if (self->families->len == 0)
    {
      if (self->fallback)
        goto add_fallback;
      else
        {
          g_warning ("Font map contains no fonts!!!!");
          goto done_no_cache;
        }
    }

  families = g_strsplit (family_name ? family_name : "", ",", -1);

  /* Unset gravity and variant since Pango2HbFace does not have these fields */
  copy = pango2_font_description_copy_static (description);
  pango2_font_description_unset_fields (copy, PANGO2_FONT_MASK_VARIATIONS |
                                              PANGO2_FONT_MASK_GRAVITY |
                                              PANGO2_FONT_MASK_VARIANT);

  for (int i = 0; families[i]; i++)
    {
      family = find_family (self, families[i]);
      if (!family)
        continue;

      if (PANGO2_IS_GENERIC_FAMILY (family))
        {
          pango2_fontset_cached_add_family (fontset, PANGO2_GENERIC_FAMILY (family));
          has_generic = TRUE;
        }
      else
        {
          face = pango2_hb_family_find_face (PANGO2_HB_FAMILY (family), copy, language, 0);
          if (face)
            pango2_fontset_cached_add_face (fontset, face);
        }
    }

  g_strfreev (families);

  pango2_font_description_free (copy);

  /* Returning an empty fontset leads to bad outcomes.
   *
   * We always include a generic family in order
   * to produce fontsets with good coverage.
   *
   * If we have a fallback fontmap, this is where we bring
   * it in and just add its results to ours.
   */
  if (!has_generic)
    {
      family = find_family (self, "sans-serif");
      if (PANGO2_IS_GENERIC_FAMILY (family))
        pango2_fontset_cached_add_family (fontset, PANGO2_GENERIC_FAMILY (family));
      else if (self->fallback)
        {
add_fallback:
          fallback = pango2_font_map_load_fontset (self->fallback, context, description, language);
          pango2_fontset_cached_append (fontset, PANGO2_FONTSET_CACHED (fallback));
          g_object_unref (fallback);
        }
    }

  g_hash_table_add (self->fontsets, fontset);

done:
  pango2_fontset_cache (fontset, self);

  if (pango2_fontset_cached_size (fontset) == 0)
    {
      char *s = pango2_font_description_to_string (description);
      g_warning ("All font fallbacks failed for \"%s\", in %s !!!!", s, pango2_language_to_string (language));
      g_free (s);
    }

done_no_cache:
  pango2_trace_mark (before, "pango2_fontmap_load_fontset", "%s", family_name);

  return g_object_ref (PANGO2_FONTSET (fontset));
}

static Pango2Font *
pango2_font_map_default_reload_font (Pango2FontMap *self,
                                     Pango2Font    *font,
                                     double         scale,
                                     Pango2Context *context,
                                     const char    *variations)
{
  Pango2FontDescription *desc;
  Pango2Context *freeme = NULL;
  Pango2Font *scaled;

  desc = pango2_font_describe_with_absolute_size (font);

  if (scale != 1.0)
    {
      int size = pango2_font_description_get_size (desc);
      pango2_font_description_set_absolute_size (desc, size * scale);
    }

  if (variations)
    pango2_font_description_set_variations_static (desc, variations);

  if (!context)
    {
      freeme = context = pango2_context_new_with_font_map (self);
#ifdef HAVE_CAIRO
      pango2_cairo_context_set_font_options (context,
                                             pango2_cairo_font_get_font_options (font));
#endif
    }

  scaled = pango2_font_map_load_font (self, context, desc);

  g_clear_object (&freeme);

  pango2_font_description_free (desc);

  return scaled;
}

static Pango2FontFamily *
pango2_font_map_default_get_family (Pango2FontMap *self,
                                    const char    *name)
{
  return PANGO2_FONT_FAMILY (find_family (self, name));
}

static void
pango2_font_map_default_populate (Pango2FontMap *self)
{
}

static guint
pango2_font_map_default_get_serial (Pango2FontMap *self)
{
  return self->serial;
}

static void
pango2_font_map_default_changed (Pango2FontMap *self)
{
  self->serial++;
  if (self->serial == 0)
    self->serial++;
}

static void
pango2_font_map_class_init (Pango2FontMapClass *class)
{
  GObjectClass *object_class = G_OBJECT_CLASS (class);

  object_class->finalize = pango2_font_map_finalize;
  object_class->set_property = pango2_font_map_set_property;
  object_class->get_property = pango2_font_map_get_property;

  class->load_font = pango2_font_map_default_load_font;
  class->load_fontset = pango2_font_map_default_load_fontset;
  class->reload_font = pango2_font_map_default_reload_font;
  class->get_serial = pango2_font_map_default_get_serial;
  class->changed = pango2_font_map_default_changed;
  class->get_family = pango2_font_map_default_get_family;
  class->populate = pango2_font_map_default_populate;

  /**
   * Pango2FontMap:resolution: (attributes org.gtk.Property.get=pango2_font_map_get_resolution org.gtk.Property.set=pango2_font_map_set_resolution)
   *
   * The resolution for the fontmap.
   *
   * This is a scale factor between points specified in a
   * `Pango2FontDescription` and Cairo units. The default value
   * is 96, meaning that a 10 point font will be 13 units high.
   * (10 * 96. / 72. = 13.3).
   */
  properties[PROP_RESOLUTION] =
      g_param_spec_float ("resolution", NULL, NULL,
                          0, G_MAXFLOAT, 96.0,
                          G_PARAM_READWRITE | G_PARAM_STATIC_STRINGS);

  /**
   * Pango2FontMap:fallback: (attributes org.gtk.Property.get=pango2_font_map_get_fallback org.gtk.Property.set=pango2_font_map_set_fallback)
   *
   * The fallback fontmap is used to look up fonts that
   * this map does not have itself.
   */
  properties[PROP_FALLBACK] =
      g_param_spec_object ("fallback", NULL, NULL,
                           PANGO2_TYPE_FONT_MAP,
                           G_PARAM_READWRITE | G_PARAM_STATIC_STRINGS);

  /**
   * Pango2FontMap:item-type:
   *
   * The type of items contained in this list.
   */
  properties[PROP_ITEM_TYPE] =
      g_param_spec_gtype ("item-type", NULL, NULL,
                          G_TYPE_OBJECT,
                          G_PARAM_READABLE | G_PARAM_STATIC_STRINGS);

  /**
   * Pango2FontMap:n-items:
   *
   * The number of items contained in this list.
   */
  properties[PROP_N_ITEMS] =
      g_param_spec_uint ("n-items", NULL, NULL,
                         0, G_MAXUINT, 0,
                         G_PARAM_READABLE | G_PARAM_STATIC_STRINGS);

  g_object_class_install_properties (object_class, N_PROPERTIES, properties);
}

/* }}} */
/* {{{ Private API */

/*< private >
 * pango2_font_map_repopulate:
 * @self: a `Pango2FontMap`
 * @add_synthetic: if `TRUE`, missing bold and italic faces will be synthesized
 *
 * Clear all cached information and repopulate the fontmap with
 * families and faces.
 *
 * Subclasses should call this when their configuration changes
 * in a way that requires recreating families and faces.
 */
void
pango2_font_map_repopulate (Pango2FontMap *self,
                            gboolean       add_synthetic)
{
  int removed, added;

  clear_caches (self);

  removed = self->families->len;

  g_hash_table_remove_all (self->families_hash);
  g_ptr_array_set_size (self->families, 0);

  self->in_populate = TRUE;

  PANGO2_FONT_MAP_GET_CLASS (self)->populate (self);

  if (add_synthetic)
    synthesize_bold_and_italic_faces (self);

  for (int i = 0; i < self->added_faces->len; i++)
    {
      Pango2FontFace *face = g_ptr_array_index (self->added_faces, i);
      pango2_font_map_add_face (self, face);
    }

  for (int i = 0; i < self->added_families->len; i++)
    {
      Pango2FontFamily *family = PANGO2_FONT_FAMILY (g_ptr_array_index (self->added_families, i));
      pango2_font_map_add_family (self, family);
    }

  self->in_populate = FALSE;

  added = self->families->len;

  g_list_model_items_changed (G_LIST_MODEL (self), 0, removed, added);
  g_object_notify_by_pspec (G_OBJECT (self), properties[PROP_N_ITEMS]);
  pango2_font_map_changed (self);
}

/*< private >
 * pango2_font_map_changed:
 * @self: a `Pango2FontMap`
 *
 * Forces a change in the context, which will cause any `Pango2Context`
 * using this fontmap to change.
 *
 * This function is only useful when implementing a new backend
 * for Pango2, something applications won't do. Backends should
 * call this function if they have attached extra data to the
 * context and such data is changed.
 */
void
pango2_font_map_changed (Pango2FontMap *self)
{
  g_return_if_fail (PANGO2_IS_FONT_MAP (self));

  PANGO2_FONT_MAP_GET_CLASS (self)->changed (self);
}

/* }}} */
/* {{{ Public API */

/**
 * pango2_font_map_load_font:
 * @self: a `Pango2FontMap`
 * @context: the `Pango2Context` the font will be used with
 * @desc: a `Pango2FontDescription` describing the font to load
 *
 * Load the font in the fontmap that is the closest match for
 * a font description.
 *
 * Returns: (transfer full) (nullable): the `Pango2Font` that
 *   was found
 */
Pango2Font *
pango2_font_map_load_font (Pango2FontMap               *self,
                           Pango2Context               *context,
                           const Pango2FontDescription *desc)
{
  g_return_val_if_fail (PANGO2_IS_FONT_MAP (self), NULL);
  g_return_val_if_fail (PANGO2_IS_CONTEXT (context), NULL);
  g_return_val_if_fail (desc != NULL, NULL);

  return PANGO2_FONT_MAP_GET_CLASS (self)->load_font (self, context, desc);
}

/**
 * pango2_font_map_load_fontset:
 * @self: a `Pango2FontMap`
 * @context: the `Pango2Context` the font will be used with
 * @desc: a `Pango2FontDescription` describing the font to load
 * @language: (nullable): a `Pango2Language` the fonts will be used for,
 *    or `NULL` to use the language of @context
 *
 * Load a set of fonts in the fontmap that can be used to render
 * a font matching a font description.
 *
 * Returns: (transfer full) (nullable): the `Pango2Fontset` containing
 *   the fonts that were found
 */
Pango2Fontset *
pango2_font_map_load_fontset (Pango2FontMap               *self,
                              Pango2Context               *context,
                              const Pango2FontDescription *desc,
                              Pango2Language              *language)
{
  g_return_val_if_fail (PANGO2_IS_FONT_MAP (self), NULL);
  g_return_val_if_fail (PANGO2_IS_CONTEXT (context), NULL);
  g_return_val_if_fail (desc != NULL, NULL);

  return PANGO2_FONT_MAP_GET_CLASS (self)->load_fontset (self, context, desc, language);
}

/**
 * pango2_font_map_reload_font:
 * @self: a `Pango2FontMap`
 * @font: a font in @fontmap
 * @scale: the scale factor to apply
 * @context: (nullable): a `Pango2Context`
 * @variations: (nullable): font variations to use
 *
 * Returns a modified version of a font.
 *
 * The new font is like @font, except that its size is multiplied
 * by @scale, its backend-dependent configuration (e.g. cairo font
 * options) is replaced by the one in @context, and its variations
 * are replaced by @variations.
 *
 * Returns: (transfer full): the modified font
 */
Pango2Font *
pango2_font_map_reload_font (Pango2FontMap *self,
                             Pango2Font    *font,
                             double         scale,
                             Pango2Context *context,
                             const char    *variations)
{
  g_return_val_if_fail (PANGO2_IS_FONT_MAP (self), NULL);
  g_return_val_if_fail (PANGO2_IS_FONT (font), NULL);
  g_return_val_if_fail (scale > 0, NULL);
  g_return_val_if_fail (context == NULL || PANGO2_IS_CONTEXT (context), NULL);

  return PANGO2_FONT_MAP_GET_CLASS (self)->reload_font (self, font, scale, context, variations);
}

/**
 * pango2_font_map_get_serial:
 * @self: a `Pango2FontMap`
 *
 * Returns the current serial number of the fontmap.
 *
 * The serial number is initialized to an small number larger than zero
 * when a new fontmap is created and is increased whenever the fontmap
 * is changed. It may wrap, but will never have the value 0. Since it can
 * wrap, never compare it with "less than", always use "not equals".
 *
 * The fontmap can only be changed using backend-specific API, like changing
 * fontmap resolution.
 *
 * This can be used to automatically detect changes to a `Pango2FontMap`,
 * like in `Pango2Context`.
 *
 * Return value: The current serial number of @fontmap.
 */
guint
pango2_font_map_get_serial (Pango2FontMap *self)
{
  g_return_val_if_fail (PANGO2_IS_FONT_MAP (self), 0);

  return PANGO2_FONT_MAP_GET_CLASS (self)->get_serial (self);
}

/**
 * pango2_font_map_get_family:
 * @self: a `Pango2FontMap`
 * @name: a family name
 *
 * Gets a font family by name.
 *
 * Returns: (transfer none): the `Pango2FontFamily`
 */
Pango2FontFamily *
pango2_font_map_get_family (Pango2FontMap *self,
                            const char    *name)
{
  g_return_val_if_fail (PANGO2_IS_FONT_MAP (self), NULL);

  return PANGO2_FONT_MAP_GET_CLASS (self)->get_family (self, name);
}

/**
 * pango2_font_map_new:
 *
 * Creates a new `Pango2FontMap`.
 *
 * A fontmap is used to cache information about available fonts,
 * and holds certain global parameters such as the resolution.
 *
 * Returns: A newly created `Pango2FontMap
 */
Pango2FontMap *
pango2_font_map_new (void)
{
  return g_object_new (PANGO2_TYPE_FONT_MAP, NULL);
}

/**
 * pango2_font_map_add_face:
 * @self: a `Pango2FontMap`
 * @face: (transfer full): a `Pango2FontFace`
 *
 * Adds a face to the fontmap.
 *
 * This is most useful for creating transformed faces or aliases.
 * See [struct@Pango2.HbFaceBuilder].
 */
void
pango2_font_map_add_face (Pango2FontMap  *self,
                          Pango2FontFace *face)
{
  const char *family_name;
  Pango2HbFamily *family;
  const Pango2FontDescription *description;

  g_return_if_fail (PANGO2_IS_FONT_MAP (self));
  g_return_if_fail (PANGO2_IS_HB_FACE (face) || PANGO2_IS_USER_FACE (face));

  description = face->description;

  if (pango2_font_description_get_set_fields (description) & PANGO2_FONT_MASK_GRAVITY)
    g_warning ("Font description for Pango2FontFace includes things that it shouldn't");

  if (!self->in_populate)
    g_ptr_array_add (self->added_faces, g_object_ref (face));

  family_name = pango2_font_description_get_family (description);
  family = PANGO2_HB_FAMILY (pango2_font_map_get_family (self, family_name));
  if (!family)
    {
      family = pango2_hb_family_new (family_name);
      pango2_font_map_add_family (self, PANGO2_FONT_FAMILY (family));
    }

  pango2_hb_family_add_face (family, face);

  pango2_font_map_changed (self);

  clear_caches (self);
}

/**
 * pango2_font_map_remove_face:
 * @self: a `Pango2FontMap`
 * @face: a `Pango2FontFace` that belongs to @map
 *
 * Removes @face from the fontmap.
 *
 * @face must have been added with [method@Pango2.FontMap.add_face].
 */
void
pango2_font_map_remove_face (Pango2FontMap  *self,
                             Pango2FontFace *face)
{
  Pango2HbFamily *family;
  unsigned int position;

  g_return_if_fail (PANGO2_IS_FONT_MAP (self));
  g_return_if_fail (PANGO2_IS_HB_FACE (face) || PANGO2_IS_USER_FACE (face));

  if (!g_ptr_array_find (self->added_faces, face, &position))
    return;

  family = PANGO2_HB_FAMILY (pango2_font_face_get_family (face));

  pango2_hb_family_remove_face (family, face);

  if (family->faces->len == 0)
    pango2_font_map_remove_family (self, PANGO2_FONT_FAMILY (family));

  pango2_font_map_changed (self);

  clear_caches (self);

  g_ptr_array_remove_index (self->added_faces, position);
}

/**
 * pango2_font_map_add_file:
 * @self: a `Pango2FontMap`
 * @file: (type filename): font filename
 *
 * Creates a new `Pango2HbFace` and adds it.
 *
 * If you need to specify an instance id or other
 * parameters, use [ctor@Pango2.HbFace.new_from_file].
 */
void
pango2_font_map_add_file (Pango2FontMap *self,
                          const char    *file)
{
  Pango2HbFace *face;

  face = pango2_hb_face_new_from_file (file, 0, -1, NULL, NULL);
  pango2_font_map_add_face (self, PANGO2_FONT_FACE (face));
}

/**
 * pango2_font_map_add_family:
 * @self: a `Pango2FontMap`
 * @family: (transfer full): a `Pango2FontFamily`
 *
 * Adds a family to the `Pango2FontMap`.
 *
 * The fontmap must not contain a family with the
 * same name as @family yet.
 *
 * This is mostly useful for adding generic families
 * using [ctor@Pango2.GenericFamily.new].
 */
void
pango2_font_map_add_family (Pango2FontMap    *self,
                            Pango2FontFamily *family)
{
  const char *name;
  int position;

  g_return_if_fail (PANGO2_IS_FONT_MAP (self));
  g_return_if_fail (PANGO2_IS_HB_FAMILY (family) || PANGO2_IS_GENERIC_FAMILY (family));
  g_return_if_fail (family->map == NULL);

  if (!self->in_populate)
    g_ptr_array_add (self->added_families, g_object_ref (family));

  name = family->name;

  position = 0;
  while (position < self->families->len)
    {
      Pango2FontFamily *f = g_ptr_array_index (self->families, position);
      if (g_ascii_strcasecmp (name, f->name) < 0)
        break;
      position++;
    }

  pango2_font_family_set_font_map (family, self);

  g_ptr_array_insert (self->families, position, family);
  g_hash_table_add (self->families_hash, family);

  if (!self->in_populate)
    {
      g_list_model_items_changed (G_LIST_MODEL (self), position, 0, 1);
      g_object_notify_by_pspec (G_OBJECT (self), properties[PROP_N_ITEMS]);
    }

  pango2_font_map_changed (self);
}

/**
 * pango2_font_map_remove_family:
 * @self: a `Pango2FontMap`
 * @family: a `Pango2FontFamily` that belongs to @self
 *
 * Removes a family from the fontmap.
 */
void
pango2_font_map_remove_family (Pango2FontMap    *self,
                               Pango2FontFamily *family)
{
  unsigned int position;

  g_return_if_fail (PANGO2_IS_FONT_MAP (self));
  g_return_if_fail (PANGO2_IS_HB_FAMILY (family) || PANGO2_IS_GENERIC_FAMILY (family));
  g_return_if_fail (family->map == self);

  if (!g_ptr_array_find (self->added_families, family, &position))
    return;

  g_hash_table_remove (self->families_hash, family);
  g_ptr_array_remove_index (self->families, position);

  pango2_font_family_set_font_map (family, NULL);

  if (!self->in_populate)
    {
      g_list_model_items_changed (G_LIST_MODEL (self), position, 1, 0);
      g_object_notify_by_pspec (G_OBJECT (self), properties[PROP_N_ITEMS]);
    }

  pango2_font_map_changed (self);

  g_ptr_array_remove_index (self->added_families, position);
}

/**
 * pango2_font_map_set_fallback:
 * @self: a `Pango2FontMap`
 * @fallback: (nullable): the `Pango2FontMap` to use as fallback
 *
 * Sets the fontmap to use as fallback when a font isn't found.
 *
 * This can be used to make a custom font available only via a
 * special fontmap, while still having all the regular fonts
 * from the fallback fontmap.
 *
 * Note that families are *not* merged. If you are iterating
 * the families, you will first get all the families in @self,
 * followed by all the families in the @fallback.
 */
void
pango2_font_map_set_fallback (Pango2FontMap *self,
                              Pango2FontMap *fallback)
{
  guint removed, added;

  g_return_if_fail (PANGO2_IS_FONT_MAP (self));
  g_return_if_fail (fallback == NULL || PANGO2_IS_FONT_MAP (fallback));

  removed = g_list_model_get_n_items (G_LIST_MODEL (self));

  if (!g_set_object (&self->fallback, fallback))
    return;

  added = g_list_model_get_n_items (G_LIST_MODEL (self));

  clear_caches (self);

  g_list_model_items_changed (G_LIST_MODEL (self), 0, removed, added);
  g_object_notify_by_pspec (G_OBJECT (self), properties[PROP_FALLBACK]);
  g_object_notify_by_pspec (G_OBJECT (self), properties[PROP_N_ITEMS]);
}

/**
 * pango2_font_map_get_fallback:
 * @self: a `Pango2FontMap`
 *
 * Returns the fallback fontmap of the fontmap.
 *
 * See [method@Pango2.FontMap.set_fallback].
 *
 * Returns: (nullable) (transfer none): the fallback fontmap
 */
Pango2FontMap *
pango2_font_map_get_fallback (Pango2FontMap *self)
{
  g_return_val_if_fail (PANGO2_IS_FONT_MAP (self), NULL);

  return self->fallback;
}

/**
 * pango2_font_map_set_resolution:
 * @self: a `Pango2FontMap`
 * @dpi: the new resolution, in "dots per inch"
 *
 * Sets the resolution for the fontmap.
 *
 * This is a scale factor between points specified in a
 * `Pango2FontDescription` and Cairo units. The default value
 * is 96, meaning that a 10 point font will be 13 units high.
 * (10 * 96. / 72. = 13.3).
 */
void
pango2_font_map_set_resolution (Pango2FontMap *self,
                                float          dpi)
{
  g_return_if_fail (PANGO2_IS_FONT_MAP (self));
  g_return_if_fail (dpi > 0);

  if (self->dpi == dpi)
    return;

  self->dpi = dpi;

  clear_caches (self);
  pango2_font_map_changed (self);

  g_object_notify_by_pspec (G_OBJECT (self), properties[PROP_RESOLUTION]);
}

/**
 * pango2_font_map_get_resolution:
 * @self: a `Pango2FontMap`
 *
 * Returns the resolution for the fontmap.
 *
 * See [method@Pango2.FontMap.set_resolution].
 *
 * Return value: the resolution for the fontmap
 */
float
pango2_font_map_get_resolution (Pango2FontMap *self)
{
  g_return_val_if_fail (PANGO2_IS_FONT_MAP (self), 0);

  return self->dpi;
}

/* }}} */
/* {{{ Default handling */

static GPrivate default_font_map = G_PRIVATE_INIT (g_object_unref); /* MT-safe */

/**
 * pango2_font_map_new_default:
 *
 * Creates a new `Pango2FontMap` object.
 *
 * Note that the type of the returned object will depend
 * on the platform that Pango is used on. If you want to
 * explicitly create an instance of `Pango2FontMap` itself
 * (and not a platform-specific subclass), see [ctor@Pango2.FontMap.new].
 *
 * In most cases, you should use [func@Pango2.FontMap.get_default],
 * which will automatically create a new default fontmap,
 * if one has not been created for the current thread yet.
 *
 * Return value: (transfer full): the newly allocated `Pango2FontMap`
 */
Pango2FontMap *
pango2_font_map_new_default (void)
{
/* If we ever go back to having multiple backends built
 * at the same time, bring back a PANGO2_BACKEND env var
 * here.
 */
#if defined (HAVE_CORE_TEXT)
  return PANGO2_FONT_MAP (pango2_core_text_font_map_new ());
#elif defined (HAVE_DIRECT_WRITE)
  return PANGO2_FONT_MAP (pango2_direct_write_font_map_new ());
#elif defined (HAVE_FONTCONFIG)
  return PANGO2_FONT_MAP (pango2_fc_font_map_new ());
#else
  return NULL;
#endif
}

/**
 * pango2_font_map_get_default:
 *
 * Gets the default `Pango2FontMap`.
 *
 * The type of the returned object will depend on the
 * platform that Pango is used on.
 *
 * Note that the default fontmap is per-thread. Each thread gets
 * its own default fontmap. In this way, Pango can be used safely
 * from multiple threads.
 *
 * The default fontmap can be changed by using
 * [method@Pango2.FontMap.set_default].
 *
 * Return value: (transfer none): the default fontmap
 *  for the current thread. This object is owned by Pango and must
 *  not be freed.
 */
Pango2FontMap *
pango2_font_map_get_default (void)
{
  Pango2FontMap *fontmap = g_private_get (&default_font_map);

  if (G_UNLIKELY (!fontmap))
    {
      fontmap = pango2_font_map_new_default ();
      g_private_replace (&default_font_map, fontmap);
    }

  return fontmap;
}

/**
 * pango2_font_map_set_default:
 * @fontmap: (nullable): The new default font map
 *
 * Sets a default `Pango2FontMap`.
 *
 * The old default font map is unreffed and the new font map referenced.
 *
 * Note that the default fontmap is per-thread.
 * This function only changes the default fontmap for
 * the current thread. Default fontmaps of existing threads
 * are not changed. Default fontmaps of any new threads will
 * still be created using [ctor@Pango2.FontMap.new_default].
 *
 * A value of %NULL for @fontmap will cause the current default
 * font map to be released and a new default font map to be created
 * on demand, using [ctor@Pango2.FontMap.new_default].
 */
void
pango2_font_map_set_default (Pango2FontMap *fontmap)
{
  g_return_if_fail (fontmap == NULL || PANGO2_IS_FONT_MAP (fontmap));

  if (fontmap)
    g_object_ref (fontmap);

  g_private_replace (&default_font_map, fontmap);
}

/* }}} */

/* vim:set foldmethod=marker expandtab: */
