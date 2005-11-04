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

#include <math.h>
#include <stdlib.h>

#include <cairo-ft.h>

#include "pango-fontmap.h"
#include "pangocairo-private.h"
#include "pangocairo-fc.h"
#include "pangofc-private.h"
#include "pango-utils.h"

#define PANGO_TYPE_CAIRO_FC_FONT           (pango_cairo_fc_font_get_type ())
#define PANGO_CAIRO_FC_FONT(object)        (G_TYPE_CHECK_INSTANCE_CAST ((object), PANGO_TYPE_CAIRO_FC_FONT, PangoCairoFcFont))
#define PANGO_CAIRO_FC_FONT_CLASS(klass)   (G_TYPE_CHECK_CLASS_CAST ((klass), PANGO_TYPE_CAIRO_FC_FONT, PangoCairoFcFontClass))
#define PANGO_CAIRO_IS_FONT_CLASS(klass)   (G_TYPE_CHECK_CLASS_TYPE ((klass), PANGO_TYPE_CAIRO_FC_FONT))
#define PANGO_CAIRO_FC_FONT_GET_CLASS(obj) (G_TYPE_INSTANCE_GET_CLASS ((obj), PANGO_TYPE_CAIRO_FC_FONT, PangoCairoFcFontClass))

#define PANGO_CAIRO_UNKNOWN_FLAG 0x10000000

typedef struct _PangoCairoFcFont      PangoCairoFcFont;
typedef struct _PangoCairoFcFontClass PangoCairoFcFontClass;
typedef struct _PangoCairoFcGlyphInfo PangoCairoFcGlyphInfo;

#undef PROFILE_GLYPH_CACHE
#ifdef PROFILE_GLYPH_CACHE
static long num_cairo_fc_fonts;
static long max_cairo_fc_fonts;
static long num_glyph_caches;
static long max_glyph_caches;
static long num_glyph_cache_hits;
static long num_glyph_cache_misses;
#endif

#define GLYPH_CACHE_NUM_ENTRIES 256
#define GLYPH_CACHE_MASK 0x000000ff

/* An entry in the fixed-size cache for the gunichar -> glyph mapping.  The
 * cache is indexed by the lower N bits of the gunichar (see
 * GLYPH_CACHE_NUM_ENTRIES and GLYPH_CACHE_MASK above).  For scripts with few
 * code points, this should provide pretty much instant lookups.
 *
 * The "ch" field is zero if that cache entry is invalid.
 */
typedef struct
{
  gunichar ch;
  PangoGlyph glyph;
} GlyphCacheEntry;

struct _PangoCairoFcFont
{
  PangoFcFont font;

  cairo_font_face_t *font_face;
  cairo_scaled_font_t *scaled_font;
  cairo_matrix_t font_matrix;
  cairo_matrix_t ctm;
  cairo_font_options_t *options;

  GHashTable *glyph_info;

  GlyphCacheEntry *char_to_glyph_cache;
};

struct _PangoCairoFcFontClass
{
  PangoFcFontClass  parent_class;
};

struct _PangoCairoFcGlyphInfo
{
  PangoRectangle logical_rect;
  PangoRectangle ink_rect;
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
      
      /* Failure of the above should only occur for out of memory,
       * we can't proceed at that point
       */
      if (!cffont->font_face)
	g_error ("Unable create Cairo font name");
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

      cffont->scaled_font = cairo_scaled_font_create (font_face,
						      &cffont->font_matrix,
						      &cffont->ctm,
						      cffont->options);

      /* Failure of the above should only occur for out of memory,
       * we can't proceed at that point
       */
      if (!cffont->scaled_font)
	g_error ("Unable create Cairo font");
    }
  
  return cffont->scaled_font;
}

/********************************
 *    Method implementations    *
 ********************************/

static void
pango_cairo_fc_font_install (PangoCairoFont *font,
			     cairo_t        *cr)
{
  PangoCairoFcFont *cffont = PANGO_CAIRO_FC_FONT (font);

  cairo_set_font_face (cr,
		       pango_cairo_fc_font_get_font_face (font));
  cairo_set_font_matrix (cr, &cffont->font_matrix);
  cairo_set_font_options (cr, cffont->options);
}

