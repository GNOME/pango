/* Pango
 * pangocoretext-fontmap.c
 *
 * Copyright (C) 2000-2003 Red Hat, Inc.
 * Copyright (C) 2005-2007 Imendio AB
 * Copyright (C) 2010  Kristian Rietveld  <kris@gtk.org>
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

#include "config.h"

#include "pango-fontmap.h"
#include "pangocoretext-private.h"
#include "pango-impl-utils.h"
#include "modules.h"

#include <Carbon/Carbon.h>

typedef struct _FontHashKey      FontHashKey;

struct _PangoCoreTextFamily
{
  PangoFontFamily parent_instance;

  char *family_name;

  guint is_monospace : 1;

  PangoFontFace **faces;
  gint n_faces;
};

#define PANGO_TYPE_CORE_TEXT_FAMILY              (pango_core_text_family_get_type ())
#define PANGO_CORE_TEXT_FAMILY(object)           (G_TYPE_CHECK_INSTANCE_CAST ((object), PANGO_TYPE_CORE_TEXT_FAMILY, PangoCoreTextFamily))
#define PANGO_IS_CORE_TEXT_FAMILY(object)        (G_TYPE_CHECK_INSTANCE_TYPE ((object), PANGO_TYPE_CORE_TEXT_FAMILY))

#define PANGO_TYPE_CORE_TEXT_FACE              (pango_core_text_face_get_type ())
#define PANGO_CORE_TEXT_FACE(object)           (G_TYPE_CHECK_INSTANCE_CAST ((object), PANGO_TYPE_CORE_TEXT_FACE, PangoCoreTextFace))
#define PANGO_IS_CORE_TEXT_FACE(object)        (G_TYPE_CHECK_INSTANCE_TYPE ((object), PANGO_TYPE_CORE_TEXT_FACE))

struct _PangoCoreTextFace
{
  PangoFontFace parent_instance;

  PangoCoreTextFamily *family;

  PangoCoverage *coverage;

  char *postscript_name;
  char *style_name;

  float weight;
  int traits;
  guint synthetic_italic : 1;
};

static GType pango_core_text_family_get_type (void);
static GType pango_core_text_face_get_type (void);

static gpointer pango_core_text_family_parent_class;
static gpointer pango_core_text_face_parent_class;

static const char *
get_real_family (const char *family_name)
{
  switch (family_name[0])
    {
    case 'm':
    case 'M':
      if (g_ascii_strcasecmp (family_name, "monospace") == 0)
	return "Courier";
      break;
    case 's':
    case 'S':
      if (g_ascii_strcasecmp (family_name, "sans") == 0)
	return "Helvetica";
      else if (g_ascii_strcasecmp (family_name, "serif") == 0)
	return "Times";
      break;
    }

  return family_name;
}

static gchar *
gchar_from_cf_string (CFStringRef str)
{
  CFIndex len;
  gchar *buffer;

  /* GetLength returns the number of UTF-16 pairs, so this number
   * times 2 should definitely gives us enough space for UTF8.
   * We add one for the terminating zero.
   */
  len = CFStringGetLength (str) * 2 + 1;
  buffer = g_new0 (char, len);
  CFStringGetCString (str, buffer, len, kCFStringEncodingUTF8);

  return buffer;
}

static PangoCoverage *
pango_coverage_from_cf_charset (CFCharacterSetRef charset)
{
  CFIndex i, length;
  CFDataRef bitmap;
  const UInt8 *ptr;
  PangoCoverage *coverage;

  coverage = pango_coverage_new ();

  bitmap = CFCharacterSetCreateBitmapRepresentation (kCFAllocatorDefault,
                                                     charset);

  /* We only handle the BMP plane */
  length = MIN (CFDataGetLength (bitmap), 8192);
  ptr = CFDataGetBytePtr (bitmap);

  /* FIXME: can and should this be done more efficiently? */
  for (i = 0; i < length; i++)
    {
      int j;

      for (j = 0; j < 8; j++)
        pango_coverage_set (coverage, i * 8 + j,
                            ((ptr[i] & (1 << j)) == (1 << j)) ?
                            PANGO_COVERAGE_EXACT : PANGO_COVERAGE_NONE);
    }

  CFRelease (bitmap);

  return coverage;
}

static inline gboolean
pango_core_text_face_is_oblique (PangoCoreTextFace *face)
{
  return g_strrstr (face->style_name, "Oblique") != NULL;
}

