/* Pango
 * pangofc-fontmap.c: Base fontmap type for fontconfig-based backends
 *
 * Copyright (C) 2000-2003 Red Hat, Inc.
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

#define FONTSET_CACHE_SIZE 256

#include "config.h"
#include <math.h>

#include "pango-context.h"
#include "pangofc-fontmap.h"
#include "pangofc-private.h"
#include "pango-impl-utils.h"
#include "modules.h"
#include "pango-enum-types.h"


/* Overview:
 *
 * All programming is a practice in caching data.  PangoFcFontMap is the
 * major caching container of a Pango system on a Linux desktop.  Here is
 * a short overview of how it all works.
 *
 * In short, Fontconfig search patterns are constructed and a fontset loaded
 * using them.  Here is how we achieve that:
 *
 * - All FcPattern's referenced by any object in the fontmap are uniquified
 *   and cached in the fontmap.  This both speeds lookups based on patterns
 *   faster, and saves memory.  This is handled by fontmap->priv->pattern_hash.
 *   The patterns are cached indefinitely.
 *
 * - The results of a FcFontSort() are used to populate fontsets.  However,
 *   FcFontSort() relies on the search pattern only, which includes the font
 *   size but not the full font matrix.  The fontset however depends on the
 *   matrix.  As a result, multiple fontsets may need results of the
 *   FcFontSort() on the same input pattern (think rotating text).  As such,
 *   we cache FcFontSort() results in fontmap->priv->patterns_hash which
 *   is a refcounted structure.  This level of abstraction also allows for
 *   optimizations like calling FcFontMatch() instead of FcFontSort(), and
 *   only calling FcFontSort() if any patterns other than the first match
 *   are needed.  Another possible optimization would be to call FcFontSort()
 *   without trimming, and do the trimming lazily as we go.  Only pattern sets
 *   already referenced by a fontset are cached.
 *
 * - A number of most-recently-used fontsets are cached and reused when
 *   needed.  This is achieved using fontmap->priv->fontset_hash and
 *   fontmap->priv->fontset_cache.
 *
 * - All fonts created by any of our fontsets are also cached and reused.
 *   This is what fontmap->priv->font_hash does.
 *
 * - Data that only depends on the font file and face index is cached and
 *   reused by multiple fonts.  This includes coverage and cmap cache info.
 *   This is done using fontmap->priv->font_face_data_hash.
 *
 * Upon a cache_clear() request, all caches are emptied.  All objects (fonts,
 * fontsets, faces, families) having a reference from outside will still live
 * and may reference the fontmap still, but will not be reused by the fontmap.
 *
 *
 * Todo:
 *
 * - Make PangoCoverage a GObject and subclass it as PangoFcCoverage which
 *   will directly use FcCharset. (#569622)
 *
 * - Lazy trimming of FcFontSort() results.  Requires fontconfig with
 *   FcCharSetMerge().
 */


typedef struct _PangoFcFontFaceData PangoFcFontFaceData;
typedef struct _PangoFcFace         PangoFcFace;
typedef struct _PangoFcFamily       PangoFcFamily;
typedef struct _PangoFcFindFuncInfo PangoFcFindFuncInfo;
typedef struct _PangoFcPatterns     PangoFcPatterns;
typedef struct _PangoFcFontset      PangoFcFontset;

#define PANGO_FC_TYPE_FAMILY            (pango_fc_family_get_type ())
#define PANGO_FC_FAMILY(object)         (G_TYPE_CHECK_INSTANCE_CAST ((object), PANGO_FC_TYPE_FAMILY, PangoFcFamily))
#define PANGO_FC_IS_FAMILY(object)      (G_TYPE_CHECK_INSTANCE_TYPE ((object), PANGO_FC_TYPE_FAMILY))

#define PANGO_FC_TYPE_FACE              (pango_fc_face_get_type ())
#define PANGO_FC_FACE(object)           (G_TYPE_CHECK_INSTANCE_CAST ((object), PANGO_FC_TYPE_FACE, PangoFcFace))
#define PANGO_FC_IS_FACE(object)        (G_TYPE_CHECK_INSTANCE_TYPE ((object), PANGO_FC_TYPE_FACE))

#define PANGO_FC_TYPE_FONTSET           (pango_fc_fontset_get_type ())
#define PANGO_FC_FONTSET(object)        (G_TYPE_CHECK_INSTANCE_CAST ((object), PANGO_FC_TYPE_FONTSET, PangoFcFontset))
#define PANGO_FC_IS_FONTSET(object)     (G_TYPE_CHECK_INSTANCE_TYPE ((object), PANGO_FC_TYPE_FONTSET))

struct _PangoFcFontMapPrivate
{
  GHashTable *fontset_hash;	/* Maps PangoFcFontsetKey -> PangoFcFontset  */
  GQueue *fontset_cache;	/* Recently used fontsets */

  GHashTable *font_hash;	/* Maps PangoFcFontKey -> PangoFcFont */

  GHashTable *patterns_hash;	/* Maps FcPattern -> PangoFcPatterns */

  /* pattern_hash is used to make sure we only store one copy of
   * each identical pattern. (Speeds up lookup).
   */
  GHashTable *pattern_hash;

  GHashTable *font_face_data_hash; /* Maps font file name/id -> data */

  /* List of all families availible */
  PangoFcFamily **families;
  int n_families;		/* -1 == uninitialized */

  double dpi;

  /* Decoders */
  GSList *findfuncs;

  guint closed : 1;
};

struct _PangoFcFontFaceData
{
  /* Key */
  char *filename;
  int id;            /* needed to handle TTC files with multiple faces */

  /* Data */
  FcPattern *pattern;	/* Referenced pattern that owns filename */
  PangoCoverage *coverage;
  PangoFcCmapCache *cmap_cache;
};

struct _PangoFcFace
{
  PangoFontFace parent_instance;

  PangoFcFamily *family;
  char *style;

  guint fake : 1;
};

struct _PangoFcFamily
{
  PangoFontFamily parent_instance;

  PangoFcFontMap *fontmap;
  char *family_name;

  PangoFcFace **faces;
  int n_faces;		/* -1 == uninitialized */

  int spacing;  /* FC_SPACING */
};

struct _PangoFcFindFuncInfo
{
  PangoFcDecoderFindFunc findfunc;
  gpointer               user_data;
  GDestroyNotify         dnotify;
  gpointer               ddata;
};

static GType    pango_fc_family_get_type     (void);
static GType    pango_fc_face_get_type       (void);
static GType    pango_fc_fontset_get_type    (void);

static void          pango_fc_font_map_finalize      (GObject                      *object);
static PangoFont *   pango_fc_font_map_load_font     (PangoFontMap                 *fontmap,
						       PangoContext                 *context,
						       const PangoFontDescription   *description);
static PangoFontset *pango_fc_font_map_load_fontset  (PangoFontMap                 *fontmap,
						       PangoContext                 *context,
						       const PangoFontDescription   *desc,
						       PangoLanguage                *language);
static void          pango_fc_font_map_list_families (PangoFontMap                 *fontmap,
						       PangoFontFamily            ***families,
						       int                          *n_families);

static double pango_fc_font_map_get_resolution (PangoFcFontMap *fcfontmap,
						PangoContext   *context);
static PangoFont *pango_fc_font_map_new_font   (PangoFcFontMap    *fontmap,
						PangoFcFontsetKey *fontset_key,
						FcPattern         *match);

static guint    pango_fc_font_face_data_hash  (PangoFcFontFaceData *key);
static gboolean pango_fc_font_face_data_equal (PangoFcFontFaceData *key1,
					       PangoFcFontFaceData *key2);

static void               pango_fc_fontset_key_init  (PangoFcFontsetKey          *key,
						      PangoFcFontMap             *fcfontmap,
						      PangoContext               *context,
						      const PangoFontDescription *desc,
						      PangoLanguage              *language);
static PangoFcFontsetKey *pango_fc_fontset_key_copy  (const PangoFcFontsetKey *key);
static void               pango_fc_fontset_key_free  (PangoFcFontsetKey       *key);
static guint              pango_fc_fontset_key_hash  (const PangoFcFontsetKey *key);
static gboolean           pango_fc_fontset_key_equal (const PangoFcFontsetKey *key_a,
						      const PangoFcFontsetKey *key_b);

static void               pango_fc_font_key_init     (PangoFcFontKey       *key,
						      PangoFcFontMap       *fcfontmap,
						      PangoFcFontsetKey    *fontset_key,
						      FcPattern            *pattern);
static PangoFcFontKey    *pango_fc_font_key_copy     (const PangoFcFontKey *key);
static void               pango_fc_font_key_free     (PangoFcFontKey       *key);
static guint              pango_fc_font_key_hash     (const PangoFcFontKey *key);
static gboolean           pango_fc_font_key_equal    (const PangoFcFontKey *key_a,
						      const PangoFcFontKey *key_b);

static PangoFcPatterns *pango_fc_patterns_new   (FcPattern       *pat,
						 PangoFcFontMap  *fontmap);
static PangoFcPatterns *pango_fc_patterns_ref   (PangoFcPatterns *pats);
static void             pango_fc_patterns_unref (PangoFcPatterns *pats);
static FcPattern       *pango_fc_patterns_get_pattern      (PangoFcPatterns *pats);
static FcPattern       *pango_fc_patterns_get_font_pattern (PangoFcPatterns *pats, int i);

static FcPattern *uniquify_pattern (PangoFcFontMap *fcfontmap,
				    FcPattern      *pattern);

static gpointer
get_gravity_class (void)
{
  static GEnumClass *class = NULL;

  if (G_UNLIKELY (!class))
    class = g_type_class_ref (PANGO_TYPE_GRAVITY);

  return class;
}

static guint
pango_fc_font_face_data_hash (PangoFcFontFaceData *key)
{
  return g_str_hash (key->filename) ^ key->id;
}

static gboolean
pango_fc_font_face_data_equal (PangoFcFontFaceData *key1,
			       PangoFcFontFaceData *key2)
{
  return key1->id == key2->id &&
	 (key1 == key2 || 0 == strcmp (key1->filename, key2->filename));
}

static void
pango_fc_font_face_data_free (PangoFcFontFaceData *data)
{
  FcPatternDestroy (data->pattern);

  if (data->coverage)
    pango_coverage_unref (data->coverage);

  if (data->cmap_cache)
    _pango_fc_cmap_cache_unref (data->cmap_cache);

  g_slice_free (PangoFcFontFaceData, data);
}

