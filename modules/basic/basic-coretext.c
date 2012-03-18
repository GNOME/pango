/* Pango
 * basic-coretext.c
 *
 * Copyright (C) 2005 Imendio AB
 * Copyright (C) 2010  Kristian Rietveld  <kris@gtk.org>
 * Copyright (C) 2012  Kristian Rietveld  <kris@lanedo.com>
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


/* The "RunIterator" helps us to iterate over the array of runs that is obtained from
 * the CoreText type setter. Even though Pango considers the string that is passed to
 * the shaping engine a single run, CoreText might consider it to consist out of
 * multiple runs. Because of this, we have an interface around the CoreText array of
 * runs that works like iterating a single array, which makes our job in the shaping
 * engine function easier.
 */

struct RunIterator
{
  CTLineRef line;
  CFStringRef cstr;
  CFArrayRef runs;
  CFIndex glyph_count;

  CFIndex total_ct_i;
  CFIndex ct_i;

  int current_run_number;
  CTRunRef current_run;
  CFIndex *current_indices;
  const CGGlyph *current_cgglyphs;
  CTRunStatus current_run_status;
};

static void
run_iterator_free_current_run (struct RunIterator *iter)
{
  iter->current_run_number = -1;
  iter->current_run = NULL;
  iter->current_cgglyphs = NULL;
  if (iter->current_indices)
    free (iter->current_indices);
  iter->current_indices = NULL;
}

static void
run_iterator_set_current_run (struct RunIterator *iter,
                              const int           run_number)
{
  CFIndex ct_glyph_count;

  run_iterator_free_current_run (iter);

  iter->current_run_number = run_number;
  iter->current_run = CFArrayGetValueAtIndex (iter->runs, run_number);
  iter->current_run_status = CTRunGetStatus (iter->current_run);
  iter->current_cgglyphs = CTRunGetGlyphsPtr (iter->current_run);

  ct_glyph_count = CTRunGetGlyphCount (iter->current_run);
  iter->current_indices = malloc (sizeof (CFIndex *) * ct_glyph_count);
  CTRunGetStringIndices (iter->current_run, CFRangeMake (0, ct_glyph_count),
                         iter->current_indices);

  iter->ct_i = 0;
}

static CFIndex
run_iterator_get_glyph_count (struct RunIterator *iter)
{
  CFIndex accumulator = 0;
  CFIndex i;

  for (i = 0; i < CFArrayGetCount (iter->runs); i++)
    accumulator += CTRunGetGlyphCount (CFArrayGetValueAtIndex (iter->runs, i));

  return accumulator;
}

static gboolean
run_iterator_is_rtl (struct RunIterator *iter)
{
  /* Assume run status is equal for all runs? */
  CTRunStatus run_status = CTRunGetStatus (CFArrayGetValueAtIndex (iter->runs, 0));

  return run_status & kCTRunStatusRightToLeft;
}

static gboolean
run_iterator_run_is_non_monotonic (struct RunIterator *iter)
{
  CTRunStatus run_status = CTRunGetStatus (iter->current_run);

  return run_status & kCTRunStatusNonMonotonic;
}

static gunichar
run_iterator_get_character (struct RunIterator *iter)
{
  return CFStringGetCharacterAtIndex (iter->cstr, iter->current_indices[iter->ct_i]);
}

static CGGlyph
run_iterator_get_cgglyph (struct RunIterator *iter)
{
  return iter->current_cgglyphs[iter->ct_i];
}

static CFIndex
run_iterator_get_index (struct RunIterator *iter)
{
  return iter->current_indices[iter->ct_i];
}

