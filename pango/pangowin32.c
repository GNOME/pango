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
#include <hb.h>

#ifdef USE_HB_GDI
#include <hb-gdi.h>
#endif

#include <hb-ot.h>

#include "pango-impl-utils.h"
#include "pangowin32.h"
#include "pangowin32-private.h"
#include "pango-coverage-private.h"

#define MAX_FREED_FONTS 256

gboolean _pango_win32_debug = FALSE;

static void pango_win32_font_dispose    (GObject             *object);
static void pango_win32_font_finalize   (GObject             *object);

static PangoFontDescription *pango_win32_font_describe          (PangoFont        *font);
static PangoFontDescription *pango_win32_font_describe_absolute (PangoFont        *font);
static PangoCoverage        *pango_win32_font_get_coverage      (PangoFont        *font,
								 PangoLanguage    *lang);
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
static gboolean pango_win32_font_is_hinted             (PangoFont *font);

static void                  pango_win32_get_item_properties    (PangoItem        *item,
								 PangoUnderline   *uline,
								 PangoAttrColor   *uline_color,
								 gboolean         *uline_set,
								 PangoAttrColor   *fg_color,
								 gboolean         *fg_set,
								 PangoAttrColor   *bg_color,
								 gboolean         *bg_set);

static hb_font_t *           pango_win32_font_create_hb_font    (PangoFont *font);

HFONT
_pango_win32_font_get_hfont (PangoFont *font)
{
  PangoWin32Font *win32font = (PangoWin32Font *)font;
  PangoWin32FontCache *cache;

  if (!win32font)
    return NULL;

  if (!win32font->hfont)
    {
      cache = pango_win32_font_map_get_font_cache (win32font->fontmap);
      if (G_UNLIKELY (!cache))
        return NULL;

      win32font->hfont = pango_win32_font_cache_loadw (cache, &win32font->logfontw);
      if (!win32font->hfont)
	{
	  gchar *face_utf8 = g_utf16_to_utf8 (win32font->logfontw.lfFaceName,
					      -1, NULL, NULL, NULL);
	  g_warning ("Cannot load font '%s\n", face_utf8);
	  g_free (face_utf8);
	  return NULL;
	}
    }

  return win32font->hfont;
}

/**
 * pango_win32_get_context:
 *
 * Retrieves a `PangoContext` appropriate for rendering with Windows fonts.
 *
 * Return value: the new `PangoContext`
 *
 * Deprecated: 1.22: Use [func@Pango.Win32FontMap.for_display] followed by
 * [method@Pango.FontMap.create_context] instead.
 */
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

static void
_delete_dc (HDC dc)
{
  /* Don't pass DeleteDC func pointer to the GDestroyNotify.
   * 32bit build requires matching calling convention (__cdecl vs __stdcall) */
  DeleteDC (dc);
}

static GPrivate display_dc_key = G_PRIVATE_INIT ((GDestroyNotify) _delete_dc);
static GPrivate dwrite_items = G_PRIVATE_INIT ((GDestroyNotify) pango_win32_dwrite_items_destroy);

HDC
_pango_win32_get_display_dc (void)
{
  HDC hdc = g_private_get (&display_dc_key);

  if (hdc == NULL)
    {
      hdc = CreateDC ("DISPLAY", NULL, NULL, NULL);

      if (G_UNLIKELY (hdc == NULL))
	g_warning ("CreateDC() failed");

      g_private_set (&display_dc_key, hdc);

      /* Also do some generic pangowin32 initialisations... this function
       * is a suitable place for those as it is called from a couple
       * of class_init functions.
       */
#ifdef PANGO_WIN32_DEBUGGING
      if (getenv ("PANGO_WIN32_DEBUG") != NULL)
	_pango_win32_debug = TRUE;
#endif
    }

  /* ensure DirectWrite is initialized */
  pango_win32_get_direct_write_items ();

  return hdc;
}

