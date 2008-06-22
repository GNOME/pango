/* Pango
 * basic-atsui.c
 *
 * Copyright (C) 2005 Imendio AB
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
#include <glib.h>
#include <string.h>
#include <Carbon/Carbon.h>
#include "pango-engine.h"
#include "pango-utils.h"
#include "pango-fontmap.h"
#include "pangoatsui.h"

/* No extra fields needed */
typedef PangoEngineShape      BasicEngineATSUI;
typedef PangoEngineShapeClass BasicEngineATSUIClass ;

#define SCRIPT_ENGINE_NAME "BasicScriptEngineATSUI"
#define RENDER_TYPE PANGO_RENDER_TYPE_ATSUI

static PangoEngineScriptInfo basic_scripts[] = {
  { PANGO_SCRIPT_COMMON,   "" }
};

static PangoEngineInfo script_engines[] = {
  {
    SCRIPT_ENGINE_NAME,
    PANGO_ENGINE_TYPE_SHAPE,
    RENDER_TYPE,
    basic_scripts, G_N_ELEMENTS(basic_scripts)
  }
};

static void
set_glyph (PangoFont        *font,
	   PangoGlyphString *glyphs,
	   int               i,
	   int               offset,
	   PangoGlyph        glyph)
{
  PangoRectangle logical_rect;

  glyphs->glyphs[i].glyph = G_UNLIKELY (glyph == kATSDeletedGlyphcode) ?
			    PANGO_GLYPH_EMPTY : glyph;

  glyphs->glyphs[i].geometry.x_offset = 0;
  glyphs->glyphs[i].geometry.y_offset = 0;

  glyphs->log_clusters[i] = offset;
  pango_font_get_glyph_extents (font, glyphs->glyphs[i].glyph, NULL, &logical_rect);
  glyphs->glyphs[i].geometry.width = logical_rect.width;
}

static void
basic_engine_shape (PangoEngineShape    *engine,
		    PangoFont           *font,
		    const char          *text,
		    gint                 length,
		    const PangoAnalysis *analysis,
		    PangoGlyphString    *glyphs)
{
  gunichar2 *utf16;
  long n16;
  ATSUTextLayout text_layout;
  ATSLayoutRecord *layout_records;
  OSStatus err;
  ItemCount glyph_count;
  int i;
  const char *p;
  PangoATSUIFont *afont = PANGO_ATSUI_FONT (font);
  ATSUStyle style;
  ATSUFontID fontID;
  ATSUAttributeTag styleTags[] = { kATSUFontTag };
  ATSUAttributeValuePtr styleValues[] = { &fontID };
  ByteCount styleSizes[] = { sizeof (ATSUFontID) };

  utf16 = g_utf8_to_utf16 (text, length, NULL, &n16, NULL);

  err = ATSUCreateTextLayout (&text_layout);
  err = ATSUSetTextPointerLocation (text_layout, utf16, 0, n16, n16);

  err = ATSUCreateStyle(&style);
  fontID = pango_atsui_font_get_atsu_font_id (afont);

  err = ATSUSetAttributes(style,
			  sizeof(styleTags) / sizeof(styleTags[0]),
			  styleTags, styleSizes, styleValues);

  err = ATSUSetRunStyle(text_layout,
			style, kATSUFromTextBeginning, kATSUToTextEnd);

  err = ATSUDirectGetLayoutDataArrayPtrFromTextLayout (text_layout, 0,
						       kATSUDirectDataLayoutRecordATSLayoutRecordCurrent,
						       (void *)&layout_records,
						       &glyph_count);

  p = text;
  pango_glyph_string_set_size (glyphs, glyph_count - 1);

  for (i = 0; i < glyph_count - 1; i++)
    {
      gunichar wc;
      gunichar mirrored_ch;

      wc = g_utf8_get_char (p);

      if (analysis->level % 2)
	if (pango_get_mirror_char (wc, &mirrored_ch))
	  wc = mirrored_ch;

      if (wc == 0xa0)	/* non-break-space */
	wc = 0x20;

      if (pango_is_zero_width (wc))
	{
	  set_glyph (font, glyphs, i, p - text, PANGO_GLYPH_EMPTY);
	}
      else
	{
	  set_glyph (font, glyphs, i, p - text, layout_records[i].glyphID);

	  if (g_unichar_type (wc) == G_UNICODE_NON_SPACING_MARK)
	    {
	      if (i > 0)
		{
		  PangoRectangle logical_rect, ink_rect;

		  glyphs->glyphs[i].geometry.width = MAX (glyphs->glyphs[i-1].geometry.width,
							  glyphs->glyphs[i].geometry.width);
		  glyphs->glyphs[i-1].geometry.width = 0;
		  glyphs->log_clusters[i] = glyphs->log_clusters[i-1];

		  /* Some heuristics to try to guess how overstrike glyphs are
		   * done and compensate
		   */
		  pango_font_get_glyph_extents (font, glyphs->glyphs[i].glyph, &ink_rect, &logical_rect);
		  if (logical_rect.width == 0 && ink_rect.x == 0)
		    glyphs->glyphs[i].geometry.x_offset = (glyphs->glyphs[i].geometry.width - ink_rect.width) / 2;
		}
	    }
	}

      p = g_utf8_next_char (p);
    }

  ATSUDirectReleaseLayoutDataArrayPtr (NULL, kATSUDirectDataLayoutRecordATSLayoutRecordCurrent,
				       (void *)&layout_records);

  ATSUDisposeStyle (style);
  ATSUDisposeTextLayout (text_layout);

  g_free (utf16);
}

static void
basic_engine_atsui_class_init (PangoEngineShapeClass *class)
{
  class->script_shape = basic_engine_shape;
}

PANGO_ENGINE_SHAPE_DEFINE_TYPE (BasicEngineATSUI, basic_engine_atsui,
				basic_engine_atsui_class_init, NULL);

void
PANGO_MODULE_ENTRY(init) (GTypeModule *module)
{
  basic_engine_atsui_register_type (module);
}

void
PANGO_MODULE_ENTRY(exit) (void)
{
}

void
PANGO_MODULE_ENTRY(list) (PangoEngineInfo **engines,
			  int              *n_engines)
{
  *engines = script_engines;
  *n_engines = G_N_ELEMENTS (script_engines);
}

PangoEngine *
PANGO_MODULE_ENTRY(create) (const char *id)
{
  if (!strcmp (id, SCRIPT_ENGINE_NAME))
    return g_object_new (basic_engine_atsui_type, NULL);
  else
    return NULL;
}

