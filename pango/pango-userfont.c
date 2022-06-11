/* Pango
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
 * PangoUserFont:
 *
 * `PangoUserFont` is a `PangoFont` implementation that uses callbacks.
 */

/* {{{ PangoFont implementation */

struct _PangoUserFontClass
{
  PangoFontClass parent_class;
};

G_DEFINE_TYPE (PangoUserFont, pango_user_font, PANGO_TYPE_FONT)

static void
pango_user_font_init (PangoUserFont *self G_GNUC_UNUSED)
{
}

static void
pango_user_font_finalize (GObject *object)
{
  G_OBJECT_CLASS (pango_user_font_parent_class)->finalize (object);
}

static PangoFontDescription *
pango_user_font_describe (PangoFont *font)
{
  PangoFontDescription *desc;

  desc = pango_font_face_describe (PANGO_FONT_FACE (font->face));
  pango_font_description_set_gravity (desc, font->gravity);
  pango_font_description_set_size (desc, font->size);

  return desc;
}

static void
pango_user_font_get_glyph_extents (PangoFont      *font,
                                   PangoGlyph      glyph,
                                   PangoRectangle *ink_rect,
                                   PangoRectangle *logical_rect)
{
  hb_font_t *hb_font = pango_font_get_hb_font (font);
  hb_glyph_extents_t extents;
  hb_direction_t direction;
  hb_font_extents_t font_extents;

  direction = PANGO_GRAVITY_IS_VERTICAL (font->gravity)
              ? HB_DIRECTION_TTB
              : HB_DIRECTION_LTR;

  hb_font_get_extents_for_direction (hb_font, direction, &font_extents);

  if (glyph == PANGO_GLYPH_EMPTY || (glyph & PANGO_GLYPH_UNKNOWN_FLAG))
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
      PangoRectangle r;

      r.x = extents.x_bearing;
      r.y = - extents.y_bearing;
      r.width = extents.width;
      r.height = - extents.height;

      switch (font->gravity)
        {
        case PANGO_GRAVITY_AUTO:
        case PANGO_GRAVITY_SOUTH:
          ink_rect->x = r.x;
          ink_rect->y = r.y;
          ink_rect->width = r.width;
          ink_rect->height = r.height;
          break;
        case PANGO_GRAVITY_NORTH:
          ink_rect->x = - r.x;
          ink_rect->y = - r.y;
          ink_rect->width = - r.width;
          ink_rect->height = - r.height;
          break;
        case PANGO_GRAVITY_EAST:
          ink_rect->x = r.y;
          ink_rect->y = - r.x - r.width;
          ink_rect->width = r.height;
          ink_rect->height = r.width;
          break;
        case PANGO_GRAVITY_WEST:
          ink_rect->x = - r.y - r.height;
          ink_rect->y = r.x;
          ink_rect->width = r.height;
          ink_rect->height = r.width;
          break;
        default:
          g_assert_not_reached ();
        }

      if (PANGO_GRAVITY_IS_IMPROPER (font->gravity))
        {
          PangoMatrix matrix = (PangoMatrix) PANGO_MATRIX_INIT;
          pango_matrix_scale (&matrix, -1, -1);
          pango_matrix_transform_rectangle (&matrix, ink_rect);
        }
    }

  if (logical_rect)
    {
      hb_position_t h_advance;
      hb_font_extents_t extents;

      h_advance = hb_font_get_glyph_h_advance (hb_font, glyph);
      hb_font_get_h_extents (hb_font, &extents);

      logical_rect->x = 0;
      logical_rect->y = - extents.ascender;
      logical_rect->width = h_advance;
      logical_rect->height = extents.ascender - extents.descender;

      switch (font->gravity)
        {
        case PANGO_GRAVITY_AUTO:
        case PANGO_GRAVITY_SOUTH:
          logical_rect->y = - extents.ascender;
          logical_rect->width = h_advance;
          break;
        case PANGO_GRAVITY_NORTH:
          logical_rect->y = extents.descender;
          logical_rect->width = h_advance;
          break;
        case PANGO_GRAVITY_EAST:
          logical_rect->y = - logical_rect->height / 2;
          logical_rect->width = logical_rect->height;
          break;
        case PANGO_GRAVITY_WEST:
          logical_rect->y = - logical_rect->height / 2;
          logical_rect->width = - logical_rect->height;
          break;
        default:
          g_assert_not_reached ();
        }

      if (PANGO_GRAVITY_IS_IMPROPER (font->gravity))
        {
          logical_rect->height = - logical_rect->height;
          logical_rect->y = - logical_rect->y;
        }
    }
}

static PangoFontMetrics *
pango_user_font_get_metrics (PangoFont     *font,
                             PangoLanguage *language)
{
  hb_font_t *hb_font = pango_font_get_hb_font (font);
  PangoFontMetrics *metrics;
  hb_font_extents_t extents;

  metrics = pango_font_metrics_new ();

  hb_font_get_extents_for_direction (hb_font, HB_DIRECTION_LTR, &extents);

  metrics->descent = - extents.descender;
  metrics->ascent = extents.ascender;
  metrics->height = extents.ascender - extents.descender + extents.line_gap;

  metrics->underline_thickness = PANGO_SCALE;
  metrics->underline_position = - PANGO_SCALE;
  metrics->strikethrough_thickness = PANGO_SCALE;
  metrics->strikethrough_position = metrics->ascent / 2;
  metrics->approximate_char_width = 0; /* FIXME */
  metrics->approximate_digit_width = 0;

  return metrics;
}

