/* Pango
 * pango-font-description.c:
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

#include "pango-font-description-private.h"

#include "pango-utils-internal.h"


struct _PangoFontDescription
{
  char *family_name;

  PangoStyle style;
  PangoVariant variant;
  PangoWeight weight;
  PangoStretch stretch;
  PangoGravity gravity;

  char *variations;

  guint16 mask;
  guint static_family : 1;
  guint static_variations : 1;
  guint size_is_absolute : 1;

  int size;
};

G_DEFINE_BOXED_TYPE (PangoFontDescription, pango_font_description,
                     pango_font_description_copy,
                     pango_font_description_free);

static const PangoFontDescription pfd_defaults = {
  NULL,                 /* family_name */

  PANGO_STYLE_NORMAL,   /* style */
  PANGO_VARIANT_NORMAL, /* variant */
  PANGO_WEIGHT_NORMAL,  /* weight */
  PANGO_STRETCH_NORMAL, /* stretch */
  PANGO_GRAVITY_SOUTH,  /* gravity */
  NULL,                 /* variations */

  0,                    /* mask */
  0,                    /* static_family */
  0,                    /* static_variations*/
  0,                    /* size_is_absolute */

  0,                    /* size */
};

/**
 * pango_font_description_new:
 *
 * Creates a new font description structure with all fields unset.
 *
 * Return value: the newly allocated `PangoFontDescription`, which
 *   should be freed using [method@Pango.FontDescription.free].
 */
PangoFontDescription *
pango_font_description_new (void)
{
  PangoFontDescription *desc = g_slice_new (PangoFontDescription);

  *desc = pfd_defaults;

  return desc;
}

/**
 * pango_font_description_set_family:
 * @desc: a `PangoFontDescription`.
 * @family: a string representing the family name.
 *
 * Sets the family name field of a font description.
 *
 * The family
 * name represents a family of related font styles, and will
 * resolve to a particular `PangoFontFamily`. In some uses of
 * `PangoFontDescription`, it is also possible to use a comma
 * separated list of family names for this field.
 */
void
pango_font_description_set_family (PangoFontDescription *desc,
                                   const char           *family)
{
  g_return_if_fail (desc != NULL);

  pango_font_description_set_family_static (desc, family ? g_strdup (family) : NULL);
  if (family)
    desc->static_family = FALSE;
}

/**
 * pango_font_description_set_family_static:
 * @desc: a `PangoFontDescription`
 * @family: a string representing the family name
 *
 * Sets the family name field of a font description, without copying the string.
 *
 * This is like [method@Pango.FontDescription.set_family], except that no
 * copy of @family is made. The caller must make sure that the
 * string passed in stays around until @desc has been freed or the
 * name is set again. This function can be used if @family is a static
 * string such as a C string literal, or if @desc is only needed temporarily.
 */
void
pango_font_description_set_family_static (PangoFontDescription *desc,
                                          const char           *family)
{
  g_return_if_fail (desc != NULL);

  if (desc->family_name == family)
    return;

  if (desc->family_name && !desc->static_family)
    g_free (desc->family_name);

  if (family)
    {
      desc->family_name = (char *)family;
      desc->static_family = TRUE;
      desc->mask |= PANGO_FONT_MASK_FAMILY;
    }
  else
    {
      desc->family_name = pfd_defaults.family_name;
      desc->static_family = pfd_defaults.static_family;
      desc->mask &= ~PANGO_FONT_MASK_FAMILY;
    }
}

/**
 * pango_font_description_get_family:
 * @desc: a `PangoFontDescription`.
 *
 * Gets the family name field of a font description.
 *
 * See [method@Pango.FontDescription.set_family].
 *
 * Return value: (nullable): the family name field for the font
 *   description, or %NULL if not previously set. This has the same
 *   life-time as the font description itself and should not be freed.
 */
const char *
pango_font_description_get_family (const PangoFontDescription *desc)
{
  g_return_val_if_fail (desc != NULL, NULL);

  return desc->family_name;
}

/**
 * pango_font_description_set_style:
 * @desc: a `PangoFontDescription`
 * @style: the style for the font description
 *
 * Sets the style field of a `PangoFontDescription`.
 *
 * The [enum@Pango.Style] enumeration describes whether the font is
 * slanted and the manner in which it is slanted; it can be either
 * %PANGO_STYLE_NORMAL, %PANGO_STYLE_ITALIC, or %PANGO_STYLE_OBLIQUE.
 *
 * Most fonts will either have a italic style or an oblique style,
 * but not both, and font matching in Pango will match italic
 * specifications with oblique fonts and vice-versa if an exact
 * match is not found.
 */
void
pango_font_description_set_style (PangoFontDescription *desc,
                                  PangoStyle            style)
{
  g_return_if_fail (desc != NULL);

  desc->style = style;
  desc->mask |= PANGO_FONT_MASK_STYLE;
}

/**
 * pango_font_description_get_style:
 * @desc: a `PangoFontDescription`
 *
 * Gets the style field of a `PangoFontDescription`.
 *
 * See [method@Pango.FontDescription.set_style].
 *
 * Return value: the style field for the font description.
 *   Use [method@Pango.FontDescription.get_set_fields] to
 *   find out if the field was explicitly set or not.
 */
PangoStyle
pango_font_description_get_style (const PangoFontDescription *desc)
{
  g_return_val_if_fail (desc != NULL, pfd_defaults.style);

  return desc->style;
}

/**
 * pango_font_description_set_variant:
 * @desc: a `PangoFontDescription`
 * @variant: the variant type for the font description.
 *
 * Sets the variant field of a font description.
 *
 * The [enum@Pango.Variant] can either be %PANGO_VARIANT_NORMAL
 * or %PANGO_VARIANT_SMALL_CAPS.
 */
void
pango_font_description_set_variant (PangoFontDescription *desc,
                                    PangoVariant          variant)
{
  g_return_if_fail (desc != NULL);

  desc->variant = variant;
  desc->mask |= PANGO_FONT_MASK_VARIANT;
}

/**
 * pango_font_description_get_variant:
 * @desc: a `PangoFontDescription`.
 *
 * Gets the variant field of a `PangoFontDescription`.
 *
 * See [method@Pango.FontDescription.set_variant].
 *
 * Return value: the variant field for the font description.
 *   Use [method@Pango.FontDescription.get_set_fields] to find
 *   out if the field was explicitly set or not.
 */
