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
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Library General Public License for more details.
 *
 * You should have received a copy of the GNU Library General Public
 * License along with this library; if not, write to the
 * Free Software Foundation, Inc., 59 Temple Place - Suite 330,
 * Boston, MA 02111-1307, USA.
 */

#include "config.h"
#include <string.h>

#include "pango-coverage-private.h"

G_DEFINE_TYPE (PangoCoverage, pango_coverage, G_TYPE_OBJECT)

static void
pango_coverage_init (PangoCoverage *coverage)
{
}

static void
pango_coverage_finalize (GObject *object)
{
  PangoCoverage *coverage = PANGO_COVERAGE (object);

  if (coverage->chars)
    hb_set_destroy (coverage->chars);

  G_OBJECT_CLASS (pango_coverage_parent_class)->finalize (object);
}

static PangoCoverageLevel
pango_coverage_real_get (PangoCoverage *coverage,
                         int            index)
{
  if (coverage->chars == NULL)
    return PANGO_COVERAGE_NONE;

  if (hb_set_has (coverage->chars, (hb_codepoint_t)index))
    return PANGO_COVERAGE_EXACT;
  else
    return PANGO_COVERAGE_NONE;
}

static void
pango_coverage_real_set (PangoCoverage     *coverage,
                         int                index,
                         PangoCoverageLevel level)
{
  if (coverage->chars == NULL)
    coverage->chars = hb_set_create ();

  if (level != PANGO_COVERAGE_NONE)
    hb_set_add (coverage->chars, (hb_codepoint_t)index);
  else
    hb_set_del (coverage->chars, (hb_codepoint_t)index);
}

static PangoCoverage *
pango_coverage_real_copy (PangoCoverage *coverage)
{
  PangoCoverage *copy;

  g_return_val_if_fail (coverage != NULL, NULL);

  copy = g_object_new (PANGO_TYPE_COVERAGE, NULL);
  if (coverage->chars)
    {
      int i;

      copy->chars = hb_set_create ();
      for (i = hb_set_get_min (coverage->chars); i <= hb_set_get_max (coverage->chars); i++)
        {
          if (hb_set_has (coverage->chars, (hb_codepoint_t)i))
            hb_set_add (copy->chars, (hb_codepoint_t)i);
        }
    }

  return copy;
}

static void
pango_coverage_class_init (PangoCoverageClass *class)
{
  GObjectClass *object_class = G_OBJECT_CLASS (class);

  object_class->finalize = pango_coverage_finalize;

  class->get = pango_coverage_real_get;
  class->set = pango_coverage_real_set;
  class->copy = pango_coverage_real_copy;
}

/**
 * pango_coverage_new:
 *
 * Create a new `PangoCoverage`
 *
 * Return value: the newly allocated `PangoCoverage`, initialized
 *   to %PANGO_COVERAGE_NONE with a reference count of one, which
 *   should be freed with [method@Pango.Coverage.unref].
 */
PangoCoverage *
pango_coverage_new (void)
{
  return g_object_new (PANGO_TYPE_COVERAGE, NULL);
}

/*< private>
 * pango_coverage_new_for_hb_face:
 * @hb_face: a `hb_face_t`
 *
 * Creates a new `PangoCoverage` for the given @hb_face.
 *
 * Returns: the newly allocated `PangoCoverage`
 */
PangoCoverage *
pango_coverage_new_for_hb_face (hb_face_t *hb_face)
{
  PangoCoverage *coverage;

  coverage = g_object_new (PANGO_TYPE_COVERAGE, NULL);

  coverage->chars = hb_set_create ();
  hb_face_collect_unicodes (hb_face, coverage->chars);

  return coverage;
}

/**
 * pango_coverage_copy:
 * @coverage: a `PangoCoverage`
 *
 * Copy an existing `PangoCoverage`.
 *
 * Return value: (transfer full): the newly allocated `PangoCoverage`,
 *   with a reference count of one, which should be freed with
 *   [method@Pango.Coverage.unref].
 */
PangoCoverage *
pango_coverage_copy (PangoCoverage *coverage)
{
  return PANGO_COVERAGE_GET_CLASS (coverage)->copy (coverage);
}

/**
 * pango_coverage_get:
 * @coverage: a `PangoCoverage`
 * @index_: the index to check
 *
 * Determine whether a particular index is covered by @coverage.
 *
 * Return value: the coverage level of @coverage for character @index_.
 */
PangoCoverageLevel
pango_coverage_get (PangoCoverage *coverage,
                    int            index)
{
  return PANGO_COVERAGE_GET_CLASS (coverage)->get (coverage, index);
}

/**
 * pango_coverage_set:
 * @coverage: a `PangoCoverage`
 * @index_: the index to modify
 * @level: the new level for @index_
 *
 * Modify a particular index within @coverage
 */
void
pango_coverage_set (PangoCoverage     *coverage,
                    int                index,
                    PangoCoverageLevel level)
{
  PANGO_COVERAGE_GET_CLASS (coverage)->set (coverage, index, level);
}