static hb_bool_t
nominal_glyph_func (hb_font_t *hb_font, void *font_data,
                    hb_codepoint_t unicode,
                    hb_codepoint_t *glyph,
                    void *user_data)
{
  PangoFont *font = font_data;
  PangoUserFace *face = PANGO_USER_FACE (font->face);

  return face->glyph_func (face, unicode, glyph, face->user_data);
}

static hb_position_t
glyph_h_advance_func (hb_font_t *hb_font, void *font_data,
                      hb_codepoint_t glyph,
                      void *user_data)
{
  PangoFont *font = font_data;
  PangoUserFace *face = PANGO_USER_FACE (font->face);
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
glyph_extents_func (hb_font_t *hb_font, void *font_data,
                    hb_codepoint_t glyph,
                    hb_glyph_extents_t *extents,
                    void *user_data)
{
  PangoFont *font = font_data;
  PangoUserFace *face = PANGO_USER_FACE (font->face);
  int size = font->size * font->dpi / 72.;
  hb_position_t h_advance, v_advance;
  gboolean is_color;

  return face->glyph_info_func (face, size, glyph,
                                extents,
                                &h_advance, &v_advance,
                                &is_color,
                                face->user_data);
}

static hb_bool_t
font_extents_func (hb_font_t *hb_font, void *font_data,
                   hb_font_extents_t *extents,
                   void *user_data)
{
  PangoFont *font = font_data;
  PangoUserFace *face = PANGO_USER_FACE (font->face);
  int size = font->size * font->dpi / 72.;

  return face->font_info_func (face, size, extents, face->user_data);
}

static hb_font_t *
pango_user_font_create_hb_font (PangoFont *font)
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

  hb_font_set_funcs (hb_font, funcs, font, NULL);

  hb_font_funcs_destroy (funcs);
  hb_face_destroy (hb_face);
  hb_blob_destroy (blob);

  size = font->size * font->dpi / 72.f;
  x_scale = y_scale = 1;

  if (PANGO_GRAVITY_IS_IMPROPER (font->gravity))
    {
      x_scale = - x_scale;
      y_scale = - y_scale;
    }

  hb_font_set_scale (hb_font, size * x_scale, size * y_scale);
  hb_font_set_ptem (hb_font, font->size / PANGO_SCALE);

  return hb_font;
}

static gboolean
pango_user_font_has_char (PangoFont *font,
                          gunichar   wc)
{
  hb_font_t *user_font = pango_font_get_hb_font (font);
  hb_codepoint_t glyph;

  return hb_font_get_nominal_glyph (user_font, wc, &glyph);
}

static void
pango_user_font_class_init (PangoUserFontClass *class)
{
  GObjectClass *object_class = G_OBJECT_CLASS (class);
  PangoFontClass *font_class = PANGO_FONT_CLASS (class);

  object_class->finalize = pango_user_font_finalize;

  font_class->describe = pango_user_font_describe;
  font_class->get_glyph_extents = pango_user_font_get_glyph_extents;
  font_class->get_metrics = pango_user_font_get_metrics;
  font_class->create_hb_font = pango_user_font_create_hb_font;
  font_class->has_char = pango_user_font_has_char;
}

/* }}} */
 /* {{{ Public API */

/**
 * pango_user_font_new:
 * @face: the `PangoUserFace` to use
 * @size: the desired size in points, scaled by `PANGO_SCALE`
 * @gravity: the gravity to use when rendering
 * @dpi: the dpi used when rendering
 * @matrix: (nullable): transformation matrix to use when rendering
 *
 * Creates a new `PangoUserFont`.
 *
 * Returns: a newly created `PangoUserFont`
 */
PangoUserFont *
pango_user_font_new (PangoUserFace     *face,
                     int                size,
                     PangoGravity       gravity,
                     float              dpi,
                     const PangoMatrix *matrix)
{
  PangoUserFont *self;
  PangoFont *font;

  self = g_object_new (PANGO_TYPE_USER_FONT, NULL);

  font = PANGO_FONT (self);

  pango_font_set_face (font, PANGO_FONT_FACE (face));
  pango_font_set_size (font, size);
  pango_font_set_dpi (font, dpi);
  pango_font_set_gravity (font, gravity);
  if (matrix)
    pango_font_set_matrix (font, matrix);

  return self;
}

/**
 * pango_user_font_new_for_description:
 * @face: the `PangoUserFace` to use
 * @description: a `PangoFontDescription`
 * @dpi: the dpi used when rendering
 * @matrix: (nullable): transformation matrix to use when rendering
 *
 * Creates a new `PangoUserFont` with size and gravity taken
 * from a font description.
 *
 * Returns: a newly created `PangoHbFont`
 */

PangoUserFont *
pango_user_font_new_for_description (PangoUserFace              *face,
                                     const PangoFontDescription *description,
                                     float                       dpi,
                                     const PangoMatrix          *matrix)
{
  int size;
  PangoGravity gravity;

  if (pango_font_description_get_size_is_absolute (description))
    size = pango_font_description_get_size (description) * 72. / dpi;
  else
    size = pango_font_description_get_size (description);

  if ((pango_font_description_get_set_fields (description) & PANGO_FONT_MASK_GRAVITY) != 0 &&
      pango_font_description_get_gravity (description) != PANGO_GRAVITY_SOUTH)
    gravity = pango_font_description_get_gravity (description);
  else
    gravity = PANGO_GRAVITY_AUTO;

  return pango_user_font_new (face, size, gravity, dpi, matrix);
}

/* }}} */

/* vim:set foldmethod=marker expandtab: */