static void
cairo_font_iface_init (PangoCairoFontIface *iface)
{
  iface->install = pango_cairo_fc_font_install;
}

G_DEFINE_TYPE_WITH_CODE (PangoCairoFcFont, pango_cairo_fc_font, PANGO_TYPE_FC_FONT,
    { G_IMPLEMENT_INTERFACE (PANGO_TYPE_CAIRO_FONT, cairo_font_iface_init) });

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

  g_hash_table_destroy (cffont->glyph_info);

  if (cffont->char_to_glyph_cache)
    {
      g_free (cffont->char_to_glyph_cache);
#ifdef PROFILE_GLYPH_CACHE
      num_glyph_caches--;
      g_assert (num_glyph_caches >= 0);
#endif
    }

  G_OBJECT_CLASS (pango_cairo_fc_font_parent_class)->finalize (object);

#ifdef PROFILE_GLYPH_CACHE
  num_cairo_fc_fonts--;
  g_assert (num_cairo_fc_fonts >= 0);
#endif
}

static void
compute_glyph_extents (PangoFont        *font,
		       PangoGlyph        glyph,
		       PangoRectangle   *ink_rect,
		       PangoRectangle   *logical_rect)
{
  PangoCairoFont *cfont = (PangoCairoFont *)font;
  cairo_scaled_font_t *scaled_font = pango_cairo_fc_font_get_scaled_font (cfont);

  /* It may well be worth caching the font_extents here, since getting them
   * is pretty expensive.
   */
  cairo_font_extents_t font_extents;
  cairo_text_extents_t extents;
  cairo_glyph_t cairo_glyph;
  
  cairo_scaled_font_extents (scaled_font, &font_extents);
  
  logical_rect->x = 0;
  logical_rect->y = - font_extents.ascent * PANGO_SCALE;
  logical_rect->width = 0;
  logical_rect->height = (font_extents.ascent + font_extents.descent) * PANGO_SCALE;

  if (glyph)
    {
      cairo_glyph.index = glyph;
      cairo_glyph.x = 0;
      cairo_glyph.y = 0;
      
      cairo_scaled_font_glyph_extents (scaled_font,
				       &cairo_glyph, 1, &extents);
      
      ink_rect->x = extents.x_bearing * PANGO_SCALE;
      ink_rect->y = extents.y_bearing * PANGO_SCALE;
      ink_rect->width = extents.width * PANGO_SCALE;
      ink_rect->height = extents.height * PANGO_SCALE;
      
      logical_rect->width = extents.x_advance * PANGO_SCALE;
    }
}

static PangoCairoFcGlyphInfo *
pango_cairo_fc_font_get_glyph_info (PangoFont   *font,
				    PangoGlyph   glyph)
{
  PangoCairoFcFont *cffont = (PangoCairoFcFont *)font;
  PangoCairoFcGlyphInfo *info;

  info = g_hash_table_lookup (cffont->glyph_info, GUINT_TO_POINTER (glyph));
  if (!info)
    {
      info = g_new0 (PangoCairoFcGlyphInfo, 1);
      
      compute_glyph_extents (font, glyph,
			     &info->ink_rect,
			     &info->logical_rect);
      
      g_hash_table_insert (cffont->glyph_info, GUINT_TO_POINTER (glyph), info);
    }

  return info;
}
     
static void
pango_cairo_fc_font_get_glyph_extents (PangoFont      *font,
				       PangoGlyph      glyph,
				       PangoRectangle *ink_rect,
				       PangoRectangle *logical_rect)
{
  PangoCairoFcGlyphInfo *info;

  info = pango_cairo_fc_font_get_glyph_info (font, glyph);
  
  if (ink_rect)
    *ink_rect = info->ink_rect;
  if (logical_rect)
    *logical_rect = info->logical_rect;
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

      info = g_new0 (PangoFcMetricsInfo, 1);
      
      fcfont->metrics_by_lang = g_slist_prepend (fcfont->metrics_by_lang, 
						 info);
	
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
  
  return cairo_ft_scaled_font_lock_face (scaled_font);
}

static void
pango_cairo_fc_font_unlock_face (PangoFcFont *font)
{
  PangoCairoFont *cfont = (PangoCairoFont *)font;
  cairo_scaled_font_t *scaled_font = pango_cairo_fc_font_get_scaled_font (cfont);
  
  cairo_ft_scaled_font_unlock_face (scaled_font);
}

