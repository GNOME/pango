/* Pango
 * pangocairowin32-font.c: Cairo font handling, fontconfig backend
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
#include <string.h>

#include "pango-fontmap.h"
#include "pangocairo-private.h"
#include "pangocairo-win32.h"

#include <cairo-win32.h>

#define PANGO_TYPE_CAIRO_WIN32_FONT           (pango_cairo_win32_font_get_type ())
#define PANGO_CAIRO_WIN32_FONT(object)        (G_TYPE_CHECK_INSTANCE_CAST ((object), PANGO_TYPE_CAIRO_WIN32_FONT, PangoCairoWin32Font))
#define PANGO_CAIRO_WIN32_FONT_CLASS(klass)   (G_TYPE_CHECK_CLASS_CAST ((klass), PANGO_TYPE_CAIRO_WIN32_FONT, PangoCairoWin32FontClass))
#define PANGO_CAIRO_IS_FONT_CLASS(klass)   (G_TYPE_CHECK_CLASS_TYPE ((klass), PANGO_TYPE_CAIRO_WIN32_FONT))
#define PANGO_CAIRO_WIN32_FONT_GET_CLASS(obj) (G_TYPE_INSTANCE_GET_CLASS ((obj), PANGO_TYPE_CAIRO_WIN32_FONT, PangoCairoWin32FontClass))

typedef struct _PangoCairoWin32Font      PangoCairoWin32Font;
typedef struct _PangoCairoWin32FontClass PangoCairoWin32FontClass;

struct _PangoCairoWin32Font
{
  PangoWin32Font font;

  int size;

  cairo_font_t *cairo_font;
  cairo_matrix_t *font_matrix;
  cairo_matrix_t *total_matrix;

  PangoFontMetrics *metrics;
};

struct _PangoCairoWin32FontClass
{
  PangoWin32FontClass  parent_class;
};

GType pango_cairo_win32_font_get_type (void);

/*******************************
 *       Utility functions     *
 *******************************/

static cairo_font_t *
pango_cairo_win32_font_get_cairo_font (PangoCairoFont *font)
{
  PangoCairoWin32Font *cwfont = PANGO_CAIRO_WIN32_FONT (font);
  PangoWin32Font *win32font = PANGO_WIN32_FONT (cwfont);

  if (cwfont->cairo_font == NULL)
    {
      LOGFONTW logfontw;

      /* Count here on the fact that all the struct fields are
       * in the same place for LOGFONTW and LOGFONTA and LOGFONTA
       * is smaller
       */
      memcpy (&logfontw, &win32font->logfont, sizeof (LOGFONTA));
      
      if (!MultiByteToWideChar (CP_ACP, MB_ERR_INVALID_CHARS,
				win32font->logfont.lfFaceName, strlen (win32font->logfont.lfFaceName),
				logfontw.lfFaceName, sizeof(logfontw.lfFaceName)))
	logfontw.lfFaceName[0] = 0; /* Hopefully this will select some font */
	
      cwfont->cairo_font = cairo_win32_font_create_for_logfontw (&logfontw,
								 cwfont->total_matrix);

      /* Failure of the above should only occur for out of memory,
       * we can't proceed at that point
       */
      if (!cwfont->cairo_font)
	g_error ("Unable create Cairo font");
    }
  
  return cwfont->cairo_font;
}

static void
cairo_font_iface_init (PangoCairoFontIface *iface)
{
  iface->get_cairo_font = pango_cairo_win32_font_get_cairo_font;
}

G_DEFINE_TYPE_WITH_CODE (PangoCairoWin32Font, pango_cairo_win32_font, PANGO_TYPE_WIN32_FONT,
    { G_IMPLEMENT_INTERFACE (PANGO_TYPE_CAIRO_FONT, cairo_font_iface_init) });

/********************************
 *    Method implementations    *
 ********************************/

static void
pango_cairo_win32_font_finalize (GObject *object)
{
  PangoCairoWin32Font *cwfont = PANGO_CAIRO_WIN32_FONT (object);

  if (cwfont->metrics)
    pango_font_metrics_unref (cwfont->metrics);
  
  if (cwfont->cairo_font)
    cairo_font_destroy (cwfont->cairo_font);

  cairo_matrix_destroy (cwfont->total_matrix);
  cairo_matrix_destroy (cwfont->font_matrix);

  G_OBJECT_CLASS (pango_cairo_win32_font_parent_class)->finalize (object);
}

static void
pango_cairo_win32_font_get_glyph_extents (PangoFont        *font,
				       PangoGlyph        glyph,
				       PangoRectangle   *ink_rect,
				       PangoRectangle   *logical_rect)
{
  PangoCairoWin32Font *cwfont = PANGO_CAIRO_WIN32_FONT (font);
  cairo_font_t *cairo_font;
  cairo_text_extents_t extents;
  cairo_glyph_t cairo_glyph;

  cairo_font = pango_cairo_win32_font_get_cairo_font (PANGO_CAIRO_FONT (font));

  cairo_glyph.index = glyph;
  cairo_glyph.x = 0;
  cairo_glyph.y = 0;

  cairo_font_glyph_extents (cairo_font, cwfont->font_matrix,
			    &cairo_glyph, 1, &extents);

  if (ink_rect)
    {
      ink_rect->x = extents.x_bearing * PANGO_SCALE;
      ink_rect->y = extents.y_bearing * PANGO_SCALE;
      ink_rect->width = extents.width * PANGO_SCALE;
      ink_rect->height = extents.height * PANGO_SCALE;
    }
  
  if (logical_rect)
    {
      cairo_font_extents_t font_extents;

      cairo_font_extents (cairo_font, cwfont->font_matrix,
			  &font_extents);
      
      logical_rect->x = 0;
      logical_rect->y = - font_extents.ascent * PANGO_SCALE;
      logical_rect->width = extents.x_advance * PANGO_SCALE;
      logical_rect->height = (font_extents.ascent + font_extents.descent) * PANGO_SCALE;
    }
}

