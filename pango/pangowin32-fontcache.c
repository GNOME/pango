/* Pango
 * pangowin32-fontcache.c: Cache of HFONTs by LOGFONTW
 *
 * Copyright (C) 2000 Red Hat Software
 * Copyright (C) 2000 Tor Lillqvist
 * Copyright (C) 2007 Novell, Inc.
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
#include <stdio.h>

#include "pangowin32-private.h"

/* Font cache
 */

/* Number of fonts to retain after they are not otherwise referenced.
 */
#define CACHE_SIZE 16

typedef struct _CacheEntry CacheEntry;

struct _PangoWin32FontCache
{
  GHashTable *forward;
  GHashTable *back;

  GList *mru;
  GList *mru_tail;
  int mru_count;
};

struct _CacheEntry
{
  LOGFONTW logfontw;
  HFONT hfont;

  gint ref_count;
  GList *mru;
};

static void
free_cache_entry (LOGFONTW            *logfont,
		  CacheEntry          *entry,
		  PangoWin32FontCache *cache)
{
  if (!DeleteObject (entry->hfont))
    PING (("DeleteObject for hfont %p failed", entry->hfont));

  g_slice_free (CacheEntry, entry);
}

/**
 * pango_win32_font_cache_free:
 * @cache: a #PangoWin32FontCache
 *
 * Frees a #PangoWin32FontCache and all associated memory. All fonts loaded
 * through this font cache will be freed along with the cache.
 **/
void
pango_win32_font_cache_free (PangoWin32FontCache *cache)
{
  g_return_if_fail (cache != NULL);

  g_hash_table_foreach (cache->forward, (GHFunc)free_cache_entry, cache);

  g_hash_table_destroy (cache->forward);
  g_hash_table_destroy (cache->back);

  g_list_free (cache->mru);

  g_slice_free (PangoWin32FontCache, cache);
}

static guint
wcs_hash (gconstpointer v)
{
  /* 31 bit hash function */
  const wchar_t *p = v;
  guint32 h = *p;

  if (h)
    for (p += 1; *p != '\0'; p++)
      h = (h << 5) - h + *p;

  return h;
}

static guint
logfontw_hash (gconstpointer v)
{
  const LOGFONTW *lfp = v;

  return wcs_hash (lfp->lfFaceName) +
    (lfp->lfItalic != 0) +
    lfp->lfWeight/10 +
    lfp->lfOrientation +
    abs (lfp->lfHeight) * 10;
}

static gint
logfontw_equal (gconstpointer v1,
		gconstpointer v2)
{
  const LOGFONTW *lfp1 = v1, *lfp2 = v2;

  return (wcscmp (lfp1->lfFaceName, lfp2->lfFaceName) == 0
	  && lfp1->lfPitchAndFamily == lfp2->lfPitchAndFamily
	  && lfp1->lfStrikeOut == lfp2->lfStrikeOut
	  && lfp1->lfUnderline == lfp2->lfUnderline
	  && (lfp1->lfItalic != 0) == (lfp2->lfItalic != 0)
	  && lfp1->lfWeight == lfp2->lfWeight
	  && lfp1->lfOrientation == lfp2->lfOrientation
	  && lfp1->lfEscapement == lfp2->lfEscapement
	  && lfp1->lfWidth == lfp2->lfWidth
	  && lfp1->lfHeight == lfp2->lfHeight);
}

/**
 * pango_win32_font_cache_new:
 *
 * Creates a font cache.
 *
 * Return value: The new font cache. This must be freed with
 * pango_win32_font_cache_free().
 **/
PangoWin32FontCache *
pango_win32_font_cache_new (void)
{
  PangoWin32FontCache *cache;

  cache = g_slice_new (PangoWin32FontCache);

  cache->forward = g_hash_table_new (logfontw_hash, logfontw_equal);
  cache->back = g_hash_table_new (g_direct_hash, g_direct_equal);

  cache->mru = NULL;
  cache->mru_tail = NULL;
  cache->mru_count = 0;

  return cache;
}

