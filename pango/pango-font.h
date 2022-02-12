/* Pango
 * pango-font.h: Font handling
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
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Library General Public License for more details.
 *
 * You should have received a copy of the GNU Library General Public
 * License along with this library; if not, write to the
 * Free Software Foundation, Inc., 59 Temple Place - Suite 330,
 * Boston, MA 02111-1307, USA.
 */

#ifndef __PANGO_FONT_H__
#define __PANGO_FONT_H__

#include <pango/pango-coverage.h>
#include <pango/pango-types.h>
#include <pango/pango-font-description.h>

#include <glib-object.h>
#include <hb.h>

G_BEGIN_DECLS

/**
 * PangoFontMetrics:
 *
 * A `PangoFontMetrics` structure holds the overall metric information
 * for a font.
 *
 * The information in a `PangoFontMetrics` structure may be restricted
 * to a script. The fields of this structure are private to implementations
 * of a font backend. See the documentation of the corresponding getters
 * for documentation of their meaning.
 *
 * For an overview of the most important metrics, see:
 *
 * <picture>
 *   <source srcset="fontmetrics-dark.png" media="(prefers-color-scheme: dark)">
 *   <img alt="Font metrics" src="fontmetrics-light.png">
 * </picture>

 */
typedef struct _PangoFontMetrics PangoFontMetrics;

/*
 * PangoFontMetrics
 */

#define PANGO_TYPE_FONT_METRICS  (pango_font_metrics_get_type ())

struct _PangoFontMetrics
{
  /* <private> */
  guint ref_count;

  int ascent;
  int descent;
  int height;
  int approximate_char_width;
  int approximate_digit_width;
  int underline_position;
  int underline_thickness;
  int strikethrough_position;
  int strikethrough_thickness;
};

PANGO_AVAILABLE_IN_ALL
GType             pango_font_metrics_get_type                    (void) G_GNUC_CONST;
PANGO_AVAILABLE_IN_ALL
PangoFontMetrics *pango_font_metrics_ref                         (PangoFontMetrics *metrics);
PANGO_AVAILABLE_IN_ALL
void              pango_font_metrics_unref                       (PangoFontMetrics *metrics);
PANGO_AVAILABLE_IN_ALL
int               pango_font_metrics_get_ascent                  (PangoFontMetrics *metrics) G_GNUC_PURE;
PANGO_AVAILABLE_IN_ALL
int               pango_font_metrics_get_descent                 (PangoFontMetrics *metrics) G_GNUC_PURE;
PANGO_AVAILABLE_IN_1_44
int               pango_font_metrics_get_height                  (PangoFontMetrics *metrics) G_GNUC_PURE;
PANGO_AVAILABLE_IN_ALL
int               pango_font_metrics_get_approximate_char_width  (PangoFontMetrics *metrics) G_GNUC_PURE;
PANGO_AVAILABLE_IN_ALL
int               pango_font_metrics_get_approximate_digit_width (PangoFontMetrics *metrics) G_GNUC_PURE;
PANGO_AVAILABLE_IN_1_6
int               pango_font_metrics_get_underline_position      (PangoFontMetrics *metrics) G_GNUC_PURE;
PANGO_AVAILABLE_IN_1_6
int               pango_font_metrics_get_underline_thickness     (PangoFontMetrics *metrics) G_GNUC_PURE;
PANGO_AVAILABLE_IN_1_6
int               pango_font_metrics_get_strikethrough_position  (PangoFontMetrics *metrics) G_GNUC_PURE;
PANGO_AVAILABLE_IN_1_6
int               pango_font_metrics_get_strikethrough_thickness (PangoFontMetrics *metrics) G_GNUC_PURE;


/*
 * PangoFontFamily
 */