static inline PangoCoreTextFace *
pango_core_text_face_from_ct_font_descriptor (CTFontDescriptorRef desc)
{
  int font_traits;
  char *buffer;
  CFStringRef str;
  CFNumberRef number;
  CGFloat value;
  CFDictionaryRef dict;
  CFCharacterSetRef charset;
  PangoCoreTextFace *face = g_object_new (PANGO_TYPE_CORE_TEXT_FACE,
                                          NULL);

  face->synthetic_italic = FALSE;

  /* Get font name */
  str = CTFontDescriptorCopyAttribute (desc, kCTFontNameAttribute);
  buffer = gchar_from_cf_string (str);

  /* We strdup again to save space. */
  face->postscript_name = g_strdup (buffer);

  CFRelease (str);
  g_free (buffer);

  /* Get style name */
  str = CTFontDescriptorCopyAttribute (desc, kCTFontStyleNameAttribute);
  buffer = gchar_from_cf_string (str);

  face->style_name = g_strdup (buffer);

  CFRelease (str);
  g_free (buffer);

  /* Get font traits, symbolic traits */
  dict = CTFontDescriptorCopyAttribute (desc, kCTFontTraitsAttribute);
  number = (CFNumberRef)CFDictionaryGetValue (dict,
                                              kCTFontWeightTrait);
  if (CFNumberGetValue (number, kCFNumberCGFloatType, &value))
    /* Map value from range [-1.0, 1.0] to range [1, 14] */
    face->weight = (value + 1.0f) * 6.5f + 1;
  else
    face->weight = PANGO_WEIGHT_NORMAL;

  number = (CFNumberRef)CFDictionaryGetValue (dict,
                                              kCTFontSymbolicTrait);
  if (CFNumberGetValue (number, kCFNumberIntType, &font_traits))
    {
      face->traits = font_traits;
    }
  CFRelease (dict);

  /* Get font coverage */
  charset = CTFontDescriptorCopyAttribute (desc,
                                           kCTFontCharacterSetAttribute);
  face->coverage = pango_coverage_from_cf_charset (charset);
  CFRelease (charset);

  return face;
}

