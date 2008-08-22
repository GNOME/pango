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
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.	 See the GNU
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

#include "pango-types.h"
#include "pango-font.h"
#include "pango-fontmap.h"
#include "pango-impl-utils.h"

struct _PangoFontDescription
{
  char *family_name;

  PangoStyle style;
  PangoVariant variant;
  PangoWeight weight;
  PangoStretch stretch;
  PangoGravity gravity;

  guint16 mask;
  guint static_family : 1;
  guint size_is_absolute : 1;

  int size;
};

GType
pango_font_description_get_type (void)
{
  static GType our_type = 0;

  if (G_UNLIKELY (our_type == 0))
    our_type = g_boxed_type_register_static (I_("PangoFontDescription"),
					     (GBoxedCopyFunc)pango_font_description_copy,
					     (GBoxedFreeFunc)pango_font_description_free);

  return our_type;
}


static const PangoFontDescription pfd_defaults = {
  NULL,			/* family_name */

  PANGO_STYLE_NORMAL,	/* style */
  PANGO_VARIANT_NORMAL,	/* variant */
  PANGO_WEIGHT_NORMAL,	/* weight */
  PANGO_STRETCH_NORMAL,	/* stretch */
  PANGO_GRAVITY_SOUTH,  /* gravity */

  0,			/* mask */
  0,			/* static_family */
  FALSE,		/* size_is_absolute */

  0,			/* size */
};

/**
 * pango_font_description_new:
 *
 * Creates a new font description structure with all fields unset.
 *
 * Return value: the newly allocated #PangoFontDescription, which
 *               should be freed using pango_font_description_free().
 **/
PangoFontDescription *
pango_font_description_new (void)
{
  PangoFontDescription *desc = g_slice_new (PangoFontDescription);

  *desc = pfd_defaults;

  return desc;
}

/**
 * pango_font_description_set_family:
 * @desc: a #PangoFontDescription.
 * @family: a string representing the family name.
 *
 * Sets the family name field of a font description. The family
 * name represents a family of related font styles, and will
 * resolve to a particular #PangoFontFamily. In some uses of
 * #PangoFontDescription, it is also possible to use a comma
 * separated list of family names for this field.
 **/
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
 * @desc: a #PangoFontDescription
 * @family: a string representing the family name.
 *
 * Like pango_font_description_set_family(), except that no
 * copy of @family is made. The caller must make sure that the
 * string passed in stays around until @desc has been freed
 * or the name is set again. This function can be used if
 * @family is a static string such as a C string literal, or
 * if @desc is only needed temporarily.
 **/
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
 * @desc: a #PangoFontDescription.
 *
 * Gets the family name field of a font description. See
 * pango_font_description_set_family().
 *
 * Return value: the family name field for the font description, or
 *               %NULL if not previously set.  This has the same life-time
 *               as the font description itself and should not be freed.
 **/
G_CONST_RETURN char *
pango_font_description_get_family (const PangoFontDescription *desc)
{
  g_return_val_if_fail (desc != NULL, NULL);

  return desc->family_name;
}

/**
 * pango_font_description_set_style:
 * @desc: a #PangoFontDescription
 * @style: the style for the font description
 *
 * Sets the style field of a #PangoFontDescription. The
 * #PangoStyle enumeration describes whether the font is slanted and
 * the manner in which it is slanted; it can be either
 * #PANGO_STYLE_NORMAL, #PANGO_STYLE_ITALIC, or #PANGO_STYLE_OBLIQUE.
 * Most fonts will either have a italic style or an oblique
 * style, but not both, and font matching in Pango will
 * match italic specifications with oblique fonts and vice-versa
 * if an exact match is not found.
 **/
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
 * @desc: a #PangoFontDescription
 *
 * Gets the style field of a #PangoFontDescription. See
 * pango_font_description_set_style().
 *
 * Return value: the style field for the font description.
 *   Use pango_font_description_get_set_fields() to find out if
 *   the field was explicitly set or not.
 **/
PangoStyle
pango_font_description_get_style (const PangoFontDescription *desc)
{
  g_return_val_if_fail (desc != NULL, pfd_defaults.style);

  return desc->style;
}

/**
 * pango_font_description_set_variant:
 * @desc: a #PangoFontDescription
 * @variant: the variant type for the font description.
 *
 * Sets the variant field of a font description. The #PangoVariant
 * can either be %PANGO_VARIANT_NORMAL or %PANGO_VARIANT_SMALL_CAPS.
 **/
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
 * @desc: a #PangoFontDescription.
 *
 * Gets the variant field of a #PangoFontDescription. See
 * pango_font_description_set_variant().
 *
 * Return value: the variant field for the font description. Use
 *   pango_font_description_get_set_fields() to find out if
 *   the field was explicitly set or not.
 **/
PangoVariant
pango_font_description_get_variant (const PangoFontDescription *desc)
{
  g_return_val_if_fail (desc != NULL, pfd_defaults.variant);

  return desc->variant;
}

