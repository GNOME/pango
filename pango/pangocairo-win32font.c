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
#include "pango-impl-utils.h"
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
  PangoCairoFontPrivate cf_priv;
};

struct _PangoCairoWin32FontClass
{
  PangoWin32FontClass  parent_class;
};

GType pango_cairo_win32_font_get_type (void);

static cairo_font_face_t *pango_cairo_win32_font_create_font_face           (PangoCairoFont *font);
static PangoFontMetrics  *pango_cairo_win32_font_create_metrics_for_context (PangoCairoFont *font,
									     PangoContext    *context);


static void
cairo_font_iface_init (PangoCairoFontIface *iface)
{
  iface->create_font_face = pango_cairo_win32_font_create_font_face;
  iface->create_metrics_for_context = pango_cairo_win32_font_create_metrics_for_context;
  iface->cf_priv_offset = G_STRUCT_OFFSET (PangoCairoWin32Font, cf_priv);
}

G_DEFINE_TYPE_WITH_CODE (PangoCairoWin32Font, pango_cairo_win32_font, PANGO_TYPE_WIN32_FONT,
    { G_IMPLEMENT_INTERFACE (PANGO_TYPE_CAIRO_FONT, cairo_font_iface_init) });

static cairo_font_face_t *
pango_cairo_win32_font_create_font_face (PangoCairoFont *font)
{
  PangoCairoWin32Font *cwfont = PANGO_CAIRO_WIN32_FONT (font);
  PangoWin32Font *win32font =  &cwfont->font;

  return cairo_win32_font_face_create_for_logfontw (&win32font->logfontw);
}

static int
max_glyph_width (PangoLayout *layout)
{
  int max_width = 0;
  GSList *l, *r;

  for (l = pango_layout_get_lines_readonly (layout); l; l = l->next)
    {
      PangoLayoutLine *line = l->data;

      for (r = line->runs; r; r = r->next)
	{
	  PangoGlyphString *glyphs = ((PangoGlyphItem *)r->data)->glyphs;
	  int i;

	  for (i = 0; i < glyphs->num_glyphs; i++)
	    if (glyphs->glyphs[i].geometry.width > max_width)
	      max_width = glyphs->glyphs[i].geometry.width;
	}
    }

  return max_width;
}

static PangoFontMetrics *
pango_cairo_win32_font_create_metrics_for_context (PangoCairoFont *font,
                                                   PangoContext   *context)
{
  PangoFontMetrics *metrics;
  PangoFontDescription *font_desc;
  PangoLayout *layout;
  PangoRectangle extents;
  PangoLanguage *language = pango_context_get_language (context);
  const char *sample_str = pango_language_get_sample_string (language);
  cairo_scaled_font_t *scaled_font;
  cairo_font_extents_t font_extents;
  double height;

  metrics = pango_font_metrics_new ();

  scaled_font = pango_cairo_font_get_scaled_font ((PangoFont *) font);

  cairo_scaled_font_extents (scaled_font, &font_extents);
  cairo_win32_scaled_font_done_font (scaled_font);

  metrics->ascent = font_extents.ascent * PANGO_SCALE;
  metrics->descent = font_extents.descent * PANGO_SCALE;

  /* FIXME: Should get the real settings for these from the TrueType
   * font file.
   */
  height = metrics->ascent + metrics->descent;
  metrics->underline_thickness = height / 14;
  metrics->underline_position = - metrics->underline_thickness;
  metrics->strikethrough_thickness = metrics->underline_thickness;
  metrics->strikethrough_position = height / 4;

  pango_quantize_line_geometry (&metrics->underline_thickness,
				&metrics->underline_position);
  pango_quantize_line_geometry (&metrics->strikethrough_thickness,
				&metrics->strikethrough_position);
  /* Quantizing may have pushed underline_position to 0.  Not good */
  if (metrics->underline_position == 0)
    metrics->underline_position = - metrics->underline_thickness;

  layout = pango_layout_new (context);
  font_desc = pango_font_describe_with_absolute_size ((PangoFont *) font);
  pango_layout_set_font_description (layout, font_desc);
  pango_layout_set_text (layout, sample_str, -1);
  pango_layout_get_extents (layout, NULL, &extents);

  metrics->approximate_char_width = extents.width / g_utf8_strlen (sample_str, -1);

  pango_layout_set_text (layout, "0123456789", -1);
  metrics->approximate_digit_width = max_glyph_width (layout);

  pango_font_description_free (font_desc);
  g_object_unref (layout);

  return metrics;
}

