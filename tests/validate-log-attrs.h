/* Pango
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
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Library General Public License for more details.
 *
 * You should have received a copy of the GNU Library General Public
 * License along with this library; if not, write to the
 * Free Software Foundation, Inc., 59 Temple Place - Suite 330,
 * Boston, MA 02111-1307, USA.
 */

#ifndef __VALIDATE_LOG_ATTRS_H__
#define __VALIDATE_LOG_ATTRS_H__

#include <glib.h>

G_BEGIN_DECLS

#include <pango/pango-item.h>

#define PANGO_VALIDATE_ERROR (pango_validate_error_quark ())

typedef enum
{
  PANGO_VALIDATE_ERROR_FAILED,
  PANGO_VALIDATE_ERROR_BREAK,
  PANGO_VALIDATE_ERROR_GRAPHEME,
  PANGO_VALIDATE_ERROR_WORD,
  PANGO_VALIDATE_ERROR_SENTENCE,
  PANGO_VALIDATE_ERROR_SPACE
} PangoValidateError;

GQuark                 pango_validate_error_quark (void);

gboolean               pango_validate_log_attrs (const char          *text,
                                                 int                  length,
                                                 const PangoLogAttr  *log_attrs,
                                                 int                  attrs_len,
                                                 GError             **error);

G_END_DECLS

#endif /* __VALIDATE_LOG_ATTRS_H__ */
