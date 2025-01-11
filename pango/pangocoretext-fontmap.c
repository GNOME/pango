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
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Library General Public License for more details.
 *
 * You should have received a copy of the GNU Library General Public
 * License along with this library; if not, write to the
 * Free Software Foundation, Inc., 59 Temple Place - Suite 330,
 * Boston, MA 02111-1307, USA.
 */

#include "config.h"

#include "pango-fontmap.h"
#include "pango-fontset.h"
#include "pangocoretext-private.h"
#include "pango-impl-utils.h"

#include <Carbon/Carbon.h>

typedef struct _FontHashKey      FontHashKey;

typedef struct _PangoCoreTextFontset PangoCoreTextFontset;

#define PANGO_TYPE_CORE_TEXT_FAMILY              (pango_core_text_family_get_type ())
#define PANGO_CORE_TEXT_FAMILY(object)           (G_TYPE_CHECK_INSTANCE_CAST ((object), PANGO_TYPE_CORE_TEXT_FAMILY, PangoCoreTextFamily))
#define PANGO_IS_CORE_TEXT_FAMILY(object)        (G_TYPE_CHECK_INSTANCE_TYPE ((object), PANGO_TYPE_CORE_TEXT_FAMILY))
#define PANGO_CORE_TEXT_FAMILY_CLASS(klass)     (G_TYPE_CHECK_CLASS_CAST ((klass), PANGO_CORE_TEXT_FAMILY, PangoCoreTextFamilyClass))
#define PANGO_IS_CORE_TEXT_FAMILY_CLASS(klass)  (G_TYPE_CHECK_CLASS_TYPE ((klass), PANGO_CORE_TEXT_FAMILY))
#define PANGO_CORE_TEXT_FAMILY_GET_CLASS(object) (G_TYPE_INSTANCE_GET_CLASS((object), PANGO_CORE_TEXT_FAMILY, PangoCoreTextFamilyClass))

#define PANGO_TYPE_CORE_TEXT_FONTSET           (pango_core_text_fontset_get_type ())
#define PANGO_CORE_TEXT_FONTSET(object)        (G_TYPE_CHECK_INSTANCE_CAST ((object), PANGO_TYPE_CORE_TEXT_FONTSET, PangoCoreTextFontset))
#define PANGO_IS_CORE_TEXT_FONTSET(object)     (G_TYPE_CHECK_INSTANCE_TYPE ((object), PANGO_TYPE_CORE_TEXT_FONTSET))


struct _PangoCoreTextFamily
{
  PangoFontFamily parent_instance;

  char *family_name;

  guint is_monospace : 1;

  PangoFontFace **faces;
  gint n_faces;
};

struct _PangoCoreTextFamilyClass
{
  PangoFontFamilyClass parent_class;
};

typedef struct _PangoCoreTextFamilyClass PangoCoreTextFamilyClass;

#define PANGO_TYPE_CORE_TEXT_FACE              (pango_core_text_face_get_type ())
#define PANGO_CORE_TEXT_FACE(object)           (G_TYPE_CHECK_INSTANCE_CAST ((object), PANGO_TYPE_CORE_TEXT_FACE, PangoCoreTextFace))
#define PANGO_IS_CORE_TEXT_FACE(object)        (G_TYPE_CHECK_INSTANCE_TYPE ((object), PANGO_TYPE_CORE_TEXT_FACE))
#define PANGO_CORE_TEXT_FACE_CLASS(klass)       (G_TYPE_CHECK_CLASS_CAST ((klass), PANGO_CORE_TEXT_FACE, PangoCoreTextFaceClass))
#define PANGO_IS_CORE_TEXT_FACE_CLASS(klass)    (G_TYPE_CHECK_CLASS_TYPE ((klass), PANGO_CORE_TEXT_FACE))
#define PANGO_CORE_TEXT_FACE_GET_CLASS(object) (G_TYPE_INSTANCE_GET_CLASS((object), PANGO_CORE_TEXT_FACE, PangoCoreTextFaceClass))

struct _PangoCoreTextFace
{
  PangoFontFace parent_instance;

  PangoCoreTextFamily *family;

  CTFontDescriptorRef ctfontdescriptor;

  char *style_name;
  PangoWeight weight;
  CTFontSymbolicTraits traits;
  guint synthetic_italic : 1;
  guint synthetic_small_caps : 1;
};

struct _PangoCoreTextFaceClass
{
  PangoFontFaceClass parent_class;
};

typedef struct _PangoCoreTextFaceClass PangoCoreTextFaceClass;

static GType pango_core_text_family_get_type (void);
static GType pango_core_text_face_get_type (void);
static GType pango_core_text_fontset_get_type (void);

static PangoCoreTextFontset    *pango_core_text_fontset_new     (PangoCoreTextFontsetKey    *key,
                                                                 const PangoFontDescription *description);
static PangoCoreTextFontsetKey *pango_core_text_fontset_get_key (PangoCoreTextFontset       *fontset);

/*
 * Helper functions to translate CoreText data to Pango
 */

typedef struct
{
    float ct_weight;
    PangoWeight pango_weight;
} PangoCTWeight;

#define ct_weight_min -0.7f
#define ct_weight_max  0.8f

/* This map is based on empirical data from analyzing a large collection of
 * fonts and comparing the opentype value with the value that OSX returns.
 * see: https://bugzilla.gnome.org/show_bug.cgi?id=766148
 * FIXME: This need recalibrating, values outside these bounds do occur!
 */

static const PangoCTWeight ct_weight_map[] = {
    { ct_weight_min, PANGO_WEIGHT_THIN },
    { -0.5, PANGO_WEIGHT_ULTRALIGHT },
    { -0.23, PANGO_WEIGHT_LIGHT },
    { -0.115, PANGO_WEIGHT_SEMILIGHT },
    {  0.00, PANGO_WEIGHT_NORMAL },
    {  0.2, PANGO_WEIGHT_MEDIUM },
    {  0.3, PANGO_WEIGHT_SEMIBOLD },
    {  0.4, PANGO_WEIGHT_BOLD },
    {  0.6, PANGO_WEIGHT_ULTRABOLD },
    {  ct_weight_max, PANGO_WEIGHT_HEAVY }
};

