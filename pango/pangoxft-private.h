/* Pango
 * pangox-private.h:
 *
 * Copyright (C) 2000 Red Hat Software
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

#ifndef __PANGOXFT_PRIVATE_H__
#define __PANGOXFT_PRIVATE_H__

#include <pangoxft.h>
#include <pango-ot.h>

G_BEGIN_DECLS

typedef struct _PangoXftFont PangoXftFont;
typedef struct _PangoXftCoverageKey PangoXftCoverageKey;

struct _PangoXftFont
{
  PangoFont parent_instance;

  FcPattern *font_pattern;	    /* fully resolved pattern */
  XftFont *xft_font;		    /* created on demand */
  PangoFont *mini_font;		    /* font used to display missing glyphs */
  PangoFontMap *fontmap;	    /* associated map */
  PangoFontDescription *description;

  GSList *metrics_by_lang;

  guint16 mini_width;		    /* metrics for missing glyph drawing */
  guint16 mini_height;
  guint16 mini_pad; 
  
  gboolean in_cache;
};

struct _PangoXftCoverageKey
{
  char *filename;	
  int id;            /* needed to handle TTC files with multiple faces */
};

PangoXftFont * _pango_xft_font_new              (PangoFontMap                *font,
						 FcPattern                  *pattern);
void           _pango_xft_font_map_cache_add    (PangoFontMap                *fontmap,
						 PangoXftFont                *xfont);
void           _pango_xft_font_map_add          (PangoFontMap                *fontmap,
						 PangoXftFont                *xfont);
void           _pango_xft_font_map_remove       (PangoFontMap                *fontmap,
						 PangoXftFont                *xfont);
void           _pango_xft_font_map_set_coverage (PangoFontMap                *fontmap,
						 const PangoXftCoverageKey   *key,
						 PangoCoverage               *coverage);
PangoCoverage *_pango_xft_font_map_get_coverage (PangoFontMap                *fontmap,
						 const PangoXftCoverageKey   *key);
void           _pango_xft_font_map_get_info     (PangoFontMap                *fontmap,
						 Display                    **display,
						 int                         *screen);

PangoFontDescription * _pango_xft_font_desc_from_pattern (FcPattern *pattern,
							  gboolean    include_size);

G_END_DECLS

#endif /* __PANGOXFT_PRIVATE_H__ */
