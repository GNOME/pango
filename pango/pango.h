/* Pango
 * pango.h:
 *
 * Copyright (C) 1999 Red Hat Software
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

/* The API here is based fairly closely on Microsoft's Uniscript
 * API. Differences here:
 * 
 * - Memory management is more convenient (Pango handles 
 *   all buffer reallocation)
 * - Unicode strings are represented in UTF-8
 * - Representation of fonts and glyphs is abstracted to be
 *   rendering-system dependent.
 */

#ifndef __PANGO_H__
#define __PANGO_H__

#include <glib.h>

#ifdef __cplusplus
extern "C" {
#endif /* __cplusplus */

typedef struct _PangoAnalysis PangoAnalysis;
typedef struct _PangoItem PangoItem;
typedef struct _PangoContext PangoContext;
typedef struct _PangoLangRange PangoLangRange;
typedef struct _PangoLogAttr PangoLogAttr;

typedef struct _PangoFont PangoFont;
typedef struct _PangoFontClass PangoFontClass;

typedef struct _PangoGlyphGeometry PangoGlyphGeometry;
typedef struct _PangoGlyphVisAttr PangoGlyphVisAttr;
typedef struct _PangoGlyphInfo PangoGlyphInfo;
typedef struct _PangoGlyphString PangoGlyphString;

typedef struct _PangoEngineInfo PangoEngineInfo;
typedef struct _PangoEngineRange PangoEngineRange;
typedef struct _PangoEngine PangoEngine;
typedef struct _PangoEngineLang PangoEngineLang;
typedef struct _PangoEngineShape PangoEngineShape;

/* 64'ths of a point - 1/4608 in, 5.51 * 10^-5 in. */
typedef guint32 PangoGlyphUnit;

/* Information about a segment of text with a consistent
 * shaping/language engine and bidirectional level
 */
struct _PangoAnalysis {
  PangoEngineShape *shape_engine;
  PangoEngineLang *lang_engine;
  guint8 level;
};

struct _PangoItem {
  gint offset;
  gint length;
  gint num_chars;
  PangoAnalysis analysis;
};

/* Sort of like a GC - application set information about how
 * to handle scripts
 */

typedef enum {
  PANGO_DIRECTION_LTR,
  PANGO_DIRECTION_RTL,
  PANGO_DIRECTION_TTB
} PangoDirection;

struct _PangoContext {
  gchar *lang;
  gchar *render_type;
  PangoDirection direction;
};

/* Language tagging information
 */
struct _PangoLangRange
{
  gint start;
  gint length;
  gchar *lang;
};

/* Break a string of Unicode characters into segments with
 * consistent shaping/language engine and bidrectional level.
 * Returns a GList of PangoItem's
 */
GList *pango_itemize (PangoContext   *context, 
                         gchar            *text, 
                         gint              length,
			 PangoLangRange *lang_info,
			 gint              n_langs);

/* Logical attributes of a character
 */
struct _PangoLogAttr {
  guint is_break : 1;  /* Break in front of character */
  guint is_white : 1;
  guint is_char_stop : 1;
  guint is_word_stop : 1;
  /* Uniscript has is_invalid  */
};

/* Determine information about cluster/word/line breaks in a string
 * of Unicode text.
 */
void pango_break (gchar           *text, 
		  gint             length, 
		  PangoAnalysis *analysis, 
		  PangoLogAttr  *attrs);

/*
 * FONT OPERATIONS
 */

/* This structure represents a logical font */
struct _PangoFont {
  PangoFontClass *klass;

  /*< private >*/
  gint ref_count;
  GData *data;
};

struct _PangoFontClass {
  void (*destroy) (PangoFont *font);
};
 
void     pango_font_init     (PangoFont      *font);
void     pango_font_ref      (PangoFont      *font);
void     pango_font_unref    (PangoFont      *font);
gpointer pango_font_get_data (PangoFont      *font,
			      gchar          *key);
void     pango_font_set_data (PangoFont      *font,
			      gchar          *key,
			      gpointer        data,
			      GDestroyNotify  destroy_func);

/*
 * GLYPH STORAGE
 */

/* A index of a glyph into a font. Rendering system dependent
 */
typedef guint32 PangoGlyph;

/* Positioning information about a glyph
 */
struct _PangoGlyphGeometry
{
  PangoGlyphUnit width;
  PangoGlyphUnit x_offset;  
  PangoGlyphUnit y_offset;
};

/* Visual attributes of a glyph
 */
struct _PangoGlyphVisAttr
{
  guint is_cluster_start : 1;
};

/* A single glyph 
 */
struct _PangoGlyphInfo
{
  PangoGlyph    glyph;
  PangoGlyphGeometry geometry;
  PangoGlyphVisAttr  attr;
};

/* A string of glyphs with positional information and visual attributes -
 * ready for drawing
 */
struct _PangoGlyphString {
  gint num_glyphs;

  PangoGlyphInfo *glyphs;

  /* This is a memory inefficient way of representing the
   * information here - each value gives the character index
   * of the start of the cluster to which the glyph belongs.
   */
  gint *log_clusters;

  /*< private >*/
  gint space;
};

PangoGlyphString *pango_glyph_string_new      (void);
void              pango_glyph_string_set_size (PangoGlyphString *string,
					       gint              new_len);
void              pango_glyph_string_free     (PangoGlyphString *string);

/* Turn a string of characters into a string of glyphs
 */
void pango_shape (PangoFont        *font,
		  gchar            *text,
		  gint              length,
		  PangoAnalysis    *analysis,
		  PangoGlyphString *glyphs);

/* [ pango_place - subsume into g_script_shape? ] */

GList *pango_reorder_items (GList *logical_items);

/* Take a PangoGlyphString and add justification to fill to a
 * given width
 */
void pango_justify (PangoGlyphString *glyphs,
		    gint              new_line_width,
		    gint              min_kashida_width);

/* For selection/cursor positioning - turn a character position into a
 * X position.
 */
void pango_cp_to_x (gchar            *text,
		    gint              length,
		    PangoAnalysis    *analysis,
		    PangoGlyphString *glyphs,
		    gint              char_pos,
		    gboolean          trailing,
		    gint             *x_pos);


/* For selection/cursor positioning - turn a X position into a
 * character position
 */
void pango_x_to_cp (gchar            *text,
		    gint              length,
		    PangoAnalysis    *analysis,
		    PangoGlyphString *glyphs,
		    gint              x_pos,
		    gint             *char_pos,
		    gint             *trailing);

/* Module API */

#define PANGO_ENGINE_TYPE_LANG "PangoEngineLang"
#define PANGO_ENGINE_TYPE_SHAPE "PangoEngineShape"

#define PANGO_RENDER_TYPE_NONE "PangoRenderNone"

struct _PangoEngineRange 
{
  guint32 start;
  guint32 end;
  gchar *langs;
};

struct _PangoEngineInfo
{
  gchar *id;
  gchar *engine_type;
  gchar *render_type;
  PangoEngineRange *ranges;
  gint n_ranges;
};

struct _PangoEngine
{
  gchar *id;
  gchar *type;
  gint length;
};

struct _PangoEngineLang
{
  PangoEngine engine;
  void (*script_break) (gchar            *text,
			gint             len,
			PangoAnalysis *analysis,
			PangoLogAttr  *attrs);
};

struct _PangoEngineShape {
  PangoEngine engine;
  void (*script_shape) (PangoFont     *font,
			gchar           *text,
			gint             length,
			PangoAnalysis *analysis,
			PangoGlyphString    *glyphs);
};

/* A module should export the following functions */

void         script_engine_list   (PangoEngineInfo **engines,
				   gint             *n_engines);
PangoEngine *script_engine_load   (const gchar      *id);
void         script_engine_unload (PangoEngine      *engine);

#ifdef __cplusplus
}
#endif /* __cplusplus */


#endif /* __PANGO_H__ */
