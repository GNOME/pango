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

#include "pango-font-private.h"
#include "pango-hbface-private.h"

#include "pango-language-set-simple-private.h"

#include <string.h>
#include <hb-ot.h>

/**
 * PangoHbFace:
 *
 * `PangoHbFace` is a `PangoFontFace` implementation that wraps
 * a `hb_face_t` object and implements all of the `PangoFontFace`
 * functionality using HarfBuzz.
 *
 * In addition to making a `hb_face_t` available for rendering
 * glyphs with Pango, `PangoHbFace` allows some tweaks to the
 * rendering, such as artificial slant (using a transformation
 * matrix) or artificial emboldening.
 */

 /* {{{ Utilities */

static char *
get_name_from_hb_face (hb_face_t       *face,
                       hb_ot_name_id_t  name_id,
                       hb_ot_name_id_t  fallback_id)
{
  char buf[256];
  unsigned int len;

  len = sizeof (buf);
  if (hb_ot_name_get_utf8 (face, name_id, HB_LANGUAGE_INVALID, &len, buf) > 0)
    return g_strdup (buf);

  if (fallback_id != HB_OT_NAME_ID_INVALID)
    {
      len = sizeof (buf);
      if (hb_ot_name_get_utf8 (face, fallback_id, HB_LANGUAGE_INVALID, &len, buf) > 0)
        return g_strdup (buf);
    }

  return g_strdup ("Unnamed");
}

static void
ensure_hb_face (PangoHbFace *self)
{
  hb_blob_t *blob;

  if (self->face)
    return;

  blob = hb_blob_create_from_file (self->file);

  if (blob == hb_blob_get_empty ())
    g_warning ("Failed to load %s", self->file);

  if (self->index >= hb_face_count (blob))
    g_warning ("Face index %d out of range for %s", self->index, self->file);

  self->face = hb_face_create (blob, self->index);

  if (self->instance_id >= (int)hb_ot_var_get_named_instance_count (self->face))
    g_warning ("Instance ID %d out of range for %s", self->instance_id, self->file);

  hb_blob_destroy (blob);
  hb_face_make_immutable (self->face);
}

static char *
variations_to_string (const hb_variation_t *variations,
                      unsigned int          n_variations,
                      const char           *equals,
                      const char           *separator)
{
  GString *s;
  char buf[128] = { 0, };

  s = g_string_new ("");

  for (unsigned int i = 0; i < n_variations; i++)
    {
      if (s->len > 0)
        g_string_append (s, separator);

      hb_tag_to_string (variations[i].tag, buf);
      g_string_append (s, buf);
      g_string_append (s, equals);
      g_ascii_formatd (buf, sizeof (buf), "%g", variations[i].value);
      g_string_append (s, buf);
    }

  return g_string_free (s, FALSE);
}

static void
set_name_and_description (PangoHbFace                *self,
                          const char                 *name,
                          const PangoFontDescription *description)
{
  if (name)
    {
      self->name = g_strdup (name);
    }
  else
    {
      hb_ot_name_id_t name_id;

      ensure_hb_face (self);

      if (self->instance_id >= 0)
        name_id = hb_ot_var_named_instance_get_subfamily_name_id (self->face, self->instance_id);
      else
        name_id = HB_OT_NAME_ID_TYPOGRAPHIC_SUBFAMILY;

      self->name = get_name_from_hb_face (self->face, name_id, HB_OT_NAME_ID_FONT_SUBFAMILY);
    }

  if (description)
    {
      self->description = pango_font_description_copy (description);
    }
  else
    {
      char *family;
      char *fullname;

      ensure_hb_face (self);

      family = get_name_from_hb_face (self->face,
                                      HB_OT_NAME_ID_TYPOGRAPHIC_FAMILY,
                                      HB_OT_NAME_ID_FONT_FAMILY);
      fullname = g_strconcat (family, " ", self->name, NULL);

      self->description = pango_font_description_from_string (fullname);
      pango_font_description_unset_fields (self->description,
                                           PANGO_FONT_MASK_VARIANT |
                                           PANGO_FONT_MASK_VARIATIONS |
                                           PANGO_FONT_MASK_GRAVITY);

      g_free (fullname);
      g_free (family);
    }

  if (self->n_variations > 0)
    {
      char *str = variations_to_string (self->variations, self->n_variations, "=", ",");
      pango_font_description_set_variations (self->description, str);
      g_free (str);
    }
}