static PangoGlyph
pango_cairo_fc_font_get_glyph (PangoFcFont *font,
			       gunichar     wc)
{
  PangoCairoFcFont *cffont = (PangoCairoFcFont *)font;
  guint idx;
  GlyphCacheEntry *entry;

  /* We create the cache dynamically because although we'll have a lot of
   * PangoCairoFcFont structures (one per font in the fontset, as many as needed
   * to cover the whole Unicode space), we'll only end up using a few of them
   * for the user's actual text.
   */
  if (cffont->char_to_glyph_cache == NULL)
    {
      cffont->char_to_glyph_cache = g_new0 (GlyphCacheEntry, GLYPH_CACHE_NUM_ENTRIES);
#ifdef PROFILE_GLYPH_CACHE
      num_glyph_caches++;
      if (num_glyph_caches > max_glyph_caches)
	max_glyph_caches = num_glyph_caches;
#endif
    }

  idx = wc & GLYPH_CACHE_MASK;
  entry = cffont->char_to_glyph_cache + idx;

  if (entry->ch != wc || wc == 0)
    {
#ifdef PROFILE_GLYPH_CACHE
      num_glyph_cache_misses++;
/*       printf ("cache MISS: cffont %p, gunichar %x = '%c'\n", cffont, wc, (wc < 128) ? wc : 0); */
#endif
      entry->ch = wc;
      entry->glyph = ((PangoFcFontClass *) pango_cairo_fc_font_parent_class)->get_glyph (font, wc);
    }
  else
    {
#ifdef PROFILE_GLYPH_CACHE
      num_glyph_cache_hits++;
/*       printf ("cache HIT: cffont %p, gunichar %x = '%c'\n", cffont, wc, (wc < 128) ? wc : 0); */
#endif
    }

  return entry->glyph;
}

static PangoGlyph
pango_cairo_fc_font_real_get_unknown_glyph (PangoFcFont *font,
					    gunichar     wc)
{
  return 0;
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

#ifdef PROFILE_GLYPH_CACHE
static void
profile_glyph_cache_exit_cb (void)
{
  printf ("Maximum number of PangoCairoFcFont objects: %ld\n", max_cairo_fc_fonts);
  printf ("Maximum number of glyph caches created: %ld (%ld KB)\n", max_glyph_caches, (max_glyph_caches * sizeof (GlyphCacheEntry) * GLYPH_CACHE_NUM_ENTRIES) / 1024);
  printf ("Glyph cache hits: %ld\n", num_glyph_cache_hits);
  printf ("Glyph cache misses: %ld\n", num_glyph_cache_misses);
  printf ("Glyph cache miss rate: %f%%\n", 100.0 * (double) num_glyph_cache_misses / (num_glyph_cache_misses + num_glyph_cache_hits));
}
#endif

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
  fc_font_class->get_glyph = pango_cairo_fc_font_get_glyph;
  fc_font_class->get_unknown_glyph = pango_cairo_fc_font_real_get_unknown_glyph;
  fc_font_class->shutdown = pango_cairo_fc_font_shutdown;

#ifdef PROFILE_GLYPH_CACHE
  atexit (profile_glyph_cache_exit_cb);
#endif
}

static void
pango_cairo_fc_font_init (PangoCairoFcFont *cffont)
{
  cffont->glyph_info = g_hash_table_new_full (g_direct_hash,
					      NULL,
					      NULL,
					      (GDestroyNotify)g_free);

#ifdef PROFILE_GLYPH_CACHE
  num_cairo_fc_fonts++;
  if (num_cairo_fc_fonts > max_cairo_fc_fonts)
    max_cairo_fc_fonts = num_cairo_fc_fonts;
#endif
}

/********************
 *    Private API   *
 ********************/

static double
get_font_size (PangoCairoFcFontMap        *cffontmap,
	       PangoContext               *context,
	       const PangoFontDescription *desc)
{
      
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

  size = get_font_size (cffontmap, context, desc);

  cairo_matrix_scale (&cffont->font_matrix,
		      size / PANGO_SCALE, size / PANGO_SCALE);

  pango_ctm = pango_context_get_matrix (context);
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
  
  return PANGO_FC_FONT (cffont);
}
