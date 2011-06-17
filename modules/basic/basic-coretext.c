/* Pango
 * basic-coretext.c
 *
 * Copyright (C) 2005 Imendio AB
 * Copyright (C) 2010  Kristian Rietveld  <kris@gtk.org>
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
#include "pangocoretext.h"

/* No extra fields needed */
typedef PangoEngineShape      BasicEngineCoreText;
typedef PangoEngineShapeClass BasicEngineCoreTextClass ;

#define SCRIPT_ENGINE_NAME "BasicScriptEngineCoreText"
#define RENDER_TYPE PANGO_RENDER_TYPE_CORE_TEXT

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

  glyphs->glyphs[i].glyph = glyph;

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
  const char *p;
  char *copy;
  CTLineRef line;
  CFStringRef cstr;
  CFDictionaryRef attributes;
  CFAttributedStringRef attstr;
  PangoCoreTextFont *cfont = PANGO_CORE_TEXT_FONT (font);
  PangoCoverage *coverage;
  CFArrayRef runs;
  CTRunRef run;
  CTRunStatus run_status;
  CFIndex i, glyph_count;
  const CGGlyph *cgglyphs;

  CFTypeRef keys[] = {
      (CFTypeRef) kCTFontAttributeName
  };

  CFTypeRef values[] = {
      pango_core_text_font_get_ctfont (cfont)
  };

  attributes = CFDictionaryCreate (kCFAllocatorDefault,
                                   (const void **)keys,
                                   (const void **)values,
                                   1,
                                   &kCFCopyStringDictionaryKeyCallBacks,
                                   &kCFTypeDictionaryValueCallBacks);

  copy = g_strndup (text, length + 1);
  copy[length] = 0;

  cstr = CFStringCreateWithCString (kCFAllocatorDefault, copy,
                                    kCFStringEncodingUTF8);
  g_free (copy);

  attstr = CFAttributedStringCreate (kCFAllocatorDefault,
                                     cstr,
                                     attributes);

  line = CTLineCreateWithAttributedString (attstr);

  runs = CTLineGetGlyphRuns (line);

  /* Since Pango divides things into runs already, we assume there is
   * only a single run in this line.
   */
  run = CFArrayGetValueAtIndex (runs, 0);
  run_status = CTRunGetStatus (run);
  glyph_count = CTRunGetGlyphCount (run);
  cgglyphs = CTRunGetGlyphsPtr (run);

  p = text;
  pango_glyph_string_set_size (glyphs, glyph_count);
  coverage = pango_font_get_coverage (PANGO_FONT (cfont),
                                      analysis->language);

  for (i = 0; i < glyph_count; i++)
    {
      CFIndex real_i, prev_i;
      gunichar wc;
      gunichar mirrored_ch;

      wc = g_utf8_get_char (p);

      if (analysis->level % 2)
	if (pango_get_mirror_char (wc, &mirrored_ch))
	  wc = mirrored_ch;

      if (run_status & kCTRunStatusRightToLeft)
        {
          real_i = glyph_count - i - 1;
          prev_i = real_i + 1;
        }
      else
        {
          real_i = i;
          prev_i = real_i - 1;
        }

      if (wc == 0xa0)	/* non-break-space */
	wc = 0x20;

      if (pango_is_zero_width (wc))
	{
	  set_glyph (font, glyphs, real_i, p - text, PANGO_GLYPH_EMPTY);
	}
      else
	{
          PangoCoverageLevel result;

          result = pango_coverage_get (coverage, wc);

          if (result != PANGO_COVERAGE_NONE)
            {
              set_glyph (font, glyphs, real_i, p - text, cgglyphs[real_i]);

              if (g_unichar_type (wc) == G_UNICODE_NON_SPACING_MARK)
                {
                  if (i > 0)
                    {
                      PangoRectangle logical_rect, ink_rect;

                      glyphs->glyphs[real_i].geometry.width = MAX (glyphs->glyphs[prev_i].geometry.width,
                                                                   glyphs->glyphs[prev_i].geometry.width);
                      glyphs->glyphs[prev_i].geometry.width = 0;
                      glyphs->log_clusters[real_i] = glyphs->log_clusters[prev_i];

                      /* Some heuristics to try to guess how overstrike glyphs are
                       * done and compensate
                       */
                      pango_font_get_glyph_extents (font, glyphs->glyphs[real_i].glyph, &ink_rect, &logical_rect);
                      if (logical_rect.width == 0 && ink_rect.x == 0)
                        glyphs->glyphs[real_i].geometry.x_offset = (glyphs->glyphs[real_i].geometry.width - ink_rect.width) / 2;
                    }
                }
            }
          else
            {
              set_glyph (font, glyphs, real_i, p - text,
                         PANGO_GET_UNKNOWN_GLYPH (wc));
            }
        }

      p = g_utf8_next_char (p);
    }

  CFRelease (line);
  CFRelease (attstr);
  CFRelease (cstr);
  CFRelease (attributes);
  pango_coverage_unref (coverage);
}

static void
basic_engine_core_text_class_init (PangoEngineShapeClass *class)
{
  class->script_shape = basic_engine_shape;
}

PANGO_ENGINE_SHAPE_DEFINE_TYPE (BasicEngineCoreText, basic_engine_core_text,
				basic_engine_core_text_class_init, NULL);

void
PANGO_MODULE_ENTRY(init) (GTypeModule *module)
{
  basic_engine_core_text_register_type (module);
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
    return g_object_new (basic_engine_core_text_type, NULL);
  else
    return NULL;
}

