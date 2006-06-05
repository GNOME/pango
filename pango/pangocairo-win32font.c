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

#include <config.h>

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
typedef struct _PangoCairoWin32GlyphInfo PangoCairoWin32GlyphInfo;

struct _PangoCairoWin32Font
{
  PangoWin32Font font;

  int size;

  cairo_font_face_t *font_face;
  cairo_scaled_font_t *scaled_font;
  
  cairo_matrix_t font_matrix;
  cairo_matrix_t ctm;
  cairo_font_options_t *options;
  
  GSList *metrics_by_lang;
  GHashTable *glyph_info;
};

struct _PangoCairoWin32FontClass
{
  PangoWin32FontClass  parent_class;
};

struct _PangoCairoWin32GlyphInfo
{
  PangoRectangle logical_rect;
  PangoRectangle ink_rect;
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

      /* Count here on the fact that all the struct fields are the
       * same for LOGFONTW and LOGFONTA except lfFaceName which is the
       * last field
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
	g_error ("Unable to create Win32 cairo font face.\nThis means out of memory or a cairo/fontconfig/FreeType bug");
    }
  
  return cwfont->font_face;
}

static cairo_scaled_font_t *
pango_cairo_win32_font_get_scaled_font (PangoCairoFont *font)
{
  PangoCairoWin32Font *cwfont = PANGO_CAIRO_WIN32_FONT (font);

  if (!cwfont->scaled_font)
    {
      cairo_font_face_t *font_face;

      font_face = pango_cairo_win32_font_get_font_face (font);
      cwfont->scaled_font = cairo_scaled_font_create (font_face,
						      &cwfont->font_matrix,
						      &cwfont->ctm,
						      cwfont->options);

      /* Failure of the above should only occur for out of memory,
       * we can't proceed at that point
       */
      if (!cwfont->scaled_font)
	g_error ("Unable to create Win32 cairo scaled font.\nThis means out of memory or a cairo/fontconfig/FreeType bug");
    }
  
  return cwfont->scaled_font;
}

/********************************
 *    Method implementations    *
 ********************************/

static gboolean
pango_cairo_win32_font_install (PangoCairoFont *font,
				cairo_t        *cr)
{
  PangoCairoWin32Font *cwfont = PANGO_CAIRO_WIN32_FONT (font);

  cairo_set_font_face (cr,
		       pango_cairo_win32_font_get_font_face (font));
  cairo_set_font_matrix (cr, &cwfont->font_matrix);
  cairo_set_font_options (cr, cwfont->options);

  return TRUE;
}

static void
cairo_font_iface_init (PangoCairoFontIface *iface)
{
  iface->install = pango_cairo_win32_font_install;
  iface->get_font_face = pango_cairo_win32_font_get_font_face;
  iface->get_scaled_font = pango_cairo_win32_font_get_scaled_font;
}

G_DEFINE_TYPE_WITH_CODE (PangoCairoWin32Font, pango_cairo_win32_font, PANGO_TYPE_WIN32_FONT,
    { G_IMPLEMENT_INTERFACE (PANGO_TYPE_CAIRO_FONT, cairo_font_iface_init) });

static void
free_metrics_info (PangoWin32MetricsInfo *info)
{
  pango_font_metrics_unref (info->metrics);
  g_slice_free (PangoWin32MetricsInfo, info);
}

static void
pango_cairo_win32_font_finalize (GObject *object)
{
  PangoCairoWin32Font *cwfont = PANGO_CAIRO_WIN32_FONT (object);

  g_hash_table_destroy (cwfont->glyph_info);

  g_slist_foreach (cwfont->metrics_by_lang, (GFunc)free_metrics_info, NULL);
  g_slist_free (cwfont->metrics_by_lang);  

  if (cwfont->scaled_font)
    cairo_scaled_font_destroy (cwfont->scaled_font);

  if (cwfont->options)
    cairo_font_options_destroy (cwfont->options);

  G_OBJECT_CLASS (pango_cairo_win32_font_parent_class)->finalize (object);
}

