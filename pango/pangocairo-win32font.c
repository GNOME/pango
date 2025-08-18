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

#if defined (HAVE_CAIRO_WIN32_DIRECTWRITE) && \
    ((CAIRO_VERSION_MAJOR > 1) || \
     (CAIRO_VERSION_MAJOR == 1 && CAIRO_VERSION_MINOR > 17) || \
     (CAIRO_VERSION_MAJOR == 1 && CAIRO_VERSION_MINOR == 17 && CAIRO_VERSION_MICRO > 6))
/*
 * We are using C here, so use a workaround as cairo-dwrite.h from 1.17.8
 * can only be used in C++, which replaces the C-friendly version of
 * cairo_dwrite_font_face_create_for_dwrite_fontface()
 */
extern cairo_font_face_t *
cairo_dwrite_font_face_create_for_dwrite_fontface (void *dwrite_font_face);
#endif

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

static cairo_font_face_t *pango_cairo_win32_font_create_font_face                (PangoCairoFont *font);
static PangoFontMetrics  *pango_cairo_win32_font_create_base_metrics_for_context (PangoCairoFont *font,
										  PangoContext    *context);


static void
cairo_font_iface_init (PangoCairoFontIface *iface)
{
  iface->create_font_face = pango_cairo_win32_font_create_font_face;
  iface->create_base_metrics_for_context = pango_cairo_win32_font_create_base_metrics_for_context;
  iface->cf_priv_offset = G_STRUCT_OFFSET (PangoCairoWin32Font, cf_priv);
}

G_DEFINE_TYPE_WITH_CODE (PangoCairoWin32Font, pango_cairo_win32_font, PANGO_TYPE_WIN32_FONT,
                         G_IMPLEMENT_INTERFACE (PANGO_TYPE_CAIRO_FONT, cairo_font_iface_init))

static cairo_font_face_t *
pango_cairo_win32_font_create_font_face (PangoCairoFont *font)
{
  PangoCairoWin32Font *cwfont = PANGO_CAIRO_WIN32_FONT (font);
  PangoWin32Font *win32font =  &cwfont->font;
  void *dwrite_font_face = NULL;

#ifdef HAVE_CAIRO_WIN32_DIRECTWRITE
  dwrite_font_face = pango_win32_font_get_dwrite_font_face (win32font);
  if (dwrite_font_face != NULL)
    {
      cairo_font_face_t *cairo_face = cairo_dwrite_font_face_create_for_dwrite_fontface (dwrite_font_face);

      if (cairo_font_face_status (cairo_face) == CAIRO_STATUS_SUCCESS)
        {
          static const cairo_user_data_key_t key;

          cairo_font_face_set_user_data (cairo_face, &key,
                                         dwrite_font_face,
                                         (cairo_destroy_func_t) pango_win32_dwrite_font_face_release);

          return cairo_face;
        }
      else
        {
          g_warning ("cairo_font_face creation failed with DirectWrite, fallback to GDI");
          pango_win32_dwrite_font_face_release (dwrite_font_face);
          cairo_font_face_destroy (cairo_face);
        }
    }
#endif

  return cairo_win32_font_face_create_for_logfontw (&win32font->logfontw);
}

static PangoFontMetrics *
pango_cairo_win32_font_create_base_metrics_for_context (PangoCairoFont *font,
							PangoContext   *context)
{
  PangoFontMetrics *metrics;
  cairo_scaled_font_t *scaled_font;
  cairo_font_extents_t font_extents;

  metrics = pango_font_metrics_new ();

  scaled_font = pango_cairo_font_get_scaled_font (font);

  cairo_scaled_font_extents (scaled_font, &font_extents);
  cairo_win32_scaled_font_done_font (scaled_font);

  metrics->ascent = font_extents.ascent * PANGO_SCALE;
  metrics->descent = font_extents.descent * PANGO_SCALE;

  /* FIXME: Should get the real settings for these from the TrueType
   * font file.
   */
  metrics->height = metrics->ascent + metrics->descent;
  metrics->underline_thickness = metrics->height / 14;
  metrics->underline_position = - metrics->underline_thickness;
  metrics->strikethrough_thickness = metrics->underline_thickness;
  metrics->strikethrough_position = metrics->height / 4;

  pango_quantize_line_geometry (&metrics->underline_thickness,
				&metrics->underline_position);
  pango_quantize_line_geometry (&metrics->strikethrough_thickness,
				&metrics->strikethrough_position);
  /* Quantizing may have pushed underline_position to 0.  Not good */
  if (metrics->underline_position == 0)
    metrics->underline_position = - metrics->underline_thickness;

  return metrics;
}

