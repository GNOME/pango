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

/* Number of freed fonts */
#define MAX_FREED_FONTS 16

typedef struct _PangoXftFontMap      PangoXftFontMap;
typedef struct _PangoXftFamily       PangoXftFamily;
typedef struct _PangoXftFace         PangoXftFace;
typedef struct _PangoXftPatternSet   PangoXftPatternSet;

#define PANGO_TYPE_XFT_FONT_MAP              (pango_xft_font_map_get_type ())
#define PANGO_XFT_FONT_MAP(object)           (G_TYPE_CHECK_INSTANCE_CAST ((object), PANGO_TYPE_XFT_FONT_MAP, PangoXftFontMap))
#define PANGO_XFT_IS_FONT_MAP(object)        (G_TYPE_CHECK_INSTANCE_TYPE ((object), PANGO_TYPE_XFT_FONT_MAP))

struct _PangoXftFontMap
{
  PangoFontMap parent_instance;

  GHashTable *fontset_hash; /* Maps PangoFontDescription -> PangoXftPatternSet */
  GHashTable *coverage_hash; /* Maps font file name -> PangoCoverage */

  GHashTable *fonts; /* Maps XftPattern -> PangoXftFont */
  GQueue *freed_fonts; /* Fonts in fonts that has been freed */

  /* List of all families availible */
  PangoXftFamily **families;
  int n_families;		/* -1 == uninitialized */

  /* List of all fonts (XftPatterns) availible */
/*  FcFontSet *font_set; */

  Display *display;
  int screen;  
};

#define PANGO_XFT_TYPE_FAMILY              (pango_xft_family_get_type ())
#define PANGO_XFT_FAMILY(object)           (G_TYPE_CHECK_INSTANCE_CAST ((object), PANGO_XFT_TYPE_FAMILY, PangoXftFamily))
#define PANGO_XFT_IS_FAMILY(object)        (G_TYPE_CHECK_INSTANCE_TYPE ((object), PANGO_XFT_TYPE_FAMILY))

struct _PangoXftFamily
{
  PangoFontFamily parent_instance;

  PangoXftFontMap *fontmap;
  char *family_name;

  PangoXftFace **faces;
  int n_faces;		/* -1 == uninitialized */
};

struct _PangoXftPatternSet
{
  int n_patterns;
  FcPattern **patterns;
};

#define PANGO_XFT_TYPE_FACE              (pango_xft_face_get_type ())
#define PANGO_XFT_FACE(object)           (G_TYPE_CHECK_INSTANCE_CAST ((object), PANGO_XFT_TYPE_FACE, PangoXftFace))
#define PANGO_XFT_IS_FACE(object)        (G_TYPE_CHECK_INSTANCE_TYPE ((object), PANGO_XFT_TYPE_FACE))

struct _PangoXftFace
{
  PangoFontFace parent_instance;

  PangoXftFamily *family;
  char *style;
};

static GType    pango_xft_font_map_get_type   (void);
GType           pango_xft_family_get_type     (void);
GType           pango_xft_face_get_type       (void);

static void          pango_xft_font_map_init          (PangoXftFontMap              *fontmap);
static void          pango_xft_font_map_class_init    (PangoFontMapClass            *class);
static void          pango_xft_font_map_finalize      (GObject                      *object);
static PangoFont *   pango_xft_font_map_load_font     (PangoFontMap                 *fontmap,
						       PangoContext                 *context,
						       const PangoFontDescription   *description);
static PangoFontset *pango_xft_font_map_load_fontset  (PangoFontMap                 *fontmap,
						       PangoContext                 *context,
						       const PangoFontDescription   *desc,
						       PangoLanguage                *language);
static void          pango_xft_font_map_list_families (PangoFontMap                 *fontmap,
						       PangoFontFamily            ***families,
						       int                          *n_families);


static void pango_xft_font_set_free         (PangoXftPatternSet *font_set);

