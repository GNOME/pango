/* Pango2
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
#include <stdlib.h>
#include <math.h>
#include <locale.h>

#include "pango-font-description-private.h"


struct _Pango2FontDescription
{
  char *family_name;

  Pango2Style style;
  Pango2Variant variant;
  Pango2Weight weight;
  Pango2Stretch stretch;
  Pango2Gravity gravity;

  int size;

  char *variations;
  char *faceid;

  GQuark palette;

  guint16 mask;
  guint static_family : 1;
  guint static_variations : 1;
  guint static_faceid : 1;
  guint size_is_absolute : 1;
};

G_DEFINE_BOXED_TYPE (Pango2FontDescription, pango2_font_description,
                     pango2_font_description_copy,
                     pango2_font_description_free);

static const Pango2FontDescription pfd_defaults = {
  NULL,                 /* family_name */

  PANGO2_STYLE_NORMAL,   /* style */
  PANGO2_VARIANT_NORMAL, /* variant */
  PANGO2_WEIGHT_NORMAL,  /* weight */
  PANGO2_STRETCH_NORMAL, /* stretch */
  PANGO2_GRAVITY_SOUTH,  /* gravity */
  0,                    /* size */
  NULL,                 /* variations */
  NULL,                 /* faceid */
  0,                    /* palette */

  0,                    /* mask */
  0,                    /* static_family */
  0,                    /* static_variations */
  0,                    /* static_faceid */
  0,                    /* size_is_absolute */
};

/**
 * pango2_font_description_new:
 *
 * Creates a new font description structure with all fields unset.
 *
 * Return value: the newly allocated `Pango2FontDescription`, which
 *   should be freed using [method@Pango2.FontDescription.free]
 */
Pango2FontDescription *
pango2_font_description_new (void)
{
  Pango2FontDescription *desc = g_slice_new (Pango2FontDescription);

  *desc = pfd_defaults;

  return desc;
}

/**
 * pango2_font_description_set_family:
 * @desc: a `Pango2FontDescription`.
 * @family: a string representing the family name.
 *
 * Sets the family name field of a font description.
 *
 * The family
 * name represents a family of related font styles, and will
 * resolve to a particular `Pango2FontFamily`. In some uses of
 * `Pango2FontDescription`, it is also possible to use a comma
 * separated list of family names for this field.
 */
void
pango2_font_description_set_family (Pango2FontDescription *desc,
                                    const char            *family)
{
  g_return_if_fail (desc != NULL);

  pango2_font_description_set_family_static (desc, family ? g_strdup (family) : NULL);
  if (family)
    desc->static_family = FALSE;
}

/**
 * pango2_font_description_set_family_static:
 * @desc: a `Pango2FontDescription`
 * @family: a string representing the family name
 *
 * Sets the family name field of a font description, without copying the string.
 *
 * This is like [method@Pango2.FontDescription.set_family], except that no
 * copy of @family is made. The caller must make sure that the
 * string passed in stays around until @desc has been freed or the
 * name is set again. This function can be used if @family is a static
 * string such as a C string literal, or if @desc is only needed temporarily.
 */
void
pango2_font_description_set_family_static (Pango2FontDescription *desc,
                                           const char            *family)
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
      desc->mask |= PANGO2_FONT_MASK_FAMILY;
    }
  else
    {
      desc->family_name = pfd_defaults.family_name;
      desc->static_family = pfd_defaults.static_family;
      desc->mask &= ~PANGO2_FONT_MASK_FAMILY;
    }
}

/**
 * pango2_font_description_get_family:
 * @desc: a `Pango2FontDescription`.
 *
 * Gets the family name field of a font description.
 *
 * See [method@Pango2.FontDescription.set_family].
 *
 * Return value: (nullable): the family name field for the font
 *   description, or %NULL if not previously set. This has the same
 *   life-time as the font description itself and should not be freed.
 */
const char *
pango2_font_description_get_family (const Pango2FontDescription *desc)
{
  g_return_val_if_fail (desc != NULL, NULL);

  return desc->family_name;
}

/**
 * pango2_font_description_set_style:
 * @desc: a `Pango2FontDescription`
 * @style: the style for the font description
 *
 * Sets the style field of a `Pango2FontDescription`.
 *
 * The [enum@Pango2.Style] enumeration describes whether the font is
 * slanted and the manner in which it is slanted; it can be either
 * %PANGO2_STYLE_NORMAL, %PANGO2_STYLE_ITALIC, or %PANGO2_STYLE_OBLIQUE.
 *
 * Most fonts will either have a italic style or an oblique style,
 * but not both, and font matching in Pango will match italic
 * specifications with oblique fonts and vice-versa if an exact
 * match is not found.
 */
void
pango2_font_description_set_style (Pango2FontDescription *desc,
                                   Pango2Style            style)
{
  g_return_if_fail (desc != NULL);

  desc->style = style;
  desc->mask |= PANGO2_FONT_MASK_STYLE;
}

/**
 * pango2_font_description_get_style:
 * @desc: a `Pango2FontDescription`
 *
 * Gets the style field of a `Pango2FontDescription`.
 *
 * See [method@Pango2.FontDescription.set_style].
 *
 * Return value: the style field for the font description.
 *   Use [method@Pango2.FontDescription.get_set_fields] to
 *   find out if the field was explicitly set or not.
 */
Pango2Style
pango2_font_description_get_style (const Pango2FontDescription *desc)
{
  g_return_val_if_fail (desc != NULL, pfd_defaults.style);

  return desc->style;
}