#define PANGO_TYPE_FONT_FAMILY              (pango_font_family_get_type ())
#define PANGO_FONT_FAMILY(object)           (G_TYPE_CHECK_INSTANCE_CAST ((object), PANGO_TYPE_FONT_FAMILY, PangoFontFamily))
#define PANGO_IS_FONT_FAMILY(object)        (G_TYPE_CHECK_INSTANCE_TYPE ((object), PANGO_TYPE_FONT_FAMILY))

typedef struct _PangoFontFace        PangoFontFace;
typedef struct _PangoFontFamily      PangoFontFamily;

PANGO_AVAILABLE_IN_ALL
GType      pango_font_family_get_type       (void) G_GNUC_CONST;

PANGO_AVAILABLE_IN_ALL
void                 pango_font_family_list_faces (PangoFontFamily  *family,
                                                   PangoFontFace  ***faces,
                                                   int              *n_faces);
PANGO_AVAILABLE_IN_ALL
const char *pango_font_family_get_name   (PangoFontFamily  *family) G_GNUC_PURE;
PANGO_AVAILABLE_IN_1_4
gboolean   pango_font_family_is_monospace         (PangoFontFamily  *family) G_GNUC_PURE;
PANGO_AVAILABLE_IN_1_44
gboolean   pango_font_family_is_variable          (PangoFontFamily  *family) G_GNUC_PURE;

PANGO_AVAILABLE_IN_1_46
PangoFontFace *pango_font_family_get_face (PangoFontFamily *family,
                                           const char      *name);


/*
 * PangoFontFace
 */

#define PANGO_TYPE_FONT_FACE              (pango_font_face_get_type ())
#define PANGO_FONT_FACE(object)           (G_TYPE_CHECK_INSTANCE_CAST ((object), PANGO_TYPE_FONT_FACE, PangoFontFace))
#define PANGO_IS_FONT_FACE(object)        (G_TYPE_CHECK_INSTANCE_TYPE ((object), PANGO_TYPE_FONT_FACE))


PANGO_AVAILABLE_IN_ALL
GType      pango_font_face_get_type       (void) G_GNUC_CONST;

PANGO_AVAILABLE_IN_ALL
PangoFontDescription *pango_font_face_describe       (PangoFontFace  *face);
PANGO_AVAILABLE_IN_ALL
const char           *pango_font_face_get_face_name  (PangoFontFace  *face) G_GNUC_PURE;
PANGO_AVAILABLE_IN_1_4
void                  pango_font_face_list_sizes     (PangoFontFace  *face,
                                                      int           **sizes,
                                                      int            *n_sizes);
PANGO_AVAILABLE_IN_1_18
gboolean              pango_font_face_is_synthesized (PangoFontFace  *face) G_GNUC_PURE;

PANGO_AVAILABLE_IN_1_46
PangoFontFamily *     pango_font_face_get_family     (PangoFontFace  *face);


/*
 * PangoFont
 */

#define PANGO_TYPE_FONT              (pango_font_get_type ())
#define PANGO_FONT(object)           (G_TYPE_CHECK_INSTANCE_CAST ((object), PANGO_TYPE_FONT, PangoFont))
#define PANGO_IS_FONT(object)        (G_TYPE_CHECK_INSTANCE_TYPE ((object), PANGO_TYPE_FONT))


PANGO_AVAILABLE_IN_ALL
GType                 pango_font_get_type          (void) G_GNUC_CONST;

PANGO_AVAILABLE_IN_ALL
PangoFontDescription *pango_font_describe          (PangoFont        *font);
PANGO_AVAILABLE_IN_1_14
PangoFontDescription *pango_font_describe_with_absolute_size (PangoFont        *font);
PANGO_AVAILABLE_IN_ALL
PangoCoverage *       pango_font_get_coverage      (PangoFont        *font,
                                                    PangoLanguage    *language);
PANGO_AVAILABLE_IN_ALL
PangoFontMetrics *    pango_font_get_metrics       (PangoFont        *font,
                                                    PangoLanguage    *language);
