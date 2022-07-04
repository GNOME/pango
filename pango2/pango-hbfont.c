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

#include "pango-hbfont-private.h"

#include "pango-font-private.h"
#include "pango-font-metrics-private.h"
#include "pango-hbface-private.h"
#include "pango-hbfamily-private.h"
#include "pango-impl-utils.h"
#include "pango-language-set-private.h"

#include <hb-ot.h>

/**
 * Pango2HbFont:
 *
 * `Pango2HbFont` is a `Pango2Font` implementation that wraps
 * a `hb_font_t` object and implements all of the `Pango2Font`
 * functionality using HarfBuzz.
 *
 * In addition to a `Pango2HbFace` and a size, a number of optional
 * parameters can be tweaked when creating a `Pango2HbFont`. First
 * there are OpenType font features, which can be used to e.g.
 * select Small Caps. If the face has variation axes, then
 * coordinates for these axes can be provided. Finally, there are
 * rendering parameters such as the dpi and the global transformation
 * matrix.
 */

/* {{{ Utilities */

static int
get_average_char_width (Pango2Font  *font,
                        const char *text)
{
  hb_font_t *hb_font = pango2_font_get_hb_font (font);
  int width = 0;

  for (const char *p = text; *p; p = g_utf8_next_char (p))
    {
      gunichar wc;
      hb_codepoint_t glyph;
      Pango2Rectangle extents;

      wc = g_utf8_get_char (p);
      if (!hb_font_get_nominal_glyph (hb_font, wc, &glyph))
        continue;

      pango2_font_get_glyph_extents (font, glyph, &extents, NULL);

      width += extents.x + extents.width;
    }

  return width / pango2_utf8_strwidth (text);
}

static void
get_max_char_size (Pango2Font *font,
                   const char *text,
                   int        *width,
                   int        *height)
{
  hb_font_t *hb_font = pango2_font_get_hb_font (font);
  int w = 0;
  int h = 0;

  for (const char *p = text; *p; p = g_utf8_next_char (p))
    {
      gunichar wc;
      hb_codepoint_t glyph;
      Pango2Rectangle extents;

      wc = g_utf8_get_char (p);
      if (!hb_font_get_nominal_glyph (hb_font, wc, &glyph))
        continue;

      pango2_font_get_glyph_extents (font, glyph, &extents, NULL);

      w = MAX (w, extents.x + extents.width);
      h = MAX (h, extents.height);
    }

  if (width)
    *width = w;

  if (height)
    *height = h;
}

static Pango2Variant
pango2_variant_from_features (hb_feature_t *features,
                              unsigned int  n_features)
{
  Pango2Variant variant = PANGO2_VARIANT_NORMAL;
  gboolean all_caps = FALSE;

  for (int i = 0; i < n_features; i++)
    {
      if (features[i].value != 1)
        continue;

      switch (features[i].tag)
        {
        case HB_TAG('s','m','c','p'):
          if (all_caps)
            variant = PANGO2_VARIANT_ALL_SMALL_CAPS;
          else
            variant = PANGO2_VARIANT_SMALL_CAPS;
          break;
        case HB_TAG('c','2','s','c'):
          if (variant == PANGO2_VARIANT_SMALL_CAPS)
            variant = PANGO2_VARIANT_ALL_SMALL_CAPS;
          else
            all_caps = TRUE;
          break;
        case HB_TAG('p','c','a','p'):
          if (all_caps)
            variant = PANGO2_VARIANT_ALL_PETITE_CAPS;
          else
            variant = PANGO2_VARIANT_PETITE_CAPS;
          break;
        case HB_TAG('c','2','p','c'):
          if (variant == PANGO2_VARIANT_PETITE_CAPS)
            variant = PANGO2_VARIANT_ALL_PETITE_CAPS;
          else
            all_caps = TRUE;
          break;
        case HB_TAG('u','n','i','c'):
          variant = PANGO2_VARIANT_UNICASE;
          break;
        case HB_TAG('t','i','t','l'):
          variant = PANGO2_VARIANT_TITLE_CAPS;
          break;
        default:
          break;
        }
    }

  return variant;
}

