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

#include "pango-hbfont-private.h"

#include "pango-font-private.h"
#include "pango-font-metrics-private.h"
#include "pango-hbface-private.h"
#include "pango-hbfamily-private.h"
#include "pango-impl-utils.h"
#include "pango-language-set-private.h"

#include <hb-ot.h>

/**
 * PangoHbFont:
 *
 * `PangoHbFont` is a `PangoFont` implementation that wraps
 * a `hb_font_t` object and implements all of the `PangoFont`
 * functionality using HarfBuzz.
 *
 * In addition to a `PangoHbFace` and a size, a number of optional
 * parameters can be tweaked when creating a `PangoHbFont`. First
 * there are OpenType font features, which can be used to e.g.
 * select Small Caps. If the face has variation axes, then
 * coordinates for these axes can be provided. Finally, there are
 * rendering parameters such as the dpi and the global transformation
 * matrix.
 */

/* {{{ Utilities */

static int
get_average_char_width (PangoFont  *font,
                        const char *text)
{
  hb_font_t *hb_font = pango_font_get_hb_font (font);
  int width = 0;

  for (const char *p = text; *p; p = g_utf8_next_char (p))
    {
      gunichar wc;
      hb_codepoint_t glyph;
      PangoRectangle extents;

      wc = g_utf8_get_char (p);
      if (!hb_font_get_nominal_glyph (hb_font, wc, &glyph))
        continue;

      pango_font_get_glyph_extents (font, glyph, &extents, NULL);

      width += extents.x + extents.width;
    }

  return width / pango_utf8_strwidth (text);
}

static void
get_max_char_size (PangoFont  *font,
                   const char *text,
                   int        *width,
                   int        *height)
{
  hb_font_t *hb_font = pango_font_get_hb_font (font);
  int w = 0;
  int h = 0;

  for (const char *p = text; *p; p = g_utf8_next_char (p))
    {
      gunichar wc;
      hb_codepoint_t glyph;
      PangoRectangle extents;

      wc = g_utf8_get_char (p);
      if (!hb_font_get_nominal_glyph (hb_font, wc, &glyph))
        continue;

      pango_font_get_glyph_extents (font, glyph, &extents, NULL);

      w = MAX (w, extents.x + extents.width);
      h = MAX (h, extents.height);
    }

  if (width)
    *width = w;

  if (height)
    *height = h;
}

static PangoVariant
pango_variant_from_features (hb_feature_t *features,
                             unsigned int  n_features)
{
  PangoVariant variant = PANGO_VARIANT_NORMAL;
  gboolean all_caps = FALSE;

  for (int i = 0; i < n_features; i++)
    {
      if (features[i].value != 1)
        continue;

      switch (features[i].tag)
        {
        case HB_TAG('s','m','c','p'):
          if (all_caps)
            variant = PANGO_VARIANT_ALL_SMALL_CAPS;
          else
            variant = PANGO_VARIANT_SMALL_CAPS;
          break;
        case HB_TAG('c','2','s','c'):
          if (variant == PANGO_VARIANT_SMALL_CAPS)
            variant = PANGO_VARIANT_ALL_SMALL_CAPS;
          else
            all_caps = TRUE;
          break;
        case HB_TAG('p','c','a','p'):
          if (all_caps)
            variant = PANGO_VARIANT_ALL_PETITE_CAPS;
          else
            variant = PANGO_VARIANT_PETITE_CAPS;
          break;
        case HB_TAG('c','2','p','c'):
          if (variant == PANGO_VARIANT_PETITE_CAPS)
            variant = PANGO_VARIANT_ALL_PETITE_CAPS;
          else
            all_caps = TRUE;
          break;
        case HB_TAG('u','n','i','c'):
          variant = PANGO_VARIANT_UNICASE;
          break;
        case HB_TAG('t','i','t','l'):
          variant = PANGO_VARIANT_TITLE_CAPS;
          break;
        default:
          break;
        }
    }

  return variant;
}