PangoVariant
pango_font_description_get_variant (const PangoFontDescription *desc)
{
  g_return_val_if_fail (desc != NULL, pfd_defaults.variant);

  return desc->variant;
}

/**
 * pango_font_description_set_weight:
 * @desc: a `PangoFontDescription`
 * @weight: the weight for the font description.
 *
 * Sets the weight field of a font description.
 *
 * The weight field
 * specifies how bold or light the font should be. In addition
 * to the values of the [enum@Pango.Weight] enumeration, other
 * intermediate numeric values are possible.
 */
void
pango_font_description_set_weight (PangoFontDescription *desc,
                                   PangoWeight          weight)
{
  g_return_if_fail (desc != NULL);

  desc->weight = weight;
  desc->mask |= PANGO_FONT_MASK_WEIGHT;
}

/**
 * pango_font_description_get_weight:
 * @desc: a `PangoFontDescription`
 *
 * Gets the weight field of a font description.
 *
 * See [method@Pango.FontDescription.set_weight].
 *
 * Return value: the weight field for the font description.
 *   Use [method@Pango.FontDescription.get_set_fields] to find
 *   out if the field was explicitly set or not.
 */
PangoWeight
pango_font_description_get_weight (const PangoFontDescription *desc)
{
  g_return_val_if_fail (desc != NULL, pfd_defaults.weight);

  return desc->weight;
}

/**
 * pango_font_description_set_stretch:
 * @desc: a `PangoFontDescription`
 * @stretch: the stretch for the font description
 *
 * Sets the stretch field of a font description.
 *
 * The [enum@Pango.Stretch] field specifies how narrow or
 * wide the font should be.
 */
void
pango_font_description_set_stretch (PangoFontDescription *desc,
                                    PangoStretch          stretch)
{
  g_return_if_fail (desc != NULL);

  desc->stretch = stretch;
  desc->mask |= PANGO_FONT_MASK_STRETCH;
}

/**
 * pango_font_description_get_stretch:
 * @desc: a `PangoFontDescription`.
 *
 * Gets the stretch field of a font description.
 *
 * See [method@Pango.FontDescription.set_stretch].
 *
 * Return value: the stretch field for the font description.
 *   Use [method@Pango.FontDescription.get_set_fields] to find
 *   out if the field was explicitly set or not.
 */
PangoStretch
pango_font_description_get_stretch (const PangoFontDescription *desc)
{
  g_return_val_if_fail (desc != NULL, pfd_defaults.stretch);

  return desc->stretch;
}

/**
 * pango_font_description_set_size:
 * @desc: a `PangoFontDescription`
 * @size: the size of the font in points, scaled by %PANGO_SCALE.
 *   (That is, a @size value of 10 * PANGO_SCALE is a 10 point font.
 *   The conversion factor between points and device units depends on
 *   system configuration and the output device. For screen display, a
 *   logical DPI of 96 is common, in which case a 10 point font corresponds
 *   to a 10 * (96 / 72) = 13.3 pixel font.
 *   Use [method@Pango.FontDescription.set_absolute_size] if you need
 *   a particular size in device units.
 *
 * Sets the size field of a font description in fractional points.
 *
 * This is mutually exclusive with
 * [method@Pango.FontDescription.set_absolute_size].
 */
void
pango_font_description_set_size (PangoFontDescription *desc,
                                 gint                  size)
{
  g_return_if_fail (desc != NULL);
  g_return_if_fail (size >= 0);

  desc->size = size;
  desc->size_is_absolute = FALSE;
  desc->mask |= PANGO_FONT_MASK_SIZE;
}

/**
 * pango_font_description_get_size:
 * @desc: a `PangoFontDescription`
 *
 * Gets the size field of a font description.
 *
 * See [method@Pango.FontDescription.set_size].
 *
 * Return value: the size field for the font description in points
 *   or device units. You must call
 *   [method@Pango.FontDescription.get_size_is_absolute] to find out
 *   which is the case. Returns 0 if the size field has not previously
 *   been set or it has been set to 0 explicitly.
 *   Use [method@Pango.FontDescription.get_set_fields] to find out
 *   if the field was explicitly set or not.
 */
gint
pango_font_description_get_size (const PangoFontDescription *desc)
{
  g_return_val_if_fail (desc != NULL, pfd_defaults.size);

  return desc->size;
}

/**
 * pango_font_description_set_absolute_size:
 * @desc: a `PangoFontDescription`
 * @size: the new size, in Pango units. There are %PANGO_SCALE Pango units
 *   in one device unit. For an output backend where a device unit is a pixel,
 *   a @size value of 10 * PANGO_SCALE gives a 10 pixel font.
 *
 * Sets the size field of a font description, in device units.
 *
 * This is mutually exclusive with [method@Pango.FontDescription.set_size]
 * which sets the font size in points.
 *
 * Since: 1.8
 */
void
pango_font_description_set_absolute_size (PangoFontDescription *desc,
                                          double                size)
{
  g_return_if_fail (desc != NULL);
  g_return_if_fail (size >= 0);

  desc->size = size;
  desc->size_is_absolute = TRUE;
  desc->mask |= PANGO_FONT_MASK_SIZE;
}

/**
 * pango_font_description_get_size_is_absolute:
 * @desc: a `PangoFontDescription`
 *
 * Determines whether the size of the font is in points (not absolute)
 * or device units (absolute).
 *
 * See [method@Pango.FontDescription.set_size]
 * and [method@Pango.FontDescription.set_absolute_size].
 *
 * Return value: whether the size for the font description is in
 *   points or device units. Use [method@Pango.FontDescription.get_set_fields]
 *   to find out if the size field of the font description was explicitly
 *   set or not.
 *
 * Since: 1.8
 */
gboolean
pango_font_description_get_size_is_absolute (const PangoFontDescription *desc)
{
  g_return_val_if_fail (desc != NULL, pfd_defaults.size_is_absolute);

  return desc->size_is_absolute;
}

