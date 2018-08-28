/* Pango
 * pangofc-shape.c: Basic shaper for FreeType-based backends
 *
 * Copyright (C) 2000, 2007, 2009 Red Hat Software
 * Authors:
 *   Owen Taylor <otaylor@redhat.com>
 *   Behdad Esfahbod <behdad@behdad.org>
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
#include <string.h>
#include <math.h>

#include "pangofc-private.h"
#include <hb-ft.h>
#include <hb-glib.h>

/* cache a single hb_buffer_t */
static hb_buffer_t *cached_buffer = NULL; /* MT-safe */
G_LOCK_DEFINE_STATIC (cached_buffer);

static hb_buffer_t *
acquire_buffer (gboolean *free_buffer)
{
  hb_buffer_t *buffer;

  if (G_LIKELY (G_TRYLOCK (cached_buffer)))
    {
      if (G_UNLIKELY (!cached_buffer))
	cached_buffer = hb_buffer_create ();

      buffer = cached_buffer;
      *free_buffer = FALSE;
    }
  else
    {
      buffer = hb_buffer_create ();
      *free_buffer = TRUE;
    }

  return buffer;
}

static void
release_buffer (hb_buffer_t *buffer, gboolean free_buffer)
{
  if (G_LIKELY (!free_buffer))
    {
      hb_buffer_reset (buffer);
      G_UNLOCK (cached_buffer);
    }
  else
    hb_buffer_destroy (buffer);
}

typedef struct _PangoFcHbContext {
  FT_Face ft_face;
  PangoFcFont *fc_font;
  gboolean vertical;
  double x_scale, y_scale; /* CTM scales. */
} PangoFcHbContext;

static hb_bool_t
pango_fc_hb_font_get_nominal_glyph (hb_font_t *font, void *font_data,
				    hb_codepoint_t unicode,
				    hb_codepoint_t *glyph,
				    void *user_data G_GNUC_UNUSED)
{
  PangoFcHbContext *context = (PangoFcHbContext *) font_data;
  PangoFcFont *fc_font = context->fc_font;

  *glyph = pango_fc_font_get_glyph (fc_font, unicode);
  if (G_LIKELY (*glyph))
    return TRUE;

  *glyph = PANGO_GET_UNKNOWN_GLYPH (unicode);

  /* We draw our own invalid-Unicode shape, so prevent HarfBuzz
   * from using REPLACEMENT CHARACTER. */
  if (unicode > 0x10FFFF)
    return TRUE;

  return FALSE;
}

static hb_bool_t
pango_fc_hb_font_get_variation_glyph (hb_font_t *font,
                                      void *font_data,
                                      hb_codepoint_t unicode,
                                      hb_codepoint_t variation_selector,
                                      hb_codepoint_t *glyph,
                                      void *user_data G_GNUC_UNUSED)
{
  PangoFcHbContext *context = (PangoFcHbContext *) font_data;
  FT_Face ft_face = context->ft_face;
  unsigned int g;

  g = FT_Face_GetCharVariantIndex (ft_face, unicode, variation_selector);

  if (G_UNLIKELY (!g))
    return FALSE;

  *glyph = g;
  return TRUE;
}

static hb_bool_t
pango_fc_hb_font_get_glyph_contour_point (hb_font_t *font, void *font_data,
					  hb_codepoint_t glyph, unsigned int point_index,
					  hb_position_t *x, hb_position_t *y,
					  void *user_data G_GNUC_UNUSED)
{
  return FALSE;
#if 0
  FT_Face ft_face = (FT_Face) font_data;
  int load_flags = FT_LOAD_DEFAULT;

  /* TODO: load_flags, embolden, etc */

  if (HB_UNLIKELY (FT_Load_Glyph (ft_face, glyph, load_flags)))
      return FALSE;

  if (HB_UNLIKELY (ft_face->glyph->format != FT_GLYPH_FORMAT_OUTLINE))
      return FALSE;

  if (HB_UNLIKELY (point_index >= (unsigned int) ft_face->glyph->outline.n_points))
      return FALSE;

  *x = ft_face->glyph->outline.points[point_index].x;
  *y = ft_face->glyph->outline.points[point_index].y;

  return TRUE;
#endif
}

