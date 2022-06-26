/*
 * pangocairo-coretext-font.c: CoreText font handling
 *
 * Copyright (C) 2022 Red Hat, Inc.
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

#include "pangocairo-private.h"
#include "pango-font.h"

#include <Carbon/Carbon.h>
#include <cairo-quartz.h>
#include <hb-coretext.h>

cairo_font_face_t *
create_cairo_core_text_font_face (Pango2Font *font)
{
  hb_font_t *hbfont;
  CTFontRef ctfont;
  CGFontRef cgfont;
  cairo_font_face_t *cairo_face;

  hbfont = pango2_font_get_hb_font (font);
  ctfont = hb_coretext_font_get_ct_font (hbfont);

  if (!ctfont)
    return NULL;

  cgfont = CTFontCopyGraphicsFont (ctfont, NULL);
  cairo_face = cairo_quartz_font_face_create_for_cgfont (cgfont);
  CFRelease (cgfont);

  return cairo_face;
}

