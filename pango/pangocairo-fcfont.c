/* Pango
 * pangocairofc-font.c: Cairo font handling, fontconfig backend
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

#include <cairo-ft.h>

#include "pango-fontmap.h"
#include "pangocairo-private.h"
#include "pangocairo-fc.h"
#include "pangofc-private.h"
#include "pango-impl-utils.h"

#define PANGO_TYPE_CAIRO_FC_FONT           (pango_cairo_fc_font_get_type ())
#define PANGO_CAIRO_FC_FONT(object)        (G_TYPE_CHECK_INSTANCE_CAST ((object), PANGO_TYPE_CAIRO_FC_FONT, PangoCairoFcFont))
#define PANGO_CAIRO_FC_FONT_CLASS(klass)   (G_TYPE_CHECK_CLASS_CAST ((klass), PANGO_TYPE_CAIRO_FC_FONT, PangoCairoFcFontClass))
#define PANGO_CAIRO_IS_FONT_CLASS(klass)   (G_TYPE_CHECK_CLASS_TYPE ((klass), PANGO_TYPE_CAIRO_FC_FONT))
#define PANGO_CAIRO_FC_FONT_GET_CLASS(obj) (G_TYPE_INSTANCE_GET_CLASS ((obj), PANGO_TYPE_CAIRO_FC_FONT, PangoCairoFcFontClass))

typedef struct _PangoCairoFcFont      PangoCairoFcFont;
typedef struct _PangoCairoFcFontClass PangoCairoFcFontClass;

struct _PangoCairoFcFont
{
  PangoFcFont font;
  PangoCairoFontPrivate cf_priv;

  PangoGravity gravity;
};

struct _PangoCairoFcFontClass
{
  PangoFcFontClass  parent_class;
};

GType pango_cairo_fc_font_get_type (void);

/********************************
 *    Method implementations    *
 ********************************/

static cairo_font_face_t *
pango_cairo_fc_font_create_font_face (PangoCairoFont *cfont)
{
  PangoFcFont *fcfont = (PangoFcFont *) (cfont);

  return cairo_ft_font_face_create_for_pattern (fcfont->font_pattern);
}

static PangoFontMetrics *
pango_cairo_fc_font_create_metrics_for_context (PangoCairoFont *cfont,
					        PangoContext   *context)
{
  PangoFcFont *fcfont = (PangoFcFont *) (cfont);

  return pango_fc_font_create_metrics_for_context (fcfont, context);
}

static void
cairo_font_iface_init (PangoCairoFontIface *iface)
{
  iface->create_font_face = pango_cairo_fc_font_create_font_face;
  iface->create_metrics_for_context = pango_cairo_fc_font_create_metrics_for_context;
  iface->cf_priv_offset = G_STRUCT_OFFSET (PangoCairoFcFont, cf_priv);
}

G_DEFINE_TYPE_WITH_CODE (PangoCairoFcFont, pango_cairo_fc_font, PANGO_TYPE_FC_FONT,
    { G_IMPLEMENT_INTERFACE (PANGO_TYPE_CAIRO_FONT, cairo_font_iface_init) })

static void
pango_cairo_fc_font_finalize (GObject *object)
{
  PangoCairoFcFont *cffont = (PangoCairoFcFont *) (object);

  _pango_cairo_font_private_finalize (&cffont->cf_priv);

  G_OBJECT_CLASS (pango_cairo_fc_font_parent_class)->finalize (object);
}

/* we want get_glyph_extents extremely fast, so we use a small wrapper here
 * to avoid having to lookup the interface data like we do for get_metrics
 * in _pango_cairo_font_get_metrics(). */
static void
pango_cairo_fc_font_get_glyph_extents (PangoFont      *font,
				       PangoGlyph      glyph,
				       PangoRectangle *ink_rect,
				       PangoRectangle *logical_rect)
{
  PangoCairoFcFont *cffont = (PangoCairoFcFont *) (font);

  _pango_cairo_font_private_get_glyph_extents (&cffont->cf_priv,
					       glyph,
					       ink_rect,
					       logical_rect);
}

static FT_Face
pango_cairo_fc_font_lock_face (PangoFcFont *font)
{
  PangoCairoFcFont *cffont = (PangoCairoFcFont *) (font);
  cairo_scaled_font_t *scaled_font = _pango_cairo_font_private_get_scaled_font (&cffont->cf_priv);

  if (G_UNLIKELY (!scaled_font))
    return NULL;

  return cairo_ft_scaled_font_lock_face (scaled_font);
}