static void
font_description_get_features (const PangoFontDescription *description,
                               hb_feature_t               *features,
                               unsigned int                length,
                               unsigned int               *n_features)
{

#define ADD_FEATURE(name) \
 features[*n_features].tag = HB_TAG (name[0], name[1], name[2], name[3]); \
 features[*n_features].value = 1; \
 features[*n_features].start = 0; \
 features[*n_features].end = (unsigned int) -1; \
 (*n_features)++

  g_assert (length >= 2);

  *n_features = 0;
  switch (pango_font_description_get_variant (description))
    {
    case PANGO_VARIANT_SMALL_CAPS:
      ADD_FEATURE ("smcp");
      break;
    case PANGO_VARIANT_ALL_SMALL_CAPS:
      ADD_FEATURE ("smcp");
      ADD_FEATURE ("c2sc");
      break;
    case PANGO_VARIANT_PETITE_CAPS:
      ADD_FEATURE ("pcap");
      break;
    case PANGO_VARIANT_ALL_PETITE_CAPS:
      ADD_FEATURE ("pcap");
      ADD_FEATURE ("c2pc");
      break;
    case PANGO_VARIANT_UNICASE:
      ADD_FEATURE ("unic");
      break;
    case PANGO_VARIANT_TITLE_CAPS:
      ADD_FEATURE ("titl");
      break;
    case PANGO_VARIANT_NORMAL:
      break;
    default:
      g_assert_not_reached ();
    }

#undef ADD_FEATURE
}

static unsigned int
count_variations (const char *string)
{
  unsigned int n;
  const char *p;

  n = 1;
  p = string;
  while ((p = strchr (p, ',')) != NULL)
    n++;

  return n;
}

static void
parse_variations (const char     *str,
                  hb_variation_t *variations,
                  unsigned int    length,
                  unsigned int   *n_variations)
{
  const char *p;

  *n_variations = 0;

  p = str;
  while (p && *p && *n_variations < length)
    {
      const char *end = strchr (p, ',');
      if (hb_variation_from_string (p, end ? end - p: -1, &variations[*n_variations]))
        (*n_variations)++;
      p = end ? end + 1 : NULL;
    }
}

static char *
variations_to_string (hb_variation_t *variations,
                      unsigned int    n_variations)
{
  GString *s;
  char buf[128];

  s = g_string_new ("");

  for (unsigned int i = 0; i < n_variations; i++)
    {
      hb_variation_to_string (&variations[i], buf, sizeof (buf));
      if (s->len > 0)
        g_string_append_c (s, ',');
      g_string_append (s, buf);
    }

  return g_string_free (s, FALSE);
}

static inline void
replace_variation (hb_variation_t       *values,
                   unsigned int         *len,
                   const hb_variation_t *v)
{
  for (int i = 0; i < *len; i++)
    {
      if (values[i].tag == v->tag)
        {
          values[i].value = v->value;
          return;
        }
    }

  values[(*len)++] = *v;
}

static unsigned int
merge_variations (const hb_variation_t *v1,
                  unsigned int          l1,
                  const hb_variation_t *v2,
                  unsigned int          l2,
                  hb_variation_t       *v,
                  unsigned int          l)
{
  unsigned int len = 0;

  for (int i = 0; i < l1; i++)
    replace_variation (v, &len, &v1[i]);

  for (int i = 0; i < l2; i++)
    replace_variation (v, &len, &v2[i]);

  return len;
}

static inline void
collect_variation (hb_variation_t        *variation,
                   unsigned int           n_axes,
                   hb_ot_var_axis_info_t *axes,
                   float                 *coords)
{
  for (int j = 0; j < n_axes; j++)
    {
      if (axes[j].tag == variation->tag)
        {
          coords[axes[j].axis_index] = variation->value;
          break;
        }
    }
}

static inline void
collect_variations (hb_variation_t        *variations,
                    unsigned int           n_variations,
                    unsigned int           n_axes,
                    hb_ot_var_axis_info_t *axes,
                    float                 *coords)
{
  for (int i = 0; i < n_variations; i++)
    collect_variation (&variations[i], n_axes, axes, coords);
}

/* }}} */
/* {{{ hex box sizing */