/**
 * pango_font_description_set_weight:
 * @desc: a #PangoFontDescription
 * @weight: the weight for the font description.
 *
 * Sets the weight field of a font description. The weight field
 * specifies how bold or light the font should be. In addition
 * to the values of the #PangoWeight enumeration, other intermediate
 * numeric values are possible.
 **/
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
 * @desc: a #PangoFontDescription
 *
 * Gets the weight field of a font description. See
 * pango_font_description_set_weight().
 *
 * Return value: the weight field for the font description. Use
 *   pango_font_description_get_set_fields() to find out if
 *   the field was explicitly set or not.
 **/
PangoWeight
pango_font_description_get_weight (const PangoFontDescription *desc)
{
  g_return_val_if_fail (desc != NULL, pfd_defaults.weight);

  return desc->weight;
}

/**
 * pango_font_description_set_stretch:
 * @desc: a #PangoFontDescription
 * @stretch: the stretch for the font description
 *
 * Sets the stretch field of a font description. The stretch field
 * specifies how narrow or wide the font should be.
 **/
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
 * @desc: a #PangoFontDescription.
 *
 * Gets the stretch field of a font description.
 * See pango_font_description_set_stretch().
 *
 * Return value: the stretch field for the font description. Use
 *   pango_font_description_get_set_fields() to find out if
 *   the field was explicitly set or not.
 **/
PangoStretch
pango_font_description_get_stretch (const PangoFontDescription *desc)
{
  g_return_val_if_fail (desc != NULL, pfd_defaults.stretch);

  return desc->stretch;
}

/**
 * pango_font_description_set_size:
 * @desc: a #PangoFontDescription
 * @size: the size of the font in points, scaled by PANGO_SCALE. (That is,
 *        a @size value of 10 * PANGO_SCALE is a 10 point font. The conversion
 *        factor between points and device units depends on system configuration
 *        and the output device. For screen display, a logical DPI of 96 is
 *        common, in which case a 10 point font corresponds to a 10 * (96 / 72) = 13.3
 *        pixel font. Use pango_font_description_set_absolute_size() if you need
 *        a particular size in device units.
 *
 * Sets the size field of a font description in fractional points. This is mutually
 * exclusive with pango_font_description_set_absolute_size().
 **/
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
 * @desc: a #PangoFontDescription
 *
 * Gets the size field of a font description.
 * See pango_font_description_set_size().
 *
 * Return value: the size field for the font description in points or device units.
 *   You must call pango_font_description_get_size_is_absolute()
 *   to find out which is the case. Returns 0 if the size field has not
 *   previously been set or it has been set to 0 explicitly.
 *   Use pango_font_description_get_set_fields() to
 *   find out if the field was explicitly set or not.
 **/
gint
pango_font_description_get_size (const PangoFontDescription *desc)
{
  g_return_val_if_fail (desc != NULL, pfd_defaults.size);

  return desc->size;
}

/**
 * pango_font_description_set_absolute_size:
 * @desc: a #PangoFontDescription
 * @size: the new size, in Pango units. There are %PANGO_SCALE Pango units in one
 *   device unit. For an output backend where a device unit is a pixel, a @size
 *   value of 10 * PANGO_SCALE gives a 10 pixel font.
 *
 * Sets the size field of a font description, in device units. This is mutually
 * exclusive with pango_font_description_set_size() which sets the font size
 * in points.
 *
 * Since: 1.8
 **/
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
 * @desc: a #PangoFontDescription
 *
 * Determines whether the size of the font is in points (not absolute) or device units (absolute).
 * See pango_font_description_set_size() and pango_font_description_set_absolute_size().
 *
 * Return value: whether the size for the font description is in
 *   points or device units.  Use pango_font_description_get_set_fields() to
 *   find out if the size field of the font description was explicitly set or not.
 *
 * Since: 1.8
 **/
gboolean
pango_font_description_get_size_is_absolute (const PangoFontDescription *desc)
{
  g_return_val_if_fail (desc != NULL, pfd_defaults.size_is_absolute);

  return desc->size_is_absolute;
}

/**
 * pango_font_description_set_gravity:
 * @desc: a #PangoFontDescription
 * @gravity: the gravity for the font description.
 *
 * Sets the gravity field of a font description. The gravity field
 * specifies how the glyphs should be rotated.  If @gravity is
 * %PANGO_GRAVITY_AUTO, this actually unsets the gravity mask on
 * the font description.
 *
 * This function is seldom useful to the user.  Gravity should normally
 * be set on a #PangoContext.
 *
 * Since: 1.16
 **/
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
 * @desc: a #PangoFontDescription
 *
 * Gets the gravity field of a font description. See
 * pango_font_description_set_gravity().
 *
 * Return value: the gravity field for the font description. Use
 *   pango_font_description_get_set_fields() to find out if
 *   the field was explicitly set or not.
 *
 * Since: 1.16
 **/
PangoGravity
pango_font_description_get_gravity (const PangoFontDescription *desc)
{
  g_return_val_if_fail (desc != NULL, pfd_defaults.gravity);

  return desc->gravity;
}

/**
 * pango_font_description_get_set_fields:
 * @desc: a #PangoFontDescription
 *
 * Determines which fields in a font description have been set.
 *
 * Return value: a bitmask with bits set corresponding to the
 *   fields in @desc that have been set.
 **/
