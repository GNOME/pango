/* Pango
 * pangowin32-private.h:
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
typedef struct _PangoWin32FontEntry PangoWin32FontEntry;
typedef struct _PangoWin32SubfontInfo PangoWin32SubfontInfo;

struct _PangoWin32Font
{
  PangoFont font;

  LOGFONT *logfonts;
  int n_logfonts;
  int size;

  /* hash table mapping from Unicode subranges to array of PangoWin32Subfont
   * ids, of length n_subfonts
   */
  GHashTable *subfonts_by_subrange;
  
  PangoWin32SubfontInfo **subfonts;

  int n_subfonts;
  int max_subfonts;

  GSList *metrics_by_lang;

  PangoFontMap *fontmap;
  /* If TRUE, font is in cache of recently unused fonts and not otherwise
   * in use.
   */
  gboolean in_cache;
  
  PangoWin32FontEntry *entry;
};

struct _PangoWin32FontEntry
{
  LOGFONT *lfp;
  int n_fonts;
  PangoFontDescription description;
  PangoCoverage *coverage;

  GSList *cached_fonts;
};

PangoWin32Font *pango_win32_font_new                (PangoFontMap    	 *fontmap,
						     const LOGFONT     	 *lfp,
						     int                  n_fonts,
						     int             	  size);
PangoMap *      pango_win32_get_shaper_map          (PangoLanguage   	 *lang);
gboolean	pango_win32_logfont_has_subrange    (PangoFontMap        *fontmap,
						     LOGFONT		 *lfp,
						     PangoWin32UnicodeSubrange subrange);
LOGFONT *       pango_win32_make_matching_logfont   (PangoFontMap    	 *fontmap,
						     LOGFONT             *lfp,
						     int             	  size);
PangoCoverage * pango_win32_font_entry_get_coverage (PangoWin32FontEntry *entry,
						     PangoFont       	 *font,
						     PangoLanguage   	 *lang);
void            pango_win32_font_entry_remove       (PangoWin32FontEntry *entry,
						     PangoFont           *font);

void            pango_win32_fontmap_cache_add       (PangoFontMap    	 *fontmap,
						     PangoWin32Font  	 *xfont);
void            pango_win32_fontmap_cache_remove    (PangoFontMap    	 *fontmap,
						     PangoWin32Font  	 *xfont);

extern HDC pango_win32_hdc;

#endif /* __PANGOWIN32_PRIVATE_H__ */
