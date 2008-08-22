/* Pango
 * pangowin32.c: Routines for handling Windows fonts
 *
 * Copyright (C) 1999 Red Hat Software
 * Copyright (C) 2000 Tor Lillqvist
 * Copyright (C) 2001 Alexander Larsson
 * Copyright (C) 2007 Novell, Inc.
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
#include <stdlib.h>
#include <glib.h>

#include "pango-impl-utils.h"
#include "pangowin32.h"
#include "pangowin32-private.h"

#define MAX_FREED_FONTS 16

#define CH_IS_UNIHAN_BMP(ch) ((ch) >= 0x3400 && (ch) <= 0x9FFF)
#define CH_IS_UNIHAN(ch) (CH_IS_UNIHAN_BMP (ch) || \
			  ((ch) >= 0x20000 && (ch) <= 0x2A6DF) ||	\
			  ((ch) >= 0x2F800 && (ch) <= 0x2FA1F))

HDC _pango_win32_hdc;
OSVERSIONINFO _pango_win32_os_version_info;
gboolean _pango_win32_debug = FALSE;

static void pango_win32_font_dispose    (GObject             *object);
static void pango_win32_font_finalize   (GObject             *object);

static gboolean pango_win32_font_real_select_font        (PangoFont *font,
							  HDC        hdc);
static void     pango_win32_font_real_done_font          (PangoFont *font);
static double   pango_win32_font_real_get_metrics_factor (PangoFont *font);

static PangoFontDescription *pango_win32_font_describe          (PangoFont        *font);
static PangoFontDescription *pango_win32_font_describe_absolute (PangoFont        *font);
static PangoCoverage        *pango_win32_font_get_coverage      (PangoFont        *font,
								 PangoLanguage    *lang);
static void                  pango_win32_font_calc_coverage     (PangoFont        *font,
								 PangoCoverage    *coverage,
								 PangoLanguage    *lang);
static PangoEngineShape     *pango_win32_font_find_shaper       (PangoFont        *font,
								 PangoLanguage    *lang,
								 guint32           ch);
static void                  pango_win32_font_get_glyph_extents (PangoFont        *font,
								 PangoGlyph        glyph,
								 PangoRectangle   *ink_rect,
								 PangoRectangle   *logical_rect);
static PangoFontMetrics *    pango_win32_font_get_metrics       (PangoFont        *font,
								 PangoLanguage    *lang);
static PangoFontMap *        pango_win32_font_get_font_map      (PangoFont        *font);

static gboolean pango_win32_font_real_select_font      (PangoFont *font,
							HDC        hdc);
static void     pango_win32_font_real_done_font        (PangoFont *font);
static double   pango_win32_font_real_get_metrics_factor (PangoFont *font);

static HFONT                 pango_win32_get_hfont              (PangoFont        *font);
static void                  pango_win32_get_item_properties    (PangoItem        *item,
								 PangoUnderline   *uline,
								 PangoAttrColor   *fg_color,
								 gboolean         *fg_set,
								 PangoAttrColor   *bg_color,
								 gboolean         *bg_set);

static HFONT
pango_win32_get_hfont (PangoFont *font)
{
  PangoWin32Font *win32font = (PangoWin32Font *)font;
  PangoWin32FontCache *cache;
  TEXTMETRIC tm;

  if (!win32font)
    return NULL;

  if (!win32font->hfont)
    {
      cache = pango_win32_font_map_get_font_cache (win32font->fontmap);

      win32font->hfont = pango_win32_font_cache_loadw (cache, &win32font->logfontw);
      if (!win32font->hfont)
	{
	  gchar *face_utf8 = g_utf16_to_utf8 (win32font->logfontw.lfFaceName,
					      -1, NULL, NULL, NULL);
	  g_warning ("Cannot load font '%s\n", face_utf8);
	  g_free (face_utf8);
	  return NULL;
	}

      SelectObject (_pango_win32_hdc, win32font->hfont);
      GetTextMetrics (_pango_win32_hdc, &tm);

      win32font->tm_overhang = tm.tmOverhang;
      win32font->tm_descent = tm.tmDescent;
      win32font->tm_ascent = tm.tmAscent;
    }

  return win32font->hfont;
}

/**
 * pango_win32_get_context:
 *
 * Retrieves a #PangoContext appropriate for rendering with Windows fonts.
 *
 * Return value: the new #PangoContext
 *
 * Deprecated: 1.22: Use pango_win32_font_map_for_display() followed by
 * pango_font_map_create_context() instead.
 **/
PangoContext *
pango_win32_get_context (void)
{
  return pango_font_map_create_context (pango_win32_font_map_for_display ());
}

G_DEFINE_TYPE (PangoWin32Font, _pango_win32_font, PANGO_TYPE_FONT)

static void
_pango_win32_font_init (PangoWin32Font *win32font)
{
  win32font->size = -1;

  win32font->metrics_by_lang = NULL;

  win32font->glyph_info = g_hash_table_new_full (NULL, NULL, NULL, g_free);
}

/**
 * pango_win32_get_dc:
 *
 * Obtains a handle to the Windows device context that is used by Pango.
 *
 * Return value: A handle to the Windows device context that is used by Pango.
 **/
HDC
pango_win32_get_dc (void)
{
  if (_pango_win32_hdc == NULL)
    {
      _pango_win32_hdc = CreateDC ("DISPLAY", NULL, NULL, NULL);
      memset (&_pango_win32_os_version_info, 0,
	      sizeof (_pango_win32_os_version_info));
      _pango_win32_os_version_info.dwOSVersionInfoSize =
	sizeof (OSVERSIONINFO);
      GetVersionEx (&_pango_win32_os_version_info);

      /* Also do some generic pangowin32 initialisations... this function
       * is a suitable place for those as it is called from a couple
       * of class_init functions.
       */
#ifdef PANGO_WIN32_DEBUGGING
      if (getenv ("PANGO_WIN32_DEBUG") != NULL)
	_pango_win32_debug = TRUE;
#endif
    }

  return _pango_win32_hdc;
}

/**
 * pango_win32_get_debug_flag:
 *
 * Returns whether debugging is turned on.
 *
 * Return value: %TRUE if debugging is turned on.
 *
 * Since: 1.2
 */
gboolean
pango_win32_get_debug_flag (void)
{
  return _pango_win32_debug;
}

static void
_pango_win32_font_class_init (PangoWin32FontClass *class)
{
  GObjectClass *object_class = G_OBJECT_CLASS (class);
  PangoFontClass *font_class = PANGO_FONT_CLASS (class);

  object_class->finalize = pango_win32_font_finalize;
  object_class->dispose = pango_win32_font_dispose;

  font_class->describe = pango_win32_font_describe;
  font_class->describe_absolute = pango_win32_font_describe_absolute;
  font_class->get_coverage = pango_win32_font_get_coverage;
  font_class->find_shaper = pango_win32_font_find_shaper;
  font_class->get_glyph_extents = pango_win32_font_get_glyph_extents;
  font_class->get_metrics = pango_win32_font_get_metrics;
  font_class->get_font_map = pango_win32_font_get_font_map;

  class->select_font = pango_win32_font_real_select_font;
  class->done_font = pango_win32_font_real_done_font;
  class->get_metrics_factor = pango_win32_font_real_get_metrics_factor;

  pango_win32_get_dc ();
}