/**
 * pango2_font_description_set_variant:
 * @desc: a `Pango2FontDescription`
 * @variant: the variant type for the font description.
 *
 * Sets the variant field of a font description.
 *
 * The [enum@Pango2.Variant] can either be %PANGO2_VARIANT_NORMAL
 * or %PANGO2_VARIANT_SMALL_CAPS.
 */
void
pango2_font_description_set_variant (Pango2FontDescription *desc,
                                     Pango2Variant          variant)
{
  g_return_if_fail (desc != NULL);

  desc->variant = variant;
  desc->mask |= PANGO2_FONT_MASK_VARIANT;
}

/**
 * pango2_font_description_get_variant:
 * @desc: a `Pango2FontDescription`.
 *
 * Gets the variant field of a `Pango2FontDescription`.
 *
 * See [method@Pango2.FontDescription.set_variant].
 *
 * Return value: the variant field for the font description.
 *   Use [method@Pango2.FontDescription.get_set_fields] to find
 *   out if the field was explicitly set or not.
 */
Pango2Variant
pango2_font_description_get_variant (const Pango2FontDescription *desc)
{
  g_return_val_if_fail (desc != NULL, pfd_defaults.variant);

  return desc->variant;
}

/**
 * pango2_font_description_set_weight:
 * @desc: a `Pango2FontDescription`
 * @weight: the weight for the font description.
 *
 * Sets the weight field of a font description.
 *
 * The weight field
 * specifies how bold or light the font should be. In addition
 * to the values of the [enum@Pango2.Weight] enumeration, other
 * intermediate numeric values are possible.
 */
void
pango2_font_description_set_weight (Pango2FontDescription *desc,
                                    Pango2Weight           weight)
{
  g_return_if_fail (desc != NULL);

  desc->weight = weight;
  desc->mask |= PANGO2_FONT_MASK_WEIGHT;
}

/**
 * pango2_font_description_get_weight:
 * @desc: a `Pango2FontDescription`
 *
 * Gets the weight field of a font description.
 *
 * See [method@Pango2.FontDescription.set_weight].
 *
 * Return value: the weight field for the font description.
 *   Use [method@Pango2.FontDescription.get_set_fields] to find
 *   out if the field was explicitly set or not.
 */
Pango2Weight
pango2_font_description_get_weight (const Pango2FontDescription *desc)
{
  g_return_val_if_fail (desc != NULL, pfd_defaults.weight);

  return desc->weight;
}

/**
 * pango2_font_description_set_stretch:
 * @desc: a `Pango2FontDescription`
 * @stretch: the stretch for the font description
 *
 * Sets the stretch field of a font description.
 *
 * The [enum@Pango2.Stretch] field specifies how narrow or
 * wide the font should be.
 */
void
pango2_font_description_set_stretch (Pango2FontDescription *desc,
                                     Pango2Stretch          stretch)
{
  g_return_if_fail (desc != NULL);

  desc->stretch = stretch;
  desc->mask |= PANGO2_FONT_MASK_STRETCH;
}

/**
 * pango2_font_description_get_stretch:
 * @desc: a `Pango2FontDescription`.
 *
 * Gets the stretch field of a font description.
 *
 * See [method@Pango2.FontDescription.set_stretch].
 *
 * Return value: the stretch field for the font description.
 *   Use [method@Pango2.FontDescription.get_set_fields] to find
 *   out if the field was explicitly set or not.
 */
Pango2Stretch
pango2_font_description_get_stretch (const Pango2FontDescription *desc)
{
  g_return_val_if_fail (desc != NULL, pfd_defaults.stretch);

  return desc->stretch;
}

/**
 * pango2_font_description_set_size:
 * @desc: a `Pango2FontDescription`
 * @size: the size of the font in points, scaled by %PANGO2_SCALE.
 *   (That is, a @size value of 10 * PANGO2_SCALE is a 10 point font.
 *   The conversion factor between points and device units depends on
 *   system configuration and the output device. For screen display, a
 *   logical DPI of 96 is common, in which case a 10 point font corresponds
 *   to a 10 * (96 / 72) = 13.3 pixel font.
 *   Use [method@Pango2.FontDescription.set_absolute_size] if you need
 *   a particular size in device units.
 *
 * Sets the size field of a font description in fractional points.
 *
 * This is mutually exclusive with
 * [method@Pango2.FontDescription.set_absolute_size].
 */
void
pango2_font_description_set_size (Pango2FontDescription *desc,
                                  int                    size)
{
  g_return_if_fail (desc != NULL);
  g_return_if_fail (size >= 0);

  desc->size = size;
  desc->size_is_absolute = FALSE;
  desc->mask |= PANGO2_FONT_MASK_SIZE;
}

/**
 * pango2_font_description_get_size:
 * @desc: a `Pango2FontDescription`
 *
 * Gets the size field of a font description.
 *
 * See [method@Pango2.FontDescription.set_size].
 *
 * Return value: the size field for the font description in points
 *   or device units. You must call
 *   [method@Pango2.FontDescription.get_size_is_absolute] to find out
 *   which is the case. Returns 0 if the size field has not previously
 *   been set or it has been set to 0 explicitly.
 *   Use [method@Pango2.FontDescription.get_set_fields] to find out
 *   if the field was explicitly set or not.
 */