typedef struct {
  guint16 major;
  guint16 minor;
  gint32 italicAngle;
  gint16 underlinePosition;
  gint16 underlineThickness;
  guint8 isFixedPitch[4];
} PostTable;

static gboolean
hb_face_is_monospace (hb_face_t *face)
{
  hb_blob_t *post_blob;
  guint8 *raw_post;
  unsigned int post_len;
  gboolean res = FALSE;

  post_blob = hb_face_reference_table (face, HB_TAG ('p', 'o', 's', 't'));
  raw_post = (guint8 *) hb_blob_get_data (post_blob, &post_len);

  if (post_len >= sizeof (PostTable))
    {
      PostTable *post = (PostTable *)raw_post;

      res = post->isFixedPitch[0] != 0 ||
            post->isFixedPitch[1] != 0 ||
            post->isFixedPitch[2] != 0 ||
            post->isFixedPitch[3] != 0;
    }

  hb_blob_destroy (post_blob);

  return res;
}

static void
ensure_psname (PangoHbFace *self)
{
  char *p;

  if (self->psname)
    return;

  ensure_hb_face (self);

  self->psname = get_name_from_hb_face (self->face, HB_OT_NAME_ID_POSTSCRIPT_NAME, HB_OT_NAME_ID_INVALID);

  /* PostScript name should not contain problematic chars, but just in case,
   * make sure we don't have any ' ', '=' or ',' that would give us parsing
   * problems.
   */
  p = self->psname;
  while ((p = strpbrk (p, " =,")) != NULL)
    *p = '?';
}

static const char *
style_from_font_description (const PangoFontDescription *desc)
{
  PangoStyle style = pango_font_description_get_style (desc);
  PangoWeight weight = pango_font_description_get_weight (desc);

  switch (style)
    {
    case PANGO_STYLE_ITALIC:
      if (weight == PANGO_WEIGHT_BOLD)
        return "Bold Italic";
      else
        return "Italic";
      break;
    case PANGO_STYLE_OBLIQUE:
      if (weight == PANGO_WEIGHT_BOLD)
        return "Bold Oblique";
      else
        return "Oblique";
      break;
    case PANGO_STYLE_NORMAL:
      if (weight == PANGO_WEIGHT_BOLD)
        return "Bold";
      else
        return "Regular";
      break;
    default: ;
    }

  return NULL;
}

/* }}} */
/* {{{ PangoFontFace implementation */

struct _PangoHbFaceClass
{
  PangoFontFaceClass parent_class;
};

G_DEFINE_TYPE (PangoHbFace, pango_hb_face, PANGO_TYPE_FONT_FACE)

static void
pango_hb_face_init (PangoHbFace *self)
{
  self->matrix = NULL;
  self->x_scale = self->y_scale = 1.;
}

static void
pango_hb_face_finalize (GObject *object)
{
  PangoHbFace *self = PANGO_HB_FACE (object);

  hb_face_destroy (self->face);
  pango_font_description_free (self->description);
  g_free (self->name);
  g_free (self->file);
  g_free (self->faceid);
  if (self->languages)
    g_object_unref (self->languages);
  g_free (self->variations);
  if (self->matrix)
    g_free (self->matrix);

  G_OBJECT_CLASS (pango_hb_face_parent_class)->finalize (object);
}

static const char *
pango_hb_face_get_face_name (PangoFontFace *face)
{
  PangoHbFace *self = PANGO_HB_FACE (face);

  return self->name;
}

