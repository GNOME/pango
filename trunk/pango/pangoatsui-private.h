/* Pango
 * pangoatsui-private.h:
 *
 * Copyright (C) 2003 Red Hat Software
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

#ifndef __PANGOATSUI_PRIVATE_H__
#define __PANGOATSUI_PRIVATE_H__

#include <pango/pango-fontmap.h>
#include <pango/pango-context.h>
#include "pangoatsui.h"

G_BEGIN_DECLS

#define PANGO_TYPE_ATSUI_FONT_MAP             (pango_atsui_font_map_get_type ())
#define PANGO_ATSUI_FONT_MAP(object)          (G_TYPE_CHECK_INSTANCE_CAST ((object), PANGO_TYPE_ATSUI_FONT_MAP, PangoATSUIFontMap))
#define PANGO_ATSUI_IS_FONT_MAP(object)       (G_TYPE_CHECK_INSTANCE_TYPE ((object), PANGO_TYPE_ATSUI_FONT_MAP))
#define PANGO_ATSUI_FONT_MAP_CLASS(klass)     (G_TYPE_CHECK_CLASS_CAST ((klass), PANGO_TYPE_ATSUI_FONT_MAP, PangoATSUIFontMapClass))
#define PANGO_IS_ATSUI_FONT_MAP_CLASS(klass)  (G_TYPE_CHECK_CLASS_TYPE ((klass), PANGO_TYPE_ATSUI_FONT_MAP))
#define PANGO_ATSUI_FONT_MAP_GET_CLASS(obj)   (G_TYPE_INSTANCE_GET_CLASS ((obj), PANGO_TYPE_ATSUI_FONT_MAP, PangoATSUIFontMapClass))


typedef struct _PangoATSUIFamily       PangoATSUIFamily;
typedef struct _PangoATSUIFace         PangoATSUIFace;

typedef struct _PangoATSUIFontMap      PangoATSUIFontMap;
typedef struct _PangoATSUIFontMapClass PangoATSUIFontMapClass;

struct _PangoATSUIFontMap
{
  PangoFontMap parent_instance;

  GHashTable *font_hash;

  GHashTable *families;
};

struct _PangoATSUIFontMapClass
{
  PangoFontMapClass parent_class;

  gconstpointer (*context_key_get)   (PangoATSUIFontMap             *atsuifontmap,
				      PangoContext               *context);
  gpointer     (*context_key_copy)   (PangoATSUIFontMap             *atsuifontmap,
				      gconstpointer               key);
  void         (*context_key_free)   (PangoATSUIFontMap             *atsuifontmap,
				      gpointer                    key);
  guint32      (*context_key_hash)   (PangoATSUIFontMap             *atsuifontmap,
				      gconstpointer               key);
  gboolean     (*context_key_equal)  (PangoATSUIFontMap             *atsuifontmap,
				      gconstpointer               key_a,
				      gconstpointer               key_b);

  PangoATSUIFont * (* create_font)   (PangoATSUIFontMap          *fontmap,
				      PangoContext               *context,
				      PangoATSUIFace             *face,
				      const PangoFontDescription *desc);
};


GType                 pango_atsui_font_map_get_type          (void) G_GNUC_CONST;

const char *          _pango_atsui_face_get_postscript_name  (PangoATSUIFace    *face);
PangoCoverage *       _pango_atsui_face_get_coverage         (PangoATSUIFace    *face,
                                                              PangoLanguage     *language);
gboolean              _pango_atsui_face_get_synthetic_italic (PangoATSUIFace    *face);

void                  _pango_atsui_font_set_font_description (PangoATSUIFont    *afont,
                                                              const PangoFontDescription *desc);
PangoFontDescription *_pango_atsui_font_get_font_description (PangoATSUIFont    *afont);
void                  _pango_atsui_font_set_font_map         (PangoATSUIFont    *afont,
                                                              PangoATSUIFontMap *fontmap);
void                  _pango_atsui_font_set_face             (PangoATSUIFont    *afont, 
                                                              PangoATSUIFace    *aface);
PangoATSUIFace *      _pango_atsui_font_get_face             (PangoATSUIFont    *font);
gpointer              _pango_atsui_font_get_context_key      (PangoATSUIFont    *afont);
void                  _pango_atsui_font_set_context_key      (PangoATSUIFont    *afont,
                                                              gpointer           context_key);
void                  _pango_atsui_font_set_atsu_font_id     (PangoATSUIFont    *font,
                                                              ATSUFontID         font_id);

G_END_DECLS

#endif /* __PANGOATSUI_H__ */