PangoWin32DWriteItems *
pango_win32_get_direct_write_items (void)
{
  PangoWin32DWriteItems *items = g_private_get (&dwrite_items);

  if (items == NULL)
    {
      items = pango_win32_init_direct_write ();
      g_private_set (&dwrite_items, items);
    }

  return items;
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
  return _pango_win32_get_display_dc ();
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

static PangoFontFace *
pango_win32_font_get_face (PangoFont *font)
{
  PangoWin32Font *win32font = (PangoWin32Font *)font;

  return PANGO_FONT_FACE (win32font->win32face);
}

static void
_pango_win32_font_class_init (PangoWin32FontClass *class)
{
  GObjectClass *object_class = G_OBJECT_CLASS (class);
  PangoFontClass *font_class = PANGO_FONT_CLASS (class);
  PangoFontClassPrivate *pclass;

  object_class->finalize = pango_win32_font_finalize;
  object_class->dispose = pango_win32_font_dispose;

  font_class->describe = pango_win32_font_describe;
  font_class->describe_absolute = pango_win32_font_describe_absolute;
  font_class->get_coverage = pango_win32_font_get_coverage;
  font_class->get_glyph_extents = pango_win32_font_get_glyph_extents;
  font_class->get_metrics = pango_win32_font_get_metrics;
  font_class->get_font_map = pango_win32_font_get_font_map;
  font_class->create_hb_font = pango_win32_font_create_hb_font;

  class->select_font = pango_win32_font_real_select_font;
  class->done_font = pango_win32_font_real_done_font;
  class->get_metrics_factor = pango_win32_font_real_get_metrics_factor;

  pclass = g_type_class_get_private ((GTypeClass *) class, PANGO_TYPE_FONT);
  pclass->is_hinted = pango_win32_font_is_hinted;
  pclass->get_face = pango_win32_font_get_face;

  _pango_win32_get_display_dc ();
}

/**
 * pango_win32_render:
 * @hdc:     the device context
 * @font:    the font in which to draw the string
 * @glyphs:  the glyph string to draw
 * @x:       the x position of start of string (in pixels)
 * @y:       the y position of baseline (in pixels)
 *
 * Render a `PangoGlyphString` onto a Windows DC
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

  hfont = _pango_win32_font_get_hfont (font);
  if (!hfont)
    return;

  old_hfont = SelectObject (hdc, hfont);

  glyph_indexes = g_new (guint16, glyphs->num_glyphs);
  dX = g_new (INT, glyphs->num_glyphs);

  /* Render glyphs using one ExtTextOutW() call for each run of glyphs
   * that have the same y offset. The big majority of glyphs will have
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
 * @hdc: a windows device context
 * @matrix: (nullable): a `PangoMatrix`
 * @font: the font in which to draw the string
 * @glyphs: the glyph string to draw
 * @x: the x position of the start of the string (in Pango
 *   units in user space coordinates)
 * @y: the y position of the baseline (in Pango units
 *   in user space coordinates)
 *
 * Renders a `PangoGlyphString` onto a windows DC, possibly
 * transforming the layed-out coordinates through a transformation
 * matrix.
 *
 * Note that the transformation matrix for @font is not
 * changed, so to produce correct rendering results, the @font
 * must have been loaded using a `PangoContext` with an identical
 * transformation matrix to that passed in to this function.
 */
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
  TEXTMETRIC tm;
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
      HDC hdc = _pango_win32_get_display_dc ();

      info = g_new0 (PangoWin32GlyphInfo, 1);

      memset (&gm, 0, sizeof (gm));

      hfont = _pango_win32_font_get_hfont (font);
      SelectObject (hdc, hfont);
      res = GetGlyphOutlineA (hdc,
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

      GetTextMetrics (hdc, &tm);
      info->logical_rect.x = 0;
      info->logical_rect.width = PANGO_SCALE * gm.gmCellIncX;
      info->logical_rect.y = - PANGO_SCALE * tm.tmAscent;
      info->logical_rect.height = PANGO_SCALE * (tm.tmAscent + tm.tmDescent);

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

      hfont = _pango_win32_font_get_hfont (font);
      if (hfont != NULL)
	{
	  PangoCoverage *coverage;
	  TEXTMETRIC tm;
	  HDC hdc = _pango_win32_get_display_dc ();

	  SelectObject (hdc, hfont);
	  GetTextMetrics (hdc, &tm);

	  metrics->ascent = tm.tmAscent * PANGO_SCALE;
	  metrics->descent = tm.tmDescent * PANGO_SCALE;
          metrics->height = (tm.tmHeight + tm.tmInternalLeading + tm.tmExternalLeading) * PANGO_SCALE;
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

	  g_object_unref (coverage);

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
  HFONT hfont = _pango_win32_font_get_hfont (font);

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

static gboolean
pango_win32_font_is_hinted (PangoFont *font)
{
  g_return_val_if_fail (PANGO_WIN32_IS_FONT (font), FALSE);

  return PANGO_WIN32_FONT (font)->is_hinted;
}

/**
 * pango_win32_font_logfont:
 * @font: a `PangoFont` which must be from the Win32 backend
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
 *   freed with g_free().
 */
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
 * @font: a `PangoFont` which must be from the Win32 backend
 *
 * Determine the LOGFONTW struct for the specified font.
 *
 * Return value: A newly allocated LOGFONTW struct. It must be
 *   freed with g_free().
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
 * @font: a `PangoFont` from the Win32 backend
 * @hdc: a windows device context
 *
 * Selects the font into the specified DC and changes the mapping mode
 * and world transformation of the DC appropriately for the font.
 *
 * You may want to surround the use of this function with calls
 * to SaveDC() and RestoreDC(). Call [method@Pango.Win32Font.done_font[ when
 * you are done using the DC to release allocated resources.
 *
 * See [method@Pango.Win32Font.get_metrics_factor] for information about
 * converting from the coordinate space used by this function
 * into Pango units.
 *
 * Return value: %TRUE if the operation succeeded.
 */
gboolean
pango_win32_font_select_font (PangoFont *font,
			      HDC        hdc)
{
  g_return_val_if_fail (PANGO_WIN32_IS_FONT (font), FALSE);

  return PANGO_WIN32_FONT_GET_CLASS (font)->select_font (font, hdc);
}

/**
 * pango_win32_font_done_font:
 * @font: a `PangoFont` from the win32 backend
 *
 * Releases any resources allocated by [method@Pango.Win32Font.select_font].
 */
void
pango_win32_font_done_font (PangoFont *font)
{
  g_return_if_fail (PANGO_WIN32_IS_FONT (font));

  PANGO_WIN32_FONT_GET_CLASS (font)->done_font (font);
}

/**
 * pango_win32_font_get_metrics_factor:
 * @font: a `PangoFont` from the win32 backend
 *
 * Returns the scale factor from logical units in the coordinate
 * space used by [method@Pango.Win32Font.select_font] to Pango
 * units in user space.
 *
 * Return value: factor to multiply logical units by to get Pango
 *   units.
 */
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

  if (cache != NULL && win32font->hfont != NULL)
    pango_win32_font_cache_unload (cache, win32font->hfont);

  g_slist_foreach (win32font->metrics_by_lang, (GFunc)free_metrics_info, NULL);
  g_slist_free (win32font->metrics_by_lang);

  pango_win32_font_entry_remove (win32font->win32face, PANGO_FONT (win32font));
  g_object_unref (win32font->win32face);

  g_hash_table_destroy (win32font->glyph_info);

  if (win32font->fontmap)
    g_object_remove_weak_pointer (G_OBJECT (win32font->fontmap), (gpointer *) &win32font->fontmap);

  g_free (win32font->variations);

  G_OBJECT_CLASS (_pango_win32_font_parent_class)->finalize (object);
}

static PangoFontDescription *
pango_win32_font_describe (PangoFont *font)
{
  PangoFontDescription *desc;
  PangoWin32Font *win32font = PANGO_WIN32_FONT (font);
  int size;

  desc = pango_font_description_copy (win32font->win32face->description);
  size = (int) (0.5 + win32font->size * 72.0 / PANGO_WIN32_FONT_MAP (win32font->fontmap)->dpi);
  pango_font_description_set_size (desc, size);

  if (win32font->variations)
    pango_font_description_set_variations (desc, win32font->variations);

  return desc;
}

static PangoFontDescription *
pango_win32_font_describe_absolute (PangoFont *font)
{
  PangoFontDescription *desc;
  PangoWin32Font *win32font = PANGO_WIN32_FONT (font);

  desc = pango_font_description_copy (win32font->win32face->description);
  pango_font_description_set_absolute_size (desc, win32font->size);

  if (win32font->variations)
    pango_font_description_set_variations (desc, win32font->variations);

  return desc;
}

static PangoCoverage *
pango_win32_font_get_coverage (PangoFont     *font,
                               PangoLanguage *lang G_GNUC_UNUSED)
{
  PangoWin32Face *win32face = ((PangoWin32Font *)font)->win32face;

  if (!win32face->coverage)
    {
      PangoCoverage *coverage = pango_coverage_new ();
      hb_font_t *hb_font = pango_font_get_hb_font (font);
      hb_face_t *hb_face = hb_font_get_face (hb_font);
      hb_set_t *chars = hb_set_create ();

      hb_face_collect_unicodes (hb_face, chars);

      coverage->chars = chars;
      win32face->coverage = coverage;
    }

  return g_object_ref (win32face->coverage);
}

/* Utility functions */

/**
 * pango_win32_get_unknown_glyph:
 * @font: a `PangoFont`
 * @wc: the Unicode character for which a glyph is needed
 *
 * Returns the index of a glyph suitable for drawing @wc as an
 * unknown character.
 *
 * Use PANGO_GET_UNKNOWN_GLYPH() instead.
 *
 * Return value: a glyph index into @font
 */
PangoGlyph
pango_win32_get_unknown_glyph (PangoFont *font,
			       gunichar   wc)
{
  return PANGO_GET_UNKNOWN_GLYPH (wc);
}

/**
 * pango_win32_render_layout_line:
 * @hdc: DC to use for drawing
 * @line: a `PangoLayoutLine`
 * @x: the x position of start of string (in pixels)
 * @y: the y position of baseline (in pixels)
 *
 * Render a `PangoLayoutLine` onto a device context.
 *
 * For underlining to work property the text alignment
 * of the DC should have TA_BASELINE and TA_LEFT.
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
  int oldbkmode = SetBkMode (hdc, TRANSPARENT);

  int x_off = 0;

  pango_layout_line_get_extents (line,NULL, &overall_rect);

  while (tmp_list)
    {
      COLORREF oldfg = 0;
      HPEN uline_pen, old_pen;
      POINT points[2];
      PangoUnderline uline = PANGO_UNDERLINE_NONE;
      PangoLayoutRun *run = tmp_list->data;
      PangoAttrColor fg_color, bg_color, uline_color;
      gboolean fg_set, bg_set, uline_set;

      tmp_list = tmp_list->next;

      pango_win32_get_item_properties (run->item, &uline, &uline_color, &uline_set, &fg_color, &fg_set, &bg_color, &bg_set);
      if (!uline_set)
	uline_color = fg_color;

      if (uline == PANGO_UNDERLINE_NONE)
	pango_glyph_string_extents (run->glyphs, run->item->analysis.font,
				    NULL, &logical_rect);
      else
	pango_glyph_string_extents (run->glyphs, run->item->analysis.font,
				    &ink_rect, &logical_rect);

      if (bg_set)
	{
	  COLORREF bg_col = RGB ((bg_color.color.red) >> 8,
				 (bg_color.color.green) >> 8,
				 (bg_color.color.blue) >> 8);
	  HBRUSH bg_brush = CreateSolidBrush (bg_col);
	  HBRUSH old_brush = SelectObject (hdc, bg_brush);
	  old_pen = SelectObject (hdc, GetStockObject (NULL_PEN));
	  Rectangle (hdc, x + PANGO_PIXELS (x_off + logical_rect.x),
			  y + PANGO_PIXELS (overall_rect.y),
			  1 + x + PANGO_PIXELS (x_off + logical_rect.x + logical_rect.width),
			  1 + y + PANGO_PIXELS (overall_rect.y + overall_rect.height));
	  SelectObject (hdc, old_brush);
	  DeleteObject (bg_brush);
	  SelectObject (hdc, old_pen);
	}

      if (fg_set)
	{
	  COLORREF fg_col = RGB ((fg_color.color.red) >> 8,
				 (fg_color.color.green) >> 8,
				 (fg_color.color.blue) >> 8);
	  oldfg = SetTextColor (hdc, fg_col);
	}

      pango_win32_render (hdc, run->item->analysis.font, run->glyphs,
			  x + PANGO_PIXELS (x_off), y);

      if (fg_set)
	SetTextColor (hdc, oldfg);

      if (uline != PANGO_UNDERLINE_NONE)
	{
	  COLORREF uline_col = RGB ((uline_color.color.red) >> 8,
				    (uline_color.color.green) >> 8,
				    (uline_color.color.blue) >> 8);
	  uline_pen = CreatePen (PS_SOLID, 1, uline_col);
	  old_pen = SelectObject (hdc, uline_pen);
	}

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
	case PANGO_UNDERLINE_SINGLE_LINE:
	case PANGO_UNDERLINE_DOUBLE_LINE:
	case PANGO_UNDERLINE_ERROR_LINE:
          g_warning ("Underline value %d not implemented", uline);
          break;
        default:
          g_assert_not_reached ();
	}

      if (uline != PANGO_UNDERLINE_NONE)
	{
	  SelectObject (hdc, old_pen);
	  DeleteObject (uline_pen);
	}

      x_off += logical_rect.width;
    }

    SetBkMode (hdc, oldbkmode);
}

/**
 * pango_win32_render_layout:
 * @hdc: HDC to use for drawing
 * @layout: a `PangoLayout`
 * @x: the X position of the left of the layout (in pixels)
 * @y: the Y position of the top of the layout (in pixels)
 *
 * Render a `PangoLayoutLine` onto an HDC.
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
				 PangoAttrColor *uline_color,
				 gboolean       *uline_set,
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

      switch ((int) attr->klass->type)
	{
	case PANGO_ATTR_UNDERLINE:
	  if (uline)
	    *uline = ((PangoAttrInt *)attr)->value;
	  break;

	case PANGO_ATTR_UNDERLINE_COLOR:
	  if (uline_color)
	    *uline_color = *((PangoAttrColor *)attr);
	  if (uline_set)
	    *uline_set = TRUE;

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

/**
 * pango_win32_font_get_glyph_index:
 * @font: a `PangoFont`
 * @wc: a Unicode character
 *
 * Obtains the index of the glyph for @wc in @font, or 0, if not
 * covered.
 *
 * Return value: the glyph index for @wc.
 */
gint
pango_win32_font_get_glyph_index (PangoFont *font,
				  gunichar   wc)
{
  hb_font_t *hb_font = pango_font_get_hb_font (font);
  hb_codepoint_t glyph = 0;

  hb_font_get_nominal_glyph (hb_font, wc, &glyph);

  return glyph;
}

/*
 * Swap HarfBuzz-style tags to tags that GetFontData() understands,
 * adapted from https://github.com/harfbuzz/harfbuzz/pull/1832,
 * by Ebrahim Byagowi.
 */

static inline guint16 hb_gdi_uint16_swap (const guint16 v)
{ return (v >> 8) | (v << 8); }
static inline guint32 hb_gdi_uint32_swap (const guint32 v)
{ return (hb_gdi_uint16_swap (v) << 16) | hb_gdi_uint16_swap (v >> 16); }

/*
 * Adapted from https://www.mail-archive.com/harfbuzz@lists.freedesktop.org/msg06538.html
 * by Konstantin Ritt.
 *
 * HarfBuzz added GDI support after this code was done, and may not have been enabled
 * in the build, so this now becomes the fallback method used to create a hb_face_t if
 * HarfBuzz was neither built with DirectWrite nor GDI support.
 */
#if !defined (USE_HB_DWRITE) && !defined (USE_HB_GDI)
static hb_blob_t *
hfont_reference_table (hb_face_t *face, hb_tag_t tag, void *user_data)
{
  HDC hdc;
  HFONT hfont, old_hfont;
  gchar *buf = NULL;
  DWORD size;

  /* We have a common DC for our PangoWin32Font, so let's just use it */
  hdc = _pango_win32_get_display_dc ();
  hfont = (HFONT) user_data;

  /* we want to restore things, just to be safe */
  old_hfont = SelectObject (hdc, hfont);
  if (old_hfont == NULL)
    {
      g_warning ("SelectObject() for the PangoWin32Font failed!");
      return hb_blob_get_empty ();
    }

  size = GetFontData (hdc, hb_gdi_uint32_swap (tag), 0, NULL, 0);

  /*
   * not all tags support retrieving the sizes, so don't warn,
   * just return hb_blob_get_empty()
   */
  if (size == GDI_ERROR)
    {
      SelectObject (hdc, old_hfont);
      return hb_blob_get_empty ();
    }

  buf = g_malloc (size * sizeof (gchar));

  /* This should be quite unlikely to fail if size was not GDI_ERROR */
  if (GetFontData (hdc, hb_gdi_uint32_swap (tag), 0, buf, size) == GDI_ERROR)
    size = 0;

  SelectObject (hdc, old_hfont);
  return hb_blob_create (buf, size, HB_MEMORY_MODE_READONLY, buf, g_free);
}
#endif

static hb_font_t *
pango_win32_font_create_hb_font (PangoFont *font)
{
  PangoWin32Font *win32font = (PangoWin32Font *)font;
  hb_face_t *face = NULL;
  hb_font_t *hb_font = NULL;

  g_return_val_if_fail (font != NULL, NULL);

#ifdef USE_HB_DWRITE
  face = pango_win32_font_create_hb_face_dwrite (win32font);
#else
  HFONT hfont;

  hfont = _pango_win32_font_get_hfont (font);

#ifdef USE_HB_GDI
  face = hb_gdi_face_create (hfont);
#else
  face = hb_face_create_for_tables (hfont_reference_table, (void *)hfont, NULL);
#endif

#endif

  hb_font = hb_font_create (face);
  hb_font_set_scale (hb_font, win32font->size, win32font->size);

  if (win32font->variations)
    {
      unsigned int n_axes;

      n_axes = hb_ot_var_get_axis_infos (face, 0, NULL, NULL);
      if (n_axes > 0)
        {
          hb_ot_var_axis_info_t *axes;
          float *coords;

          axes = g_newa (hb_ot_var_axis_info_t, n_axes);
          coords = g_newa (float, n_axes);

          hb_ot_var_get_axis_infos (face, 0, &n_axes, axes);
          for (unsigned int i = 0; i < n_axes; i++)
            coords[axes[i].axis_index] = axes[i].default_value;

          pango_parse_variations (win32font->variations, axes, n_axes, coords);

          hb_font_set_var_coords_design (hb_font, coords, n_axes);
        }
    }
  hb_face_destroy (face);

  return hb_font;
}
