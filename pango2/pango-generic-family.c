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

#include <gio/gio.h>

#include "pango-generic-family-private.h"
#include "pango-hbfamily-private.h"
#include "pango-impl-utils.h"
#include "pango-hbface-private.h"
#include "pango-font-private.h"

/**
 * Pango2GenericFamily:
 *
 * An implementation of `Pango2FontFamily`
 * that provides faces from other families.
 *
 * `Pango2GenericFamily can be used to e.g. assemble a
 * generic 'sans-serif' family from a number of other
 * font families.
 */


/* {{{ GListModel implementation */

static GType
pango2_generic_family_get_item_type (GListModel *list)
{
  return PANGO2_TYPE_FONT_FACE;
}

static guint
pango2_generic_family_get_n_items (GListModel *list)
{
  Pango2GenericFamily *self = PANGO2_GENERIC_FAMILY (list);
  guint n;

  n = 0;
  for (int i = 0; i < self->families->len; i++)
    {
      Pango2HbFamily *family = g_ptr_array_index (self->families, i);

      n += g_list_model_get_n_items (G_LIST_MODEL (family));
    }

  return n;
}

static gpointer
pango2_generic_family_get_item (GListModel *list,
                                guint       position)
{
  Pango2GenericFamily *self = PANGO2_GENERIC_FAMILY (list);
  guint pos;

  pos = position;
  for (int i = 0; i < self->families->len; i++)
    {
      Pango2HbFamily *family = g_ptr_array_index (self->families, i);

      if (pos < g_list_model_get_n_items (G_LIST_MODEL (family)))
        return g_list_model_get_item (G_LIST_MODEL (family), pos);

      pos -= g_list_model_get_n_items (G_LIST_MODEL (family));
    }

  return NULL;
}

static void
pango2_generic_family_list_model_init (GListModelInterface *iface)
{
  iface->get_item_type = pango2_generic_family_get_item_type;
  iface->get_n_items = pango2_generic_family_get_n_items;
  iface->get_item = pango2_generic_family_get_item;
}

 /* }}} */
/* {{{ Family list model implementation */

G_DECLARE_FINAL_TYPE (Pango2FamilyModel, pango2_family_model, PANGO2, FAMILY_MODEL, GObject)

struct _Pango2FamilyModel {
  GObject parent;
  Pango2GenericFamily *family;
};

struct _Pango2FamilyModelClass {
  GObjectClass parent_class;
};

static GType
pango2_family_model_get_item_type (GListModel *list)
{
  return PANGO2_TYPE_FONT_FAMILY;
}

static guint
pango2_family_model_get_n_items (GListModel *list)
{
  Pango2FamilyModel *self = PANGO2_FAMILY_MODEL (list);

  return self->family->families->len;
}

static gpointer
pango2_family_model_get_item (GListModel *list,
                              guint       position)
{
  Pango2FamilyModel *self = PANGO2_FAMILY_MODEL (list);

  if (position < self->family->families->len)
    return g_object_ref (g_ptr_array_index (self->family->families, position));

  return NULL;
}

static void
pango2_family_model_list_model_init (GListModelInterface *iface)
{
  iface->get_item_type = pango2_family_model_get_item_type;
  iface->get_n_items = pango2_family_model_get_n_items;
  iface->get_item = pango2_family_model_get_item;
}

G_DEFINE_TYPE_WITH_CODE (Pango2FamilyModel, pango2_family_model, G_TYPE_OBJECT,
                         G_IMPLEMENT_INTERFACE (G_TYPE_LIST_MODEL, pango2_family_model_list_model_init))

static void
pango2_family_model_init (Pango2FamilyModel *self)
{
}

static void
pango2_family_model_finalize (GObject *object)
{
  Pango2FamilyModel *self = PANGO2_FAMILY_MODEL (object);

  g_object_unref (self->family);

  G_OBJECT_CLASS (pango2_family_model_parent_class)->finalize (object);
}

static void
pango2_family_model_class_init (Pango2FamilyModelClass *class)
{
  G_OBJECT_CLASS (class)->finalize = pango2_family_model_finalize;
}

/* }}} */
/* {{{ Pango2FontFamily implementation */

struct _Pango2GenericFamilyClass
{
  Pango2FontFamilyClass parent_class;
};

