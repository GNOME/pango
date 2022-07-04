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

#include <pango2/pango-types.h>
#include <pango2/pango-glyph.h>
#include <pango2/pango-font-face.h>

G_BEGIN_DECLS

typedef struct _Pango2UserFont Pango2UserFont;

#define PANGO2_TYPE_USER_FACE      (pango2_user_face_get_type ())

PANGO2_AVAILABLE_IN_ALL
PANGO2_DECLARE_INTERNAL_TYPE (Pango2UserFace, pango2_user_face, PANGO2, USER_FACE, Pango2FontFace)

typedef gboolean      (* Pango2UserFaceGetFontInfoFunc)     (Pango2UserFace       *face,
                                                             int                   size,
                                                             hb_font_extents_t    *extents,
                                                             gpointer              user_data);

typedef gboolean      (* Pango2UserFaceUnicodeToGlyphFunc)  (Pango2UserFace       *face,
                                                             hb_codepoint_t        unicode,
                                                             hb_codepoint_t       *glyph,
                                                             gpointer              user_data);

typedef gboolean      (* Pango2UserFaceGetGlyphInfoFunc)    (Pango2UserFace       *face,
                                                             int                   size,
                                                             hb_codepoint_t        glyph,
                                                             hb_glyph_extents_t   *extents,
                                                             hb_position_t        *h_advance,
                                                             hb_position_t        *v_advance,
                                                             gboolean             *is_color_glyph,
                                                             gpointer              user_data);

typedef gboolean      (* Pango2UserFaceTextToGlyphFunc)     (Pango2UserFace       *face,
                                                             int                   size,
                                                             const char           *text,
                                                             int                   length,
                                                             const Pango2Analysis *analysis,
                                                             Pango2GlyphString    *glyphs,
                                                             Pango2ShapeFlags      flags,
                                                             gpointer              user_data);

typedef gboolean      (* Pango2UserFaceRenderGlyphFunc)     (Pango2UserFace       *face,
                                                             int                   size,
                                                             hb_codepoint_t        glyph,
                                                             gpointer              user_data,
                                                             const char           *backend_id,
                                                             gpointer              backend_data);

PANGO2_AVAILABLE_IN_ALL
Pango2UserFace *        pango2_user_face_new          (Pango2UserFaceGetFontInfoFunc     font_info_func,
                                                       Pango2UserFaceUnicodeToGlyphFunc  glyph_func,
                                                       Pango2UserFaceGetGlyphInfoFunc    glyph_info_func,
                                                       Pango2UserFaceTextToGlyphFunc     shape_func,
                                                       Pango2UserFaceRenderGlyphFunc     render_func,
                                                       gpointer                          user_data,
                                                       GDestroyNotify                    destroy,
                                                       const char                       *name,
                                                       const Pango2FontDescription      *description);

G_END_DECLS