/**
 * pango_win32_render:
 * @hdc:     the device context
 * @font:    the font in which to draw the string
 * @glyphs:  the glyph string to draw
 * @x:       the x position of start of string (in pixels)
 * @y:       the y position of baseline (in pixels)
 *
 * Render a #PangoGlyphString onto a Windows DC
 */
void
pango_win32_render (HDC               hdc,
		    PangoFont        *font,
		    PangoGlyphString *glyphs,
		    int               x,
		    int               y)
{
  HFONT hfont, old_hfont = NULL;
  int i, j, num_valid_glyphs;
  guint16 *glyph_indexes;
  gint *dX;
  gint this_x;
  gint start_x_offset, x_offset, next_x_offset, cur_y_offset; /* in Pango units */

  g_return_if_fail (glyphs != NULL);

#ifdef PANGO_WIN32_DEBUGGING
  if (_pango_win32_debug)
    {
      PING (("num_glyphs:%d", glyphs->num_glyphs));
      for (i = 0; i < glyphs->num_glyphs; i++)
	{
	  g_print (" %d:%d", glyphs->glyphs[i].glyph, glyphs->glyphs[i].geometry.width);
	  if (glyphs->glyphs[i].geometry.x_offset != 0 ||
	      glyphs->glyphs[i].geometry.y_offset != 0)
	    g_print (":%d,%d", glyphs->glyphs[i].geometry.x_offset,
		     glyphs->glyphs[i].geometry.y_offset);
	}
      g_print ("\n");
    }
#endif

  if (glyphs->num_glyphs == 0)
    return;

  hfont = pango_win32_get_hfont (font);
  if (!hfont)
    return;

  old_hfont = SelectObject (hdc, hfont);

  glyph_indexes = g_new (guint16, glyphs->num_glyphs);
  dX = g_new (INT, glyphs->num_glyphs);

  /* Render glyphs using one ExtTextOutW() call for each run of glyphs
   * that have the same y offset. The big majoroty of glyphs will have
   * y offset of zero, so in general, the whole glyph string will be
   * rendered by one call to ExtTextOutW().
   *
   * In order to minimize buildup of rounding errors, we keep track of
   * where the glyphs should be rendered in Pango units, and round
   * to pixels separately for each glyph,
   */

  i = 0;

  /* Outer loop through all glyphs in string */
  while (i < glyphs->num_glyphs)
    {
      cur_y_offset = glyphs->glyphs[i].geometry.y_offset;
      num_valid_glyphs = 0;
      x_offset = 0;
      start_x_offset = glyphs->glyphs[i].geometry.x_offset;
      this_x = PANGO_PIXELS (start_x_offset);

      /* Inner loop through glyphs with the same y offset, or code
       * point zero (just spacing).
       */
      while (i < glyphs->num_glyphs &&
	     (glyphs->glyphs[i].glyph == PANGO_GLYPH_EMPTY ||
	      cur_y_offset == glyphs->glyphs[i].geometry.y_offset))
	{
	  if (glyphs->glyphs[i].glyph == PANGO_GLYPH_EMPTY)
	    {
	      /* PANGO_GLYPH_EMPTY glyphs should not be rendered, but their
	       * indicated width (set up by PangoLayout) should be taken
	       * into account.
	       */

	      /* If the string starts with spacing, must shift the
	       * starting point for the glyphs actually rendered.  For
	       * spacing in the middle of the glyph string, add to the dX
	       * of the previous glyph to be rendered.
	       */
	      if (num_valid_glyphs == 0)
		start_x_offset += glyphs->glyphs[i].geometry.width;
	      else
		{
		  x_offset += glyphs->glyphs[i].geometry.width;
		  dX[num_valid_glyphs-1] = PANGO_PIXELS (x_offset) - this_x;
		}
	    }
	  else
	    {
	      if (glyphs->glyphs[i].glyph & PANGO_GLYPH_UNKNOWN_FLAG)
		{
		  /* Glyph index is actually the char value that doesn't
		   * have any glyph (ORed with the flag). We should really
		   * do the same that pango_xft_real_render() does: render
		   * a box with the char value in hex inside it in a tiny
		   * font. Later. For now, use the TrueType invalid glyph
		   * at 0.
		   */
		  glyph_indexes[num_valid_glyphs] = 0;
		}
	      else
		glyph_indexes[num_valid_glyphs] = glyphs->glyphs[i].glyph;

	      x_offset += glyphs->glyphs[i].geometry.width;

	      /* If the next glyph has an X offset, take that into consideration now */
	      if (i < glyphs->num_glyphs - 1)
		next_x_offset = glyphs->glyphs[i+1].geometry.x_offset;
	      else
		next_x_offset = 0;

	      dX[num_valid_glyphs] = PANGO_PIXELS (x_offset + next_x_offset) - this_x;

	      /* Prepare for next glyph */
	      this_x += dX[num_valid_glyphs];
	      num_valid_glyphs++;
	    }
	  i++;
	}
#ifdef PANGO_WIN32_DEBUGGING
      if (_pango_win32_debug)
	{
	  g_print ("ExtTextOutW at %d,%d deltas:",
		   x + PANGO_PIXELS (start_x_offset),
		   y + PANGO_PIXELS (cur_y_offset));
	  for (j = 0; j < num_valid_glyphs; j++)
	    g_print (" %d", dX[j]);
	  g_print ("\n");
	}
#endif

      ExtTextOutW (hdc,
		   x + PANGO_PIXELS (start_x_offset),
		   y + PANGO_PIXELS (cur_y_offset),
		   ETO_GLYPH_INDEX,
		   NULL,
		   glyph_indexes, num_valid_glyphs,
		   dX);
      x += this_x;
    }


  SelectObject (hdc, old_hfont); /* restore */
  g_free (glyph_indexes);
  g_free (dX);
}

/**
 * pango_win32_render_transformed:
 * @hdc:     a windows device context
 * @matrix:  a #PangoMatrix, or %NULL to use an identity transformation
 * @font:    the font in which to draw the string
 * @glyphs:  the glyph string to draw
 * @x:       the x position of the start of the string (in Pango
 *           units in user space coordinates)
 * @y:       the y position of the baseline (in Pango units
 *           in user space coordinates)
 *
 * Renders a #PangoGlyphString onto a windows DC, possibly
 * transforming the layed-out coordinates through a transformation
 * matrix. Note that the transformation matrix for @font is not
 * changed, so to produce correct rendering results, the @font
 * must have been loaded using a #PangoContext with an identical
 * transformation matrix to that passed in to this function.
 **/
void
pango_win32_render_transformed (HDC                hdc,
				const PangoMatrix *matrix,
				PangoFont         *font,
				PangoGlyphString  *glyphs,
				int                x,
				int                y)
{
  XFORM xForm;
  XFORM xFormPrev = {1.0, 0.0, 0.0, 1.0, 0.0, 0.0};
  int   mode = GetGraphicsMode (hdc);

  if (!SetGraphicsMode (hdc, GM_ADVANCED))
    g_warning ("SetGraphicsMode() failed");
  else if (!GetWorldTransform (hdc, &xFormPrev))
    g_warning ("GetWorldTransform() failed");
  else if (matrix)
    {
      xForm.eM11 = matrix->xx;
      xForm.eM12 = matrix->yx;
      xForm.eM21 = matrix->xy;
      xForm.eM22 = matrix->yy;
      xForm.eDx = matrix->x0;
      xForm.eDy = matrix->y0;
      if (!SetWorldTransform (hdc, &xForm))
	g_warning ("GetWorldTransform() failed");
    }

  pango_win32_render (hdc, font, glyphs, x/PANGO_SCALE, y/PANGO_SCALE);

  /* restore */
  SetWorldTransform (hdc, &xFormPrev);
  SetGraphicsMode (hdc, mode);
}

