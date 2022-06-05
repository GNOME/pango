/*
 * Copyright (C) 2002 Red Hat Software
 *
 * SPDX-License-Identifier: LGPL-2.1-or-later
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public
 * License as published by the Free Software Foundation; either
 * version 2.1 of the License, or (at your option) any later version.
 *
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public
 * License along with this library; if not, see <http://www.gnu.org/licenses/>.
 */

#pragma once


#define PAREN_STACK_DEPTH 128

typedef struct _ParenStackEntry ParenStackEntry;

struct _ParenStackEntry
{
  int pair_index;
  GUnicodeScript script_code;
};

struct _PangoScriptIter
{
  const gchar *text_start;
  const gchar *text_end;

  const gchar *script_start;
  const gchar *script_end;
  GUnicodeScript script_code;

  ParenStackEntry paren_stack[PAREN_STACK_DEPTH];
  int paren_sp;
};

PangoScriptIter *
_pango_script_iter_init (PangoScriptIter *iter,
	                 const char      *text,
			 int              length);

void
_pango_script_iter_fini (PangoScriptIter *iter);