static hb_position_t
pango_fc_hb_font_get_glyph_advance (hb_font_t *font, void *font_data,
				    hb_codepoint_t glyph,
				    void *user_data G_GNUC_UNUSED)
{
  PangoFcHbContext *context = (PangoFcHbContext *) font_data;
  PangoFcFont *fc_font = context->fc_font;
  PangoRectangle logical;

  pango_font_get_glyph_extents ((PangoFont *) fc_font, glyph, NULL, &logical);

  return logical.width;
}

static hb_bool_t
pango_fc_hb_font_get_glyph_extents (hb_font_t *font,  void *font_data,
				    hb_codepoint_t glyph,
				    hb_glyph_extents_t *extents,
				    void *user_data G_GNUC_UNUSED)
{
  PangoFcHbContext *context = (PangoFcHbContext *) font_data;
  PangoFcFont *fc_font = context->fc_font;
  PangoRectangle ink;

  pango_font_get_glyph_extents ((PangoFont *) fc_font, glyph, &ink, NULL);

  if (G_LIKELY (!context->vertical)) {
    extents->x_bearing  = ink.x;
    extents->y_bearing  = ink.y;
    extents->width      = ink.width;
    extents->height     = ink.height;
  } else {
    /* XXX */
    extents->x_bearing  = ink.x;
    extents->y_bearing  = ink.y;
    extents->width      = ink.height;
    extents->height     = ink.width;
  }

  return TRUE;
}

static hb_bool_t
pango_fc_hb_font_get_glyph_h_origin (hb_font_t *font, void *font_data,
				     hb_codepoint_t glyph,
				     hb_position_t *x, hb_position_t *y,
				     void *user_data G_GNUC_UNUSED)
{
  PangoFcHbContext *context = (PangoFcHbContext *) font_data;
  FT_Face ft_face = context->ft_face;
  int load_flags = FT_LOAD_DEFAULT;

  if (!context->vertical) return TRUE;

  if (FT_Load_Glyph (ft_face, glyph, load_flags))
    return FALSE;

  /* Note: FreeType's vertical metrics grows downward while other FreeType coordinates
   * have a Y growing upward.  Hence the extra negations. */
  *x = -PANGO_UNITS_26_6 (ft_face->glyph->metrics.horiBearingX -   ft_face->glyph->metrics.vertBearingX);
  *y = +PANGO_UNITS_26_6 (ft_face->glyph->metrics.horiBearingY - (-ft_face->glyph->metrics.vertBearingY));

  return TRUE;
}

static hb_bool_t
pango_fc_hb_font_get_glyph_v_origin (hb_font_t *font, void *font_data,
				     hb_codepoint_t glyph,
				     hb_position_t *x, hb_position_t *y,
				     void *user_data G_GNUC_UNUSED)
{
  PangoFcHbContext *context = (PangoFcHbContext *) font_data;
  FT_Face ft_face = context->ft_face;
  int load_flags = FT_LOAD_DEFAULT;

  /* pangocairo-fc configures font in vertical origin for vertical writing. */
  if (context->vertical) return TRUE;

  if (FT_Load_Glyph (ft_face, glyph, load_flags))
    return FALSE;

  /* Note: FreeType's vertical metrics grows downward while other FreeType coordinates
   * have a Y growing upward.  Hence the extra negation. */
  *x = PANGO_UNITS_26_6 (ft_face->glyph->metrics.horiBearingX -   ft_face->glyph->metrics.vertBearingX);
  *y = PANGO_UNITS_26_6 (ft_face->glyph->metrics.horiBearingY - (-ft_face->glyph->metrics.vertBearingY));

  /* XXX */

  return TRUE;
}


