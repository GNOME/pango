/* Pango
 * pangoxft-fontmap.c: Xft font handling
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

#include "pango-fontmap.h"
#include "pangoxft.h"
#include "pangoxft-private.h"
#include "modules.h"

/* For XExtSetCloseDisplay */
#include <X11/Xlibint.h>

typedef struct _PangoXftFamily       PangoXftFamily;
typedef struct _PangoXftFontMap      PangoXftFontMap;

#define PANGO_TYPE_XFT_FONT_MAP              (pango_xft_font_map_get_type ())
#define PANGO_XFT_FONT_MAP(object)           (G_TYPE_CHECK_INSTANCE_CAST ((object), PANGO_TYPE_XFT_FONT_MAP, PangoXftFontMap))
#define PANGO_XFT_IS_FONT_MAP(object)        (G_TYPE_CHECK_INSTANCE_TYPE ((object), PANGO_TYPE_XFT_FONT_MAP))

struct _PangoXftFontMap
{
  PangoFontMap parent_instance;

  /* We have one map from  PangoFontDescription -> PangoXftPatternSet
   * per language tag.
   */
  GList *fontset_hash_list; 
  /* pattern_hash is used to make sure we only store one copy of
   * each identical pattern. (Speeds up lookup).
   */
  GHashTable *pattern_hash; 
  GHashTable *coverage_hash; /* Maps font file name/id -> PangoCoverage */

  GHashTable *fonts; /* Maps XftPattern -> PangoXftFont */
  GQueue *freed_fonts; /* Fonts in fonts that has been freed */

  /* List of all families availible */
  PangoXftFamily **families;
  int n_families;		/* -1 == uninitialized */

  Display *display;
  int screen;
  guint closed : 1;
  
  /* Function to call on prepared patterns to do final
   * config tweaking.
   */
  PangoXftSubstituteFunc substitute_func;
  gpointer substitute_data;
  GDestroyNotify substitute_destroy;
};

/************************************************************
 *              Code shared with PangoFT2                   *
 ************************************************************/

#define PangoFcFamily      PangoXftFamily
#define _PangoFcFamily      _PangoXftFamily
#define PangoFcFontMap     PangoXftFontMap
#define PangoFcFont     PangoXftFont

#define PANGO_FC_FONT_MAP PANGO_XFT_FONT_MAP

#define pango_fc_font_map_get_type pango_xft_font_map_get_type
#define _pango_fc_font_map_add _pango_xft_font_map_add
#define _pango_fc_font_map_remove _pango_xft_font_map_remove
#define _pango_fc_font_map_cache_add _pango_xft_font_map_cache_add
#define _pango_fc_font_map_cache_remove _pango_xft_font_map_cache_remove
#define _pango_fc_font_map_get_coverage _pango_xft_font_map_get_coverage
#define _pango_fc_font_map_set_coverage _pango_xft_font_map_set_coverage
#define _pango_fc_font_desc_from_pattern _pango_xft_font_desc_from_pattern
#define _pango_fc_font_new               _pango_xft_font_new

#define PANGO_FC_NAME "PangoXft"

#include "pangofc-fontmap.cI"

/*************************************************************
 *                      Xft specific code                    *
 *************************************************************/

static PangoFontMap *
pango_xft_find_font_map (Display *display,
			 int      screen)
{
  GSList *tmp_list;
  
  tmp_list = fontmaps;
  while (tmp_list)
    {
      PangoXftFontMap *xftfontmap = tmp_list->data;

      if (xftfontmap->display == display &&
	  xftfontmap->screen == screen) 
	return PANGO_FONT_MAP (xftfontmap);

      tmp_list = tmp_list->next;
    }

  return NULL;
}

/*
 * Hackery to set up notification when a Display is closed
 */
static GSList *registered_displays;

static int
close_display_cb (Display   *display,
		  XExtCodes *extcodes)
{
  GSList *tmp_list;
  
  tmp_list = fontmaps;
  while (tmp_list)
    {
      PangoXftFontMap *xftfontmap = tmp_list->data;
      tmp_list = tmp_list->next;

      if (xftfontmap->display == display)
	pango_xft_shutdown_display (display, xftfontmap->screen);
    }

  registered_displays = g_slist_remove (registered_displays, display);

  return 0;
}

static void
register_display (Display *display)
{
  XExtCodes *extcodes;
  GSList *tmp_list;

  for (tmp_list = registered_displays; tmp_list; tmp_list = tmp_list->next)
    {
      if (tmp_list->data == display)
	return;
    }

  registered_displays = g_slist_prepend (registered_displays, display);
    
  extcodes = XAddExtension (display);
  XESetCloseDisplay (display, extcodes->extension, close_display_cb);
}

/**
 * pango_xft_get_font_map:
 * @display: an X display
 * @screen: the screen number of a screen within @display
 * 
 * Returns the PangoXftFontmap for the given display and screen.
 * The fontmap is owned by Pango and will be valid until
 * the display is closed.
 * 
 * Return value: a #PangoFontMap object, owned by Pango.
 **/
