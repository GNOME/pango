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

#include "modules.h"
#include "pangox.h"
#include "pangox-private.h"

typedef struct _PangoXFont PangoXFont;
typedef struct _PangoXFontEntry PangoXFontEntry;
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
  
  PangoXFontEntry *entry;	/* Used to remove cached fonts */
};

PangoXFont *   pango_x_font_new                (Display         *display,
						const char      *spec,
						int              size);
PangoMap *     pango_x_get_shaper_map          (const char      *lang);
char *         pango_x_make_matching_xlfd      (PangoFontMap    *fontmap,
						char            *xlfd,
						const char      *charset,
						int              size);
PangoCoverage *pango_x_font_entry_get_coverage (PangoXFontEntry *entry,
						PangoFont       *font,
						const char      *lang);
void           pango_x_font_entry_remove       (PangoXFontEntry *entry,
						PangoFont       *font);

#endif /* __PANGOX_PRIVATE_H__ */