/* Fowler / Noll / Vo (FNV) Hash (http://www.isthe.com/chongo/tech/comp/fnv/)
 *
 * Not necessarily better than a lot of other hashes, but should be OK, and
 * well tested with binary data.
 */

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

static void
get_context_matrix (PangoContext *context,
		    PangoMatrix *matrix)
{
  const PangoMatrix *set_matrix;
  static const PangoMatrix identity = PANGO_MATRIX_INIT;

  set_matrix = context ? pango_context_get_matrix (context) : NULL;
  *matrix = set_matrix ? *set_matrix : identity;
  matrix->x0 = matrix->y0 = 0.;
}

static int
get_scaled_size (PangoFcFontMap             *fcfontmap,
		 PangoContext               *context,
		 const PangoFontDescription *desc)
{
  double size = pango_font_description_get_size (desc);

  if (!pango_font_description_get_size_is_absolute (desc))
    {
      double dpi = pango_fc_font_map_get_resolution (fcfontmap, context);

      size = size * dpi / 72.;
    }

  return .5 + pango_matrix_get_font_scale_factor (pango_context_get_matrix (context)) * size;
}



struct _PangoFcFontsetKey {
  PangoFcFontMap *fontmap;
  PangoLanguage *language;
  PangoFontDescription *desc;
  PangoMatrix matrix;
  int pixelsize;
  double resolution;
  gpointer context_key;
};

struct _PangoFcFontKey {
  PangoFcFontMap *fontmap;
  FcPattern *pattern;
  PangoMatrix matrix;
  gpointer context_key;
};

static void
pango_fc_fontset_key_init (PangoFcFontsetKey          *key,
			   PangoFcFontMap             *fcfontmap,
			   PangoContext               *context,
			   const PangoFontDescription *desc,
			   PangoLanguage              *language)
{
  if (!language && context)
    language = pango_context_get_language (context);

  key->fontmap = fcfontmap;
  get_context_matrix (context, &key->matrix);
  key->pixelsize = get_scaled_size (fcfontmap, context, desc);
  key->resolution = pango_fc_font_map_get_resolution (fcfontmap, context);
  key->language = language;
  key->desc = pango_font_description_copy_static (desc);
  pango_font_description_unset_fields (key->desc, PANGO_FONT_MASK_SIZE);

  if (context && PANGO_FC_FONT_MAP_GET_CLASS (fcfontmap)->context_key_get)
    key->context_key = (gpointer)PANGO_FC_FONT_MAP_GET_CLASS (fcfontmap)->context_key_get (fcfontmap, context);
  else
    key->context_key = NULL;
}

static gboolean
pango_fc_fontset_key_equal (const PangoFcFontsetKey *key_a,
			    const PangoFcFontsetKey *key_b)
{
  if (key_a->language == key_b->language &&
      key_a->pixelsize == key_b->pixelsize &&
      key_a->resolution == key_b->resolution &&
      pango_font_description_equal (key_a->desc, key_b->desc) &&
      0 == memcmp (&key_a->matrix, &key_b->matrix, 4 * sizeof (double)))
    {
      if (key_a->context_key)
	return PANGO_FC_FONT_MAP_GET_CLASS (key_a->fontmap)->context_key_equal (key_a->fontmap,
										key_a->context_key,
										key_b->context_key);
      else
        return key_a->context_key == key_b->context_key;
    }
  else
    return FALSE;
}

static guint
pango_fc_fontset_key_hash (const PangoFcFontsetKey *key)
{
    guint32 hash = FNV1_32_INIT;

    /* We do a bytewise hash on the doubles */
    hash = hash_bytes_fnv ((unsigned char *)(&key->matrix), sizeof (double) * 4, hash);
    hash = hash_bytes_fnv ((unsigned char *)(&key->resolution), sizeof (double), hash);

    hash ^= key->pixelsize;

    if (key->context_key)
      hash ^= PANGO_FC_FONT_MAP_GET_CLASS (key->fontmap)->context_key_hash (key->fontmap,
									    key->context_key);

    return (hash ^
	    GPOINTER_TO_UINT (key->language) ^
	    pango_font_description_hash (key->desc));
}

static void
pango_fc_fontset_key_free (PangoFcFontsetKey *key)
{
  pango_font_description_free (key->desc);

  if (key->context_key)
    PANGO_FC_FONT_MAP_GET_CLASS (key->fontmap)->context_key_free (key->fontmap,
								  key->context_key);

  g_slice_free (PangoFcFontsetKey, key);
}

static PangoFcFontsetKey *
pango_fc_fontset_key_copy (const PangoFcFontsetKey *old)
{
  PangoFcFontsetKey *key = g_slice_new (PangoFcFontsetKey);

  key->fontmap = old->fontmap;
  key->language = old->language;
  key->desc = pango_font_description_copy (old->desc);
  key->matrix = old->matrix;
  key->pixelsize = old->pixelsize;
  key->resolution = old->resolution;
  if (old->context_key)
    key->context_key = PANGO_FC_FONT_MAP_GET_CLASS (key->fontmap)->context_key_copy (key->fontmap,
										     old->context_key);
  else
    key->context_key = NULL;

  return key;
}

/**
 * pango_fc_fontset_key_get_language:
 * @key: the fontset key
 *
 * Gets the language member of @key.
 *
 * Returns: the language
 *
 * Since: 1.24
 **/
PangoLanguage *
pango_fc_fontset_key_get_language (const PangoFcFontsetKey *key)
{
  return key->language;
}

/**
 * pango_fc_fontset_key_get_description:
 * @key: the fontset key
 *
 * Gets the font description of @key.
 *
 * Returns: the font description, which is owned by @key and should not be modified.
 *
 * Since: 1.24
 **/
const PangoFontDescription *
pango_fc_fontset_key_get_description (const PangoFcFontsetKey *key)
{
  return key->desc;
}

/**
 * pango_fc_fontset_key_get_matrix:
 * @key: the fontset key
 *
 * Gets the matrix member of @key.
 *
 * Returns: the matrix, which is owned by @key and should not be modified.
 *
 * Since: 1.24
 **/
const PangoMatrix *
pango_fc_fontset_key_get_matrix      (const PangoFcFontsetKey *key)
{
  return &key->matrix;
}

/**
 * pango_fc_fontset_key_get_absolute_size:
 * @key: the fontset key
 *
 * Gets the absolute font size of @key in Pango units.  This is adjusted
 * for both resolution and transformation matrix.
 *
 * Returns: the pixel size of @key.
 *
 * Since: 1.24
 **/
double
pango_fc_fontset_key_get_absolute_size   (const PangoFcFontsetKey *key)
{
  return key->pixelsize;
}

/**
 * pango_fc_fontset_key_get_resolution:
 * @key: the fontset key
 *
 * Gets the resolution of @key
 *
 * Returns: the resolution of @key
 *
 * Since: 1.24
 **/
double
pango_fc_fontset_key_get_resolution  (const PangoFcFontsetKey *key)
{
  return key->resolution;
}

/**
 * pango_fc_fontset_key_get_context_key:
 * @key: the font key
 *
 * Gets the context key member of @key.
 *
 * Returns: the context key, which is owned by @key and should not be modified.
 *
 * Since: 1.24
 **/
gpointer
pango_fc_fontset_key_get_context_key (const PangoFcFontsetKey *key)
{
  return key->context_key;
}

/*
 * PangoFcFontKey
 */

static gboolean
pango_fc_font_key_equal (const PangoFcFontKey *key_a,
			 const PangoFcFontKey *key_b)
{
  if (key_a->pattern == key_b->pattern &&
      0 == memcmp (&key_a->matrix, &key_b->matrix, 4 * sizeof (double)))
    {
      if (key_a->context_key && key_b->context_key)
	return PANGO_FC_FONT_MAP_GET_CLASS (key_a->fontmap)->context_key_equal (key_a->fontmap,
										key_a->context_key,
										key_b->context_key);
      else
        return key_a->context_key == key_b->context_key;
    }
  else
    return FALSE;
}

static guint
pango_fc_font_key_hash (const PangoFcFontKey *key)
{
    guint32 hash = FNV1_32_INIT;

    /* We do a bytewise hash on the doubles */
    hash = hash_bytes_fnv ((unsigned char *)(&key->matrix), sizeof (double) * 4, hash);

    if (key->context_key)
      hash ^= PANGO_FC_FONT_MAP_GET_CLASS (key->fontmap)->context_key_hash (key->fontmap,
									    key->context_key);

    return (hash ^ GPOINTER_TO_UINT (key->pattern));
}

static void
pango_fc_font_key_free (PangoFcFontKey *key)
{
  if (key->pattern)
    FcPatternDestroy (key->pattern);

  if (key->context_key)
    PANGO_FC_FONT_MAP_GET_CLASS (key->fontmap)->context_key_free (key->fontmap,
								  key->context_key);

  g_slice_free (PangoFcFontKey, key);
}

static PangoFcFontKey *
pango_fc_font_key_copy (const PangoFcFontKey *old)
{
  PangoFcFontKey *key = g_slice_new (PangoFcFontKey);

  key->fontmap = old->fontmap;
  FcPatternReference (old->pattern);
  key->pattern = old->pattern;
  key->matrix = old->matrix;
  if (old->context_key)
    key->context_key = PANGO_FC_FONT_MAP_GET_CLASS (key->fontmap)->context_key_copy (key->fontmap,
										     old->context_key);
  else
    key->context_key = NULL;

  return key;
}

static void
pango_fc_font_key_init (PangoFcFontKey    *key,
			PangoFcFontMap    *fcfontmap,
			PangoFcFontsetKey *fontset_key,
			FcPattern         *pattern)
{
  key->fontmap = fcfontmap;
  key->pattern = pattern;
  key->matrix = *pango_fc_fontset_key_get_matrix (fontset_key);
  key->context_key = pango_fc_fontset_key_get_context_key (fontset_key);
}

/* Public API */

/**
 * pango_fc_font_key_get_pattern:
 * @key: the font key
 *
 * Gets the fontconfig pattern member of @key.
 *
 * Returns: the pattern, which is owned by @key and should not be modified.
 *
 * Since: 1.24
 **/
const FcPattern *
pango_fc_font_key_get_pattern (const PangoFcFontKey *key)
{
  return key->pattern;
}

/**
 * pango_fc_font_key_get_matrix:
 * @key: the font key
 *
 * Gets the matrix member of @key.
 *
 * Returns: the matrix, which is owned by @key and should not be modified.
 *
 * Since: 1.24
 **/
const PangoMatrix *
pango_fc_font_key_get_matrix (const PangoFcFontKey *key)
{
  return &key->matrix;
}

