/* Pango
 * pangoatsui-fontmap.c
 *
 * Copyright (C) 2000-2003 Red Hat, Inc.
 * Copyright (C) 2005-2007 Imendio AB
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

#include "pango-fontmap.h"
#include "pangoatsui-private.h"
#include "pango-impl-utils.h"
#include "modules.h"

#import <Cocoa/Cocoa.h>

typedef struct _FontHashKey      FontHashKey;

struct _PangoATSUIFamily
{
  PangoFontFamily parent_instance;

  char *family_name;

  PangoFontFace **faces;
  gint n_faces;
};

#define PANGO_TYPE_ATSUI_FAMILY              (pango_atsui_family_get_type ())
#define PANGO_ATSUI_FAMILY(object)           (G_TYPE_CHECK_INSTANCE_CAST ((object), PANGO_TYPE_ATSUI_FAMILY, PangoATSUIFamily))
#define PANGO_IS_ATSUI_FAMILY(object)        (G_TYPE_CHECK_INSTANCE_TYPE ((object), PANGO_TYPE_ATSUI_FAMILY))

#define PANGO_TYPE_ATSUI_FACE              (pango_atsui_face_get_type ())
#define PANGO_ATSUI_FACE(object)           (G_TYPE_CHECK_INSTANCE_CAST ((object), PANGO_TYPE_ATSUI_FACE, PangoATSUIFace))
#define PANGO_IS_ATSUI_FACE(object)        (G_TYPE_CHECK_INSTANCE_TYPE ((object), PANGO_TYPE_ATSUI_FACE))

struct _PangoATSUIFace
{
  PangoFontFace parent_instance;

  PangoATSUIFamily *family;

  PangoCoverage *coverage;

  char *postscript_name;
  char *style_name;

  int weight;
  int traits;
  guint synthetic_italic : 1;
};

static GType pango_atsui_family_get_type (void);
static GType pango_atsui_face_get_type (void);

static gpointer pango_atsui_family_parent_class;
static gpointer pango_atsui_face_parent_class;

static const char *
get_real_family (const char *family_name)
{
  switch (family_name[0])
    {
    case 'm':
    case 'M':
      if (g_ascii_strcasecmp (family_name, "monospace") == 0)
	return "Courier";
      break;
    case 's':
    case 'S':
      if (g_ascii_strcasecmp (family_name, "sans") == 0)
	return "Helvetica";
      else if (g_ascii_strcasecmp (family_name, "serif") == 0)
	return "Times";
      break;
    }

  return family_name;
}

static void
pango_atsui_family_list_faces (PangoFontFamily  *family,
			       PangoFontFace  ***faces,
			       int              *n_faces)
{
  PangoATSUIFamily *atsuifamily = PANGO_ATSUI_FAMILY (family);

  if (atsuifamily->n_faces < 0)
    {
      NSAutoreleasePool *pool = [[NSAutoreleasePool alloc] init];
      const char *real_family = get_real_family (atsuifamily->family_name);
      NSFontManager *manager = [NSFontManager sharedFontManager];
      NSArray *members = [manager availableMembersOfFontFamily:[NSString stringWithUTF8String:real_family]];
      int i, count;
      GHashTable *hash_table;
      GList *faces = NULL, *l;
      GList *synthetic_faces = NULL;

      /* The NSFontManager API returns italic faces for some families
       * even if they don't exist. When using Cocoa to create
       * instances of those fonts, Cocoa synthesizes italic versions
       * by applying a shear transformation. We do that manually for
       * those fonts in pangocairo-atsuifont.c. For many other fonts,
       * there is no italic face at all, so we create synthesized
       * versions of those like in the win32 and fontconfig backends.
       */
      hash_table = g_hash_table_new (g_direct_hash, g_direct_equal);

      count = [members count];
      for (i = 0; i < count; i++)
	{
	  PangoATSUIFace *face = g_object_new (PANGO_TYPE_ATSUI_FACE, NULL);
	  NSArray *font_array = [members objectAtIndex:i];

	  face->family = atsuifamily;
	  face->postscript_name = g_strdup ([[font_array objectAtIndex:0] UTF8String]);
	  face->style_name = g_strdup ([[font_array objectAtIndex:1] UTF8String]);
	  face->weight = [[font_array objectAtIndex:2] intValue];
	  face->traits = [[font_array objectAtIndex:3] intValue];

	  faces = g_list_prepend (faces, face);

	  if (face->traits & NSItalicFontMask)
	    g_hash_table_insert (hash_table, GINT_TO_POINTER (face->weight), face);
	}

      for (l = faces; l; l = l->next)
	{
	  PangoATSUIFace *face = l->data;

	  if (!g_hash_table_lookup (hash_table, GINT_TO_POINTER (face->weight)))
	    {
	      PangoATSUIFace *italic_face = g_object_new (PANGO_TYPE_ATSUI_FACE, NULL);

	      italic_face->family = atsuifamily;
	      italic_face->postscript_name = g_strdup (face->postscript_name);
	      italic_face->weight = face->weight;
	      italic_face->traits = face->traits | NSItalicFontMask;
	      italic_face->synthetic_italic = TRUE;

	      /* Try to create a sensible face name. */
	      if (strcasecmp (face->style_name, "regular") == 0)
		italic_face->style_name = g_strdup ("Oblique");
	      else 
		italic_face->style_name = g_strdup_printf ("%s Oblique", face->style_name);

	      synthetic_faces = g_list_prepend (synthetic_faces, italic_face);
	    }
	}

      faces = g_list_concat (faces, synthetic_faces);

      atsuifamily->n_faces = g_list_length (faces);
      atsuifamily->faces = g_new (PangoFontFace *, atsuifamily->n_faces);

      for (l = faces, i = 0; l; l = l->next, i++)
	atsuifamily->faces[i] = l->data;

      g_list_free (faces);
      g_hash_table_destroy (hash_table);

      [pool release];
    }

  if (n_faces)
    *n_faces = atsuifamily->n_faces;

  if (faces)
    *faces = g_memdup (atsuifamily->faces, atsuifamily->n_faces * sizeof (PangoFontFace *));
}

