/* Pango
 * pangoft2-fontmap.c:
 *
 * Copyright (C) 2000 Red Hat Software
 * Copyright (C) 2000 Tor Lillqvist
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

#include <glib.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <errno.h>
#ifdef HAVE_DIRENT_H
#include <dirent.h>
#endif

#include <fontconfig/fontconfig.h>

#include "pango-fontmap.h"
#include "pango-utils.h"
#include "pangoft2-private.h"
#include "modules.h"

#ifdef G_OS_WIN32
#define STRICT
#include <windows.h>
#endif

typedef struct _PangoFT2Family       PangoFT2Family;

struct _PangoFT2FontMap
{
  PangoFontMap parent_instance;

  FT_Library library;

  /* We have one map from  PangoFontDescription -> PangoXftPatternSet
   * per language tag.
   */
  GList *fontset_hash_list;
  /* pattern_hash is used to make sure we only store one copy of
   * each identical pattern. (Speeds up lookup).
   */
  GHashTable *pattern_hash; 
  GHashTable *coverage_hash; /* Maps font file name -> PangoCoverage */

  GHashTable *fonts; /* Maps XftPattern -> PangoFT2Font */
	
  GQueue *fontset_cache;	/* Recently used fontsets */

  /* List of all families availible */
  PangoFT2Family **families;
  int n_families;		/* -1 == uninitialized */

  double dpi_x;
  double dpi_y;

  guint closed : 1;

  /* Function to call on prepared patterns to do final
   * config tweaking.
   */
  PangoFT2SubstituteFunc substitute_func;
  gpointer substitute_data;
  GDestroyNotify substitute_destroy;
};

/************************************************************
 *              Code shared with PangoXft                   *
 ************************************************************/

#define PangoFcFamily      PangoFT2Family
#define _PangoFcFamily      _PangoFT2Family
#define PangoFcFontMap     PangoFT2FontMap
#define PangoFcFont     PangoFT2Font

#define PANGO_FC_FONT_MAP PANGO_FT2_FONT_MAP

#define pango_fc_font_map_get_type pango_ft2_font_map_get_type
#define _pango_fc_font_map_add _pango_ft2_font_map_add
#define _pango_fc_font_map_remove _pango_ft2_font_map_remove
#define _pango_fc_font_map_get_coverage _pango_ft2_font_map_get_coverage
#define _pango_fc_font_map_set_coverage _pango_ft2_font_map_set_coverage
#define _pango_fc_font_desc_from_pattern _pango_ft2_font_desc_from_pattern
#define _pango_fc_font_new               _pango_ft2_font_new

#define PANGO_FC_NAME "PangoFT2"

#include "pangofc-fontmap.cI"

/*************************************************************
 *                  FreeType specific code                   *
 *************************************************************/

static PangoFT2FontMap *pango_ft2_global_fontmap = NULL;

/**
 * pango_ft2_font_map_new:
 * 
 * Create a new #PangoFT2FontMap object; a fontmap is used
 * to cache information about available fonts, and holds
 * certain global parameters such as the resolution and
 * the default substitute function (see
 * pango_ft2_font_map_set_default_substitute()).
 * 
 * Return value: the newly created fontmap object. Unref
 * with g_object_unref() when you are finished with it.
 *
 * Since: 1.2
 **/
PangoFontMap *
pango_ft2_font_map_new (void)
{
  static gboolean registered_modules = FALSE;
  PangoFT2FontMap *ft2fontmap;
  FT_Error error;
  
  if (!registered_modules)
    {
      int i;
      
      registered_modules = TRUE;
      
      /* Make sure that the type system is initialized */
      g_type_init ();
  
      for (i = 0; _pango_included_ft2_modules[i].list; i++)
        pango_module_register (&_pango_included_ft2_modules[i]);
    }

  ft2fontmap = g_object_new (PANGO_TYPE_FT2_FONT_MAP, NULL);
  
  error = FT_Init_FreeType (&ft2fontmap->library);
  if (error != FT_Err_Ok)
    {
      g_warning ("Error from FT_Init_FreeType: %s",
		 _pango_ft2_ft_strerror (error));
      return NULL;
    }

  return (PangoFontMap *)ft2fontmap;
}

/**
 * pango_ft2_font_map_set_default_substitute:
 * @fontmap: a #PangoFT2FontMap
 * @func: function to call to to do final config tweaking
 *        on #FcPattern objects.
 * @data: data to pass to @func
 * @notify: function to call when @data is no longer used.
 * 
 * Sets a function that will be called to do final configuration
 * substitution on a #FcPattern before it is used to load
 * the font. This function can be used to do things like set
 * hinting and antialiasing options.
 *
 * Since: 1.2
 **/
void
pango_ft2_font_map_set_default_substitute (PangoFT2FontMap        *fontmap,
					   PangoFT2SubstituteFunc  func,
					   gpointer                data,
					   GDestroyNotify          notify)
{
  if (fontmap->substitute_destroy)
    fontmap->substitute_destroy (fontmap->substitute_data);

  fontmap->substitute_func = func;
  fontmap->substitute_data = data;
  fontmap->substitute_destroy = notify;
  
  pango_fc_font_map_cache_clear (fontmap);
}

