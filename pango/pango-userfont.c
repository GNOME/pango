/* Pango2
 *
 * Copyright (C) 2021 Matthias Clasen
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

#include "pango-userfont-private.h"

#include "pango-font-private.h"
#include "pango-font-metrics-private.h"
#include "pango-userface-private.h"
#include "pango-hbfamily-private.h"
#include "pango-impl-utils.h"
#include "pango-language-set-private.h"

#include <hb-ot.h>

/**
 * Pango2UserFont:
 *
 * `Pango2UserFont` is a `Pango2Font` implementation that uses callbacks.
 */

/* {{{ Pango2Font implementation */

struct _Pango2UserFontClass
{
  Pango2FontClass parent_class;
};

G_DEFINE_FINAL_TYPE (Pango2UserFont, pango2_user_font, PANGO2_TYPE_FONT)

static void
pango2_user_font_init (Pango2UserFont *self G_GNUC_UNUSED)
{
}

static void
pango2_user_font_finalize (GObject *object)
{
  G_OBJECT_CLASS (pango2_user_font_parent_class)->finalize (object);
}

static Pango2FontDescription *
pango2_user_font_describe (Pango2Font *font)
{
  Pango2FontDescription *desc;

  desc = pango2_font_face_describe (PANGO2_FONT_FACE (font->face));
  pango2_font_description_set_gravity (desc, font->gravity);
  pango2_font_description_set_size (desc, font->size);

  return desc;
}

static void
pango2_user_font_get_glyph_extents (Pango2Font      *font,
                                    Pango2Glyph      glyph,
                                    Pango2Rectangle *ink_rect,
                                    Pango2Rectangle *logical_rect)
{
  hb_font_t *hb_font = pango2_font_get_hb_font (font);
  hb_glyph_extents_t extents;
  hb_direction_t direction;
  hb_font_extents_t font_extents;

  direction = PANGO2_GRAVITY_IS_VERTICAL (font->gravity)
              ? HB_DIRECTION_TTB
              : HB_DIRECTION_LTR;

  hb_font_get_extents_for_direction (hb_font, direction, &font_extents);

  if (glyph == PANGO2_GLYPH_EMPTY || (glyph & PANGO2_GLYPH_UNKNOWN_FLAG))
    {
      if (ink_rect)
        ink_rect->x = ink_rect->y = ink_rect->width = ink_rect->height = 0;

      if (logical_rect)
        {
          logical_rect->x = logical_rect->width = 0;
          logical_rect->y = - font_extents.ascender;
          logical_rect->height = font_extents.ascender - font_extents.descender;
        }

      return;
    }

  hb_font_get_glyph_extents (hb_font, glyph, &extents);

  if (ink_rect)
    {
      Pango2Rectangle r;

      r.x = extents.x_bearing;
      r.y = - extents.y_bearing;
      r.width = extents.width;
      r.height = - extents.height;

      switch (font->gravity)
        {
        case PANGO2_GRAVITY_AUTO:
        case PANGO2_GRAVITY_SOUTH:
          ink_rect->x = r.x;
          ink_rect->y = r.y;
          ink_rect->width = r.width;
          ink_rect->height = r.height;
          break;
        case PANGO2_GRAVITY_NORTH:
          ink_rect->x = - r.x;
          ink_rect->y = - r.y;
          ink_rect->width = - r.width;
          ink_rect->height = - r.height;
          break;
        case PANGO2_GRAVITY_EAST:
          ink_rect->x = r.y;
          ink_rect->y = - r.x - r.width;
          ink_rect->width = r.height;
          ink_rect->height = r.width;
          break;
        case PANGO2_GRAVITY_WEST:
          ink_rect->x = - r.y - r.height;
          ink_rect->y = r.x;
          ink_rect->width = r.height;
          ink_rect->height = r.width;
          break;
        default:
          g_assert_not_reached ();
        }

      if (PANGO2_GRAVITY_IS_IMPROPER (font->gravity))
        {
          Pango2Matrix matrix = (Pango2Matrix) PANGO2_MATRIX_INIT;
          pango2_matrix_scale (&matrix, -1, -1);
          pango2_matrix_transform_rectangle (&matrix, ink_rect);
        }
    }

  if (logical_rect)
    {
      hb_position_t h_advance;
      hb_font_extents_t extents;

      h_advance = hb_font_get_glyph_h_advance (hb_font, glyph);
      hb_font_get_h_extents (hb_font, &extents);

      logical_rect->x = 0;
      logical_rect->height = extents.ascender - extents.descender;

      switch (font->gravity)
        {
        case PANGO2_GRAVITY_AUTO:
        case PANGO2_GRAVITY_SOUTH:
          logical_rect->y = - extents.ascender;
          logical_rect->width = h_advance;
          break;
        case PANGO2_GRAVITY_NORTH:
          logical_rect->y = extents.descender;
          logical_rect->width = h_advance;
          break;
        case PANGO2_GRAVITY_EAST:
          logical_rect->y = - logical_rect->height / 2;
          logical_rect->width = logical_rect->height;
          break;
        case PANGO2_GRAVITY_WEST:
          logical_rect->y = - logical_rect->height / 2;
          logical_rect->width = - logical_rect->height;
          break;
        default:
          g_assert_not_reached ();
        }

      if (PANGO2_GRAVITY_IS_IMPROPER (font->gravity))
        {
          logical_rect->height = - logical_rect->height;
          logical_rect->y = - logical_rect->y;
        }
    }
}