/**
 * pango_fc_font_key_get_context_key:
 * @key: the font key
 *
 * Gets the context key member of @key.
 *
 * Returns: the context key, which is owned by @key and should not be modified.
 *
 * Since: 1.24
 **/
gpointer
pango_fc_font_key_get_context_key (const PangoFcFontKey *key)
{
  return key->context_key;
}


/*
 * PangoFcPatterns
 */

struct _PangoFcPatterns {
  guint ref_count;

  PangoFcFontMap *fontmap;

  FcPattern *pattern;
  FcPattern *match;
  FcFontSet *fontset;
};

static PangoFcPatterns *
pango_fc_patterns_new (FcPattern *pat, PangoFcFontMap *fontmap)
{
  PangoFcPatterns *pats;

  pat = uniquify_pattern (fontmap, pat);
  pats = g_hash_table_lookup (fontmap->priv->patterns_hash, pat);
  if (pats)
    return pango_fc_patterns_ref (pats);

  pats = g_slice_new0 (PangoFcPatterns);

  pats->fontmap = fontmap;

  pats->ref_count = 1;
  FcPatternReference (pat);
  pats->pattern = pat;

  g_hash_table_insert (fontmap->priv->patterns_hash,
		       pats->pattern, pats);

  return pats;
}

static PangoFcPatterns *
pango_fc_patterns_ref (PangoFcPatterns *pats)
{
  g_return_val_if_fail (pats->ref_count > 0, NULL);

  pats->ref_count++;

  return pats;
}

static void
pango_fc_patterns_unref (PangoFcPatterns *pats)
{
  g_return_if_fail (pats->ref_count > 0);

  pats->ref_count--;

  if (pats->ref_count)
    return;

  /* Only remove from fontmap hash if we are in it.  This is not necessarily
   * the case after a cache_clear() call. */
  if (pats->fontmap->priv->patterns_hash &&
      pats == g_hash_table_lookup (pats->fontmap->priv->patterns_hash, pats->pattern))
    g_hash_table_remove (pats->fontmap->priv->patterns_hash,
			 pats->pattern);

  if (pats->pattern)
    FcPatternDestroy (pats->pattern);

  if (pats->match)
    FcPatternDestroy (pats->match);

  if (pats->fontset)
    FcFontSetDestroy (pats->fontset);

  g_slice_free (PangoFcPatterns, pats);
}

static FcPattern *
pango_fc_patterns_get_pattern (PangoFcPatterns *pats)
{
  return pats->pattern;
}

static FcPattern *
pango_fc_patterns_get_font_pattern (PangoFcPatterns *pats, int i)
{
  if (i == 0)
    {
      FcResult result;
      if (!pats->match && !pats->fontset)
        {
	  pats->match = FcFontMatch (NULL, pats->pattern, &result);
	}

      if (pats->match)
	return pats->match;
    }
  else
    {
      if (!pats->fontset)
        {
	  FcResult result;
	  pats->fontset = FcFontSort (NULL, pats->pattern, FcTrue, NULL, &result);
	  if (pats->match)
	    {
	      FcPatternDestroy (pats->match);
	      pats->match = NULL;
	    }
	}
    }

  if (pats->fontset && i < pats->fontset->nfont)
    return pats->fontset->fonts[i];
  else
    return NULL;
}


/*
 * PangoFcFontset
 */

static void              pango_fc_fontset_finalize     (GObject                 *object);
static PangoLanguage *   pango_fc_fontset_get_language (PangoFontset            *fontset);
static  PangoFont *      pango_fc_fontset_get_font     (PangoFontset            *fontset,
							guint                    wc);
static void              pango_fc_fontset_foreach      (PangoFontset            *fontset,
							PangoFontsetForeachFunc  func,
							gpointer                 data);

struct _PangoFcFontset
{
  PangoFontset parent_instance;

  PangoFcFontsetKey *key;

  PangoFcPatterns *patterns;
  int patterns_i;

  GPtrArray *fonts;
  GPtrArray *coverages;

  GList *cache_link;
};

typedef PangoFontsetClass PangoFcFontsetClass;
G_DEFINE_TYPE (PangoFcFontset, pango_fc_fontset, PANGO_TYPE_FONTSET);

static PangoFcFontset *
pango_fc_fontset_new (PangoFcFontsetKey *key,
		      PangoFcPatterns   *patterns)
{
  PangoFcFontset *fontset;

  fontset = g_object_new (PANGO_FC_TYPE_FONTSET, NULL);

  fontset->key = pango_fc_fontset_key_copy (key);
  fontset->patterns = pango_fc_patterns_ref (patterns);

  return fontset;
}

static PangoFcFontsetKey *
pango_fc_fontset_get_key (PangoFcFontset *fontset)
{
  return fontset->key;
}

static PangoFont *
pango_fc_fontset_load_next_font (PangoFcFontset *fontset)
{
  FcPattern *pattern, *font_pattern;
  PangoFont *font;

  pattern = pango_fc_patterns_get_pattern (fontset->patterns),
  font_pattern = pango_fc_patterns_get_font_pattern (fontset->patterns,
						     fontset->patterns_i++);
  if (G_UNLIKELY (!font_pattern))
    return NULL;

  font_pattern = FcFontRenderPrepare (NULL, pattern, font_pattern);

  if (G_UNLIKELY (!font_pattern))
    return NULL;

#ifdef FC_PATTERN
    /* The FC_PATTERN element, which points back to our the original
     * pattern defeats our hash tables.
     */
    FcPatternDel (font_pattern, FC_PATTERN);
#endif /* FC_PATTERN */

  font = pango_fc_font_map_new_font (fontset->key->fontmap,
				     fontset->key,
				     font_pattern);

  FcPatternDestroy (font_pattern);

  return font;
}

static PangoFont *
pango_fc_fontset_get_font_at (PangoFcFontset *fontset,
			      unsigned int i)
{
  while (i >= fontset->fonts->len)
    {
      PangoFont *font = pango_fc_fontset_load_next_font (fontset);
      g_ptr_array_add (fontset->fonts, font);
      g_ptr_array_add (fontset->coverages, NULL);
      if (!font)
        return NULL;
    }

  return g_ptr_array_index (fontset->fonts, i);
}

static void
pango_fc_fontset_class_init (PangoFcFontsetClass *class)
{
  GObjectClass *object_class = G_OBJECT_CLASS (class);
  PangoFontsetClass *fontset_class = PANGO_FONTSET_CLASS (class);

  object_class->finalize = pango_fc_fontset_finalize;

  fontset_class->get_font = pango_fc_fontset_get_font;
  fontset_class->get_language = pango_fc_fontset_get_language;
  fontset_class->foreach = pango_fc_fontset_foreach;
}

static void
pango_fc_fontset_init (PangoFcFontset *fontset)
{
  fontset->fonts = g_ptr_array_new ();
  fontset->coverages = g_ptr_array_new ();
}

static void
pango_fc_fontset_finalize (GObject *object)
{
  PangoFcFontset *fontset = PANGO_FC_FONTSET (object);
  unsigned int i;

  for (i = 0; i < fontset->fonts->len; i++)
  {
    PangoFont *font = g_ptr_array_index(fontset->fonts, i);
    if (font)
      g_object_unref (font);
  }
  g_ptr_array_free (fontset->fonts, TRUE);

  for (i = 0; i < fontset->coverages->len; i++)
    {
      PangoCoverage *coverage = g_ptr_array_index (fontset->coverages, i);
      if (coverage)
	pango_coverage_unref (coverage);
    }
  g_ptr_array_free (fontset->coverages, TRUE);

  if (fontset->key)
    pango_fc_fontset_key_free (fontset->key);

  if (fontset->patterns)
    pango_fc_patterns_unref (fontset->patterns);

  G_OBJECT_CLASS (pango_fc_fontset_parent_class)->finalize (object);
}

static PangoLanguage *
pango_fc_fontset_get_language (PangoFontset  *fontset)
{
  PangoFcFontset *fcfontset = PANGO_FC_FONTSET (fontset);

  return pango_fc_fontset_key_get_language (pango_fc_fontset_get_key (fcfontset));
}

static PangoFont *
pango_fc_fontset_get_font (PangoFontset  *fontset,
			   guint          wc)
{
  PangoFcFontset *fcfontset = PANGO_FC_FONTSET (fontset);
  PangoCoverageLevel best_level = PANGO_COVERAGE_NONE;
  PangoCoverageLevel level;
  PangoFont *font;
  PangoCoverage *coverage;
  int result = -1;
  unsigned int i;

  for (i = 0;
       pango_fc_fontset_get_font_at (fcfontset, i);
       i++)
    {
      coverage = g_ptr_array_index (fcfontset->coverages, i);

      if (coverage == NULL)
	{
	  font = g_ptr_array_index (fcfontset->fonts, i);

	  coverage = pango_font_get_coverage (font, fcfontset->key->language);
	  g_ptr_array_index (fcfontset->coverages, i) = coverage;
	}

      level = pango_coverage_get (coverage, wc);

      if (result == -1 || level > best_level)
	{
	  result = i;
	  best_level = level;
	  if (level == PANGO_COVERAGE_EXACT)
	    break;
	}
    }

  if (G_UNLIKELY (result == -1))
    return NULL;

  font = g_ptr_array_index(fcfontset->fonts, result);
  return g_object_ref (font);
}

static void
pango_fc_fontset_foreach (PangoFontset           *fontset,
			  PangoFontsetForeachFunc func,
			  gpointer                data)
{
  PangoFcFontset *fcfontset = PANGO_FC_FONTSET (fontset);
  PangoFont *font;
  unsigned int i;

  for (i = 0;
       (font = pango_fc_fontset_get_font_at (fcfontset, i));
       i++)
    {
      if ((*func) (fontset, font, data))
	return;
    }
}


/*
 * PangoFcFontMap
 */

G_DEFINE_ABSTRACT_TYPE (PangoFcFontMap, pango_fc_font_map, PANGO_TYPE_FONT_MAP)

