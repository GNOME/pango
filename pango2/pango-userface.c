/* Pango2
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
 * Pango2UserFace:
 *
 * `Pango2UserFace` is a `Pango2FontFace` implementation that uses callbacks.
 *
 * It allows to draw the glyphs in a font using custom code. This can
 * be used to implement fonts in non-standard formats, but can also be
 * used by games and other application to draw "funky" fonts.
 *
 * To get a font instance at a specific size from a `Pango2UserFace`,
 * use [ctor@Pango2.UserFont.new].
 */

/* {{{ Utilities */

static void
ensure_faceid (Pango2UserFace *self)
{
  Pango2FontFace *face = PANGO2_FONT_FACE (self);
  char *psname;
  char *p;

  if (self->faceid)
    return;

  psname = g_strconcat (pango2_font_description_get_family (face->description), "_", face->name, NULL);

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
style_from_font_description (const Pango2FontDescription *desc)
{
  Pango2Style style = pango2_font_description_get_style (desc);
  Pango2Weight weight = pango2_font_description_get_weight (desc);

  switch (style)
    {
    case PANGO2_STYLE_ITALIC:
      if (weight == PANGO2_WEIGHT_BOLD)
        return "Bold Italic";
      else
        return "Italic";
      break;
    case PANGO2_STYLE_OBLIQUE:
      if (weight == PANGO2_WEIGHT_BOLD)
        return "Bold Oblique";
      else
        return "Oblique";
      break;
    case PANGO2_STYLE_NORMAL:
      if (weight == PANGO2_WEIGHT_BOLD)
        return "Bold";
      else
        return "Regular";
      break;
    default: ;
    }

  return NULL;
}

static gboolean
default_shape_func (Pango2UserFace       *face,
                    int                   size,
                    const char           *text,
                    int                   length,
                    const Pango2Analysis *analysis,
                    Pango2GlyphString    *glyphs,
                    Pango2ShapeFlags      flags,
                    gpointer              user_data)
{
  int n_chars;
  const char *p;
  int cluster = 0;
  int i;
  int last_cluster;
  gboolean is_color;
  hb_glyph_extents_t ext;
  hb_position_t dummy;
  hb_font_t *hb_font;
  hb_font_extents_t font_extents;

  n_chars = g_utf8_strlen (text, length);

  pango2_glyph_string_set_size (glyphs, n_chars);

  last_cluster = -1;

  hb_font = pango2_font_get_hb_font (analysis->font);
  hb_font_get_h_extents (hb_font, &font_extents);

  p = text;
  for (i = 0; i < n_chars; i++)
    {
      gunichar wc;
      Pango2Glyph glyph;
      Pango2Rectangle ink_rect;
      Pango2Rectangle logical_rect;

      wc = g_utf8_get_char (p);

      if (g_unichar_type (wc) != G_UNICODE_NON_SPACING_MARK)
        cluster = p - text;

      if (pango2_is_zero_width (wc))
        glyph = PANGO2_GLYPH_EMPTY;
      else if (!face->glyph_func (face, wc, &glyph, face->user_data))
        glyph = PANGO2_GET_UNKNOWN_GLYPH (wc);

      face->glyph_info_func (face, size, glyph, &ext, &dummy, &dummy, &is_color, face->user_data);
      pango2_font_get_glyph_extents (analysis->font, glyph, &ink_rect, &logical_rect);

      glyphs->glyphs[i].glyph = glyph;

      glyphs->glyphs[i].attr.is_cluster_start = cluster != last_cluster;
      glyphs->glyphs[i].attr.is_color = is_color;

      if (analysis->gravity == PANGO2_GRAVITY_EAST)
        {
          glyphs->glyphs[i].geometry.x_offset = font_extents.ascender;
          glyphs->glyphs[i].geometry.y_offset = - logical_rect.y - (logical_rect.height - ink_rect.height) / 2;
          glyphs->glyphs[i].geometry.width = logical_rect.width;
        }
      else if (analysis->gravity == PANGO2_GRAVITY_WEST)
        {
          glyphs->glyphs[i].geometry.x_offset = font_extents.descender;
          glyphs->glyphs[i].geometry.y_offset = logical_rect.y + (logical_rect.height - ink_rect.height) / 2;
          glyphs->glyphs[i].geometry.width = logical_rect.width;
        }
      else if (analysis->gravity == PANGO2_GRAVITY_SOUTH)
        {
          glyphs->glyphs[i].geometry.x_offset = 0;
          glyphs->glyphs[i].geometry.y_offset = 0;
          glyphs->glyphs[i].geometry.width = logical_rect.width;
        }
      else if (analysis->gravity == PANGO2_GRAVITY_NORTH)
        {
          glyphs->glyphs[i].geometry.x_offset = 0;
          glyphs->glyphs[i].geometry.y_offset = 0;
          glyphs->glyphs[i].geometry.width = - logical_rect.width;
        }

      glyphs->log_clusters[i] = cluster;
      last_cluster = cluster;

      p = g_utf8_next_char (p);
    }

  if (analysis->level & 1)
    pango2_glyph_string_reverse_range (glyphs, 0, glyphs->num_glyphs);

  return TRUE;
}

static gboolean
default_render_func (Pango2UserFace *face,
                     int             size,
                     hb_codepoint_t  glyph,
                     gpointer        user_data,
                     const char      *backend_id,
                     gpointer        backend_data)
{
  /* Draw nothing... not very exciting */
  return TRUE;
}

/* }}} */
/* {{{ Pango2FontFace implementation */

struct _Pango2UserFaceClass
{
  Pango2FontFaceClass parent_class;
};

G_DEFINE_FINAL_TYPE (Pango2UserFace, pango2_user_face, PANGO2_TYPE_FONT_FACE)

static void
pango2_user_face_init (Pango2UserFace *self)
{
}

static void
pango2_user_face_finalize (GObject *object)
{
  Pango2UserFace *self = PANGO2_USER_FACE (object);

  g_free (self->faceid);
  if (self->destroy)
    self->destroy (self->user_data);

  G_OBJECT_CLASS (pango2_user_face_parent_class)->finalize (object);
}

static gboolean
pango2_user_face_is_synthesized (Pango2FontFace *face)
{
  return TRUE;
}

static gboolean
pango2_user_face_is_monospace (Pango2FontFace *face)
{
  return FALSE;
}

static gboolean
pango2_user_face_is_variable (Pango2FontFace *face)
{
  return FALSE;
}

static gboolean
pango2_user_face_has_char (Pango2FontFace *face,
                           gunichar        wc)
{
  Pango2UserFace *self = PANGO2_USER_FACE (face);
  hb_codepoint_t glyph;

  return self->glyph_func (self, wc, &glyph, self->user_data);
}

static const char *
pango2_user_face_get_faceid (Pango2FontFace *face)
{
  Pango2UserFace *self = PANGO2_USER_FACE (face);

  ensure_faceid (self);

  return self->faceid;
}

static Pango2Font *
pango2_user_face_create_font (Pango2FontFace              *face,
                              const Pango2FontDescription *desc,
                              float                        dpi,
                              const Pango2Matrix          *ctm)
{
  Pango2UserFace *self = PANGO2_USER_FACE (face);

  return PANGO2_FONT (pango2_user_font_new_for_description (self, desc, dpi, ctm));
}

static void
pango2_user_face_class_init (Pango2UserFaceClass *class)
{
  GObjectClass *object_class = G_OBJECT_CLASS (class);
  Pango2FontFaceClass *face_class = PANGO2_FONT_FACE_CLASS (class);

  object_class->finalize = pango2_user_face_finalize;

  face_class->is_synthesized = pango2_user_face_is_synthesized;
  face_class->is_monospace = pango2_user_face_is_monospace;
  face_class->is_variable = pango2_user_face_is_variable;
  face_class->has_char = pango2_user_face_has_char;
  face_class->get_faceid = pango2_user_face_get_faceid;
  face_class->create_font = pango2_user_face_create_font;
}

/* }}} */
 /* {{{ Public API */

/**
 * Pango2UserFaceGetFontInfoFunc:
 * @face: the `Pango2UserFace`
 * @size: the size of the font that is being created
 * @extents: (out caller-allocates): return location for font extents
 * user_data: user data that was passed to [ctor@Pango2.UserFace.new]
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
 * Pango2UserFaceUnicodeToGlyphFunc:
 * @face: the `Pango2UserFace`
 * @unicode: the Unicode character
 * @glyph: (out caller-allocates): return location for the glyph that
 * @user_data: user data that was passed to [ctor@Pango2.UserFace.new]
 *
 * Callback to determine if a user font can render a character,
 * and what glyph it will use.
 *
 *  Returns: `TRUE` on success
 */

/**
 * Pango2UserFaceGetGlyphInfoFunc:
 * @face: the `Pango2UserFace`
 * @size: the size of the font that is queried
 * @glyph: the glyph that is being queried
 * @extents: (out caller-allocates): return location for the glyphs ink rectangle
 * @h_advance: (out caller-allocates): return location for the h advance
 * @v_advance: (out caller-allocates): return location for the v advance
 * @is_color_glyph: (out caller-allocates): return location for information about
 *   whether @glyph has color
 * @user_data: user data that was passed to [ctor@Pango2.UserFace.new]
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
 * Pango2UserFaceTextToGlyphFunc:
 * @face: the `Pango2UserFace`
 * @size: the size of the font that is used
 * @text: the text to shape
 * @length: the length of @text
 * @analysis: `Pango2Analysis` for @text
 * @glyphs: (out caller-allocates): the `Pango2GlyphString` to populate
 * @flags: `Pango2ShapeFlags` to use
 * @user_data: user data that was pased to [ctor@Pango2.UserFace.new]
 *
 * Callback to shape a segment of text with a user font.
 *
 * This callback is optional when creating a user font. If it isn't
 * provided, Pango will rely on the `Pango2UserFaceUnicodeToGlyphFunc`
 * and the `Pango2UserFaceGetGlyphInfo` callback to translate Unicode
 * characters to glyphs 1-1, and position the glyphs according to their
 * advance widths.
 *
 * If this callback is provided, it replaces all of Pango2's own shaping.
 * The function can implement ligatures, reordering, and other features
 * that turn the text-to-glyph mapping into an m-n relationship. The
 * function is responsible for filling not just the glyphs and their
 * positions, but also cluster information and glyph attributes in
 * [struct@Pango2.GlyphVisAttr].
 *
 * Returns: `TRUE` on success
 */

/**
 * Pango2UserFaceRenderGlyphFunc:
 * @face: the `Pango2UserFace`
 * @size: the size of the font that is used
 * @glyph: the glyph that is being queried
 * @user_data: user data that was pased to [ctor@Pango2.UserFace.new]
 * @backend_id: a string identifying the [class@Pango2.Renderer] in use
 * @backend_data: backend-specific data
 *
 * Callback to render a glyph with a user font.
 *
 * This callback is optional when creating a user font. If it isn't
 * provided, the font will not produce any visible output.
 *
 * The @backend_id identifies the [class@Pango2.Renderer] in use.
 * Implementations should return `FALSE` for unsupported backends.
 *
 * The cairo backend uses the string "cairo" as @backend_id, and
 * provides a `cairo_t` as @backend_data. The context is set up
 * to render in `font space`, i.e. The transformation is set up
 * to map the unit square to @size x @size. If supported, Pango2
 * uses `cairo_user_font_face_set_render_color_glyph_func` to
 * allow glyphs to be rendered with colors. For more information,
 * see the cairo documentation about user fonts.
 *
 * Returns: `TRUE` on success
 */

/**
 * pango2_user_face_new:
 * @font_info_func: (scope notified): a `Pango2UserFaceGetFontInfoFunc`
 * @glyph_func: (scope notified): a `Pango2UserFaceUnicodeToGlyphFunc`
 * @glyph_info_func: (scope notified): a `Pango2UserFaceGetGlyphInfoFunc`
 * @shape_func: (scope notified) (nullable): a `Pango2UserFaceTextToGlyphFunc`
 * @render_func: (scope notified) (nullable): a `Pango2UserFaceRenderGlyphFunc`
 * @user_data: user data that will be assed to the callbacks
 * @destroy: destroy notify for @user_data
 * @name: name for the face
 * @description: `Pango2FontDescription` for the font
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
 * Returns: (transfer full): a newly created `Pango2UserFace`
 */
Pango2UserFace *
pango2_user_face_new (Pango2UserFaceGetFontInfoFunc     font_info_func,
                      Pango2UserFaceUnicodeToGlyphFunc  glyph_func,
                      Pango2UserFaceGetGlyphInfoFunc    glyph_info_func,
                      Pango2UserFaceTextToGlyphFunc     shape_func,
                      Pango2UserFaceRenderGlyphFunc     render_func,
                      gpointer                          user_data,
                      GDestroyNotify                    destroy,
                      const char                       *name,
                      const Pango2FontDescription      *description)
{
  Pango2UserFace *self;
  Pango2FontFace *face;

  g_return_val_if_fail (font_info_func != NULL, NULL);
  g_return_val_if_fail (glyph_func != NULL, NULL);
  g_return_val_if_fail (glyph_info_func != NULL, NULL);
  g_return_val_if_fail (name != NULL, NULL);
  g_return_val_if_fail (description != NULL, NULL);

  self = g_object_new (PANGO2_TYPE_USER_FACE, NULL);

  self->font_info_func = font_info_func;
  self->glyph_func = glyph_func;
  self->glyph_info_func = glyph_info_func;
  self->shape_func = shape_func ? shape_func : default_shape_func;
  self->render_func = render_func ? render_func : default_render_func;
  self->user_data = user_data;
  self->destroy = destroy;

  if (!name)
    name = style_from_font_description (description);

  face = PANGO2_FONT_FACE (self);

  face->name = g_strdup (name);
  face->description = pango2_font_description_copy (description);

  return self;
}

/* }}} */

/* vim:set foldmethod=marker expandtab: */
