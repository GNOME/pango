/* Pango
 * utils.h:
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

#ifndef __UTILS_H__
#define __UTILS_H__

#include <glib.h>

typedef guint16 GUChar2;
typedef guint32 GUChar4;

gboolean _pango_utf8_iterate (gchar *cur, char **next, GUChar4 *wc_out);
GUChar2 *_pango_utf8_to_ucs2 (gchar *str, gint len);
gint _pango_utf8_len (gchar *str, gint limit);

#endif /* __UTILS_H__ */