static void
pango_win32_font_get_glyph_extents (PangoFont      *font,
				    PangoGlyph      glyph,
				    PangoRectangle *ink_rect,
				    PangoRectangle *logical_rect)
{
  PangoWin32Font *win32font = (PangoWin32Font *)font;
  guint16 glyph_index = glyph;
  GLYPHMETRICS gm;
  guint32 res;
  HFONT hfont;
  MAT2 m = {{0,1}, {0,0}, {0,0}, {0,1}};
  PangoWin32GlyphInfo *info;

  if (glyph == PANGO_GLYPH_EMPTY)
    {
      if (ink_rect)
	ink_rect->x = ink_rect->width = ink_rect->y = ink_rect->height = 0;
      if (logical_rect)
	logical_rect->x = logical_rect->width = logical_rect->y = logical_rect->height = 0;
      return;
    }

  if (glyph & PANGO_GLYPH_UNKNOWN_FLAG)
    glyph_index = glyph = 0;

  info = g_hash_table_lookup (win32font->glyph_info, GUINT_TO_POINTER (glyph));

  if (!info)
    {
      info = g_new0 (PangoWin32GlyphInfo, 1);

      memset (&gm, 0, sizeof (gm));

      hfont = pango_win32_get_hfont (font);
      SelectObject (_pango_win32_hdc, hfont);
      /* FIXME: (Alex) This constant reuse of _pango_win32_hdc is
	 not thread-safe */
      res = GetGlyphOutlineA (_pango_win32_hdc,
			      glyph_index,
			      GGO_METRICS | GGO_GLYPH_INDEX,
			      &gm,
			      0, NULL,
			      &m);

      if (res == GDI_ERROR)
	{
	  gchar *error = g_win32_error_message (GetLastError ());
	  g_warning ("GetGlyphOutline(%04X) failed: %s\n",
		     glyph_index, error);
	  g_free (error);

	  /* Don't just return now, use the still zeroed out gm */
	}

      info->ink_rect.x = PANGO_SCALE * gm.gmptGlyphOrigin.x;
      info->ink_rect.width = PANGO_SCALE * gm.gmBlackBoxX;
      info->ink_rect.y = - PANGO_SCALE * gm.gmptGlyphOrigin.y;
      info->ink_rect.height = PANGO_SCALE * gm.gmBlackBoxY;

      info->logical_rect.x = 0;
      info->logical_rect.width = PANGO_SCALE * gm.gmCellIncX;
      info->logical_rect.y = - PANGO_SCALE * win32font->tm_ascent;
      info->logical_rect.height = PANGO_SCALE * (win32font->tm_ascent + win32font->tm_descent);

      g_hash_table_insert (win32font->glyph_info, GUINT_TO_POINTER(glyph), info);
    }

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
pango_win32_font_get_metrics (PangoFont     *font,
			      PangoLanguage *language)
{
  PangoWin32MetricsInfo *info = NULL; /* Quiet gcc */
  PangoWin32Font *win32font = (PangoWin32Font *)font;
  GSList *tmp_list;

  const char *sample_str = pango_language_get_sample_string (language);

  tmp_list = win32font->metrics_by_lang;
  while (tmp_list)
    {
      info = tmp_list->data;

      if (info->sample_str == sample_str)    /* We _don't_ need strcmp */
	break;

      tmp_list = tmp_list->next;
    }

  if (!tmp_list)
    {
      HFONT hfont;
      PangoFontMetrics *metrics;

      info = g_new (PangoWin32MetricsInfo, 1);
      win32font->metrics_by_lang = g_slist_prepend (win32font->metrics_by_lang, info);

      info->sample_str = sample_str;
      info->metrics = metrics = pango_font_metrics_new ();

      hfont = pango_win32_get_hfont (font);
      if (hfont != NULL)
	{
	  PangoCoverage *coverage;
	  TEXTMETRIC tm;

	  SelectObject (_pango_win32_hdc, hfont);
	  GetTextMetrics (_pango_win32_hdc, &tm);

	  metrics->ascent = tm.tmAscent * PANGO_SCALE;
	  metrics->descent = tm.tmDescent * PANGO_SCALE;
	  metrics->approximate_char_width = tm.tmAveCharWidth * PANGO_SCALE;

	  coverage = pango_win32_font_get_coverage (font, language);
	  if (pango_coverage_get (coverage, '0') != PANGO_COVERAGE_NONE &&
	      pango_coverage_get (coverage, '9') != PANGO_COVERAGE_NONE)
	    {
	      PangoContext *context;
	      PangoFontDescription *font_desc;
	      PangoLayout *layout;

	      /*  Get the average width of the chars in "0123456789" */
	      context = pango_font_map_create_context (pango_win32_font_map_for_display ());
	      pango_context_set_language (context, language);
	      font_desc = pango_font_describe_with_absolute_size (font);
	      pango_context_set_font_description (context, font_desc);
	      layout = pango_layout_new (context);
	      pango_layout_set_text (layout, "0123456789", -1);

	      metrics->approximate_digit_width = max_glyph_width (layout);

	      pango_font_description_free (font_desc);
	      g_object_unref (layout);
	      g_object_unref (context);
	    }
	  else
	    metrics->approximate_digit_width = metrics->approximate_char_width;

	  pango_coverage_unref (coverage);

	  /* FIXME: Should get the real values from the TrueType font file */
	  metrics->underline_position = -2 * PANGO_SCALE;
	  metrics->underline_thickness = 1 * PANGO_SCALE;
	  metrics->strikethrough_thickness = metrics->underline_thickness;
	  /* Really really wild guess */
	  metrics->strikethrough_position = metrics->ascent / 3;
	}
    }

  return pango_font_metrics_ref (info->metrics);
}

static PangoFontMap *
pango_win32_font_get_font_map (PangoFont *font)
{
  PangoWin32Font *win32font = (PangoWin32Font *)font;

  return win32font->fontmap;
}

static gboolean
pango_win32_font_real_select_font (PangoFont *font,
				   HDC        hdc)
{
  HFONT hfont = pango_win32_get_hfont (font);

  if (!hfont)
    return FALSE;

  if (!SelectObject (hdc, hfont))
    {
      g_warning ("pango_win32_font_real_select_font: Cannot select font\n");
      return FALSE;
    }

  return TRUE;
}

static void
pango_win32_font_real_done_font (PangoFont *font)
{
}

static double
pango_win32_font_real_get_metrics_factor (PangoFont *font)
{
  return PANGO_SCALE;
}

/**
 * pango_win32_font_logfont:
 * @font: a #PangoFont which must be from the Win32 backend
 *
 * Determine the LOGFONTA struct for the specified font. Note that
 * Pango internally uses LOGFONTW structs, so if converting the UTF-16
 * face name in the LOGFONTW struct to system codepage fails, the
 * returned LOGFONTA will have an emppty face name. To get the
 * LOGFONTW of a PangoFont, use pango_win32_font_logfontw(). It
 * is recommended to do that always even if you don't expect
 * to come across fonts with odd names.
 *
 * Return value: A newly allocated LOGFONTA struct. It must be
 * freed with g_free().
 **/
LOGFONTA *
pango_win32_font_logfont (PangoFont *font)
{
  PangoWin32Font *win32font = (PangoWin32Font *)font;
  LOGFONTA *lfp;

  g_return_val_if_fail (font != NULL, NULL);
  g_return_val_if_fail (PANGO_WIN32_IS_FONT (font), NULL);

  lfp = g_new (LOGFONTA, 1);

  *lfp = *(LOGFONTA*) &win32font->logfontw;
  if (!WideCharToMultiByte (CP_ACP, 0,
			    win32font->logfontw.lfFaceName, -1,
			    lfp->lfFaceName, G_N_ELEMENTS (lfp->lfFaceName),
			    NULL, NULL))
    lfp->lfFaceName[0] = '\0';

  return lfp;
}

/**
 * pango_win32_font_logfontw:
 * @font: a #PangoFont which must be from the Win32 backend
 * 
 * Determine the LOGFONTW struct for the specified font.
 * 
 * Return value: A newly allocated LOGFONTW struct. It must be
 * freed with g_free().
 *
 * Since: 1.16
 **/
LOGFONTW *
pango_win32_font_logfontw (PangoFont *font)
{
  PangoWin32Font *win32font = (PangoWin32Font *)font;
  LOGFONTW *lfp;

  g_return_val_if_fail (font != NULL, NULL);
  g_return_val_if_fail (PANGO_WIN32_IS_FONT (font), NULL);

  lfp = g_new (LOGFONTW, 1);
  *lfp = win32font->logfontw;

  return lfp;
}

/**
 * pango_win32_font_select_font:
 * @font: a #PangoFont from the Win32 backend
 * @hdc: a windows device context
 *
 * Selects the font into the specified DC and changes the mapping mode
 * and world transformation of the DC appropriately for the font.
 * You may want to surround the use of this function with calls
 * to SaveDC() and RestoreDC(). Call pango_win32_font_done_font() when
 * you are done using the DC to release allocated resources.
 *
 * See pango_win32_font_get_metrics_factor() for information about
 * converting from the coordinate space used by this function
 * into Pango units.
 *
 * Return value: %TRUE if the operation succeeded.
 **/
gboolean
pango_win32_font_select_font (PangoFont *font,
			      HDC        hdc)
{
  g_return_val_if_fail (PANGO_WIN32_IS_FONT (font), FALSE);

  return PANGO_WIN32_FONT_GET_CLASS (font)->select_font (font, hdc);
}

/**
 * pango_win32_font_done_font:
 * @font: a #PangoFont from the win32 backend
 *
 * Releases any resources allocated by pango_win32_font_done_font()
 **/
void
pango_win32_font_done_font (PangoFont *font)
{
  g_return_if_fail (PANGO_WIN32_IS_FONT (font));

  PANGO_WIN32_FONT_GET_CLASS (font)->done_font (font);
}

/**
 * pango_win32_font_get_metrics_factor:
 * @font: a #PangoFont from the win32 backend
 *
 * Returns the scale factor from logical units in the coordinate
 * space used by pango_win32_font_select_font() to Pango units
 * in user space.
 *
 * Return value: factor to multiply logical units by to get Pango
 *               units.
 **/
double
pango_win32_font_get_metrics_factor (PangoFont *font)
{
  g_return_val_if_fail (PANGO_WIN32_IS_FONT (font), 1.);

  return PANGO_WIN32_FONT_GET_CLASS (font)->get_metrics_factor (font);
}

static void
pango_win32_fontmap_cache_add (PangoFontMap   *fontmap,
			       PangoWin32Font *win32font)
{
  PangoWin32FontMap *win32fontmap = PANGO_WIN32_FONT_MAP (fontmap);

  if (win32fontmap->freed_fonts->length == MAX_FREED_FONTS)
    {
      PangoWin32Font *old_font = g_queue_pop_tail (win32fontmap->freed_fonts);
      g_object_unref (old_font);
    }

  g_object_ref (win32font);
  g_queue_push_head (win32fontmap->freed_fonts, win32font);
  win32font->in_cache = TRUE;
}

static void
pango_win32_font_dispose (GObject *object)
{
  PangoWin32Font *win32font = PANGO_WIN32_FONT (object);

  /* If the font is not already in the freed-fonts cache, add it,
   * if it is already there, do nothing and the font will be
   * freed.
   */
  if (!win32font->in_cache && win32font->fontmap)
    pango_win32_fontmap_cache_add (win32font->fontmap, win32font);

  G_OBJECT_CLASS (_pango_win32_font_parent_class)->dispose (object);
}

static void
free_metrics_info (PangoWin32MetricsInfo *info)
{
  pango_font_metrics_unref (info->metrics);
  g_free (info);
}

static void
pango_win32_font_entry_remove (PangoWin32Face *face,
			       PangoFont      *font)
{
  face->cached_fonts = g_slist_remove (face->cached_fonts, font);
}

static void
pango_win32_font_finalize (GObject *object)
{
  PangoWin32Font *win32font = (PangoWin32Font *)object;
  PangoWin32FontCache *cache = pango_win32_font_map_get_font_cache (win32font->fontmap);

  if (win32font->hfont != NULL)
    pango_win32_font_cache_unload (cache, win32font->hfont);

  g_slist_foreach (win32font->metrics_by_lang, (GFunc)free_metrics_info, NULL);
  g_slist_free (win32font->metrics_by_lang);

  if (win32font->win32face)
    pango_win32_font_entry_remove (win32font->win32face, PANGO_FONT (win32font));

  g_hash_table_destroy (win32font->glyph_info);

  g_assert (win32font->fontmap != NULL);
  g_object_remove_weak_pointer (G_OBJECT (win32font->fontmap), (gpointer *) (gpointer) &win32font->fontmap);
  win32font->fontmap = NULL;

  G_OBJECT_CLASS (_pango_win32_font_parent_class)->finalize (object);
}

static PangoFontDescription *
pango_win32_font_describe (PangoFont *font)
{
  PangoFontDescription *desc;
  PangoWin32Font *win32font = PANGO_WIN32_FONT (font);

  desc = pango_font_description_copy (win32font->win32face->description);
  pango_font_description_set_size (desc, win32font->size / (PANGO_SCALE / PANGO_WIN32_FONT_MAP (win32font->fontmap)->resolution));

  return desc;
}

static PangoFontDescription *
pango_win32_font_describe_absolute (PangoFont *font)
{
  PangoFontDescription *desc;
  PangoWin32Font *win32font = PANGO_WIN32_FONT (font);

  desc = pango_font_description_copy (win32font->win32face->description);
  pango_font_description_set_absolute_size (desc, win32font->size);

  return desc;
}

static PangoMap *
pango_win32_get_shaper_map (PangoLanguage *lang)
{
  static guint engine_type_id = 0;
  static guint render_type_id = 0;

  if (engine_type_id == 0)
    {
      engine_type_id = g_quark_from_static_string (PANGO_ENGINE_TYPE_SHAPE);
      render_type_id = g_quark_from_static_string (PANGO_RENDER_TYPE_WIN32);
    }

  return pango_find_map (lang, engine_type_id, render_type_id);
}

static gint
pango_win32_coverage_language_classify (PangoLanguage *lang)
{
  if (pango_language_matches (lang, "zh-tw"))
    return PANGO_WIN32_COVERAGE_ZH_TW;
  else if (pango_language_matches (lang, "zh-cn"))
    return PANGO_WIN32_COVERAGE_ZH_CN;
  else if (pango_language_matches (lang, "ja"))
    return PANGO_WIN32_COVERAGE_JA;
  else if (pango_language_matches (lang, "ko"))
    return PANGO_WIN32_COVERAGE_KO;
  else if (pango_language_matches (lang, "vi"))
    return PANGO_WIN32_COVERAGE_VI;
  else
    return PANGO_WIN32_COVERAGE_UNSPEC;
}

static PangoCoverage *
pango_win32_font_entry_get_coverage (PangoWin32Face *face,
				     PangoLanguage  *lang)
{
  gint i = pango_win32_coverage_language_classify (lang);
  if (face->coverages[i])
    {
      pango_coverage_ref (face->coverages[i]);
      return face->coverages[i];
    }

  return NULL;
}

static void
pango_win32_font_entry_set_coverage (PangoWin32Face *face,
				     PangoCoverage  *coverage,
				     PangoLanguage  *lang)
{
  face->coverages[pango_win32_coverage_language_classify (lang)] = pango_coverage_ref (coverage);
}

static PangoCoverage *
pango_win32_font_get_coverage (PangoFont     *font,
			       PangoLanguage *lang)
{
  PangoCoverage *coverage;
  PangoWin32Font *win32font = (PangoWin32Font *)font;

  coverage = pango_win32_font_entry_get_coverage (win32font->win32face, lang);
  if (!coverage)
    {
      coverage = pango_coverage_new ();
      pango_win32_font_calc_coverage (font, coverage, lang);

      pango_win32_font_entry_set_coverage (win32font->win32face, coverage, lang);
    }

  return coverage;
}

static PangoEngineShape *
pango_win32_font_find_shaper (PangoFont     *font,
			      PangoLanguage *lang,
			      guint32        ch)
{
  PangoMap *shape_map = NULL;
  PangoScript script;

  shape_map = pango_win32_get_shaper_map (lang);
  script = pango_script_for_unichar (ch);
  return (PangoEngineShape *)pango_map_get_engine (shape_map, script);
}

/* Utility functions */

/**
 * pango_win32_get_unknown_glyph:
 * @font: a #PangoFont
 * @wc: the Unicode character for which a glyph is needed.
 *
 * Returns the index of a glyph suitable for drawing @wc as an
 * unknown character.
 *
 * Use PANGO_GET_UNKNOWN_GLYPH() instead.
 *
 * Return value: a glyph index into @font
 **/
PangoGlyph
pango_win32_get_unknown_glyph (PangoFont *font,
			       gunichar   wc)
{
  return PANGO_GET_UNKNOWN_GLYPH (wc);
}

/**
 * pango_win32_render_layout_line:
 * @hdc:       DC to use for uncolored drawing
 * @line:      a #PangoLayoutLine
 * @x:         the x position of start of string (in pixels)
 * @y:         the y position of baseline (in pixels)
 *
 * Render a #PangoLayoutLine onto a device context. For underlining to
 * work property the text alignment of the DC should have TA_BASELINE
 * and TA_LEFT.
 */
void
pango_win32_render_layout_line (HDC              hdc,
				PangoLayoutLine *line,
				int              x,
				int              y)
{
  GSList *tmp_list = line->runs;
  PangoRectangle overall_rect;
  PangoRectangle logical_rect;
  PangoRectangle ink_rect;

  int x_off = 0;

  pango_layout_line_get_extents (line,NULL, &overall_rect);

  while (tmp_list)
    {
      HBRUSH oldfg = NULL;
      HBRUSH brush = NULL;
      POINT points[2];
      PangoUnderline uline = PANGO_UNDERLINE_NONE;
      PangoLayoutRun *run = tmp_list->data;
      PangoAttrColor fg_color, bg_color;
      gboolean fg_set, bg_set;

      tmp_list = tmp_list->next;

      pango_win32_get_item_properties (run->item, &uline, &fg_color, &fg_set, &bg_color, &bg_set);

      if (uline == PANGO_UNDERLINE_NONE)
	pango_glyph_string_extents (run->glyphs, run->item->analysis.font,
				    NULL, &logical_rect);
      else
	pango_glyph_string_extents (run->glyphs, run->item->analysis.font,
				    &ink_rect, &logical_rect);

      if (bg_set)
	{
	  HBRUSH oldbrush;

	  brush = CreateSolidBrush (RGB ((bg_color.color.red + 128) >> 8,
					 (bg_color.color.green + 128) >> 8,
					 (bg_color.color.blue + 128) >> 8));
	  oldbrush = SelectObject (hdc, brush);
	  Rectangle (hdc, x + PANGO_PIXELS (x_off + logical_rect.x),
			  y + PANGO_PIXELS (overall_rect.y),
			  PANGO_PIXELS (logical_rect.width),
			  PANGO_PIXELS (overall_rect.height));
	  SelectObject (hdc, oldbrush);
	  DeleteObject (brush);
	}

      if (fg_set)
	{
	  brush = CreateSolidBrush (RGB ((fg_color.color.red + 128) >> 8,
					 (fg_color.color.green + 128) >> 8,
					 (fg_color.color.blue + 128) >> 8));
	  oldfg = SelectObject (hdc, brush);
	}

      pango_win32_render (hdc, run->item->analysis.font, run->glyphs,
			  x + PANGO_PIXELS (x_off), y);

      switch (uline)
	{
	case PANGO_UNDERLINE_NONE:
	  break;
	case PANGO_UNDERLINE_DOUBLE:
	  points[0].x = x + PANGO_PIXELS (x_off + ink_rect.x) - 1;
	  points[0].y = points[1].y = y + 4;
	  points[1].x = x + PANGO_PIXELS (x_off + ink_rect.x + ink_rect.width);
	  Polyline (hdc, points, 2);
	  points[0].y = points[1].y = y + 2;
	  Polyline (hdc, points, 2);
	  break;
	case PANGO_UNDERLINE_SINGLE:
	  points[0].x = x + PANGO_PIXELS (x_off + ink_rect.x) - 1;
	  points[0].y = points[1].y = y + 2;
	  points[1].x = x + PANGO_PIXELS (x_off + ink_rect.x + ink_rect.width);
	  Polyline (hdc, points, 2);
	  break;
	case PANGO_UNDERLINE_ERROR:
	  {
	    int point_x;
	    int counter = 0;
	    int end_x = x + PANGO_PIXELS (x_off + ink_rect.x + ink_rect.width);

	    for (point_x = x + PANGO_PIXELS (x_off + ink_rect.x) - 1;
		 point_x <= end_x;
		 point_x += 2)
	    {
	      points[0].x = point_x;
	      points[1].x = MAX (point_x + 1, end_x);

	      if (counter)
		points[0].y = points[1].y = y + 2;
	      else
		points[0].y = points[1].y = y + 3;

	      Polyline (hdc, points, 2);
	      counter = (counter + 1) % 2;
	    }
	  }
	  break;
	case PANGO_UNDERLINE_LOW:
	  points[0].x = x + PANGO_PIXELS (x_off + ink_rect.x) - 1;
	  points[0].y = points[1].y = y + PANGO_PIXELS (ink_rect.y + ink_rect.height) + 2;
	  points[1].x = x + PANGO_PIXELS (x_off + ink_rect.x + ink_rect.width);
	  Polyline (hdc, points, 2);
	  break;
	}

      if (fg_set)
	{
	  SelectObject (hdc, oldfg);
	  DeleteObject (brush);
	}

      x_off += logical_rect.width;
    }
}

/**
 * pango_win32_render_layout:
 * @hdc:       HDC to use for uncolored drawing
 * @layout:    a #PangoLayout
 * @x:         the X position of the left of the layout (in pixels)
 * @y:         the Y position of the top of the layout (in pixels)
 *
 * Render a #PangoLayoutLine onto an X drawable
 */
void
pango_win32_render_layout (HDC          hdc,
			   PangoLayout *layout,
			   int          x,
			   int          y)
{
  PangoLayoutIter *iter;

  g_return_if_fail (hdc != NULL);
  g_return_if_fail (PANGO_IS_LAYOUT (layout));

  iter = pango_layout_get_iter (layout);

  do
    {
      PangoRectangle   logical_rect;
      PangoLayoutLine *line;
      int              baseline;

      line = pango_layout_iter_get_line_readonly (iter);

      pango_layout_iter_get_line_extents (iter, NULL, &logical_rect);
      baseline = pango_layout_iter_get_baseline (iter);

      pango_win32_render_layout_line (hdc,
				      line,
				      x + PANGO_PIXELS (logical_rect.x),
				      y + PANGO_PIXELS (baseline));
    }
  while (pango_layout_iter_next_line (iter));

  pango_layout_iter_free (iter);
}

/* This utility function is duplicated here and in pango-layout.c; should it be
 * public? Trouble is - what is the appropriate set of properties?
 */
static void
pango_win32_get_item_properties (PangoItem      *item,
				 PangoUnderline *uline,
				 PangoAttrColor *fg_color,
				 gboolean       *fg_set,
				 PangoAttrColor *bg_color,
				 gboolean       *bg_set)
{
  GSList *tmp_list = item->analysis.extra_attrs;

  if (fg_set)
    *fg_set = FALSE;

  if (bg_set)
    *bg_set = FALSE;

  while (tmp_list)
    {
      PangoAttribute *attr = tmp_list->data;

      switch (attr->klass->type)
	{
	case PANGO_ATTR_UNDERLINE:
	  if (uline)
	    *uline = ((PangoAttrInt *)attr)->value;
	  break;

	case PANGO_ATTR_FOREGROUND:
	  if (fg_color)
	    *fg_color = *((PangoAttrColor *)attr);
	  if (fg_set)
	    *fg_set = TRUE;

	  break;

	case PANGO_ATTR_BACKGROUND:
	  if (bg_color)
	    *bg_color = *((PangoAttrColor *)attr);
	  if (bg_set)
	    *bg_set = TRUE;

	  break;

	default:
	  break;
	}
      tmp_list = tmp_list->next;
    }
}

static guint
get_cmap_offset (HDC     hdc,
		 guint16 encoding_id)
{
  guint16 n_tables;
  struct cmap_encoding_subtable *table;
  gint32 res;
  int i;
  guint32 offset;

  /* Get The number of encoding tables, at offset 2 */
  res = GetFontData (hdc, CMAP, 2, &n_tables, 2);
  if (res != 2)
    return 0;

  n_tables = GUINT16_FROM_BE (n_tables);

  table = g_malloc (ENCODING_TABLE_SIZE*n_tables);

  res = GetFontData (hdc, CMAP, CMAP_HEADER_SIZE, table, ENCODING_TABLE_SIZE*n_tables);
  if (res != ENCODING_TABLE_SIZE*n_tables)
    return 0;

  for (i = 0; i < n_tables; i++)
    {
      if (table[i].platform_id == GUINT16_TO_BE (MICROSOFT_PLATFORM_ID) &&
	  table[i].encoding_id == GUINT16_TO_BE (encoding_id))
	{
	  offset = GUINT32_FROM_BE (table[i].offset);
	  g_free (table);
	  return offset;
	}
    }
  g_free (table);
  return 0;
}

static gpointer
get_format_4_cmap (HDC hdc)
{
  guint32 offset;
  guint32 res;
  guint16 length;
  guint16 *tbl, *tbl_end;
  struct format_4_cmap *table;

  /* FIXME: Could look here at the CRC for the font in the DC
	    and return a cached copy if the same */

  offset = get_cmap_offset (hdc, UNICODE_ENCODING_ID);
  if (offset == 0)
    return NULL;

  res = GetFontData (hdc, CMAP, offset + 2, &length, 2);
  if (res != 2)
    return NULL;
  length = GUINT16_FROM_BE (length);

  table = g_malloc (length);

  res = GetFontData (hdc, CMAP, offset, table, length);
  if (res != length ||
      GUINT16_FROM_BE (table->format) != 4 ||
      (GUINT16_FROM_BE (table->length) % 2) != 0)
    {
      g_free (table);
      return NULL;
    }

  table->format = GUINT16_FROM_BE (table->format);
  table->length = GUINT16_FROM_BE (table->length);
  table->language = GUINT16_FROM_BE (table->language);
  table->seg_count_x_2 = GUINT16_FROM_BE (table->seg_count_x_2);
  table->search_range = GUINT16_FROM_BE (table->search_range);
  table->entry_selector = GUINT16_FROM_BE (table->entry_selector);
  table->range_shift = GUINT16_FROM_BE (table->range_shift);

  tbl_end = (guint16 *)((char *)table + length);
  tbl = &table->reserved;

  while (tbl < tbl_end)
    {
      *tbl = GUINT16_FROM_BE (*tbl);
      tbl++;
    }

  return table;
}

static guint16 *
get_id_range_offset (struct format_4_cmap *table)
{
  gint32 seg_count = table->seg_count_x_2/2;
  return &table->arrays[seg_count*3];
}

static guint16 *
get_id_delta (struct format_4_cmap *table)
{
  gint32 seg_count = table->seg_count_x_2/2;
  return &table->arrays[seg_count*2];
}

static guint16 *
get_start_count (struct format_4_cmap *table)
{
  gint32 seg_count = table->seg_count_x_2/2;
  return &table->arrays[seg_count*1];
}

static guint16 *
get_end_count (struct format_4_cmap *table)
{
  gint32 seg_count = table->seg_count_x_2/2;
  /* Apparently the reseved spot is not reserved for
     the end_count array!? */
  return (&table->arrays[seg_count*0])-1;
}

static gboolean
find_segment (struct format_4_cmap *table,
	      guint16               wc,
	      guint16              *segment)
{
  guint16 start, end, i;
  guint16 seg_count = table->seg_count_x_2/2;
  guint16 *end_count = get_end_count (table);
  guint16 *start_count = get_start_count (table);
  static guint last = 0; /* Cache of one */

  if (last < seg_count &&
      wc >= start_count[last] &&
      wc <= end_count[last])
    {
      *segment = last;
      return TRUE;
    }


  /* Binary search for the segment */
  start = 0; /* inclusive */
  end = seg_count; /* not inclusive */
  while (start < end)
    {
      /* Look at middle pos */
      i = (start + end)/2;

      if (i == start)
	{
	  /* We made no progress. Look if this is the one. */

	  if (wc >= start_count[i] &&
	      wc <= end_count[i])
	    {
	      *segment = i;
	      last = i;
	      return TRUE;
	    }
	  else
	    return FALSE;
	}
      else if (wc < start_count[i])
	{
	  end = i;
	}
      else if (wc > end_count[i])
	{
	  start = i;
	}
      else
	{
	  /* Found it! */
	  *segment = i;
	  last = i;
	  return TRUE;
	}
    }
  return FALSE;
}

static gpointer
get_format_12_cmap (HDC hdc)
{
  guint32 offset;
  guint32 res;
  guint32 length;
  guint32 *tbl, *tbl_end;
  struct format_12_cmap *table;

  offset = get_cmap_offset (hdc, UCS4_ENCODING_ID);
  if (offset == 0)
    return NULL;

  res = GetFontData (hdc, CMAP, offset + 4, &length, 4);
  if (res != 4)
    return NULL;
  length = GUINT32_FROM_BE (length);

  table = g_malloc (length);

  res = GetFontData (hdc, CMAP, offset, table, length);
  if (res != length)
    {
      g_free (table);
      return NULL;
    }

  table->format = GUINT16_FROM_BE (table->format);
  table->length = GUINT32_FROM_BE (table->length);
  table->language = GUINT32_FROM_BE (table->language);
  table->count = GUINT32_FROM_BE (table->count);

  if (table->format != 12 ||
      (table->length % 4) != 0 ||
      table->length > length ||
      table->length < 16 + table->count * 12)
    {
      g_free (table);
      return NULL;
    }

  tbl_end = (guint32 *) ((char *) table + length);
  tbl = table->groups;

  while (tbl < tbl_end)
    {
      *tbl = GUINT32_FROM_BE (*tbl);
      tbl++;
    }

  return table;
}

static gpointer
font_get_cmap (PangoFont *font)
{
  PangoWin32Font *win32font = (PangoWin32Font *)font;
  gpointer cmap;

  if (win32font->win32face->cmap)
    return win32font->win32face->cmap;

  pango_win32_font_select_font (font, _pango_win32_hdc);

  /* Prefer the format 12 cmap */
  if ((cmap = get_format_12_cmap (_pango_win32_hdc)) != NULL)
    {
      win32font->win32face->cmap_format = 12;
      win32font->win32face->cmap = cmap;
    }
  else if ((cmap = get_format_4_cmap (_pango_win32_hdc)) != NULL)
    {
      win32font->win32face->cmap_format = 4;
      win32font->win32face->cmap = cmap;
    }

  pango_win32_font_done_font (font);

  return cmap;
}

static gint
pango_win32_font_get_type1_glyph_index (PangoFont *font,
                                        gunichar   wc)
{
  wchar_t unicode[2];
  WORD glyph_index;
  gint32 res;

  unicode[0] = wc;
  unicode[1] = 0;
  pango_win32_font_select_font (font, _pango_win32_hdc);
  res = GetGlyphIndicesW (_pango_win32_hdc, unicode, 1, &glyph_index, 0);
  pango_win32_font_done_font (font);

  if (res == 1)
      return glyph_index;
  else
      return 0;
}

/**
 * pango_win32_font_get_glyph_index:
 * @font: a #PangoFont.
 * @wc: a Unicode character.
 *
 * Obtains the index of the glyph for @wc in @font, or 0, if not
 * covered.
 *
 * Return value: the glyph index for @wc.
 **/
gint
pango_win32_font_get_glyph_index (PangoFont *font,
				  gunichar   wc)
{
  PangoWin32Font *win32font = (PangoWin32Font *)font;
  gpointer cmap;
  guint16 glyph;

  if (win32font->win32face->has_cmap)
    {
      /* Do GetFontData magic on font->hfont here. */
      cmap = font_get_cmap (font);
      if (cmap == NULL)
	win32font->win32face->has_cmap = FALSE;
    }

  if (!win32font->win32face->has_cmap)
    return pango_win32_font_get_type1_glyph_index (font, wc);

  if (win32font->win32face->cmap_format == 4)
    {
      struct format_4_cmap *cmap4 = cmap;
      guint16 *id_range_offset;
      guint16 *id_delta;
      guint16 *start_count;
      guint16 segment;
      guint16 id;
      guint16 ch = wc;

      if (wc > 0xFFFF)
	return 0;

      if (!find_segment (cmap4, ch, &segment))
	return 0;

      id_range_offset = get_id_range_offset (cmap4);
      id_delta = get_id_delta (cmap4);
      start_count = get_start_count (cmap4);

      if (id_range_offset[segment] == 0)
	glyph = (id_delta[segment] + ch) % 65536;
      else
	{
	  id = *(id_range_offset[segment]/2 +
		 (ch - start_count[segment]) +
		 &id_range_offset[segment]);
	  if (id)
	    glyph = (id_delta[segment] + id) %65536;
	  else
	    glyph = 0;
	}
    }
  else if (win32font->win32face->cmap_format == 12)
    {
      struct format_12_cmap *cmap12 = cmap;
      guint32 i;

      glyph = 0;
      for (i = 0; i < cmap12->count; i++)
	{
	  if (cmap12->groups[i*3+0] <= wc && wc <= cmap12->groups[i*3+1])
	    {
	      glyph = cmap12->groups[i*3+2] + (wc - cmap12->groups[i*3+0]);
	      break;
	    }
	}
    }
  else
    g_assert_not_reached ();

  return glyph;
}

gboolean
_pango_win32_get_name_header (HDC                 hdc,
			      struct name_header *header)
{
  if (GetFontData (hdc, NAME, 0, header, sizeof (*header)) != sizeof (*header))
    return FALSE;

  header->num_records = GUINT16_FROM_BE (header->num_records);
  header->string_storage_offset = GUINT16_FROM_BE (header->string_storage_offset);

  return TRUE;
}

gboolean
_pango_win32_get_name_record (HDC                 hdc,
			      gint                i,
			      struct name_record *record)
{
  if (GetFontData (hdc, NAME, 6 + i * sizeof (*record),
		   record, sizeof (*record)) != sizeof (*record))
    return FALSE;

  record->platform_id = GUINT16_FROM_BE (record->platform_id);
  record->encoding_id = GUINT16_FROM_BE (record->encoding_id);
  record->language_id = GUINT16_FROM_BE (record->language_id);
  record->name_id = GUINT16_FROM_BE (record->name_id);
  record->string_length = GUINT16_FROM_BE (record->string_length);
  record->string_offset = GUINT16_FROM_BE (record->string_offset);

  return TRUE;
}

static gboolean
font_has_name_in (PangoFont                       *font,
		  PangoWin32CoverageLanguageClass  cjkv)
{
  HFONT hfont, oldhfont;
  struct name_header header;
  struct name_record record;
  gint i;
  gboolean retval = FALSE;

  if (cjkv == PANGO_WIN32_COVERAGE_UNSPEC)
    return TRUE;

  hfont = pango_win32_get_hfont (font);
  oldhfont = SelectObject (_pango_win32_hdc, hfont);

  if (!_pango_win32_get_name_header (_pango_win32_hdc, &header))
    {
      SelectObject (_pango_win32_hdc, oldhfont);
      return FALSE;
    }

  for (i = 0; i < header.num_records; i++)
    {
      if (!_pango_win32_get_name_record (_pango_win32_hdc, i, &record))
	{
	  SelectObject (_pango_win32_hdc, oldhfont);
	  return FALSE;
	}

      if ((record.name_id != 1 && record.name_id != 16) || record.string_length <= 0)
	continue;

      PING (("platform:%d encoding:%d language:%04x name_id:%d",
	     record.platform_id, record.encoding_id, record.language_id, record.name_id));

      if (record.platform_id == MICROSOFT_PLATFORM_ID)
	if ((cjkv == PANGO_WIN32_COVERAGE_ZH_TW &&
	     record.language_id == MAKELANGID (LANG_CHINESE, SUBLANG_CHINESE_TRADITIONAL))
	    ||
	    (cjkv == PANGO_WIN32_COVERAGE_ZH_CN &&
	     record.language_id == MAKELANGID (LANG_CHINESE, SUBLANG_CHINESE_SIMPLIFIED))
	    ||
	    (cjkv == PANGO_WIN32_COVERAGE_JA &&
	     PRIMARYLANGID (record.language_id) == LANG_JAPANESE)
	    ||
	    (cjkv == PANGO_WIN32_COVERAGE_KO &&
	     PRIMARYLANGID (record.language_id) == LANG_KOREAN)
	    ||
	    (cjkv == PANGO_WIN32_COVERAGE_VI &&
	     PRIMARYLANGID (record.language_id) == LANG_VIETNAMESE))
	  {
	    PING (("yep:%d:%04x", cjkv, record.language_id));
	    retval = TRUE;
	    break;
	  }
    }

  SelectObject (_pango_win32_hdc, oldhfont);
  return retval;
}

static void
pango_win32_font_calc_type1_coverage (PangoFont     *font,
				      PangoCoverage *coverage,
				      PangoLanguage *lang)
{
  GLYPHSET *glyph_set;
  gint32 res;
  guint32 ch;
  guint32 i;
  
  pango_win32_font_select_font (font, _pango_win32_hdc);
  res = GetFontUnicodeRanges(_pango_win32_hdc, NULL);
  if (res == 0)
    goto fail1;

  glyph_set = g_malloc (res);
  res = GetFontUnicodeRanges(_pango_win32_hdc, glyph_set);
  if (res == 0)
    goto fail2;
  
  for (i = 0; i < glyph_set->cRanges; i++) 
    {
      guint32 end = glyph_set->ranges[i].wcLow + glyph_set->ranges[i].cGlyphs;

      for (ch = glyph_set->ranges[i].wcLow; ch < end; ch++)
	  if (CH_IS_UNIHAN_BMP (ch))
	      pango_coverage_set (coverage, ch, PANGO_COVERAGE_APPROXIMATE);
	  else
	      pango_coverage_set (coverage, ch, PANGO_COVERAGE_EXACT);
    }

 fail2:
  g_free (glyph_set);

 fail1:
  pango_win32_font_done_font (font);
}

static void
pango_win32_font_calc_coverage (PangoFont     *font,
				PangoCoverage *coverage,
				PangoLanguage *lang)
{
  PangoWin32Font *win32font = (PangoWin32Font *)font;
  gpointer cmap;
  guint32 ch;
  guint32 i;
  PangoWin32CoverageLanguageClass cjkv;
  gboolean hide_unihan = FALSE;
  PangoFontDescription *desc;
  gchar *name;

  desc = pango_font_describe (font);
  name = pango_font_description_to_string (desc);
  PING (("font:%s lang:%s", name,
	 pango_language_to_string (lang)));
  g_free (name);
  pango_font_description_free (desc);

  if (win32font->win32face->has_cmap)
    {
      /* Do GetFontData magic on font->hfont here. */
      cmap = font_get_cmap (font);
      if (cmap == NULL)
	win32font->win32face->has_cmap = FALSE;
    }

  if (!win32font->win32face->has_cmap)
    {
      pango_win32_font_calc_type1_coverage (font, coverage, lang);
      return;
    }

  cjkv = pango_win32_coverage_language_classify (lang);

  if (cjkv != PANGO_WIN32_COVERAGE_UNSPEC && !font_has_name_in (font, cjkv))
    {
      PING (("hiding UniHan chars"));
      hide_unihan = TRUE;
    }

  PING (("coverage:"));
  if (win32font->win32face->cmap_format == 4)
    {
      struct format_4_cmap *cmap4 = cmap;
      guint16 *id_range_offset;
      guint16 *start_count;
      guint16 *end_count;
      guint16 seg_count;
      guint16 id;

      seg_count = cmap4->seg_count_x_2/2;
      end_count = get_end_count (cmap4);
      start_count = get_start_count (cmap4);
      id_range_offset = get_id_range_offset (cmap4);

      for (i = 0; i < seg_count; i++)
	{
	  if (id_range_offset[i] == 0)
	    {
#ifdef PANGO_WIN32_DEBUGGING
	      if (_pango_win32_debug)
		{
		  if (end_count[i] == start_count[i])
		    g_print ("%04x ", start_count[i]);
		  else
		    g_print ("%04x:%04x ", start_count[i], end_count[i]);
		}
#endif
	      for (ch = start_count[i]; ch <= end_count[i]; ch++)
		if (hide_unihan && CH_IS_UNIHAN_BMP (ch))
		  pango_coverage_set (coverage, ch, PANGO_COVERAGE_APPROXIMATE);
		else
		  pango_coverage_set (coverage, ch, PANGO_COVERAGE_EXACT);
	    }
	  else
	    {
#ifdef PANGO_WIN32_DEBUGGING
	      guint32 ch0 = G_MAXUINT;
#endif
	      for (ch = start_count[i]; ch <= end_count[i]; ch++)
		{
		  if (ch == 0xFFFF)
		    break;

		  id = *(id_range_offset[i]/2 +
			 (ch - start_count[i]) +
			 &id_range_offset[i]);
		  if (id)
		    {
#ifdef PANGO_WIN32_DEBUGGING
		      if (ch0 == G_MAXUINT)
			ch0 = ch;
#endif
		      if (hide_unihan && CH_IS_UNIHAN_BMP (ch))
			pango_coverage_set (coverage, ch, PANGO_COVERAGE_APPROXIMATE);
		      else
			pango_coverage_set (coverage, ch, PANGO_COVERAGE_EXACT);
		    }
#ifdef PANGO_WIN32_DEBUGGING
		  else if (ch0 < G_MAXUINT)
		    {
		      if (_pango_win32_debug)
			{
			  if (ch > ch0 + 2)
			    g_print ("%04x:%04x ", ch0, ch - 1);
			  else
			    g_print ("%04x ", ch0);
			}
		      ch0 = G_MAXUINT;
		    }
#endif
		}
#ifdef PANGO_WIN32_DEBUGGING
	      if (ch0 < G_MAXUINT)
		{
		  if (_pango_win32_debug)
		    {
		      if (ch > ch0 + 2)
			g_print ("%04x:%04x ", ch0, ch - 1);
		      else
			g_print ("%04x ", ch0);
		    }
		}
#endif
	    }
	}
    }
  else if (win32font->win32face->cmap_format == 12)
    {
      struct format_12_cmap *cmap12 = cmap;

      for (i = 0; i < cmap12->count; i++)
	{
#ifdef PANGO_WIN32_DEBUGGING
	  if (_pango_win32_debug)
	    {
	      if (cmap12->groups[i*3+0] == cmap12->groups[i*3+1])
		g_print ("%04x ", cmap12->groups[i*3+0]);
	      else
		g_print ("%04x:%04x ", cmap12->groups[i*3+0], cmap12->groups[i*3+1]);
	    }
#endif
	  for (ch = cmap12->groups[i*3+0]; ch <= cmap12->groups[i*3+1]; ch++)
	    {
	      if (hide_unihan && CH_IS_UNIHAN (ch))
		pango_coverage_set (coverage, ch, PANGO_COVERAGE_APPROXIMATE);
	      else
		pango_coverage_set (coverage, ch, PANGO_COVERAGE_EXACT);
	    }
	}
    }
  else
    g_assert_not_reached ();
#ifdef PANGO_WIN32_DEBUGGING
  if (_pango_win32_debug)
    g_print ("\n");
#endif
}
