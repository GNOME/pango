/* Pango
 * pango-engine-private.h: Private routines related to engines for
 *   script and language specific processing
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

#ifndef __PANGO_ENGINE_PRIVATE_H__
#define __PANGO_ENGINE_PRIVATE_H__

#include <pango-engine.h>

G_BEGIN_DECLS

void               _pango_engine_shape_shape  (PangoEngineShape *engine,
					       PangoFont        *font,
					       const char       *text,
					       int               length,
					       const PangoAnalysis *analysis,
					       PangoGlyphString *glyphs);
PangoCoverageLevel _pango_engine_shape_covers (PangoEngineShape *engine,
					       PangoFont        *font,
					       PangoLanguage    *language,
					       gunichar          wc);

PangoEngineShape *_pango_get_fallback_shaper (void);

G_END_DECLS

#endif /* __PANGO_ENGINE_PRIVATE_H__ */

