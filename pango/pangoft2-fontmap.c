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

#include "pango-fontmap.h"
#include "pango-utils.h"
#include "pangoft2-private.h"

#include "mini-xft/MiniXftFreetype.h"

#ifdef G_OS_WIN32
#define STRICT
#include <windows.h>
#endif

#define PANGO_TYPE_FT2_FONT_MAP              (pango_ft2_font_map_get_type ())
#define PANGO_FT2_FONT_MAP(object)           (G_TYPE_CHECK_INSTANCE_CAST ((object), PANGO_TYPE_FT2_FONT_MAP, PangoFT2FontMap))
#define PANGO_FT2_IS_FONT_MAP(object)        (G_TYPE_CHECK_INSTANCE_TYPE ((object), PANGO_TYPE_FT2_FONT_MAP))

typedef struct _PangoFT2FontMap      PangoFT2FontMap;
typedef struct _PangoFT2SizeInfo     PangoFT2SizeInfo;
typedef struct _PangoFT2PatternSet   PangoFT2PatternSet;

/* Number of freed fonts */
#define MAX_FREED_FONTS 16

struct _PangoFT2Family
{
  PangoFontFamily parent_instance;
  
  PangoFT2FontMap *fontmap;
  char *family_name;

  PangoFT2Face **faces;
  int n_faces;		/* -1 == uninitialized */
};


struct _PangoFT2FontMap
{
  PangoFontMap parent_instance;

  FT_Library library;

  GHashTable *fontset_hash; /* Maps PangoFontDescription -> PangoXftFontSet */
  GHashTable *coverage_hash; /* Maps font file name -> PangoCoverage */

  GHashTable *fonts; /* Maps XftPattern -> PangoFT2Font */
  GQueue *freed_fonts; /* Fonts in fonts that has been freed */

  /* List of all families availible */
  PangoFT2Family **families;
  int n_families;		/* -1 == uninitialized */
};

struct _PangoFT2PatternSet
{
  int n_patterns;
  MiniXftPattern **patterns;
};

#define PANGO_FT2_TYPE_FAMILY              (pango_ft2_family_get_type ())
#define PANGO_FT2_FAMILY(object)           (G_TYPE_CHECK_INSTANCE_CAST ((object), PANGO_FT2_TYPE_FAMILY, PangoFT2Family))
#define PANGO_FT2_IS_FAMILY(object)        (G_TYPE_CHECK_INSTANCE_TYPE ((object), PANGO_FT2_TYPE_FAMILY))

#define PANGO_FT2_TYPE_FACE              (pango_ft2_face_get_type ())
#define PANGO_FT2_FACE(object)           (G_TYPE_CHECK_INSTANCE_CAST ((object), PANGO_FT2_TYPE_FACE, PangoFT2Face))
#define PANGO_FT2_IS_FACE(object)        (G_TYPE_CHECK_INSTANCE_TYPE ((object), PANGO_FT2_TYPE_FACE))

static GType    pango_ft2_font_map_get_type      (void);
GType           pango_ft2_family_get_type          (void);
GType           pango_ft2_face_get_type            (void);

static void          pango_ft2_font_map_init          (PangoFT2FontMap              *fontmap);
static void          pango_ft2_font_map_class_init    (PangoFontMapClass            *class);
static void          pango_ft2_font_map_finalize      (GObject                      *object);
static PangoFont *   pango_ft2_font_map_load_font     (PangoFontMap                 *fontmap,
						       PangoContext                 *context,
						       const PangoFontDescription   *description);
static PangoFontset *pango_ft2_font_map_load_fontset  (PangoFontMap                 *fontmap,
						       PangoContext                 *context,
						       const PangoFontDescription   *desc,
						       PangoLanguage                *language);
static void          pango_ft2_font_set_free          (PangoFT2PatternSet           *font_set);
static void          pango_ft2_font_map_list_families (PangoFontMap                 *fontmap,
						       PangoFontFamily            ***families,
						       int                          *n_families);
static void          pango_ft2_font_map_cache_remove  (PangoFontMap                 *fontmap,
						       PangoFT2Font                 *ft2font);
static void          pango_ft2_font_map_cache_clear   (PangoFT2FontMap              *ft2fontmap);



static PangoFontClass  *parent_class;	/* Parent class structure for PangoFT2FontMap */

static PangoFT2FontMap *pango_ft2_global_fontmap = NULL;

