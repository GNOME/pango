/* Pango
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
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.	 See the GNU
 * Library General Public License for more details.
 *
 * You should have received a copy of the GNU Library General Public
 * License along with this library; if not, write to the
 * Free Software Foundation, Inc., 59 Temple Place - Suite 330,
 * Boston, MA 02111-1307, USA.
 */

#include "config.h"

#include <math.h>

#include "pangocairo.h"
#include "pangocairo-private.h"
#include "pango-impl-utils.h"

#define PANGO_CAIRO_FONT_PRIVATE(font)		\
  ((PangoCairoFontPrivate *)			\
   (font == NULL ? NULL :			\
    G_STRUCT_MEMBER_P (font,			\
    PANGO_CAIRO_FONT_GET_IFACE(PANGO_CAIRO_FONT(font))->cf_priv_offset)))

GType
pango_cairo_font_get_type (void)
{
  static GType cairo_font_type = 0;

  if (! cairo_font_type)
    {
      const GTypeInfo cairo_font_info =
      {
	sizeof (PangoCairoFontIface), /* class_size */
	NULL,           /* base_init */
	NULL,		/* base_finalize */
	NULL,
	NULL,		/* class_finalize */
	NULL,		/* class_data */
	0,
	0,
	NULL,
	NULL
      };

      cairo_font_type =
	g_type_register_static (G_TYPE_INTERFACE, I_("PangoCairoFont"),
				&cairo_font_info, 0);

      g_type_interface_add_prerequisite (cairo_font_type, PANGO_TYPE_FONT);
    }

  return cairo_font_type;
}

static PangoCairoFontPrivateScaledFontData *
_pango_cairo_font_private_scaled_font_data_create (void)
{
  return g_slice_new (PangoCairoFontPrivateScaledFontData);
}

static void
_pango_cairo_font_private_scaled_font_data_destroy (PangoCairoFontPrivateScaledFontData *data)
{
  if (data)
    {
      cairo_font_options_destroy (data->options);
      g_slice_free (PangoCairoFontPrivateScaledFontData, data);
    }
}

cairo_scaled_font_t *
_pango_cairo_font_private_get_scaled_font (PangoCairoFontPrivate *cf_priv)
{
  cairo_font_face_t *font_face;

  if (G_LIKELY (cf_priv->scaled_font))
    return cf_priv->scaled_font;

  /* need to create it */

  if (G_UNLIKELY (cf_priv->data == NULL))
    {
      /* we have tried to create and failed before */
      return NULL;
    }

  font_face = (* PANGO_CAIRO_FONT_GET_IFACE (cf_priv->cfont)->create_font_face) (cf_priv->cfont);
  if (G_UNLIKELY (font_face == NULL))
    goto done;

  cf_priv->scaled_font = cairo_scaled_font_create (font_face,
						   &cf_priv->data->font_matrix,
						   &cf_priv->data->ctm,
						   cf_priv->data->options);

  cairo_font_face_destroy (font_face);

done:

  if (G_UNLIKELY (cf_priv->scaled_font == NULL || cairo_scaled_font_status (cf_priv->scaled_font) != CAIRO_STATUS_SUCCESS))
    {
      PangoFont *font = PANGO_FONT (cf_priv->cfont);
      static GQuark warned_quark = 0;
      if (!warned_quark)
	warned_quark = g_quark_from_static_string ("pangocairo-scaledfont-warned");

      if (!g_object_get_qdata (G_OBJECT (font), warned_quark))
	{
	  PangoFontDescription *desc;
	  char *s;

	  desc = pango_font_describe (font);
	  s = pango_font_description_to_string (desc);
	  pango_font_description_free (desc);

	  g_warning ("failed to create cairo %s, expect ugly output. the offending font is '%s'",
		     font_face ? "scaled font" : "font face",
		     s);

	  g_free (s);

	  g_object_set_qdata_full (G_OBJECT (font), warned_quark,
				   GINT_TO_POINTER (1), NULL);
	}
    }

  _pango_cairo_font_private_scaled_font_data_destroy (cf_priv->data);
  cf_priv->data = NULL;

  return cf_priv->scaled_font;
}

/**
 * pango_cairo_font_get_scaled_font:
 * @font: a #PangoFont from a #PangoCairoFontMap
 *
 * Gets the #cairo_scaled_font_t used by @font.
 * The scaled font can be referenced and kept using
 * cairo_scaled_font_reference().
 *
 * Return value: the #cairo_scaled_font_t used by @font,
 *               or %NULL if @font is %NULL.
 *
 * Since: 1.18
 **/
