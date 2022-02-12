/* Pango
 * fonts.c:
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
#include <stdlib.h>
#include <math.h>
#include <string.h>

#include <gio/gio.h>

#include "pango-types.h"
#include "pango-font-private.h"
#include "pango-font-metrics-private.h"
#include "pango-fontmap.h"
#include "pango-impl-utils.h"


/*
 * PangoFont
 */

typedef struct {
  hb_font_t *hb_font;
} PangoFontPrivate;

#define PANGO_FONT_GET_CLASS_PRIVATE(font) ((PangoFontClassPrivate *) \
   g_type_class_get_private ((GTypeClass *) PANGO_FONT_GET_CLASS (font), \
                            PANGO_TYPE_FONT))

G_DEFINE_ABSTRACT_TYPE_WITH_CODE (PangoFont, pango_font, G_TYPE_OBJECT,
                                  G_ADD_PRIVATE (PangoFont)
                                  g_type_add_class_private (g_define_type_id, sizeof (PangoFontClassPrivate)))

static void
pango_font_finalize (GObject *object)
{
  PangoFont *font = PANGO_FONT (object);
  PangoFontPrivate *priv = pango_font_get_instance_private (font);

  hb_font_destroy (priv->hb_font);

  G_OBJECT_CLASS (pango_font_parent_class)->finalize (object);
}

static PangoLanguage **
pango_font_default_get_languages (PangoFont *font)
{
  return NULL;
}

static gboolean
pango_font_default_is_hinted (PangoFont *font)
{
  return FALSE;
}

static void
pango_font_default_get_scale_factors (PangoFont *font,
                                      double    *x_scale,
                                      double    *y_scale)
{
  *x_scale = *y_scale = 1.0;
}

static gboolean
pango_font_default_has_char (PangoFont *font,
                             gunichar   wc)
{
  PangoCoverage *coverage = pango_font_get_coverage (font, pango_language_get_default ());
  PangoCoverageLevel result = pango_coverage_get (coverage, wc);
  g_object_unref (coverage);
  return result != PANGO_COVERAGE_NONE;
}

static PangoFontFace *
pango_font_default_get_face (PangoFont *font)
{
  PangoFontMap *map = pango_font_get_font_map (font);

  return PANGO_FONT_MAP_GET_CLASS (map)->get_face (map,font);
}

static void
pango_font_default_get_matrix (PangoFont   *font,
                               PangoMatrix *matrix)
{
  *matrix = (PangoMatrix) PANGO_MATRIX_INIT;
}

static int
pango_font_default_get_absolute_size (PangoFont *font)
{
  PangoFontDescription *desc;
  int size;

  desc = pango_font_describe_with_absolute_size (font);
  size = pango_font_description_get_size (desc);
  pango_font_description_free (desc);

  return size;
}

static void
pango_font_class_init (PangoFontClass *class G_GNUC_UNUSED)
{
  GObjectClass *object_class = G_OBJECT_CLASS (class);
  PangoFontClassPrivate *pclass;

  object_class->finalize = pango_font_finalize;

  pclass = g_type_class_get_private ((GTypeClass *) class, PANGO_TYPE_FONT);

  pclass->get_languages = pango_font_default_get_languages;
  pclass->is_hinted = pango_font_default_is_hinted;
  pclass->get_scale_factors = pango_font_default_get_scale_factors;
  pclass->has_char = pango_font_default_has_char;
  pclass->get_face = pango_font_default_get_face;
  pclass->get_matrix = pango_font_default_get_matrix;
  pclass->get_absolute_size = pango_font_default_get_absolute_size;
}

static void
pango_font_init (PangoFont *font G_GNUC_UNUSED)
{
}

/**
 * pango_font_describe:
 * @font: a `PangoFont`
 *
 * Returns a description of the font, with font size set in points.
 *
 * Use [method@Pango.Font.describe_with_absolute_size] if you want
 * the font size in device units.
 *
 * Return value: a newly-allocated `PangoFontDescription` object.
 */
