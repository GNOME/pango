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

#include <stdlib.h>

#include "pango-fontmap.h"
#include "pangocairo-private.h"
#include "pangocairo-fc.h"
#include "pangofc-private.h"

#define PANGO_TYPE_CAIRO_FC_FONT           (pango_cairo_fc_font_get_type ())
#define PANGO_CAIRO_FC_FONT(object)        (G_TYPE_CHECK_INSTANCE_CAST ((object), PANGO_TYPE_CAIRO_FC_FONT, PangoCairoFcFont))
#define PANGO_CAIRO_FC_FONT_CLASS(klass)   (G_TYPE_CHECK_CLASS_CAST ((klass), PANGO_TYPE_CAIRO_FC_FONT, PangoCairoFcFontClass))
#define PANGO_CAIRO_IS_FONT_CLASS(klass)   (G_TYPE_CHECK_CLASS_TYPE ((klass), PANGO_TYPE_CAIRO_FC_FONT))
#define PANGO_CAIRO_FC_FONT_GET_CLASS(obj) (G_TYPE_INSTANCE_GET_CLASS ((obj), PANGO_TYPE_CAIRO_FC_FONT, PangoCairoFcFontClass))

#define PANGO_CAIRO_UNKNOWN_FLAG 0x10000000

typedef struct _PangoCairoFcFont      PangoCairoFcFont;
typedef struct _PangoCairoFcFontClass PangoCairoFcFontClass;

struct _PangoCairoFcFont
{
  PangoFcFont font;

  FT_Face face;
  cairo_font_t *cairo_font;
  
  int load_flags;
  gboolean have_size;

  GHashTable *glyph_info;
};

struct _PangoCairoFcFontClass
{
  PangoFcFontClass  parent_class;
};

GType pango_cairo_fc_font_get_type (void);

/*******************************
 *       Utility functions     *
 *******************************/

static FT_Face
load_face (PangoCairoFcFont *cffont)
{
  PangoFcFont *fcfont = PANGO_FC_FONT (cffont);
  FT_Library library = PANGO_CAIRO_FC_FONT_MAP (fcfont->fontmap)->library;
  FT_Error error;
  FcPattern *pattern;
  FcChar8 *filename;
  FcBool antialias, hinting, autohint;
  FT_Face face = NULL;
  int id;

  pattern = fcfont->font_pattern;

  cffont->load_flags = 0;

  /* disable antialiasing if requested */
  if (FcPatternGetBool (pattern,
			FC_ANTIALIAS, 0, &antialias) != FcResultMatch)
    antialias = FcTrue;
  
  if (antialias)
    cffont->load_flags |= FT_LOAD_NO_BITMAP;
  else
    cffont->load_flags |= FT_LOAD_TARGET_MONO;
  
  /* disable hinting if requested */
  if (FcPatternGetBool (pattern,
			FC_HINTING, 0, &hinting) != FcResultMatch)
    hinting = FcTrue;
  
  if (!hinting)
    cffont->load_flags |= FT_LOAD_NO_HINTING;
  
  /* force autohinting if requested */
  if (FcPatternGetBool (pattern,
			FC_AUTOHINT, 0, &autohint) != FcResultMatch)
    autohint = FcFalse;
  
  if (autohint)
    cffont->load_flags |= FT_LOAD_FORCE_AUTOHINT;
  
  if (FcPatternGetString (pattern, FC_FILE, 0, &filename) != FcResultMatch)
    goto bail;
  
  if (FcPatternGetInteger (pattern, FC_INDEX, 0, &id) != FcResultMatch)
    goto bail;

  error = FT_New_Face (library, (char *) filename, id, &face);

 bail:
  return face;
} 

static FT_Face
load_fallback_face (PangoCairoFcFont *cffont)
{
  PangoFcFont *fcfont = PANGO_FC_FONT (cffont);
  FT_Library library = PANGO_CAIRO_FC_FONT_MAP (fcfont->fontmap)->library;
  FcPattern *sans;
  FcPattern *matched;
  FcResult result;
  FT_Error error;
  FcChar8 *filename = NULL;
  FT_Face face = NULL;
  int id;
  
  /* FIXME: pass in a size in case Sans is bitmap */
  sans = FcPatternBuild (NULL,
			 FC_FAMILY, FcTypeString, "sans",
			 NULL);
  
  matched = FcFontMatch (NULL, sans, &result);
  
  if (FcPatternGetString (matched, FC_FILE, 0, &filename) != FcResultMatch)
    goto bail;
  
  if (FcPatternGetInteger (matched, FC_INDEX, 0, &id) != FcResultMatch)
    goto bail;
  
  face = NULL;
  error = FT_New_Face (library, (char *) filename, id, &face);
  
  if (error)
    {
    bail:
      g_warning ("Unable to open font file %s for Sans, exiting", filename);
      exit (1);
    }

  FcPatternDestroy (sans);
  FcPatternDestroy (matched);

  return face;
}

