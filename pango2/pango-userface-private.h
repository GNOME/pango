/*
 * Copyright 2022 Red Hat, Inc.
 *
 * SPDX-License-Identifier: LGPL-2.1-or-later
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public
 * License as published by the Free Software Foundation; either
 * version 2.1 of the License, or (at your option) any later version.
 *
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public
 * License along with this library; if not, see <http://www.gnu.org/licenses/>.
 */

#pragma once

#include "pango-userface.h"
#include "pango-fontmap.h"
#include "pango-font-face-private.h"


struct _Pango2UserFace
{
  Pango2FontFace parent_instance;

  char *faceid;

  Pango2UserFaceGetFontInfoFunc font_info_func;
  Pango2UserFaceUnicodeToGlyphFunc glyph_func;
  Pango2UserFaceGetGlyphInfoFunc glyph_info_func;
  Pango2UserFaceTextToGlyphFunc shape_func;
  Pango2UserFaceRenderGlyphFunc render_func;
  gpointer user_data;
  GDestroyNotify destroy;
};
