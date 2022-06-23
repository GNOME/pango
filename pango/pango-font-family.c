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

/* {{{ GListModel implementation */

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

/* }}} */
/* {{{ PangoFontFamily implementation */

enum {
  PROP_NAME = 1,
  PROP_ITEM_TYPE,
  PROP_N_ITEMS,
  N_PROPERTIES
};

static GParamSpec *properties[N_PROPERTIES] = { NULL, };

G_DEFINE_ABSTRACT_TYPE_WITH_CODE (PangoFontFamily, pango_font_family, G_TYPE_OBJECT,
                                  G_IMPLEMENT_INTERFACE (G_TYPE_LIST_MODEL, pango_font_family_list_model_init))

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
          strcmp (name, pango_font_face_get_name (f)) == 0)
        {
          face = f;
          break;
        }
    }

  return face;
}

static void
pango_font_family_finalize (GObject *object)
{
  PangoFontFamily *family = PANGO_FONT_FAMILY (object);

  g_free (family->name);
  if (family->map)
    g_object_remove_weak_pointer (G_OBJECT (family->map), (gpointer *)&family->map);

  G_OBJECT_CLASS (pango_font_family_parent_class)->finalize (object);
}

static void
pango_font_family_get_property (GObject    *object,
                                guint       property_id,
                                GValue     *value,
                                GParamSpec *pspec)
{
  PangoFontFamily *family = PANGO_FONT_FAMILY (object);

  switch (property_id)
    {
    case PROP_NAME:
      g_value_set_string (value, family->name);
      break;

    case PROP_ITEM_TYPE:
      g_value_set_gtype (value, PANGO_TYPE_FONT);
      break;

    case PROP_N_ITEMS:
      g_value_set_uint (value, pango_font_family_get_n_items (G_LIST_MODEL (object)));
      break;

    default:
      G_OBJECT_WARN_INVALID_PROPERTY_ID (object, property_id, pspec);
    }
}

static void
pango_font_family_class_init (PangoFontFamilyClass *class)
{
  GObjectClass *object_class = G_OBJECT_CLASS (class);

  object_class->finalize = pango_font_family_finalize;
  object_class->get_property = pango_font_family_get_property;

  class->get_face = pango_font_family_real_get_face;

  /**
   * PangoFontFamily:name: (attributes org.gtk.Property.get=pango_font_family_get_name)
   *
   * The name of the family.
   */
  properties[PROP_NAME] =
      g_param_spec_string ("name", NULL, NULL, NULL,
                           G_PARAM_READABLE | G_PARAM_STATIC_STRINGS);

  /**
   * PangoFontFamily:item-type:
   *
   * The type of objects that the family contains.
   */
  properties[PROP_ITEM_TYPE] =
      g_param_spec_gtype ("item-type", NULL, NULL, PANGO_TYPE_FONT_FACE,
                          G_PARAM_READABLE | G_PARAM_STATIC_STRINGS);

  /**
   * PangoFontFamily:n-items:
   *
   * The number of faces contained in the family.
   */
  properties[PROP_N_ITEMS] =
      g_param_spec_uint ("n-items", "", "", 0, G_MAXUINT, 0,
                         G_PARAM_READABLE | G_PARAM_STATIC_STRINGS);

  g_object_class_install_properties (object_class, N_PROPERTIES, properties);
}

static void
pango_font_family_init (PangoFontFamily *family G_GNUC_UNUSED)
{
}

/* }}} */
/* {{{ Public API */

/**
 * pango_font_family_get_font_map:
 * @family: a `PangoFontFamily`
 *
 * Returns the `PangoFontMap that @family belongs to.
 *
 * Note that the family maintains a *weak* reference to
 * the font map, so if all references to font map are
 * dropped, the font map will be finalized even if there
 * are fonts created with the font map that are still alive.
 * In that case this function will return %NULL.
 *
 * It is the responsibility of the user to ensure that the
 * font map is kept alive. In most uses this is not an issue
 * as a `PangoContext` holds a reference to the font map.

 *
 * Return value: (transfer none) (nullable): the `PangoFontMap
 */
PangoFontMap *
pango_font_family_get_font_map (PangoFontFamily *family)
{
  return family->map;
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
pango_font_family_get_name (PangoFontFamily *family)
{
  g_return_val_if_fail (PANGO_IS_FONT_FAMILY (family), NULL);

  return family->name;
}

/**
 * pango_font_family_get_face:
 * @family: a `PangoFontFamily`
 * @name: (nullable): the name of a face. If the name is `NULL`,
 *   the family's default face (fontconfig calls it "Regular")
 *   will be returned.
 *
 * Gets the `PangoFontFace` of the family with the given name.
 *
 * Returns: (transfer none) (nullable): the `PangoFontFace`,
 *   or `NULL` if no face with the given name exists.
 */
PangoFontFace *
pango_font_family_get_face (PangoFontFamily *family,
                            const char      *name)
{
  g_return_val_if_fail (PANGO_IS_FONT_FAMILY (family), NULL);

  return PANGO_FONT_FAMILY_GET_CLASS (family)->get_face (family, name);
}

/* }}} */

/* vim:set foldmethod=marker expandtab: */