/**
 * pango_font_description_set_gravity:
 * @desc: a `PangoFontDescription`
 * @gravity: the gravity for the font description.
 *
 * Sets the gravity field of a font description.
 *
 * The gravity field
 * specifies how the glyphs should be rotated. If @gravity is
 * %PANGO_GRAVITY_AUTO, this actually unsets the gravity mask on
 * the font description.
 *
 * This function is seldom useful to the user. Gravity should normally
 * be set on a `PangoContext`.
 *
 * Since: 1.16
 */
void
pango_font_description_set_gravity (PangoFontDescription *desc,
                                    PangoGravity          gravity)
{
  g_return_if_fail (desc != NULL);

  if (gravity == PANGO_GRAVITY_AUTO)
    {
      pango_font_description_unset_fields (desc, PANGO_FONT_MASK_GRAVITY);
      return;
    }

  desc->gravity = gravity;
  desc->mask |= PANGO_FONT_MASK_GRAVITY;
}

/**
 * pango_font_description_get_gravity:
 * @desc: a `PangoFontDescription`
 *
 * Gets the gravity field of a font description.
 *
 * See [method@Pango.FontDescription.set_gravity].
 *
 * Return value: the gravity field for the font description.
 *   Use [method@Pango.FontDescription.get_set_fields] to find out
 *   if the field was explicitly set or not.
 *
 * Since: 1.16
 */
PangoGravity
pango_font_description_get_gravity (const PangoFontDescription *desc)
{
  g_return_val_if_fail (desc != NULL, pfd_defaults.gravity);

  return desc->gravity;
}

/**
 * pango_font_description_set_variations_static:
 * @desc: a `PangoFontDescription`
 * @variations: a string representing the variations
 *
 * Sets the variations field of a font description.
 *
 * This is like [method@Pango.FontDescription.set_variations], except
 * that no copy of @variations is made. The caller must make sure that
 * the string passed in stays around until @desc has been freed
 * or the name is set again. This function can be used if
 * @variations is a static string such as a C string literal,
 * or if @desc is only needed temporarily.
 *
 * Since: 1.42
 */
void
pango_font_description_set_variations_static (PangoFontDescription *desc,
                                              const char           *variations)
{
  g_return_if_fail (desc != NULL);

  if (desc->variations == variations)
    return;

  if (desc->variations && !desc->static_variations)
    g_free (desc->variations);

  if (variations)
    {
      desc->variations = (char *)variations;
      desc->static_variations = TRUE;
      desc->mask |= PANGO_FONT_MASK_VARIATIONS;
    }
  else
    {
      desc->variations = pfd_defaults.variations;
      desc->static_variations = pfd_defaults.static_variations;
      desc->mask &= ~PANGO_FONT_MASK_VARIATIONS;
    }
}

/**
 * pango_font_description_set_variations:
 * @desc: a `PangoFontDescription`.
 * @variations: (nullable): a string representing the variations
 *
 * Sets the variations field of a font description.
 *
 * OpenType font variations allow to select a font instance by
 * specifying values for a number of axes, such as width or weight.
 *
 * The format of the variations string is
 *
 *     AXIS1=VALUE,AXIS2=VALUE...
 *
 * with each AXIS a 4 character tag that identifies a font axis,
 * and each VALUE a floating point number. Unknown axes are ignored,
 * and values are clamped to their allowed range.
 *
 * Pango does not currently have a way to find supported axes of
 * a font. Both harfbuzz and freetype have API for this. See
 * for example [hb_ot_var_get_axis_infos](https://harfbuzz.github.io/harfbuzz-hb-ot-var.html#hb-ot-var-get-axis-infos).
 *
 * Since: 1.42
 */
void
pango_font_description_set_variations (PangoFontDescription *desc,
                                       const char           *variations)
{
  g_return_if_fail (desc != NULL);

  pango_font_description_set_variations_static (desc, g_strdup (variations));
  if (variations)
    desc->static_variations = FALSE;
}

/**
 * pango_font_description_get_variations:
 * @desc: a `PangoFontDescription`
 *
 * Gets the variations field of a font description.
 *
 * See [method@Pango.FontDescription.set_variations].
 *
 * Return value: (nullable): the variations field for the font
 *   description, or %NULL if not previously set. This has the same
 *   life-time as the font description itself and should not be freed.
 *
 * Since: 1.42
 */
const char *
pango_font_description_get_variations (const PangoFontDescription *desc)
{
  g_return_val_if_fail (desc != NULL, NULL);

  return desc->variations;
}

/**
 * pango_font_description_get_set_fields:
 * @desc: a `PangoFontDescription`
 *
 * Determines which fields in a font description have been set.
 *
 * Return value: a bitmask with bits set corresponding to the
 *   fields in @desc that have been set.
 */
PangoFontMask
pango_font_description_get_set_fields (const PangoFontDescription *desc)
{
  g_return_val_if_fail (desc != NULL, pfd_defaults.mask);

  return desc->mask;
}

/**
 * pango_font_description_unset_fields:
 * @desc: a `PangoFontDescription`
 * @to_unset: bitmask of fields in the @desc to unset.
 *
 * Unsets some of the fields in a `PangoFontDescription`.
 *
 * The unset fields will get back to their default values.
 */
void
pango_font_description_unset_fields (PangoFontDescription *desc,
                                     PangoFontMask         to_unset)
{
  PangoFontDescription unset_desc;

  g_return_if_fail (desc != NULL);

  unset_desc = pfd_defaults;
  unset_desc.mask = to_unset;

  pango_font_description_merge_static (desc, &unset_desc, TRUE);

  desc->mask &= ~to_unset;
}

/**
 * pango_font_description_merge:
 * @desc: a `PangoFontDescription`
 * @desc_to_merge: (nullable): the `PangoFontDescription` to merge from,
 *   or %NULL
 * @replace_existing: if %TRUE, replace fields in @desc with the
 *   corresponding values from @desc_to_merge, even if they
 *   are already exist.
 *
 * Merges the fields that are set in @desc_to_merge into the fields in
 * @desc.
 *
 * If @replace_existing is %FALSE, only fields in @desc that
 * are not already set are affected. If %TRUE, then fields that are
 * already set will be replaced as well.
 *
 * If @desc_to_merge is %NULL, this function performs nothing.
 */
