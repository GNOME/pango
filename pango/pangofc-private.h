/* Pango
 * pangofc-private.h: Private routines and declarations for generic
 *  fontconfig operation
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

#ifndef __PANGOFC_PRIVATE_H__
#define __PANGOFC_PRIVATE_H__

#include <pangofc-fontmap.h>

G_BEGIN_DECLS

void _pango_fc_font_shutdown (PangoFcFont *fcfont);

void           _pango_fc_font_map_remove         (PangoFcFontMap *fcfontmap,
						  PangoFcFont    *fcfont);
PangoCoverage *_pango_fc_font_map_get_coverage   (PangoFcFontMap *fcfontmap,
						  FcPattern      *pattern);

G_END_DECLS

#endif /* __PANGOFC_PRIVATE_H__ */
