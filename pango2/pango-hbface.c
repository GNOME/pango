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

#include "pango-font-private.h"
#include "pango-hbface-private.h"
#include "pango-hbfont.h"

#include "pango-language-set-simple-private.h"

#include <string.h>
#include <hb-ot.h>
#include <hb-gobject.h>

/**
 * Pango2HbFace:
 *
 * `Pango2HbFace` is a `Pango2FontFace` implementation that wraps
 * a `hb_face_t` object and implements all of the `Pango2FontFace`
 * functionality using HarfBuzz.
 *
 * In addition to making a `hb_face_t` available for rendering
 * glyphs with Pango2, `Pango2HbFace` allows some tweaks to the
 * rendering, such as artificial slant (using a transformation
 * matrix) or artificial emboldening.
 *
 * To get a font instance at a specific size from a `Pango2HbFace`,
 * use [ctor@Pango2.HbFont.new].
 */

 /* {{{ Utilities */

static void
get_name_from_hb_face (hb_face_t       *face,
                       hb_ot_name_id_t  name_id,
                       hb_ot_name_id_t  fallback_id,
                       char            *buf,
                       unsigned int     len)
{
  unsigned int size = len;

  if (hb_ot_name_get_utf8 (face, name_id, HB_LANGUAGE_INVALID, &size, buf) > 0)
    return;

  if (fallback_id != HB_OT_NAME_ID_INVALID)
    {
      size = len;

      if (hb_ot_name_get_utf8 (face, fallback_id, HB_LANGUAGE_INVALID, &size, buf) > 0)
        return;
    }

  strncpy (buf, "Unnamed", len);
  buf[len - 1] = '\0';
}