static Pango2FontMetrics *
pango2_user_font_get_metrics (Pango2Font     *font,
                              Pango2Language *language)
{
  hb_font_t *hb_font = pango2_font_get_hb_font (font);
  Pango2FontMetrics *metrics;
  hb_font_extents_t extents;

  metrics = pango2_font_metrics_new ();

  hb_font_get_extents_for_direction (hb_font, HB_DIRECTION_LTR, &extents);

  metrics->descent = - extents.descender;
  metrics->ascent = extents.ascender;
  metrics->height = extents.ascender - extents.descender + extents.line_gap;

  metrics->underline_thickness = PANGO2_SCALE;
  metrics->underline_position = - PANGO2_SCALE;
  metrics->strikethrough_thickness = PANGO2_SCALE;
  metrics->strikethrough_position = metrics->ascent / 2;
  metrics->approximate_char_width = 0; /* FIXME */
  metrics->approximate_digit_width = 0;

  return metrics;
}

static hb_bool_t
nominal_glyph_func (hb_font_t      *hb_font,
                    void           *font_data,
                    hb_codepoint_t  unicode,
                    hb_codepoint_t *glyph,
                    void           *user_data)
{
  Pango2Font *font = font_data;
  Pango2UserFace *face = PANGO2_USER_FACE (font->face);

  return face->glyph_func (face, unicode, glyph, face->user_data);
}

static hb_position_t
glyph_h_advance_func (hb_font_t      *hb_font,
                      void           *font_data,
                      hb_codepoint_t  glyph,
                      void           *user_data)
{
  Pango2Font *font = font_data;
  Pango2UserFace *face = PANGO2_USER_FACE (font->face);
  int size = font->size * font->dpi / 72.;
  hb_position_t h_advance, v_advance;
  hb_glyph_extents_t glyph_extents;
  gboolean is_color;

  face->glyph_info_func (face, size, glyph,
                         &glyph_extents,
                         &h_advance, &v_advance,
                         &is_color,
                         face->user_data);

  return h_advance;
}

static hb_bool_t
glyph_extents_func (hb_font_t          *hb_font,
                    void               *font_data,
                    hb_codepoint_t      glyph,
                    hb_glyph_extents_t *extents,
                    void               *user_data)
{
  Pango2Font *font = font_data;
  Pango2UserFace *face = PANGO2_USER_FACE (font->face);
  int size = font->size * font->dpi / 72.;
  hb_position_t h_advance, v_advance;
  gboolean is_color;

  face->glyph_info_func (face, size, glyph,
                         extents,
                         &h_advance, &v_advance,
                         &is_color,
                         face->user_data);

  if (PANGO2_GRAVITY_IS_IMPROPER (font->gravity))
    {
      extents->x_bearing = - extents->x_bearing;
      extents->y_bearing = - extents->y_bearing;
      extents->width = - extents->width;
      extents->height = - extents->height;
    }

  return TRUE;
}

static hb_bool_t
font_extents_func (hb_font_t         *hb_font,
                   void              *font_data,
                   hb_font_extents_t *extents,
                   void              *user_data)
{
  Pango2Font *font = font_data;
  Pango2UserFace *face = PANGO2_USER_FACE (font->face);
  int size = font->size * font->dpi / 72.;
  gboolean ret;

  ret = face->font_info_func (face, size, extents, face->user_data);

  if (PANGO2_GRAVITY_IS_IMPROPER (font->gravity))
    {
      extents->ascender = - extents->ascender;
      extents->descender = - extents->descender;
    }

  return ret;
}

