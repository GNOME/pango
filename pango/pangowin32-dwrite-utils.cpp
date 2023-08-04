/* Pango
 * pangowin32-dwrite-utils.cpp: Win32 font handling with DirectWrite
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

#include <dwrite_1.h>

#include "pangowin32-dwrite-utils.h"

#ifdef _MSC_VER
# define UUID_OF_IDWriteFontCollectionLoader __uuidof(IDWriteFontCollectionLoader)
# define UUID_OF_IDWriteFontFileEnumerator __uuidof (IDWriteFontFileEnumerator)
#else
# define UUID_OF_IDWriteFontCollectionLoader IID_IDWriteFontCollectionLoader
# define UUID_OF_IDWriteFontFileEnumerator IID_IDWriteFontFileEnumerator
#endif

IDWriteFontCollectionLoader*
PangoWin32FontCollectionLoader::instance_ (new(std::nothrow) PangoWin32FontCollectionLoader());

HRESULT STDMETHODCALLTYPE
PangoWin32FontCollectionLoader::QueryInterface (REFIID   iid,
                                                void   **ppvObject)
{
  if (iid == IID_IUnknown || iid == UUID_OF_IDWriteFontCollectionLoader)
    {
      *ppvObject = this;
      AddRef();
      return S_OK;
    }
  else
    {
      *ppvObject = NULL;
      return E_NOINTERFACE;
    }
}

ULONG STDMETHODCALLTYPE PangoWin32FontCollectionLoader::AddRef ()
{
  return InterlockedIncrement(&refCount_);
}

ULONG STDMETHODCALLTYPE PangoWin32FontCollectionLoader::Release ()
{
  ULONG newCount = InterlockedDecrement(&refCount_);

  if (newCount == 0)
    delete this;

  return newCount;
}

HRESULT STDMETHODCALLTYPE
PangoWin32FontCollectionLoader::CreateEnumeratorFromKey(IDWriteFactory *factory,
                                                        void const     *key,
                                                        /*  [collectionKeySize] in bytes */
                                                        UINT32          key_size,
                                                        OUT IDWriteFontFileEnumerator** fontFileEnumerator)
{
  HRESULT hr = S_OK;

  *fontFileEnumerator = NULL;

  if (key_size % sizeof(UINT) != 0)
    return E_INVALIDARG;

  PangoWin32FontFileEnumerator* enumerator =
    new(std::nothrow) PangoWin32FontFileEnumerator(factory);

  if (enumerator == NULL)
    return E_OUTOFMEMORY;

  UINT const* coll_key = static_cast<UINT const*>(key);
  UINT32 const size = key_size;

  hr = enumerator->Initialize (coll_key, size);

  if (FAILED(hr))
    {
      delete enumerator;
      return hr;
    }

   if (enumerator != NULL)
     enumerator->AddRef ();

  *fontFileEnumerator = enumerator;

  return hr;
}

/* PangoWin32FontFileEnumerator */
PangoWin32FontFileEnumerator::PangoWin32FontFileEnumerator(IDWriteFactory* factory) :
  refCount_ (0),
  factory_ (factory),
  currentFile_ (),
  nextIndex_ (0)
{
}

HRESULT
PangoWin32FontFileEnumerator::Initialize(UINT const *key,
                                         UINT32      key_size)
{
  try
    {
      // dereference key in order to get index of collection in PangoWin32FontGlobals::fontCollections vector
      UINT cPos = *key;
      for (PangoWin32Collection::iterator it =
             PangoWin32FontGlobals::collections()[cPos].begin();
           it != PangoWin32FontGlobals::collections()[cPos].end();
         ++it)
        {
          filePaths_.push_back(it->c_str());
        }
    }
  catch (std::bad_alloc&) {return E_OUTOFMEMORY;}
  catch (...) {return E_FAIL;};

  return S_OK;
}

