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

#include <string.h>

#include <pango/pango-coverage.h>

typedef struct _PangoBlockInfo PangoBlockInfo;

#define N_BLOCKS_INCREMENT 256

/* The structure of a PangoCoverage object is a two-level table, with blocks of size 256.
 * each block is stored as a packed array of 2 bit values for each index, in LSB order.
 */

struct _PangoBlockInfo
{
  guchar *data;		
  PangoCoverageLevel level;	/* Used if data == NULL */
};

struct _PangoCoverage
{
  guint ref_count;
  int n_blocks;
  int data_size;
  
  PangoBlockInfo *blocks;
};

/**
 * pango_coverage_new:
 * 
 * Create a new #PangoCoverage
 * 
 * Return value: a new PangoCoverage object, initialized to %PANGO_COVERAGE_NONE
 *               with a reference count of 0.
 **/
PangoCoverage *
pango_coverage_new (void)
{
  int i;
  PangoCoverage *coverage = g_new (PangoCoverage, 1);

  coverage->n_blocks = N_BLOCKS_INCREMENT;
  coverage->blocks = g_new (PangoBlockInfo, coverage->n_blocks);
  coverage->ref_count = 1;

  for (i=0; i<coverage->n_blocks; i++)
    {
      coverage->blocks[i].data = NULL;
      coverage->blocks[i].level = PANGO_COVERAGE_NONE;
    }
  
  return coverage;
}

/**
 * pango_coverage_copy:
 * @coverage: a #PangoCoverage
 * 
 * Copy an existing #PangoCoverage. (This function may now be unecessary 
 * since we refcount the structure. Mail otaylor@redhat.com if you
 * use it.)
 * 
 * Return value: a copy of @coverage with a reference count of 1
 **/
PangoCoverage *
pango_coverage_copy (PangoCoverage *coverage)
{
  int i;
  PangoCoverage *result = g_new (PangoCoverage, 1);

  g_return_val_if_fail (coverage != NULL, NULL);
  
  result->n_blocks = coverage->n_blocks;
  result->blocks = g_new (PangoBlockInfo, coverage->n_blocks);
  result->ref_count = 1;

  for (i=0; i<coverage->n_blocks; i++)
    {
      if (coverage->blocks[i].data)
	{
	  result->blocks[i].data = g_new (guchar, 64);
	  memcpy (result->blocks[i].data, coverage->blocks[i].data, 64);
	}
      else
	result->blocks[i].data = NULL;
	
      result->blocks[i].level = coverage->blocks[i].level;
    }
  
  return result;
}

/**
 * pango_coverage_ref:
 * @coverage: a #PangoCoverage
 * 
 * Increase the reference count on the #PangoCoverage by one
 **/
void
pango_coverage_ref (PangoCoverage *coverage)
{
  g_return_if_fail (coverage != NULL);

  coverage->ref_count++;
}

/**
 * pango_coverage_unref:
 * @coverage: a #PangoCoverage
 * 
 * Increase the reference count on the #PangoCoverage by one.
 * if the result is zero, free the coverage and all associated memory.
 **/
void
pango_coverage_unref (PangoCoverage *coverage)
{
  int i;
  
  g_return_if_fail (coverage != NULL);
  g_return_if_fail (coverage->ref_count > 0);

  coverage->ref_count--;

  if (coverage->ref_count == 0)
    {
      for (i=0; i<coverage->n_blocks; i++)
	{
	  if (coverage->blocks[i].data)
	    g_free (coverage->blocks[i].data);
	}
      
      g_free (coverage->blocks);
      g_free (coverage);
    }
}

/**
 * pango_coverage_get:
 * @coverage: a #PangoCoverage
 * @index: the index to check
 * 
 * Determine whether a particular index is covered by @coverage
 * 
 * Return value: 
 **/
PangoCoverageLevel
pango_coverage_get (PangoCoverage *coverage,
		    int            index)
{
  int block_index;
  
  g_return_val_if_fail (coverage != NULL, PANGO_COVERAGE_NONE);

  block_index = index / 256;

  if (block_index > coverage->n_blocks)
    return PANGO_COVERAGE_NONE;
  else
    {
      guchar *data = coverage->blocks[block_index].data;
      if (data)
	{
	  int i = index % 256;
	  int shift = (i % 4) * 2;

	  return (data[i/4] >> shift) & 0x3;
	}
      else
	return coverage->blocks[block_index].level;
    }
}