PANGO_AVAILABLE_IN_ALL
void                  pango_font_get_glyph_extents (PangoFont        *font,
                                                    PangoGlyph        glyph,
                                                    PangoRectangle   *ink_rect,
                                                    PangoRectangle   *logical_rect);
PANGO_AVAILABLE_IN_1_10
PangoFontMap         *pango_font_get_font_map      (PangoFont        *font);

PANGO_AVAILABLE_IN_1_46
PangoFontFace *       pango_font_get_face          (PangoFont        *font);

PANGO_AVAILABLE_IN_1_44
gboolean              pango_font_has_char          (PangoFont        *font,
                                                    gunichar          wc);
PANGO_AVAILABLE_IN_1_44
void                  pango_font_get_features      (PangoFont        *font,
                                                    hb_feature_t     *features,
                                                    guint             len,
                                                    guint            *num_features);
PANGO_AVAILABLE_IN_1_44
hb_font_t *           pango_font_get_hb_font       (PangoFont        *font);

PANGO_AVAILABLE_IN_1_50
PangoLanguage **      pango_font_get_languages     (PangoFont        *font);

PANGO_AVAILABLE_IN_1_50
GBytes *              pango_font_serialize         (PangoFont        *font);

PANGO_AVAILABLE_IN_1_50
PangoFont *           pango_font_deserialize       (PangoContext     *context,
                                                    GBytes           *bytes,
                                                    GError          **error);

/**
 * PANGO_GLYPH_EMPTY:
 *
 * A `PangoGlyph` value that indicates a zero-width empty glpyh.
 *
 * This is useful for example in shaper modules, to use as the glyph for
 * various zero-width Unicode characters (those passing [func@is_zero_width]).
 */

/**
 * PANGO_GLYPH_INVALID_INPUT:
 *
 * A `PangoGlyph` value for invalid input.
 *
 * `PangoLayout` produces one such glyph per invalid input UTF-8 byte and such
 * a glyph is rendered as a crossed box.
 *
 * Note that this value is defined such that it has the %PANGO_GLYPH_UNKNOWN_FLAG
 * set.
 *
 * Since: 1.20
 */
/**
 * PANGO_GLYPH_UNKNOWN_FLAG:
 *
 * Flag used in `PangoGlyph` to turn a `gunichar` value of a valid Unicode
 * character into an unknown-character glyph for that `gunichar`.
 *
 * Such unknown-character glyphs may be rendered as a 'hex box'.
 */
/**
 * PANGO_GET_UNKNOWN_GLYPH:
 * @wc: a Unicode character
 *
 * The way this unknown glyphs are rendered is backend specific. For example,
 * a box with the hexadecimal Unicode code-point of the character written in it
 * is what is done in the most common backends.
 *
 * Returns: a `PangoGlyph` value that means no glyph was found for @wc.
 */
#define PANGO_GLYPH_EMPTY           ((PangoGlyph)0x0FFFFFFF)
#define PANGO_GLYPH_INVALID_INPUT   ((PangoGlyph)0xFFFFFFFF)
#define PANGO_GLYPH_UNKNOWN_FLAG    ((PangoGlyph)0x10000000)
#define PANGO_GET_UNKNOWN_GLYPH(wc) ((PangoGlyph)(wc)|PANGO_GLYPH_UNKNOWN_FLAG)

#ifndef __GI_SCANNER__
#ifndef PANGO_DISABLE_DEPRECATED
#define PANGO_UNKNOWN_GLYPH_WIDTH  10
#define PANGO_UNKNOWN_GLYPH_HEIGHT 14
#endif
#endif

G_DEFINE_AUTOPTR_CLEANUP_FUNC(PangoFontFamily, g_object_unref)
G_DEFINE_AUTOPTR_CLEANUP_FUNC(PangoFontFace, g_object_unref)
G_DEFINE_AUTOPTR_CLEANUP_FUNC(PangoFont, g_object_unref)

G_END_DECLS

#endif /* __PANGO_FONT_H__ */
