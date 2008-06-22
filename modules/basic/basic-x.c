/* Pango
 * basic.c:
 *
 * Copyright (C) 1999 Red Hat Software
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
#include "pango-engine.h"
#include "pango-utils.h"

#undef PANGO_DISABLE_DEPRECATED
#include "pangox.h"

/* No extra fields needed */
typedef PangoEngineShape      BasicEngineX;
typedef PangoEngineShapeClass BasicEngineXClass ;

typedef struct _CharRange CharRange;
typedef struct _Charset Charset;
typedef struct _CharsetOrdering CharsetOrdering;
typedef struct _CharCache CharCache;
typedef struct _CharCachePointer CharCachePointer;
typedef struct _MaskTable MaskTable;

typedef PangoGlyph (*ConvFunc) (CharCache   *cache,
				GIConv       cd,
				const gchar *input);

#define MAX_CHARSETS 32

#define SCRIPT_ENGINE_NAME "BasicScriptEngineX"

struct _Charset
{
  int   index;
  const char *id;
  const char *x_charset;
  ConvFunc conv_func;
};

struct _CharsetOrdering
{
  const char *langs;
  char charsets[MAX_CHARSETS];
};

struct _CharRange
{
  guint16 start;
  guint16 end;
  guint16 charsets;
};

struct _MaskTable
{
  int n_subfonts;

  PangoXSubfont *subfonts;
  Charset **charsets;
};

struct _CharCache
{
  guint ref_count;
  CharsetOrdering *ordering;
  MaskTable *mask_tables[256];
  GIConv converters[MAX_CHARSETS];
  PangoCoverage *coverage;
};

struct _CharCachePointer
{
  PangoLanguage *lang;
  CharCache *cache;
};

static PangoGlyph conv_8bit (CharCache  *cache,
			     GIConv      cd,
			     const char *input);
static PangoGlyph conv_eucjp (CharCache  *cache,
			      GIConv      cd,
			      const char *input);
static PangoGlyph conv_16bit (CharCache  *cache,
			      GIConv      cd,
			      const char *input);
static PangoGlyph conv_ucs4 (CharCache  *cache,
			     GIConv      cd,
			     const char *input);
static PangoGlyph conv_16bit_MSB_on (CharCache  *cache,
			      GIConv      cd,
			      const char *input);
static PangoGlyph conv_gb18030_1 (CharCache  *cache,
			      GIConv      cd,
			      const char *input);
static PangoGlyph conv_euctw (CharCache  *cache,
			      GIConv      cd,
			      const char *input);

#include "tables-big.i"

static PangoEngineScriptInfo basic_scripts[] = {
  { PANGO_SCRIPT_COMMON,  "" },
};

static PangoEngineInfo script_engines[] = {
  {
    SCRIPT_ENGINE_NAME,
    PANGO_ENGINE_TYPE_SHAPE,
    PANGO_RENDER_TYPE_X,
    basic_scripts, G_N_ELEMENTS(basic_scripts)
  }
};

/*
 * X window system script engine portion
 */

/* Structure of our cache:
 *
 * PangoFont => CharCachePointer  ===\
 *                    |               \
 *              CharCachePointer  ======> CharCache => CharsetOrdering
 *                    |                       |======> MaskTable[0]    => {subfonts,charset}[n_subfonts],
 *                    |                       |======> MaskTable[1]    => {subfonts,charset}[n_subfonts],
 *                    |                       \======> MaskTable[...]  => {subfonts,charset}[n_subfonts]
 *                    |
 *              CharCachePointer  ======> CharCache => CharsetOrdering
 *                                            |======> MaskTable[0]    => {subfonts,charset}[n_subfonts],
 *                                            |======> MaskTable[1]    => {subfonts,charset}[n_subfonts],
 *                                            \======> MaskTable[...]  => {subfonts,charset}[n_subfonts]
 *
 * A CharCache structure caches the lookup of what subfonts can be used for what characters for a pair of a Font
 * and CharsetOrdering. Multiple language tags can share the same CharsetOrdering - the list of CharCachePointer
 * structures that is attached to the font as object data provides lookups from language tag to charcache.
 */
static CharCache *
char_cache_new (CharsetOrdering *ordering)
{
  CharCache *result;
  int i;

  result = g_new0 (CharCache, 1);

  result->ref_count = 1;
  result->ordering = ordering;
  for (i=0; i<MAX_CHARSETS; i++)
    result->converters[i] = (GIConv)-1;

  return result;
}

