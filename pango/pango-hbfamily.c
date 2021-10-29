/* Pango
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

/* {{{ GListModel implementation */

static GType
pango_hb_family_get_item_type (GListModel *list)
{
  return PANGO_TYPE_FONT_FACE;
}

static guint
pango_hb_family_get_n_items (GListModel *list)
{
  PangoHbFamily *self = PANGO_HB_FAMILY (list);

  return self->faces->len;
}

static gpointer
pango_hb_family_get_item (GListModel *list,
                              guint       position)
{
  PangoHbFamily *self = PANGO_HB_FAMILY (list);

  if (position < self->faces->len)
    return g_object_ref (g_ptr_array_index (self->faces, position));

  return NULL;
}

static void
pango_hb_family_list_model_init (GListModelInterface *iface)
{
  iface->get_item_type = pango_hb_family_get_item_type;
  iface->get_n_items = pango_hb_family_get_n_items;
  iface->get_item = pango_hb_family_get_item;
}

/* }}} */
/* {{{ Utilities */

static int
sort_face_func (PangoHbFace *face1,
                PangoHbFace *face2)
{
  int a, b;

  a = pango_font_description_get_style (face1->description);
  b = pango_font_description_get_style (face2->description);
  if (a != b)
    return a - b;

  a = pango_font_description_get_weight (face1->description);
  b = pango_font_description_get_weight (face2->description);
  if (a != b)
    return a - b;

  a = pango_font_description_get_stretch (face1->description);
  b = pango_font_description_get_stretch (face2->description);
  if (a != b)
    return a - b;

  return strcmp (face1->name, face2->name);
}

/* return 2 if face is a named instance,
 * 1 if it is variable
 * 0 otherwise
 */
static int
hb_face_get_variableness (PangoHbFace *face)
{
  if (pango_font_face_is_variable (PANGO_FONT_FACE (face)))
    {
      if (face->instance_id != -1)
        return 2;
      else
        return 1;
    }
 return 0;
}

/* }}} */
/* {{{ PangoFontFamily implementation */

struct _PangoHbFamilyClass
{
  PangoFontFamilyClass parent_class;
};

G_DEFINE_TYPE_WITH_CODE (PangoHbFamily, pango_hb_family, PANGO_TYPE_FONT_FAMILY,
                         G_IMPLEMENT_INTERFACE (G_TYPE_LIST_MODEL, pango_hb_family_list_model_init))

static void
pango_hb_family_init (PangoHbFamily *self)
{
  self->faces = g_ptr_array_new_with_free_func (g_object_unref);
}

static void
pango_hb_family_finalize (GObject *object)
{
  PangoHbFamily *self = PANGO_HB_FAMILY (object);

  g_free (self->name);
  g_ptr_array_unref (self->faces);

  G_OBJECT_CLASS (pango_hb_family_parent_class)->finalize (object);
}

static const char *
pango_hb_family_get_name (PangoFontFamily *family)
{
  PangoHbFamily *self = PANGO_HB_FAMILY (family);

  return self->name;
}

static PangoFontFace *
pango_hb_family_get_face (PangoFontFamily *family,
                          const char      *name)
{
  PangoHbFamily *self = PANGO_HB_FAMILY (family);

  for (int i = 0; i < self->faces->len; i++)
    {
      PangoFontFace *face = g_ptr_array_index (self->faces, i);

      if (name == NULL || strcmp (name, pango_font_face_get_face_name (face)) == 0)
        return face;
    }

  return NULL;
}

static gboolean
pango_hb_family_is_monospace (PangoFontFamily *family)
{
  PangoHbFamily *self = PANGO_HB_FAMILY (family);

  for (int i = 0; i < self->faces->len; i++)
    {
      PangoFontFace *face = g_ptr_array_index (self->faces, i);

      if (pango_font_face_is_monospace (face))
        return TRUE;
    }

  return FALSE;
}

static gboolean
pango_hb_family_is_variable (PangoFontFamily *family)
{
  PangoHbFamily *self = PANGO_HB_FAMILY (family);

  for (int i = 0; i < self->faces->len; i++)
    {
      PangoFontFace *face = g_ptr_array_index (self->faces, i);

      if (pango_font_face_is_variable (face))
        return TRUE;
    }

  return FALSE;
}

