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

#ifndef __PANGO_FONT_PRIVATE_H__
#define __PANGO_FONT_PRIVATE_H__

#include <pango/pango-font.h>
#include <pango/pango-coverage.h>
#include <pango/pango-types.h>

#include <glib-object.h>

G_BEGIN_DECLS

PANGO_AVAILABLE_IN_ALL
PangoFontMetrics *pango_font_metrics_new (void);

struct _PangoFontMetrics
{
  /* <private> */
  guint ref_count;

  int ascent;
  int descent;
  int height;
  int approximate_char_width;
  int approximate_digit_width;
  int underline_position;
  int underline_thickness;
  int strikethrough_position;
  int strikethrough_thickness;
};


#define PANGO_FONT_FAMILY_CLASS(klass)      (G_TYPE_CHECK_CLASS_CAST ((klass), PANGO_TYPE_FONT_FAMILY, PangoFontFamilyClass))
#define PANGO_IS_FONT_FAMILY_CLASS(klass)   (G_TYPE_CHECK_CLASS_TYPE ((klass), PANGO_TYPE_FONT_FAMILY))
#define PANGO_FONT_FAMILY_GET_CLASS(obj)    (G_TYPE_INSTANCE_GET_CLASS ((obj), PANGO_TYPE_FONT_FAMILY, PangoFontFamilyClass))

typedef struct _PangoFontFamilyClass PangoFontFamilyClass;


/**
 * PangoFontFamily:
 *
 * The #PangoFontFamily structure is used to represent a family of related
 * font faces. The faces in a family share a common design, but differ in
 * slant, weight, width and other aspects.
 */
struct _PangoFontFamily
{
  GObject parent_instance;
};

struct _PangoFontFamilyClass
{
  GObjectClass parent_class;

  /*< public >*/

  void  (*list_faces)      (PangoFontFamily  *family,
			    PangoFontFace  ***faces,
			    int              *n_faces);
  const char * (*get_name) (PangoFontFamily  *family);
  gboolean (*is_monospace) (PangoFontFamily *family);
  gboolean (*is_variable)  (PangoFontFamily *family);

  /*< private >*/

  /* Padding for future expansion */
  void (*_pango_reserved2) (void);
  void (*_pango_reserved3) (void);
};


#define PANGO_FONT_FACE_CLASS(klass)      (G_TYPE_CHECK_CLASS_CAST ((klass), PANGO_TYPE_FONT_FACE, PangoFontFaceClass))
#define PANGO_IS_FONT_FACE_CLASS(klass)   (G_TYPE_CHECK_CLASS_TYPE ((klass), PANGO_TYPE_FONT_FACE))
#define PANGO_FONT_FACE_GET_CLASS(obj)    (G_TYPE_INSTANCE_GET_CLASS ((obj), PANGO_TYPE_FONT_FACE, PangoFontFaceClass))

typedef struct _PangoFontFaceClass   PangoFontFaceClass;

/**
 * PangoFontFace:
 *
 * The #PangoFontFace structure is used to represent a group of fonts with
 * the same family, slant, weight, width, but varying sizes.
 */
struct _PangoFontFace
{
  GObject parent_instance;
};

struct _PangoFontFaceClass
{
  GObjectClass parent_class;

  /*< public >*/

  const char           * (*get_face_name)  (PangoFontFace *face);
  PangoFontDescription * (*describe)       (PangoFontFace *face);
  void                   (*list_sizes)     (PangoFontFace  *face,
					    int           **sizes,
					    int            *n_sizes);
  gboolean               (*is_synthesized) (PangoFontFace *face);

  /*< private >*/

  /* Padding for future expansion */
  void (*_pango_reserved3) (void);
  void (*_pango_reserved4) (void);
};


#define PANGO_FONT_CLASS(klass)      (G_TYPE_CHECK_CLASS_CAST ((klass), PANGO_TYPE_FONT, PangoFontClass))
#define PANGO_IS_FONT_CLASS(klass)   (G_TYPE_CHECK_CLASS_TYPE ((klass), PANGO_TYPE_FONT))
#define PANGO_FONT_GET_CLASS(obj)    (G_TYPE_INSTANCE_GET_CLASS ((obj), PANGO_TYPE_FONT, PangoFontClass))

typedef struct _PangoFontClass       PangoFontClass;

/**
 * PangoFont:
 *
 * The #PangoFont structure is used to represent
 * a font in a rendering-system-independent matter.
 * To create an implementation of a #PangoFont,
 * the rendering-system specific code should allocate
 * a larger structure that contains a nested
 * #PangoFont, fill in the <structfield>klass</structfield> member of
 * the nested #PangoFont with a pointer to
 * a appropriate #PangoFontClass, then call
 * pango_font_init() on the structure.
 *
 * The #PangoFont structure contains one member
 * which the implementation fills in.
 */
struct _PangoFont
{
  GObject parent_instance;
};

struct _PangoFontClass
{
  GObjectClass parent_class;

  /*< public >*/

  PangoFontDescription *(*describe)           (PangoFont      *font);
  PangoCoverage *       (*get_coverage)       (PangoFont      *font,
					       PangoLanguage  *language);
  PangoEngineShape *    (*find_shaper)        (PangoFont      *font,
					       PangoLanguage  *language,
					       guint32         ch);
  void                  (*get_glyph_extents)  (PangoFont      *font,
					       PangoGlyph      glyph,
					       PangoRectangle *ink_rect,
					       PangoRectangle *logical_rect);
  PangoFontMetrics *    (*get_metrics)        (PangoFont      *font,
					       PangoLanguage  *language);
  PangoFontMap *        (*get_font_map)       (PangoFont      *font);
  PangoFontDescription *(*describe_absolute)  (PangoFont      *font);
  hb_font_t *           (*create_hb_font)     (PangoFont      *font);
  void                  (*get_features)       (PangoFont      *font,
                                               hb_feature_t   *features,
                                               guint           len,
                                               guint          *num_features);

};

/* used for very rare and miserable situtations that we cannot even
 * draw a hexbox
 */
#define PANGO_UNKNOWN_GLYPH_WIDTH  10
#define PANGO_UNKNOWN_GLYPH_HEIGHT 14


G_END_DECLS

#endif /* __PANGO_FONT_PRIVATE_H__ */
