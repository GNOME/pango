/* Pango2
 *
 * Copyright (C) 2021 Matthias Clasen
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

#include "pango-hbfamily-private.h"
#include "pango-impl-utils.h"
#include "pango-hbface-private.h"
#include "pango-font-face-private.h"
#include "pango-font-description-private.h"
#include "pango-userface-private.h"

/* {{{ GListModel implementation */

static GType
pango2_hb_family_get_item_type (GListModel *list)
{
  return PANGO2_TYPE_FONT_FACE;
}

static guint
pango2_hb_family_get_n_items (GListModel *list)
{
  Pango2HbFamily *self = PANGO2_HB_FAMILY (list);

  return self->faces->len;
}

static gpointer
pango2_hb_family_get_item (GListModel *list,
                           guint       position)
{
  Pango2HbFamily *self = PANGO2_HB_FAMILY (list);

  if (position < self->faces->len)
    return g_object_ref (g_ptr_array_index (self->faces, position));

  return NULL;
}

static void
pango2_hb_family_list_model_init (GListModelInterface *iface)
{
  iface->get_item_type = pango2_hb_family_get_item_type;
  iface->get_n_items = pango2_hb_family_get_n_items;
  iface->get_item = pango2_hb_family_get_item;
}

/* }}} */
/* {{{ Utilities */

static int
sort_face_func (Pango2FontFace *face1,
                Pango2FontFace *face2)
{
  int a, b;

  a = pango2_font_description_get_style (face1->description);
  b = pango2_font_description_get_style (face2->description);
  if (a != b)
    return a - b;

  a = pango2_font_description_get_weight (face1->description);
  b = pango2_font_description_get_weight (face2->description);
  if (a != b)
    return a - b;

  a = pango2_font_description_get_stretch (face1->description);
  b = pango2_font_description_get_stretch (face2->description);
  if (a != b)
    return a - b;

  return strcmp (face1->name, face2->name);
}

/* return 2 if face is a named instance,
 * 1 if it is variable
 * 0 otherwise
 */
static int
face_get_variableness (Pango2FontFace *face)
{
  if (pango2_font_face_is_variable (PANGO2_FONT_FACE (face)))
    {
      if (PANGO2_HB_FACE (face)->instance_id != -1)
        return 2;
      else
        return 1;
    }
 return 0;
}

/* }}} */
/* {{{ Pango2FontFamily implementation */

struct _Pango2HbFamilyClass
{
  Pango2FontFamilyClass parent_class;
};

G_DEFINE_TYPE_WITH_CODE (Pango2HbFamily, pango2_hb_family, PANGO2_TYPE_FONT_FAMILY,
                         G_IMPLEMENT_INTERFACE (G_TYPE_LIST_MODEL, pango2_hb_family_list_model_init))

static void
pango2_hb_family_init (Pango2HbFamily *self)
{
  self->faces = g_ptr_array_new_with_free_func (g_object_unref);
}

static void
pango2_hb_family_finalize (GObject *object)
{
  Pango2HbFamily *self = PANGO2_HB_FAMILY (object);

  for (int i = 0; i < self->faces->len; i++)
    {
      Pango2FontFace *face = g_ptr_array_index (self->faces, i);
      face->family = NULL;
    }

  g_ptr_array_unref (self->faces);

  G_OBJECT_CLASS (pango2_hb_family_parent_class)->finalize (object);
}

static Pango2FontFace *
pango2_hb_family_get_face (Pango2FontFamily *family,
                           const char       *name)
{
  Pango2HbFamily *self = PANGO2_HB_FAMILY (family);

  for (int i = 0; i < self->faces->len; i++)
    {
      Pango2FontFace *face = g_ptr_array_index (self->faces, i);

      if (name == NULL || strcmp (name, pango2_font_face_get_name (face)) == 0)
        return face;
    }

  return NULL;
}

static void
pango2_hb_family_class_init (Pango2HbFamilyClass *class)
{
  GObjectClass *object_class = G_OBJECT_CLASS (class);
  Pango2FontFamilyClass *family_class = PANGO2_FONT_FAMILY_CLASS (class);

  object_class->finalize = pango2_hb_family_finalize;

  family_class->get_face = pango2_hb_family_get_face;
}

/* }}} */
/* {{{ Private API */

/*< private >
 * pango2_hb_family_new:
 * @name: the family name
 *
 * Creates a new `Pango2HbFamily`.
 *
 * Returns: a newly created `Pango2HbFamily`
 */
