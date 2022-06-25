/*
 * Copyright (C) 2000 Red Hat Software
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

#include <glib-object.h>
#include <pango/pango-types.h>
#include <pango/pango-attributes.h>
#include <pango/pango-lines.h>
#include <pango/pango-tabs.h>

G_BEGIN_DECLS

#define PANGO2_TYPE_LAYOUT pango2_layout_get_type ()

PANGO2_AVAILABLE_IN_ALL
G_DECLARE_FINAL_TYPE (Pango2Layout, pango2_layout, PANGO2, LAYOUT, GObject);

PANGO2_AVAILABLE_IN_ALL
Pango2Layout *          pango2_layout_new            (Pango2Context                 *context);

PANGO2_AVAILABLE_IN_ALL
Pango2Layout *          pango2_layout_copy           (Pango2Layout                  *layout);

PANGO2_AVAILABLE_IN_ALL
guint                   pango2_layout_get_serial     (Pango2Layout                  *layout);

PANGO2_AVAILABLE_IN_ALL
Pango2Context *         pango2_layout_get_context    (Pango2Layout                  *layout);

PANGO2_AVAILABLE_IN_ALL
void                    pango2_layout_context_changed (Pango2Layout                 *layout);

PANGO2_AVAILABLE_IN_ALL
void                    pango2_layout_set_text       (Pango2Layout                  *layout,
                                                      const char                    *text,
                                                      int                            length);
PANGO2_AVAILABLE_IN_ALL
const char *            pango2_layout_get_text       (Pango2Layout                  *layout);

PANGO2_AVAILABLE_IN_ALL
void                    pango2_layout_set_markup     (Pango2Layout                  *layout,
                                                      const char                    *markup,
                                                      int                            length);

PANGO2_AVAILABLE_IN_ALL
int                     pango2_layout_get_character_count
                                                     (Pango2Layout                  *layout);

PANGO2_AVAILABLE_IN_ALL
void                    pango2_layout_set_attributes (Pango2Layout                  *layout,
                                                      Pango2AttrList                *attrs);

PANGO2_AVAILABLE_IN_ALL
Pango2AttrList *        pango2_layout_get_attributes (Pango2Layout                  *layout);

PANGO2_AVAILABLE_IN_ALL
void                    pango2_layout_set_font_description
                                                     (Pango2Layout                  *layout,
                                                      const Pango2FontDescription   *desc);

PANGO2_AVAILABLE_IN_ALL
const Pango2FontDescription *
                        pango2_layout_get_font_description
                                                     (Pango2Layout                  *layout);

PANGO2_AVAILABLE_IN_ALL
void                    pango2_layout_set_line_height
                                                     (Pango2Layout                  *layout,
                                                      float                          line_height);

PANGO2_AVAILABLE_IN_ALL
float                   pango2_layout_get_line_height
                                                     (Pango2Layout                  *layout);

PANGO2_AVAILABLE_IN_ALL
void                    pango2_layout_set_spacing    (Pango2Layout                  *layout,
                                                      int                            spacing);

PANGO2_AVAILABLE_IN_ALL
int                     pango2_layout_get_spacing    (Pango2Layout                  *layout);

PANGO2_AVAILABLE_IN_ALL
void                    pango2_layout_set_width      (Pango2Layout                  *layout,
                                                      int                            width);

PANGO2_AVAILABLE_IN_ALL
int                     pango2_layout_get_width      (Pango2Layout                  *layout);

PANGO2_AVAILABLE_IN_ALL
void                    pango2_layout_set_height     (Pango2Layout                  *layout,
                                                      int                            height);

PANGO2_AVAILABLE_IN_ALL
int                     pango2_layout_get_height     (Pango2Layout                  *layout);

PANGO2_AVAILABLE_IN_ALL
void                    pango2_layout_set_tabs       (Pango2Layout                  *layout,
                                                      Pango2TabArray                *tabs);

PANGO2_AVAILABLE_IN_ALL
Pango2TabArray *        pango2_layout_get_tabs       (Pango2Layout                  *layout);

PANGO2_AVAILABLE_IN_ALL
void                    pango2_layout_set_single_paragraph
                                                     (Pango2Layout                  *layout,
                                                      gboolean                       single_paragraph);
PANGO2_AVAILABLE_IN_ALL
gboolean                pango2_layout_get_single_paragraph
                                                     (Pango2Layout                  *layout);

PANGO2_AVAILABLE_IN_ALL
void                    pango2_layout_set_wrap       (Pango2Layout                  *layout,
                                                      Pango2WrapMode                 wrap);

PANGO2_AVAILABLE_IN_ALL
Pango2WrapMode          pango2_layout_get_wrap       (Pango2Layout                  *layout);

PANGO2_AVAILABLE_IN_ALL
void                    pango2_layout_set_indent     (Pango2Layout                  *layout,
                                                      int                            indent);

PANGO2_AVAILABLE_IN_ALL
int                     pango2_layout_get_indent     (Pango2Layout                  *layout);

PANGO2_AVAILABLE_IN_ALL
void                    pango2_layout_set_alignment  (Pango2Layout                  *layout,
                                                      Pango2Alignment                alignment);

PANGO2_AVAILABLE_IN_ALL
Pango2Alignment         pango2_layout_get_alignment  (Pango2Layout                  *layout);

PANGO2_AVAILABLE_IN_ALL
void                    pango2_layout_set_ellipsize  (Pango2Layout                  *layout,
                                                      Pango2EllipsizeMode            ellipsize);

PANGO2_AVAILABLE_IN_ALL
Pango2EllipsizeMode     pango2_layout_get_ellipsize  (Pango2Layout                  *layout);

PANGO2_AVAILABLE_IN_ALL
void                    pango2_layout_set_auto_dir   (Pango2Layout                  *layout,
                                                      gboolean                       auto_dir);

PANGO2_AVAILABLE_IN_ALL
gboolean                pango2_layout_get_auto_dir   (Pango2Layout                  *layout);

PANGO2_AVAILABLE_IN_ALL
Pango2Lines *           pango2_layout_get_lines      (Pango2Layout                  *layout);

PANGO2_AVAILABLE_IN_ALL
Pango2LineIter *        pango2_layout_get_iter       (Pango2Layout                  *layout);

PANGO2_AVAILABLE_IN_ALL
const Pango2LogAttr *   pango2_layout_get_log_attrs  (Pango2Layout                  *layout,
                                                      int                           *n_attrs);

typedef enum {
  PANGO2_LAYOUT_SERIALIZE_DEFAULT = 0,
  PANGO2_LAYOUT_SERIALIZE_CONTEXT = 1 << 0,
  PANGO2_LAYOUT_SERIALIZE_OUTPUT = 1 << 1,
} Pango2LayoutSerializeFlags;

PANGO2_AVAILABLE_IN_ALL
GBytes *                pango2_layout_serialize      (Pango2Layout                  *layout,
                                                      Pango2LayoutSerializeFlags     flags);

PANGO2_AVAILABLE_IN_ALL
gboolean                pango2_layout_write_to_file  (Pango2Layout                  *layout,
                                                      const char                    *filename);

#define PANGO2_LAYOUT_DESERIALIZE_ERROR (pango2_layout_deserialize_error_quark ())

/**
 * Pango2LayoutDeserializeError:
 * @PANGO2_LAYOUT_DESERIALIZE_INVALID: Unspecified error
 * @PANGO2_LAYOUT_DESERIALIZE_INVALID_VALUE: A JSon value could not be
 *   interpreted
 * @PANGO2_LAYOUT_DESERIALIZE_MISSING_VALUE: A required JSon member was
 *   not found
 *
 * Errors that can be returned by [func@Pango2.Layout.deserialize].
 */
typedef enum {
  PANGO2_LAYOUT_DESERIALIZE_INVALID,
  PANGO2_LAYOUT_DESERIALIZE_INVALID_VALUE,
  PANGO2_LAYOUT_DESERIALIZE_MISSING_VALUE,
} Pango2LayoutDeserializeError;

typedef enum {
  PANGO2_LAYOUT_DESERIALIZE_DEFAULT = 0,
  PANGO2_LAYOUT_DESERIALIZE_CONTEXT = 1 << 0,
} Pango2LayoutDeserializeFlags;

PANGO2_AVAILABLE_IN_ALL
GQuark                  pango2_layout_deserialize_error_quark (void);

PANGO2_AVAILABLE_IN_ALL
Pango2Layout *          pango2_layout_deserialize    (Pango2Context                 *context,
                                                      GBytes                        *bytes,
                                                      Pango2LayoutDeserializeFlags   flags,
                                                      GError                       **error);

G_END_DECLS
