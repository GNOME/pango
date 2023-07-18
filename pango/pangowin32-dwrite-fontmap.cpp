/* Pango
 * pangowin32-dwrite-fontmap.cpp: Win32 font handling with DirectWrite
 *
 * Copyright (C) 2022 Luca Bacci
 * Copyright (C) 2022 Chun-wei Fan
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

#include <initguid.h>

#ifdef HAVE_DWRITE_3_H
/* we need dwrite_3.h for IDWriteFactory[3|5] */
#include <dwrite_3.h>

# ifdef _MSC_VER
#  define UUID_OF_IDWriteFactory3 __uuidof (IDWriteFactory3)
#  define UUID_OF_IDWriteFactory5 __uuidof (IDWriteFactory5)
# else
#  define UUID_OF_IDWriteFactory3 IID_IDWriteFactory3
#  define UUID_OF_IDWriteFactory5 IID_IDWriteFactory5
# endif
#else
/* stub, for simplicity reasons, if we don't have IDWriteFactory[3|5] */
# define IDWriteFactory3 IUnknown
# define IDWriteFactory5 IUnknown
#endif
#include <dwrite_1.h>

#ifdef STRICT
#undef STRICT
#endif
#include "pangowin32-private.h"

#ifdef USE_HB_DWRITE
#include <hb-directwrite.h>
#endif

#ifdef _MSC_VER
# define UUID_OF_IDWriteFactory __uuidof (IDWriteFactory)
# define UUID_OF_IDWriteFont1 __uuidof (IDWriteFont1)
#else
# define UUID_OF_IDWriteFactory IID_IDWriteFactory
# define UUID_OF_IDWriteFont1 IID_IDWriteFont1
#endif

struct _PangoWin32DWriteItems
{
  IDWriteFactory5   *dwrite_factory5;
  IDWriteFactory3   *dwrite_factory3;
  IDWriteFactory    *dwrite_factory;
  IDWriteGdiInterop *gdi_interop;
  guint              have_idwritefactory5 : 1;
  guint              have_idwritefactory3 : 1;
};

PangoWin32DWriteItems *
pango_win32_init_direct_write (void)
{
  PangoWin32DWriteItems *dwrite_items = g_new0 (PangoWin32DWriteItems, 1);
  HRESULT hr;
  IDWriteFactory5 *factory5 = NULL;
  IDWriteFactory3 *factory3 = NULL;
  IDWriteFactory *factory = NULL;
  gboolean failed = FALSE;
  gboolean have_idwritefactory3 = FALSE;
  gboolean have_idwritefactory5 = FALSE;

#ifdef HAVE_DWRITE_3_H
  /* Try to create a IDWriteFactory3 first, which is available on Windows 10+ */
  hr = DWriteCreateFactory (DWRITE_FACTORY_TYPE_SHARED,
                            UUID_OF_IDWriteFactory3,
                            reinterpret_cast<IUnknown**> (&factory3));
  if (SUCCEEDED(hr))
    {
      /*
       * Try to acquire a IDWriteFactory5 object from the IDWriteFactory3 object,
       * which is only available on or after Windows 10 Creators' Update
       */
      have_idwritefactory3 = TRUE;
      hr = factory3->QueryInterface (UUID_OF_IDWriteFactory5,
                                     reinterpret_cast<void**> (&factory5));
      if (SUCCEEDED (hr))
        have_idwritefactory5 = TRUE;

      hr = factory3->QueryInterface (UUID_OF_IDWriteFactory,
                                     reinterpret_cast<void**> (&factory));
    }
  else
#endif
    hr = DWriteCreateFactory (DWRITE_FACTORY_TYPE_SHARED,
                              UUID_OF_IDWriteFactory,
                              reinterpret_cast<IUnknown**> (&factory));

  if (SUCCEEDED (hr) && factory != NULL)
    {
      IDWriteGdiInterop *gdi_interop = NULL;

      hr = factory->GetGdiInterop (&gdi_interop);
      if (FAILED (hr) || gdi_interop == NULL)
        {
          g_error ("DWriteFactory::GetGdiInterop failed with error code %x", (unsigned)hr);
          factory->Release ();

          if (have_idwritefactory5)
            factory5->Release ();

          if (have_idwritefactory3)
            factory3->Release ();

          failed = TRUE;
        }

      if (have_idwritefactory3)
        dwrite_items->dwrite_factory3 = factory3;
      if (have_idwritefactory5)
        dwrite_items->dwrite_factory5 = factory5;

      dwrite_items->have_idwritefactory3 = have_idwritefactory3;
      dwrite_items->have_idwritefactory5 = have_idwritefactory5;
      dwrite_items->gdi_interop = gdi_interop;
      dwrite_items->dwrite_factory = factory;
    }
  else
    {
      g_error ("DWriteCreateFactory failed with error code %x", (unsigned)hr);
      failed = TRUE;
    }

  if (failed)
    g_free (dwrite_items);

  return failed ? NULL : dwrite_items;
}

