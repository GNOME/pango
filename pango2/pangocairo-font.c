/* Pango2
 * pangocairo-font.c: Cairo font handling
 *
 * Copyright (C) 2000-2005 Red Hat Software
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

#include <math.h>
#include <string.h>

#include <hb-ot.h>
#include <hb-cairo.h>

#include "pangocairo.h"
#include "pangocairo-private.h"
#include "pango-font-private.h"
#include "pango-font-metrics-private.h"
#include "pango-impl-utils.h"
#include "pango-hbfont-private.h"
#include "pango-hbface-private.h"
#include "pango-userfont-private.h"
#include "pango-userface-private.h"
#include "pango-font-private.h"


static Pango2CairoFontPrivate * _pango2_font_get_cairo_font_private (Pango2Font *font);
static cairo_scaled_font_t * _pango2_font_get_scaled_font (Pango2Font *font,
                                                           GQuark      palette);
static void _pango2_cairo_font_private_initialize (Pango2CairoFontPrivate     *cf_priv,
                                                   Pango2Font                 *font,
                                                   Pango2Gravity               gravity,
                                                   const cairo_font_options_t *font_options,
                                                   const Pango2Matrix         *pango2_ctm,
                                                   const cairo_matrix_t       *font_matrix);
static void _pango2_cairo_font_private_finalize (Pango2CairoFontPrivate *cf_priv);



static Pango2CairoFontPrivateScaledFontData *
_pango2_cairo_font_private_scaled_font_data_create (void)
{
  return g_slice_new (Pango2CairoFontPrivateScaledFontData);
}

static void
_pango2_cairo_font_private_scaled_font_data_destroy (Pango2CairoFontPrivateScaledFontData *data)
{
  if (data)
    {
      cairo_font_options_destroy (data->options);
      g_slice_free (Pango2CairoFontPrivateScaledFontData, data);
    }
}

static cairo_font_face_t *
create_cairo_font_face (Pango2Font *font)
{
  cairo_font_face_t *cairo_face;
  hb_font_t *hbfont;

  if (PANGO2_IS_USER_FONT (font))
    return create_cairo_user_font_face (font);

#ifdef CAIRO_HAS_FT_FONT
  cairo_face = create_cairo_ft_font_face (font);
  if (cairo_face)
    return cairo_face;
#endif

#if 0
#ifdef HAVE_CORE_TEXT
  cairo_face = create_cairo_core_text_font_face (font);
  if (cairo_face)
    return cairo_face;
#endif

#ifdef HAVE_DIRECT_WRITE
  cairo_face = create_cairo_dwrite_font_face (font);
  if (cairo_face)
    return cairo_face;
#endif
#endif

  hbfont = pango2_font_get_hb_font (font);
  return hb_cairo_font_face_create_for_font (hbfont);
}

static int
find_palette_index_for_font (Pango2Font *font,
                             GQuark      palette)
{
  Pango2FontFace *face = pango2_font_get_face (font);

  if (PANGO2_IS_HB_FACE (face))
    return pango2_hb_face_get_palette_index (PANGO2_HB_FACE (face), palette);

  return CAIRO_COLOR_PALETTE_DEFAULT;
}

static cairo_scaled_font_t *
_pango2_cairo_font_private_get_scaled_font (Pango2CairoFontPrivate *cf_priv,
                                            GQuark                  palette)
{
  cairo_font_face_t *font_face;
  cairo_font_options_t *options;
  cairo_matrix_t _font_matrix, *font_matrix;
  cairo_matrix_t _ctm, *ctm;

  if (G_LIKELY (cf_priv->palette == palette && cf_priv->scaled_font))
    return cf_priv->scaled_font;

  if (!cf_priv->data && !cf_priv->scaled_font)
    return NULL;

  /* need to create it */

  if (cf_priv->scaled_font)
    {
      font_face = cairo_scaled_font_get_font_face (cf_priv->scaled_font);
      cairo_font_face_reference (font_face);

      options = cairo_font_options_create ();
      cairo_scaled_font_get_font_options (cf_priv->scaled_font, options);
      cairo_scaled_font_get_font_matrix (cf_priv->scaled_font, &_font_matrix);
      font_matrix = &_font_matrix;
      cairo_scaled_font_get_ctm (cf_priv->scaled_font, &_ctm);
      ctm = &_ctm;

      cairo_scaled_font_destroy (cf_priv->scaled_font);
      cf_priv->scaled_font = NULL;
    }
  else
    {
      font_face = create_cairo_font_face (cf_priv->cfont);
      options = cairo_font_options_copy (cf_priv->data->options);
      font_matrix = &cf_priv->data->font_matrix;
      ctm = &cf_priv->data->ctm;
    }

  if (G_UNLIKELY (font_face == NULL))
    goto done;

  cairo_font_options_set_color_mode (options, CAIRO_COLOR_MODE_COLOR);
  cairo_font_options_set_color_palette (options,
                                        find_palette_index_for_font (cf_priv->cfont, palette));

  cf_priv->palette = palette;
  cf_priv->scaled_font = cairo_scaled_font_create (font_face, font_matrix, ctm, options);

  cairo_font_options_destroy (options);
  cairo_font_face_destroy (font_face);