cairo_scaled_font_t *
pango_cairo_font_get_scaled_font (PangoCairoFont *cfont)
{
  PangoCairoFontPrivate *cf_priv;

  if (G_UNLIKELY (!cfont))
    return NULL;

  cf_priv = PANGO_CAIRO_FONT_PRIVATE (cfont);

  return _pango_cairo_font_private_get_scaled_font (cf_priv);
}

/**
 * _pango_cairo_font_install:
 * @font: a #PangoCairoFont
 * @cr: a #cairo_t
 *
 * Makes @font the current font for rendering in the specified
 * Cairo context.
 *
 * Return value: %TRUE if font was installed successfully, %FALSE otherwise.
 **/
gboolean
_pango_cairo_font_install (PangoFont *font,
			   cairo_t   *cr)
{
  cairo_scaled_font_t *scaled_font = pango_cairo_font_get_scaled_font ((PangoCairoFont *)font);

  if (G_UNLIKELY (scaled_font == NULL || cairo_scaled_font_status (scaled_font) != CAIRO_STATUS_SUCCESS))
    return FALSE;

  cairo_set_scaled_font (cr, scaled_font);

  return TRUE;
}


typedef struct _PangoCairoFontMetricsInfo
{
  const char       *sample_str;
  PangoFontMetrics *metrics;
} PangoCairoFontMetricsInfo;

PangoFontMetrics *
_pango_cairo_font_get_metrics (PangoFont     *font,
			       PangoLanguage *language)
{
  PangoCairoFont *cfont = (PangoCairoFont *) font;
  PangoCairoFontPrivate *cf_priv = PANGO_CAIRO_FONT_PRIVATE (font);
  PangoCairoFontMetricsInfo *info = NULL; /* Quiet gcc */
  GSList *tmp_list;

  const char *sample_str = pango_language_get_sample_string (language);

  tmp_list = cf_priv->metrics_by_lang;
  while (tmp_list)
    {
      info = tmp_list->data;

      if (info->sample_str == sample_str)    /* We _don't_ need strcmp */
	break;

      tmp_list = tmp_list->next;
    }

  if (!tmp_list)
    {
      PangoFontMap *fontmap;
      PangoContext *context;
      cairo_font_options_t *font_options;
      int height, shift;

      /* XXX this is racy.  need a ref'ing getter... */
      fontmap = pango_font_get_font_map (font);
      if (!fontmap)
        return pango_font_metrics_new ();
      fontmap = g_object_ref (fontmap);

      info = g_slice_new0 (PangoCairoFontMetricsInfo);

      cf_priv->metrics_by_lang = g_slist_prepend (cf_priv->metrics_by_lang, info);

      info->sample_str = sample_str;

      context = pango_font_map_create_context (fontmap);
      pango_context_set_language (context, language);
      font_options = cairo_font_options_create ();
      cairo_scaled_font_get_font_options (_pango_cairo_font_private_get_scaled_font (cf_priv), font_options);
      pango_cairo_context_set_font_options (context, font_options);
      cairo_font_options_destroy (font_options);

      info->metrics = (* PANGO_CAIRO_FONT_GET_IFACE (font)->create_metrics_for_context) (cfont, context);

      /* We may actually reuse ascent/descent we got from cairo here.  that's
       * in cf_priv->font_extents.
       */
      height = info->metrics->ascent + info->metrics->descent;
      switch (cf_priv->gravity)
	{
	  default:
	  case PANGO_GRAVITY_AUTO:
	  case PANGO_GRAVITY_SOUTH:
	    break;
	  case PANGO_GRAVITY_NORTH:
	    info->metrics->ascent = info->metrics->descent;
	    break;
	  case PANGO_GRAVITY_EAST:
	  case PANGO_GRAVITY_WEST:
	    {
	      int ascent = height / 2;
	      if (cf_priv->is_hinted)
	        ascent = PANGO_UNITS_ROUND (ascent);
	      info->metrics->ascent = ascent;
	    }
	}
      shift = (height - info->metrics->ascent) - info->metrics->descent;
      info->metrics->descent += shift;
      info->metrics->underline_position -= shift;
      info->metrics->strikethrough_position -= shift;
      info->metrics->ascent = height - info->metrics->descent;

      g_object_unref (context);
      g_object_unref (fontmap);
    }

  return pango_font_metrics_ref (info->metrics);
}