static void
pango_fc_font_map_init (PangoFcFontMap *fcfontmap)
{
  static gboolean registered_modules = FALSE;
  PangoFcFontMapPrivate *priv;

  priv = fcfontmap->priv = G_TYPE_INSTANCE_GET_PRIVATE (fcfontmap,
							PANGO_TYPE_FC_FONT_MAP,
							PangoFcFontMapPrivate);

  if (!registered_modules)
    {
      int i;

      registered_modules = TRUE;

      for (i = 0; _pango_included_fc_modules[i].list; i++)
	pango_module_register (&_pango_included_fc_modules[i]);
    }

  priv->n_families = -1;

  priv->font_hash = g_hash_table_new ((GHashFunc)pango_fc_font_key_hash,
				      (GEqualFunc)pango_fc_font_key_equal);

  priv->fontset_hash = g_hash_table_new_full ((GHashFunc)pango_fc_fontset_key_hash,
					      (GEqualFunc)pango_fc_fontset_key_equal,
					      NULL,
					      (GDestroyNotify)g_object_unref);
  priv->fontset_cache = g_queue_new ();

  priv->patterns_hash = g_hash_table_new (NULL, NULL);

  priv->pattern_hash = g_hash_table_new_full ((GHashFunc) FcPatternHash,
					      (GEqualFunc) FcPatternEqual,
					      (GDestroyNotify) FcPatternDestroy,
					      NULL);

  priv->font_face_data_hash = g_hash_table_new_full ((GHashFunc)pango_fc_font_face_data_hash,
						     (GEqualFunc)pango_fc_font_face_data_equal,
						     (GDestroyNotify)pango_fc_font_face_data_free,
						     NULL);
  priv->dpi = -1;
}

static void
pango_fc_font_map_fini (PangoFcFontMap *fcfontmap)
{
  PangoFcFontMapPrivate *priv = fcfontmap->priv;
  int i;

  g_queue_free (priv->fontset_cache);
  priv->fontset_cache = NULL;

  g_hash_table_destroy (priv->fontset_hash);
  priv->fontset_hash = NULL;

  g_hash_table_destroy (priv->patterns_hash);
  priv->patterns_hash = NULL;

  g_hash_table_destroy (priv->font_hash);
  priv->font_hash = NULL;

  g_hash_table_destroy (priv->font_face_data_hash);
  priv->font_face_data_hash = NULL;

  g_hash_table_destroy (priv->pattern_hash);
  priv->pattern_hash = NULL;

  for (i = 0; i < priv->n_families; i++)
    g_object_unref (priv->families[i]);
  g_free (priv->families);
  priv->n_families = -1;
  priv->families = NULL;
}

static void
pango_fc_font_map_class_init (PangoFcFontMapClass *class)
{
  GObjectClass *object_class = G_OBJECT_CLASS (class);
  PangoFontMapClass *fontmap_class = PANGO_FONT_MAP_CLASS (class);

  object_class->finalize = pango_fc_font_map_finalize;
  fontmap_class->load_font = pango_fc_font_map_load_font;
  fontmap_class->load_fontset = pango_fc_font_map_load_fontset;
  fontmap_class->list_families = pango_fc_font_map_list_families;
  fontmap_class->shape_engine_type = PANGO_RENDER_TYPE_FC;

  g_type_class_add_private (object_class, sizeof (PangoFcFontMapPrivate));
}


/**
 * pango_fc_font_map_add_decoder_find_func:
 * @fcfontmap: The #PangoFcFontMap to add this method to.
 * @findfunc: The #PangoFcDecoderFindFunc callback function
 * @user_data: User data.
 * @dnotify: A #GDestroyNotify callback that will be called when the
 *  fontmap is finalized and the decoder is released.
 *
 * This function saves a callback method in the #PangoFcFontMap that
 * will be called whenever new fonts are created.  If the
 * function returns a #PangoFcDecoder, that decoder will be used to
 * determine both coverage via a #FcCharSet and a one-to-one mapping of
 * characters to glyphs.  This will allow applications to have
 * application-specific encodings for various fonts.
 *
 * Since: 1.6
 **/
void
pango_fc_font_map_add_decoder_find_func (PangoFcFontMap        *fcfontmap,
					 PangoFcDecoderFindFunc findfunc,
					 gpointer               user_data,
					 GDestroyNotify         dnotify)
{
  PangoFcFontMapPrivate *priv;
  PangoFcFindFuncInfo *info;

  g_return_if_fail (PANGO_IS_FC_FONT_MAP (fcfontmap));

  priv = fcfontmap->priv;

  info = g_slice_new (PangoFcFindFuncInfo);

  info->findfunc = findfunc;
  info->user_data = user_data;
  info->dnotify = dnotify;

  priv->findfuncs = g_slist_append (priv->findfuncs, info);
}

/**
 * pango_fc_font_map_find_decoder:
 * @fcfontmap: The #PangoFcFontMap to use.
 * @pattern: The #FcPattern to find the decoder for.
 *
 * Finds the decoder to use for @pattern.  Decoders can be added to
 * a font map using pango_fc_font_map_add_decoder_find_func().
 *
 * Returns: a newly created #PangoFcDecoder object or %NULL if
 *          no decoder is set for @pattern.
 *
 * Since: 1.26
 **/
PangoFcDecoder *
pango_fc_font_map_find_decoder  (PangoFcFontMap *fcfontmap,
				 FcPattern      *pattern)
{
  GSList *l;

  g_return_val_if_fail (PANGO_IS_FC_FONT_MAP (fcfontmap), NULL);
  g_return_val_if_fail (pattern != NULL, NULL);

  for (l = fcfontmap->priv->findfuncs; l && l->data; l = l->next)
    {
      PangoFcFindFuncInfo *info = l->data;
      PangoFcDecoder *decoder;

      decoder = info->findfunc (pattern, info->user_data);
      if (decoder)
	return decoder;
    }

  return NULL;
}

static void
pango_fc_font_map_finalize (GObject *object)
{
  PangoFcFontMap *fcfontmap = PANGO_FC_FONT_MAP (object);

  pango_fc_font_map_shutdown (fcfontmap);

  G_OBJECT_CLASS (pango_fc_font_map_parent_class)->finalize (object);
}

/* Add a mapping from key to fcfont */
static void
pango_fc_font_map_add (PangoFcFontMap *fcfontmap,
		       PangoFcFontKey *key,
		       PangoFcFont    *fcfont)
{
  PangoFcFontMapPrivate *priv = fcfontmap->priv;
  PangoFcFontKey *key_copy;

  key_copy = pango_fc_font_key_copy (key);
  _pango_fc_font_set_font_key (fcfont, key_copy);
  g_hash_table_insert (priv->font_hash, key_copy, fcfont);
}

/* Remove mapping from fcfont->key to fcfont */
/* Closely related to shutdown_font() */
void
_pango_fc_font_map_remove (PangoFcFontMap *fcfontmap,
			   PangoFcFont    *fcfont)
{
  PangoFcFontMapPrivate *priv = fcfontmap->priv;
  PangoFcFontKey *key;

  key = _pango_fc_font_get_font_key (fcfont);
  if (key)
    {
      /* Only remove from fontmap hash if we are in it.  This is not necessarily
       * the case after a cache_clear() call. */
      if (priv->font_hash &&
	  fcfont == g_hash_table_lookup (priv->font_hash, key))
        {
	  g_hash_table_remove (priv->font_hash, key);
	}
      _pango_fc_font_set_font_key (fcfont, NULL);
      pango_fc_font_key_free (key);
    }
}

static PangoFcFamily *
create_family (PangoFcFontMap *fcfontmap,
	       const char     *family_name,
	       int             spacing)
{
  PangoFcFamily *family = g_object_new (PANGO_FC_TYPE_FAMILY, NULL);
  family->fontmap = fcfontmap;
  family->family_name = g_strdup (family_name);
  family->spacing = spacing;

  return family;
}

static gboolean
is_alias_family (const char *family_name)
{
  switch (family_name[0])
    {
    case 'm':
    case 'M':
      return (g_ascii_strcasecmp (family_name, "monospace") == 0);
    case 's':
    case 'S':
      return (g_ascii_strcasecmp (family_name, "sans") == 0 ||
	      g_ascii_strcasecmp (family_name, "serif") == 0);
    }

  return FALSE;
}

static void
pango_fc_font_map_list_families (PangoFontMap      *fontmap,
				 PangoFontFamily ***families,
				 int               *n_families)
{
  PangoFcFontMap *fcfontmap = PANGO_FC_FONT_MAP (fontmap);
  PangoFcFontMapPrivate *priv = fcfontmap->priv;
  FcFontSet *fontset;
  int i;
  int count;

  if (priv->closed)
    {
      if (families)
	*families = NULL;
      if (n_families)
	*n_families = 0;

      return;
    }

  if (priv->n_families < 0)
    {
      FcObjectSet *os = FcObjectSetBuild (FC_FAMILY, FC_SPACING, NULL);
      FcPattern *pat = FcPatternCreate ();
      /* use hash table to avoid duplicate listings if different faces in
       * the same family have different spacing values */
      GHashTable *temp_family_hash;

      fontset = FcFontList (NULL, pat, os);

      FcPatternDestroy (pat);
      FcObjectSetDestroy (os);

      priv->families = g_new (PangoFcFamily *, fontset->nfont + 3); /* 3 standard aliases */
      temp_family_hash = g_hash_table_new_full (g_str_hash, g_str_equal, g_free, NULL);

      count = 0;
      for (i = 0; i < fontset->nfont; i++)
	{
	  char *s;
	  FcResult res;
	  int spacing;

	  res = FcPatternGetString (fontset->fonts[i], FC_FAMILY, 0, (FcChar8 **)(void*)&s);
	  g_assert (res == FcResultMatch);

	  res = FcPatternGetInteger (fontset->fonts[i], FC_SPACING, 0, &spacing);
	  g_assert (res == FcResultMatch || res == FcResultNoMatch);
	  if (res == FcResultNoMatch)
	    spacing = FC_PROPORTIONAL;

	  if (!is_alias_family (s) && !g_hash_table_lookup (temp_family_hash, s))
	    {
	      PangoFcFamily *temp_family = create_family (fcfontmap, s, spacing);
	      g_hash_table_insert (temp_family_hash, g_strdup (s), s);
	      priv->families[count++] = temp_family;
	    }
	}

      FcFontSetDestroy (fontset);
      g_hash_table_destroy (temp_family_hash);

      priv->families[count++] = create_family (fcfontmap, "Sans", FC_PROPORTIONAL);
      priv->families[count++] = create_family (fcfontmap, "Serif", FC_PROPORTIONAL);
      priv->families[count++] = create_family (fcfontmap, "Monospace", FC_MONO);

      priv->n_families = count;
    }

  if (n_families)
    *n_families = priv->n_families;

  if (families)
    *families = g_memdup (priv->families, priv->n_families * sizeof (PangoFontFamily *));
}