static void
pango_core_text_family_list_faces (PangoFontFamily  *family,
                                   PangoFontFace  ***faces,
                                   int              *n_faces)
{
  PangoCoreTextFamily *ctfamily = PANGO_CORE_TEXT_FAMILY (family);

  if (ctfamily->n_faces < 0)
    {
      GList *l;
      GList *faces = NULL;
      GList *synthetic_faces = NULL;
      GHashTable *italic_faces;
      const char *real_family = get_real_family (ctfamily->family_name);
      CTFontCollectionRef collection;
      CFArrayRef ctfaces;
      CFArrayRef font_descriptors;
      CFDictionaryRef attributes;
      CFIndex i, count;

      CFTypeRef keys[] = {
          (CFTypeRef) kCTFontFamilyNameAttribute
      };

      CFStringRef values[] = {
          CFStringCreateWithCString (kCFAllocatorDefault,
                                     real_family,
                                     kCFStringEncodingUTF8)
      };

      CTFontDescriptorRef descriptors[1];

      attributes = CFDictionaryCreate (kCFAllocatorDefault,
                                       (const void **)keys,
                                       (const void **)values,
                                       1,
                                       &kCFTypeDictionaryKeyCallBacks,
                                       &kCFTypeDictionaryValueCallBacks);
      descriptors[0] = CTFontDescriptorCreateWithAttributes (attributes);
      font_descriptors = CFArrayCreate (kCFAllocatorDefault,
                                        (const void **)descriptors,
                                        1,
                                        &kCFTypeArrayCallBacks);
      collection = CTFontCollectionCreateWithFontDescriptors (font_descriptors,
                                                              NULL);

      ctfaces = CTFontCollectionCreateMatchingFontDescriptors (collection);

      italic_faces = g_hash_table_new (g_direct_hash, g_direct_equal);

      count = CFArrayGetCount (ctfaces);
      for (i = 0; i < count; i++)
        {
          PangoCoreTextFace *face;
          CTFontDescriptorRef desc = CFArrayGetValueAtIndex (ctfaces, i);

          face = pango_core_text_face_from_ct_font_descriptor (desc);
          face->family = ctfamily;

          faces = g_list_prepend (faces, face);

          if (face->traits & kCTFontItalicTrait
              || pango_core_text_face_is_oblique (face))
            g_hash_table_insert (italic_faces, GINT_TO_POINTER (face->weight),
                                 face);
        }

      CFRelease (font_descriptors);
      CFRelease (attributes);
      CFRelease (ctfaces);

      /* For all fonts for which a non-synthetic italic variant does
       * not exist on the system, we create synthesized versions here.
       */
      for (l = faces; l; l = l->next)
        {
          PangoCoreTextFace *face = l->data;

          if (!g_hash_table_lookup (italic_faces,
                                    GINT_TO_POINTER (face->weight)))
            {
              PangoCoreTextFace *italic_face;

              italic_face = g_object_new (PANGO_TYPE_CORE_TEXT_FACE, NULL);

              italic_face->family = ctfamily;
              italic_face->postscript_name = g_strdup (face->postscript_name);
              italic_face->weight = face->weight;
              italic_face->traits = face->traits | kCTFontItalicTrait;
              italic_face->synthetic_italic = TRUE;
              italic_face->coverage = pango_coverage_ref (face->coverage);

              /* Try to create a sensible face name. */
              if (strcasecmp (face->style_name, "regular") == 0)
                italic_face->style_name = g_strdup ("Oblique");
              else
                italic_face->style_name = g_strdup_printf ("%s Oblique",
                                                           face->style_name);

              synthetic_faces = g_list_prepend (synthetic_faces, italic_face);
            }
        }

      faces = g_list_concat (faces, synthetic_faces);

      ctfamily->n_faces = g_list_length (faces);
      ctfamily->faces = g_new (PangoFontFace *, ctfamily->n_faces);

      for (l = faces, i = 0; l; l = l->next, i++)
	ctfamily->faces[i] = l->data;

      g_list_free (faces);
      g_hash_table_destroy (italic_faces);
    }

  if (n_faces)
    *n_faces = ctfamily->n_faces;

  if (faces)
    *faces = g_memdup (ctfamily->faces, ctfamily->n_faces * sizeof (PangoFontFace *));
}

static const char *
pango_core_text_family_get_name (PangoFontFamily *family)
{
  PangoCoreTextFamily *ctfamily = PANGO_CORE_TEXT_FAMILY (family);

  return ctfamily->family_name;
}

static gboolean
pango_core_text_family_is_monospace (PangoFontFamily *family)
{
  PangoCoreTextFamily *ctfamily = PANGO_CORE_TEXT_FAMILY (family);

  return ctfamily->is_monospace;
}

static void
pango_core_text_family_finalize (GObject *object)
{
  PangoCoreTextFamily *family = PANGO_CORE_TEXT_FAMILY (object);
  int i;

  g_free (family->family_name);

  if (family->n_faces != -1)
    {
      for (i = 0; i < family->n_faces; i++)
	g_object_unref (family->faces[i]);

      g_free (family->faces);
    }

  G_OBJECT_CLASS (pango_core_text_family_parent_class)->finalize (object);
}

static void
pango_core_text_family_class_init (PangoFontFamilyClass *class)
{
  GObjectClass *object_class = (GObjectClass *)class;
  int i;

  pango_core_text_family_parent_class = g_type_class_peek_parent (class);

  object_class->finalize = pango_core_text_family_finalize;

  class->list_faces = pango_core_text_family_list_faces;
  class->get_name = pango_core_text_family_get_name;
  class->is_monospace = pango_core_text_family_is_monospace;

  for (i = 0; _pango_included_core_text_modules[i].list; i++)
    pango_module_register (&_pango_included_core_text_modules[i]);
}

static void
pango_core_text_family_init (PangoCoreTextFamily *family)
{
  family->n_faces = -1;
}

static GType
pango_core_text_family_get_type (void)
{
  static GType object_type = 0;

  if (G_UNLIKELY (!object_type))
    {
      const GTypeInfo object_info =
      {
	sizeof (PangoFontFamilyClass),
	(GBaseInitFunc) NULL,
	(GBaseFinalizeFunc) NULL,
	(GClassInitFunc) pango_core_text_family_class_init,
	NULL,           /* class_finalize */
	NULL,           /* class_data */
	sizeof (PangoCoreTextFamily),
	0,              /* n_preallocs */
	(GInstanceInitFunc) pango_core_text_family_init,
      };

      object_type = g_type_register_static (PANGO_TYPE_FONT_FAMILY,
					    I_("PangoCoreTextFamily"),
					    &object_info, 0);
    }

  return object_type;
}

