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

#include <gio/gio.h>

#include "pango-generic-family-private.h"
#include "pango-impl-utils.h"
#include "pango-hbface-private.h"
#include "pango-font-private.h"

/**
 * PangoGenericFamily:
 *
 * An implementation of `PangoFontFamily`
 * that provides faces from other families.
 *
 * `PangoGenericFamily can be used to e.g. assemble a
 * generic 'sans-serif' family from a number of other
 * font families.
 */


/* {{{ GListModel implementation */

static GType
pango_generic_family_get_item_type (GListModel *list)
{
  return PANGO_TYPE_FONT_FACE;
}

static guint
pango_generic_family_get_n_items (GListModel *list)
{
  PangoGenericFamily *self = PANGO_GENERIC_FAMILY (list);
  guint n;

  n = 0;
  for (int i = 0; i < self->families->len; i++)
    {
      PangoHbFamily *family = g_ptr_array_index (self->families, i);

      n += g_list_model_get_n_items (G_LIST_MODEL (family));
    }

  return n;
}

static gpointer
pango_generic_family_get_item (GListModel *list,
                               guint       position)
{
  PangoGenericFamily *self = PANGO_GENERIC_FAMILY (list);
  guint pos;

  pos = position;
  for (int i = 0; i < self->families->len; i++)
    {
      PangoHbFamily *family = g_ptr_array_index (self->families, i);

      if (pos < g_list_model_get_n_items (G_LIST_MODEL (family)))
        return g_list_model_get_item (G_LIST_MODEL (family), pos);

      pos -= g_list_model_get_n_items (G_LIST_MODEL (family));
    }

  return NULL;
}

static void
pango_generic_family_list_model_init (GListModelInterface *iface)
{
  iface->get_item_type = pango_generic_family_get_item_type;
  iface->get_n_items = pango_generic_family_get_n_items;
  iface->get_item = pango_generic_family_get_item;
}

/* }}} */
/* {{{ PangoFontFamily implementation */

struct _PangoGenericFamilyClass
{
  PangoFontFamilyClass parent_class;
};

G_DEFINE_TYPE_WITH_CODE (PangoGenericFamily, pango_generic_family, PANGO_TYPE_FONT_FAMILY,
                         G_IMPLEMENT_INTERFACE (G_TYPE_LIST_MODEL, pango_generic_family_list_model_init))

static void
pango_generic_family_init (PangoGenericFamily *self)
{
  self->families = g_ptr_array_new_with_free_func (g_object_unref);
}

static void
pango_generic_family_finalize (GObject *object)
{
  PangoGenericFamily *self = PANGO_GENERIC_FAMILY (object);

  g_free (self->name);
  g_ptr_array_unref (self->families);

  G_OBJECT_CLASS (pango_generic_family_parent_class)->finalize (object);
}

static const char *
pango_generic_family_get_name (PangoFontFamily *family)
{
  PangoGenericFamily *self = PANGO_GENERIC_FAMILY (family);

  return self->name;
}

static void
pango_generic_family_class_init (PangoGenericFamilyClass *class)
{
  GObjectClass *object_class = G_OBJECT_CLASS (class);
  PangoFontFamilyClass *family_class = PANGO_FONT_FAMILY_CLASS (class);

  object_class->finalize = pango_generic_family_finalize;

  family_class->get_name = pango_generic_family_get_name;
}

/* }}} */
/* {{{ Private API */

/*< private >
 * pango_generic_family_find_face:
 * @family: a `PangoGenericFamily`
 * @description: `PangoFontDescription` to match
 * @language: (nullable): `PangoLanguage` to support
 * @wc: a Unicode character, or 0 to ignore
 *
 * Finds the face that best matches the font description while
 * also supporting the given language and character, from the first
 * family in @family that has any matching faces.
 *
 * Returns: (transfer none) (nullable): the face
 */
PangoFontFace *
pango_generic_family_find_face (PangoGenericFamily   *self,
                                PangoFontDescription *description,
                                PangoLanguage        *language,
                                gunichar              wc)
{
  PangoFontFace *face = NULL;

  for (int i = 0; i < self->families->len; i++)
    {
      PangoHbFamily *family = g_ptr_array_index (self->families, i);

      face = pango_hb_family_find_face (family, description, language, wc);
      if (face)
        break;
    }

  /* last resort */
  if (!face && self->families->len > 0)
    {
      PangoHbFamily *family = g_ptr_array_index (self->families, 0);
      face = g_list_model_get_item (G_LIST_MODEL (family), 0);
      g_object_unref (face);
    }

  return face;
}

/* }}} */
/* {{{ Public API */

/**
 * pango_generic_family_new:
 * @name: the family name
 *
 * Creates a new `PangoGenericFamily`.
 *
 * A generic family does not contain faces, but will return
 * faces from other families that match a given query.
 *
 * Returns: a newly created `PangoGenericFamily`
 */
PangoGenericFamily *
pango_generic_family_new (const char *name)
{
  PangoGenericFamily *self;

  g_return_val_if_fail (name != NULL, NULL);

  self = g_object_new (PANGO_TYPE_GENERIC_FAMILY, NULL);

  self->name = g_strdup (name);

  return self;
}

/**
 * pango_generic_family_add_family:
 * @self: a `PangoGenericFamily`
 * @family: (transfer none): a `PangoFontFamily` to add
 *
 * Adds a `PangoFontFamily` to a `PangoGenericFamily`.
 *
 * It is an error to call this function more than
 * once for the same family.
 */
void
pango_generic_family_add_family (PangoGenericFamily *self,
                                 PangoFontFamily    *family)
{
  g_return_if_fail (PANGO_IS_GENERIC_FAMILY (self));
  g_return_if_fail (PANGO_IS_FONT_FAMILY (family));

  g_ptr_array_add (self->families, g_object_ref (family));
}

/* }}} */

/* vim:set foldmethod=marker expandtab: */
