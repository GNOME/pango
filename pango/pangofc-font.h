/* Pango
 * pangofc-font.h: Shared interfaces for fontconfig-based backends
 *
 * Copyright (C) 2003 Red Hat Software
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

#ifndef __PANGO_FC_FONT_H__
#define __PANGO_FC_FONT_H__

#if defined(PANGO_ENABLE_ENGINE) || defined(PANGO_ENABLE_BACKEND)

#include <freetype/freetype.h>
#include <pango/pango-font.h>

G_BEGIN_DECLS

#define PANGO_TYPE_FC_FONT              (pango_fc_font_get_type ())
#define PANGO_FC_FONT(object)           (G_TYPE_CHECK_INSTANCE_CAST ((object), PANGO_TYPE_FC_FONT, PangoFcFont))
#define PANGO_IS_FC_FONT(object)        (G_TYPE_CHECK_INSTANCE_TYPE ((object), PANGO_TYPE_FC_FONT))

typedef struct _PangoFcFont      PangoFcFont;
typedef struct _PangoFcFontClass PangoFcFontClass;

#ifdef PANGO_ENABLE_BACKEND

#define PANGO_FC_FONT_CLASS(klass)      (G_TYPE_CHECK_CLASS_CAST ((klass), PANGO_TYPE_FC_FONT, PangoFcFontClass))
#define PANGO_IS_FC_FONT_CLASS(klass)   (G_TYPE_CHECK_CLASS_TYPE ((klass), PANGO_TYPE_FC_FONT))
#define PANGO_FC_FONT_GET_CLASS(obj)    (G_TYPE_INSTANCE_GET_CLASS ((obj), PANGO_TYPE_FC_FONT, PangoFcFontClass))

struct _PangoFcFont
{
  PangoFont parent_instance;
};

struct _PangoFcFontClass
{
  PangoFontClass parent_class;

  /*< public >*/
  FT_Face    (*lock_face)         (PangoFcFont *font);
  void       (*unlock_face)       (PangoFcFont *font);
  gboolean   (*has_char)          (PangoFcFont *font,
				   gunichar     wc);
  guint      (*get_glyph)         (PangoFcFont *font,
				   gunichar     wc);
  PangoGlyph (*get_unknown_glyph) (PangoFcFont *font,
				   gunichar     wc);
  int        (*get_kerning)       (PangoFcFont *font,
				   PangoGlyph   left,
				   PangoGlyph   right);
  /*< private >*/

  /* Padding for future expansion */
  void (*_pango_reserved1) (void);
  void (*_pango_reserved2) (void);
  void (*_pango_reserved3) (void);
  void (*_pango_reserved4) (void);
};

#endif /* PANGO_ENABLE_BACKEND */

GType      pango_fc_font_get_type (void);

FT_Face    pango_fc_font_lock_face         (PangoFcFont *font);
void       pango_fc_font_unlock_face       (PangoFcFont *font);
gboolean   pango_fc_font_has_char          (PangoFcFont *font,
					    gunichar     wc);
guint      pango_fc_font_get_glyph         (PangoFcFont *font,
					    gunichar     wc);
PangoGlyph pango_fc_font_get_unknown_glyph (PangoFcFont *font,
					    gunichar     wc);
int        pango_fc_font_get_kerning       (PangoFcFont *font,
					    PangoGlyph   left,
					    PangoGlyph   right);
G_END_DECLS

#endif /* PANGO_ENABLE_ENGINE || PANGO_ENABLE_BACKEND */

#endif /* __PANGO_FC_FONT_H__ */
