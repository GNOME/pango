/*
 * Copyright © 2020 Red Hat, Inc
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
 * License along with this library. If not, see <http://www.gnu.org/licenses/>.
 *
 * Authors: Matthias Clasen <mclasen@redhat.com>
 */


#ifndef __GTK_CSS_SERIALIZER_H__
#define __GTK_CSS_SERIALIZER_H__

#include <glib.h>

G_BEGIN_DECLS

void gtk_css_print_string (GString    *str,
                           const char *string,
                           gboolean    multiline);

G_END_DECLS

#endif /* __GTK_CSS_SERIALIZER_H__ */