HRESULT STDMETHODCALLTYPE
PangoWin32FontFileEnumerator::QueryInterface (REFIID iid, void** ppvObject)
{
  if (iid == IID_IUnknown || iid == UUID_OF_IDWriteFontFileEnumerator)
    {
      *ppvObject = this;
      AddRef();
      return S_OK;
    }
  else
    {
      *ppvObject = NULL;
      return E_NOINTERFACE;
    }
}

ULONG STDMETHODCALLTYPE
PangoWin32FontFileEnumerator::AddRef ()
{
  return InterlockedIncrement(&refCount_);
}

ULONG STDMETHODCALLTYPE
PangoWin32FontFileEnumerator::Release()
{
  ULONG newCount = InterlockedDecrement(&refCount_);
  if (newCount == 0)
    delete this;

  return newCount;
}

HRESULT STDMETHODCALLTYPE PangoWin32FontFileEnumerator::MoveNext(OUT BOOL* hasCurrentFile)
{
  HRESULT hr = S_OK;

  *hasCurrentFile = FALSE;
  if (currentFile_ != NULL)
    {
      currentFile_->Release ();
      currentFile_ = NULL;
    }

  if (nextIndex_ < filePaths_.size())
    {
      hr = factory_->CreateFontFileReference (filePaths_[nextIndex_].c_str(),
                                              NULL,
                                             &currentFile_);

      if (SUCCEEDED(hr))
        {
          *hasCurrentFile = TRUE;
          ++nextIndex_;
        }
    }

    return hr;
}

HRESULT STDMETHODCALLTYPE
PangoWin32FontFileEnumerator::GetCurrentFontFile(OUT IDWriteFontFile **fontFile)
{
  if (currentFile_ != NULL)
    currentFile_->AddRef ();

  *fontFile = currentFile_;

  return (currentFile_ != NULL) ? S_OK : E_FAIL;
}

/* PangoWin32FontContext */

PangoWin32FontContext::PangoWin32FontContext(IDWriteFactory *pFactory) :
  hr_(S_FALSE), 
  g_dwriteFactory(pFactory)
{
}

PangoWin32FontContext::~PangoWin32FontContext ()
{
  g_dwriteFactory->UnregisterFontCollectionLoader(PangoWin32FontCollectionLoader::GetLoader());
}

HRESULT PangoWin32FontContext::Initialize ()
{
  if (hr_ == S_FALSE)
    {
      hr_ = InitializeInternal();
    }

  return hr_;
}

HRESULT
PangoWin32FontContext::InitializeInternal ()
{
  HRESULT hr = S_OK;

  if (!PangoWin32FontCollectionLoader::IsLoaderInitialized())
    {
      return E_FAIL;
    }

  // Register our custom loader with the factory object.
  hr = g_dwriteFactory->RegisterFontCollectionLoader(PangoWin32FontCollectionLoader::GetLoader());

  return hr;
}

HRESULT
PangoWin32FontContext::CreateFontCollection (PangoWin32Collection       &newCollection,
                                             OUT IDWriteFontCollection **result)
{
  HRESULT hr = S_OK;
  *result = NULL;

  // save new collection in PangoWin32FontGlobals::fontCollections vector
  UINT collectionKey = PangoWin32FontGlobals::push (newCollection);
  cKeys.push_back (collectionKey);
  const void *fontCollectionKey = &cKeys.back ();
  UINT32 keySize = sizeof (collectionKey);

  hr = Initialize ();
  if (FAILED(hr))
    return hr;

  hr = g_dwriteFactory->CreateCustomFontCollection(PangoWin32FontCollectionLoader::GetLoader(),
                                                   fontCollectionKey,
                                                   keySize,
                                                   result);

  return hr;
}

std::vector<unsigned int>
PangoWin32FontContext::cKeys = std::vector<unsigned int>(0);

// ----------------------------------- PangoWin32FontGlobals ---------------------------------------------------------

std::vector<PangoWin32Collection>
PangoWin32FontGlobals::fontCollections = std::vector<PangoWin32Collection>(0);
