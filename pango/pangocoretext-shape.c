/* Pango
 * pangocoretext-shape.c
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
#include "pango-utils.h"
#include "pangocoretext-private.h"
#include "pango-impl-utils.h"

#if defined(MAC_OS_X_VERSION_MAX_ALLOWED) && MAC_OS_X_VERSION_MAX_ALLOWED < 1060
CF_INLINE Boolean CFStringIsSurrogateHighCharacter(UniChar character) {
    return ((character >= 0xD800UL) && (character <= 0xDBFFUL) ? true : false);
}

CF_INLINE Boolean CFStringIsSurrogateLowCharacter(UniChar character) {
    return ((character >= 0xDC00UL) && (character <= 0xDFFFUL) ? true : false);
}

CF_INLINE UTF32Char CFStringGetLongCharacterForSurrogatePair(UniChar surrogateHigh, UniChar surrogateLow) {
    return ((surrogateHigh - 0xD800UL) << 10) + (surrogateLow - 0xDC00UL) + 0x0010000UL;
}
#endif

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
 * the shape function a single run, CoreText might consider it to consist out of
 * multiple runs. Because of this, we have an interface around the CoreText array of
 * runs that works like iterating a single array, which makes our job in the shape
 * function easier.
 */

struct RunIterator
{
  CTLineRef line;
  CFStringRef cstr;
  CFArrayRef runs;
  CFIndex glyph_count;

  CFIndex total_ct_i;
  CFIndex ct_i;

  CFIndex *chr_idx_lut;

  int current_run_number;
  CTRunRef current_run;
  CFIndex *current_indices;
  const CGGlyph *current_cgglyphs;
  CGGlyph *current_cgglyphs_buffer;
  CTRunStatus current_run_status;
};

static void
run_iterator_free_current_run (struct RunIterator *iter)
{
  iter->current_run_number = -1;
  iter->current_run = NULL;
  iter->current_cgglyphs = NULL;
  if (iter->current_cgglyphs_buffer)
    free (iter->current_cgglyphs_buffer);
  iter->current_cgglyphs_buffer = NULL;
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
  ct_glyph_count = CTRunGetGlyphCount (iter->current_run);

  iter->current_run_status = CTRunGetStatus (iter->current_run);
  iter->current_cgglyphs = CTRunGetGlyphsPtr (iter->current_run);
  if (!iter->current_cgglyphs)
    {
      iter->current_cgglyphs_buffer = (CGGlyph *)malloc (sizeof (CGGlyph) * ct_glyph_count);
      CTRunGetGlyphs (iter->current_run, CFRangeMake (0, ct_glyph_count),
                      iter->current_cgglyphs_buffer);
      iter->current_cgglyphs = iter->current_cgglyphs_buffer;
    }

  iter->current_indices = malloc (sizeof (CFIndex) * ct_glyph_count);
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

/* This function generates a lookup table to match string indices of glyphs to
 * actual unicode character indices. This also takes unicode characters into
 * account that are encoded using 2 UTF16 code points in CFStrings. We use the
 * unicode character index to match up with the unicode characters in the UTF8
 * string provided by Pango.
 */
static CFIndex *
run_iterator_get_chr_idx_lut (CFStringRef cstr)
{
  CFIndex cstr_length = CFStringGetLength (cstr);
  CFIndex *chr_idx_lut = malloc (sizeof (CFIndex) * cstr_length);
  CFIndex i;
  CFIndex current_value = 0;

  for (i = 0; i < cstr_length; i++)
    {
      chr_idx_lut[i] = current_value;

      if (CFStringIsSurrogateHighCharacter (CFStringGetCharacterAtIndex (cstr, i)) &&
          i + 1 < cstr_length &&
          CFStringIsSurrogateLowCharacter (CFStringGetCharacterAtIndex (cstr, i + 1)))
        continue;

      current_value++;
    }

  return chr_idx_lut;
}

/* These functions are commented out to silence the compiler, but
 * kept around because they might be of use when fixing the more
 * intricate issues noted in the comment in the function
 * pangocoretext_shape() below.
 */
#if 0
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
#endif

static gunichar
run_iterator_get_character (struct RunIterator *iter)
{
  UniChar ch = CFStringGetCharacterAtIndex (iter->cstr, iter->current_indices[iter->ct_i]);

  if (CFStringIsSurrogateHighCharacter (ch) &&
      iter->current_indices[iter->ct_i] + 1 < CFStringGetLength (iter->cstr))
    {
      UniChar ch2 = CFStringGetCharacterAtIndex (iter->cstr, iter->current_indices[iter->ct_i]+1);

      if (CFStringIsSurrogateLowCharacter (ch2))
        return CFStringGetLongCharacterForSurrogatePair (ch, ch2);
    }

  return ch;
}

static CGGlyph
run_iterator_get_cgglyph (struct RunIterator *iter)
{
  return iter->current_cgglyphs[iter->ct_i];
}

static CFIndex
run_iterator_get_index (struct RunIterator *iter)
{
  return iter->chr_idx_lut[iter->current_indices[iter->ct_i]];
}

static gboolean
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
  iter->chr_idx_lut = NULL;
  iter->current_cgglyphs = NULL;
  iter->current_cgglyphs_buffer = NULL;

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

  if (!iter->cstr)
    /* Creating a CFString can fail if the input string does not
     * adhere to the specified encoding (i.e. it contains invalid UTF8).
     */
    return FALSE;

  attstr = CFAttributedStringCreate (kCFAllocatorDefault,
                                     iter->cstr,
                                     attributes);

  iter->line = CTLineCreateWithAttributedString (attstr);
  iter->runs = CTLineGetGlyphRuns (iter->line);

  CFRelease (attstr);
  CFRelease (attributes);

  iter->chr_idx_lut = run_iterator_get_chr_idx_lut (iter->cstr);

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

  return TRUE;
}