PangoFontDescription *
pango_font_describe (PangoFont *font)
{
  g_return_val_if_fail (font != NULL, NULL);

  return PANGO_FONT_GET_CLASS (font)->describe (font);
}

/**
 * pango_font_describe_with_absolute_size:
 * @font: a `PangoFont`
 *
 * Returns a description of the font, with absolute font size set
 * in device units.
 *
 * Use [method@Pango.Font.describe] if you want the font size in points.
 *
 * Return value: a newly-allocated `PangoFontDescription` object.
 *
 * Since: 1.14
 */
PangoFontDescription *
pango_font_describe_with_absolute_size (PangoFont *font)
{
  g_return_val_if_fail (font != NULL, NULL);

  if (G_UNLIKELY (!PANGO_FONT_GET_CLASS (font)->describe_absolute))
    {
      g_warning ("describe_absolute not implemented for this font class, report this as a bug");
      return pango_font_describe (font);
    }

  return PANGO_FONT_GET_CLASS (font)->describe_absolute (font);
}

/**
 * pango_font_get_coverage:
 * @font: a `PangoFont`
 * @language: the language tag
 *
 * Computes the coverage map for a given font and language tag.
 *
 * Return value: (transfer full): a newly-allocated `PangoCoverage`
 *   object.
 */
PangoCoverage *
pango_font_get_coverage (PangoFont     *font,
                         PangoLanguage *language)
{
  g_return_val_if_fail (font != NULL, NULL);

  return PANGO_FONT_GET_CLASS (font)->get_coverage (font, language);
}

/**
 * pango_font_get_glyph_extents:
 * @font: (nullable): a `PangoFont`
 * @glyph: the glyph index
 * @ink_rect: (out) (optional): rectangle used to store the extents of the glyph as drawn
 * @logical_rect: (out) (optional): rectangle used to store the logical extents of the glyph
 *
 * Gets the logical and ink extents of a glyph within a font.
 *
 * The coordinate system for each rectangle has its origin at the
 * base line and horizontal origin of the character with increasing
 * coordinates extending to the right and down. The macros PANGO_ASCENT(),
 * PANGO_DESCENT(), PANGO_LBEARING(), and PANGO_RBEARING() can be used to convert
 * from the extents rectangle to more traditional font metrics. The units
 * of the rectangles are in 1/PANGO_SCALE of a device unit.
 *
 * If @font is %NULL, this function gracefully sets some sane values in the
 * output variables and returns.
 */
void
pango_font_get_glyph_extents  (PangoFont      *font,
                               PangoGlyph      glyph,
                               PangoRectangle *ink_rect,
                               PangoRectangle *logical_rect)
{
  if (G_UNLIKELY (!font))
    {
      if (ink_rect)
        {
          ink_rect->x = PANGO_SCALE;
          ink_rect->y = - (PANGO_UNKNOWN_GLYPH_HEIGHT - 1) * PANGO_SCALE;
          ink_rect->height = (PANGO_UNKNOWN_GLYPH_HEIGHT - 2) * PANGO_SCALE;
          ink_rect->width = (PANGO_UNKNOWN_GLYPH_WIDTH - 2) * PANGO_SCALE;
        }
      if (logical_rect)
        {
          logical_rect->x = 0;
          logical_rect->y = - PANGO_UNKNOWN_GLYPH_HEIGHT * PANGO_SCALE;
          logical_rect->height = PANGO_UNKNOWN_GLYPH_HEIGHT * PANGO_SCALE;
          logical_rect->width = PANGO_UNKNOWN_GLYPH_WIDTH * PANGO_SCALE;
        }
      return;
    }

  PANGO_FONT_GET_CLASS (font)->get_glyph_extents (font, glyph, ink_rect, logical_rect);
}