static hb_font_t *
pango2_user_font_create_hb_font (Pango2Font *font)
{
  double x_scale, y_scale;
  int size;
  hb_blob_t *blob;
  hb_face_t *hb_face;
  hb_font_t *hb_font;
  hb_font_funcs_t *funcs;

  blob = hb_blob_create ("", 0, HB_MEMORY_MODE_READONLY, NULL, NULL);
  hb_face = hb_face_create (blob, 0);
  hb_font = hb_font_create (hb_face);

  funcs = hb_font_funcs_create ();

  hb_font_funcs_set_nominal_glyph_func (funcs, nominal_glyph_func, NULL, NULL);
  hb_font_funcs_set_glyph_h_advance_func (funcs, glyph_h_advance_func, NULL, NULL);
  hb_font_funcs_set_glyph_extents_func (funcs, glyph_extents_func, NULL, NULL);
  hb_font_funcs_set_font_h_extents_func (funcs, font_extents_func, NULL, NULL);
  hb_font_funcs_set_font_v_extents_func (funcs, font_extents_func, NULL, NULL);

  hb_font_set_funcs (hb_font, funcs, font, NULL);

  hb_font_funcs_destroy (funcs);
  hb_face_destroy (hb_face);
  hb_blob_destroy (blob);

  size = font->size * font->dpi / 72.f;
  x_scale = y_scale = 1;

  if (PANGO2_GRAVITY_IS_IMPROPER (font->gravity))
    {
      x_scale = - x_scale;
      y_scale = - y_scale;
    }

  hb_font_set_scale (hb_font, size * x_scale, size * y_scale);
  hb_font_set_ptem (hb_font, font->size / PANGO2_SCALE);

  return hb_font;
}

static void
pango2_user_font_class_init (Pango2UserFontClass *class)
{
  GObjectClass *object_class = G_OBJECT_CLASS (class);
  Pango2FontClass *font_class = PANGO2_FONT_CLASS (class);

  object_class->finalize = pango2_user_font_finalize;

  font_class->describe = pango2_user_font_describe;
  font_class->get_glyph_extents = pango2_user_font_get_glyph_extents;
  font_class->get_metrics = pango2_user_font_get_metrics;
  font_class->create_hb_font = pango2_user_font_create_hb_font;
}

/* }}} */
 /* {{{ Public API */

/**
 * pango2_user_font_new:
 * @face: the `Pango2UserFace` to use
 * @size: the desired size in points, scaled by `PANGO2_SCALE`
 * @gravity: the gravity to use when rendering
 * @dpi: the dpi used when rendering
 * @ctm: (nullable): transformation matrix to use when rendering
 *
 * Creates a new `Pango2UserFont`.
 *
 * Returns: a newly created `Pango2UserFont`
 */
Pango2UserFont *
pango2_user_font_new (Pango2UserFace     *face,
                      int                 size,
                      Pango2Gravity       gravity,
                      float               dpi,
                      const Pango2Matrix *ctm)
{
  Pango2UserFont *self;
  Pango2Font *font;

  self = g_object_new (PANGO2_TYPE_USER_FONT, NULL);

  font = PANGO2_FONT (self);

  pango2_font_set_face (font, PANGO2_FONT_FACE (face));
  pango2_font_set_size (font, size);
  pango2_font_set_dpi (font, dpi);
  pango2_font_set_gravity (font, gravity);
  pango2_font_set_ctm (font, ctm);

  return self;
}

/**
 * pango2_user_font_new_for_description:
 * @face: the `Pango2UserFace` to use
 * @description: a `Pango2FontDescription`
 * @dpi: the dpi used when rendering
 * @ctm: (nullable): transformation matrix to use when rendering
 *
 * Creates a new `Pango2UserFont` with size and gravity taken
 * from a font description.
 *
 * Returns: a newly created `Pango2HbFont`
 */

Pango2UserFont *
pango2_user_font_new_for_description (Pango2UserFace              *face,
                                      const Pango2FontDescription *description,
                                      float                        dpi,
                                      const Pango2Matrix          *ctm)
{
  int size;
  Pango2Gravity gravity;

  if (pango2_font_description_get_size_is_absolute (description))
    size = pango2_font_description_get_size (description) * 72. / dpi;
  else
    size = pango2_font_description_get_size (description);

  if ((pango2_font_description_get_set_fields (description) & PANGO2_FONT_MASK_GRAVITY) != 0 &&
      pango2_font_description_get_gravity (description) != PANGO2_GRAVITY_SOUTH)
    gravity = pango2_font_description_get_gravity (description);
  else
    gravity = PANGO2_GRAVITY_AUTO;

  return pango2_user_font_new (face, size, gravity, dpi, ctm);
}

/* }}} */

/* vim:set foldmethod=marker expandtab: */
