/* Pango
 * pango-engine.h: Module handling
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

#ifndef __PANGO_ENGINE_H__
#define __PANGO_ENGINE_H__

#include <pango/pango-types.h>
#include <pango/pango-item.h>
#include <pango/pango-font.h>
#include <pango/pango-glyph.h>

G_BEGIN_DECLS

#ifdef PANGO_ENABLE_ENGINE

/* Module API */

#define PANGO_ENGINE_TYPE_LANG "PangoEngineLang"
#define PANGO_ENGINE_TYPE_SHAPE "PangoEngineShape"

#define PANGO_RENDER_TYPE_NONE "PangoRenderNone"

typedef struct _PangoEngineInfo PangoEngineInfo;
typedef struct _PangoEngineRange PangoEngineRange;
typedef struct _PangoEngine PangoEngine;

struct _PangoEngineRange 
{
  guint32 start;
  guint32 end;
  gchar *langs;
};

struct _PangoEngineInfo
{
  gchar *id;
  gchar *engine_type;
  gchar *render_type;
  PangoEngineRange *ranges;
  gint n_ranges;
};

struct _PangoEngine
{
  gchar *id;
  gchar *type;
  gint length;
};

struct _PangoEngineLang
{
  PangoEngine engine;
  void (*script_break) (const char    *text,
			int            len,
			PangoAnalysis *analysis,
			PangoLogAttr  *attrs,
                        int            attrs_len);
};

struct _PangoEngineShape
{
  PangoEngine engine;
  void (*script_shape) (PangoFont        *font,
			const char       *text,
			int               length,
			PangoAnalysis    *analysis,
			PangoGlyphString *glyphs);
  PangoCoverage *(*get_coverage) (PangoFont        *font,
				  PangoLanguage    *language);
};

/* A module should export the following functions */

void         script_engine_list   (PangoEngineInfo **engines,
				   int              *n_engines);
PangoEngine *script_engine_load   (const char       *id);
void         script_engine_unload (PangoEngine      *engine);


#endif /* PANGO_ENABLE_ENGINE */

G_END_DECLS

#endif /* __PANGO_ENGINE_H__ */
