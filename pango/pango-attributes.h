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

#include <pango-font.h>

#ifdef __cplusplus
extern "C" {
#endif /* __cplusplus */

typedef struct _PangoAttribute         PangoAttribute;
typedef struct _PangoAttrClass         PangoAttrClass;

typedef struct _PangoAttrLang          PangoAttrLang;
typedef struct _PangoAttrStyle         PangoAttrStyle;
typedef struct _PangoAttrWeight        PangoAttrWeight;
typedef struct _PangoAttrVariant       PangoAttrVariant;
typedef struct _PangoAttrStretch       PangoAttrStretch;
typedef struct _PangoAttrSize          PangoAttrSize;
typedef struct _PangoAttrColor         PangoAttrColor;
typedef struct _PangoAttrUnderline     PangoAttrUnderline;
typedef struct _PangoAttrStrikethrough PangoAttrStrikethrough;
typedef struct _PangoAttrRise          PangoAttrRise;

typedef struct _PangoAttrList          PangoAttribute;
typedef struct _PangoAttrIterator      PangoAttrClass;

typedef guint PangoAttributeType;

typedef enum {
  PANGO_ATTR_LANG,
  PANGO_ATTR_FAMILY,
  PANGO_ATTR_STYLE,
  PANGO_ATTR_WEIGHT,
  PANGO_ATTR_VARIANT,
  PANGO_ATTR_STRETCH,
  PANGO_ATTR_SIZE,
  PANGO_ATTR_FOREGROUND,
  PANGO_ATTR_BACKGROUND,
  PANGO_ATTR_UNDERLINE,
  PANGO_ATTR_STRIKETHROUGH,
  PANGO_ATTR_RISE
} PangoAttrType;

struct _PangoAttribute
{
  PangoAttrClass *klass;
  guint start_index;
  guint end_index;
};

struct _PangoAttrClass
{
  PangoAttributeType type;
  PangoAttribute * (*copy) (const PangoAttribute *attr);
  void             (*destroy) (PangoAttribute *attr);
  gboolean         (*compare) (const PangoAttribute *attr1, const PangoAttribute *attr2);
};

struct _PangoAttrLang
{
  PangoAttribute attr;
  char *lang;
};

struct _PangoAttrFamily
{
  PangoAttribute attr;
  char *family_list;
};

struct _PangoAttrStyle
{
  PangoAttribute attr;
  PangoStyle style;
};

struct _PangoAttrWeight
{
  PangoAttribute attr;
  PangoWeight weight;
};

struct _PangoAttrVariant
{
  PangoAttribute attr;
  PangoVariant variant;
};

struct _PangoAttrStretch
{
  PangoAttribute attr;
  PangoStretch stretch;
};

struct _PangoAttrSize
{
  PangoAttribute attr;
  double size;
};

struct _PangoAttrColor
{
  PangoAttribute attr;
  guint16 r;
  guint16 g;
  guint16 b;
};

struct _PangoAttrUnderline
{
  PangoAttribute attr;
  gboolean underline;
};

struct _PangoAttrStrikethrough
{
  PangoAttribute attr;
  gboolean strikethrough;
};

struct _PangoAttrRise
{
  PangoAttribute attr;
  int rise;
};

PangoAttributeType pango_attribute_register_type (const gchar          *name);
PangoAttribute *   pango_attribute_copy          (const PangoAttribute *attr);
PangoAttribute *   pango_attribute_destroy       (PangoAttribute       *attr);
gboolean           pango_attribute_compare       (const PangoAttribute *attr1,
						  const PangoAttribute *attr2);

PangoAttribute *pango_attr_lang_new          (const char   *lang);
PangoAttribute *pango_attr_color_new         (guint16       red,
					      guint16       green,
					      guint16       blue);
PangoAttribute *pango_attr_size_new          (double        size);
PangoAttribute *pango_attr_style_new         (PangoStyle    style);
PangoAttribute *pango_attr_weight_new        (PangoWeight   weight);
PangoAttribute *pango_attr_variant_new       (PangoVariant  variant);
PangoAttribute *pango_attr_stretch_new       (PangoStretch  stretch);
PangoAttribute *pango_attr_underline_new     (gboolean      underline);
PangoAttribute *pango_attr_strikethrough_new (gboolean      strikethrough);
PangoAttribute *pango_attr_rise_new          (int           rise);

PangoAttrList *    pango_attr_list_new          (void);
PangoAttrList *    pango_attr_list_ref          (PangoAttrList  *list);
PangoAttrList *    pango_attr_list_unref        (PangoAttrList  *list);
PangoAttrList *    pango_attr_list_append       (PangoAttrList  *list,
						 PangoAttribute *attr);
PangoAttrList *    pango_attr_list_change       (PangoAttrList  *list,
						 PangoAttribute *attr);
PangoAttrIterator *pango_attr_list_get_iterator (PangoAttrList  *list);

int             pango_attr_iterator_next     (PangoAttrIterator    *iterator);
void            pango_attr_iterator_destroy  (PangoAttrIterator    *iterator);
PangoAttribute *pango_attr_iterator_get      (PangoAttrIterator    *iterator,
					      PangoAttrType         type);
void           *pango_attr_iterator_get_font (PangoAttrIterator    *iterator,
					      PangoFontDescription *base,
					      PangoFontDescription *current);

#ifdef __cplusplus
}
#endif /* __cplusplus */

#endif __PANGO_ATTRIBUTES_H__
