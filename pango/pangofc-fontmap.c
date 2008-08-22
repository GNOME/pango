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

/* Size of fontset cache */
#define FONTSET_CACHE_SIZE 64

#include "config.h"
#include <math.h>

#include "pango-context.h"
#include "pangofc-fontmap.h"
#include "pangofc-private.h"
#include "pango-impl-utils.h"
#include "modules.h"
#include "pango-enum-types.h"

typedef struct _PangoFcCoverageKey  PangoFcCoverageKey;
typedef struct _PangoFcFace         PangoFcFace;
typedef struct _PangoFcFamily       PangoFcFamily;
typedef struct _PangoFcPatternSet   PangoFcPatternSet;
typedef struct _PangoFcFindFuncInfo PangoFcFindFuncInfo;
typedef struct _FontsetHashKey      FontsetHashKey;
typedef struct _FontHashKey         FontHashKey;

#define PANGO_FC_TYPE_FAMILY              (pango_fc_family_get_type ())
#define PANGO_FC_FAMILY(object)           (G_TYPE_CHECK_INSTANCE_CAST ((object), PANGO_FC_TYPE_FAMILY, PangoFcFamily))
#define PANGO_FC_IS_FAMILY(object)        (G_TYPE_CHECK_INSTANCE_TYPE ((object), PANGO_FC_TYPE_FAMILY))

#define PANGO_FC_TYPE_FACE              (pango_fc_face_get_type ())
#define PANGO_FC_FACE(object)           (G_TYPE_CHECK_INSTANCE_CAST ((object), PANGO_FC_TYPE_FACE, PangoFcFace))
#define PANGO_FC_IS_FACE(object)        (G_TYPE_CHECK_INSTANCE_TYPE ((object), PANGO_FC_TYPE_FACE))

struct _PangoFcFontMapPrivate
{
  GHashTable *fontset_hash;	/* Maps PangoFontDescription -> PangoFcPatternSet  */

  /* pattern_hash is used to make sure we only store one copy of
   * each identical pattern. (Speeds up lookup).
   */
  GHashTable *pattern_hash;
  GHashTable *coverage_hash; /* Maps font file name/id -> PangoCoverage */

  GHashTable *font_hash; /* Maps FcPattern -> PangoFcFont */

  GQueue *fontset_cache;	/* Recently used fontsets */

  /* List of all families availible */
  PangoFcFamily **families;
  int n_families;		/* -1 == uninitialized */

  double dpi;

  /* Decoders */
  GSList *findfuncs;

  guint closed : 1;
};

struct _PangoFcCoverageKey
{
  char *filename;
  int id;            /* needed to handle TTC files with multiple faces */
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

struct _PangoFcPatternSet
{
  int n_patterns;
  FcPattern **patterns;
  PangoFontset *fontset;
  GList *cache_link;

  FontsetHashKey *key;
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

static void pango_fc_font_map_cache_fontset (PangoFcFontMap    *fcfontmap,
					     PangoFcPatternSet *patterns);

static void pango_fc_pattern_set_free      (PangoFcPatternSet *patterns);

static guint    pango_fc_pattern_hash       (FcPattern           *pattern);
static gboolean pango_fc_pattern_equal      (FcPattern           *pattern1,
					      FcPattern           *pattern2);
static guint    pango_fc_coverage_key_hash  (PangoFcCoverageKey *key);
static gboolean pango_fc_coverage_key_equal (PangoFcCoverageKey *key1,
					      PangoFcCoverageKey *key2);

static guint    font_hash_key_hash  (const FontHashKey *key);
static gboolean font_hash_key_equal (const FontHashKey *key_a,
				     const FontHashKey *key_b);
static void     font_hash_key_free  (FontHashKey       *key);

static guint    fontset_hash_key_hash  (const FontsetHashKey *key);
static gboolean fontset_hash_key_equal (const FontsetHashKey *key_a,
					const FontsetHashKey *key_b);
static void     fontset_hash_key_free  (FontsetHashKey       *key);

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

  priv->font_hash = g_hash_table_new_full ((GHashFunc)font_hash_key_hash,
					   (GEqualFunc)font_hash_key_equal,
					   (GDestroyNotify)font_hash_key_free,
					   NULL);
  priv->fontset_hash = g_hash_table_new_full ((GHashFunc)fontset_hash_key_hash,
					      (GEqualFunc)fontset_hash_key_equal,
					      (GDestroyNotify)fontset_hash_key_free,
					      (GDestroyNotify)pango_fc_pattern_set_free);