void
pango_font_description_merge (PangoFontDescription       *desc,
                              const PangoFontDescription *desc_to_merge,
                              gboolean                    replace_existing)
{
  gboolean family_merged;
  gboolean variations_merged;

  g_return_if_fail (desc != NULL);

  if (desc_to_merge == NULL)
    return;

  family_merged = desc_to_merge->family_name && (replace_existing || !desc->family_name);
  variations_merged = desc_to_merge->variations && (replace_existing || !desc->variations);

  pango_font_description_merge_static (desc, desc_to_merge, replace_existing);

  if (family_merged)
    {
      desc->family_name = g_strdup (desc->family_name);
      desc->static_family = FALSE;
    }

  if (variations_merged)
    {
      desc->variations = g_strdup (desc->variations);
      desc->static_variations = FALSE;
    }
}

/**
 * pango_font_description_merge_static:
 * @desc: a `PangoFontDescription`
 * @desc_to_merge: the `PangoFontDescription` to merge from
 * @replace_existing: if %TRUE, replace fields in @desc with the
 *   corresponding values from @desc_to_merge, even if they
 *   are already exist.
 *
 * Merges the fields that are set in @desc_to_merge into the fields in
 * @desc, without copying allocated fields.
 *
 * This is like [method@Pango.FontDescription.merge], but only a shallow copy
 * is made of the family name and other allocated fields. @desc can only
 * be used until @desc_to_merge is modified or freed. This is meant to
 * be used when the merged font description is only needed temporarily.
 */
void
pango_font_description_merge_static (PangoFontDescription       *desc,
                                     const PangoFontDescription *desc_to_merge,
                                     gboolean                    replace_existing)
{
  PangoFontMask new_mask;

  g_return_if_fail (desc != NULL);
  g_return_if_fail (desc_to_merge != NULL);

  if (replace_existing)
    new_mask = desc_to_merge->mask;
  else
    new_mask = desc_to_merge->mask & ~desc->mask;

  if (new_mask & PANGO_FONT_MASK_FAMILY)
    pango_font_description_set_family_static (desc, desc_to_merge->family_name);
  if (new_mask & PANGO_FONT_MASK_STYLE)
    desc->style = desc_to_merge->style;
  if (new_mask & PANGO_FONT_MASK_VARIANT)
    desc->variant = desc_to_merge->variant;
  if (new_mask & PANGO_FONT_MASK_WEIGHT)
    desc->weight = desc_to_merge->weight;
  if (new_mask & PANGO_FONT_MASK_STRETCH)
    desc->stretch = desc_to_merge->stretch;
  if (new_mask & PANGO_FONT_MASK_SIZE)
    {
      desc->size = desc_to_merge->size;
      desc->size_is_absolute = desc_to_merge->size_is_absolute;
    }
  if (new_mask & PANGO_FONT_MASK_GRAVITY)
    desc->gravity = desc_to_merge->gravity;
  if (new_mask & PANGO_FONT_MASK_VARIATIONS)
    pango_font_description_set_variations_static (desc, desc_to_merge->variations);

  desc->mask |= new_mask;
}

static gint
compute_distance (const PangoFontDescription *a,
                  const PangoFontDescription *b)
{
  if (a->style == b->style)
    {
      return abs((int)(a->weight) - (int)(b->weight));
    }
  else if (a->style != PANGO_STYLE_NORMAL &&
           b->style != PANGO_STYLE_NORMAL)
    {
      /* Equate oblique and italic, but with a big penalty
       */
      return 1000000 + abs ((int)(a->weight) - (int)(b->weight));
    }
  else
    return G_MAXINT;
}

/**
 * pango_font_description_better_match:
 * @desc: a `PangoFontDescription`
 * @old_match: (nullable): a `PangoFontDescription`, or %NULL
 * @new_match: a `PangoFontDescription`
 *
 * Determines if the style attributes of @new_match are a closer match
 * for @desc than those of @old_match are, or if @old_match is %NULL,
 * determines if @new_match is a match at all.
 *
 * Approximate matching is done for weight and style; other style attributes
 * must match exactly. Style attributes are all attributes other than family
 * and size-related attributes. Approximate matching for style considers
 * %PANGO_STYLE_OBLIQUE and %PANGO_STYLE_ITALIC as matches, but not as good
 * a match as when the styles are equal.
 *
 * Note that @old_match must match @desc.
 *
 * Return value: %TRUE if @new_match is a better match
 */
gboolean
pango_font_description_better_match (const PangoFontDescription *desc,
                                     const PangoFontDescription *old_match,
                                     const PangoFontDescription *new_match)
{
  g_return_val_if_fail (desc != NULL, G_MAXINT);
  g_return_val_if_fail (new_match != NULL, G_MAXINT);

  if (new_match->variant == desc->variant &&
      new_match->stretch == desc->stretch &&
      new_match->gravity == desc->gravity)
    {
      int old_distance = old_match ? compute_distance (desc, old_match) : G_MAXINT;
      int new_distance = compute_distance (desc, new_match);

      if (new_distance < old_distance)
        return TRUE;
    }

  return FALSE;
}

/**
 * pango_font_description_copy:
 * @desc: (nullable): a `PangoFontDescription`, may be %NULL
 *
 * Make a copy of a `PangoFontDescription`.
 *
 * Return value: (nullable): the newly allocated `PangoFontDescription`,
 *   which should be freed with [method@Pango.FontDescription.free],
 *   or %NULL if @desc was %NULL.
 */
PangoFontDescription *
pango_font_description_copy (const PangoFontDescription *desc)
{
  PangoFontDescription *result;

  if (desc == NULL)
    return NULL;

  result = g_slice_new (PangoFontDescription);

  *result = *desc;

  if (result->family_name)
    {
      result->family_name = g_strdup (result->family_name);
      result->static_family = FALSE;
    }

  result->variations = g_strdup (result->variations);
  result->static_variations = FALSE;

  return result;
}

/**
 * pango_font_description_copy_static:
 * @desc: (nullable): a `PangoFontDescription`, may be %NULL
 *
 * Make a copy of a `PangoFontDescription`, but don't duplicate
 * allocated fields.
 *
 * This is like [method@Pango.FontDescription.copy], but only a shallow
 * copy is made of the family name and other allocated fields. The result
 * can only be used until @desc is modified or freed. This is meant
 * to be used when the copy is only needed temporarily.
 *
 * Return value: (nullable): the newly allocated `PangoFontDescription`,
 *   which should be freed with [method@Pango.FontDescription.free],
 *   or %NULL if @desc was %NULL.
 */
