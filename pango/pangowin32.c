/* Pango
 * pangowin32.c: Routines for handling Windows fonts
 *
 * Copyright (C) 1999 Red Hat Software
 * Copyright (C) 2000 Tor Lillqvist
 * Copyright (C) 2001 Alexander Larsson
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

#include "pango-utils.h"
#include "pangowin32.h"
#include "pangowin32-private.h"
#include "modules.h"

#define PANGO_TYPE_WIN32_FONT            (pango_win32_font_get_type ())
#define PANGO_WIN32_FONT(object)         (G_TYPE_CHECK_INSTANCE_CAST ((object), PANGO_TYPE_WIN32_FONT, PangoWin32Font))
#define PANGO_WIN32_FONT_CLASS(klass)    (G_TYPE_CHECK_CLASS_CAST ((klass), PANGO_TYPE_WIN32_FONT, PangoWin32FontClass))
#define PANGO_WIN32_IS_FONT(object)      (G_TYPE_CHECK_INSTANCE_TYPE ((object), PANGO_TYPE_WIN32_FONT))
#define PANGO_WIN32_IS_FONT_CLASS(klass) (G_TYPE_CHECK_CLASS_TYPE ((klass), PANGO_TYPE_WIN32_FONT))
#define PANGO_WIN32_FONT_GET_CLASS(obj)  (G_TYPE_INSTANCE_GET_CLASS ((obj), PANGO_TYPE_WIN32_FONT, PangoWin32FontClass))

HDC pango_win32_hdc;

typedef struct _PangoWin32FontClass   PangoWin32FontClass;

struct _PangoWin32FontClass
{
  PangoFontClass parent_class;
};

static PangoFontClass *parent_class;	/* Parent class structure for PangoWin32Font */

static void pango_win32_font_class_init (PangoWin32FontClass *class);
static void pango_win32_font_init       (PangoWin32Font      *win32font);
static void pango_win32_font_dispose    (GObject             *object);
static void pango_win32_font_finalize   (GObject             *object);

static PangoFontDescription *pango_win32_font_describe          (PangoFont        *font);
static PangoCoverage        *pango_win32_font_get_coverage      (PangoFont        *font,
								 PangoLanguage    *lang);
static void                  pango_win32_font_calc_coverage     (PangoFont        *font,
								 PangoCoverage    *coverage);
static PangoEngineShape     *pango_win32_font_find_shaper       (PangoFont        *font,
								 PangoLanguage    *lang,
								 guint32           ch);
static void                  pango_win32_font_get_glyph_extents (PangoFont        *font,
								 PangoGlyph        glyph,
								 PangoRectangle   *ink_rect,
								 PangoRectangle   *logical_rect);
static void                  pango_win32_font_get_metrics       (PangoFont        *font,
								 PangoLanguage    *lang,
								 PangoFontMetrics *metrics);
static HFONT                 pango_win32_get_hfont              (PangoFont        *font);
static void                  pango_win32_get_item_properties    (PangoItem        *item,
								 PangoUnderline   *uline,
								 PangoAttrColor   *fg_color,
								 gboolean         *fg_set,
								 PangoAttrColor   *bg_color,
								 gboolean         *bg_set);