static hb_position_t
pango_fc_hb_font_get_h_kerning (hb_font_t *font, void *font_data,
				hb_codepoint_t left_glyph, hb_codepoint_t right_glyph,
				void *user_data G_GNUC_UNUSED)
{
  PangoFcHbContext *context = (PangoFcHbContext *) font_data;
  FT_Face ft_face = context->ft_face;
  FT_Vector kerning;

  if (FT_Get_Kerning (ft_face, left_glyph, right_glyph, FT_KERNING_DEFAULT, &kerning))
    return 0;

  return PANGO_UNITS_26_6 (kerning.x * context->x_scale);
}

static hb_font_funcs_t *
pango_fc_get_hb_font_funcs (void)
{
  static hb_font_funcs_t *funcs;

  if (G_UNLIKELY (!funcs)) {
    funcs = hb_font_funcs_create ();
    hb_font_funcs_set_nominal_glyph_func (funcs, pango_fc_hb_font_get_nominal_glyph, NULL, NULL);
    hb_font_funcs_set_variation_glyph_func (funcs, pango_fc_hb_font_get_variation_glyph, NULL, NULL);
    hb_font_funcs_set_glyph_h_advance_func (funcs, pango_fc_hb_font_get_glyph_advance, NULL, NULL);
    hb_font_funcs_set_glyph_v_advance_func (funcs, pango_fc_hb_font_get_glyph_advance, NULL, NULL);
    hb_font_funcs_set_glyph_h_origin_func (funcs, pango_fc_hb_font_get_glyph_h_origin, NULL, NULL);
    hb_font_funcs_set_glyph_v_origin_func (funcs, pango_fc_hb_font_get_glyph_v_origin, NULL, NULL);
    hb_font_funcs_set_glyph_h_kerning_func (funcs, pango_fc_hb_font_get_h_kerning, NULL, NULL);
    /* Don't need v_kerning. */
    hb_font_funcs_set_glyph_extents_func (funcs, pango_fc_hb_font_get_glyph_extents, NULL, NULL);
    hb_font_funcs_set_glyph_contour_point_func (funcs, pango_fc_hb_font_get_glyph_contour_point, NULL, NULL);
    /* Don't need glyph_name / glyph_from_name */
  }

  return funcs;
}

static void
parse_variations (const char      *variations,
                  hb_variation_t **hb_variations,
                  guint           *n_variations)
{
  guint n;
  hb_variation_t *var;
  int i;
  const char *p;

  n = 1;
  for (i = 0; variations[i]; i++)
    {
      if (variations[i] == ',')
        n++;
    }

  var = g_new (hb_variation_t, n);

  p = variations;
  n = 0;
  while (p && *p)
    {
      char *end = strchr (p, ',');
      if (hb_variation_from_string (p, end ? end - p: -1, &var[n]))
        n++;
      p = end ? end + 1 : NULL;
    }

  *hb_variations = var;
  *n_variations = n;
}