PangoFontDescription *
pango_font_description_copy_static (const PangoFontDescription *desc)
{
  PangoFontDescription *result;

  if (desc == NULL)
    return NULL;

  result = g_slice_new (PangoFontDescription);

  *result = *desc;
  if (result->family_name)
    result->static_family = TRUE;


  if (result->variations)
    result->static_variations = TRUE;

  return result;
}

/**
 * pango_font_description_equal:
 * @desc1: a `PangoFontDescription`
 * @desc2: another `PangoFontDescription`
 *
 * Compares two font descriptions for equality.
 *
 * Two font descriptions are considered equal if the fonts they describe
 * are provably identical. This means that their masks do not have to match,
 * as long as other fields are all the same. (Two font descriptions may
 * result in identical fonts being loaded, but still compare %FALSE.)
 *
 * Return value: %TRUE if the two font descriptions are identical,
 *   %FALSE otherwise.
 */
gboolean
pango_font_description_equal (const PangoFontDescription *desc1,
                              const PangoFontDescription *desc2)
{
  g_return_val_if_fail (desc1 != NULL, FALSE);
  g_return_val_if_fail (desc2 != NULL, FALSE);

  return desc1->style == desc2->style &&
         desc1->variant == desc2->variant &&
         desc1->weight == desc2->weight &&
         desc1->stretch == desc2->stretch &&
         desc1->size == desc2->size &&
         desc1->size_is_absolute == desc2->size_is_absolute &&
         desc1->gravity == desc2->gravity &&
         (desc1->family_name == desc2->family_name ||
          (desc1->family_name && desc2->family_name && g_ascii_strcasecmp (desc1->family_name, desc2->family_name) == 0)) &&
         (g_strcmp0 (desc1->variations, desc2->variations) == 0);
}

#define TOLOWER(c) \
  (((c) >= 'A' && (c) <= 'Z') ? (c) - 'A' + 'a' : (c))

static guint
case_insensitive_hash (const char *key)
{
  const char *p = key;
  guint h = TOLOWER (*p);

  if (h)
    {
      for (p += 1; *p != '\0'; p++)
        h = (h << 5) - h + TOLOWER (*p);
    }

  return h;
}

/**
 * pango_font_description_hash:
 * @desc: a `PangoFontDescription`
 *
 * Computes a hash of a `PangoFontDescription` structure.
 *
 * This is suitable to be used, for example, as an argument
 * to g_hash_table_new(). The hash value is independent of @desc->mask.
 *
 * Return value: the hash value.
 */
guint
pango_font_description_hash (const PangoFontDescription *desc)
{
  guint hash = 0;

  g_return_val_if_fail (desc != NULL, 0);

  if (desc->family_name)
    hash = case_insensitive_hash (desc->family_name);
  if (desc->variations)
    hash ^= g_str_hash (desc->variations);
  hash ^= desc->size;
  hash ^= desc->size_is_absolute ? 0xc33ca55a : 0;
  hash ^= desc->style << 16;
  hash ^= desc->variant << 18;
  hash ^= desc->weight << 16;
  hash ^= desc->stretch << 26;
  hash ^= desc->gravity << 28;

  return hash;
}

/**
 * pango_font_description_free:
 * @desc: (nullable): a `PangoFontDescription`, may be %NULL
 *
 * Frees a font description.
 */
void
pango_font_description_free (PangoFontDescription *desc)
{
  if (desc == NULL)
    return;

  if (desc->family_name && !desc->static_family)
    g_free (desc->family_name);

  if (desc->variations && !desc->static_variations)
    g_free (desc->variations);

  g_slice_free (PangoFontDescription, desc);
}

typedef struct
{
  int value;
  const char str[16];
} FieldMap;

static const FieldMap style_map[] = {
  { PANGO_STYLE_NORMAL, "" },
  { PANGO_STYLE_NORMAL, "Roman" },
  { PANGO_STYLE_OBLIQUE, "Oblique" },
  { PANGO_STYLE_ITALIC, "Italic" }
};

static const FieldMap variant_map[] = {
  { PANGO_VARIANT_NORMAL, "" },
  { PANGO_VARIANT_SMALL_CAPS, "Small-Caps" },
  { PANGO_VARIANT_ALL_SMALL_CAPS, "All-Small-Caps" },
  { PANGO_VARIANT_PETITE_CAPS, "Petite-Caps" },
  { PANGO_VARIANT_ALL_PETITE_CAPS, "All-Petite-Caps" },
  { PANGO_VARIANT_UNICASE, "Unicase" },
  { PANGO_VARIANT_TITLE_CAPS, "Title-Caps" }
};

static const FieldMap weight_map[] = {
  { PANGO_WEIGHT_THIN, "Thin" },
  { PANGO_WEIGHT_ULTRALIGHT, "Ultra-Light" },
  { PANGO_WEIGHT_ULTRALIGHT, "Extra-Light" },
  { PANGO_WEIGHT_LIGHT, "Light" },
  { PANGO_WEIGHT_SEMILIGHT, "Semi-Light" },
  { PANGO_WEIGHT_SEMILIGHT, "Demi-Light" },
  { PANGO_WEIGHT_BOOK, "Book" },
  { PANGO_WEIGHT_NORMAL, "" },
  { PANGO_WEIGHT_NORMAL, "Regular" },
  { PANGO_WEIGHT_MEDIUM, "Medium" },
  { PANGO_WEIGHT_SEMIBOLD, "Semi-Bold" },
  { PANGO_WEIGHT_SEMIBOLD, "Demi-Bold" },
  { PANGO_WEIGHT_BOLD, "Bold" },
  { PANGO_WEIGHT_ULTRABOLD, "Ultra-Bold" },
  { PANGO_WEIGHT_ULTRABOLD, "Extra-Bold" },
  { PANGO_WEIGHT_HEAVY, "Heavy" },
  { PANGO_WEIGHT_HEAVY, "Black" },
  { PANGO_WEIGHT_ULTRAHEAVY, "Ultra-Heavy" },
  { PANGO_WEIGHT_ULTRAHEAVY, "Extra-Heavy" },
  { PANGO_WEIGHT_ULTRAHEAVY, "Ultra-Black" },
  { PANGO_WEIGHT_ULTRAHEAVY, "Extra-Black" }
};

