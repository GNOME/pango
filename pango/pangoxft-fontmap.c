/* Pango
 * pangoxft-fontmap.h: Font handling
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

#include "pango-fontmap.h"
#include "pangoxft.h"
#include "pangoxft-private.h"

#include "X11/Xft/XftFreetype.h"

#define PANGO_TYPE_XFT_FONT_MAP              (pango_xft_font_map_get_type ())
#define PANGO_XFT_FONT_MAP(object)           (G_TYPE_CHECK_INSTANCE_CAST ((object), PANGO_TYPE_XFT_FONT_MAP, PangoXftFontMap))
#define PANGO_XFT_FONT_MAP_CLASS(klass)      (G_TYPE_CHECK_CLASS_CAST ((klass), PANGO_TYPE_XFT_FONT_MAP, PangoXftFontMapClass))
#define PANGO_XFT_IS_FONT_MAP(object)        (G_TYPE_CHECK_INSTANCE_TYPE ((object), PANGO_TYPE_XFT_FONT_MAP))
#define PANGO_XFT_IS_FONT_MAP_CLASS(klass)   (G_TYPE_CHECK_CLASS_TYPE ((klass), PANGO_TYPE_XFT_FONT_MAP))
#define PANGO_XFT_FONT_MAP_GET_CLASS(obj)    (G_TYPE_INSTANCE_GET_CLASS ((obj), PANGO_TYPE_XFT_FONT_MAP, PangoXftFontMapClass))

/* Number of freed fonts */
#define MAX_FREED_FONTS 16

typedef struct _PangoXftFontMap      PangoXftFontMap;
typedef struct _PangoXftFontMapClass PangoXftFontMapClass;

struct _PangoXftFontMap
{
  PangoFontMap parent_instance;

  GHashTable *font_hash;
  GHashTable *coverage_hash;
  GQueue *freed_fonts;

  Display *display;
  int screen;  
};

struct _PangoXftFontMapClass
{
  PangoFontMapClass parent_class;
};

static GType    pango_xft_font_map_get_type   (void);
static void     pango_xft_font_map_init       (PangoXftFontMap      *fontmap);
static void     pango_xft_font_map_class_init (PangoXftFontMapClass *class);

static void       pango_xft_font_map_finalize      (GObject                      *object);
static PangoFont *pango_xft_font_map_load_font     (PangoFontMap                 *fontmap,
						  const PangoFontDescription   *description);
static void       pango_xft_font_map_list_fonts    (PangoFontMap                 *fontmap,
						  const gchar                  *family,
						  PangoFontDescription       ***descs,
						  int                          *n_descs);
static void       pango_xft_font_map_list_families (PangoFontMap                 *fontmap,
						  gchar                      ***families,
						  int                          *n_families);

static void pango_xft_font_map_cache_clear  (PangoXftFontMap *xfontmap);
static void pango_xft_font_map_cache_remove (PangoFontMap    *fontmap,
					     PangoXftFont    *xfont);

static PangoFontClass *parent_class;	/* Parent class structure for PangoXftFontMap */

static GType
pango_xft_font_map_get_type (void)
{
  static GType object_type = 0;

  if (!object_type)
    {
      static const GTypeInfo object_info =
      {
        sizeof (PangoXftFontMapClass),
        (GBaseInitFunc) NULL,
        (GBaseFinalizeFunc) NULL,
        (GClassInitFunc) pango_xft_font_map_class_init,
        NULL,           /* class_finalize */
        NULL,           /* class_data */
        sizeof (PangoXftFontMap),
        0,              /* n_preallocs */
        (GInstanceInitFunc) pango_xft_font_map_init,
      };
      
      object_type = g_type_register_static (PANGO_TYPE_FONT_MAP,
                                            "PangoXftFontMap",
                                            &object_info, 0);
    }
  
  return object_type;
}

static void 
pango_xft_font_map_init (PangoXftFontMap *xfontmap)
{
}

static void
pango_xft_font_map_class_init (PangoXftFontMapClass *class)
{
  GObjectClass *object_class = G_OBJECT_CLASS (class);
  PangoFontMapClass *font_map_class = PANGO_FONT_MAP_CLASS (class);
  
  parent_class = g_type_class_peek_parent (class);
  
  object_class->finalize = pango_xft_font_map_finalize;
  font_map_class->load_font = pango_xft_font_map_load_font;
  font_map_class->list_fonts = pango_xft_font_map_list_fonts;
  font_map_class->list_families = pango_xft_font_map_list_families;
}

static GSList *fontmaps = NULL;