static int
pango_fc_convert_weight_to_fc (PangoWeight pango_weight)
{
  if (pango_weight <= (PANGO_WEIGHT_THIN + PANGO_WEIGHT_ULTRALIGHT) / 2)
    return FC_WEIGHT_THIN;
  else if (pango_weight <= (PANGO_WEIGHT_ULTRALIGHT + PANGO_WEIGHT_LIGHT) / 2)
    return FC_WEIGHT_ULTRALIGHT;
  else if (pango_weight <= (PANGO_WEIGHT_LIGHT + PANGO_WEIGHT_BOOK) / 2)
    return FC_WEIGHT_LIGHT;
  else if (pango_weight <= (PANGO_WEIGHT_BOOK + PANGO_WEIGHT_NORMAL) / 2)
    return FC_WEIGHT_BOOK;
  else if (pango_weight <= (PANGO_WEIGHT_NORMAL + PANGO_WEIGHT_MEDIUM) / 2)
    return FC_WEIGHT_NORMAL;
  else if (pango_weight <= (PANGO_WEIGHT_MEDIUM + PANGO_WEIGHT_SEMIBOLD) / 2)
    return FC_WEIGHT_MEDIUM;
  else if (pango_weight <= (PANGO_WEIGHT_SEMIBOLD + PANGO_WEIGHT_BOLD) / 2)
    return FC_WEIGHT_DEMIBOLD;
  else if (pango_weight <= (PANGO_WEIGHT_BOLD + PANGO_WEIGHT_ULTRABOLD) / 2)
    return FC_WEIGHT_BOLD;
  else if (pango_weight <= (PANGO_WEIGHT_ULTRABOLD + PANGO_WEIGHT_HEAVY) / 2)
    return FC_WEIGHT_ULTRABOLD;
  else if (pango_weight <= (PANGO_WEIGHT_HEAVY + PANGO_WEIGHT_ULTRAHEAVY) / 2)
    return FC_WEIGHT_BLACK;
  else
    return FC_WEIGHT_EXTRABLACK;
}

static int
pango_fc_convert_slant_to_fc (PangoStyle pango_style)
{
  switch (pango_style)
    {
    case PANGO_STYLE_NORMAL:
      return FC_SLANT_ROMAN;
    case PANGO_STYLE_ITALIC:
      return FC_SLANT_ITALIC;
    case PANGO_STYLE_OBLIQUE:
      return FC_SLANT_OBLIQUE;
    default:
      return FC_SLANT_ROMAN;
    }
}

#ifdef FC_WIDTH
static int
pango_fc_convert_width_to_fc (PangoStretch pango_stretch)
{
  switch (pango_stretch)
    {
    case PANGO_STRETCH_NORMAL:
      return FC_WIDTH_NORMAL;
    case PANGO_STRETCH_ULTRA_CONDENSED:
      return FC_WIDTH_ULTRACONDENSED;
    case PANGO_STRETCH_EXTRA_CONDENSED:
      return FC_WIDTH_EXTRACONDENSED;
    case PANGO_STRETCH_CONDENSED:
      return FC_WIDTH_CONDENSED;
    case PANGO_STRETCH_SEMI_CONDENSED:
      return FC_WIDTH_SEMICONDENSED;
    case PANGO_STRETCH_SEMI_EXPANDED:
      return FC_WIDTH_SEMIEXPANDED;
    case PANGO_STRETCH_EXPANDED:
      return FC_WIDTH_EXPANDED;
    case PANGO_STRETCH_EXTRA_EXPANDED:
      return FC_WIDTH_EXTRAEXPANDED;
    case PANGO_STRETCH_ULTRA_EXPANDED:
      return FC_WIDTH_ULTRAEXPANDED;
    default:
      return FC_WIDTH_NORMAL;
    }
}
#endif

static FcPattern *
pango_fc_make_pattern (const  PangoFontDescription *description,
		       PangoLanguage               *language,
		       int                          pixel_size,
		       double                       dpi)
{
  FcPattern *pattern;
  const char *prgname;
  int slant;
  int weight;
  PangoGravity gravity;
  FcBool vertical;
  char **families;
  int i;
#ifdef FC_WIDTH
  int width;
#endif

  prgname = g_get_prgname ();
  slant = pango_fc_convert_slant_to_fc (pango_font_description_get_style (description));
  weight = pango_fc_convert_weight_to_fc (pango_font_description_get_weight (description));
#ifdef FC_WIDTH
  width = pango_fc_convert_width_to_fc (pango_font_description_get_stretch (description));
#endif

  gravity = pango_font_description_get_gravity (description);
  vertical = PANGO_GRAVITY_IS_VERTICAL (gravity) ? FcTrue : FcFalse;

  /* The reason for passing in FC_SIZE as well as FC_PIXEL_SIZE is
   * to work around a bug in libgnomeprint where it doesn't look
   * for FC_PIXEL_SIZE. See http://bugzilla.gnome.org/show_bug.cgi?id=169020
   *
   * Putting FC_SIZE in here slightly reduces the efficiency
   * of caching of patterns and fonts when working with multiple different
   * dpi values.
   */
  pattern = FcPatternBuild (NULL,
			    PANGO_FC_VERSION, FcTypeInteger, pango_version(),
			    FC_WEIGHT, FcTypeInteger, weight,
			    FC_SLANT,  FcTypeInteger, slant,
#ifdef FC_WIDTH
			    FC_WIDTH,  FcTypeInteger, width,
#endif
#ifdef FC_VERTICAL_LAYOUT
			    FC_VERTICAL_LAYOUT,  FcTypeBool, vertical,
#endif
			    FC_DPI, FcTypeDouble, dpi,
			    FC_SIZE,  FcTypeDouble,  pixel_size * (72. / 1024. / dpi),
			    FC_PIXEL_SIZE,  FcTypeDouble,  pixel_size / 1024.,
			    NULL);

  if (pango_font_description_get_family (description))
    {
      families = g_strsplit (pango_font_description_get_family (description), ",", -1);

      for (i = 0; families[i]; i++)
	FcPatternAddString (pattern, FC_FAMILY, (FcChar8*) families[i]);

      g_strfreev (families);
    }

  if (language)
    FcPatternAddString (pattern, FC_LANG, (FcChar8 *) pango_language_to_string (language));

  if (gravity != PANGO_GRAVITY_SOUTH)
    {
      GEnumValue *value = g_enum_get_value (get_gravity_class (), gravity);
      FcPatternAddString (pattern, PANGO_FC_GRAVITY, (FcChar8*) value->value_nick);
    }

  if (prgname)
    FcPatternAddString (pattern, PANGO_FC_PRGNAME, (FcChar8*) prgname);

  return pattern;
}

static FcPattern *
uniquify_pattern (PangoFcFontMap *fcfontmap,
		  FcPattern      *pattern)
{
  PangoFcFontMapPrivate *priv = fcfontmap->priv;
  FcPattern *old_pattern;

  old_pattern = g_hash_table_lookup (priv->pattern_hash, pattern);
  if (old_pattern)
    {
      return old_pattern;
    }
  else
    {
      FcPatternReference (pattern);
      g_hash_table_insert (priv->pattern_hash, pattern, pattern);
      return pattern;
    }
}

static PangoFont *
pango_fc_font_map_new_font (PangoFcFontMap    *fcfontmap,
			    PangoFcFontsetKey *fontset_key,
			    FcPattern         *match)
{
  PangoFcFontMapClass *class;
  PangoFcFontMapPrivate *priv = fcfontmap->priv;
  FcPattern *pattern;
  PangoFcFont *fcfont;
  PangoFcFontKey key;

  if (priv->closed)
    return NULL;

  match = uniquify_pattern (fcfontmap, match);

  pango_fc_font_key_init (&key, fcfontmap, fontset_key, match);

  fcfont = g_hash_table_lookup (priv->font_hash, &key);
  if (fcfont)
    return g_object_ref (fcfont);

  class = PANGO_FC_FONT_MAP_GET_CLASS (fcfontmap);

  if (class->create_font)
    {
      fcfont = class->create_font (fcfontmap, &key);
    }
  else
    {
      const PangoMatrix *pango_matrix = pango_fc_fontset_key_get_matrix (fontset_key);
      FcMatrix fc_matrix, *fc_matrix_val;
      int i;

      /* Fontconfig has the Y axis pointing up, Pango, down.
       */
      fc_matrix.xx = pango_matrix->xx;
      fc_matrix.xy = - pango_matrix->xy;
      fc_matrix.yx = - pango_matrix->yx;
      fc_matrix.yy = pango_matrix->yy;

      pattern = FcPatternDuplicate (match);

      for (i = 0; FcPatternGetMatrix (pattern, FC_MATRIX, i, &fc_matrix_val) == FcResultMatch; i++)
	FcMatrixMultiply (&fc_matrix, &fc_matrix, fc_matrix_val);

      FcPatternDel (pattern, FC_MATRIX);
      FcPatternAddMatrix (pattern, FC_MATRIX, &fc_matrix);

      fcfont = class->new_font (fcfontmap, uniquify_pattern (fcfontmap, pattern));

      FcPatternDestroy (pattern);
    }

  if (!fcfont)
    return NULL;

  fcfont->matrix = key.matrix;
  /* In case the backend didn't set the fontmap */
  if (!fcfont->fontmap)
    g_object_set (fcfont,
		  "fontmap", fcfontmap,
		  NULL);

  /* cache it on fontmap */
  pango_fc_font_map_add (fcfontmap, &key, fcfont);

  return (PangoFont *)fcfont;
}

static void
pango_fc_default_substitute (PangoFcFontMap    *fontmap,
			     PangoFcFontsetKey *fontsetkey,
			     FcPattern         *pattern)
{
  if (PANGO_FC_FONT_MAP_GET_CLASS (fontmap)->fontset_key_substitute)
    PANGO_FC_FONT_MAP_GET_CLASS (fontmap)->fontset_key_substitute (fontmap, fontsetkey, pattern);
  else if (PANGO_FC_FONT_MAP_GET_CLASS (fontmap)->default_substitute)
    PANGO_FC_FONT_MAP_GET_CLASS (fontmap)->default_substitute (fontmap, pattern);
}

static double
pango_fc_font_map_get_resolution (PangoFcFontMap *fcfontmap,
				  PangoContext   *context)
{
  if (PANGO_FC_FONT_MAP_GET_CLASS (fcfontmap)->get_resolution)
    return PANGO_FC_FONT_MAP_GET_CLASS (fcfontmap)->get_resolution (fcfontmap, context);

  if (fcfontmap->priv->dpi < 0)
    {
      FcResult result = FcResultNoMatch;
      FcPattern *tmp = FcPatternBuild (NULL,
				       FC_FAMILY, FcTypeString, "Sans",
				       FC_SIZE,   FcTypeDouble, 10.,
				       NULL);
      if (tmp)
	{
	  pango_fc_default_substitute (fcfontmap, NULL, tmp);
	  result = FcPatternGetDouble (tmp, FC_DPI, 0, &fcfontmap->priv->dpi);
	  FcPatternDestroy (tmp);
	}

      if (result != FcResultMatch)
	{
	  g_warning ("Error getting DPI from fontconfig, using 72.0");
	  fcfontmap->priv->dpi = 72.0;
	}
    }

  return fcfontmap->priv->dpi;
}

