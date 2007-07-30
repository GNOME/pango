/* Pango
 * pangox-private.h:
 *
 * Copyright (C) 1999 Red Hat Software
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

#ifndef __PANGOX_PRIVATE_H__
#define __PANGOX_PRIVATE_H__

#include <pango/pangox.h>
#include <pango/pango-modules.h>

typedef struct _PangoXFace        PangoXFace;
typedef struct _PangoXFont        PangoXFont;
typedef struct _PangoXSubfontInfo PangoXSubfontInfo;

struct _PangoXFont
{
  PangoFont font;
  Display *display;

  char **fonts;
  int n_fonts;
  int size;

  /* hash table mapping from charset-name to array of PangoXSubfont ids,
   * of length n_fonts
   */
  GHashTable *subfonts_by_charset;

  PangoXSubfontInfo **subfonts;

  int n_subfonts;
  int max_subfonts;

  GSList *metrics_by_lang;

  PangoFontMap *fontmap;
  /* If TRUE, font is in cache of recently unused fonts and not otherwise
   * in use.
   */
  gboolean in_cache;

  PangoXFace *xface;	/* Used to remove cached fonts */
};


#define PANGO_TYPE_X_FONT_MAP              (pango_x_font_map_get_type ())
#define PANGO_X_FONT_MAP(object)           (G_TYPE_CHECK_INSTANCE_CAST ((object), PANGO_TYPE_X_FONT_MAP, PangoXFontMap))
#define PANGO_X_FONT_MAP_CLASS(klass)      (G_TYPE_CHECK_CLASS_CAST ((klass), PANGO_TYPE_X_FONT_MAP, PangoXFontMapClass))
#define PANGO_X_IS_FONT_MAP(object)        (G_TYPE_CHECK_INSTANCE_TYPE ((object), PANGO_TYPE_X_FONT_MAP))
#define PANGO_X_IS_FONT_MAP_CLASS(klass)   (G_TYPE_CHECK_CLASS_TYPE ((klass), PANGO_TYPE_X_FONT_MAP))
#define PANGO_X_FONT_MAP_GET_CLASS(obj)    (G_TYPE_INSTANCE_GET_CLASS ((obj), PANGO_TYPE_X_FONT_MAP, PangoXFontMapClass))

typedef struct _PangoXFontMap      PangoXFontMap;
typedef struct _PangoXFontMapClass PangoXFontMapClass;

struct _PangoXFontMap
{
  PangoFontMap parent_instance;

  Display *display;

  PangoXFontCache *font_cache;
  GQueue *freed_fonts;

  GHashTable *families;
  GHashTable *size_infos;

  GHashTable *to_atom_cache;
  GHashTable *from_atom_cache;

  int n_fonts;

  double resolution;		/* (points / pixel) * PANGO_SCALE */

  Window coverage_win;
};

GType    pango_x_font_map_get_type   (void) G_GNUC_CONST;

PangoXFont *   pango_x_font_new                (PangoFontMap    *fontmap,
						const char      *spec,
						int              size);
PangoMap *     pango_x_get_shaper_map          (PangoLanguage   *language);
char *         pango_x_make_matching_xlfd      (PangoFontMap    *fontmap,
						char            *xlfd,
						const char      *charset,
						int              size);
PangoCoverage *pango_x_face_get_coverage       (PangoXFace      *xface,
						PangoFont       *font,
						PangoLanguage   *language);
void           pango_x_face_remove             (PangoXFace      *xface,
						PangoFont       *font);

Display *      pango_x_fontmap_get_display     (PangoFontMap    *fontmap);
void           pango_x_fontmap_cache_add       (PangoFontMap    *fontmap,
						PangoXFont      *xfont);
void           pango_x_fontmap_cache_remove    (PangoFontMap    *fontmap,
						PangoXFont      *xfont);

Atom           pango_x_fontmap_atom_from_name (PangoFontMap *fontmap,
					       const char   *atomname);
const char    *pango_x_fontmap_name_from_atom  (PangoFontMap *fontmap,
						Atom          atom);

#endif /* __PANGOX_PRIVATE_H__ */