PangoFontMask
pango_font_description_get_set_fields (const PangoFontDescription *desc)
{
  g_return_val_if_fail (desc != NULL, pfd_defaults.mask);

  return desc->mask;
}

/**
 * pango_font_description_unset_fields:
 * @desc: a #PangoFontDescription
 * @to_unset: bitmask of fields in the @desc to unset.
 *
 * Unsets some of the fields in a #PangoFontDescription.  The unset
 * fields will get back to their default values.
 **/
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
 * @desc: a #PangoFontDescription
 * @desc_to_merge: the #PangoFontDescription to merge from
 * @replace_existing: if %TRUE, replace fields in @desc with the
 *   corresponding values from @desc_to_merge, even if they
 *   are already exist.
 *
 * Merges the fields that are set in @desc_to_merge into the fields in
 * @desc.  If @replace_existing is %FALSE, only fields in @desc that
 * are not already set are affected. If %TRUE, then fields that are
 * already set will be replaced as well.
 **/
void
pango_font_description_merge (PangoFontDescription       *desc,
			      const PangoFontDescription *desc_to_merge,
			      gboolean                    replace_existing)
{
  gboolean family_merged;

  g_return_if_fail (desc != NULL);
  g_return_if_fail (desc_to_merge != NULL);

  family_merged = desc_to_merge->family_name && (replace_existing || !desc->family_name);

  pango_font_description_merge_static (desc, desc_to_merge, replace_existing);

  if (family_merged)
    {
      desc->family_name = g_strdup (desc->family_name);
      desc->static_family = FALSE;
    }
}

/**
 * pango_font_description_merge_static:
 * @desc: a #PangoFontDescription
 * @desc_to_merge: the #PangoFontDescription to merge from
 * @replace_existing: if %TRUE, replace fields in @desc with the
 *   corresponding values from @desc_to_merge, even if they
 *   are already exist.
 *
 * Like pango_font_description_merge(), but only a shallow copy is made
 * of the family name and other allocated fields. @desc can only be
 * used until @desc_to_merge is modified or freed. This is meant
 * to be used when the merged font description is only needed temporarily.
 **/
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

  desc->mask |= new_mask;
}

static gint
compute_distance (const PangoFontDescription *a,
		  const PangoFontDescription *b)
{
  if (a->style == b->style)
    {
      return abs(a->weight - b->weight);
    }
  else if (a->style != PANGO_STYLE_NORMAL &&
	   b->style != PANGO_STYLE_NORMAL)
    {
      /* Equate oblique and italic, but with a big penalty
       */
      return 1000000 + abs (a->weight - b->weight);
    }
  else
    return G_MAXINT;
}

/**
 * pango_font_description_better_match:
 * @desc: a #PangoFontDescription
 * @old_match: a #PangoFontDescription, or %NULL
 * @new_match: a #PangoFontDescription
 *
 * Determines if the style attributes of @new_match are a closer match
 * for @desc than @old_match, or if @old_match is %NULL, determines if
 * @new_match is a match at all. Approximate matching is done for
 * weight and style; other attributes must match exactly.
 *
 * Return value: %TRUE if @new_match is a better match
 **/
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
 * @desc: a #PangoFontDescription, may be %NULL
 *
 * Make a copy of a #PangoFontDescription.
 *
 * Return value: the newly allocated #PangoFontDescription, which should
 *               be freed with pango_font_description_free(), or %NULL
 *               if @desc was %NULL.
 **/
PangoFontDescription *
pango_font_description_copy  (const PangoFontDescription  *desc)
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

  return result;
}

/**
 * pango_font_description_copy_static:
 * @desc: a #PangoFontDescription, may be %NULL
 *
 * Like pango_font_description_copy(), but only a shallow copy is made
 * of the family name and other allocated fields. The result can only
 * be used until @desc is modified or freed. This is meant to be used
 * when the copy is only needed temporarily.
 *
 * Return value: the newly allocated #PangoFontDescription, which should
 *               be freed with pango_font_description_free(), or %NULL
 *               if @desc was %NULL.
 **/
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

  return result;
}

/**
 * pango_font_description_equal:
 * @desc1: a #PangoFontDescription
 * @desc2: another #PangoFontDescription
 *
 * Compares two font descriptions for equality. Two font descriptions
 * are considered equal if the fonts they describe are provably identical.
 * This means that their masks do not have to match, as long as other fields
 * are all the same. (Two font descriptions may result in identical fonts
 * being loaded, but still compare %FALSE.)
 *
 * Return value: %TRUE if the two font descriptions are identical,
 *		 %FALSE otherwise.
 **/
gboolean
pango_font_description_equal (const PangoFontDescription  *desc1,
			      const PangoFontDescription  *desc2)
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
	  (desc1->family_name && desc2->family_name && g_ascii_strcasecmp (desc1->family_name, desc2->family_name) == 0));
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
 * @desc: a #PangoFontDescription
 *
 * Computes a hash of a #PangoFontDescription structure suitable
 * to be used, for example, as an argument to g_hash_table_new().
 * The hash value is independent of @desc->mask.
 *
 * Return value: the hash value.
 **/
