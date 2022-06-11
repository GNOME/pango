/* Pango
 *
 * Copyright (C) 2022 Matthias Clasen
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Library General Public
 * License as published by the Free Software Foundation; either
 * version 2 of the License, or (at your option) any later version.
 *
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the GNU
 * Library General Public License for more details.
 *
 * You should have received a copy of the GNU Library General Public
 * License along with this library; if not, write to the
 * Free Software Foundation, Inc., 59 Temple Place - Suite 330,
 * Boston, MA 02111-1307, USA.
 */

#include "config.h"

#include "pango-font-face-private.h"
#include "pango-userface-private.h"
#include "pango-userfont-private.h"
#include "pango-utils.h"
#include "pango-item-private.h"
#include "pango-impl-utils.h"

#include <string.h>
#include <hb-ot.h>

/**
 * PangoUserFace:
 *
 * `PangoUserFace` is a `PangoFontFace` implementation that uses callbacks.
 *
 * It allows to draw the glyphs in a font using custom code. This can
 * be used to implement fonts in non-standard formats, but can also be
 * used by games and other application to draw "funky" fonts.
 *
 * To get a font instance at a specific size from a `PangoUserFace`,
 * use [ctor@Pango.UserFont.new].
 */

 /* {{{ Utilities */

static void
ensure_faceid (PangoUserFace *self)
{
  PangoFontFace *face = PANGO_FONT_FACE (self);
  char *psname;
  char *p;

  if (self->faceid)
    return;

  psname = g_strconcat (pango_font_description_get_family (face->description), "_", face->name, NULL);

  /* PostScript name should not contain problematic chars, but just in case,
   * make sure we don't have any ' ', '=' or ',' that would give us parsing
   * problems.
   */
  p = psname;
  while ((p = strpbrk (p, " =,")) != NULL)
    *p = '?';

  self->faceid = g_strconcat ("user:", psname, NULL);

  g_free (psname);
}

static const char *
style_from_font_description (const PangoFontDescription *desc)
{
  PangoStyle style = pango_font_description_get_style (desc);
  PangoWeight weight = pango_font_description_get_weight (desc);

  switch (style)
    {
    case PANGO_STYLE_ITALIC:
      if (weight == PANGO_WEIGHT_BOLD)
        return "Bold Italic";
      else
        return "Italic";
      break;
    case PANGO_STYLE_OBLIQUE:
      if (weight == PANGO_WEIGHT_BOLD)
        return "Bold Oblique";
      else
        return "Oblique";
      break;
    case PANGO_STYLE_NORMAL:
      if (weight == PANGO_WEIGHT_BOLD)
        return "Bold";
      else
        return "Regular";
      break;
    default: ;
    }

  return NULL;
}

static gboolean
default_shape_func (PangoUserFace       *face,
                    int                  size,
                    const char          *text,
                    int                  length,
                    const PangoAnalysis *analysis,
                    PangoGlyphString    *glyphs,
                    PangoShapeFlags      flags,
                    gpointer             user_data)
{
  int n_chars;
  const char *p;
  int cluster = 0;
  int i;
  int last_cluster;
  gboolean is_color;
  hb_glyph_extents_t ext;
  hb_position_t dummy;

  n_chars = g_utf8_strlen (text, length);

  pango_glyph_string_set_size (glyphs, n_chars);

  last_cluster = -1;

  p = text;
  for (i = 0; i < n_chars; i++)
    {
      gunichar wc;
      PangoGlyph glyph;
      PangoRectangle logical_rect;

      wc = g_utf8_get_char (p);

      if (g_unichar_type (wc) != G_UNICODE_NON_SPACING_MARK)
        cluster = p - text;

      if (pango_is_zero_width (wc))
        glyph = PANGO_GLYPH_EMPTY;
      else if (!face->glyph_func (face, wc, &glyph, face->user_data))
        glyph = PANGO_GET_UNKNOWN_GLYPH (wc);

      face->glyph_info_func (face, size, glyph, &ext, &dummy, &dummy, &is_color, face->user_data);
      pango_font_get_glyph_extents (analysis->font, glyph, NULL, &logical_rect);

      glyphs->glyphs[i].glyph = glyph;

      glyphs->glyphs[i].attr.is_cluster_start = cluster != last_cluster;
      glyphs->glyphs[i].attr.is_color = is_color;

      glyphs->glyphs[i].geometry.x_offset = 0;
      glyphs->glyphs[i].geometry.y_offset = 0;
      glyphs->glyphs[i].geometry.width = logical_rect.width;

      glyphs->log_clusters[i] = cluster;
      last_cluster = cluster;

      p = g_utf8_next_char (p);
    }

  if (analysis->level & 1)
    pango_glyph_string_reverse_range (glyphs, 0, glyphs->num_glyphs);

  return TRUE;
}