static void
char_cache_free (CharCache *cache)
{
  int i;

  for (i=0; i<256; i++)
    if (cache->mask_tables[i])
      {
	g_free (cache->mask_tables[i]->subfonts);
	g_free (cache->mask_tables[i]->charsets);

	g_free (cache->mask_tables[i]);
      }

  for (i=0; i<MAX_CHARSETS; i++)
    if (cache->converters[i] != (GIConv)-1)
      g_iconv_close (cache->converters[i]);

  g_free (cache);
}

static PangoGlyph
find_char (CharCache *cache, PangoFont *font, gunichar wc, const char *input)
{
  int mask_index;
  MaskTable *mask_table;
  int i;

  switch (wc)
    {
    case '\n':
    case '\r':
    case 0x2028: /* Line separator */
    case 0x2029: /* Paragraph separator */
      return PANGO_GET_UNKNOWN_GLYPH (wc);
      break;
    }

  if (wc >= G_N_ELEMENTS (char_masks))
    mask_index = 0;
  else
    mask_index = char_masks[wc];

  if (cache->mask_tables[mask_index])
    mask_table = cache->mask_tables[mask_index];
  else
    {
      const char *charset_names[G_N_ELEMENTS(charsets)];
      Charset *charsets_map[G_N_ELEMENTS(charsets)];
      guint mask;
      int n_charsets = 0;
      int *subfont_charsets;

      mask_table = g_new (MaskTable, 1);

      mask = char_mask_map[mask_index] | ENC_ISO_10646;

      /* Find the character sets that are included in this mask
       */

      for (i=0; i<(int)G_N_ELEMENTS(charsets); i++)
	{
	  int charset_index = cache->ordering->charsets[i];

	  if (mask & (1 << charset_index))
	    {
	      charset_names[n_charsets] = charsets[charset_index].x_charset;
	      charsets_map[n_charsets] = &charsets[charset_index];

	      n_charsets++;
	    }
	}

      mask_table->n_subfonts = pango_x_list_subfonts (font, (char**)charset_names, n_charsets, &mask_table->subfonts, &subfont_charsets);

      mask_table->charsets = g_new (Charset *, mask_table->n_subfonts);
      for (i=0; i<mask_table->n_subfonts; i++)
	mask_table->charsets[i] = charsets_map[subfont_charsets[i]];

      g_free (subfont_charsets);

      cache->mask_tables[mask_index] = mask_table;
    }

  for (i=0; i < mask_table->n_subfonts; i++)
    {
      PangoGlyph index;
      PangoGlyph glyph;
      Charset *charset;

      charset = mask_table->charsets[i];
      if (charset)
	{
	  GIConv cd = cache->converters[charset->index];

	  if (charset->id && cd == (GIConv)-1)
	    {
	      cd = g_iconv_open (charset->id, "UTF-8");
	      if (cd == (GIConv)-1)
		{
		  g_warning ("Could not load converter from %s to UTF-8", charset->id);
		  mask_table->charsets[i] = NULL;
		  continue;
		}

	      cache->converters[charset->index] = cd;
	    }

	  index = (*charset->conv_func) (cache, cd, input);
	  glyph = PANGO_X_MAKE_GLYPH (mask_table->subfonts[i], index);

	  if (pango_x_has_glyph (font, glyph))
	    return glyph;
	}
    }

  return 0;
}

static void
set_glyph (PangoFont *font, PangoGlyphString *glyphs, int i, int offset, PangoGlyph glyph)
{
  PangoRectangle logical_rect;

  glyphs->glyphs[i].glyph = glyph;

  glyphs->glyphs[i].geometry.x_offset = 0;
  glyphs->glyphs[i].geometry.y_offset = 0;

  glyphs->log_clusters[i] = offset;

  pango_font_get_glyph_extents (font, glyphs->glyphs[i].glyph, NULL, &logical_rect);
  glyphs->glyphs[i].geometry.width = logical_rect.width;
}

static PangoGlyph
conv_8bit (CharCache  *cache,
	   GIConv      cd,
	   const char *input)
{
  char outbuf;

  const char *inptr = input;
  size_t inbytesleft;
  char *outptr = &outbuf;
  size_t outbytesleft = 1;

  inbytesleft = g_utf8_next_char (input) - input;

  g_iconv (cd, (char **)&inptr, &inbytesleft, &outptr, &outbytesleft);

  return (guchar)outbuf;
}

static PangoGlyph
conv_eucjp (CharCache  *cache,
	    GIConv      cd,
	    const char *input)
{
  char outbuf[4];

  const char *inptr = input;
  size_t inbytesleft;
  char *outptr = outbuf;
  size_t outbytesleft = 4;

  inbytesleft = g_utf8_next_char (input) - input;

  g_iconv (cd, (char **)&inptr, &inbytesleft, &outptr, &outbytesleft);

  if ((guchar)outbuf[0] < 128)
    return outbuf[0];
  else if ((guchar)outbuf[0] == 0x8e && outbytesleft == 2)
    return ((guchar)outbuf[1]);
  else if ((guchar)outbuf[0] == 0x8f && outbytesleft == 1)
    return ((guchar)outbuf[1] & 0x7f) * 256 + ((guchar)outbuf[2] & 0x7f);
  else
    return ((guchar)outbuf[0] & 0x7f) * 256 + ((guchar)outbuf[1] & 0x7f);
}

