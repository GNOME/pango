/* Pango
 * pangocairo-dwrite-font.cpp: PangoCairo fonts using DirectWrite
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

#ifdef HAVE_DIRECT_WRITE

#include <windows.h>
#include <dwrite.h>
#include <hb-directwrite.h>
#include <cairo-win32.h>

#include "pangocairo-private.h"

/* {{{ DirectWrite PangoCairo utilities */
cairo_font_face_t *
pango_cairo_create_font_face_for_dwrite_pango_font (PangoFont *font)
{
  hb_font_t *hb_font;
  IDWriteFontFace *dwrite_font_face = NULL;
  cairo_font_face_t *result;

  hb_font = pango_font_get_hb_font (font);
  dwrite_font_face = hb_directwrite_face_get_font_face (hb_font_get_face (hb_font));

  result = cairo_dwrite_font_face_create_for_dwrite_fontface (dwrite_font_face);

  return result;
}

#endif /* HAVE_DIRECT_WRITE */

/* }}} */

/* vim:set foldmethod=marker expandtab: */