static PangoFontDescription *
pango_hb_face_describe (PangoFontFace *face)
{
  PangoHbFace *self = PANGO_HB_FACE (face);

  if ((pango_font_description_get_set_fields (self->description) & PANGO_FONT_MASK_FACEID) == 0)
    pango_font_description_set_faceid (self->description, pango_hb_face_get_faceid (self));

  return pango_font_description_copy (self->description);
}

static PangoFontFamily *
pango_hb_face_get_family (PangoFontFace *face)
{
  PangoHbFace *self = PANGO_HB_FACE (face);

  return self->family;
}

static gboolean
pango_hb_face_is_synthesized (PangoFontFace *face)
{
  PangoHbFace *self = PANGO_HB_FACE (face);

  return self->synthetic;
}

static gboolean
pango_hb_face_is_monospace (PangoFontFace *face)
{
  PangoHbFace *self = PANGO_HB_FACE (face);

  ensure_hb_face (self);

  return hb_face_is_monospace (self->face);
}

static gboolean
pango_hb_face_is_variable (PangoFontFace *face)
{
  PangoHbFace *self = PANGO_HB_FACE (face);

  /* We don't consider named instances as variable, i.e.
   * a font chooser UI should not expose axes for them.
   *
   * In theory, there could be multi-axis fonts where the
   * variations only pin some of the axes, but we are not
   * going to worry about possibility here.
   */
  if (self->instance_id >= -1 || self->n_variations)
    return FALSE;

  ensure_hb_face (self);

  return hb_ot_var_get_axis_count (self->face) > 0;
}

static gboolean
pango_hb_face_supports_language (PangoFontFace *face,
                                 PangoLanguage *language)
{
  PangoHbFace *self = PANGO_HB_FACE (face);
  PangoLanguageSet *set = pango_hb_face_get_language_set (self);

  if (set)
    return pango_language_set_matches_language (set, language);

  return TRUE;
}

static PangoLanguage **
pango_hb_face_get_languages (PangoFontFace *face)
{
  PangoHbFace *self = PANGO_HB_FACE (face);
  PangoLanguageSet *set = pango_hb_face_get_language_set (self);

  if (set)
    return pango_language_set_get_languages (set);

  return NULL;
}

static void
pango_hb_face_class_init (PangoHbFaceClass *class)
{
  GObjectClass *object_class = G_OBJECT_CLASS (class);
  PangoFontFaceClass *face_class = PANGO_FONT_FACE_CLASS (class);
  PangoFontFaceClassPrivate *pclass;

  object_class->finalize = pango_hb_face_finalize;

  face_class->get_face_name = pango_hb_face_get_face_name;
  face_class->describe = pango_hb_face_describe;
  face_class->list_sizes = NULL;
  face_class->is_synthesized = pango_hb_face_is_synthesized;
  face_class->get_family = pango_hb_face_get_family;
  face_class->is_monospace = pango_hb_face_is_monospace;
  face_class->is_variable = pango_hb_face_is_variable;

  pclass = g_type_class_get_private ((GTypeClass *) class, PANGO_TYPE_FONT_FACE);

  pclass->supports_language = pango_hb_face_supports_language;
  pclass->get_languages = pango_hb_face_get_languages;
}

/* }}} */
/* {{{ Private API */

/*< private >
 * pango_hb_face_set_family:
 * @self: a `PangoHbFace`
 * @family: a `PangoFontFamily`
 *
 * Sets the font family of a `PangoHbFace`.
 *
 * This should only be called by fontmap implementations.
 */
void
pango_hb_face_set_family (PangoHbFace     *self,
                          PangoFontFamily *family)
{
  self->family = family;
}

/*< private >
 * pango_hb_face_get_language_set:
 * @face: a `PangoHbFace`
 *
 * Returns the languages supported by @face.
 *
 * Returns: (transfer none): a `PangoLanguageSet`
 */
PangoLanguageSet *
pango_hb_face_get_language_set (PangoHbFace *face)
{
  return face->languages;
}

/*< private >
 * pango_hb_face_set_languages:
 * @self: a `PangoHbFace`
 * @languages: a `PangoLanguageSet`
 *
 * Sets the languages that are supported by @face.
 *
 * This should only be called by fontmap implementations.
 */