done:

  if (G_UNLIKELY (cf_priv->scaled_font == NULL || cairo_scaled_font_status (cf_priv->scaled_font) != CAIRO_STATUS_SUCCESS))
    {
      cairo_scaled_font_t *scaled_font = cf_priv->scaled_font;
      Pango2Font *font = cf_priv->cfont;
      static GQuark warned_quark = 0; /* MT-safe */
      if (!warned_quark)
        warned_quark = g_quark_from_static_string ("pangocairo-scaledfont-warned");

      if (!g_object_get_qdata (G_OBJECT (font), warned_quark))
        {
          Pango2FontDescription *desc;
          char *s;

          desc = pango2_font_describe (font);
          s = pango2_font_description_to_string (desc);
          pango2_font_description_free (desc);

          g_warning ("failed to create cairo %s, expect ugly output. the offending font is '%s'",
                     font_face ? "scaled font" : "font face",
                     s);

          if (!font_face)
                g_warning ("font_face is NULL");
          else
                g_warning ("font_face status is: %s",
                           cairo_status_to_string (cairo_font_face_status (font_face)));

          if (!scaled_font)
                g_warning ("scaled_font is NULL");
          else
                g_warning ("scaled_font status is: %s",
                           cairo_status_to_string (cairo_scaled_font_status (scaled_font)));

          g_free (s);

          g_object_set_qdata_full (G_OBJECT (font), warned_quark,
                                   GINT_TO_POINTER (1), NULL);
        }
    }

  _pango2_cairo_font_private_scaled_font_data_destroy (cf_priv->data);
  cf_priv->data = NULL;

  return cf_priv->scaled_font;
}

static cairo_scaled_font_t *
_pango2_font_get_scaled_font (Pango2Font *font,
                              GQuark      palette)
{
  Pango2CairoFontPrivate *cf_priv;

  cf_priv = _pango2_font_get_cairo_font_private (font);

  if (G_UNLIKELY (!cf_priv))
    return NULL;

  return _pango2_cairo_font_private_get_scaled_font (cf_priv, palette);
}

/**
 * _pango2_cairo_font_install:
 * @font: a `Pango2CairoFont`
 * @palette: a palette
 * @cr: a #cairo_t
 *
 * Makes @font the current font for rendering in the specified
 * Cairo context.
 *
 * Return value: %TRUE if font was installed successfully, %FALSE otherwise.
 */
gboolean
_pango2_cairo_font_install (Pango2Font *font,
                            GQuark      palette,
                            cairo_t    *cr)
{
  cairo_scaled_font_t *scaled_font;

  scaled_font = _pango2_font_get_scaled_font (font, palette);

  if (G_UNLIKELY (scaled_font == NULL || cairo_scaled_font_status (scaled_font) != CAIRO_STATUS_SUCCESS))
    return FALSE;

  cairo_set_scaled_font (cr, scaled_font);

  return TRUE;
}