/* This code needs to stay in sync with the hexbox rendering code in pangocairo-render.c */
static HexBoxInfo *
create_hex_box_info (PangoHbFont *self)
{
  const char hexdigits[] = "0123456789ABCDEF";
  hb_font_t *hb_font;
  PangoFont *mini_font;
  HexBoxInfo *hbi;
  int rows;
  double pad;
  int width = 0;
  int height = 0;
  hb_font_extents_t font_extents;
  double font_ascent, font_descent;
  double mini_size;
  PangoFontDescription *desc;
  PangoContext *context;
  PangoFontMap *map;

  if (!PANGO_FONT_FACE (self->face)->family)
    return NULL;

  map = PANGO_FONT_FACE (self->face)->family->map;

  if (!map)
    return NULL;

  desc = pango_font_describe_with_absolute_size (PANGO_FONT (self));
  hb_font = pango_font_get_hb_font (PANGO_FONT (self));

  /* Create mini_font description */

  /* We inherit most font properties for the mini font.
   * Just change family and size, so you get bold
   * hex digits in the hexbox for a bold font.
   */

  /* We should rotate the box, not glyphs */
  pango_font_description_unset_fields (desc, PANGO_FONT_MASK_GRAVITY);

  pango_font_description_set_family_static (desc, "monospace");

  rows = 2;
  mini_size = self->size / 2.2;

  if (mini_size < 6.0)
    {
      rows = 1;
      mini_size = MIN (MAX (self->size - 1, 0), 6.0);
    }

  pango_font_description_set_size (desc, mini_size);

  /* Load mini_font */
  context = pango_font_map_create_context (map);
  pango_context_set_matrix (context, &self->matrix);
  pango_context_set_language (context, pango_script_get_sample_language (G_UNICODE_SCRIPT_LATIN));

  mini_font = pango_font_map_load_font (map, context, desc);

  g_object_unref (context);

  pango_font_description_free (desc);

  get_max_char_size (mini_font, hexdigits, &width, &height);

  hb_font_get_extents_for_direction (hb_font, HB_DIRECTION_LTR, &font_extents);
  font_ascent = font_extents.ascender / (double) PANGO_SCALE;
  font_descent = - font_extents.descender / (double) PANGO_SCALE;

  pad = (font_ascent + font_descent) / 43.;
  pad = MIN (pad, mini_size / (double) PANGO_SCALE);

  hbi = g_new (HexBoxInfo, 1);
  hbi->font = mini_font;
  hbi->rows = rows;

  hbi->digit_width  = width / (double) PANGO_SCALE;
  hbi->digit_height = height / (double) PANGO_SCALE;

  hbi->pad_x = pad;
  hbi->pad_y = pad;

  hbi->line_width = MIN (hbi->pad_x, hbi->pad_y);

  hbi->box_height = 3 * hbi->pad_y + rows * (hbi->pad_y + hbi->digit_height);

  if (rows == 1 || hbi->box_height <= font_ascent)
    hbi->box_descent = 2 * hbi->pad_y;
  else if (hbi->box_height <= font_ascent + font_descent - 2 * hbi->pad_y)
    hbi->box_descent = 2 * hbi->pad_y + hbi->box_height - font_ascent;
  else
    hbi->box_descent = font_descent * hbi->box_height / (font_ascent + font_descent);

  return hbi;
}

static void
get_space_extents (PangoHbFont    *self,
                   PangoRectangle *ink_rect,
                   PangoRectangle *logical_rect)
{
  hb_font_t *hb_font = pango_font_get_hb_font (PANGO_FONT (self));
  int width;
  hb_direction_t direction;
  hb_font_extents_t font_extents;

  direction = PANGO_GRAVITY_IS_VERTICAL (self->gravity)
              ? HB_DIRECTION_TTB
              : HB_DIRECTION_LTR;

  hb_font_get_extents_for_direction (hb_font, direction, &font_extents);

  /* See https://docs.microsoft.com/en-us/typography/develop/character-design-standards/whitespace */

  width = self->size / 4;

  if (ink_rect)
    {
      ink_rect->x = ink_rect->y = ink_rect->height = 0;
      ink_rect->width = width;
    }

  if (logical_rect)
    {
      logical_rect->x = 0;
      logical_rect->y = - font_extents.ascender;
      logical_rect->height = font_extents.ascender - font_extents.descender;
      logical_rect->width = width;
    }
}