static const char *
get_real_family (const char *family_name)
{
  switch (family_name[0])
    {
    case 'c':
    case 'C':
      if (g_ascii_strcasecmp (family_name, "cursive") == 0)
        return "Apple Chancery";
      break;
    case 'f':
    case 'F':
      if (g_ascii_strcasecmp (family_name, "fantasy") == 0)
        return "Papyrus";
      break;
    case 'm':
    case 'M':
      if (g_ascii_strcasecmp (family_name, "monospace") == 0)
        return "Menlo";
      break;
    case 's':
    case 'S':
      if (g_ascii_strcasecmp (family_name, "sans") == 0)
        return "Helvetica";
      else if (g_ascii_strcasecmp (family_name, "serif") == 0)
        return "Times";
      else if (g_ascii_strcasecmp (family_name, "system-ui") == 0)
        return ".AppleSystemUIFont";
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

static char *
ct_font_descriptor_get_family_name (CTFontDescriptorRef desc,
                                    gboolean            may_fail)
{
  CFStringRef cf_str;
  char *buffer;

  cf_str = CTFontDescriptorCopyAttribute (desc, kCTFontFamilyNameAttribute);
  if (!cf_str)
    {
      int i;

      /* No font family name is set, try to retrieve font name and deduce
       * the family name from that instead.
       */
      cf_str = CTFontDescriptorCopyAttribute (desc, kCTFontNameAttribute);
      if (!cf_str)
        {
          if (may_fail)
            return NULL;

          /* This font is likely broken, return a default family name ... */
          return g_strdup ("Sans");
        }

      buffer = gchar_from_cf_string (cf_str);
      CFRelease (cf_str);

      for (i = 0; i < strlen (buffer); i++)
        if (buffer[i] == '-')
          break;

      if (i < strlen (buffer))
        {
          char *ret;

          ret = g_strndup (buffer, i);
          g_free (buffer);

          return ret;
        }
      else
        return buffer;
    }
  /* else */

  buffer = gchar_from_cf_string (cf_str);
  CFRelease (cf_str);

  return buffer;
}

static char *
ct_font_descriptor_get_style_name (CTFontDescriptorRef desc)
{
  CFStringRef cf_str;
  char *buffer;

  cf_str = CTFontDescriptorCopyAttribute (desc, kCTFontStyleNameAttribute);
  if (!cf_str)
    return NULL;

  buffer = gchar_from_cf_string (cf_str);
  CFRelease (cf_str);

  return buffer;
}

static CTFontSymbolicTraits
ct_font_descriptor_get_traits (CTFontDescriptorRef desc)
{
  CFDictionaryRef dict;
  CFNumberRef cf_number;
  SInt64 traits;

  /* This is interesting, the value stored is a CTFontSymbolicTraits which
   * is defined as uint32_t.  CFNumber does not have an obvious type which
   * deals with unsigned values.  Upon inspection with CFNumberGetType,
   * it turns out this value is stored as SInt64, so we use that to
   * obtain the value from the CFNumber.
   */
  dict = CTFontDescriptorCopyAttribute (desc, kCTFontTraitsAttribute);
  cf_number = (CFNumberRef)CFDictionaryGetValue (dict, kCTFontSymbolicTrait);
  if (!(cf_number && CFNumberGetValue (cf_number, kCFNumberSInt64Type, &traits)))
    traits = 0;
  CFRelease (dict);

  return (CTFontSymbolicTraits)traits;
}

static CTFontDescriptorRef
cf_font_descriptor_copy_with_traits (CTFontDescriptorRef        desc,
                                     const CTFontSymbolicTraits traits)
{
  CFMutableDictionaryRef dict, traits_dict;
  CFDictionaryRef tmp;
  CTFontDescriptorRef new_desc;
  SInt64 tmp_traits;

  tmp = CTFontDescriptorCopyAttributes (desc);
  dict = CFDictionaryCreateMutableCopy (kCFAllocatorDefault, 0, tmp);
  CFRelease (tmp);

  tmp = CTFontDescriptorCopyAttribute (desc, kCTFontTraitsAttribute);
  traits_dict = CFDictionaryCreateMutableCopy (kCFAllocatorDefault, 0, tmp);
  CFRelease (tmp);

  tmp_traits = traits;
  CFDictionarySetValue (traits_dict, (CFTypeRef) kCTFontSymbolicTrait,
                        CFNumberCreate (kCFAllocatorDefault, kCFNumberSInt64Type, &tmp_traits));

  CFDictionarySetValue (dict, (CFTypeRef)kCTFontTraitsAttribute, traits_dict);

  new_desc = CTFontDescriptorCreateCopyWithAttributes (desc, dict);
  CFRelease (dict);

  return new_desc;
}

static int
lerp(float x, float x1, float x2, int y1, int y2) {
  float dx = x2 - x1;
  int dy = y2 - y1;
  return y1 + (dy*(x-x1) + dx/2) / dx;
}

static PangoWeight
ct_font_descriptor_get_weight (CTFontDescriptorRef desc)
{
  CFDictionaryRef dict;
  CFNumberRef cf_number;
  CGFloat value;
  PangoWeight weight = PANGO_WEIGHT_NORMAL;
  guint i;

  dict = CTFontDescriptorCopyAttribute (desc, kCTFontTraitsAttribute);
  cf_number = (CFNumberRef)CFDictionaryGetValue (dict,
                                                 kCTFontWeightTrait);

  if (cf_number != NULL && CFNumberGetValue (cf_number, kCFNumberCGFloatType, &value))
    {
    if (!(value >= ct_weight_min && value <= ct_weight_max))
      {
        i = value > ct_weight_max ? G_N_ELEMENTS (ct_weight_map) - 1 : 0;
        weight = ct_weight_map[i].pango_weight;
      }
    else
      {
        for (i = 1; value > ct_weight_map[i].ct_weight; ++i)
          ;

        weight = lerp(value, ct_weight_map[i-1].ct_weight, ct_weight_map[i].ct_weight,
                      ct_weight_map[i-1].pango_weight, ct_weight_map[i].pango_weight);

      }
    }
  else
    weight = PANGO_WEIGHT_NORMAL;

  CFRelease (dict);

  return weight;
}

static gboolean
ct_font_descriptor_is_small_caps (CTFontDescriptorRef desc)
{
  CFIndex i, count;
  CFArrayRef array;
  CFStringRef str;
  gboolean retval = FALSE;

  /* See http://stackoverflow.com/a/4811371 for why this works and an
   * explanation of the magic number "3" used below.
   */
  array = CTFontDescriptorCopyAttribute (desc, kCTFontFeaturesAttribute);
  if (!array)
    return FALSE;

  str = CFStringCreateWithCString (NULL, "CTFeatureTypeIdentifier",
                                   kCFStringEncodingASCII);

  count = CFArrayGetCount (array);
  for (i = 0; i < count; i++)
    {
      CFDictionaryRef dict = CFArrayGetValueAtIndex (array, i);
      CFNumberRef num;

      num = (CFNumberRef)CFDictionaryGetValue (dict, str);
      if (num)
        {
          int value = 0;

          if (CFNumberGetValue (num, kCFNumberSInt32Type, &value) &&
              value == 3)
            {
              /* This font supports small caps. */
              retval = TRUE;
              break;
            }
        }
    }

  CFRelease (str);
  CFRelease (array);

  return retval;
}

static inline gboolean
pango_core_text_style_name_is_oblique (const char *style_name)
{
  if (!style_name)
    return FALSE;

  return g_strrstr (style_name, "Oblique") != NULL;
}

PangoFontDescription *
_pango_core_text_font_description_from_ct_font_descriptor (CTFontDescriptorRef desc)
{
  SInt64 font_traits;
  char *family_name;
  char *style_name;
  PangoFontDescription *font_desc;
  CFNumberRef cf_number;
  CGFloat pointsize;

  font_desc = pango_font_description_new ();

  /* Family name */

  /* FIXME: Should we actually retrieve the family name from the list of
   * families in a font map?
   */
  family_name = ct_font_descriptor_get_family_name (desc, FALSE);
  pango_font_description_set_family (font_desc, family_name);
  g_free (family_name);

  /* Size (if we have one) */
  cf_number = CTFontDescriptorCopyAttribute (desc, kCTFontSizeAttribute);
  if (cf_number != NULL && CFNumberGetValue (cf_number, kCFNumberCGFloatType, &pointsize))
    pango_font_description_set_size (font_desc, (pointsize / (96./72.)) * 1024);

  /* Weight */
  pango_font_description_set_weight (font_desc,
                                     ct_font_descriptor_get_weight (desc));

  /* Font traits, style name; from this we deduce style and variant */
  font_traits = ct_font_descriptor_get_traits (desc);
  style_name = ct_font_descriptor_get_style_name (desc);

  if ((font_traits & kCTFontItalicTrait) == kCTFontItalicTrait)
    pango_font_description_set_style (font_desc, PANGO_STYLE_ITALIC);
  else if (pango_core_text_style_name_is_oblique (style_name))
    pango_font_description_set_style (font_desc, PANGO_STYLE_OBLIQUE);
  else
    pango_font_description_set_style (font_desc, PANGO_STYLE_NORMAL);

  if ((font_traits & kCTFontCondensedTrait) == kCTFontCondensedTrait)
    pango_font_description_set_stretch (font_desc, PANGO_STRETCH_CONDENSED);

  if (ct_font_descriptor_is_small_caps (desc))
    pango_font_description_set_variant (font_desc, PANGO_VARIANT_SMALL_CAPS);
  else
    pango_font_description_set_variant (font_desc, PANGO_VARIANT_NORMAL);

  g_free (style_name);

  return font_desc;
}

/*
 * PangoCoreTextFace
 */

static inline gboolean
pango_core_text_face_is_oblique (PangoCoreTextFace *face)
{
  return pango_core_text_style_name_is_oblique (face->style_name);
}

static void
pango_core_text_face_make_italic (PangoCoreTextFace *ctface,
                                  gboolean           synthetic_italic)
{
  CTFontDescriptorRef new_desc;

  ctface->traits |= kCTFontItalicTrait;
  if (synthetic_italic)
    ctface->synthetic_italic = TRUE;

  /* Update the font descriptor */
  new_desc = cf_font_descriptor_copy_with_traits (ctface->ctfontdescriptor,
                                                  ctface->traits);
  CFRelease (ctface->ctfontdescriptor);
  ctface->ctfontdescriptor = new_desc;
}

static inline PangoCoreTextFace *
pango_core_text_face_copy (const PangoCoreTextFace *old)
{
  PangoCoreTextFace *face;

  face = g_object_new (PANGO_TYPE_CORE_TEXT_FACE, NULL);
  face->family = old->family;
  face->ctfontdescriptor = CFRetain (old->ctfontdescriptor);
  face->style_name = g_strdup (old->style_name);
  face->weight = old->weight;
  face->traits = old->traits;
  face->synthetic_italic = old->synthetic_italic;
  face->synthetic_small_caps = old->synthetic_small_caps;

  return face;
}

static inline PangoCoreTextFace *
pango_core_text_face_from_ct_font_descriptor (CTFontDescriptorRef desc)
{
  PangoCoreTextFace *face = g_object_new (PANGO_TYPE_CORE_TEXT_FACE,
                                          NULL);

  face->synthetic_italic = FALSE;
  face->synthetic_small_caps = FALSE;

  face->ctfontdescriptor = CFRetain (desc);

  face->style_name = ct_font_descriptor_get_style_name (desc);
  face->traits = ct_font_descriptor_get_traits (desc);
  face->weight = ct_font_descriptor_get_weight (desc);

  return face;
}

static PangoFontDescription *
pango_core_text_face_describe (PangoFontFace *face)
{
  PangoCoreTextFace *ctface = PANGO_CORE_TEXT_FACE (face);

  PangoFontDescription * ret = _pango_core_text_font_description_from_ct_font_descriptor (ctface->ctfontdescriptor);

  if (ctface->synthetic_small_caps)
    pango_font_description_set_variant (ret, PANGO_VARIANT_SMALL_CAPS);

  return ret;
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
  if (sizes)
    *sizes = NULL;
}

G_DEFINE_TYPE (PangoCoreTextFace, pango_core_text_face, PANGO_TYPE_FONT_FACE);

static void
pango_core_text_face_init (PangoCoreTextFace *face)
{
  face->family = NULL;
}

static void
pango_core_text_face_finalize (GObject *object)
{
  PangoCoreTextFace *ctface = PANGO_CORE_TEXT_FACE (object);

  g_free (ctface->style_name);
  CFRelease (ctface->ctfontdescriptor);

  G_OBJECT_CLASS (pango_core_text_face_parent_class)->finalize (object);
}

static gboolean
pango_core_text_face_is_synthesized (PangoFontFace *face)
{
  PangoCoreTextFace *cface = PANGO_CORE_TEXT_FACE (face);

  return cface->synthetic_italic || cface->synthetic_small_caps;
}

static PangoFontFamily *
pango_core_text_face_get_family (PangoFontFace *face)
{
  PangoCoreTextFace *cface = PANGO_CORE_TEXT_FACE (face);

  return PANGO_FONT_FAMILY (cface->family);
}

static void
pango_core_text_face_class_init (PangoCoreTextFaceClass *klass)
{
  GObjectClass *object_class = (GObjectClass *)klass;
  PangoFontFaceClass *pfclass = PANGO_FONT_FACE_CLASS(klass);

  object_class->finalize = pango_core_text_face_finalize;

  pfclass->describe = pango_core_text_face_describe;
  pfclass->get_face_name = pango_core_text_face_get_face_name;
  pfclass->list_sizes = pango_core_text_face_list_sizes;
  pfclass->is_synthesized = pango_core_text_face_is_synthesized;
  pfclass->get_family = pango_core_text_face_get_family;
}

/*
 * PangoCoreTextFamily
 */

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
      GHashTable *small_caps_faces;
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
      small_caps_faces = g_hash_table_new (g_direct_hash, g_direct_equal);

      count = ctfaces ? CFArrayGetCount (ctfaces) : 0;
      for (i = 0; i < count; i++)
        {
          PangoCoreTextFace *face;
          CTFontDescriptorRef desc = CFArrayGetValueAtIndex (ctfaces, i);

          face = pango_core_text_face_from_ct_font_descriptor (desc);
          face->family = ctfamily;

          faces = g_list_prepend (faces, face);

          if ((face->traits & kCTFontItalicTrait) == kCTFontItalicTrait ||
              pango_core_text_face_is_oblique (face))
            g_hash_table_insert (italic_faces,
                                 GINT_TO_POINTER ((gint)face->weight),
                                 face);

          if (ct_font_descriptor_is_small_caps (desc))
            g_hash_table_insert (small_caps_faces,
                                 GINT_TO_POINTER ((gint)face->weight),
                                 face);
        }

      CFRelease (font_descriptors);
      CFRelease (attributes);
      if (ctfaces) CFRelease (ctfaces);

      /* For all fonts for which a non-synthetic italic variant does
       * not exist on the system, we create synthesized versions here.
       */
      for (l = faces; l; l = l->next)
        {
          PangoCoreTextFace *face = l->data;

          if (!g_hash_table_lookup (italic_faces,
                                    GUINT_TO_POINTER ((gint)face->weight)))
            {
              PangoCoreTextFace *italic_face;

              italic_face = pango_core_text_face_copy (face);

              italic_face->family = ctfamily;
              pango_core_text_face_make_italic (italic_face, TRUE);

              /* Try to create a sensible face name. */
              g_free (italic_face->style_name);
              if (strcasecmp (face->style_name, "regular") == 0)
                italic_face->style_name = g_strdup ("Oblique");
              else
                italic_face->style_name = g_strdup_printf ("%s Oblique",
                                                           face->style_name);

              synthetic_faces = g_list_prepend (synthetic_faces, italic_face);
            }
          if (!g_hash_table_lookup (small_caps_faces,
                                    GINT_TO_POINTER ((gint)face->weight)))
            {
              PangoCoreTextFace *small_caps_face;

              small_caps_face = pango_core_text_face_copy (face);

              small_caps_face->family = ctfamily;
              small_caps_face->synthetic_small_caps = true;

              g_free (small_caps_face->style_name);
              if (strcasecmp (face->style_name, "regular") == 0)
                small_caps_face->style_name = g_strdup ("Small-Caps");
              else
                small_caps_face->style_name = g_strdup_printf ("%s Small-Caps",
                                                               face->style_name);

              synthetic_faces = g_list_prepend (synthetic_faces, small_caps_face);
            }
        }

      faces = g_list_concat (faces, synthetic_faces);

      ctfamily->n_faces = g_list_length (faces);
      ctfamily->faces = g_new (PangoFontFace *, ctfamily->n_faces);

      for (l = faces, i = 0; l; l = l->next, i++)
        ctfamily->faces[i] = l->data;

      g_list_free (faces);
      g_hash_table_destroy (italic_faces);
      g_hash_table_destroy (small_caps_faces);
    }

  if (n_faces)
    *n_faces = ctfamily->n_faces;

  if (faces)
    *faces = g_memdup2 (ctfamily->faces, ctfamily->n_faces * sizeof (PangoFontFace *));
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

G_DEFINE_TYPE (PangoCoreTextFamily, pango_core_text_family, PANGO_TYPE_FONT_FAMILY);

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
pango_core_text_family_class_init (PangoCoreTextFamilyClass *klass)
{
  GObjectClass *object_class = (GObjectClass *)klass;
  PangoFontFamilyClass *pfclass = PANGO_FONT_FAMILY_CLASS(klass);

  object_class->finalize = pango_core_text_family_finalize;

  pfclass->list_faces = pango_core_text_family_list_faces;
  pfclass->get_name = pango_core_text_family_get_name;
  pfclass->is_monospace = pango_core_text_family_is_monospace;
}

static void
pango_core_text_family_init (PangoCoreTextFamily *family)
{
  family->n_faces = -1;
}



static void pango_core_text_font_map_class_init (PangoCoreTextFontMapClass *class);
static void pango_core_text_font_map_init (PangoCoreTextFontMap *ctfontmap);


G_DEFINE_TYPE (PangoCoreTextFontMap, pango_core_text_font_map, PANGO_TYPE_FONT_MAP);

static void
pango_core_text_font_map_finalize (GObject *object)
{
  PangoCoreTextFontMap *fontmap = PANGO_CORE_TEXT_FONT_MAP (object);

  g_hash_table_destroy (fontmap->fontset_hash);
  g_hash_table_destroy (fontmap->font_hash);
  g_hash_table_destroy (fontmap->families);

  G_OBJECT_CLASS (pango_core_text_font_map_parent_class)->finalize (object);
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
  const PangoMatrix identity = PANGO_MATRIX_INIT;

  if (context)
    set_matrix = pango_context_get_matrix (context);
  else
    set_matrix = NULL;

  if (set_matrix)
    *matrix = *set_matrix;
  else
    *matrix = identity;
}

/*
 * Helper functions for PangoCoreTextFontsetKey
 */
static const double ppi = 72.0; /* typographic points per inch */

static double
pango_core_text_font_map_get_resolution (PangoCoreTextFontMap *fontmap,
                                         PangoContext         *context)
{
  if (PANGO_CORE_TEXT_FONT_MAP_GET_CLASS (fontmap)->get_resolution)
    return PANGO_CORE_TEXT_FONT_MAP_GET_CLASS (fontmap)->get_resolution (fontmap, context);

  /* FIXME: acquire DPI from CoreText using some deafault font */
  g_warning ("FIXME: returning default DPI");

  return ppi;
}

static int
get_scaled_size (PangoCoreTextFontMap       *fontmap,
                 PangoContext               *context,
                 const PangoFontDescription *desc)
{
  double size = pango_font_description_get_size (desc);
  const PangoMatrix *matrix = pango_context_get_matrix (context);
  double scale_factor = pango_matrix_get_font_scale_factor (matrix);
  
  if (!pango_font_description_get_size_is_absolute(desc))
  {
    double dpi = pango_core_text_font_map_get_resolution (fontmap, context);
    size *= (dpi/ppi);
  }

  return .5 +  scale_factor * size;
}


/*
 * PangoCoreTextFontsetKey
 */
struct _PangoCoreTextFontsetKey
{
  PangoCoreTextFontMap *fontmap;
  PangoLanguage *language;
  PangoFontDescription *desc;
  PangoMatrix matrix;
  int pointsize;
  double resolution;
  PangoGravity gravity;
  gpointer context_key;
};

static void
pango_core_text_fontset_key_init (PangoCoreTextFontsetKey    *key,
                                  PangoCoreTextFontMap       *fontmap,
                                  PangoContext               *context,
                                  const PangoFontDescription *desc,
                                  PangoLanguage              *language)
{
  if (!language && context)
    language = pango_context_get_language (context);

  key->fontmap = fontmap;
  get_context_matrix (context, &key->matrix);
  key->language = language;
  key->pointsize = get_scaled_size (fontmap, context, desc);
  key->resolution = pango_core_text_font_map_get_resolution (fontmap, context);
  key->gravity = pango_context_get_gravity (context);
  key->desc = pango_font_description_copy_static (desc);
  pango_font_description_unset_fields (key->desc, PANGO_FONT_MASK_SIZE);

  if (context && PANGO_CORE_TEXT_FONT_MAP_GET_CLASS (fontmap)->context_key_get)
    key->context_key = (gpointer)PANGO_CORE_TEXT_FONT_MAP_GET_CLASS (fontmap)->context_key_get (fontmap, context);
  else
    key->context_key = NULL;
}

static PangoCoreTextFontsetKey *
pango_core_text_fontset_key_copy (const PangoCoreTextFontsetKey *old)
{
  PangoCoreTextFontsetKey *key = g_slice_new (PangoCoreTextFontsetKey);

  key->fontmap = old->fontmap;
  key->matrix = old->matrix;
  key->language = old->language;
  key->pointsize = old->pointsize;
  key->resolution = old->resolution;
  key->gravity = old->gravity;
  key->desc = pango_font_description_copy (old->desc);
  if (old->context_key)
    key->context_key = PANGO_CORE_TEXT_FONT_MAP_GET_CLASS (key->fontmap)->context_key_copy (key->fontmap, old->context_key);
  else
    key->context_key = NULL;

  return key;
}

static void
pango_core_text_fontset_key_free (PangoCoreTextFontsetKey *key)
{
  pango_font_description_free (key->desc);

  if (key->context_key)
    PANGO_CORE_TEXT_FONT_MAP_GET_CLASS (key->fontmap)->context_key_free (key->fontmap, key->context_key);

  g_slice_free (PangoCoreTextFontsetKey, key);
}

static guint
pango_core_text_fontset_key_hash (const PangoCoreTextFontsetKey *key)
{
  guint32 hash = FNV1_32_INIT;

  hash = hash_bytes_fnv ((unsigned char *)(&key->matrix), sizeof (double) * 4, hash);
  hash ^= hash_bytes_fnv ((unsigned char *)(&key->resolution), sizeof (double), hash);

  if (key->context_key)
    hash ^= PANGO_CORE_TEXT_FONT_MAP_GET_CLASS (key->fontmap)->context_key_hash (key->fontmap, key->context_key);

  return (hash ^
          GPOINTER_TO_UINT (key->language) ^
          pango_font_description_hash (key->desc));
}

static gboolean
pango_core_text_fontset_key_equal (const PangoCoreTextFontsetKey *key_a,
                                   const PangoCoreTextFontsetKey *key_b)
{
  if (key_a->language == key_b->language &&
      key_a->pointsize == key_b->pointsize &&
      key_a->resolution == key_b->resolution &&
      key_a->gravity == key_b->gravity &&
      pango_font_description_equal (key_a->desc, key_b->desc) &&
      memcmp ((void *)&key_a->matrix, (void *)&key_b->matrix, 4 * sizeof (double)) == 0)
    {
      if (key_a->context_key)
        return PANGO_CORE_TEXT_FONT_MAP_GET_CLASS (key_a->fontmap)->context_key_equal (key_a->fontmap,
                                                                                       key_a->context_key,
                                                                                       key_b->context_key);
      else
        return key_a->context_key == key_b->context_key;
    }
  /* else */
  return FALSE;
}

static PangoLanguage *
pango_core_text_fontset_key_get_language (const PangoCoreTextFontsetKey *key)
{
  return key->language;
}

static const PangoMatrix *
pango_core_text_fontset_key_get_matrix (const PangoCoreTextFontsetKey *key)
{
  return &key->matrix;
}

static PangoGravity
pango_core_text_fontset_key_get_gravity (const PangoCoreTextFontsetKey *key)
{
  return key->gravity;
}

static gpointer
pango_core_text_fontset_key_get_context_key (const PangoCoreTextFontsetKey *key)
{
  return key->context_key;
}

/*
 * PangoCoreTextFontKey
 */
struct _PangoCoreTextFontKey
{
  PangoCoreTextFontMap *fontmap;
  CTFontDescriptorRef ctfontdescriptor;
  PangoMatrix matrix;
  PangoGravity gravity;
  int pointsize;
  double resolution;
  gboolean synthetic_italic;
  gboolean synthetic_small_caps;
  gpointer context_key;
  char *variations;
};

static void
pango_core_text_font_key_init (PangoCoreTextFontKey    *key,
                               PangoCoreTextFontMap    *ctfontmap,
                               PangoCoreTextFontsetKey *fontset_key,
                               CTFontDescriptorRef      ctdescriptor,
                               gboolean                 synthetic_italic,
                               gboolean                 synthetic_small_caps)
{
  key->fontmap = ctfontmap;
  key->ctfontdescriptor = ctdescriptor;
  key->matrix = *pango_core_text_fontset_key_get_matrix (fontset_key);
  key->pointsize = fontset_key->pointsize;
  key->resolution = fontset_key->resolution;
  key->synthetic_italic = synthetic_italic;
  key->synthetic_small_caps = synthetic_small_caps;
  key->gravity = pango_core_text_fontset_key_get_gravity (fontset_key);
  key->context_key = pango_core_text_fontset_key_get_context_key (fontset_key);
  key->variations = (char *) pango_font_description_get_variations (fontset_key->desc);
}

static void
pango_core_text_font_key_init_from_key (PangoCoreTextFontKey       *key,
                                        const PangoCoreTextFontKey *orig)
{
  key->fontmap = orig->fontmap;
  key->ctfontdescriptor = orig->ctfontdescriptor;
  key->matrix = orig->matrix;
  key->pointsize = orig->pointsize;
  key->resolution = orig->resolution;
  key->synthetic_italic = orig->synthetic_italic;
  key->synthetic_small_caps = orig->synthetic_small_caps;
  key->gravity = orig->gravity;
  key->context_key = orig->context_key;
  key->variations = orig->variations;
}

static PangoCoreTextFontKey *
pango_core_text_font_key_copy (const PangoCoreTextFontKey *old)
{
  PangoCoreTextFontKey *key = g_slice_new (PangoCoreTextFontKey);

  key->fontmap = old->fontmap;
  key->ctfontdescriptor = old->ctfontdescriptor;
  CFRetain (key->ctfontdescriptor);
  key->matrix = old->matrix;
  key->pointsize = old->pointsize;
  key->resolution = old->resolution;
  key->synthetic_italic = old->synthetic_italic;
  key->synthetic_small_caps = old->synthetic_small_caps;
  key->gravity = old->gravity;
  if (old->context_key)
    key->context_key = PANGO_CORE_TEXT_FONT_MAP_GET_CLASS (key->fontmap)->context_key_copy (key->fontmap, old->context_key);
  else
    key->context_key = NULL;
  key->variations = g_strdup (old->variations);

  return key;
}

static void
pango_core_text_font_key_free (PangoCoreTextFontKey *key)
{
  if (key->ctfontdescriptor)
    CFRelease (key->ctfontdescriptor);

  if (key->context_key)
    PANGO_CORE_TEXT_FONT_MAP_GET_CLASS (key->fontmap)->context_key_free (key->fontmap, key->context_key);

  g_free (key->variations);

  g_slice_free (PangoCoreTextFontKey, key);
}

static guint
pango_core_text_font_key_hash (const PangoCoreTextFontKey *key)
{
  guint32 hash = FNV1_32_INIT;

  /* Not everything is included here, probably good enough for a hash */

  hash = hash_bytes_fnv ((unsigned char *)(&key->matrix), sizeof (double) * 4, hash);

  if (key->context_key)
    hash ^= PANGO_CORE_TEXT_FONT_MAP_GET_CLASS (key->fontmap)->context_key_hash (key->fontmap, key->context_key);

  if (key->variations)
    hash ^= g_str_hash (key->variations);

  return (hash ^ CFHash (key->ctfontdescriptor));
}

static gboolean
pango_core_text_font_key_equal (const PangoCoreTextFontKey *key_a,
                                const PangoCoreTextFontKey *key_b)
{
  if (CFEqual (key_a->ctfontdescriptor, key_b->ctfontdescriptor) &&
      memcmp (&key_a->matrix, &key_b->matrix, 4 * sizeof (double)) == 0 &&
      key_a->gravity == key_b->gravity &&
      key_a->pointsize == key_b->pointsize &&
      key_a->resolution == key_b->resolution &&
      key_a->synthetic_italic == key_b->synthetic_italic &&
      key_a->synthetic_small_caps == key_b->synthetic_small_caps &&
      g_strcmp0 (key_a->variations, key_b->variations) == 0)
    {
      if (key_a->context_key && key_b->context_key)
        return PANGO_CORE_TEXT_FONT_MAP_GET_CLASS (key_a->fontmap)->context_key_equal (key_a->fontmap,
                                                                                       key_a->context_key,
                                                                                       key_b->context_key);
      else
        return key_a->context_key == key_b->context_key;
    }
  else
    return FALSE;
}

int
pango_core_text_font_key_get_size (const PangoCoreTextFontKey *key)
{
  return key->pointsize;
}

double
pango_core_text_font_key_get_resolution (const PangoCoreTextFontKey *key)
{
  return key->resolution;
}

gboolean
pango_core_text_font_key_get_synthetic_italic (const PangoCoreTextFontKey *key)
{
  return key->synthetic_italic;
}

gboolean
pango_core_text_font_key_get_synthetic_small_caps (const PangoCoreTextFontKey *key)
{
  return key->synthetic_small_caps;
}

gpointer
pango_core_text_font_key_get_context_key (const PangoCoreTextFontKey *key)
{
  return key->context_key;
}

const PangoMatrix *
pango_core_text_font_key_get_matrix (const PangoCoreTextFontKey *key)
{
  return &key->matrix;
}

PangoGravity
pango_core_text_font_key_get_gravity (const PangoCoreTextFontKey *key)
{
  return key->gravity;
}

CTFontDescriptorRef
pango_core_text_font_key_get_ctfontdescriptor (const PangoCoreTextFontKey *key)
{
  return key->ctfontdescriptor;
}

const char *
pango_core_text_font_key_get_variations (const PangoCoreTextFontKey *key)
{
  return key->variations;
}


static void
pango_core_text_font_map_add (PangoCoreTextFontMap *ctfontmap,
                              PangoCoreTextFontKey *key,
                              PangoCoreTextFont    *ctfont)
{
  PangoCoreTextFontKey *key_copy;

  _pango_core_text_font_set_font_map (ctfont, ctfontmap);

  key_copy = pango_core_text_font_key_copy (key);
  _pango_core_text_font_set_font_key (ctfont, key_copy);
  g_hash_table_insert (ctfontmap->font_hash, key_copy, ctfont);
}

static PangoCoreTextFont *
pango_core_text_font_map_new_font_from_key (PangoCoreTextFontMap *fontmap,
                                            PangoCoreTextFontKey *key)
{
  PangoCoreTextFont *font;

  font = g_hash_table_lookup (fontmap->font_hash, key);
  if (font)
    return g_object_ref (font);

  font = PANGO_CORE_TEXT_FONT_MAP_GET_CLASS (fontmap)->create_font (fontmap, key);

  if (!font)
    return NULL;

  pango_core_text_font_map_add (fontmap, key, font);

  return g_object_ref (font);
}

static PangoCoreTextFont *
pango_core_text_font_map_new_font (PangoCoreTextFontMap    *fontmap,
                                   PangoCoreTextFontsetKey *fontset_key,
                                   CTFontDescriptorRef      ctfontdescriptor,
                                   gboolean                 synthetic_italic,
                                   gboolean                 synthetic_small_caps)
{
  PangoCoreTextFontKey key;

  pango_core_text_font_key_init (&key, fontmap, fontset_key, ctfontdescriptor,
                                 synthetic_italic, synthetic_small_caps);

  return pango_core_text_font_map_new_font_from_key (fontmap, &key);
}

static gboolean
find_best_match (PangoCoreTextFamily         *font_family,
                 const PangoFontDescription  *description,
                 PangoCoreTextFace          **best_face)
{
  PangoFontDescription *new_desc;
  PangoFontDescription *best_description = NULL;
  int i;

  *best_face = NULL;

  for (i = 0; i < font_family->n_faces; i++)
    {
      new_desc = pango_font_face_describe (font_family->faces[i]);
      pango_font_description_set_gravity (new_desc, pango_font_description_get_gravity (description));

      if (pango_font_description_better_match (description, best_description,
                                               new_desc))
        {
          pango_font_description_free (best_description);
          best_description = new_desc;
          *best_face = (PangoCoreTextFace *)font_family->faces[i];
        }
      else
        pango_font_description_free (new_desc);
    }

  if (best_description)
    {
      pango_font_description_free (best_description);
      return TRUE;
    }
  return FALSE;
}

static gboolean
get_first_font (PangoFontset *fontset G_GNUC_UNUSED,
                PangoFont    *font,
                gpointer      data)
{
  *(PangoFont **)data = font;

  return TRUE;
}

static guint
pango_core_text_font_map_get_serial (PangoFontMap *fontmap)
{
  PangoCoreTextFontMap *ctfontmap = PANGO_CORE_TEXT_FONT_MAP (fontmap);

  return ctfontmap->serial;
}

static void
pango_core_text_font_map_changed (PangoFontMap *fontmap)
{
  PangoCoreTextFontMap *ctfontmap = PANGO_CORE_TEXT_FONT_MAP (fontmap);

  ctfontmap->serial++;
  if (ctfontmap->serial == 0)
    ctfontmap->serial++;
}

static PangoFont *
pango_core_text_font_map_load_font (PangoFontMap               *fontmap,
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

  fontset = pango_font_map_load_fontset (fontmap, context,
                                         description, language);

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

static PangoFontset *
pango_core_text_font_map_load_fontset (PangoFontMap               *fontmap,
                                       PangoContext               *context,
                                       const PangoFontDescription *desc,
                                       PangoLanguage              *language)
{
  PangoCoreTextFontset *fontset;
  PangoCoreTextFontsetKey key;
  PangoCoreTextFontMap *ctfontmap = PANGO_CORE_TEXT_FONT_MAP (fontmap);
  static gboolean warned_full_fallback = FALSE; /* MT-safe */

  pango_core_text_fontset_key_init (&key, ctfontmap,
                                    context, desc, language);

  fontset = g_hash_table_lookup (ctfontmap->fontset_hash, &key);

  if (G_UNLIKELY (!fontset))
    {
      gboolean insert_in_hash = TRUE;

      fontset = pango_core_text_fontset_new (&key, desc);

      if (G_UNLIKELY (!fontset))
        {
          /* If no font(set) could be loaded, we fallback to "Apple Color
           * Emoji" for emoji font, fallback to "Sans" for other fonts,
           * which should always work on Mac. We try to adhere to the
           * requested style at first.
           */
          PangoFontDescription *tmp_desc;

          /* Cannot use pango_core_text_fontset_key_free() here */
          pango_font_description_free (key.desc);

          tmp_desc = pango_font_description_copy_static (desc);
          if (strcmp (pango_font_description_get_family (tmp_desc), "emoji") == 0)
            pango_font_description_set_family_static (tmp_desc, "Apple Color Emoji");
          else
            pango_font_description_set_family_static (tmp_desc, "Sans");

          pango_core_text_fontset_key_init (&key, ctfontmap, context, tmp_desc,
                                            language);

          fontset = g_hash_table_lookup (ctfontmap->fontset_hash, &key);
          if (G_LIKELY (fontset))
            insert_in_hash = FALSE;
          else
            fontset = pango_core_text_fontset_new (&key, tmp_desc);

          if (G_UNLIKELY (!fontset))
            {
              /* We could not load Sans in the requested style; reset
               * variant, weight and stretch to sensible defaults (we should
               * be able to adhere the PangoStyle with "Sans").
               */
              pango_font_description_set_variant (tmp_desc, PANGO_VARIANT_NORMAL);
              pango_font_description_set_weight (tmp_desc, PANGO_WEIGHT_NORMAL);
              pango_font_description_set_stretch (tmp_desc, PANGO_STRETCH_NORMAL);

              if (!warned_full_fallback)
                {
                  char *ctmp;

                  warned_full_fallback = TRUE;

                  ctmp = pango_font_description_to_string (desc);
                  g_warning ("couldn't load font \"%s\", modified variant/"
                             "weight/stretch as fallback, expect ugly output.",
                             ctmp);
                  g_free (ctmp);
                }

              fontset = g_hash_table_lookup (ctfontmap->fontset_hash, &key);
              if (G_LIKELY (fontset))
                insert_in_hash = FALSE;
              else
                fontset = pango_core_text_fontset_new (&key, tmp_desc);

              if (G_UNLIKELY (!fontset))
                {
                  /* If even that failed, display a sensible error message
                   * and bail out, in contrast to failing randomly.
                   */
                  g_error ("Could not load fallback font, bailing out.");
                }
            }

          if (tmp_desc)
            pango_font_description_free (tmp_desc);
        }

      if (insert_in_hash)
        g_hash_table_insert (ctfontmap->fontset_hash,
                             pango_core_text_fontset_get_key (fontset),
                             fontset);
    }

  /* Cannot use pango_core_text_fontset_key_free() here */
  pango_font_description_free (key.desc);

  return g_object_ref (PANGO_FONTSET (fontset));
}

static void
pango_core_text_font_map_init (PangoCoreTextFontMap *ctfontmap)
{
  PangoCoreTextFamily *family;
  CTFontCollectionRef collection;
  CFArrayRef ctfaces;
  CFIndex i, count;

  ctfontmap->serial = 1;
  ctfontmap->families = g_hash_table_new_full (g_str_hash, g_str_equal,
                                               g_free, g_object_unref);


  ctfontmap->font_hash = g_hash_table_new_full ((GHashFunc)pango_core_text_font_key_hash,
                                                (GEqualFunc)pango_core_text_font_key_equal,
                                                (GDestroyNotify)pango_core_text_font_key_free,
                                                NULL);

  ctfontmap->fontset_hash = g_hash_table_new_full ((GHashFunc)pango_core_text_fontset_key_hash,
                                                   (GEqualFunc)pango_core_text_fontset_key_equal,
                                                   NULL,
                                                   (GDestroyNotify)g_object_unref);

  collection = CTFontCollectionCreateFromAvailableFonts (0);
  ctfaces = CTFontCollectionCreateMatchingFontDescriptors (collection);
  count = CFArrayGetCount (ctfaces);

  for (i = 0; i < count; i++)
    {
      SInt64 font_traits;
      char *buffer;
      char *family_name;
      CFNumberRef number;
      CFDictionaryRef dict;
      CTFontDescriptorRef desc = CFArrayGetValueAtIndex (ctfaces, i);

      buffer = ct_font_descriptor_get_family_name (desc, TRUE);
      if (!buffer)
        continue;

      family_name = g_utf8_casefold (buffer, -1);

      family = g_hash_table_lookup (ctfontmap->families, family_name);
      if (!family)
        {
          family = g_object_new (PANGO_TYPE_CORE_TEXT_FAMILY, NULL);
          g_hash_table_insert (ctfontmap->families, g_strdup (family_name),
                               family);

          family->family_name = g_strdup (buffer);
        }

      g_free (buffer);

      g_free (family_name);

      /* We assume that all faces in the family are monospaced or none. */
      dict = CTFontDescriptorCopyAttribute (desc, kCTFontTraitsAttribute);
      number = (CFNumberRef)CFDictionaryGetValue (dict,
                                                  kCTFontSymbolicTrait);

      if (CFNumberGetValue (number, kCFNumberSInt64Type, &font_traits))
        {
          if ((font_traits & kCTFontMonoSpaceTrait) == kCTFontMonoSpaceTrait)
            family->is_monospace = TRUE;
        }

      CFRelease (dict);
    }

  /* Insert aliases */
  /* Keep in sync with get_real_family() */
  gchar* aliases[] = {
    "Sans", "Serif", "system-ui", "cursive", "fantasy", "Monospace"
  };

  for (int i = 0; i <  G_N_ELEMENTS(aliases); i++)
  {
    family = g_object_new (PANGO_TYPE_CORE_TEXT_FAMILY, NULL);
    family->family_name = g_strdup (aliases[i]);
    if (g_ascii_strcasecmp (family->family_name, "Monospace") == 0)
      family->is_monospace = TRUE;
    g_hash_table_insert (ctfontmap->families,
                         g_utf8_casefold (family->family_name, -1), family);
  }
  /* Insert .AppleSystemUIFont because it isn't included in the result
   * set from CTFontCollectionCreateFromAvailableFonts. If it's not
   * included in the families fontset it can't be used to render text.
   */
  family = g_object_new (PANGO_TYPE_CORE_TEXT_FAMILY, NULL);
  family->family_name = g_strdup (".AppleSystemUIFont");
    g_hash_table_insert (ctfontmap->families,
                         g_utf8_casefold (family->family_name, -1), family);

}

static PangoFontFace *
pango_core_text_font_map_get_face (PangoFontMap *fontmap,
                                   PangoFont    *font)
{
  PangoCoreTextFont *cfont = PANGO_CORE_TEXT_FONT (font);

  return PANGO_FONT_FACE (_pango_core_text_font_get_face (cfont));
}

static PangoFont *
pango_core_text_font_map_reload_font (PangoFontMap *fontmap,
                                      PangoFont    *font,
                                      double        scale,
                                      PangoContext *context,
                                      const char   *variations)
{
  PangoCoreTextFontMap *ctfontmap = PANGO_CORE_TEXT_FONT_MAP (fontmap);
  PangoCoreTextFont *ctfont = PANGO_CORE_TEXT_FONT (font);
  PangoCoreTextFontKey key;
  PangoFont *scaled;

  pango_core_text_font_key_init_from_key (&key, _pango_core_text_font_get_font_key (ctfont));

  if (scale != 1.0)
    key.pointsize *= scale;

  if (context)
    {
      get_context_matrix (context, &key.matrix);
      if (context && PANGO_CORE_TEXT_FONT_MAP_GET_CLASS (fontmap)->context_key_get)
        key.context_key = (gpointer) PANGO_CORE_TEXT_FONT_MAP_GET_CLASS (fontmap)->context_key_get (ctfontmap, context);
    }

  if (variations)
    g_warning_once ("pango_core_text_font_map_reload_font: variations are ignored");

  scaled = (PangoFont *)pango_core_text_font_map_new_font_from_key (ctfontmap, &key);

  return scaled;
}

static void
pango_core_text_font_map_class_init (PangoCoreTextFontMapClass *class)
{
  GObjectClass *object_class = G_OBJECT_CLASS (class);
  PangoFontMapClass *fontmap_class = PANGO_FONT_MAP_CLASS (class);
  PangoFontMapClassPrivate *pclass;

  object_class->finalize = pango_core_text_font_map_finalize;

  fontmap_class->load_font = pango_core_text_font_map_load_font;
  fontmap_class->list_families = pango_core_text_font_map_list_families;
  fontmap_class->load_fontset = pango_core_text_font_map_load_fontset;
  fontmap_class->shape_engine_type = PANGO_RENDER_TYPE_CORE_TEXT;
  fontmap_class->get_serial = pango_core_text_font_map_get_serial;
  fontmap_class->changed = pango_core_text_font_map_changed;
  fontmap_class->get_face = pango_core_text_font_map_get_face;

  pclass = g_type_class_get_private ((GTypeClass *) class, PANGO_TYPE_FONT_MAP);

  pclass->reload_font = pango_core_text_font_map_reload_font;
}

/*
 * PangoCoreTextFontSet
 */

static void              pango_core_text_fontset_finalize     (GObject                   *object);
static void              pango_core_text_fontset_init         (PangoCoreTextFontset      *fontset);
static PangoLanguage *   pango_core_text_fontset_get_language (PangoFontset              *fontset);
static  PangoFont *      pango_core_text_fontset_get_font     (PangoFontset              *fontset,
                                                               guint                      wc);
static void              pango_core_text_fontset_foreach      (PangoFontset              *fontset,
                                                               PangoFontsetForeachFunc    func,
                                                               gpointer                   data);

struct _PangoCoreTextFontset
{
  PangoFontset parent_instance;

  const gchar *orig_family;
  PangoFontDescription *orig_description;

  PangoCoreTextFontsetKey *key;
  CFArrayRef cascade_list;

  GPtrArray *fonts;
  GPtrArray *coverages;
  guint real_font_count;
};

struct _PangoCoreTextFontsetClass
{
  PangoFontsetClass parent_instance;
};

typedef struct _PangoCoreTextFontsetClass PangoCoreTextFontsetClass;

G_DEFINE_TYPE (PangoCoreTextFontset,
               pango_core_text_fontset,
               PANGO_TYPE_FONTSET);

static PangoCoreTextFontset *
pango_core_text_fontset_new (PangoCoreTextFontsetKey    *key,
                             const PangoFontDescription *description)
{
  PangoCoreTextFamily *font_family;
  PangoCoreTextFontset *fontset;
  PangoCoreTextFont *best_font = NULL;
  gchar **family_names;
  const gchar *family;
  gchar *name;
  int i;

  fontset = g_object_new (PANGO_TYPE_CORE_TEXT_FONTSET, NULL);
  family = pango_font_description_get_family (description);
  family_names = g_strsplit (family ? family : "", ",", -1);

  for (i = 0; family_names[i]; ++i)
    {
      name = g_utf8_casefold (family_names[i], -1);
      font_family = g_hash_table_lookup (key->fontmap->families, name);
      g_free (name);

      if (font_family)
        {
          PangoCoreTextFace *family_face;
          PangoCoreTextFont *font;

          /* Force a listing of the available faces */
          pango_font_family_list_faces ((PangoFontFamily *)font_family, NULL, NULL);

          if (find_best_match (font_family, description, &family_face))
            {
              font = pango_core_text_font_map_new_font (key->fontmap,
                                                        key,
                                                        family_face->ctfontdescriptor,
                                                        family_face->synthetic_italic,
                                                        family_face->synthetic_small_caps);

              if (font)
                {
                  g_ptr_array_add (fontset->fonts, font);
                  if (best_font == NULL) best_font = font;
                }
            }
        }
    }

  g_strfreev (family_names);

  if (!best_font)
    {
      g_object_unref (fontset);
      return NULL;
    }

  /* Create a font set with best font */
  fontset->key = pango_core_text_fontset_key_copy (key);
  fontset->orig_description = pango_font_description_copy (description);

  fontset->real_font_count = fontset->fonts->len;

  /* Add the cascade list for this language */
  CFArrayRef language_pref_list = NULL;
  CFStringRef languages[1];

  if (key->language)
    {
      languages[0] = CFStringCreateWithCString (NULL,
                                                pango_language_to_string (key->language),
                                                kCFStringEncodingASCII);
      language_pref_list = CFArrayCreate (kCFAllocatorDefault,
                                          (const void **) languages,
                                          1,
                                          &kCFTypeArrayCallBacks);
    }

  fontset->cascade_list = CTFontCopyDefaultCascadeListForLanguages (pango_core_text_font_get_ctfont (best_font), language_pref_list);

  if (language_pref_list)
    {
      CFRelease (languages[0]);
      CFRelease (language_pref_list);
    }

  /* length of cascade list + real_font_count for the "real" fonts at the front */
  g_ptr_array_set_size (fontset->fonts, CFArrayGetCount (fontset->cascade_list) + fontset->real_font_count);
  g_ptr_array_set_size (fontset->coverages, CFArrayGetCount (fontset->cascade_list) + fontset->real_font_count);

  return fontset;
}

static PangoFont *
pango_core_text_fontset_load_font (PangoCoreTextFontset *ctfontset,
                                   CTFontDescriptorRef   ctdescriptor)
{
  PangoCoreTextFont *font;

  /* For now, we will default the fallbacks to not have synthetic italic,
   * in the future this may be improved.
   */
  font = pango_core_text_font_map_new_font (ctfontset->key->fontmap,
                                            ctfontset->key,
                                            ctdescriptor,
                                            FALSE,
                                            FALSE);

  return PANGO_FONT (font);
}

static PangoFont *
pango_core_text_fontset_get_font_at (PangoCoreTextFontset *ctfontset,
                                     unsigned int          i)
{
  /* These fonts are loaded as soon as the fontset is created */
  if (i < ctfontset->real_font_count)
    return g_ptr_array_index (ctfontset->fonts, i);

  if (i >= ctfontset->fonts->len)
    return NULL;

  if (g_ptr_array_index (ctfontset->fonts, i) == NULL)
    {
      CTFontDescriptorRef ctdescriptor = CFArrayGetValueAtIndex (ctfontset->cascade_list, i - ctfontset->real_font_count);
      PangoFont *font = pango_core_text_fontset_load_font (ctfontset, ctdescriptor);
      g_ptr_array_index (ctfontset->fonts, i) = font;
      g_ptr_array_index (ctfontset->coverages, i) = NULL;
    }

  return g_ptr_array_index (ctfontset->fonts, i);
}

static void
pango_core_text_fontset_class_init (PangoCoreTextFontsetClass *klass)
{
  GObjectClass *object_class = G_OBJECT_CLASS (klass);
  PangoFontsetClass *fontset_class = PANGO_FONTSET_CLASS (klass);

  object_class->finalize = pango_core_text_fontset_finalize;

  fontset_class->get_font = pango_core_text_fontset_get_font;
  fontset_class->get_language = pango_core_text_fontset_get_language;
  fontset_class->foreach = pango_core_text_fontset_foreach;
}

static void
pango_core_text_fontset_init (PangoCoreTextFontset *ctfontset)
{
  ctfontset->key = NULL;
  ctfontset->cascade_list = NULL;
  ctfontset->fonts = g_ptr_array_new ();
  ctfontset->coverages = g_ptr_array_new ();
  ctfontset->real_font_count = 0;
}

static void
pango_core_text_fontset_finalize (GObject *object)
{
  PangoCoreTextFontset *ctfontset = PANGO_CORE_TEXT_FONTSET (object);
  unsigned int i;

  for (i = 0; i < ctfontset->fonts->len; i++)
    {
      PangoFont *font = g_ptr_array_index (ctfontset->fonts, i);
      if (font)
        g_object_unref (font);
    }
  g_ptr_array_free (ctfontset->fonts, TRUE);

  for (i = 0; i < ctfontset->coverages->len; i++)
    {
      PangoCoverage *coverage = g_ptr_array_index (ctfontset->coverages, i);
      if (coverage)
        g_object_unref (coverage);
    }
  g_ptr_array_free (ctfontset->coverages, TRUE);

  if (ctfontset->cascade_list)
    CFRelease (ctfontset->cascade_list);

  pango_font_description_free (ctfontset->orig_description);

  if (ctfontset->key)
    pango_core_text_fontset_key_free (ctfontset->key);

  G_OBJECT_CLASS (pango_core_text_fontset_parent_class)->finalize (object);
}

static PangoCoreTextFontsetKey *
pango_core_text_fontset_get_key (PangoCoreTextFontset *fontset)
{
  return fontset->key;
}

static PangoLanguage *
pango_core_text_fontset_get_language (PangoFontset *fontset)
{
  PangoCoreTextFontset *ctfontset = PANGO_CORE_TEXT_FONTSET (fontset);

  return pango_core_text_fontset_key_get_language (pango_core_text_fontset_get_key (ctfontset));
}

static PangoFont *
pango_core_text_fontset_get_font (PangoFontset *fontset,
                                  guint         wc)
{
  PangoCoreTextFontset *ctfontset = PANGO_CORE_TEXT_FONTSET (fontset);
  PangoCoverageLevel best_level = PANGO_COVERAGE_NONE;
  PangoCoverageLevel level;
  PangoFont *font;
  PangoCoverage *coverage;
  int result = -1;
  unsigned int i;

  for (i = 0; i < ctfontset->fonts->len; i++)
    {
      PangoFont *font = pango_core_text_fontset_get_font_at (ctfontset, i);
      if (!font)
        continue;

      coverage = g_ptr_array_index (ctfontset->coverages, i);

      if (coverage == NULL)
        {
          font = g_ptr_array_index (ctfontset->fonts, i);

          coverage = pango_font_get_coverage (font, ctfontset->key->language);
          g_ptr_array_index (ctfontset->coverages, i) = coverage;
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

  font = g_ptr_array_index (ctfontset->fonts, result);
  return g_object_ref (font);
}

static void
pango_core_text_fontset_foreach (PangoFontset *fontset,
                                 PangoFontsetForeachFunc func,
                                 gpointer data)
{
  PangoCoreTextFontset *ctfontset = PANGO_CORE_TEXT_FONTSET (fontset);
  unsigned int i;

  for (i = 0; i < ctfontset->fonts->len; i++)
    {
      PangoFont *font = pango_core_text_fontset_get_font_at (ctfontset, i);
      if (!font)
        continue;

      if ((* func) (fontset, font, data))
        return;
    }
}

PangoCoreTextFace *
pango_core_text_font_map_find_face (PangoCoreTextFontMap       *map,
                                    const PangoCoreTextFontKey *key)
{
  CTFontDescriptorRef desc;
  gboolean synthetic_italic;
  gboolean synthetic_small_caps;
  char *family;
  char *family_name;
  char *style_name;
  PangoWeight weight;
  CTFontSymbolicTraits traits;
  PangoCoreTextFamily *font_family;
  PangoCoreTextFace *result = NULL;

  desc = pango_core_text_font_key_get_ctfontdescriptor (key);
  synthetic_italic = pango_core_text_font_key_get_synthetic_italic (key);
  synthetic_small_caps = pango_core_text_font_key_get_synthetic_small_caps (key);

  family_name = ct_font_descriptor_get_family_name (desc, FALSE);
  style_name = ct_font_descriptor_get_style_name (desc);
  weight = ct_font_descriptor_get_weight (desc);
  traits = ct_font_descriptor_get_traits (desc);

  family = g_utf8_casefold (family_name, -1);

  font_family = g_hash_table_lookup (map->families, family);

  if (font_family)
    {
      pango_font_family_list_faces ((PangoFontFamily *)font_family, NULL, NULL);

      for (int i = 0; i < font_family->n_faces; i++)
        {
          PangoCoreTextFace *face = (PangoCoreTextFace *)font_family->faces[i];

          if (face->weight == weight &&
              face->traits == traits &&
              face->synthetic_italic == synthetic_italic &&
              face->synthetic_small_caps == synthetic_small_caps &&
              strcmp (face->style_name, style_name) == 0)
            {
              result = face;
              break;
            }
        }
    }

  g_free (family);
  g_free (family_name);
  g_free (style_name);

  return result;
}