static gboolean
default_render_func (PangoUserFace *face,
                     int            size,
                     hb_codepoint_t  glyph,
                     gpointer        user_data,
                     const char     *backend_id,
                     gpointer        backend_data)
{
  /* Draw nothing... not very exciting */
  return TRUE;
}

/* }}} */
/* {{{ PangoFontFace implementation */

struct _PangoUserFaceClass
{
  PangoFontFaceClass parent_class;
};

G_DEFINE_FINAL_TYPE (PangoUserFace, pango_user_face, PANGO_TYPE_FONT_FACE)

static void
pango_user_face_init (PangoUserFace *self)
{
}

static void
pango_user_face_finalize (GObject *object)
{
  PangoUserFace *self = PANGO_USER_FACE (object);

  g_free (self->faceid);
  if (self->destroy)
    self->destroy (self->user_data);

  G_OBJECT_CLASS (pango_user_face_parent_class)->finalize (object);
}

static gboolean
pango_user_face_is_synthesized (PangoFontFace *face)
{
  return TRUE;
}

static gboolean
pango_user_face_is_monospace (PangoFontFace *face)
{
  return FALSE;
}

static gboolean
pango_user_face_is_variable (PangoFontFace *face)
{
  return FALSE;
}

static gboolean
pango_user_face_has_char (PangoFontFace *face,
                          gunichar       wc)
{
  PangoUserFace *self = PANGO_USER_FACE (face);
  hb_codepoint_t glyph;

  return self->glyph_func (self, wc, &glyph, self->user_data);
}

static const char *
pango_user_face_get_faceid (PangoFontFace *face)
{
  PangoUserFace *self = PANGO_USER_FACE (face);

  ensure_faceid (self);

  return self->faceid;
}

static PangoFont *
pango_user_face_create_font (PangoFontFace              *face,
                             const PangoFontDescription *desc,
                             float                       dpi,
                             const PangoMatrix          *matrix)
{
  PangoUserFace *self = PANGO_USER_FACE (face);

  return PANGO_FONT (pango_user_font_new_for_description (self, desc, dpi, matrix));
}

static void
pango_user_face_class_init (PangoUserFaceClass *class)
{
  GObjectClass *object_class = G_OBJECT_CLASS (class);
  PangoFontFaceClass *face_class = PANGO_FONT_FACE_CLASS (class);

  object_class->finalize = pango_user_face_finalize;

  face_class->is_synthesized = pango_user_face_is_synthesized;
  face_class->is_monospace = pango_user_face_is_monospace;
  face_class->is_variable = pango_user_face_is_variable;
  face_class->has_char = pango_user_face_has_char;
  face_class->get_faceid = pango_user_face_get_faceid;
  face_class->create_font = pango_user_face_create_font;
}

/* }}} */
 /* {{{ Public API */

/**
 * PangoUserFaceGetFontInfoFunc:
 * @face: the `PangoUserFace`
 * @size: the size of the font that is being created
 * @extents: (out caller-allocates): return location for font extents
 * user_data: user data that was passed to [ctor@Pango.UserFace.new]
 *
 * Callback to obtain font extents for user fonts.
 *
 * Typically, the `ascender` and `descender` fields in the @extents
 * struct should both the filled with non-negative values that add
 * up to @size.
 *
 *  Returns: `TRUE` on success
 */

/**
 * PangoUserFaceUnicodeToGlyphFunc:
 * @face: the `PangoUserFace`
 * @unicode: the Unicode character
 * @glyph: (out caller-allocates): return location for the glyph that
 * @user_data: user data that was passed to [ctor@Pango.UserFace.new]
 *
 * Callback to determine if a user font can render a character,
 * and what glyph it will use.
 *
 *  Returns: `TRUE` on success
 */

/**
 * PangoUserFaceGetGlyphInfoFunc:
 * @face: the `PangoUserFace`
 * @size: the size of the font that is queried
 * @glyph: the glyph that is being queried
 * @extents: (out caller-allocates): return location for the glyphs ink rectangle
 * @h_advance: (out caller-allocates): return location for the h advance
 * @v_advance: (out caller-allocates): return location for the v advance
 * @is_color_glyph: (out caller-allocates): return location for information about
 *   whether @glyph has color
 * @user_data: user data that was passed to [ctor@Pango.UserFace.new]
 *
 * Callback to obtain information about a glyph in a user font.
 *
 * The @extents, @h_advance and @v_advance arguments should be filled with
 * values that are scaled according to @size. Note that @y_bearing will typically
 * be positive, and @height negative.
 *
 * Returns: `TRUE` on success
 */