static PangoCairoFontHexBoxInfo *
_pango_cairo_font_private_get_hex_box_info (PangoCairoFontPrivate *cf_priv)
{
  static const char hexdigits[] = "0123456789ABCDEF";
  char c[2] = {0, 0};
  PangoFont *mini_font;
  PangoCairoFontHexBoxInfo *hbi;

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
  PangoFontDescription *desc;
  cairo_scaled_font_t *scaled_font, *scaled_mini_font;
  PangoMatrix pango_ctm;
  cairo_matrix_t cairo_ctm;
  PangoGravity gravity;

  if (!cf_priv)
    return NULL;

  if (cf_priv->hbi)
    return cf_priv->hbi;

  scaled_font = _pango_cairo_font_private_get_scaled_font (cf_priv);
  if (G_UNLIKELY (scaled_font == NULL || cairo_scaled_font_status (scaled_font) != CAIRO_STATUS_SUCCESS))
    return NULL;

  is_hinted = cf_priv->is_hinted;

  font_options = cairo_font_options_create ();
  desc = pango_font_describe_with_absolute_size ((PangoFont *)cf_priv->cfont);
  size = pango_font_description_get_size (desc) / (1.*PANGO_SCALE);
  gravity = pango_font_description_get_gravity (desc);

  cairo_scaled_font_get_ctm (scaled_font, &cairo_ctm);
  cairo_scaled_font_get_font_options (scaled_font, font_options);
  /* I started adding support for vertical hexboxes here, but it's too much
   * work.  Easier to do with cairo user fonts and vertical writing mode
   * support in cairo.
   */
  /*cairo_matrix_rotate (&cairo_ctm, pango_gravity_to_rotation (gravity));*/
  pango_ctm.xx = cairo_ctm.xx;
  pango_ctm.yx = cairo_ctm.yx;
  pango_ctm.xy = cairo_ctm.xy;
  pango_ctm.yy = cairo_ctm.yy;
  pango_ctm.x0 = cairo_ctm.x0;
  pango_ctm.y0 = cairo_ctm.y0;

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
    PangoFontMap *fontmap;
    PangoContext *context;

    /* XXX this is racy.  need a ref'ing getter... */
    fontmap = pango_font_get_font_map ((PangoFont *)cf_priv->cfont);
    if (!fontmap)
      return NULL;
    fontmap = g_object_ref (fontmap);

    /* we inherit most font properties for the mini font.  just
     * change family and size.  means, you get bold hex digits
     * in the hexbox for a bold font.
     */

    /* We should rotate the box, not glyphs */
    pango_font_description_unset_fields (desc, PANGO_FONT_MASK_GRAVITY);

    pango_font_description_set_family_static (desc, "monospace");

    rows = 2;
    mini_size = size / 2.2;
    if (is_hinted)
      {
	mini_size = HINT_Y (mini_size);

	if (mini_size < 6.0)
	  {
	    rows = 1;
	    mini_size = MIN (MAX (size - 1, 0), 6.0);
	  }
      }

    pango_font_description_set_absolute_size (desc, pango_units_from_double (mini_size));

    /* load mini_font */

    context = pango_font_map_create_context (fontmap);

    pango_context_set_matrix (context, &pango_ctm);
    pango_context_set_language (context, pango_script_get_sample_language (PANGO_SCRIPT_LATIN));
    pango_cairo_context_set_font_options (context, font_options);
    mini_font = pango_font_map_load_font (fontmap, context, desc);

    g_object_unref (context);
    g_object_unref (fontmap);
  }

  pango_font_description_free (desc);
  cairo_font_options_destroy (font_options);


  scaled_mini_font = pango_cairo_font_get_scaled_font ((PangoCairoFont *) mini_font);

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
      font_extents.ascent = PANGO_UNKNOWN_GLYPH_HEIGHT;
      font_extents.descent = 0;
    }

  pad = (font_extents.ascent + font_extents.descent) / 43;
  pad = MIN (pad, mini_size);

  hbi = g_slice_new (PangoCairoFontHexBoxInfo);
  hbi->font = (PangoCairoFont *) mini_font;
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
_pango_cairo_font_hex_box_info_destroy (PangoCairoFontHexBoxInfo *hbi)
{
  if (hbi)
    {
      g_object_unref (hbi->font);
      g_slice_free (PangoCairoFontHexBoxInfo, hbi);
    }
}

PangoCairoFontHexBoxInfo *
_pango_cairo_font_get_hex_box_info (PangoCairoFont *cfont)
{
  PangoCairoFontPrivate *cf_priv = PANGO_CAIRO_FONT_PRIVATE (cfont);

  return _pango_cairo_font_private_get_hex_box_info (cf_priv);
}

