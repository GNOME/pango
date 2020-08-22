/* Pango
 * coverageorder.c:
 *
 * Copyright (C) 2020 Red Hat, Inc.
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
 * You should have received a copy of the GNU Lesser General Public
 * License along with this library. If not, see <http://www.gnu.org/licenses/>.
 *
 * Authors: Matthias Clasen
 */

#include "config.h"

#include "pangofc-coverageorder-private.h"
#include <glib.h>

/* BitMatrix is a simple matrix of bits that can be
 * addressed by their row and column.
 */
typedef struct _BitMatrix BitMatrix;

struct _BitMatrix
{
  int rows;
  int cols;
  char *bits;
};

static void
bit_matrix_free (BitMatrix *b)
{
  g_free (b->bits);
  g_free (b);
}

static BitMatrix *
bit_matrix_new (int rows, int cols)
{
  BitMatrix *b = g_new (BitMatrix, 1);

  b->rows = rows;
  b->cols = cols;
  b->bits = g_new0 (char, (rows * cols) / 8 + 1);

  return b;
}

static gboolean
bit_matrix_get (BitMatrix *b, int i, int j)
{
  int pos = i * b->cols + j;
  int byte = pos / 8;
  int bit = pos % 8;

  return (b->bits[byte] & (1 << bit)) != 0;
}

static void
bit_matrix_set (BitMatrix *b, int i, int j, gboolean value)
{
  int pos = i * b->cols + j;
  int byte = pos / 8;
  int bit = pos % 8;

  if (value)
    b->bits[byte] = b->bits[byte] | (1 << bit);
  else
    b->bits[byte] = b->bits[byte] & ~(1 << bit);
}

/* By coverage order, we mean the partial order that is defined on
 * the fonts by the subset relation on their charsets. This order
 * only depends on the font configuration, and can be computed
 * ahead of time or even saved to disk.
 *
 * An important aspect of why this works is that in practice,
 * many fonts have the same coverage, so our matrix stays much
 * smaller than |fonts| * |fonts|.
 */

struct _CoverageOrder
{
  GHashTable *idx;
  BitMatrix *order;
};

/*
 * coverage_order_free:
 *
 * Frees a CoverageOrder struct and all associated memory.
 */
void
coverage_order_free (CoverageOrder *co)
{
  g_hash_table_unref (co->idx);
  bit_matrix_free (co->order);
  g_free (co);
}

/*
 * coverage_order_is_subset:
 * @co: a #CoverageOrder
 * @p1: a pattern
 * @p2: another pattern
 *
 * Determines if the charset of @p1 is a subset
 * of the charset of @p2.
 *
 * Returns: %TRUE if @p1 is a subset of @p2
 */
gboolean
coverage_order_is_subset (CoverageOrder *co,
                          FcPattern     *p1,
                          FcPattern     *p2)
{
  int idx1, idx2;

  idx1 = GPOINTER_TO_INT (g_hash_table_lookup (co->idx, p1)) - 1;
  idx2 = GPOINTER_TO_INT (g_hash_table_lookup (co->idx, p2)) - 1;

  return bit_matrix_get (co->order, idx1, idx2);
}

/*
 * coverage_order_new:
 * @fonts: a set of fonts
 *
 * Compute the coverage order for the fonts in @fonts.
 *
 * Returns: a new #CoverageOrder
 */
CoverageOrder *
coverage_order_new (FcFontSet *fonts)
{
  CoverageOrder *co;
  int *idx;
  GPtrArray *coverages;
  int max, i, j;

  co = g_new (CoverageOrder, 1);

  co->idx = g_hash_table_new (g_direct_hash, g_direct_equal);

  idx = g_new (int, fonts->nfont);
  coverages = g_ptr_array_new ();

  /* First group the fonts that have the same
   * charset, by giving them the same 'index'.
   */
  max = -1;
  for (i = 0; i < fonts->nfont; i++)
    {
      FcPattern *p1 = fonts->fonts[i];
      FcCharSet *c1;

      FcPatternGetCharSet (p1, "charset", 0, &c1);
      idx[i] = max + 1;
      for (j = 0; j < i; j++)
        {
          FcPattern *p2 = fonts->fonts[j];
          FcCharSet *c2;

          FcPatternGetCharSet (p2, "charset", 0, &c2);

          if (FcCharSetEqual (c1, c2))
            {
              idx[i] = idx[j];
              break;
            }
        }

      g_hash_table_insert (co->idx, p1, GINT_TO_POINTER (idx[i] + 1));
      if (idx[i] > max)
        {
          g_ptr_array_add (coverages, c1);
          max = idx[i];
        }
    }

  /* Now compute the full incidence matrix for the
   * remaining charsets.
   */
  co->order = bit_matrix_new (coverages->len, coverages->len);

  for (i = 0; i < coverages->len; i++)
    {
      FcCharSet *ci = g_ptr_array_index (coverages, i);
      bit_matrix_set (co->order, i, i, TRUE);
      for (j = 0; j < i; j++)
        {
          FcCharSet *cj = g_ptr_array_index (coverages, j);
          gboolean v;
          int k;

          v = FALSE;
          for (k = 0; k < coverages->len; k++)
            {
              if (bit_matrix_get (co->order, j, k) &&
                  bit_matrix_get (co->order, k, i))
                {
                  v = TRUE;
                  break;
                }
            }

          if (v || FcCharSetIsSubset (cj, ci))
            bit_matrix_set (co->order, j, i, TRUE);

          v = FALSE;
          for (k = 0; k < coverages->len; k++)
            {
              if (bit_matrix_get (co->order, i, k) &&
                  bit_matrix_get (co->order, k, j))
                {
                  v = TRUE;
                  break;
                }
            }

          if (v || FcCharSetIsSubset (ci, cj))
            bit_matrix_set (co->order, i, j, TRUE);
        }
    }

  g_ptr_array_unref (coverages);
  g_free (idx);

  return co;
}