static cairo_font_t *
get_cairo_font (PangoCairoFcFont *cffont)
{
  PangoFcFont *fcfont = PANGO_FC_FONT (cffont);

  if (cffont->cairo_font == NULL)
    {
      FT_Face face = load_face (cffont);

      if (face)
	cffont->cairo_font = cairo_ft_font_create_for_ft_face (face);
      
      if (!cffont->cairo_font)
	{
	  gchar *name = pango_font_description_to_string (fcfont->description);
	  g_warning ("Cannot open font file for font %s, trying Sans", name);
  	  g_free (name);

	  if (face)
	    FT_Done_Face (face);

	  face = load_fallback_face (cffont);

	  if (face)
	    cffont->cairo_font = cairo_ft_font_create_for_ft_face (face);

	  if (!cffont->cairo_font)
	    {
	      g_warning ("Unable create Cairo font for Sans, exiting");
	      exit (1);
	    }
	}

      cffont->face = face;
    }
  
  return cffont->cairo_font;
}

static void
make_current (PangoCairoFcFont *cffont,
	      cairo_t          *cr)
{
  PangoFcFont *fcfont = PANGO_FC_FONT (cffont);
  double scale;
  
  if (pango_font_description_get_size_is_absolute (fcfont->description))
    scale = pango_font_description_get_size (fcfont->description);
  else
    scale = (PANGO_CAIRO_FC_FONT_MAP (fcfont->fontmap)->dpi *
	     pango_font_description_get_size (fcfont->description)) / (PANGO_SCALE * 72.);
    
  cairo_set_font (cr, get_cairo_font (cffont));
  cairo_scale_font (cr, scale);
}

static void
pango_cairo_fc_font_make_current (PangoCairoFont *font,
				  cairo_t        *cr)
{
  make_current (PANGO_CAIRO_FC_FONT (font), cr);
}

static void
cairo_font_iface_init (PangoCairoFontIface *iface)
{
  iface->make_current = pango_cairo_fc_font_make_current;
}

G_DEFINE_TYPE_WITH_CODE (PangoCairoFcFont, pango_cairo_fc_font, PANGO_TYPE_FC_FONT,
    { G_IMPLEMENT_INTERFACE (PANGO_TYPE_CAIRO_FONT, cairo_font_iface_init) });

/********************************
 *    Method implementations    *
 ********************************/

static void
pango_cairo_fc_font_finalize (GObject *object)
{
  PangoCairoFcFont *cffont = PANGO_CAIRO_FC_FONT (object);

  if (cffont->cairo_font)
    {
      cairo_font_destroy (cffont->cairo_font);
      cffont->cairo_font = NULL;

      FT_Done_Face (cffont->face);
      cffont->face = NULL;
    }

  if (cffont->glyph_info)
    g_hash_table_destroy (cffont->glyph_info);

  G_OBJECT_CLASS (pango_cairo_fc_font_parent_class)->finalize (object);
}

static cairo_t *
get_temporary_context (PangoCairoFcFont *cffont)
{
  PangoFcFont *fcfont = PANGO_FC_FONT (cffont);
  FcMatrix *fc_matrix;
  cairo_t *cr;

  cr = cairo_create ();
  
  if  (FcPatternGetMatrix (fcfont->font_pattern,
			   FC_MATRIX, 0, &fc_matrix) == FcResultMatch)
    {
      cairo_matrix_t *cairo_matrix = cairo_matrix_create ();
      cairo_matrix_set_affine (cairo_matrix,
			       fc_matrix->xx, fc_matrix->yx,
			       fc_matrix->xy, fc_matrix->yy,
			       0, 0);
      cairo_set_matrix (cr, cairo_matrix);
      cairo_matrix_destroy (cairo_matrix);
    }

  make_current (cffont, cr);

  return cr;
}

static void
get_ascent_descent (PangoCairoFcFont *cffont,
		    int              *ascent,
		    int              *descent)
{
  /* This is complicated in general (see pangofc-font.c:get_face_metrics(),
   * but simple for hinted, untransformed fonts. cairo_glyph_extents() will
   * have set up the right size on the font as a side-effect.
   */
  *descent = - PANGO_UNITS_26_6 (cffont->face->size->metrics.descender);
  *ascent = PANGO_UNITS_26_6 (cffont->face->size->metrics.ascender);
}

static void
get_glyph_extents_cairo (PangoFcFont      *fcfont,
			 PangoGlyph        glyph,
			 PangoRectangle   *ink_rect,
			 PangoRectangle   *logical_rect)
{
  PangoCairoFcFont *cffont = PANGO_CAIRO_FC_FONT (fcfont);
  cairo_text_extents_t extents;
  cairo_glyph_t cairo_glyph;
  cairo_t *cr;

  cairo_glyph.index = glyph;
  cairo_glyph.x = 0;
  cairo_glyph.y = 0;
  
  cr = get_temporary_context (cffont);
  cairo_glyph_extents (cr, &cairo_glyph, 1, &extents);
  cairo_destroy (cr);

  if (ink_rect)
    {
      ink_rect->x = extents.x_bearing * PANGO_SCALE;
      ink_rect->y = extents.y_bearing * PANGO_SCALE;
      ink_rect->width = extents.width * PANGO_SCALE;
      ink_rect->height = extents.height * PANGO_SCALE;
    }
  
  if (logical_rect)
    {
      int ascent, descent;

      get_ascent_descent (cffont, &ascent, &descent);
      
      logical_rect->x = 0;
      logical_rect->y = - ascent;
      logical_rect->width = extents.x_advance * PANGO_SCALE;
      logical_rect->height = ascent + descent;
    }
}