/**
 * pango_font_get_metrics:
 * @font: (nullable): a `PangoFont`
 * @language: (nullable): language tag used to determine which script
 *   to get the metrics for, or %NULL to indicate to get the metrics for
 *   the entire font.
 *
 * Gets overall metric information for a font.
 *
 * Since the metrics may be substantially different for different scripts,
 * a language tag can be provided to indicate that the metrics should be
 * retrieved that correspond to the script(s) used by that language.
 *
 * If @font is %NULL, this function gracefully sets some sane values in the
 * output variables and returns.
 *
 * Return value: a `PangoFontMetrics` object. The caller must call
 *   [method@Pango.FontMetrics.unref] when finished using the object.
 */
PangoFontMetrics *
pango_font_get_metrics (PangoFont     *font,
                        PangoLanguage *language)
{
  if (G_UNLIKELY (!font))
    {
      PangoFontMetrics *metrics = pango_font_metrics_new ();

      metrics->ascent = PANGO_SCALE * PANGO_UNKNOWN_GLYPH_HEIGHT;
      metrics->descent = 0;
      metrics->height = 0;
      metrics->approximate_char_width = PANGO_SCALE * PANGO_UNKNOWN_GLYPH_WIDTH;
      metrics->approximate_digit_width = PANGO_SCALE * PANGO_UNKNOWN_GLYPH_WIDTH;
      metrics->underline_position = -PANGO_SCALE;
      metrics->underline_thickness = PANGO_SCALE;
      metrics->strikethrough_position = PANGO_SCALE * PANGO_UNKNOWN_GLYPH_HEIGHT / 2;
      metrics->strikethrough_thickness = PANGO_SCALE;

      return metrics;
    }

  return PANGO_FONT_GET_CLASS (font)->get_metrics (font, language);
}

/**
 * pango_font_get_font_map:
 * @font: (nullable): a `PangoFont`
 *
 * Gets the font map for which the font was created.
 *
 * Note that the font maintains a *weak* reference to
 * the font map, so if all references to font map are
 * dropped, the font map will be finalized even if there
 * are fonts created with the font map that are still alive.
 * In that case this function will return %NULL.
 *
 * It is the responsibility of the user to ensure that the
 * font map is kept alive. In most uses this is not an issue
 * as a `PangoContext` holds a reference to the font map.
 *
 * Return value: (transfer none) (nullable): the `PangoFontMap`
 *   for the font
 *
 * Since: 1.10
 */
PangoFontMap *
pango_font_get_font_map (PangoFont *font)
{
  if (G_UNLIKELY (!font))
    return NULL;

  if (PANGO_FONT_GET_CLASS (font)->get_font_map)
    return PANGO_FONT_GET_CLASS (font)->get_font_map (font);
  else
    return NULL;
}

/**
 * pango_font_get_face:
 * @font: a `PangoFont`
 *
 * Gets the `PangoFontFace` to which @font belongs.
 *
 * Returns: (transfer none): the `PangoFontFace`
 *
 * Since: 1.46
 */
PangoFontFace *
pango_font_get_face (PangoFont *font)
{
  PangoFontClassPrivate *pclass = PANGO_FONT_GET_CLASS_PRIVATE (font);

  return pclass->get_face (font);
}

/**
 * pango_font_get_hb_font: (skip)
 * @font: a `PangoFont`
 *
 * Get a `hb_font_t` object backing this font.
 *
 * Note that the objects returned by this function are cached
 * and immutable. If you need to make changes to the `hb_font_t`,
 * use [hb_font_create_sub_font()](https://harfbuzz.github.io/harfbuzz-hb-font.html#hb-font-create-sub-font).
 *
 * Returns: (transfer none) (nullable): the `hb_font_t` object
 *   backing the font
 *
 * Since: 1.44
 */
hb_font_t *
pango_font_get_hb_font (PangoFont *font)
{
  PangoFontPrivate *priv = pango_font_get_instance_private (font);

  g_return_val_if_fail (PANGO_IS_FONT (font), NULL);

  if (priv->hb_font)
    return priv->hb_font;

  priv->hb_font = PANGO_FONT_GET_CLASS (font)->create_hb_font (font);

  hb_font_make_immutable (priv->hb_font);

  return priv->hb_font;
}

