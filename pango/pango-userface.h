/* Pango
 *
 * Copyright (C) 2022 Matthias Clasen
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

#pragma once

#include <pango/pango-types.h>
#include <pango/pango-glyph.h>
#include <pango/pango-font.h>

G_BEGIN_DECLS

typedef struct _PangoUserFont PangoUserFont;

#define PANGO_TYPE_USER_FACE      (pango_user_face_get_type ())

PANGO_AVAILABLE_IN_1_52
G_DECLARE_FINAL_TYPE (PangoUserFace, pango_user_face, PANGO, USER_FACE, PangoFontFace)

typedef gboolean      (* PangoUserFaceGetFontInfoFunc)     (PangoUserFace     *face,
                                                            int                size,
                                                            hb_font_extents_t *extents,
                                                            gpointer           user_data);

typedef gboolean      (* PangoUserFaceUnicodeToGlyphFunc)  (PangoUserFace  *face,
                                                            hb_codepoint_t  unicode,
                                                            hb_codepoint_t *glyph,
                                                            gpointer        user_data);

typedef gboolean      (* PangoUserFaceGetGlyphInfoFunc)    (PangoUserFace      *face,
                                                            int                 size,
                                                            hb_codepoint_t      glyph,
                                                            hb_glyph_extents_t *extents,
                                                            hb_position_t      *h_advance,
                                                            hb_position_t      *v_advance,
                                                            gboolean           *is_color_glyph,
                                                            gpointer            user_data);

typedef gboolean      (* PangoUserFaceTextToGlyphFunc)     (PangoUserFace       *face,
                                                            int                  size,
                                                            const char          *text,
                                                            int                  length,
                                                            const PangoAnalysis *analysis,
                                                            PangoGlyphString    *glyphs,
                                                            PangoShapeFlags      flags,
                                                            gpointer             user_data);

typedef gboolean      (* PangoUserFaceRenderGlyphFunc)     (PangoUserFace  *face,
                                                            int             size,
                                                            hb_codepoint_t  glyph,
                                                            gpointer        user_data,
                                                            const char     *backend_id,
                                                            gpointer        backend_data);

PANGO_AVAILABLE_IN_1_52
PangoUserFace *   pango_user_face_new          (PangoUserFaceGetFontInfoFunc    font_info_func,
                                                PangoUserFaceUnicodeToGlyphFunc glyph_func,
                                                PangoUserFaceGetGlyphInfoFunc   glyph_info_func,
                                                PangoUserFaceTextToGlyphFunc    shape_func,
                                                PangoUserFaceRenderGlyphFunc    render_func,
                                                gpointer                        user_data,
                                                GDestroyNotify                  destroy,
                                                const char                     *name,
                                                const PangoFontDescription     *description);

G_END_DECLS
