/* Pango
 * pango-attributes.h: Attributed text
 *
 * Copyright (C) 2000 Red Hat Software
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

#ifndef __PANGO_ATTRIBUTES_H__
#define __PANGO_ATTRIBUTES_H__

#include <pango/pango-font.h>

#ifdef __cplusplus
extern "C" {
#endif /* __cplusplus */

typedef struct _PangoAttribute    PangoAttribute;
typedef struct _PangoAttrClass    PangoAttrClass;
				  
typedef struct _PangoAttrString   PangoAttrString;
typedef struct _PangoAttrInt      PangoAttrInt;
typedef struct _PangoAttrColor    PangoAttrColor;
typedef struct _PangoAttrFontDesc PangoAttrFontDesc;
				  
typedef struct _PangoAttrList     PangoAttrList;
typedef struct _PangoAttrIterator PangoAttrIterator;

typedef enum {
  PANGO_ATTR_LANG,		/* PangoAttrString */
  PANGO_ATTR_FAMILY,		/* PangoAttrString */
  PANGO_ATTR_STYLE,		/* PangoAttrInt */
  PANGO_ATTR_WEIGHT,		/* PangoAttrInt */
  PANGO_ATTR_VARIANT,		/* PangoAttrInt */
  PANGO_ATTR_STRETCH,		/* PangoAttrInt */
  PANGO_ATTR_SIZE,		/* PangoAttrSize */
  PANGO_ATTR_FONT_DESC,		/* PangoAttrFontDesc */
  PANGO_ATTR_FOREGROUND,	/* PangoAttrColor */
  PANGO_ATTR_BACKGROUND,	/* PangoAttrColor */
  PANGO_ATTR_UNDERLINE,		/* PangoAttrInt */
  PANGO_ATTR_STRIKETHROUGH,	/* PangoAttrInt */
  PANGO_ATTR_RISE		/* PangoAttrInt */
} PangoAttrType;

typedef enum {
  PANGO_UNDERLINE_NONE,
  PANGO_UNDERLINE_SINGLE,
  PANGO_UNDERLINE_DOUBLE,
  PANGO_UNDERLINE_LOW
} PangoUnderline;

struct _PangoAttribute
{
  const PangoAttrClass *klass;
  guint start_index;
  guint end_index;
};

struct _PangoAttrClass
{
  PangoAttrType type;
  PangoAttribute * (*copy) (const PangoAttribute *attr);
  void             (*destroy) (PangoAttribute *attr);
  gboolean         (*compare) (const PangoAttribute *attr1, const PangoAttribute *attr2);
};

struct _PangoAttrString
{
  PangoAttribute attr;
  char *value;
};

struct _PangoAttrInt
{
  PangoAttribute attr;
  int value;
};

struct _PangoAttrColor
{
  PangoAttribute attr;
  guint16 red;
  guint16 green;
  guint16 blue;
};

struct _PangoAttrFontDesc
{
  PangoAttribute attr;
  PangoFontDescription desc;
};

PangoAttrType    pango_attr_type_register (const gchar          *name);

PangoAttribute * pango_attribute_copy          (const PangoAttribute *attr);
void             pango_attribute_destroy       (PangoAttribute       *attr);
gboolean         pango_attribute_compare       (const PangoAttribute *attr1,
						const PangoAttribute *attr2);

PangoAttribute *pango_attr_lang_new          (const char                 *lang);
PangoAttribute *pango_attr_family_new        (const char                 *family);
PangoAttribute *pango_attr_foreground_new    (guint16                     red,
					      guint16                     green,
					      guint16                     blue);
PangoAttribute *pango_attr_background_new    (guint16                     red,
					      guint16                     green,
					      guint16                     blue);
PangoAttribute *pango_attr_size_new          (int                         size);
PangoAttribute *pango_attr_style_new         (PangoStyle                  style);
PangoAttribute *pango_attr_weight_new        (PangoWeight                 weight);
PangoAttribute *pango_attr_variant_new       (PangoVariant                variant);
PangoAttribute *pango_attr_stretch_new       (PangoStretch                stretch);
PangoAttribute *pango_attr_font_desc_new     (const PangoFontDescription *desc);
PangoAttribute *pango_attr_underline_new     (PangoUnderline              underline);
PangoAttribute *pango_attr_strikethrough_new (gboolean                    strikethrough);
PangoAttribute *pango_attr_rise_new          (int                         rise);

PangoAttrList *     pango_attr_list_new          (void);
void                pango_attr_list_ref          (PangoAttrList  *list);
void                pango_attr_list_unref        (PangoAttrList  *list);
PangoAttrList *     pango_attr_list_copy         (PangoAttrList  *list);
void                pango_attr_list_insert       (PangoAttrList  *list,
						  PangoAttribute *attr);
void                pango_attr_list_change       (PangoAttrList  *list,
						  PangoAttribute *attr);
PangoAttrIterator * pango_attr_list_get_iterator (PangoAttrList  *list);

void               pango_attr_iterator_range    (PangoAttrIterator     *iterator,
                                                 gint                  *start,
                                                 gint                  *end);
gboolean           pango_attr_iterator_next     (PangoAttrIterator     *iterator);
PangoAttrIterator *pango_attr_iterator_copy     (PangoAttrIterator     *iterator);
void               pango_attr_iterator_destroy  (PangoAttrIterator     *iterator);
PangoAttribute *   pango_attr_iterator_get      (PangoAttrIterator     *iterator,
                                                 PangoAttrType          type);
void               pango_attr_iterator_get_font (PangoAttrIterator     *iterator,
                                                 PangoFontDescription  *base,
                                                 PangoFontDescription  *current,
                                                 GSList               **extra_attrs);



#ifdef __cplusplus
}
#endif /* __cplusplus */

#endif /* __PANGO_ATTRIBUTES_H__ */
