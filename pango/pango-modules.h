/* Pango
 * pango-modules.h:
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

#ifndef __PANGO_MODULES_H__
#define __PANGO_MODULES_H__

#include <pango/pango-engine.h>

G_BEGIN_DECLS

#ifdef PANGO_ENABLE_BACKEND

typedef struct _PangoMap PangoMap;
typedef struct _PangoMapEntry PangoMapEntry;

struct _PangoMapEntry 
{
  PangoEngineInfo *info;
  gboolean is_exact;
};

typedef struct _PangoIncludedModule PangoIncludedModule;

struct _PangoIncludedModule
{
  void (*list) (PangoEngineInfo **engines, int *n_engines);
  PangoEngine *(*load) (const char *id);
  void (*unload) (PangoEngine *engine);
};

PangoMap *     pango_find_map        (PangoLanguage       *language,
				      guint                engine_type_id,
				      guint                render_type_id);
PangoMapEntry *pango_map_get_entry   (PangoMap            *map,
				      guint32              wc);
PangoEngine *  pango_map_get_engine  (PangoMap            *map,
				      guint32              wc);
void           pango_module_register (PangoIncludedModule *module);

#endif /* PANGO_ENABLE_BACKEND */

G_END_DECLS

#endif /* __PANGO_MODULES_H__ */