static inline HFONT
pango_win32_get_hfont (PangoFont *font)
{
  PangoWin32Font *win32font = (PangoWin32Font *)font;
  PangoWin32FontCache *cache;
  HGDIOBJ oldfont;
  TEXTMETRIC tm;

  if (!win32font->hfont)
    {
      cache = pango_win32_font_map_get_font_cache (win32font->fontmap);
      
      win32font->hfont = pango_win32_font_cache_load (cache, &win32font->logfont);
      if (!win32font->hfont)
	{
	  g_warning ("Cannot load font '%s\n", win32font->logfont.lfFaceName);
	  return NULL;
	}

      SelectObject (pango_win32_hdc, win32font->hfont);
      GetTextMetrics (pango_win32_hdc, &tm);

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
 **/
PangoContext *
pango_win32_get_context (void)
{
  PangoContext *result;
  static gboolean registered_modules = FALSE;
  int i;

  if (!registered_modules)
    {
      registered_modules = TRUE;
      
      for (i = 0; _pango_included_win32_modules[i].list; i++)
        pango_module_register (&_pango_included_win32_modules[i]);
    }

  result = pango_context_new ();
  pango_context_add_font_map (result, pango_win32_font_map_for_display ());

  return result;
}

static GType
pango_win32_font_get_type (void)
{
  static GType object_type = 0;

  if (!object_type)
    {
      static const GTypeInfo object_info =
      {
        sizeof (PangoWin32FontClass),
        (GBaseInitFunc) NULL,
        (GBaseFinalizeFunc) NULL,
        (GClassInitFunc) pango_win32_font_class_init,
        NULL,           /* class_finalize */
        NULL,           /* class_data */
        sizeof (PangoWin32Font),
        0,              /* n_preallocs */
        (GInstanceInitFunc) pango_win32_font_init,
      };
      
      object_type = g_type_register_static (PANGO_TYPE_FONT,
                                            "PangoWin32Font",
                                            &object_info, 0);
    }
  
  return object_type;
}

static void 
pango_win32_font_init (PangoWin32Font *win32font)
{
  win32font->size = -1;

  win32font->glyph_info = g_hash_table_new_full (NULL, NULL, NULL, g_free);
}

static void
pango_win32_font_class_init (PangoWin32FontClass *class)
{
  GObjectClass *object_class = G_OBJECT_CLASS (class);
  PangoFontClass *font_class = PANGO_FONT_CLASS (class);

  parent_class = g_type_class_peek_parent (class);
  
  object_class->finalize = pango_win32_font_finalize;
  object_class->dispose = pango_win32_font_dispose;
  
  font_class->describe = pango_win32_font_describe;
  font_class->get_coverage = pango_win32_font_get_coverage;
  font_class->find_shaper = pango_win32_font_find_shaper;
  font_class->get_glyph_extents = pango_win32_font_get_glyph_extents;
  font_class->get_metrics = pango_win32_font_get_metrics;

  if (pango_win32_hdc == NULL)
    pango_win32_hdc = CreateDC ("DISPLAY", NULL, NULL, NULL);
}

PangoWin32Font *
pango_win32_font_new (PangoFontMap  *fontmap,
		      const LOGFONT *lfp,
		      int	     size)
{
  PangoWin32Font *result;

  g_return_val_if_fail (fontmap != NULL, NULL);
  g_return_val_if_fail (lfp != NULL, NULL);

  result = (PangoWin32Font *)g_object_new (PANGO_TYPE_WIN32_FONT, NULL);
  
  result->fontmap = fontmap;
  g_object_ref (G_OBJECT (fontmap));

  result->size = size;
  pango_win32_make_matching_logfont (fontmap, lfp, size,
				     &result->logfont);

  return result;
}

/**
 * pango_win32_render:
 * @hdc:     the device context
 * @font:    the font in which to draw the string
 * @glyphs:  the glyph string to draw
 * @x:       the x position of start of string (in pixels)
 * @y:       the y position of baseline (in pixels)
 *
 * Render a PangoGlyphString onto a Windows DC
 */
void 
pango_win32_render (HDC                hdc,
		    PangoFont         *font,
		    PangoGlyphString  *glyphs,
		    int                x, 
		    int                y)
{
  HFONT old_hfont = NULL;
  HFONT hfont;
  UINT orig_align = -1;
  int i;
  int x_off = 0;
  guint16 *glyph_indexes;
  INT *dX;

  g_return_if_fail (glyphs != NULL);

  hfont = pango_win32_get_hfont (font);
  if (!hfont)
    return;

  old_hfont = SelectObject (hdc, hfont);
		
  glyph_indexes = g_new (guint16, glyphs->num_glyphs);
  dX = g_new (INT, glyphs->num_glyphs);

  for (i = 0; i <glyphs->num_glyphs; i++)
    {
      glyph_indexes[i] = glyphs->glyphs[i].glyph;
      dX[i] = glyphs->glyphs[i].geometry.width / PANGO_SCALE;
    }

  ExtTextOutW (hdc, x, y,
	       ETO_GLYPH_INDEX,
	       NULL,
	       glyph_indexes, glyphs->num_glyphs,
	       dX);

  SelectObject (hdc, old_hfont); /* restore */
  g_free (glyph_indexes);
  g_free (dX);
}

static void
pango_win32_font_get_glyph_extents (PangoFont      *font,
				    PangoGlyph      glyph,
				    PangoRectangle *ink_rect,
				    PangoRectangle *logical_rect)
{
  PangoWin32Font *win32font = (PangoWin32Font *)font;
  guint16 glyph_index = glyph;
  SIZE size;
  GLYPHMETRICS gm;
  guint32 res;
  HFONT hfont;
  MAT2 m = {{0,1}, {0,0}, {0,0}, {0,1}};
  PangoWin32GlyphInfo *info;

  info = g_hash_table_lookup (win32font->glyph_info, GUINT_TO_POINTER (glyph));
  
  if (!info)
    {
      info = g_new (PangoWin32GlyphInfo, 1);

      info->ink_rect.x = 0;
      info->ink_rect.width = 0;
      info->ink_rect.y = 0;
      info->ink_rect.height = 0;

      info->logical_rect.x = 0;
      info->logical_rect.width = 0;
      info->logical_rect.y = 0;
      info->logical_rect.height = 0;

      memset (&gm, 0, sizeof (gm));

      hfont = pango_win32_get_hfont (font);
      SelectObject (pango_win32_hdc, hfont);
      /* FIXME: (Alex) This constant reuse of pango_win32_hdc is
	 not thread-safe */
      res = GetGlyphOutlineA (pango_win32_hdc, 
			      glyph_index,
			      GGO_METRICS | GGO_GLYPH_INDEX,
			      &gm, 
			      0, NULL,
			      &m);
  
      if (res == GDI_ERROR)
	{
	  guint32 error = GetLastError ();
	  g_warning ("GetGlyphOutline() failed with error: %d\n", error);
	  return;
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

static void
pango_win32_font_get_metrics (PangoFont        *font,
			      PangoLanguage    *language,
			      PangoFontMetrics *metrics)
{
  HFONT hfont;
  TEXTMETRIC tm;
  PangoLayout *layout;
  PangoRectangle extents;
  PangoContext *context;
  
  metrics->ascent = 0;
  metrics->descent = 0;
  metrics->approximate_digit_width = 0;
  metrics->approximate_char_width = 0;
  
  hfont = pango_win32_get_hfont (font);

  if (hfont != NULL)
    {
      SelectObject (pango_win32_hdc, hfont);
      GetTextMetrics (pango_win32_hdc, &tm);

      metrics->ascent = tm.tmAscent * PANGO_SCALE;
      metrics->descent = tm.tmDescent * PANGO_SCALE;
      metrics->approximate_digit_width = /* really an approximation ... */
      metrics->approximate_char_width = tm.tmAveCharWidth * PANGO_SCALE;

      /* lovely copy&paste programming (from pangox.c) */
      /* This is sort of a sledgehammer solution, but we cache this
       * stuff so not a huge deal hopefully. Get the avg. width of the
       * chars in "0123456789"
       */
      context = pango_win32_get_context ();
      pango_context_set_language (context, language);
      layout = pango_layout_new (context);
      pango_layout_set_text (layout, "0123456789", -1);

      pango_layout_get_extents (layout, NULL, &extents);
   
      metrics->approximate_digit_width = extents.width / 10.0;

      g_object_unref (G_OBJECT (layout));
      g_object_unref (G_OBJECT (context));
    }

  return;
}


/**
 * pango_win32_font_logfont:
 * @font: a #PangoFont which must be from the Win32 backend
 * 
 * Determine the LOGFONT struct for the specified bfont.
 * 
 * Return value: A newly allocated LOGFONT struct. It must be
 * freed with g_free().
 **/
LOGFONT *
pango_win32_font_logfont (PangoFont *font)
{
  PangoWin32Font *win32font = (PangoWin32Font *)font;
  LOGFONT *lfp;

  g_return_val_if_fail (font != NULL, NULL);
  g_return_val_if_fail (PANGO_WIN32_IS_FONT (font), NULL);

  lfp = g_new (LOGFONT, 1);
  *lfp = win32font->logfont;

  return lfp;
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

  G_OBJECT_CLASS (parent_class)->dispose (object);
}

static void
pango_win32_font_finalize (GObject *object)
{
  PangoWin32Font *win32font = (PangoWin32Font *)object;
  PangoWin32FontCache *cache = pango_win32_font_map_get_font_cache (win32font->fontmap);
  int i;

  if (win32font->hfont != NULL)
    pango_win32_font_cache_unload (cache, win32font->hfont);

  if (win32font->entry)
    pango_win32_font_entry_remove (win32font->entry, PANGO_FONT (win32font));
 
  g_hash_table_destroy (win32font->glyph_info);
 
  g_object_unref (G_OBJECT (win32font->fontmap));

  G_OBJECT_CLASS (parent_class)->finalize (object);
}

static PangoFontDescription *
pango_win32_font_describe (PangoFont *font)
{
  PangoFontDescription *desc;
  PangoWin32Font *win32font = PANGO_WIN32_FONT (font);

  desc = pango_font_description_copy (&win32font->entry->description);
  desc->size = win32font->size;
  
  return desc;
}

PangoMap *
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

static PangoCoverage *
pango_win32_font_get_coverage (PangoFont  *font,
			       PangoLanguage *lang)
{
  PangoCoverage *coverage;
  PangoWin32Font *win32font = (PangoWin32Font *)font;

  coverage = pango_win32_font_entry_get_coverage (win32font->entry);
  if (!coverage)
    {
      coverage = pango_coverage_new ();
      pango_win32_font_calc_coverage (font, coverage);
      
      pango_win32_font_entry_set_coverage (win32font->entry, coverage);
    }

  return coverage;
}

static PangoEngineShape *
pango_win32_font_find_shaper (PangoFont   *font,
			      PangoLanguage *lang,
			      guint32      ch)
{
  PangoMap *shape_map = NULL;

  shape_map = pango_win32_get_shaper_map (lang);
  return (PangoEngineShape *)pango_map_get_engine (shape_map, ch);
}

/* Utility functions */

/**
 * pango_win32_get_unknown_glyph:
 * @font: a #PangoFont
 * 
 * Return the index of a glyph suitable for drawing unknown characters.
 * 
 * Return value: a glyph index into @font
 **/
PangoGlyph
pango_win32_get_unknown_glyph (PangoFont *font)
{
  return 0;
}

/**
 * pango_win32_render_layout_line:
 * @hdc:       HDC to use for uncolored drawing
 * @line:      a #PangoLayoutLine
 * @x:         the x position of start of string (in pixels)
 * @y:         the y position of baseline (in pixels)
 *
 * Render a #PangoLayoutLine onto a device context
 */
void 
pango_win32_render_layout_line (HDC               hdc,
				PangoLayoutLine  *line,
				int               x, 
				int               y)
{
  GSList *tmp_list = line->runs;
  PangoRectangle overall_rect;
  PangoRectangle logical_rect;
  PangoRectangle ink_rect;
  
  int x_off = 0;

  pango_layout_line_get_extents (line,NULL, &overall_rect);
  
  while (tmp_list)
    {
      HBRUSH oldfg;
      HBRUSH brush;
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
	  Rectangle (hdc, x + (x_off + logical_rect.x) / PANGO_SCALE,
			  y + overall_rect.y / PANGO_SCALE,
			  logical_rect.width / PANGO_SCALE,
			  overall_rect.height / PANGO_SCALE);
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
			  x + x_off / PANGO_SCALE, y);

      switch (uline)
	{
	case PANGO_UNDERLINE_NONE:
	  break;
	case PANGO_UNDERLINE_DOUBLE:
	  points[0].x = x + (x_off + ink_rect.x) / PANGO_SCALE - 1;
	  points[0].y = points[1].y = y + 4;
	  points[1].x = x + (x_off + ink_rect.x + ink_rect.width) / PANGO_SCALE;
	  Polyline (hdc, points, 2);
	  /* Fall through */
	case PANGO_UNDERLINE_SINGLE:
	  points[0].y = points[1].y = y + 2;
	  Polyline (hdc, points, 2);
	  break;
	case PANGO_UNDERLINE_LOW:
	  points[0].x = x + (x_off + ink_rect.x) / PANGO_SCALE - 1;
	  points[0].y = points[1].y = y + (ink_rect.y + ink_rect.height) / PANGO_SCALE + 2;
	  points[1].x = x + (x_off + ink_rect.x + ink_rect.width) / PANGO_SCALE;
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
pango_win32_render_layout (HDC              hdc,
			   PangoLayout     *layout,
			   int              x, 
			   int              y)
{
  PangoRectangle logical_rect;
  GSList *tmp_list;
  PangoAlignment align;
  int indent;
  int width;
  int y_offset = 0;

  gboolean first = FALSE;
  
  g_return_if_fail (layout != NULL);

  indent = pango_layout_get_indent (layout);
  width = pango_layout_get_width (layout);
  align = pango_layout_get_alignment (layout);

  if (width == -1 && align != PANGO_ALIGN_LEFT)
    {
      pango_layout_get_extents (layout, NULL, &logical_rect);
      width = logical_rect.width;
    }
  
  tmp_list = pango_layout_get_lines (layout);
  while (tmp_list)
    {
      PangoLayoutLine *line = tmp_list->data;
      int x_offset;
      
      pango_layout_line_get_extents (line, NULL, &logical_rect);

      if (width != 1 && align == PANGO_ALIGN_RIGHT)
	x_offset = width - logical_rect.width;
      else if (width != 1 && align == PANGO_ALIGN_CENTER)
	x_offset = (width - logical_rect.width) / 2;
      else
	x_offset = 0;

      if (first)
	{
	  if (indent > 0)
	    {
	      if (align == PANGO_ALIGN_LEFT)
		x_offset += indent;
	      else
		x_offset -= indent;
	    }

	  first = FALSE;
	}
      else
	{
	  if (indent < 0)
	    {
	      if (align == PANGO_ALIGN_LEFT)
		x_offset -= indent;
	      else
		x_offset += indent;
	    }
	}
	  
      pango_win32_render_layout_line (hdc, line,
				      x + x_offset / PANGO_SCALE,
				      y + (y_offset - logical_rect.y) / PANGO_SCALE);

      y_offset += logical_rect.height;
      tmp_list = tmp_list->next;
    }
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



/* Various truetype defines: */

#define MAKE_TT_TABLE_NAME(c1, c2, c3, c4) \
   (((guint32)c4) << 24 | ((guint32)c3) << 16 | ((guint32)c2) << 8 | ((guint32)c1))

#define CMAP (MAKE_TT_TABLE_NAME('c','m','a','p'))
#define CMAP_HEADER_SIZE 4
#define ENCODING_TABLE_SIZE 8
#define TABLE_4_HEADER_SIZE (sizeof (guint16)*6)

#define UNICODE_PLATFORM_ID 3
#define UNICODE_SPECIFIC_ID 1

#pragma pack(1)

struct encoding_table { /* Must be packed! */
  guint16 platform_id;
  guint16 encoding_id;
  guint32 offset;
};

struct type_4_table { /* Must be packed! */
  guint16 format;
  guint16 length;
  guint16 version;
  guint16 seg_count_x_2;
  guint16 search_range;
  guint16 entry_selector;
  guint16 range_shift;

  guint16 reserved;

  guint16 arrays[1];
};

guint 
get_unicode_mapping_offset (HDC hdc)
{
  guint16 n_tables;
  struct encoding_table *table;
  gint32 res;
  int i;
  guint32 offset;

  /* Get The number of encoding tables, at offset 2 */
  res = GetFontData (hdc, CMAP, 2, &n_tables, sizeof (guint16));
  if (res != sizeof (guint16))
    return 0;

  n_tables = GUINT16_FROM_BE (n_tables);
  
  table = g_malloc (ENCODING_TABLE_SIZE*n_tables);
  
  res = GetFontData (hdc, CMAP, CMAP_HEADER_SIZE, table, ENCODING_TABLE_SIZE*n_tables);
  if (res != ENCODING_TABLE_SIZE*n_tables)
    return 0;

  for (i = 0; i < n_tables; i++)
    {
      if (table[i].platform_id == GUINT16_TO_BE (UNICODE_PLATFORM_ID) &&
	  table[i].encoding_id == GUINT16_TO_BE (UNICODE_SPECIFIC_ID))
	{
	  offset = GUINT32_FROM_BE (table[i].offset);
	  g_free (table);
	  return offset;
	}
    }
  g_free (table);
  return 0;
}

struct type_4_table *
get_unicode_mapping (HDC hdc)
{
  guint32 offset;
  guint32 res;
  guint16 length;
  guint16 *tbl, *tbl_end;
  struct type_4_table *table;

  /* FIXME: Could look here at the CRC for the font in the DC 
            and return a cached copy if the same */

  offset = get_unicode_mapping_offset (hdc);
  if (offset == 0)
    return NULL;

  res = GetFontData (hdc, CMAP, offset + 2, &length, sizeof (guint16));
  if (res != sizeof (guint16))
    return NULL;
  length = GUINT16_FROM_BE (length);

  table = g_malloc (length);

  res = GetFontData (hdc, CMAP, offset, table, length);
  if (res != length)
    {
      g_free (table);
      return NULL;
    }
  
  table->format = GUINT16_FROM_BE (table->format);
  table->length = GUINT16_FROM_BE (table->length);
  table->version = GUINT16_FROM_BE (table->version);
  table->seg_count_x_2 = GUINT16_FROM_BE (table->seg_count_x_2);
  table->search_range = GUINT16_FROM_BE (table->search_range);
  table->entry_selector = GUINT16_FROM_BE (table->entry_selector);
  table->range_shift = GUINT16_FROM_BE (table->range_shift);

  if (table->format != 4)
    {
      g_free (table);
      return NULL;
    }
  
  tbl_end = (guint16 *)((char *)table + length);
  tbl = &table->reserved;

  while (tbl < tbl_end)
    {
      *tbl = GUINT16_FROM_BE (*tbl);
      tbl++;
    }

  return table;
}

guint16 *
get_id_range_offset (struct type_4_table *table)
{
  gint32 seg_count = table->seg_count_x_2/2;
  return &table->arrays[seg_count*3];
}

guint16 *
get_id_delta (struct type_4_table *table)
{
  gint32 seg_count = table->seg_count_x_2/2;
  return &table->arrays[seg_count*2];
}

guint16 *
get_start_count (struct type_4_table *table)
{
  gint32 seg_count = table->seg_count_x_2/2;
  return &table->arrays[seg_count*1];
}

guint16 *
get_end_count (struct type_4_table *table)
{
  gint32 seg_count = table->seg_count_x_2/2;
  /* Apparently the reseved spot is not reserved for 
     the end_count array!? */
  return (&table->arrays[seg_count*0])-1;
}


gboolean
find_segment (struct type_4_table *table, 
	      guint16              wc,
	      guint16             *segment)
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

static struct type_4_table *
font_get_unicode_table (PangoFont *font)
{
  PangoWin32Font *win32font = (PangoWin32Font *)font;
  HFONT hfont;
  struct type_4_table *table;

  if (win32font->entry->unicode_table)
    return (struct type_4_table *)win32font->entry->unicode_table;

  hfont = pango_win32_get_hfont (font);
  SelectObject (pango_win32_hdc, hfont);
  table = get_unicode_mapping (pango_win32_hdc);
  
  win32font->entry->unicode_table = table;

  return table;
}

gint
pango_win32_font_get_glyph_index (PangoFont *font,
				  gunichar   wc)
{
  struct type_4_table *table;
  guint16 *id_range_offset;
  guint16 *id_delta;
  guint16 *start_count;
  guint16 segment;
  guint16 id;
  guint16 ch = wc;
  guint16 glyph;

  /* Do GetFontData magic on font->hfont here. */
  table = font_get_unicode_table (font);

  if (table == NULL)
    return 0;
  
  if (!find_segment (table, ch, &segment))
    return 0;

  id_range_offset = get_id_range_offset (table);
  id_delta = get_id_delta (table);
  start_count = get_start_count (table);

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
  return glyph;
}

static void
pango_win32_font_calc_coverage (PangoFont     *font,
				PangoCoverage *coverage)
{
  struct type_4_table *table;
  guint16 *id_range_offset;
  guint16 *id_delta;
  guint16 *start_count;
  guint16 *end_count;
  guint16 seg_count;
  guint segment;
  guint16 id;
  guint32 ch;
  guint16 glyph;
  int i;

  /* Do GetFontData magic on font->hfont here. */

  table = font_get_unicode_table (font);

  if (table == NULL)
    return;
  
  seg_count = table->seg_count_x_2/2;
  end_count = get_end_count (table);
  start_count = get_start_count (table);
  id_range_offset = get_id_range_offset (table);

  for (i = 0; i < seg_count; i++)
    {
      if (id_range_offset[i] == 0)
	{
	  for (ch = start_count[i]; 
	       ch <= end_count[i];
	       ch++)
	    pango_coverage_set (coverage, ch, PANGO_COVERAGE_EXACT);
	}
      else
	{
	  for (ch = start_count[i]; 
	       ch <= end_count[i];
	       ch++)
	    {
	      id = *(id_range_offset[i]/2 +
		     (ch - start_count[i]) +
		     &id_range_offset[i]);
	      if (id)
		pango_coverage_set (coverage, ch, PANGO_COVERAGE_EXACT);
	    }
	}
    }
}