static void
pango_cairo_win32_font_finalize (GObject *object)
{
  PangoCairoWin32Font *cwfont = (PangoCairoWin32Font *) object;

  _pango_cairo_font_private_finalize (&cwfont->cf_priv);

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
pango_cairo_win32_font_init (PangoCairoWin32Font *cwfont G_GNUC_UNUSED)
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
  PangoWin32FontMap *win32fontmap;
  PangoCairoWin32Font *cwfont;
  PangoWin32Font *win32font;
  double size;
  double dpi;
  GSList *tmp_list;
  cairo_matrix_t font_matrix;
  cairo_font_options_t *options;
  const char *variations;

  g_return_val_if_fail (PANGO_IS_CAIRO_WIN32_FONT_MAP (cwfontmap), NULL);

  size = (double) pango_font_description_get_size (desc) / PANGO_SCALE;

  if (context)
    {
      dpi = pango_cairo_context_get_resolution (context);

      if (dpi <= 0)
        dpi = ((PangoWin32FontMap *) cwfontmap)->dpi;

      options = cairo_font_options_copy (_pango_cairo_context_get_merged_font_options (context));
    }
  else
    {
      dpi = ((PangoWin32FontMap *) cwfontmap)->dpi;
      options = cairo_font_options_create ();
    }

  if (!pango_font_description_get_size_is_absolute (desc))
    size *= dpi / 72.;

  variations = pango_font_description_get_variations (desc);
  cairo_font_options_set_variations (options, variations);

  win32fontmap = PANGO_WIN32_FONT_MAP (cwfontmap);

  tmp_list = face->cached_fonts;
  while (tmp_list)
    {
      cairo_font_options_t *options2;

      win32font = tmp_list->data;
      cwfont = PANGO_CAIRO_WIN32_FONT (win32font);

      options2 = cairo_font_options_create ();
      pango_cairo_font_get_font_options ((PangoCairoFont *) cwfont, options2);

      if (ABS (win32font->size - size * PANGO_SCALE) < 2 &&
          cairo_font_options_equal (options, options2))
        {
          g_object_ref (win32font);
          if (win32font->in_cache)
            _pango_win32_fontmap_cache_remove (PANGO_FONT_MAP (win32fontmap), win32font);

          cairo_font_options_destroy (options);
          cairo_font_options_destroy (options2);

          return PANGO_FONT (win32font);
        }

      cairo_font_options_destroy (options2);
      tmp_list = tmp_list->next;
    }

  cwfont = g_object_new (PANGO_TYPE_CAIRO_WIN32_FONT, NULL);
  win32font = PANGO_WIN32_FONT (cwfont);

  g_assert (win32font->fontmap == NULL);

  if (cwfontmap)
    {
      win32font->fontmap = (PangoFontMap *) cwfontmap;
      g_object_add_weak_pointer (G_OBJECT (win32font->fontmap), (gpointer *) &win32font->fontmap);
    }

  win32font->win32face = g_object_ref (face);
  face->cached_fonts = g_slist_prepend (face->cached_fonts, win32font);

  /* FIXME: This is a pixel size, so not really what we want for describe(),
   * but it's what we need when computing the scale factor.
   */
  win32font->size = size * PANGO_SCALE;

  win32font->variations = g_strdup (variations);

  _pango_win32_make_matching_logfontw (win32font->fontmap,
				       &face->logfontw,
				       win32font->size,
				       &win32font->logfontw);

  win32font->is_hinted = pango_win32_dwrite_font_check_is_hinted (win32font);
  cairo_matrix_init_identity (&font_matrix);

  cairo_matrix_scale (&font_matrix, size, size);

  _pango_cairo_font_private_initialize (&cwfont->cf_priv,
					(PangoCairoFont *) cwfont,
					pango_font_description_get_gravity (desc),
                                        options,
					pango_context_get_matrix (context),
					&font_matrix);

  cairo_font_options_destroy (options);

  return PANGO_FONT (cwfont);
}
