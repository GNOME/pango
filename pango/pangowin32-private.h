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

G_BEGIN_DECLS

#define PANGO_TYPE_WIN32_FONT_MAP             (pango_win32_font_map_get_type ())
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
typedef struct _PangoWin32DWriteItems  PangoWin32DWriteItems;
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
  guint serial;

  /* Map Pango family names to PangoWin32Family structs */
  GHashTable *families;
  gpointer *families_list;

  /* Map LOGFONTWs (taking into account only the lfFaceName, lfItalic
   * and lfWeight fields) corresponding to actual fonts to IDWriteFonts
   * (or NULL if not loaded yet).
   */
  GHashTable *fonts;

  /* keeps track of the system font aliases that we might have */
  GHashTable *aliases;

  double dpi;		/* (points / pixel) * PANGO_SCALE */

  /* for loading custom fonts on Windows 10+ */
  gpointer font_set_builder1; /* IDWriteFontSetBuilder1 */
  gpointer font_set_builder; /* IDWriteFontSetBuilder */
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
  char *variations;

  GSList *metrics_by_lang;

  PangoFontMap *fontmap;

  /* Written by _pango_win32_font_get_hfont: */
  HFONT hfont;  

  PangoWin32Face *win32face;

  GHashTable *glyph_info;

  /* whether the font supports hinting */
  gboolean is_hinted;
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
  gpointer dwrite_font;
  PangoFontDescription *description;
  PangoCoverage *coverage;
  char *face_name;
  gboolean is_synthetic;

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

_PANGO_EXTERN
GType           _pango_win32_font_get_type          (void) G_GNUC_CONST;

_PANGO_EXTERN
void            _pango_win32_make_matching_logfontw (PangoFontMap   *fontmap,
						     const LOGFONTW *lfp,
						     int             size,
						     LOGFONTW       *out);

_PANGO_EXTERN
GType           pango_win32_font_map_get_type      (void) G_GNUC_CONST;

_PANGO_EXTERN
HFONT		_pango_win32_font_get_hfont         (PangoFont          *font);

_PANGO_EXTERN
HDC             _pango_win32_get_display_dc                 (void);

_PANGO_EXTERN
gboolean        pango_win32_dwrite_font_check_is_hinted       (PangoWin32Font    *font);

_PANGO_EXTERN
void           *pango_win32_font_get_dwrite_font_face         (PangoWin32Font    *font);

extern gboolean _pango_win32_debug;

void                   pango_win32_insert_font                (PangoWin32FontMap     *win32fontmap,
                                                               LOGFONTW              *lfp,
                                                               gpointer               dwrite_font,
                                                               gboolean               is_synthetic);

PangoWin32DWriteItems *pango_win32_init_direct_write          (void);

PangoWin32DWriteItems *pango_win32_get_direct_write_items     (void);

void                   pango_win32_dwrite_font_map_populate   (PangoWin32FontMap     *map);

void                   pango_win32_dwrite_items_destroy       (PangoWin32DWriteItems *items);

gboolean               pango_win32_dwrite_font_is_monospace   (gpointer               dwrite_font,
                                                               gboolean              *is_monospace);

_PANGO_EXTERN
void                   pango_win32_dwrite_font_face_release   (gpointer               dwrite_font_face);

gpointer               pango_win32_logfontw_get_dwrite_font   (LOGFONTW              *logfontw);

PangoFontDescription *
pango_win32_font_description_from_logfontw_dwrite             (const LOGFONTW        *logfontw);

hb_face_t *
pango_win32_font_create_hb_face_dwrite                        (PangoWin32Font        *font);

PangoFontDescription *
pango_win32_font_description_from_dwrite_font                 (void                  *dwrite_font);

/* This needs to be called from PangoCairo as well */
_PANGO_EXTERN
void                  pango_win32_font_map_cache_clear        (PangoFontMap          *font_map);

gboolean              pango_win32_dwrite_add_font_file        (PangoFontMap          *font_map,
                                                               const char            *font_file_path,
                                                               GError               **error);

G_END_DECLS

#endif /* __PANGOWIN32_PRIVATE_H__ */
