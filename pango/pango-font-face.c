/* Pango
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
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Library General Public License for more details.
 *
 * You should have received a copy of the GNU Library General Public
 * License along with this library; if not, write to the
 * Free Software Foundation, Inc., 59 Temple Place - Suite 330,
 * Boston, MA 02111-1307, USA.
 */

#include "config.h"

#include "pango-font-face-private.h"
#include "pango-font-family.h"


G_DEFINE_ABSTRACT_TYPE (PangoFontFace, pango_font_face, G_TYPE_OBJECT)

static gboolean
pango_font_face_default_is_monospace (PangoFontFace *face)
{
  return FALSE;
}

static gboolean
pango_font_face_default_is_variable (PangoFontFace *face)
{
  return FALSE;
}

static gboolean
pango_font_face_default_supports_language (PangoFontFace *face,
                                           PangoLanguage *language)
{
  return TRUE;
}

static PangoLanguage **
pango_font_face_default_get_languages (PangoFontFace *face)
{
  return NULL;
}

static gboolean
pango_font_face_default_has_char (PangoFontFace *face,
                                  gunichar       wc)
{
  return FALSE;
}

static const char *
pango_font_face_default_get_faceid (PangoFontFace *face)
{
  return "";
}

static PangoFont *
pango_font_face_default_create_font (PangoFontFace              *face,
                                     const PangoFontDescription *desc,
                                     float                       dpi,
                                     const PangoMatrix          *matrix)
{
  return NULL;
}

static void
pango_font_face_class_init (PangoFontFaceClass *class G_GNUC_UNUSED)
{
  class->is_monospace = pango_font_face_default_is_monospace;
  class->is_variable = pango_font_face_default_is_variable;
  class->get_languages = pango_font_face_default_get_languages;
  class->supports_language = pango_font_face_default_supports_language;
  class->has_char = pango_font_face_default_has_char;
  class->get_faceid = pango_font_face_default_get_faceid;
  class->create_font = pango_font_face_default_create_font;
}

static void
pango_font_face_init (PangoFontFace *face G_GNUC_UNUSED)
{
}

/**
 * pango_font_face_describe:
 * @face: a `PangoFontFace`
 *
 * Returns a font description that matches the face.
 *
 * The resulting font description will have the family, style,
 * variant, weight and stretch of the face, but its size field
 * will be unset.
 *
 * Return value: a newly-created `PangoFontDescription` structure
 *   holding the description of the face. Use [method@Pango.FontDescription.free]
 *   to free the result.
 */
PangoFontDescription *
pango_font_face_describe (PangoFontFace *face)
{
  g_return_val_if_fail (PANGO_IS_FONT_FACE (face), NULL);

  return PANGO_FONT_FACE_GET_CLASS (face)->describe (face);
}

/**
 * pango_font_face_is_synthesized:
 * @face: a `PangoFontFace`
 *
 * Returns whether a `PangoFontFace` is synthesized.
 *
 * This will be the case if the underlying font rendering engine
 * creates this face from another face, by shearing, emboldening,
 * lightening or modifying it in some other way.
 *
 * Return value: whether @face is synthesized
 */
gboolean
pango_font_face_is_synthesized (PangoFontFace  *face)
{
  g_return_val_if_fail (PANGO_IS_FONT_FACE (face), FALSE);

  if (PANGO_FONT_FACE_GET_CLASS (face)->is_synthesized != NULL)
    return PANGO_FONT_FACE_GET_CLASS (face)->is_synthesized (face);
  else
    return FALSE;
}

/**
 * pango_font_face_get_face_name:
 * @face: a `PangoFontFace`.
 *
 * Gets a name representing the style of this face.
 *
 * Note that a font family may contain multiple faces
 * with the same name (e.g. a variable and a non-variable
 * face for the same style).
 *
 * Return value: the face name for the face. This string is
 *   owned by the face object and must not be modified or freed.
 */
const char *
pango_font_face_get_face_name (PangoFontFace *face)
{
  g_return_val_if_fail (PANGO_IS_FONT_FACE (face), NULL);

  return PANGO_FONT_FACE_GET_CLASS (face)->get_face_name (face);
}

/**
 * pango_font_face_get_family:
 * @face: a `PangoFontFace`
 *
 * Gets the `PangoFontFamily` that @face belongs to.
 *
 * Returns: (transfer none): the `PangoFontFamily`
 */
PangoFontFamily *
pango_font_face_get_family (PangoFontFace *face)
{
  g_return_val_if_fail (PANGO_IS_FONT_FACE (face), NULL);

  return PANGO_FONT_FACE_GET_CLASS (face)->get_family (face);
}