static const FieldMap stretch_map[] = {
  { PANGO_STRETCH_ULTRA_CONDENSED, "Ultra-Condensed" },
  { PANGO_STRETCH_EXTRA_CONDENSED, "Extra-Condensed" },
  { PANGO_STRETCH_CONDENSED,       "Condensed" },
  { PANGO_STRETCH_SEMI_CONDENSED,  "Semi-Condensed" },
  { PANGO_STRETCH_NORMAL,          "" },
  { PANGO_STRETCH_SEMI_EXPANDED,   "Semi-Expanded" },
  { PANGO_STRETCH_EXPANDED,        "Expanded" },
  { PANGO_STRETCH_EXTRA_EXPANDED,  "Extra-Expanded" },
  { PANGO_STRETCH_ULTRA_EXPANDED,  "Ultra-Expanded" }
};

static const FieldMap gravity_map[] = {
  { PANGO_GRAVITY_SOUTH, "Not-Rotated" },
  { PANGO_GRAVITY_SOUTH, "South" },
  { PANGO_GRAVITY_NORTH, "Upside-Down" },
  { PANGO_GRAVITY_NORTH, "North" },
  { PANGO_GRAVITY_EAST,  "Rotated-Left" },
  { PANGO_GRAVITY_EAST,  "East" },
  { PANGO_GRAVITY_WEST,  "Rotated-Right" },
  { PANGO_GRAVITY_WEST,  "West" }
};

static gboolean
field_matches (const gchar *s1,
               const gchar *s2,
               gsize n)
{
  gint c1, c2;

  g_return_val_if_fail (s1 != NULL, 0);
  g_return_val_if_fail (s2 != NULL, 0);

  while (n && *s1 && *s2)
    {
      c1 = (gint)(guchar) TOLOWER (*s1);
      c2 = (gint)(guchar) TOLOWER (*s2);
      if (c1 != c2) {
        if (c1 == '-') {
          s1++;
          continue;
        }
        return FALSE;
      }
      s1++; s2++;
      n--;
    }

  return n == 0 && *s1 == '\0';
}

static gboolean
parse_int (const char *word,
           size_t      wordlen,
           int        *out)
{
  char *end;
  long val = strtol (word, &end, 10);
  int i = val;

  if (end != word && (end == word + wordlen) && val >= 0 && val == i)
    {
      if (out)
        *out = i;

      return TRUE;
    }

  return FALSE;
}

static gboolean
find_field (const char *what,
            const FieldMap *map,
            int n_elements,
            const char *str,
            int len,
            int *val)
{
  int i;
  gboolean had_prefix = FALSE;

  if (what)
    {
      i = strlen (what);
      if (len > i && 0 == strncmp (what, str, i) && str[i] == '=')
        {
          str += i + 1;
          len -= i + 1;
          had_prefix = TRUE;
        }
    }

  for (i=0; i<n_elements; i++)
    {
      if (map[i].str[0] && field_matches (map[i].str, str, len))
        {
          if (val)
            *val = map[i].value;
          return TRUE;
        }
    }

  if (!what || had_prefix)
    return parse_int (str, len, val);

  return FALSE;
}

static gboolean
find_field_any (const char *str, int len, PangoFontDescription *desc)
{
  if (field_matches ("Normal", str, len))
    return TRUE;

#define FIELD(NAME, MASK) \
  G_STMT_START { \
  if (find_field (G_STRINGIFY (NAME), NAME##_map, G_N_ELEMENTS (NAME##_map), str, len, \
                  desc ? (int *)(void *)&desc->NAME : NULL)) \
    { \
      if (desc) \
        desc->mask |= MASK; \
      return TRUE; \
    } \
  } G_STMT_END

  FIELD (weight,  PANGO_FONT_MASK_WEIGHT);
  FIELD (style,   PANGO_FONT_MASK_STYLE);
  FIELD (stretch, PANGO_FONT_MASK_STRETCH);
  FIELD (variant, PANGO_FONT_MASK_VARIANT);
  FIELD (gravity, PANGO_FONT_MASK_GRAVITY);

#undef FIELD

  return FALSE;
}

static const char *
getword (const char *str, const char *last, size_t *wordlen, const char *stop)
{
  const char *result;

  while (last > str && g_ascii_isspace (*(last - 1)))
    last--;

  result = last;
  while (result > str && !g_ascii_isspace (*(result - 1)) && !strchr (stop, *(result - 1)))
    result--;

  *wordlen = last - result;

  return result;
}

static gboolean
parse_size (const char *word,
            size_t      wordlen,
            int        *pango_size,
            gboolean   *size_is_absolute)
{
  char *end;
  double size = g_ascii_strtod (word, &end);

  if (end != word &&
      (end == word + wordlen ||
       (end + 2 == word + wordlen && !strncmp (end, "px", 2))
      ) && size >= 0 && size <= 1000000) /* word is a valid float */
    {
      if (pango_size)
        *pango_size = (int)(size * PANGO_SCALE + 0.5);

      if (size_is_absolute)
        *size_is_absolute = end < word + wordlen;

      return TRUE;
    }

  return FALSE;
}

static gboolean
parse_variations (const char  *word,
                  size_t       wordlen,
                  char       **variations)
{
  if (word[0] != '@')
    {
      *variations = NULL;
      return FALSE;
    }

  /* XXX: actually validate here */
  *variations = g_strndup (word + 1, wordlen - 1);

  return TRUE;
}

