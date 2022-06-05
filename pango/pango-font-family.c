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

static gboolean
pango_font_family_default_is_generic (PangoFontFamily *family)
{
  return FALSE;
}

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
pango_font_family_class_init (PangoFontFamilyClass *class G_GNUC_UNUSED)
{
  class->is_generic = pango_font_family_default_is_generic;
  class->is_monospace = pango_font_family_default_is_monospace;
  class->is_variable = pango_font_family_default_is_variable;
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

/**
 * pango_font_family_is_generic:
 * @family: a `PangoFontFamily`
 *
 * A generic family is using a generic name such as 'sans' or
 * 'monospace', and collects fonts matching those characteristics.
 *
 * Generic families are often used as fallback.
 *
 * Return value: %TRUE if the family is generic
 */
gboolean
pango_font_family_is_generic (PangoFontFamily  *family)
{
  g_return_val_if_fail (PANGO_IS_FONT_FAMILY (family), FALSE);

  return PANGO_FONT_FAMILY_GET_CLASS (family)->is_generic (family);
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
 */
gboolean
pango_font_family_is_variable (PangoFontFamily  *family)
{
  g_return_val_if_fail (PANGO_IS_FONT_FAMILY (family), FALSE);

  return PANGO_FONT_FAMILY_GET_CLASS (family)->is_variable (family);
}