static PangoFontDescription *
pango_core_text_face_describe (PangoFontFace *face)
{
  PangoCoreTextFace *ctface = PANGO_CORE_TEXT_FACE (face);
  PangoFontDescription *description;
  PangoWeight pango_weight;
  PangoStyle pango_style;
  PangoVariant pango_variant;
  int weight;

  description = pango_font_description_new ();

  pango_font_description_set_family (description, ctface->family->family_name);

  weight = ctface->weight;

  switch (weight)
    {
      case 1:
      case 2:
        pango_weight = PANGO_WEIGHT_ULTRALIGHT;
        break;

      case 3:
      case 4:
        pango_weight = PANGO_WEIGHT_LIGHT;
        break;

      case 5:
      case 6:
        pango_weight = PANGO_WEIGHT_NORMAL;
        break;

      case 7:
      case 8:
        pango_weight = PANGO_WEIGHT_SEMIBOLD;
        break;

      case 9:
      case 10:
        pango_weight = PANGO_WEIGHT_BOLD;
        break;

      case 11:
      case 12:
        pango_weight = PANGO_WEIGHT_ULTRABOLD;
        break;

      case 13:
      case 14:
        pango_weight = PANGO_WEIGHT_HEAVY;
        break;

      default:
        g_assert_not_reached ();
    };

  if (ctface->traits & kCTFontItalicTrait)
    pango_style = PANGO_STYLE_ITALIC;
  else if (pango_core_text_face_is_oblique (ctface))
    pango_style = PANGO_STYLE_OBLIQUE;
  else
    pango_style = PANGO_STYLE_NORMAL;

  /* FIXME: How can this be figured using CoreText? */
#if 0
  if (ctface->traits & NSSmallCapsFontMask)
    pango_variant = PANGO_VARIANT_SMALL_CAPS;
  else
#endif
    pango_variant = PANGO_VARIANT_NORMAL;

  pango_font_description_set_weight (description, pango_weight);
  pango_font_description_set_style (description, pango_style);
  pango_font_description_set_variant (description, pango_variant);

  return description;
}

static const char *
pango_core_text_face_get_face_name (PangoFontFace *face)
{
  PangoCoreTextFace *ctface = PANGO_CORE_TEXT_FACE (face);

  return ctface->style_name;
}

static void
pango_core_text_face_list_sizes (PangoFontFace  *face,
                                 int           **sizes,
                                 int            *n_sizes)
{
  *n_sizes = 0;
  *sizes = NULL;
}

static void
pango_core_text_face_finalize (GObject *object)
{
  PangoCoreTextFace *ctface = PANGO_CORE_TEXT_FACE (object);

  if (ctface->coverage)
    pango_coverage_unref (ctface->coverage);

  g_free (ctface->postscript_name);
  g_free (ctface->style_name);

  G_OBJECT_CLASS (pango_core_text_face_parent_class)->finalize (object);
}

static gboolean
pango_core_text_face_is_synthesized (PangoFontFace *face)
{
  PangoCoreTextFace *cface = PANGO_CORE_TEXT_FACE (face);

  return cface->synthetic_italic;
}

static void
pango_core_text_face_class_init (PangoFontFaceClass *class)
{
  GObjectClass *object_class = (GObjectClass *)class;

  pango_core_text_face_parent_class = g_type_class_peek_parent (class);

  object_class->finalize = pango_core_text_face_finalize;

  class->describe = pango_core_text_face_describe;
  class->get_face_name = pango_core_text_face_get_face_name;
  class->list_sizes = pango_core_text_face_list_sizes;
  class->is_synthesized = pango_core_text_face_is_synthesized;
}

GType
pango_core_text_face_get_type (void)
{
  static GType object_type = 0;

  if (G_UNLIKELY (!object_type))
    {
      const GTypeInfo object_info =
      {
	sizeof (PangoFontFaceClass),
	(GBaseInitFunc) NULL,
	(GBaseFinalizeFunc) NULL,
	(GClassInitFunc) pango_core_text_face_class_init,
	NULL,           /* class_finalize */
	NULL,           /* class_data */
	sizeof (PangoCoreTextFace),
	0,              /* n_preallocs */
	(GInstanceInitFunc) NULL,
      };

      object_type = g_type_register_static (PANGO_TYPE_FONT_FACE,
					    I_("PangoCoreTextFace"),
					    &object_info, 0);
    }

  return object_type;
}

