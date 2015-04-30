/* Pango
 * pangofc-private.h: Private routines and declarations for generic
 *  fontconfig operation
 *
 * Copyright (C) 2003 Red Hat Software
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

#ifndef __PANGOFC_PRIVATE_H__
#define __PANGOFC_PRIVATE_H__

#include <pangofc-fontmap.h>

G_BEGIN_DECLS


#ifndef FC_WEIGHT_DEMILIGHT
#define FC_WEIGHT_DEMILIGHT 55
#define FC_WEIGHT_SEMILIGHT FC_WEIGHT_DEMILIGHT
#endif


typedef struct _PangoFcMetricsInfo  PangoFcMetricsInfo;

struct _PangoFcMetricsInfo
{
  const char       *sample_str;
  PangoFontMetrics *metrics;
};


typedef struct _PangoFcCmapCacheEntry  PangoFcCmapCacheEntry;
typedef struct _PangoFcCmapCache       PangoFcCmapCache;

#define CMAP_CACHE_NUM_ENTRIES 256 /* should be power of two */
#define CMAP_CACHE_MASK (CMAP_CACHE_NUM_ENTRIES - 1)

struct _PangoFcCmapCacheEntry
{
  gunichar   ch;
  PangoGlyph glyph;
};

struct _PangoFcCmapCache
{
  guint ref_count;
  PangoFcCmapCacheEntry entries[CMAP_CACHE_NUM_ENTRIES];
};


#define PANGO_SCALE_26_6 (PANGO_SCALE / (1<<6))
#define PANGO_PIXELS_26_6(d)				\
  (((d) >= 0) ?						\
   ((d) + PANGO_SCALE_26_6 / 2) / PANGO_SCALE_26_6 :	\
   ((d) - PANGO_SCALE_26_6 / 2) / PANGO_SCALE_26_6)
#define PANGO_UNITS_26_6(d)    ((d) * PANGO_SCALE_26_6)
#define PANGO_UNITS_TO_26_6(d) ((d) / PANGO_SCALE_26_6)

void _pango_fc_font_shutdown (PangoFcFont *fcfont);

void           _pango_fc_font_map_remove          (PangoFcFontMap *fcfontmap,
						   PangoFcFont    *fcfont);

PangoCoverage *_pango_fc_font_map_get_coverage    (PangoFcFontMap *fcfontmap,
						   PangoFcFont    *fcfont);
PangoCoverage  *_pango_fc_font_map_fc_to_coverage (FcCharSet      *charset);

PangoFcCmapCache *_pango_fc_font_map_get_cmap_cache (PangoFcFontMap *fcfontmap,
						     PangoFcFont    *fcfont);
void              _pango_fc_cmap_cache_unref (PangoFcCmapCache *cmap_cache);

PangoFcDecoder *_pango_fc_font_get_decoder       (PangoFcFont    *font);
void            _pango_fc_font_set_decoder       (PangoFcFont    *font,
						  PangoFcDecoder *decoder);

PangoFcFontKey *_pango_fc_font_get_font_key      (PangoFcFont    *fcfont);
void            _pango_fc_font_set_font_key      (PangoFcFont    *fcfont,
						  PangoFcFontKey *key);

void            pango_fc_font_get_raw_extents    (PangoFcFont    *font,
						  FT_Int32        load_flags,
						  PangoGlyph      glyph,
						  PangoRectangle *ink_rect,
						  PangoRectangle *logical_rect);

PangoFontMetrics *pango_fc_font_create_base_metrics_for_context (PangoFcFont   *font,
								 PangoContext  *context);

void
_pango_fc_shape (PangoFont           *font,
		 const char          *item_text,
		 unsigned int         item_length,
		 const PangoAnalysis *analysis,
		 PangoGlyphString    *glyphs,
		 const char          *paragraph_text,
		 unsigned int         paragraph_length);

G_END_DECLS

#endif /* __PANGOFC_PRIVATE_H__ */