static void
font_description_get_features (const Pango2FontDescription *description,
                               hb_feature_t                *features,
                               unsigned int                 length,
                               unsigned int                *n_features)
{

#define ADD_FEATURE(name) \
 features[*n_features].tag = HB_TAG (name[0], name[1], name[2], name[3]); \
 features[*n_features].value = 1; \
 features[*n_features].start = 0; \
 features[*n_features].end = (unsigned int) -1; \
 (*n_features)++

  g_assert (length >= 2);

  *n_features = 0;
  switch (pango2_font_description_get_variant (description))
    {
    case PANGO2_VARIANT_SMALL_CAPS:
      ADD_FEATURE ("smcp");
      break;
    case PANGO2_VARIANT_ALL_SMALL_CAPS:
      ADD_FEATURE ("smcp");
      ADD_FEATURE ("c2sc");
      break;
    case PANGO2_VARIANT_PETITE_CAPS:
      ADD_FEATURE ("pcap");
      break;
    case PANGO2_VARIANT_ALL_PETITE_CAPS:
      ADD_FEATURE ("pcap");
      ADD_FEATURE ("c2pc");
      break;
    case PANGO2_VARIANT_UNICASE:
      ADD_FEATURE ("unic");
      break;
    case PANGO2_VARIANT_TITLE_CAPS:
      ADD_FEATURE ("titl");
      break;
    case PANGO2_VARIANT_NORMAL:
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
create_hex_box_info (Pango2HbFont *self)
{
  Pango2Font *font = PANGO2_FONT (self);
  const char hexdigits[] = "0123456789ABCDEF";
  hb_font_t *hb_font;
  Pango2Font *mini_font;
  HexBoxInfo *hbi;
  int rows;
  double pad;
  int width = 0;
  int height = 0;
  hb_font_extents_t font_extents;
  double font_ascent, font_descent;
  double mini_size;
  Pango2FontDescription *desc;
  Pango2Context *context;
  Pango2FontMap *map;

  if (!PANGO2_FONT_FACE (font->face)->family)
    return NULL;

  map = PANGO2_FONT_FACE (font->face)->family->map;

  if (!map)
    return NULL;

  desc = pango2_font_describe_with_absolute_size (font);
  hb_font = pango2_font_get_hb_font (font);

  /* Create mini_font description */

  /* We inherit most font properties for the mini font.
   * Just change family and size, so you get bold
   * hex digits in the hexbox for a bold font.
   */

  /* We should rotate the box, not glyphs */
  pango2_font_description_unset_fields (desc, PANGO2_FONT_MASK_GRAVITY);

  pango2_font_description_set_family_static (desc, "monospace");

  rows = 2;
  mini_size = font->size / 2.2;

  if (mini_size < 6.0)
    {
      rows = 1;
      mini_size = MIN (MAX (font->size - 1, 0), 6.0);
    }

  pango2_font_description_set_size (desc, mini_size);

  /* Load mini_font */
  context = pango2_context_new_with_font_map (map);
  pango2_context_set_matrix (context, &font->ctm);
  pango2_context_set_language (context, pango2_script_get_sample_language (G_UNICODE_SCRIPT_LATIN));

  mini_font = pango2_font_map_load_font (map, context, desc);

  g_object_unref (context);

  pango2_font_description_free (desc);

  get_max_char_size (mini_font, hexdigits, &width, &height);

  hb_font_get_extents_for_direction (hb_font, HB_DIRECTION_LTR, &font_extents);
  font_ascent = font_extents.ascender / (double) PANGO2_SCALE;
  font_descent = - font_extents.descender / (double) PANGO2_SCALE;

  pad = (font_ascent + font_descent) / 43.;
  pad = MIN (pad, mini_size / (double) PANGO2_SCALE);

  hbi = g_new (HexBoxInfo, 1);
  hbi->font = mini_font;
  hbi->rows = rows;

  hbi->digit_width  = width / (double) PANGO2_SCALE;
  hbi->digit_height = height / (double) PANGO2_SCALE;

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
get_space_extents (Pango2HbFont    *self,
                   Pango2Rectangle *ink_rect,
                   Pango2Rectangle *logical_rect)
{
  Pango2Font *font = PANGO2_FONT (self);
  hb_font_t *hb_font = pango2_font_get_hb_font (font);
  int width;
  hb_direction_t direction;
  hb_font_extents_t font_extents;

  direction = PANGO2_GRAVITY_IS_VERTICAL (font->gravity)
              ? HB_DIRECTION_TTB
              : HB_DIRECTION_LTR;

  hb_font_get_extents_for_direction (hb_font, direction, &font_extents);

  /* See https://docs.microsoft.com/en-us/typography/develop/character-design-standards/whitespace */

  width = font->size / 4;

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
get_glyph_extents_missing (Pango2HbFont    *self,
                           Pango2Glyph      glyph,
                           Pango2Rectangle *ink_rect,
                           Pango2Rectangle *logical_rect)
{
  gunichar ch;
  int rows, cols;
  HexBoxInfo *hbi;

  ch = glyph & ~PANGO2_GLYPH_UNKNOWN_FLAG;

  if (!self->hex_box_info)
    self->hex_box_info = create_hex_box_info (self);

  if (ch == 0x20 || ch == 0x2423)
    {
      get_space_extents (self, ink_rect, logical_rect);
      return;
    }

  hbi = self->hex_box_info;

  if (G_UNLIKELY (glyph == PANGO2_GLYPH_INVALID_INPUT || ch > 0x10FFFF))
    {
      rows = hbi->rows;
      cols = 1;
    }
  else if (pango2_get_ignorable_size (ch, &rows, &cols))
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
      ink_rect->x = PANGO2_SCALE * hbi->pad_x;
      ink_rect->y = PANGO2_SCALE * (hbi->box_descent - hbi->box_height);
      ink_rect->width = PANGO2_SCALE * (3 * hbi->pad_x + cols * (hbi->digit_width + hbi->pad_x));
      ink_rect->height = PANGO2_SCALE * hbi->box_height;
    }

  if (logical_rect)
    {
      logical_rect->x = 0;
      logical_rect->y = PANGO2_SCALE * (hbi->box_descent - (hbi->box_height + hbi->pad_y));
      logical_rect->width = PANGO2_SCALE * (5 * hbi->pad_x + cols * (hbi->digit_width + hbi->pad_x));
      logical_rect->height = PANGO2_SCALE * (hbi->box_height + 2 * hbi->pad_y);
    }
}

 /* }}} */
/* {{{ Pango2Font implementation */

struct _Pango2HbFontClass
{
  Pango2FontClass parent_class;
};

enum {
  PROP_VARIATIONS = 1,
  N_PROPERTIES
};

static GParamSpec *properties[N_PROPERTIES] = { NULL, };

G_DEFINE_FINAL_TYPE (Pango2HbFont, pango2_hb_font, PANGO2_TYPE_FONT)

static void
pango2_hb_font_init (Pango2HbFont *self)
{
}

static void
hex_box_info_destroy (HexBoxInfo *hex_box_info)
{
  g_object_unref (hex_box_info->font);
  g_free (hex_box_info);
}

static void
pango2_hb_font_finalize (GObject *object)
{
  Pango2HbFont *self = PANGO2_HB_FONT (object);

  g_free (self->variations);
  g_clear_pointer (&self->hex_box_info, hex_box_info_destroy);

  G_OBJECT_CLASS (pango2_hb_font_parent_class)->finalize (object);
}


static void
pango2_hb_font_get_property (GObject    *object,
                             guint       property_id,
                             GValue     *value,
                             GParamSpec *pspec)
{
  Pango2HbFont *self = PANGO2_HB_FONT (object);

  switch (property_id)
    {
    case PROP_VARIATIONS:
      {
        char *str = NULL;
        if (self->variations)
          str = variations_to_string (self->variations, self->n_variations);
        g_value_take_string (value, str);
      }
      break;

    default:
      G_OBJECT_WARN_INVALID_PROPERTY_ID (object, property_id, pspec);
    }
}

static Pango2FontDescription *
pango2_hb_font_describe (Pango2Font *font)
{
  Pango2HbFont *self = PANGO2_HB_FONT (font);
  Pango2HbFace *face = PANGO2_HB_FACE (font->face);
  Pango2FontDescription *desc;
  Pango2Variant variant;

  desc = pango2_font_face_describe (font->face);
  pango2_font_description_set_gravity (desc, font->gravity);
  pango2_font_description_set_size (desc, font->size);

  variant = pango2_variant_from_features (self->features, self->n_features);
  if (variant != PANGO2_VARIANT_NORMAL)
    pango2_font_description_set_variant (desc, variant);
  if (self->n_variations > 0 || face->n_variations > 0)
    {
      hb_variation_t *variations;
      unsigned int n_variations;
      char *str;

      if (face->n_variations > 0)
        {
           variations = g_alloca (sizeof (hb_variation_t) * (self->n_variations + face->n_variations));
           n_variations = merge_variations (face->variations, face->n_variations,
                                            self->variations, self->n_variations,
                                            variations, self->n_variations + face->n_variations);
        }
      else
        {
          variations = self->variations;
          n_variations = self->n_variations;
        }

      str = variations_to_string (variations, n_variations);
      pango2_font_description_set_variations (desc, str);
      g_free (str);
    }

  return desc;
}

static void
pango2_hb_font_get_glyph_extents (Pango2Font      *font,
                                  Pango2Glyph      glyph,
                                  Pango2Rectangle *ink_rect,
                                  Pango2Rectangle *logical_rect)
{
  Pango2HbFont *self = PANGO2_HB_FONT (font);
  Pango2HbFace *face = PANGO2_HB_FACE (font->face);
  hb_font_t *hb_font = pango2_font_get_hb_font (font);
  hb_glyph_extents_t extents;
  hb_direction_t direction;
  hb_font_extents_t font_extents;

  direction = PANGO2_GRAVITY_IS_VERTICAL (font->gravity)
              ? HB_DIRECTION_TTB
              : HB_DIRECTION_LTR;

  hb_font_get_extents_for_direction (hb_font, direction, &font_extents);

  if (glyph == PANGO2_GLYPH_EMPTY)
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
  else if (glyph & PANGO2_GLYPH_UNKNOWN_FLAG)
    {
      get_glyph_extents_missing (self, glyph, ink_rect, logical_rect);

      return;
    }

  hb_font_get_glyph_extents (hb_font, glyph, &extents);

  if (ink_rect)
    {
      Pango2Rectangle r;
      Pango2Matrix m = PANGO2_MATRIX_INIT;

      r.x = extents.x_bearing;
      r.y = - extents.y_bearing;
      r.width = extents.width;
      r.height = - extents.height;

      if (face->transform)
        {
          m.xx = face->transform->xx;
          m.yx = - face->transform->yx;
          m.xy = - face->transform->xy;
          m.yy = face->transform->yy;

          pango2_matrix_transform_rectangle (&m, &r);
        }

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
pango2_hb_font_get_metrics (Pango2Font     *font,
                            Pango2Language *language)
{
  Pango2HbFont *self = PANGO2_HB_FONT (font);
  hb_font_t *hb_font = pango2_font_get_hb_font (font);
  Pango2FontMetrics *metrics;
  hb_font_extents_t extents;
  hb_position_t position;

  metrics = pango2_font_metrics_new ();

  hb_font_get_extents_for_direction (hb_font, HB_DIRECTION_LTR, &extents);

  metrics->descent = - extents.descender;
  metrics->ascent = extents.ascender;
  metrics->height = extents.ascender - extents.descender + extents.line_gap;

  if (hb_ot_metrics_get_position (hb_font, HB_OT_METRICS_TAG_UNDERLINE_SIZE, &position) && position != 0)
    metrics->underline_thickness = position;
  else
    metrics->underline_thickness = PANGO2_SCALE;

  if (hb_ot_metrics_get_position (hb_font, HB_OT_METRICS_TAG_UNDERLINE_OFFSET, &position) && position != 0)
    metrics->underline_position = position;
  else
    metrics->underline_position = - PANGO2_SCALE;

  if (hb_ot_metrics_get_position (hb_font, HB_OT_METRICS_TAG_STRIKEOUT_SIZE, &position) && position != 0)
    metrics->strikethrough_thickness = position;
  else
    metrics->strikethrough_thickness = PANGO2_SCALE;

  if (hb_ot_metrics_get_position (hb_font, HB_OT_METRICS_TAG_STRIKEOUT_OFFSET, &position) && position != 0)
    metrics->strikethrough_position = position;
  else
    metrics->strikethrough_position = metrics->ascent / 2;

  if (self->approximate_char_width == 0 || self->approximate_char_lang != language)
    {
      self->approximate_char_width = get_average_char_width (font, pango2_language_get_sample_string (language));
      self->approximate_char_lang = language;
    }

  if (self->approximate_digit_width == 0)
    get_max_char_size (font, "0123456789", &self->approximate_digit_width, NULL);

  metrics->approximate_char_width = self->approximate_char_width;
  metrics->approximate_digit_width = self->approximate_digit_width;

  return metrics;
}

static hb_font_t *
pango2_hb_font_create_hb_font (Pango2Font *font)
{
  Pango2HbFont *self = PANGO2_HB_FONT (font);
  Pango2HbFace *face = PANGO2_HB_FACE (font->face);
  hb_font_t *hb_font;
  double x_scale, y_scale;
  unsigned int n_axes;
  hb_ot_var_axis_info_t *axes;
  float *coords;
  int size;

  hb_font = hb_font_create (pango2_hb_face_get_hb_face (face));

  size = font->size * font->dpi / 72.f;
  x_scale = face->x_scale;
  y_scale = face->y_scale;

  if (PANGO2_GRAVITY_IS_IMPROPER (font->gravity))
    {
      x_scale = - x_scale;
      y_scale = - y_scale;
    }

  hb_font_set_scale (hb_font, size * x_scale, size * y_scale);
  hb_font_set_ptem (hb_font, font->size / PANGO2_SCALE);

#if HB_VERSION_ATLEAST (3, 3, 0)
  hb_font_set_synthetic_slant (hb_font, pango2_matrix_get_slant_ratio (face->transform));
#endif

  if (face->instance_id >= 0)
    hb_font_set_var_named_instance (hb_font, face->instance_id);

  if (self->n_variations > 0 || face->n_variations > 0)
    {
      n_axes = hb_ot_var_get_axis_count (face->face);
      axes = g_alloca (sizeof (hb_ot_var_axis_info_t) * n_axes);
      coords = g_alloca (sizeof (float) * n_axes);

      hb_ot_var_get_axis_infos (face->face, 0, &n_axes, axes);

      if (face->instance_id >= 0)
        hb_ot_var_named_instance_get_design_coords (face->face, face->instance_id, &n_axes, coords);
      else
        {
          for (int i = 0; i < n_axes; i++)
            coords[axes[i].axis_index] = axes[i].default_value;
        }

      collect_variations (face->variations, face->n_variations, n_axes, axes, coords);
      collect_variations (self->variations, self->n_variations, n_axes, axes, coords);

      hb_font_set_var_coords_design (hb_font, coords, n_axes);
    }

  return hb_font;
}

static void
pango2_hb_font_get_transform (Pango2Font   *font,
                              Pango2Matrix *matrix)
{
  Pango2HbFace *face = PANGO2_HB_FACE (font->face);

  if (face->transform)
    {
      *matrix = *face->transform;
      pango2_matrix_scale (matrix, face->x_scale, face->y_scale);
    }
  else
    *matrix = (Pango2Matrix) PANGO2_MATRIX_INIT;
}

static void
pango2_hb_font_class_init (Pango2HbFontClass *class)
{
  GObjectClass *object_class = G_OBJECT_CLASS (class);
  Pango2FontClass *font_class = PANGO2_FONT_CLASS (class);

  object_class->finalize = pango2_hb_font_finalize;
  object_class->get_property = pango2_hb_font_get_property;

  font_class->describe = pango2_hb_font_describe;
  font_class->get_glyph_extents = pango2_hb_font_get_glyph_extents;
  font_class->get_metrics = pango2_hb_font_get_metrics;
  font_class->create_hb_font = pango2_hb_font_create_hb_font;
  font_class->get_transform = pango2_hb_font_get_transform;

  /**
   * Pango2HbFont:variations: (attributes org.gtk.Property.get=pango2_hb_font_get_variations)
   *
   * The variations that are applied for this font.
   *
   * This property contains a string representation of the variations.
   */
  properties[PROP_VARIATIONS] =
      g_param_spec_string ("variations", NULL, NULL, NULL,
                           G_PARAM_READABLE | G_PARAM_STATIC_STRINGS);

  g_object_class_install_properties (object_class, N_PROPERTIES, properties);
}

/* }}} */
 /* {{{ Public API */

/**
 * pango2_hb_font_new:
 * @face: the `Pango2HbFace` to use
 * @size: the desired size in points, scaled by `PANGO2_SCALE`
 * @features: (nullable) (array length=n_features): OpenType font features to use for this font
 * @n_features: length of @features
 * @variations: (nullable) (array length=n_variations): font variations to apply
 * @n_variations: length of @variations
 * @gravity: the gravity to use when rendering
 * @dpi: the dpi used when rendering
 * @ctm: (nullable): transformation matrix to use when rendering
 *
 * Creates a new `Pango2HbFont`.
 *
 * Returns: a newly created `Pango2HbFont`
 */
Pango2HbFont *
pango2_hb_font_new (Pango2HbFace       *face,
                    int                 size,
                    hb_feature_t       *features,
                    unsigned int        n_features,
                    hb_variation_t     *variations,
                    unsigned int        n_variations,
                    Pango2Gravity       gravity,
                    float               dpi,
                    const Pango2Matrix *ctm)
{
  Pango2HbFont *self;
  Pango2Font *font;

  g_return_val_if_fail (PANGO2_IS_HB_FACE (face), NULL);
  g_return_val_if_fail (size > 0, NULL);
  g_return_val_if_fail (dpi > 0, NULL);

  self = g_object_new (PANGO2_TYPE_HB_FONT, NULL);

  font = PANGO2_FONT (self);

  pango2_font_set_face (font, PANGO2_FONT_FACE (face));
  pango2_font_set_size (font, size);
  pango2_font_set_dpi (font, dpi);
  pango2_font_set_gravity (font, gravity);
  pango2_font_set_ctm (font, ctm);

  self->features = g_memdup2 (features, sizeof (hb_feature_t) * n_features);
  self->n_features = n_features;
  self->variations = g_memdup2 (variations, sizeof (hb_variation_t) * n_variations);
  self->n_variations = n_variations;

  return self;
}

/**
 * pango2_hb_font_new_for_description:
 * @face: the `Pango2HbFace` to use
 * @description: a `Pango2FontDescription`
 * @dpi: the dpi used when rendering
 * @ctm: (nullable): transformation matrix to use when rendering
 *
 * Creates a new `Pango2HbFont` with size, features, variations and
 * gravity taken from a font description.
 *
 * Returns: a newly created `Pango2HbFont`
 */
Pango2HbFont *
pango2_hb_font_new_for_description (Pango2HbFace                *face,
                                    const Pango2FontDescription *description,
                                    float                        dpi,
                                    const Pango2Matrix          *ctm)
{
  int size;
  hb_feature_t features[10];
  unsigned int n_features;
  hb_variation_t *variations;
  unsigned int n_variations;
  unsigned int length;
  Pango2Gravity gravity;
  const char *str;

  g_return_val_if_fail (PANGO2_IS_HB_FACE (face), NULL);
  g_return_val_if_fail (description != NULL, NULL);
  g_return_val_if_fail (dpi > 0, NULL);

  if ((pango2_font_description_get_set_fields (description) & PANGO2_FONT_MASK_SIZE) == 0)
    size = 10 * PANGO2_SCALE;
  else if (pango2_font_description_get_size_is_absolute (description))
    size = pango2_font_description_get_size (description) * 72. / dpi;
  else
    size = pango2_font_description_get_size (description);

  g_return_val_if_fail (size > 0, NULL);

  font_description_get_features (description, features, G_N_ELEMENTS (features), &n_features);

  if ((pango2_font_description_get_set_fields (description) & PANGO2_FONT_MASK_VARIATIONS) != 0)
    {
      str = pango2_font_description_get_variations (description);
      length = count_variations (str);
      variations = g_alloca (sizeof (hb_variation_t) * length);
      parse_variations (str, variations, length, &n_variations);
    }
  else
    {
      variations = NULL;
      n_variations = 0;
    }

  if ((pango2_font_description_get_set_fields (description) & PANGO2_FONT_MASK_GRAVITY) != 0 &&
      pango2_font_description_get_gravity (description) != PANGO2_GRAVITY_SOUTH)
    gravity = pango2_font_description_get_gravity (description);
  else
    gravity = PANGO2_GRAVITY_AUTO;

  return pango2_hb_font_new (face, size, features, n_features, variations, n_variations, gravity, dpi, ctm);
}

/**
 * pango2_hb_font_get_features:
 * @self: a `Pango2Font`
 * @n_features: (nullable) (out caller-allocates): return location for
 *   the length of the returned array
 *
 * Obtain the OpenType features that are provided by the font.
 *
 * These are passed to the rendering system, together with features
 * that have been explicitly set via attributes.
 *
 * Note that this does not include OpenType features which the
 * rendering system enables by default.
 *
 * Returns: (nullable) (transfer none): the features
 */
const hb_feature_t *
pango2_hb_font_get_features (Pango2HbFont *self,
                             unsigned int *n_features)
{
  g_return_val_if_fail (PANGO2_IS_HB_FONT (self), NULL);

  if (n_features)
    *n_features = self->n_features;

  return self->features;
}


/**
 * pango2_hb_font_get_variations:
 * @self: a `Pango2HbFont`
 * @n_variations: (nullable) (out caller-allocates): return location for
 *   the length of the returned array
 *
 * Gets the variations of the font.
 *
 * Returns: (nullable) (transfer none): the variations
 */
const hb_variation_t *
pango2_hb_font_get_variations (Pango2HbFont *self,
                               unsigned int *n_variations)
{
  g_return_val_if_fail (PANGO2_IS_HB_FONT (self), NULL);

  if (n_variations)
    *n_variations = self->n_variations;

  return self->variations;
}

/* }}} */

/* vim:set foldmethod=marker expandtab: */
