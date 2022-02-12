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

#include "pango-font-family-private.h"
#include "pango-font.h"

#if 0
#include <stdlib.h>
#include <math.h>
#include <string.h>

#include <gio/gio.h>

#include "pango-types.h"
#include "pango-font-private.h"
#include "pango-fontmap.h"
#include "pango-impl-utils.h"
#endif


static GType
pango_font_family_get_item_type (GListModel *list)
{
  return PANGO_TYPE_FONT_FACE;
}

static guint
pango_font_family_get_n_items (GListModel *list)
{
  PangoFontFamily *family = PANGO_FONT_FAMILY (list);
  int n_faces;

  pango_font_family_list_faces (family, NULL, &n_faces);

  return (guint)n_faces;
}

static gpointer
pango_font_family_get_item (GListModel *list,
                            guint       position)
{
  PangoFontFamily *family = PANGO_FONT_FAMILY (list);
  PangoFontFace **faces;
  int n_faces;
  PangoFontFace *face;

  pango_font_family_list_faces (family, &faces, &n_faces);

  if (position < n_faces)
    face = g_object_ref (faces[position]);
  else
    face = NULL;

  g_free (faces);

  return face;
}

static void
pango_font_family_list_model_init (GListModelInterface *iface)
{
  iface->get_item_type = pango_font_family_get_item_type;
  iface->get_n_items = pango_font_family_get_n_items;
  iface->get_item = pango_font_family_get_item;
}

G_DEFINE_ABSTRACT_TYPE_WITH_CODE (PangoFontFamily, pango_font_family, G_TYPE_OBJECT,
                                  G_IMPLEMENT_INTERFACE (G_TYPE_LIST_MODEL, pango_font_family_list_model_init))

static PangoFontFace *pango_font_family_real_get_face (PangoFontFamily *family,
                                                       const char      *name);

static gboolean
pango_font_family_default_is_monospace (PangoFontFamily *family)
{
  return FALSE;
}

static gboolean
pango_font_family_default_is_variable (PangoFontFamily *family)
{
  return FALSE;
}

static void
pango_font_family_default_list_faces (PangoFontFamily   *family,
                                      PangoFontFace   ***faces,
                                      int               *n_faces)
{
  if (faces)
    *faces = NULL;

  if (n_faces)
    *n_faces = 0;
}

static void
pango_font_family_class_init (PangoFontFamilyClass *class G_GNUC_UNUSED)
{
  class->is_monospace = pango_font_family_default_is_monospace;
  class->is_variable = pango_font_family_default_is_variable;
  class->get_face = pango_font_family_real_get_face;
  class->list_faces = pango_font_family_default_list_faces;
}

static void
pango_font_family_init (PangoFontFamily *family G_GNUC_UNUSED)
{
}

/**
 * pango_font_family_get_name:
 * @family: a `PangoFontFamily`
 *
 * Gets the name of the family.
 *
 * The name is unique among all fonts for the font backend and can
 * be used in a `PangoFontDescription` to specify that a face from
 * this family is desired.
 *
 * Return value: the name of the family. This string is owned
 *   by the family object and must not be modified or freed.
 */
const char *
pango_font_family_get_name (PangoFontFamily  *family)
{
  g_return_val_if_fail (PANGO_IS_FONT_FAMILY (family), NULL);

  return PANGO_FONT_FAMILY_GET_CLASS (family)->get_name (family);
}

/**
 * pango_font_family_list_faces:
 * @family: a `PangoFontFamily`
 * @faces: (out) (optional) (array length=n_faces) (transfer container):
 *   location to store an array of pointers to `PangoFontFace` objects,
 *   or %NULL. This array should be freed with g_free() when it is no
 *   longer needed.
 * @n_faces: (out): location to store number of elements in @faces.
 *
 * Lists the different font faces that make up @family.
 *
 * The faces in a family share a common design, but differ in slant, weight,
 * width and other aspects.
 *
 * Note that the returned faces are not in any particular order, and
 * multiple faces may have the same name or characteristics.
 *
 * `PangoFontFamily` also implemented the [iface@Gio.ListModel] interface
 * for enumerating faces.
 */
void
pango_font_family_list_faces (PangoFontFamily  *family,
                              PangoFontFace  ***faces,
                              int              *n_faces)
{
  g_return_if_fail (PANGO_IS_FONT_FAMILY (family));

  PANGO_FONT_FAMILY_GET_CLASS (family)->list_faces (family, faces, n_faces);
}

static PangoFontFace *
pango_font_family_real_get_face (PangoFontFamily *family,
                                 const char      *name)
{
  PangoFontFace **faces;
  int n_faces;
  PangoFontFace *face;
  int i;

  pango_font_family_list_faces (family, &faces, &n_faces);

  face = NULL;
  if (name == NULL && n_faces > 0)
    {
      face = faces[0];
    }
  else
    {
      for (i = 0; i < n_faces; i++)
        {
          if (strcmp (name, pango_font_face_get_face_name (faces[i])) == 0)
            {
              face = faces[i];
              break;
            }
        }
    }

  g_free (faces);

  return face;
}

/**
 * pango_font_family_get_face:
 * @family: a `PangoFontFamily`
 * @name: (nullable): the name of a face. If the name is %NULL,
 *   the family's default face (fontconfig calls it "Regular")
 *   will be returned.
 *
 * Gets the `PangoFontFace` of @family with the given name.
 *
 * Returns: (transfer none) (nullable): the `PangoFontFace`,
 *   or %NULL if no face with the given name exists.
 *
 * Since: 1.46
 */
PangoFontFace *
pango_font_family_get_face (PangoFontFamily *family,
                            const char      *name)
{
  g_return_val_if_fail (PANGO_IS_FONT_FAMILY (family), NULL);

  return PANGO_FONT_FAMILY_GET_CLASS (family)->get_face (family, name);
}

/**
 * pango_font_family_is_monospace:
 * @family: a `PangoFontFamily`
 *
 * A monospace font is a font designed for text display where the the
 * characters form a regular grid.
 *
 * For Western languages this would
 * mean that the advance width of all characters are the same, but
 * this categorization also includes Asian fonts which include
 * double-width characters: characters that occupy two grid cells.
 * g_unichar_iswide() returns a result that indicates whether a
 * character is typically double-width in a monospace font.
 *
 * The best way to find out the grid-cell size is to call
 * [method@Pango.FontMetrics.get_approximate_digit_width], since the
 * results of [method@Pango.FontMetrics.get_approximate_char_width] may
 * be affected by double-width characters.
 *
 * Return value: %TRUE if the family is monospace.
 *
 * Since: 1.4
 */
gboolean
pango_font_family_is_monospace (PangoFontFamily  *family)
{
  g_return_val_if_fail (PANGO_IS_FONT_FAMILY (family), FALSE);

  return PANGO_FONT_FAMILY_GET_CLASS (family)->is_monospace (family);
}

/**
 * pango_font_family_is_variable:
 * @family: a `PangoFontFamily`
 *
 * A variable font is a font which has axes that can be modified to
 * produce different faces.
 *
 * Such axes are also known as _variations_; see
 * [method@Pango.FontDescription.set_variations] for more information.
 *
 * Return value: %TRUE if the family is variable
 *
 * Since: 1.44
 */
gboolean
pango_font_family_is_variable (PangoFontFamily  *family)
{
  g_return_val_if_fail (PANGO_IS_FONT_FAMILY (family), FALSE);

  return PANGO_FONT_FAMILY_GET_CLASS (family)->is_variable (family);
}