static void
get_glyph_extents_missing (PangoHbFont    *self,
                           PangoGlyph      glyph,
                           PangoRectangle *ink_rect,
                           PangoRectangle *logical_rect)
{
  gunichar ch;
  int rows, cols;
  HexBoxInfo *hbi;

  ch = glyph & ~PANGO_GLYPH_UNKNOWN_FLAG;

  if (!self->hex_box_info)
    self->hex_box_info = create_hex_box_info (self);

  if (ch == 0x20 || ch == 0x2423)
    {
      get_space_extents (self, ink_rect, logical_rect);
      return;
    }

  hbi = self->hex_box_info;

  if (G_UNLIKELY (glyph == PANGO_GLYPH_INVALID_INPUT || ch > 0x10FFFF))
    {
      rows = hbi->rows;
      cols = 1;
    }
  else if (pango_get_ignorable_size (ch, &rows, &cols))
    {
      /* We special-case ignorables when rendering hex boxes */
    }
  else
    {
      rows = hbi->rows;
      cols = (ch > 0xffff ? 6 : 4) / rows;
    }

  if (ink_rect)
    {
      ink_rect->x = PANGO_SCALE * hbi->pad_x;
      ink_rect->y = PANGO_SCALE * (hbi->box_descent - hbi->box_height);
      ink_rect->width = PANGO_SCALE * (3 * hbi->pad_x + cols * (hbi->digit_width + hbi->pad_x));
      ink_rect->height = PANGO_SCALE * hbi->box_height;
    }

  if (logical_rect)
    {
      logical_rect->x = 0;
      logical_rect->y = PANGO_SCALE * (hbi->box_descent - (hbi->box_height + hbi->pad_y));
      logical_rect->width = PANGO_SCALE * (5 * hbi->pad_x + cols * (hbi->digit_width + hbi->pad_x));
      logical_rect->height = PANGO_SCALE * (hbi->box_height + 2 * hbi->pad_y);
    }
}

 /* }}} */
/* {{{ PangoFont implementation */

struct _PangoHbFontClass
{
  PangoFontClass parent_class;
};

G_DEFINE_TYPE (PangoHbFont, pango_hb_font, PANGO_TYPE_FONT)

static void
pango_hb_font_init (PangoHbFont *self)
{
  self->gravity = PANGO_GRAVITY_AUTO;
  self->matrix = (PangoMatrix) PANGO_MATRIX_INIT;
}

static void
hex_box_info_destroy (HexBoxInfo *hex_box_info)
{
  g_object_unref (hex_box_info->font);
  g_free (hex_box_info);
}

static void
pango_hb_font_finalize (GObject *object)
{
  PangoHbFont *self = PANGO_HB_FONT (object);

  g_object_unref (self->face);
  g_free (self->variations);
  g_clear_pointer (&self->hex_box_info, hex_box_info_destroy);

  G_OBJECT_CLASS (pango_hb_font_parent_class)->finalize (object);
}

static PangoFontDescription *
pango_hb_font_describe (PangoFont *font)
{
  PangoHbFont *self = PANGO_HB_FONT (font);
  PangoFontDescription *desc;
  PangoVariant variant;

  desc = pango_font_face_describe (PANGO_FONT_FACE (self->face));
  pango_font_description_set_gravity (desc, self->gravity);
  variant = pango_variant_from_features (self->features, self->n_features);
  if (variant != PANGO_VARIANT_NORMAL)
    pango_font_description_set_variant (desc, variant);
  if (self->n_variations > 0)
    {
      hb_variation_t *variations;
      unsigned int n_variations;
      char *str;

      if (self->face->n_variations > 0)
        {
           variations = g_alloca (sizeof (hb_variation_t) * (self->n_variations + self->face->n_variations));
           n_variations = merge_variations (self->face->variations, self->face->n_variations,
                                            self->variations, self->n_variations,
                                            variations, self->n_variations + self->face->n_variations);
        }
      else
        {
          variations = self->variations;
          n_variations = self->n_variations;
        }

      str = variations_to_string (variations, n_variations);
      pango_font_description_set_variations (desc, str);
      g_free (str);
    }
  pango_font_description_set_size (desc, self->size);

  return desc;
}

static PangoFontDescription *
pango_hb_font_describe_absolute (PangoFont *font)
{
  PangoHbFont *self = PANGO_HB_FONT (font);
  PangoFontDescription *desc;

  desc = pango_hb_font_describe (font);
  pango_font_description_set_absolute_size (desc, self->size * self->dpi / 72.);

  return desc;
}

