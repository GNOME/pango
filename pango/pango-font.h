/* Pango
 * pango-font.h: Font handling
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

#ifndef __PANGO_FONT_H__
#define __PANGO_FONT_H__

#ifdef __cplusplus
extern "C" {
#endif /* __cplusplus */

#include <pango-coverage.h>
#include <pango-types.h>

typedef struct _PangoFontDescription PangoFontDescription;
typedef struct _PangoFontClass PangoFontClass;
typedef struct _PangoFontMap PangoFontMap;
typedef struct _PangoFontMapClass PangoFontMapClass;

typedef enum {
  PANGO_STYLE_NORMAL,
  PANGO_STYLE_OBLIQUE,
  PANGO_STYLE_ITALIC
} PangoStyle;

typedef enum {
  PANGO_VARIANT_NORMAL,
  PANGO_VARIANT_SMALL_CAPS
} PangoVariant;

typedef enum {
  PANGO_WEIGHT_NORMAL = 400,
  PANGO_WEIGHT_BOLD = 700
} PangoWeight;

typedef enum {
  PANGO_STRETCH_ULTRA_CONDENSED,
  PANGO_STRETCH_EXTRA_CONDENSED,
  PANGO_STRETCH_CONDENSED,
  PANGO_STRETCH_SEMI_CONDENSED,
  PANGO_STRETCH_NORMAL,
  PANGO_STRETCH_SEMI_EXPANDED,
  PANGO_STRETCH_EXPANDED,
  PANGO_STRETCH_EXTRA_EXPANDED,
  PANGO_STRETCH_ULTRA_EXPANDED
} PangoStretch;

struct _PangoFontDescription
{
  char *family_name;

  PangoStyle style;
  PangoVariant variant;
  PangoWeight weight;
  PangoStretch stretch;

  int size;
};

PangoFontDescription *pango_font_description_copy        (const PangoFontDescription  *desc);
gboolean              pango_font_description_compare     (const PangoFontDescription  *desc1,
							  const PangoFontDescription  *desc2);
void                  pango_font_description_free        (PangoFontDescription        *desc);
void                  pango_font_descriptions_free       (PangoFontDescription       **descs,
							  int                          n_descs);

PangoFontDescription *pango_font_description_from_string (const char                  *str);
char *                pango_font_description_to_string   (const PangoFontDescription  *desc);


/* Logical fonts
  */
struct _PangoFont
{
  PangoFontClass *klass;

  /*< private >*/
  gint ref_count;
  GData *data;
};

struct _PangoFontClass
{
  void                  (*destroy)      (PangoFont   *font);
  PangoFontDescription *(*describe)     (PangoFont   *font);
  PangoCoverage *       (*get_coverage) (PangoFont   *font,
					 const char  *lang);
  PangoEngineShape *    (*find_shaper)  (PangoFont   *font,
					 const char  *lang,
					 guint32      ch);
};

void                  pango_font_init         (PangoFont      *font);
void                  pango_font_ref          (PangoFont      *font);
void                  pango_font_unref        (PangoFont      *font);
gpointer              pango_font_get_data     (PangoFont      *font,
					       gchar          *key);
void                  pango_font_set_data     (PangoFont      *font,
					       gchar          *key,
					       gpointer        data,
					       GDestroyNotify  destroy_func);

PangoFontDescription *pango_font_describe     (PangoFont      *font);
PangoCoverage *       pango_font_get_coverage (PangoFont      *font,
					       const char     *lang);
PangoEngineShape *    pango_font_find_shaper  (PangoFont      *font,
					       const char     *lang,
					       guint32         ch);

/*
 * Font Map
 */

struct _PangoFontMap
{
  PangoFontMapClass *klass;

  /*< private >*/
  gint ref_count;
};

struct _PangoFontMapClass
{
  void       (*destroy)       (PangoFontMap               *fontmap);
  PangoFont *(*load_font)     (PangoFontMap               *fontmap,
			       const PangoFontDescription *desc);
  void       (*list_fonts)    (PangoFontMap               *fontmap,
			       const gchar                *family,
			       PangoFontDescription     ***descs,
			       int                        *n_descs);
  void       (*list_families) (PangoFontMap               *fontmap,
			       gchar                    ***families,
			       int                        *n_families);
};

void       pango_font_map_init      (PangoFontMap               *fontmap);
void       pango_font_map_ref       (PangoFontMap               *fontmap);
void       pango_font_map_unref     (PangoFontMap               *fontmap);
PangoFont *pango_font_map_load_font (PangoFontMap               *fontmap,
				     const PangoFontDescription *desc);

void       pango_font_map_list_fonts    (PangoFontMap           *fontmap,
					 const gchar            *family,
					 PangoFontDescription ***descs,
					 int                    *n_descs);
void       pango_font_map_list_families (PangoFontMap           *fontmap,
					 gchar                ***families,
					 int                    *n_families);
void       pango_font_map_free_families (gchar                 **families,
					 int                     n_families);

#ifdef __cplusplus
}
#endif /* __cplusplus */

#endif __PANGO_FONT_H__
