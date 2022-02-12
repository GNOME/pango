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

typedef struct _PangoFontFamilyClass PangoFontFamilyClass;

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

  PangoFontFace * (*get_face) (PangoFontFamily *family,
                               const char      *name);


  /*< private >*/

  /* Padding for future expansion */
  void (*_pango_reserved2) (void);
};

#define PANGO_FONT_FAMILY_CLASS(klass)      (G_TYPE_CHECK_CLASS_CAST ((klass), PANGO_TYPE_FONT_FAMILY, PangoFontFamilyClass))
#define PANGO_IS_FONT_FAMILY_CLASS(klass)   (G_TYPE_CHECK_CLASS_TYPE ((klass), PANGO_TYPE_FONT_FAMILY))
#define PANGO_FONT_FAMILY_GET_CLASS(obj)    (G_TYPE_INSTANCE_GET_CLASS ((obj), PANGO_TYPE_FONT_FAMILY, PangoFontFamilyClass))

struct _PangoFontFace
{
  GObject parent_instance;
};

typedef struct _PangoFontFaceClass PangoFontFaceClass;

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
  PangoFontFamily *      (*get_family)     (PangoFontFace *face);

  /*< private >*/

  /* Padding for future expansion */
  void (*_pango_reserved3) (void);
  void (*_pango_reserved4) (void);
};

#define PANGO_FONT_FACE_CLASS(klass)      (G_TYPE_CHECK_CLASS_CAST ((klass), PANGO_TYPE_FONT_FACE, PangoFontFaceClass))
#define PANGO_IS_FONT_FACE_CLASS(klass)   (G_TYPE_CHECK_CLASS_TYPE ((klass), PANGO_TYPE_FONT_FACE))
#define PANGO_FONT_FACE_GET_CLASS(obj)    (G_TYPE_INSTANCE_GET_CLASS ((obj), PANGO_TYPE_FONT_FACE, PangoFontFaceClass))

struct _PangoFont
{
  GObject parent_instance;
};

typedef struct _PangoFontClass       PangoFontClass;
struct _PangoFontClass
{
  GObjectClass parent_class;

  /*< public >*/

  PangoFontDescription *(*describe)           (PangoFont      *font);
  PangoCoverage *       (*get_coverage)       (PangoFont      *font,
                                               PangoLanguage  *language);
  void                  (*get_glyph_extents)  (PangoFont      *font,
                                               PangoGlyph      glyph,
                                               PangoRectangle *ink_rect,
                                               PangoRectangle *logical_rect);
  PangoFontMetrics *    (*get_metrics)        (PangoFont      *font,
                                               PangoLanguage  *language);
  PangoFontMap *        (*get_font_map)       (PangoFont      *font);
  PangoFontDescription *(*describe_absolute)  (PangoFont      *font);
  void                  (*get_features)       (PangoFont      *font,
                                               hb_feature_t   *features,
                                               guint           len,
                                               guint          *num_features);
  hb_font_t *           (*create_hb_font)     (PangoFont      *font);
};

#define PANGO_FONT_CLASS(klass)      (G_TYPE_CHECK_CLASS_CAST ((klass), PANGO_TYPE_FONT, PangoFontClass))
#define PANGO_IS_FONT_CLASS(klass)   (G_TYPE_CHECK_CLASS_TYPE ((klass), PANGO_TYPE_FONT))
#define PANGO_FONT_GET_CLASS(obj)    (G_TYPE_INSTANCE_GET_CLASS ((obj), PANGO_TYPE_FONT, PangoFontClass))


typedef struct {
  PangoLanguage ** (* get_languages) (PangoFont *font);

  gboolean         (* is_hinted) (PangoFont *font);

  void             (* get_scale_factors) (PangoFont *font,
                                          double    *x_scale,
                                          double    *y_scale);

  gboolean         (* has_char) (PangoFont *font,
                                 gunichar   wc);
  PangoFontFace *  (* get_face) (PangoFont *font);
  void             (* get_matrix) (PangoFont   *font,
                                   PangoMatrix *matrix);
  int              (* get_absolute_size) (PangoFont *font);
} PangoFontClassPrivate;

gboolean pango_font_is_hinted         (PangoFont *font);
void     pango_font_get_scale_factors (PangoFont *font,
                                       double    *x_scale,
                                       double    *y_scale);
void     pango_font_get_matrix        (PangoFont   *font,
                                       PangoMatrix *matrix);

static inline int pango_font_get_absolute_size (PangoFont *font)
{
  GTypeClass *klass = (GTypeClass *) PANGO_FONT_GET_CLASS (font);
  PangoFontClassPrivate *priv = g_type_class_get_private (klass, PANGO_TYPE_FONT);
  return priv->get_absolute_size (font);
}

G_END_DECLS

#endif /* __PANGO_FONT_PRIVATE_H__ */