static GType
pango_ft2_font_map_get_type (void)
{
  static GType object_type = 0;

  if (!object_type)
    {
      static const GTypeInfo object_info =
      {
        sizeof (PangoFontMapClass),
        (GBaseInitFunc) NULL,
        (GBaseFinalizeFunc) NULL,
        (GClassInitFunc) pango_ft2_font_map_class_init,
        NULL,           /* class_finalize */
        NULL,           /* class_data */
        sizeof (PangoFT2FontMap),
        0,              /* n_preallocs */
        (GInstanceInitFunc) pango_ft2_font_map_init,
      };
      
      object_type = g_type_register_static (PANGO_TYPE_FONT_MAP,
                                            "PangoFT2FontMap",
                                            &object_info, 0);
    }
  
  return object_type;
}

static void
pango_ft2_font_set_free (PangoFT2PatternSet *font_set)
{
  int i;
  
  for (i = 0; i < font_set->n_patterns; i++)
    MiniXftPatternDestroy (font_set->patterns[i]);

  g_free (font_set);
}

static guint
pango_ft2_pattern_hash (MiniXftPattern *pattern)
{
  char *str;
  int i;
  double d;
  guint hash = 0;
  
  MiniXftPatternGetString (pattern, XFT_FILE, 0, &str);
  if (str)
    hash = g_str_hash (str);

  if (MiniXftPatternGetInteger (pattern, XFT_INDEX, 0, &i) == MiniXftResultMatch)
    hash ^= i;

  if (MiniXftPatternGetDouble (pattern, XFT_PIXEL_SIZE, 0, &d) == MiniXftResultMatch)
    hash ^= (guint) (d*1000.0);

  return hash;
}

static gboolean
pango_ft2_pattern_equal (MiniXftPattern *pattern1,
			 MiniXftPattern *pattern2)
{
  char *file1, *file2;
  int index1, index2;
  double size1, size2;
  MiniXftResult res1, res2;
  int int1, int2;
  Bool bool1, bool2;
  
  MiniXftPatternGetString (pattern1, XFT_FILE, 0, &file1);
  MiniXftPatternGetString (pattern2, XFT_FILE, 0, &file2);

  g_assert (file1 != NULL && file2 != NULL);

  if (strcmp (file1, file2) != 0)
    return FALSE;
  
  if (MiniXftPatternGetInteger (pattern1, XFT_INDEX, 0, &index1) != MiniXftResultMatch)
    return FALSE;
  
  if (MiniXftPatternGetInteger (pattern2, XFT_INDEX, 0, &index2) != MiniXftResultMatch)
    return FALSE;

  if (index1 != index2)
    return FALSE;

  if (MiniXftPatternGetDouble (pattern1, XFT_PIXEL_SIZE, 0, &size1) != MiniXftResultMatch)
    return FALSE;

  if (MiniXftPatternGetDouble (pattern2, XFT_PIXEL_SIZE, 0, &size2) != MiniXftResultMatch)
    return FALSE;

  if (size1 != size2)
    return FALSE;

  res1 = MiniXftPatternGetInteger (pattern1, XFT_RGBA, 0, &int1);
  res2 = MiniXftPatternGetInteger (pattern2, XFT_RGBA, 0, &int2);
  if (res1 != res2 || (res1 == MiniXftResultMatch && int1 != int2))
    return FALSE;
  
  res1 = MiniXftPatternGetBool (pattern1, XFT_ANTIALIAS, 0, &bool1);
  res2 = MiniXftPatternGetBool (pattern2, XFT_ANTIALIAS, 0, &bool2);
  if (res1 != res2 || (res1 == MiniXftResultMatch && bool1 != bool2))
    return FALSE;
  
  res1 = MiniXftPatternGetBool (pattern1, XFT_MINSPACE, 0, &bool1);
  res2 = MiniXftPatternGetBool (pattern2, XFT_MINSPACE, 0, &bool2);
  if (res1 != res2 || (res1 == MiniXftResultMatch && bool1 != bool2))
    return FALSE;

  res1 = MiniXftPatternGetInteger (pattern1, XFT_SPACING, 0, &int1);
  res2 = MiniXftPatternGetInteger (pattern2, XFT_SPACING, 0, &int2);
  if (res1 != res2 || (res1 == MiniXftResultMatch && int1 != int2))
    return FALSE;

  res1 = MiniXftPatternGetInteger (pattern1, XFT_CHAR_WIDTH, 0, &int1);
  res2 = MiniXftPatternGetInteger (pattern2, XFT_CHAR_WIDTH, 0, &int2);
  if (res1 != res2 || (res1 == MiniXftResultMatch && int1 != int2))
    return FALSE;
  
  return TRUE;
}