void
pango_win32_dwrite_items_destroy (PangoWin32DWriteItems *dwrite_items)
{
  dwrite_items->gdi_interop->Release ();
  dwrite_items->dwrite_factory->Release ();

  if (dwrite_items->have_idwritefactory5)
    dwrite_items->dwrite_factory5->Release ();

  if (dwrite_items->have_idwritefactory3)
    dwrite_items->dwrite_factory3->Release ();

  g_free (dwrite_items);
}

static IDWriteFontFace *
_pango_win32_get_dwrite_font_face_from_dwrite_font (IDWriteFont *font)
{
  IDWriteFontFace *face = NULL;
  HRESULT hr;

  g_return_val_if_fail (font != NULL, NULL);

  hr = font->CreateFontFace (&face);
  if (SUCCEEDED (hr) && face != NULL)
    return face;

  g_warning ("IDWriteFont::CreateFontFace failed with error code %x\n", (unsigned)hr);
  return NULL;
}

static IDWriteFont *
get_dwrite_font_from_pango_win32_font (PangoWin32Font *font)
{
  PangoWin32DWriteItems *dwrite_items = pango_win32_get_direct_write_items ();
  IDWriteFont *dwrite_font = NULL;
  PangoWin32FontMap *fontmap = PANGO_WIN32_FONT_MAP (font->fontmap);

  dwrite_font = (IDWriteFont *) g_hash_table_lookup (fontmap->dwrite_fonts,
                                                    &font->logfontw);

  /* create the IDWriteFont from the logfont underlying the PangoWin32Font if needed */
  if (dwrite_font == NULL)
    {
      if (SUCCEEDED (dwrite_items->gdi_interop->CreateFontFromLOGFONT (&font->logfontw,
                                                                       &dwrite_font)) &&
          dwrite_font != NULL)
        {
          g_hash_table_insert (fontmap->dwrite_fonts,
                              &font->logfontw,
                               dwrite_font);
        }
    }

  return dwrite_font;
}

void *
pango_win32_font_get_dwrite_font_face (PangoWin32Font *font)
{
  PangoWin32Font *win32font = PANGO_WIN32_FONT (font);
  PangoWin32FontMap *win32fontmap = PANGO_WIN32_FONT_MAP (win32font->fontmap);
  IDWriteFont *dwrite_font = NULL;
  IDWriteFontFace *face = NULL;

  dwrite_font = get_dwrite_font_from_pango_win32_font (font);

  if (dwrite_font != NULL)
    return (void *)_pango_win32_get_dwrite_font_face_from_dwrite_font (dwrite_font);

  return NULL;
}

void
pango_win32_dwrite_font_map_populate (PangoWin32FontMap *map)
{
  IDWriteFontCollection *collection = NULL;
  UINT32 count;
  HRESULT hr;
  gboolean failed = FALSE;
  PangoWin32DWriteItems *dwrite_items = pango_win32_get_direct_write_items ();

  hr = dwrite_items->dwrite_factory->GetSystemFontCollection (&collection, FALSE);
  if (FAILED (hr) || collection == NULL)
    {
      g_error ("IDWriteFactory::GetSystemFontCollection failed with error code %x\n", (unsigned)hr);
      return;
    }

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
          DWRITE_FONT_FACE_TYPE font_face_type;

          hr = family->GetFont (j, &font);
          if (FAILED (hr) || font == NULL)
            {
              g_warning ("IDWriteFontFamily::GetFont failed with error code %x\n", (unsigned)hr);
              break;
            }

          face = _pango_win32_get_dwrite_font_face_from_dwrite_font (font);
          if (face == NULL)
            {
              font->Release ();
              continue;
            }

          font_face_type = face->GetType ();

          /* don't include Type-1 fonts */
          if (font_face_type != DWRITE_FONT_FACE_TYPE_TYPE1)
            {
              LOGFONTW lfw;
              BOOL is_sys_font;

              hr = dwrite_items->gdi_interop->ConvertFontToLOGFONT (font, &lfw, &is_sys_font);

              if (SUCCEEDED (hr))
                pango_win32_insert_font (map, &lfw, font, FALSE);
              else
                g_warning ("GDIInterop::ConvertFontToLOGFONT failed with error code %x\n",
                           (unsigned)hr);
            }

         face->Release ();
        }

      family->Release ();
    }

  collection->Release ();
  collection = NULL;
}

