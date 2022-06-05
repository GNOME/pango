/* Pango
 * pango-script.h: Script tag handling
 *
 * Copyright (C) 2002 Red Hat Software
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Library General Public
 * License as published by the Free Software Foundation; either
 * version 2 of the License, or (at your option) any later version.
 *
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Library General Public License for more details.
 *
 * You should have received a copy of the GNU Library General Public
 * License along with this library; if not, write to the
 * Free Software Foundation, Inc., 59 Temple Place - Suite 330,
 * Boston, MA 02111-1307, USA.
 */

#ifndef __PANGO_SCRIPT_H__
#define __PANGO_SCRIPT_H__

#include <glib-object.h>

#include <pango/pango-version-macros.h>

G_BEGIN_DECLS

/**
 * PangoScriptIter:
 *
 * A `PangoScriptIter` is used to iterate through a string
 * and identify ranges in different scripts.
 **/
typedef struct _PangoScriptIter PangoScriptIter;

PANGO_AVAILABLE_IN_ALL
GType            pango_script_iter_get_type  (void) G_GNUC_CONST;

PANGO_AVAILABLE_IN_ALL
PangoScriptIter *pango_script_iter_new       (const char          *text,
                                              int                  length);
PANGO_AVAILABLE_IN_ALL
void             pango_script_iter_get_range (PangoScriptIter     *iter,
                                              const char         **start,
                                              const char         **end,
                                              GUnicodeScript      *script);
PANGO_AVAILABLE_IN_ALL
gboolean         pango_script_iter_next      (PangoScriptIter     *iter);
PANGO_AVAILABLE_IN_ALL
void             pango_script_iter_free      (PangoScriptIter     *iter);

#include <pango/pango-language.h>

PANGO_AVAILABLE_IN_ALL
PangoLanguage *pango_script_get_sample_language (GUnicodeScript    script) G_GNUC_PURE;

G_END_DECLS

#endif /* __PANGO_SCRIPT_H__ */