int
pango2_font_description_get_size (const Pango2FontDescription *desc)
{
  g_return_val_if_fail (desc != NULL, pfd_defaults.size);

  return desc->size;
}

/**
 * pango2_font_description_set_absolute_size:
 * @desc: a `Pango2FontDescription`
 * @size: the new size, in Pango units. There are %PANGO2_SCALE Pango units
 *   in one device unit. For an output backend where a device unit is a pixel,
 *   a @size value of 10 * PANGO2_SCALE gives a 10 pixel font.
 *
 * Sets the size field of a font description, in device units.
 *
 * This is mutually exclusive with [method@Pango2.FontDescription.set_size]
 * which sets the font size in points.
 */
void
pango2_font_description_set_absolute_size (Pango2FontDescription *desc,
                                           double                 size)
{
  g_return_if_fail (desc != NULL);
  g_return_if_fail (size >= 0);

  desc->size = round (size);
  desc->size_is_absolute = TRUE;
  desc->mask |= PANGO2_FONT_MASK_SIZE;
}

/**
 * pango2_font_description_get_size_is_absolute:
 * @desc: a `Pango2FontDescription`
 *
 * Determines whether the size of the font is in points (not absolute)
 * or device units (absolute).
 *
 * See [method@Pango2.FontDescription.set_size]
 * and [method@Pango2.FontDescription.set_absolute_size].
 *
 * Return value: whether the size for the font description is in
 *   points or device units. Use [method@Pango2.FontDescription.get_set_fields]
 *   to find out if the size field of the font description was explicitly
 *   set or not.
 */
gboolean
pango2_font_description_get_size_is_absolute (const Pango2FontDescription *desc)
{
  g_return_val_if_fail (desc != NULL, pfd_defaults.size_is_absolute);

  return desc->size_is_absolute;
}

/**
 * pango2_font_description_set_gravity:
 * @desc: a `Pango2FontDescription`
 * @gravity: the gravity for the font description.
 *
 * Sets the gravity field of a font description.
 *
 * The gravity field
 * specifies how the glyphs should be rotated. If @gravity is
 * %PANGO2_GRAVITY_AUTO, this actually unsets the gravity mask on
 * the font description.
 *
 * This function is seldom useful to the user. Gravity should normally
 * be set on a `Pango2Context`.
 */
void
pango2_font_description_set_gravity (Pango2FontDescription *desc,
                                     Pango2Gravity          gravity)
{
  g_return_if_fail (desc != NULL);

  if (gravity == PANGO2_GRAVITY_AUTO)
    {
      pango2_font_description_unset_fields (desc, PANGO2_FONT_MASK_GRAVITY);
      return;
    }

  desc->gravity = gravity;
  desc->mask |= PANGO2_FONT_MASK_GRAVITY;
}

/**
 * pango2_font_description_get_gravity:
 * @desc: a `Pango2FontDescription`
 *
 * Gets the gravity field of a font description.
 *
 * See [method@Pango2.FontDescription.set_gravity].
 *
 * Return value: the gravity field for the font description.
 *   Use [method@Pango2.FontDescription.get_set_fields] to find out
 *   if the field was explicitly set or not.
 */
Pango2Gravity
pango2_font_description_get_gravity (const Pango2FontDescription *desc)
{
  g_return_val_if_fail (desc != NULL, pfd_defaults.gravity);

  return desc->gravity;
}

/**
 * pango2_font_description_set_variations_static:
 * @desc: a `Pango2FontDescription`
 * @variations: a string representing the variations
 *
 * Sets the variations field of a font description.
 *
 * This is like [method@Pango2.FontDescription.set_variations], except
 * that no copy of @variations is made. The caller must make sure that
 * the string passed in stays around until @desc has been freed
 * or the name is set again. This function can be used if
 * @variations is a static string such as a C string literal,
 * or if @desc is only needed temporarily.
 */
void
pango2_font_description_set_variations_static (Pango2FontDescription *desc,
                                               const char            *variations)
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
      desc->mask |= PANGO2_FONT_MASK_VARIATIONS;
    }
  else
    {
      desc->variations = pfd_defaults.variations;
      desc->static_variations = pfd_defaults.static_variations;
      desc->mask &= ~PANGO2_FONT_MASK_VARIATIONS;
    }
}

/**
 * pango2_font_description_set_variations:
 * @desc: a `Pango2FontDescription`.
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
 */
void
pango2_font_description_set_variations (Pango2FontDescription *desc,
                                        const char            *variations)
{
  g_return_if_fail (desc != NULL);

  pango2_font_description_set_variations_static (desc, g_strdup (variations));
  if (variations)
    desc->static_variations = FALSE;
}

/**
 * pango2_font_description_get_variations:
 * @desc: a `Pango2FontDescription`
 *
 * Gets the variations field of a font description.
 *
 * See [method@Pango2.FontDescription.set_variations].
 *
 * Return value: (nullable): the variations field for the font
 *   description, or %NULL if not previously set. This has the same
 *   life-time as the font description itself and should not be freed.
 */
const char *
pango2_font_description_get_variations (const Pango2FontDescription *desc)
{
  g_return_val_if_fail (desc != NULL, NULL);

  return desc->variations;
}

/**
 * pango2_font_description_get_set_fields:
 * @desc: a `Pango2FontDescription`
 *
 * Determines which fields in a font description have been set.
 *
 * Return value: a bitmask with bits set corresponding to the
 *   fields in @desc that have been set.
 */