static const char *
pango_atsui_family_get_name (PangoFontFamily *family)
{
  PangoATSUIFamily *atsuifamily = PANGO_ATSUI_FAMILY (family);

  return atsuifamily->family_name;
}

static gboolean
pango_atsui_family_is_monospace (PangoFontFamily *family)
{
  /* Fixme: Implement */
  return FALSE;
}

static void
pango_atsui_family_finalize (GObject *object)
{
  PangoATSUIFamily *family = PANGO_ATSUI_FAMILY (object);
  int i;

  g_free (family->family_name);

  if (family->n_faces != -1)
    {
      for (i = 0; i < family->n_faces; i++)
	g_object_unref (family->faces[i]);

      g_free (family->faces);
    }

  G_OBJECT_CLASS (pango_atsui_family_parent_class)->finalize (object);
}

static void
pango_atsui_family_class_init (PangoFontFamilyClass *class)
{
  GObjectClass *object_class = (GObjectClass *)class;
  int i;

  pango_atsui_family_parent_class = g_type_class_peek_parent (class);

  object_class->finalize = pango_atsui_family_finalize;

  class->list_faces = pango_atsui_family_list_faces;
  class->get_name = pango_atsui_family_get_name;
  class->is_monospace = pango_atsui_family_is_monospace;

  for (i = 0; _pango_included_atsui_modules[i].list; i++)
    pango_module_register (&_pango_included_atsui_modules[i]);
}

static void
pango_atsui_family_init (PangoATSUIFamily *family)
{
  family->n_faces = -1;
}

static GType
pango_atsui_family_get_type (void)
{
  static GType object_type = 0;

  if (G_UNLIKELY (!object_type))
    {
      const GTypeInfo object_info =
      {
	sizeof (PangoFontFamilyClass),
	(GBaseInitFunc) NULL,
	(GBaseFinalizeFunc) NULL,
	(GClassInitFunc) pango_atsui_family_class_init,
	NULL,           /* class_finalize */
	NULL,           /* class_data */
	sizeof (PangoATSUIFamily),
	0,              /* n_preallocs */
	(GInstanceInitFunc) pango_atsui_family_init,
      };

      object_type = g_type_register_static (PANGO_TYPE_FONT_FAMILY,
					    I_("PangoATSUIFamily"),
					    &object_info, 0);
    }

  return object_type;
}

