/* Pango
 * pangoft2-private.h:
 *
 * Copyright (C) 1999 Red Hat Software
 * Copyright (C) 2000 Tor Lillqvist
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

#ifndef __PANGOFT2_PRIVATE_H__
#define __PANGOFT2_PRIVATE_H__

#include "pango-modules.h"
#include "pangoft2.h"
#include "mini-xft/MiniXft.h"

/* Debugging... */
/*#define DEBUGGING 1*/

#if defined(DEBUGGING) && DEBUGGING
#ifdef __GNUC__
#define PING(printlist)					\
(g_print ("%s:%d ", __PRETTY_FUNCTION__, __LINE__),	\
 g_print printlist,					\
 g_print ("\n"))
#else
#define PING(printlist)					\
(g_print ("%s:%d ", __FILE__, __LINE__),		\
 g_print printlist,					\
 g_print ("\n"))
#endif
#else  /* !DEBUGGING */
#define PING(printlist)
#endif

#define PANGO_SCALE_26_6 (PANGO_SCALE / (1<<6))
#define PANGO_PIXELS_26_6(d)				\
  (((d) >= 0) ?						\
   ((d) + PANGO_SCALE_26_6 / 2) / PANGO_SCALE_26_6 :	\
   ((d) - PANGO_SCALE_26_6 / 2) / PANGO_SCALE_26_6)
#define PANGO_UNITS_26_6(d) (PANGO_SCALE_26_6 * (d))

typedef struct _PangoFT2Font      PangoFT2Font;
typedef struct _PangoFT2GlyphInfo PangoFT2GlyphInfo;
typedef struct _PangoFT2Face      PangoFT2Face;
typedef struct _PangoFT2Family    PangoFT2Family;


struct _PangoFT2Font
{
  PangoFont font;

  MiniXftPattern *font_pattern;
  FT_Face face;

  int size;

  PangoFontMap *fontmap;
  PangoFontDescription *description;
  
  /* If TRUE, font is in cache of recently unused fonts and not otherwise
   * in use. */
  gboolean in_cache;

  GHashTable *glyph_info;
};

struct _PangoFT2GlyphInfo
{
  PangoRectangle logical_rect;
  PangoRectangle ink_rect;
};

struct _PangoFT2Face
{
  PangoFontFace parent_instance;
  
  PangoFT2Family *family;
  char *style;
};

PangoFT2Font * _pango_ft2_font_new                (PangoFontMap                *font,
						   MiniXftPattern              *pattern);
PangoMap      *_pango_ft2_get_shaper_map          (PangoLanguage     *language);
void           _pango_ft2_font_map_set_coverage   (PangoFontMap                *fontmap,
						   const char                  *name,
						   PangoCoverage               *coverage);
PangoCoverage *_pango_ft2_font_map_get_coverage   (PangoFontMap                *fontmap,
						   const char                  *name);
void           _pango_ft2_face_remove             (PangoFT2Face      *face,
						   PangoFont         *font);
FT_Library     _pango_ft2_font_map_get_library    (PangoFontMap      *fontmap);
void           _pango_ft2_font_map_cache_add      (PangoFontMap      *fontmap,
						   PangoFT2Font      *ft2font);
void           _pango_ft2_font_map_cache_remove    (PangoFontMap      *fontmap,
						   PangoFT2Font      *ft2font);
void           _pango_ft2_font_map_add            (PangoFontMap      *fontmap,
						   PangoFT2Font      *ft2font);
void           _pango_ft2_font_map_remove         (PangoFontMap      *fontmap,
						   PangoFT2Font      *ft2font);
const char    *_pango_ft2_ft_strerror             (FT_Error           error);
PangoFontDescription *_pango_ft2_font_desc_from_pattern (MiniXftPattern *pattern);

#endif /* __PANGOFT2_PRIVATE_H__ */