static FcPattern *
pango_fc_fontset_key_make_pattern (PangoFcFontsetKey *key)
{
  return pango_fc_make_pattern (key->desc,
				key->language,
				key->pixelsize,
				key->resolution);
}

static PangoFcPatterns *
pango_fc_font_map_get_patterns (PangoFontMap      *fontmap,
				PangoFcFontsetKey *key)
{
  PangoFcFontMap *fcfontmap = (PangoFcFontMap *)fontmap;
  PangoFcPatterns *patterns;
  FcPattern *pattern;

  pattern = pango_fc_fontset_key_make_pattern (key);
  pango_fc_default_substitute (fcfontmap, key, pattern);

  patterns = pango_fc_patterns_new (pattern, fcfontmap);

  FcPatternDestroy (pattern);

  return patterns;
}

static gboolean
get_first_font (PangoFontset  *fontset G_GNUC_UNUSED,
		PangoFont     *font,
		gpointer       data)
{
  *(PangoFont **)data = font;

  return TRUE;
}

static PangoFont *
pango_fc_font_map_load_font (PangoFontMap               *fontmap,
			     PangoContext               *context,
			     const PangoFontDescription *description)
{
  PangoLanguage *language;
  PangoFontset *fontset;
  PangoFont *font = NULL;

  if (context)
    language = pango_context_get_language (context);
  else
    language = NULL;

  fontset = pango_font_map_load_fontset (fontmap, context, description, language);

  if (fontset)
    {
      pango_fontset_foreach (fontset, get_first_font, &font);

      if (font)
	g_object_ref (font);

      g_object_unref (fontset);
    }

  return font;
}

static void
pango_fc_fontset_cache (PangoFcFontset *fontset,
			PangoFcFontMap *fcfontmap)
{
  PangoFcFontMapPrivate *priv = fcfontmap->priv;
  GQueue *cache = priv->fontset_cache;

  if (fontset->cache_link)
    {
      if (fontset->cache_link == cache->head)
        return;

      /* Already in cache, move to head
       */
      if (fontset->cache_link == cache->tail)
	cache->tail = fontset->cache_link->prev;

      cache->head = g_list_remove_link (cache->head, fontset->cache_link);
      cache->length--;
    }
  else
    {
      /* Add to cache initially
       */
      if (cache->length == FONTSET_CACHE_SIZE)
	{
	  PangoFcFontset *tmp_fontset = g_queue_pop_tail (cache);
	  tmp_fontset->cache_link = NULL;
	  g_hash_table_remove (priv->fontset_hash, tmp_fontset->key);
	}

      fontset->cache_link = g_list_prepend (NULL, fontset);
    }

  g_queue_push_head_link (cache, fontset->cache_link);
}

static PangoFontset *
pango_fc_font_map_load_fontset (PangoFontMap                 *fontmap,
				PangoContext                 *context,
				const PangoFontDescription   *desc,
				PangoLanguage                *language)
{
  PangoFcFontMap *fcfontmap = (PangoFcFontMap *)fontmap;
  PangoFcFontMapPrivate *priv = fcfontmap->priv;
  PangoFcFontset *fontset;
  PangoFcFontsetKey key;

  pango_fc_fontset_key_init (&key, fcfontmap, context, desc, language);

  fontset = g_hash_table_lookup (priv->fontset_hash, &key);

  if (G_UNLIKELY (!fontset))
    {
      PangoFcPatterns *patterns = pango_fc_font_map_get_patterns (fontmap, &key);

      if (!patterns)
	return NULL;

      fontset = pango_fc_fontset_new (&key, patterns);
      g_hash_table_insert (priv->fontset_hash, pango_fc_fontset_get_key (fontset), fontset);

      pango_fc_patterns_unref (patterns);
    }

  pango_fc_fontset_cache (fontset, fcfontmap);

  pango_font_description_free (key.desc);

  return g_object_ref (fontset);
}

/**
 * pango_fc_font_map_cache_clear:
 * @fcfontmap: a #PangoFcFontMap
 *
 * Clear all cached information and fontsets for this font map;
 * this should be called whenever there is a change in the
 * output of the default_substitute() virtual function of the
 * font map, or if fontconfig has been reinitialized to new
 * configuration.
 *
 * Since: 1.4
 **/
void
pango_fc_font_map_cache_clear (PangoFcFontMap *fcfontmap)
{
  if (G_UNLIKELY (fcfontmap->priv->closed))
    return;

  pango_fc_font_map_fini (fcfontmap);
  pango_fc_font_map_init (fcfontmap);
}

static PangoFcFontFaceData *
pango_fc_font_map_get_font_face_data (PangoFcFontMap *fcfontmap,
				      FcPattern      *font_pattern)
{
  PangoFcFontMapPrivate *priv = fcfontmap->priv;
  PangoFcFontFaceData key;
  PangoFcFontFaceData *data;

  if (FcPatternGetString (font_pattern, FC_FILE, 0, (FcChar8 **)(void*)&key.filename) != FcResultMatch)
    return NULL;

  if (FcPatternGetInteger (font_pattern, FC_INDEX, 0, &key.id) != FcResultMatch)
    return NULL;

  data = g_hash_table_lookup (priv->font_face_data_hash, &key);
  if (G_LIKELY (data))
    return data;

  data = g_slice_new0 (PangoFcFontFaceData);
  data->filename = key.filename;
  data->id = key.id;

  data->pattern = font_pattern;
  FcPatternReference (data->pattern);

  g_hash_table_insert (priv->font_face_data_hash, data, data);

  return data;
}

static PangoFcCmapCache *
_pango_fc_cmap_cache_ref (PangoFcCmapCache *cmap_cache)
{
  g_atomic_int_inc ((int *) &cmap_cache->ref_count);

  return cmap_cache;
}

void
_pango_fc_cmap_cache_unref (PangoFcCmapCache *cmap_cache)
{
  g_return_if_fail (cmap_cache->ref_count > 0);

  if (g_atomic_int_dec_and_test ((int *) &cmap_cache->ref_count))
    {
      g_free (cmap_cache);
    }
}

PangoFcCmapCache *
_pango_fc_font_map_get_cmap_cache (PangoFcFontMap *fcfontmap,
				   PangoFcFont    *fcfont)
{
  PangoFcFontFaceData *data;

  if (G_UNLIKELY (fcfontmap == NULL))
	return NULL;

  if (G_UNLIKELY (!fcfont->font_pattern))
    return NULL;

  data = pango_fc_font_map_get_font_face_data (fcfontmap, fcfont->font_pattern);
  if (G_UNLIKELY (!data))
    return NULL;

  if (G_UNLIKELY (data->cmap_cache == NULL))
    {
      data->cmap_cache = g_new0 (PangoFcCmapCache, 1);
      data->cmap_cache->ref_count = 1;

      /* Make sure all cache entries are invalid initially */
      data->cmap_cache->entries[0].ch = 1; /* char 1 cannot happen in bucket 0 */
    }

  return _pango_fc_cmap_cache_ref (data->cmap_cache);
}

PangoCoverage *
_pango_fc_font_map_get_coverage (PangoFcFontMap *fcfontmap,
				 PangoFcFont    *fcfont)
{
  PangoFcFontFaceData *data;
  FcCharSet *charset;

  if (G_UNLIKELY (!fcfont->font_pattern))
    return NULL;

  data = pango_fc_font_map_get_font_face_data (fcfontmap, fcfont->font_pattern);
  if (G_UNLIKELY (!data))
    return NULL;

  if (G_UNLIKELY (data->coverage == NULL))
    {
      /*
       * Pull the coverage out of the pattern, this
       * doesn't require loading the font
       */
      if (FcPatternGetCharSet (fcfont->font_pattern, FC_CHARSET, 0, &charset) != FcResultMatch)
	return NULL;

      data->coverage = _pango_fc_font_map_fc_to_coverage (charset);
    }

  return pango_coverage_ref (data->coverage);
}

/**
 * _pango_fc_font_map_fc_to_coverage:
 * @charset: #FcCharSet to convert to a #PangoCoverage object.
 *
 * Convert the given #FcCharSet into a new #PangoCoverage object.  The
 * caller is responsible for freeing the newly created object.
 *
 * Since: 1.6
 **/
PangoCoverage  *
_pango_fc_font_map_fc_to_coverage (FcCharSet *charset)
{
  PangoCoverage *coverage;
  FcChar32  ucs4, pos;
  FcChar32  map[FC_CHARSET_MAP_SIZE];
  int i;

  /*
   * Convert an Fc CharSet into a pango coverage structure.  Sure
   * would be nice to just use the Fc structure in place...
   */
  coverage = pango_coverage_new ();
  for (ucs4 = FcCharSetFirstPage (charset, map, &pos);
       ucs4 != FC_CHARSET_DONE;
       ucs4 = FcCharSetNextPage (charset, map, &pos))
    {
      for (i = 0; i < FC_CHARSET_MAP_SIZE; i++)
	{
	  FcChar32  bits = map[i];
	  FcChar32  base = ucs4 + i * 32;
	  int b = 0;

	  while (bits)
	    {
	      if (bits & 1)
		pango_coverage_set (coverage, base + b, PANGO_COVERAGE_EXACT);

	      bits >>= 1;
	      b++;
	    }
	}
    }

  /* Awful hack so Hangul Tone marks get rendered with the same
   * font and in the same run as other Hangul characters. If a font
   * covers the first composed Hangul glyph, then it is declared to cover
   * the Hangul tone marks. This hack probably needs to be formalized
   * by choosing fonts for scripts rather than individual code points.
   */
  if (pango_coverage_get (coverage, 0xac00) == PANGO_COVERAGE_EXACT)
    {
      pango_coverage_set (coverage, 0x302e, PANGO_COVERAGE_EXACT);
      pango_coverage_set (coverage, 0x302f, PANGO_COVERAGE_EXACT);
    }

  return coverage;
}

/**
 * pango_fc_font_map_create_context:
 * @fcfontmap: a #PangoFcFontMap
 *
 * Creates a new context for this fontmap. This function is intended
 * only for backend implementations deriving from #PangoFcFontMap;
 * it is possible that a backend will store additional information
 * needed for correct operation on the #PangoContext after calling
 * this function.
 *
 * Return value: a new #PangoContext
 *
 * Since: 1.4
 *
 * Deprecated: 1.22: Use pango_font_map_create_context() instead.
 **/