void
_pango_cairo_font_private_initialize (PangoCairoFontPrivate      *cf_priv,
				      PangoCairoFont             *cfont,
				      PangoContext               *context,
				      const PangoFontDescription *desc,
				      const cairo_matrix_t       *font_matrix)
{
  const cairo_font_options_t *font_options;
  cairo_matrix_t gravity_matrix;
  const PangoMatrix *pango_ctm;

  cf_priv->cfont = cfont;
  cf_priv->gravity = pango_font_description_get_gravity (desc);

  cf_priv->data = _pango_cairo_font_private_scaled_font_data_create (); 

  /* first apply gravity rotation, then font_matrix, such that
   * vertical italic text comes out "correct".  we don't do anything
   * like baseline adjustment etc though.  should be specially
   * handled when we support italic correction. */
  cairo_matrix_init_rotate(&gravity_matrix,
			   pango_gravity_to_rotation (cf_priv->gravity));
  cairo_matrix_multiply (&cf_priv->data->font_matrix,
			 font_matrix,
			 &gravity_matrix);

  pango_ctm = pango_context_get_matrix (context);
  if (pango_ctm)
    cairo_matrix_init (&cf_priv->data->ctm,
		       pango_ctm->xx,
		       pango_ctm->yx,
		       pango_ctm->xy,
		       pango_ctm->yy,
		       0., 0.);
  else
    cairo_matrix_init_identity (&cf_priv->data->ctm);

  font_options = _pango_cairo_context_get_merged_font_options (context);
  cf_priv->data->options = cairo_font_options_copy (font_options);
  cf_priv->is_hinted = cairo_font_options_get_hint_metrics (font_options) != CAIRO_HINT_METRICS_OFF;

  cf_priv->scaled_font = NULL;
  cf_priv->hbi = NULL;
  cf_priv->glyph_extents_cache = NULL;
  cf_priv->metrics_by_lang = NULL;
}

static void
free_metrics_info (PangoCairoFontMetricsInfo *info)
{
  pango_font_metrics_unref (info->metrics);
  g_slice_free (PangoCairoFontMetricsInfo, info);
}

void
_pango_cairo_font_private_finalize (PangoCairoFontPrivate *cf_priv)
{
  _pango_cairo_font_private_scaled_font_data_destroy (cf_priv->data);

  if (cf_priv->scaled_font)
    cairo_scaled_font_destroy (cf_priv->scaled_font);

  _pango_cairo_font_hex_box_info_destroy (cf_priv->hbi);

  if (cf_priv->glyph_extents_cache)
    g_free (cf_priv->glyph_extents_cache);

  g_slist_foreach (cf_priv->metrics_by_lang, (GFunc)free_metrics_info, NULL);
  g_slist_free (cf_priv->metrics_by_lang);
}

gboolean
_pango_cairo_font_private_is_metrics_hinted (PangoCairoFontPrivate *cf_priv)
{
  return cf_priv->is_hinted;
}