gpointer
pango_win32_logfontw_get_dwrite_font (LOGFONTW *logfontw)
{
  PangoWin32DWriteItems *dwrite_items = pango_win32_get_direct_write_items ();
  IDWriteFont *font = NULL;
  HRESULT hr;

  hr = dwrite_items->gdi_interop->CreateFontFromLOGFONT (logfontw, &font);

  if (FAILED (hr) || font == NULL)
    g_warning ("IDWriteFactory::GdiInterop::CreateFontFromLOGFONT failed with error %x\n",
               (unsigned)hr);

  return font;
}

gboolean
pango_win32_dwrite_font_is_monospace (gpointer  dwrite_font,
                                      gboolean *is_monospace)
{
  IDWriteFont *font = static_cast<IDWriteFont *>(dwrite_font);
  IDWriteFont1 *font1 = NULL;
  gboolean result = FALSE;

  if (SUCCEEDED (font->QueryInterface(UUID_OF_IDWriteFont1,
                                      reinterpret_cast<void**>(&font1))))
    {
      *is_monospace = font1->IsMonospacedFont ();
      font1->Release ();
      result = TRUE;
    }
  else
    *is_monospace = FALSE;

  return result;
}

static PangoStretch
util_to_pango_stretch (DWRITE_FONT_STRETCH stretch)
{
  PangoStretch pango_stretch = PANGO_STRETCH_NORMAL;

  switch (stretch)
    {
      case DWRITE_FONT_STRETCH_ULTRA_CONDENSED:
        pango_stretch = PANGO_STRETCH_ULTRA_CONDENSED;
        break;
      case DWRITE_FONT_STRETCH_EXTRA_CONDENSED:
        pango_stretch = PANGO_STRETCH_EXTRA_CONDENSED;
        break;
     case DWRITE_FONT_STRETCH_CONDENSED:
        pango_stretch = PANGO_STRETCH_CONDENSED;
        break;
      case DWRITE_FONT_STRETCH_SEMI_CONDENSED:
        pango_stretch = PANGO_STRETCH_SEMI_CONDENSED;
        break;
      case DWRITE_FONT_STRETCH_NORMAL:
        /* also DWRITE_FONT_STRETCH_MEDIUM */
        pango_stretch = PANGO_STRETCH_NORMAL;
        break;
      case DWRITE_FONT_STRETCH_SEMI_EXPANDED:
        pango_stretch = PANGO_STRETCH_SEMI_EXPANDED;
        break;
      case DWRITE_FONT_STRETCH_EXPANDED:
        pango_stretch = PANGO_STRETCH_EXPANDED;
        break;
      case DWRITE_FONT_STRETCH_EXTRA_EXPANDED:
        pango_stretch = PANGO_STRETCH_EXTRA_EXPANDED;
        break;
      case DWRITE_FONT_STRETCH_ULTRA_EXPANDED:
        pango_stretch = PANGO_STRETCH_ULTRA_EXPANDED;
        break;
      case DWRITE_FONT_STRETCH_UNDEFINED:
      default:
        pango_stretch = PANGO_STRETCH_NORMAL;
    }

  return pango_stretch;
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
  /* DirectWrite weight values range from 1 to 999, Pango weight values
   * range from 100 to 1000. */

  return (PangoWeight) util_map_weight (weight);
}

