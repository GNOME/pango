/* Pango
 * pangodwrite-fontmap.cpp: Fontmap using DirectWrite
 *
 * Copyright (C) 2022 the GTK team
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

#include <windows.h>
#include <dwrite.h>

#include <hb-ot.h>
#include <hb-directwrite.h>

#include "pangodwrite-fontmap.h"
#include "pango-hbfamily-private.h"
#include "pango-fontmap-private.h"
#include "pango-hbface-private.h"
#include "pango-hbfont-private.h"
#include "pango-context.h"
#include "pango-impl-utils.h"
#include "pango-trace-private.h"


/**
 * PangoDirectWriteFontMap:
 *
 * `PangoDirectWriteFontMap` is a subclass of `PangoFontMap` that
 * uses DirectWrite to populate the fontmap with the available fonts.
 */


struct _PangoDirectWriteFontMap
{
  PangoFontMap parent_instance;
};

struct _PangoDirectWriteFontMapClass
{
  PangoFontMapClass parent_class;
};

#ifdef _MSC_VER
# define UUID_OF_IDWriteFactory __uuidof (IDWriteFactory)
#else
# define UUID_OF_IDWriteFactory IID_IDWriteFactory
#endif

/* {{{ DirectWrite utilities */

static PangoStretch
util_to_pango_stretch (DWRITE_FONT_STRETCH stretch)
{
  int value = (int) stretch;

  if G_UNLIKELY (stretch <= DWRITE_FONT_STRETCH_UNDEFINED ||
                 stretch > DWRITE_FONT_STRETCH_ULTRA_EXPANDED)
    return PANGO_STRETCH_NORMAL;

  return (PangoStretch) --value;
}

static PangoStyle
util_to_pango_style (DWRITE_FONT_STYLE style)
{
  switch (style)
    {
    case DWRITE_FONT_STYLE_NORMAL:
      return PANGO_STYLE_NORMAL;
    case DWRITE_FONT_STYLE_OBLIQUE:
      return PANGO_STYLE_OBLIQUE;
    case DWRITE_FONT_STYLE_ITALIC:
      return PANGO_STYLE_ITALIC;
    default:
      g_assert_not_reached ();
      return PANGO_STYLE_NORMAL;
    }
}

static int
util_map_weight (int weight)
{
  if G_UNLIKELY (weight < 100)
    weight = 100;

  if G_UNLIKELY (weight > 1000)
    weight = 1000;

  return weight;
}

static PangoWeight
util_to_pango_weight (DWRITE_FONT_WEIGHT weight)
{
  /* DirectWrite weight values range from 1 to 999, Pango values
   * range from 100 to 1000. */

  return (PangoWeight) util_map_weight (weight);
}

static PangoFontDescription*
util_get_pango_font_description (IDWriteFont *font,
                                 const char *family_name)
{
  DWRITE_FONT_STRETCH stretch = font->GetStretch ();
  DWRITE_FONT_STYLE style = font->GetStyle ();
  DWRITE_FONT_WEIGHT weight = font->GetWeight ();
  PangoFontDescription *description;

  description = pango_font_description_new ();
  pango_font_description_set_family (description, family_name);
  pango_font_description_set_stretch (description, util_to_pango_stretch (stretch));
  pango_font_description_set_style (description, util_to_pango_style (style));
  pango_font_description_set_weight (description, util_to_pango_weight (weight));

  return description;
}

static char*
util_free_to_string (IDWriteLocalizedStrings *strings)
{
  char *string = NULL;
  HRESULT hr;

  if (strings->GetCount() > 0)
    {
      UINT32 index = 0;
      BOOL exists = FALSE;
      UINT32 length = 0;

      hr = strings->FindLocaleName (L"en-us", &index, &exists);
      if (FAILED (hr) || !exists || index == UINT32_MAX)
        index = 0;

      hr = strings->GetStringLength (index, &length);
      if (SUCCEEDED (hr) && length > 0)
        {
          gunichar2 *string_utf16 = g_new (gunichar2, length + 1);

          hr = strings->GetString (index, (wchar_t*) string_utf16, length + 1);
          if (SUCCEEDED (hr))
            string = g_utf16_to_utf8 (string_utf16, -1, NULL, NULL, NULL);

          g_free (string_utf16);
        }
    }

  strings->Release ();

  return string;
}

static char*
util_dwrite_get_font_variant_name (IDWriteFont *font)
{
  IDWriteLocalizedStrings *strings = NULL;
  HRESULT hr;

  hr = font->GetFaceNames (&strings);
  if (FAILED (hr) || strings == NULL)
    {
      g_warning ("IDWriteFont::GetFaceNames failed with error code %x", (unsigned) hr);
      return NULL;
    }

  return util_free_to_string (strings);
}

static char*
util_dwrite_get_font_family_name (IDWriteFontFamily *family)
{
  IDWriteLocalizedStrings *strings = NULL;
  HRESULT hr;

  hr = family->GetFamilyNames (&strings);
  if (FAILED (hr) || strings == NULL)
    {
      g_warning ("IDWriteFontFamily::GetFamilyNames failed with error code %x", (unsigned) hr);
      return NULL;
    }

  return util_free_to_string (strings);
}

