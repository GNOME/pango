/* Pango
 * pangowin32-dwrite-utils.h: Win32 font handling with DirectWrite
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

/*
 * Portions adapted from https://stackoverflow.com/questions/37572961/c-directwrite-load-font-from-file-at-runtime,
 * by @M_F
 */
/*
 * Define our COM interface for the IDWriteFontCollectionLoader that we need
 * to implement
 */

#ifndef __PANGOWIN32_DWRITE_UTILS_H__
#define __PANGOWIN32_DWRITE_UTILS_H__

#include <dwrite_1.h>
#include <string>
#include <vector>

typedef std::vector<std::wstring> PangoWin32Collection;

class PangoWin32FontCollectionLoader : public IDWriteFontCollectionLoader
{
public:
  PangoWin32FontCollectionLoader() : refCount_(0)
    {
    }

  // IUnknown methods
  virtual HRESULT STDMETHODCALLTYPE QueryInterface        (REFIID         iid,
                                                           void         **ppvObject);
  virtual ULONG STDMETHODCALLTYPE   AddRef                ();
  virtual ULONG STDMETHODCALLTYPE   Release               ();

  // IDWriteFontCollectionLoader methods
  virtual HRESULT STDMETHODCALLTYPE
  CreateEnumeratorFromKey (IDWriteFactory *factory,
                           void const     *key,
                           UINT32          key_size,
                           OUT             IDWriteFontFileEnumerator **enumerator);

  // Gets the singleton loader instance.
  static IDWriteFontCollectionLoader* GetLoader           ()
    {
      return instance_;
    }

  static bool                         IsLoaderInitialized ()
    {
      return instance_ != NULL;
    }

private:
  ULONG refCount_;
  static IDWriteFontCollectionLoader* instance_;
};

class PangoWin32FontFileEnumerator : public IDWriteFontFileEnumerator
{
public:
  PangoWin32FontFileEnumerator                         (IDWriteFactory      *factory);
  HRESULT Initialize                                   (UINT const          *key,
                                                        UINT32               key_size);
  ~PangoWin32FontFileEnumerator                        ()
    {
      if (currentFile_ != NULL)
        {
          currentFile_->Release  ();
          currentFile_ = NULL;
        }
    }

    // IUnknown methods
  virtual HRESULT STDMETHODCALLTYPE QueryInterface     (REFIID               iid,
                                                        void               **ppvObject);
  virtual ULONG STDMETHODCALLTYPE   AddRef             ();
  virtual ULONG STDMETHODCALLTYPE   Release            ();

  // IDWriteFontFileEnumerator methods
  virtual HRESULT STDMETHODCALLTYPE MoveNext           (OUT BOOL             *hasCurrentFile);
  virtual HRESULT STDMETHODCALLTYPE GetCurrentFontFile (OUT IDWriteFontFile **fontFile);

private:
    ULONG refCount_;

    IDWriteFactory* factory_;
    IDWriteFontFile* currentFile_;
    std::vector<std::wstring> filePaths_;
    size_t nextIndex_;
};

class PangoWin32FontContext
{
public:
  PangoWin32FontContext        (IDWriteFactory             *factory);
  ~PangoWin32FontContext       ();

  HRESULT Initialize           ();

  HRESULT CreateFontCollection (PangoWin32Collection       &newCollection,
                                OUT IDWriteFontCollection **result);

private:
  // Not copyable or assignable.
  PangoWin32FontContext        (PangoWin32FontContext const&);
  void                operator=(PangoWin32FontContext const&);

  HRESULT InitializeInternal ();
  IDWriteFactory *g_dwriteFactory;
  static std::vector<unsigned int> cKeys;

  // Error code from Initialize().
  HRESULT hr_;
};

class PangoWin32FontGlobals
{
public:
  PangoWin32FontGlobals                                  () {}
  static unsigned int                        push        (PangoWin32Collection &addCollection)
    {
      unsigned int ret = fontCollections.size ();
      fontCollections.push_back (addCollection);
      return ret;
    }
  static std::vector<PangoWin32Collection>&  collections ()
    {
      return fontCollections;
    }
private:
    static std::vector<PangoWin32Collection> fontCollections;
};

#endif /* __PANGOWIN32_DWRITE_UTILS_H__ */