PangoContext *
pango_fc_font_map_create_context (PangoFcFontMap *fcfontmap)
{
  g_return_val_if_fail (PANGO_IS_FC_FONT_MAP (fcfontmap), NULL);

  return pango_font_map_create_context (PANGO_FONT_MAP (fcfontmap));
}

static void
shutdown_font (gpointer        key,
	       PangoFcFont    *fcfont,
	       PangoFcFontMap *fcfontmap)
{
  _pango_fc_font_shutdown (fcfont);

  _pango_fc_font_set_font_key (fcfont, NULL);
  pango_fc_font_key_free (key);
}

/**
 * pango_fc_font_map_shutdown:
 * @fcfontmap: a #PangoFcFontMap
 *
 * Clears all cached information for the fontmap and marks
 * all fonts open for the fontmap as dead. (See the shutdown()
 * virtual function of #PangoFcFont.) This function might be used
 * by a backend when the underlying windowing system for the font
 * map exits. This function is only intended to be called
 * only for backend implementations deriving from #PangoFcFontMap.
 *
 * Since: 1.4
 **/
void
pango_fc_font_map_shutdown (PangoFcFontMap *fcfontmap)
{
  PangoFcFontMapPrivate *priv = fcfontmap->priv;
  int i;

  if (priv->closed)
    return;

  g_hash_table_foreach (priv->font_hash, (GHFunc) shutdown_font, fcfontmap);
  for (i = 0; i < priv->n_families; i++)
    priv->families[i]->fontmap = NULL;

  pango_fc_font_map_fini (fcfontmap);

  while (priv->findfuncs)
    {
      PangoFcFindFuncInfo *info;
      info = priv->findfuncs->data;
      if (info->dnotify)
	info->dnotify (info->user_data);

      g_slice_free (PangoFcFindFuncInfo, info);
      priv->findfuncs = g_slist_delete_link (priv->findfuncs, priv->findfuncs);
    }

  priv->closed = TRUE;
}

static PangoWeight
pango_fc_convert_weight_to_pango (int fc_weight)
{
  if (fc_weight <= (FC_WEIGHT_THIN + FC_WEIGHT_EXTRALIGHT) / 2)
    return PANGO_WEIGHT_THIN;
  else if (fc_weight <= (FC_WEIGHT_EXTRALIGHT + FC_WEIGHT_LIGHT) / 2)
    return PANGO_WEIGHT_ULTRALIGHT;
  else if (fc_weight <= (FC_WEIGHT_LIGHT + FC_WEIGHT_BOOK) / 2)
    return PANGO_WEIGHT_LIGHT;
  else if (fc_weight <= (FC_WEIGHT_BOOK + FC_WEIGHT_REGULAR) / 2)
    return PANGO_WEIGHT_BOOK;
  else if (fc_weight <= (FC_WEIGHT_REGULAR + FC_WEIGHT_MEDIUM) / 2)
    return PANGO_WEIGHT_NORMAL;
   else if (fc_weight <= (FC_WEIGHT_MEDIUM + FC_WEIGHT_DEMIBOLD) / 2)
    return PANGO_WEIGHT_MEDIUM;
  else if (fc_weight <= (FC_WEIGHT_DEMIBOLD + FC_WEIGHT_BOLD) / 2)
    return PANGO_WEIGHT_SEMIBOLD;
  else if (fc_weight <= (FC_WEIGHT_BOLD + FC_WEIGHT_EXTRABOLD) / 2)
    return PANGO_WEIGHT_BOLD;
  else if (fc_weight <= (FC_WEIGHT_EXTRABOLD + FC_WEIGHT_BLACK) / 2)
    return PANGO_WEIGHT_ULTRABOLD;
  else if (fc_weight <= (FC_WEIGHT_BLACK + FC_WEIGHT_EXTRABLACK) / 2)
    return PANGO_WEIGHT_HEAVY;
  else
    return PANGO_WEIGHT_ULTRAHEAVY;
}

static PangoStyle
pango_fc_convert_slant_to_pango (int fc_style)
{
  switch (fc_style)
    {
    case FC_SLANT_ROMAN:
      return PANGO_STYLE_NORMAL;
    case FC_SLANT_ITALIC:
      return PANGO_STYLE_ITALIC;
    case FC_SLANT_OBLIQUE:
      return PANGO_STYLE_OBLIQUE;
    default:
      return PANGO_STYLE_NORMAL;
    }
}

#ifdef FC_WIDTH
static PangoStretch
pango_fc_convert_width_to_pango (int fc_stretch)
{
  switch (fc_stretch)
    {
    case FC_WIDTH_NORMAL:
      return PANGO_STRETCH_NORMAL;
    case FC_WIDTH_ULTRACONDENSED:
      return PANGO_STRETCH_ULTRA_CONDENSED;
    case FC_WIDTH_EXTRACONDENSED:
      return PANGO_STRETCH_EXTRA_CONDENSED;
    case FC_WIDTH_CONDENSED:
      return PANGO_STRETCH_CONDENSED;
    case FC_WIDTH_SEMICONDENSED:
      return PANGO_STRETCH_SEMI_CONDENSED;
    case FC_WIDTH_SEMIEXPANDED:
      return PANGO_STRETCH_SEMI_EXPANDED;
    case FC_WIDTH_EXPANDED:
      return PANGO_STRETCH_EXPANDED;
    case FC_WIDTH_EXTRAEXPANDED:
      return PANGO_STRETCH_EXTRA_EXPANDED;
    case FC_WIDTH_ULTRAEXPANDED:
      return PANGO_STRETCH_ULTRA_EXPANDED;
    default:
      return PANGO_STRETCH_NORMAL;
    }
}
#endif

/**
 * pango_fc_font_description_from_pattern:
 * @pattern: a #FcPattern
 * @include_size: if %TRUE, the pattern will include the size from
 *   the @pattern; otherwise the resulting pattern will be unsized.
 *   (only %FC_SIZE is examined, not %FC_PIXEL_SIZE)
 *
 * Creates a #PangoFontDescription that matches the specified
 * Fontconfig pattern as closely as possible. Many possible Fontconfig
 * pattern values, such as %FC_RASTERIZER or %FC_DPI, don't make sense in
 * the context of #PangoFontDescription, so will be ignored.
 *
 * Return value: a new #PangoFontDescription. Free with
 *  pango_font_description_free().
 *
 * Since: 1.4
 **/
PangoFontDescription *
pango_fc_font_description_from_pattern (FcPattern *pattern, gboolean include_size)
{
  PangoFontDescription *desc;
  PangoStyle style;
  PangoWeight weight;
  PangoStretch stretch;
  double size;
  PangoGravity gravity;

  FcChar8 *s;
  int i;
  FcResult res;

  desc = pango_font_description_new ();

  res = FcPatternGetString (pattern, FC_FAMILY, 0, (FcChar8 **) &s);
  g_assert (res == FcResultMatch);

  pango_font_description_set_family (desc, (gchar *)s);

  if (FcPatternGetInteger (pattern, FC_SLANT, 0, &i) == FcResultMatch)
    style = pango_fc_convert_slant_to_pango (i);
  else
    style = PANGO_STYLE_NORMAL;

  pango_font_description_set_style (desc, style);

  if (FcPatternGetInteger (pattern, FC_WEIGHT, 0, &i) == FcResultMatch)
    weight = pango_fc_convert_weight_to_pango (i);
  else
    weight = PANGO_WEIGHT_NORMAL;

  pango_font_description_set_weight (desc, weight);

#ifdef FC_WIDTH
  if (FcPatternGetInteger (pattern, FC_WIDTH, 0, &i) == FcResultMatch)
    stretch = pango_fc_convert_width_to_pango (i);
  else
#endif
    stretch = PANGO_STRETCH_NORMAL;

  pango_font_description_set_stretch (desc, stretch);

  pango_font_description_set_variant (desc, PANGO_VARIANT_NORMAL);

  if (include_size && FcPatternGetDouble (pattern, FC_SIZE, 0, &size) == FcResultMatch)
    pango_font_description_set_size (desc, size * PANGO_SCALE);

  /* gravity is a bit different.  we don't want to set it if it was not set on
   * the pattern */
  if (FcPatternGetString (pattern, PANGO_FC_GRAVITY, 0, (FcChar8 **)&s) == FcResultMatch)
    {
      GEnumValue *value = g_enum_get_value_by_nick (get_gravity_class (), (char *)s);
      gravity = value->value;

      pango_font_description_set_gravity (desc, gravity);
    }

  return desc;
}

/*
 * PangoFcFace
 */

typedef PangoFontFaceClass PangoFcFaceClass;
G_DEFINE_TYPE (PangoFcFace, pango_fc_face, PANGO_TYPE_FONT_FACE);

static PangoFontDescription *
make_alias_description (PangoFcFamily *fcfamily,
			gboolean        bold,
			gboolean        italic)
{
  PangoFontDescription *desc = pango_font_description_new ();

  pango_font_description_set_family (desc, fcfamily->family_name);
  pango_font_description_set_style (desc, italic ? PANGO_STYLE_ITALIC : PANGO_STYLE_NORMAL);
  pango_font_description_set_weight (desc, bold ? PANGO_WEIGHT_BOLD : PANGO_WEIGHT_NORMAL);

  return desc;
}

static PangoFontDescription *
pango_fc_face_describe (PangoFontFace *face)
{
  PangoFcFace *fcface = PANGO_FC_FACE (face);
  PangoFcFamily *fcfamily = fcface->family;
  PangoFontDescription *desc = NULL;
  FcResult res;
  FcPattern *match_pattern;
  FcPattern *result_pattern;

  if (G_UNLIKELY (!fcfamily))
    return pango_font_description_new ();

  if (fcface->fake)
    {
      if (strcmp (fcface->style, "Regular") == 0)
	return make_alias_description (fcfamily, FALSE, FALSE);
      else if (strcmp (fcface->style, "Bold") == 0)
	return make_alias_description (fcfamily, TRUE, FALSE);
      else if (strcmp (fcface->style, "Italic") == 0)
	return make_alias_description (fcfamily, FALSE, TRUE);
      else			/* Bold Italic */
	return make_alias_description (fcfamily, TRUE, TRUE);
    }

  match_pattern = FcPatternBuild (NULL,
				  FC_FAMILY, FcTypeString, fcfamily->family_name,
				  FC_STYLE, FcTypeString, fcface->style,
				  NULL);

  g_assert (match_pattern);

  FcConfigSubstitute (NULL, match_pattern, FcMatchPattern);
  FcDefaultSubstitute (match_pattern);

  result_pattern = FcFontMatch (NULL, match_pattern, &res);
  if (result_pattern)
    {
      desc = pango_fc_font_description_from_pattern (result_pattern, FALSE);
      FcPatternDestroy (result_pattern);
    }

  FcPatternDestroy (match_pattern);

  return desc;
}

