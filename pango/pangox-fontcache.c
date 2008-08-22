/* Pango
 * pango-fontcache.c: Cache of XFontStructs by XLFD
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

#include "config.h"
#include "pangox.h"

/* Font cache
 */

/* Number of fonts to retain after they are not otherwise referenced.
 */
#define CACHE_SIZE 16

typedef struct _CacheEntry CacheEntry;

struct _PangoXFontCache
{
  Display *display;

  GHashTable *forward;
  GHashTable *back;

  GList *mru;
  GList *mru_tail;
  int mru_count;
};

struct _CacheEntry
{
  char *xlfd;
  XFontStruct *fs;

  gint ref_count;
  GList *mru;
};

static void
free_cache_entry (char            *xlfd,
		  CacheEntry      *entry,
		  PangoXFontCache *cache)
{
  g_free (entry->xlfd);
  XFreeFont (cache->display, entry->fs);

  g_slice_free (CacheEntry, entry);
}

/**
 * pango_x_font_cache_free:
 * @cache: a #PangoXFontCache
 *
 * Frees a #PangoXFontCache and all associated memory. All fonts loaded
 * through this font cache will be freed along with the cache.
 **/
void
pango_x_font_cache_free (PangoXFontCache *cache)
{
  g_return_if_fail (cache != NULL);

  g_hash_table_foreach (cache->forward, (GHFunc)free_cache_entry, cache);

  g_hash_table_destroy (cache->forward);
  g_hash_table_destroy (cache->back);

  g_list_free (cache->mru);

  g_slice_free (PangoXFontCache, cache);
}

/**
 * pango_x_font_cache_new:
 * @display: an X display.
 *
 * Creates a font cache for the specified display.
 *
 * Return value: The newly allocated #PangoXFontCache, which should be
 *               freed with pango_x_font_cache_free().
 **/
PangoXFontCache *
pango_x_font_cache_new (Display *display)
{
  PangoXFontCache *cache;

  g_return_val_if_fail (display != NULL, NULL);

  cache = g_slice_new (PangoXFontCache);

  cache->display = display;

  cache->forward = g_hash_table_new (g_str_hash, g_str_equal);
  cache->back = g_hash_table_new (g_direct_hash, g_direct_equal);

  cache->mru = NULL;
  cache->mru_tail = NULL;
  cache->mru_count = 0;

  return cache;
}

static void
cache_entry_unref (PangoXFontCache *cache, CacheEntry *entry)
{
  if (g_atomic_int_dec_and_test (&entry->ref_count))
    {
      g_hash_table_remove (cache->forward, entry->xlfd);
      g_hash_table_remove (cache->back, entry->fs);

      free_cache_entry (NULL, entry, cache);
    }
}

/**
 * pango_x_font_cache_load:
 * @cache: a #PangoXFontCache
 * @xlfd: the X Logical Font Description to load.
 *
 * Loads a #XFontStruct from a X Logical Font Description. The
 * result may be newly loaded, or it may have been previously
 * stored.
 *
 * Return value: The font structure, or %NULL if the font could
 * not be loaded. In order to free this structure, you must call
 * pango_x_font_cache_unload().
 **/
XFontStruct *
pango_x_font_cache_load (PangoXFontCache *cache,
			 const char      *xlfd)
{
  CacheEntry *entry;

  g_return_val_if_fail (cache != NULL, NULL);
  g_return_val_if_fail (xlfd != NULL, NULL);

  entry = g_hash_table_lookup (cache->forward, xlfd);

  if (entry)
    {
      g_atomic_int_inc (&entry->ref_count);
    }
  else
    {
      XFontStruct *fs = XLoadQueryFont (cache->display, xlfd);

      if (!fs)
	return NULL;

      entry = g_slice_new (CacheEntry);

      entry->xlfd = g_strdup (xlfd);
      entry->fs = fs;

      entry->ref_count = 1;
      entry->mru = NULL;

      g_hash_table_insert (cache->forward, entry->xlfd, entry);
      g_hash_table_insert (cache->back, entry->fs, entry);
    }

  if (entry->mru)
    {
      if (cache->mru_count > 1 && entry->mru->prev)
	{
	  /* Move to the head of the mru list */

	  if (entry->mru == cache->mru_tail)
	    {
	      cache->mru_tail = cache->mru_tail->prev;
	      cache->mru_tail->next = NULL;
	    }
	  else
	    {
	      entry->mru->prev->next = entry->mru->next;
	      entry->mru->next->prev = entry->mru->prev;
	    }

	  entry->mru->next = cache->mru;
	  entry->mru->prev = NULL;
	  cache->mru->prev = entry->mru;
	  cache->mru = entry->mru;
	}
    }
  else
    {
      g_atomic_int_inc (&entry->ref_count);

      /* Insert into the mru list */

      if (cache->mru_count == CACHE_SIZE)
	{
	  CacheEntry *old_entry = cache->mru_tail->data;

	  cache->mru_tail = cache->mru_tail->prev;
	  cache->mru_tail->next = NULL;

	  g_list_free_1 (old_entry->mru);
	  old_entry->mru = NULL;
	  cache_entry_unref (cache, old_entry);
	}
      else
	cache->mru_count++;

      cache->mru = g_list_prepend (cache->mru, entry);
      if (!cache->mru_tail)
	cache->mru_tail = cache->mru;
      entry->mru = cache->mru;
    }

  return entry->fs;
}

/**
 * pango_x_font_cache_unload:
 * @cache: a #PangoXFontCache
 * @fs: the font structure to unload
 *
 * Frees a font structure previously loaded with pango_x_font_cache_load().
 **/
void
pango_x_font_cache_unload (PangoXFontCache *cache,
			   XFontStruct     *fs)
{
  CacheEntry *entry;

  g_return_if_fail (cache != NULL);
  g_return_if_fail (fs != NULL);

  entry = g_hash_table_lookup (cache->back, fs);
  g_return_if_fail (entry != NULL);

  cache_entry_unref (cache, entry);
}