/*
 * PangoFontFamily
 */

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

/*
 * PangoFontFace
 */

G_DEFINE_ABSTRACT_TYPE (PangoFontFace, pango_font_face, G_TYPE_OBJECT)

static void
pango_font_face_class_init (PangoFontFaceClass *class G_GNUC_UNUSED)
{
}

static void
pango_font_face_init (PangoFontFace *face G_GNUC_UNUSED)
{
}

/**
 * pango_font_face_describe:
 * @face: a `PangoFontFace`
 *
 * Returns a font description that matches the face.
 *
 * The resulting font description will have the family, style,
 * variant, weight and stretch of the face, but its size field
 * will be unset.
 *
 * Return value: a newly-created `PangoFontDescription` structure
 *   holding the description of the face. Use [method@Pango.FontDescription.free]
 *   to free the result.
 */
PangoFontDescription *
pango_font_face_describe (PangoFontFace *face)
{
  g_return_val_if_fail (PANGO_IS_FONT_FACE (face), NULL);

  return PANGO_FONT_FACE_GET_CLASS (face)->describe (face);
}

/**
 * pango_font_face_is_synthesized:
 * @face: a `PangoFontFace`
 *
 * Returns whether a `PangoFontFace` is synthesized.
 *
 * This will be the case if the underlying font rendering engine
 * creates this face from another face, by shearing, emboldening,
 * lightening or modifying it in some other way.
 *
 * Return value: whether @face is synthesized
 *
 * Since: 1.18
 */
gboolean
pango_font_face_is_synthesized (PangoFontFace  *face)
{
  g_return_val_if_fail (PANGO_IS_FONT_FACE (face), FALSE);

  if (PANGO_FONT_FACE_GET_CLASS (face)->is_synthesized != NULL)
    return PANGO_FONT_FACE_GET_CLASS (face)->is_synthesized (face);
  else
    return FALSE;
}

/**
 * pango_font_face_get_face_name:
 * @face: a `PangoFontFace`.
 *
 * Gets a name representing the style of this face.
 *
 * Note that a font family may contain multiple faces
 * with the same name (e.g. a variable and a non-variable
 * face for the same style).
 *
 * Return value: the face name for the face. This string is
 *   owned by the face object and must not be modified or freed.
 */
const char *
pango_font_face_get_face_name (PangoFontFace *face)
{
  g_return_val_if_fail (PANGO_IS_FONT_FACE (face), NULL);

  return PANGO_FONT_FACE_GET_CLASS (face)->get_face_name (face);
}

/**
 * pango_font_face_list_sizes:
 * @face: a `PangoFontFace`.
 * @sizes: (out) (array length=n_sizes) (nullable) (optional):
 *   location to store a pointer to an array of int. This array
 *   should be freed with g_free().
 * @n_sizes: location to store the number of elements in @sizes
 *
 * List the available sizes for a font.
 *
 * This is only applicable to bitmap fonts. For scalable fonts, stores
 * %NULL at the location pointed to by @sizes and 0 at the location pointed
 * to by @n_sizes. The sizes returned are in Pango units and are sorted
 * in ascending order.
 *
 * Since: 1.4
 */
void
pango_font_face_list_sizes (PangoFontFace  *face,
                            int           **sizes,
                            int            *n_sizes)
{
  g_return_if_fail (PANGO_IS_FONT_FACE (face));
  g_return_if_fail (sizes == NULL || n_sizes != NULL);

  if (n_sizes == NULL)
    return;

  if (PANGO_FONT_FACE_GET_CLASS (face)->list_sizes != NULL)
    PANGO_FONT_FACE_GET_CLASS (face)->list_sizes (face, sizes, n_sizes);
  else
    {
      if (sizes != NULL)
        *sizes = NULL;
      *n_sizes = 0;
    }
}