void
pango_hb_face_set_language_set (PangoHbFace      *self,
                                PangoLanguageSet *languages)
{
  g_set_object (&self->languages, languages);
}

/*< private  >
 * pango_hb_face_set_matrix:
 * @self: a `PangoHbFace`
 * @matrix: the `PangoMatrix`
 *
 * Sets the font matrix for @self.
 *
 * This should only be called by fontmap implementations.
 */
void
pango_hb_face_set_matrix (PangoHbFace       *self,
                          const PangoMatrix *matrix)
{
  if (!self->matrix)
    self->matrix = g_new (PangoMatrix, 1);

  *self->matrix = *matrix;

  pango_matrix_get_font_scale_factors (self->matrix, &self->x_scale, &self->y_scale);
  pango_matrix_scale (self->matrix, 1./self->x_scale, 1./self->y_scale);
}

/*< private >
 * pango_hb_face_has_char:
 * @self: a `PangoHbFace`
 * @wc: a Unicode character
 *
 * Returns whether the face provides a glyph for this character.
 *
 * Returns: `TRUE` if @font can render @wc
 */
gboolean
pango_hb_face_has_char (PangoHbFace *self,
                        gunichar     wc)
{
  hb_font_t *hb_font;
  hb_codepoint_t glyph;
  gboolean ret;

  ensure_hb_face (self);

  hb_font = hb_font_create (self->face);
  ret = hb_font_get_nominal_glyph (hb_font, wc, &glyph);
  hb_font_destroy (hb_font);

  return ret;
}

/*< private >
 * pango_hb_face_get_faceid:
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
pango_hb_face_get_faceid (PangoHbFace *self)
{
  if (!self->faceid)
    {
      double slant;
      char buf0[32], buf1[32], buf2[32];
      char *str = NULL;

      ensure_psname (self);

      if (self->matrix)
        slant = pango_matrix_get_slant_ratio (self->matrix);
      else
        slant = 0.;

      if (self->n_variations > 0)
        str = variations_to_string (self->variations, self->n_variations, "_", ":");

      self->faceid = g_strdup_printf ("hb:%s:%u:%d:%d:%s:%s:%s%s%s",
                                      self->psname,
                                      self->index,
                                      self->instance_id,
                                      self->embolden,
                                      g_ascii_formatd (buf0, sizeof (buf0), "%g", self->x_scale),
                                      g_ascii_formatd (buf1, sizeof (buf1), "%g", self->y_scale),
                                      g_ascii_formatd (buf2, sizeof (buf2), "%g", slant),
                                      self->n_variations > 0 ? ":" : "",
                                      self->n_variations > 0 ? str : "");
      g_free (str);
    }

  return self->faceid;
}

/* }}} */
 /* {{{ Public API */

/**
 * pango_hb_face_new_from_hb_face:
 * @face: an immutable `hb_face_t`
 * @instance_id: named instance id, or -1 for the default instance
 *   or -2 for no instance
 * @name: (nullable): name for the face
 * @description: (nullable): `PangoFontDescription` for the font
 *
 * Creates a new `PangoHbFace` by wrapping an existing `hb_face_t`.
 *
 * The @instance_id can be used to pick one of the available named
 * instances in a variable font. See hb_ot_var_get_named_instance_count()
 * to learn about the available named instances.
 *
 * If @instance_id is -2 and @face has variation axes, then
 * [method@Pango.FontFace.is_variable] will return `TRUE` for
 * the returned `PangoHbFace`.
 *
 * If @name is provided, it is used as the name for the face.
 * Otherwise, Pango will use the named instance subfamily name
 * or `HB_OT_NAME_ID_TYPOGRAPHIC_SUBFAMILY`.
 *
 * If @description is provided, it is used as the font description
 * for the face. Otherwise, Pango creates a description using
 * `HB_OT_NAME_ID_TYPOGRAPHIC_FAMILY` and the name of the face.
 *
 * Returns: a newly created `PangoHbFace`
 *
 * Since: 1.52
 */