static PangoGlyph
conv_16bit (CharCache  *cache,
	    GIConv      cd,
	    const char *input)
{
  char outbuf[2];

  const char *inptr = input;
  size_t inbytesleft;
  char *outptr = outbuf;
  size_t outbytesleft = 2;

  inbytesleft = g_utf8_next_char (input) - input;

  g_iconv (cd, (char **)&inptr, &inbytesleft, &outptr, &outbytesleft);

  if ((guchar)outbuf[0] < 128)
    return outbuf[0];
  else
    return ((guchar)outbuf[0] & 0x7f) * 256 + ((guchar)outbuf[1] & 0x7f);
}

static PangoGlyph
conv_16bit_MSB_on (CharCache  *cache,
		   GIConv      cd,
		   const char *input)
{
  char outbuf[2];

  const char *inptr = input;
  size_t inbytesleft;
  char *outptr = outbuf;
  size_t outbytesleft = 2;

  inbytesleft = g_utf8_next_char (input) - input;

  g_iconv (cd, (char **)&inptr, &inbytesleft, &outptr, &outbytesleft);

  if ((guchar)outbuf[0] < 128)
    return outbuf[0];
  else
    return (guchar)outbuf[0] * 256 + (guchar)outbuf[1];
}

static PangoGlyph
conv_gb18030_1 (CharCache  *cache,
		GIConv      cd,
		const char *input)
{
  char outbuf[4];

  const char *inptr = input;
  size_t inbytesleft;
  char *outptr = outbuf;
  size_t outbytesleft = 4;


  inbytesleft = g_utf8_next_char (input) - input;

  g_iconv (cd, (char **)&inptr, &inbytesleft, &outptr, &outbytesleft);

  if ((guchar)outbuf[0] < 128)
    return outbuf[0];
  else
    return  12600 * ((guchar)outbuf[0] - 0x81) + 1260 * ((guchar)outbuf[1] - 0x30) + 10 * ((guchar)outbuf[2] - 0x81) + ((guchar)outbuf[3] - 0x30);
}

static PangoGlyph
conv_euctw (CharCache  *cache,
	    GIConv      cd,
	    const char *input)
{
  char outbuf[4];

  const char *inptr = input;
  size_t inbytesleft;
  char *outptr = outbuf;
  size_t outbytesleft = 4;

  inbytesleft = g_utf8_next_char (input) - input;

  g_iconv (cd, (char **)&inptr, &inbytesleft, &outptr, &outbytesleft);

  /* The first two bytes determine which page of CNS to use; we
   * get this information from tables-big.i, so ignore them
   */
  if ((guchar)outbuf[0] < 128)
    return outbuf[0];
  else
    return ((guchar)outbuf[2] & 0x7f) * 256 + ((guchar)outbuf[3] & 0x7f);
}

static PangoGlyph
conv_ucs4 (CharCache  *cache,
	   GIConv      cd,
	   const char *input)
{
  return g_utf8_get_char (input);
}

static void
swap_range (PangoGlyphString *glyphs, int start, int end)
{
  int i, j;

  for (i = start, j = end - 1; i < j; i++, j--)
    {
      PangoGlyphInfo glyph_info;
      gint log_cluster;

      glyph_info = glyphs->glyphs[i];
      glyphs->glyphs[i] = glyphs->glyphs[j];
      glyphs->glyphs[j] = glyph_info;

      log_cluster = glyphs->log_clusters[i];
      glyphs->log_clusters[i] = glyphs->log_clusters[j];
      glyphs->log_clusters[j] = log_cluster;
    }
}

static void
char_caches_free (GSList *caches)
{
  GSList *tmp_list = caches;
  while (tmp_list)
    {
      CharCachePointer *pointer = tmp_list->data;

      pointer->cache->ref_count--;
      if (pointer->cache->ref_count == 0)
	char_cache_free (pointer->cache);
      g_free (pointer);

      tmp_list = tmp_list->next;
    }
  g_slist_free (caches);
}

static CharsetOrdering *
ordering_for_lang (PangoLanguage *lang)
{
  int i;

  for (i = 0; i < (int)G_N_ELEMENTS (charset_orderings) - 1; i++)
    {
      if (pango_language_matches (lang, charset_orderings[i].langs))
	return &charset_orderings[i];
    }

  return &charset_orderings[i];
}