static Pango2CairoFontHexBoxInfo *
_pango2_cairo_font_private_get_hex_box_info (Pango2CairoFontPrivate *cf_priv)
{
  const char hexdigits[] = "0123456789ABCDEF";
  char c[2] = {0, 0};
  Pango2Font *mini_font;
  Pango2CairoFontHexBoxInfo *hbi;

  /* for metrics hinting */
  double scale_x = 1., scale_x_inv = 1., scale_y = 1., scale_y_inv = 1.;
  gboolean is_hinted;

  int i;
  int rows;
  double pad;
  double width = 0;
  double height = 0;
  cairo_font_options_t *font_options;
  cairo_font_extents_t font_extents;
  double size, mini_size;
  Pango2FontDescription *desc;
  cairo_scaled_font_t *scaled_font, *scaled_mini_font;
  Pango2Matrix pango2_ctm, pango2_font_matrix;
  cairo_matrix_t cairo_ctm, cairo_font_matrix;
  /*Pango2Gravity gravity;*/

  if (!cf_priv)
    return NULL;

  if (cf_priv->hbi)
    return cf_priv->hbi;

  scaled_font = _pango2_cairo_font_private_get_scaled_font (cf_priv, 0);
  if (G_UNLIKELY (scaled_font == NULL || cairo_scaled_font_status (scaled_font) != CAIRO_STATUS_SUCCESS))
    return NULL;

  is_hinted = cf_priv->is_hinted;

  font_options = cairo_font_options_create ();
  desc = pango2_font_describe_with_absolute_size (cf_priv->cfont);
  /*gravity = pango2_font_description_get_gravity (desc);*/

  cairo_scaled_font_get_ctm (scaled_font, &cairo_ctm);
  cairo_scaled_font_get_font_matrix (scaled_font, &cairo_font_matrix);
  cairo_scaled_font_get_font_options (scaled_font, font_options);
  /* I started adding support for vertical hexboxes here, but it's too much
   * work.  Easier to do with cairo user fonts and vertical writing mode
   * support in cairo.
   */
  /*cairo_matrix_rotate (&cairo_ctm, pango2_gravity_to_rotation (gravity));*/
  pango2_ctm.xx = cairo_ctm.xx;
  pango2_ctm.yx = cairo_ctm.yx;
  pango2_ctm.xy = cairo_ctm.xy;
  pango2_ctm.yy = cairo_ctm.yy;
  pango2_ctm.x0 = cairo_ctm.x0;
  pango2_ctm.y0 = cairo_ctm.y0;
  pango2_font_matrix.xx = cairo_font_matrix.xx;
  pango2_font_matrix.yx = cairo_font_matrix.yx;
  pango2_font_matrix.xy = cairo_font_matrix.xy;
  pango2_font_matrix.yy = cairo_font_matrix.yy;
  pango2_font_matrix.x0 = cairo_font_matrix.x0;
  pango2_font_matrix.y0 = cairo_font_matrix.y0;

  size = pango2_matrix_get_font_scale_factor (&pango2_font_matrix) /
         pango2_matrix_get_font_scale_factor (&pango2_ctm);

  if (is_hinted)
    {
      /* prepare for some hinting */
      double x, y;

      x = 1.; y = 0.;
      cairo_matrix_transform_distance (&cairo_ctm, &x, &y);
      scale_x = sqrt (x*x + y*y);
      scale_x_inv = 1 / scale_x;

      x = 0.; y = 1.;
      cairo_matrix_transform_distance (&cairo_ctm, &x, &y);
      scale_y = sqrt (x*x + y*y);
      scale_y_inv = 1 / scale_y;
    }

/* we hint to the nearest device units */
#define HINT(value, scale, scale_inv) (ceil ((value-1e-5) * scale) * scale_inv)
#define HINT_X(value) HINT ((value), scale_x, scale_x_inv)
#define HINT_Y(value) HINT ((value), scale_y, scale_y_inv)

  /* create mini_font description */
  {
    Pango2FontFace *face;
    Pango2FontFamily *family;
    Pango2FontMap *fontmap;
    Pango2Context *context;

    /* XXX this is racy.  need a ref'ing getter... */
    face = pango2_font_get_face (cf_priv->cfont);
    family = pango2_font_face_get_family (face);
    if (!family)
      return NULL;
    fontmap = pango2_font_family_get_font_map (family);
    if (!fontmap)
      return NULL;
    fontmap = g_object_ref (fontmap);

    /* we inherit most font properties for the mini font.  just
     * change family and size.  means, you get bold hex digits
     * in the hexbox for a bold font.
     */

    /* We should rotate the box, not glyphs */
    pango2_font_description_unset_fields (desc, PANGO2_FONT_MASK_GRAVITY);

    pango2_font_description_set_family_static (desc, "monospace");

    rows = 2;
    mini_size = size / 2.2;
    if (is_hinted)
      mini_size = HINT_Y (mini_size);

    pango2_font_description_set_absolute_size (desc, pango2_units_from_double (mini_size));

    /* load mini_font */

    context = pango2_context_new_with_font_map (fontmap);

    pango2_context_set_matrix (context, &pango2_ctm);
    pango2_context_set_language (context, pango2_script_get_sample_language (G_UNICODE_SCRIPT_LATIN));
    pango2_cairo_context_set_font_options (context, font_options);
    mini_font = pango2_font_map_load_font (fontmap, context, desc);

    g_object_unref (context);
    g_object_unref (fontmap);
  }

  pango2_font_description_free (desc);
  cairo_font_options_destroy (font_options);

  scaled_mini_font = _pango2_font_get_scaled_font (mini_font, 0);
  if (G_UNLIKELY (scaled_mini_font == NULL || cairo_scaled_font_status (scaled_mini_font) != CAIRO_STATUS_SUCCESS))
    return NULL;

  for (i = 0 ; i < 16 ; i++)
    {
      cairo_text_extents_t extents;

      c[0] = hexdigits[i];
      cairo_scaled_font_text_extents (scaled_mini_font, c, &extents);
      width = MAX (width, extents.width);
      height = MAX (height, extents.height);
    }

  cairo_scaled_font_extents (scaled_font, &font_extents);
  if (font_extents.ascent + font_extents.descent <= 0)
    {
      font_extents.ascent = PANGO2_UNKNOWN_GLYPH_HEIGHT;
      font_extents.descent = 0;
    }

  pad = (font_extents.ascent + font_extents.descent) / 43;
  pad = MIN (pad, mini_size);

  hbi = g_slice_new (Pango2CairoFontHexBoxInfo);
  hbi->font = mini_font;
  hbi->rows = rows;

  hbi->digit_width  = width;
  hbi->digit_height = height;

  hbi->pad_x = pad;
  hbi->pad_y = pad;

  if (is_hinted)
    {
      hbi->digit_width  = HINT_X (hbi->digit_width);
      hbi->digit_height = HINT_Y (hbi->digit_height);
      hbi->pad_x = HINT_X (hbi->pad_x);
      hbi->pad_y = HINT_Y (hbi->pad_y);
    }

  hbi->line_width = MIN (hbi->pad_x, hbi->pad_y);

  hbi->box_height = 3 * hbi->pad_y + rows * (hbi->pad_y + hbi->digit_height);

  if (rows == 1 || hbi->box_height <= font_extents.ascent)
    {
      hbi->box_descent = 2 * hbi->pad_y;
    }
  else if (hbi->box_height <= font_extents.ascent + font_extents.descent - 2 * hbi->pad_y)
    {
      hbi->box_descent = 2 * hbi->pad_y + hbi->box_height - font_extents.ascent;
    }
  else
    {
      hbi->box_descent = font_extents.descent * hbi->box_height /
                         (font_extents.ascent + font_extents.descent);
    }
  if (is_hinted)
    {
       hbi->box_descent = HINT_Y (hbi->box_descent);
    }

  cf_priv->hbi = hbi;
  return hbi;
}

