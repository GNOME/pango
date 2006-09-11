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

#include <config.h>

#include <math.h>
#include <stdlib.h>

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

typedef struct _GlyphExtentsCacheEntry    GlyphExtentsCacheEntry;

#define GLYPH_CACHE_NUM_ENTRIES 256 /* should be power of two */
#define GLYPH_CACHE_MASK (GLYPH_CACHE_NUM_ENTRIES - 1)

#define PANGO_UNITS(Double) ((int)((Double) * PANGO_SCALE + 0.49999))

/* An entry in the fixed-size cache for the glyph -> ink_rect mapping.
 * The cache is indexed by the lower N bits of the glyph (see
 * GLYPH_CACHE_NUM_ENTRIES).  For scripts with few glyphs,
 * this should provide pretty much instant lookups.
 */
struct _GlyphExtentsCacheEntry
{
  PangoGlyph     glyph;
  int            width;
  PangoRectangle ink_rect;
};

struct _PangoCairoFcFont
{
  PangoFcFont font;

  cairo_font_face_t *font_face;
  cairo_scaled_font_t *scaled_font;
  cairo_matrix_t font_matrix;
  cairo_matrix_t ctm;
  cairo_font_options_t *options;

  PangoRectangle font_extents;
  GlyphExtentsCacheEntry    *glyph_extents_cache;  
};

struct _PangoCairoFcFontClass
{
  PangoFcFontClass  parent_class;
};

GType pango_cairo_fc_font_get_type (void);

static cairo_font_face_t *pango_cairo_fc_font_get_font_face (PangoCairoFont *font);

/*******************************
 *       Utility functions     *
 *******************************/

static cairo_font_face_t *
pango_cairo_fc_font_get_font_face (PangoCairoFont *font)
{
  PangoCairoFcFont *cffont = PANGO_CAIRO_FC_FONT (font);
  PangoFcFont *fcfont = PANGO_FC_FONT (cffont);

  if (!cffont->font_face)
    {
      cffont->font_face = cairo_ft_font_face_create_for_pattern (fcfont->font_pattern);
      
      /* Unable to create FT2 cairo scaled font.
       * This means out of memory or a cairo/fontconfig/FreeType bug,
       */
      if (!cffont->font_face)
        return NULL;
    }
  
  return cffont->font_face;
}

static cairo_scaled_font_t *
pango_cairo_fc_font_get_scaled_font (PangoCairoFont *font)
{
  PangoCairoFcFont *cffont = (PangoCairoFcFont *)font;

  if (!cffont->scaled_font)
    {
      cairo_font_face_t *font_face;

      font_face = pango_cairo_fc_font_get_font_face (font);

      if (!font_face)
        return NULL;

      cffont->scaled_font = cairo_scaled_font_create (font_face,
						      &cffont->font_matrix,
						      &cffont->ctm,
						      cffont->options);

      /* Unable to create FT2 cairo scaled font.
       * This means out of memory or a cairo/fontconfig/FreeType bug,
       * or a missing font...
       */
      if (!cffont->scaled_font)
        return NULL;
    }
  
  return cffont->scaled_font;
}

/********************************
 *    Method implementations    *
 ********************************/

static gboolean
pango_cairo_fc_font_install (PangoCairoFont *font,
			     cairo_t        *cr)
{
  PangoCairoFcFont *cffont = PANGO_CAIRO_FC_FONT (font);

  cairo_set_font_face (cr,
		       pango_cairo_fc_font_get_font_face (font));
  cairo_set_font_matrix (cr, &cffont->font_matrix);
  cairo_set_font_options (cr, cffont->options);

  return TRUE;
}

static void
cairo_font_iface_init (PangoCairoFontIface *iface)
{
  iface->install = pango_cairo_fc_font_install;
  iface->get_font_face = pango_cairo_fc_font_get_font_face;
  iface->get_scaled_font = pango_cairo_fc_font_get_scaled_font;
}

G_DEFINE_TYPE_WITH_CODE (PangoCairoFcFont, pango_cairo_fc_font, PANGO_TYPE_FC_FONT,
    { G_IMPLEMENT_INTERFACE (PANGO_TYPE_CAIRO_FONT, cairo_font_iface_init) })

static void
pango_cairo_fc_font_finalize (GObject *object)
{
  PangoCairoFcFont *cffont = PANGO_CAIRO_FC_FONT (object);

  if (cffont->font_face)
    cairo_font_face_destroy (cffont->font_face);
  if (cffont->scaled_font)
    cairo_scaled_font_destroy (cffont->scaled_font);
  if (cffont->options)
    cairo_font_options_destroy (cffont->options);


  if (cffont->glyph_extents_cache)
    {
      g_free (cffont->glyph_extents_cache);
    }  

  G_OBJECT_CLASS (pango_cairo_fc_font_parent_class)->finalize (object);
}

