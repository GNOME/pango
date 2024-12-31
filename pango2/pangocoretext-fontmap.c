/* Pango2
 *
 * Copyright (C) 2021 Red Hat, Inc.
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

#include "pangocoretext-fontmap.h"
#include "pango-hbfamily-private.h"
#include "pango-fontmap-private.h"
#include "pango-hbface-private.h"
#include "pango-hbfont-private.h"
#include "pango-context.h"
#include "pango-impl-utils.h"
#include "pango-trace-private.h"

#include <Carbon/Carbon.h>
#include <hb-ot.h>
#include <hb-coretext.h>


/**
 * Pango2CoreTextFontMap:
 *
 * `Pango2CoreTextFontMap` is a subclass of `Pango2FontMap` that uses
 * CoreText to populate the fontmap with the available fonts.
 */

/**
 * PANGO2_HAS_CORE_TEXT_FONTMAP:
 *
 * Defined to 1 at compile time if Pango was built with CoreText support.
 */

struct _Pango2CoreTextFontMap
{
  Pango2FontMap parent_instance;
};

struct _Pango2CoreTextFontMapClass
{
  Pango2FontMapClass parent_class;
};

/* {{{ CoreText utilities */

#define ct_weight_min -0.7f
#define ct_weight_max  0.8f

/* This map is based on empirical data from analyzing a large collection of
 * fonts and comparing the opentype value with the value that OSX returns.
 * see: https://bugzilla.gnome.org/show_bug.cgi?id=766148
 * FIXME: This need recalibrating, values outside these bounds do occur!
 */

static struct {
  float ct_weight;
  Pango2Weight pango2_weight;
} ct_weight_map[] = {
  { ct_weight_min,  PANGO2_WEIGHT_THIN },
  { -0.5,           PANGO2_WEIGHT_ULTRALIGHT },
  { -0.23,          PANGO2_WEIGHT_LIGHT },
  { -0.115,         PANGO2_WEIGHT_SEMILIGHT },
  {  0.00,          PANGO2_WEIGHT_NORMAL },
  {  0.2,           PANGO2_WEIGHT_MEDIUM },
  {  0.3,           PANGO2_WEIGHT_SEMIBOLD },
  {  0.4,           PANGO2_WEIGHT_BOLD },
  {  0.6,           PANGO2_WEIGHT_ULTRABOLD },
  {  ct_weight_max, PANGO2_WEIGHT_HEAVY }
};

static int
lerp (float x, float x1, float x2, int y1, int y2)
{
  float dx = x2 - x1;
  int dy = y2 - y1;
  return y1 + (dy * (x - x1) + dx / 2) / dx;
}

static Pango2Weight
ct_font_descriptor_get_weight (CTFontDescriptorRef desc)
{
  CFDictionaryRef dict;
  CFNumberRef cf_number;
  CGFloat value;
  Pango2Weight weight = PANGO2_WEIGHT_NORMAL;
  guint i;

  dict = CTFontDescriptorCopyAttribute (desc, kCTFontTraitsAttribute);
  cf_number = (CFNumberRef)CFDictionaryGetValue (dict, kCTFontWeightTrait);

  if (cf_number != NULL && CFNumberGetValue (cf_number, kCFNumberCGFloatType, &value))
    {
    if (!(value >= ct_weight_min && value <= ct_weight_max))
      {
        i = value > ct_weight_max ? G_N_ELEMENTS (ct_weight_map) - 1 : 0;
        weight = ct_weight_map[i].ct_weight;
      }
    else
      {
        for (i = 1; value > ct_weight_map[i].ct_weight; ++i)
          ;

        weight = lerp (value, ct_weight_map[i-1].ct_weight, ct_weight_map[i].ct_weight,
                       ct_weight_map[i-1].pango2_weight, ct_weight_map[i].pango2_weight);

      }
    }
  else
    weight = PANGO2_WEIGHT_NORMAL;

  CFRelease (dict);

  return weight;
}

