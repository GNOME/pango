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

#include <pango/pango-coverage.h>
#include <pango/pango-types.h>

#include <glib-object.h>

typedef struct _PangoFontDescription PangoFontDescription;
typedef struct _PangoFontMetrics PangoFontMetrics;

/**
 * PangoStyle:
 * @PANGO_STYLE_NORMAL: the font is upright.
 * @PANGO_STYLE_OBLIQUE: the font is slanted, but in a roman style.
 * @PANGO_STYLE_ITALIC: the font is slanted in an italic style.
 *
 * An enumeration specifying the various slant styles possible for a font.
 **/
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

struct _PangoFontMetrics
{
  int ascent;
  int descent;
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

typedef struct _PangoFontClass PangoFontClass;

#define PANGO_TYPE_FONT              (pango_font_get_type ())
#define PANGO_FONT(object)           (G_TYPE_CHECK_INSTANCE_CAST ((object), PANGO_TYPE_FONT, PangoFont))
#define PANGO_FONT_CLASS(klass)      (G_TYPE_CHECK_CLASS_CAST ((klass), PANGO_TYPE_FONT, PangoFontClass))
#define PANGO_IS_FONT(object)        (G_TYPE_CHECK_INSTANCE_TYPE ((object), PANGO_TYPE_FONT))
#define PANGO_IS_FONT_CLASS(klass)   (G_TYPE_CHECK_CLASS_TYPE ((klass), PANGO_TYPE_FONT))
#define PANGO_FONT_GET_CLASS(obj)    (G_TYPE_INSTANCE_GET_CLASS ((obj), PANGO_TYPE_FONT, PangoFontClass))

struct _PangoFont
{
  GObject parent_instance;
};

struct _PangoFontClass
{
  GObjectClass parent_class;
  
  PangoFontDescription *(*describe)           (PangoFont      *font);
  PangoCoverage *       (*get_coverage)       (PangoFont      *font,
					      const char      *lang);
  PangoEngineShape *    (*find_shaper)        (PangoFont      *font,
					       const char     *lang,
					       guint32         ch);
  void                  (*get_glyph_extents)  (PangoFont      *font,
					       PangoGlyph      glyph,
					       PangoRectangle *ink_rect,
					       PangoRectangle *logical_rect);
  void                  (*get_metrics)        (PangoFont        *font,
					       const gchar      *lang,
					       PangoFontMetrics *metrics);
};

GType                 pango_font_get_type          (void);

PangoFontDescription *pango_font_describe          (PangoFont        *font);
PangoCoverage *       pango_font_get_coverage      (PangoFont        *font,
						    const char       *lang);
PangoEngineShape *    pango_font_find_shaper       (PangoFont        *font,
						    const char       *lang,
						    guint32           ch);
void                  pango_font_get_metrics       (PangoFont        *font,
						    const gchar      *lang,
						    PangoFontMetrics *metrics);
void                  pango_font_get_glyph_extents (PangoFont        *font,
						    PangoGlyph        glyph,
						    PangoRectangle   *ink_rect,
						    PangoRectangle   *logical_rect);

#ifdef __cplusplus
}
#endif /* __cplusplus */

#endif __PANGO_FONT_H__