G_DEFINE_FINAL_TYPE_WITH_CODE (Pango2GenericFamily, pango2_generic_family, PANGO2_TYPE_FONT_FAMILY,
                               G_IMPLEMENT_INTERFACE (G_TYPE_LIST_MODEL, pango2_generic_family_list_model_init))

static void
pango2_generic_family_init (Pango2GenericFamily *self)
{
  self->families = g_ptr_array_new_with_free_func (g_object_unref);
}

static void
pango2_generic_family_finalize (GObject *object)
{
  Pango2GenericFamily *self = PANGO2_GENERIC_FAMILY (object);

  g_ptr_array_unref (self->families);

  G_OBJECT_CLASS (pango2_generic_family_parent_class)->finalize (object);
}

static void
pango2_generic_family_class_init (Pango2GenericFamilyClass *class)
{
  GObjectClass *object_class = G_OBJECT_CLASS (class);

  object_class->finalize = pango2_generic_family_finalize;
}

/* }}} */
/* {{{ Private API */

/*< private >
 * pango2_generic_family_find_face:
 * @family: a `Pango2GenericFamily`
 * @description: `Pango2FontDescription` to match
 * @language: (nullable): `Pango2Language` to support
 * @wc: a Unicode character, or 0 to ignore
 *
 * Finds the face that best matches the font description while
 * also supporting the given language and character, from the first
 * family in @family that has any matching faces.
 *
 * Returns: (transfer none) (nullable): the face
 */
Pango2FontFace *
pango2_generic_family_find_face (Pango2GenericFamily   *self,
                                 Pango2FontDescription *description,
                                 Pango2Language        *language,
                                 gunichar               wc)
{
  Pango2FontFace *face = NULL;

  for (int i = 0; i < self->families->len; i++)
    {
      Pango2HbFamily *family = g_ptr_array_index (self->families, i);

      face = pango2_hb_family_find_face (family, description, language, wc);
      if (face)
        break;
    }

  /* Try without language */
  for (int i = 0; i < self->families->len; i++)
    {
      Pango2HbFamily *family = g_ptr_array_index (self->families, i);

      face = pango2_hb_family_find_face (family, description, NULL, wc);
      if (face)
        break;
    }

  /* last resort */
  if (!face && self->families->len > 0)
    {
      Pango2HbFamily *family = g_ptr_array_index (self->families, 0);
      face = g_list_model_get_item (G_LIST_MODEL (family), 0);
      g_object_unref (face);
    }

  return face;
}

/* }}} */
/* {{{ Public API */

/**
 * pango2_generic_family_new:
 * @name: the family name
 *
 * Creates a new `Pango2GenericFamily`.
 *
 * A generic family does not contain faces, but will return
 * faces from other families that match a given query.
 *
 * Returns: a newly created `Pango2GenericFamily`
 */
Pango2GenericFamily *
pango2_generic_family_new (const char *name)
{
  Pango2GenericFamily *self;

  g_return_val_if_fail (name != NULL, NULL);

  self = g_object_new (PANGO2_TYPE_GENERIC_FAMILY, NULL);

  pango2_font_family_set_name (PANGO2_FONT_FAMILY (self), name);

  return self;
}

/**
 * pango2_generic_family_add_family:
 * @self: a `Pango2GenericFamily`
 * @family: (transfer none): a `Pango2FontFamily` to add
 *
 * Adds a `Pango2FontFamily` to a `Pango2GenericFamily`.
 *
 * It is an error to call this function more than
 * once for the same family.
 */
void
pango2_generic_family_add_family (Pango2GenericFamily *self,
                                  Pango2FontFamily    *family)
{
  g_return_if_fail (PANGO2_IS_GENERIC_FAMILY (self));
  g_return_if_fail (PANGO2_IS_FONT_FAMILY (family));

  g_ptr_array_add (self->families, g_object_ref (family));
}

/**
 * pango2_generic_family_get_families:
 * @self: a `Pango2GenericFamily`
 *
 * Returns a list model of the families contained in the generic family.
 *
 * Returns: (transfer full): a list model of families
 */
GListModel *
pango2_generic_family_get_families (Pango2GenericFamily *self)
{
  Pango2FamilyModel *model;

  model = g_object_new (pango2_family_model_get_type (), NULL);
  model->family = g_object_ref (self);

  return G_LIST_MODEL (model);
}

/* }}} */

/* vim:set foldmethod=marker expandtab: */