guint
pango_font_description_hash (const PangoFontDescription *desc)
{
  guint hash = 0;

  g_return_val_if_fail (desc != NULL, 0);

  if (desc->family_name)
    hash = case_insensitive_hash (desc->family_name);
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
 * @desc: a #PangoFontDescription, may be %NULL
 *
 * Frees a font description.
 **/
void
pango_font_description_free  (PangoFontDescription  *desc)
{
  if (desc == NULL)
    return;

  if (desc->family_name && !desc->static_family)
    g_free (desc->family_name);

  g_slice_free (PangoFontDescription, desc);
}

/**
 * pango_font_descriptions_free:
 * @descs: a pointer to an array of #PangoFontDescription, may be %NULL
 * @n_descs: number of font descriptions in @descs
 *
 * Frees a list of font descriptions from pango_font_map_list_fonts()
 **/
void
pango_font_descriptions_free (PangoFontDescription **descs,
			      int                    n_descs)
{
  int i;

  if (descs == NULL)
    return;

  for (i = 0; i<n_descs; i++)
    pango_font_description_free (descs[i]);
  g_free (descs);
}

typedef struct
{
  int value;
  const char str[16];
} FieldMap;

static const FieldMap style_map[] = {
  { PANGO_STYLE_NORMAL, "" },
  { PANGO_STYLE_OBLIQUE, "Oblique" },
  { PANGO_STYLE_ITALIC, "Italic" }
};

static const FieldMap variant_map[] = {
  { PANGO_VARIANT_NORMAL, "" },
  { PANGO_VARIANT_SMALL_CAPS, "Small-Caps" }
};

static const FieldMap weight_map[] = {
  { PANGO_WEIGHT_ULTRALIGHT, "Ultra-Light" },
  { PANGO_WEIGHT_LIGHT, "Light" },
  { PANGO_WEIGHT_NORMAL, "" },
  { 500, "Medium" },
  { PANGO_WEIGHT_SEMIBOLD, "Semi-Bold" },
  { PANGO_WEIGHT_BOLD, "Bold" },
  { PANGO_WEIGHT_ULTRABOLD, "Ultra-Bold" },
  { PANGO_WEIGHT_HEAVY, "Heavy" }
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
  { PANGO_GRAVITY_SOUTH, "Upside-Down" },
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
find_field (const FieldMap *map, int n_elements, const char *str, int len, int *val)
{
  int i;

  for (i=0; i<n_elements; i++)
    {
      if (map[i].str[0] && field_matches (map[i].str, str, len))
	{
	  if (val)
	    *val = map[i].value;
	  return TRUE;
	}
    }

  return FALSE;
}

static gboolean
find_field_any (const char *str, int len, PangoFontDescription *desc)
{
  gboolean found = FALSE;

  if (g_ascii_strcasecmp (str, "Normal") == 0)
    return TRUE;

#define FIELD(NAME, MASK) \
  G_STMT_START { \
  if (find_field (NAME##_map, G_N_ELEMENTS (NAME##_map), str, len, \
		  desc ? (int *)&desc->NAME : NULL)) \
    { \
      found = TRUE; \
      if (desc) \
	desc->mask |= MASK; \
    } \
  } G_STMT_END

  FIELD (style,   PANGO_FONT_MASK_STYLE);
  FIELD (variant, PANGO_FONT_MASK_VARIANT);
  FIELD (weight,  PANGO_FONT_MASK_WEIGHT);
  FIELD (stretch, PANGO_FONT_MASK_STRETCH);
  FIELD (gravity, PANGO_FONT_MASK_GRAVITY);

#undef FIELD

  return found;
}

static const char *
getword (const char *str, const char *last, size_t *wordlen)
{
  const char *result;

  while (last > str && g_ascii_isspace (*(last - 1)))
    last--;

  result = last;
  while (result > str && !g_ascii_isspace (*(result - 1)))
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

/**
 * pango_font_description_from_string:
 * @str: string representation of a font description.
 *
 * Creates a new font description from a string representation in the
 * form "[FAMILY-LIST] [STYLE-OPTIONS] [SIZE]", where FAMILY-LIST is a
 * comma separated list of families optionally terminated by a comma,
 * STYLE_OPTIONS is a whitespace separated list of words where each WORD
 * describes one of style, variant, weight, stretch, or gravity, and SIZE
 * is a decimal number (size in points) or optionally followed by the
 * unit modifier "px" for absolute size. Any one of the options may
 * be absent.  If FAMILY-LIST is absent, then the family_name field of
 * the resulting font description will be initialized to %NULL.  If
 * STYLE-OPTIONS is missing, then all style options will be set to the
 * default values. If SIZE is missing, the size in the resulting font
 * description will be set to 0.
 *
 * Return value: a new #PangoFontDescription.
 **/
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
  p = getword (str, last, &wordlen);

  /* Look for a size at the end of the string
   */
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
  p = getword (str, last, &wordlen);
  while (wordlen != 0)
    {
      if (!find_field_any (p, wordlen, desc))
	break;
      else
	{
	  last = p;
	  p = getword (str, last, &wordlen);
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
      desc->family_name = g_strndup (str, last - str);
      desc->mask |= PANGO_FONT_MASK_FAMILY;
    }

  return desc;
}

static void
append_field (GString *str, const FieldMap *map, int n_elements, int val)
{
  int i;
  for (i=0; i<n_elements; i++)
    {
      if (map[i].value == val)
	{
	  if (map[i].str && map[i].str[0])
	    {
	      if (str->len > 0 && str->str[str->len -1] != ' ')
		g_string_append_c (str, ' ');
	      g_string_append (str, map[i].str);
	    }
	  return;
	}
    }

  if (str->len > 0 || str->str[str->len -1] != ' ')
    g_string_append_c (str, ' ');
  g_string_append_printf (str, "%d", val);
}

/**
 * pango_font_description_to_string:
 * @desc: a #PangoFontDescription
 *
 * Creates a string representation of a font description. See
 * pango_font_description_from_string() for a description of the
 * format of the string representation. The family list in the
 * string description will only have a terminating comma if the
 * last word of the list is a valid style option.
 *
 * Return value: a new string that must be freed with g_free().
 **/
char *
pango_font_description_to_string (const PangoFontDescription  *desc)
{
  GString *result;

  g_return_val_if_fail (desc != NULL, NULL);

  result = g_string_new (NULL);

  if (desc->family_name && desc->mask & PANGO_FONT_MASK_FAMILY)
    {
      const char *p;
      size_t wordlen;

      g_string_append (result, desc->family_name);

      /* We need to add a trailing comma if the family name ends
       * in a keyword like "Bold", or if the family name ends in
       * a number and no keywords will be added.
       */
      p = getword (desc->family_name, desc->family_name + strlen(desc->family_name), &wordlen);
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

  append_field (result, weight_map, G_N_ELEMENTS (weight_map), desc->weight);
  append_field (result, style_map, G_N_ELEMENTS (style_map), desc->style);
  append_field (result, stretch_map, G_N_ELEMENTS (stretch_map), desc->stretch);
  append_field (result, variant_map, G_N_ELEMENTS (variant_map), desc->variant);
  if (desc->mask & PANGO_FONT_MASK_GRAVITY)
    append_field (result, gravity_map, G_N_ELEMENTS (gravity_map), desc->gravity);

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

  return g_string_free (result, FALSE);
}

/**
 * pango_font_description_to_filename:
 * @desc: a #PangoFontDescription
 *
 * Creates a filename representation of a font description. The
 * filename is identical to the result from calling
 * pango_font_description_to_string(), but with underscores instead of
 * characters that are untypical in filenames, and in lower case only.
 *
 * Return value: a new string that must be freed with g_free().
 **/
char *
pango_font_description_to_filename (const PangoFontDescription  *desc)
{
  char *result;
  char *p;

  g_return_val_if_fail (desc != NULL, NULL);

  result = pango_font_description_to_string (desc);

  /* XXX This should be rewritten to read char-by-char instead
   * of byte-by-byte, to be Unicode safe.
   */
  p = result;
  while (*p)
    {
      if (strchr ("-+_.", *p) == NULL && !g_ascii_isalnum (*p))
	*p = '_';
      else
	*p = g_ascii_tolower (*p);
      p++;
    }

  return result;
}

G_DEFINE_ABSTRACT_TYPE (PangoFont, pango_font, G_TYPE_OBJECT)

static void
pango_font_class_init (PangoFontClass *class)
{
}

static void
pango_font_init (PangoFont *font)
{
}

/**
 * pango_font_describe:
 * @font: a #PangoFont
 *
 * Returns a description of the font, with font size set in points.
 * Use pango_font_describe_with_absolute_size() if you want the font
 * size in device units.
 *
 * Return value: a newly-allocated #PangoFontDescription object.
 **/
PangoFontDescription *
pango_font_describe (PangoFont      *font)
{
  g_return_val_if_fail (font != NULL, NULL);

  return PANGO_FONT_GET_CLASS (font)->describe (font);
}

/**
 * pango_font_describe_with_absolute_size:
 * @font: a #PangoFont
 *
 * Returns a description of the font, with absolute font size set
 * (in device units). Use pango_font_describe() if you want the font
 * size in points.
 *
 * Return value: a newly-allocated #PangoFontDescription object.
 *
 * Since: 1.14
 **/
PangoFontDescription *
pango_font_describe_with_absolute_size (PangoFont      *font)
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
 * @font: a #PangoFont
 * @language: the language tag
 *
 * Computes the coverage map for a given font and language tag.
 *
 * Return value: a newly-allocated #PangoCoverage object.
 **/
PangoCoverage *
pango_font_get_coverage (PangoFont     *font,
			 PangoLanguage *language)
{
  g_return_val_if_fail (font != NULL, NULL);

  return PANGO_FONT_GET_CLASS (font)->get_coverage (font, language);
}

/**
 * pango_font_find_shaper:
 * @font: a #PangoFont
 * @language: the language tag
 * @ch: a Unicode character.
 *
 * Finds the best matching shaper for a font for a particular
 * language tag and character point.
 *
 * Return value: the best matching shaper.
 **/
PangoEngineShape *
pango_font_find_shaper (PangoFont     *font,
			PangoLanguage *language,
			guint32        ch)
{
  PangoEngineShape* shaper;

  g_return_val_if_fail (font != NULL, NULL);

  shaper = PANGO_FONT_GET_CLASS (font)->find_shaper (font, language, ch);

  return shaper;
}

/**
 * pango_font_get_glyph_extents:
 * @font: a #PangoFont
 * @glyph: the glyph index
 * @ink_rect: rectangle used to store the extents of the glyph as drawn
 *            or %NULL to indicate that the result is not needed.
 * @logical_rect: rectangle used to store the logical extents of the glyph
 *            or %NULL to indicate that the result is not needed.
 *
 * Gets the logical and ink extents of a glyph within a font. The
 * coordinate system for each rectangle has its origin at the
 * base line and horizontal origin of the character with increasing
 * coordinates extending to the right and down. The macros PANGO_ASCENT(),
 * PANGO_DESCENT(), PANGO_LBEARING(), and PANGO_RBEARING() can be used to convert
 * from the extents rectangle to more traditional font metrics. The units
 * of the rectangles are in 1/PANGO_SCALE of a device unit.
 *
 * If @font is %NULL, this function gracefully sets some sane values in the
 * output variables and returns.
 **/
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
	  logical_rect->x = logical_rect->y = 0;
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
 * @font: a #PangoFont
 * @language: language tag used to determine which script to get the metrics
 *            for, or %NULL to indicate to get the metrics for the entire
 *            font.
 *
 * Gets overall metric information for a font. Since the metrics may be
 * substantially different for different scripts, a language tag can
 * be provided to indicate that the metrics should be retrieved that
 * correspond to the script(s) used by that language.
 *
 * If @font is %NULL, this function gracefully sets some sane values in the
 * output variables and returns.
 *
 * Return value: a #PangoFontMetrics object. The caller must call pango_font_metrics_unref()
 *   when finished using the object.
 **/
PangoFontMetrics *
pango_font_get_metrics (PangoFont        *font,
			PangoLanguage    *language)
{
  if (G_UNLIKELY (!font))
    {
      PangoFontMetrics *metrics = pango_font_metrics_new ();

      metrics->ascent = PANGO_SCALE * PANGO_UNKNOWN_GLYPH_HEIGHT;
      metrics->descent = 0;
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
 * @font: a #PangoFont, or %NULL
 *
 * Gets the font map for which the font was created.
 *
 * Note that the font maintains a <firstterm>weak</firstterm> reference
 * to the font map, so if all references to font map are dropped, the font
 * map will be finalized even if there are fonts created with the font
 * map that are still alive.  In that case this function will return %NULL.
 * It is the responsibility of the user to ensure that the font map is kept
 * alive.  In most uses this is not an issue as a #PangoContext holds
 * a reference to the font map.
 *
 * Return value: the #PangoFontMap for the font, or %NULL if @font is %NULL.
 *
 * Since: 1.10
 **/
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

GType
pango_font_metrics_get_type (void)
{
  static GType our_type = 0;

  if (G_UNLIKELY (our_type == 0))
    our_type = g_boxed_type_register_static (I_("PangoFontMetrics"),
					     (GBoxedCopyFunc)pango_font_metrics_ref,
					     (GBoxedFreeFunc)pango_font_metrics_unref);

  return our_type;
}


/**
 * pango_font_metrics_new:
 *
 * Creates a new #PangoFontMetrics structure. This is only for
 * internal use by Pango backends and there is no public way
 * to set the fields of the structure.
 *
 * Return value: a newly-created #PangoFontMetrics structure
 *   with a reference count of 1.
 **/
PangoFontMetrics *
pango_font_metrics_new (void)
{
  PangoFontMetrics *metrics = g_slice_new0 (PangoFontMetrics);
  metrics->ref_count = 1;

  return metrics;
}

/**
 * pango_font_metrics_ref:
 * @metrics: a #PangoFontMetrics structure, may be %NULL
 *
 * Increase the reference count of a font metrics structure by one.
 *
 * Return value: @metrics
 **/
PangoFontMetrics *
pango_font_metrics_ref (PangoFontMetrics *metrics)
{
  if (metrics == NULL)
    return NULL;

  g_atomic_int_inc (&metrics->ref_count);

  return metrics;
}

/**
 * pango_font_metrics_unref:
 * @metrics: a #PangoFontMetrics structure, may be %NULL
 *
 * Decrease the reference count of a font metrics structure by one. If
 * the result is zero, frees the structure and any associated
 * memory.
 **/
void
pango_font_metrics_unref (PangoFontMetrics *metrics)
{
  if (metrics == NULL)
    return;

  g_return_if_fail (metrics->ref_count > 0 );

  if (g_atomic_int_dec_and_test (&metrics->ref_count))
    g_slice_free (PangoFontMetrics, metrics);
}

/**
 * pango_font_metrics_get_ascent:
 * @metrics: a #PangoFontMetrics structure
 *
 * Gets the ascent from a font metrics structure. The ascent is
 * the distance from the baseline to the logical top of a line
 * of text. (The logical top may be above or below the top of the
 * actual drawn ink. It is necessary to lay out the text to figure
 * where the ink will be.)
 *
 * Return value: the ascent, in Pango units. (1 point == %PANGO_SCALE Pango units.)
 **/
int
pango_font_metrics_get_ascent (PangoFontMetrics *metrics)
{
  g_return_val_if_fail (metrics != NULL, 0);

  return metrics->ascent;
}

/**
 * pango_font_metrics_get_descent:
 * @metrics: a #PangoFontMetrics structure
 *
 * Gets the descent from a font metrics structure. The descent is
 * the distance from the baseline to the logical bottom of a line
 * of text. (The logical bottom may be above or below the bottom of the
 * actual drawn ink. It is necessary to lay out the text to figure
 * where the ink will be.)
 *
 * Return value: the descent, in Pango units. (1 point == %PANGO_SCALE Pango units.)
 **/
int
pango_font_metrics_get_descent (PangoFontMetrics *metrics)
{
  g_return_val_if_fail (metrics != NULL, 0);

  return metrics->descent;
}

/**
 * pango_font_metrics_get_approximate_char_width:
 * @metrics: a #PangoFontMetrics structure
 *
 * Gets the approximate character width for a font metrics structure.
 * This is merely a representative value useful, for example, for
 * determining the initial size for a window. Actual characters in
 * text will be wider and narrower than this.
 *
 * Return value: the character width, in Pango units. (1 point == %PANGO_SCALE Pango units.)
 **/
int
pango_font_metrics_get_approximate_char_width (PangoFontMetrics *metrics)
{
  g_return_val_if_fail (metrics != NULL, 0);

  return metrics->approximate_char_width;
}

/**
 * pango_font_metrics_get_approximate_digit_width:
 * @metrics: a #PangoFontMetrics structure
 *
 * Gets the approximate digit width for a font metrics structure.
 * This is merely a representative value useful, for example, for
 * determining the initial size for a window. Actual digits in
 * text can be wider or narrower than this, though this value
 * is generally somewhat more accurate than the result of
 * pango_font_metrics_get_approximate_char_width() for digits.
 *
 * Return value: the digit width, in Pango units. (1 point == %PANGO_SCALE Pango units.)
 **/
int
pango_font_metrics_get_approximate_digit_width (PangoFontMetrics *metrics)
{
  g_return_val_if_fail (metrics != NULL, 0);

  return metrics->approximate_digit_width;
}

/**
 * pango_font_metrics_get_underline_position:
 * @metrics: a #PangoFontMetrics structure
 *
 * Gets the suggested position to draw the underline.
 * The value returned is the distance <emphasis>above</emphasis> the
 * baseline of the top of the underline. Since most fonts have
 * underline positions beneath the baseline, this value is typically
 * negative.
 *
 * Return value: the suggested underline position, in Pango units.
 *
 * Since: 1.6
 **/
int
pango_font_metrics_get_underline_position (PangoFontMetrics *metrics)
{
  g_return_val_if_fail (metrics != NULL, 0);

  return metrics->underline_position;
}

/**
 * pango_font_metrics_get_underline_thickness:
 * @metrics: a #PangoFontMetrics structure
 *
 * Gets the suggested thickness to draw for the underline.
 *
 * Return value: the suggested underline thickness, in Pango units.
 *
 * Since: 1.6
 **/
int
pango_font_metrics_get_underline_thickness (PangoFontMetrics *metrics)
{
  g_return_val_if_fail (metrics != NULL, 0);

  return metrics->underline_thickness;
}

/**
 * pango_font_metrics_get_strikethrough_position:
 * @metrics: a #PangoFontMetrics structure
 *
 * Gets the suggested position to draw the strikethrough.
 * The value returned is the distance <emphasis>above</emphasis> the
 * baseline of the top of the strikethrough.
 *
 * Return value: the suggested strikethrough position, in Pango units.
 *
 * Since: 1.6
 **/
int
pango_font_metrics_get_strikethrough_position (PangoFontMetrics *metrics)
{
  g_return_val_if_fail (metrics != NULL, 0);

  return metrics->strikethrough_position;
}

/**
 * pango_font_metrics_get_strikethrough_thickness:
 * @metrics: a #PangoFontMetrics structure
 *
 * Gets the suggested thickness to draw for the strikethrough.
 *
 * Return value: the suggested strikethrough thickness, in Pango units.
 *
 * Since: 1.6
 **/
int
pango_font_metrics_get_strikethrough_thickness (PangoFontMetrics *metrics)
{
  g_return_val_if_fail (metrics != NULL, 0);

  return metrics->strikethrough_thickness;
}

/*
 * PangoFontFamily
 */

G_DEFINE_ABSTRACT_TYPE (PangoFontFamily, pango_font_family, G_TYPE_OBJECT)

static void
pango_font_family_class_init (PangoFontFamilyClass *class)
{
}

static void
pango_font_family_init (PangoFontFamily *family)
{
}

/**
 * pango_font_family_get_name:
 * @family: a #PangoFontFamily
 *
 * Gets the name of the family. The name is unique among all
 * fonts for the font backend and can be used in a #PangoFontDescription
 * to specify that a face from this family is desired.
 *
 * Return value: the name of the family. This string is owned
 *   by the family object and must not be modified or freed.
 **/
G_CONST_RETURN char *
pango_font_family_get_name (PangoFontFamily  *family)
{
  g_return_val_if_fail (PANGO_IS_FONT_FAMILY (family), NULL);

  return PANGO_FONT_FAMILY_GET_CLASS (family)->get_name (family);
}

/**
 * pango_font_family_list_faces:
 * @family: a #PangoFontFamily
 * @faces: location to store an array of pointers to #PangoFontFace
 *   objects, or %NULL. This array should be freed with g_free()
 *   when it is no longer needed.
 * @n_faces: location to store number of elements in @faces.
 *
 * Lists the different font faces that make up @family. The faces
 * in a family share a common design, but differ in slant, weight,
 * width and other aspects.
 **/
void
pango_font_family_list_faces (PangoFontFamily  *family,
			      PangoFontFace  ***faces,
			      int              *n_faces)
{
  g_return_if_fail (PANGO_IS_FONT_FAMILY (family));

  PANGO_FONT_FAMILY_GET_CLASS (family)->list_faces (family, faces, n_faces);
}

/**
 * pango_font_family_is_monospace:
 * @family: a #PangoFontFamily
 *
 * A monospace font is a font designed for text display where the the
 * characters form a regular grid. For Western languages this would
 * mean that the advance width of all characters are the same, but
 * this categorization also includes Asian fonts which include
 * double-width characters: characters that occupy two grid cells.
 * g_unichar_iswide() returns a result that indicates whether a
 * character is typically double-width in a monospace font.
 *
 * The best way to find out the grid-cell size is to call
 * pango_font_metrics_get_approximate_digit_width(), since the results
 * of pango_font_metrics_get_approximate_char_width() may be affected
 * by double-width characters.
 *
 * Return value: %TRUE if the family is monospace.
 *
 * Since: 1.4
 **/
gboolean
pango_font_family_is_monospace (PangoFontFamily  *family)
{
  g_return_val_if_fail (PANGO_IS_FONT_FAMILY (family), FALSE);

  if (PANGO_FONT_FAMILY_GET_CLASS (family)->is_monospace)
    return PANGO_FONT_FAMILY_GET_CLASS (family)->is_monospace (family);
  else
    return FALSE;
}

/*
 * PangoFontFace
 */

G_DEFINE_ABSTRACT_TYPE (PangoFontFace, pango_font_face, G_TYPE_OBJECT)

static void
pango_font_face_class_init (PangoFontFaceClass *class)
{
}

static void
pango_font_face_init (PangoFontFace *face)
{
}

/**
 * pango_font_face_describe:
 * @face: a #PangoFontFace
 *
 * Returns the family, style, variant, weight and stretch of
 * a #PangoFontFace. The size field of the resulting font description
 * will be unset.
 *
 * Return value: a newly-created #PangoFontDescription structure
 *  holding the description of the face. Use pango_font_description_free()
 *  to free the result.
 **/
PangoFontDescription *
pango_font_face_describe (PangoFontFace *face)
{
  g_return_val_if_fail (PANGO_IS_FONT_FACE (face), NULL);

  return PANGO_FONT_FACE_GET_CLASS (face)->describe (face);
}

/**
 * pango_font_face_is_synthesized:
 * @face: a #PangoFontFace
 *
 * Returns whether a #PangoFontFace is synthesized by the underlying
 * font rendering engine from another face, perhaps by shearing, emboldening,
 * or lightening it.
 *
 * Return value: whether @face is synthesized.
 *
 * Since: 1.18
 **/
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
 * @face: a #PangoFontFace.
 *
 * Gets a name representing the style of this face among the
 * different faces in the #PangoFontFamily for the face. This
 * name is unique among all faces in the family and is suitable
 * for displaying to users.
 *
 * Return value: the face name for the face. This string is
 *   owned by the face object and must not be modified or freed.
 **/
G_CONST_RETURN char *
pango_font_face_get_face_name (PangoFontFace *face)
{
  g_return_val_if_fail (PANGO_IS_FONT_FACE (face), NULL);

  return PANGO_FONT_FACE_GET_CLASS (face)->get_face_name (face);
}

/**
 * pango_font_face_list_sizes:
 * @face: a #PangoFontFace.
 * @sizes: location to store a pointer to an array of int. This array
 *         should be freed with g_free().
 * @n_sizes: location to store the number of elements in @sizes
 *
 * List the available sizes for a font. This is only applicable to bitmap
 * fonts. For scalable fonts, stores %NULL at the location pointed to by
 * @sizes and 0 at the location pointed to by @n_sizes. The sizes returned
 * are in Pango units and are sorted in ascending order.
 *
 * Since: 1.4
 **/
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