static PangoFontDescription *
pango_atsui_face_describe (PangoFontFace *face)
{
  PangoATSUIFace *atsuiface = PANGO_ATSUI_FACE (face);
  PangoFontDescription *description;
  PangoWeight pango_weight;
  PangoStyle pango_style;
  PangoVariant pango_variant;
  int weight;

  description = pango_font_description_new ();

  pango_font_description_set_family (description, atsuiface->family->family_name);

  weight = atsuiface->weight;

  if (weight == 1 || weight == 2)
    pango_weight = PANGO_WEIGHT_ULTRALIGHT;
  else if (weight == 3 || weight == 4)
    pango_weight = PANGO_WEIGHT_LIGHT;
  else if (weight == 5 || weight == 6)
    pango_weight = PANGO_WEIGHT_NORMAL;
  else if (weight == 7 || weight == 8)
    pango_weight = PANGO_WEIGHT_SEMIBOLD;
  else if (weight == 9 || weight == 10)
    pango_weight = PANGO_WEIGHT_BOLD;
  else if (weight == 11 || weight == 12)
    pango_weight = PANGO_WEIGHT_ULTRABOLD;
  else if (weight == 13 || weight == 14)
    pango_weight = PANGO_WEIGHT_HEAVY;
  else
    g_assert_not_reached ();

  if (atsuiface->traits & NSItalicFontMask)
    pango_style = PANGO_STYLE_ITALIC;
  else
    pango_style = PANGO_STYLE_NORMAL;

  if (atsuiface->traits & NSSmallCapsFontMask)
    pango_variant = PANGO_VARIANT_SMALL_CAPS;
  else
    pango_variant = PANGO_VARIANT_NORMAL;

  pango_font_description_set_weight (description, pango_weight);
  pango_font_description_set_style (description, pango_style);
  pango_font_description_set_variant (description, pango_variant);

  return description;
}

static const char *
pango_atsui_face_get_face_name (PangoFontFace *face)
{
  PangoATSUIFace *atsuiface = PANGO_ATSUI_FACE (face);

  return atsuiface->style_name;
}

static void
pango_atsui_face_list_sizes (PangoFontFace  *face,
			     int           **sizes,
			     int            *n_sizes)
{
  *n_sizes = 0;
  *sizes = NULL;
}

static void
pango_atsui_face_finalize (GObject *object)
{
  PangoATSUIFace *atsuiface = PANGO_ATSUI_FACE (object);

  if (atsuiface->coverage)
    pango_coverage_unref (atsuiface->coverage);

  g_free (atsuiface->postscript_name);
  g_free (atsuiface->style_name);

  G_OBJECT_CLASS (pango_atsui_face_parent_class)->finalize (object);
}

static gboolean
pango_atsui_face_is_synthesized (PangoFontFace *face)
{
  PangoATSUIFace *atsuiface = PANGO_ATSUI_FACE (face);

  return atsuiface->synthetic_italic;
}

static void
pango_atsui_face_class_init (PangoFontFaceClass *class)
{
  GObjectClass *object_class = (GObjectClass *)class;

  pango_atsui_face_parent_class = g_type_class_peek_parent (class);

  object_class->finalize = pango_atsui_face_finalize;

  class->describe = pango_atsui_face_describe;
  class->get_face_name = pango_atsui_face_get_face_name;
  class->list_sizes = pango_atsui_face_list_sizes;
  class->is_synthesized = pango_atsui_face_is_synthesized;
}

GType
pango_atsui_face_get_type (void)
{
  static GType object_type = 0;

  if (G_UNLIKELY (!object_type))
    {
      const GTypeInfo object_info =
      {
	sizeof (PangoFontFaceClass),
	(GBaseInitFunc) NULL,
	(GBaseFinalizeFunc) NULL,
	(GClassInitFunc) pango_atsui_face_class_init,
	NULL,           /* class_finalize */
	NULL,           /* class_data */
	sizeof (PangoATSUIFace),
	0,              /* n_preallocs */
	(GInstanceInitFunc) NULL,
      };

      object_type = g_type_register_static (PANGO_TYPE_FONT_FACE,
					    I_("PangoATSUIFace"),
					    &object_info, 0);
    }

  return object_type;
}

const char *
_pango_atsui_face_get_postscript_name (PangoATSUIFace *face)
{
  return face->postscript_name;
}

gboolean
_pango_atsui_face_get_synthetic_italic (PangoATSUIFace *face)
{
  return face->synthetic_italic;
}

