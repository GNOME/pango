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

typedef struct _PangoFT2OA PangoFT2OA;
typedef struct _PangoFT2Font PangoFT2Font;
typedef struct _PangoFT2GlyphInfo PangoFT2GlyphInfo;
typedef struct _PangoFT2FontEntry PangoFT2FontEntry;
typedef struct _PangoFT2SubfontInfo PangoFT2SubfontInfo;

struct _PangoFT2OA
{
  FT_Open_Args *open_args;
  FT_Long face_index;
};

struct _PangoFT2Font
{
  PangoFont font;

  /* A PangoFT2Font consists of one or several FT2 fonts (faces) that
   * are assumed to blend visually well, and cover separate parts of
   * the Unicode characters. The FT2 faces are not kept unnecessarily
   * open, thus also we keep both the FT_Open_Args (and face index),
   * and FT_Face.
   */
  PangoFT2OA **oa;
  FT_Face *faces;
  int n_fonts;

  int size;

  GSList *metrics_by_lang;

  PangoFontMap *fontmap;
  /* If TRUE, font is in cache of recently unused fonts and not otherwise
   * in use.
   */
  gboolean in_cache;
  
  PangoFT2FontEntry *entry;

  GHashTable *glyph_info;
};

struct _PangoFT2GlyphInfo
{
  PangoRectangle logical_rect;
  PangoRectangle ink_rect;
};

struct _PangoFT2FontEntry
{
  FT_Open_Args **open_args;
  FT_Long *face_indices;
  int n_fonts;
  PangoFontDescription description;
  PangoCoverage *coverage;

  GSList *cached_fonts;
};

PangoMap      *pango_ft2_get_shaper_map          (const char        *lang);
PangoCoverage *pango_ft2_font_entry_get_coverage (PangoFT2FontEntry *entry,
						  PangoFont         *font,
						  const char        *lang);
void           pango_ft2_font_entry_remove       (PangoFT2FontEntry *entry,
						  PangoFont         *font);
FT_Library    *pango_ft2_fontmap_get_library     (PangoFontMap      *fontmap);
void           pango_ft2_fontmap_cache_add       (PangoFontMap      *fontmap,
						  PangoFT2Font      *ft2font);
void           pango_ft2_fontmap_cache_remove    (PangoFontMap      *fontmap,
						  PangoFT2Font      *ft2font);
const char    *pango_ft2_ft_strerror             (FT_Error           error);

#endif /* __PANGOFT2_PRIVATE_H__ */
