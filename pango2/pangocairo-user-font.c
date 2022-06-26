/*
 * pangocairo-user-font.c: User font handling
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

#include "pango-features.h"
#include "pangocairo-private.h"
#include "pango-userfont-private.h"
#include "pango-userface-private.h"

#include <hb-ot.h>

static cairo_user_data_key_t cairo_user_data;

static cairo_status_t
render_func (cairo_scaled_font_t  *scaled_font,
             unsigned long         glyph,
             cairo_t              *cr,
             cairo_text_extents_t *extents)
{
  cairo_font_face_t *font_face;
  Pango2Font *font;
  Pango2UserFace *face;
  hb_glyph_extents_t glyph_extents;
  hb_position_t h_advance;
  hb_position_t v_advance;
  gboolean is_color;

  font_face = cairo_scaled_font_get_font_face (scaled_font);
  font = cairo_font_face_get_user_data (font_face, &cairo_user_data);
  face = PANGO2_USER_FACE (font->face);

  extents->x_bearing = 0;
  extents->y_bearing = 0;
  extents->width = 0;
  extents->height = 0;
  extents->x_advance = 0;
  extents->y_advance = 0;

  if (!face->glyph_info_func (face, 1024,
                              (hb_codepoint_t)glyph,
                              &glyph_extents,
                              &h_advance, &v_advance,
                              &is_color,
                              face->user_data))
    {
      return CAIRO_STATUS_USER_FONT_ERROR;
    }

  extents->x_bearing = glyph_extents.x_bearing / 1024.;
  extents->y_bearing = - glyph_extents.y_bearing / 1024.;
  extents->width = glyph_extents.width / 1024.;
  extents->height = - glyph_extents.height / 1024.;
  extents->x_advance = h_advance / 1024.;
  extents->y_advance = v_advance / 1024.;

  if (!face->render_func (face, font->size,
                          (hb_codepoint_t)glyph,
                          face->user_data,
                          "cairo",
                          cr))
    {
      return CAIRO_STATUS_USER_FONT_ERROR;
    }

  return CAIRO_STATUS_SUCCESS;
}

static cairo_status_t
init_func (cairo_scaled_font_t  *scaled_font,
           cairo_t              *cr,
           cairo_font_extents_t *extents)
{
  cairo_font_face_t *cairo_face;
  Pango2Font *font;
  Pango2UserFace *face;
  hb_font_extents_t font_extents;

  cairo_face = cairo_scaled_font_get_font_face (scaled_font);
  font = cairo_font_face_get_user_data (cairo_face, &cairo_user_data);
  face = (Pango2UserFace *) pango2_font_get_face (font);

  face->font_info_func (face,
                        pango2_font_get_size (font),
                        &font_extents,
                        face->user_data);

  extents->ascent = font_extents.ascender / (font_extents.ascender + font_extents.descender);
  extents->descent = font_extents.descender / (font_extents.ascender + font_extents.descender);

  return CAIRO_STATUS_SUCCESS;
}

cairo_font_face_t *
create_cairo_user_font_face (Pango2Font *font)
{
  cairo_font_face_t *cairo_face;

  cairo_face = cairo_user_font_face_create ();
  cairo_font_face_set_user_data (cairo_face, &cairo_user_data, font, NULL);
  cairo_user_font_face_set_init_func (cairo_face, init_func);
  cairo_user_font_face_set_render_color_glyph_func (cairo_face, render_func);

  return cairo_face;
}