PangoCoverage *
_pango_atsui_face_get_coverage (PangoATSUIFace *face,
				PangoLanguage  *language)
{
  int i;

  /* Note: We currently fake the coverage by reporting that the 255 first 
   * glyphs exist in all fonts, which is not true. It's unclear how to get
   * this done correctly.
   */

  if (face->coverage)
    return face->coverage;

  face->coverage = pango_coverage_new ();

  for (i = 0; i < 256; i++)
    pango_coverage_set (face->coverage, i, PANGO_COVERAGE_EXACT);

  return face->coverage;
}

static void pango_atsui_font_map_class_init (PangoATSUIFontMapClass *class);
static void pango_atsui_font_map_init (PangoATSUIFontMap *atsuifontmap);

static guint    font_hash_key_hash  (const FontHashKey *key);
static gboolean font_hash_key_equal (const FontHashKey *key_a,
				     const FontHashKey *key_b);
static void     font_hash_key_free  (FontHashKey       *key);

G_DEFINE_TYPE (PangoATSUIFontMap, pango_atsui_font_map, PANGO_TYPE_FONT_MAP);

static void
pango_atsui_font_map_finalize (GObject *object)
{
  PangoATSUIFontMap *fontmap = PANGO_ATSUI_FONT_MAP (object);

  g_hash_table_destroy (fontmap->font_hash);
  g_hash_table_destroy (fontmap->families);

  G_OBJECT_CLASS (pango_atsui_font_map_parent_class)->finalize (object);
}

struct _FontHashKey {
  PangoATSUIFontMap *fontmap;
  PangoMatrix matrix;
  PangoFontDescription *desc;
  char *postscript_name;
  gpointer context_key;
};

/* Fowler / Noll / Vo (FNV) Hash (http://www.isthe.com/chongo/tech/comp/fnv/)
 *
 * Not necessarily better than a lot of other hashes, but should be OK, and
 * well tested with binary data.
 */

#define FNV_32_PRIME ((guint32)0x01000193)
#define FNV1_32_INIT ((guint32)0x811c9dc5)

static guint32
hash_bytes_fnv (unsigned char *buffer,
		int            len,
		guint32        hval)
{
  while (len--)
    {
      hval *= FNV_32_PRIME;
      hval ^= *buffer++;
    }

  return hval;
}

static gboolean
font_hash_key_equal (const FontHashKey *key_a,
		     const FontHashKey *key_b)
{
  if (key_a->matrix.xx == key_b->matrix.xx &&
      key_a->matrix.xy == key_b->matrix.xy &&
      key_a->matrix.yx == key_b->matrix.yx &&
      key_a->matrix.yy == key_b->matrix.yy &&
      pango_font_description_equal (key_a->desc, key_b->desc) &&
      strcmp (key_a->postscript_name, key_b->postscript_name) == 0)
    {
      if (key_a->context_key)
	return PANGO_ATSUI_FONT_MAP_GET_CLASS (key_a->fontmap)->context_key_equal (key_a->fontmap,
										key_a->context_key,
										key_b->context_key);
      else
	return TRUE;
    }
  else
    return FALSE;
}

static guint
font_hash_key_hash (const FontHashKey *key)
{
    guint32 hash = FNV1_32_INIT;

    /* We do a bytewise hash on the context matrix */
    hash = hash_bytes_fnv ((unsigned char *)(&key->matrix),
			   sizeof(double) * 4,
			   hash);

    if (key->context_key)
      hash ^= PANGO_ATSUI_FONT_MAP_GET_CLASS (key->fontmap)->context_key_hash (key->fontmap,
									       key->context_key);

    hash ^= g_str_hash (key->postscript_name);

    return (hash ^ pango_font_description_hash (key->desc));
}

static void
font_hash_key_free (FontHashKey *key)
{
  if (key->context_key)
    PANGO_ATSUI_FONT_MAP_GET_CLASS (key->fontmap)->context_key_free (key->fontmap,
								     key->context_key);

  g_slice_free (FontHashKey, key);
}

static FontHashKey *
font_hash_key_copy (FontHashKey *old)
{
  FontHashKey *key = g_slice_new (FontHashKey);

  key->fontmap = old->fontmap;
  key->matrix = old->matrix;
  key->desc = pango_font_description_copy (old->desc);
  key->postscript_name = g_strdup (old->postscript_name);
  if (old->context_key)
    key->context_key = PANGO_ATSUI_FONT_MAP_GET_CLASS (key->fontmap)->context_key_copy (key->fontmap,
											old->context_key);
  else
    key->context_key = NULL;

  return key;
}


