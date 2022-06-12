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
#include "pango-font-face.h"
#include "pango-font.h"

/**
 * PangoFontFamily:
 *
 * A `PangoFontFamily` is used to represent a family of related
 * font faces.
 *
 * The font faces in a family share a common design, but differ in
 * slant, weight, width or other aspects.
 *
 * `PangoFontFamily` implements the [iface@Gio.ListModel] interface,
 * to provide a list of font faces.
 */

static GType
pango_font_family_get_item_type (GListModel *list)
{
  return PANGO_TYPE_FONT_FACE;
}

static guint
pango_font_family_get_n_items (GListModel *list)
{
  g_assert_not_reached ();
  return 0;
}

static gpointer
pango_font_family_get_item (GListModel *list,
                            guint       position)
{
  g_assert_not_reached ();
  return NULL;
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

static void
pango_font_family_class_init (PangoFontFamilyClass *class G_GNUC_UNUSED)
{
  class->get_face = pango_font_family_real_get_face;
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

static PangoFontFace *
pango_font_family_real_get_face (PangoFontFamily *family,
                                 const char      *name)
{
  PangoFontFace *face;

  face = NULL;

  for (int i = 0; i < g_list_model_get_n_items (G_LIST_MODEL (family)); i++)
    {
      PangoFontFace *f = g_list_model_get_item (G_LIST_MODEL (family), i);
      g_object_unref (f);
      if (name == NULL ||
          strcmp (name, pango_font_face_get_face_name (f)) == 0)
        {
          face = f;
          break;
        }
    }

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
 */
PangoFontFace *
pango_font_family_get_face (PangoFontFamily *family,
                            const char      *name)
{
  g_return_val_if_fail (PANGO_IS_FONT_FAMILY (family), NULL);

  return PANGO_FONT_FAMILY_GET_CLASS (family)->get_face (family, name);
}