static void 
pango_ft2_font_map_init (PangoFT2FontMap *ft2fontmap)
{
  ft2fontmap->n_families = -1;

  ft2fontmap->fonts =
    g_hash_table_new ((GHashFunc)pango_ft2_pattern_hash,
		      (GEqualFunc)pango_ft2_pattern_equal);
  ft2fontmap->fontset_hash =
    g_hash_table_new_full ((GHashFunc)pango_font_description_hash,
			   (GEqualFunc)pango_font_description_equal,
			   (GDestroyNotify)pango_font_description_free,
			   (GDestroyNotify)pango_ft2_font_set_free);
  
  ft2fontmap->coverage_hash =
    g_hash_table_new_full (g_str_hash, g_str_equal,
			   (GDestroyNotify)g_free,
			   (GDestroyNotify)pango_coverage_unref);
  ft2fontmap->freed_fonts = g_queue_new ();
}

static void
pango_ft2_font_map_class_init (PangoFontMapClass *class)
{
  GObjectClass *object_class = G_OBJECT_CLASS (class);
  
  parent_class = g_type_class_peek_parent (class);
  
  object_class->finalize = pango_ft2_font_map_finalize;
  class->load_font = pango_ft2_font_map_load_font;
  class->load_fontset = pango_ft2_font_map_load_fontset;
  class->list_families = pango_ft2_font_map_list_families;
}

/**
 * pango_ft2_font_map_for_display:
 *
 * Returns a #PangoFT2FontMap. Font maps are cached and should
 * not be freed. If the font map is no longer needed, it can
 * be released with pango_ft2_shutdown_display().
 *
 * Returns: a #PangoFT2FontMap.
 **/
PangoFontMap *
pango_ft2_font_map_for_display (void)
{
  FT_Error error;

  /* Make sure that the type system is initialized */
  g_type_init ();
  
  if (pango_ft2_global_fontmap != NULL)
    return PANGO_FONT_MAP (pango_ft2_global_fontmap);

  pango_ft2_global_fontmap = (PangoFT2FontMap *)g_object_new (PANGO_TYPE_FT2_FONT_MAP, NULL);
  
  error = FT_Init_FreeType (&pango_ft2_global_fontmap->library);
  if (error != FT_Err_Ok)
    {
      g_warning ("Error from FT_Init_FreeType: %s",
		 _pango_ft2_ft_strerror (error));
      return NULL;
    }

  return PANGO_FONT_MAP (pango_ft2_global_fontmap);
}

/**
 * pango_ft2_shutdown_display:
 * 
 * Free cached resources.
 **/
void
pango_ft2_shutdown_display (void)
{
  if (pango_ft2_global_fontmap)
    {
      pango_ft2_font_map_cache_clear (pango_ft2_global_fontmap);
      
      g_object_unref (G_OBJECT (pango_ft2_global_fontmap));
      
      pango_ft2_global_fontmap = NULL;
    }
}


static void
pango_ft2_font_map_finalize (GObject *object)
{
  PangoFT2FontMap *ft2fontmap = PANGO_FT2_FONT_MAP (object);
  
  g_queue_free (ft2fontmap->freed_fonts);
  g_hash_table_destroy (ft2fontmap->fontset_hash);
  g_hash_table_destroy (ft2fontmap->coverage_hash);
  
  FT_Done_FreeType (ft2fontmap->library);

  G_OBJECT_CLASS (parent_class)->finalize (object);
}

/* Add a mapping from xfont->font_pattern to xfont */
void
_pango_ft2_font_map_add (PangoFontMap *fontmap,
			 PangoFT2Font *ft2font)
{
  PangoFT2FontMap *ft2fontmap = (PangoFT2FontMap *) fontmap;

  g_hash_table_insert (ft2fontmap->fonts,
		       ft2font->font_pattern,
		       ft2font);
}