static void
run_iterator_free (struct RunIterator *iter)
{
  run_iterator_free_current_run (iter);

  free (iter->chr_idx_lut);

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

  if (!run_iterator_create (&riter, text, length, ctfont))
    return NULL;

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


void
_pango_core_text_shape (PangoFont           *font,
			const char          *text,
			gint                 length,
			const PangoAnalysis *analysis,
			PangoGlyphString    *glyphs,
			const char          *paragraph_text G_GNUC_UNUSED,
			unsigned int         paragraph_length G_GNUC_UNUSED)
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
   *   # We currently don't bother about LTR, Pango core appears to fix this up for us.
   *     (Even when we cared warnings were generated that strings were in the wrong
   *     order, this should be investigated).
   *   # When CoreText generates two glyphs for one character, only one is stored.
   *     This breaks the example strings for e.g. Georgian and Gothic.
   */

  glyph_list = create_core_text_glyph_list (text, length,
                                            pango_core_text_font_get_ctfont (cfont));
  if (!glyph_list)
    return;

  /* Set up for translation of the glyph list to a PangoGlyphString. */
  n_chars = pango_utf8_strlen (text, length);
  pango_glyph_string_set_size (glyphs, n_chars);

  glyph_iter = glyph_list;

  coverage = pango_font_get_coverage (PANGO_FONT (cfont),
                                      analysis->language);

  /* gs_i is the index into the Pango glyph string. gi is the iterator into
   * the (CoreText) glyph list, gi->index is the index into the CFString.
   * In matching, we want gs_i and gi->index to match up.
   */
  for (gs_prev_i = -1, gs_i = 0, p = text; gs_i < n_chars;
       gs_prev_i = gs_i, gs_i++, p = g_utf8_next_char (p))
    {
      struct GlyphInfo *gi = glyph_iter != NULL ? glyph_iter->data : NULL;

      if (gi == NULL || gi->index > gs_i)
        {
          /* The glyph string is behind, insert an empty glyph to catch
           * up with the CoreText glyph list. This occurs for instance when
           * CoreText inserts a ligature that covers two characters.
           */
          set_glyph (font, glyphs, gs_i, p - text, PANGO_GLYPH_EMPTY);
          continue;
        }
      else if (gi->index < gs_i)
        {
          /* The CoreText glyph list is behind, fast forward the iterator
           * to catch up. This can happen when CoreText emits two glyphs
           * for once character, which is (as noted in the FIXME) above
           * not handled by us yet.
           */
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

  if (analysis->level & 1)
    pango_glyph_string_reverse_range (glyphs, 0, glyphs->num_glyphs);
}