static guint
font_description_hash (const PangoFontDescription *desc)
{
  guint hash = 0;

  if (desc->family_name)
    hash =  g_str_hash (desc->family_name);
  
  hash ^= desc->size;
  hash ^= desc->style << 16;
  hash ^= desc->variant << 18;
  hash ^= desc->weight << 16;
  hash ^= desc->stretch << 26;

  return hash;
}

static PangoFontMap *
pango_xft_get_font_map (Display *display,
			int      screen)
{
  PangoXftFontMap *xfontmap;
  GSList *tmp_list = fontmaps;

  g_return_val_if_fail (display != NULL, NULL);
  
  /* Make sure that the type system is initialized */
  g_type_init();
  
  while (tmp_list)
    {
      xfontmap = tmp_list->data;

      if (xfontmap->display == display &&
	  xfontmap->screen == screen) 
	return PANGO_FONT_MAP (xfontmap);
    }

  xfontmap = (PangoXftFontMap *)g_object_new (PANGO_TYPE_XFT_FONT_MAP, NULL);
  
  xfontmap->display = display;
  xfontmap->screen = screen;

  xfontmap->font_hash = g_hash_table_new ((GHashFunc)font_description_hash,
					  (GEqualFunc)pango_font_description_equal);
  xfontmap->coverage_hash = g_hash_table_new (g_str_hash, g_str_equal);
  xfontmap->freed_fonts = g_queue_new ();

  fontmaps = g_slist_prepend (fontmaps, xfontmap);

  return PANGO_FONT_MAP (xfontmap);
}

PangoContext *
pango_xft_get_context (Display *display,
		       int      screen)
{
  PangoContext *result;

  g_return_val_if_fail (display != NULL, NULL);

  result = pango_context_new ();
  pango_context_add_font_map (result, pango_xft_get_font_map (display, screen));

  return result;
}

static void
coverage_foreach (gpointer key, gpointer value, gpointer data)
{
  PangoCoverage *coverage = value;

  g_free (key);
  pango_coverage_unref (coverage);
}

static void
pango_xft_font_map_finalize (GObject *object)
{
  PangoXftFontMap *xfontmap = PANGO_XFT_FONT_MAP (object);

  fontmaps = g_slist_remove (fontmaps, object);

  g_queue_free (xfontmap->freed_fonts);
  g_hash_table_destroy (xfontmap->font_hash);

  g_hash_table_foreach (xfontmap->coverage_hash, coverage_foreach, NULL);
  g_hash_table_destroy (xfontmap->coverage_hash);
  
  G_OBJECT_CLASS (parent_class)->finalize (object);
}

void
_pango_xft_font_map_add (PangoFontMap *fontmap,
			PangoXftFont *xfont)
{
  PangoXftFontMap *xfontmap = PANGO_XFT_FONT_MAP (fontmap);

  g_hash_table_insert (xfontmap->font_hash, xfont->description, xfont);
}

void
_pango_xft_font_map_remove (PangoFontMap *fontmap,
			   PangoXftFont *xfont)
{
  PangoXftFontMap *xfontmap = PANGO_XFT_FONT_MAP (fontmap);

  g_hash_table_remove (xfontmap->font_hash, xfont->description);
}

static PangoFontDescription *
font_desc_from_pattern (XftPattern *pattern)
{
  PangoFontDescription *desc;
  char *s;
  int i;

  desc = g_new (PangoFontDescription, 1);

  g_assert (XftPatternGetString (pattern, XFT_FAMILY, 0, &s) == XftResultMatch);

  desc->family_name = g_strdup (s);
  
  if (XftPatternGetInteger (pattern, XFT_SLANT, 0, &i) == XftResultMatch)
    {
      if (i == XFT_SLANT_ROMAN)
	desc->style = PANGO_STYLE_NORMAL;
      else if (i == XFT_SLANT_OBLIQUE)
	desc->style = PANGO_STYLE_OBLIQUE;
      else
	desc->style = PANGO_STYLE_ITALIC;
    }
  else
    desc->style = PANGO_STYLE_NORMAL;
  
  if (XftPatternGetInteger (pattern, XFT_WEIGHT, 0, &i) == XftResultMatch)
    {
      if (i < XFT_WEIGHT_LIGHT)
	desc->weight = PANGO_WEIGHT_ULTRALIGHT;
      else if (i < (XFT_WEIGHT_LIGHT + XFT_WEIGHT_MEDIUM) / 2)
	desc->weight = PANGO_WEIGHT_LIGHT;
      else if (i < (XFT_WEIGHT_MEDIUM + XFT_WEIGHT_DEMIBOLD) / 2)
	desc->weight = PANGO_WEIGHT_NORMAL;
      else if (i < (XFT_WEIGHT_DEMIBOLD + XFT_WEIGHT_BOLD) / 2)
	desc->weight = 600;
      else if (i < (XFT_WEIGHT_BOLD + XFT_WEIGHT_BLACK) / 2)
	desc->weight = PANGO_WEIGHT_BOLD;
      else
	desc->weight = PANGO_WEIGHT_ULTRABOLD;
    }

  desc->variant = PANGO_VARIANT_NORMAL;
  desc->stretch = PANGO_STRETCH_NORMAL;
  desc->size = -1;

  return desc;
}