PangoHbFace *
pango_hb_face_new_from_hb_face (hb_face_t                 *face,
                                int                        instance_id,
                                const char                 *name,
                                const PangoFontDescription *description)
{
  PangoHbFace *self;

  g_return_val_if_fail (hb_face_is_immutable (face), NULL);
  g_return_val_if_fail (instance_id >= -2, NULL);
  g_return_val_if_fail (description == NULL ||
                        (pango_font_description_get_set_fields (description) &
                         (PANGO_FONT_MASK_VARIANT|
                          PANGO_FONT_MASK_SIZE|
                          PANGO_FONT_MASK_GRAVITY)) == 0, NULL);

  self = g_object_new (PANGO_TYPE_HB_FACE, NULL);

  self->face = hb_face_reference (face);
  self->index = hb_face_get_index (face) & 0xffff;
  self->instance_id = instance_id;

  if (instance_id >= (int)hb_ot_var_get_named_instance_count (face))
    g_warning ("Instance ID %d out of range", instance_id);

  set_name_and_description (self, name, description);

  return self;
}

/**
 * pango_hb_face_new_from_file:
 * @file: font filename
 * @index: face index
 * @instance_id: named instance id, or -1 for the default instance
 *   or -2 for no instance
 * @name: (nullable): name for the face
 * @description: (nullable): `PangoFontDescription` for the font
 *
 * Creates a new `PangoHbFace` from a font file.
 *
 * The @instance_id can be used to pick one of the available named
 * instances in a variable font. See hb_ot_var_get_named_instance_count()
 * to learn about the available named instances.
 *
 * If @instance_id is -2 and @face has variation axes, then
 * [method@Pango.FontFace.is_variable] will return `TRUE` for
 * the returned `PangoHbFace`.
 *
 * If @name is provided, it is used as the name for the face.
 * Otherwise, Pango will use the named instance subfamily name
 * or `HB_OT_NAME_ID_TYPOGRAPHIC_SUBFAMILY`.
 *
 * If @description is provided, it is used as the font description
 * for the face. Otherwise, Pango creates a description using
 * `HB_OT_NAME_ID_TYPOGRAPHIC_FAMILY` and the name of the face.
 *
 * If @desc and @name are provided, then the returned `PangoHbFace`
 * object will be lazily initialized as needed.
 *
 * Returns: a newly created `PangoHbFace`
 *
 * Since: 1.52
 */
PangoHbFace *
pango_hb_face_new_from_file (const char                 *file,
                             unsigned int                index,
                             int                         instance_id,
                             const char                 *name,
                             const PangoFontDescription *description)
{
  PangoHbFace *self;

  g_return_val_if_fail (instance_id >= -2, NULL);
  g_return_val_if_fail (description == NULL ||
                        (pango_font_description_get_set_fields (description) &
                         (PANGO_FONT_MASK_VARIANT|
                          PANGO_FONT_MASK_SIZE|
                          PANGO_FONT_MASK_GRAVITY)) == 0, NULL);

  self = g_object_new (PANGO_TYPE_HB_FACE, NULL);

  self->file = g_strdup (file);
  self->index = index;
  self->instance_id = instance_id;

  set_name_and_description (self, name, description);

  return self;
}

/**
 * pango_hb_face_new_synthetic:
 * @face: a `PangoHbFace`
 * @transform: (nullable): the transform to apply
 * @embolden: `TRUE` to render the font bolder
 * @name: (nullable): name for the face
 * @description: a `PangoFontDescription` to override fields from @face's description
 *
 * Creates a new `PangoHbFace` that is a synthetic variant of @face.
 *
 * Here, 'synthetic' means that the variant is implemented by rendering
 * the glyphs differently, not by using data from the original @face.
 * See [ctor@Pango.HbFace.new_instance] for that.
 *
 * @transform can be used to specify a non-trivial font matrix for creating
 * synthetic italics or synthetic condensed variants of an existing face.
 *
 * If @embolden is `TRUE`, Pango will render the glyphs bolder, creating
 * a synthetic bold variant of the face.
 *
 * If a @name is not specified, the name for the face will be derived
 * from the @description.
 *
 * Apart from setting the style that this face will be used for,
 * @description can provide an alternative family name. This can
 * be used to create generic aliases such as 'sans' or 'monospace'.
 *
 * Note that only the following fields in @description should be set:
 * - style or stretch, to indicate a transformed style
 * - weight, to indicate a bolder weight
 * - family, to provide an alternative family name
 *
 * [method@Pango.Face.is_synthesized] will return `TRUE` for objects
 * created by this function.
 *
 * Returns: (transfer full): a newly created `PangoHbFace`
 *
 * Since: 1.52
 */
