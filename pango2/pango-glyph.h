/*
 * Copyright (C) 2000 Red Hat Software
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

#include <pango2/pango-types.h>
#include <pango2/pango-item.h>
#include <pango2/pango-break.h>

G_BEGIN_DECLS

typedef struct _Pango2GlyphGeometry Pango2GlyphGeometry;
typedef struct _Pango2GlyphVisAttr Pango2GlyphVisAttr;
typedef struct _Pango2GlyphInfo Pango2GlyphInfo;
typedef struct _Pango2GlyphString Pango2GlyphString;

/* 1024ths of a device unit */
/**
 * Pango2GlyphUnit:
 *
 * The `Pango2GlyphUnit` type is used to store dimensions within
 * Pango2.
 *
 * Dimensions are stored in 1/PANGO2_SCALE of a device unit.
 * (A device unit might be a pixel for screen display, or
 * a point on a printer.) PANGO2_SCALE is currently 1024, and
 * may change in the future (unlikely though), but you should not
 * depend on its exact value.
 *
 * The PANGO2_PIXELS() macro can be used to convert from glyph units
 * into device units with correct rounding.
 */
typedef gint32 Pango2GlyphUnit;

/* Positioning information about a glyph
 */
/**
 * Pango2GlyphGeometry:
 * @width: the logical width to use for the the character.
 * @x_offset: horizontal offset from nominal character position.
 * @y_offset: vertical offset from nominal character position.
 *
 * The `Pango2GlyphGeometry` structure contains width and positioning
 * information for a single glyph.
 *
 * Note that @width is not guaranteed to be the same as the glyph
 * extents. Kerning and other positioning applied during shaping will
 * affect both the @width and the @x_offset for the glyphs in the
 * glyph string that results from shaping.
 *
 * The information in this struct is intended for rendering the glyphs,
 * as follows:
 *
 * 1. Assume the current point is (x, y)
 * 2. Render the current glyph at (x + x_offset, y + y_offset),
 * 3. Advance the current point to (x + width, y)
 * 4. Render the next glyph
 */
struct _Pango2GlyphGeometry
{
  Pango2GlyphUnit width;
  Pango2GlyphUnit x_offset;
  Pango2GlyphUnit y_offset;
};

/* Visual attributes of a glyph
 */
/**
 * Pango2GlyphVisAttr:
 * @is_cluster_start: set for the first logical glyph in each cluster.
 * @is_color: set if the the font will render this glyph with color. Since 1.50
 *
 * A `Pango2GlyphVisAttr` structure communicates information between
 * the shaping and rendering phases.
 *
 * Currently, it contains cluster start and color information.
 * More attributes may be added in the future.
 *
 * Clusters are stored in visual order, within the cluster, glyphs
 * are always ordered in logical order, since visual order is meaningless;
 * that is, in Arabic text, accent glyphs follow the glyphs for the
 * base character.
 */
struct _Pango2GlyphVisAttr
{
  guint is_cluster_start : 1;
  guint is_color         : 1;
};

/* A single glyph
 */
/**
 * Pango2GlyphInfo:
 * @glyph: the glyph itself.
 * @geometry: the positional information about the glyph.
 * @attr: the visual attributes of the glyph.
 *
 * A `Pango2GlyphInfo` structure represents a single glyph with
 * positioning information and visual attributes.
 */
struct _Pango2GlyphInfo
{
  Pango2Glyph    glyph;
  Pango2GlyphGeometry geometry;
  Pango2GlyphVisAttr  attr;
};

/**
 * Pango2GlyphString:
 * @num_glyphs: number of glyphs in this glyph string
 * @glyphs: (array length=num_glyphs): array of glyph information
 * @log_clusters: logical cluster info, indexed by the byte index
 *   within the text corresponding to the glyph string
 *
 * A `Pango2GlyphString` is used to store strings of glyphs with geometry
 * and visual attribute information.
 *
 * The storage for the glyph information is owned by the structure
 * which simplifies memory management.
 */
struct _Pango2GlyphString {
  int num_glyphs;

  Pango2GlyphInfo *glyphs;
  int *log_clusters;

  /*< private >*/
  int space;
};

#define PANGO2_TYPE_GLYPH_STRING (pango2_glyph_string_get_type ())

PANGO2_AVAILABLE_IN_ALL
GType                   pango2_glyph_string_get_type             (void) G_GNUC_CONST;

PANGO2_AVAILABLE_IN_ALL
Pango2GlyphString *     pango2_glyph_string_new                  (void);
PANGO2_AVAILABLE_IN_ALL
void                    pango2_glyph_string_set_size             (Pango2GlyphString    *string,
                                                                  int                   new_len);

PANGO2_AVAILABLE_IN_ALL
Pango2GlyphString *     pango2_glyph_string_copy                 (Pango2GlyphString    *string);
PANGO2_AVAILABLE_IN_ALL
void                    pango2_glyph_string_free                 (Pango2GlyphString    *string);

PANGO2_AVAILABLE_IN_ALL
void                    pango2_glyph_string_extents              (Pango2GlyphString    *glyphs,
                                                                  Pango2Font           *font,
                                                                  Pango2Rectangle      *ink_rect,
                                                                  Pango2Rectangle      *logical_rect);