static void
pango_cairo_fc_font_unlock_face (PangoFcFont *font)
{
  PangoCairoFcFont *cffont = (PangoCairoFcFont *) (font);
  cairo_scaled_font_t *scaled_font = _pango_cairo_font_private_get_scaled_font (&cffont->cf_priv);

  if (G_UNLIKELY (!scaled_font))
    return;

  cairo_ft_scaled_font_unlock_face (scaled_font);
}

static void
pango_cairo_fc_font_shutdown (PangoFcFont *fcfont)
{
  PangoCairoFcFont *cffont = (PangoCairoFcFont *) (fcfont);

  _pango_cairo_font_private_finalize (&cffont->cf_priv);
}

static void
pango_cairo_fc_font_class_init (PangoCairoFcFontClass *class)
{
  GObjectClass *object_class = G_OBJECT_CLASS (class);
  PangoFontClass *font_class = PANGO_FONT_CLASS (class);
  PangoFcFontClass *fc_font_class = PANGO_FC_FONT_CLASS (class);

  object_class->finalize = pango_cairo_fc_font_finalize;

  font_class->get_glyph_extents = pango_cairo_fc_font_get_glyph_extents;
  font_class->get_metrics = _pango_cairo_font_get_metrics;

  fc_font_class->lock_face = pango_cairo_fc_font_lock_face;
  fc_font_class->unlock_face = pango_cairo_fc_font_unlock_face;
  fc_font_class->shutdown = pango_cairo_fc_font_shutdown;
}

static void
pango_cairo_fc_font_init (PangoCairoFcFont *cffont)
{
}

/********************
 *    Private API   *
 ********************/

static double
get_font_size (PangoCairoFcFontMap        *cffontmap,
	       PangoContext               *context,
	       const PangoFontDescription *desc,
	       FcPattern                  *pattern)
{
  double size;

 /* The reason why we read FC_PIXEL_SIZE here rather than just
  * using the specified size is to support operations like clamping
  * a font to a minimal readable size in fonts.conf. This is pretty weird,
  * since it could mean that changing the Cairo CTM doesn't change the
  * font size, but it's just a more radical version of the non-linear
  * font scaling we already have due to hinting and due to bitmap
  * fonts being only available at a few sizes.
  *
  * If honoring FC_PIXEL_SIZE gets in the way of more useful features
  * it should be removed since it only matters in the unusual case
  * of people doing exotic stuff in fonts.conf.
  */

  if (FcPatternGetDouble (pattern, FC_PIXEL_SIZE, 0, &size) == FcResultMatch)
    return size * PANGO_SCALE / pango_matrix_get_font_scale_factor (pango_context_get_matrix (context));

  /* Just in case FC_PIXEL_SIZE got unset between pango_fc_make_pattern()
   * and here.
   */
  if (pango_font_description_get_size_is_absolute (desc))
    return pango_font_description_get_size (desc);
  else
    {
      double dpi = pango_cairo_context_get_resolution (context);

      if (dpi <= 0)
	dpi = cffontmap->dpi;

      return dpi * pango_font_description_get_size (desc) / 72.;
    }
}

PangoFcFont *
_pango_cairo_fc_font_new (PangoCairoFcFontMap        *cffontmap,
			  PangoContext               *context,
			  const PangoFontDescription *desc,
			  FcPattern	             *pattern)
{
  PangoCairoFcFont *cffont;
  cairo_matrix_t font_matrix;
  FcMatrix *fc_matrix;
  double size;

  g_return_val_if_fail (PANGO_IS_CAIRO_FC_FONT_MAP (cffontmap), NULL);
  g_return_val_if_fail (pattern != NULL, NULL);

  cffont = g_object_new (PANGO_TYPE_CAIRO_FC_FONT,
			 "pattern", pattern,
			 NULL);

  size = get_font_size (cffontmap, context, desc, pattern);

  if  (FcPatternGetMatrix (pattern,
			   FC_MATRIX, 0, &fc_matrix) == FcResultMatch)
    cairo_matrix_init (&font_matrix,
		       fc_matrix->xx,
		       - fc_matrix->yx,
		       - fc_matrix->xy,
		       fc_matrix->yy,
		       0., 0.);
  else
    cairo_matrix_init_identity (&font_matrix);

  cairo_matrix_scale (&font_matrix,
		      size / PANGO_SCALE, size / PANGO_SCALE);

  _pango_cairo_font_private_initialize (&cffont->cf_priv,
					(PangoCairoFont *) cffont,
					context,
					desc,
					&font_matrix);

  ((PangoFcFont *)(cffont))->is_hinted = _pango_cairo_font_private_is_metrics_hinted (&cffont->cf_priv);

  return (PangoFcFont *) cffont;
}