/**
 * pango_font_face_is_monospace:
 * @face: a `PangoFontFace`
 *
 * A monospace font is a font designed for text display where the the
 * characters form a regular grid.
 *
 * See [method@Pango.FontFamily.is_monospace] for more details.
 *
 * Returns: `TRUE` if @face is monospace
 */
gboolean
pango_font_face_is_monospace (PangoFontFace *face)
{
  g_return_val_if_fail (PANGO_IS_FONT_FACE (face), FALSE);

  return PANGO_FONT_FACE_GET_CLASS (face)->is_monospace (face);
}

/**
 * pango_font_face_is_variable:
 * @face: a `PangoFontFace`
 *
 * A variable font is a font which has axes that can be modified
 * to produce variations.
 *
 * See [method@Pango.FontFamily.is_variable] for more details.
 *
 * Returns: `TRUE` if @face is variable
 */
gboolean
pango_font_face_is_variable (PangoFontFace *face)
{
  g_return_val_if_fail (PANGO_IS_FONT_FACE (face), FALSE);

  return PANGO_FONT_FACE_GET_CLASS (face)->is_variable (face);
}

/**
 * pango_font_face_supports_language:
 * @face: a `PangoFontFace`
 * @language: a `PangoLanguage`
 *
 * Returns whether @face has all the glyphs necessary to write @language.
 *
 * Returns: `TRUE` if @face supports @language
 */
gboolean
pango_font_face_supports_language (PangoFontFace *face,
                                   PangoLanguage *language)
{
  g_return_val_if_fail (PANGO_IS_FONT_FACE (face), FALSE);

  return PANGO_FONT_FACE_GET_CLASS (face)->supports_language (face, language);
}

/**
 * pango_font_face_get_languages:
 * @face: a `PangoFontFace`
 *
 * Returns the languages that are supported by @face.
 *
 * If the font backend does not provide this information,
 * %NULL is returned. For the fontconfig backend, this
 * corresponds to the FC_LANG member of the FcPattern.
 *
 * The returned array is only valid as long as the face
 * and its fontmap are valid.
 *
 * Returns: (transfer none) (nullable) (array zero-terminated=1) (element-type PangoLanguage):
 *   an array of `PangoLanguage`
 */
PangoLanguage **
pango_font_face_get_languages (PangoFontFace *face)
{
  g_return_val_if_fail (PANGO_IS_FONT_FACE (face), FALSE);

  return PANGO_FONT_FACE_GET_CLASS (face)->get_languages (face);
}

/**
 * pango_font_face_has_char:
 * @face: a `PangoFontFace`
 * @wc: a Unicode character
 *
 * Returns whether the face provides a glyph for this character.
 *
 * Returns: `TRUE` if @font can render @wc
 */
gboolean
pango_font_face_has_char (PangoFontFace *face,
                          gunichar       wc)
{
  g_return_val_if_fail (PANGO_IS_FONT_FACE (face), FALSE);

  return PANGO_FONT_FACE_GET_CLASS (face)->has_char (face, wc);
}

/*< private >
 * pango_font_face_get_faceid:
 * @self: a `PangoHbFace`
 *
 * Returns the faceid of the face.
 *
 * The faceid is meant to uniquely identify the face among the
 * faces of its family. It includes an identifier for the font
 * file that is used (currently, we use the PostScript name for
 * this), the face index, the instance ID, as well as synthetic
 * tweaks such as emboldening and transforms and variations.
 *
 * [method@Pango.FontFace.describe] adds the faceid to the font
 * description that it produces.
 *
 * See pango_hb_family_find_face() for the code that takes the
 * faceid into account when searching for a face. It is careful
 * to fall back to approximate matching if an exact match for
 * the faceid isn't found. That should make it safe to preserve
 * faceids when saving font descriptions in configuration or
 * other data.
 *
 * There are no guarantees about the format of the string that
 * this function produces, except for that it does not contain
 * ' ', ',' or '=', so it can be safely embedded in the '@' part
 * of a serialized font description.
 *
 * Returns: (transfer none): the faceid
 */
const char *
pango_font_face_get_faceid (PangoFontFace *face)
{
  g_return_val_if_fail (PANGO_IS_FONT_FACE (face), "");

  return PANGO_FONT_FACE_GET_CLASS (face)->get_faceid (face);
}

PangoFont *
pango_font_face_create_font (PangoFontFace              *face,
                             const PangoFontDescription *desc,
                             float                       dpi,
                             const PangoMatrix          *matrix)
{
  g_return_val_if_fail (PANGO_IS_FONT_FACE (face), NULL);

  return PANGO_FONT_FACE_GET_CLASS (face)->create_font (face, desc, dpi, matrix);
}