/**
 * pango_font_description_from_string:
 * @str: string representation of a font description.
 *
 * Creates a new font description from a string representation.
 *
 * The string must have the form
 *
 *     "\[FAMILY-LIST] \[STYLE-OPTIONS] \[SIZE] \[VARIATIONS]",
 *
 * where FAMILY-LIST is a comma-separated list of families optionally
 * terminated by a comma, STYLE_OPTIONS is a whitespace-separated list
 * of words where each word describes one of style, variant, weight,
 * stretch, or gravity, and SIZE is a decimal number (size in points)
 * or optionally followed by the unit modifier "px" for absolute size.
 * VARIATIONS is a comma-separated list of font variation
 * specifications of the form "\@axis=value" (the = sign is optional).
 *
 * The following words are understood as styles:
 * "Normal", "Roman", "Oblique", "Italic".
 *
 * The following words are understood as variants:
 * "Small-Caps", "All-Small-Caps", "Petite-Caps", "All-Petite-Caps",
 * "Unicase", "Title-Caps".
 *
 * The following words are understood as weights:
 * "Thin", "Ultra-Light", "Extra-Light", "Light", "Semi-Light",
 * "Demi-Light", "Book", "Regular", "Medium", "Semi-Bold", "Demi-Bold",
 * "Bold", "Ultra-Bold", "Extra-Bold", "Heavy", "Black", "Ultra-Black",
 * "Extra-Black".
 *
 * The following words are understood as stretch values:
 * "Ultra-Condensed", "Extra-Condensed", "Condensed", "Semi-Condensed",
 * "Semi-Expanded", "Expanded", "Extra-Expanded", "Ultra-Expanded".
 *
 * The following words are understood as gravity values:
 * "Not-Rotated", "South", "Upside-Down", "North", "Rotated-Left",
 * "East", "Rotated-Right", "West".
 *
 * Any one of the options may be absent. If FAMILY-LIST is absent, then
 * the family_name field of the resulting font description will be
 * initialized to %NULL. If STYLE-OPTIONS is missing, then all style
 * options will be set to the default values. If SIZE is missing, the
 * size in the resulting font description will be set to 0.
 *
 * A typical example:
 *
 *     "Cantarell Italic Light 15 \@wght=200"
 *
 * Return value: a new `PangoFontDescription`.
 */
PangoFontDescription *
pango_font_description_from_string (const char *str)
{
  PangoFontDescription *desc;
  const char *p, *last;
  size_t len, wordlen;

  g_return_val_if_fail (str != NULL, NULL);

  desc = pango_font_description_new ();

  desc->mask = PANGO_FONT_MASK_STYLE |
               PANGO_FONT_MASK_WEIGHT |
               PANGO_FONT_MASK_VARIANT |
               PANGO_FONT_MASK_STRETCH;

  len = strlen (str);
  last = str + len;
  p = getword (str, last, &wordlen, "");
  /* Look for variations at the end of the string */
  if (wordlen != 0)
    {
      if (parse_variations (p, wordlen, &desc->variations))
        {
          desc->mask |= PANGO_FONT_MASK_VARIATIONS;
          last = p;
        }
    }

  p = getword (str, last, &wordlen, ",");
  /* Look for a size */
  if (wordlen != 0)
    {
      gboolean size_is_absolute;
      if (parse_size (p, wordlen, &desc->size, &size_is_absolute))
        {
          desc->size_is_absolute = size_is_absolute;
          desc->mask |= PANGO_FONT_MASK_SIZE;
          last = p;
        }
    }

  /* Now parse style words
   */
  p = getword (str, last, &wordlen, ",");
  while (wordlen != 0)
    {
      if (!find_field_any (p, wordlen, desc))
        break;
      else
        {
          last = p;
          p = getword (str, last, &wordlen, ",");
        }
    }

  /* Remainder (str => p) is family list. Trim off trailing commas and leading and trailing white space
   */

  while (last > str && g_ascii_isspace (*(last - 1)))
    last--;

  if (last > str && *(last - 1) == ',')
    last--;

  while (last > str && g_ascii_isspace (*(last - 1)))
    last--;

  while (last > str && g_ascii_isspace (*str))
    str++;

  if (str != last)
    {
      int i;
      char **families;

      desc->family_name = g_strndup (str, last - str);

      /* Now sanitize it to trim space from around individual family names.
       * bug #499624 */

      families = g_strsplit (desc->family_name, ",", -1);

      for (i = 0; families[i]; i++)
        g_strstrip (families[i]);

      g_free (desc->family_name);
      desc->family_name = g_strjoinv (",", families);
      g_strfreev (families);

      desc->mask |= PANGO_FONT_MASK_FAMILY;
    }

  return desc;
}

static void
append_field (GString *str, const char *what, const FieldMap *map, int n_elements, int val)
{
  int i;
  for (i=0; i<n_elements; i++)
    {
      if (map[i].value != val)
        continue;

      if (G_LIKELY (map[i].str[0]))
        {
          if (G_LIKELY (str->len > 0 && str->str[str->len -1] != ' '))
            g_string_append_c (str, ' ');
          g_string_append (str, map[i].str);
        }
      return;
    }

  if (G_LIKELY (str->len > 0 || str->str[str->len -1] != ' '))
    g_string_append_c (str, ' ');
  g_string_append_printf (str, "%s=%d", what, val);
}

/**
 * pango_font_description_to_string:
 * @desc: a `PangoFontDescription`
 *
 * Creates a string representation of a font description.
 *
 * See [func@Pango.FontDescription.from_string] for a description
 * of the format of the string representation. The family list in
 * the string description will only have a terminating comma if
 * the last word of the list is a valid style option.
 *
 * Return value: a new string that must be freed with g_free().
 */