static void
pango_hb_font_get_glyph_extents (PangoFont      *font,
                                 PangoGlyph      glyph,
                                 PangoRectangle *ink_rect,
                                 PangoRectangle *logical_rect)
{
  PangoHbFont *self = PANGO_HB_FONT (font);
  hb_font_t *hb_font = pango_font_get_hb_font (font);
  hb_glyph_extents_t extents;
  hb_direction_t direction;
  hb_font_extents_t font_extents;

  direction = PANGO_GRAVITY_IS_VERTICAL (self->gravity)
              ? HB_DIRECTION_TTB
              : HB_DIRECTION_LTR;

  hb_font_get_extents_for_direction (hb_font, direction, &font_extents);

  if (glyph == PANGO_GLYPH_EMPTY)
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
  else if (glyph & PANGO_GLYPH_UNKNOWN_FLAG)
    {
      get_glyph_extents_missing (self, glyph, ink_rect, logical_rect);

      return;
    }

  hb_font_get_glyph_extents (hb_font, glyph, &extents);

  if (ink_rect)
    {
      PangoRectangle r;
      PangoMatrix m = PANGO_MATRIX_INIT;

      r.x = extents.x_bearing;
      r.y = - extents.y_bearing;
      r.width = extents.width;
      r.height = - extents.height;

      if (self->face->matrix)
        {
          m.xx = self->face->matrix->xx;
          m.yx = - self->face->matrix->yx;
          m.xy = - self->face->matrix->xy;
          m.yy = self->face->matrix->yy;
        }

      pango_matrix_transform_rectangle (&m, &r);

      switch (self->gravity)
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

      if (PANGO_GRAVITY_IS_IMPROPER (self->gravity))
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
      logical_rect->height = extents.ascender - extents.descender;

      switch (self->gravity)
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

      if (PANGO_GRAVITY_IS_IMPROPER (self->gravity))
        {
          logical_rect->height = - logical_rect->height;
          logical_rect->y = - logical_rect->y;
        }
    }
}

static PangoFontMetrics *
pango_hb_font_get_metrics (PangoFont     *font,
                           PangoLanguage *language)
{
  PangoHbFont *self = PANGO_HB_FONT (font);
  hb_font_t *hb_font = pango_font_get_hb_font (font);
  PangoFontMetrics *metrics;
  hb_font_extents_t extents;
  hb_position_t position;

  metrics = pango_font_metrics_new ();

  hb_font_get_extents_for_direction (hb_font, HB_DIRECTION_LTR, &extents);

  metrics->descent = - extents.descender;
  metrics->ascent = extents.ascender;
  metrics->height = extents.ascender - extents.descender + extents.line_gap;

  if (hb_ot_metrics_get_position (hb_font, HB_OT_METRICS_TAG_UNDERLINE_SIZE, &position) && position != 0)
    metrics->underline_thickness = position;
  else
    metrics->underline_thickness = PANGO_SCALE;

  if (hb_ot_metrics_get_position (hb_font, HB_OT_METRICS_TAG_UNDERLINE_OFFSET, &position) && position != 0)
    metrics->underline_position = position;
  else
    metrics->underline_position = - PANGO_SCALE;

  if (hb_ot_metrics_get_position (hb_font, HB_OT_METRICS_TAG_STRIKEOUT_SIZE, &position) && position != 0)
    metrics->strikethrough_thickness = position;
  else
    metrics->strikethrough_thickness = PANGO_SCALE;

  if (hb_ot_metrics_get_position (hb_font, HB_OT_METRICS_TAG_STRIKEOUT_OFFSET, &position) && position != 0)
    metrics->strikethrough_position = position;
  else
    metrics->strikethrough_position = metrics->ascent / 2;

  if (self->approximate_char_width == 0 || self->approximate_char_lang != language)
    {
      self->approximate_char_width = get_average_char_width (font, pango_language_get_sample_string (language));
      self->approximate_char_lang = language;
    }

  if (self->approximate_digit_width == 0)
    get_max_char_size (font, "0123456789", &self->approximate_digit_width, NULL);

  metrics->approximate_char_width = self->approximate_char_width;
  metrics->approximate_digit_width = self->approximate_digit_width;

  return metrics;
}