static void
pango_hb_family_class_init (PangoHbFamilyClass *class)
{
  GObjectClass *object_class = G_OBJECT_CLASS (class);
  PangoFontFamilyClass *family_class = PANGO_FONT_FAMILY_CLASS (class);

  object_class->finalize = pango_hb_family_finalize;

  family_class->get_name = pango_hb_family_get_name;
  family_class->get_face = pango_hb_family_get_face;
  family_class->is_monospace = pango_hb_family_is_monospace;
  family_class->is_variable = pango_hb_family_is_variable;
}

/* }}} */
/* {{{ Private API */

/*< private >
 * pango_hb_family_new:
 * @name: the family name
 *
 * Creates a new `PangoHbFamily`.
 *
 * Returns: a newly created `PangoHbFamily`
 */
PangoHbFamily *
pango_hb_family_new (const char *name)
{
  PangoHbFamily *self;

  self = g_object_new (PANGO_TYPE_HB_FAMILY, NULL);

  self->name = g_strdup (name);

  return self;
}

/*< private >
 * pango_hb_family_set_font_map:
 * @self: a `PangoHbFamily`
 * @map: (nullable): a `PangoFontMap`
 *
 * Sets the map of @self.
 */
void
pango_hb_family_set_font_map (PangoHbFamily *self,
                              PangoFontMap  *map)
{
  if (self->map)
    g_object_remove_weak_pointer (G_OBJECT (self->map), (gpointer *)&self->map);

  self->map = map;

  if (self->map)
    g_object_add_weak_pointer (G_OBJECT (self->map), (gpointer *)&self->map);
}

/*< private >
 * pango_hb_family_add_face:
 * @self: a `PangoHbFamily`
 * @face: (transfer full): a `PangoFontFace` to add
 *
 * Adds a `PangoFontFace` to a `PangoHbFamily`.
 *
 * The family takes ownership of the added face.
 *
 * It is an error to call this function more than
 * once for the same face.
 */
void
pango_hb_family_add_face (PangoHbFamily *self,
                          PangoHbFace   *face)
{
  int position;

  position = 0;
  while (position < self->faces->len)
    {
      PangoHbFace *f = g_ptr_array_index (self->faces, position);
      if (sort_face_func (face, f) < 0)
        break;
      position++;
    }

  g_ptr_array_insert (self->faces, position, face);
  pango_hb_face_set_family (face, PANGO_FONT_FAMILY (self));

  g_list_model_items_changed (G_LIST_MODEL (self), position, 0, 1);
}

/*< private >
 * pango_hb_family_remove_face:
 * @self: a `PangoHbFamily`
 * @face: a `PangoHbFace`
 *
 * Remove a `PangoFontFace` from a `PangoHbFamily`.
 */
void
pango_hb_family_remove_face (PangoHbFamily *self,
                             PangoHbFace   *face)
{
  unsigned int position;

  if (!g_ptr_array_find (self->faces, face, &position))
    return;

  pango_hb_face_set_family (face, NULL);
  g_ptr_array_remove_index (self->faces, position);

  g_list_model_items_changed (G_LIST_MODEL (self), position, 1, 0);
}

/*< private >
 * pango_hb_family_find_face:
 * @family: a `PangoHbFamily`
 * @description: `PangoFontDescription` to match
 * @language: (nullable): `PangoLanguage` to support
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
PangoHbFace *
pango_hb_family_find_face (PangoHbFamily        *family,
                           PangoFontDescription *description,
                           PangoLanguage        *language,
                           gunichar              wc)
{
  PangoHbFace *face = NULL;
  int best_distance = G_MAXINT;
  int best_variableness = 0;

  /* First look for an exact match if the description has a faceid */
  if (pango_font_description_get_set_fields (description) & PANGO_FONT_MASK_FACEID)
    {
      for (int i = 0; i < family->faces->len; i++)
        {
          PangoHbFace *face2 = g_ptr_array_index (family->faces, i);

          if (g_strcmp0 (pango_font_description_get_faceid (description),
                         pango_hb_face_get_faceid (face2)) == 0)
            return face2;
        }
    }

  for (int i = 0; i < family->faces->len; i++)
    {
      PangoHbFace *face2 = g_ptr_array_index (family->faces, i);
      int distance;
      int variableness;

      if (language && !pango_font_face_supports_language (PANGO_FONT_FACE (face2), language))
        continue;

      if (wc && !pango_hb_face_has_char (face2, wc))
        continue;

      if (!pango_font_description_is_similar (description, face2->description))
        continue;

      distance = pango_font_description_compute_distance (description, face2->description);

      variableness = hb_face_get_variableness (face2);
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