static PangoHbFace*
util_create_pango_hb_face (IDWriteFontFamily *family,
                           IDWriteFont *font,
                           IDWriteFontFace *face)
{
  char *family_name = util_dwrite_get_font_family_name (family);
  char *variant_name = util_dwrite_get_font_variant_name (font);
  PangoHbFace *pango_face = NULL;

  if (family_name && variant_name)
    {
      PangoFontDescription *description = util_get_pango_font_description (font, family_name);
      hb_face_t *hb_face = hb_directwrite_face_create (face);
      char *name = g_strconcat (family_name, " ", variant_name, NULL);

      pango_face = pango_hb_face_new_from_hb_face (hb_face, -1, name, description);

      g_free (name);
      hb_face_destroy (hb_face);
      pango_font_description_free (description);
    }

  g_free (family_name);
  g_free (variant_name);

  return pango_face;
}

/* }}} */
/* {{{ PangoFontMap implementation */

static void
pango_direct_write_font_map_populate (PangoFontMap *map)
{
  IDWriteFactory *factory = NULL;
  IDWriteFontCollection *collection = NULL;
  UINT32 count;
  HRESULT hr;

  hr = DWriteCreateFactory (DWRITE_FACTORY_TYPE_SHARED,
                            UUID_OF_IDWriteFactory,
                            reinterpret_cast<IUnknown**> (&factory));
  if (FAILED (hr) || !factory)
    g_error ("DWriteCreateFactory failed with error code %x", (unsigned)hr);

  hr = factory->GetSystemFontCollection (&collection, FALSE);
  if (FAILED (hr) || collection == NULL)
    g_error ("IDWriteFactory::GetSystemFontCollection failed with error code %x\n", (unsigned)hr);

  count = collection->GetFontFamilyCount ();

  for (UINT32 i = 0; i < count; i++)
    {
      IDWriteFontFamily *family = NULL;
      UINT32 font_count;

      hr = collection->GetFontFamily (i, &family);
      if G_UNLIKELY (FAILED (hr) || family == NULL)
        {
          g_warning ("IDWriteFontCollection::GetFontFamily failed with error code %x\n", (unsigned)hr);
          continue;
        }

      font_count = family->GetFontCount ();

      for (UINT32 j = 0; j < font_count; j++)
        {
          IDWriteFont *font = NULL;
          IDWriteFontFace *face = NULL;
          PangoHbFace *pango_face = NULL;

          hr = family->GetFont (j, &font);
          if (FAILED (hr) || font == NULL)
            {
              g_warning ("IDWriteFontFamily::GetFont failed with error code %x\n", (unsigned)hr);
              break;
            }

          hr = font->CreateFontFace (&face);
          if (FAILED (hr) || face == NULL)
            {
              g_warning ("IDWriteFont::CreateFontFace failed with error code %x\n", (unsigned)hr);
              font->Release ();
              continue;
            }

          pango_face = util_create_pango_hb_face (family, font, face);
          if (pango_face)
            pango_font_map_add_face (map, PANGO_FONT_FACE (pango_face));

          face->Release ();
          font->Release ();
        }

      family->Release ();
    }

  collection->Release ();
  collection = NULL;

  factory->Release ();
  factory = NULL;

  /* Add generic aliases */
  struct {
    const char *alias_name;
    const char *family_name;
  } aliases[] = {
    { "monospace",  "Consolas" },
    { "sans-serif", "Arial" },
    { "serif",      "Times New Roman" },
    { "system-ui",  "Segoe UI" },
    { "emoji",      "Segoe UI Emoji" }
  };

#if 0
  if (IsWindows11OrLater ())
    aliases[0].family_name = "Cascadia Mono";
#endif

  for (gsize i = 0; i < G_N_ELEMENTS (aliases); i++)
    {
      PangoFontFamily *family = pango_font_map_get_family (map, aliases[i].family_name);

      if (family)
        {
          PangoGenericFamily *alias_family;

          alias_family = pango_generic_family_new (aliases[i].alias_name);
          pango_generic_family_add_family (alias_family, family);
          pango_font_map_add_family (map, PANGO_FONT_FAMILY (alias_family));
        }
    }
}

/* }}} */
/* {{{ PangoDirctWriteFontMap implementation */

G_DEFINE_TYPE (PangoDirectWriteFontMap, pango_direct_write_font_map, PANGO_TYPE_FONT_MAP)

static void
pango_direct_write_font_map_init (PangoDirectWriteFontMap *self)
{
  pango_font_map_repopulate (PANGO_FONT_MAP (self), TRUE);
}

static void
pango_direct_write_font_map_finalize (GObject *object)
{
  G_OBJECT_CLASS (pango_direct_write_font_map_parent_class)->finalize (object);
}

static void
pango_direct_write_font_map_class_init (PangoDirectWriteFontMapClass *class_)
{
  GObjectClass *object_class = G_OBJECT_CLASS (class_);
  PangoFontMapClass *font_map_class = PANGO_FONT_MAP_CLASS (class_);

  object_class->finalize = pango_direct_write_font_map_finalize;

  font_map_class->populate = pango_direct_write_font_map_populate;
}

/* }}} */
 /* {{{ Public API */

/**
 * pango_direct_write_font_map_new:
 *
 * Creates a new `PangoDirectWriteFontMap` object.
 *
 * Returns: a new `PangoDirectWriteFontMap`
 */
PangoDirectWriteFontMap *
pango_direct_write_font_map_new (void)
{
  return (PangoDirectWriteFontMap *) g_object_new (PANGO_TYPE_DIRECT_WRITE_FONT_MAP, NULL);
}

/* }}} */

/* vim:set foldmethod=marker expandtab: */