/* Remove mapping from xfont->font_pattern to xfont */
void
_pango_ft2_font_map_remove (PangoFontMap *fontmap,
			    PangoFT2Font *ft2font)
{
  PangoFT2FontMap *ft2fontmap = (PangoFT2FontMap *) fontmap;

  g_hash_table_remove (ft2fontmap->fonts,
		       ft2font->font_pattern);
}

static PangoFT2Family *
create_family (PangoFT2FontMap *xfontmap,
	       const char      *family_name)
{
  PangoFT2Family *family = g_object_new (PANGO_FT2_TYPE_FAMILY, NULL);
  family->fontmap = xfontmap;
  family->family_name = g_strdup (family_name);
  family->n_faces = -1;

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
pango_ft2_font_map_list_families (PangoFontMap           *fontmap,
				  PangoFontFamily      ***families,
				  int                    *n_families)
{
  PangoFT2FontMap *ft2fontmap = (PangoFT2FontMap *)fontmap;
  MiniXftFontSet *fontset;
  int i;
  int count;

  if (ft2fontmap->n_families < 0)
    {
      fontset = MiniXftListFonts ((Display *)1, 0,
				  XFT_CORE, MiniXftTypeBool, False,
				  XFT_ENCODING, MiniXftTypeString, "iso10646-1",
				  NULL,
				  XFT_FAMILY,
				  NULL);

      ft2fontmap->families = g_new (PangoFT2Family *, fontset->nfont + 3); /* 3 standard aliases */

      count = 0;
      for (i = 0; i < fontset->nfont; i++)
	{
	  char *s;
	  MiniXftResult res;
	  
	  res = MiniXftPatternGetString (fontset->fonts[i], XFT_FAMILY, 0, &s);
	  g_assert (res == MiniXftResultMatch);
	  
 	  if (!is_alias_family (s))
	    ft2fontmap->families[count++] = create_family (ft2fontmap, s);

	}

      ft2fontmap->families[count++] = create_family (ft2fontmap, "Sans");
      ft2fontmap->families[count++] = create_family (ft2fontmap, "Serif");
      ft2fontmap->families[count++] = create_family (ft2fontmap, "Monospace");

      MiniXftFontSetDestroy (fontset);

      ft2fontmap->n_families = count;
    }
  
  if (n_families)
    *n_families = ft2fontmap->n_families;
  
  if (families)
    *families = g_memdup (ft2fontmap->families,
                          ft2fontmap->n_families * sizeof (PangoFontFamily *));
}

static int
pango_ft2_convert_weight (PangoWeight pango_weight)
{
  int weight;
  
  if (pango_weight < (PANGO_WEIGHT_NORMAL + PANGO_WEIGHT_LIGHT) / 2)
    weight = XFT_WEIGHT_LIGHT;
  else if (pango_weight < (PANGO_WEIGHT_NORMAL + 600) / 2)
    weight = XFT_WEIGHT_MEDIUM;
  else if (pango_weight < (600 + PANGO_WEIGHT_BOLD) / 2)
    weight = XFT_WEIGHT_DEMIBOLD;
  else if (pango_weight < (PANGO_WEIGHT_BOLD + PANGO_WEIGHT_ULTRABOLD) / 2)
    weight = XFT_WEIGHT_BOLD;
  else
    weight = XFT_WEIGHT_BLACK;
  
  return weight;
}

static int
pango_ft2_convert_slant (PangoStyle pango_style)
{
  int slant;
  
  if (pango_style == PANGO_STYLE_ITALIC)
    slant = XFT_SLANT_ITALIC;
  else if (pango_style == PANGO_STYLE_OBLIQUE)
    slant = XFT_SLANT_OBLIQUE;
  else
    slant = XFT_SLANT_ROMAN;
  
  return slant;
}


static MiniXftPattern *
pango_ft2_make_pattern (const PangoFontDescription *description)
{
  MiniXftPattern *pattern;
  PangoStyle pango_style;
  int slant;
  int weight;
  char **families;
  int i;
  
  pango_style = pango_font_description_get_style (description);

  slant = pango_ft2_convert_slant (pango_style);
  weight = pango_ft2_convert_weight (pango_font_description_get_weight (description));
  
  /* To fool Xft into not munging glyph indices, we open it as glyphs-fontspecific
   * then set the encoding ourself
   */
  pattern = MiniXftPatternBuild (0,
				 XFT_ENCODING, MiniXftTypeString, "glyphs-fontspecific",
				 XFT_CORE, MiniXftTypeBool, False,
				 XFT_FAMILY, MiniXftTypeString,  pango_font_description_get_family (description),
				 XFT_WEIGHT, MiniXftTypeInteger, weight,
				 XFT_SLANT,  MiniXftTypeInteger, slant,
				 XFT_SIZE, MiniXftTypeDouble, (double)pango_font_description_get_size (description)/PANGO_SCALE,
				 NULL);

  families = g_strsplit (pango_font_description_get_family (description), ",", -1);
  
  for (i = 0; families[i]; i++)
    MiniXftPatternAddString (pattern, XFT_FAMILY, families[i]);

  g_strfreev (families);

  return pattern;
}

static PangoFont *
pango_ft2_font_map_new_font (PangoFontMap    *fontmap,
			     MiniXftPattern  *match)
{
  PangoFT2FontMap *ft2fontmap = (PangoFT2FontMap *)fontmap;
  PangoFT2Font *font;
  
  /* Look up cache */
  font = g_hash_table_lookup (ft2fontmap->fonts, match);
  
  if (font)
    {
      g_object_ref (font);
      
      /* Revive font from cache */
      if (font->in_cache)
	pango_ft2_font_map_cache_remove (fontmap, font);
	  
      return (PangoFont *)font;
    }
  
  return  (PangoFont *)_pango_ft2_font_new (fontmap, MiniXftPatternDuplicate (match));
}


static PangoFont *
pango_ft2_font_map_load_font (PangoFontMap               *fontmap,
			      PangoContext               *context,
			      const PangoFontDescription *description)
{
  MiniXftPattern *pattern, *match;
  MiniXftResult res;
  PangoFont *font;
  
  pattern = pango_ft2_make_pattern (description);
      
  match = MiniXftFontMatch ((Display *)1, 0, pattern, &res);
      
  MiniXftPatternDestroy (pattern);

  font = NULL;
  
  if (match)
    {
      font = pango_ft2_font_map_new_font (fontmap, match);
      MiniXftPatternDestroy (match);
    }

  return font;
}

static PangoFontset *
pango_ft2_font_map_load_fontset (PangoFontMap                 *fontmap,
				 PangoContext                 *context,
				 const PangoFontDescription   *desc,
				 PangoLanguage                *language)
{
  PangoFT2FontMap *ft2fontmap = (PangoFT2FontMap *)fontmap;
  MiniXftPattern *pattern, *pattern_copy;
  MiniXftPattern *match;
  int i;
  char *family, *family_res;
  MiniXftResult res;
  GPtrArray *array;
  int id;
  PangoFT2PatternSet *patterns;
  PangoFontsetSimple *simple;

  patterns = g_hash_table_lookup (ft2fontmap->fontset_hash, desc);

  if (patterns == NULL)
    {
      pattern = pango_ft2_make_pattern (desc);

      MiniXftInit (0);
      MiniXftInitFtLibrary ();
	  
      MiniXftConfigSubstitute (pattern);
      MiniXftDefaultSubstitute ((Display *)1, 0, pattern);

      pattern_copy = MiniXftPatternDuplicate (pattern);

      array = g_ptr_array_new ();
      patterns = g_new (PangoFT2PatternSet, 1);

      match = NULL;
      id = 0;
      while (MiniXftPatternGetString (pattern, XFT_FAMILY, id++, &family) == MiniXftResultMatch)
	{
	  MiniXftPatternDel (pattern_copy, XFT_FAMILY);
	  MiniXftPatternAddString (pattern_copy, XFT_FAMILY, family);

	  match = MiniXftFontSetMatch (&_MiniXftFontSet, 1, pattern_copy, &res);
	  
	  if (match &&
	      MiniXftPatternGetString (match, XFT_FAMILY, 0, &family_res) == MiniXftResultMatch &&
	      g_ascii_strcasecmp (family, family_res) == 0)
	    {
	      g_ptr_array_add (array, match);
	      match = NULL;
	    }
	  if (match)
	    MiniXftPatternDestroy (match);
	}
      
      if (array->len == 0)
	{
	  match = MiniXftFontSetMatch (&_MiniXftFontSet, 1, pattern, &res);
	  g_ptr_array_add (array, match);
	}

      MiniXftPatternDestroy (pattern);
      MiniXftPatternDestroy (pattern_copy);

      patterns->n_patterns = array->len;
      patterns->patterns = (MiniXftPattern **)g_ptr_array_free (array, FALSE);
      
      g_hash_table_insert (ft2fontmap->fontset_hash,
			   pango_font_description_copy (desc),
			   patterns);
    }

  simple = pango_fontset_simple_new (language);

  for (i = 0; i < patterns->n_patterns; i++)
    pango_fontset_simple_append (simple,
				 pango_ft2_font_map_new_font (fontmap, patterns->patterns[i]));
  
  return PANGO_FONTSET (simple);
}

void
_pango_ft2_font_map_cache_add (PangoFontMap *fontmap,
			       PangoFT2Font *ft2font)
{
  PangoFT2FontMap *ft2fontmap = PANGO_FT2_FONT_MAP (fontmap);

  if (ft2fontmap->freed_fonts->length == MAX_FREED_FONTS)
    {
      PangoFT2Font *old_font = g_queue_pop_tail (ft2fontmap->freed_fonts);
      g_object_unref (G_OBJECT (old_font));
    }

  g_object_ref (G_OBJECT (ft2font));
  g_queue_push_head (ft2fontmap->freed_fonts, ft2font);
  ft2font->in_cache = TRUE;
}

static void
pango_ft2_font_map_cache_remove (PangoFontMap *fontmap,
				 PangoFT2Font *ft2font)
{
  PangoFT2FontMap *ft2fontmap = PANGO_FT2_FONT_MAP (fontmap);

  GList *link = g_list_find (ft2fontmap->freed_fonts->head, ft2font);
  if (link == ft2fontmap->freed_fonts->tail)
    {
      ft2fontmap->freed_fonts->tail = ft2fontmap->freed_fonts->tail->prev;
      if (ft2fontmap->freed_fonts->tail)
	ft2fontmap->freed_fonts->tail->next = NULL;
    }
  
  ft2fontmap->freed_fonts->head = g_list_delete_link (ft2fontmap->freed_fonts->head, link);
  ft2fontmap->freed_fonts->length--;
  ft2font->in_cache = FALSE;

  g_object_unref (G_OBJECT (ft2font));
}

static void
pango_ft2_font_map_cache_clear (PangoFT2FontMap *ft2fontmap)
{
  g_list_foreach (ft2fontmap->freed_fonts->head, (GFunc)g_object_unref, NULL);
  g_list_free (ft2fontmap->freed_fonts->head);
  ft2fontmap->freed_fonts->head = NULL;
  ft2fontmap->freed_fonts->tail = NULL;
  ft2fontmap->freed_fonts->length = 0;
}

/* 
 * PangoFT2Face
 */

PangoFontDescription *
_pango_ft2_font_desc_from_pattern (MiniXftPattern *pattern,
				   gboolean        include_size)
{
  PangoFontDescription *desc;
  PangoStyle style;
  PangoWeight weight;
  double size;
  
  char *s;
  int i;

  desc = pango_font_description_new ();

  g_assert (MiniXftPatternGetString (pattern, XFT_FAMILY, 0, &s) == MiniXftResultMatch);

  pango_font_description_set_family (desc, s);
  
  if (MiniXftPatternGetInteger (pattern, XFT_SLANT, 0, &i) == MiniXftResultMatch)
    {
      if (i == XFT_SLANT_ROMAN)
	style = PANGO_STYLE_NORMAL;
      else if (i == XFT_SLANT_OBLIQUE)
	style = PANGO_STYLE_OBLIQUE;
      else
	style = PANGO_STYLE_ITALIC;
    }
  else
    style = PANGO_STYLE_NORMAL;

  pango_font_description_set_style (desc, style);

  if (MiniXftPatternGetInteger (pattern, XFT_WEIGHT, 0, &i) == MiniXftResultMatch)
    { 
     if (i < XFT_WEIGHT_LIGHT)
	weight = PANGO_WEIGHT_ULTRALIGHT;
      else if (i < (XFT_WEIGHT_LIGHT + XFT_WEIGHT_MEDIUM) / 2)
	weight = PANGO_WEIGHT_LIGHT;
      else if (i < (XFT_WEIGHT_MEDIUM + XFT_WEIGHT_DEMIBOLD) / 2)
	weight = PANGO_WEIGHT_NORMAL;
      else if (i < (XFT_WEIGHT_DEMIBOLD + XFT_WEIGHT_BOLD) / 2)
	weight = 600;
      else if (i < (XFT_WEIGHT_BOLD + XFT_WEIGHT_BLACK) / 2)
	weight = PANGO_WEIGHT_BOLD;
      else
	weight = PANGO_WEIGHT_ULTRABOLD;
    }
  else
    weight = PANGO_WEIGHT_NORMAL;

  if (include_size && MiniXftPatternGetDouble (pattern, XFT_SIZE, 0, &size) == MiniXftResultMatch)
      pango_font_description_set_size (desc, size * PANGO_SCALE);
  
  pango_font_description_set_weight (desc, weight);
  
  pango_font_description_set_variant (desc, PANGO_VARIANT_NORMAL);
  pango_font_description_set_stretch (desc, PANGO_STRETCH_NORMAL);

  return desc;
}


static PangoFontDescription *
make_alias_description (PangoFT2Family *ft2family,
			gboolean        bold,
			gboolean        italic)
{
  PangoFontDescription *desc = pango_font_description_new ();

  pango_font_description_set_family (desc, ft2family->family_name);
  pango_font_description_set_style (desc, italic ? PANGO_STYLE_ITALIC : PANGO_STYLE_NORMAL);
  pango_font_description_set_variant (desc, PANGO_VARIANT_NORMAL);
  pango_font_description_set_weight (desc, bold ? PANGO_WEIGHT_BOLD : PANGO_WEIGHT_NORMAL);
  pango_font_description_set_stretch (desc, PANGO_STRETCH_NORMAL);

  return desc;
}

static PangoFontDescription *
pango_ft2_face_describe (PangoFontFace *face)
{
  PangoFT2Face *ft2face = (PangoFT2Face *) face;
  PangoFT2Family *ft2family = ft2face->family;
  PangoFontDescription *desc = NULL;
  MiniXftResult res;
  MiniXftPattern *match_pattern;
  MiniXftPattern *result_pattern;

  if (is_alias_family (ft2family->family_name))
    {
      if (strcmp (ft2face->style, "Regular") == 0)
	return make_alias_description (ft2family, FALSE, FALSE);
      else if (strcmp (ft2face->style, "Bold") == 0)
	return make_alias_description (ft2family, TRUE, FALSE);
      else if (strcmp (ft2face->style, "Italic") == 0)
	return make_alias_description (ft2family, FALSE, TRUE);
      else			/* Bold Italic */
	return make_alias_description (ft2family, TRUE, TRUE);
    }
  
  match_pattern = MiniXftPatternBuild (NULL,
				       XFT_ENCODING, MiniXftTypeString, "iso10646-1",
				       XFT_FAMILY, MiniXftTypeString, ft2family->family_name,
				       XFT_CORE, MiniXftTypeBool, False,
				       XFT_STYLE, MiniXftTypeString, ft2face->style,
				       NULL);
  g_assert (match_pattern);
  
  result_pattern = MiniXftFontMatch ((Display *)1, 0, match_pattern, &res);
  if (result_pattern)
    {
      desc = _pango_ft2_font_desc_from_pattern (result_pattern, FALSE);
      MiniXftPatternDestroy (result_pattern);
    }

  MiniXftPatternDestroy (match_pattern);
  
  return desc;
}

static const char *
pango_ft2_face_get_face_name (PangoFontFace *face)
{
  PangoFT2Face *ft2face = PANGO_FT2_FACE (face);

  return ft2face->style;
}

static void
pango_ft2_face_class_init (PangoFontFaceClass *class)
{
  class->describe = pango_ft2_face_describe;
  class->get_face_name = pango_ft2_face_get_face_name;
}

GType
pango_ft2_face_get_type (void)
{
  static GType object_type = 0;

  if (!object_type)
    {
      static const GTypeInfo object_info =
      {
        sizeof (PangoFontFaceClass),
        (GBaseInitFunc) NULL,
        (GBaseFinalizeFunc) NULL,
        (GClassInitFunc) pango_ft2_face_class_init,
        NULL,           /* class_finalize */
        NULL,           /* class_data */
        sizeof (PangoFT2Face),
        0,              /* n_preallocs */
        (GInstanceInitFunc) NULL,
      };
      
      object_type = g_type_register_static (PANGO_TYPE_FONT_FACE,
                                            "PangoFT2Face",
                                            &object_info, 0);
    }
  
  return object_type;
}

void
_pango_ft2_font_map_set_coverage (PangoFontMap  *fontmap,
				  const char    *name,
				  PangoCoverage *coverage)
{
  PangoFT2FontMap *ft2fontmap = (PangoFT2FontMap *)fontmap;

  g_hash_table_insert (ft2fontmap->coverage_hash, g_strdup (name),
		       pango_coverage_ref (coverage));
}

PangoCoverage *
_pango_ft2_font_map_get_coverage (PangoFontMap  *fontmap,
				  const char    *name)
{
  PangoFT2FontMap *ft2fontmap = (PangoFT2FontMap *)fontmap;

  return g_hash_table_lookup (ft2fontmap->coverage_hash, name);
}

FT_Library
_pango_ft2_font_map_get_library (PangoFontMap *fontmap)
{
  PangoFT2FontMap *ft2fontmap = (PangoFT2FontMap *)fontmap;
  
  return ft2fontmap->library;
}

/*
 * PangoFT2FontFamily
 */
static PangoFT2Face *
create_face (PangoFT2Family *ft2family,
 	     const char     *style)
{
  PangoFT2Face *face = g_object_new (PANGO_FT2_TYPE_FACE, NULL);
  face->style = g_strdup (style);
  face->family = ft2family;

  return face;
}
 
static void
pango_ft2_family_list_faces (PangoFontFamily  *family,
			     PangoFontFace  ***faces,
			     int              *n_faces)
{
  PangoFT2Family *ft2family = PANGO_FT2_FAMILY (family);

  if (ft2family->n_faces < 0)
    {
      MiniXftFontSet *fontset;
      int i;

      if (is_alias_family (ft2family->family_name))
  	{
 	  ft2family->n_faces = 4;
 	  ft2family->faces = g_new (PangoFT2Face *, ft2family->n_faces);
 
 	  i = 0;
 	  ft2family->faces[i++] = create_face (ft2family, "Regular");
 	  ft2family->faces[i++] = create_face (ft2family, "Bold");
 	  ft2family->faces[i++] = create_face (ft2family, "Italic");
 	  ft2family->faces[i++] = create_face (ft2family, "Bold Italic");
	}
      else
	{
	  fontset = MiniXftListFonts ((Display *)1, 0,
				      XFT_ENCODING, MiniXftTypeString, "iso10646-1",
				      XFT_FAMILY, MiniXftTypeString, ft2family->family_name,
				      XFT_CORE, MiniXftTypeBool, False,
				      NULL,
				      XFT_STYLE,
				      NULL);
	  
	  ft2family->n_faces = fontset->nfont;
	  ft2family->faces = g_new (PangoFT2Face *, ft2family->n_faces);
	  
	  for (i = 0; i < fontset->nfont; i++)
	    {
	      char *s;
	      MiniXftResult res;
	      
	      res = MiniXftPatternGetString (fontset->fonts[i], XFT_STYLE, 0, &s);
	      g_assert (res == MiniXftResultMatch);
	      
	      ft2family->faces[i] = create_face (ft2family, s);
	    }
	  
	  MiniXftFontSetDestroy (fontset);
	}
    }
  
  if (n_faces)
    *n_faces = ft2family->n_faces;
  
  if (faces)
    *faces = g_memdup (ft2family->faces, ft2family->n_faces * sizeof (PangoFontFace *));
}

const char *
pango_ft2_family_get_name (PangoFontFamily  *family)
{
  PangoFT2Family *ft2family = PANGO_FT2_FAMILY (family);
  return ft2family->family_name;
}

static void
pango_ft2_family_class_init (PangoFontFamilyClass *class)
{
  class->list_faces = pango_ft2_family_list_faces;
  class->get_name = pango_ft2_family_get_name;
}

GType
pango_ft2_family_get_type (void)
{
  static GType object_type = 0;

  if (!object_type)
    {
      static const GTypeInfo object_info =
      {
        sizeof (PangoFontFamilyClass),
        (GBaseInitFunc) NULL,
        (GBaseFinalizeFunc) NULL,
        (GClassInitFunc) pango_ft2_family_class_init,
        NULL,           /* class_finalize */
        NULL,           /* class_data */
        sizeof (PangoFT2Family),
        0,              /* n_preallocs */
        (GInstanceInitFunc) NULL,
      };
      
      object_type = g_type_register_static (PANGO_TYPE_FONT_FAMILY,
                                            "PangoFT2Family",
                                            &object_info, 0);
    }
  
  return object_type;
}