Pango2FontMask
pango2_font_description_get_set_fields (const Pango2FontDescription *desc)
{
  g_return_val_if_fail (desc != NULL, pfd_defaults.mask);

  return desc->mask;
}

/**
 * pango2_font_description_unset_fields:
 * @desc: a `Pango2FontDescription`
 * @to_unset: bitmask of fields in the @desc to unset.
 *
 * Unsets some of the fields in a `Pango2FontDescription`.
 *
 * The unset fields will get back to their default values.
 */
void
pango2_font_description_unset_fields (Pango2FontDescription *desc,
                                      Pango2FontMask         to_unset)
{
  Pango2FontDescription unset_desc;

  g_return_if_fail (desc != NULL);

  unset_desc = pfd_defaults;
  unset_desc.mask = to_unset;

  pango2_font_description_merge_static (desc, &unset_desc, TRUE);

  desc->mask &= ~to_unset;
}

/**
 * pango2_font_description_merge:
 * @desc: a `Pango2FontDescription`
 * @desc_to_merge: (nullable): the `Pango2FontDescription` to merge from,
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
pango2_font_description_merge (Pango2FontDescription       *desc,
                               const Pango2FontDescription *desc_to_merge,
                               gboolean                     replace_existing)
{
  gboolean family_merged;
  gboolean variations_merged;
  gboolean faceid_merged;

  g_return_if_fail (desc != NULL);

  if (desc_to_merge == NULL)
    return;

  family_merged = desc_to_merge->family_name && (replace_existing || !desc->family_name);
  variations_merged = desc_to_merge->variations && (replace_existing || !desc->variations);
  faceid_merged = desc_to_merge->faceid && (replace_existing || !desc->faceid);

  pango2_font_description_merge_static (desc, desc_to_merge, replace_existing);

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

  if (faceid_merged)
    {
      desc->faceid = g_strdup (desc->faceid);
      desc->static_faceid = FALSE;
    }
}

/**
 * pango2_font_description_merge_static:
 * @desc: a `Pango2FontDescription`
 * @desc_to_merge: the `Pango2FontDescription` to merge from
 * @replace_existing: if %TRUE, replace fields in @desc with the
 *   corresponding values from @desc_to_merge, even if they
 *   are already exist.
 *
 * Merges the fields that are set in @desc_to_merge into the fields in
 * @desc, without copying allocated fields.
 *
 * This is like [method@Pango2.FontDescription.merge], but only a shallow copy
 * is made of the family name and other allocated fields. @desc can only
 * be used until @desc_to_merge is modified or freed. This is meant to
 * be used when the merged font description is only needed temporarily.
 */
void
pango2_font_description_merge_static (Pango2FontDescription       *desc,
                                      const Pango2FontDescription *desc_to_merge,
                                      gboolean                     replace_existing)
{
  Pango2FontMask new_mask;

  g_return_if_fail (desc != NULL);
  g_return_if_fail (desc_to_merge != NULL);

  if (replace_existing)
    new_mask = desc_to_merge->mask;
  else
    new_mask = desc_to_merge->mask & ~desc->mask;

  if (new_mask & PANGO2_FONT_MASK_FAMILY)
    pango2_font_description_set_family_static (desc, desc_to_merge->family_name);
  if (new_mask & PANGO2_FONT_MASK_STYLE)
    desc->style = desc_to_merge->style;
  if (new_mask & PANGO2_FONT_MASK_VARIANT)
    desc->variant = desc_to_merge->variant;
  if (new_mask & PANGO2_FONT_MASK_WEIGHT)
    desc->weight = desc_to_merge->weight;
  if (new_mask & PANGO2_FONT_MASK_STRETCH)
    desc->stretch = desc_to_merge->stretch;
  if (new_mask & PANGO2_FONT_MASK_SIZE)
    {
      desc->size = desc_to_merge->size;
      desc->size_is_absolute = desc_to_merge->size_is_absolute;
    }
  if (new_mask & PANGO2_FONT_MASK_GRAVITY)
    desc->gravity = desc_to_merge->gravity;
  if (new_mask & PANGO2_FONT_MASK_VARIATIONS)
    pango2_font_description_set_variations_static (desc, desc_to_merge->variations);
  if (new_mask & PANGO2_FONT_MASK_FACEID)
    pango2_font_description_set_faceid_static (desc, desc_to_merge->faceid);

  desc->mask |= new_mask;
}

gboolean
pango2_font_description_is_similar (const Pango2FontDescription *a,
                                    const Pango2FontDescription *b)
{
  return a->variant == b->variant &&
         a->gravity == b->gravity;
}

int
pango2_font_description_compute_distance (const Pango2FontDescription *a,
                                          const Pango2FontDescription *b)
{
  if (a->style == b->style)
    {
      return abs ((int)(a->weight) - (int)(b->weight)) +
             abs ((int)(a->stretch) - (int)(b->stretch));
    }
  else if (a->style != PANGO2_STYLE_NORMAL &&
           b->style != PANGO2_STYLE_NORMAL)
    {
      /* Equate oblique and italic, but with a big penalty
       */
      return 1000000 + abs ((int)(a->weight) - (int)(b->weight))
                     + abs ((int)(a->stretch) - (int)(b->stretch));
    }
  else
    return G_MAXINT;
}

/**
 * pango2_font_description_copy:
 * @desc: (nullable): a `Pango2FontDescription`, may be %NULL
 *
 * Make a copy of a `Pango2FontDescription`.
 *
 * Return value: (nullable): the newly allocated `Pango2FontDescription`,
 *   which should be freed with [method@Pango2.FontDescription.free]
 */
