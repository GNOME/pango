/* Pango
 * modules.h:
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

#include <pango/pango-engine.h>
#include <pango/pango-modules.h>

#ifndef __MODULES_H__
#define __MODULES_H__

extern PangoIncludedModule _pango_included_lang_modules[];
extern PangoIncludedModule _pango_included_x_modules[];
extern PangoIncludedModule _pango_included_fc_modules[];
extern PangoIncludedModule _pango_included_win32_modules[];
extern PangoIncludedModule _pango_included_atsui_modules[];

#endif /* __MODULES_H__ */