  priv->coverage_hash = g_hash_table_new_full ((GHashFunc)pango_fc_coverage_key_hash,
					       (GEqualFunc)pango_fc_coverage_key_equal,
					       (GDestroyNotify)g_free,
					       (GDestroyNotify)pango_coverage_unref);
  priv->fontset_cache = g_queue_new ();
  priv->dpi = -1;
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

static gpointer
get_gravity_class (void)
{
  static GEnumClass *class = NULL;

  if (G_UNLIKELY (!class))
    class = g_type_class_ref (PANGO_TYPE_GRAVITY);

  return class;
}

static guint
pango_fc_pattern_hash (FcPattern *pattern)
{
#if 1
  return FcPatternHash (pattern);
#else
  /* Hashing only part of the pattern can improve speed a bit.
   */
  char *str;
  int i;
  double d;
  guint hash = 0;

  FcPatternGetString (pattern, FC_FILE, 0, (FcChar8 **) &str);
  if (str)
    hash = g_str_hash (str);

  if (FcPatternGetInteger (pattern, FC_INDEX, 0, &i) == FcResultMatch)
    hash ^= i;

  if (FcPatternGetDouble (pattern, FC_PIXEL_SIZE, 0, &d) == FcResultMatch)
    hash ^= (guint) (d*1000.0);

  return hash;
#endif
}

static gboolean
pango_fc_pattern_equal (FcPattern *pattern1,
			 FcPattern *pattern2)
{
  if (pattern1 == pattern2)
    return TRUE;
  else
    return FcPatternEqual (pattern1, pattern2);
}

static guint
pango_fc_coverage_key_hash (PangoFcCoverageKey *key)
{
  return g_str_hash (key->filename) ^ key->id;
}

static gboolean
pango_fc_coverage_key_equal (PangoFcCoverageKey *key1,
			       PangoFcCoverageKey *key2)
{
  return key1->id == key2->id && strcmp (key1->filename, key2->filename) == 0;
}

struct _FontsetHashKey {
  PangoFcFontMap *fontmap;
  PangoLanguage *language;
  PangoFontDescription *desc;
  int scaled_size;
  gpointer context_key;
};

struct _FontHashKey {
  PangoFcFontMap *fontmap;
  PangoMatrix matrix;
  FcPattern *pattern;
  gpointer context_key;
};

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

static gboolean
fontset_hash_key_equal (const FontsetHashKey *key_a,
			const FontsetHashKey *key_b)
{
  if (key_a->scaled_size == key_b->scaled_size &&
      pango_font_description_equal (key_a->desc, key_b->desc) &&
      key_a->language == key_b->language)
    {
      if (key_a->context_key)
	return PANGO_FC_FONT_MAP_GET_CLASS (key_a->fontmap)->context_key_equal (key_a->fontmap,
										key_a->context_key,
										key_b->context_key);
      else
	return TRUE;
    }
  else
    return FALSE;
}

static guint
fontset_hash_key_hash (const FontsetHashKey *key)
{
    guint32 hash = FNV1_32_INIT;

    if (key->context_key)
      hash ^= PANGO_FC_FONT_MAP_GET_CLASS (key->fontmap)->context_key_hash (key->fontmap,
									    key->context_key);

    /* 1237 is just an abitrary prime */
    return (hash ^
	    GPOINTER_TO_UINT (key->language) ^
	    (key->scaled_size * 1237) ^
	    pango_font_description_hash (key->desc));
}

static void
fontset_hash_key_free (FontsetHashKey *key)
{
  pango_font_description_free (key->desc);

  if (key->context_key)
    PANGO_FC_FONT_MAP_GET_CLASS (key->fontmap)->context_key_free (key->fontmap,
								  key->context_key);

  g_slice_free (FontsetHashKey, key);
}

static FontsetHashKey *
fontset_hash_key_copy (FontsetHashKey *old)
{
  FontsetHashKey *key = g_slice_new (FontsetHashKey);

  key->fontmap = old->fontmap;
  key->language = old->language;
  key->desc = pango_font_description_copy (old->desc);
  key->scaled_size = old->scaled_size;
  if (old->context_key)
    key->context_key = PANGO_FC_FONT_MAP_GET_CLASS (key->fontmap)->context_key_copy (key->fontmap,
										     old->context_key);
  else
    key->context_key = NULL;

  return key;
}

static gboolean
font_hash_key_equal (const FontHashKey *key_a,
		     const FontHashKey *key_b)
{
  if (key_a->matrix.xx == key_b->matrix.xx &&
      key_a->matrix.xy == key_b->matrix.xy &&
      key_a->matrix.yx == key_b->matrix.yx &&
      key_a->matrix.yy == key_b->matrix.yy &&
      key_a->pattern == key_b->pattern)
    {
      if (key_a->context_key)
	return PANGO_FC_FONT_MAP_GET_CLASS (key_a->fontmap)->context_key_equal (key_a->fontmap,
										key_a->context_key,
										key_b->context_key);
      else
	return TRUE;
    }
  else
    return FALSE;
}

static guint
font_hash_key_hash (const FontHashKey *key)
{
    guint32 hash = FNV1_32_INIT;

    /* We do a bytewise hash on the context matrix */
    hash = hash_bytes_fnv ((unsigned char *)(&key->matrix),
			   sizeof(double) * 4,
			   hash);

    if (key->context_key)
      hash ^= PANGO_FC_FONT_MAP_GET_CLASS (key->fontmap)->context_key_hash (key->fontmap,
									    key->context_key);

    return (hash ^ GPOINTER_TO_UINT (key->pattern));
}

static void
font_hash_key_free (FontHashKey *key)
{
  if (key->context_key)
    PANGO_FC_FONT_MAP_GET_CLASS (key->fontmap)->context_key_free (key->fontmap,
								  key->context_key);

  g_slice_free (FontHashKey, key);
}

static FontHashKey *
font_hash_key_copy (FontHashKey *old)
{
  FontHashKey *key = g_slice_new (FontHashKey);

  key->fontmap = old->fontmap;
  key->matrix = old->matrix;
  key->pattern = old->pattern;
  if (old->context_key)
    key->context_key = PANGO_FC_FONT_MAP_GET_CLASS (key->fontmap)->context_key_copy (key->fontmap,
										     old->context_key);
  else
    key->context_key = NULL;

  return key;
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
 * Since: 1.6.
 **/
void
pango_fc_font_map_add_decoder_find_func (PangoFcFontMap        *fcfontmap,
					 PangoFcDecoderFindFunc findfunc,
					 gpointer               user_data,
					 GDestroyNotify         dnotify)
{
  PangoFcFontMapPrivate *priv = fcfontmap->priv;
  PangoFcFindFuncInfo *info;

  info = g_slice_new (PangoFcFindFuncInfo);

  info->findfunc = findfunc;
  info->user_data = user_data;
  info->dnotify = dnotify;

  priv->findfuncs = g_slist_append (priv->findfuncs, info);
}

static void
pango_fc_font_map_finalize (GObject *object)
{
  PangoFcFontMap *fcfontmap = PANGO_FC_FONT_MAP (object);
  PangoFcFontMapPrivate *priv = fcfontmap->priv;

  pango_fc_font_map_cache_clear (fcfontmap);
  g_queue_free (priv->fontset_cache);
  g_hash_table_destroy (priv->coverage_hash);

  if (priv->fontset_hash)
    g_hash_table_destroy (priv->fontset_hash);

  if (priv->font_hash)
    g_hash_table_destroy (priv->font_hash);

  if (priv->pattern_hash)
    g_hash_table_destroy (priv->pattern_hash);

  while (priv->findfuncs)
    {
      PangoFcFindFuncInfo *info;
      info = priv->findfuncs->data;
      if (info->dnotify)
	info->dnotify (info->user_data);

      g_slice_free (PangoFcFindFuncInfo, info);
      priv->findfuncs = g_slist_delete_link (priv->findfuncs, priv->findfuncs);
    }

  G_OBJECT_CLASS (pango_fc_font_map_parent_class)->finalize (object);
}

static void
get_context_matrix (PangoContext *context,
		    PangoMatrix *matrix)
{
  const PangoMatrix *set_matrix;
  static const PangoMatrix identity = PANGO_MATRIX_INIT;

  if (context)
    set_matrix = pango_context_get_matrix (context);
  else
    set_matrix = NULL;

  if (set_matrix)
    *matrix = *set_matrix;
  else
    *matrix = identity;
}

static void
font_hash_key_for_context (PangoFcFontMap *fcfontmap,
			   PangoContext   *context,
			   FontHashKey    *key)
{
  key->fontmap = fcfontmap;
  get_context_matrix (context, &key->matrix);

  if (PANGO_FC_FONT_MAP_GET_CLASS (fcfontmap)->context_key_get)
    key->context_key = (gpointer)PANGO_FC_FONT_MAP_GET_CLASS (fcfontmap)->context_key_get (fcfontmap, context);
  else
    key->context_key = NULL;
}

/* Add a mapping from fcfont->font_pattern to fcfont */
static void
pango_fc_font_map_add (PangoFcFontMap *fcfontmap,
		       PangoContext   *context,
		       PangoFcFont    *fcfont)
{
  PangoFcFontMapPrivate *priv = fcfontmap->priv;
  FontHashKey key;
  FontHashKey *key_copy;

  g_assert (fcfont->fontmap == NULL);
  fcfont->fontmap = (PangoFontMap *) fcfontmap;
  /* In other fontmaps we add a weak pointer on ->fontmap so the
   * field is unset when fontmap is finalized.  We don't need it
   * here though as PangoFcFontMap already cleans up fcfont->fontmap
   * as part of it's caching scheme. */

  font_hash_key_for_context (fcfontmap, context, &key);
  key.pattern = fcfont->font_pattern;

  key_copy = font_hash_key_copy (&key);
  _pango_fc_font_set_context_key (fcfont, key_copy->context_key);
  fcfont->matrix = key.matrix;

  g_hash_table_insert (priv->font_hash, key_copy, fcfont);
}

static PangoFcFont *
pango_fc_font_map_lookup (PangoFcFontMap *fcfontmap,
			  PangoContext   *context,
			  FcPattern      *pattern)
{
  PangoFcFontMapPrivate *priv = fcfontmap->priv;
  FontHashKey key;

  font_hash_key_for_context (fcfontmap, context, &key);
  key.pattern = pattern;

  return g_hash_table_lookup (priv->font_hash, &key);
}

/* Remove mapping from fcfont->font_pattern to fcfont */
void
_pango_fc_font_map_remove (PangoFcFontMap *fcfontmap,
			   PangoFcFont    *fcfont)
{
  PangoFcFontMapPrivate *priv = fcfontmap->priv;
  FontHashKey key;

  key.fontmap = fcfontmap;
  key.matrix = fcfont->matrix;
  key.pattern = fcfont->font_pattern;
  key.context_key = _pango_fc_font_get_context_key (fcfont);

  g_hash_table_remove (priv->font_hash, &key);
  fcfont->fontmap = NULL;
  _pango_fc_font_set_context_key (fcfont, NULL);
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
	  FcChar8 *s;
	  FcResult res;
	  int spacing;

	  res = FcPatternGetString (fontset->fonts[i], FC_FAMILY, 0, (FcChar8 **) &s);
	  g_assert (res == FcResultMatch);

	  res = FcPatternGetInteger (fontset->fonts[i], FC_SPACING, 0, &spacing);
	  g_assert (res == FcResultMatch || res == FcResultNoMatch);
	  if (res == FcResultNoMatch)
	    spacing = FC_PROPORTIONAL;

	  if (!is_alias_family (s) && !g_hash_table_lookup (temp_family_hash, s))
	    {
	      PangoFcFamily *temp_family = create_family (fcfontmap, (gchar *)s, spacing);
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
#ifdef FC_WEIGHT_ULTRABOLD
  /* fontconfig 2.1 only had light/medium/demibold/bold/black */
  if (pango_weight < (PANGO_WEIGHT_ULTRALIGHT + PANGO_WEIGHT_LIGHT) / 2)
    return FC_WEIGHT_ULTRALIGHT;
  else if (pango_weight < (PANGO_WEIGHT_LIGHT + PANGO_WEIGHT_NORMAL) / 2)
    return FC_WEIGHT_LIGHT;
  else if (pango_weight < (PANGO_WEIGHT_NORMAL + 500 /* PANGO_WEIGHT_MEDIUM */) / 2)
    return FC_WEIGHT_NORMAL;
  else if (pango_weight < (500 /* PANGO_WEIGHT_MEDIUM */ + PANGO_WEIGHT_SEMIBOLD) / 2)
    return FC_WEIGHT_MEDIUM;
  else if (pango_weight < (PANGO_WEIGHT_SEMIBOLD + PANGO_WEIGHT_BOLD) / 2)
    return FC_WEIGHT_DEMIBOLD;
  else if (pango_weight < (PANGO_WEIGHT_BOLD + PANGO_WEIGHT_ULTRABOLD) / 2)
    return FC_WEIGHT_BOLD;
  else if (pango_weight < (PANGO_WEIGHT_ULTRABOLD + PANGO_WEIGHT_HEAVY) / 2)
    return FC_WEIGHT_ULTRABOLD;
  else
    return FC_WEIGHT_BLACK;
#else  /* fontconfig < 2.2 */
  if (pango_weight < (PANGO_WEIGHT_LIGHT + PANGO_WEIGHT_NORMAL) / 2)
    return FC_WEIGHT_LIGHT;
  else if (pango_weight < (500 /* PANGO_WEIGHT_MEDIUM */ + PANGO_WEIGHT_SEMIBOLD) / 2)
    return FC_WEIGHT_MEDIUM;
  else if (pango_weight < (PANGO_WEIGHT_SEMIBOLD + PANGO_WEIGHT_BOLD) / 2)
    return FC_WEIGHT_DEMIBOLD;
  else if (pango_weight < (PANGO_WEIGHT_BOLD + PANGO_WEIGHT_ULTRABOLD) / 2)
    return FC_WEIGHT_BOLD;
  else
    return FC_WEIGHT_BLACK;
#endif
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
		       double                       pixel_size,
		       double                       dpi)
{
  FcPattern *pattern;
  int slant;
  int weight;
  PangoGravity gravity;
  FcBool vertical;
  char **families;
  int i;
#ifdef FC_WIDTH
  int width;
#endif

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
			    FC_SIZE,  FcTypeDouble,  pixel_size * (72. / dpi),
			    FC_PIXEL_SIZE,  FcTypeDouble,  pixel_size,
			    NULL);

  families = g_strsplit (pango_font_description_get_family (description), ",", -1);

  for (i = 0; families[i]; i++)
    FcPatternAddString (pattern, FC_FAMILY, families[i]);

  g_strfreev (families);

  if (language)
    FcPatternAddString (pattern, FC_LANG, (FcChar8 *) pango_language_to_string (language));

  if (gravity != PANGO_GRAVITY_SOUTH)
    {
      GEnumValue *value = g_enum_get_value (get_gravity_class (), gravity);
      FcPatternAddString (pattern, PANGO_FC_GRAVITY, value->value_nick);
    }

  return pattern;
}

static PangoFont *
pango_fc_font_map_new_font (PangoFontMap               *fontmap,
			    PangoContext               *context,
			    const PangoFontDescription *description,
			    FcPattern                  *match)
{
  PangoFcFontMapClass *class;
  PangoFcFontMap *fcfontmap = (PangoFcFontMap *)fontmap;
  PangoFcFontMapPrivate *priv = fcfontmap->priv;
  FcPattern *pattern;
  PangoFcFont *fcfont;
  GSList *l;

  if (priv->closed)
    return NULL;

  fcfont = pango_fc_font_map_lookup (fcfontmap, context, match);
  if (fcfont)
    return g_object_ref (fcfont);

  class = PANGO_FC_FONT_MAP_GET_CLASS (fontmap);

  if (class->create_font)
    {
      fcfont = PANGO_FC_FONT_MAP_GET_CLASS (fontmap)->create_font (fcfontmap,
								   context,
								   description,
								   match);
    }
  else
    {
      const PangoMatrix *pango_matrix;

      if (context)
	pango_matrix = pango_context_get_matrix (context);
      else
	pango_matrix = NULL;

      if (pango_matrix)
	{
	  FcMatrix fc_matrix;

	  /* Fontconfig has the Y axis pointing up, Pango, down.
	   */
	  fc_matrix.xx = pango_matrix->xx;
	  fc_matrix.xy = - pango_matrix->xy;
	  fc_matrix.yx = - pango_matrix->yx;
	  fc_matrix.yy = pango_matrix->yy;

	  pattern = FcPatternDuplicate (match);
	  FcPatternAddMatrix (pattern, FC_MATRIX, &fc_matrix);
	}
      else
	pattern = match;

      fcfont = PANGO_FC_FONT_MAP_GET_CLASS (fontmap)->new_font (fcfontmap, pattern);

      if (pango_matrix)
	FcPatternDestroy (pattern);
    }

  if (!fcfont)
    return NULL;

  pango_fc_font_map_add (fcfontmap, context, fcfont);

  /*
   * Give any custom decoders a crack at this font now that it's been
   * created.
   */
  for (l = priv->findfuncs; l && l->data; l = l->next)
    {
      PangoFcFindFuncInfo *info = l->data;
      PangoFcDecoder *decoder;

      decoder = info->findfunc (match, info->user_data);
      if (decoder)
	{
	  _pango_fc_font_set_decoder (fcfont, decoder);
	  break;
	}
    }

  return (PangoFont *)fcfont;
}

static FcPattern *
uniquify_pattern (PangoFcFontMap *fcfontmap,
		  FcPattern      *pattern)
{
  PangoFcFontMapPrivate *priv = fcfontmap->priv;
  FcPattern *old_pattern;

  if (!priv->pattern_hash)
    priv->pattern_hash =
      g_hash_table_new_full ((GHashFunc)pango_fc_pattern_hash,
			     (GEqualFunc)pango_fc_pattern_equal,
			     (GDestroyNotify)FcPatternDestroy, NULL);

  old_pattern = g_hash_table_lookup (priv->pattern_hash, pattern);
  if (old_pattern)
    {
      FcPatternDestroy (pattern);
      FcPatternReference (old_pattern);
      return old_pattern;
    }
  else
    {
      FcPatternReference (pattern);
      g_hash_table_insert (priv->pattern_hash, pattern, pattern);
      return pattern;
    }
}

static void
pango_fc_default_substitute (PangoFcFontMap    *fontmap,
			     PangoContext      *context,
			     FcPattern         *pattern)
{
  if (PANGO_FC_FONT_MAP_GET_CLASS (fontmap)->context_substitute)
    PANGO_FC_FONT_MAP_GET_CLASS (fontmap)->context_substitute (fontmap, context, pattern);
  else if (PANGO_FC_FONT_MAP_GET_CLASS (fontmap)->default_substitute)
    PANGO_FC_FONT_MAP_GET_CLASS (fontmap)->default_substitute (fontmap, pattern);
}

static gdouble
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

static PangoFcPatternSet *
pango_fc_font_map_get_patterns (PangoFontMap               *fontmap,
				PangoContext               *context,
				const PangoFontDescription *desc,
				PangoLanguage              *language)
{
  PangoFcFontMap *fcfontmap = (PangoFcFontMap *)fontmap;
  PangoFcFontMapPrivate *priv = fcfontmap->priv;
  FcPattern *pattern, *font_pattern;
  FcResult res;
  int f;
  PangoFcPatternSet *patterns;
  FcFontSet *font_patterns;
  PangoMatrix matrix;
  FontsetHashKey key;

  if (!language && context)
    language = pango_context_get_language (context);

  key.fontmap = fcfontmap;
  key.scaled_size = get_scaled_size (fcfontmap, context, desc);
  key.language = language;
  key.desc = pango_font_description_copy_static (desc);
  pango_font_description_unset_fields (key.desc, PANGO_FONT_MASK_SIZE);

  if (PANGO_FC_FONT_MAP_GET_CLASS (fcfontmap)->context_key_get)
    key.context_key = (gpointer)PANGO_FC_FONT_MAP_GET_CLASS (fcfontmap)->context_key_get (fcfontmap, context);
  else
    key.context_key = NULL;

  patterns = g_hash_table_lookup (priv->fontset_hash, &key);

  if (patterns == NULL)
    {
      pattern = pango_fc_make_pattern (desc, language,
				       key.scaled_size / 1024.,
				       pango_fc_font_map_get_resolution (fcfontmap, context));

      pango_fc_default_substitute (fcfontmap, context, pattern);

      font_patterns = FcFontSort (NULL, pattern, FcTrue, NULL, &res);

      if (!font_patterns)
	{
	  g_critical ("No fonts found:\n"
		      "This probably means that the fontconfig\n"
		      "library is not correctly configured. You may need to\n"
		      "edit the fonts.conf configuration file. More information\n"
		      "about fontconfig can be found in the fontconfig(3) manual\n"
		      "page and on http://fontconfig.org");

	  font_patterns = FcFontSetCreate ();
	}

      patterns = g_slice_new (PangoFcPatternSet);
      patterns->patterns = g_new (FcPattern *, font_patterns->nfont);
      patterns->n_patterns = 0;
      patterns->fontset = NULL;
      patterns->cache_link = NULL;

      for (f = 0; f < font_patterns->nfont; f++)
	{
	  font_pattern = FcFontRenderPrepare (NULL, pattern,
					      font_patterns->fonts[f]);

	  if (font_pattern)
	    {
#ifdef FC_PATTERN
	      /* The FC_PATTERN element, which points back to our the original
	       * pattern defeats our hash tables.
	       */
	      FcPatternDel (font_pattern, FC_PATTERN);
#endif /* FC_PATTERN */

	      patterns->patterns[patterns->n_patterns++] = uniquify_pattern (fcfontmap, font_pattern);
	    }
	}

      FcPatternDestroy (pattern);

      FcFontSetDestroy (font_patterns);

      patterns->key = fontset_hash_key_copy (&key);
      g_hash_table_insert (priv->fontset_hash,
			   patterns->key,
			   patterns);
    }

  if ((!patterns->cache_link ||
       patterns->cache_link != priv->fontset_cache->head))
    pango_fc_font_map_cache_fontset (fcfontmap, patterns);

  pango_font_description_free (key.desc);

  return patterns;
}

static gboolean
get_first_font (PangoFontset  *fontset,
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
pango_fc_pattern_set_free (PangoFcPatternSet *patterns)
{
  int i;

  if (patterns->fontset)
    g_object_unref (patterns->fontset);

  for (i = 0; i < patterns->n_patterns; i++)
    FcPatternDestroy (patterns->patterns[i]);

  g_free (patterns->patterns);
  g_slice_free (PangoFcPatternSet, patterns);
}

static void
pango_fc_font_map_cache_fontset (PangoFcFontMap    *fcfontmap,
				 PangoFcPatternSet *patterns)
{
  PangoFcFontMapPrivate *priv = fcfontmap->priv;
  GQueue *cache = priv->fontset_cache;

  if (patterns->cache_link)
    {
      /* Already in cache, move to head
       */
      if (patterns->cache_link == cache->tail)
	cache->tail = patterns->cache_link->prev;

      cache->head = g_list_remove_link (cache->head, patterns->cache_link);
      cache->length--;
    }
  else
    {
      /* Add to cache initially
       */
      if (cache->length == FONTSET_CACHE_SIZE)
	{
	  PangoFcPatternSet *tmp_patterns = g_queue_pop_tail (cache);
	  tmp_patterns->cache_link = NULL;
	  g_hash_table_remove (priv->fontset_hash, tmp_patterns->key);
	}

      patterns->cache_link = g_list_prepend (NULL, patterns);
    }

  g_queue_push_head_link (cache, patterns->cache_link);
}

static PangoFontset *
pango_fc_font_map_load_fontset (PangoFontMap                 *fontmap,
				PangoContext                 *context,
				const PangoFontDescription   *desc,
				PangoLanguage                *language)
{
  PangoFcPatternSet *patterns = pango_fc_font_map_get_patterns (fontmap, context, desc, language);
  PangoFontset *result;
  int i;

  if (!patterns)
    return NULL;

  if (!patterns->fontset)
    {
      PangoFontsetSimple *simple;
      simple = pango_fontset_simple_new (language);
      result = PANGO_FONTSET (simple);

      for (i = 0; i < patterns->n_patterns; i++)
	{
	  PangoFont *font;

	  font = pango_fc_font_map_new_font (fontmap, context, desc,
					     patterns->patterns[i]);
	  if (font)
	    pango_fontset_simple_append (simple, font);
	}

      patterns->fontset = PANGO_FONTSET (simple);
    }

  result = g_object_ref (patterns->fontset);

  return result;
}

static void
uncache_patterns (PangoFcPatternSet *patterns,
		  PangoFcFontMap    *fcfontmap)
{
  PangoFcFontMapPrivate *priv = fcfontmap->priv;

  g_hash_table_remove (priv->fontset_hash, patterns->key);
}

static void
pango_fc_font_map_clear_fontset_cache (PangoFcFontMap *fcfontmap)
{
  PangoFcFontMapPrivate *priv = fcfontmap->priv;
  GQueue *cache = priv->fontset_cache;

  g_list_foreach (cache->head, (GFunc)uncache_patterns, fcfontmap);
  g_list_free (cache->head);
  cache->head = NULL;
  cache->tail = NULL;
  cache->length = 0;
}

/**
 * pango_fc_font_map_cache_clear:
 * @fcfontmap: a #PangoFcFontmap
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
  fcfontmap->priv->dpi = -1;

  pango_fc_font_map_clear_fontset_cache (fcfontmap);
}

static void
pango_fc_font_map_set_coverage (PangoFcFontMap            *fcfontmap,
				PangoFcCoverageKey        *key,
				PangoCoverage             *coverage)
{
  PangoFcFontMapPrivate *priv = fcfontmap->priv;
  PangoFcCoverageKey *key_dup;

  key_dup = g_malloc (sizeof (PangoFcCoverageKey) + strlen (key->filename) + 1);
  key_dup->id = key->id;
  key_dup->filename = (char *) (key_dup + 1);
  strcpy (key_dup->filename, key->filename);

  g_hash_table_insert (priv->coverage_hash,
		       key_dup, pango_coverage_ref (coverage));
}

PangoCoverage *
_pango_fc_font_map_get_coverage (PangoFcFontMap *fcfontmap,
				 PangoFcFont    *fcfont)
{
  PangoFcFontMapPrivate *priv = fcfontmap->priv;
  PangoFcCoverageKey key;
  PangoCoverage *coverage;
  FcCharSet *charset;

  /*
   * Assume that coverage information is identified by
   * a filename/index pair; there shouldn't be any reason
   * this isn't true, but it's not specified anywhere
   */
  if (FcPatternGetString (fcfont->font_pattern, FC_FILE, 0, (FcChar8 **) &key.filename) != FcResultMatch)
    return NULL;

  if (FcPatternGetInteger (fcfont->font_pattern, FC_INDEX, 0, &key.id) != FcResultMatch)
    return NULL;

  coverage = g_hash_table_lookup (priv->coverage_hash, &key);
  if (coverage)
    return pango_coverage_ref (coverage);

  /*
   * Pull the coverage out of the pattern, this
   * doesn't require loading the font
   */
  if (FcPatternGetCharSet (fcfont->font_pattern, FC_CHARSET, 0, &charset) != FcResultMatch)
    return NULL;

  coverage = _pango_fc_font_map_fc_to_coverage (charset);

  pango_fc_font_map_set_coverage (fcfontmap, &key, coverage);

  return coverage;
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
 * only for backend implementations deriving from #PangoFcFontmap;
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
cleanup_font (gpointer        key,
	      PangoFcFont    *fcfont)
{
  _pango_fc_font_shutdown (fcfont);

  fcfont->fontmap = NULL;
}

/**
 * pango_fc_font_map_shutdown:
 * @fcfontmap: a #PangoFcFontmap
 *
 * Clears all cached information for the fontmap and marks
 * all fonts open for the fontmap as dead. (See the shutdown()
 * virtual function of #PangoFcFont.) This function might be used
 * by a backend when the underlying windowing system for the font
 * map exits. This function is only intended to be called from
 * only for backend implementations deriving from #PangoFcFontmap.
 *
 * Since: 1.4
 **/
void
pango_fc_font_map_shutdown (PangoFcFontMap *fcfontmap)
{
  PangoFcFontMapPrivate *priv = fcfontmap->priv;

  pango_fc_font_map_cache_clear (fcfontmap);
  g_hash_table_destroy (priv->fontset_hash);
  priv->fontset_hash = NULL;

  g_hash_table_foreach (priv->font_hash, (GHFunc)cleanup_font, NULL);
  g_hash_table_destroy (priv->font_hash);
  priv->font_hash = NULL;
  priv->closed = TRUE;
}

static PangoWeight
pango_fc_convert_weight_to_pango (int fc_weight)
{
#ifdef FC_WEIGHT_ULTRABOLD
  /* fontconfig 2.1 only had light/medium/demibold/bold/black */
  if (fc_weight < (FC_WEIGHT_ULTRALIGHT + FC_WEIGHT_LIGHT) / 2)
    return PANGO_WEIGHT_ULTRALIGHT;
  else if (fc_weight < (FC_WEIGHT_LIGHT + FC_WEIGHT_REGULAR) / 2)
    return PANGO_WEIGHT_LIGHT;
  else if (fc_weight < (FC_WEIGHT_REGULAR + FC_WEIGHT_DEMIBOLD) / 2)
    return PANGO_WEIGHT_NORMAL;
  /* We group the 500/MEDIUM weight with normal to reduce confusion
   *
   * else if (fc_weight < (FC_WEIGHT_MEDIUM + FC_WEIGHT_DEMIBOLD) / 2)
   *  return 500;
   */
  else if (fc_weight < (FC_WEIGHT_DEMIBOLD + FC_WEIGHT_BOLD) / 2)
    return PANGO_WEIGHT_SEMIBOLD;
  else if (fc_weight < (FC_WEIGHT_BOLD + FC_WEIGHT_ULTRABOLD) / 2)
    return PANGO_WEIGHT_BOLD;
  else if (fc_weight < (FC_WEIGHT_ULTRABOLD + FC_WEIGHT_BLACK) / 2)
    return PANGO_WEIGHT_ULTRABOLD;
  else
    return PANGO_WEIGHT_HEAVY;
#else  /* fontconfig < 2.2 */
  if (fc_weight < (FC_WEIGHT_LIGHT + FC_WEIGHT_MEDIUM) / 2)
    return PANGO_WEIGHT_LIGHT;
  else if (fc_weight < (FC_WEIGHT_MEDIUM + FC_WEIGHT_DEMIBOLD) / 2)
    return PANGO_WEIGHT_NORMAL;
  else if (fc_weight < (FC_WEIGHT_DEMIBOLD + FC_WEIGHT_BOLD) / 2)
    return PANGO_WEIGHT_SEMIBOLD;
  else if (fc_weight < (FC_WEIGHT_BOLD + FC_WEIGHT_BLACK) / 2)
    return PANGO_WEIGHT_BOLD;
  else
    return PANGO_WEIGHT_ULTRABOLD;
#endif
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
      GEnumValue *value = g_enum_get_value_by_nick (get_gravity_class (), s);
      gravity = value->value;

      pango_font_description_set_gravity (desc, gravity);
    }

  return desc;
}

/*
 * PangoFcFace
 */

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

  pattern = FcPatternCreate ();
  FcPatternAddString (pattern, FC_FAMILY, fcface->family->family_name);
  FcPatternAddString (pattern, FC_STYLE, fcface->style);

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
pango_fc_face_class_init (PangoFontFaceClass *class)
{
  class->describe = pango_fc_face_describe;
  class->get_face_name = pango_fc_face_get_face_name;
  class->list_sizes = pango_fc_face_list_sizes;
  class->is_synthesized = pango_fc_face_is_synthesized;
}

static GType
pango_fc_face_get_type (void)
{
  static GType object_type = 0;

  if (G_UNLIKELY (!object_type))
    {
      const GTypeInfo object_info =
      {
	sizeof (PangoFontFaceClass),
	(GBaseInitFunc) NULL,
	(GBaseFinalizeFunc) NULL,
	(GClassInitFunc) pango_fc_face_class_init,
	NULL,           /* class_finalize */
	NULL,           /* class_data */
	sizeof (PangoFcFace),
	0,              /* n_preallocs */
	(GInstanceInitFunc) NULL,
	NULL            /* value_table */
      };

      object_type = g_type_register_static (PANGO_TYPE_FONT_FACE,
					    I_("PangoFcFace"),
					    &object_info, 0);
    }

  return object_type;
}

/*
 * PangoFcFamily
 */
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
  PangoFcFontMapPrivate *priv = fcfontmap->priv;

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
	      FcChar8 *style, *font_style = NULL;
	      int weight, slant;

	      if (FcPatternGetInteger(fontset->fonts[i], FC_WEIGHT, 0, &weight) != FcResultMatch)
		weight = FC_WEIGHT_MEDIUM;

	      if (FcPatternGetInteger(fontset->fonts[i], FC_SLANT, 0, &slant) != FcResultMatch)
		slant = FC_SLANT_ROMAN;

	      if (FcPatternGetString (fontset->fonts[i], FC_STYLE, 0, &font_style) != FcResultMatch)
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
pango_fc_family_class_init (PangoFontFamilyClass *class)
{
  class->list_faces = pango_fc_family_list_faces;
  class->get_name = pango_fc_family_get_name;
  class->is_monospace = pango_fc_family_is_monospace;
}

static void
pango_fc_family_init (PangoFcFamily *fcfamily)
{
  fcfamily->n_faces = -1;
}

static GType
pango_fc_family_get_type (void)
{
  static GType object_type = 0;

  if (G_UNLIKELY (!object_type))
    {
      const GTypeInfo object_info =
      {
	sizeof (PangoFontFamilyClass),
	(GBaseInitFunc) NULL,
	(GBaseFinalizeFunc) NULL,
	(GClassInitFunc) pango_fc_family_class_init,
	NULL,           /* class_finalize */
	NULL,           /* class_data */
	sizeof (PangoFcFamily),
	0,              /* n_preallocs */
	(GInstanceInitFunc) pango_fc_family_init,
	NULL            /* value_table */
      };

      object_type = g_type_register_static (PANGO_TYPE_FONT_FAMILY,
					    I_("PangoFcFamily"),
					    &object_info, 0);
    }

  return object_type;
}
