/* Pango
 * pangoft2-fontcache.c: Cache of FreeType2 faces (FT_Face)
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

#include "pangoft2-private.h"

#include <string.h>

/* Font cache
 */

/* Number of fonts to retain after they are not otherwise referenced.
 */
#define CACHE_SIZE 3

typedef struct _CacheEntry CacheEntry;

struct _PangoFT2FontCache
{
  FT_Library  library;
  
  GHashTable *forward;
  GHashTable *back;

  GList *mru;
  GList *mru_tail;
  int mru_count;
};

struct _CacheEntry
{
  PangoFT2OA oa;
  FT_Face face;

  gint ref_count;
  GList *mru;
};

static void
free_cache_entry (PangoFT2OA        *oa,
		  CacheEntry        *entry,
		  PangoFT2FontCache *cache)
{
  FT_Error error;

  PING (("FT_Done_Face (%p)\n", entry->face));

  error = FT_Done_Face (entry->face);
  if (error != FT_Err_Ok)
    g_warning ("Error from FT_Done_Face: %s",
	       pango_ft2_ft_strerror (error));

  g_free (entry);
}

/**
 * pango_ft2_font_cache_free:
 * @cache: a #PangoFT2FontCache
 * 
 * Free a #PangoFT2FontCache and all associated memory. All fonts loaded
 * through this font cache will be freed along with the cache.
 **/
void
pango_ft2_font_cache_free (PangoFT2FontCache *cache)
{
  g_return_if_fail (cache != NULL);
  
  g_hash_table_foreach (cache->forward, (GHFunc)free_cache_entry, cache);
  
  g_hash_table_destroy (cache->forward);
  g_hash_table_destroy (cache->back);

  g_list_free (cache->mru);
}

static guint
oa_hash (gconstpointer v)
{
  PangoFT2OA *oa = (PangoFT2OA *) v;

  if (oa->open_args->flags & ft_open_memory)
    return (guint) oa->open_args->memory_base;
  else if (oa->open_args->flags == ft_open_pathname)
    return g_str_hash (oa->open_args->pathname);
  else if (oa->open_args->flags & ft_open_stream)
    return (guint) oa->open_args->stream;
  else
    return 0;
}

static gint
oa_equal (gconstpointer v1,
	  gconstpointer v2)
{
  PangoFT2OA *oa1 = (PangoFT2OA *) v1;
  PangoFT2OA *oa2 = (PangoFT2OA *) v2;

  if (oa1->open_args->flags != oa2->open_args->flags)
    return 0;
  else if (oa1->open_args->flags & ft_open_memory)
    return (oa1->open_args->memory_base == oa2->open_args->memory_base &&
	    oa1->face_index == oa2->face_index);
  else if (oa1->open_args->flags == ft_open_pathname)
    return (strcmp (oa1->open_args->pathname,
		    oa2->open_args->pathname) == 0 &&
	    oa1->face_index == oa2->face_index);
  else if (oa1->open_args->flags & ft_open_stream)
    return (oa1->open_args->stream == oa2->open_args->stream &&
	    oa1->face_index == oa2->face_index);
  else
    return 0;
}

/**
 * pango_ft2_font_cache_new:
 * 
 * Create a font cache.
 * 
 * Return value: The new font cache. This must be freed with
 * pango_ft2_font_cache_free().
 **/
PangoFT2FontCache *
pango_ft2_font_cache_new (FT_Library library)
{
  PangoFT2FontCache *cache;

  cache = g_new (PangoFT2FontCache, 1);

  cache->library = library;
  
  cache->forward = g_hash_table_new (oa_hash, oa_equal);
  cache->back = g_hash_table_new (g_direct_hash, g_direct_equal);
      
  cache->mru = NULL;
  cache->mru_tail = NULL;
  cache->mru_count = 0;
  
  return cache;
}

static void
cache_entry_unref (PangoFT2FontCache *cache,
		   CacheEntry        *entry)
{
  entry->ref_count--;
  PING (("face:%p ref_count:%d\n", entry->face, entry->ref_count));
  if (entry->ref_count == 0)
    {
      g_hash_table_remove (cache->forward, &entry->oa);
      g_hash_table_remove (cache->back, entry->face);

      free_cache_entry (NULL, entry, cache);
    }
}

/**
 * pango_ft2_font_cache_load:
 * @cache: a #PangoFT2FontCache
 * 
 * Load a #FT_Face from #FT_Open_Args and a face index. The
 * result may be newly loaded, or it may have been previously
 * stored
 * 
 * Return value: The #FT_Face, or %NULL if the font could
 * not be loaded. In order to free this structure, you must call
 * pango_ft2_font_cache_unload().
 **/
FT_Face
pango_ft2_font_cache_load (PangoFT2FontCache *cache,
			   FT_Open_Args      *args,
			   FT_Long            face_index)
{
  CacheEntry *entry;
  PangoFT2OA oa;

  g_return_val_if_fail (cache != NULL, NULL);
  g_return_val_if_fail (args != NULL, NULL);

  oa.open_args = args;
  oa.face_index = face_index;

  entry = g_hash_table_lookup (cache->forward, &oa);

  if (entry)
    entry->ref_count++;
  else
    {
      FT_Face face;
      FT_Error error;

      PING (("FT_Open_Face (%s,%ld)\n", args->pathname, face_index));

      error = FT_Open_Face (cache->library, args, face_index, &face);
      if (error != FT_Err_Ok)
	{
	  g_warning ("Error from FT_Open_Face: %s",
		     pango_ft2_ft_strerror (error));
	  return NULL;
	}
      
      PING (("  = %p\n", face));

      entry = g_new (CacheEntry, 1);

      entry->oa = oa;
      entry->face = face;

      entry->ref_count = 1;
      entry->mru = NULL;

      g_hash_table_insert (cache->forward, &entry->oa, entry);
      g_hash_table_insert (cache->back, entry->face, entry); 
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
      entry->ref_count++;
      
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

  return entry->face;
}

/**
 * pango_ft2_font_cache_unload:
 * @cache: a #PangoFT2FontCache
 * @face: the face to unload
 * 
 * Free a font structure previously loaded with pango_ft2_font_cache_load()
 **/
void
pango_ft2_font_cache_unload (PangoFT2FontCache *cache,
			     FT_Face            face)
{
  CacheEntry *entry;

  g_return_if_fail (cache != NULL);
  g_return_if_fail (face != NULL);

  entry = g_hash_table_lookup (cache->back, face);
  g_return_if_fail (entry != NULL);

  PING (("pango_ft2_font_cache_unload\n"));
  cache_entry_unref (cache, entry);  
}