const char *
_pango_core_text_face_get_postscript_name (PangoCoreTextFace *face)
{
  return face->postscript_name;
}

gboolean
_pango_core_text_face_get_synthetic_italic (PangoCoreTextFace *face)
{
  return face->synthetic_italic;
}

PangoCoverage *
_pango_core_text_face_get_coverage (PangoCoreTextFace *face,
                                    PangoLanguage     *language)
{
  return face->coverage;
}

static void pango_core_text_font_map_class_init (PangoCoreTextFontMapClass *class);
static void pango_core_text_font_map_init (PangoCoreTextFontMap *ctfontmap);

static guint    font_hash_key_hash  (const FontHashKey *key);
static gboolean font_hash_key_equal (const FontHashKey *key_a,
				     const FontHashKey *key_b);
static void     font_hash_key_free  (FontHashKey       *key);

G_DEFINE_TYPE (PangoCoreTextFontMap, pango_core_text_font_map, PANGO_TYPE_FONT_MAP);

static void
pango_core_text_font_map_finalize (GObject *object)
{
  PangoCoreTextFontMap *fontmap = PANGO_CORE_TEXT_FONT_MAP (object);

  g_hash_table_destroy (fontmap->font_hash);
  g_hash_table_destroy (fontmap->families);

  G_OBJECT_CLASS (pango_core_text_font_map_parent_class)->finalize (object);
}