/* This function is cut-and-pasted from pangocairo-fcfont.c - it might be
 * better to add a virtual fcfont->create_context (font).
 */
static PangoFontMetrics *
pango_cairo_fc_font_get_metrics (PangoFont     *font,
				 PangoLanguage *language)
{
  PangoFcFont *fcfont = PANGO_FC_FONT (font);
  PangoCairoFcFont *cffont = PANGO_CAIRO_FC_FONT (font);
  PangoFcMetricsInfo *info = NULL; /* Quiet gcc */
  GSList *tmp_list;      

  const char *sample_str = pango_language_get_sample_string (language);
  
  tmp_list = fcfont->metrics_by_lang;
  while (tmp_list)
    {
      info = tmp_list->data;
      
      if (info->sample_str == sample_str)    /* We _don't_ need strcmp */
	break;

      tmp_list = tmp_list->next;
    }

  if (!tmp_list)
    {
      PangoContext *context;

      if (!fcfont->fontmap)
	return pango_font_metrics_new ();

      info = g_slice_new0 (PangoFcMetricsInfo);
      
      fcfont->metrics_by_lang = g_slist_prepend (fcfont->metrics_by_lang, info);
	
      info->sample_str = sample_str;

      context = pango_fc_font_map_create_context (PANGO_FC_FONT_MAP (fcfont->fontmap));
      pango_context_set_language (context, language);
      pango_cairo_context_set_font_options (context, cffont->options);

      info->metrics = pango_fc_font_create_metrics_for_context (fcfont, context);

      g_object_unref (context);
    }

  return pango_font_metrics_ref (info->metrics);
}

static FT_Face
pango_cairo_fc_font_lock_face (PangoFcFont *font)
{
  PangoCairoFont *cfont = (PangoCairoFont *)font;
  cairo_scaled_font_t *scaled_font = pango_cairo_fc_font_get_scaled_font (cfont);
  if (G_UNLIKELY (!scaled_font))
    return NULL;
  
  return cairo_ft_scaled_font_lock_face (scaled_font);
}

static void
pango_cairo_fc_font_unlock_face (PangoFcFont *font)
{
  PangoCairoFont *cfont = (PangoCairoFont *)font;
  cairo_scaled_font_t *scaled_font = pango_cairo_fc_font_get_scaled_font (cfont);
  if (G_UNLIKELY (!scaled_font))
    return;
  
  cairo_ft_scaled_font_unlock_face (scaled_font);
}

static void
pango_cairo_fc_font_glyph_extents_cache_init (PangoCairoFcFont *cffont)
{
  PangoCairoFont *cfont = (PangoCairoFont *)cffont;
  cairo_scaled_font_t *scaled_font = pango_cairo_fc_font_get_scaled_font (cfont);
  cairo_font_extents_t font_extents;

  cairo_scaled_font_extents (scaled_font, &font_extents);

  cffont->font_extents.x = 0;
  cffont->font_extents.y = - PANGO_UNITS (font_extents.ascent);
  cffont->font_extents.height = PANGO_UNITS (font_extents.ascent + font_extents.descent);
  cffont->font_extents.width = 0;

  cffont->glyph_extents_cache = g_new0 (GlyphExtentsCacheEntry, GLYPH_CACHE_NUM_ENTRIES);
  /* Make sure all cache entries are invalid initially */
  cffont->glyph_extents_cache[0].glyph = 1; /* glyph 1 cannot happen in bucket 0 */
}

/* Fills in the glyph extents cache entry
 */
static void
compute_glyph_extents (PangoCairoFcFont       *cffont,
		       PangoGlyph              glyph,
		       GlyphExtentsCacheEntry *entry)
{
  cairo_text_extents_t extents;
  cairo_glyph_t cairo_glyph;

  cairo_glyph.index = glyph;
  cairo_glyph.x = 0;
  cairo_glyph.y = 0;
  
  cairo_scaled_font_glyph_extents (cffont->scaled_font,
				   &cairo_glyph, 1, &extents);
  
  entry->glyph = glyph;
  entry->width = PANGO_UNITS (extents.x_advance);
  entry->ink_rect.x = PANGO_UNITS (extents.x_bearing);
  entry->ink_rect.y = PANGO_UNITS (extents.y_bearing);
  entry->ink_rect.width = PANGO_UNITS (extents.width);
  entry->ink_rect.height = PANGO_UNITS (extents.height);
}
     
static GlyphExtentsCacheEntry *
pango_cairo_fc_font_get_glyph_extents_cache_entry (PangoCairoFcFont       *cffont,
						   PangoGlyph              glyph)
{
  GlyphExtentsCacheEntry *entry;
  guint idx;

  idx = glyph & GLYPH_CACHE_MASK;
  entry = cffont->glyph_extents_cache + idx;

  if (entry->glyph != glyph)
    {
      compute_glyph_extents (cffont, glyph, entry);
    }

  return entry;
}

