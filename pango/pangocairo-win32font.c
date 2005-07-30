/* Pango
 * pangocairowin32-font.c: Cairo font handling, Win32 backend
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

  cairo_font_face_t *font_face;
  cairo_scaled_font_t *scaled_font;
  
  cairo_matrix_t font_matrix;
  cairo_matrix_t ctm;
  cairo_font_options_t *options;
  
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

static cairo_font_face_t *
pango_cairo_win32_font_get_font_face (PangoCairoFont *font)
{
  PangoCairoWin32Font *cwfont = PANGO_CAIRO_WIN32_FONT (font);
  PangoWin32Font *win32font = PANGO_WIN32_FONT (cwfont);

  if (cwfont->font_face == NULL)
    {
      LOGFONTW logfontw;

      /* Count here on the fact that all the struct fields are
       * in the same place for LOGFONTW and LOGFONTA and LOGFONTA
       * is smaller
       */
      memcpy (&logfontw, &win32font->logfont, sizeof (LOGFONTA));
      
      if (!MultiByteToWideChar (CP_ACP, MB_ERR_INVALID_CHARS,
				win32font->logfont.lfFaceName, -1,
				logfontw.lfFaceName, G_N_ELEMENTS (logfontw.lfFaceName)))
	logfontw.lfFaceName[0] = 0; /* Hopefully this will select some font */
      
      cwfont->font_face = cairo_win32_font_face_create_for_logfontw (&logfontw);

      /* Failure of the above should only occur for out of memory,
       * we can't proceed at that point
       */
      if (!cwfont->font_face)
	g_error ("Unable create Cairo font");
    }
  
  return cwfont->font_face;
}

static cairo_scaled_font_t *
pango_cairo_win32_font_get_scaled_font (PangoCairoFont *font)
{
  PangoCairoWin32Font *cffont = PANGO_CAIRO_WIN32_FONT (font);

  if (!cffont->scaled_font)
    {
      cairo_font_face_t *font_face;

      font_face = pango_cairo_win32_font_get_font_face (font);
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
pango_cairo_win32_font_install (PangoCairoFont *font,
				cairo_t        *cr)
{
  PangoCairoWin32Font *cwfont = PANGO_CAIRO_WIN32_FONT (font);

  cairo_set_font_face (cr,
		       pango_cairo_win32_font_get_font_face (font));
  cairo_set_font_matrix (cr, &cwfont->font_matrix);
  cairo_set_font_options (cr, cwfont->options);
}

static void
cairo_font_iface_init (PangoCairoFontIface *iface)
{
  iface->install = pango_cairo_win32_font_install;
}

G_DEFINE_TYPE_WITH_CODE (PangoCairoWin32Font, pango_cairo_win32_font, PANGO_TYPE_WIN32_FONT,
    { G_IMPLEMENT_INTERFACE (PANGO_TYPE_CAIRO_FONT, cairo_font_iface_init) });

static void
pango_cairo_win32_font_finalize (GObject *object)
{
  PangoCairoWin32Font *cwfont = PANGO_CAIRO_WIN32_FONT (object);

  if (cwfont->metrics)
    pango_font_metrics_unref (cwfont->metrics);
  
  if (cwfont->scaled_font)
    cairo_scaled_font_destroy (cwfont->scaled_font);

  if (cwfont->options)
    cairo_font_options_destroy (cwfont->options);

  G_OBJECT_CLASS (pango_cairo_win32_font_parent_class)->finalize (object);
}

static void
pango_cairo_win32_font_get_glyph_extents (PangoFont        *font,
					  PangoGlyph        glyph,
					  PangoRectangle   *ink_rect,
					  PangoRectangle   *logical_rect)
{
  cairo_scaled_font_t *scaled_font;
  cairo_text_extents_t extents;
  cairo_glyph_t cairo_glyph;

  scaled_font = pango_cairo_win32_font_get_scaled_font (PANGO_CAIRO_FONT (font));

  cairo_glyph.index = glyph;
  cairo_glyph.x = 0;
  cairo_glyph.y = 0;

  cairo_scaled_font_glyph_extents (scaled_font,
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

      cairo_scaled_font_extents (scaled_font, &font_extents);
      
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
      double height;
      cairo_scaled_font_t *scaled_font;
      cairo_font_extents_t font_extents;
      cwfont->metrics = pango_font_metrics_new ();

      scaled_font = pango_cairo_win32_font_get_scaled_font (PANGO_CAIRO_FONT (font));

      cairo_scaled_font_extents (scaled_font, &font_extents);
      
      cwfont->metrics->ascent = font_extents.ascent * PANGO_SCALE;
      cwfont->metrics->descent = font_extents.descent * PANGO_SCALE;
      cwfont->metrics->approximate_char_width = font_extents.max_x_advance * PANGO_SCALE;
      cwfont->metrics->approximate_digit_width = font_extents.max_x_advance * PANGO_SCALE;

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
  cairo_scaled_font_t *scaled_font = pango_cairo_win32_font_get_scaled_font (PANGO_CAIRO_FONT (font));
  
  return cairo_win32_scaled_font_select_font (scaled_font, hdc) == CAIRO_STATUS_SUCCESS;
}

static void
pango_cairo_win32_font_done_font (PangoFont *font)
{
  cairo_scaled_font_t *scaled_font = pango_cairo_win32_font_get_scaled_font (PANGO_CAIRO_FONT (font));
  
  cairo_win32_scaled_font_done_font (scaled_font);
}

static double
pango_cairo_win32_font_get_metrics_factor (PangoFont *font)
{
  PangoWin32Font *win32font = PANGO_WIN32_FONT (font);
  cairo_scaled_font_t *scaled_font = pango_cairo_win32_font_get_scaled_font (PANGO_CAIRO_FONT (font));

  return cairo_win32_scaled_font_get_metrics_factor (scaled_font) * win32font->size;
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
  win32_font_class->get_metrics_factor = pango_cairo_win32_font_get_metrics_factor;
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
  double dpi;

  cwfont = g_object_new (PANGO_TYPE_CAIRO_WIN32_FONT, NULL);
  win32font = PANGO_WIN32_FONT (cwfont);
  
  win32font->fontmap = PANGO_FONT_MAP (cwfontmap);
  g_object_ref (cwfontmap);

  win32font->win32face = face;

  size = (double) pango_font_description_get_size (desc) / PANGO_SCALE;

  if (context)
    {
      dpi = pango_cairo_context_get_resolution (context);
      
      if (dpi <= 0)
	dpi = cwfontmap->dpi;
    }
  else
    dpi = cwfontmap->dpi;
  
  if (!pango_font_description_get_size_is_absolute (desc))
    size *= dpi / 72.;

  /* FIXME: This is a pixel size, so not really what we want for describe(),
   * but it's what we need when computing the scale factor.
   */
  win32font->size = size * PANGO_SCALE;

  cairo_matrix_init_scale (&cwfont->font_matrix,
			   size, size);

  pango_ctm = pango_context_get_matrix (context);
  if (pango_ctm)
    cairo_matrix_init (&cwfont->ctm,
		       pango_ctm->xx,
		       pango_ctm->yx,
		       pango_ctm->xy,
		       pango_ctm->yy,
		       0., 0.);
  else
    cairo_matrix_init_identity (&cwfont->ctm);

  pango_win32_make_matching_logfont (win32font->fontmap,
				     &face->logfont,
				     win32font->size,
				     &win32font->logfont);

  cwfont->options = cairo_font_options_copy (_pango_cairo_context_get_merged_font_options (context));
  
  return PANGO_FONT (cwfont);
}