typedef struct
{
  PangoRectangle ink_rect;
  PangoRectangle logical_rect;
} Extents;

static void
get_glyph_extents_raw (PangoCairoFcFont *cffont,
		       PangoGlyph        glyph,
		       PangoRectangle   *ink_rect,
		       PangoRectangle   *logical_rect)
{
  Extents *extents;

  if (!cffont->glyph_info)
    cffont->glyph_info = g_hash_table_new_full (NULL, NULL,
						NULL, (GDestroyNotify)g_free);

  extents = g_hash_table_lookup (cffont->glyph_info,
				 GUINT_TO_POINTER (glyph));

  if (!extents)
    {
      extents = g_new (Extents, 1);
     
      pango_fc_font_get_raw_extents (PANGO_FC_FONT (cffont),
				     FT_LOAD_NO_BITMAP | FT_LOAD_NO_HINTING,
				     glyph,
				     &extents->ink_rect,
				     &extents->logical_rect);

      g_hash_table_insert (cffont->glyph_info,
			   GUINT_TO_POINTER (glyph),
			   extents);
    }
  
  if (ink_rect)
    *ink_rect = extents->ink_rect;

  if (logical_rect)
    *logical_rect = extents->logical_rect;
}

static void
pango_cairo_fc_font_get_glyph_extents (PangoFont        *font,
				       PangoGlyph        glyph,
				       PangoRectangle   *ink_rect,
				       PangoRectangle   *logical_rect)
{
  PangoCairoFcFont *cffont = PANGO_CAIRO_FC_FONT (font);
  PangoFcFont *fcfont = PANGO_FC_FONT (font);

  if (!fcfont->fontmap)		/* Display closed */
    goto fallback;

  if (glyph == (PangoGlyph)-1)
    glyph = 0;

  if (glyph)
    {
      if (!fcfont->is_transformed && fcfont->is_hinted)
	get_glyph_extents_cairo (fcfont, glyph, ink_rect, logical_rect);
      else
	get_glyph_extents_raw (cffont, glyph, ink_rect, logical_rect);
    }
  else
    {
    fallback:
      
      if (ink_rect)
	{
	  ink_rect->x = 0;
	  ink_rect->width = 0;
	  ink_rect->y = 0;
	  ink_rect->height = 0;
	}
      if (logical_rect)
	{
	  logical_rect->x = 0;
	  logical_rect->width = 0;
	  logical_rect->y = 0;
	  logical_rect->height = 0;
	}
    }
}

static FT_Face
pango_cairo_fc_font_lock_face (PangoFcFont *font)
{
  PangoCairoFcFont *cffont = PANGO_CAIRO_FC_FONT (font);

  /* Horrible hack, we need the font's face to be sized when
   * locked, but sizing is only done as a side-effect
   */
  if (!cffont->have_size)
    {
      cairo_t *cr = get_temporary_context (cffont);
      cairo_font_extents_t extents;

      cairo_current_font_extents (cr, &extents);
      cairo_destroy (cr);

      cffont->have_size = TRUE;
    }

  return cairo_ft_font_face (cffont->cairo_font);
}

static void
pango_cairo_fc_font_unlock_face (PangoFcFont *font)
{
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
  if (cffont->cairo_font)
    {
      cairo_font_destroy (cffont->cairo_font);
      cffont->cairo_font = NULL;

      FT_Done_Face (cffont->face);
      cffont->face = NULL;
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

  fc_font_class->lock_face = pango_cairo_fc_font_lock_face;
  fc_font_class->unlock_face = pango_cairo_fc_font_unlock_face;
  fc_font_class->get_unknown_glyph = pango_cairo_fc_font_real_get_unknown_glyph;
  fc_font_class->shutdown = pango_cairo_fc_font_shutdown;
}

static void
pango_cairo_fc_font_init (PangoCairoFcFont *cffont)
{
}

/********************
 *    Private API   *
 ********************/

PangoFcFont *
_pango_cairo_fc_font_new (PangoCairoFcFontMap *cffontmap,
			  FcPattern	      *pattern)
{
  g_return_val_if_fail (PANGO_IS_CAIRO_FC_FONT_MAP (cffontmap), NULL);
  g_return_val_if_fail (pattern != NULL, NULL);

  return g_object_new (PANGO_TYPE_CAIRO_FC_FONT,
		       "pattern", pattern,
		       NULL);
}