PangoFontMap *
pango_xft_get_font_map (Display *display,
			int      screen)
{
  static gboolean registered_modules = FALSE;
  PangoFontMap *fontmap;
  PangoXftFontMap *xftfontmap;

  g_return_val_if_fail (display != NULL, NULL);
  
  if (!registered_modules)
    {
      int i;
      
      registered_modules = TRUE;
      
      /* Make sure that the type system is initialized */
      g_type_init ();
  
      for (i = 0; _pango_included_xft_modules[i].list; i++)
        pango_module_register (&_pango_included_xft_modules[i]);
    }

  fontmap = pango_xft_find_font_map (display, screen);
  if (fontmap)
    return fontmap;
  
  xftfontmap = (PangoXftFontMap *)g_object_new (PANGO_TYPE_XFT_FONT_MAP, NULL);
  
  xftfontmap->display = display;
  xftfontmap->screen = screen;

  register_display (display);

  fontmaps = g_slist_prepend (fontmaps, xftfontmap);

  return PANGO_FONT_MAP (xftfontmap);
}

static void
cleanup_font (gpointer         key,
	      PangoXftFont    *xfont,
	      PangoXftFontMap *xftfontmap)
{
  if (xfont->xft_font)
    XftFontClose (xftfontmap->display, xfont->xft_font);
  
  xfont->fontmap = NULL;
}

/**
 * pango_xft_shutdown_display:
 * @display: an X display
 * @screen: the screen number of a screen within @display
 * 
 * Release any resources that have been cached for the
 * combination of @display and @screen.
 **/
void
pango_xft_shutdown_display (Display *display,
			    int      screen)
{
  PangoFontMap *fontmap;
  
  fontmap = pango_xft_find_font_map (display, screen);
  if (fontmap)
    {
      PangoXftFontMap *xftfontmap = PANGO_XFT_FONT_MAP (fontmap);
	    
      fontmaps = g_slist_remove (fontmaps, fontmap);
      pango_fc_font_map_cache_clear (xftfontmap);
      
      g_hash_table_foreach (xftfontmap->fonts, (GHFunc)cleanup_font, fontmap);
      g_hash_table_destroy (xftfontmap->fonts);
      xftfontmap->fonts = NULL;
      
      xftfontmap->display = NULL;
      xftfontmap->closed = TRUE;
      g_object_unref (fontmap);
    }
}  

/**
 * pango_xft_font_map_set_default_substitute:
 * @display: an X Display
 * @screen: the screen number of a screen within @display
 * @func: function to call to to do final config tweaking
 *        on #FcPattern objects.
 * @data: data to pass to @func
 * @notify: function to call when @data is no longer used.
 * 
 * Sets a function that will be called to do final configuration
 * substitution on a #FcPattern before it is used to load
 * the font. This function can be used to do things like set
 * hinting and antiasing options.
 **/
void
pango_xft_set_default_substitute (Display                *display,
				  int                     screen,
				  PangoXftSubstituteFunc  func,
				  gpointer                data,
				  GDestroyNotify          notify)
{
  PangoXftFontMap *xftfontmap = (PangoXftFontMap *)pango_xft_get_font_map (display, screen);
  
  if (xftfontmap->substitute_destroy)
    xftfontmap->substitute_destroy (xftfontmap->substitute_data);

  xftfontmap->substitute_func = func;
  xftfontmap->substitute_data = data;
  xftfontmap->substitute_destroy = notify;
  
  pango_fc_clear_fontset_hash_list (xftfontmap);
}

/**
 * pango_substitute_changed:
 * @fontmap: a #PangoXftFontmap
 * 
 * Call this function any time the results of the
 * default substitution function set with
 * pango_xft_font_map_set_default_substitute() change.
 * That is, if your subsitution function will return different
 * results for the same input pattern, you must call this function.
 **/
void
pango_xft_substitute_changed (Display *display,
			      int      screen)
{
  PangoXftFontMap *xftfontmap = (PangoXftFontMap *)pango_xft_get_font_map (display, screen);
  
  pango_fc_clear_fontset_hash_list (xftfontmap);
}

void
_pango_xft_font_map_get_info (PangoFontMap *fontmap,
			      Display     **display,
			      int          *screen)
{
  PangoXftFontMap *xftfontmap = (PangoXftFontMap *)fontmap;

  if (display)
    *display = xftfontmap->display;
  if (screen)
    *screen = xftfontmap->screen;
}

/**
 * pango_xft_get_context:
 * @display: an X display.
 * @screen: an X screen.
 *
 * Retrieves a #PangoContext appropriate for rendering with
 * Xft fonts on the given screen of the given display. 
 *
 * Return value: the new #PangoContext. 
 **/
PangoContext *
pango_xft_get_context (Display *display,
		       int      screen)
{
  PangoContext *result;

  g_return_val_if_fail (display != NULL, NULL);

  result = pango_context_new ();
  pango_context_set_font_map (result, pango_xft_get_font_map (display, screen));

  return result;
}

static void
pango_fc_do_finalize (PangoXftFontMap *fontmap)
{
}

static void
pango_fc_default_substitute (PangoXftFontMap *fontmap,
			     FcPattern       *pattern)
{
  FcConfigSubstitute (NULL, pattern, FcMatchPattern);
  if (fontmap->substitute_func)
    fontmap->substitute_func (pattern, fontmap->substitute_data);
  XftDefaultSubstitute (fontmap->display, fontmap->screen, pattern);
}