static void
cache_entry_unref (PangoWin32FontCache *cache,
		   CacheEntry          *entry)
{
  if (g_atomic_int_dec_and_test (&entry->ref_count))
    {
      PING (("removing cache entry %p", entry->hfont));

      g_hash_table_remove (cache->forward, &entry->logfontw);
      g_hash_table_remove (cache->back, entry->hfont);

      free_cache_entry (NULL, entry, cache);
    }
}

/**
 * pango_win32_font_cache_load:
 * @cache: a #PangoWin32FontCache
 * @logfont: a pointer to a LOGFONTA structure describing the font to load.
 *
 * Creates a HFONT from a LOGFONTA. The
 * result may be newly loaded, or it may have been previously
 * stored
 *
 * Return value: The font structure, or %NULL if the font could
 * not be loaded. In order to free this structure, you must call
 * pango_win32_font_cache_unload().
 **/
HFONT
pango_win32_font_cache_load (PangoWin32FontCache *cache,
			     const LOGFONTA      *lfp)
{
  LOGFONTW lf;

  /* We know that the lfFaceName is the last member in the structs */
  *(LOGFONTA *)&lf = *lfp;
  
  if (!MultiByteToWideChar (CP_ACP, MB_ERR_INVALID_CHARS,
			    lfp->lfFaceName, -1,
			    lf.lfFaceName, G_N_ELEMENTS (lf.lfFaceName)))
    return NULL;

  return pango_win32_font_cache_loadw (cache, &lf);
}

/**
 * pango_win32_font_cache_loadw:
 * @cache: a #PangoWin32FontCache
 * @logfont: a pointer to a LOGFONTW structure describing the font to load.
 *
 * Creates a HFONT from a LOGFONTW. The
 * result may be newly loaded, or it may have been previously
 * stored
 *
 * Return value: The font structure, or %NULL if the font could
 * not be loaded. In order to free this structure, you must call
 * pango_win32_font_cache_unload().
 *
 * Since: 1.16
 **/