/**
 * pango_font_face_get_family:
 * @face: a `PangoFontFace`
 *
 * Gets the `PangoFontFamily` that @face belongs to.
 *
 * Returns: (transfer none): the `PangoFontFamily`
 *
 * Since: 1.46
 */
PangoFontFamily *
pango_font_face_get_family (PangoFontFace *face)
{
  g_return_val_if_fail (PANGO_IS_FONT_FACE (face), NULL);

  return PANGO_FONT_FACE_GET_CLASS (face)->get_family (face);
}

/**
 * pango_font_has_char:
 * @font: a `PangoFont`
 * @wc: a Unicode character
 *
 * Returns whether the font provides a glyph for this character.
 *
 * Returns: `TRUE` if @font can render @wc
 *
 * Since: 1.44
 */
gboolean
pango_font_has_char (PangoFont *font,
                     gunichar   wc)
{
  PangoFontClassPrivate *pclass = PANGO_FONT_GET_CLASS_PRIVATE (font);

  return pclass->has_char (font, wc);
}

/**
 * pango_font_get_features:
 * @font: a `PangoFont`
 * @features: (out caller-allocates) (array length=len): Array to features in
 * @len: the length of @features
 * @num_features: (inout): the number of used items in @features
 *
 * Obtain the OpenType features that are provided by the font.
 *
 * These are passed to the rendering system, together with features
 * that have been explicitly set via attributes.
 *
 * Note that this does not include OpenType features which the
 * rendering system enables by default.
 *
 * Since: 1.44
 */
void
pango_font_get_features (PangoFont    *font,
                         hb_feature_t *features,
                         guint         len,
                         guint        *num_features)
{
  if (PANGO_FONT_GET_CLASS (font)->get_features)
    PANGO_FONT_GET_CLASS (font)->get_features (font, features, len, num_features);
}

/**
 * pango_font_get_languages:
 * @font: a `PangoFont`
 *
 * Returns the languages that are supported by @font.
 *
 * If the font backend does not provide this information,
 * %NULL is returned. For the fontconfig backend, this
 * corresponds to the FC_LANG member of the FcPattern.
 *
 * The returned array is only valid as long as the font
 * and its fontmap are valid.
 *
 * Returns: (transfer none) (nullable) (array zero-terminated=1) (element-type PangoLanguage): an array of `PangoLanguage`
 *
 * Since: 1.50
 */
PangoLanguage **
pango_font_get_languages (PangoFont *font)
{
  PangoFontClassPrivate *pclass = PANGO_FONT_GET_CLASS_PRIVATE (font);

  return pclass->get_languages (font);
}

/*< private >
 * pango_font_get_matrix:
 * @font: a `PangoFont`
 *
 * Gets the matrix for the transformation from 'font space' to 'user space'.
 */
void
pango_font_get_matrix (PangoFont   *font,
                       PangoMatrix *matrix)
{
  PangoFontClassPrivate *pclass = PANGO_FONT_GET_CLASS_PRIVATE (font);

  pclass->get_matrix (font, matrix);
}

/*< private >
 * pango_font_is_hinted:
 * @font: a `PangoFont`
 *
 * Gets whether this font is hinted.
 *
 * Returns: %TRUE if @font is hinted
 */
gboolean
pango_font_is_hinted (PangoFont *font)
{
  return PANGO_FONT_GET_CLASS_PRIVATE (font)->is_hinted (font);
}

/*< private >
 * pango_font_get_scale_factors:
 * @font: a `PangoFont`
 * @x_scale: return location for X scale
 * @y_scale: return location for Y scale
 *
 * Gets the font scale factors of the ctm for this font.
 *
 * The ctm is the matrix set on the context that this font was
 * loaded for.
 */
void
pango_font_get_scale_factors (PangoFont *font,
                              double    *x_scale,
                              double    *y_scale)
{
  PANGO_FONT_GET_CLASS_PRIVATE (font)->get_scale_factors (font, x_scale, y_scale);
}