static void
run_iterator_create (struct RunIterator *iter,
                     const char         *text,
                     const gint          length,
                     CTFontRef           ctfont)
{
  char *copy;
  CFDictionaryRef attributes;
  CFAttributedStringRef attstr;

  CFTypeRef keys[] = {
      (CFTypeRef) kCTFontAttributeName
  };

  CFTypeRef values[] = {
      ctfont
  };

  /* Initialize RunIterator structure */
  iter->current_run_number = -1;
  iter->current_run = NULL;
  iter->current_indices = NULL;
  iter->current_cgglyphs = NULL;

  /* Create CTLine */
  attributes = CFDictionaryCreate (kCFAllocatorDefault,
                                   (const void **)keys,
                                   (const void **)values,
                                   1,
                                   &kCFCopyStringDictionaryKeyCallBacks,
                                   &kCFTypeDictionaryValueCallBacks);

  copy = g_strndup (text, length + 1);
  copy[length] = 0;

  iter->cstr = CFStringCreateWithCString (kCFAllocatorDefault, copy,
                                          kCFStringEncodingUTF8);
  g_free (copy);

  attstr = CFAttributedStringCreate (kCFAllocatorDefault,
                                     iter->cstr,
                                     attributes);

  iter->line = CTLineCreateWithAttributedString (attstr);
  iter->runs = CTLineGetGlyphRuns (iter->line);

  CFRelease (attstr);
  CFRelease (attributes);

  iter->total_ct_i = 0;
  iter->glyph_count = run_iterator_get_glyph_count (iter);

  /* If CoreText did not render any glyphs for this string (can happen,
   * e.g. a run solely consisting of a BOM), glyph_count will be zero and
   * we immediately set the iterator variable to indicate end of glyph list.
   */
  if (iter->glyph_count > 0)
    run_iterator_set_current_run (iter, 0);
  else
    iter->total_ct_i = -1;
}

static void
run_iterator_free (struct RunIterator *iter)
{
  run_iterator_free_current_run (iter);

  CFRelease (iter->line);
  CFRelease (iter->cstr);
}

static gboolean
run_iterator_at_end (struct RunIterator *iter)
{
  if (iter->total_ct_i == -1)
    return TRUE;

  return FALSE;
}

static void
run_iterator_advance (struct RunIterator *iter)
{
  if (iter->total_ct_i >= iter->glyph_count - 1)
    {
      run_iterator_free_current_run (iter);
      iter->ct_i = iter->total_ct_i = -1;
    }
  else
    {
      iter->total_ct_i++;
      iter->ct_i++;

      if (iter->total_ct_i < iter->glyph_count &&
          iter->ct_i >= CTRunGetGlyphCount (iter->current_run))
        {
          iter->current_run_number++;
          run_iterator_set_current_run (iter, iter->current_run_number);
        }
    }
}



struct GlyphInfo
{
  CFIndex index;
  CGGlyph cgglyph;
  gunichar wc;
};

static gint
glyph_info_compare_func (gconstpointer a, gconstpointer b)
{
  const struct GlyphInfo *gi_a = a;
  const struct GlyphInfo *gi_b = b;

  if (gi_a->index < gi_b->index)
    return -1;
  else if (gi_a->index > gi_b->index)
    return 1;
  /* else */
  return 0;
}

static void
glyph_info_free (gpointer data, gpointer user_data)
{
  g_slice_free (struct GlyphInfo, data);
}