static void
pango_xft_font_map_list_fonts (PangoFontMap           *fontmap,
			       const gchar            *family,
			       PangoFontDescription ***descs,
			       int                    *n_descs)
{
  PangoXftFontMap *xfontmap = PANGO_XFT_FONT_MAP (fontmap);
  XftFontSet *fontset;

  if (family)
    fontset = XftListFonts (xfontmap->display, xfontmap->screen,
			    XFT_ENCODING, XftTypeString, "iso10646-1",
			    XFT_FAMILY, XftTypeString, family,
			    XFT_CORE, XftTypeBool, False,
			    NULL,
			    XFT_FAMILY,
			    XFT_STYLE,
			    XFT_WEIGHT,
			    XFT_SLANT,
			    NULL);
  else
    fontset = XftListFonts (xfontmap->display, xfontmap->screen,
			    XFT_ENCODING, XftTypeString, "iso10646-1",
			    XFT_CORE, XftTypeBool, False,
			    NULL,
			    XFT_FAMILY,
			    XFT_STYLE,
			    XFT_WEIGHT,
			    XFT_SLANT,
			    NULL);

  if (n_descs)
    *n_descs = fontset->nfont;

  if (descs)
    {
      gint i;
      
      *descs = g_new (PangoFontDescription *, fontset->nfont);

      for (i = 0; i < fontset->nfont; i++)
	(*descs)[i] = font_desc_from_pattern (fontset->fonts[i]);
    }

  XftFontSetDestroy (fontset);
}

static void
pango_xft_font_map_list_families (PangoFontMap           *fontmap,
				  gchar                ***families,
				  int                    *n_families)
{
  PangoXftFontMap *xfontmap = PANGO_XFT_FONT_MAP (fontmap);
  XftFontSet *fontset;
  int i;

  fontset = XftListFonts (xfontmap->display, xfontmap->screen,
			  XFT_CORE, XftTypeBool, False,
			  XFT_ENCODING, XftTypeString, "iso10646-1",
			  NULL,
			  XFT_FAMILY,
			  NULL);

  if (n_families)
    *n_families = fontset->nfont;

  if (families)
    {
      *families = g_new (gchar *, fontset->nfont);

      for (i = 0; i < fontset->nfont; i++)
	{
	  char *s;
	  XftResult res;
	  
	  res = XftPatternGetString (fontset->fonts[i], XFT_FAMILY, 0, &s);
	  g_assert (res == XftResultMatch);

	  (*families)[i] = g_strdup (s);
	}
    }

  XftFontSetDestroy (fontset);
}

