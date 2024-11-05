/* Pango
 * pangowin32-dwrite-utils-legacy.cpp: Win32 font handling with DirectWrite (for older Windows)
 *
 * Copyright (C) 2023 Chun-wei Fan
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

#ifdef STRICT
#undef STRICT
#endif


#include <initguid.h>
#include <string>
#include <vector>

/* stub, for simplicity reasons, if we don't have IDWriteFactory[3|5] */
#ifdef HAVE_DWRITE_3_H
# include <dwrite_3.h>
#else
# include <dwrite_1.h>
# define IDWriteFactory3 IUnknown
# define IDWriteFactory5 IUnknown
# define IDWriteFontSet IUnknown
# define IDWriteFontSetBuilder IUnknown
# define IDWriteFontSetBuilder1 IUnknown
#endif

#include "pangowin32-private.hpp"

#ifdef _MSC_VER
# define UUID_OF_IDWRITE_FONT_COLLECTION_LOADER __uuidof (IDWriteFontCollectionLoader)
# define UUID_OF_IDWRITE_FONT_FILE_ENUMERATOR   __uuidof (IDWriteFontFileEnumerator)
#else
# define UUID_OF_IDWRITE_FONT_COLLECTION_LOADER IID_IDWriteFontCollectionLoader
# define UUID_OF_IDWRITE_FONT_FILE_ENUMERATOR   IID_IDWriteFontFileEnumerator
#endif

/* COM-derived class for loading custom fonts on systems without IDWriteFactory[3|5] */
class pango_win32_font_collection_loader_legacy : public IDWriteFontCollectionLoader
{
  public:
    pango_win32_font_collection_loader_legacy () : ref_count(0) {}

    /* IUnknown methods */
    virtual HRESULT STDMETHODCALLTYPE QueryInterface (REFIID iid, void** obj_ptr);
    virtual ULONG STDMETHODCALLTYPE AddRef ();
    virtual ULONG STDMETHODCALLTYPE Release ();

    /* IDWriteFontCollectionLoader methods */
    virtual HRESULT STDMETHODCALLTYPE
    CreateEnumeratorFromKey (IDWriteFactory                 *factory,
                             /* [key] in bytes */
                             void const                     *key,
                             UINT32                          size,
                             OUT IDWriteFontFileEnumerator **enumerator);

  private:
    ULONG ref_count;
};

typedef struct _Win32CustomFontKey
{
  PangoWin32FontMap *map;
  const char *path;
} Win32CustomFontKey;

/* COM-dervied class for implementing custom font enumeration */
class pango_win32_font_file_enumerator_legacy : public IDWriteFontFileEnumerator
{
  public:
    pango_win32_font_file_enumerator_legacy ();

    ~pango_win32_font_file_enumerator_legacy ()
      {
        pangowin32_release_com_obj (&curr_file);
        g_array_unref (paths);
      }

    /* Utility method */
    GArray *add_font_file_path (const char *path);

    // IUnknown methods
    virtual HRESULT STDMETHODCALLTYPE
	QueryInterface (REFIID iid, void **obj_ptr);
    virtual ULONG STDMETHODCALLTYPE AddRef ();
    virtual ULONG STDMETHODCALLTYPE Release ();

    // IDWriteFontFileEnumerator methods
    virtual HRESULT STDMETHODCALLTYPE MoveNext (OUT BOOL *has_current_file);
    virtual HRESULT STDMETHODCALLTYPE
	GetCurrentFontFile (OUT IDWriteFontFile **font_file);

private:
    ULONG ref_count;

    IDWriteFontFile *curr_file;
    GArray *paths;
    size_t next_idx;
};

/* methods for implementing IDWriteFontCollectionLoader */
HRESULT STDMETHODCALLTYPE
pango_win32_font_collection_loader_legacy::QueryInterface (REFIID iid, void **obj_ptr)
{
  if (iid == IID_IUnknown || iid == UUID_OF_IDWRITE_FONT_COLLECTION_LOADER)
    {
      *obj_ptr = this;
      AddRef();
      return S_OK;
    }
  else
    {
      *obj_ptr = NULL;
      return E_NOINTERFACE;
    }
}

ULONG STDMETHODCALLTYPE
pango_win32_font_collection_loader_legacy::AddRef ()
{
  return InterlockedIncrement (&ref_count);
}

ULONG STDMETHODCALLTYPE
pango_win32_font_collection_loader_legacy::Release ()
{
  ULONG newCount = InterlockedDecrement (&ref_count);
  if (newCount == 0)
    delete this;

  return newCount;
}

/* methods for implementing IDWriteFontFileEnumerator */
HRESULT STDMETHODCALLTYPE
pango_win32_font_file_enumerator_legacy::QueryInterface (REFIID iid, void **obj_ptr)
{
  if (iid == IID_IUnknown || iid == UUID_OF_IDWRITE_FONT_FILE_ENUMERATOR)
    {
      *obj_ptr = this;
      AddRef ();
      return S_OK;
    }
  else
    {
      *obj_ptr = NULL;
      return E_NOINTERFACE;
    }
}

ULONG STDMETHODCALLTYPE
pango_win32_font_file_enumerator_legacy::AddRef ()
{
  return InterlockedIncrement(&ref_count);
}

ULONG STDMETHODCALLTYPE
pango_win32_font_file_enumerator_legacy::Release ()
{
  ULONG new_count = InterlockedDecrement (&ref_count);
  if (new_count == 0)
    delete this;

  return new_count;
}