static PangoVariant
util_to_pango_variant (IDWriteFont *font)
{
  PangoVariant variant = PANGO_VARIANT_NORMAL;

  return variant;
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
  pango_font_description_set_variant (description, util_to_pango_variant (font));
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

PangoFontDescription *
pango_win32_font_description_from_logfontw_dwrite (const LOGFONTW *logfontw)
{
  PangoFontDescription *desc = NULL;
  IDWriteFont *font = NULL;
  HRESULT hr;
  gchar *family;
  PangoStyle style;
  PangoVariant variant;
  PangoWeight weight;
  PangoStretch stretch;
  PangoWin32DWriteItems *dwrite_items;

  dwrite_items = pango_win32_get_direct_write_items ();
  if (dwrite_items == NULL)
    return NULL;

  hr = dwrite_items->gdi_interop->CreateFontFromLOGFONT (logfontw, &font);

  if (SUCCEEDED (hr) && font != NULL)
    {
      IDWriteFontFamily *family = NULL;

      hr = font->GetFontFamily (&family);

      if (SUCCEEDED (hr) && family != NULL)
        {
          char *family_name = util_dwrite_get_font_family_name (family);

          if (family_name != NULL)
            desc = util_get_pango_font_description (font, family_name);

          family->Release ();
        }

      font->Release ();
    }

  return desc;
}

/* macros to help parse the 'gasp' font table, referring to FreeType2 */
#define DWRITE_UCHAR_USHORT( p, i, s ) ( (unsigned short)( ((const unsigned char *)(p))[(i)] ) << (s) )

#define DWRITE_PEEK_USHORT( p ) (unsigned short)( DWRITE_UCHAR_USHORT( p, 0, 8 ) | DWRITE_UCHAR_USHORT( p, 1, 0 ) )

#define DWRITE_NEXT_USHORT( buffer ) ( (unsigned short)( buffer += 2, DWRITE_PEEK_USHORT( buffer - 2 ) ) )

/* values to indicate that grid fit (hinting) is supported by the font */
#define GASP_GRIDFIT 0x0001
#define GASP_SYMMETRIC_GRIDFIT 0x0004

gboolean
pango_win32_dwrite_font_check_is_hinted (PangoWin32Font *font)
{
  IDWriteFontFace *dwrite_font_face = NULL;
  gboolean result = FALSE;

  dwrite_font_face = (IDWriteFontFace *)pango_win32_font_get_dwrite_font_face (font);

  if (dwrite_font_face != NULL)
    {
      UINT32 gasp_tag = DWRITE_MAKE_OPENTYPE_TAG ('g', 'a', 's', 'p');
      UINT32 table_size;
      const unsigned short *table_data;
      void *table_ctx;
      gboolean exists;

      /* The 'gasp' table may not exist for the font, so no 'gasp' == no hinting */
      if (SUCCEEDED (dwrite_font_face->TryGetFontTable (gasp_tag,
                                                        (const void **)(&table_data),
                                                       &table_size,
                                                       &table_ctx,
                                                       &exists)))
        {
          if (exists && table_size > 4)
            {
              guint16 version = DWRITE_NEXT_USHORT (table_data);

              if (version == 0 || version == 1)
                {
                  guint16 num_ranges = DWRITE_NEXT_USHORT (table_data);
                  UINT32 max_ranges = (table_size - 4) / (sizeof (guint16) * 2);
                  guint16 i = 0;

                  if (num_ranges > max_ranges)
                    num_ranges = max_ranges;

                  for (i = 0; i < num_ranges; i++)
                    {
                      G_GNUC_UNUSED
                      guint16 ppem = DWRITE_NEXT_USHORT (table_data);
                      guint16 behavior = DWRITE_NEXT_USHORT (table_data);

                      if (behavior & (GASP_GRIDFIT | GASP_SYMMETRIC_GRIDFIT))
                        {
                          result = TRUE;
                          break;
                        }
                    }
                }
            }

          dwrite_font_face->ReleaseFontTable (table_ctx);
        }

      dwrite_font_face->Release ();
    }

  return result;
}

#ifdef USE_HB_DWRITE
/* Sadly, hb-directwrite.h stuff has to be done via C++... */
hb_face_t *
pango_win32_font_create_hb_face_dwrite (PangoWin32Font *font)
{
  hb_face_t *hb_face = NULL;
  IDWriteFontFace *face = NULL;

  g_return_val_if_fail (PANGO_WIN32_IS_FONT (font), NULL);

  face = static_cast<IDWriteFontFace *>(pango_win32_font_get_dwrite_font_face (font));

  if (face != NULL)
    hb_face = hb_directwrite_face_create (face);

  if (hb_face != NULL)
    {
      static hb_user_data_key_t key;

      hb_face_set_user_data (hb_face,
                            &key,
                             face,
                             (hb_destroy_func_t) pango_win32_dwrite_font_face_release,
                             TRUE);
    }

  return hb_face;
}
#endif

void
pango_win32_dwrite_font_release (gpointer dwrite_font)
{
  IDWriteFont *font = static_cast<IDWriteFont *>(dwrite_font);

  if (font != NULL)
    font->Release ();
}

void
pango_win32_dwrite_font_face_release (gpointer dwrite_font_face)
{
  IDWriteFontFace *face = static_cast<IDWriteFontFace *>(dwrite_font_face);

  if (face != NULL)
    face->Release ();
}