HFONT
pango_win32_font_cache_loadw (PangoWin32FontCache *cache,
			      const LOGFONTW      *lfp)
{
  CacheEntry *entry;
  LOGFONTW lf;
  HFONT hfont;
  int tries;

  g_return_val_if_fail (cache != NULL, NULL);
  g_return_val_if_fail (lfp != NULL, NULL);

  entry = g_hash_table_lookup (cache->forward, lfp);

  if (entry)
    {
      g_atomic_int_inc (entry->ref_count);
      PING (("increased refcount for cache entry %p: %d", entry->hfont, entry->ref_count));
    }
  else
    {
      BOOL font_smoothing;
      lf = *lfp;
      SystemParametersInfo (SPI_GETFONTSMOOTHING, 0, &font_smoothing, 0);
      /* If on XP or better, try to use ClearType if the global system
       * settings ask for it.
       */
      if (font_smoothing &&
	  (_pango_win32_os_version_info.dwMajorVersion > 5 ||
	   (_pango_win32_os_version_info.dwMajorVersion == 5 &&
	    _pango_win32_os_version_info.dwMinorVersion >= 1)))
	{
	  UINT smoothing_type;

#ifndef SPI_GETFONTSMOOTHINGTYPE
#define SPI_GETFONTSMOOTHINGTYPE 0x200a
#endif
#ifndef FE_FONTSMOOTHINGCLEARTYPE
#define FE_FONTSMOOTHINGCLEARTYPE 2
#endif
#ifndef CLEARTYPE_QUALITY
#define CLEARTYPE_QUALITY 5
#endif
	  SystemParametersInfo (SPI_GETFONTSMOOTHINGTYPE, 0, &smoothing_type, 0);
	  lf.lfQuality =
	    (font_smoothing ?
	     (smoothing_type == FE_FONTSMOOTHINGCLEARTYPE ?
	      CLEARTYPE_QUALITY : ANTIALIASED_QUALITY) :
	     DEFAULT_QUALITY);
	}
      else
	lf.lfQuality = (font_smoothing ? ANTIALIASED_QUALITY : DEFAULT_QUALITY);
      lf.lfCharSet = DEFAULT_CHARSET;
      for (tries = 0; ; tries++)
	{
	  PING (("... trying CreateFontIndirect "
		 "height=%ld,width=%ld,escapement=%ld,orientation=%ld,"
		 "weight=%ld,%s%s%s"
		 "charset=%d,outprecision=%d,clipprecision=%d,"
		 "quality=%d,pitchandfamily=%#.02x,facename=\"%S\")",
		 lf.lfHeight, lf.lfWidth, lf.lfEscapement, lf.lfOrientation,
		 lf.lfWeight, (lf.lfItalic ? "italic," : ""),
		 (lf.lfUnderline ? "underline," : ""),
		 (lf.lfStrikeOut ? "strikeout," : ""),
		 lf.lfCharSet, lf.lfOutPrecision, lf.lfClipPrecision,
		 lf.lfQuality, lf.lfPitchAndFamily, lf.lfFaceName));
	  hfont = CreateFontIndirectW (&lf);

	  if (hfont != NULL)
	    {
	      PING (("Success! hfont=%p", hfont));
	      break;
	    }

	  /* If we fail, try some similar fonts often found on Windows. */
	  if (tries == 0)
	    {
	      gchar *p = g_utf16_to_utf8 (lf.lfFaceName, -1, NULL, NULL, NULL);
	      if (!p)
		; /* Nothing */
	      else if (g_ascii_strcasecmp (p, "helvetica") == 0)
		wcscpy (lf.lfFaceName, L"arial");
	      else if (g_ascii_strcasecmp (p, "new century schoolbook") == 0)
		wcscpy (lf.lfFaceName, L"century schoolbook");
	      else if (g_ascii_strcasecmp (p, "courier") == 0)
		wcscpy (lf.lfFaceName, L"courier new");
	      else if (g_ascii_strcasecmp (p, "lucida") == 0)
		wcscpy (lf.lfFaceName, L"lucida sans unicode");
	      else if (g_ascii_strcasecmp (p, "lucidatypewriter") == 0)
		wcscpy (lf.lfFaceName, L"lucida console");
	      else if (g_ascii_strcasecmp (p, "times") == 0)
		wcscpy (lf.lfFaceName, L"times new roman");
	      g_free (p);
	    }
	  else if (tries == 1)
	    {
	      gchar *p = g_utf16_to_utf8 (lf.lfFaceName, -1, NULL, NULL, NULL);
	      if (!p)
		; /* Nothing */
	      else if (g_ascii_strcasecmp (p, "courier") == 0)
		{
		  wcscpy (lf.lfFaceName, L"");
		  lf.lfPitchAndFamily |= FF_MODERN;
		}
	      else if (g_ascii_strcasecmp (p, "times new roman") == 0)
		{
		  wcscpy (lf.lfFaceName, L"");
		  lf.lfPitchAndFamily |= FF_ROMAN;
		}
	      else if (g_ascii_strcasecmp (p, "helvetica") == 0
		       || g_ascii_strcasecmp (p, "lucida") == 0)
		{
		  wcscpy (lf.lfFaceName, L"");
		  lf.lfPitchAndFamily |= FF_SWISS;
		}
	      else
		{
		  wcscpy (lf.lfFaceName, L"");
		  lf.lfPitchAndFamily = (lf.lfPitchAndFamily & 0x0F) | FF_DONTCARE;
		}
	      g_free (p);
	    }
	  else
	    break;
	  tries++;
	}

      if (!hfont)
	return NULL;

      entry = g_slice_new (CacheEntry);

      entry->logfontw = lf;
      entry->hfont = hfont;

      entry->ref_count = 1;
      entry->mru = NULL;

      g_hash_table_insert (cache->forward, &entry->logfontw, entry);
      g_hash_table_insert (cache->back, entry->hfont, entry);
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
      g_atomic_int_inc (entry->ref_count);

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

  return entry->hfont;
}

/**
 * pango_win32_font_cache_unload:
 * @cache: a #PangoWin32FontCache
 * @hfont: the HFONT to unload
 *
 * Frees a font structure previously loaded with pango_win32_font_cache_load().
 **/
void
pango_win32_font_cache_unload (PangoWin32FontCache *cache,
			       HFONT		    hfont)
{
  CacheEntry *entry;

  g_return_if_fail (cache != NULL);
  g_return_if_fail (hfont != NULL);

  entry = g_hash_table_lookup (cache->back, hfont);
  g_return_if_fail (entry != NULL);

  cache_entry_unref (cache, entry);
}
