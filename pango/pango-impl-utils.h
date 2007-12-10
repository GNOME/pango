/* Pango
 * pango-impl-utils.h: Macros for get_type() functions
 * Inspired by Jody Goldberg's gsf-impl-utils.h
 *
 * Copyright (C) 2003 Red Hat Software
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

#ifndef __PANGO_IMPL_UTILS_H__
#define __PANGO_IMPL_UTILS_H__

#include <glib-object.h>
#include <pango/pango.h>

G_BEGIN_DECLS

#define PANGO_DEFINE_TYPE_FULL(name, prefix,				   \
			       class_init, instance_init,		   \
			       parent_type, abstract)			   \
GType									   \
prefix ## _get_type (void)				                   \
{									   \
  static GType object_type = 0;						   \
									   \
  if (!object_type)							   \
    {									   \
      static const GTypeInfo object_info =				   \
	{								   \
	  sizeof (name ## Class),					   \
	  (GBaseInitFunc) NULL,						   \
	  (GBaseFinalizeFunc) NULL,					   \
	  (GClassInitFunc) class_init,					   \
	  (GClassFinalizeFunc) NULL,					   \
	  NULL,          /* class_data */				   \
	  sizeof (name),						   \
	  0,             /* n_prelocs */				   \
	  (GInstanceInitFunc) instance_init,				   \
	  NULL           /* value_table */				   \
	};								   \
									   \
      object_type = g_type_register_static (parent_type,		   \
					    g_intern_static_string (# name), \
					    &object_info, abstract);	   \
    }									   \
									   \
  return object_type;							   \
}

#define PANGO_DEFINE_TYPE(name, prefix,			\
			  class_init, instance_init,	\
			  parent_type)			\
 PANGO_DEFINE_TYPE_FULL (name, prefix,			\
			 class_init, instance_init,	\
			 parent_type, 0)

#define PANGO_DEFINE_TYPE_ABSTRACT(name, prefix,		\
			  class_init, instance_init,		\
			  parent_type)				\
 PANGO_DEFINE_TYPE_FULL (name, prefix,				\
			 class_init, instance_init,		\
			 parent_type, G_TYPE_FLAG_ABSTRACT)


/* String interning for static strings */
#define I_(string) g_intern_static_string (string)


/* Some functions for handling PANGO_ATTR_SHAPE */
void _pango_shape_shape (const char       *text,
			 gint              n_chars,
			 PangoRectangle   *shape_ink,
			 PangoRectangle   *shape_logical,
			 PangoGlyphString *glyphs);

void _pango_shape_get_extents (gint              n_chars,
			       PangoRectangle   *shape_ink,
			       PangoRectangle   *shape_logical,
			       PangoRectangle   *ink_rect,
			       PangoRectangle   *logical_rect);

G_END_DECLS

#endif /* __PANGO_IMPL_UTILS_H__ */