HRESULT STDMETHODCALLTYPE
pango_win32_font_file_enumerator_legacy::MoveNext (OUT BOOL* has_current_file)
{
  HRESULT hr = S_OK;
  PangoWin32DWriteItems *items = pango_win32_get_direct_write_items ();

  *has_current_file = FALSE;
  pangowin32_release_com_obj (&curr_file);

  if (next_idx < paths->len)
    {
      const char **path = &g_array_index (paths, const char *, next_idx);
      wchar_t *w_path = (wchar_t *)g_utf8_to_utf16 (*path, -1, NULL, NULL, NULL);
      gboolean supported;
      DWRITE_FONT_FILE_TYPE file_type;
      UINT32 num_fonts;

      if (w_path == NULL)
        return E_FAIL;

      hr = items->dwrite_factory->CreateFontFileReference (w_path, NULL, &curr_file);

      if (SUCCEEDED (hr))
        hr = curr_file->Analyze (&supported, &file_type, nullptr, &num_fonts);

      if (SUCCEEDED (hr) && supported)
        {
          *has_current_file = TRUE;

         ++next_idx;
        }

      g_free (w_path);
    }

  return hr;
}

GArray *
pango_win32_font_file_enumerator_legacy::add_font_file_path (const char *path)
{
  GArray *ary;
  ary = g_array_append_vals (paths, &path, 1);
  return ary;
}

HRESULT STDMETHODCALLTYPE
pango_win32_font_collection_loader_legacy::CreateEnumeratorFromKey (IDWriteFactory *factory,
                                                                    /* [key] in bytes */
                                                                    void const     *key,
                                                                    UINT32          key_size,
                                                                    OUT IDWriteFontFileEnumerator **enumerator)
{
  *enumerator = NULL;

  HRESULT hr = S_OK;
  PangoWin32FontMap *map;

  if (key_size % sizeof (Win32CustomFontKey) != 0)
    return E_INVALIDARG;

  pango_win32_font_file_enumerator_legacy* legacy_enumerator =
    new (std::nothrow) pango_win32_font_file_enumerator_legacy ();
  if (enumerator == NULL)
    return E_OUTOFMEMORY;

  Win32CustomFontKey const *collection_key = static_cast<Win32CustomFontKey const*>(key);
  map = collection_key->map;

  for (guint i = 0; i < map->custom_fonts_legacy->paths->len; i ++)
    {
      const char **path = &g_array_index (map->custom_fonts_legacy->paths, const char *, i);
      legacy_enumerator->add_font_file_path (*path);
    }

  legacy_enumerator->add_font_file_path (collection_key->path);

  *enumerator = pangowin32_acquire_com_obj (legacy_enumerator);

  return hr;
}

pango_win32_font_file_enumerator_legacy::pango_win32_font_file_enumerator_legacy () :
  ref_count(0),
  curr_file (),
  next_idx (0)
{
  paths = g_array_new (FALSE, FALSE, sizeof (const char *));
}

HRESULT STDMETHODCALLTYPE
pango_win32_font_file_enumerator_legacy::GetCurrentFontFile (OUT IDWriteFontFile** fontFile)
{
  *fontFile = pangowin32_acquire_com_obj (curr_file);

  return (curr_file != NULL) ? S_OK : E_FAIL;
}

HRESULT 
pango_win32_create_legacy_font_collection (PangoWin32FontMap          *map,
                                           IDWriteFactory             *factory,
                                           const char                 *path,
                                           OUT IDWriteFontCollection **collection)
{
  HRESULT hr = S_OK;

  /* sadly, no designated initializers in older non-GNU C++ */
  Win32CustomFontKey key = {map, path};

  const void *collection_key = &key;
  UINT32 key_size = sizeof (key);
  IDWriteFontCollectionLoader *custom_loader;

  if (map->custom_fonts_legacy == NULL)
    map->custom_fonts_legacy = g_new0 (PangoWin32CustomFontsLegacy, 1);

  if (map->custom_fonts_legacy->paths == NULL)
    map->custom_fonts_legacy->paths = g_array_new (FALSE, FALSE, sizeof (const char *));

  if (map->custom_fonts_legacy->font_collection_loader == NULL)
    {
      custom_loader = new(std::nothrow) pango_win32_font_collection_loader_legacy ();
      hr = factory->RegisterFontCollectionLoader (custom_loader);
      if (FAILED (hr))
        return hr;

      map->custom_fonts_legacy->font_collection_loader = custom_loader;
      map->custom_fonts_legacy->is_loader_registered = TRUE;
    }
  else
    custom_loader = map->custom_fonts_legacy->font_collection_loader;

  hr = factory->CreateCustomFontCollection (custom_loader,
                                            collection_key,
                                            key_size,
                                            collection);

  if (SUCCEEDED (hr))
    g_array_append_vals (map->custom_fonts_legacy->paths, &path, 1);

  return hr;
}

gboolean
pango_win32_release_legacy_font_loader (PangoWin32FontMap *map)
{
  if (map->custom_fonts_legacy != NULL)
    {
      HRESULT hr;
      PangoWin32DWriteItems *items = pango_win32_get_direct_write_items ();
      IDWriteFactory *factory = items->dwrite_factory;
	  hr = factory->UnregisterFontCollectionLoader (map->custom_fonts_legacy->font_collection_loader);
      if (FAILED (hr))
        return FALSE;

      map->custom_fonts_legacy->is_loader_registered = FALSE;
      map->custom_fonts_legacy->font_collection_loader->Release ();
      g_free (map->custom_fonts_legacy);
      map->custom_fonts_legacy = NULL;
    }

  return TRUE;
}
