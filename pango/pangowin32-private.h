/* Pango
 * pangowin32-private.h:
 *
 * Copyright (C) 1999 Red Hat Software
 * Copyright (C) 2000 Tor Lillqvist
 * Copyright (C) 2001 Alexander Larsson
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

#ifndef __PANGOWIN32_PRIVATE_H__
#define __PANGOWIN32_PRIVATE_H__

#define DEBUGGING 0

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

#include "pango-modules.h"
#include "pangowin32.h"

#ifndef FS_VIETNAMESE
#define FS_VIETNAMESE 0x100
#endif

typedef struct _PangoWin32Font PangoWin32Font;
typedef struct _PangoWin32Face PangoWin32Face;
typedef struct _PangoWin32GlyphInfo PangoWin32GlyphInfo;

struct _PangoWin32Font
{
  PangoFont font;

  LOGFONT logfont;
  int size;

  PangoFontMap *fontmap;

  /* Written by pango_win32_get_hfont: */
  HFONT hfont;
  gint   tm_ascent;     
  gint   tm_descent;
  gint   tm_overhang;

  PangoWin32Face *entry;

  /* If TRUE, font is in cache of recently unused fonts and not otherwise
   * in use.
   */
  gboolean in_cache;
  GHashTable *glyph_info;
};

struct _PangoWin32Face
{
  PangoFontFace parent_instance;

  LOGFONT logfont;
  PangoFontDescription *description;
  PangoCoverage *coverage;

  char *face_name;

  gpointer unicode_table;

  GSList *cached_fonts;
};

struct _PangoWin32GlyphInfo
{
  PangoRectangle logical_rect;
  PangoRectangle ink_rect;
};


PangoWin32Font *pango_win32_font_new                (PangoFontMap    	 *fontmap,
						     const LOGFONT     	 *lfp,
						     int             	  size);
PangoMap *      pango_win32_get_shaper_map          (PangoLanguage   	 *lang);
void            pango_win32_make_matching_logfont   (PangoFontMap    	 *fontmap,
						     const LOGFONT       *lfp,
						     int             	  size,
						     LOGFONT             *out);
PangoCoverage * pango_win32_font_entry_get_coverage (PangoWin32Face *face);
void            pango_win32_font_entry_set_coverage (PangoWin32Face *face,
						     PangoCoverage       *coverage);
void            pango_win32_font_entry_remove       (PangoWin32Face *face,
						     PangoFont           *font);

void            pango_win32_fontmap_cache_add       (PangoFontMap    	 *fontmap,
						     PangoWin32Font  	 *xfont);
void            pango_win32_fontmap_cache_remove    (PangoFontMap    	 *fontmap,
						     PangoWin32Font  	 *xfont);

extern HDC pango_win32_hdc;
extern OSVERSIONINFO pango_win32_os_version_info;

#endif /* __PANGOWIN32_PRIVATE_H__ */