static void
_pango2_cairo_font_hex_box_info_destroy (Pango2CairoFontHexBoxInfo *hbi)
{
  if (hbi)
    {
      g_object_unref (hbi->font);
      g_slice_free (Pango2CairoFontHexBoxInfo, hbi);
    }
}

static void
free_cairo_font_private (gpointer data)
{
  Pango2CairoFontPrivate *cf_priv = data;
  _pango2_cairo_font_private_finalize (cf_priv);
  g_free (data);
}

static Pango2CairoFontPrivate *
_pango2_font_get_cairo_font_private (Pango2Font *font)
{
  Pango2CairoFontPrivate *cf_priv;

  cf_priv = g_object_get_data (G_OBJECT (font), "pango-font-cairo_private");
  if (!cf_priv)
    {
      cairo_font_options_t *font_options;
      cairo_matrix_t font_matrix;
      double x_scale, y_scale;
      int size;

      cairo_matrix_init (&font_matrix, 1., 0., 0., 1., 0., 0.);
      x_scale = y_scale = 1;

      if (PANGO2_IS_HB_FONT (font))
        {
          Pango2HbFace *face = PANGO2_HB_FACE (font->face);
          if (face->transform)
            cairo_matrix_init (&font_matrix,
                               face->transform->xx,
                               - face->transform->yx,
                               - face->transform->xy,
                               face->transform->yy,
                               0., 0.);

          x_scale = face->x_scale;
          y_scale = face->y_scale;
        }

      size = font->size * font->dpi / 72.;

      cairo_matrix_scale (&font_matrix,
                          x_scale * size / (double)PANGO2_SCALE,
                          y_scale * size / (double)PANGO2_SCALE);

      font_options = (cairo_font_options_t *)pango2_cairo_font_get_font_options (font);
      if (font_options)
        font_options = cairo_font_options_copy (font_options);
      else
        {
          font_options = cairo_font_options_create ();
          cairo_font_options_set_hint_style (font_options, CAIRO_HINT_STYLE_NONE);
          cairo_font_options_set_hint_metrics (font_options, CAIRO_HINT_METRICS_OFF);
        }

      cf_priv = g_new0 (Pango2CairoFontPrivate, 1);
      _pango2_cairo_font_private_initialize (cf_priv,
                                            font,
                                            font->gravity,
                                            font_options,
                                            &font->ctm,
                                            &font_matrix);

      cairo_font_options_destroy (font_options);

      g_object_set_data_full (G_OBJECT (font), "pango-font-cairo_private",
                              cf_priv, free_cairo_font_private);
    }

  return cf_priv;
}