/**
 * pango_coverage_set:
 * @coverage: a #PangoCoverage
 * @index: the index to modify
 * @level: the new level for @index
 * 
 * Modify a particular index within @coverage
 **/
void pango_coverage_set (PangoCoverage     *coverage,
			 int                index,
			 PangoCoverageLevel level)
{
  int block_index, i;
  guchar *data;
  
  g_return_if_fail (coverage != NULL);
  g_return_if_fail (level >= 0 || level <= 3);

  block_index = index / 256;

  if (block_index > coverage->n_blocks)
    {
      coverage->n_blocks += N_BLOCKS_INCREMENT;
      coverage->blocks = g_renew (PangoBlockInfo, coverage->blocks, coverage->n_blocks);
    }

  data = coverage->blocks[block_index].data;
  if (!data)
    {
      guchar byte;
      
      if (level == coverage->blocks[block_index].level)
	return;
      
      data = g_new (guchar, 64);
      coverage->blocks[block_index].data = data;

      byte = coverage->blocks[block_index].level |
	(coverage->blocks[block_index].level << 2) |
	(coverage->blocks[block_index].level << 4) |
        (coverage->blocks[block_index].level << 6);
      
      for (i=0; i<64; i++)
	memset (data, byte, 64);
    }

  i = index % 256;
  data[i/4] |= level << ((i % 4) * 2);
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
void pango_coverage_max (PangoCoverage *coverage,
			 PangoCoverage *other)
{
  int block_index, i;
  int old_blocks;
  
  g_return_if_fail (coverage != NULL);
  
  old_blocks = MIN (coverage->n_blocks, other->n_blocks);

  if (other->n_blocks > coverage->n_blocks)
    {
      coverage->n_blocks += N_BLOCKS_INCREMENT;
      coverage->blocks = g_renew (PangoBlockInfo, coverage->blocks, coverage->n_blocks);

      for (block_index = old_blocks; block_index < coverage->n_blocks; block_index++)
	{
	  if (other->blocks[block_index].data)
	    {
	      coverage->blocks[block_index].data = g_new (guchar, 64);
	      memcpy (coverage->blocks[block_index].data, other->blocks[block_index].data, 64);
	    }
	  else
	    coverage->blocks[block_index].data = NULL;
	  
	  coverage->blocks[block_index].level = other->blocks[block_index].level;
	}
    }
  
  for (block_index = 0; block_index < old_blocks; block_index++)
    {
      if (!coverage->blocks[block_index].data && !other->blocks[block_index].data)
	{
	  coverage->blocks[block_index].level = MAX (coverage->blocks[block_index].level, other->blocks[block_index].level);
	}
      else if (coverage->blocks[block_index].data && other->blocks[block_index].data)
	{
	  guchar *data = coverage->blocks[block_index].data;
	  
	  for (i=0; i<64; i++)
	    {
	      int byte1 = data[i];
	      int byte2 = other->blocks[block_index].data[i];

	      /* There are almost certainly some clever logical ops to do this */
	      data[i] =
		MAX (byte1 & 0x3, byte2 & 0x3) |
		MAX (byte1 & 0xc, byte2 & 0xc) |
		MAX (byte1 & 0x30, byte2 & 0x30) |
		MAX (byte1 & 0xc0, byte2 & 0xc00);
	    }
	}
      else
	{
	  guchar *src, *dest;
	  int level, byte2;

	  if (coverage->blocks[block_index].data)
	    {
	      src = dest = coverage->blocks[block_index].data;
	      level = other->blocks[block_index].level;
	    }
	  else
	    {
	      src = other->blocks[block_index].data;
	      dest = g_new (guchar, 64);
	      coverage->blocks[block_index].data = dest;
	      level = coverage->blocks[block_index].level;
	    }
	  
	  byte2 = level | (level << 2) | (level << 4) | (level << 6);
	  
	  for (i=0; i<64; i++)
	    {
	      int byte1 = src[i];

	      /* There are almost certainly some clever logical ops to do this */
	      dest[i] =
		MAX (byte1 & 0x3, byte2 & 0x3) |
		MAX (byte1 & 0xc, byte2 & 0xc) |
		MAX (byte1 & 0x30, byte2 & 0x30) |
		MAX (byte1 & 0xc0, byte2 & 0xc00);
	    }
	}
    }
}