Pango2HbFamily *
pango2_hb_family_new (const char *name)
{
  Pango2HbFamily *self;

  self = g_object_new (PANGO2_TYPE_HB_FAMILY, NULL);

  pango2_font_family_set_name (PANGO2_FONT_FAMILY (self), name);

  return self;
}

/*< private >
 * pango2_hb_family_add_face:
 * @self: a `Pango2HbFamily`
 * @face: (transfer full): a `Pango2FontFace` to add
 *
 * Adds a `Pango2FontFace` to a `Pango2HbFamily`.
 *
 * The family takes ownership of the added face.
 *
 * It is an error to call this function more than
 * once for the same face.
 */
void
pango2_hb_family_add_face (Pango2HbFamily *self,
                           Pango2FontFace *face)
{
  int position;

  g_return_if_fail (PANGO2_IS_HB_FACE (face) || PANGO2_IS_USER_FACE (face));

  position = 0;
  while (position < self->faces->len)
    {
      Pango2FontFace *f = g_ptr_array_index (self->faces, position);
      if (sort_face_func (face, f) < 0)
        break;
      position++;
    }

  g_ptr_array_insert (self->faces, position, face);

  pango2_font_face_set_family (face, PANGO2_FONT_FAMILY (self));

  g_list_model_items_changed (G_LIST_MODEL (self), position, 0, 1);
  g_object_notify (G_OBJECT (self), "n-items");
}

/*< private >
 * pango2_hb_family_remove_face:
 * @self: a `Pango2HbFamily`
 * @face: a `Pango2FontFace`
 *
 * Remove a `Pango2FontFace` from a `Pango2HbFamily`.
 */
void
pango2_hb_family_remove_face (Pango2HbFamily *self,
                              Pango2FontFace *face)
{
  unsigned int position;

  g_return_if_fail (PANGO2_IS_HB_FACE (face) || PANGO2_IS_USER_FACE (face));

  if (!g_ptr_array_find (self->faces, face, &position))
    return;

  pango2_font_face_set_family (face, NULL);

  g_ptr_array_remove_index (self->faces, position);

  g_list_model_items_changed (G_LIST_MODEL (self), position, 1, 0);
  g_object_notify (G_OBJECT (self), "n-items");
}

/*< private >
 * pango2_hb_family_find_face:
 * @family: a `Pango2HbFamily`
 * @description: `Pango2FontDescription` to match
 * @language: (nullable): `Pango2Language` to support
 * @wc: a Unicode character, or 0 to ignore
 *
 * Finds the face in @family that best matches the font description while
 * also supporting the given language and character.
 *
 * Amongst equally matching faces, a variable face is preferred over a
 * non-variable one, and a named instance is preferred over a bare variable
 * face.
 *
 * Returns: (transfer none) (nullable): the face
 */
Pango2FontFace *
pango2_hb_family_find_face (Pango2HbFamily        *family,
                            Pango2FontDescription *description,
                            Pango2Language        *language,
                            gunichar               wc)
{
  Pango2FontFace *face = NULL;
  int best_distance = G_MAXINT;
  int best_variableness = 0;

  /* First look for an exact match if the description has a faceid */
  if (pango2_font_description_get_set_fields (description) & PANGO2_FONT_MASK_FACEID)
    {
      const char *faceid = pango2_font_description_get_faceid (description);

      for (int i = 0; i < family->faces->len; i++)
        {
          Pango2FontFace *face2 = g_ptr_array_index (family->faces, i);
          const char *faceid2 = pango2_font_face_get_faceid (face2);

          if (g_strcmp0 (faceid, faceid2) == 0)
            return face2;
        }
    }

  for (int i = 0; i < family->faces->len; i++)
    {
      Pango2FontFace *face2 = g_ptr_array_index (family->faces, i);
      int distance;
      int variableness;

      if (language && !pango2_font_face_supports_language (face2, language))
        continue;

      if (wc && !pango2_font_face_has_char (face2, wc))
        continue;

      if (!pango2_font_description_is_similar (description, face2->description))
        continue;

      distance = pango2_font_description_compute_distance (description, face2->description);

      variableness = face_get_variableness (PANGO2_FONT_FACE (face2));
      if (distance < best_distance ||
          (distance == best_distance && variableness > best_variableness))
        {
          face = face2;
          best_distance = distance;
          best_variableness = variableness;
        }
    }

  return face;
}

/* }}} */

/* vim:set foldmethod=marker expandtab: */