Pango2CairoFontHexBoxInfo *
_pango2_cairo_font_get_hex_box_info (Pango2Font *font)
{
  Pango2CairoFontPrivate *cf_priv = _pango2_font_get_cairo_font_private (font);

  return _pango2_cairo_font_private_get_hex_box_info (cf_priv);
}

void
_pango2_cairo_font_private_initialize (Pango2CairoFontPrivate     *cf_priv,
                                       Pango2Font                 *cfont,
                                       Pango2Gravity               gravity,
                                       const cairo_font_options_t *font_options,
                                       const Pango2Matrix         *pango2_ctm,
                                       const cairo_matrix_t       *font_matrix)
{
  cairo_matrix_t gravity_matrix;

  cf_priv->cfont = cfont;
  cf_priv->gravity = gravity != PANGO2_GRAVITY_AUTO ? gravity : PANGO2_GRAVITY_SOUTH;

  cf_priv->data = _pango2_cairo_font_private_scaled_font_data_create ();

  /* first apply gravity rotation, then font_matrix, such that
   * vertical italic text comes out "correct".  we don't do anything
   * like baseline adjustment etc though.  should be specially
   * handled when we support italic correction.
   */
  cairo_matrix_init_rotate (&gravity_matrix,
                            pango2_gravity_to_rotation (cf_priv->gravity));
  cairo_matrix_multiply (&cf_priv->data->font_matrix,
                         font_matrix,
                         &gravity_matrix);

  if (pango2_ctm)
    cairo_matrix_init (&cf_priv->data->ctm,
                       pango2_ctm->xx,
                       pango2_ctm->yx,
                       pango2_ctm->xy,
                       pango2_ctm->yy,
                       0., 0.);
  else
    cairo_matrix_init_identity (&cf_priv->data->ctm);

  cf_priv->data->options = cairo_font_options_copy (font_options);
  cf_priv->is_hinted = cairo_font_options_get_hint_metrics (font_options) != CAIRO_HINT_METRICS_OFF;

  cf_priv->scaled_font = NULL;
  cf_priv->hbi = NULL;
}

void
_pango2_cairo_font_private_finalize (Pango2CairoFontPrivate *cf_priv)
{
  _pango2_cairo_font_private_scaled_font_data_destroy (cf_priv->data);

  if (cf_priv->scaled_font)
    cairo_scaled_font_destroy (cf_priv->scaled_font);
  cf_priv->scaled_font = NULL;

  _pango2_cairo_font_hex_box_info_destroy (cf_priv->hbi);
  cf_priv->hbi = NULL;
}

/**
 * pango2_cairo_font_set_font_options:
 * @font: a `Pango2Font`
 * @options: (nullable): a `cairo_font_options_t`, or %NULL to unset
 *   any previously set options. A copy is made.
 *
 * Sets the font options used when rendering text with this font.
 *
 * This is rarely needed. Fonts usually get font options from the
 * `Pango2Context` in which they are loaded.
 */
void
pango2_cairo_font_set_font_options (Pango2Font                 *font,
                                    const cairo_font_options_t *options)
{
  g_return_if_fail (PANGO2_IS_FONT (font));

  if (!font->options && !options)
    return;

  if (font->options && options &&
      cairo_font_options_equal (font->options, options))
    return;

  if (font->options)
    cairo_font_options_destroy (font->options);

  if (options)
    font->options = cairo_font_options_copy (options);
  else
    font->options = NULL;
}

/**
 * pango2_cairo_font_get_font_options:
 * @font: a `Pango2Font`
 *
 * Gets font options for the font.
 *
 * Returns: (transfer none): font options that are
 *   applied when rendering text with this font
 */
const cairo_font_options_t *
pango2_cairo_font_get_font_options (Pango2Font *font)
{
  g_return_val_if_fail (PANGO2_IS_FONT (font), NULL);

  return font->options;
}