Pango2FontDescription *
pango2_font_description_copy (const Pango2FontDescription *desc)
{
  Pango2FontDescription *result;

  if (desc == NULL)
    return NULL;

  result = g_slice_new (Pango2FontDescription);

  *result = *desc;

  if (result->family_name)
    {
      result->family_name = g_strdup (result->family_name);
      result->static_family = FALSE;
    }

  result->variations = g_strdup (result->variations);
  result->static_variations = FALSE;

  result->faceid = g_strdup (result->faceid);
  result->static_faceid = FALSE;

  return result;
}

/**
 * pango2_font_description_copy_static:
 * @desc: (nullable): a `Pango2FontDescription`, may be %NULL
 *
 * Make a copy of a `Pango2FontDescription`, but don't duplicate
 * allocated fields.
 *
 * This is like [method@Pango2.FontDescription.copy], but only a shallow
 * copy is made of the family name and other allocated fields. The result
 * can only be used until @desc is modified or freed. This is meant
 * to be used when the copy is only needed temporarily.
 *
 * Return value: (nullable): the newly allocated `Pango2FontDescription`,
 *   which should be freed with [method@Pango2.FontDescription.free]
 */
Pango2FontDescription *
pango2_font_description_copy_static (const Pango2FontDescription *desc)
{
  Pango2FontDescription *result;

  if (desc == NULL)
    return NULL;

  result = g_slice_new (Pango2FontDescription);

  *result = *desc;
  if (result->family_name)
    result->static_family = TRUE;

  if (result->variations)
    result->static_variations = TRUE;

  if (result->faceid)
    result->static_faceid = TRUE;

  return result;
}

/**
 * pango2_font_description_equal:
 * @desc1: a `Pango2FontDescription`
 * @desc2: another `Pango2FontDescription`
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
pango2_font_description_equal (const Pango2FontDescription *desc1,
                               const Pango2FontDescription *desc2)
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
         (g_strcmp0 (desc1->variations, desc2->variations) == 0) &&
         (g_strcmp0 (desc1->faceid, desc2->faceid) == 0) &&
         desc1->palette == desc2->palette;
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
 * pango2_font_description_hash:
 * @desc: a `Pango2FontDescription`
 *
 * Computes a hash of a `Pango2FontDescription` structure.
 *
 * This is suitable to be used, for example, as an argument
 * to [GLib.HashTable.new]. The hash value is independent of @desc->mask.
 *
 * Return value: the hash value.
 */
guint
pango2_font_description_hash (const Pango2FontDescription *desc)
{
  guint hash = 0;

  g_return_val_if_fail (desc != NULL, 0);

  if (desc->family_name)
    hash = case_insensitive_hash (desc->family_name);
  if (desc->variations)
    hash ^= g_str_hash (desc->variations);
  if (desc->faceid)
    hash ^= g_str_hash (desc->faceid);
  hash ^= desc->size;
  hash ^= desc->size_is_absolute ? 0xc33ca55a : 0;
  hash ^= desc->style << 16;
  hash ^= desc->variant << 18;
  hash ^= desc->weight << 16;
  hash ^= desc->stretch << 26;
  hash ^= desc->gravity << 28;
  hash ^= desc->palette;

  return hash;
}

/**
 * pango2_font_description_free:
 * @desc: (nullable): a `Pango2FontDescription`, may be %NULL
 *
 * Frees a font description.
 */
void
pango2_font_description_free (Pango2FontDescription *desc)
{
  if (desc == NULL)
    return;

  if (desc->family_name && !desc->static_family)
    g_free (desc->family_name);

  if (desc->variations && !desc->static_variations)
    g_free (desc->variations);

  if (desc->faceid && !desc->static_faceid)
    g_free (desc->faceid);

  g_slice_free (Pango2FontDescription, desc);
}

typedef struct
{
  int value;
  const char str[16];
} FieldMap;

static const FieldMap style_map[] = {
  { PANGO2_STYLE_NORMAL, "" },
  { PANGO2_STYLE_NORMAL, "Roman" },
  { PANGO2_STYLE_OBLIQUE, "Oblique" },
  { PANGO2_STYLE_ITALIC, "Italic" }
};

static const FieldMap variant_map[] = {
  { PANGO2_VARIANT_NORMAL, "" },
  { PANGO2_VARIANT_SMALL_CAPS, "Small-Caps" },
  { PANGO2_VARIANT_ALL_SMALL_CAPS, "All-Small-Caps" },
  { PANGO2_VARIANT_PETITE_CAPS, "Petite-Caps" },
  { PANGO2_VARIANT_ALL_PETITE_CAPS, "All-Petite-Caps" },
  { PANGO2_VARIANT_UNICASE, "Unicase" },
  { PANGO2_VARIANT_TITLE_CAPS, "Title-Caps" }
};