PangoHbFace *
pango_hb_face_new_synthetic (PangoHbFace                *face,
                             const PangoMatrix          *transform,
                             gboolean                    embolden,
                             const char                 *name,
                             const PangoFontDescription *description)
{
  PangoHbFace *self;
  PangoFontDescription *desc;

  g_return_val_if_fail (PANGO_IS_HB_FACE (face), NULL);
  g_return_val_if_fail (description != NULL, NULL);
  g_return_val_if_fail ((pango_font_description_get_set_fields (description) &
                         ~(PANGO_FONT_MASK_FAMILY|
                           PANGO_FONT_MASK_STYLE|
                           PANGO_FONT_MASK_STRETCH|
                           PANGO_FONT_MASK_WEIGHT)) == 0, NULL);

  self = g_object_new (PANGO_TYPE_HB_FACE, NULL);

  self->file = g_strdup (face->file);
  if (face->face)
    self->face = hb_face_reference (face->face);

  self->index = face->index;
  self->instance_id = face->instance_id;
  self->variations = g_memdup2 (face->variations, sizeof (hb_variation_t) * face->n_variations);
  self->n_variations = face->n_variations;

  if (transform)
    pango_hb_face_set_matrix (self, transform);

  self->embolden = embolden;
  self->synthetic = self->embolden || (self->matrix != NULL);

  desc = pango_font_description_copy (face->description);
  pango_font_description_merge (desc, description, TRUE);

  if (!name)
    name = style_from_font_description (desc);

  set_name_and_description (self, name, desc);

  pango_font_description_free (desc);

  return self;
}

/**
 * pango_hb_face_new_instance:
 * @face: a `PangoHbFace`
 * @variations: (nullable) (array length=n_variations): font variations to apply
 * @n_variations: length of @variations
 * @name: (nullable): name for the face
 * @description: a `PangoFontDescription` to override fields from @face's description
 *
 * Creates a new `PangoHbFace` that is a variant of @face.
 *
 * The @variations provide values for variation axes of @face. Axes that
 * are not included in @variations will keep the values they have in @face.
 * @variations that refer to axes that the face does not have are ignored.
 *
 * Conceptually, this is similar to a named instance of the face, except
 * that the mapping of the name to a set of coordinates on the variation
 * axes is provided externally, and not by the face itself.
 *
 * If a @name is not specified, the name for the face will be derived
 * from the @description.
 *
 * Apart from setting the style that this face will be used for,
 * @description can provide an alternative family name. This can
 * be used to create generic aliases such as 'sans' or 'monospace'.
 *
 * Note that only the following fields in @description should be set:
 * - style or stretch, to indicate a transformed style
 * - weight, to indicate a bolder weight
 * - family, to provide an alternative family name
 *
 * Returns: (transfer full): a newly created `PangoHbFace`
 *
 * Since: 1.52
 */