static void
get_context_matrix (PangoContext *context,
		    PangoMatrix *matrix)
{
  const PangoMatrix *set_matrix;
  static const PangoMatrix identity = PANGO_MATRIX_INIT;

  if (context)
    set_matrix = pango_context_get_matrix (context);
  else
    set_matrix = NULL;

  if (set_matrix)
    *matrix = *set_matrix;
  else
    *matrix = identity;
}

static void
font_hash_key_for_context (PangoATSUIFontMap *fcfontmap,
			   PangoContext   *context,
			   FontHashKey    *key)
{
  key->fontmap = fcfontmap;
  get_context_matrix (context, &key->matrix);

  if (PANGO_ATSUI_FONT_MAP_GET_CLASS (fcfontmap)->context_key_get)
    key->context_key = (gpointer)PANGO_ATSUI_FONT_MAP_GET_CLASS (fcfontmap)->context_key_get (fcfontmap, context);
  else
    key->context_key = NULL;
}

static void
pango_atsui_font_map_add (PangoATSUIFontMap *atsuifontmap,
			  PangoContext      *context,
			  PangoATSUIFont    *atsuifont)
{
  FontHashKey key;
  FontHashKey *key_copy;
  PangoATSUIFace *face;

  _pango_atsui_font_set_font_map (atsuifont, atsuifontmap);

  font_hash_key_for_context (atsuifontmap, context, &key);
  face = _pango_atsui_font_get_face (atsuifont);
  key.postscript_name = (char *)_pango_atsui_face_get_postscript_name (face);
  key.desc = _pango_atsui_font_get_font_description (atsuifont);

  key_copy = font_hash_key_copy (&key);
  _pango_atsui_font_set_context_key (atsuifont, key_copy->context_key);
  g_hash_table_insert (atsuifontmap->font_hash, key_copy, g_object_ref (atsuifont));
}

static PangoATSUIFont *
pango_atsui_font_map_lookup (PangoATSUIFontMap    *atsuifontmap,
			     PangoContext         *context,
			     PangoFontDescription *desc,
			     PangoATSUIFace       *face)
{
  FontHashKey key;

  font_hash_key_for_context (atsuifontmap, context, &key);
  key.postscript_name = (char *)_pango_atsui_face_get_postscript_name (face);
  key.desc = desc;

  return g_hash_table_lookup (atsuifontmap->font_hash, &key);
}

static gboolean
find_best_match (PangoATSUIFamily            *font_family,
		 const PangoFontDescription  *description,
		 PangoFontDescription       **best_description,
		 PangoATSUIFace             **best_face)
{
  PangoFontDescription *new_desc;
  int i;

  *best_description = NULL;
  *best_face = NULL;

  for (i = 0; i < font_family->n_faces; i++)
    {
      new_desc = pango_font_face_describe (font_family->faces[i]);

      if (pango_font_description_better_match (description, *best_description, new_desc))
	{
	  pango_font_description_free (*best_description);
	  *best_description = new_desc;
	  *best_face = (PangoATSUIFace *)font_family->faces[i];
	}
      else
	pango_font_description_free (new_desc);
    }

  if (*best_description)
    return TRUE;

  return FALSE;
}

static PangoFont *
pango_atsui_font_map_load_font (PangoFontMap               *fontmap,
				PangoContext               *context,
				const PangoFontDescription *description)
{
  PangoATSUIFontMap *atsuifontmap = (PangoATSUIFontMap *)fontmap;
  PangoATSUIFamily *font_family;
  gchar *name;
  gint size;

  size = pango_font_description_get_size (description);
  if (size < 0)
    return NULL;

  name = g_utf8_casefold (pango_font_description_get_family (description), -1);
  font_family = g_hash_table_lookup (atsuifontmap->families, name);
  g_free (name);

  if (font_family)
    {
      PangoFontDescription *best_description;
      PangoATSUIFace *best_face;
      PangoATSUIFont *best_font;

      /* Force a listing of the available faces */
      pango_font_family_list_faces ((PangoFontFamily *)font_family, NULL, NULL);

      if (!find_best_match (font_family, description, &best_description, &best_face))
	return NULL;
      
      pango_font_description_set_size (best_description, size);

      best_font = pango_atsui_font_map_lookup (atsuifontmap, 
					       context, 
					       best_description, 
					       best_face);

      if (best_font)
	g_object_ref (best_font);
      else
	{
	  PangoATSUIFontMapClass *klass;

	  klass = PANGO_ATSUI_FONT_MAP_GET_CLASS (atsuifontmap);
	  best_font = klass->create_font (atsuifontmap, context, 
					  best_face, best_description);
	  
	  if (best_font)
	    pango_atsui_font_map_add (atsuifontmap, context, best_font);
	    /* TODO: Handle the else case here. */
	}

      pango_font_description_free (best_description);

      return (PangoFont *)best_font;
    }

  return NULL;
}