static hb_font_t *
pango_hb_font_create_hb_font (PangoFont *font)
{
  PangoHbFont *self = PANGO_HB_FONT (font);
  hb_font_t *hb_font;
  double x_scale, y_scale;
  unsigned int n_axes;
  hb_ot_var_axis_info_t *axes;
  float *coords;
  int size;

  hb_font = hb_font_create (pango_hb_face_get_hb_face (self->face));

  size = self->size * self->dpi / 72.f;
  x_scale = self->face->x_scale;
  y_scale = self->face->y_scale;

  if (PANGO_GRAVITY_IS_IMPROPER (self->gravity))
    {
      x_scale = - x_scale;
      y_scale = - y_scale;
    }

  hb_font_set_scale (hb_font, size * x_scale, size * y_scale);
  hb_font_set_ptem (hb_font, self->size / PANGO_SCALE);

#if HB_VERSION_ATLEAST (3, 3, 0)
  hb_font_set_synthetic_slant (hb_font, pango_matrix_get_slant_ratio (self->face->matrix));
#endif

  if (self->face->instance_id >= 0)
    hb_font_set_var_named_instance (hb_font, self->face->instance_id);

  if (self->n_variations > 0 || self->face->n_variations > 0)
    {
      n_axes = hb_ot_var_get_axis_count (self->face->face);
      axes = g_alloca (sizeof (hb_ot_var_axis_info_t) * n_axes);
      coords = g_alloca (sizeof (float) * n_axes);

      hb_ot_var_get_axis_infos (self->face->face, 0, &n_axes, axes);

      if (self->face->instance_id >= 0)
        hb_ot_var_named_instance_get_design_coords (self->face->face, self->face->instance_id, &n_axes, coords);
      else
        {
          for (int i = 0; i < n_axes; i++)
            coords[axes[i].axis_index] = axes[i].default_value;
        }

      collect_variations (self->face->variations, self->face->n_variations, n_axes, axes, coords);
      collect_variations (self->variations, self->n_variations, n_axes, axes, coords);

      hb_font_set_var_coords_design (hb_font, coords, n_axes);
    }

  return hb_font;
}

static gboolean
pango_hb_font_has_char (PangoFont *font,
                        gunichar   wc)
{
  hb_font_t *hb_font = pango_font_get_hb_font (font);
  hb_codepoint_t glyph;

  return hb_font_get_nominal_glyph (hb_font, wc, &glyph);
}

static PangoFontFace *
pango_hb_font_get_face (PangoFont *font)
{
  PangoHbFont *self = PANGO_HB_FONT (font);

  return PANGO_FONT_FACE (self->face);
}

static void
pango_hb_font_get_matrix (PangoFont   *font,
                          PangoMatrix *matrix)
{
  PangoHbFont *self = PANGO_HB_FONT (font);

  if (self->face->matrix)
    {
      *matrix = *self->face->matrix;
      pango_matrix_scale (matrix, self->face->x_scale, self->face->y_scale);
    }
  else
    *matrix = (PangoMatrix) PANGO_MATRIX_INIT;
}

static void
pango_hb_font_get_features (PangoFont    *font,
                            hb_feature_t *features,
                            guint         len,
                            guint        *num_features)
{
  PangoHbFont *self = PANGO_HB_FONT (font);

  *num_features = MIN (len, self->n_features);
  memcpy (features, self->features, sizeof (hb_feature_t) * *num_features);
}

static void
pango_hb_font_class_init (PangoHbFontClass *class)
{
  GObjectClass *object_class = G_OBJECT_CLASS (class);
  PangoFontClass *font_class = PANGO_FONT_CLASS (class);

  object_class->finalize = pango_hb_font_finalize;

  font_class->describe = pango_hb_font_describe;
  font_class->describe_absolute = pango_hb_font_describe_absolute;
  font_class->get_glyph_extents = pango_hb_font_get_glyph_extents;
  font_class->get_metrics = pango_hb_font_get_metrics;
  font_class->create_hb_font = pango_hb_font_create_hb_font;
  font_class->get_features = pango_hb_font_get_features;
  font_class->has_char = pango_hb_font_has_char;
  font_class->get_face = pango_hb_font_get_face;
  font_class->get_matrix = pango_hb_font_get_matrix;
}