static PangoFont *
pango_xft_font_map_load_font (PangoFontMap               *fontmap,
			      const PangoFontDescription *description)
{
  PangoXftFontMap *xfontmap = (PangoXftFontMap *)fontmap;
  PangoXftFont *font;
  int slant;
  int weight;
  XftFont *xft_font;

  font = g_hash_table_lookup (xfontmap->font_hash, description);

  if (font)
    {
      if (font->in_cache)
	pango_xft_font_map_cache_remove (fontmap, font);
      
      return (PangoFont *)g_object_ref (G_OBJECT (font));
    }

  if (description->style == PANGO_STYLE_ITALIC)
    slant = XFT_SLANT_ITALIC;
  else if (description->style == PANGO_STYLE_OBLIQUE)
    slant = XFT_SLANT_OBLIQUE;
  else
    slant = XFT_SLANT_ROMAN;

  if (description->weight < (PANGO_WEIGHT_NORMAL + PANGO_WEIGHT_LIGHT) / 2)
    weight = XFT_WEIGHT_LIGHT;
  else if (description->weight < (PANGO_WEIGHT_NORMAL + 600) / 2)
    weight = XFT_WEIGHT_MEDIUM;
  else if (description->weight < (600 + PANGO_WEIGHT_BOLD) / 2)
    weight = XFT_WEIGHT_DEMIBOLD;
  else if (description->weight < (PANGO_WEIGHT_BOLD + PANGO_WEIGHT_ULTRABOLD) / 2)
    weight = XFT_WEIGHT_BOLD;
  else
    weight = XFT_WEIGHT_BLACK;

  /* To fool Xft into not munging glyph indices, we open it as glyphs-fontspecific
   * then set the encoding ourself
   */
  xft_font = XftFontOpen (xfontmap->display, xfontmap->screen,
			  XFT_ENCODING, XftTypeString, "glyphs-fontspecific",
			  XFT_CORE, XftTypeBool, False,
			  XFT_FAMILY, XftTypeString,  description->family_name,
			  XFT_WEIGHT, XftTypeInteger, weight,
			  XFT_SLANT,  XftTypeInteger, slant,
			  XFT_SIZE, XftTypeDouble, (double)description->size/PANGO_SCALE,
			  NULL);

  if (xft_font)
    {
      FT_Face face;
      FT_Error error;
      
      int charmap;

      g_assert (!xft_font->core);
      
      face = xft_font->u.ft.font->face;

      for (charmap = 0; charmap < face->num_charmaps; charmap++)
	if (face->charmaps[charmap]->encoding == ft_encoding_unicode)
	  break;

      if (charmap == face->num_charmaps)
	goto error;

      error = FT_Set_Charmap(face, face->charmaps[charmap]);
      
      if (error)
	goto error;
      
      font = _pango_xft_font_new (fontmap, description, xft_font);
    }
  else
    return NULL;

  return (PangoFont *)font;

 error:

  XftFontClose (xfontmap->display, xft_font);
  return NULL;
}

void
_pango_xft_font_map_cache_add (PangoFontMap *fontmap,
			       PangoXftFont *xfont)
{
  PangoXftFontMap *xfontmap = PANGO_XFT_FONT_MAP (fontmap);

  if (xfontmap->freed_fonts->length == MAX_FREED_FONTS)
    {
      GObject *old_font = g_queue_pop_tail (xfontmap->freed_fonts);
      g_object_unref (old_font);
    }

  g_object_ref (G_OBJECT (xfont));
  g_queue_push_head (xfontmap->freed_fonts, xfont);
  xfont->in_cache = TRUE;
}

static void
pango_xft_font_map_cache_remove (PangoFontMap *fontmap,
				 PangoXftFont *xfont)
{
  PangoXftFontMap *xfontmap = PANGO_XFT_FONT_MAP (fontmap);

  GList *link = g_list_find (xfontmap->freed_fonts->head, xfont);
  if (link == xfontmap->freed_fonts->tail)
    {
      xfontmap->freed_fonts->tail = xfontmap->freed_fonts->tail->prev;
      if (xfontmap->freed_fonts->tail)
	xfontmap->freed_fonts->tail->next = NULL;
    }
  
  xfontmap->freed_fonts->head = g_list_delete_link (xfontmap->freed_fonts->head, link);
  xfontmap->freed_fonts->length--;
  xfont->in_cache = FALSE;

  g_object_unref (G_OBJECT (xfont));
}

static void
pango_xft_font_map_cache_clear (PangoXftFontMap   *xfontmap)
{
  g_list_foreach (xfontmap->freed_fonts->head, (GFunc)g_object_unref, NULL);
  g_list_free (xfontmap->freed_fonts->head);
  xfontmap->freed_fonts->head = NULL;
  xfontmap->freed_fonts->tail = NULL;
  xfontmap->freed_fonts->length = 0;
}

void
_pango_xft_font_map_set_coverage (PangoFontMap  *fontmap,
				  const char    *name,
				  PangoCoverage *coverage)
{
  PangoXftFontMap *xfontmap = PANGO_XFT_FONT_MAP (fontmap);

  g_hash_table_insert (xfontmap->coverage_hash, g_strdup (name),
		       pango_coverage_ref (coverage));
}

PangoCoverage *
_pango_xft_font_map_get_coverage (PangoFontMap  *fontmap,
				  const char    *name)
{
  PangoXftFontMap *xfontmap = PANGO_XFT_FONT_MAP (fontmap);

  return g_hash_table_lookup (xfontmap->coverage_hash, name);
}

void
_pango_xft_font_map_get_info (PangoFontMap *fontmap,
			      Display     **display,
			      int          *screen)
{
  PangoXftFontMap *xfontmap = PANGO_XFT_FONT_MAP (fontmap);

  if (display)
    *display = xfontmap->display;
  if (screen)
    *screen = xfontmap->screen;

}
