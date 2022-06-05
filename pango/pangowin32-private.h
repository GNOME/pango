/*
 * Copyright (C) 1999 Red Hat Software
 * Copyright (C) 2000-2002 Tor Lillqvist
 * Copyright (C) 2001 Alexander Larsson
 *
 * SPDX-License-Identifier: LGPL-2.1-or-later
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public
 * License as published by the Free Software Foundation; either
 * version 2.1 of the License, or (at your option) any later version.
 *
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public
 * License along with this library; if not, see <http://www.gnu.org/licenses/>.
 */

#pragma once

/* Define if you want the possibility to get copious debugging output.
 * (You still need to set the PANGO_WIN32_DEBUG environment variable
 * to get it.)
 */
#define PANGO_WIN32_DEBUGGING 1

#ifdef PANGO_WIN32_DEBUGGING
#ifdef __GNUC__
#define PING(printlist)					\
(_pango_win32_debug ?					\
 (g_print ("%s:%d ", __PRETTY_FUNCTION__, __LINE__),	\
  g_print printlist,					\
  g_print ("\n"),					\
  0) :							\
 0)
#else
#define PING(printlist)					\
(_pango_win32_debug ?					\
 (g_print ("%s:%d ", __FILE__, __LINE__),		\
  g_print printlist,					\
  g_print ("\n"),					\
  0) :							\
 0)
#endif
#else  /* !PANGO_WIN32_DEBUGGING */
#define PING(printlist)
#endif

#include "pangowin32.h"
#include "pango-font-private.h"
#include "pango-fontset.h"
#include "pango-fontmap-private.h"

#define PANGO_TYPE_WIN32_FONT_MAP             (_pango_win32_font_map_get_type ())
#define PANGO_WIN32_FONT_MAP(object)          (G_TYPE_CHECK_INSTANCE_CAST ((object), PANGO_TYPE_WIN32_FONT_MAP, PangoWin32FontMap))
#define PANGO_WIN32_IS_FONT_MAP(object)       (G_TYPE_CHECK_INSTANCE_TYPE ((object), PANGO_TYPE_WIN32_FONT_MAP))
#define PANGO_WIN32_FONT_MAP_CLASS(klass)     (G_TYPE_CHECK_CLASS_CAST ((klass), PANGO_TYPE_WIN32_FONT_MAP, PangoWin32FontMapClass))
#define PANGO_IS_WIN32_FONT_MAP_CLASS(klass)  (G_TYPE_CHECK_CLASS_TYPE ((klass), PANGO_TYPE_WIN32_FONT_MAP))
#define PANGO_WIN32_FONT_MAP_GET_CLASS(obj)   (G_TYPE_INSTANCE_GET_CLASS ((obj), PANGO_TYPE_WIN32_FONT_MAP, PangoWin32FontMapClass))

#define PANGO_TYPE_WIN32_FONT            (_pango_win32_font_get_type ())
#define PANGO_WIN32_FONT(object)         (G_TYPE_CHECK_INSTANCE_CAST ((object), PANGO_TYPE_WIN32_FONT, PangoWin32Font))
#define PANGO_WIN32_FONT_CLASS(klass)    (G_TYPE_CHECK_CLASS_CAST ((klass), PANGO_TYPE_WIN32_FONT, PangoWin32FontClass))
#define PANGO_WIN32_IS_FONT(object)      (G_TYPE_CHECK_INSTANCE_TYPE ((object), PANGO_TYPE_WIN32_FONT))
#define PANGO_WIN32_IS_FONT_CLASS(klass) (G_TYPE_CHECK_CLASS_TYPE ((klass), PANGO_TYPE_WIN32_FONT))
#define PANGO_WIN32_FONT_GET_CLASS(obj)  (G_TYPE_INSTANCE_GET_CLASS ((obj), PANGO_TYPE_WIN32_FONT, PangoWin32FontClass))