struct _FontHashKey {
  PangoCoreTextFontMap *fontmap;
  PangoMatrix matrix;
  PangoFontDescription *desc;
  char *postscript_name;
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
font_hash_key_equal (const FontHashKey *key_a,
		     const FontHashKey *key_b)
{
  if (key_a->matrix.xx == key_b->matrix.xx &&
      key_a->matrix.xy == key_b->matrix.xy &&
      key_a->matrix.yx == key_b->matrix.yx &&
      key_a->matrix.yy == key_b->matrix.yy &&
      pango_font_description_equal (key_a->desc, key_b->desc) &&
      strcmp (key_a->postscript_name, key_b->postscript_name) == 0)
    {
      if (key_a->context_key)
        return PANGO_CORE_TEXT_FONT_MAP_GET_CLASS (key_a->fontmap)->context_key_equal (key_a->fontmap,
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
    hash ^= PANGO_CORE_TEXT_FONT_MAP_GET_CLASS (key->fontmap)->context_key_hash (key->fontmap,
                                                                                 key->context_key);

  hash ^= g_str_hash (key->postscript_name);

  return (hash ^ pango_font_description_hash (key->desc));
}

static void
font_hash_key_free (FontHashKey *key)
{
  if (key->context_key)
    PANGO_CORE_TEXT_FONT_MAP_GET_CLASS (key->fontmap)->context_key_free (key->fontmap,
                                                                         key->context_key);

  g_slice_free (FontHashKey, key);
}

static FontHashKey *
font_hash_key_copy (FontHashKey *old)
{
  FontHashKey *key = g_slice_new (FontHashKey);

  key->fontmap = old->fontmap;
  key->matrix = old->matrix;
  key->desc = pango_font_description_copy (old->desc);
  key->postscript_name = g_strdup (old->postscript_name);
  if (old->context_key)
    key->context_key = PANGO_CORE_TEXT_FONT_MAP_GET_CLASS (key->fontmap)->context_key_copy (key->fontmap, old->context_key);
  else
    key->context_key = NULL;

  return key;
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
font_hash_key_for_context (PangoCoreTextFontMap *fcfontmap,
                           PangoContext         *context,
                           FontHashKey          *key)
{
  key->fontmap = fcfontmap;
  get_context_matrix (context, &key->matrix);

  if (PANGO_CORE_TEXT_FONT_MAP_GET_CLASS (fcfontmap)->context_key_get)
    key->context_key = (gpointer)PANGO_CORE_TEXT_FONT_MAP_GET_CLASS (fcfontmap)->context_key_get (fcfontmap, context);
  else
    key->context_key = NULL;
}

static void
pango_core_text_font_map_add (PangoCoreTextFontMap *ctfontmap,
                              PangoContext         *context,
                              PangoCoreTextFont    *ctfont)
{
  FontHashKey key;
  FontHashKey *key_copy;
  PangoCoreTextFace *face;

  _pango_core_text_font_set_font_map (ctfont, ctfontmap);

  font_hash_key_for_context (ctfontmap, context, &key);
  face = _pango_core_text_font_get_face (ctfont);
  key.postscript_name = (char *)_pango_core_text_face_get_postscript_name (face);
  key.desc = _pango_core_text_font_get_font_description (ctfont);

  key_copy = font_hash_key_copy (&key);
  _pango_core_text_font_set_context_key (ctfont, key_copy->context_key);
  g_hash_table_insert (ctfontmap->font_hash, key_copy, g_object_ref (ctfont));
}

static PangoCoreTextFont *
pango_core_text_font_map_lookup (PangoCoreTextFontMap *ctfontmap,
                                 PangoContext         *context,
                                 PangoFontDescription *desc,
                                 PangoCoreTextFace    *face)
{
  FontHashKey key;

  font_hash_key_for_context (ctfontmap, context, &key);
  key.postscript_name = (char *)_pango_core_text_face_get_postscript_name (face);
  key.desc = desc;

  return g_hash_table_lookup (ctfontmap->font_hash, &key);
}

static gboolean
find_best_match (PangoCoreTextFamily         *font_family,
                 const PangoFontDescription  *description,
                 PangoFontDescription       **best_description,
                 PangoCoreTextFace          **best_face)
{
  PangoFontDescription *new_desc;
  int i;

  *best_description = NULL;
  *best_face = NULL;

  for (i = 0; i < font_family->n_faces; i++)
    {
      new_desc = pango_font_face_describe (font_family->faces[i]);

      if (pango_font_description_better_match (description, *best_description, new_desc))
	{
	  pango_font_description_free (*best_description);
	  *best_description = new_desc;
	  *best_face = (PangoCoreTextFace *)font_family->faces[i];
	}
      else
	pango_font_description_free (new_desc);
    }

  if (*best_description)
    return TRUE;

  return FALSE;
}

static PangoFont *
pango_core_text_font_map_load_font (PangoFontMap               *fontmap,
                                    PangoContext               *context,
                                    const PangoFontDescription *description)
{
  PangoCoreTextFontMap *ctfontmap = (PangoCoreTextFontMap *)fontmap;
  PangoCoreTextFamily *font_family;
  const gchar *family;
  gchar *name;
  gint size;

  size = pango_font_description_get_size (description);
  if (size < 0)
    return NULL;

  family = pango_font_description_get_family (description);
  family = family ? family : "";
  name = g_utf8_casefold (family, -1);
  font_family = g_hash_table_lookup (ctfontmap->families, name);
  g_free (name);

  if (font_family)
    {
      PangoFontDescription *best_description;
      PangoCoreTextFace *best_face;
      PangoCoreTextFont *best_font;

      /* Force a listing of the available faces */
      pango_font_family_list_faces ((PangoFontFamily *)font_family, NULL, NULL);

      if (!find_best_match (font_family, description, &best_description, &best_face))
	return NULL;
      
      pango_font_description_set_size (best_description, size);

      best_font = pango_core_text_font_map_lookup (ctfontmap,
                                                   context,
                                                   best_description,
                                                   best_face);

      if (best_font)
        g_object_ref (best_font);
      else
        {
          PangoCoreTextFontMapClass *klass;

          klass = PANGO_CORE_TEXT_FONT_MAP_GET_CLASS (ctfontmap);
          best_font = klass->create_font (ctfontmap, context,
                                          best_face, best_description);

          if (best_font)
            pango_core_text_font_map_add (ctfontmap, context, best_font);
          /* FIXME: Handle the else case here. */
        }

      pango_font_description_free (best_description);

      return (PangoFont *)best_font;
    }

  return NULL;
}

static void
list_families_foreach (gpointer key,
		       gpointer value,
		       gpointer user_data)
{
  GSList **list = user_data;

  *list = g_slist_prepend (*list, value);
}

static void
pango_core_text_font_map_list_families (PangoFontMap      *fontmap,
                                        PangoFontFamily ***families,
                                        int               *n_families)
{
  GSList *family_list = NULL;
  GSList *tmp_list;
  PangoCoreTextFontMap *ctfontmap = (PangoCoreTextFontMap *)fontmap;

  if (!n_families)
    return;

  g_hash_table_foreach (ctfontmap->families,
                        list_families_foreach, &family_list);

  *n_families = g_slist_length (family_list);

  if (families)
    {
      int i = 0;

      *families = g_new (PangoFontFamily *, *n_families);

      tmp_list = family_list;
      while (tmp_list)
	{
	  (*families)[i] = tmp_list->data;
	  i++;
	  tmp_list = tmp_list->next;
	}
    }

  g_slist_free (family_list);
}

static void
pango_core_text_font_map_init (PangoCoreTextFontMap *ctfontmap)
{
  PangoCoreTextFamily *family;
  CTFontCollectionRef collection;
  CFArrayRef ctfaces;
  CFIndex i, count;

  ctfontmap->families = g_hash_table_new_full (g_str_hash, g_str_equal,
                                               g_free, g_object_unref);


  ctfontmap->font_hash = g_hash_table_new_full ((GHashFunc)font_hash_key_hash,
                                                (GEqualFunc)font_hash_key_equal,
                                                (GDestroyNotify)font_hash_key_free,
                                                NULL);

  collection = CTFontCollectionCreateFromAvailableFonts (0);
  ctfaces = CTFontCollectionCreateMatchingFontDescriptors (collection);
  count = CFArrayGetCount (ctfaces);

  for (i = 0; i < count; i++)
    {
      int font_traits;
      char *buffer;
      char *family_name;
      CFStringRef str;
      CFNumberRef number;
      CFDictionaryRef dict;
      CTFontDescriptorRef desc = CFArrayGetValueAtIndex (ctfaces, i);

      str = CTFontDescriptorCopyAttribute (desc, kCTFontFamilyNameAttribute);
      buffer = gchar_from_cf_string (str);

      family_name = g_utf8_casefold (buffer, -1);

      CFRelease (str);
      g_free (buffer);

      family = g_hash_table_lookup (ctfontmap->families, family_name);
      if (!family)
        {
          family = g_object_new (PANGO_TYPE_CORE_TEXT_FAMILY, NULL);
          g_hash_table_insert (ctfontmap->families, g_strdup (family_name),
                               family);

          family->family_name = family_name;
          family_name = NULL;
        }

      if (family_name)
        g_free (family_name);

      /* We assume that all faces in the family are monospaced or none. */
      dict = CTFontDescriptorCopyAttribute (desc, kCTFontTraitsAttribute);
      number = (CFNumberRef)CFDictionaryGetValue (dict,
                                                  kCTFontSymbolicTrait);

      if (CFNumberGetValue (number, kCFNumberIntType, &font_traits))
        {
          if (font_traits & kCTFontMonoSpaceTrait)
            family->is_monospace = TRUE;
        }

      CFRelease (dict);
    }

  /* Insert aliases */
  family = g_object_new (PANGO_TYPE_CORE_TEXT_FAMILY, NULL);
  family->family_name = g_strdup ("Sans");
  g_hash_table_insert (ctfontmap->families,
                       g_utf8_casefold (family->family_name, -1), family);

  family = g_object_new (PANGO_TYPE_CORE_TEXT_FAMILY, NULL);
  family->family_name = g_strdup ("Serif");
  g_hash_table_insert (ctfontmap->families,
                       g_utf8_casefold (family->family_name, -1), family);

  family = g_object_new (PANGO_TYPE_CORE_TEXT_FAMILY, NULL);
  family->family_name = g_strdup ("Monospace");
  family->is_monospace = TRUE;
  g_hash_table_insert (ctfontmap->families,
                       g_utf8_casefold (family->family_name, -1), family);
}

static void
pango_core_text_font_map_class_init (PangoCoreTextFontMapClass *class)
{
  GObjectClass *object_class = G_OBJECT_CLASS (class);
  PangoFontMapClass *fontmap_class = PANGO_FONT_MAP_CLASS (class);

  object_class->finalize = pango_core_text_font_map_finalize;

  fontmap_class->load_font = pango_core_text_font_map_load_font;
  fontmap_class->list_families = pango_core_text_font_map_list_families;
  fontmap_class->shape_engine_type = PANGO_RENDER_TYPE_CORE_TEXT;
}
