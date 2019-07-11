/* Pango
 * pango-coverage.c: Coverage maps for fonts
 *
 * Copyright (C) 2000 Red Hat Software
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

/**
 * SECTION:coverage-maps
 * @short_description:Unicode character range coverage storage
 * @title:Coverage Maps
 *
 * It is often necessary in Pango to determine if a particular font can
 * represent a particular character, and also how well it can represent
 * that character. The #PangoCoverage is a data structure that is used
 * to represent that information.
 */
#include "config.h"
#include <string.h>

#include "pango-coverage.h"

struct _PangoCoverage
{
  guint ref_count;

  hb_set_t *chars;
};

/**
 * pango_coverage_new:
 *
 * Create a new #PangoCoverage
 *
 * Return value: the newly allocated #PangoCoverage,
 *               initialized to %PANGO_COVERAGE_NONE
 *               with a reference count of one, which
 *               should be freed with pango_coverage_unref().
 **/
PangoCoverage *
pango_coverage_new (void)
{
 return pango_coverage_new_with_chars (hb_set_create ());
}

PangoCoverage *
pango_coverage_new_with_chars (hb_set_t *chars)
{
  PangoCoverage *coverage = g_slice_new (PangoCoverage);

  coverage->ref_count = 1;

  coverage->chars = hb_set_reference (chars);

  return coverage;
}

/**
 * pango_coverage_copy:
 * @coverage: a #PangoCoverage
 *
 * Copy an existing #PangoCoverage. (This function may now be unnecessary
 * since we refcount the structure. File a bug if you use it.)
 *
 * Return value: (transfer full): the newly allocated #PangoCoverage,
 *               with a reference count of one, which should be freed
 *               with pango_coverage_unref().
 **/
PangoCoverage *
pango_coverage_copy (PangoCoverage *coverage)
{
  g_return_val_if_fail (coverage != NULL, NULL);

  return pango_coverage_new_with_chars (coverage->chars);
}

/**
 * pango_coverage_ref:
 * @coverage: a #PangoCoverage
 *
 * Increase the reference count on the #PangoCoverage by one
 *
 * Return value: @coverage
 **/
PangoCoverage *
pango_coverage_ref (PangoCoverage *coverage)
{
  g_return_val_if_fail (coverage != NULL, NULL);

  g_atomic_int_inc ((int *) &coverage->ref_count);

  return coverage;
}

/**
 * pango_coverage_unref:
 * @coverage: a #PangoCoverage
 *
 * Decrease the reference count on the #PangoCoverage by one.
 * If the result is zero, free the coverage and all associated memory.
 **/
void
pango_coverage_unref (PangoCoverage *coverage)
{
  g_return_if_fail (coverage != NULL);
  g_return_if_fail (coverage->ref_count > 0);

  if (g_atomic_int_dec_and_test ((int *) &coverage->ref_count))
    {
      hb_set_destroy (coverage->chars);
      g_slice_free (PangoCoverage, coverage);
    }
}

/**
 * pango_coverage_get:
 * @coverage: a #PangoCoverage
 * @index_: the index to check
 *
 * Determine whether a particular index is covered by @coverage
 *
 * Return value: the coverage level of @coverage for character @index_.
 **/
PangoCoverageLevel
pango_coverage_get (PangoCoverage *coverage,
		    int            index)
{
  if (hb_set_has (coverage->chars, (hb_codepoint_t)index))
    return PANGO_COVERAGE_EXACT;

  return PANGO_COVERAGE_NONE;
}

/**
 * pango_coverage_set:
 * @coverage: a #PangoCoverage
 * @index_: the index to modify
 * @level: the new level for @index_
 *
 * Modify a particular index within @coverage
 **/
void
pango_coverage_set (PangoCoverage     *coverage,
		    int                index,
		    PangoCoverageLevel level)
{
  if (level != PANGO_COVERAGE_NONE)
    hb_set_add (coverage->chars, (hb_codepoint_t)index);
  else
    hb_set_del (coverage->chars, (hb_codepoint_t)index);
}

/**
 * pango_coverage_max:
 * @coverage: a #PangoCoverage
 * @other: another #PangoCoverage
 *
 * Set the coverage for each index in @coverage to be the max (better)
 * value of the current coverage for the index and the coverage for
 * the corresponding index in @other.
 **/
void
pango_coverage_max (PangoCoverage *coverage,
		    PangoCoverage *other)
{
}

/**
 * pango_coverage_to_bytes:
 * @coverage: a #PangoCoverage
 * @bytes: (out) (array length=n_bytes) (element-type guint8):
 *   location to store result (must be freed with g_free())
 * @n_bytes: (out): location to store size of result
 *
 * Convert a #PangoCoverage structure into a flat binary format
 **/
void
pango_coverage_to_bytes (PangoCoverage  *coverage,
			 guchar        **bytes,
			 int            *n_bytes)
{
  *bytes = NULL;
  *n_bytes = 0;
}

/**
 * pango_coverage_from_bytes:
 * @bytes: (array length=n_bytes) (element-type guint8): binary data
 *   representing a #PangoCoverage
 * @n_bytes: the size of @bytes in bytes
 *
 * Convert data generated from pango_coverage_to_bytes() back
 * to a #PangoCoverage
 *
 * Return value: (transfer full) (nullable): a newly allocated
 *               #PangoCoverage, or %NULL if the data was invalid.
 **/
PangoCoverage *
pango_coverage_from_bytes (guchar *bytes,
			   int     n_bytes)
{
  return NULL;
}