static void
pango_cairo_fc_font_get_glyph_extents (PangoFont      *font,
				       PangoGlyph      glyph,
				       PangoRectangle *ink_rect,
				       PangoRectangle *logical_rect)
{
  PangoCairoFcFont *cffont = (PangoCairoFcFont *)font;
  GlyphExtentsCacheEntry *entry;

  if (!pango_cairo_fc_font_get_scaled_font ((PangoCairoFont *)cffont))
    {
      /* Get generic unknown-glyph extents. */
      pango_font_get_glyph_extents (NULL, glyph, ink_rect, logical_rect);
      return;
    }

  /* We need to initialize the cache here, since we use cffont->font_extents
   */
  if (cffont->glyph_extents_cache == NULL)
    pango_cairo_fc_font_glyph_extents_cache_init (cffont);

  if (glyph == PANGO_GLYPH_EMPTY)
    {
      if (ink_rect)
	ink_rect->x = ink_rect->y = ink_rect->width = ink_rect->height = 0;
      if (logical_rect)
	*logical_rect = cffont->font_extents;
      return;
    }
  else if (glyph & PANGO_GLYPH_UNKNOWN_FLAG) 
    {
      _pango_cairo_get_glyph_extents_missing((PangoCairoFont *)font, glyph, ink_rect, logical_rect);
      return;
    }

  entry = pango_cairo_fc_font_get_glyph_extents_cache_entry (cffont, glyph);

  if (ink_rect)
    *ink_rect = entry->ink_rect;
  if (logical_rect)
    {
      *logical_rect = cffont->font_extents;
      logical_rect->width = entry->width;
    }
}

static void
pango_cairo_fc_font_shutdown (PangoFcFont *fcfont)
{
  PangoCairoFcFont *cffont = PANGO_CAIRO_FC_FONT (fcfont);
  if (cffont->scaled_font)
    {
      cairo_scaled_font_destroy (cffont->scaled_font);
      cffont->scaled_font = NULL;
    }
  if (cffont->font_face)
    {
      cairo_font_face_destroy (cffont->font_face);
      cffont->font_face = NULL;
    }
}

static void
pango_cairo_fc_font_class_init (PangoCairoFcFontClass *class)
{
  GObjectClass *object_class = G_OBJECT_CLASS (class);
  PangoFontClass *font_class = PANGO_FONT_CLASS (class);
  PangoFcFontClass *fc_font_class = PANGO_FC_FONT_CLASS (class);

  object_class->finalize = pango_cairo_fc_font_finalize;
  
  font_class->get_glyph_extents = pango_cairo_fc_font_get_glyph_extents;
  font_class->get_metrics = pango_cairo_fc_font_get_metrics;

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
	       FcPattern                  *pattern,
	       const PangoMatrix          *matrix)
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
    return size * PANGO_SCALE / pango_matrix_get_font_scale_factor (matrix);

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
  const PangoMatrix *pango_ctm;
  FcMatrix *fc_matrix;
  double size;
  
  g_return_val_if_fail (PANGO_IS_CAIRO_FC_FONT_MAP (cffontmap), NULL);
  g_return_val_if_fail (pattern != NULL, NULL);

  cffont = g_object_new (PANGO_TYPE_CAIRO_FC_FONT,
			 "pattern", pattern,
			 NULL);

  if  (FcPatternGetMatrix (pattern,
			   FC_MATRIX, 0, &fc_matrix) == FcResultMatch)
    {
      cairo_matrix_init (&cffont->font_matrix,
			 fc_matrix->xx,
			 - fc_matrix->yx,
			 - fc_matrix->xy,
			 fc_matrix->yy,
			 0., 0.);
    }
  else
    cairo_matrix_init_identity (&cffont->font_matrix);

  pango_ctm = pango_context_get_matrix (context);

  size = get_font_size (cffontmap, context, desc, pattern, pango_ctm);

  cairo_matrix_scale (&cffont->font_matrix,
		      size / PANGO_SCALE, size / PANGO_SCALE);

  if (pango_ctm)
    cairo_matrix_init (&cffont->ctm,
		       pango_ctm->xx,
		       pango_ctm->yx,
		       pango_ctm->xy,
		       pango_ctm->yy,
		       0., 0.);
  else
    cairo_matrix_init_identity (&cffont->ctm);


  cffont->options = cairo_font_options_copy (_pango_cairo_context_get_merged_font_options (context));
  
  /* fcfont's is_hinted controls metric hinting
   */
  PANGO_FC_FONT(cffont)->is_hinted = 
    (cairo_font_options_get_hint_metrics(cffont->options) != CAIRO_HINT_METRICS_OFF);

  return PANGO_FC_FONT (cffont);
}