static const FieldMap weight_map[] = {
  { PANGO2_WEIGHT_THIN, "Thin" },
  { PANGO2_WEIGHT_ULTRALIGHT, "Ultra-Light" },
  { PANGO2_WEIGHT_ULTRALIGHT, "Extra-Light" },
  { PANGO2_WEIGHT_LIGHT, "Light" },
  { PANGO2_WEIGHT_SEMILIGHT, "Semi-Light" },
  { PANGO2_WEIGHT_SEMILIGHT, "Demi-Light" },
  { PANGO2_WEIGHT_BOOK, "Book" },
  { PANGO2_WEIGHT_NORMAL, "" },
  { PANGO2_WEIGHT_NORMAL, "Regular" },
  { PANGO2_WEIGHT_MEDIUM, "Medium" },
  { PANGO2_WEIGHT_SEMIBOLD, "Semi-Bold" },
  { PANGO2_WEIGHT_SEMIBOLD, "Demi-Bold" },
  { PANGO2_WEIGHT_BOLD, "Bold" },
  { PANGO2_WEIGHT_ULTRABOLD, "Ultra-Bold" },
  { PANGO2_WEIGHT_ULTRABOLD, "Extra-Bold" },
  { PANGO2_WEIGHT_HEAVY, "Heavy" },
  { PANGO2_WEIGHT_HEAVY, "Black" },
  { PANGO2_WEIGHT_ULTRAHEAVY, "Ultra-Heavy" },
  { PANGO2_WEIGHT_ULTRAHEAVY, "Extra-Heavy" },
  { PANGO2_WEIGHT_ULTRAHEAVY, "Ultra-Black" },
  { PANGO2_WEIGHT_ULTRAHEAVY, "Extra-Black" }
};

static const FieldMap stretch_map[] = {
  { PANGO2_STRETCH_ULTRA_CONDENSED, "Ultra-Condensed" },
  { PANGO2_STRETCH_EXTRA_CONDENSED, "Extra-Condensed" },
  { PANGO2_STRETCH_CONDENSED,       "Condensed" },
  { PANGO2_STRETCH_SEMI_CONDENSED,  "Semi-Condensed" },
  { PANGO2_STRETCH_NORMAL,          "" },
  { PANGO2_STRETCH_SEMI_EXPANDED,   "Semi-Expanded" },
  { PANGO2_STRETCH_EXPANDED,        "Expanded" },
  { PANGO2_STRETCH_EXTRA_EXPANDED,  "Extra-Expanded" },
  { PANGO2_STRETCH_ULTRA_EXPANDED,  "Ultra-Expanded" }
};

static const FieldMap gravity_map[] = {
  { PANGO2_GRAVITY_SOUTH, "Not-Rotated" },
  { PANGO2_GRAVITY_SOUTH, "South" },
  { PANGO2_GRAVITY_NORTH, "Upside-Down" },
  { PANGO2_GRAVITY_NORTH, "North" },
  { PANGO2_GRAVITY_EAST,  "Rotated-Left" },
  { PANGO2_GRAVITY_EAST,  "East" },
  { PANGO2_GRAVITY_WEST,  "Rotated-Right" },
  { PANGO2_GRAVITY_WEST,  "West" }
};

static gboolean
field_matches (const char *s1,
               const char *s2,
               gsize       n)
{
  int c1, c2;

  g_return_val_if_fail (s1 != NULL, 0);
  g_return_val_if_fail (s2 != NULL, 0);

  while (n && *s1 && *s2)
    {
      c1 = (int)(guchar) TOLOWER (*s1);
      c2 = (int)(guchar) TOLOWER (*s2);
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
find_field (const char     *what,
            const FieldMap *map,
            int             n_elements,
            const char     *str,
            int             len,
            int            *val)
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
find_field_any (const char            *str,
                int                    len,
                Pango2FontDescription *desc)
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

  FIELD (weight,  PANGO2_FONT_MASK_WEIGHT);
  FIELD (style,   PANGO2_FONT_MASK_STYLE);
  FIELD (stretch, PANGO2_FONT_MASK_STRETCH);
  FIELD (variant, PANGO2_FONT_MASK_VARIANT);
  FIELD (gravity, PANGO2_FONT_MASK_GRAVITY);

#undef FIELD

  return FALSE;
}

static const char *
getword (const char *str,
         const char *last,
         size_t     *wordlen,
         const char *stop)
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
            int        *pango2_size,
            gboolean   *size_is_absolute)
{
  char *end;
  double size = g_ascii_strtod (word, &end);

  if (end != word &&
      (end == word + wordlen ||
       (end + 2 == word + wordlen && !strncmp (end, "px", 2))
      ) && size >= 0 && size <= 1000000) /* word is a valid float */
    {
      if (pango2_size)
        *pango2_size = (int)(size * PANGO2_SCALE + 0.5);

      if (size_is_absolute)
        *size_is_absolute = end < word + wordlen;

      return TRUE;
    }

  return FALSE;
}