PangoHbFace *
pango_hb_face_new_instance (PangoHbFace                *face,
                            const hb_variation_t       *variations,
                            unsigned int                n_variations,
                            const char                 *name,
                            const PangoFontDescription *description)
{
  PangoHbFace *self;
  PangoFontDescription *desc;

  g_return_val_if_fail (PANGO_IS_HB_FACE (face), NULL);
  g_return_val_if_fail (description != NULL, NULL);
  g_return_val_if_fail ((pango_font_description_get_set_fields (description) &
                         ~(PANGO_FONT_MASK_FAMILY|
                           PANGO_FONT_MASK_STYLE|
                           PANGO_FONT_MASK_STRETCH|
                           PANGO_FONT_MASK_WEIGHT)) == 0, NULL);

  self = g_object_new (PANGO_TYPE_HB_FACE, NULL);

  self->file = g_strdup (face->file);
  if (face->face)
    self->face = hb_face_reference (face->face);

  self->index = face->index;
  self->instance_id = face->instance_id;

  if (face->matrix)
    {
      self->matrix = g_memdup2 (face->matrix, sizeof (PangoMatrix));
      self->x_scale = face->x_scale;
      self->y_scale = face->y_scale;
    }

  self->embolden = face->embolden;
  self->synthetic = self->embolden || (self->matrix != NULL);

  self->variations = g_memdup2 (variations, sizeof (hb_variation_t) * n_variations);
  self->n_variations = n_variations;

  desc = pango_font_description_copy (face->description);
  pango_font_description_merge (desc, description, TRUE);

  if (!name)
    name = style_from_font_description (desc);

  set_name_and_description (self, name, desc);

  pango_font_description_free (desc);

  return self;
}

/**
 * pango_hb_face_get_hb_face:
 * @self: a `PangoHbFace`
 *
 * Gets the `hb_face_t` object backing this face.
 *
 * Note that the objects returned by this function are cached
 * and immutable, and may be shared between `PangoHbFace` objects.
 *
 * Returns: (transfer none): the `hb_face_t` object
 *   backing the face
 *
 * Since: 1.52
 */
hb_face_t *
pango_hb_face_get_hb_face (PangoHbFace *self)
{
  ensure_hb_face (self);

  return self->face;
}

/**
 * pango_hb_face_get_file:
 * @self: a `PangoHbFace`
 *
 * Gets the file that backs the face.
 *
 * Returns: (transfer none) (nullable): the file backing the face
 *
 * Since: 1.52
 */
const char *
pango_hb_face_get_file (PangoHbFace *self)
{
  return self->file;
}

/**
 * pango_hb_face_get_face_index:
 * @self: a `PangoHbFace`
 *
 * Gets the face index of the face.
 *
 * Returns: the face indexx
 *
 * Since: 1.52
 */
unsigned int
pango_hb_face_get_face_index (PangoHbFace *self)
{
  return self->index;
}

/**
 * pango_hb_face_get_instance_id:
 * @self: a `PangoHbFace`
 *
 * Gets the instance id of the face.
 *
 * Returns: the instance id
 *
 * Since: 1.52
 */
int
pango_hb_face_get_instance_id (PangoHbFace *self)
{
  return self->instance_id;
}

/**
 * pango_hb_face_get_variations:
 * @self: a `PangoHbFace`
 * @n_variations: (nullable) (out caller-allocates): return location for
 *   the length of the returned array
 *
 * Gets the variations of the face.
 *
 *
 * Returns: (nullable) (transfer none): the variations
 *
 * Since: 1.52
 */
const hb_variation_t *
pango_hb_face_get_variations (PangoHbFace  *self,
                              unsigned int *n_variations)
{
  if (n_variations)
    *n_variations = self->n_variations;

  return self->variations;
}

/**
 * pango_hb_face_get_embolden:
 * @self: a `PangoHbFace`
 *
 * Gets whether face is using synthetic emboldening.
 *
 * Returns: `TRUE` if the face is using synthetic embolding
 */
gboolean
pango_hb_face_get_embolden (PangoHbFace *self)
{
  return self->embolden;
}

/**
 * pango_hb_face_get_transform:
 * @self: a `PangoHbFace`
 *
 * Gets the transform that this face uses.
 *
 * This is the 'font matrix' which iis used for
 * sythetic italics and width variations.
 *
 * Returns: (nullable) (transfer none): the transform of face
 */
const PangoMatrix *
pango_hb_face_get_transform (PangoHbFace *self)
{
  return self->matrix;
}

/* }}} */

/* vim:set foldmethod=marker expandtab: */
