/* Pango2
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

#define PANGO2_VALIDATE_ERROR (pango2_validate_error_quark ())

typedef enum
{
  PANGO2_VALIDATE_ERROR_FAILED,
  PANGO2_VALIDATE_ERROR_BREAK,
  PANGO2_VALIDATE_ERROR_GRAPHEME,
  PANGO2_VALIDATE_ERROR_WORD,
  PANGO2_VALIDATE_ERROR_SENTENCE,
  PANGO2_VALIDATE_ERROR_SPACE
} Pango2ValidateError;

GQuark                 pango2_validate_error_quark (void);

gboolean               pango2_validate_log_attrs (const char          *text,
                                                 int                  length,
                                                 const Pango2LogAttr  *log_attrs,
                                                 int                  attrs_len,
                                                 GError             **error);

G_END_DECLS

#endif /* __VALIDATE_LOG_ATTRS_H__ */
