/* Pango
 * pangowin32-private.h:
 *
 * Copyright (C) 1999 Red Hat Software
 * Copyright (C) 2000-2002 Tor Lillqvist
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

/* Define if you want the possibility to get copious debugging output.
 * (You still need to set the PANGO_WIN32_DEBUG environment variable
 * to get it.)
 */
#define PANGO_WIN32_DEBUGGING 1

#ifdef PANGO_WIN32_DEBUGGING
#ifdef __GNUC__
#define PING(printlist)					\
(pango_win32_debug ?					\
 (printf ("%s:%d ", __PRETTY_FUNCTION__, __LINE__),	\
  printf printlist,					\
  printf ("\n")) :					\
 0)
#else
#define PING(printlist)					\
(pango_win32_debug ?					\
 (printf ("%s:%d ", __FILE__, __LINE__),		\
  printf printlist,					\
  printf ("\n")) :					\
 0)
#endif
#else  /* !PANGO_WIN32_DEBUGGING */
#define PING(printlist)
#endif

#include "pango-modules.h"
#include "pangowin32.h"

typedef enum
  {
    PANGO_WIN32_COVERAGE_UNSPEC,
    PANGO_WIN32_COVERAGE_ZH_TW,
    PANGO_WIN32_COVERAGE_ZH_CN,
    PANGO_WIN32_COVERAGE_JA,
    PANGO_WIN32_COVERAGE_KO,
    PANGO_WIN32_COVERAGE_VI,
    PANGO_WIN32_N_COVERAGES
  } PangoWin32CoverageLanguageClass;

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
  gint tm_ascent;     
  gint tm_descent;
  gint tm_overhang;

  PangoWin32Face *win32face;

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
  PangoCoverage *coverages[PANGO_WIN32_N_COVERAGES];
  char *face_name;

  gpointer unicode_table;

  GSList *cached_fonts;
};

struct _PangoWin32GlyphInfo
{
  PangoRectangle logical_rect;
  PangoRectangle ink_rect;
};


/* TrueType defines: */

#define MAKE_TT_TABLE_NAME(c1, c2, c3, c4) \
   (((guint32)c4) << 24 | ((guint32)c3) << 16 | ((guint32)c2) << 8 | ((guint32)c1))

#define CMAP (MAKE_TT_TABLE_NAME('c','m','a','p'))
#define CMAP_HEADER_SIZE 4

#define NAME (MAKE_TT_TABLE_NAME('n','a','m','e'))
#define NAME_HEADER_SIZE 6

#define ENCODING_TABLE_SIZE 8

#define APPLE_UNICODE_PLATFORM_ID 0
#define MACINTOSH_PLATFORM_ID 1
#define ISO_PLATFORM_ID 2
#define MICROSOFT_PLATFORM_ID 3

#define SYMBOL_ENCODING_ID 0
#define UNICODE_ENCODING_ID 1
#define UCS4_ENCODING_ID 10

struct cmap_encoding_subtable
{ /* Must be packed! */
  guint16 platform_id;
  guint16 encoding_id;
  guint32 offset;
};

struct type_4_cmap
{ /* Must be packed! */
  guint16 format;
  guint16 length;
  guint16 language;
  guint16 seg_count_x_2;
  guint16 search_range;
  guint16 entry_selector;
  guint16 range_shift;
  
  guint16 reserved;
  
  guint16 arrays[1];
};

struct name_header
{
  guint16 format_selector;
  guint16 num_records;
  guint16 string_storage_offset;
};

struct name_record
{
  guint16 platform_id;
  guint16 encoding_id;
  guint16 language_id;
  guint16 name_id;
  guint16 string_length;
  guint16 string_offset;
};

PangoWin32Font *pango_win32_font_new                (PangoFontMap   *fontmap,
						     const LOGFONT  *lfp,
						     int             size);
PangoMap *      pango_win32_get_shaper_map          (PangoLanguage  *lang);
void            pango_win32_make_matching_logfont   (PangoFontMap   *fontmap,
						     const LOGFONT  *lfp,
						     int             size,
						     LOGFONT        *out);
PangoCoverage * pango_win32_font_entry_get_coverage (PangoWin32Face *face,
						     PangoLanguage  *lang);
void            pango_win32_font_entry_set_coverage (PangoWin32Face *face,
						     PangoCoverage  *coverage,
						     PangoLanguage  *lang);
void            pango_win32_font_entry_remove       (PangoWin32Face *face,
						     PangoFont      *font);

void            pango_win32_fontmap_cache_add       (PangoFontMap   *fontmap,
						     PangoWin32Font *xfont);
void            pango_win32_fontmap_cache_remove    (PangoFontMap   *fontmap,
						     PangoWin32Font *xfont);

gint		pango_win32_coverage_language_classify (PangoLanguage  *lang);

gboolean	pango_win32_get_name_header	    (HDC                 hdc,
						     struct name_header *header);
gboolean	pango_win32_get_name_record         (HDC                 hdc,
						     gint                i,
						     struct name_record *record);

extern HDC pango_win32_hdc;
extern OSVERSIONINFO pango_win32_os_version_info;
extern gboolean pango_win32_debug;

#endif /* __PANGOWIN32_PRIVATE_H__ */