void
_pango_fc_shape (PangoFont           *font,
		 const char          *item_text,
		 unsigned int         item_length,
		 const PangoAnalysis *analysis,
		 PangoGlyphString    *glyphs,
		 const char          *paragraph_text,
		 unsigned int         paragraph_length)
{
  PangoFcHbContext context;
  PangoFcFont *fc_font;
  PangoFcFontKey *key;
  FT_Face ft_face;
  hb_face_t *hb_face;
  hb_font_t *hb_font;
  hb_buffer_t *hb_buffer;
  hb_direction_t hb_direction;
  gboolean free_buffer;
  hb_glyph_info_t *hb_glyph;
  hb_glyph_position_t *hb_position;
  int last_cluster;
  guint i, num_glyphs;
  unsigned int item_offset = item_text - paragraph_text;
  hb_feature_t features[32];
  unsigned int num_features = 0;
  double x_scale_inv, y_scale_inv;
  PangoGlyphInfo *infos;
  const char *variations;

  g_return_if_fail (font != NULL);
  g_return_if_fail (analysis != NULL);

  fc_font = PANGO_FC_FONT (font);
  ft_face = pango_fc_font_lock_face (fc_font);
  if (!ft_face)
    return;

  /* TODO: Cache hb_font? */

  x_scale_inv = y_scale_inv = 1.0;
  key = _pango_fc_font_get_font_key (fc_font);
  if (key)
  {
    const PangoMatrix *matrix = pango_fc_font_key_get_matrix (key);
    pango_matrix_get_font_scale_factors (matrix, &x_scale_inv, &y_scale_inv);
  }
  if (PANGO_GRAVITY_IS_IMPROPER (analysis->gravity))
  {
    x_scale_inv = -x_scale_inv;
    y_scale_inv = -y_scale_inv;
  }
  context.x_scale = 1. / x_scale_inv;
  context.y_scale = 1. / y_scale_inv;
  context.ft_face = ft_face;
  context.fc_font = fc_font;
  context.vertical = PANGO_GRAVITY_IS_VERTICAL (analysis->gravity);
  hb_face = hb_ft_face_create_cached (ft_face);
  hb_font = hb_font_create (hb_face);
  hb_font_set_funcs (hb_font,
		     pango_fc_get_hb_font_funcs (),
		     &context,
		     NULL);
  hb_font_set_scale (hb_font,
		     +(((gint64) ft_face->size->metrics.x_scale * ft_face->units_per_EM) >> 12) * context.x_scale,
		     -(((gint64) ft_face->size->metrics.y_scale * ft_face->units_per_EM) >> 12) * context.y_scale);
  hb_font_set_ppem (hb_font,
		    fc_font->is_hinted ? ft_face->size->metrics.x_ppem : 0,
		    fc_font->is_hinted ? ft_face->size->metrics.y_ppem : 0);

  if (key)
  {
    variations = pango_fc_font_key_get_variations (key);
    if (variations)
    {
      guint n_variations;
      hb_variation_t *hb_variations;

      parse_variations (variations, &hb_variations, &n_variations);
      hb_font_set_variations (hb_font, hb_variations, n_variations);

      g_free (hb_variations);
    }
  }

  hb_buffer = acquire_buffer (&free_buffer);

  hb_direction = PANGO_GRAVITY_IS_VERTICAL (analysis->gravity) ? HB_DIRECTION_TTB : HB_DIRECTION_LTR;
  if (analysis->level % 2)
    hb_direction = HB_DIRECTION_REVERSE (hb_direction);
  if (PANGO_GRAVITY_IS_IMPROPER (analysis->gravity))
    hb_direction = HB_DIRECTION_REVERSE (hb_direction);

  /* setup buffer */

  hb_buffer_set_direction (hb_buffer, hb_direction);
  hb_buffer_set_script (hb_buffer, hb_glib_script_to_script (analysis->script));
  hb_buffer_set_language (hb_buffer, hb_language_from_string (pango_language_to_string (analysis->language), -1));
#if HB_VERSION_ATLEAST(1,0,3)
  hb_buffer_set_cluster_level (hb_buffer, HB_BUFFER_CLUSTER_LEVEL_MONOTONE_CHARACTERS);
#endif
  hb_buffer_set_flags (hb_buffer, HB_BUFFER_FLAG_BOT | HB_BUFFER_FLAG_EOT);

  hb_buffer_add_utf8 (hb_buffer, paragraph_text, paragraph_length, item_offset, item_length);

  /* Setup features from fontconfig pattern. */
  if (fc_font->font_pattern)
    {
      char *s;
      while (num_features < G_N_ELEMENTS (features) &&
	     FcResultMatch == FcPatternGetString (fc_font->font_pattern,
						  PANGO_FC_FONT_FEATURES,
						  num_features,
						  (FcChar8 **) &s))
	{
	  gboolean ret = hb_feature_from_string (s, -1, &features[num_features]);
	  features[num_features].start = 0;
	  features[num_features].end   = (unsigned int) -1;
	  if (ret)
	    num_features++;
	}
    }

  if (analysis->extra_attrs)
    {
      GSList *tmp_attrs;

      for (tmp_attrs = analysis->extra_attrs; tmp_attrs && num_features < G_N_ELEMENTS (features); tmp_attrs = tmp_attrs->next)
       {
	 if (((PangoAttribute *) tmp_attrs->data)->klass->type == PANGO_ATTR_FONT_FEATURES)
	   {
	     const PangoAttrFontFeatures *fattr = (const PangoAttrFontFeatures *) tmp_attrs->data;
	      const gchar *feat;
	      const gchar *end;
	      int len;

	      feat = fattr->features;

	      while (feat != NULL && num_features < G_N_ELEMENTS (features))
		{
		  end = strchr (feat, ',');
		  if (end)
		    len = end - feat;
		  else
		    len = -1;

		  if (hb_feature_from_string (feat, len, &features[num_features]))
		    num_features++;

		  if (end == NULL)
		    break;

		  feat = end + 1;
		}
	   }
       }
    }

  hb_shape (hb_font, hb_buffer, features, num_features);

  if (PANGO_GRAVITY_IS_IMPROPER (analysis->gravity))
    hb_buffer_reverse (hb_buffer);

  /* buffer output */
  num_glyphs = hb_buffer_get_length (hb_buffer);
  hb_glyph = hb_buffer_get_glyph_infos (hb_buffer, NULL);
  pango_glyph_string_set_size (glyphs, num_glyphs);
  infos = glyphs->glyphs;
  last_cluster = -1;
  for (i = 0; i < num_glyphs; i++)
    {
      infos[i].glyph = hb_glyph->codepoint;
      glyphs->log_clusters[i] = hb_glyph->cluster - item_offset;
      infos[i].attr.is_cluster_start = glyphs->log_clusters[i] != last_cluster;
      hb_glyph++;
      last_cluster = glyphs->log_clusters[i];
    }

  hb_position = hb_buffer_get_glyph_positions (hb_buffer, NULL);
  if (context.vertical)
    for (i = 0; i < num_glyphs; i++)
      {
        /* 90 degrees rotation counter-clockwise. */
	infos[i].geometry.width    =  hb_position->y_advance;
	infos[i].geometry.x_offset =  hb_position->y_offset;
	infos[i].geometry.y_offset = -hb_position->x_offset;
	hb_position++;
      }
  else /* horizontal */
    for (i = 0; i < num_glyphs; i++)
      {
	infos[i].geometry.width    = hb_position->x_advance;
	infos[i].geometry.x_offset = hb_position->x_offset;
	infos[i].geometry.y_offset = hb_position->y_offset;
	hb_position++;
      }

  if (fc_font->is_hinted)
  {
    if (context.x_scale == 1.0 && context.y_scale == 1.0)
      {
	for (i = 0; i < num_glyphs; i++)
	  infos[i].geometry.width = PANGO_UNITS_ROUND (infos[i].geometry.width);
      }
    else
      {
#if 0
	if (context.vertical)
	  {
	    /* XXX */
	    double tmp = x_scale;
	    x_scale = y_scale;
	    y_scale = -tmp;
	  }
#endif
#define HINT(value, scale_inv, scale) (PANGO_UNITS_ROUND ((int) ((value) * scale)) * scale_inv)
#define HINT_X(value) HINT ((value), context.x_scale, x_scale_inv)
#define HINT_Y(value) HINT ((value), context.y_scale, y_scale_inv)
	for (i = 0; i < num_glyphs; i++)
	  {
	    infos[i].geometry.width    = HINT_X (infos[i].geometry.width);
	    infos[i].geometry.x_offset = HINT_X (infos[i].geometry.x_offset);
	    infos[i].geometry.y_offset = HINT_Y (infos[i].geometry.y_offset);
	  }
#undef HINT_Y
#undef HINT_X
#undef HINT
      }
  }

  release_buffer (hb_buffer, free_buffer);
  hb_font_destroy (hb_font);
  hb_face_destroy (hb_face);
  pango_fc_font_unlock_face (fc_font);
}