static CharCache *
get_char_cache (PangoFont     *font,
		PangoLanguage *lang)
{
  GQuark cache_id = g_quark_from_string ("basic-char-cache");
  CharCache *cache = NULL;
  CharCachePointer *pointer;
  CharsetOrdering *ordering;
  GSList *caches;
  GSList *tmp_list;

  caches = g_object_get_qdata (G_OBJECT (font), cache_id);
  tmp_list = caches;
  while (tmp_list)
    {
      pointer = tmp_list->data;
      if (pointer->lang == lang)
	return pointer->cache;

      tmp_list = tmp_list->next;
    }

  ordering = ordering_for_lang (lang);

  tmp_list = caches;
  while (tmp_list)
    {
      pointer = tmp_list->data;
      if (pointer->cache->ordering == ordering)
	{
	  cache = pointer->cache;
	  break;
	}

      tmp_list = tmp_list->next;
    }

  if (!cache)
    cache = char_cache_new (ordering);
  else
    cache->ref_count++;

  pointer = g_new (CharCachePointer, 1);
  pointer->lang = lang;
  pointer->cache = cache;

  caches = g_slist_prepend (caches, pointer);

  g_object_steal_qdata (G_OBJECT (font), cache_id);
  g_object_set_qdata_full (G_OBJECT (font), cache_id,
			   caches, (GDestroyNotify)char_caches_free);

  return cache;
}

static void
basic_engine_shape (PangoEngineShape *engine,
		    PangoFont        *font,
		    const char       *text,
		    gint              length,
		    const PangoAnalysis *analysis,
		    PangoGlyphString *glyphs)
{
  int n_chars;
  int i;
  const char *p;

  CharCache *cache;

  g_return_if_fail (font != NULL);
  g_return_if_fail (text != NULL);
  g_return_if_fail (length >= 0);
  g_return_if_fail (analysis != NULL);

  cache = get_char_cache (font, analysis->language);

  n_chars = g_utf8_strlen (text, length);
  pango_glyph_string_set_size (glyphs, n_chars);

  p = text;
  for (i=0; i < n_chars; i++)
    {
      gunichar wc;
      gunichar mirrored_ch;
      PangoGlyph index;
      char buf[6];
      const char *input;

      wc = g_utf8_get_char (p);

      input = p;
      if (analysis->level % 2)
	if (pango_get_mirror_char (wc, &mirrored_ch))
	  {
	    wc = mirrored_ch;

	    g_unichar_to_utf8 (wc, buf);
	    input = buf;
	  }

      if (wc == 0xa0)	/* non-break-space */
	{
	  wc = 0x20;

	  g_unichar_to_utf8 (wc, buf);
	  input = buf;
	}

      if (pango_is_zero_width (wc))
	{
	  set_glyph (font, glyphs, i, p - text, PANGO_GLYPH_EMPTY);
	}
      else
	{
	  index = find_char (cache, font, wc, input);
	  if (index)
	    {
	      set_glyph (font, glyphs, i, p - text, index);

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
	  else
	    set_glyph (font, glyphs, i, p - text, PANGO_GET_UNKNOWN_GLYPH (wc));
	}

      p = g_utf8_next_char (p);
    }

  /* Simple bidi support... may have separate modules later */

  if (analysis->level % 2)
    {
      int start, end;

      /* Swap all glyphs */
      swap_range (glyphs, 0, n_chars);

      /* Now reorder glyphs within each cluster back to LTR */
      for (start=0; start<n_chars;)
	{
	  end = start;
	  while (end < n_chars &&
		 glyphs->log_clusters[end] == glyphs->log_clusters[start])
	    end++;

	  swap_range (glyphs, start, end);
	  start = end;
	}
    }
}

static PangoCoverageLevel
basic_engine_covers (PangoEngineShape *engine,
		     PangoFont        *font,
		     PangoLanguage    *lang,
		     gunichar          wc)
{
  CharCache *cache = get_char_cache (font, lang);
  char buf[6];

  g_unichar_to_utf8 (wc, buf);

  return find_char (cache, font, wc, buf) ? PANGO_COVERAGE_EXACT : PANGO_COVERAGE_NONE;
}

static void
basic_engine_x_class_init (PangoEngineShapeClass *class)
{
  class->covers = basic_engine_covers;
  class->script_shape = basic_engine_shape;
}

PANGO_ENGINE_SHAPE_DEFINE_TYPE (BasicEngineX, basic_engine_x,
				basic_engine_x_class_init, NULL)

void
PANGO_MODULE_ENTRY(init) (GTypeModule *module)
{
  basic_engine_x_register_type (module);
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
    return g_object_new (basic_engine_x_type, NULL);
  else
    return NULL;
}