static PangoFontMetrics *
pango_cairo_win32_font_get_metrics (PangoFont        *font,
				    PangoLanguage    *language)
{
  PangoCairoWin32Font *cwfont = PANGO_CAIRO_WIN32_FONT (font);

  if (!cwfont->metrics)
    {
      cairo_font_t *cairo_font;
      cairo_font_extents_t font_extents;
      cwfont->metrics = pango_font_metrics_new ();
      double height;

      cairo_font = pango_cairo_win32_font_get_cairo_font (PANGO_CAIRO_FONT (font));

      cairo_font_extents (cairo_font, cwfont->font_matrix,
			  &font_extents);
      
      cwfont->metrics->ascent = font_extents.ascent * PANGO_SCALE;
      cwfont->metrics->descent = font_extents.ascent * PANGO_SCALE;
      cwfont->metrics->approximate_char_width = font_extents.max_x_advance * PANGO_SCALE;
      cwfont->metrics->approximate_digit_width = font_extents.max_y_advance * PANGO_SCALE;

      height = font_extents.ascent + font_extents.descent;
      
      cwfont->metrics->underline_thickness = (PANGO_SCALE * height) / 14;
      cwfont->metrics->underline_position = - cwfont->metrics->underline_thickness;
      cwfont->metrics->strikethrough_thickness = cwfont->metrics->underline_thickness;
      cwfont->metrics->strikethrough_position = (PANGO_SCALE * height) / 4;
    }

  return pango_font_metrics_ref (cwfont->metrics);
}

static gboolean
pango_cairo_win32_font_select_font (PangoFont *font,
				    HDC        hdc)
{
  cairo_font_t *cairo_font = pango_cairo_win32_font_get_cairo_font (PANGO_CAIRO_FONT (font));
  
  return cairo_win32_font_select_font (cairo_font, hdc) == CAIRO_STATUS_SUCCESS;
}

static void
pango_cairo_win32_font_done_font (PangoFont *font)
{
  cairo_font_t *cairo_font = pango_cairo_win32_font_get_cairo_font (PANGO_CAIRO_FONT (font));
  
  cairo_win32_font_done_font (cairo_font);
}

static double
pango_cairo_win32_font_get_scale_factor (PangoFont *font)
{
  PangoWin32Font *win32font = PANGO_WIN32_FONT (font);
  cairo_font_t *cairo_font = pango_cairo_win32_font_get_cairo_font (PANGO_CAIRO_FONT (font));

  return cairo_win32_font_get_scale_factor (cairo_font) * win32font->size;
}

static void
pango_cairo_win32_font_class_init (PangoCairoWin32FontClass *class)
{
  GObjectClass *object_class = G_OBJECT_CLASS (class);
  PangoFontClass *font_class = PANGO_FONT_CLASS (class);
  PangoWin32FontClass *win32_font_class = PANGO_WIN32_FONT_CLASS (class);

  object_class->finalize = pango_cairo_win32_font_finalize;
  
  font_class->get_glyph_extents = pango_cairo_win32_font_get_glyph_extents;
  font_class->get_metrics = pango_cairo_win32_font_get_metrics;

  win32_font_class->select_font = pango_cairo_win32_font_select_font;
  win32_font_class->done_font = pango_cairo_win32_font_done_font;
  win32_font_class->get_scale_factor = pango_cairo_win32_font_get_scale_factor;
}

static void
pango_cairo_win32_font_init (PangoCairoWin32Font *cwfont)
{
}

/********************
 *    Private API   *
 ********************/

PangoFont *
_pango_cairo_win32_font_new (PangoCairoWin32FontMap     *cwfontmap,
			     PangoContext               *context,
			     PangoWin32Face             *face,
			     const PangoFontDescription *desc)
{
  PangoCairoWin32Font *cwfont;
  PangoWin32Font *win32font;
  const PangoMatrix *pango_ctm;
  double size;

  cwfont = g_object_new (PANGO_TYPE_CAIRO_WIN32_FONT, NULL);
  win32font = PANGO_WIN32_FONT (cwfont);
  
  win32font->fontmap = PANGO_FONT_MAP (cwfontmap);
  g_object_ref (cwfontmap);

  win32font->win32face = face;

  size = (double) pango_font_description_get_size (desc) / PANGO_SCALE;
  
  if (!pango_font_description_get_size_is_absolute (desc))
    size *= cwfontmap->dpi / 72.;

  /* FIXME: THis is a pixel size, so not really what we want for describe(),
   * but it's what we need when computing the scale factor.
   */
  win32font->size = size * PANGO_SCALE;

  cwfont->font_matrix = cairo_matrix_create ();
  cairo_matrix_scale (cwfont->font_matrix, size, size);

  cwfont->total_matrix = cairo_matrix_create ();

  pango_ctm = pango_context_get_matrix (context);
  if (pango_ctm)
    {
      cairo_matrix_t *ctm;

      ctm = cairo_matrix_create ();
      cairo_matrix_set_affine (ctm,
			       pango_ctm->xx,
			       pango_ctm->yx,
			       pango_ctm->xy,
			       pango_ctm->yy,
			       0., 0.);
      
      cairo_matrix_multiply (cwfont->total_matrix, cwfont->font_matrix, ctm);
      cairo_matrix_destroy (ctm);
    }
  else
    cairo_matrix_copy (cwfont->total_matrix, cwfont->font_matrix);

  return PANGO_FONT (cwfont);
}
