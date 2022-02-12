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


G_DEFINE_ABSTRACT_TYPE (PangoFontFace, pango_font_face, G_TYPE_OBJECT)

static void
pango_font_face_class_init (PangoFontFaceClass *class G_GNUC_UNUSED)
{
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
 *
 * Since: 1.18
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
 * pango_font_face_list_sizes:
 * @face: a `PangoFontFace`.
 * @sizes: (out) (array length=n_sizes) (nullable) (optional):
 *   location to store a pointer to an array of int. This array
 *   should be freed with g_free().
 * @n_sizes: location to store the number of elements in @sizes
 *
 * List the available sizes for a font.
 *
 * This is only applicable to bitmap fonts. For scalable fonts, stores
 * %NULL at the location pointed to by @sizes and 0 at the location pointed
 * to by @n_sizes. The sizes returned are in Pango units and are sorted
 * in ascending order.
 *
 * Since: 1.4
 */
void
pango_font_face_list_sizes (PangoFontFace  *face,
                            int           **sizes,
                            int            *n_sizes)
{
  g_return_if_fail (PANGO_IS_FONT_FACE (face));
  g_return_if_fail (sizes == NULL || n_sizes != NULL);

  if (n_sizes == NULL)
    return;

  if (PANGO_FONT_FACE_GET_CLASS (face)->list_sizes != NULL)
    PANGO_FONT_FACE_GET_CLASS (face)->list_sizes (face, sizes, n_sizes);
  else
    {
      if (sizes != NULL)
        *sizes = NULL;
      *n_sizes = 0;
    }
}

/**
 * pango_font_face_get_family:
 * @face: a `PangoFontFace`
 *
 * Gets the `PangoFontFamily` that @face belongs to.
 *
 * Returns: (transfer none): the `PangoFontFamily`
 *
 * Since: 1.46
 */
PangoFontFamily *
pango_font_face_get_family (PangoFontFace *face)
{
  g_return_val_if_fail (PANGO_IS_FONT_FACE (face), NULL);

  return PANGO_FONT_FACE_GET_CLASS (face)->get_family (face);
}