/* }}} */
 /* {{{ Public API */

/**
 * pango_hb_font_new:
 * @face: the `PangoHbFace` to use
 * @size: the desired size in points, scaled by `PANGO_SCALE`
 * @features: (nullable) (array length=n_features): OpenType font features to use for this font
 * @n_features: length of @features
 * @variations: (nullable) (array length=n_variations): font variations to apply
 * @n_variations: length of @variations
 * @gravity: the gravity to use when rendering
 * @dpi: the dpi used when rendering
 * @matrix: (nullable): transformation matrix to use when rendering
 *
 * Creates a new `PangoHbFont`.
 *
 * Returns: a newly created `PangoHbFont`
 */
PangoHbFont *
pango_hb_font_new (PangoHbFace       *face,
                   int                size,
                   hb_feature_t      *features,
                   unsigned int       n_features,
                   hb_variation_t    *variations,
                   unsigned int       n_variations,
                   PangoGravity       gravity,
                   float              dpi,
                   const PangoMatrix *matrix)
{
  PangoHbFont *self;

  g_return_val_if_fail (PANGO_IS_HB_FACE (face), NULL);
  g_return_val_if_fail (size > 0, NULL);
  g_return_val_if_fail (dpi > 0, NULL);

  self = g_object_new (PANGO_TYPE_HB_FONT, NULL);

  self->face = g_object_ref (face);

  self->size = size;
  self->dpi = dpi;
  self->features = g_memdup2 (features, sizeof (hb_feature_t) * n_features);
  self->n_features = n_features;
  self->variations = g_memdup2 (variations, sizeof (hb_variation_t) * n_variations);
  self->n_variations = n_variations;
  if (gravity != PANGO_GRAVITY_AUTO)
    self->gravity = gravity;
  if (matrix)
    self->matrix = *matrix;

  return self;
}

/**
 * pango_hb_font_new_for_description:
 * @face: the `PangoHbFace` to use
 * @description: a `PangoFontDescription`
 * @dpi: the dpi used when rendering
 * @matrix: (nullable): transformation matrix to use when rendering
 *
 * Creates a new `PangoHbFont` with size, features, variations and
 * gravity taken from a font description.
 *
 * Returns: a newly created `PangoHbFont`
 */
PangoHbFont *
pango_hb_font_new_for_description (PangoHbFace                *face,
                                   const PangoFontDescription *description,
                                   float                       dpi,
                                   const PangoMatrix          *matrix)
{
  int size;
  hb_feature_t features[10];
  unsigned int n_features;
  hb_variation_t *variations;
  unsigned int n_variations;
  unsigned int length;
  PangoGravity gravity;
  const char *str;

  g_return_val_if_fail (PANGO_IS_HB_FACE (face), NULL);
  g_return_val_if_fail (description != NULL, NULL);
  g_return_val_if_fail ((pango_font_description_get_set_fields (description) & PANGO_FONT_MASK_SIZE) != 0, NULL);
  g_return_val_if_fail (dpi > 0, NULL);

  if (pango_font_description_get_size_is_absolute (description))
    size = pango_font_description_get_size (description) * 72. / dpi;
  else
    size = pango_font_description_get_size (description);

  g_return_val_if_fail (size > 0, NULL);

  font_description_get_features (description, features, G_N_ELEMENTS (features), &n_features);

  if ((pango_font_description_get_set_fields (description) & PANGO_FONT_MASK_VARIATIONS) != 0)
    {
      str = pango_font_description_get_variations (description);
      length = count_variations (str);
      variations = g_alloca (sizeof (hb_variation_t) * length);
      parse_variations (str, variations, length, &n_variations);
    }
  else
    {
      variations = NULL;
      n_variations = 0;
    }

  if ((pango_font_description_get_set_fields (description) & PANGO_FONT_MASK_GRAVITY) != 0 &&
      pango_font_description_get_gravity (description) != PANGO_GRAVITY_SOUTH)
    gravity = pango_font_description_get_gravity (description);
  else
    gravity = PANGO_GRAVITY_AUTO;

  return pango_hb_font_new (face, size, features, n_features, variations, n_variations, gravity, dpi, matrix);
}

/* }}} */

/* vim:set foldmethod=marker expandtab: */