PANGO2_AVAILABLE_IN_ALL
int                     pango2_glyph_string_get_width            (Pango2GlyphString    *glyphs);

PANGO2_AVAILABLE_IN_ALL
void                    pango2_glyph_string_extents_range        (Pango2GlyphString    *glyphs,
                                                                  int                   start,
                                                                  int                   end,
                                                                  Pango2Font           *font,
                                                                  Pango2Rectangle      *ink_rect,
                                                                  Pango2Rectangle      *logical_rect);

PANGO2_AVAILABLE_IN_ALL
void                    pango2_glyph_string_get_logical_widths   (Pango2GlyphString    *glyphs,
                                                                  const char           *text,
                                                                  int                   length,
                                                                  int                   embedding_level,
                                                                  int                  *logical_widths);

PANGO2_AVAILABLE_IN_ALL
void                    pango2_glyph_string_index_to_x           (Pango2GlyphString    *glyphs,
                                                                  const char           *text,
                                                                  int                   length,
                                                                  const Pango2Analysis *analysis,
                                                                  int                   index_,
                                                                  gboolean              trailing,
                                                                  int                  *x_pos);
PANGO2_AVAILABLE_IN_ALL
void                    pango2_glyph_string_x_to_index           (Pango2GlyphString    *glyphs,
                                                                  const char           *text,
                                                                  int                   length,
                                                                  const Pango2Analysis *analysis,
                                                                  int                   x_pos,
                                                                  int                  *index_,
                                                                  int                  *trailing);

PANGO2_AVAILABLE_IN_ALL
void                    pango2_glyph_string_index_to_x_full      (Pango2GlyphString    *glyphs,
                                                                  const char           *text,
                                                                  int                   length,
                                                                  const Pango2Analysis *analysis,
                                                                  Pango2LogAttr        *attrs,
                                                                  int                   index_,
                                                                  gboolean              trailing,
                                                                  int                  *x_pos);

G_DEFINE_AUTOPTR_CLEANUP_FUNC(Pango2GlyphString, pango2_glyph_string_free)


/* Shaping */

/**
 * Pango2ShapeFlags:
 * @PANGO2_SHAPE_NONE: Default value
 * @PANGO2_SHAPE_ROUND_POSITIONS: Round glyph positions and widths to whole device units
 *   This option should be set if the target renderer can't do subpixel positioning of glyphs
 *
 * `Pango2ShapeFlags` influence the shaping process.
 *
 * These flags can be passed to [func@Pango2.shape].
 */
typedef enum {
  PANGO2_SHAPE_NONE            = 0,
  PANGO2_SHAPE_ROUND_POSITIONS = 1 << 0,
} Pango2ShapeFlags;

PANGO2_AVAILABLE_IN_ALL
void                    pango2_shape             (const char          *item_text,
                                                 int                  item_length,
                                                 const char          *paragraph_text,
                                                 int                  paragraph_length,
                                                 const Pango2Analysis *analysis,
                                                 Pango2GlyphString    *glyphs,
                                                 Pango2ShapeFlags      flags);


PANGO2_AVAILABLE_IN_ALL
void                    pango2_shape_item        (Pango2Item           *item,
                                                 const char          *paragraph_text,
                                                 int                  paragraph_length,
                                                 Pango2LogAttr        *log_attrs,
                                                 Pango2GlyphString    *glyphs,
                                                 Pango2ShapeFlags      flags);

/**
 * PANGO2_GLYPH_EMPTY:
 *
 * A `Pango2Glyph` value that indicates a zero-width empty glpyh.
 *
 * This is useful for example in shaper modules, to use as the glyph for
 * various zero-width Unicode characters (those passing [func@is_zero_width]).
 */
#define PANGO2_GLYPH_EMPTY           ((Pango2Glyph)0x0FFFFFFF)

/**
 * PANGO2_GLYPH_INVALID_INPUT:
 *
 * A `Pango2Glyph` value for invalid input.
 *
 * `Pango2Layout` produces one such glyph per invalid input UTF-8 byte and such
 * a glyph is rendered as a crossed box.
 *
 * Note that this value is defined such that it has the %PANGO2_GLYPH_UNKNOWN_FLAG
 * set.
 */
#define PANGO2_GLYPH_INVALID_INPUT   ((Pango2Glyph)0xFFFFFFFF)

/**
 * PANGO2_GLYPH_UNKNOWN_FLAG:
 *
 * Flag used in `Pango2Glyph` to turn a `gunichar` value of a valid Unicode
 * character into an unknown-character glyph for that `gunichar`.
 *
 * Such unknown-character glyphs may be rendered as a 'hex box'.
 */
#define PANGO2_GLYPH_UNKNOWN_FLAG    ((Pango2Glyph)0x10000000)

/**
 * PANGO2_GET_UNKNOWN_GLYPH:
 * @wc: a Unicode character
 *
 * The way this unknown glyphs are rendered is backend specific. For example,
 * a box with the hexadecimal Unicode code-point of the character written in it
 * is what is done in the most common backends.
 *
 * Returns: a `Pango2Glyph` value that means no glyph was found for @wc.
 */
#define PANGO2_GET_UNKNOWN_GLYPH(wc) ((Pango2Glyph)(wc)|PANGO2_GLYPH_UNKNOWN_FLAG)


G_END_DECLS