static GSList *
create_core_text_glyph_list (const char *text,
                             gint        length,
                             CTFontRef   ctfont)
{
  GSList *glyph_list = NULL;
  struct RunIterator riter;

  run_iterator_create (&riter, text, length, ctfont);

  while (!run_iterator_at_end (&riter))
    {
      struct GlyphInfo *gi;

      gi = g_slice_new (struct GlyphInfo);
      gi->index = run_iterator_get_index (&riter);
      gi->cgglyph = run_iterator_get_cgglyph (&riter);
      gi->wc = run_iterator_get_character (&riter);

      glyph_list = g_slist_prepend (glyph_list, gi);

      run_iterator_advance (&riter);
    }

  glyph_list = g_slist_sort (glyph_list, glyph_info_compare_func);

  run_iterator_free (&riter);

  return glyph_list;
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
  gulong n_chars, gs_i, gs_prev_i;
  PangoCoreTextFont *cfont = PANGO_CORE_TEXT_FONT (font);
  PangoCoverage *coverage;
  GSList *glyph_list;
  GSList *glyph_iter;

  /* We first fully iterate over the glyph sequence generated by CoreText and
   * store this into a list, which is sorted after the iteration. We make a pass
   * over the sorted linked list to build up the PangoGlyphString.
   *
   * We have to do this in order to properly handle a bunch of characteristics of the
   * glyph sequence generated by the CoreText typesetter:
   *   # E.g. zero-width spaces do not end up in the CoreText glyph sequence. We have
   *     to manually account for the gap in the character indices.
   *   # Sometimes, CoreText generates two glyph for the same character index. We
   *     currently handle this "properly" as in we do not crash or corrupt memory,
   *     but that's about it.
   *   # Due to mismatches in size, the CoreText glyph sequence can either be longer or
   *     shorter than the PangoGlyphString. Note that the size of the PangoGlyphString
   *     should match the number of characters in "text".
   *
   * If performance becomes a problem, it is certainly possible to use a faster code
   * that only does a single iteration over the string for "simple cases". Simple cases
   * could include these that only consist out of one run (simple Latin text), which
   * don't have gaps in the glyph sequence and which are monotonically
   * increasing/decreasing.
   *
   * FIXME items for future fixing:
   *   # CoreText strings are UTF16, and the indices *often* refer to characters,
   *     but not *always*. Notable exception is when a character is encoded using
   *     two UTF16 code points. This are two characters in a CFString. At this point
   *     advancing a single character in the CFString and advancing a single character
   *     using g_utf8_next_char in the const char string goes out of sync.
   *   # We currently don't bother about LTR, Pango core appears to fix this up for us.
   *     (Even when we cared warnings were generated that strings were in the wrong
   *     order, this should be investigated).
   *   # When CoreText generates two glyphs for one character, only one is stored.
   *     This breaks the example strings for e.g. Georgian and Gothic.
   */

  glyph_list = create_core_text_glyph_list (text, length,
                                            pango_core_text_font_get_ctfont (cfont));

  /* Translate the glyph list to a PangoGlyphString */
  n_chars = g_utf8_strlen (text, length);
  pango_glyph_string_set_size (glyphs, n_chars);

  glyph_iter = glyph_list;

  coverage = pango_font_get_coverage (PANGO_FONT (cfont),
                                      analysis->language);

  for (gs_prev_i = -1, gs_i = 0, p = text; gs_i < n_chars;
       gs_prev_i = gs_i, gs_i++, p = g_utf8_next_char (p))
    {
      struct GlyphInfo *gi = glyph_iter != NULL ? glyph_iter->data : NULL;

      if (gi == NULL || gi->index > gs_i)
        {
          /* gs_i is behind, insert empty glyph */
          set_glyph (font, glyphs, gs_i, p - text, PANGO_GLYPH_EMPTY);
          continue;
        }
      else if (gi->index < gs_i)
        {
          while (gi && gi->index < gs_i)
            {
              glyph_iter = g_slist_next (glyph_iter);
              if (glyph_iter)
                gi = glyph_iter->data;
              else
                gi = NULL;
            }
        }

      if (gi != NULL && gi->index == gs_i)
        {
          gunichar mirrored_ch;
          PangoCoverageLevel result;

          if (analysis->level % 2)
            if (g_unichar_get_mirror_char (gi->wc, &mirrored_ch))
              gi->wc = mirrored_ch;

          if (gi->wc == 0xa0)	/* non-break-space */
            gi->wc = 0x20;

          result = pango_coverage_get (coverage, gi->wc);

          if (result != PANGO_COVERAGE_NONE)
            {
              set_glyph (font, glyphs, gs_i, p - text, gi->cgglyph);

              if (g_unichar_type (gi->wc) == G_UNICODE_NON_SPACING_MARK)
                {
                  if (gi->index > 0)
                    {
                      PangoRectangle logical_rect, ink_rect;

                      glyphs->glyphs[gs_i].geometry.width = MAX (glyphs->glyphs[gs_prev_i].geometry.width,
                                                                 glyphs->glyphs[gs_i].geometry.width);
                      glyphs->glyphs[gs_prev_i].geometry.width = 0;
                      glyphs->log_clusters[gs_i] = glyphs->log_clusters[gs_prev_i];

                      /* Some heuristics to try to guess how overstrike glyphs are
                       * done and compensate
                       */
                      pango_font_get_glyph_extents (font, glyphs->glyphs[gs_i].glyph, &ink_rect, &logical_rect);
                      if (logical_rect.width == 0 && ink_rect.x == 0)
                        glyphs->glyphs[gs_i].geometry.x_offset = (glyphs->glyphs[gs_i].geometry.width - ink_rect.width) / 2;
                    }
                }
            }
          else
            set_glyph (font, glyphs, gs_i, p - text, PANGO_GET_UNKNOWN_GLYPH (gi->wc));

          glyph_iter = g_slist_next (glyph_iter);
        }
    }

  pango_coverage_unref (coverage);
  g_slist_foreach (glyph_list, glyph_info_free, NULL);
  g_slist_free (glyph_list);
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