static gboolean
parse_faceid (const char  *word,
              size_t       wordlen,
              char       **faceid)
{
  const char *p, *q;

  if (!g_str_has_prefix (word, "@faceid="))
    return FALSE;

  p = word + strlen ("@faceid=");
  q = word + wordlen;

  *faceid = g_strndup (p, q - p);

  return TRUE;
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
 * pango2_font_description_from_string:
 * @str: string representation of a font description.
 *
 * Creates a new font description from a string representation.
 *
 * The string must have the form
 *
 *     "\[FAMILY-LIST] \[STYLE-OPTIONS] \[SIZE] \[VARIATIONS] \[FACEID]",
 *
 * where FAMILY-LIST is a comma-separated list of families optionally
 * terminated by a comma, STYLE_OPTIONS is a whitespace-separated list
 * of words where each word describes one of style, variant, weight,
 * stretch, or gravity, and SIZE is a decimal number (size in points)
 * or optionally followed by the unit modifier "px" for absolute size.
 *
 * VARIATIONS is a comma-separated list of font variation
 * specifications of the form "\@axis=value" (the = sign is optional),
 * where "axis" is a 3-character name of an OpenType variation axis
 * like "wght", "wdth" or "opsz".
 *
 * FACEID must have the form "\@faceid=string" with the literal string
 * "faceid".
 *
 * The VARIATION, FACEID parts can appear in any order,
 * as long as they are at the end.
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
 * Return value: a new `Pango2FontDescription`.
 */
Pango2FontDescription *
pango2_font_description_from_string (const char *str)
{
  Pango2FontDescription *desc;
  const char *p, *last;
  size_t len, wordlen;

  g_return_val_if_fail (str != NULL, NULL);

  desc = pango2_font_description_new ();

  desc->mask = PANGO2_FONT_MASK_STYLE |
               PANGO2_FONT_MASK_WEIGHT |
               PANGO2_FONT_MASK_VARIANT |
               PANGO2_FONT_MASK_STRETCH;

  len = strlen (str);
  last = str + len;

  do
    {
      p = getword (str, last, &wordlen, "");

      if (wordlen == 0 || p[0] != '@')
        break;

      /* Look for faceid and variations at the end of the string */
      if (parse_faceid (p, wordlen, &desc->faceid))
        {
          desc->mask |= PANGO2_FONT_MASK_FACEID;
          last = p;
        }
      else if (parse_variations (p, wordlen, &desc->variations))
        {
          desc->mask |= PANGO2_FONT_MASK_VARIATIONS;
          last = p;
        }
      else
        break;
    }
  while ((desc->mask & (PANGO2_FONT_MASK_FACEID | PANGO2_FONT_MASK_VARIATIONS)) !=
                       (PANGO2_FONT_MASK_FACEID | PANGO2_FONT_MASK_VARIATIONS));

  p = getword (str, last, &wordlen, ",");
  /* Look for a size */
  if (wordlen != 0)
    {
      gboolean size_is_absolute;
      if (parse_size (p, wordlen, &desc->size, &size_is_absolute))
        {
          desc->size_is_absolute = size_is_absolute;
          desc->mask |= PANGO2_FONT_MASK_SIZE;
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

      desc->mask |= PANGO2_FONT_MASK_FAMILY;
    }

  return desc;
}

static void
append_field (GString        *str,
              const char     *what,
              const FieldMap *map,
              int             n_elements,
              int             val)
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


static void
g_ascii_format_nearest_multiple (char   *buf,
                                 gsize   len,
                                 double  value,
                                 double  factor)
{
  double eps, lo, hi;
  char buf1[24];
  char buf2[24];
  int i, j;
  const char *dot;

  value = round (value / factor) * factor;
  eps = 0.5 * factor;
  lo = value - eps;
  hi = value + eps;

  if (floor (lo) != floor (hi))
    {
      g_snprintf (buf, len, "%d", (int) round (value));
      return;
    }

  g_ascii_formatd (buf1, sizeof (buf1), "%.8f", lo);
  g_ascii_formatd (buf2, sizeof (buf2), "%.8f", hi);

  g_assert (strlen (buf1) == strlen (buf2));

  for (i = 0; buf1[i]; i++)
    {
      if (buf1[i] != buf2[i])
        break;
    }

  dot = strchr (buf1, '.');

  j = dot - buf1;

  g_assert (j < i);

  g_snprintf (buf1, sizeof (buf1), "%%.%df", i - j);
  g_ascii_formatd (buf, len, buf1, value);
}

/**
 * pango2_font_description_to_string:
 * @desc: a `Pango2FontDescription`
 *
 * Creates a string representation of a font description.
 *
 * See [func@Pango2.FontDescription.from_string] for a description
 * of the format of the string representation. The family list in
 * the string description will only have a terminating comma if
 * the last word of the list is a valid style option.
 *
 * Return value: a newly allocated string
 */
char *
pango2_font_description_to_string (const Pango2FontDescription *desc)
{
  GString *result;

  g_return_val_if_fail (desc != NULL, NULL);

  result = g_string_new (NULL);

  if (G_LIKELY (desc->family_name && desc->mask & PANGO2_FONT_MASK_FAMILY))
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
            desc->weight == PANGO2_WEIGHT_NORMAL &&
            desc->style == PANGO2_STYLE_NORMAL &&
            desc->stretch == PANGO2_STRETCH_NORMAL &&
            desc->variant == PANGO2_VARIANT_NORMAL &&
            (desc->mask & (PANGO2_FONT_MASK_GRAVITY | PANGO2_FONT_MASK_SIZE)) == 0)))
        g_string_append_c (result, ',');
    }

