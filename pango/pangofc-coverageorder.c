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
#include <gio/gio.h>

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

static int
bit_matrix_get_length (BitMatrix *b)
{
  return (b->rows * b->cols) / 8 + 1;
}

static void
bit_matrix_get_size (BitMatrix *b,
                     int       *rows,
                     int       *cols)
{
  *rows = b->rows;
  *cols = b->cols;
}

static char *
bit_matrix_get_bits (BitMatrix *b)
{
  return b->bits;
}

static BitMatrix *
bit_matrix_new (int rows, int cols, char *bits)
{
  BitMatrix *b = g_new (BitMatrix, 1);
  int len;

  b->rows = rows;
  b->cols = cols;
  len = (rows * cols) / 8 + 1;
  b->bits = g_new (char, len);
  if (bits)
    memcpy (b->bits, bits, len);
  else
    memset (b->bits, 0, len);

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
  co->order = bit_matrix_new (coverages->len, coverages->len, NULL);

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

gboolean
coverage_order_save (CoverageOrder  *co,
                     FcFontSet      *fonts,
                     const char     *filename,
                     GError        **error)
{
  char *prefix;
  gsize prefix_len;
  gsize idx_len;
  gsize len;
  char *contents;
  gboolean retval;
  guint32 *idx;
  int i;
  int rows, cols;

  bit_matrix_get_size (co->order, &rows, &cols);

  prefix = g_strdup_printf ("%d %d\n", rows, fonts->nfont);
  prefix_len = strlen (prefix);

  idx_len = sizeof (guint32) * fonts->nfont * 2;
  idx = g_new (guint32, idx_len);
  for (i = 0; i < fonts->nfont; i++)
    {
      FcPattern *p = fonts->fonts[i];
      idx[2*i] = FcPatternHash (p);
      idx[2*i+1] = GPOINTER_TO_UINT (g_hash_table_lookup (co->idx, p)) - 1;
    }

  len = prefix_len + idx_len + bit_matrix_get_length (co->order);
  contents = malloc (len);

  memcpy (contents, prefix, prefix_len);
  memcpy (contents + prefix_len, idx, idx_len);
  memcpy (contents + prefix_len + idx_len, bit_matrix_get_bits (co->order), bit_matrix_get_length (co->order));

  retval = g_file_set_contents (filename, contents, len, error);

  g_free (contents);
  g_free (prefix);
  g_free (idx);

  g_debug ("Wrote %ld bytes to %s.", len, filename);
  if (g_getenv ("EXIT")) exit (0);

  return retval;
}

CoverageOrder *
coverage_order_load (FcFontSet   *fonts,
                     const char  *filename,
                     GError     **error)
{
  CoverageOrder *co = NULL;
  GMappedFile *file;
  char *contents;
  char *prefix;
  char **parts;
  int size;
  int nfont;
  int prefix_len;
  int idx_len;
  guint32 *idx;
  GHashTable *table;
  int i;

  file = g_mapped_file_new (filename, FALSE, error);
  if (!file)
    return NULL;

  contents = g_mapped_file_get_contents (file);

  prefix = NULL;
  for (i = 0; i < 100; i++)
    {
      if (contents[i] == '\n')
        prefix = g_strndup (contents, i);
    }

  if (prefix == NULL)
    {
      g_set_error (error,
                   G_IO_ERROR, G_IO_ERROR_FAILED,
                   "%s: Didn't find prefix", filename);
      goto out;
    }

  parts = g_strsplit (prefix, " ", -1);
  if (g_strv_length (parts) != 2)
    {
      g_set_error (error,
                   G_IO_ERROR, G_IO_ERROR_FAILED,
                   "%s: Prefix looks bad", filename);
      g_free (prefix);
      g_strfreev (parts);
      goto out;
    }

  prefix_len = strlen (prefix) + 1;

  size = (int) g_ascii_strtoll (parts[0], NULL, 10);
  nfont = (int) g_ascii_strtoll (parts[1], NULL, 10);

  g_strfreev (parts);
  g_free (prefix);

  if (size <= 0 || nfont <= size)
    {
      g_set_error (error,
                   G_IO_ERROR, G_IO_ERROR_FAILED,
                   "%s: Numbers don't add up", filename);
      goto out;
    }

  if (nfont != fonts->nfont)
    {
      g_set_error (error,
                   G_IO_ERROR, G_IO_ERROR_FAILED,
                   "%s: Wrong number of fonts", filename);
      goto out;
    }

  table = g_hash_table_new (g_direct_hash, g_direct_equal);

  idx_len = sizeof (guint32) * nfont * 2;
  idx = (guint32 *)(contents + prefix_len);

  for (i = 0; i < fonts->nfont; i++)
    {
      FcPattern *p = fonts->fonts[i];
      guint32 hash = idx[2*i];
      guint32 index = idx[2*i+1];

      if (hash != FcPatternHash (p))
        {
          g_set_error (error,
                       G_IO_ERROR, G_IO_ERROR_FAILED,
                       "%s: Fonts changed", filename);
          g_hash_table_unref (table);
          goto out;
        }

      g_hash_table_insert (table, p, GUINT_TO_POINTER (index + 1));
    }

  co = g_new (CoverageOrder, 1);
  co->idx = table;
  co->order = bit_matrix_new (size, size, contents + prefix_len + idx_len);

out:
  g_mapped_file_unref (file);

  return co;
}