static char *
gchar_from_cf_string (CFStringRef str)
{
  CFIndex len;
  char *buffer;

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
  if (!CFNumberGetValue (cf_number, kCFNumberSInt64Type, &traits))
    traits = 0;
  CFRelease (dict);

  return (CTFontSymbolicTraits)traits;
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

static gboolean
pango2_core_text_style_name_is_oblique (const char *style_name)
{
  return style_name && strstr (style_name, "Oblique");
}

static Pango2FontDescription *
font_description_from_ct_font_descriptor (CTFontDescriptorRef desc)
{
  SInt64 font_traits;
  char *family_name;
  char *style_name;
  Pango2FontDescription *font_desc;

  font_desc = pango2_font_description_new ();

  family_name = ct_font_descriptor_get_family_name (desc, FALSE);
  pango2_font_description_set_family (font_desc, family_name);
  g_free (family_name);

  pango2_font_description_set_weight (font_desc, ct_font_descriptor_get_weight (desc));

  font_traits = ct_font_descriptor_get_traits (desc);
  style_name = ct_font_descriptor_get_style_name (desc);

  if ((font_traits & kCTFontItalicTrait) == kCTFontItalicTrait)
    pango2_font_description_set_style (font_desc, PANGO2_STYLE_ITALIC);
  else if (pango2_core_text_style_name_is_oblique (style_name))
    pango2_font_description_set_style (font_desc, PANGO2_STYLE_OBLIQUE);
  else
    pango2_font_description_set_style (font_desc, PANGO2_STYLE_NORMAL);

  if ((font_traits & kCTFontCondensedTrait) == kCTFontCondensedTrait)
    pango2_font_description_set_stretch (font_desc, PANGO2_STRETCH_CONDENSED);

  if (ct_font_descriptor_is_small_caps (desc))
    pango2_font_description_set_variant (font_desc, PANGO2_VARIANT_SMALL_CAPS);
  else
    pango2_font_description_set_variant (font_desc, PANGO2_VARIANT_NORMAL);

  g_free (style_name);

  return font_desc;
}

static Pango2HbFace *
face_from_ct_font_descriptor (CTFontDescriptorRef desc)
{
  Pango2HbFace *face;
  CTFontRef ctfont;
  CGFontRef cgfont;
  hb_face_t *hb_face;
  char *name;
  Pango2FontDescription *description;

  ctfont = CTFontCreateWithFontDescriptor (desc, 0.0, NULL);
  cgfont = CTFontCopyGraphicsFont (ctfont, NULL);

  hb_face = hb_coretext_face_create (cgfont);

  CFRelease (cgfont);
  CFRelease (ctfont);

  name = ct_font_descriptor_get_style_name (desc);
  description = font_description_from_ct_font_descriptor (desc);
  pango2_font_description_unset_fields (description, PANGO2_FONT_MASK_VARIANT |
                                                     PANGO2_FONT_MASK_SIZE |
                                                     PANGO2_FONT_MASK_GRAVITY);

  hb_face_make_immutable (hb_face);

  face = pango2_hb_face_new_from_hb_face (hb_face, -1, name, description);
  // FIXME: what about languages? see CTFontCopySupportedLanguages

  pango2_font_description_free (description);
  g_free (name);
  hb_face_destroy (hb_face);

  return face;
}

/* }}} */
/* {{{ Pango2FontMap implementation */

static void
pango2_core_text_font_map_populate (Pango2FontMap *map)
{
  CTFontCollectionRef collection;
  CFArrayRef ctfaces;
  CFIndex count;

  /* Add all system fonts */
  collection = CTFontCollectionCreateFromAvailableFonts (0);
  ctfaces = CTFontCollectionCreateMatchingFontDescriptors (collection);
  count = CFArrayGetCount (ctfaces);

  for (int i = 0; i < count; i++)
    {
      CTFontDescriptorRef desc = CFArrayGetValueAtIndex (ctfaces, i);
      Pango2HbFace *face = face_from_ct_font_descriptor (desc);
      pango2_font_map_add_face (map, PANGO2_FONT_FACE (face));
    }

  /* Add generic aliases */
  struct {
    const char *alias_name;
    const char *family_name;
  } aliases[] = {
    { "Monospace",  "Menlo" },
    { "Sans-serif", "Helvetica" },
    { "Sans", "Helvetica" },
    { "Serif",      "Times" },
    { "Cursive",    "Apple Chancery" },
    { "Fantasy",    "Papyrus", },
    { "System-ui",  ".AppleSystemUIFont" },
    { "Emoji",      "Apple Color Emoji" }
  };

  for (int i = 0; i < G_N_ELEMENTS (aliases); i++)
    {
      Pango2FontFamily *family = pango2_font_map_get_family (map, aliases[i].family_name);

      if (family)
        {
          Pango2GenericFamily *alias_family;

          alias_family = pango2_generic_family_new (aliases[i].alias_name);
          pango2_generic_family_add_family (alias_family, family);
          pango2_font_map_add_family (map, PANGO2_FONT_FAMILY (alias_family));
        }
    }
}

/* }}} */
/* {{{ Pango2CoreTextFontMap implementation */

G_DEFINE_FINAL_TYPE (Pango2CoreTextFontMap, pango2_core_text_font_map, PANGO2_TYPE_FONT_MAP)

static void
pango2_core_text_font_map_init (Pango2CoreTextFontMap *self)
{
  pango2_font_map_repopulate (PANGO2_FONT_MAP (self), TRUE);
}

static void
pango2_core_text_font_map_finalize (GObject *object)
{
  G_OBJECT_CLASS (pango2_core_text_font_map_parent_class)->finalize (object);
}

static void
pango2_core_text_font_map_class_init (Pango2CoreTextFontMapClass *class)
{
  GObjectClass *object_class = G_OBJECT_CLASS (class);
  Pango2FontMapClass *font_map_class = PANGO2_FONT_MAP_CLASS (class);

  object_class->finalize = pango2_core_text_font_map_finalize;

  font_map_class->populate = pango2_core_text_font_map_populate;
}

/* }}} */
 /* {{{ Public API */

/**
 * pango2_core_text_font_map_new:
 *
 * Creates a new `Pango2CoreTextFontMap` object.
 *
 * Returns: a new `Pango2CoreTextFontMap`
 */
Pango2CoreTextFontMap *
pango2_core_text_font_map_new (void)
{
  return g_object_new (PANGO2_TYPE_CORE_TEXT_FONT_MAP, NULL);
}

/* }}} */

/* vim:set foldmethod=marker expandtab: */