#define FIELD(NAME, MASK) \
  append_field (result, G_STRINGIFY (NAME), NAME##_map, G_N_ELEMENTS (NAME##_map), desc->NAME)

  FIELD (weight,  PANGO2_FONT_MASK_WEIGHT);
  FIELD (style,   PANGO2_FONT_MASK_STYLE);
  FIELD (stretch, PANGO2_FONT_MASK_STRETCH);
  FIELD (variant, PANGO2_FONT_MASK_VARIANT);
  if (desc->mask & PANGO2_FONT_MASK_GRAVITY)
    FIELD (gravity, PANGO2_FONT_MASK_GRAVITY);

#undef FIELD

  if (result->len == 0)
    g_string_append (result, "Normal");

  if (desc->mask & PANGO2_FONT_MASK_SIZE)
    {
      char buf[G_ASCII_DTOSTR_BUF_SIZE];

      if (result->len > 0 || result->str[result->len -1] != ' ')
        g_string_append_c (result, ' ');

      g_ascii_format_nearest_multiple (buf, sizeof (buf),
                                       desc->size / (double) PANGO2_SCALE,
                                       1 / (double) PANGO2_SCALE);
      g_string_append (result, buf);

      if (desc->size_is_absolute)
        g_string_append (result, "px");
    }

  if (desc->mask & PANGO2_FONT_MASK_FACEID)
    {
      g_string_append (result, " @");
      g_string_append_printf (result, "faceid=%s", desc->faceid);
    }

  if ((desc->variations && desc->mask & PANGO2_FONT_MASK_VARIATIONS) &&
      desc->variations[0] != '\0')
    {
      g_string_append (result, " @");
      g_string_append (result, desc->variations);
    }

  return g_string_free (result, FALSE);
}

static gboolean
parse_field (const char     *what,
             const FieldMap *map,
             int             n_elements,
             const char     *str,
             int            *val,
             gboolean        warn)
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

/*< private >
 * pango2_parse_style:
 * @str: a string to parse.
 * @style: (out): a `Pango2Style` to store the result in.
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
pango2_parse_style (const char  *str,
                    Pango2Style *style,
                    gboolean     warn)
{
  return FIELD (style,   PANGO2_FONT_MASK_STYLE);
}

/*< private >
 * pango2_parse_variant:
 * @str: a string to parse.
 * @variant: (out): a `Pango2Variant` to store the result in.
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
pango2_parse_variant (const char    *str,
                      Pango2Variant *variant,
                      gboolean       warn)
{
  return FIELD (variant, PANGO2_FONT_MASK_VARIANT);
}

/*< private >
 * pango2_parse_weight:
 * @str: a string to parse.
 * @weight: (out): a `Pango2Weight` to store the result in.
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
pango2_parse_weight (const char   *str,
                     Pango2Weight *weight,
                     gboolean      warn)
{
  return FIELD (weight,  PANGO2_FONT_MASK_WEIGHT);
}

/*< private >
 * pango2_parse_stretch:
 * @str: a string to parse.
 * @stretch: (out): a `Pango2Stretch` to store the result in.
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
pango2_parse_stretch (const char    *str,
                      Pango2Stretch *stretch,
                      gboolean       warn)
{
  return FIELD (stretch, PANGO2_FONT_MASK_STRETCH);
}

/**
 * pango2_font_description_set_faceid_static:
 * @desc: a `Pango2FontDescription`
 * @faceid: the faceid string
 *
 * Sets the faceid field of a font description.
 *
 * This is like [method@Pango2.FontDescription.set_faceid], except
 * that no copy of @faceid is made. The caller must make sure that
 * the string passed in stays around until @desc has been freed
 * or the name is set again. This function can be used if
 * @faceid is a static string such as a C string literal,
 * or if @desc is only needed temporarily.
 */
void
pango2_font_description_set_faceid_static (Pango2FontDescription *desc,
                                           const char            *faceid)
{
  g_return_if_fail (desc != NULL);

  if (desc->faceid == faceid)
    return;

  if (desc->faceid && !desc->static_faceid)
    g_free (desc->faceid);

  if (faceid)
    {
      desc->faceid = (char *)faceid;
      desc->static_faceid = TRUE;
      desc->mask |= PANGO2_FONT_MASK_FACEID;
    }
  else
    {
      desc->faceid = pfd_defaults.faceid;
      desc->static_faceid = pfd_defaults.static_faceid;
      desc->mask &= ~PANGO2_FONT_MASK_FACEID;
    }
}

/**
 * pango2_font_description_set_faceid:
 * @desc: a `Pango2FontDescription`.
 * @faceid: (nullable): the faceid string
 *
 * Sets the faceid field of a font description.
 *
 * The faceid is mainly for internal use by Pango, to ensure
 * that font -> description -> font roundtrips end up with
 * the same font they started with, if possible.
 *
 * Font descriptions originating from [method@Pango2.FontFace.describe]
 * should ideally include a faceid. Pango takes the faceid
 * into account when looking for the best matching face while
 * loading a fontset or font.
 *
 * The format of this string is not guaranteed.
 */
void
pango2_font_description_set_faceid (Pango2FontDescription *desc,
                                    const char            *faceid)
{
  g_return_if_fail (desc != NULL);

  pango2_font_description_set_faceid_static (desc, g_strdup (faceid));
  if (faceid)
    desc->static_faceid = FALSE;
}

/**
 * pango2_font_description_get_faceid:
 * @desc: a `Pango2FontDescription`
 *
 * Gets the faceid field of a font description.
 *
 * See [method@Pango2.FontDescription.set_faceid].
 *
 * Return value: (nullable): the faceid field for the font
 *   description, or %NULL if not previously set. This has the same
 *   life-time as the font description itself and should not be freed.
 */
const char *
pango2_font_description_get_faceid (const Pango2FontDescription *desc)
{
  g_return_val_if_fail (desc != NULL, NULL);

  return desc->faceid;
}

/*< private >
 * pango2_font_description_get_palette_quark:
 * @desc: a `Pango2FontDescription
 *
 * Gets the palette field as a `GQuark`.
 *
 * Return value: the palette field as a quark
 */
GQuark
pango2_font_description_get_palette_quark (const Pango2FontDescription *desc)
{
  return desc->palette;
}