static void
list_families_foreach (gpointer key,
		       gpointer value,
		       gpointer user_data)
{
  GSList **list = user_data;

  *list = g_slist_prepend (*list, value);
}

static void
pango_atsui_font_map_list_families (PangoFontMap      *fontmap,
				    PangoFontFamily ***families,
				    int               *n_families)
{
  GSList *family_list = NULL;
  GSList *tmp_list;
  PangoATSUIFontMap *atsuifontmap = (PangoATSUIFontMap *)fontmap;

  if (!n_families)
    return;

  g_hash_table_foreach (atsuifontmap->families, list_families_foreach, &family_list);

  *n_families = g_slist_length (family_list);

  if (families)
    {
      int i = 0;

      *families = g_new (PangoFontFamily *, *n_families);

      tmp_list = family_list;
      while (tmp_list)
	{
	  (*families)[i] = tmp_list->data;
	  i++;
	  tmp_list = tmp_list->next;
	}
    }

  g_slist_free (family_list);
}

static void
pango_atsui_font_map_init (PangoATSUIFontMap *atsuifontmap)
{
  NSArray *family_array;
  NSAutoreleasePool *pool;
  PangoATSUIFamily *family;

  int size, i;

  atsuifontmap->families = g_hash_table_new_full (g_str_hash, g_str_equal,
						  g_free, g_object_unref);


  atsuifontmap->font_hash = g_hash_table_new_full ((GHashFunc)font_hash_key_hash,
						   (GEqualFunc)font_hash_key_equal,
						   (GDestroyNotify)font_hash_key_free,
						   NULL);

  pool = [[NSAutoreleasePool alloc] init];
  family_array = [[NSFontManager sharedFontManager] availableFontFamilies];
  size = [family_array count];

  for (i = 0; i < size; i++) {
    NSString *family_name = [family_array objectAtIndex:i];

    family = g_object_new (PANGO_TYPE_ATSUI_FAMILY, NULL);
    family->family_name = g_strdup ([family_name UTF8String]);

    g_hash_table_insert (atsuifontmap->families, g_utf8_casefold (family->family_name, -1), family);
  }

  /* Insert aliases */
  family = g_object_new (PANGO_TYPE_ATSUI_FAMILY, NULL);
  family->family_name = g_strdup ("Sans");
  g_hash_table_insert (atsuifontmap->families, g_utf8_casefold (family->family_name, -1), family);

  family = g_object_new (PANGO_TYPE_ATSUI_FAMILY, NULL);
  family->family_name = g_strdup ("Serif");
  g_hash_table_insert (atsuifontmap->families, g_utf8_casefold (family->family_name, -1), family);

  family = g_object_new (PANGO_TYPE_ATSUI_FAMILY, NULL);
  family->family_name = g_strdup ("Monospace");
  g_hash_table_insert (atsuifontmap->families, g_utf8_casefold (family->family_name, -1), family);

  [pool release];
}

static void
pango_atsui_font_map_class_init (PangoATSUIFontMapClass *class)
{
  GObjectClass *object_class = G_OBJECT_CLASS (class);
  PangoFontMapClass *fontmap_class = PANGO_FONT_MAP_CLASS (class);

  object_class->finalize = pango_atsui_font_map_finalize;

  fontmap_class->load_font = pango_atsui_font_map_load_font;
  fontmap_class->list_families = pango_atsui_font_map_list_families;
  fontmap_class->shape_engine_type = PANGO_RENDER_TYPE_ATSUI;
}