char *
pango_font_description_to_string (const PangoFontDescription *desc)
{
  GString *result;

  g_return_val_if_fail (desc != NULL, NULL);

  result = g_string_new (NULL);

  if (G_LIKELY (desc->family_name && desc->mask & PANGO_FONT_MASK_FAMILY))
    {
      const char *p;
      size_t wordlen;

      g_string_append (result, desc->family_name);

      /* We need to add a trailing comma if the family name ends
       * in a keyword like "Bold", or if the family name ends in
       * a number and no keywords will be added.
       */
      p = getword (desc->family_name, desc->family_name + strlen(desc->family_name), &wordlen, ",");
      if (wordlen != 0 &&
          (find_field_any (p, wordlen, NULL) ||
           (parse_size (p, wordlen, NULL, NULL) &&
            desc->weight == PANGO_WEIGHT_NORMAL &&
            desc->style == PANGO_STYLE_NORMAL &&
            desc->stretch == PANGO_STRETCH_NORMAL &&
            desc->variant == PANGO_VARIANT_NORMAL &&
            (desc->mask & (PANGO_FONT_MASK_GRAVITY | PANGO_FONT_MASK_SIZE)) == 0)))
        g_string_append_c (result, ',');
    }

#define FIELD(NAME, MASK) \
  append_field (result, G_STRINGIFY (NAME), NAME##_map, G_N_ELEMENTS (NAME##_map), desc->NAME)

  FIELD (weight,  PANGO_FONT_MASK_WEIGHT);
  FIELD (style,   PANGO_FONT_MASK_STYLE);
  FIELD (stretch, PANGO_FONT_MASK_STRETCH);
  FIELD (variant, PANGO_FONT_MASK_VARIANT);
  if (desc->mask & PANGO_FONT_MASK_GRAVITY)
    FIELD (gravity, PANGO_FONT_MASK_GRAVITY);

#undef FIELD

  if (result->len == 0)
    g_string_append (result, "Normal");

  if (desc->mask & PANGO_FONT_MASK_SIZE)
    {
      char buf[G_ASCII_DTOSTR_BUF_SIZE];

      if (result->len > 0 || result->str[result->len -1] != ' ')
        g_string_append_c (result, ' ');

      g_ascii_dtostr (buf, sizeof (buf), (double)desc->size / PANGO_SCALE);
      g_string_append (result, buf);

      if (desc->size_is_absolute)
        g_string_append (result, "px");
    }

  if ((desc->variations && desc->mask & PANGO_FONT_MASK_VARIATIONS) &&
      desc->variations[0] != '\0')
    {
      g_string_append (result, " @");
      g_string_append (result, desc->variations);
    }

  return g_string_free (result, FALSE);
}

/**
 * pango_font_description_to_filename:
 * @desc: a `PangoFontDescription`
 *
 * Creates a filename representation of a font description.
 *
 * The filename is identical to the result from calling
 * [method@Pango.FontDescription.to_string], but with underscores
 * instead of characters that are untypical in filenames, and in
 * lower case only.
 *
 * Return value: a new string that must be freed with g_free().
 */
char *
pango_font_description_to_filename (const PangoFontDescription *desc)
{
  char *result;
  char *p;

  g_return_val_if_fail (desc != NULL, NULL);

  result = pango_font_description_to_string (desc);

  p = result;
  while (*p)
    {
      if (G_UNLIKELY ((guchar) *p >= 128))
        /* skip over non-ASCII chars */;
      else if (strchr ("-+_.", *p) == NULL && !g_ascii_isalnum (*p))
        *p = '_';
      else
        *p = g_ascii_tolower (*p);
      p++;
    }

  return result;
}

static gboolean
parse_field (const char *what,
             const FieldMap *map,
             int n_elements,
             const char *str,
             int *val,
             gboolean warn)
{
  gboolean found;
  int len = strlen (str);

  if (G_UNLIKELY (*str == '\0'))
    return FALSE;

  if (field_matches ("Normal", str, len))
    {
      /* find the map entry with empty string */
      int i;

      for (i = 0; i < n_elements; i++)
        if (map[i].str[0] == '\0')
          {
            *val = map[i].value;
            return TRUE;
          }

      *val = 0;
      return TRUE;
    }

  found = find_field (NULL, map, n_elements, str, len, val);

  if (!found && warn)
    {
        int i;
        GString *s = g_string_new (NULL);

        for (i = 0; i < n_elements; i++)
          {
            if (i)
              g_string_append_c (s, '/');
            g_string_append (s, map[i].str[0] == '\0' ? "Normal" : map[i].str);
          }

        g_warning ("%s must be one of %s or a number",
                   what,
                   s->str);

        g_string_free (s, TRUE);
    }

  return found;
}

#define FIELD(NAME, MASK) \
  parse_field (G_STRINGIFY (NAME), NAME##_map, G_N_ELEMENTS (NAME##_map), str, (int *)(void *)NAME, warn)

/**
 * pango_parse_style:
 * @str: a string to parse.
 * @style: (out): a `PangoStyle` to store the result in.
 * @warn: if %TRUE, issue a g_warning() on bad input.
 *
 * Parses a font style.
 *
 * The allowed values are "normal", "italic" and "oblique", case
 * variations being
 * ignored.
 *
 * Return value: %TRUE if @str was successfully parsed.
 */
gboolean
pango_parse_style (const char *str,
                   PangoStyle *style,
                   gboolean    warn)
{
  return FIELD (style,   PANGO_FONT_MASK_STYLE);
}

/**
 * pango_parse_variant:
 * @str: a string to parse.
 * @variant: (out): a `PangoVariant` to store the result in.
 * @warn: if %TRUE, issue a g_warning() on bad input.
 *
 * Parses a font variant.
 *
 * The allowed values are "normal", "small-caps", "all-small-caps",
 * "petite-caps", "all-petite-caps", "unicase" and "title-caps",
 * case variations being ignored.
 *
 * Return value: %TRUE if @str was successfully parsed.
 */
gboolean
pango_parse_variant (const char   *str,
                     PangoVariant *variant,
                     gboolean      warn)
{
  return FIELD (variant, PANGO_FONT_MASK_VARIANT);
}

/**
 * pango_parse_weight:
 * @str: a string to parse.
 * @weight: (out): a `PangoWeight` to store the result in.
 * @warn: if %TRUE, issue a g_warning() on bad input.
 *
 * Parses a font weight.
 *
 * The allowed values are "heavy",
 * "ultrabold", "bold", "normal", "light", "ultraleight"
 * and integers. Case variations are ignored.
 *
 * Return value: %TRUE if @str was successfully parsed.
 */
gboolean
pango_parse_weight (const char  *str,
                    PangoWeight *weight,
                    gboolean     warn)
{
  return FIELD (weight,  PANGO_FONT_MASK_WEIGHT);
}

/**
 * pango_parse_stretch:
 * @str: a string to parse.
 * @stretch: (out): a `PangoStretch` to store the result in.
 * @warn: if %TRUE, issue a g_warning() on bad input.
 *
 * Parses a font stretch.
 *
 * The allowed values are
 * "ultra_condensed", "extra_condensed", "condensed",
 * "semi_condensed", "normal", "semi_expanded", "expanded",
 * "extra_expanded" and "ultra_expanded". Case variations are
 * ignored and the '_' characters may be omitted.
 *
 * Return value: %TRUE if @str was successfully parsed.
 */
gboolean
pango_parse_stretch (const char   *str,
                     PangoStretch *stretch,
                     gboolean      warn)
{
  return FIELD (stretch, PANGO_FONT_MASK_STRETCH);
}