static void
pango_cairo_win32_font_finalize (GObject *object)
{
  G_OBJECT_CLASS (pango_cairo_win32_font_parent_class)->finalize (object);
}

static void
pango_cairo_win32_font_get_glyph_extents (PangoFont        *font,
					  PangoGlyph        glyph,
					  PangoRectangle   *ink_rect,
					  PangoRectangle   *logical_rect)
{
  PangoCairoWin32Font *cwfont = (PangoCairoWin32Font *) font;

  _pango_cairo_font_private_get_glyph_extents (&cwfont->cf_priv,
					       glyph,
					       ink_rect,
					       logical_rect);
}

static gboolean
pango_cairo_win32_font_select_font (PangoFont *font,
				    HDC        hdc)
{
  cairo_scaled_font_t *scaled_font = _pango_cairo_font_private_get_scaled_font (&PANGO_CAIRO_WIN32_FONT (font)->cf_priv);

  return cairo_win32_scaled_font_select_font (scaled_font, hdc) == CAIRO_STATUS_SUCCESS;
}

static void
pango_cairo_win32_font_done_font (PangoFont *font)
{
  cairo_scaled_font_t *scaled_font = _pango_cairo_font_private_get_scaled_font (&PANGO_CAIRO_WIN32_FONT (font)->cf_priv);

  cairo_win32_scaled_font_done_font (scaled_font);
}

static double
pango_cairo_win32_font_get_metrics_factor (PangoFont *font)
{
  PangoWin32Font *win32font = PANGO_WIN32_FONT (font);
  cairo_scaled_font_t *scaled_font = _pango_cairo_font_private_get_scaled_font (&PANGO_CAIRO_WIN32_FONT (font)->cf_priv);

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
  font_class->get_metrics = _pango_cairo_font_get_metrics;

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
  double size;
  double dpi;
#define USE_FACE_CACHED_FONTS
#ifdef USE_FACE_CACHED_FONTS
  PangoWin32FontMap *win32fontmap;
  GSList *tmp_list;
#endif
  cairo_matrix_t font_matrix;

  g_return_val_if_fail (PANGO_IS_CAIRO_WIN32_FONT_MAP (cwfontmap), NULL);

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

#ifdef USE_FACE_CACHED_FONTS
  win32fontmap = PANGO_WIN32_FONT_MAP (cwfontmap);

  tmp_list = face->cached_fonts;
  while (tmp_list)
    {
      win32font = tmp_list->data;
      if (ABS (win32font->size - size * PANGO_SCALE) < 2)
	{
	  g_object_ref (win32font);
	  if (win32font->in_cache)
	    _pango_win32_fontmap_cache_remove (PANGO_FONT_MAP (win32fontmap), win32font);

	  return PANGO_FONT (win32font);
	}
      tmp_list = tmp_list->next;
    }
#endif
  cwfont = g_object_new (PANGO_TYPE_CAIRO_WIN32_FONT, NULL);
  win32font = PANGO_WIN32_FONT (cwfont);

  g_assert (win32font->fontmap == NULL);
  win32font->fontmap = (PangoFontMap *) cwfontmap;
  g_object_add_weak_pointer (G_OBJECT (win32font->fontmap), (gpointer *) (gpointer) &win32font->fontmap);

  win32font->win32face = face;

#ifdef USE_FACE_CACHED_FONTS
  face->cached_fonts = g_slist_prepend (face->cached_fonts, win32font);
#endif

  /* FIXME: This is a pixel size, so not really what we want for describe(),
   * but it's what we need when computing the scale factor.
   */
  win32font->size = size * PANGO_SCALE;

  _pango_win32_make_matching_logfontw (win32font->fontmap,
				       &face->logfontw,
				       win32font->size,
				       &win32font->logfontw);

  cairo_matrix_init_identity (&font_matrix);

  cairo_matrix_scale (&font_matrix, size, size);

  _pango_cairo_font_private_initialize (&cwfont->cf_priv,
					(PangoCairoFont *) cwfont,
					context,
					desc,
					&font_matrix);

  return PANGO_FONT (cwfont);
}
