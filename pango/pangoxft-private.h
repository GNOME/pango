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

#include <pangoxft.h>

#ifndef __PANGOXFT_PRIVATE_H__
#define __PANGOXFT_PRIVATE_H__

typedef struct _PangoXftFont PangoXftFont;

struct _PangoXftFont
{
  PangoFont parent_instance;
  
  XftFont *xft_font;
  PangoFont *mini_font;
  PangoFontMap *fontmap;
  PangoFontDescription *description;

  guint16 mini_width;
  guint16 mini_height;
  guint16 mini_pad; 
  
  gboolean in_cache;
};

PangoXftFont * _pango_xft_font_new              (PangoFontMap                *font,
						 const PangoFontDescription  *description,
						 XftFont                     *xft_font);
void           _pango_xft_font_map_cache_add    (PangoFontMap                *fontmap,
						 PangoXftFont                *xfont);
void           _pango_xft_font_map_add          (PangoFontMap                *fontmap,
						 PangoXftFont                *xfont);
void           _pango_xft_font_map_remove       (PangoFontMap                *fontmap,
						 PangoXftFont                *xfont);
void           _pango_xft_font_map_set_coverage (PangoFontMap                *fontmap,
						 const char                  *name,
						 PangoCoverage               *coverage);
PangoCoverage *_pango_xft_font_map_get_coverage (PangoFontMap                *fontmap,
						 const char                  *name);
void           _pango_xft_font_map_get_info     (PangoFontMap                *fontmap,
						 Display                    **display,
						 int                         *screen);

#endif /* __PANGOXFT_PRIVATE_H__ */