typedef struct _PangoWin32FontMap      PangoWin32FontMap;
typedef struct _PangoWin32FontMapClass PangoWin32FontMapClass;
typedef struct _PangoWin32Font         PangoWin32Font;
typedef struct _PangoWin32FontClass    PangoWin32FontClass;
typedef struct _PangoWin32Face         PangoWin32Face;
typedef PangoFontFaceClass             PangoWin32FaceClass;
typedef struct _PangoWin32GlyphInfo    PangoWin32GlyphInfo;
typedef struct _PangoWin32MetricsInfo  PangoWin32MetricsInfo;

struct _PangoWin32FontMap
{
  PangoFontMap parent_instance;

  PangoWin32FontCache *font_cache;
  GQueue *freed_fonts;

  /* Map Pango family names to PangoWin32Family structs */
  GHashTable *families;

  /* Map LOGFONTWs (taking into account only the lfFaceName, lfItalic
   * and lfWeight fields) to LOGFONTWs corresponding to actual fonts
   * installed.
   */
  GHashTable *fonts;

  /* keeps track of the system font aliases that we might have */
  GHashTable *aliases;

  /* keeps track of the warned fonts that we might have */
  GHashTable *warned_fonts;

  double resolution;		/* (points / pixel) * PANGO_SCALE */
};

struct _PangoWin32FontMapClass
{
  PangoFontMapClass parent_class;

  PangoFont *(*find_font) (PangoWin32FontMap          *fontmap,
			   PangoContext               *context,
			   PangoWin32Face             *face,
			   const PangoFontDescription *desc);
  GHashTable *aliases;
};

struct _PangoWin32Font
{
  PangoFont font;

  LOGFONTW logfontw;
  int size;

  GSList *metrics_by_lang;

  PangoFontMap *fontmap;

  /* Written by _pango_win32_font_get_hfont: */
  HFONT hfont;

  PangoWin32Face *win32face;

  /* If TRUE, font is in cache of recently unused fonts and not otherwise
   * in use.
   */
  gboolean in_cache;
  GHashTable *glyph_info;
};

struct _PangoWin32FontClass
{
  PangoFontClass parent_class;

  gboolean (*select_font)        (PangoFont *font,
				  HDC        hdc);
  void     (*done_font)          (PangoFont *font);
  double   (*get_metrics_factor) (PangoFont *font);
};

struct _PangoWin32Face
{
  PangoFontFace parent_instance;

  gpointer family;
  LOGFONTW logfontw;
  PangoFontDescription *description;
  PangoCoverage *coverage;
  char *face_name;
  gboolean is_synthetic;

  gboolean has_cmap;
  guint16 cmap_format;
  gpointer cmap;

  GSList *cached_fonts;
};

struct _PangoWin32GlyphInfo
{
  PangoRectangle logical_rect;
  PangoRectangle ink_rect;
};

struct _PangoWin32MetricsInfo
{
  const char *sample_str;
  PangoFontMetrics *metrics;
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

/* All the below structs must be packed! */

struct cmap_encoding_subtable
{
  guint16 platform_id;
  guint16 encoding_id;
  guint32 offset;
};

struct format_4_cmap
{
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

struct format_12_cmap
{
  guint16 format;
  guint16 reserved;
  guint32 length;
  guint32 language;
  guint32 count;

  guint32 groups[1];
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

_PANGO_EXTERN
GType           _pango_win32_font_get_type          (void) G_GNUC_CONST;

_PANGO_EXTERN
void            _pango_win32_make_matching_logfontw (PangoFontMap   *fontmap,
						     const LOGFONTW *lfp,
						     int             size,
						     LOGFONTW       *out);

_PANGO_EXTERN
GType           _pango_win32_font_map_get_type      (void) G_GNUC_CONST;

_PANGO_EXTERN
void            _pango_win32_fontmap_cache_remove   (PangoFontMap   *fontmap,
						     PangoWin32Font *xfont);

_PANGO_EXTERN
HFONT		_pango_win32_font_get_hfont         (PangoFont          *font);

_PANGO_EXTERN
HDC             _pango_win32_get_display_dc                 (void);

_PANGO_EXTERN
gpointer        _pango_win32_copy_cmap (gpointer cmap,
                                        guint16 cmap_format);

extern gboolean _pango_win32_debug;