static void pango_xft_font_map_cache_clear  (PangoXftFontMap    *xfontmap);
static void pango_xft_font_map_cache_remove (PangoFontMap       *fontmap,
					     PangoXftFont       *xfont);

static PangoFontClass *parent_class;	/* Parent class structure for PangoXftFontMap */

static GType
pango_xft_font_map_get_type (void)
{
  static GType object_type = 0;

  if (!object_type)
    {
      static const GTypeInfo object_info =
      {
        sizeof (PangoFontMapClass),
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
  xfontmap->n_families = -1;
}

static void
pango_xft_font_map_class_init (PangoFontMapClass *class)
{
  GObjectClass *object_class = G_OBJECT_CLASS (class);
  
  parent_class = g_type_class_peek_parent (class);
  
  object_class->finalize = pango_xft_font_map_finalize;
  class->load_font = pango_xft_font_map_load_font;
  class->load_fontset = pango_xft_font_map_load_fontset;
  class->list_families = pango_xft_font_map_list_families;
}

static GSList *fontmaps = NULL;

guint
pango_xft_pattern_hash (FcPattern *pattern)
{
  char *str;
  int i;
  double d;
  guint hash = 0;
  
  FcPatternGetString (pattern, FC_FILE, 0, (FcChar8 **) &str);
  if (str)
    hash = g_str_hash (str);

  if (FcPatternGetInteger (pattern, FC_INDEX, 0, &i) == FcResultMatch)
    hash ^= i;

  if (FcPatternGetDouble (pattern, FC_PIXEL_SIZE, 0, &d) == FcResultMatch)
    hash ^= (guint) (d*1000.0);

  return hash;
}

gboolean
pango_xft_pattern_equal (FcPattern *pattern1,
			 FcPattern *pattern2)
{
  return FcPatternEqual (pattern1, pattern2);
}

static PangoFontMap *
pango_xft_get_font_map (Display *display,
			int      screen)
{
  PangoXftFontMap *xfontmap;
  GSList *tmp_list = fontmaps;

  g_return_val_if_fail (display != NULL, NULL);
  
  /* Make sure that the type system is initialized */
  g_type_init ();
  
  while (tmp_list)
    {
      xfontmap = tmp_list->data;

      if (xfontmap->display == display &&
	  xfontmap->screen == screen) 
	return PANGO_FONT_MAP (xfontmap);

      tmp_list = tmp_list->next;
    }

  xfontmap = (PangoXftFontMap *)g_object_new (PANGO_TYPE_XFT_FONT_MAP, NULL);
  
  xfontmap->display = display;
  xfontmap->screen = screen;

  xfontmap->fonts = g_hash_table_new ((GHashFunc)pango_xft_pattern_hash,
				      (GEqualFunc)pango_xft_pattern_equal);
  xfontmap->fontset_hash = g_hash_table_new_full ((GHashFunc)pango_font_description_hash,
						  (GEqualFunc)pango_font_description_equal,
						  (GDestroyNotify)pango_font_description_free,
						  (GDestroyNotify)pango_xft_font_set_free);
  xfontmap->coverage_hash = g_hash_table_new_full (g_str_hash, g_str_equal,
						   (GDestroyNotify)g_free,
						   (GDestroyNotify)pango_coverage_unref);
  xfontmap->freed_fonts = g_queue_new ();

  fontmaps = g_slist_prepend (fontmaps, xfontmap);

  return PANGO_FONT_MAP (xfontmap);
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
  int i;
  static gboolean registered_modules = FALSE;

  g_return_val_if_fail (display != NULL, NULL);

  if (!registered_modules)
    {
      registered_modules = TRUE;
      
      for (i = 0; _pango_included_xft_modules[i].list; i++)
        pango_module_register (&_pango_included_xft_modules[i]);
    }
  
  result = pango_context_new ();
  pango_context_set_font_map (result, pango_xft_get_font_map (display, screen));

  return result;
}

static void
pango_xft_font_map_finalize (GObject *object)
{
  PangoXftFontMap *xfontmap = PANGO_XFT_FONT_MAP (object);

  fontmaps = g_slist_remove (fontmaps, object);

  g_queue_free (xfontmap->freed_fonts);
  g_hash_table_destroy (xfontmap->fontset_hash);
  g_hash_table_destroy (xfontmap->coverage_hash);
  
  G_OBJECT_CLASS (parent_class)->finalize (object);
}


/* Add a mapping from xfont->font_pattern to xfont */
void
_pango_xft_font_map_add (PangoFontMap *fontmap,
			 PangoXftFont *xfont)
{
  PangoXftFontMap *xfontmap = PANGO_XFT_FONT_MAP (fontmap);

  g_hash_table_insert (xfontmap->fonts,
		       xfont->font_pattern,
		       xfont);
}

/* Remove mapping from xfont->font_pattern to xfont */
void
_pango_xft_font_map_remove (PangoFontMap *fontmap,
			   PangoXftFont *xfont)
{
  PangoXftFontMap *xfontmap = PANGO_XFT_FONT_MAP (fontmap);

  g_hash_table_remove (xfontmap->fonts,
		       xfont->font_pattern);
}

static PangoXftFamily *
create_family (PangoXftFontMap *xfontmap,
	       const char      *family_name)
{
  PangoXftFamily *family = g_object_new (PANGO_XFT_TYPE_FAMILY, NULL);
  family->fontmap = xfontmap;
  family->family_name = g_strdup (family_name);

  return family;
}

static gboolean
is_alias_family (const char *family_name)
{
  switch (family_name[0])
    {
    case 'm':
    case 'M':
      return (g_ascii_strcasecmp (family_name, "monospace") == 0);
    case 's':
    case 'S':
      return (g_ascii_strcasecmp (family_name, "sans") == 0 ||
	      g_ascii_strcasecmp (family_name, "serif") == 0);
    }

  return FALSE;
}

static void
pango_xft_font_map_list_families (PangoFontMap           *fontmap,
				  PangoFontFamily      ***families,
				  int                    *n_families)
{
  PangoXftFontMap *xfontmap = PANGO_XFT_FONT_MAP (fontmap);
  FcFontSet *fontset;
  int i;
  int count;

  if (xfontmap->n_families < 0)
    {
      fontset = XftListFonts (xfontmap->display, xfontmap->screen,
			      NULL,
			      FC_FAMILY,
			      NULL);

      xfontmap->families = g_new (PangoXftFamily *, fontset->nfont + 3); /* 3 standard aliases */

      count = 0;
      for (i = 0; i < fontset->nfont; i++)
	{
	  char *s;
	  FcResult res;
	  
	  res = FcPatternGetString (fontset->fonts[i], FC_FAMILY, 0, (FcChar8 **) &s);
	  g_assert (res == FcResultMatch);
	  
	  if (!is_alias_family (s))
	    xfontmap->families[count++] = create_family (xfontmap, s);
	}

      FcFontSetDestroy (fontset);

      xfontmap->families[count++] = create_family (xfontmap, "Sans");
      xfontmap->families[count++] = create_family (xfontmap, "Serif");
      xfontmap->families[count++] = create_family (xfontmap, "Monospace");
      
      xfontmap->n_families = count;
    }

  if (n_families)
    *n_families = xfontmap->n_families;
  
  if (families)
    *families = g_memdup (xfontmap->families, xfontmap->n_families * sizeof (PangoFontFamily *));
}

static int
pango_xft_convert_weight (PangoWeight pango_weight)
{
  int weight;
  
  if (pango_weight < (PANGO_WEIGHT_NORMAL + PANGO_WEIGHT_LIGHT) / 2)
    weight = FC_WEIGHT_LIGHT;
  else if (pango_weight < (PANGO_WEIGHT_NORMAL + 600) / 2)
    weight = FC_WEIGHT_MEDIUM;
  else if (pango_weight < (600 + PANGO_WEIGHT_BOLD) / 2)
    weight = FC_WEIGHT_DEMIBOLD;
  else if (pango_weight < (PANGO_WEIGHT_BOLD + PANGO_WEIGHT_ULTRABOLD) / 2)
    weight = FC_WEIGHT_BOLD;
  else
    weight = FC_WEIGHT_BLACK;
  
  return weight;
}

static int
pango_xft_convert_slant (PangoStyle pango_style)
{
  int slant;
  
  if (pango_style == PANGO_STYLE_ITALIC)
    slant = FC_SLANT_ITALIC;
  else if (pango_style == PANGO_STYLE_OBLIQUE)
    slant = FC_SLANT_OBLIQUE;
  else
    slant = FC_SLANT_ROMAN;
  
  return slant;
}


static FcPattern *
pango_xft_make_pattern (const PangoFontDescription *description)
{
  FcPattern *pattern;
  PangoStyle pango_style;
  int slant;
  int weight;
  char **families;
  int i;

  pango_style = pango_font_description_get_style (description);

  slant = pango_xft_convert_slant (pango_style);
  weight = pango_xft_convert_weight (pango_font_description_get_weight (description));
  
  /* To fool Xft into not munging glyph indices, we open it as glyphs-fontspecific
   * then set the encoding ourself
   */
  pattern = FcPatternBuild (0,
			     FC_WEIGHT, FcTypeInteger, weight,
			     FC_SLANT,  FcTypeInteger, slant,
			     FC_SIZE, FcTypeDouble, (double)pango_font_description_get_size (description)/PANGO_SCALE,
			     NULL);

  families = g_strsplit (pango_font_description_get_family (description), ",", -1);
  
  for (i = 0; families[i]; i++)
    FcPatternAddString (pattern, FC_FAMILY, families[i]);

  g_strfreev (families);

  return pattern;
}

static PangoFont *
pango_xft_font_map_new_font (PangoFontMap  *fontmap,
			     FcPattern    *match)
{
  PangoXftFontMap *xfontmap = (PangoXftFontMap *)fontmap;
  PangoXftFont *font;
  
  /* Look up cache */
  font = g_hash_table_lookup (xfontmap->fonts, match);
  
  if (font)
    {
      g_object_ref (font);

      /* Revive font from cache */
      if (font->in_cache)
	pango_xft_font_map_cache_remove (fontmap, font);

      return (PangoFont *)font;
    }
  
  return  (PangoFont *)_pango_xft_font_new (fontmap, FcPatternDuplicate (match));
}

static PangoXftPatternSet *
pango_xft_font_map_get_patterns (PangoFontMap               *fontmap,
				 PangoContext               *context,
				 const PangoFontDescription *desc)
{
  PangoXftFontMap *xfontmap = (PangoXftFontMap *)fontmap;
  FcPattern *pattern, *pattern_copy;
  FcPattern *match;
  char *family, *family_res;
  FcResult res;
  int id;
  GPtrArray *array;
  PangoXftPatternSet *patterns;

  patterns = g_hash_table_lookup (xfontmap->fontset_hash, desc);

  if (patterns == NULL)
    {
      pattern = pango_xft_make_pattern (desc);

      FcConfigSubstitute (0, pattern, FcMatchPattern);
      XftDefaultSubstitute (xfontmap->display, xfontmap->screen, pattern);

      pattern_copy = FcPatternDuplicate (pattern);

      array = g_ptr_array_new ();
      patterns = g_new (PangoXftPatternSet, 1);

      match = NULL;
      id = 0;
      while (FcPatternGetString (pattern, FC_FAMILY, id++, (FcChar8 **) &family) == FcResultMatch)
	{
	  FcPatternDel (pattern_copy, FC_FAMILY);
	  FcPatternAddString (pattern_copy, FC_FAMILY, family);
	  
	  match = FcFontMatch (NULL, pattern_copy, &res);
	  
	  if (match &&
	      FcPatternGetString (match, FC_FAMILY, 0, (FcChar8 **) &family_res) == FcResultMatch &&
	      g_ascii_strcasecmp (family, family_res) == 0)
	    {
	      g_ptr_array_add (array, match);
	      match = NULL;
	    }
	  if (match)
	    FcPatternDestroy (match);
	}
      
      if (array->len == 0)
	{
	  match = XftFontMatch (xfontmap->display, xfontmap->screen,
				pattern, &res);
	  if (match == NULL)
	    g_error ("Failed to match any font. This could be due to a broken Xft "
		     "configuration, or if you run XFree 4.1.0 due to a bug in libXrender. "
		     "For more information about this, read http://bugzilla.gnome.org/show_bug.cgi?id=68030\n");
	  g_ptr_array_add (array, match);
	}

      FcPatternDestroy (pattern);
      FcPatternDestroy (pattern_copy);

      patterns->n_patterns = array->len;
      patterns->patterns = (FcPattern **)g_ptr_array_free (array, FALSE);
      
      g_hash_table_insert (xfontmap->fontset_hash,
			   pango_font_description_copy (desc),
			   patterns);
    }

  return patterns;
}

static PangoFont *
pango_xft_font_map_load_font (PangoFontMap               *fontmap,
			      PangoContext               *context,
			      const PangoFontDescription *description)
{
  PangoXftPatternSet *patterns = pango_xft_font_map_get_patterns (fontmap, context, description);

  if (patterns->n_patterns > 0)
    return pango_xft_font_map_new_font (fontmap, patterns->patterns[0]);
  
  return NULL;
}

static void
pango_xft_font_set_free (PangoXftPatternSet *font_set)
{
  int i;
  
  for (i = 0; i < font_set->n_patterns; i++)
    FcPatternDestroy (font_set->patterns[i]);

  g_free (font_set);
}


static PangoFontset *
pango_xft_font_map_load_fontset (PangoFontMap                 *fontmap,
				 PangoContext                 *context,
				 const PangoFontDescription   *desc,
				 PangoLanguage                *language)
{
  PangoFontsetSimple *simple;
  int i;
  PangoXftPatternSet *patterns = pango_xft_font_map_get_patterns (fontmap, context, desc);
	  
  simple = pango_fontset_simple_new (language);

  for (i = 0; i < patterns->n_patterns; i++)
    pango_fontset_simple_append (simple,
				 pango_xft_font_map_new_font (fontmap, patterns->patterns[i]));
  
  return PANGO_FONTSET (simple);
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


/* 
 * PangoXftFace
 */

PangoFontDescription *
_pango_xft_font_desc_from_pattern (FcPattern *pattern, gboolean include_size)
{
  PangoFontDescription *desc;
  PangoStyle style;
  PangoWeight weight;
  double size;
  
  char *s;
  int i;

  desc = pango_font_description_new ();

  g_assert (FcPatternGetString (pattern, FC_FAMILY, 0, (FcChar8 **) &s) == FcResultMatch);

  pango_font_description_set_family (desc, s);
  
  if (FcPatternGetInteger (pattern, FC_SLANT, 0, &i) == FcResultMatch)
    {
      if (i == FC_SLANT_ROMAN)
	style = PANGO_STYLE_NORMAL;
      else if (i == FC_SLANT_OBLIQUE)
	style = PANGO_STYLE_OBLIQUE;
      else
	style = PANGO_STYLE_ITALIC;
    }
  else
    style = PANGO_STYLE_NORMAL;

  pango_font_description_set_style (desc, style);

  if (FcPatternGetInteger (pattern, FC_WEIGHT, 0, &i) == FcResultMatch)
    { 
     if (i < FC_WEIGHT_LIGHT)
	weight = PANGO_WEIGHT_ULTRALIGHT;
      else if (i < (FC_WEIGHT_LIGHT + FC_WEIGHT_MEDIUM) / 2)
	weight = PANGO_WEIGHT_LIGHT;
      else if (i < (FC_WEIGHT_MEDIUM + FC_WEIGHT_DEMIBOLD) / 2)
	weight = PANGO_WEIGHT_NORMAL;
      else if (i < (FC_WEIGHT_DEMIBOLD + FC_WEIGHT_BOLD) / 2)
	weight = 600;
      else if (i < (FC_WEIGHT_BOLD + FC_WEIGHT_BLACK) / 2)
	weight = PANGO_WEIGHT_BOLD;
      else
	weight = PANGO_WEIGHT_ULTRABOLD;
    }
  else
    weight = PANGO_WEIGHT_NORMAL;

  if (include_size && FcPatternGetDouble (pattern, FC_SIZE, 0, &size) == FcResultMatch)
      pango_font_description_set_size (desc, size * PANGO_SCALE);
  
  pango_font_description_set_weight (desc, weight);
  
  pango_font_description_set_variant (desc, PANGO_VARIANT_NORMAL);
  pango_font_description_set_stretch (desc, PANGO_STRETCH_NORMAL);

  return desc;
}

static PangoFontDescription *
make_alias_description (PangoXftFamily *xfamily,
			gboolean        bold,
			gboolean        italic)
{
  PangoFontDescription *desc = pango_font_description_new ();

  pango_font_description_set_family (desc, xfamily->family_name);
  pango_font_description_set_style (desc, italic ? PANGO_STYLE_ITALIC : PANGO_STYLE_NORMAL);
  pango_font_description_set_variant (desc, PANGO_VARIANT_NORMAL);
  pango_font_description_set_weight (desc, bold ? PANGO_WEIGHT_BOLD : PANGO_WEIGHT_NORMAL);
  pango_font_description_set_stretch (desc, PANGO_STRETCH_NORMAL);

  return desc;
}

static PangoFontDescription *
pango_xft_face_describe (PangoFontFace *face)
{
  PangoXftFace *xface = PANGO_XFT_FACE (face);
  PangoXftFamily *xfamily = xface->family;
  PangoXftFontMap *xfontmap = xfamily->fontmap;
  PangoFontDescription *desc = NULL;
  FcResult res;
  FcPattern *match_pattern;
  FcPattern *result_pattern;

  if (is_alias_family (xfamily->family_name))
    {
      if (strcmp (xface->style, "Regular") == 0)
	return make_alias_description (xfamily, FALSE, FALSE);
      else if (strcmp (xface->style, "Bold") == 0)
	return make_alias_description (xfamily, TRUE, FALSE);
      else if (strcmp (xface->style, "Italic") == 0)
	return make_alias_description (xfamily, FALSE, TRUE);
      else			/* Bold Italic */
	return make_alias_description (xfamily, TRUE, TRUE);
    }
  
  match_pattern = FcPatternBuild (NULL,
				  FC_FAMILY, FcTypeString, xfamily->family_name,
				  FC_STYLE, FcTypeString, xface->style,
				  NULL);

  g_assert (match_pattern);
  
  result_pattern = XftFontMatch (xfontmap->display, xfontmap->screen, match_pattern, &res);
  if (result_pattern)
    {
      desc = _pango_xft_font_desc_from_pattern (result_pattern, FALSE);
      FcPatternDestroy (result_pattern);
    }

  FcPatternDestroy (match_pattern);
  
  return desc;
}

static const char *
pango_xft_face_get_face_name (PangoFontFace *face)
{
  PangoXftFace *xface = PANGO_XFT_FACE (face);

  return xface->style;
}

static void
pango_xft_face_class_init (PangoFontFaceClass *class)
{
  class->describe = pango_xft_face_describe;
  class->get_face_name = pango_xft_face_get_face_name;
}

GType
pango_xft_face_get_type (void)
{
  static GType object_type = 0;

  if (!object_type)
    {
      static const GTypeInfo object_info =
      {
        sizeof (PangoFontFaceClass),
        (GBaseInitFunc) NULL,
        (GBaseFinalizeFunc) NULL,
        (GClassInitFunc) pango_xft_face_class_init,
        NULL,           /* class_finalize */
        NULL,           /* class_data */
        sizeof (PangoXftFace),
        0,              /* n_preallocs */
        (GInstanceInitFunc) NULL,
      };
      
      object_type = g_type_register_static (PANGO_TYPE_FONT_FACE,
                                            "PangoXftFace",
                                            &object_info, 0);
    }
  
  return object_type;
}

/*
 * PangoXFontFamily
 */
static PangoXftFace *
create_face (PangoXftFamily *xfamily,
	     const char     *style)
{
  PangoXftFace *face = g_object_new (PANGO_XFT_TYPE_FACE, NULL);
  face->style = g_strdup (style);
  face->family = xfamily;

  return face;
}

static void
pango_xft_family_list_faces (PangoFontFamily  *family,
			     PangoFontFace  ***faces,
			     int              *n_faces)
{
  PangoXftFamily *xfamily = PANGO_XFT_FAMILY (family);
  PangoXftFontMap *xfontmap = xfamily->fontmap;

  if (xfamily->n_faces < 0)
    {
      FcFontSet *fontset;
      int i;
      
      if (is_alias_family (xfamily->family_name))
	{
	  xfamily->n_faces = 4;
	  xfamily->faces = g_new (PangoXftFace *, xfamily->n_faces);

	  i = 0;
	  xfamily->faces[i++] = create_face (xfamily, "Regular");
	  xfamily->faces[i++] = create_face (xfamily, "Bold");
	  xfamily->faces[i++] = create_face (xfamily, "Italic");
	  xfamily->faces[i++] = create_face (xfamily, "Bold Italic");
	}
      else
	{
	  fontset = XftListFonts (xfontmap->display, xfontmap->screen,
				  FC_FAMILY, FcTypeString, xfamily->family_name,
				  NULL,
				  FC_STYLE,
				  NULL);
      
	  xfamily->n_faces = fontset->nfont;
	  xfamily->faces = g_new (PangoXftFace *, xfamily->n_faces);
	  
	  for (i = 0; i < fontset->nfont; i++)
	    {
	      FcChar8 *s;
	      FcResult res;

	      res = FcPatternGetString (fontset->fonts[i], FC_STYLE, 0, (FcChar8 **) &s);
	      if (res != FcResultMatch)
		s = "Regular";

	      xfamily->faces[i] = create_face (xfamily, s);
	    }

	  FcFontSetDestroy (fontset);
	}
    }
  
  if (n_faces)
    *n_faces = xfamily->n_faces;
  
  if (faces)
    *faces = g_memdup (xfamily->faces, xfamily->n_faces * sizeof (PangoFontFace *));
}

const char *
pango_xft_family_get_name (PangoFontFamily  *family)
{
  PangoXftFamily *xfamily = PANGO_XFT_FAMILY (family);

  return xfamily->family_name;
}

static void
pango_xft_family_class_init (PangoFontFamilyClass *class)
{
  class->list_faces = pango_xft_family_list_faces;
  class->get_name = pango_xft_family_get_name;
}

void
pango_xft_family_init (PangoXftFamily *xfamily)
{
  xfamily->n_faces = -1;
}

GType
pango_xft_family_get_type (void)
{
  static GType object_type = 0;

  if (!object_type)
    {
      static const GTypeInfo object_info =
      {
        sizeof (PangoFontFamilyClass),
        (GBaseInitFunc) NULL,
        (GBaseFinalizeFunc) NULL,
        (GClassInitFunc) pango_xft_family_class_init,
        NULL,           /* class_finalize */
        NULL,           /* class_data */
        sizeof (PangoXftFamily),
        0,              /* n_preallocs */
        (GInstanceInitFunc) pango_xft_family_init,
      };
      
      object_type = g_type_register_static (PANGO_TYPE_FONT_FAMILY,
                                            "PangoXftFamily",
                                            &object_info, 0);
    }
  
  return object_type;
}