/**
 * pango_ft2_font_map_substitute_changed:
 * @fontmap: a #PangoFT2Fontmap
 * 
 * Call this function any time the results of the
 * default substitution function set with
 * pango_ft2_font_map_set_default_substitute() change.
 * That is, if your subsitution function will return different
 * results for the same input pattern, you must call this function.
 *
 * Since: 1.2
 **/
void
pango_ft2_font_map_substitute_changed (PangoFT2FontMap *fontmap)
{
  pango_fc_font_map_cache_clear (fontmap);
}

/**
 * pango_ft2_font_map_set_resolution:
 * @fontmap: a #PangoFT2Fontmap 
 * @dpi_x: dots per inch in the X direction
 * @dpi_y: dots per inch in the Y direction
 * 
 * Sets the horizontal and vertical resolutions for the fontmap.
 **/
void
pango_ft2_font_map_set_resolution (PangoFT2FontMap *fontmap,
				   double           dpi_x,
				   double           dpi_y)
{
  g_return_if_fail (PANGO_FT2_IS_FONT_MAP (fontmap));
  
  fontmap->dpi_x = dpi_x;
  fontmap->dpi_y = dpi_y;

  pango_ft2_font_map_substitute_changed (fontmap);
}

/**
 * pango_ft2_font_map_create_context:
 * @fontmap: a #PangoFT2Fontmap
 * 
 * Create a #PangoContext for the given fontmap.
 * 
 * Return value: the newly created context; free with g_object_unref().
 *
 * Since: 1.2
 **/
PangoContext *
pango_ft2_font_map_create_context (PangoFT2FontMap *fontmap)
{
  PangoContext *context;

  g_return_val_if_fail (PANGO_FT2_IS_FONT_MAP (fontmap), NULL);
  
  context = pango_context_new ();
  pango_context_set_font_map (context, PANGO_FONT_MAP (fontmap));

  return context;
}

/**
 * pango_ft2_get_context:
 * @dpi_x:  the horizontal dpi of the target device
 * @dpi_y:  the vertical dpi of the target device
 * 
 * Retrieves a #PangoContext for the default PangoFT2 fontmap
 * (see pango_ft2_fontmap_get_for_display()) and sets the resolution
 * for the default fontmap to @dpi_x by @dpi_y.
 *
 * Use of this function is discouraged, see pango_ft2_fontmap_create_context()
 * instead.
 * 
 * Return value: the new #PangoContext
 **/
PangoContext *
pango_ft2_get_context (double dpi_x, double dpi_y)
{
  PangoFontMap *fontmap;
  
  fontmap = pango_ft2_font_map_for_display ();
  pango_ft2_font_map_set_resolution (PANGO_FT2_FONT_MAP (fontmap), dpi_x, dpi_y);

  return pango_ft2_font_map_create_context (PANGO_FT2_FONT_MAP (fontmap));
}

/**
 * pango_ft2_font_map_for_display:
 *
 * Returns a #PangoFT2FontMap. This font map is cached and should
 * not be freed. If the font map is no longer needed, it can
 * be released with pango_ft2_shutdown_display().
 *
 * Returns: a #PangoFT2FontMap.
 **/
PangoFontMap *
pango_ft2_font_map_for_display (void)
{
  if (pango_ft2_global_fontmap != NULL)
    return PANGO_FONT_MAP (pango_ft2_global_fontmap);

  pango_ft2_global_fontmap = (PangoFT2FontMap *)pango_ft2_font_map_new ();

  return PANGO_FONT_MAP (pango_ft2_global_fontmap);
}

/**
 * pango_ft2_shutdown_display:
 * 
 * Free the global fontmap. (See pango_ft2_font_map_for_display())
 **/
void
pango_ft2_shutdown_display (void)
{
  if (pango_ft2_global_fontmap)
    {
      pango_fc_font_map_cache_clear (pango_ft2_global_fontmap);
      
      g_object_unref (pango_ft2_global_fontmap);
      
      pango_ft2_global_fontmap = NULL;
    }
}

FT_Library
_pango_ft2_font_map_get_library (PangoFontMap *fontmap)
{
  PangoFT2FontMap *ft2fontmap = (PangoFT2FontMap *)fontmap;
  
  return ft2fontmap->library;
}

static void
pango_fc_do_finalize (PangoFT2FontMap    *fontmap)
{
  if (fontmap->substitute_destroy)
    fontmap->substitute_destroy (fontmap->substitute_data);

  FT_Done_FreeType (fontmap->library);
}

static void
pango_fc_default_substitute (PangoFT2FontMap *fontmap,
			     FcPattern       *pattern)
{
  FcValue v;
  
  FcConfigSubstitute (NULL, pattern, FcMatchPattern);

  if (fontmap->substitute_func)
    fontmap->substitute_func (pattern, fontmap->substitute_data);

  if (FcPatternGet (pattern, FC_DPI, 0, &v) == FcResultNoMatch)
    FcPatternAddDouble (pattern, FC_DPI, fontmap->dpi_y);
  FcDefaultSubstitute (pattern);
}