static void
compute_glyph_extents (PangoFont        *font,
		       PangoGlyph        glyph,
		       PangoRectangle   *ink_rect,
		       PangoRectangle   *logical_rect)
{
  PangoCairoFont *cfont = (PangoCairoFont *)font;
  cairo_scaled_font_t *scaled_font = pango_cairo_win32_font_get_scaled_font (PANGO_CAIRO_FONT (cfont));

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

  if (glyph == PANGO_GLYPH_EMPTY)
    {
      /* already initialized above */
    }
  else if (glyph & PANGO_GLYPH_UNKNOWN_FLAG)
    {
      /* space for the hex box */
      _pango_cairo_get_glyph_extents_missing(cfont, glyph, ink_rect, logical_rect);
    }
  else
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

static PangoCairoWin32GlyphInfo *
pango_cairo_win32_font_get_glyph_info (PangoFont   *font,
				       PangoGlyph   glyph)
{
  PangoCairoWin32Font *cwfont = (PangoCairoWin32Font *)font;
  PangoCairoWin32GlyphInfo *info;

  info = g_hash_table_lookup (cwfont->glyph_info, GUINT_TO_POINTER (glyph));
  if (!info)
    {
      info = g_new0 (PangoCairoWin32GlyphInfo, 1);
      
      compute_glyph_extents (font, glyph,
			     &info->ink_rect,
			     &info->logical_rect);
      
      g_hash_table_insert (cwfont->glyph_info, GUINT_TO_POINTER (glyph), info);
    }

  return info;
}
     
static void
pango_cairo_win32_font_get_glyph_extents (PangoFont        *font,
					  PangoGlyph        glyph,
					  PangoRectangle   *ink_rect,
					  PangoRectangle   *logical_rect)
{
  PangoCairoWin32GlyphInfo *info;

  info = pango_cairo_win32_font_get_glyph_info (font, glyph);
  
  if (ink_rect)
    *ink_rect = info->ink_rect;
  if (logical_rect)
    *logical_rect = info->logical_rect;
}

static int
max_glyph_width (PangoLayout *layout)
{
  int max_width = 0;
  GSList *l, *r;

  for (l = pango_layout_get_lines (layout); l; l = l->next)
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
create_metrics_for_context (PangoFont    *font,
			    PangoContext *context)
{
  PangoCairoWin32Font *cwfont = PANGO_CAIRO_WIN32_FONT (font);
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
  
  scaled_font = pango_cairo_win32_font_get_scaled_font (PANGO_CAIRO_FONT (cwfont));

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

  layout = pango_layout_new (context);
  font_desc = pango_font_describe_with_absolute_size (font);
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

static PangoFontMetrics *
pango_cairo_win32_font_get_metrics (PangoFont        *font,
				    PangoLanguage    *language)
{
  PangoWin32Font *win32font = PANGO_WIN32_FONT (font);
  PangoCairoWin32Font *cwfont = PANGO_CAIRO_WIN32_FONT (font);
  PangoWin32MetricsInfo *info = NULL; /* Quiet gcc */
  GSList *tmp_list;      
  const char *sample_str = pango_language_get_sample_string (language);
  
  tmp_list = cwfont->metrics_by_lang;
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

      if (!win32font->fontmap)
	return pango_font_metrics_new ();

      info = g_slice_new0 (PangoWin32MetricsInfo);
      
      cwfont->metrics_by_lang = g_slist_prepend (cwfont->metrics_by_lang, 
						 info);
	
      info->sample_str = sample_str;

      context = pango_context_new ();
      pango_context_set_font_map (context, win32font->fontmap);
      pango_context_set_language (context, language);
      pango_cairo_context_set_font_options (context, cwfont->options);

      info->metrics = create_metrics_for_context (font, context);

      g_object_unref (context);
    }

  return pango_font_metrics_ref (info->metrics);
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
  cwfont->glyph_info = g_hash_table_new_full (g_direct_hash,
					      NULL,
					      NULL,
					      (GDestroyNotify)g_free);
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
#define USE_FACE_CACHED_FONTS
#ifdef USE_FACE_CACHED_FONTS
  PangoWin32FontMap *win32fontmap;
  GSList *tmp_list;
#endif

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
	    pango_win32_fontmap_cache_remove (PANGO_FONT_MAP (win32fontmap), win32font);
	  
	  return PANGO_FONT (win32font);
	}
      tmp_list = tmp_list->next;
    }
#endif
  cwfont = g_object_new (PANGO_TYPE_CAIRO_WIN32_FONT, NULL);
  win32font = PANGO_WIN32_FONT (cwfont);
  
  win32font->fontmap = PANGO_FONT_MAP (cwfontmap);
  g_object_ref (cwfontmap);

  win32font->win32face = face;

#ifdef USE_FACE_CACHED_FONTS
  face->cached_fonts = g_slist_prepend (face->cached_fonts, win32font);
#endif

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
