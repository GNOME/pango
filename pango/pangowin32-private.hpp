/* Pango
 * pangowin32-private.hpp: C++ header-only utility code
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
 
#ifndef __PANGOWIN32_PRIVATE_HPP__
#define __PANGOWIN32_PRIVATE_HPP__

/* We are likely to use items from the following by including this header */
#include "pangowin32-private.h"

struct _PangoWin32DWriteItems
{
  IDWriteFactory5   *dwrite_factory5;
  IDWriteFactory3   *dwrite_factory3;
  IDWriteFactory    *dwrite_factory;
  IDWriteGdiInterop *gdi_interop;
};

struct _PangoWin32CustomFontsLegacy
{
  /* custom fonts files that have been loaded */
  GArray *paths;

  /* our custom IDWriteFontCollectionLoader to be used on Windows 7/8.x */
  IDWriteFontCollectionLoader *font_collection_loader;

  /* whether custom loader has been registered in DirectWrite */
  gboolean is_loader_registered;

  /* temporary IDWriteFontCollection for legacy use */
  IDWriteFontCollection *font_collection_temp;
};

/* Releases a COM object and clears it */
template <typename com_interface>
inline void pangowin32_release_com_obj (com_interface **curr_obj)
{
  if (*curr_obj != NULL)
    {
      (*curr_obj)->Release ();
      *curr_obj = NULL;
    }
}

/* Acquires an additional reference, if non-null */
template <typename com_interface>
inline com_interface* pangowin32_acquire_com_obj (com_interface *new_obj)
{
  if (new_obj != NULL)
    new_obj->AddRef ();

  return new_obj;
}

HRESULT
pango_win32_create_legacy_font_collection (PangoWin32FontMap          *map,
                                           IDWriteFactory             *factory,
                                           const char                 *path,
                                           OUT IDWriteFontCollection **collection);

#endif /* __PANGOWIN32_PRIVATE_HPP__ */