/**
 * PangoUserFaceTextToGlyphFunc:
 * @face: the `PangoUserFace`
 * @size: the size of the font that is used
 * @text: the text to shape
 * @length: the length of @text
 * @analysis: `PangoAnalysis` for @text
 * @glyphs: (out caller-allocates): the `PangoGlyphString` to populate
 * @flags: `PangoShapeFlags` to use
 * @user_data: user data that was pased to [ctor@Pango.UserFace.new]
 *
 * Callback to shape a segment of text with a user font.
 *
 * This callback is optional when creating a user font. If it isn't
 * provided, Pango will rely on the `PangoUserFaceUnicodeToGlyphFunc`
 * and the `PangoUserFaceGetGlyphInfo` callback to translate Unicode
 * characters to glyphs 1-1, and position the glyphs according to their
 * advance widths.
 *
 * If this callback is provided, it replaces all of Pango's own shaping.
 * The function can implement ligatures, reordering, and other features
 * that turn the text-to-glyph mapping into an m-n relationship. The
 * function is responsible for filling not just the glyphs and their
 * positions, but also cluster information and glyph attributes in
 * [struct@Pango.GlyphVisAttr].
 *
 * Returns: `TRUE` on success
 */

/**
 * PangoUserFaceRenderGlyphFunc:
 * @face: the `PangoUserFace`
 * @size: the size of the font that is used
 * @glyph: the glyph that is being queried
 * @user_data: user data that was pased to [ctor@Pango.UserFace.new]
 * @backend_id: a string identifying the [class@Pango.Renderer] in use
 * @backend_data: backend-specific data
 *
 * Callback to render a glyph with a user font.
 *
 * This callback is optional when creating a user font. If it isn't
 * provided, the font will not produce any visible output.
 *
 * The @backend_id identifies the [class@Pango.Renderer] in use.
 * Implementations should return `FALSE` for unsupported backends.
 *
 * The cairo backend uses the string "cairo" as @backend_id, and
 * provides a `cairo_t` as @backend_data. The context is set up
 * to render in `font space`, i.e. The transformation is set up
 * to map the unit square to @size x @size. If supported, Pango
 * uses `cairo_user_font_face_set_render_color_glyph_func` to
 * allow glyphs to be rendered with colors. For more information,
 * see the cairo documentation about user fonts.
 *
 * Returns: `TRUE` on success
 */

/**
 * pango_user_face_new:
 * @font_info_func: (scope notified): a `PangoUserFaceGetFontInfoFunc`
 * @glyph_func: (scope notified): a `PangoUserFaceUnicodeToGlyphFunc`
 * @glyph_info_func: (scope notified): a `PangoUserFaceGetGlyphInfoFunc`
 * @shape_func: (scope notified) (nullable): a `PangoUserFaceTextToGlyphFunc`
 * @render_func: (scope notified) (nullable): a `PangoUserFaceRenderGlyphFunc`
 * @user_data: user data that will be assed to the callbacks
 * @destroy: destroy notify for @user_data
 * @name: name for the face
 * @description: `PangoFontDescription` for the font
 *
 * Creates a new user font face.
 *
 * A user font face does not rely on font data from a font file,
 * but instead uses callbacks to determine glyph extents, positions
 * and rendering.
 *
 * If @shape_func is `NULL`, Pango will rely on @glyph_func and
 * @glyph_info_func to find and position a glyph for each character.
 *
 * If @render_func is `NULL`, the font will not produce any visible
 * glyphs.
 *
 * Returns: (transfer full): a newly created `PangoUserFace`
 */
PangoUserFace *
pango_user_face_new (PangoUserFaceGetFontInfoFunc     font_info_func,
                     PangoUserFaceUnicodeToGlyphFunc  glyph_func,
                     PangoUserFaceGetGlyphInfoFunc    glyph_info_func,
                     PangoUserFaceTextToGlyphFunc     shape_func,
                     PangoUserFaceRenderGlyphFunc     render_func,
                     gpointer                         user_data,
                     GDestroyNotify                   destroy,
                     const char                      *name,
                     const PangoFontDescription      *description)
{
  PangoUserFace *self;
  PangoFontFace *face;

  g_return_val_if_fail (font_info_func != NULL, NULL);
  g_return_val_if_fail (glyph_func != NULL, NULL);
  g_return_val_if_fail (glyph_info_func != NULL, NULL);
  g_return_val_if_fail (name != NULL, NULL);
  g_return_val_if_fail (description != NULL, NULL);

  self = g_object_new (PANGO_TYPE_USER_FACE, NULL);

  self->font_info_func = font_info_func;
  self->glyph_func = glyph_func;
  self->glyph_info_func = glyph_info_func;
  self->shape_func = shape_func ? shape_func : default_shape_func;
  self->render_func = render_func ? render_func : default_render_func;
  self->user_data = user_data;
  self->destroy = destroy;

  if (!name)
    name = style_from_font_description (description);

  face = PANGO_FONT_FACE (self);

  face->name = g_strdup (name);
  face->description = pango_font_description_copy (description);

  return self;
}

/* }}} */

/* vim:set foldmethod=marker expandtab: */
