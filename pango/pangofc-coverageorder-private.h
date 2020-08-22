/* Pango
 * coverageorder.h:
 *
 * Copyright (C) 2020 Red Hat, Inc.
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
 * You should have received a copy of the GNU Lesser General Public
 * License along with this library. If not, see <http://www.gnu.org/licenses/>.
 *
 * Authors: Matthias Clasen
 */

#ifndef __COVERAGE_ORDER_H__
#define __COVERAGE_ORDER_H__

#include <glib.h>
#include <fontconfig/fontconfig.h>

G_BEGIN_DECLS

typedef struct _CoverageOrder CoverageOrder;

CoverageOrder * coverage_order_new       (FcFontSet     *fonts);
void            coverage_order_free      (CoverageOrder *co);
gboolean        coverage_order_is_subset (CoverageOrder *co,
                                          FcPattern     *p1,
                                          FcPattern     *p2);

G_END_DECLS

#endif /* __COVERAGE_ORDER_H__ */