static void
ensure_hb_face (Pango2HbFace *self)
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
set_name_and_description (Pango2HbFace                *self,
                          const char                  *name,
                          const Pango2FontDescription *description)
{
  Pango2FontFace *face = PANGO2_FONT_FACE (self);

  if (name)
    {
      pango2_font_face_set_name (face, name);
    }
  else
    {
      hb_ot_name_id_t name_id;
      char face_name[256] = { 0, };

      ensure_hb_face (self);

      if (self->instance_id >= 0)
        name_id = hb_ot_var_named_instance_get_subfamily_name_id (self->face, self->instance_id);
      else
        name_id = HB_OT_NAME_ID_TYPOGRAPHIC_SUBFAMILY;

      get_name_from_hb_face (self->face,
                             name_id,
                             HB_OT_NAME_ID_FONT_SUBFAMILY,
                             face_name, sizeof (face_name));

      pango2_font_face_set_name (face, face_name);
    }

  if (description)
    {
      face->description = pango2_font_description_copy (description);
    }
  else
    {
      char family[256] = { 0, };
      char *fullname;

      ensure_hb_face (self);

      get_name_from_hb_face (self->face,
                             HB_OT_NAME_ID_TYPOGRAPHIC_FAMILY,
                             HB_OT_NAME_ID_FONT_FAMILY,
                             family, sizeof (family));
      fullname = g_strconcat (family, " ", face->name, NULL);

      /* This is an attempt to parse style/weight/variant information
       * out of the face name. FIXME: we should look at variation
       * coordinates too, here, instead of these guessing games.
       */
      face->description = pango2_font_description_from_string (fullname);
      pango2_font_description_unset_fields (face->description,
                                            PANGO2_FONT_MASK_VARIATIONS |
                                            PANGO2_FONT_MASK_GRAVITY);

      /* Make sure we don't leave any leftovers misinterpreted
       * as part of the family name.
       */
      pango2_font_description_set_family (face->description, family);

      g_free (fullname);
    }

  if (self->n_variations > 0)
    {
      char *str = variations_to_string (self->variations, self->n_variations, "=", ",");
      pango2_font_description_set_variations (face->description, str);
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
ensure_faceid (Pango2HbFace *self)
{
  double slant;
  char buf0[32], buf1[32], buf2[32];
  char *str = NULL;
  char psname[256] = { 0, };
  char *p;

  if (self->faceid)
    return;

  ensure_hb_face (self);

  get_name_from_hb_face (self->face,
                         HB_OT_NAME_ID_POSTSCRIPT_NAME,
                         HB_OT_NAME_ID_INVALID,
                         psname, sizeof (psname));

  /* PostScript name should not contain problematic chars, but just in case,
   * make sure we don't have any ' ', '=' or ',' that would give us parsing
   * problems.
   */
  p = psname;
  while ((p = strpbrk (p, " =,")) != NULL)
    *p = '?';

  if (self->transform)
    slant = pango2_matrix_get_slant_ratio (self->transform);
  else
    slant = 0.;

  if (self->n_variations > 0)
    str = variations_to_string (self->variations, self->n_variations, "_", ":");

  self->faceid = g_strdup_printf ("hb:%s:%u:%d:%d:%s:%s:%s%s%s",
                                  psname,
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

static const char *
style_from_font_description (const Pango2FontDescription *desc)
{
  Pango2Style style = pango2_font_description_get_style (desc);
  Pango2Weight weight = pango2_font_description_get_weight (desc);

  switch (style)
    {
    case PANGO2_STYLE_ITALIC:
      if (weight == PANGO2_WEIGHT_BOLD)
        return "Bold Italic";
      else
        return "Italic";
      break;
    case PANGO2_STYLE_OBLIQUE:
      if (weight == PANGO2_WEIGHT_BOLD)
        return "Bold Oblique";
      else
        return "Oblique";
      break;
    case PANGO2_STYLE_NORMAL:
      if (weight == PANGO2_WEIGHT_BOLD)
        return "Bold";
      else
        return "Regular";
      break;
    default: ;
    }

  return NULL;
}

/* }}} */
/* {{{ Pango2FontFace implementation */

struct _Pango2HbFaceClass
{
  Pango2FontFaceClass parent_class;
};

enum {
  PROP_HB_FACE = 1,
  PROP_FILE,
  PROP_FACE_INDEX,
  PROP_INSTANCE_ID,
  PROP_VARIATIONS,
  PROP_EMBOLDEN,
  PROP_TRANSFORM,
  N_PROPERTIES
};

static GParamSpec *properties[N_PROPERTIES] = { NULL, };

G_DEFINE_FINAL_TYPE (Pango2HbFace, pango2_hb_face, PANGO2_TYPE_FONT_FACE)

static void
pango2_hb_face_init (Pango2HbFace *self)
{
  self->transform = NULL;
  self->x_scale = self->y_scale = 1.;
}

static void
pango2_hb_face_finalize (GObject *object)
{
  Pango2HbFace *self = PANGO2_HB_FACE (object);

  g_free (self->faceid);
  if (self->face)
    hb_face_destroy (self->face);
  g_free (self->file);
  if (self->languages)
    g_object_unref (self->languages);
  g_free (self->variations);
  if (self->transform)
    g_free (self->transform);

  G_OBJECT_CLASS (pango2_hb_face_parent_class)->finalize (object);
}

static gboolean
pango2_hb_face_is_synthesized (Pango2FontFace *face)
{
  Pango2HbFace *self = PANGO2_HB_FACE (face);

  return self->synthetic;
}

static gboolean
pango2_hb_face_is_monospace (Pango2FontFace *face)
{
  Pango2HbFace *self = PANGO2_HB_FACE (face);

  ensure_hb_face (self);

  return hb_face_is_monospace (self->face);
}

static gboolean
pango2_hb_face_is_variable (Pango2FontFace *face)
{
  Pango2HbFace *self = PANGO2_HB_FACE (face);

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
pango2_hb_face_supports_language (Pango2FontFace *face,
                                  Pango2Language *language)
{
  Pango2HbFace *self = PANGO2_HB_FACE (face);
  Pango2LanguageSet *set = pango2_hb_face_get_language_set (self);

  if (set)
    return pango2_language_set_matches_language (set, language);

  return TRUE;
}

static Pango2Language **
pango2_hb_face_get_languages (Pango2FontFace *face)
{
  Pango2HbFace *self = PANGO2_HB_FACE (face);
  Pango2LanguageSet *set = pango2_hb_face_get_language_set (self);

  if (set)
    return pango2_language_set_get_languages (set);

  return NULL;
}

static gboolean
pango2_hb_face_has_char (Pango2FontFace *face,
                         gunichar        wc)
{
  Pango2HbFace *self = PANGO2_HB_FACE (face);
  hb_font_t *hb_font;
  hb_codepoint_t glyph;
  gboolean ret;

  ensure_hb_face (self);

  hb_font = hb_font_create (self->face);
  ret = hb_font_get_nominal_glyph (hb_font, wc, &glyph);
  hb_font_destroy (hb_font);

  return ret;
}

static const char *
pango2_hb_face_get_faceid (Pango2FontFace *face)
{
  Pango2HbFace *self = PANGO2_HB_FACE (face);

  ensure_faceid (self);

  return self->faceid;
}

static Pango2Font *
pango2_hb_face_create_font (Pango2FontFace              *face,
                            const Pango2FontDescription *desc,
                            float                        dpi,
                            const Pango2Matrix          *ctm)
{
  Pango2HbFace *self = PANGO2_HB_FACE (face);

  return PANGO2_FONT (pango2_hb_font_new_for_description (self, desc, dpi, ctm));
}

static void
pango2_hb_face_get_property (GObject    *object,
                             guint       property_id,
                             GValue     *value,
                             GParamSpec *pspec)
{
  Pango2HbFace *self = PANGO2_HB_FACE (object);

  switch (property_id)
    {
    case PROP_HB_FACE:
      g_value_set_boxed (value, pango2_hb_face_get_hb_face (self));
      break;

    case PROP_FILE:
      g_value_set_string (value, pango2_hb_face_get_file (self));
      break;

    case PROP_FACE_INDEX:
      g_value_set_uint (value, pango2_hb_face_get_face_index (self));
      break;

    case PROP_INSTANCE_ID:
      g_value_set_int (value, pango2_hb_face_get_instance_id (self));
      break;

    case PROP_VARIATIONS:
      {
        char *str = NULL;
        if (self->variations)
          str = variations_to_string (self->variations, self->n_variations, "=", ",");
        g_value_take_string (value, str);
      }
      break;

    case PROP_EMBOLDEN:
      g_value_set_boolean (value, pango2_hb_face_get_embolden (self));
      break;

    case PROP_TRANSFORM:
      g_value_set_boxed (value, pango2_hb_face_get_transform (self));
      break;

    default:
      G_OBJECT_WARN_INVALID_PROPERTY_ID (object, property_id, pspec);
    }
}

static void
pango2_hb_face_class_init (Pango2HbFaceClass *class)
{
  GObjectClass *object_class = G_OBJECT_CLASS (class);
  Pango2FontFaceClass *face_class = PANGO2_FONT_FACE_CLASS (class);

  object_class->finalize = pango2_hb_face_finalize;
  object_class->get_property = pango2_hb_face_get_property;

  face_class->is_synthesized = pango2_hb_face_is_synthesized;
  face_class->is_monospace = pango2_hb_face_is_monospace;
  face_class->is_variable = pango2_hb_face_is_variable;
  face_class->supports_language = pango2_hb_face_supports_language;
  face_class->get_languages = pango2_hb_face_get_languages;
  face_class->has_char = pango2_hb_face_has_char;
  face_class->get_faceid = pango2_hb_face_get_faceid;
  face_class->create_font = pango2_hb_face_create_font;

  /**
   * Pango2HbFace:hb-face: (attributes org.gtk.Property.get=pango2_hb_face_get_hb_face)
   *
   * A `hb_face_t` object backing this face.
   */
  properties[PROP_HB_FACE] =
      g_param_spec_boxed ("hb-face", NULL, NULL, HB_GOBJECT_TYPE_FACE,
                          G_PARAM_READABLE | G_PARAM_STATIC_STRINGS);

  /**
   * Pango2HbFace:file: (attributes org.gtk.Property.get=pango2_hb_face_get_file)
   *
   * The file that this face was created from, if any.
   */
  properties[PROP_FILE] =
      g_param_spec_string ("file", NULL, NULL, NULL,
                          G_PARAM_READABLE | G_PARAM_STATIC_STRINGS);

  /**
   * Pango2HbFace:face-index: (attributes org.gtk.Property.get=pango2_hb_face_get_face_index)
   *
   * The index of the face, in case that it was created
   * from a file containing data for multiple faces.
   */
  properties[PROP_FACE_INDEX] =
      g_param_spec_uint ("face-index", NULL, NULL, 0, G_MAXUINT, 0,
                          G_PARAM_READABLE | G_PARAM_STATIC_STRINGS);

  /**
   * Pango2HbFace:instance-id: (attributes org.gtk.Property.get=pango2_hb_face_get_instance_id)
   *
   * The ID of the named instance of this face.
   *
   * -1 represents the default instance, and
   * -2 represents no instance, meaning that the face will
   *  be variable.
   */
  properties[PROP_INSTANCE_ID] =
      g_param_spec_int ("instance-id", NULL, NULL, -2, G_MAXINT, -1,
                        G_PARAM_READABLE | G_PARAM_STATIC_STRINGS);

  /**
   * Pango2HbFace:variations: (attributes org.gtk.Property.get=pango2_hb_face_get_variations)
   *
   * The variations that are applied for this face.
   *
   * This property contains a string representation of the variations.
   */
  properties[PROP_VARIATIONS] =
      g_param_spec_string ("variations", NULL, NULL, NULL,
                           G_PARAM_READABLE | G_PARAM_STATIC_STRINGS);

  /**
   * Pango2HbFace:embolden: (attributes org.gtk.Property.get=pango2_hb_face_get_embolden)
   *
   * `TRUE` if the face is using synthetic emboldening.
   */
  properties[PROP_EMBOLDEN] =
      g_param_spec_boolean ("embolden", NULL, NULL, FALSE,
                            G_PARAM_READABLE | G_PARAM_STATIC_STRINGS);

  /**
   * Pango2HbFace:transform: (attributes org.gtk.Property.get=pango2_hb_face_get_transform)
   *
   * The transform from 'font space' to 'user space' that
   * this face uses.
   *
   * This is the 'font matrix' which is used for
   * sythetic italics and width variations.
   */
  properties[PROP_TRANSFORM] =
      g_param_spec_boxed ("transform", NULL, NULL, PANGO2_TYPE_MATRIX,
                          G_PARAM_READABLE | G_PARAM_STATIC_STRINGS);

  g_object_class_install_properties (object_class, N_PROPERTIES, properties);
}

/* }}} */
/* {{{ Private API */

/*< private >
 * pango2_hb_face_get_language_set:
 * @face: a `Pango2HbFace`
 *
 * Returns the languages supported by @face.
 *
 * Returns: (transfer none): a `Pango2LanguageSet`
 */
Pango2LanguageSet *
pango2_hb_face_get_language_set (Pango2HbFace *face)
{
  return face->languages;
}

/*< private >
 * pango2_hb_face_set_language_set:
 * @self: a `Pango2HbFace`
 * @languages: a `Pango2LanguageSet`
 *
 * Sets the languages that are supported by @face.
 *
 * This should only be called by fontmap implementations.
 */
void
pango2_hb_face_set_language_set (Pango2HbFace      *self,
                                 Pango2LanguageSet *languages)
{
  g_set_object (&self->languages, languages);
}

/*< private  >
 * pango2_hb_face_set_matrix:
 * @self: a `Pango2HbFace`
 * @matrix: the `Pango2Matrix`
 *
 * Sets the font matrix for @self.
 *
 * This should only be called by fontmap implementations.
 */
void
pango2_hb_face_set_matrix (Pango2HbFace       *self,
                           const Pango2Matrix *matrix)
{
  if (!self->transform)
    self->transform = g_new (Pango2Matrix, 1);

  *self->transform = *matrix;

  pango2_matrix_get_font_scale_factors (self->transform, &self->x_scale, &self->y_scale);
  pango2_matrix_scale (self->transform, 1./self->x_scale, 1./self->y_scale);
}

/* }}} */
 /* {{{ Public API */

/**
 * pango2_hb_face_new_from_hb_face:
 * @face: an immutable `hb_face_t`
 * @instance_id: named instance id, or -1 for the default instance
 *   or -2 for no instance
 * @name: (nullable): name for the face
 * @description: (nullable): `Pango2FontDescription` for the font
 *
 * Creates a new `Pango2HbFace` by wrapping an existing `hb_face_t`.
 *
 * The @instance_id can be used to pick one of the available named
 * instances in a variable font. See hb_ot_var_get_named_instance_count()
 * to learn about the available named instances.
 *
 * If @instance_id is -2 and @face has variation axes, then
 * [method@Pango2.FontFace.is_variable] will return `TRUE` for
 * the returned `Pango2HbFace`.
 *
 * If @name is provided, it is used as the name for the face.
 * Otherwise, Pango will use the named instance subfamily name
 * or `HB_OT_NAME_ID_TYPOGRAPHIC_SUBFAMILY`.
 *
 * If @description is provided, it is used as the font description
 * for the face. Otherwise, Pango creates a description using
 * `HB_OT_NAME_ID_TYPOGRAPHIC_FAMILY` and the name of the face.
 *
 * Returns: a newly created `Pango2HbFace`
 */
Pango2HbFace *
pango2_hb_face_new_from_hb_face (hb_face_t                  *face,
                                 int                         instance_id,
                                 const char                  *name,
                                 const Pango2FontDescription *description)
{
  Pango2HbFace *self;

  g_return_val_if_fail (face != NULL, NULL);
  g_return_val_if_fail (hb_face_is_immutable (face), NULL);
  g_return_val_if_fail (instance_id >= -2, NULL);
  g_return_val_if_fail (description == NULL ||
                        (pango2_font_description_get_set_fields (description) &
                         (PANGO2_FONT_MASK_SIZE|PANGO2_FONT_MASK_GRAVITY)) == 0, NULL);

  self = g_object_new (PANGO2_TYPE_HB_FACE, NULL);

  self->face = hb_face_reference (face);
  self->index = hb_face_get_index (face) & 0xffff;
  self->instance_id = instance_id;

  if (instance_id >= (int)hb_ot_var_get_named_instance_count (face))
    g_warning ("Instance ID %d out of range", instance_id);

  set_name_and_description (self, name, description);

  return self;
}

/**
 * pango2_hb_face_new_from_file:
 * @file: font filename
 * @index: face index
 * @instance_id: named instance id, or -1 for the default instance
 *   or -2 for no instance
 * @name: (nullable): name for the face
 * @description: (nullable): `Pango2FontDescription` for the font
 *
 * Creates a new `Pango2HbFace` from a font file.
 *
 * The @index can be used to pick a face from a file containing
 * multiple faces, such as TTC or DFont.
 *
 * The @instance_id can be used to pick one of the available named
 * instances in a variable font. See hb_ot_var_get_named_instance_count()
 * to learn about the available named instances.
 *
 * If @instance_id is -2 and @face has variation axes, then
 * [method@Pango2.FontFace.is_variable] will return `TRUE` for
 * the returned `Pango2HbFace`.
 *
 * If @name is provided, it is used as the name for the face.
 * Otherwise, Pango will use the named instance subfamily name
 * or `HB_OT_NAME_ID_TYPOGRAPHIC_SUBFAMILY`.
 *
 * If @description is provided, it is used as the font description
 * for the face. Otherwise, Pango creates a description using
 * `HB_OT_NAME_ID_TYPOGRAPHIC_FAMILY` and the name of the face.
 *
 * If @desc and @name are provided, then the returned `Pango2HbFace`
 * object will be lazily initialized as needed.
 *
 * Returns: a newly created `Pango2HbFace`
 */
Pango2HbFace *
pango2_hb_face_new_from_file (const char                  *file,
                              unsigned int                 index,
                              int                          instance_id,
                              const char                  *name,
                              const Pango2FontDescription *description)
{
  Pango2HbFace *self;

  g_return_val_if_fail (file!= NULL, NULL);
  g_return_val_if_fail (instance_id >= -2, NULL);
  g_return_val_if_fail (description == NULL ||
                        (pango2_font_description_get_set_fields (description) &
                         (PANGO2_FONT_MASK_VARIANT|
                          PANGO2_FONT_MASK_SIZE|
                          PANGO2_FONT_MASK_GRAVITY)) == 0, NULL);

  self = g_object_new (PANGO2_TYPE_HB_FACE, NULL);

  self->file = g_strdup (file);
  self->index = index;
  self->instance_id = instance_id;

  set_name_and_description (self, name, description);

  return self;
}

/**
 * pango2_hb_face_new_synthetic:
 * @face: a `Pango2HbFace`
 * @transform: (nullable): the transform to apply
 * @embolden: `TRUE` to render the font bolder
 * @name: (nullable): name for the face
 * @description: a `Pango2FontDescription` to override fields from @face's description
 *
 * Creates a new `Pango2HbFace` that is a synthetic variant of @face.
 *
 * Here, 'synthetic' means that the variant is implemented by rendering
 * the glyphs differently, not by using data from the original @face.
 * See [method@Pango2.HbFace.new_instance] for that.
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
 *
 * + style or stretch, to indicate a transformed style
 * + weight, to indicate a bolder weight
 * + family, to provide an alternative family name
 *
 * [method@Pango2.FontFace.is_synthesized] will return `TRUE` for objects
 * created by this function.
 *
 * Returns: (transfer full): a newly created `Pango2HbFace`
 */
Pango2HbFace *
pango2_hb_face_new_synthetic (Pango2HbFace                *face,
                              const Pango2Matrix          *transform,
                              gboolean                     embolden,
                              const char                  *name,
                              const Pango2FontDescription *description)
{
  Pango2HbFace *self;
  Pango2FontDescription *desc;

  g_return_val_if_fail (PANGO2_IS_HB_FACE (face), NULL);
  g_return_val_if_fail (description != NULL, NULL);
  g_return_val_if_fail ((pango2_font_description_get_set_fields (description) &
                         ~(PANGO2_FONT_MASK_FAMILY|
                           PANGO2_FONT_MASK_STYLE|
                           PANGO2_FONT_MASK_STRETCH|
                           PANGO2_FONT_MASK_WEIGHT)) == 0, NULL);

  self = g_object_new (PANGO2_TYPE_HB_FACE, NULL);

  self->file = g_strdup (face->file);
  if (face->face)
    self->face = hb_face_reference (face->face);

  self->index = face->index;
  self->instance_id = face->instance_id;
  self->variations = g_memdup2 (face->variations, sizeof (hb_variation_t) * face->n_variations);
  self->n_variations = face->n_variations;

  if (transform)
    pango2_hb_face_set_matrix (self, transform);

  self->embolden = embolden;
  self->synthetic = self->embolden || (self->transform != NULL);

  desc = pango2_font_description_copy (PANGO2_FONT_FACE (face)->description);
  pango2_font_description_merge (desc, description, TRUE);

  if (!name)
    name = style_from_font_description (desc);

  set_name_and_description (self, name, desc);

  pango2_hb_face_set_language_set (self, face->languages);

  pango2_font_description_free (desc);

  return self;
}

/**
 * pango2_hb_face_new_instance:
 * @face: a `Pango2HbFace`
 * @variations: (nullable) (array length=n_variations): font variations to apply
 * @n_variations: length of @variations
 * @name: (nullable): name for the face
 * @description: a `Pango2FontDescription` to override fields from @face's description
 *
 * Creates a new `Pango2HbFace` that is a variant of @face.
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
 * Returns: (transfer full): a newly created `Pango2HbFace`
 */
Pango2HbFace *
pango2_hb_face_new_instance (Pango2HbFace                *face,
                             const hb_variation_t        *variations,
                             unsigned int                 n_variations,
                             const char                  *name,
                             const Pango2FontDescription *description)
{
  Pango2HbFace *self;
  Pango2FontDescription *desc;

  g_return_val_if_fail (PANGO2_IS_HB_FACE (face), NULL);
  g_return_val_if_fail (description != NULL, NULL);
  g_return_val_if_fail ((pango2_font_description_get_set_fields (description) &
                         ~(PANGO2_FONT_MASK_FAMILY|
                           PANGO2_FONT_MASK_STYLE|
                           PANGO2_FONT_MASK_STRETCH|
                           PANGO2_FONT_MASK_WEIGHT)) == 0, NULL);

  self = g_object_new (PANGO2_TYPE_HB_FACE, NULL);

  self->file = g_strdup (face->file);
  if (face->face)
    self->face = hb_face_reference (face->face);

  self->index = face->index;
  self->instance_id = face->instance_id;

  if (face->transform)
    {
      self->transform = g_memdup2 (face->transform, sizeof (Pango2Matrix));
      self->x_scale = face->x_scale;
      self->y_scale = face->y_scale;
    }

  self->embolden = face->embolden;
  self->synthetic = self->embolden || (self->transform != NULL);

  self->variations = g_memdup2 (variations, sizeof (hb_variation_t) * n_variations);
  self->n_variations = n_variations;

  desc = pango2_font_description_copy (PANGO2_FONT_FACE (face)->description);
  pango2_font_description_merge (desc, description, TRUE);

  if (!name)
    name = style_from_font_description (desc);

  set_name_and_description (self, name, desc);

  pango2_font_description_free (desc);

  return self;
}

/**
 * pango2_hb_face_get_hb_face:
 * @self: a `Pango2HbFace`
 *
 * Gets the `hb_face_t` object backing this face.
 *
 * Note that the objects returned by this function are cached
 * and immutable, and may be shared between `Pango2HbFace` objects.
 *
 * Returns: (transfer none): the `hb_face_t` object
 *   backing the face
 */
hb_face_t *
pango2_hb_face_get_hb_face (Pango2HbFace *self)
{
  g_return_val_if_fail (PANGO2_IS_HB_FACE (self), NULL);

  ensure_hb_face (self);

  return self->face;
}

/**
 * pango2_hb_face_get_file:
 * @self: a `Pango2HbFace`
 *
 * Gets the file that backs the face.
 *
 * Returns: (transfer none) (nullable): the file backing the face
 */
const char *
pango2_hb_face_get_file (Pango2HbFace *self)
{
  g_return_val_if_fail (PANGO2_IS_HB_FACE (self), NULL);

  return self->file;
}

/**
 * pango2_hb_face_get_face_index:
 * @self: a `Pango2HbFace`
 *
 * Gets the face index of the face.
 *
 * Returns: the face indexx
 */
unsigned int
pango2_hb_face_get_face_index (Pango2HbFace *self)
{
  g_return_val_if_fail (PANGO2_IS_HB_FACE (self), 0);

  return self->index;
}

/**
 * pango2_hb_face_get_instance_id:
 * @self: a `Pango2HbFace`
 *
 * Gets the instance id of the face.
 *
 * Returns: the instance id
 */
int
pango2_hb_face_get_instance_id (Pango2HbFace *self)
{
  g_return_val_if_fail (PANGO2_IS_HB_FACE (self), -1);

  return self->instance_id;
}

/**
 * pango2_hb_face_get_variations:
 * @self: a `Pango2HbFace`
 * @n_variations: (nullable) (out caller-allocates): return location for
 *   the length of the returned array
 *
 * Gets the variations of the face.
 *
 * Returns: (nullable) (transfer none): the variations
 */
const hb_variation_t *
pango2_hb_face_get_variations (Pango2HbFace *self,
                               unsigned int *n_variations)
{
  g_return_val_if_fail (PANGO2_IS_HB_FACE (self), NULL);

  if (n_variations)
    *n_variations = self->n_variations;

  return self->variations;
}

/**
 * pango2_hb_face_get_embolden:
 * @self: a `Pango2HbFace`
 *
 * Gets whether face is using synthetic emboldening.
 *
 * Returns: `TRUE` if the face is using synthetic embolding
 */
gboolean
pango2_hb_face_get_embolden (Pango2HbFace *self)
{
  g_return_val_if_fail (PANGO2_IS_HB_FACE (self), FALSE);

  return self->embolden;
}

/**
 * pango2_hb_face_get_transform:
 * @self: a `Pango2HbFace`
 *
 * Gets the transform from 'font space' to 'user space' that this face uses.
 *
 * This is the 'font matrix' which is used for
 * sythetic italics and width variations.
 *
 * Returns: (nullable) (transfer none): the transform of face
 */
const Pango2Matrix *
pango2_hb_face_get_transform (Pango2HbFace *self)
{
  g_return_val_if_fail (PANGO2_IS_HB_FACE (self), NULL);

  return self->transform;
}

/* }}} */

/* vim:set foldmethod=marker expandtab: */