static void
_pango_cairo_font_private_get_glyph_extents_missing (PangoCairoFontPrivate *cf_priv,
						     PangoGlyph             glyph,
						     PangoRectangle        *ink_rect,
						     PangoRectangle        *logical_rect)
{
  PangoCairoFontHexBoxInfo *hbi;
  gunichar ch;
  gint rows, cols;

  hbi = _pango_cairo_font_private_get_hex_box_info (cf_priv);
  if (!hbi)
    {
      pango_font_get_glyph_extents (NULL, glyph, ink_rect, logical_rect);
      return;
    }

  ch = glyph & ~PANGO_GLYPH_UNKNOWN_FLAG;

  rows = hbi->rows;
  if (G_UNLIKELY (glyph == PANGO_GLYPH_INVALID_INPUT || ch > 0x10FFFF))
    cols = 1;
  else
    cols = ((glyph & ~PANGO_GLYPH_UNKNOWN_FLAG) > 0xffff ? 6 : 4) / rows;

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

#define GLYPH_CACHE_NUM_ENTRIES 256 /* should be power of two */
#define GLYPH_CACHE_MASK (GLYPH_CACHE_NUM_ENTRIES - 1)
/* An entry in the fixed-size cache for the glyph->extents mapping.
 * The cache is indexed by the lower N bits of the glyph (see
 * GLYPH_CACHE_NUM_ENTRIES).  For scripts with few glyphs,
 * this should provide pretty much instant lookups.
 */
struct _PangoCairoFontGlyphExtentsCacheEntry
{
  PangoGlyph     glyph;
  int            width;
  PangoRectangle ink_rect;
};

static gboolean
_pango_cairo_font_private_glyph_extents_cache_init (PangoCairoFontPrivate *cf_priv)
{
  cairo_scaled_font_t *scaled_font = _pango_cairo_font_private_get_scaled_font (cf_priv);
  cairo_font_extents_t font_extents;

  if (G_UNLIKELY (scaled_font == NULL || cairo_scaled_font_status (scaled_font) != CAIRO_STATUS_SUCCESS))
    return FALSE;

  cairo_scaled_font_extents (scaled_font, &font_extents);

  cf_priv->font_extents.x = 0;
  cf_priv->font_extents.width = 0;
  cf_priv->font_extents.height = pango_units_from_double (font_extents.ascent + font_extents.descent);
  switch (cf_priv->gravity)
    {
      default:
      case PANGO_GRAVITY_AUTO:
      case PANGO_GRAVITY_SOUTH:
	cf_priv->font_extents.y = - pango_units_from_double (font_extents.ascent);
	break;
      case PANGO_GRAVITY_NORTH:
	cf_priv->font_extents.y = - pango_units_from_double (font_extents.descent);
	break;
      case PANGO_GRAVITY_EAST:
      case PANGO_GRAVITY_WEST:
	{
	  int ascent = pango_units_from_double (font_extents.ascent + font_extents.descent) / 2;
	  if (cf_priv->is_hinted)
	    ascent = PANGO_UNITS_ROUND (ascent);
	  cf_priv->font_extents.y = - ascent;
	}
    }

  cf_priv->glyph_extents_cache = g_new0 (PangoCairoFontGlyphExtentsCacheEntry, GLYPH_CACHE_NUM_ENTRIES);
  /* Make sure all cache entries are invalid initially */
  cf_priv->glyph_extents_cache[0].glyph = 1; /* glyph 1 cannot happen in bucket 0 */

  return TRUE;
}

/* Fills in the glyph extents cache entry
 */
static void
compute_glyph_extents (PangoCairoFontPrivate  *cf_priv,
		       PangoGlyph              glyph,
		       PangoCairoFontGlyphExtentsCacheEntry *entry)
{
  cairo_text_extents_t extents;
  cairo_glyph_t cairo_glyph;

  cairo_glyph.index = glyph;
  cairo_glyph.x = 0;
  cairo_glyph.y = 0;

  cairo_scaled_font_glyph_extents (_pango_cairo_font_private_get_scaled_font (cf_priv),
				   &cairo_glyph, 1, &extents);

  entry->glyph = glyph;
  entry->width = pango_units_from_double (extents.x_advance);
  entry->ink_rect.x = pango_units_from_double (extents.x_bearing);
  entry->ink_rect.y = pango_units_from_double (extents.y_bearing);
  entry->ink_rect.width = pango_units_from_double (extents.width);
  entry->ink_rect.height = pango_units_from_double (extents.height);
}

static PangoCairoFontGlyphExtentsCacheEntry *
_pango_cairo_font_private_get_glyph_extents_cache_entry (PangoCairoFontPrivate  *cf_priv,
							 PangoGlyph              glyph)
{
  PangoCairoFontGlyphExtentsCacheEntry *entry;
  guint idx;

  idx = glyph & GLYPH_CACHE_MASK;
  entry = cf_priv->glyph_extents_cache + idx;

  if (entry->glyph != glyph)
    {
      compute_glyph_extents (cf_priv, glyph, entry);
    }

  return entry;
}

void
_pango_cairo_font_private_get_glyph_extents (PangoCairoFontPrivate *cf_priv,
					     PangoGlyph             glyph,
					     PangoRectangle        *ink_rect,
					     PangoRectangle        *logical_rect)
{
  PangoCairoFontGlyphExtentsCacheEntry *entry;

  if (!cf_priv ||
      (cf_priv->glyph_extents_cache == NULL &&
       !_pango_cairo_font_private_glyph_extents_cache_init (cf_priv)))
    {
      /* Get generic unknown-glyph extents. */
      pango_font_get_glyph_extents (NULL, glyph, ink_rect, logical_rect);
      return;
    }

  if (glyph == PANGO_GLYPH_EMPTY)
    {
      if (ink_rect)
	ink_rect->x = ink_rect->y = ink_rect->width = ink_rect->height = 0;
      if (logical_rect)
	*logical_rect = cf_priv->font_extents;
      return;
    }
  else if (glyph & PANGO_GLYPH_UNKNOWN_FLAG)
    {
      _pango_cairo_font_private_get_glyph_extents_missing(cf_priv, glyph, ink_rect, logical_rect);
      return;
    }

  entry = _pango_cairo_font_private_get_glyph_extents_cache_entry (cf_priv, glyph);

  if (ink_rect)
    *ink_rect = entry->ink_rect;
  if (logical_rect)
    {
      *logical_rect = cf_priv->font_extents;
      logical_rect->width = entry->width;
    }
}
