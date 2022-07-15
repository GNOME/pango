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
#include <dwrite_1.h>

#ifdef STRICT
#undef STRICT
#endif
#include "pangowin32-private.h"

#ifdef _MSC_VER
# define UUID_OF_IDWriteFactory __uuidof (IDWriteFactory)
# define UUID_OF_IDWriteFont1 __uuidof (IDWriteFont1)
#else
# define UUID_OF_IDWriteFactory IID_IDWriteFactory
# define UUID_OF_IDWriteFont1 IID_IDWriteFont1
#endif

struct _PangoWin32DWriteItems
{
  IDWriteFactory    *dwrite_factory;
  IDWriteGdiInterop *gdi_interop;
};

PangoWin32DWriteItems *
pango_win32_init_direct_write (void)
{
  PangoWin32DWriteItems *dwrite_items = g_new0 (PangoWin32DWriteItems, 1);
  HRESULT hr;
  gboolean failed = FALSE;

  hr = DWriteCreateFactory (DWRITE_FACTORY_TYPE_SHARED,
                            UUID_OF_IDWriteFactory,
                            reinterpret_cast<IUnknown**> (&dwrite_items->dwrite_factory));

  if (SUCCEEDED (hr) && dwrite_items->dwrite_factory != NULL)
    {
      hr = dwrite_items->dwrite_factory->GetGdiInterop (&dwrite_items->gdi_interop);
      if (FAILED (hr) || dwrite_items->gdi_interop == NULL)
        {
          g_error ("DWriteCreateFactory::GetGdiInterop failed with error code %x", (unsigned)hr);
          dwrite_items->dwrite_factory->Release ();
          failed = TRUE;
        }
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

  g_free (dwrite_items);
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

          hr = font->CreateFontFace (&face);
          if (FAILED (hr) || face == NULL)
            {
              g_warning ("IDWriteFont::CreateFontFace failed with error code %x\n", (unsigned)hr);
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

void
pango_win32_dwrite_font_release (gpointer dwrite_font)
{
  IDWriteFont *font = static_cast<IDWriteFont *>(dwrite_font);

  if (font != NULL)
    font->Release ();
}