static const char *
pango_fc_face_get_face_name (PangoFontFace *face)
{
  PangoFcFace *fcface = PANGO_FC_FACE (face);

  return fcface->style;
}

static int
compare_ints (gconstpointer ap,
	      gconstpointer bp)
{
  int a = *(int *)ap;
  int b = *(int *)bp;

  if (a == b)
    return 0;
  else if (a > b)
    return 1;
  else
    return -1;
}

static void
pango_fc_face_list_sizes (PangoFontFace  *face,
			  int           **sizes,
			  int            *n_sizes)
{
  PangoFcFace *fcface = PANGO_FC_FACE (face);
  FcPattern *pattern;
  FcFontSet *fontset;
  FcObjectSet *objectset;

  *sizes = NULL;
  *n_sizes = 0;
  if (G_UNLIKELY (!fcface->family || !fcface->family->fontmap))
    return;

  pattern = FcPatternCreate ();
  FcPatternAddString (pattern, FC_FAMILY, (FcChar8*)(void*)fcface->family->family_name);
  FcPatternAddString (pattern, FC_STYLE, (FcChar8*)(void*)fcface->style);

  objectset = FcObjectSetCreate ();
  FcObjectSetAdd (objectset, FC_PIXEL_SIZE);

  fontset = FcFontList (NULL, pattern, objectset);

  if (fontset)
    {
      GArray *size_array;
      double size, dpi = -1.0;
      int i, size_i;

      size_array = g_array_new (FALSE, FALSE, sizeof (int));

      for (i = 0; i < fontset->nfont; i++)
	{
	  if (FcPatternGetDouble (fontset->fonts[i], FC_PIXEL_SIZE, 0, &size) == FcResultMatch)
	    {
	      if (dpi < 0)
		dpi = pango_fc_font_map_get_resolution (fcface->family->fontmap, NULL);

	      size_i = (int) (PANGO_SCALE * size * 72.0 / dpi);
	      g_array_append_val (size_array, size_i);
	    }
	}

      g_array_sort (size_array, compare_ints);

      if (size_array->len == 0)
	{
	  *n_sizes = 0;
	  if (sizes)
	    *sizes = NULL;
	  g_array_free (size_array, TRUE);
	}
      else
	{
	  *n_sizes = size_array->len;
	  if (sizes)
	    {
	      *sizes = (int *) size_array->data;
	      g_array_free (size_array, FALSE);
	    }
	  else
	    g_array_free (size_array, TRUE);
	}

      FcFontSetDestroy (fontset);
    }
  else
    {
      *n_sizes = 0;
      if (sizes)
	*sizes = NULL;
    }

  FcPatternDestroy (pattern);
  FcObjectSetDestroy (objectset);
}

static gboolean
pango_fc_face_is_synthesized (PangoFontFace *face)
{
  PangoFcFace *fcface = PANGO_FC_FACE (face);

  return fcface->fake;
}

static void
pango_fc_face_finalize (GObject *object)
{
  PangoFcFace *fcface = PANGO_FC_FACE (object);

  g_free (fcface->style);

  G_OBJECT_CLASS (pango_fc_face_parent_class)->finalize (object);
}

static void
pango_fc_face_init (PangoFcFace *self)
{
}

static void
pango_fc_face_class_init (PangoFcFaceClass *class)
{
  GObjectClass *object_class = G_OBJECT_CLASS (class);

  object_class->finalize = pango_fc_face_finalize;

  class->describe = pango_fc_face_describe;
  class->get_face_name = pango_fc_face_get_face_name;
  class->list_sizes = pango_fc_face_list_sizes;
  class->is_synthesized = pango_fc_face_is_synthesized;
}


/*
 * PangoFcFamily
 */

typedef PangoFontFamilyClass PangoFcFamilyClass;
G_DEFINE_TYPE (PangoFcFamily, pango_fc_family, PANGO_TYPE_FONT_FAMILY);

static PangoFcFace *
create_face (PangoFcFamily *fcfamily,
	     const char     *style,
	     gboolean       fake)
{
  PangoFcFace *face = g_object_new (PANGO_FC_TYPE_FACE, NULL);
  face->style = g_strdup (style);
  face->family = fcfamily;
  face->fake = fake;

  return face;
}

static void
pango_fc_family_list_faces (PangoFontFamily  *family,
			    PangoFontFace  ***faces,
			    int              *n_faces)
{
  PangoFcFamily *fcfamily = PANGO_FC_FAMILY (family);
  PangoFcFontMap *fcfontmap = fcfamily->fontmap;
  PangoFcFontMapPrivate *priv;

  *faces = NULL;
  *n_faces = 0;
  if (G_UNLIKELY (!fcfontmap))
    return;

  priv = fcfontmap->priv;

  if (fcfamily->n_faces < 0)
    {
      FcFontSet *fontset;
      int i;

      if (is_alias_family (fcfamily->family_name) || priv->closed)
	{
	  fcfamily->n_faces = 4;
	  fcfamily->faces = g_new (PangoFcFace *, fcfamily->n_faces);

	  i = 0;
	  fcfamily->faces[i++] = create_face (fcfamily, "Regular", TRUE);
	  fcfamily->faces[i++] = create_face (fcfamily, "Bold", TRUE);
	  fcfamily->faces[i++] = create_face (fcfamily, "Italic", TRUE);
	  fcfamily->faces[i++] = create_face (fcfamily, "Bold Italic", TRUE);
	}
      else
	{
	  FcObjectSet *os = FcObjectSetBuild (FC_STYLE, FC_WEIGHT, FC_SLANT, NULL);
	  FcPattern *pat = FcPatternBuild (NULL,
					   FC_FAMILY, FcTypeString, fcfamily->family_name,
					   NULL);

	  enum {
	    REGULAR,
	    ITALIC,
	    BOLD,
	    BOLD_ITALIC
	  };
	  /* Regular, Italic, Bold, Bold Italic */
	  gboolean has_face [4] = { FALSE, FALSE, FALSE, FALSE };
	  PangoFcFace **faces;
	  gint num = 0;

	  fontset = FcFontList (NULL, pat, os);

	  FcPatternDestroy (pat);
	  FcObjectSetDestroy (os);

	  /* at most we have 3 additional artifical faces */
	  faces = g_new (PangoFcFace *, fontset->nfont + 3);

	  for (i = 0; i < fontset->nfont; i++)
	    {
	      const char *style, *font_style = NULL;
	      int weight, slant;

	      if (FcPatternGetInteger(fontset->fonts[i], FC_WEIGHT, 0, &weight) != FcResultMatch)
		weight = FC_WEIGHT_MEDIUM;

	      if (FcPatternGetInteger(fontset->fonts[i], FC_SLANT, 0, &slant) != FcResultMatch)
		slant = FC_SLANT_ROMAN;

	      if (FcPatternGetString (fontset->fonts[i], FC_STYLE, 0, (FcChar8 **)(void*)&font_style) != FcResultMatch)
		font_style = NULL;

	      if (weight <= FC_WEIGHT_MEDIUM)
		{
		  if (slant == FC_SLANT_ROMAN)
		    {
		      has_face[REGULAR] = TRUE;
		      style = "Regular";
		    }
		  else
		    {
		      has_face[ITALIC] = TRUE;
		      style = "Italic";
		    }
		}
	      else
		{
		  if (slant == FC_SLANT_ROMAN)
		    {
		      has_face[BOLD] = TRUE;
		      style = "Bold";
		    }
		  else
		    {
		      has_face[BOLD_ITALIC] = TRUE;
		      style = "Bold Italic";
		    }
		}

	      if (!font_style)
		font_style = style;
	      faces[num++] = create_face (fcfamily, font_style, FALSE);
	    }

	  if (has_face[REGULAR])
	    {
	      if (!has_face[ITALIC])
		faces[num++] = create_face (fcfamily, "Italic", TRUE);
	      if (!has_face[BOLD])
		faces[num++] = create_face (fcfamily, "Bold", TRUE);

	    }
	  if ((has_face[REGULAR] || has_face[ITALIC] || has_face[BOLD]) && !has_face[BOLD_ITALIC])
	    faces[num++] = create_face (fcfamily, "Bold Italic", TRUE);

	  faces = g_renew (PangoFcFace *, faces, num);

	  fcfamily->n_faces = num;
	  fcfamily->faces = faces;

	  FcFontSetDestroy (fontset);
	}
    }

  if (n_faces)
    *n_faces = fcfamily->n_faces;

  if (faces)
    *faces = g_memdup (fcfamily->faces, fcfamily->n_faces * sizeof (PangoFontFace *));
}

static const char *
pango_fc_family_get_name (PangoFontFamily  *family)
{
  PangoFcFamily *fcfamily = PANGO_FC_FAMILY (family);

  return fcfamily->family_name;
}

static gboolean
pango_fc_family_is_monospace (PangoFontFamily *family)
{
  PangoFcFamily *fcfamily = PANGO_FC_FAMILY (family);

  return fcfamily->spacing == FC_MONO ||
#ifdef FC_DUAL
	 fcfamily->spacing == FC_DUAL ||
#endif
	 fcfamily->spacing == FC_CHARCELL;
}

static void
pango_fc_family_finalize (GObject *object)
{
  int i;
  PangoFcFamily *fcfamily = PANGO_FC_FAMILY (object);

  g_free (fcfamily->family_name);

  for (i = 0; i < fcfamily->n_faces; i++)
    {
      fcfamily->faces[i]->family = NULL;
      g_object_unref (fcfamily->faces[i]);
    }
  g_free (fcfamily->faces);

  G_OBJECT_CLASS (pango_fc_family_parent_class)->finalize (object);
}

static void
pango_fc_family_class_init (PangoFcFamilyClass *class)
{
  GObjectClass *object_class = G_OBJECT_CLASS (class);

  object_class->finalize = pango_fc_family_finalize;

  class->list_faces = pango_fc_family_list_faces;
  class->get_name = pango_fc_family_get_name;
  class->is_monospace = pango_fc_family_is_monospace;
}

static void
pango_fc_family_init (PangoFcFamily *fcfamily)
{
  fcfamily->n_faces = -1;
}
