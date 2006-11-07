/* Pango
 * pango-layout.c: Highlevel layout driver
 *
 * Copyright (C) 2000, 2001 Red Hat Software
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

#include <config.h>
#include "pango-glyph.h"		/* For pango_shape() */
#include "pango-break.h"
#include "pango-item.h"
#include "pango-engine.h"
#include "pango-impl-utils.h"
#include <string.h>

#include "pango-layout-private.h"


typedef struct _Extents Extents;
typedef struct _ItemProperties ItemProperties;
typedef struct _ParaBreakState ParaBreakState;

struct _Extents
{
  /* Vertical position of the line's baseline in layout coords */
  int baseline;
  
  /* Line extents in layout coords */
  PangoRectangle ink_rect;
  PangoRectangle logical_rect;
};

struct _ItemProperties
{
  PangoUnderline  uline;
  gboolean        strikethrough;
  gint            rise;
  gint            letter_spacing;
  gboolean        shape_set;
  PangoRectangle *shape_ink_rect;
  PangoRectangle *shape_logical_rect;
};

struct _PangoLayoutIter
{
  PangoLayout *layout;
  GSList *line_list_link;
  PangoLayoutLine *line;

  /* If run is NULL, it means we're on a "virtual run"
   * at the end of the line with 0 width
   */
  GSList *run_list_link;
  PangoLayoutRun *run; /* FIXME nuke this, just keep the link */
  int index;

  /* layout extents in layout coordinates */
  PangoRectangle logical_rect;

  /* list of Extents for each line in layout coordinates */
  GSList *line_extents;
  GSList *line_extents_link;
  
  /* X position of the current run */
  int run_x;

  /* Extents of the current run */
  PangoRectangle run_logical_rect;

  /* this run is left-to-right */
  gboolean ltr;

  /* X position of the left side of the current cluster */
  int cluster_x;

  /* The width of the current cluster */
  int cluster_width;

  /* glyph offset to the current cluster start */
  int cluster_start;

  /* first glyph in the next cluster */
  int next_cluster_glyph;

  /* number of Unicode chars in current cluster */
  int cluster_num_chars;

  /* visual position of current character within the cluster */
  int character_position;

  /* the real width of layout */
  int layout_width;
};

typedef struct _PangoLayoutLinePrivate PangoLayoutLinePrivate;

struct _PangoLayoutLinePrivate
{
  PangoLayoutLine line;
  guint ref_count;
};

struct _PangoLayoutClass
{
  GObjectClass parent_class;


};

#define LINE_IS_VALID(line) ((line)->layout != NULL)

#ifdef G_DISABLE_CHECKS
#define ITER_IS_INVALID(iter) FALSE
#else
#define ITER_IS_INVALID(iter) check_invalid ((iter), G_STRLOC)
static gboolean
check_invalid (PangoLayoutIter *iter,
               const char      *loc)
{
  if (iter->line->layout == NULL)
    {
      g_warning ("%s: PangoLayout changed since PangoLayoutIter was created, iterator invalid", loc);
      return TRUE;
    }
  else
    {
      return FALSE;
    }
}
#endif

static void pango_layout_clear_lines (PangoLayout *layout);
static void pango_layout_check_lines (PangoLayout *layout);

static PangoAttrList *pango_layout_get_effective_attributes (PangoLayout *layout);

static PangoLayoutLine * pango_layout_line_new         (PangoLayout     *layout);
static void              pango_layout_line_postprocess (PangoLayoutLine *line,
							ParaBreakState  *state);

static int *pango_layout_line_get_log2vis_map (PangoLayoutLine  *line,
					       gboolean          strong);
static int *pango_layout_line_get_vis2log_map (PangoLayoutLine  *line,
					       gboolean          strong);

static void pango_layout_get_item_properties (PangoItem      *item,
					      ItemProperties *properties);

static void pango_layout_finalize    (GObject          *object);

G_DEFINE_TYPE (PangoLayout, pango_layout, G_TYPE_OBJECT)

static void
pango_layout_init (PangoLayout *layout)
{
  layout->attrs = NULL;
  layout->font_desc = NULL;
  layout->text = NULL;
  layout->length = 0;
  layout->width = -1;
  layout->indent = 0;

  layout->alignment = PANGO_ALIGN_LEFT;
  layout->justify = FALSE;
  layout->auto_dir = TRUE;

  layout->log_attrs = NULL;
  layout->lines = NULL;

  layout->tab_width = -1;

  layout->wrap = PANGO_WRAP_WORD;
  layout->ellipsize = PANGO_ELLIPSIZE_NONE;
}

static void
pango_layout_class_init (PangoLayoutClass *klass)
{
  GObjectClass *object_class = G_OBJECT_CLASS (klass);
  
  object_class->finalize = pango_layout_finalize;
}

static void
pango_layout_finalize (GObject *object)
{
  PangoLayout *layout;

  layout = PANGO_LAYOUT (object);

  pango_layout_clear_lines (layout);
  
  if (layout->context)
    g_object_unref (layout->context);
  
  if (layout->attrs)
    pango_attr_list_unref (layout->attrs);
  if (layout->text)
    g_free (layout->text);

  if (layout->font_desc)
    pango_font_description_free (layout->font_desc);

  if (layout->tabs)
    pango_tab_array_free (layout->tabs);
  
  G_OBJECT_CLASS (pango_layout_parent_class)->finalize (object);
}


/**
 * pango_layout_new:
 * @context: a #PangoContext
 * 
 * Create a new #PangoLayout object with attributes initialized to
 * default values for a particular #PangoContext.
 * 
 * Return value: the newly allocated #PangoLayout, with a reference
 *               count of one, which should be freed with
 *               g_object_unref().
 **/
PangoLayout *
pango_layout_new (PangoContext *context)
{
  PangoLayout *layout;

  g_return_val_if_fail (context != NULL, NULL);

  layout = g_object_new (PANGO_TYPE_LAYOUT, NULL);

  layout->context = context;
  g_object_ref (context);

  return layout;
}

/**
 * pango_layout_copy:
 * @src: a #PangoLayout
 * 
 * Does a deep copy-by-value of the @src layout. The attribute list,
 * tab array, and text from the original layout are all copied by
 * value.
 * 
 * Return value: the newly allocated #PangoLayout, with a reference
 *               count of one, which should be freed with
 *               g_object_unref().
 **/
PangoLayout*
pango_layout_copy (PangoLayout *src)
{
  PangoLayout *layout;
  
  g_return_val_if_fail (PANGO_IS_LAYOUT (src), NULL);

  layout = pango_layout_new (src->context);

  if (src->attrs)
    layout->attrs = pango_attr_list_copy (src->attrs);

  if (src->font_desc)
    layout->font_desc = pango_font_description_copy (src->font_desc);

  layout->text = g_strdup (src->text);
  layout->length = src->length;
  layout->width = src->width;
  layout->indent = src->indent;
  layout->spacing = src->spacing;
  layout->justify = src->justify;
  layout->auto_dir = src->auto_dir;
  layout->alignment = src->alignment;
  layout->n_chars = src->n_chars;
  layout->tab_width = src->tab_width;

  if (src->tabs)
    layout->tabs = pango_tab_array_copy (src->tabs);
  layout->wrap = src->wrap;  
  layout->ellipsize = src->ellipsize;
  
  /* log_attrs, lines fields are updated by check_lines */

  return layout;
}

/**
 * pango_layout_get_context:
 * @layout: a #PangoLayout
 * 
 * Retrieves the #PangoContext used for this layout.
 * 
 * Return value: the #PangoContext for the layout. This does not
 * have an additional refcount added, so if you want to keep
 * a copy of this around, you must reference it yourself.
 **/
PangoContext *
pango_layout_get_context (PangoLayout *layout)
{
  g_return_val_if_fail (layout != NULL, NULL);

  return layout->context;
}

/**
 * pango_layout_set_width:
 * @layout: a #PangoLayout.
 * @width: the desired width in Pango units, or -1 to indicate that no
 *         wrapping should be performed.
 * 
 * Sets the width to which the lines of the #PangoLayout should wrap.
 **/
void
pango_layout_set_width (PangoLayout *layout,
			int          width)
{
  g_return_if_fail (layout != NULL);

  if (width != layout->width)
    {
      layout->width = width;
      pango_layout_clear_lines (layout);
    }
}

/**
 * pango_layout_get_width:
 * @layout: a #PangoLayout
 * 
 * Gets the width to which the lines of the #PangoLayout should wrap.
 * 
 * Return value: the width, or -1 if no width set.
 **/
int
pango_layout_get_width (PangoLayout    *layout)
{
  g_return_val_if_fail (layout != NULL, 0);
  return layout->width;
}

/**
 * pango_layout_set_wrap:
 * @layout: a #PangoLayout
 * @wrap: the wrap mode
 * 
 * Sets the wrap mode; the wrap mode only has effect if a width
 * is set on the layout with pango_layout_set_width(). To turn off wrapping,
 * set the width to -1.
 **/
void
pango_layout_set_wrap (PangoLayout  *layout,
                       PangoWrapMode wrap)
{
  g_return_if_fail (PANGO_IS_LAYOUT (layout));

  if (layout->wrap != wrap)
    {
      pango_layout_clear_lines (layout);
      layout->wrap = wrap;
    }
}

/**
 * pango_layout_get_wrap:
 * @layout: a #PangoLayout
 * 
 * Gets the wrap mode for the layout.
 * 
 * Return value: active wrap mode.
 **/
PangoWrapMode
pango_layout_get_wrap (PangoLayout *layout)
{
  g_return_val_if_fail (PANGO_IS_LAYOUT (layout), 0);
  
  return layout->wrap;
}

/**
 * pango_layout_set_indent
 * @layout: a #PangoLayout.
 * @indent: the amount by which to indent.
 * 
 * Sets the width in Pango units to indent each paragraph. A negative value
 * of @indent will produce a hanging indentation. That is, the first line will
 * have the full width, and subsequent lines will be indented by the
 * absolute value of @indent.
 **/
void
pango_layout_set_indent (PangoLayout *layout,
			 int          indent)
{
  g_return_if_fail (layout != NULL);

  if (indent != layout->indent)
    {
      layout->indent = indent;
      pango_layout_clear_lines (layout);
    }
}

/**
 * pango_layout_get_indent:
 * @layout: a #PangoLayout
 * 
 * Gets the paragraph indent width in Pango units. A negative value
 * indicates a hanging indentation.
 * 
 * Return value: the indent.
 **/
int
pango_layout_get_indent (PangoLayout *layout)
{
  g_return_val_if_fail (layout != NULL, 0);
  return layout->indent;
}

/**
 * pango_layout_set_spacing:
 * @layout: a #PangoLayout.
 * @spacing: the amount of spacing
 * 
 * Sets the amount of spacing in #PangoGlyphUnit between the lines of the
 * layout.
 *
 **/
void
pango_layout_set_spacing (PangoLayout *layout,
                          int          spacing)
{
  g_return_if_fail (layout != NULL);

  if (spacing != layout->spacing)
    {
      layout->spacing = spacing;
      pango_layout_clear_lines (layout);
    }
}

/**
 * pango_layout_get_spacing:
 * @layout: a #PangoLayout
 * 
 * Gets the amount of spacing in #PangoGlyphUnit between the lines of the
 * layout.
 * 
 * Return value: the spacing.
 **/
int
pango_layout_get_spacing (PangoLayout *layout)
{
  g_return_val_if_fail (layout != NULL, 0);
  return layout->spacing;
}

/**
 * pango_layout_set_attributes:
 * @layout: a #PangoLayout
 * @attrs: a #PangoAttrList
 * 
 * Sets the text attributes for a layout object.
 **/
void
pango_layout_set_attributes (PangoLayout   *layout,
			     PangoAttrList *attrs)
{
  PangoAttrList *old_attrs;
  g_return_if_fail (layout != NULL);

  old_attrs = layout->attrs;

  layout->attrs = attrs;
  if (layout->attrs)
    pango_attr_list_ref (layout->attrs);
  pango_layout_clear_lines (layout);

  if (old_attrs)
    pango_attr_list_unref (old_attrs);
  layout->tab_width = -1;
}

/**
 * pango_layout_get_attributes:
 * @layout: a #PangoLayout
 * 
 * Gets the attribute list for the layout, if any.
 * 
 * Return value: a #PangoAttrList.
 **/
PangoAttrList*
pango_layout_get_attributes (PangoLayout *layout)
{
  g_return_val_if_fail (PANGO_IS_LAYOUT (layout), NULL);

  return layout->attrs;
}

/**
 * pango_layout_set_font_description:
 * @layout: a #PangoLayout
 * @desc: the new #PangoFontDescription, or %NULL to unset the
 *        current font description
 * 
 * Sets the default font description for the layout. If no font 
 * description is set on the layout, the font description from
 * the layout's context is used.
 **/
void
pango_layout_set_font_description (PangoLayout                 *layout,
				    const PangoFontDescription *desc)
{
  g_return_if_fail (layout != NULL);

  if (desc != layout->font_desc)
    {
      if (layout->font_desc)
	pango_font_description_free (layout->font_desc);
      
      if (desc)
	layout->font_desc = pango_font_description_copy (desc);
      else
	layout->font_desc = NULL;
      
      pango_layout_clear_lines (layout);
      layout->tab_width = -1;
    }
}

/**
 * pango_layout_get_font_description:
 * @layout: a #PangoLayout
 * 
 * Gets the font description for the layout, if any.
 * 
 * Return value: a pointer to the layout's font description,
 *  or %NULL if the font description from the layout's
 *  context is inherited. This value is owned by the layout
 *  and must not be modified or freed.
 *
 * Since: 1.8
 **/
G_CONST_RETURN PangoFontDescription *
pango_layout_get_font_description (PangoLayout *layout)
{
  g_return_val_if_fail (PANGO_IS_LAYOUT (layout), NULL);

  return layout->font_desc;
}

/**
 * pango_layout_set_justify:
 * @layout: a #PangoLayout
 * @justify: whether the lines in the layout should be justified.
 * 
 * Sets whether each complete line should be stretched to
 * fill the entire width of the layout. This stretching is typically
 * done by adding whitespace, but for some scripts (such as Arabic),
 * the justification may be done in more complex ways, like extending
 * the characters.
 *
 * Note that as of Pango-1.10, this functionality is not yet implemented.
 **/
void
pango_layout_set_justify (PangoLayout *layout,
			  gboolean     justify)
{
  g_return_if_fail (layout != NULL);

  layout->justify = justify;
}

/**
 * pango_layout_get_justify:
 * @layout: a #PangoLayout
 * 
 * Gets whether each complete line should be stretched to fill the entire
 * width of the layout.
 * 
 * Return value: the justify.
 **/
gboolean
pango_layout_get_justify (PangoLayout *layout)
{
  g_return_val_if_fail (layout != NULL, FALSE);
  return layout->justify;
}

/**
 * pango_layout_set_auto_dir:
 * @layout: a #PangoLayout
 * @auto_dir: if %TRUE, compute the bidirectional base direction
 *   from the layout's contents.
 * 
 * Sets whether to calculate the bidirectional base direction
 * for the layout according to the contents of the layout;
 * when this flag is on (the default), then paragraphs in
   @layout that begin with strong right-to-left characters
 * (Arabic and Hebrew principally), will have right-to-left
 * layout, paragraphs with letters from other scripts will
 * have left-to-right layout. Paragraphs with only neutral
 * characters get their direction from the surrounding paragraphs.
 *
 * When %FALSE, the choice between left-to-right and
 * right-to-left layout is done according to the base direction
 * of the layout's #PangoContext. (See pango_context_set_base_dir()).
 *
 * When the auto-computed direction of a paragraph differs from the
 * base direction of the context, the interpretation of
 * %PANGO_ALIGN_LEFT and %PANGO_ALIGN_RIGHT are swapped. 
 *
 * Since: 1.4
 **/
void
pango_layout_set_auto_dir (PangoLayout *layout,
			   gboolean     auto_dir)
{
  g_return_if_fail (PANGO_IS_LAYOUT (layout));

  auto_dir = auto_dir != FALSE;

  if (auto_dir != layout->auto_dir)
    {
      layout->auto_dir = auto_dir;
      pango_layout_clear_lines (layout);
    }
}

/**
 * pango_layout_get_auto_dir:
 * @layout: a #PangoLayout
 * 
 * Gets whether to calculate the bidirectional base direction
 * for the layout according to the contents of the layout.
 * See pango_layout_set_auto_dir().
 * 
 * Return value: %TRUE if the bidirectional base direction
 *   is computed from the layout's contents, %FALSE otherwise.
 *
 * Since: 1.4
 **/
gboolean
pango_layout_get_auto_dir (PangoLayout *layout)
{
  g_return_val_if_fail (PANGO_IS_LAYOUT (layout), FALSE);
  
  return layout->auto_dir;
}

/**
 * pango_layout_set_alignment:
 * @layout: a #PangoLayout
 * @alignment: the alignment
 * 
 * Sets the alignment for the layout: how partial lines are
 * positioned within the horizontal space available.
 **/
void
pango_layout_set_alignment (PangoLayout   *layout,
			    PangoAlignment alignment)
{
  g_return_if_fail (layout != NULL);

  layout->alignment = alignment;
}

/**
 * pango_layout_get_alignment:
 * @layout: a #PangoLayout
 * 
 * Gets the alignment for the layout: how partial lines are
 * positioned within the horizontal space available.
 * 
 * Return value: the alignment.
 **/
PangoAlignment
pango_layout_get_alignment (PangoLayout *layout)
{
  g_return_val_if_fail (layout != NULL, PANGO_ALIGN_LEFT);
  return layout->alignment;
}


/**
 * pango_layout_set_tabs:
 * @layout: a #PangoLayout 
 * @tabs: a #PangoTabArray
 * 
 * Sets the tabs to use for @layout, overriding the default tabs
 * (by default, tabs are every 8 spaces). If @tabs is %NULL, the default
 * tabs are reinstated. @tabs is copied into the layout; you must
 * free your copy of @tabs yourself.
 **/
void
pango_layout_set_tabs (PangoLayout   *layout,
                       PangoTabArray *tabs)
{
  g_return_if_fail (PANGO_IS_LAYOUT (layout));
  
  if (layout->tabs)
    pango_tab_array_free (layout->tabs);

  layout->tabs = tabs ? pango_tab_array_copy (tabs) : NULL;
}

/**
 * pango_layout_get_tabs:
 * @layout: a #PangoLayout
 * 
 * Gets the current #PangoTabArray used by this layout. If no
 * #PangoTabArray has been set, then the default tabs are in use
 * and %NULL is returned. Default tabs are every 8 spaces.
 * The return value should be freed with pango_tab_array_free().
 * 
 * Return value: a copy of the tabs for this layout, or %NULL.
 **/
PangoTabArray*
pango_layout_get_tabs (PangoLayout *layout)
{
  g_return_val_if_fail (PANGO_IS_LAYOUT (layout), NULL);

  if (layout->tabs)
    return pango_tab_array_copy (layout->tabs);
  else
    return NULL;
}

/**
 * pango_layout_set_single_paragraph_mode:
 * @layout: a #PangoLayout
 * @setting: new setting
 * 
 * If @setting is %TRUE, do not treat newlines and similar characters
 * as paragraph separators; instead, keep all text in a single paragraph,
 * and display a glyph for paragraph separator characters. Used when
 * you want to allow editing of newlines on a single text line.
 * 
 **/
void
pango_layout_set_single_paragraph_mode (PangoLayout *layout,
                                        gboolean     setting)
{
  g_return_if_fail (PANGO_IS_LAYOUT (layout));

  setting = setting != FALSE;

  if (layout->single_paragraph != setting)
    {
      layout->single_paragraph = setting;

      pango_layout_clear_lines (layout);
    }
}

/**
 * pango_layout_get_single_paragraph_mode:
 * @layout: a #PangoLayout
 * 
 * Obtains the value set by pango_layout_set_single_paragraph_mode().
 * 
 * Return value: %TRUE if the layout does not break paragraphs at 
 * paragraph separator characters, %FALSE otherwise.
 **/
gboolean
pango_layout_get_single_paragraph_mode (PangoLayout *layout)
{
  g_return_val_if_fail (PANGO_IS_LAYOUT (layout), FALSE);

  return layout->single_paragraph;
}

/**
 * pango_layout_set_ellipsize:
 * @layout: a #PangoLayout
 * @ellipsize: the new ellipsization mode for @layout
 * 
 * Sets the type of ellipsization being performed for @layout.
 * Depending on the ellipsization mode @ellipsize text is
 * removed from the start, middle, or end of lines so they
 * fit within the width of layout set with pango_layout_set_width ().
 *
 * If the layout contains characters such as newlines that
 * force it to be layed out in multiple lines, then each line
 * is ellipsized separately.
 *
 * Since: 1.6
 **/
void
pango_layout_set_ellipsize (PangoLayout        *layout,
			    PangoEllipsizeMode  ellipsize)
{
  g_return_if_fail (PANGO_IS_LAYOUT (layout));

  if (ellipsize != layout->ellipsize)
    {
      layout->ellipsize = ellipsize;
      
      pango_layout_clear_lines (layout);
    }
}

/**
 * pango_layout_get_ellipsize:
 * @layout: a #PangoLayout
 * 
 * Gets the type of ellipsization being performed for @layout.
 * See pango_layout_set_ellipsize()
 * 
 * Return value: the current ellipsization mode for @layout.
 *
 * Since: 1.6
 **/
PangoEllipsizeMode
pango_layout_get_ellipsize (PangoLayout *layout)
{
  g_return_val_if_fail (PANGO_IS_LAYOUT (layout), PANGO_ELLIPSIZE_NONE);

  return layout->ellipsize;
}

/**
 * pango_layout_set_text:
 * @layout: a #PangoLayout
 * @text: a valid UTF-8 string
 * @length: maximum length of @text, in bytes. -1 indicates that
 *          the string is nul-terminated and the length should be
 *          calculated.  The text will also be truncated on
 *          encountaring a nul-termination even when @length is
 *          positive.
 * 
 * Sets the text of the layout.
 **/
void
pango_layout_set_text (PangoLayout *layout,
		       const char  *text,
		       int          length)
{
  char *old_text, *start, *end;
  
  g_return_if_fail (layout != NULL);
  g_return_if_fail (length == 0 || text != NULL);

  old_text = layout->text;
  
  if (length < 0)
    layout->text = g_strdup (text);
  else if (length > 0)
    /* This is not exactly what we want.  We don't need the padding...
     */
    layout->text = g_strndup (text, length);
  else
    layout->text = g_malloc0 (1);

  layout->length = strlen (layout->text);

  /* validate it, and replace invalid bytes with '?'
   */
  start = layout->text;
  for (;;) {
    gboolean valid;
    
    valid = g_utf8_validate (start, -1, (const char **)&end);

    if (!*end)
      break;

    if (!valid)
      *end++ = '?';

    start = end;
  }

  if (start != layout->text)
    /* TODO: Write out the beginning excerpt of text? */
    g_warning ("Invalid UTF-8 string passed to pango_layout_set_text()");

  layout->n_chars = g_utf8_strlen (layout->text, -1);

  pango_layout_clear_lines (layout);
  
  if (old_text)
    g_free (old_text);
}

/**
 * pango_layout_get_text:
 * @layout: a #PangoLayout
 * 
 * Gets the text in the layout. The returned text should not
 * be freed or modified.
 * 
 * Return value: the text in the @layout.
 **/
G_CONST_RETURN char*
pango_layout_get_text (PangoLayout *layout)
{
  g_return_val_if_fail (PANGO_IS_LAYOUT (layout), NULL);

  return layout->text;
}

/**
 * pango_layout_set_markup:
 * @layout: a #PangoLayout
 * @markup: marked-up text
 * @length: length of marked-up text in bytes, or -1 if @markup is
 * nul-terminated
 *
 * Same as pango_layout_set_markup_with_accel(), but
 * the markup text isn't scanned for accelerators.
 * 
 **/
void
pango_layout_set_markup (PangoLayout *layout,
                         const char  *markup,
                         int          length)
{
  pango_layout_set_markup_with_accel (layout, markup, length, 0, NULL);
}

/**
 * pango_layout_set_markup_with_accel:
 * @layout: a #PangoLayout
 * @markup: marked-up text 
 * (see <link linkend="PangoMarkupFormat">markup format</link>)
 * @length: length of marked-up text in bytes, or -1 if @markup is
 * nul-terminated
 * @accel_marker: marker for accelerators in the text
 * @accel_char: return location for first located accelerator, or %NULL
 * 
 * Sets the layout text and attribute list from marked-up text (see
 * <link linkend="PangoMarkupFormat">markup format</link>). Replaces
 * the current text and attribute list.
 *
 * If @accel_marker is nonzero, the given character will mark the
 * character following it as an accelerator. For example, the accel
 * marker might be an ampersand or underscore. All characters marked
 * as an accelerator will receive a %PANGO_UNDERLINE_LOW attribute,
 * and the first character so marked will be returned in @accel_char.
 * Two @accel_marker characters following each other produce a single
 * literal @accel_marker character.
 **/
void
pango_layout_set_markup_with_accel (PangoLayout    *layout,
                                    const char     *markup,
                                    int             length,
                                    gunichar        accel_marker,
                                    gunichar       *accel_char)
{
  PangoAttrList *list = NULL;
  char *text = NULL;
  GError *error;
  
  g_return_if_fail (PANGO_IS_LAYOUT (layout));
  g_return_if_fail (markup != NULL);
  
  error = NULL;
  if (!pango_parse_markup (markup, length,
                           accel_marker,
                           &list, &text,
                           accel_char,
                           &error))
    {
      g_warning ("pango_layout_set_markup_with_accel: %s", error->message);
      g_error_free (error);
      return;
    }
  
  pango_layout_set_text (layout, text, -1);
  pango_layout_set_attributes (layout, list);
  pango_attr_list_unref (list);
  g_free (text);
}

/**
 * pango_layout_context_changed:
 * @layout: a #PangoLayout
 * 
 * Forces recomputation of any state in the #PangoLayout that
 * might depend on the layout's context. This function should
 * be called if you make changes to the context subsequent
 * to creating the layout.
 **/
void
pango_layout_context_changed (PangoLayout *layout)
{
  pango_layout_clear_lines (layout);
  layout->tab_width = -1;
}

/**
 * pango_layout_get_log_attrs:
 * @layout: a #PangoLayout
 * @attrs: location to store a pointer to an array of logical attributes
 *         This value must be freed with g_free().
 * @n_attrs: location to store the number of the attributes in the
 *           array. (The stored value will be one more than the total number
 *           of characters in the layout, since there need to be attributes
 *           corresponding to both the position before the first character
 *           and the position after the last character.)
 * 
 * Retrieves an array of logical attributes for each character in
 * the @layout. 
 **/
void
pango_layout_get_log_attrs (PangoLayout    *layout,
			    PangoLogAttr  **attrs,
			    gint           *n_attrs)
{
  g_return_if_fail (layout != NULL);

  pango_layout_check_lines (layout);

  if (attrs)
    {
      *attrs = g_new (PangoLogAttr, layout->n_chars + 1);
      memcpy (*attrs, layout->log_attrs, sizeof(PangoLogAttr) * (layout->n_chars + 1));
    }
  
  if (n_attrs)
    *n_attrs = layout->n_chars + 1;
}


/**
 * pango_layout_get_line_count:
 * @layout: #PangoLayout
 * 
 * Retrieves the count of lines for the @layout.
 * 
 * Return value: the line count.
 **/
int
pango_layout_get_line_count (PangoLayout   *layout)
{
  g_return_val_if_fail (layout != NULL, 0);

  pango_layout_check_lines (layout);
  return g_slist_length (layout->lines);
}

/**
 * pango_layout_get_lines:
 * @layout: a #PangoLayout
 * 
 * Returns the lines of the @layout as a list.
 * 
 * Return value: a #GSList containing the lines in the layout. This
 * points to internal data of the #PangoLayout and must be used with
 * care. It will become invalid on any change to the layout's
 * text or properties.
 **/
GSList *
pango_layout_get_lines (PangoLayout *layout)
{
  pango_layout_check_lines (layout);
  return layout->lines;
}

/**
 * pango_layout_get_line:
 * @layout: a #PangoLayout
 * @line: the index of a line, which must be between 0 and
 *        <literal>pango_layout_get_line_count(layout) - 1</literal>, inclusive.
 * 
 * Retrieves a particular line from a #PangoLayout.
 * 
 * Return value: the requested #PangoLayoutLine, or %NULL if the
 *               index is out of range. This layout line can
 *               be ref'ed and retained, but will become invalid
 *               if changes are made to the #PangoLayout.
 **/
PangoLayoutLine *
pango_layout_get_line (PangoLayout *layout,
		       int          line)
{
  GSList *list_item;
  g_return_val_if_fail (layout != NULL, NULL);
  g_return_val_if_fail (line >= 0, NULL);

  if (line < 0)
    return NULL;

  pango_layout_check_lines (layout);
  list_item = g_slist_nth (layout->lines, line);
  if (list_item)
    return list_item->data;
  return NULL;
}

/**
 * pango_layout_line_index_to_x:
 * @line:     a #PangoLayoutLine
 * @index_:   byte offset of a grapheme within the layout
 * @trailing: an integer indicating the edge of the grapheme to retrieve 
 *            the position of. If 0, the trailing edge of the grapheme, 
 *            if > 0, the leading of the grapheme.
 * @x_pos: location to store the x_offset (in #PangoGlyphUnit)
 * 
 * Converts an index within a line to a X position.
 *
 **/
void
pango_layout_line_index_to_x (PangoLayoutLine  *line,
			      int               index,
			      int               trailing,
			      int              *x_pos)
{
  PangoLayout *layout = line->layout;
  GSList *run_list = line->runs;
  int width = 0;

  while (run_list)
    {
      PangoLayoutRun *run = run_list->data;
      ItemProperties properties;
      
      pango_layout_get_item_properties (run->item, &properties);

      if (run->item->offset <= index && run->item->offset + run->item->length > index)
	{
	  if (properties.shape_set)
	    {
	      if (x_pos)
		*x_pos = width + (trailing > 0 ? properties.shape_logical_rect->width : 0);
	    }
	  else
	    {
	      int offset = g_utf8_pointer_to_offset (layout->text, layout->text + index);
	      if (trailing)
		{
		  while (index < line->start_index + line->length &&
			 offset + 1 < layout->n_chars &&
			 !layout->log_attrs[offset + 1].is_cursor_position)
		    {
		      offset++;
		      index = g_utf8_next_char (layout->text + index) - layout->text;
		    }
		}
	      else
		{
		  while (index > line->start_index &&
			 !layout->log_attrs[offset].is_cursor_position)
		    {
		      offset--;
		      index = g_utf8_prev_char (layout->text + index) - layout->text;
		    }
		  
		}
	      
	      pango_glyph_string_index_to_x (run->glyphs,
					     layout->text + run->item->offset,
					     run->item->length,
					     &run->item->analysis,
					     index - run->item->offset, trailing, x_pos);
	      if (x_pos)
	        *x_pos += width;
	    }
	  
	  return;
	}
      
      if (!properties.shape_set)
	width += pango_glyph_string_get_width (run->glyphs);
      else
	width += properties.shape_logical_rect->width;

      run_list = run_list->next;
    }

  if (x_pos)
    *x_pos = width;
}

static PangoLayoutLine *
pango_layout_index_to_line (PangoLayout      *layout,
			    int               index,
			    int              *line_nr,
			    PangoLayoutLine **line_before,
			    PangoLayoutLine **line_after)
{
  GSList *tmp_list;
  GSList *line_list;
  PangoLayoutLine *line = NULL;
  PangoLayoutLine *prev_line = NULL;
  int i = 0;
  
  line_list = tmp_list = layout->lines;
  while (tmp_list)
    {
      PangoLayoutLine *tmp_line = tmp_list->data;

      if (tmp_line->start_index > index)
        break; /* index was in paragraph delimiters */

      prev_line = line;
      line = tmp_line;
      line_list = tmp_list;
      i++;

      if (line->start_index + line->length > index)
        break;
      
      tmp_list = tmp_list->next;
    }

  if (line_nr)
    *line_nr = i;
  
  if (line_before)
    *line_before = prev_line;
  
  if (line_after)
    *line_after = (line_list && line_list->next) ? line_list->next->data : NULL;

  return line;
}

static PangoLayoutLine *
pango_layout_index_to_line_and_extents (PangoLayout     *layout,
					int              index,
					PangoRectangle  *line_rect)
{
  PangoLayoutIter *iter;
  PangoLayoutLine *line = NULL;
  
  iter = pango_layout_get_iter (layout);

  if (!ITER_IS_INVALID (iter))
    while (TRUE)
      {
	PangoLayoutLine *tmp_line = pango_layout_iter_get_line (iter);

	if (tmp_line->start_index > index)
	    break; /* index was in paragraph delimiters */

	line = tmp_line;
	
	pango_layout_iter_get_line_extents (iter, NULL, line_rect);
	
	if (line->start_index + line->length > index)
	  break;

	if (!pango_layout_iter_next_line (iter))
	  break; /* Use end of last line */
      }

  pango_layout_iter_free (iter);

  return line;
}

/**
 * pango_layout_index_to_line_x:
 * @layout:    a #PangoLayout
 * @index_:    the byte index of a grapheme within the layout.
 * @trailing:  an integer indicating the edge of the grapheme to retrieve the 
 *             position of. If 0, the trailing edge of the grapheme, if > 0, 
 *             the leading of the grapheme.
 * @line:      location to store resulting line index. (which will
 *             between 0 and pango_layout_get_line_count(layout) - 1)
 * @x_pos:     location to store resulting position within line
 *             (%PANGO_SCALE units per device unit)
 *
 * Converts from byte @index_ within the @layout to line and X position.
 * (X position is measured from the left edge of the line)
 */
void
pango_layout_index_to_line_x (PangoLayout *layout,
			      int          index,
			      gboolean     trailing,
			      int         *line,
			      int         *x_pos)
{
  int line_num;
  PangoLayoutLine *layout_line = NULL;
  
  g_return_if_fail (layout != NULL);
  g_return_if_fail (index >= 0);
  g_return_if_fail (index <= layout->length);

  pango_layout_check_lines (layout);

  layout_line = pango_layout_index_to_line (layout, index,
					    &line_num, NULL, NULL);

  if (layout_line)
    {
      /* use end of line if index was in the paragraph delimiters */
      if (index > layout_line->start_index + layout_line->length)
	index = layout_line->start_index + layout_line->length;
      
      if (line)
	*line = line_num;
      
      pango_layout_line_index_to_x (layout_line, index, trailing, x_pos);
    }
  else
    {
      if (line)
	*line = -1;
      if (x_pos)
	*x_pos = -1;
    }
}

/**
 * pango_layout_move_cursor_visually:
 * @layout:       a #PangoLayout.
 * @strong:       whether the moving cursor is the strong cursor or the
 *                weak cursor. The strong cursor is the cursor corresponding
 *                to text insertion in the base direction for the layout.
 * @old_index:    the byte index of the grapheme for the old index
 * @old_trailing: if 0, the cursor was at the trailing edge of the 
 *                grapheme indicated by @old_index, if > 0, the cursor
 *                was at the leading edge.
 * @direction:    direction to move cursor. A negative
 *                value indicates motion to the left.
 * @new_index:    location to store the new cursor byte index. A value of -1 
 *                indicates that the cursor has been moved off the beginning
 *                of the layout. A value of G_MAXINT indicates that
 *                the cursor has been moved off the end of the layout.
 * @new_trailing: number of characters to move forward from the location returned
 *                for @new_index to get the position where the cursor should
 *                be displayed. This allows distinguishing the position at
 *                the beginning of one line from the position at the end
 *                of the preceding line. @new_index is always on the line
 *                where the cursor should be displayed. 
 * 
 * Computes a new cursor position from an old position and
 * a count of positions to move visually. If @count is positive,
 * then the new strong cursor position will be one position
 * to the right of the old cursor position. If @count is negative,
 * then the new strong cursor position will be one position
 * to the left of the old cursor position. 
 *
 * In the presence of bidirection text, the correspondence
 * between logical and visual order will depend on the direction
 * of the current run, and there may be jumps when the cursor
 * is moved off of the end of a run.
 *
 * Motion here is in cursor positions, not in characters, so a
 * single call to pango_layout_move_cursor_visually() may move the
 * cursor over multiple characters when multiple characters combine
 * to form a single grapheme.
 **/
void
pango_layout_move_cursor_visually (PangoLayout *layout,
				   gboolean     strong,
				   int          old_index,
				   int          old_trailing,
				   int          direction,
				   int         *new_index,
				   int         *new_trailing)
{
  PangoLayoutLine *line = NULL;
  PangoLayoutLine *prev_line;
  PangoLayoutLine *next_line;

  int *log2vis_map;
  int *vis2log_map;
  int n_vis;
  int vis_pos, vis_pos_old, log_pos;
  int start_offset;
  gboolean off_start = FALSE;
  gboolean off_end = FALSE;

  g_return_if_fail (layout != NULL);
  g_return_if_fail (old_index >= 0 && old_index <= layout->length);
  g_return_if_fail (old_index < layout->length || old_trailing == 0);
  g_return_if_fail (new_index != NULL);
  g_return_if_fail (new_trailing != NULL);

  pango_layout_check_lines (layout);

  /* Find the line the old cursor is on */
  line = pango_layout_index_to_line (layout, old_index,
				     NULL, &prev_line, &next_line);

  start_offset = g_utf8_pointer_to_offset (layout->text, layout->text + line->start_index);

  while (old_trailing--)
    old_index = g_utf8_next_char (layout->text + old_index) - layout->text;

  log2vis_map = pango_layout_line_get_log2vis_map (line, strong);
  n_vis = g_utf8_strlen (layout->text + line->start_index, line->length);

  /* Clamp old_index to fit on the line */
  if (old_index > (line->start_index + line->length))
    old_index = line->start_index + line->length;
  
  vis_pos = log2vis_map[old_index - line->start_index];
  g_free (log2vis_map);

  /* Handling movement between lines */
  if (vis_pos == 0 && direction < 0)
    {
      if (line->resolved_dir == PANGO_DIRECTION_LTR)
	off_start = TRUE;
      else
	off_end = TRUE;
    }
  else if (vis_pos == n_vis && direction > 0)
    {
      if (line->resolved_dir == PANGO_DIRECTION_LTR)
	off_end = TRUE;
      else
	off_start = TRUE;
    }

  if (off_start || off_end)
    {
      /* If we move over a paragraph boundary, count that as
       * an extra position in the motion
       */
      gboolean paragraph_boundary;

      if (off_start)
	{
	  if (!prev_line)
	    {
	      *new_index = -1;
	      *new_trailing = 0;
	      return;
	    }
	  line = prev_line;
	  paragraph_boundary = (line->start_index + line->length != old_index);
	}
      else
	{
	  if (!next_line)
	    {
	      *new_index = G_MAXINT;
	      *new_trailing = 0;
	      return;
	    }
	  line = next_line;
	  paragraph_boundary = (line->start_index != old_index);
	}

      if (vis_pos == 0 && direction < 0)
	{
	  vis_pos = g_utf8_strlen (layout->text + line->start_index, line->length);
	  if (paragraph_boundary)
	    vis_pos++;
	}
      else /* (vis_pos == n_vis && direction > 0) */
	{
	  vis_pos = 0;
	  if (paragraph_boundary)
	    vis_pos--;
	}
    }

  vis2log_map = pango_layout_line_get_vis2log_map (line, strong);

  vis_pos_old = vis_pos + (direction > 0 ? 1 : -1);
  log_pos = g_utf8_pointer_to_offset (layout->text + line->start_index,
				      layout->text + line->start_index + vis2log_map[vis_pos_old]);
  do
    {
      vis_pos += direction > 0 ? 1 : -1;
      log_pos += g_utf8_pointer_to_offset (layout->text + line->start_index + vis2log_map[vis_pos_old],
					   layout->text + line->start_index + vis2log_map[vis_pos]);
      vis_pos_old = vis_pos;
    }
  while (vis_pos > 0 && vis_pos < n_vis &&
	 !layout->log_attrs[start_offset + log_pos].is_cursor_position);
  
  *new_index = line->start_index + vis2log_map[vis_pos];
  g_free (vis2log_map);

  *new_trailing = 0;
    
  if (*new_index == line->start_index + line->length && line->length > 0)
    {
      do
	{
	  log_pos--;
	  *new_index = g_utf8_prev_char (layout->text + *new_index) - layout->text;
	  (*new_trailing)++;
	}
      while (log_pos > 0 && !layout->log_attrs[start_offset + log_pos].is_cursor_position);
    }
}

/**
 * pango_layout_xy_to_index:
 * @layout:    a #PangoLayout
 * @x:         the X offset (in #PangoGlyphUnit)
 *             from the left edge of the layout.
 * @y:         the Y offset (in #PangoGlyphUnit)
 *             from the top edge of the layout
 * @index_:    location to store calculated byte index
 * @trailing:  location to store a integer indicating where
 *             in the grapheme the user clicked. It will either
 *             be zero, or the number of characters in the
 *             grapheme. 0 represents the trailing edge of the grapheme.
 *
 * Converts from X and Y position within a layout to the byte 
 * index to the character at that logical position. If the
 * Y position is not inside the layout, the closest position is chosen
 * (the position will be clamped inside the layout). If the
 * X position is not within the layout, then the start or the
 * end of the line is chosen as  described for pango_layout_x_to_index().
 * If either the X or Y positions were not inside the layout, then the
 * function returns %FALSE; on an exact hit, it returns %TRUE.
 *
 * Return value: %TRUE if the coordinates were inside text, %FALSE otherwise.
 **/
gboolean
pango_layout_xy_to_index (PangoLayout *layout,
			  int          x,
			  int          y,
			  int         *index,
			  gint        *trailing)
{
  PangoLayoutIter *iter;
  PangoLayoutLine *prev_line = NULL;
  PangoLayoutLine *found = NULL;
  int found_line_x = 0;
  int prev_last = 0;
  int prev_line_x = 0;
  gboolean retval = FALSE;
  gboolean outside = FALSE;
  
  g_return_val_if_fail (PANGO_IS_LAYOUT (layout), FALSE);
  
  iter = pango_layout_get_iter (layout);
  
  do
    {
      PangoRectangle line_logical;
      int first_y, last_y;
      
      pango_layout_iter_get_line_extents (iter, NULL, &line_logical);
      pango_layout_iter_get_line_yrange (iter, &first_y, &last_y);

      if (y < first_y)
        {
          if (prev_line && y < (prev_last + (first_y - prev_last) / 2))
            {
              found = prev_line;
              found_line_x = prev_line_x;
            }
          else
            {
              if (prev_line == NULL)
                outside = TRUE; /* off the top */
              
              found = pango_layout_iter_get_line (iter);
              found_line_x = x - line_logical.x;
            }
        }
      else if (y >= first_y &&
               y < last_y)
        {
          found = pango_layout_iter_get_line (iter);
          found_line_x = x - line_logical.x;
        }

      prev_line = pango_layout_iter_get_line (iter);
      prev_last = last_y;
      prev_line_x = x - line_logical.x;

      if (found != NULL)
        break;
    }
  while (pango_layout_iter_next_line (iter));
  
  pango_layout_iter_free (iter);

  if (found == NULL)
    {
      /* Off the bottom of the layout */
      outside = TRUE;
      
      found = prev_line;
      found_line_x = prev_line_x;
    }
  
  retval = pango_layout_line_x_to_index (found,
                                         found_line_x,
                                         index, trailing);

  if (outside)
    retval = FALSE;

  return retval;
}

/**
 * pango_layout_index_to_pos:
 * @layout: a #PangoLayout
 * @index_: byte index within @layout
 * @pos: rectangle in which to store the position of the grapheme
 * 
 * Converts from an index within a #PangoLayout to the onscreen position
 * corresponding to the grapheme at that index, which is represented
 * as rectangle.  Note that <literal>pos->x</literal> is always the leading 
 * edge of the grapheme and <literal>pos->x + pos->width</literal> the trailing 
 * edge of the grapheme. If the directionality of the grapheme is right-to-left,
 * then <literal>pos->width</literal> will be negative.
 **/
void
pango_layout_index_to_pos (PangoLayout    *layout,
			   int             index,
			   PangoRectangle *pos)
{
  PangoRectangle logical_rect;
  PangoLayoutIter *iter;
  PangoLayoutLine *layout_line = NULL;
  int x_pos;
  
  g_return_if_fail (layout != NULL);
  g_return_if_fail (index >= 0);
  g_return_if_fail (pos != NULL);
  
  iter = pango_layout_get_iter (layout);

  if (!ITER_IS_INVALID (iter))
    {
      while (TRUE)
	{
	  PangoLayoutLine *tmp_line = pango_layout_iter_get_line (iter);

	  if (tmp_line->start_index > index)
	    {
	      /* index is in the paragraph delimiters, move to
	       * end of previous line
	       */
	      index = layout_line->start_index + layout_line->length;
	      break;
	    }

	  layout_line = tmp_line;
	  
	  pango_layout_iter_get_line_extents (iter, NULL, &logical_rect);
	  
	  if (layout_line->start_index + layout_line->length > index)
	    break;

	  if (!pango_layout_iter_next_line (iter))
	    {
	      index = layout_line->start_index + layout_line->length;
	      break;
	    }
	}

      pos->y = logical_rect.y;
      pos->height = logical_rect.height;

      pango_layout_line_index_to_x (layout_line, index, 0, &x_pos);
      pos->x = logical_rect.x + x_pos;

      if (index < layout_line->start_index + layout_line->length)
	{
	  pango_layout_line_index_to_x (layout_line, index, 1, &x_pos);
	  pos->width = (logical_rect.x + x_pos) - pos->x;
	}
      else
	pos->width = 0;
    }
  
  pango_layout_iter_free (iter);
}

static void
pango_layout_line_get_range (PangoLayoutLine *line,
			     char           **start,
			     char           **end)
{
  char *p;
  
  p = line->layout->text + line->start_index;

  if (start)
    *start = p;
  if (end)
    *end = p + line->length;
}

static int *
pango_layout_line_get_vis2log_map (PangoLayoutLine *line,
				   gboolean         strong)
{
  PangoLayout *layout = line->layout;
  PangoDirection prev_dir;
  PangoDirection cursor_dir;
  GSList *tmp_list;
  gchar *start, *end;
  int *result;
  int pos;
  int n_chars;

  pango_layout_line_get_range (line, &start, &end);
  n_chars = g_utf8_strlen (start, end - start);
  
  result = g_new (int, n_chars + 1);

  if (strong)
    cursor_dir = line->resolved_dir;
  else
    cursor_dir = (line->resolved_dir == PANGO_DIRECTION_LTR) ? PANGO_DIRECTION_RTL : PANGO_DIRECTION_LTR;

  /* Handle the first visual position
   */
  if (line->resolved_dir == cursor_dir)
    result[0] = line->resolved_dir == PANGO_DIRECTION_LTR ? 0 : end - start;

  prev_dir = line->resolved_dir;
  pos = 0;
  tmp_list = line->runs;
  while (tmp_list)
    {
      PangoLayoutRun *run = tmp_list->data;
      int run_n_chars = run->item->num_chars;
      PangoDirection run_dir = (run->item->analysis.level % 2) ? PANGO_DIRECTION_RTL : PANGO_DIRECTION_LTR;
      char *p = layout->text + run->item->offset;
      int i;

      /* pos is the visual position at the start of the run */
      /* p is the logical byte index at the start of the run */

      if (run_dir == PANGO_DIRECTION_LTR)
	{
	  if ((cursor_dir == PANGO_DIRECTION_LTR) ||
	      (prev_dir == run_dir))
	    result[pos] = p - start;
	  
	  p = g_utf8_next_char (p);
	  
	  for (i = 1; i < run_n_chars; i++)
	    {
	      result[pos + i] = p - start;
	      p = g_utf8_next_char (p);
	    }
	  
	  if (cursor_dir == PANGO_DIRECTION_LTR)
	    result[pos + run_n_chars] = p - start;
	}
      else
	{
	  if (cursor_dir == PANGO_DIRECTION_RTL)
	    result[pos + run_n_chars] = p - start;

	  p = g_utf8_next_char (p);

	  for (i = 1; i < run_n_chars; i++)
	    {
	      result[pos + run_n_chars - i] = p - start;
	      p = g_utf8_next_char (p);
	    }

	  if ((cursor_dir == PANGO_DIRECTION_RTL) ||
	      (prev_dir == run_dir))
	    result[pos] = p - start;
	}

      pos += run_n_chars;
      prev_dir = run_dir;
      tmp_list = tmp_list->next;
    }

  /* And the last visual position
   */
  if ((cursor_dir == line->resolved_dir) || (prev_dir == line->resolved_dir))
    result[pos] = line->resolved_dir == PANGO_DIRECTION_LTR ? end - start : 0;
      
  return result;
}

static int *
pango_layout_line_get_log2vis_map (PangoLayoutLine *line,
				   gboolean         strong)
{
  gchar *start, *end;
  int *reverse_map;
  int *result;
  int i;
  int n_chars;

  pango_layout_line_get_range (line, &start, &end);
  n_chars = g_utf8_strlen (start, end - start);
  result = g_new0 (int, end - start + 1);

  reverse_map = pango_layout_line_get_vis2log_map (line, strong);

  for (i=0; i <= n_chars; i++)
    result[reverse_map[i]] = i;

  g_free (reverse_map);

  return result;
}

static PangoDirection
pango_layout_line_get_char_direction (PangoLayoutLine *layout_line,
				      int              index)
{
  GSList *run_list;

  run_list = layout_line->runs;
  while (run_list)
    {
      PangoLayoutRun *run = run_list->data;
      
      if (run->item->offset <= index && run->item->offset + run->item->length > index)
	return run->item->analysis.level % 2 ? PANGO_DIRECTION_RTL : PANGO_DIRECTION_LTR;

      run_list = run_list->next;
    }

  g_assert_not_reached ();

  return PANGO_DIRECTION_LTR;
}

/**
 * pango_layout_get_cursor_pos:
 * @layout: a #PangoLayout
 * @index_: the byte index of the cursor
 * @strong_pos: location to store the strong cursor position (may be %NULL)
 * @weak_pos: location to store the weak cursor position (may be %NULL)
 * 
 * Given an index within a layout, determines the positions that of the
 * strong and weak cursors if the insertion point is at that
 * index. The position of each cursor is stored as a zero-width
 * rectangle. The strong cursor location is the location where
 * characters of the directionality equal to the base direction of the
 * layout are inserted.  The weak cursor location is the location
 * where characters of the directionality opposite to the base
 * direction of the layout are inserted.
 **/
void
pango_layout_get_cursor_pos (PangoLayout    *layout,
			     int             index,
			     PangoRectangle *strong_pos,
			     PangoRectangle *weak_pos)
{
  PangoDirection dir1;
  PangoRectangle line_rect;
  PangoLayoutLine *layout_line = NULL; /* Quiet GCC */
  int x1_trailing;
  int x2;
  
  g_return_if_fail (layout != NULL);
  g_return_if_fail (index >= 0 && index <= layout->length);
  
  layout_line = pango_layout_index_to_line_and_extents (layout, index,
							&line_rect);

  g_assert (index >= layout_line->start_index);
  
  /* Examine the trailing edge of the character before the cursor */
  if (index == layout_line->start_index)
    {
      dir1 = layout_line->resolved_dir;
      if (layout_line->resolved_dir == PANGO_DIRECTION_LTR)
        x1_trailing = 0;
      else
        x1_trailing = line_rect.width;
    }
  else
    {
      gint prev_index = g_utf8_prev_char (layout->text + index) - layout->text;
      dir1 = pango_layout_line_get_char_direction (layout_line, prev_index);
      pango_layout_line_index_to_x (layout_line, prev_index, TRUE, &x1_trailing);
    }
  
  /* Examine the leading edge of the character after the cursor */
  if (index >= layout_line->start_index + layout_line->length)
    {
      if (layout_line->resolved_dir == PANGO_DIRECTION_LTR)
        x2 = line_rect.width;
      else
        x2 = 0;
    }
  else
    {
      pango_layout_line_index_to_x (layout_line, index, FALSE, &x2);
    }
	  
  if (strong_pos)
    {
      strong_pos->x = line_rect.x;
      
      if (dir1 == layout_line->resolved_dir)
	strong_pos->x += x1_trailing;
      else
	strong_pos->x += x2;

      strong_pos->y = line_rect.y;
      strong_pos->width = 0;
      strong_pos->height = line_rect.height;
    }

  if (weak_pos)
    {
      weak_pos->x = line_rect.x;
      
      if (dir1 == layout_line->resolved_dir)
	weak_pos->x += x2;
      else
	weak_pos->x += x1_trailing;

      weak_pos->y = line_rect.y;
      weak_pos->width = 0;
      weak_pos->height = line_rect.height;
    }
}

static inline int
direction_simple (PangoDirection d)
{
  switch (d)
    {
    case PANGO_DIRECTION_LTR :
    case PANGO_DIRECTION_WEAK_LTR :
    case PANGO_DIRECTION_TTB_RTL :
      return 1;
    case PANGO_DIRECTION_RTL :
    case PANGO_DIRECTION_WEAK_RTL :
    case PANGO_DIRECTION_TTB_LTR :
      return -1;
    case PANGO_DIRECTION_NEUTRAL :
      return 0;
    /* no default, compiler should complain if a new values is added */
    }
  /* not reached */
  return 0;
}

static PangoAlignment
get_alignment (PangoLayout     *layout,
	       PangoLayoutLine *line)
{
  PangoAlignment alignment = layout->alignment;

  if (line->layout->auto_dir &&
      direction_simple (line->resolved_dir) ==
      -direction_simple (pango_context_get_base_dir (layout->context)))
    {
      if (alignment == PANGO_ALIGN_LEFT)
	alignment = PANGO_ALIGN_RIGHT;
      else if (alignment == PANGO_ALIGN_RIGHT)
	alignment = PANGO_ALIGN_LEFT;
    }

  return alignment;
}

static void
get_x_offset (PangoLayout     *layout,
              PangoLayoutLine *line,
              int              layout_width,
              int              line_width,
              int             *x_offset)
{
  PangoAlignment alignment = get_alignment (layout, line);

  /* Alignment */
  if (alignment == PANGO_ALIGN_RIGHT)
    *x_offset = layout_width - line_width;
  else if (alignment == PANGO_ALIGN_CENTER)
    *x_offset = (layout_width - line_width) / 2;
  else
    *x_offset = 0;

  /* Indentation */


  /* For center, we ignore indentation; I think I've seen word
   * processors that still do the indentation here as if it were
   * indented left/right, though we can't sensibly do that without
   * knowing whether left/right is the "normal" thing for this text
   */
   
  if (alignment == PANGO_ALIGN_CENTER)
    return;
  
  if (line->is_paragraph_start)
    {
      if (layout->indent > 0)
        {
          if (alignment == PANGO_ALIGN_LEFT)
            *x_offset += layout->indent;
          else
            *x_offset -= layout->indent;
        }
    }
  else
    {
      if (layout->indent < 0)
        {
          if (alignment == PANGO_ALIGN_LEFT)
            *x_offset -= layout->indent;
          else
            *x_offset += layout->indent;
        }
    }
}

static void
get_line_extents_layout_coords (PangoLayout     *layout,
                                PangoLayoutLine *line,
                                int              layout_width,
                                int              y_offset,
                                int             *baseline,
                                PangoRectangle  *line_ink_layout,
                                PangoRectangle  *line_logical_layout)
{
  int x_offset;
  /* Line extents in line coords (origin at line baseline) */
  PangoRectangle line_ink;
  PangoRectangle line_logical;
  
  pango_layout_line_get_extents (line, line_ink_layout ? &line_ink : NULL,
                                 &line_logical);
  
  get_x_offset (layout, line, layout_width, line_logical.width, &x_offset);
  
  /* Convert the line's extents into layout coordinates */
  if (line_ink_layout)
    {
      *line_ink_layout = line_ink;
      line_ink_layout->x = line_ink.x + x_offset;
      line_ink_layout->y = y_offset - line_logical.y + line_ink.y;
    }

  if (line_logical_layout)
    {
      *line_logical_layout = line_logical;
      line_logical_layout->x = line_logical.x + x_offset;
      line_logical_layout->y = y_offset;
    }

  if (baseline)
    *baseline = y_offset - line_logical.y;
}

/* if non-NULL line_extents returns a list of line extents
 * in layout coordinates
 */
static void
pango_layout_get_extents_internal (PangoLayout    *layout,
                                   PangoRectangle *ink_rect,
                                   PangoRectangle *logical_rect,
                                   GSList        **line_extents,
                                   int            *real_width)
{
  GSList *line_list;
  int y_offset = 0;
  int width;
  gboolean need_width = FALSE;

  g_return_if_fail (layout != NULL);

  pango_layout_check_lines (layout);

  /* When we are not wrapping, we need the overall width of the layout to 
   * figure out the x_offsets of each line. However, we only need the 
   * x_offsets if we are computing the ink_rect or individual line extents.
   */
  width = layout->width;

  if (real_width)
    need_width = TRUE;
  else if (layout->auto_dir)
    {
      /* If one of the lines of the layout is not left aligned, then we need
       * the width of the width to calculate line x-offsets; this requires
       * looping through the lines for layout->auto_dir.
       */
      line_list = layout->lines;
      while (line_list)
	{
	  PangoLayoutLine *line = line_list->data;

	  if (get_alignment (layout, line) != PANGO_ALIGN_LEFT)
	    need_width = TRUE;

	  line_list = line_list->next;
	}
    }
  else if (layout->alignment != PANGO_ALIGN_LEFT)
    need_width = TRUE;
  
  if (width == -1 && need_width && (ink_rect || line_extents))
    {
      PangoRectangle overall_logical;
      
      pango_layout_get_extents (layout, NULL, &overall_logical);
      width = overall_logical.width;
    }

  if (logical_rect)
    {
      logical_rect->x = 0;
      logical_rect->y = 0;
      logical_rect->width = 0;
      logical_rect->height = 0;
    }
  
  line_list = layout->lines;
  while (line_list)
    {
      PangoLayoutLine *line = line_list->data;
      /* Line extents in layout coords (origin at 0,0 of the layout) */
      PangoRectangle line_ink_layout;
      PangoRectangle line_logical_layout;
      
      int new_pos;

      /* This block gets the line extents in layout coords */
      {
        int baseline;
        
        get_line_extents_layout_coords (layout, line,
                                        width, y_offset,
                                        &baseline,
                                        ink_rect ? &line_ink_layout : NULL,
                                        &line_logical_layout);
        
        if (line_extents)
          {
            Extents *ext = g_slice_new (Extents);
            ext->baseline = baseline;
            ext->ink_rect = line_ink_layout;
            ext->logical_rect = line_logical_layout;
            *line_extents = g_slist_prepend (*line_extents, ext);
          }
      }
      
      if (ink_rect)
	{
          /* Compute the union of the current ink_rect with
           * line_ink_layout
           */
          
	  if (line_list == layout->lines)
	    {
              *ink_rect = line_ink_layout;
	    }
	  else
	    {
	      new_pos = MIN (ink_rect->x, line_ink_layout.x);
	      ink_rect->width =
                MAX (ink_rect->x + ink_rect->width,
                     line_ink_layout.x + line_ink_layout.width) - new_pos;
	      ink_rect->x = new_pos;

	      new_pos = MIN (ink_rect->y, line_ink_layout.y);
	      ink_rect->height =
                MAX (ink_rect->y + ink_rect->height,
                     line_ink_layout.y + line_ink_layout.height) - new_pos;
	      ink_rect->y = new_pos;
            }
	}

      if (logical_rect)
	{
	  if (layout->width == -1)
	    {
	      /* When no width is set on layout, we can just compute the max of the
	       * line lengths to get the horizontal extents ... logical_rect.x = 0.
	       */
	      logical_rect->width = MAX (logical_rect->width, line_logical_layout.width);
	    }
	  else
	    {
	      /* When a width is set, we have to compute the union of the horizontal
	       * extents of all the lines.
	       */
	      if (line_list == layout->lines)
		{
		  logical_rect->x = line_logical_layout.x;
		  logical_rect->width = line_logical_layout.width;
		}
	      else
		{
		  new_pos = MIN (logical_rect->x, line_logical_layout.x);
		  logical_rect->width =
		    MAX (logical_rect->x + logical_rect->width,
			 line_logical_layout.x + line_logical_layout.width) - new_pos;
		  logical_rect->x = new_pos;
		  
		}
	    }
	  
	  logical_rect->height += line_logical_layout.height;

          /* No space after the last line, of course. */
          if (line_list->next != NULL)
            logical_rect->height += layout->spacing;
	}
      
      y_offset += line_logical_layout.height + layout->spacing;
      line_list = line_list->next;
    }

  if (line_extents)
    *line_extents = g_slist_reverse (*line_extents);

  if (real_width)
    *real_width = width;
}

/**
 * pango_layout_get_extents:
 * @layout:   a #PangoLayout
 * @ink_rect: rectangle used to store the extents of the layout as drawn
 *            or %NULL to indicate that the result is not needed.
 * @logical_rect: rectangle used to store the logical extents of the layout 
                 or %NULL to indicate that the result is not needed.
 * 
 * Computes the logical and ink extents of @layout. Logical extents
 * are usually what you want for positioning things.  Note that both extents
 * may have non-zero x and y.  You may want to use those to offset where you
 * render the layout.  Not doing that is a very typical bug that shows up as
 * right-to-left layouts not being correctly positioned in a layout with
 * a set width.
 *
 * The extents are given in layout coordinates and in Pango units; layout
 * coordinates begin at the top left corner of the layout. 
 */
void
pango_layout_get_extents (PangoLayout    *layout,
			  PangoRectangle *ink_rect,
			  PangoRectangle *logical_rect)
{	  
  g_return_if_fail (layout != NULL);

  pango_layout_get_extents_internal (layout, ink_rect, logical_rect, NULL, NULL);
}

/**
 * pango_layout_get_pixel_extents:
 * @layout:   a #PangoLayout
 * @ink_rect: rectangle used to store the extents of the layout as drawn
 *            or %NULL to indicate that the result is not needed.
 * @logical_rect: rectangle used to store the logical extents of the 
 *              layout or %NULL to indicate that the result is not needed.
 * 
 * Computes the logical and ink extents of @layout in device units.
 * See pango_layout_get_extents(); this function just calls
 * pango_layout_get_extents() and then converts the extents to
 * device units using the %PANGO_SCALE factor.
 **/
void
pango_layout_get_pixel_extents (PangoLayout *layout,
				PangoRectangle *ink_rect,
				PangoRectangle *logical_rect)
{
  g_return_if_fail (PANGO_IS_LAYOUT (layout));

  pango_layout_get_extents (layout, ink_rect, logical_rect);

  if (ink_rect)
    {
      int orig_x = ink_rect->x;
      int orig_y = ink_rect->y;

      ink_rect->x = PANGO_PIXELS_FLOOR (ink_rect->x);
      ink_rect->y = PANGO_PIXELS_FLOOR (ink_rect->y);

      ink_rect->width  = PANGO_PIXELS_CEIL (orig_x + ink_rect->width ) - ink_rect->x;
      ink_rect->height = PANGO_PIXELS_CEIL (orig_y + ink_rect->height) - ink_rect->y;
    }

  if (logical_rect)
    {
      int orig_x = logical_rect->x;
      int orig_y = logical_rect->y;

      logical_rect->x = PANGO_PIXELS (logical_rect->x);
      logical_rect->y = PANGO_PIXELS (logical_rect->y);

      logical_rect->width  = PANGO_PIXELS (orig_x + logical_rect->width ) - logical_rect->x;
      logical_rect->height = PANGO_PIXELS (orig_y + logical_rect->height) - logical_rect->y;
    }
}

/**
 * pango_layout_get_size:
 * @layout: a #PangoLayout
 * @width: location to store the logical width, or %NULL
 * @height: location to store the logical height, or %NULL
 * 
 * Determines the logical width and height of a #PangoLayout
 * in Pango units. (device units scaled by %PANGO_SCALE). This
 * is simply a convenience function around pango_layout_get_extents().
 **/
void
pango_layout_get_size (PangoLayout *layout,
		       int         *width,
		       int         *height)
{
  PangoRectangle logical_rect;

  pango_layout_get_extents (layout, NULL, &logical_rect);

  if (width)
    *width = logical_rect.width;
  if (height)
    *height = logical_rect.height;
}

/**
 * pango_layout_get_pixel_size:
 * @layout: a #PangoLayout
 * @width: location to store the logical width, or %NULL
 * @height: location to store the logical height, or %NULL
 * 
 * Determines the logical width and height of a #PangoLayout
 * in device units. (pango_layout_get_size() returns the width
 * and height scaled by %PANGO_SCALE.) This
 * is simply a convenience function around
 * pango_layout_get_pixel_extents().
 **/
void
pango_layout_get_pixel_size (PangoLayout *layout,
			     int         *width,
			     int         *height)
{
  PangoRectangle logical_rect;

  pango_layout_get_pixel_extents (layout, NULL, &logical_rect);

  if (width)
    *width = logical_rect.width;
  if (height)
    *height = logical_rect.height;
}

static void
pango_layout_clear_lines (PangoLayout *layout)
{
  if (layout->lines)
    {
      GSList *tmp_list = layout->lines;
      while (tmp_list)
	{
	  PangoLayoutLine *line = tmp_list->data;
	  tmp_list = tmp_list->next;
	  
	  line->layout = NULL;
	  pango_layout_line_unref (line);
	}
      
      g_slist_free (layout->lines);
      layout->lines = NULL;

      /* This could be handled separately, since we don't need to
       * recompute log_attrs on a width change, but this is easiest
       */
      g_free (layout->log_attrs);
      layout->log_attrs = NULL;
    }
}


/************************************************
 * Some functions for handling PANGO_ATTR_SHAPE *
 ************************************************/

static void
imposed_shape (const char       *text,
	       gint              n_chars,
	       PangoRectangle   *shape_ink,
	       PangoRectangle   *shape_logical,
	       PangoGlyphString *glyphs)
{
  int i;
  const char *p;
  
  pango_glyph_string_set_size (glyphs, n_chars);

  for (i=0, p = text; i < n_chars; i++, p = g_utf8_next_char (p))
    {
      glyphs->glyphs[i].glyph = PANGO_GLYPH_EMPTY;
      glyphs->glyphs[i].geometry.x_offset = 0;
      glyphs->glyphs[i].geometry.y_offset = 0;
      glyphs->glyphs[i].geometry.width = shape_logical->width;
      glyphs->glyphs[i].attr.is_cluster_start = 1;
      
      glyphs->log_clusters[i] = p - text;
    }
}

static void
imposed_extents (gint              n_chars,
		 PangoRectangle   *shape_ink,
		 PangoRectangle   *shape_logical,
		 PangoRectangle   *ink_rect,
		 PangoRectangle   *logical_rect)
{
  if (n_chars > 0)
    {
      if (ink_rect)
	{
	  ink_rect->x = MIN (shape_ink->x, shape_ink->x + shape_logical->width * (n_chars - 1));
	  ink_rect->width = MAX (shape_ink->width, shape_ink->width + shape_logical->width * (n_chars - 1));
	  ink_rect->y = shape_ink->y;
	  ink_rect->height = shape_ink->height;
	}
      if (logical_rect)
	{
	  logical_rect->x = MIN (shape_logical->x, shape_logical->x + shape_logical->width * (n_chars - 1));
	  logical_rect->width = MAX (shape_logical->width, shape_logical->width + shape_logical->width * (n_chars - 1));
	  logical_rect->y = shape_logical->y;
	  logical_rect->height = shape_logical->height;
	}
    }
  else
    {
      if (ink_rect)
	{
	  ink_rect->x = 0;
	  ink_rect->y = 0;
	  ink_rect->width = 0;
	  ink_rect->height = 0;
	}
      
      if (logical_rect)
	{
	  logical_rect->x = 0;
	  logical_rect->y = 0;
	  logical_rect->width = 0;
	  logical_rect->height = 0;
	}
    }
}


/*****************
 * Line Breaking *
 *****************/

static void shape_tab (PangoLayoutLine  *line,
		       PangoGlyphString *glyphs);

static void
free_run (PangoLayoutRun *run, gpointer data)
{
  gboolean free_item = data != NULL;
  if (free_item)
    pango_item_free (run->item);

  pango_glyph_string_free (run->glyphs);
  g_slice_free (PangoLayoutRun, run);
}

static void
extents_free (Extents *ext, gpointer data)
{
  g_slice_free (Extents, ext);
}

static PangoItem *
uninsert_run (PangoLayoutLine *line)
{
  PangoLayoutRun *run;
  PangoItem *item;
  
  GSList *tmp_node = line->runs;

  run = tmp_node->data;
  item = run->item;
  
  line->runs = tmp_node->next;
  line->length -= item->length;
  
  g_slist_free_1 (tmp_node);
  free_run (run, (gpointer)FALSE);

  return item;
}

static void
ensure_tab_width (PangoLayout *layout)
{
  if (layout->tab_width == -1)
    {
      /* Find out how wide 8 spaces are in the context's default
       * font. Utter performance killer. :-(
       */
      PangoGlyphString *glyphs = pango_glyph_string_new ();
      PangoItem *item;
      GList *items;
      PangoAttribute *attr;
      PangoAttrList *layout_attrs;
      PangoAttrList *tmp_attrs;
      PangoAttrIterator *iter;
      PangoFontDescription *font_desc = pango_font_description_copy_static (pango_context_get_font_description (layout->context));
      PangoLanguage *language;
      int i;

      layout_attrs = pango_layout_get_effective_attributes (layout);
      iter = pango_attr_list_get_iterator (layout_attrs);
      pango_attr_iterator_get_font (iter, font_desc, &language, NULL);
      
      tmp_attrs = pango_attr_list_new ();

      attr = pango_attr_font_desc_new (font_desc);
      pango_font_description_free (font_desc);
      
      attr->start_index = 0;
      attr->end_index = 1;
      pango_attr_list_insert_before (tmp_attrs, attr);
      
      if (language)
	{
	  attr = pango_attr_language_new (language);
	  attr->start_index = 0;
	  attr->end_index = 1;
	  pango_attr_list_insert_before (tmp_attrs, attr);
	}
	  
      items = pango_itemize (layout->context, " ", 0, 1, tmp_attrs, NULL);

      pango_attr_iterator_destroy (iter);
      if (layout_attrs != layout->attrs)
	pango_attr_list_unref (layout_attrs);
      pango_attr_list_unref (tmp_attrs);
      
      item = items->data;
      pango_shape ("        ", 8, &item->analysis, glyphs);
      
      pango_item_free (item);
      g_list_free (items);
      
      layout->tab_width = 0;
      for (i=0; i < glyphs->num_glyphs; i++)
	layout->tab_width += glyphs->glyphs[i].geometry.width;
      
      pango_glyph_string_free (glyphs);

      /* We need to make sure the tab_width is > 0 so finding tab positions
       * terminates. This check should be necessary only under extreme
       * problems with the font.
       */
      if (layout->tab_width <= 0)
	layout->tab_width = 50 * PANGO_SCALE; /* pretty much arbitrary */
    }
}

/* For now we only need the tab position, we assume
 * all tabs are left-aligned.
 */
static int
get_tab_pos (PangoLayout *layout, int index)
{
  gint n_tabs;
  gboolean in_pixels;

  if (layout->tabs)
    {
      n_tabs = pango_tab_array_get_size (layout->tabs);
      in_pixels = pango_tab_array_get_positions_in_pixels (layout->tabs);
    }
  else
    {
      n_tabs = 0;
      in_pixels = FALSE;
    }
  
  if (index < n_tabs)
    {
      gint pos = 0;

      pango_tab_array_get_tab (layout->tabs, index, NULL, &pos);

      if (in_pixels)
        return pos * PANGO_SCALE;
      else
        return pos;
    }

  if (n_tabs > 0)
    {
      /* Extrapolate tab position, repeating the last tab gap to
       * infinity.
       */
      int last_pos = 0;
      int next_to_last_pos = 0;
      int tab_width;
      
      pango_tab_array_get_tab (layout->tabs, n_tabs - 1, NULL, &last_pos);

      if (n_tabs > 1)
        pango_tab_array_get_tab (layout->tabs, n_tabs - 2, NULL, &next_to_last_pos);
      else
        next_to_last_pos = 0;

      if (in_pixels)
	{
	  next_to_last_pos *= PANGO_SCALE;
	  last_pos *= PANGO_SCALE;
	}
      
      if (last_pos > next_to_last_pos)
	{
	  tab_width = last_pos - next_to_last_pos;
	}
      else
	{
	  tab_width = layout->tab_width;
	}

      return last_pos + tab_width * (index - n_tabs + 1);
    }
  else
    {
      /* No tab array set, so use default tab width
       */
      return layout->tab_width * index;
    }
}

static int
line_width (PangoLayoutLine *line)
{
  GSList *l;
  int i;
  int width = 0;
  
  /* Compute the width of the line currently - inefficient, but easier
   * than keeping the current width of the line up to date everywhere
   */
  for (l = line->runs; l; l = l->next)
    {
      PangoLayoutRun *run = l->data;
      
      for (i=0; i < run->glyphs->num_glyphs; i++)
	width += run->glyphs->glyphs[i].geometry.width;
    }

  return width;
}

static void
shape_tab (PangoLayoutLine  *line,
	   PangoGlyphString *glyphs)
{
  int i, space_width;

  int current_width = line_width (line);

  pango_glyph_string_set_size (glyphs, 1);
  
  glyphs->glyphs[0].glyph = PANGO_GLYPH_EMPTY;
  glyphs->glyphs[0].geometry.x_offset = 0;
  glyphs->glyphs[0].geometry.y_offset = 0;
  glyphs->glyphs[0].attr.is_cluster_start = 1;
  
  glyphs->log_clusters[0] = 0;

  ensure_tab_width (line->layout);
  space_width = line->layout->tab_width / 8;

  for (i=0;;i++)
    {
      int tab_pos = get_tab_pos (line->layout, i);
      if (tab_pos >= current_width + space_width)
	{
	  glyphs->glyphs[0].geometry.width = tab_pos - current_width;
	  break;
	}
    }
}

static inline gboolean
can_break_at (PangoLayout *layout,
	      gint         offset,
	      gboolean     always_wrap_char)
{
  PangoWrapMode wrap;
  /* We probably should have a mode where we treat all white-space as
   * of fungible width - appropriate for typography but not for
   * editing.
   */
  wrap = layout->wrap;
  
  if (wrap == PANGO_WRAP_WORD_CHAR)
    wrap = always_wrap_char ? PANGO_WRAP_CHAR : PANGO_WRAP_WORD;
  
  if (offset == layout->n_chars)
    return TRUE;
  else if (wrap == PANGO_WRAP_WORD)
    return layout->log_attrs[offset].is_line_break;
  else if (wrap == PANGO_WRAP_CHAR)
    return layout->log_attrs[offset].is_char_break;
  else
    {
      g_warning (G_STRLOC": broken PangoLayout");
      return TRUE;
    }
}

static inline gboolean
can_break_in (PangoLayout *layout,
	      int          start_offset,
	      int          num_chars,
	      gboolean     allow_break_at_start)
{
  int i;

  for (i = allow_break_at_start ? 0 : 1; i < num_chars; i++)
    if (can_break_at (layout, start_offset + i, FALSE))
      return TRUE;

  return FALSE;
}

typedef enum 
{
  BREAK_NONE_FIT,
  BREAK_SOME_FIT,
  BREAK_ALL_FIT,
  BREAK_EMPTY_FIT,
  BREAK_LINE_SEPARATOR
} BreakResult;

struct _ParaBreakState
{
  PangoAttrList *attrs;		/* Attributes being used for itemization */
  GList *items;			/* This paragraph turned into items */
  PangoDirection base_dir;	/* Current resolved base direction */
  gboolean first_line;		/* TRUE if this is the first line of the paragraph */
  int line_start_index;		/* Start index of line in layout->text */

  int remaining_width;		/* Amount of space remaining on line; < 0 is infinite */

  int start_offset;		/* Character offset of first item in state->items in layout->text */
  PangoGlyphString *glyphs;	/* Glyphs for the first item in state->items */
  ItemProperties properties;	/* Properties for the first item in state->items */
  PangoGlyphUnit *log_widths;	/* Logical widths for first item in state->items.. */
  int log_widths_offset;        /* Offset into log_widths to the point corresponding
				 * to the remaining portion of the first item */
};

static PangoGlyphString *
shape_run (PangoLayoutLine *line,
	   ParaBreakState  *state,
	   PangoItem       *item)
{
  PangoLayout *layout = line->layout;
  PangoGlyphString *glyphs = pango_glyph_string_new ();
  
  if (layout->text[item->offset] == '\t')
    shape_tab (line, glyphs);
  else
    {
      if (state->properties.shape_set)
	imposed_shape (layout->text + item->offset, item->num_chars,
		       state->properties.shape_ink_rect, state->properties.shape_logical_rect,
		       glyphs);
      else 
	pango_shape (layout->text + item->offset, item->length, &item->analysis, glyphs);

      if (state->properties.letter_spacing)
	{
	  PangoGlyphItem glyph_item;
	  
	  glyph_item.item = item;
	  glyph_item.glyphs = glyphs;
	  
	  pango_glyph_item_letter_space (&glyph_item,
					 layout->text,
					 layout->log_attrs + state->start_offset,
					 state->properties.letter_spacing);

	  /* We put all the letter spacing after the last glyph, then
	   * will go back and redistribute it at the beginning and the
	   * end in a post-processing step over the whole line.
	   */
	  glyphs->glyphs[glyphs->num_glyphs - 1].geometry.width += state->properties.letter_spacing;
	}
    }

  return glyphs;
}

static void
insert_run (PangoLayoutLine *line,
	    ParaBreakState  *state,
	    PangoItem       *run_item,
	    gboolean         last_run)
{
  PangoLayoutRun *run = g_slice_new (PangoLayoutRun);

  run->item = run_item;

  if (last_run && state->log_widths_offset == 0)
    run->glyphs = state->glyphs;
  else
    run->glyphs = shape_run (line, state, run_item);

  if (last_run)
    {
      if (state->log_widths_offset > 0)
	pango_glyph_string_free (state->glyphs);
      state->glyphs = NULL;
      g_free (state->log_widths);
    }
  
  line->runs = g_slist_prepend (line->runs, run);
  line->length += run_item->length;
}

/* Tries to insert as much as possible of the item at the head of
 * state->items onto @line. Five results are possible:
 *
 *  BREAK_NONE_FIT: Couldn't fit anything.
 *  BREAK_SOME_FIT: The item was broken in the middle.
 *  BREAK_ALL_FIT: Everything fit.
 *  BREAK_EMPTY_FIT: Nothing fit, but that was ok, as we can break at the first char.
 *  BREAK_LINE_SEPARATOR: Item begins with a line separator.
 *
 * If @force_fit is TRUE, then BREAK_NONE_FIT will never
 * be returned, a run will be added even if inserting the minimum amount
 * will cause the line to overflow. This is used at the start of a line
 * and until we've found at least some place to break.
 *
 * If @no_break_at_end is TRUE, then BREAK_ALL_FIT will never be
 * returned even everything fits; the run will be broken earlier,
 * or BREAK_NONE_FIT returned. This is used when the end of the
 * run is not a break position.
 */
static BreakResult
process_item (PangoLayout     *layout,
	      PangoLayoutLine *line,
	      ParaBreakState  *state,
	      gboolean         force_fit,
	      gboolean         no_break_at_end)
{
  PangoItem *item = state->items->data;
  gboolean shape_set = FALSE;
  int width;
  int length;
  int i;
  gboolean processing_new_item = FALSE;

  /* Only one character has type G_UNICODE_LINE_SEPARATOR in Unicode 4.0;
   * update this if that changes. */
#define LINE_SEPARATOR 0x2028

  if (!state->glyphs)
    {
      pango_layout_get_item_properties (item, &state->properties);
      state->glyphs = shape_run (line, state, item);
      
      state->log_widths = NULL;
      state->log_widths_offset = 0;

      processing_new_item = TRUE;
    }

  if (!layout->single_paragraph &&
      g_utf8_get_char (layout->text + item->offset) == LINE_SEPARATOR)
    {
      insert_run (line, state, item, TRUE);
      state->log_widths_offset += item->num_chars;
      return BREAK_LINE_SEPARATOR;
    }
  
  if (state->remaining_width < 0 && !no_break_at_end)  /* Wrapping off */
    {
      insert_run (line, state, item, TRUE);

      return BREAK_ALL_FIT;
    }

  width = 0;
  if (processing_new_item)
    {
      for (i = 0; i < state->glyphs->num_glyphs; i++)
	width += state->glyphs->glyphs[i].geometry.width;
    }
  else
    {
      for (i = 0; i < item->num_chars; i++)
	width += state->log_widths[state->log_widths_offset + i];
    }

  if ((width <= state->remaining_width || (item->num_chars == 1 && !line->runs)) &&
      !no_break_at_end)
    {
      state->remaining_width -= width;
      state->remaining_width = MAX (state->remaining_width, 0);
      insert_run (line, state, item, TRUE);

      return BREAK_ALL_FIT;
    }
  else
    {
      int num_chars = item->num_chars;
      int break_num_chars = num_chars;
      int break_width = width;
      int orig_width = width;
      gboolean retrying_with_char_breaks = FALSE;

      if (processing_new_item)
	{
	  state->log_widths = g_new (PangoGlyphUnit, item->num_chars);
	  pango_glyph_string_get_logical_widths (state->glyphs,
						 layout->text + item->offset, item->length, item->analysis.level,
						 state->log_widths);

	  /* The extra run letter spacing is actually divided after
	   * the last and before the first, but it works to
	   * account it all on the last
	   */
	  if (item->num_chars > 0)
	    state->log_widths[item->num_chars - 1] += state->properties.letter_spacing;
	}

    retry_break:
      
      /* Shorten the item by one line break
       */
      while (--num_chars >= 0)
	{
	  width -= (state->log_widths)[state->log_widths_offset + num_chars];

	  /* If there are no previous runs we have to take care to grab at least one char. */
	  if (can_break_at (layout, state->start_offset + num_chars, retrying_with_char_breaks) &&
	      (num_chars > 0 || line->runs))
	    {
	      break_num_chars = num_chars;
	      break_width = width;
	      
	      if (width <= state->remaining_width || (num_chars == 1 && !line->runs))
		break;
	    }
	}

      if (layout->wrap == PANGO_WRAP_WORD_CHAR && force_fit && break_width > state->remaining_width && !retrying_with_char_breaks)
	{
	  retrying_with_char_breaks = TRUE;
	  num_chars = item->num_chars;
	  width = orig_width;
	  break_num_chars = num_chars;
	  break_width = width;
	  goto retry_break;
	}

      if (force_fit || break_width <= state->remaining_width)	/* Succesfully broke the item */
	{
	  if (state->remaining_width >= 0)
	    {
	      state->remaining_width -= break_width;
	      state->remaining_width = MAX (state->remaining_width, 0);
	    }
	  
	  if (break_num_chars == item->num_chars)
	    {
	      insert_run (line, state, item, TRUE);

	      return BREAK_ALL_FIT;
	    }
	  else if (break_num_chars == 0)
	    {
	      return BREAK_EMPTY_FIT;
	    }
	  else
	    {
	      PangoItem *new_item;

	      length = g_utf8_offset_to_pointer (layout->text + item->offset, break_num_chars) - (layout->text + item->offset);

              new_item = pango_item_split (item, length, break_num_chars);
	      
	      insert_run (line, state, new_item, FALSE);

	      state->log_widths_offset += break_num_chars;

	      /* Shaped items should never be broken */
	      g_assert (!shape_set);

	      return BREAK_SOME_FIT;
	    }
	}
      else
	{
	  pango_glyph_string_free (state->glyphs);
	  state->glyphs = NULL;
	  g_free (state->log_widths);

	  return BREAK_NONE_FIT;
	}
    }
}

/* The resolved direction for the line is always one
 * of LTR/RTL; not a week or neutral directions
 */
static void
line_set_resolved_dir (PangoLayoutLine *line,
		       PangoDirection   direction)
{
  switch (direction)
    {
    case PANGO_DIRECTION_LTR:
    case PANGO_DIRECTION_TTB_RTL:
    case PANGO_DIRECTION_WEAK_LTR:
    case PANGO_DIRECTION_NEUTRAL:
      line->resolved_dir = PANGO_DIRECTION_LTR;
      break; 
    case PANGO_DIRECTION_RTL:
    case PANGO_DIRECTION_WEAK_RTL:
    case PANGO_DIRECTION_TTB_LTR:
      line->resolved_dir = PANGO_DIRECTION_RTL;
      break;
    }
}

static void
process_line (PangoLayout    *layout,
	      ParaBreakState *state)
{
  PangoLayoutLine *line;
  
  gboolean have_break = FALSE;      /* If we've seen a possible break yet */
  int break_remaining_width = 0;    /* Remaining width before adding run with break */
  int break_start_offset = 0;	    /* Start width before adding run with break */
  GSList *break_link = NULL;        /* Link holding run before break */

  line = pango_layout_line_new (layout);
  line->start_index = state->line_start_index;
  line->is_paragraph_start = state->first_line;
  line_set_resolved_dir (line, state->base_dir);

  if (layout->ellipsize != PANGO_ELLIPSIZE_NONE)
    state->remaining_width = -1;
  else if (state->first_line)
    state->remaining_width = (layout->indent >= 0) ? layout->width - layout->indent : layout->width;
  else
    state->remaining_width = (layout->indent >= 0) ? layout->width : layout->width + layout->indent;

  while (state->items)
    {
      PangoItem *item = state->items->data;
      BreakResult result;
      int old_num_chars;
      int old_remaining_width;
      gboolean first_item_in_line;

      old_num_chars = item->num_chars;
      old_remaining_width = state->remaining_width;
      first_item_in_line = line->runs != NULL;

      result = process_item (layout, line, state, !have_break, FALSE);

      switch (result)
	{
	case BREAK_ALL_FIT:
	  if (can_break_in (layout, state->start_offset, old_num_chars, first_item_in_line))
	    {
	      have_break = TRUE;
	      break_remaining_width = old_remaining_width;
	      break_start_offset = state->start_offset;
	      break_link = line->runs->next;
	    }
	  
	  state->items = g_list_delete_link (state->items, state->items);
	  state->start_offset += old_num_chars;

	  break;

	case BREAK_EMPTY_FIT:
	  goto done;
	  
	case BREAK_SOME_FIT:
	  state->start_offset += old_num_chars - item->num_chars;
	  goto done;
	  
	case BREAK_NONE_FIT:
	  /* Back up over unused runs to run where there is a break */
	  while (line->runs && line->runs != break_link)
	    state->items = g_list_prepend (state->items, uninsert_run (line));

	  state->start_offset = break_start_offset;
	  state->remaining_width = break_remaining_width;

	  /* Reshape run to break */
	  item = state->items->data;
	  
	  old_num_chars = item->num_chars;
	  result = process_item (layout, line, state, TRUE, TRUE);
	  g_assert (result == BREAK_SOME_FIT || result == BREAK_EMPTY_FIT);
	  
	  state->start_offset += old_num_chars - item->num_chars;
	  
	  goto done;

        case BREAK_LINE_SEPARATOR:
          state->items = g_list_delete_link (state->items, state->items);
          state->start_offset += old_num_chars;
          goto done;
	}
    }

 done:  
  pango_layout_line_postprocess (line, state);
  layout->lines = g_slist_prepend (layout->lines, line);
  state->first_line = FALSE;
  state->line_start_index += line->length;
}

static void
get_items_log_attrs (const char   *text,
                     GList        *items,
                     PangoLogAttr *log_attrs,
                     int           para_delimiter_len)
{
  int offset = 0;
  int index = 0;
  
  while (items)
    {
      PangoItem tmp_item = *(PangoItem *)items->data;

      /* Accumulate all the consecutive items that match in language
       * characteristics, ignoring font, style tags, etc.
       */
      while (items->next)
	{
	  PangoItem *next_item = items->next->data;

	  /* FIXME: Handle language tags */
	  if (next_item->analysis.lang_engine != tmp_item.analysis.lang_engine)
	    break;
	  else
	    {
	      tmp_item.length += next_item->length;
	      tmp_item.num_chars += next_item->num_chars;
	    }

	  items = items->next;
	}

      /* Break the paragraph delimiters with the last item */
      if (items->next == NULL)
	{
	  tmp_item.num_chars += g_utf8_strlen (text + index + tmp_item.length, para_delimiter_len);
	  tmp_item.length += para_delimiter_len;
	}

      pango_break (text + index, tmp_item.length, &tmp_item.analysis,
                   log_attrs + offset, tmp_item.num_chars + 1);

      offset += tmp_item.num_chars;
      index += tmp_item.length;
      
      items = items->next;
    }
}

static PangoAttrList *
pango_layout_get_effective_attributes (PangoLayout *layout)
{
  PangoAttrList *attrs;
  
  if (layout->attrs)
   attrs = pango_attr_list_copy (layout->attrs);
  else
    attrs = pango_attr_list_new ();

  if (layout->font_desc)
    {
      PangoAttribute *attr = pango_attr_font_desc_new (layout->font_desc);
      attr->start_index = 0;
      attr->end_index = layout->length;
	  
      pango_attr_list_insert_before (attrs, attr);
    }

  return attrs;
}

static gboolean
no_shape_filter_func (PangoAttribute *attribute,
		      gpointer        data)
{
  static const PangoAttrType no_shape_types[] = {
    PANGO_ATTR_FOREGROUND,
    PANGO_ATTR_BACKGROUND,
    PANGO_ATTR_UNDERLINE,
    PANGO_ATTR_STRIKETHROUGH,
    PANGO_ATTR_RISE
  };

  int i;

  for (i = 0; i < (int)G_N_ELEMENTS (no_shape_types); i++)
    if (attribute->klass->type == no_shape_types[i])
      return TRUE;

  return FALSE;
}

static PangoAttrList *
filter_no_shape_attributes (PangoAttrList *attrs)
{
  return pango_attr_list_filter (attrs,
				 no_shape_filter_func,
				 NULL);
}

static void
apply_no_shape_attributes (PangoLayout   *layout,
			   PangoAttrList *no_shape_attrs)
{
  GSList *line_list;

  for (line_list = layout->lines; line_list; line_list = line_list->next)
    {
      PangoLayoutLine *line = line_list->data;
      GSList *old_runs = g_slist_reverse (line->runs);
      GSList *run_list;
      
      line->runs = NULL;
      for (run_list = old_runs; run_list; run_list = run_list->next)
	{
	  PangoGlyphItem *glyph_item = run_list->data;
	  GSList *new_runs;
	  
	  new_runs = pango_glyph_item_apply_attrs (glyph_item,
						   layout->text,
						   no_shape_attrs);

	  line->runs = g_slist_concat (new_runs, line->runs);
	}
      
      g_slist_free (old_runs);
    }
}

static void
pango_layout_check_lines (PangoLayout *layout)
{
  const char *start;
  gboolean done = FALSE;
  int start_offset;
  PangoAttrList *attrs;
  PangoAttrList *no_shape_attrs;
  PangoAttrIterator *iter;
  PangoDirection prev_base_dir = PANGO_DIRECTION_NEUTRAL, base_dir = PANGO_DIRECTION_NEUTRAL;
  
  if (layout->lines)
    return;

  g_assert (!layout->log_attrs);

  /* For simplicity, we make sure at this point that layout->text
   * is non-NULL even if it is zero length
   */
  if (!layout->text)
    pango_layout_set_text (layout, NULL, 0);

  attrs = pango_layout_get_effective_attributes (layout);
  no_shape_attrs = filter_no_shape_attributes (attrs);
  iter = pango_attr_list_get_iterator (attrs);
  
  layout->log_attrs = g_new (PangoLogAttr, layout->n_chars + 1);
  
  start_offset = 0;
  start = layout->text;
  
  /* Find the first strong direction of the text */
  if (layout->auto_dir)
    {
      prev_base_dir = pango_find_base_dir (layout->text, layout->length);
      if (prev_base_dir == PANGO_DIRECTION_NEUTRAL)
	prev_base_dir = pango_context_get_base_dir (layout->context);
    }
  else
    base_dir = pango_context_get_base_dir (layout->context);

  do
    {
      int delim_len;
      const char *end;
      int delimiter_index, next_para_index;
      ParaBreakState state;

      if (layout->single_paragraph)
        {
          delimiter_index = layout->length;
          next_para_index = layout->length;
        }
      else
        {
          pango_find_paragraph_boundary (start,
                                         (layout->text + layout->length) - start,
                                         &delimiter_index,
                                         &next_para_index);
        }

      g_assert (next_para_index >= delimiter_index);

      if (layout->auto_dir)
	{
	  base_dir = pango_find_base_dir (start, delimiter_index);
	  
	  /* Propagate the base direction for neutral paragraphs */
	  if (base_dir == PANGO_DIRECTION_NEUTRAL)
	    base_dir = prev_base_dir;
	  else
	    prev_base_dir = base_dir;
	}

      end = start + delimiter_index;
      
      delim_len = next_para_index - delimiter_index;
      
      if (end == (layout->text + layout->length))
	done = TRUE;

      g_assert (end <= (layout->text + layout->length));
      g_assert (start <= (layout->text + layout->length));
      g_assert (delim_len < 4);	/* PS is 3 bytes */
      g_assert (delim_len >= 0);

      state.attrs = attrs;
      state.items = pango_itemize_with_base_dir (layout->context,
						 base_dir,
						 layout->text,
						 start - layout->text,
						 end - start,
						 attrs,
						 iter);

      get_items_log_attrs (start, state.items,
                           layout->log_attrs + start_offset,
                           delim_len);

      if (state.items)
	{
	  state.first_line = TRUE;
	  state.base_dir = base_dir;
	  state.start_offset = start_offset;
          state.line_start_index = start - layout->text;

	  state.glyphs = NULL;
	  state.log_widths = NULL;
	  
	  while (state.items)
            process_line (layout, &state);
	}
      else
        {
          PangoLayoutLine *empty_line;

          empty_line = pango_layout_line_new (layout);
          empty_line->start_index = start - layout->text;
	  empty_line->is_paragraph_start = TRUE;
	  line_set_resolved_dir (empty_line, base_dir);

          layout->lines = g_slist_prepend (layout->lines,
                                           empty_line);
        }

      if (!done)
        start_offset += g_utf8_strlen (start, (end - start) + delim_len);

      start = end + delim_len;
    }
  while (!done);

  pango_attr_iterator_destroy (iter);
  pango_attr_list_unref (attrs);

  if (no_shape_attrs)
    {
      apply_no_shape_attributes (layout, no_shape_attrs);
      pango_attr_list_unref (no_shape_attrs);
    }

  layout->lines = g_slist_reverse (layout->lines);
}

/**
 * pango_layout_line_ref:
 * @line: a #PangoLayoutLine
 * 
 * Increase the reference count of a #PangoLayoutLine by one.
 *
 * Return value: the line passed in.
 *
 * Since: 1.10
 **/
PangoLayoutLine *
pango_layout_line_ref (PangoLayoutLine *line)
{
  PangoLayoutLinePrivate *private = (PangoLayoutLinePrivate *)line;
  
  g_return_val_if_fail (line != NULL, NULL);

  private->ref_count++;

  return line;
}

/**
 * pango_layout_line_unref:
 * @line: a #PangoLayoutLine
 * 
 * Decrease the reference count of a #PangoLayoutLine by one.
 * If the result is zero, the line and all associated memory
 * will be freed.
 **/
void
pango_layout_line_unref (PangoLayoutLine *line)
{
  PangoLayoutLinePrivate *private = (PangoLayoutLinePrivate *)line;
  
  g_return_if_fail (line != NULL);
  g_return_if_fail (private->ref_count > 0);

  private->ref_count--;
  if (private->ref_count == 0)
    {
      g_slist_foreach (line->runs, (GFunc)free_run, GINT_TO_POINTER (1));
      g_slist_free (line->runs);
      g_slice_free (PangoLayoutLinePrivate, private);
    }
}

GType
pango_layout_line_get_type(void)
{
  static GType our_type = 0;
  
  if (our_type == 0)
    our_type = g_boxed_type_register_static (I_("PangoLayoutLine"),
                                             (GBoxedCopyFunc) pango_layout_line_ref,
                                             (GBoxedFreeFunc) pango_layout_line_unref);
  return our_type;
}

/**
 * pango_layout_line_x_to_index:
 * @line:      a #PangoLayoutLine
 * @x_pos:     the X offset (in #PangoGlyphUnit)
 *             from the left edge of the line.
 * @index_:    location to store calculated byte index for
 *             the grapheme in which the user clicked.
 * @trailing:  location to store a integer indicating where
 *             in the grapheme the user clicked. It will either
 *             be zero, or the number of characters in the
 *             grapheme. 0 represents the trailing edge of the grapheme.
 *
 * Converts from x offset to the byte index of the corresponding
 * character within the text of the layout. If @x_pos is outside the line,
 * @index_ and @trailing will point to the very first or very last position
 * in the line. This determination is based on the resolved direction
 * of the paragraph; for example, if the resolved direction is
 * right-to-left, then an X position to the right of the line (after it)
 * results in 0 being stored in @index_ and @trailing. An X position to the
 * left of the line results in @index_ pointing to the (logical) last
 * grapheme in the line and @trailing being set to the number of characters
 * in that grapheme. The reverse is true for a left-to-right line.
 *
 * Return value: %FALSE if @x_pos was outside the line, %TRUE if inside
 **/
gboolean
pango_layout_line_x_to_index (PangoLayoutLine *line,
			      int              x_pos,
			      int             *index,
			      int             *trailing)
{
  GSList *tmp_list;
  gint start_pos = 0;
  gint first_index = 0; /* line->start_index */
  gint first_offset;
  gint last_index;      /* start of last grapheme in line */
  gint last_offset;
  gint end_index;       /* end iterator for line */
  gint end_offset;      /* end iterator for line */
  PangoLayout *layout;
  gint last_trailing;
  gboolean suppress_last_trailing;

  g_return_val_if_fail (line != NULL, FALSE);
  g_return_val_if_fail (LINE_IS_VALID (line), FALSE);

  if (!LINE_IS_VALID (line))
    return FALSE;

  layout = line->layout;

  /* Find the last index in the line
   */
  first_index = line->start_index;

  if (line->length == 0)
    {
      if (index)
	*index = first_index;
      if (trailing)
	*trailing = 0;
      
      return FALSE;
    }

  g_assert (line->length > 0);
  
  first_offset = g_utf8_pointer_to_offset (layout->text, layout->text + line->start_index);

  end_index = first_index + line->length;
  end_offset = first_offset + g_utf8_pointer_to_offset (layout->text + first_index, layout->text + end_index);
  
  last_index = end_index;
  last_offset = end_offset;
  last_trailing = 0;
  do
    {
      last_index = g_utf8_prev_char (layout->text + last_index) - layout->text;
      last_offset--;
      last_trailing++;
    }
  while (last_offset > first_offset && !layout->log_attrs[last_offset].is_cursor_position);

  /* This is a HACK. If a program only keeps track if cursor (etc)
   * indices and not the trailing flag, then the trailing index of the
   * last character on a wrapped line is identical to the leading
   * index of the next line. So, we fake it and set the trailing flag
   * to zero.
   *
   * That is, if the text is "now is the time", and is broken between
   * 'now' and 'is'
   * 
   * Then when the cursor is actually at:
   *
   * n|o|w| |i|s| 
   *              ^
   * we lie and say it is at:
   *
   * n|o|w| |i|s| 
   *            ^
   *
   * So the cursor won't appear on the next line before 'the'.
   *
   * Actually, any program keeping cursor
   * positions with wrapped lines should distinguish leading and
   * trailing cursors.
   */
  tmp_list = layout->lines;
  while (tmp_list->data != line)
    tmp_list = tmp_list->next;

  if (tmp_list->next &&
      line->start_index + line->length == ((PangoLayoutLine *)tmp_list->next->data)->start_index)
    suppress_last_trailing = TRUE;
  else
    suppress_last_trailing = FALSE;
  
  if (x_pos < 0)
    {
      /* pick the leftmost char */
      if (index)
	*index = (line->resolved_dir == PANGO_DIRECTION_LTR) ? first_index : last_index;
      /* and its leftmost edge */
      if (trailing)
	*trailing = (line->resolved_dir == PANGO_DIRECTION_LTR || suppress_last_trailing) ? 0 : last_trailing;
      
      return FALSE;
    }

  tmp_list = line->runs;
  while (tmp_list)
    {
      PangoLayoutRun *run = tmp_list->data;
      ItemProperties properties;
      int logical_width;

      pango_layout_get_item_properties (run->item, &properties);

      if (properties.shape_set)
	logical_width = properties.shape_logical_rect->width;
      else
	logical_width = pango_glyph_string_get_width (run->glyphs);

      if (x_pos >= start_pos && x_pos < start_pos + logical_width)
	{
	  int offset;
	  gboolean char_trailing;
	  int grapheme_start_index;
	  int grapheme_start_offset;
	  int grapheme_end_offset;
	  int pos;
	  int char_index;

	  char_index = run->item->offset;

	  if (properties.shape_set)
	    {
	      char_trailing = FALSE;
	    }
	  else
	    {
	      pango_glyph_string_x_to_index (run->glyphs,
					     layout->text + run->item->offset, run->item->length,
					     &run->item->analysis,
					     x_pos - start_pos,
					     &pos, &char_trailing);

	      char_index += pos;
	    }

	  /* Convert from characters to graphemes */

	  offset = g_utf8_pointer_to_offset (layout->text, layout->text + char_index);

	  grapheme_start_offset = offset;
	  grapheme_start_index = char_index;
	  while (grapheme_start_offset > first_offset &&
		 !layout->log_attrs[grapheme_start_offset].is_cursor_position)
	    {
	      grapheme_start_index = g_utf8_prev_char (layout->text + grapheme_start_index) - layout->text;
	      grapheme_start_offset--;
	    }
	  
	  grapheme_end_offset = offset;
	  do
	    {
	      grapheme_end_offset++;
	    }
	  while (grapheme_end_offset < end_offset &&
		 !layout->log_attrs[grapheme_end_offset].is_cursor_position);

	  if (index)
	    *index = grapheme_start_index;
	  
	  if (trailing)
	    {
	      if ((grapheme_end_offset == end_offset && suppress_last_trailing) ||
		  offset + char_trailing <= (grapheme_start_offset + grapheme_end_offset) / 2)
		*trailing = 0;
	      else
		*trailing = grapheme_end_offset - grapheme_start_offset;
	    }

	  return TRUE;
	}

      start_pos += logical_width;
      tmp_list = tmp_list->next;
    }

  /* pick the rightmost char */
  if (index)
    *index = (line->resolved_dir == PANGO_DIRECTION_LTR) ? last_index : first_index;

  /* and its rightmost edge */
  if (trailing)
    *trailing = (line->resolved_dir == PANGO_DIRECTION_LTR && !suppress_last_trailing) ? last_trailing : 0;

  return FALSE;
}

/**
 * pango_layout_line_get_x_ranges:
 * @line:        a #PangoLayoutLine
 * @start_index: Start byte index of the logical range. If this value
 *               is less than the start index for the line, then
 *               the first range will extend all the way to the leading
 *               edge of the layout. Otherwise it will start at the
 *               leading edge of the first character.
 * @end_index:   Ending byte index of the logical range. If this value
 *               is greater than the end index for the line, then
 *               the last range will extend all the way to the trailing
 *               edge of the layout. Otherwise, it will end at the
 *               trailing edge of the last character.
 * @ranges:      location to store a pointer to an array of ranges.
 *               The array will be of length <literal>2*n_ranges</literal>, 
 *               with each range starting at <literal>(*ranges)[2*n]</literal>
 *               and of width <literal>(*ranges)[2*n + 1] - (*ranges)[2*n]</literal>. 
 *               This array must be freed with g_free(). The coordinates are relative 
 *               to the layout and are in #PangoGlyphUnit.
 * @n_ranges: The number of ranges stored in @ranges.
 * 
 * Gets a list of visual ranges corresponding to a given logical range.
 * This list is not necessarily minimal - there may be consecutive
 * ranges which are adjacent. The ranges will be sorted from left to
 * right. The ranges are with respect to the left edge of the entire
 * layout, not with respect to the line.
 **/
void
pango_layout_line_get_x_ranges (PangoLayoutLine  *line,
				int               start_index,
				int               end_index,
				int             **ranges,
				int              *n_ranges)
{
  PangoRectangle logical_rect;
  gint line_start_index = 0;
  GSList *tmp_list;
  int range_count = 0;
  int accumulated_width = 0;
  int x_offset;
  int width;
  PangoAlignment alignment;

  g_return_if_fail (line != NULL);
  g_return_if_fail (line->layout != NULL);
  g_return_if_fail (start_index <= end_index);

  alignment = get_alignment (line->layout, line);

  width = line->layout->width;
  if (width == -1 && alignment != PANGO_ALIGN_LEFT)
    {
      pango_layout_get_extents (line->layout, NULL, &logical_rect);
      width = logical_rect.width;
    }

  /* FIXME: The computations here could be optimized, by moving the
   * computations of the x_offset after we go through and figure
   * out where each range is.
   */
  pango_layout_line_get_extents (line, NULL, &logical_rect);

  /* FIXME it seems to me that width can be -1 here? */
  get_x_offset (line->layout, line, width, logical_rect.width, &x_offset);

  line_start_index = line->start_index;

  /* Allocate the maximum possible size */
  if (ranges)
    *ranges = g_new (int, 2 * (2 + g_slist_length (line->runs)));
    
  if (x_offset > 0 &&
      ((line->resolved_dir == PANGO_DIRECTION_LTR && start_index < line_start_index) ||
       (line->resolved_dir == PANGO_DIRECTION_RTL && end_index > line_start_index + line->length)))
    {
      if (ranges)
	{
	  (*ranges)[2*range_count] = 0;
	  (*ranges)[2*range_count + 1] = x_offset;
	}
      
      range_count ++;
    }

  tmp_list = line->runs;
  while (tmp_list)
    {
      PangoLayoutRun *run = (PangoLayoutRun *)tmp_list->data;

      if ((start_index < run->item->offset + run->item->length &&
	   end_index > run->item->offset))
	{
	  if (ranges)
	    {
	      int run_start_index = MAX (start_index, run->item->offset);
	      int run_end_index = MIN (end_index, run->item->offset + run->item->length);
	      int run_start_x, run_end_x;

	      g_assert (run_end_index > 0);
	      
	      /* Back the end_index off one since we want to find the trailing edge of the preceding character */

	      run_end_index = g_utf8_prev_char (line->layout->text + run_end_index) - line->layout->text;

	      pango_glyph_string_index_to_x (run->glyphs,
					     line->layout->text + run->item->offset,
					     run->item->length,
					     &run->item->analysis,
					     run_start_index - run->item->offset, FALSE,
					     &run_start_x);
	      pango_glyph_string_index_to_x (run->glyphs,
					     line->layout->text + run->item->offset,
					     run->item->length,
					     &run->item->analysis,
					     run_end_index - run->item->offset, TRUE,
					     &run_end_x);

	      (*ranges)[2*range_count] = x_offset + accumulated_width + MIN (run_start_x, run_end_x);
	      (*ranges)[2*range_count + 1] = x_offset + accumulated_width + MAX (run_start_x, run_end_x);
	    }

	  range_count++;
	}

      if (tmp_list->next)
	accumulated_width += pango_glyph_string_get_width (run->glyphs);

      tmp_list = tmp_list->next;
    }

  if (x_offset + logical_rect.width < line->layout->width &&
      ((line->resolved_dir == PANGO_DIRECTION_LTR && end_index > line_start_index + line->length) ||
       (line->resolved_dir == PANGO_DIRECTION_RTL && start_index < line_start_index)))
    {
      if (ranges)
	{
	  (*ranges)[2*range_count] = x_offset + logical_rect.width;
	  (*ranges)[2*range_count + 1] = line->layout->width;
	}
      
      range_count ++;
    }

  if (n_ranges)
    *n_ranges = range_count;
}

static void
pango_layout_line_get_empty_extents (PangoLayoutLine *line,
				     PangoRectangle  *logical_rect)
{
  if (logical_rect)
    {
      char *line_start;
      int index;
      PangoLayout *layout = line->layout;
      PangoFont *font;
      PangoFontDescription *font_desc = NULL;
      gboolean free_font_desc = FALSE;

      pango_layout_line_get_range (line, &line_start, NULL);
      index = line_start - layout->text;

      /* Find the font description for this line
       */
      if (layout->attrs)
	{
	  PangoAttrIterator *iter = pango_attr_list_get_iterator (layout->attrs);
	  int start, end;
	  
	  do
	    {
	      pango_attr_iterator_range (iter, &start, &end);
	      
	      if (start <= index && index < end)
		{
		  PangoFontDescription *base_font_desc;

		  if (layout->font_desc)
		    base_font_desc = layout->font_desc;
		  else
		    base_font_desc = pango_context_get_font_description (layout->context);

		  font_desc = pango_font_description_copy_static (base_font_desc);
		  free_font_desc = TRUE;
		    
		  pango_attr_iterator_get_font (iter,
						font_desc,
						NULL,
						NULL);

		  break;
		}

	    }
	  while (pango_attr_iterator_next (iter));
	  
	  pango_attr_iterator_destroy (iter);
	}
      else
	{
	  if (layout->font_desc)
	    font_desc = layout->font_desc;
	  else
	    font_desc = pango_context_get_font_description (layout->context);
	}

      font = pango_context_load_font (layout->context, font_desc);
      if (font)
	{
	  PangoFontMetrics *metrics;

	  metrics = pango_font_get_metrics (font,
					    pango_context_get_language (layout->context));

	  if (metrics)
	    {
	      logical_rect->y = - pango_font_metrics_get_ascent (metrics);
	      logical_rect->height = - logical_rect->y + pango_font_metrics_get_descent (metrics);

	      pango_font_metrics_unref (metrics);
	    }
	  else
	    {
	      logical_rect->y = 0;
	      logical_rect->height = 0;
	    }
	  g_object_unref (font);
	}
      else
	{
	  logical_rect->y = 0;
	  logical_rect->height = 0;
	}

      if (free_font_desc)
	pango_font_description_free (font_desc);
      
      logical_rect->x = 0;
      logical_rect->width = 0;
    }
}

static void
pango_layout_run_get_extents (PangoLayoutRun *run,
                              PangoRectangle *run_ink,
                              PangoRectangle *run_logical)
{
  PangoRectangle logical;
  ItemProperties properties;

  pango_layout_get_item_properties (run->item, &properties);

  if (!run_logical && (properties.uline != PANGO_UNDERLINE_NONE || properties.strikethrough))
    run_logical = &logical;

  if (properties.shape_set)
    imposed_extents (run->item->num_chars,
		     properties.shape_ink_rect,
		     properties.shape_logical_rect,
		     run_ink, run_logical);
  else
    pango_glyph_string_extents (run->glyphs, run->item->analysis.font,
				run_ink, run_logical);

  if (run_ink && (properties.uline != PANGO_UNDERLINE_NONE || properties.strikethrough))
    {
      PangoFontMetrics *metrics = pango_font_get_metrics (run->item->analysis.font,
							  run->item->analysis.language); 
      int underline_thickness = pango_font_metrics_get_underline_thickness (metrics);
      int underline_position = pango_font_metrics_get_underline_position (metrics);
      int strikethrough_thickness = pango_font_metrics_get_strikethrough_thickness (metrics);
      int strikethrough_position = pango_font_metrics_get_strikethrough_position (metrics);

      int new_pos;

      /* the underline/strikethrough takes x,width of logical_rect.  reflect
       * that into ink_rect.
       */
      new_pos = MIN (run_ink->x, run_logical->x);
      run_ink->width = MAX (run_ink->x + run_ink->width, run_logical->x + run_logical->width) - new_pos;
      run_ink->x = new_pos;

      /* We should better handle the case of height==0 in the following cases.
       * If run_ink->height == 0, we should adjust run_ink->y appropriately.
       */

      if (properties.strikethrough)
        {
	  if (run_ink->height == 0)
	    {
	      run_ink->height = strikethrough_thickness;
	      run_ink->y = -strikethrough_position;
	    }
	}

      switch (properties.uline)
	{
	case PANGO_UNDERLINE_ERROR:
	  run_ink->height = MAX (run_ink->height,
				 3 * underline_thickness - underline_position - run_ink->y);
	  break;
	case PANGO_UNDERLINE_SINGLE:
	  run_ink->height = MAX (run_ink->height,
				 underline_thickness - underline_position - run_ink->y);
	  break;
	case PANGO_UNDERLINE_DOUBLE:
	  run_ink->height = MAX (run_ink->height,
				 3 * underline_thickness - underline_position - run_ink->y);
	  break;
	case PANGO_UNDERLINE_LOW:
	  run_ink->height += 2 * underline_thickness;
	  break;
	case PANGO_UNDERLINE_NONE:
	  break;
	default:
	  g_critical ("unknown underline mode");
	  break;
	}

      pango_font_metrics_unref (metrics);
    }

  if (properties.rise != 0)
    {
      if (run_ink)
        run_ink->y -= properties.rise;

      if (run_logical)
	run_logical->y -= properties.rise;
    }
}

/**
 * pango_layout_line_get_extents:
 * @line:     a #PangoLayoutLine
 * @ink_rect: rectangle used to store the extents of the glyph string
 *            as drawn, or %NULL
 * @logical_rect: rectangle used to store the logical extents of the glyph
 *            string, or %NULL
 * 
 * Computes the logical and ink extents of a layout line. See
 * pango_font_get_glyph_extents() for details about the interpretation
 * of the rectangles.
 */
void
pango_layout_line_get_extents (PangoLayoutLine *line,
			       PangoRectangle  *ink_rect,
			       PangoRectangle  *logical_rect)
{
  GSList *tmp_list;
  int x_pos = 0;
  
  g_return_if_fail (LINE_IS_VALID (line));

  if (!LINE_IS_VALID (line))
    return;

  if (ink_rect)
    {
      ink_rect->x = 0;
      ink_rect->y = 0;
      ink_rect->width = 0;
      ink_rect->height = 0;
    }
  
  if (logical_rect)
    {
      logical_rect->x = 0;
      logical_rect->y = 0;
      logical_rect->width = 0;
      logical_rect->height = 0;
    }
  
  tmp_list = line->runs;
  while (tmp_list)
    {
      PangoLayoutRun *run = tmp_list->data;
      int new_pos;
      PangoRectangle run_ink;
      PangoRectangle run_logical;

      pango_layout_run_get_extents (run,
                                    ink_rect ? &run_ink : NULL,
                                    &run_logical);
      
      if (ink_rect)
	{
	  if (ink_rect->width == 0 || ink_rect->height == 0)
	    {
	      *ink_rect = run_ink;
	      ink_rect->x += x_pos;
	    }
	  else if (run_ink.width != 0 && run_ink.height != 0)
	    {
	      new_pos = MIN (ink_rect->x, x_pos + run_ink.x);
	      ink_rect->width = MAX (ink_rect->x + ink_rect->width,
				     x_pos + run_ink.x + run_ink.width) - new_pos;
	      ink_rect->x = new_pos;
	  
	      new_pos = MIN (ink_rect->y, run_ink.y);
	      ink_rect->height = MAX (ink_rect->y + ink_rect->height,
				      run_ink.y + run_ink.height) - new_pos;
	      ink_rect->y = new_pos;
	    }
	}
      
      if (logical_rect)
	{
	  new_pos = MIN (logical_rect->x, x_pos + run_logical.x);
	  logical_rect->width = MAX (logical_rect->x + logical_rect->width,
				     x_pos + run_logical.x + run_logical.width) - new_pos;
	  logical_rect->x = new_pos;
	  
	  new_pos = MIN (logical_rect->y, run_logical.y);
	  logical_rect->height = MAX (logical_rect->y + logical_rect->height,
				      run_logical.y + run_logical.height) - new_pos;
	  logical_rect->y = new_pos;
	}
      
     x_pos += run_logical.width;
     tmp_list = tmp_list->next;
    }
  
  if (logical_rect && !line->runs) 
    {
      PangoRectangle temp_rect;
      pango_layout_line_get_empty_extents (line, &temp_rect);
      logical_rect->height = temp_rect.height;
    }
}

static PangoLayoutLine *
pango_layout_line_new (PangoLayout *layout)
{
  PangoLayoutLinePrivate *private = g_slice_new (PangoLayoutLinePrivate);

  private->ref_count = 1;
  private->line.layout = layout;
  private->line.runs = NULL;
  private->line.length = 0;

  /* Note that we leave start_index, resolved_dir, and is_paragraph_start
   *  uninitialized */

  return (PangoLayoutLine *) private;
}

/**
 * pango_layout_line_get_pixel_extents:
 * @layout_line: a #PangoLayoutLine
 * @ink_rect:    rectangle used to store the extents of the glyph string
 *               as drawn, or %NULL
 * @logical_rect: rectangle used to store the logical extents of the glyph
 *               string, or %NULL
 * 
 * Computes the logical and ink extents of a layout line. See
 * pango_font_get_glyph_extents() for details about the interpretation
 * of the rectangles. The returned rectangles are in device units, as
 * opposed to pango_layout_line_get_extents(), which returns the extents in
 * #PangoGlyphUnit.
 **/
void
pango_layout_line_get_pixel_extents (PangoLayoutLine *layout_line,
				     PangoRectangle  *ink_rect,
				     PangoRectangle  *logical_rect)
{
  g_return_if_fail (LINE_IS_VALID (layout_line));

  pango_layout_line_get_extents (layout_line, ink_rect, logical_rect);

  if (ink_rect)
    {
      ink_rect->width = (ink_rect->width + PANGO_SCALE / 2) / PANGO_SCALE;
      ink_rect->height = (ink_rect->height + PANGO_SCALE / 2) / PANGO_SCALE;

      ink_rect->x = PANGO_PIXELS (ink_rect->x);
      ink_rect->y = PANGO_PIXELS (ink_rect->y);
    }

  if (logical_rect)
    {
      logical_rect->width = (logical_rect->width + PANGO_SCALE / 2) / PANGO_SCALE;
      logical_rect->height = (logical_rect->height + PANGO_SCALE / 2) / PANGO_SCALE;

      logical_rect->x = PANGO_PIXELS (logical_rect->x);
      logical_rect->y = PANGO_PIXELS (logical_rect->y);
    }
}

/*
 * NB: This implement the exact same algorithm as
 *     reorder-items.c:pango_reorder_items().
 */

static GSList *
reorder_runs_recurse (GSList *items, int n_items)
{
  GSList *tmp_list, *level_start_node;
  int i, level_start_i;
  int min_level = G_MAXINT;
  GSList *result = NULL;

  if (n_items == 0)
    return NULL;

  tmp_list = items;
  for (i=0; i<n_items; i++)
    {
      PangoLayoutRun *run = tmp_list->data;

      min_level = MIN (min_level, run->item->analysis.level);

      tmp_list = tmp_list->next;
    }

  level_start_i = 0;
  level_start_node = items;
  tmp_list = items;
  for (i=0; i<n_items; i++)
    {
      PangoLayoutRun *run = tmp_list->data;

      if (run->item->analysis.level == min_level)
	{
	  if (min_level % 2)
	    {
	      if (i > level_start_i)
		result = g_slist_concat (reorder_runs_recurse (level_start_node, i - level_start_i), result);
	      result = g_slist_prepend (result, run);
	    }
	  else
	    {
	      if (i > level_start_i)
		result = g_slist_concat (result, reorder_runs_recurse (level_start_node, i - level_start_i));
	      result = g_slist_append (result, run);
	    }

	  level_start_i = i + 1;
	  level_start_node = tmp_list->next;
	}

      tmp_list = tmp_list->next;
    }
  
  if (min_level % 2)
    {
      if (i > level_start_i)
	result = g_slist_concat (reorder_runs_recurse (level_start_node, i - level_start_i), result);
    }
  else
    {
      if (i > level_start_i)
	result = g_slist_concat (result, reorder_runs_recurse (level_start_node, i - level_start_i));
    }

  return result;
}

static void
pango_layout_line_reorder (PangoLayoutLine *line)
{
  GSList *logical_runs = line->runs;
  line->runs = reorder_runs_recurse (logical_runs, g_slist_length (logical_runs));
  g_slist_free (logical_runs);
}

static int
get_item_letter_spacing (PangoItem *item)
{
  ItemProperties properties;

  pango_layout_get_item_properties (item, &properties);

  return properties.letter_spacing;
}

static void
adjust_final_space (PangoGlyphString *glyphs,
		    int               adjustment)
{
  glyphs->glyphs[glyphs->num_glyphs - 1].geometry.width += adjustment;
}

static gboolean
is_tab_run (PangoLayout    *layout,
	    PangoLayoutRun *run)
{
  return (layout->text[run->item->offset] == '\t');
}

/* When doing shaping, we add the letter spacing value for a
 * run after every grapheme in the run. This produces ugly
 * asymetrical results, so what this routine is redistributes
 * that space to the beginning and the end of the run.
 *
 * We also trim the letter spacing from runs adjacent to
 * tabs and from the outside runs of the lines so that things
 * line up properly. The line breaking and tab positioning
 * were computed without this trimming so they are no longer
 * exactly correct, but this won't be very noticable in most
 * cases.
 */
static void
adjust_line_letter_spacing (PangoLayoutLine *line)
{
  PangoLayout *layout = line->layout;
  gboolean reversed;
  PangoLayoutRun *last_run;
  int tab_adjustment;
  GSList *l;
  
  /* If we have tab stops and the resolved direction of the
   * line is RTL, then we need to walk through the line
   * in reverse direction to figure out the corrections for
   * tab stops.
   */
  reversed = FALSE;
  if (line->resolved_dir == PANGO_DIRECTION_RTL)
    {
      for (l = line->runs; l; l = l->next)
	if (is_tab_run (layout, l->data))
	  {
	    line->runs = g_slist_reverse (line->runs);
	    reversed = TRUE;
	    break;
	  }
    }

  /* Walk over the runs in the line, redistributing letter
   * spacing from the end of the run to the start of the
   * run and trimming letter spacing from the ends of the
   * runs adjacent to the ends of the line or tab stops.
   *
   * We accumulate a correction factor from this trimming
   * which we add onto the next tab stop space to keep the
   * things properly aligned.
   */
  
  last_run = NULL;
  tab_adjustment = 0;
  for (l = line->runs; l; l = l->next)
    {
      PangoLayoutRun *run = l->data;
      PangoLayoutRun *next_run = l->next ? l->next->data : NULL;

      if (is_tab_run (layout, run))
	{
	  adjust_final_space (run->glyphs, tab_adjustment);
	  tab_adjustment = 0;
	}
      else
	{
	  PangoLayoutRun *visual_next_run = reversed ? last_run : next_run;
	  PangoLayoutRun *visual_last_run = reversed ? next_run : last_run;
	  int run_spacing = get_item_letter_spacing (run->item);
	  int adjustment = run_spacing / 2;
	  
	  if (visual_last_run && !is_tab_run (layout, visual_last_run))
	    adjust_final_space (visual_last_run->glyphs, adjustment);
	  else
	    tab_adjustment += adjustment;
	  
	  if (visual_next_run && !is_tab_run (layout, visual_next_run))
	    adjust_final_space (run->glyphs, - adjustment);
	  else
	    {
	      adjust_final_space (run->glyphs, - run_spacing);
	      tab_adjustment += run_spacing - adjustment;
	    }
	}

      last_run = run;
    }

  if (reversed)
    line->runs = g_slist_reverse (line->runs);
}

static void
pango_layout_line_postprocess (PangoLayoutLine *line,
			       ParaBreakState  *state)
{
  /* NB: the runs are in reverse order at this point, since we prepended them to the list
   */
  
  /* Reverse the runs
   */
  line->runs = g_slist_reverse (line->runs);

  /* Ellipsize the line if necessary
   */
  _pango_layout_line_ellipsize (line, state->attrs);
  
  /* Now convert logical to visual order
   */
  pango_layout_line_reorder (line);
  
  /* Fixup letter spacing between runs
   */
  adjust_line_letter_spacing (line);
}

static void
pango_layout_get_item_properties (PangoItem      *item,
				  ItemProperties *properties)
{
  GSList *tmp_list = item->analysis.extra_attrs;

  properties->uline = PANGO_UNDERLINE_NONE;
  properties->strikethrough = FALSE;
  properties->letter_spacing = 0;
  properties->rise = 0;
  properties->shape_set = FALSE;
  properties->shape_ink_rect = NULL;
  properties->shape_logical_rect = NULL;

  while (tmp_list)
    {
      PangoAttribute *attr = tmp_list->data;

      switch (attr->klass->type)
	{
	case PANGO_ATTR_UNDERLINE:
	  properties->uline = ((PangoAttrInt *)attr)->value;
	  break;

	case PANGO_ATTR_STRIKETHROUGH:
	  properties->strikethrough = ((PangoAttrInt *)attr)->value;
	  break;
	  
        case PANGO_ATTR_RISE:
	  properties->rise = ((PangoAttrInt *)attr)->value;
          break;
          
	case PANGO_ATTR_LETTER_SPACING:
	  properties->letter_spacing = ((PangoAttrInt *)attr)->value;
	  break;

	case PANGO_ATTR_SHAPE:
	  properties->shape_set = TRUE;
	  properties->shape_logical_rect = &((PangoAttrShape *)attr)->logical_rect;
	  properties->shape_ink_rect = &((PangoAttrShape *)attr)->ink_rect;
	  break;

	default:
	  break;
	}
      tmp_list = tmp_list->next;
    }
}

static int
next_cluster_start (PangoGlyphString *gs,
                    int               cluster_start)
{
  int i;

  i = cluster_start + 1;
  while (i < gs->num_glyphs)
    {
      if (gs->glyphs[i].attr.is_cluster_start)
        return i;
      
      i++;
    }

  return gs->num_glyphs;
}

static int
cluster_width (PangoGlyphString *gs,
	       int               cluster_start)
{
  int i;
  int log_cluster;
  int width = 0;

  log_cluster = gs->log_clusters[cluster_start];
  for (i = cluster_start; i < gs->num_glyphs; i++) 
    {
      if (gs->log_clusters[i] != log_cluster)
	break;
      width += gs->glyphs[i].geometry.width;
    }

  return width;
}

static inline void
offset_y (PangoLayoutIter *iter,
          int             *y)
{
  Extents *line_ext;

  line_ext = (Extents*)iter->line_extents_link->data;

  *y += line_ext->baseline;
}

/* Sets up the iter for the start of a new cluster. cluster_start_index
 * is the byte index of the cluster start relative to the run.
 */
static void 
update_cluster (PangoLayoutIter *iter,
		int              cluster_start_index)
{
  char             *cluster_text;
  PangoGlyphString *gs;
  int               cluster_length;

  iter->character_position = 0;

  gs = iter->run->glyphs;
  iter->cluster_width = cluster_width (gs, iter->cluster_start);
  iter->next_cluster_glyph = next_cluster_start (gs, iter->cluster_start);

  if (iter->ltr)
    {
      /* For LTR text, finding the length of the cluster is easy
       * since logical and visual runs are in the same direction.
       */
      if (iter->next_cluster_glyph < gs->num_glyphs)
	cluster_length = gs->log_clusters[iter->next_cluster_glyph] - cluster_start_index;
      else
	cluster_length = iter->run->item->length - cluster_start_index;
    }
  else
    {
      /* For RTL text, we have to scan backwards to find the previous
       * visual cluster which is the next logical cluster.
       */
      int i = iter->cluster_start;
      while (i > 0 && gs->log_clusters[i - 1] == cluster_start_index)
	i--;

      if (i == 0)
	cluster_length = iter->run->item->length - cluster_start_index;
      else
	cluster_length = gs->log_clusters[i - 1] - cluster_start_index;
    }

  cluster_text = iter->layout->text + iter->run->item->offset + cluster_start_index;
  iter->cluster_num_chars = g_utf8_strlen (cluster_text, cluster_length);

  if (iter->ltr)
    iter->index = cluster_text - iter->layout->text;
  else
    iter->index = g_utf8_prev_char (cluster_text + cluster_length) - iter->layout->text;
}

static void
update_run (PangoLayoutIter *iter,
	    int              run_start_index)
{
  Extents *line_ext;

  line_ext = (Extents*)iter->line_extents_link->data;
  
  /* Note that in iter_new() the iter->run_logical_rect.width
   * is garbage but we don't use it since we're on the first run of
   * a line.
   */
  if (iter->run_list_link == iter->line->runs)
    iter->run_x = line_ext->logical_rect.x;
  else
    iter->run_x += iter->run_logical_rect.width;
  
  if (iter->run)
    {
      pango_layout_run_get_extents (iter->run,
                                    NULL,
                                    &iter->run_logical_rect);

      /* Fix coordinates of the run extents to be layout-relative*/
      iter->run_logical_rect.x += iter->run_x;
      
      offset_y (iter, &iter->run_logical_rect.y);
    }
  else
    {
      /* The empty run at the end of a line */
      
      iter->run_logical_rect.x = iter->run_x;
      iter->run_logical_rect.y = line_ext->logical_rect.y;
      iter->run_logical_rect.width = 0;
      iter->run_logical_rect.height = line_ext->logical_rect.height;
    }

  if (iter->run)
    iter->ltr = (iter->run->item->analysis.level % 2) == 0;
  else
    iter->ltr = TRUE;

  iter->cluster_start = 0;
  iter->cluster_x = iter->run_logical_rect.x;

  if (iter->run)
    {
      update_cluster (iter, iter->run->glyphs->log_clusters[0]);
    }
  else
    {
      iter->cluster_width = 0;
      iter->character_position = 0;
      iter->cluster_num_chars = 0;
      iter->index = run_start_index;
    }
}

static PangoLayoutIter *
pango_layout_iter_copy (PangoLayoutIter *iter)
{
  PangoLayoutIter *new;
  GSList *l;

  new = g_slice_new (PangoLayoutIter);
  
  new->layout = g_object_ref (iter->layout);
  new->line_list_link = iter->line_list_link;
  new->line = iter->line;
  pango_layout_line_ref (new->line);

  new->run_list_link = iter->run_list_link;
  new->run = iter->run;
  new->index = iter->index;

  new->logical_rect = iter->logical_rect;

  new->line_extents = NULL;
  new->line_extents_link = NULL;
  for (l = iter->line_extents; l; l = l->next)
    {
      new->line_extents = g_slist_prepend (new->line_extents,
					   g_memdup (l->data,
						     sizeof (Extents)));
      if (l == iter->line_extents_link)
	new->line_extents_link = new->line_extents;
    }
  new->line_extents = g_slist_reverse (new->line_extents);
  
  new->run_x = iter->run_x;
  new->run_logical_rect = iter->run_logical_rect;
  new->ltr = iter->ltr;
  
  new->cluster_x = iter->cluster_x;
  new->cluster_width = iter->cluster_width;

  new->cluster_start = iter->cluster_start;
  new->next_cluster_glyph = iter->next_cluster_glyph;

  new->cluster_num_chars = iter->cluster_num_chars;
  new->character_position = iter->character_position;

  new->layout_width = iter->layout_width;

  return new;
}

GType
pango_layout_iter_get_type (void)
{
  static GType our_type = 0;
  
  if (our_type == 0)
    our_type = g_boxed_type_register_static (I_("PangoLayoutIter"),
					     (GBoxedCopyFunc)pango_layout_iter_copy,
					     (GBoxedFreeFunc)pango_layout_iter_free);

  return our_type;
}

/**
 * pango_layout_get_iter:
 * @layout: a #PangoLayout
 * 
 * Returns an iterator to iterate over the visual extents of the layout.
 * 
 * Return value: the new #PangoLayoutIter that should be freed using
 *               pango_layout_iter_free().
 **/
PangoLayoutIter*
pango_layout_get_iter (PangoLayout *layout)
{
  int run_start_index;
  PangoLayoutIter *iter;
  
  g_return_val_if_fail (PANGO_IS_LAYOUT (layout), NULL);
  
  iter = g_slice_new (PangoLayoutIter);

  iter->layout = layout;
  g_object_ref (iter->layout);

  pango_layout_check_lines (layout);
  
  iter->line_list_link = layout->lines;
  iter->line = iter->line_list_link->data;
  pango_layout_line_ref (iter->line);

  run_start_index = iter->line->start_index;
  iter->run_list_link = iter->line->runs;

  if (iter->run_list_link)
    {
      iter->run = iter->run_list_link->data;
      run_start_index = iter->run->item->offset;
    }
  else
    iter->run = NULL;

  iter->line_extents = NULL;
  pango_layout_get_extents_internal (layout,
                                     NULL,
                                     &iter->logical_rect,
                                     &iter->line_extents,
				     &iter->layout_width);

  iter->line_extents_link = iter->line_extents;

  update_run (iter, run_start_index);

  return iter;
}

/**
 * pango_layout_iter_free:
 * @iter: a #PangoLayoutIter
 * 
 * Frees an iterator that's no longer in use.
 **/
void
pango_layout_iter_free (PangoLayoutIter *iter)
{
  g_return_if_fail (iter != NULL);

  g_slist_foreach (iter->line_extents, (GFunc)extents_free, NULL);
  g_slist_free (iter->line_extents);
  pango_layout_line_unref (iter->line);
  g_object_unref (iter->layout);
  g_slice_free (PangoLayoutIter, iter);
}

/**
 * pango_layout_iter_get_index:
 * @iter: a #PangoLayoutIter
 * 
 * Gets the current byte index. Note that iterating forward by char
 * moves in visual order, not logical order, so indexes may not be
 * sequential. Also, the index may be equal to the length of the text
 * in the layout, if on the %NULL run (see pango_layout_iter_get_run()).
 * 
 * Return value: current byte index.
 **/
int
pango_layout_iter_get_index (PangoLayoutIter *iter)
{
  if (ITER_IS_INVALID (iter))
    return 0;
  
  return iter->index;
}

/**
 * pango_layout_iter_get_run:
 * @iter: a #PangoLayoutIter
 * 
 * Gets the current run. When iterating by run, at the end of each
 * line, there's a position with a %NULL run, so this function can return
 * %NULL. The %NULL run at the end of each line ensures that all lines have
 * at least one run, even lines consisting of only a newline.
 * 
 * Return value: the current run.
 **/
PangoLayoutRun*
pango_layout_iter_get_run (PangoLayoutIter *iter)
{
  if (ITER_IS_INVALID (iter))
    return NULL;

  return iter->run;
}

/**
 * pango_layout_iter_get_line:
 * @iter: a #PangoLayoutIter
 * 
 * Gets the current line.
 * 
 * Return value: the current line.
 **/
PangoLayoutLine*
pango_layout_iter_get_line (PangoLayoutIter *iter)
{
  if (ITER_IS_INVALID (iter))
    return NULL;

  return iter->line;
}

/**
 * pango_layout_iter_at_last_line:
 * @iter: a #PangoLayoutIter
 * 
 * Determines whether @iter is on the last line of the layout.
 * 
 * Return value: %TRUE if @iter is on the last line.
 **/
gboolean
pango_layout_iter_at_last_line (PangoLayoutIter *iter)
{
  if (ITER_IS_INVALID (iter))
    return FALSE;

  return iter->line_extents_link->next == NULL;
}

static gboolean
line_is_terminated (PangoLayoutIter *iter)
{
  /* There is a real terminator at the end of each paragraph other
   * than the last.
   */
  if (iter->line_list_link->next)
    {
      PangoLayoutLine *next_line = iter->line_list_link->next->data;
      if (next_line->is_paragraph_start)
	return TRUE;
    }

  return FALSE;
}

/* Moves to the next non-empty line. If @include_terminators
 * is set, a line with just an explicit paragraph separator
 * is considered non-empty.
 */
static gboolean
next_nonempty_line (PangoLayoutIter *iter,
		    gboolean         include_terminators)
{
  gboolean result;

  while (TRUE)
    {
      result = pango_layout_iter_next_line (iter);
      if (!result)
	break;
      
      if (iter->line->runs)
	break;
      
      if (include_terminators && line_is_terminated (iter))
	break;
    }
  
  return result;
}

/* Moves to the next non-empty run. If @include_terminators
 * is set, the trailing run at the end of a line with an explicit
 * paragraph separator is considered non-empty.
 */
static gboolean
next_nonempty_run (PangoLayoutIter *iter,
		    gboolean         include_terminators)
{
  gboolean result;
  
  while (TRUE)
    {
      result = pango_layout_iter_next_run (iter);
      if (!result)
	break;
      
      if (iter->run)
	break;
      
      if (include_terminators && line_is_terminated (iter))
	break;
    }
  
  return result;
}

/* Like pango_layout_next_cluster(), but if @include_terminators
 * is set, includes the fake runs/clusters for empty lines.
 * (But not positions introduced by line wrapping).
 */
static gboolean
next_cluster_internal (PangoLayoutIter *iter,
		       gboolean         include_terminators)
{
  PangoGlyphString *gs;
  int               next_start;
  
  if (ITER_IS_INVALID (iter))
    return FALSE;

  if (iter->run == NULL)
    return next_nonempty_line (iter, include_terminators);
  
  gs = iter->run->glyphs;
  
  next_start = iter->next_cluster_glyph;
  if (next_start == gs->num_glyphs)
    {
      return next_nonempty_run (iter, include_terminators);
    }
  else
    {
      iter->cluster_start = next_start;
      iter->cluster_x += iter->cluster_width;
      update_cluster(iter, gs->log_clusters[iter->cluster_start]);

      return TRUE;
    }
}

/**
 * pango_layout_iter_next_char:
 * @iter: a #PangoLayoutIter
 * 
 * Moves @iter forward to the next character in visual order. If @iter was already at
 * the end of the layout, returns %FALSE.
 * 
 * Return value: whether motion was possible.
 **/
gboolean
pango_layout_iter_next_char (PangoLayoutIter *iter)
{
  const char *text;

  if (ITER_IS_INVALID (iter))
    return FALSE;

  if (iter->run == NULL)
    {
      /* We need to fake an iterator position in the middle of a \r\n line terminator */
      if (line_is_terminated (iter) &&
	  strncmp (iter->layout->text + iter->line->start_index + iter->line->length, "\r\n", 2) == 0 &&
	  iter->character_position == 0)
	{
	  iter->character_position++;
	  return TRUE;
	}

      return next_nonempty_line (iter, TRUE);
    }

  iter->character_position++;
  if (iter->character_position >= iter->cluster_num_chars)
    return next_cluster_internal (iter, TRUE);
  
  text = iter->layout->text;
  if (iter->ltr)
    iter->index = g_utf8_next_char (text + iter->index) - text;
  else
    iter->index = g_utf8_prev_char (text + iter->index) - text;
  
  return TRUE;
}

/**
 * pango_layout_iter_next_cluster:
 * @iter: a #PangoLayoutIter
 * 
 * Moves @iter forward to the next cluster in visual order. If @iter
 * was already at the end of the layout, returns %FALSE.
 * 
 * Return value: whether motion was possible.
 **/
gboolean
pango_layout_iter_next_cluster (PangoLayoutIter *iter)
{
  return next_cluster_internal (iter, FALSE);
}

/**
 * pango_layout_iter_next_run:
 * @iter: a #PangoLayoutIter
 * 
 * Moves @iter forward to the next run in visual order. If @iter was
 * already at the end of the layout, returns %FALSE.
 * 
 * Return value: whether motion was possible.
 **/
gboolean
pango_layout_iter_next_run (PangoLayoutIter *iter)
{
  int next_run_start; /* byte index */
  GSList *next_link;
  
  if (ITER_IS_INVALID (iter))
    return FALSE;

  if (iter->run == NULL)
    return pango_layout_iter_next_line (iter);

  next_link = iter->run_list_link->next;

  if (next_link == NULL)
    {
      /* Moving on to the zero-width "virtual run" at the end of each
       * line
       */
      next_run_start = iter->run->item->offset + iter->run->item->length; 
      iter->run = NULL;
      iter->run_list_link = NULL;
    }
  else
    {
      iter->run_list_link = next_link;
      iter->run = iter->run_list_link->data;
      next_run_start = iter->run->item->offset;
    }
  
  update_run (iter, next_run_start);
  
  return TRUE;
}

/**
 * pango_layout_iter_next_line:
 * @iter: a #PangoLayoutIter
 * 
 * Moves @iter forward to the start of the next line. If @iter is
 * already on the last line, returns %FALSE.
 * 
 * Return value: whether motion was possible.
 **/
gboolean
pango_layout_iter_next_line (PangoLayoutIter *iter)
{
  GSList *next_link;

  if (ITER_IS_INVALID (iter))
    return FALSE;

  next_link = iter->line_list_link->next;

  if (next_link == NULL)
    return FALSE;

  iter->line_list_link = next_link;

  pango_layout_line_unref (iter->line);
  
  iter->line = iter->line_list_link->data;

  pango_layout_line_ref (iter->line);

  iter->run_list_link = iter->line->runs;

  if (iter->run_list_link)
    iter->run = iter->run_list_link->data;
  else
    iter->run = NULL;

  iter->line_extents_link = iter->line_extents_link->next;
  g_assert (iter->line_extents_link != NULL);  

  update_run (iter, iter->line->start_index);
  
  return TRUE;
}

/**
 * pango_layout_iter_get_char_extents:
 * @iter: a #PangoLayoutIter
 * @logical_rect: rectangle to fill with logical extents
 * 
 * Gets the extents of the current character, in layout coordinates
 * (origin is the top left of the entire layout). Only logical extents
 * can sensibly be obtained for characters; ink extents make sense only
 * down to the level of clusters.
 * 
 **/
void
pango_layout_iter_get_char_extents (PangoLayoutIter *iter,
                                    PangoRectangle  *logical_rect)
{
  PangoRectangle cluster_rect;
  int            x0, x1;

  if (ITER_IS_INVALID (iter))
    return;

  if (logical_rect == NULL)
    return;

  pango_layout_iter_get_cluster_extents (iter, NULL, &cluster_rect);

  if (iter->run == NULL)
    {
      /* When on the NULL run, cluster, char, and run all have the
       * same extents
       */
      *logical_rect = cluster_rect;
      return;
    }
  
  g_assert (cluster_rect.width == iter->cluster_width);

  x0 = (iter->character_position * cluster_rect.width) / iter->cluster_num_chars;
  x1 = ((iter->character_position + 1) * cluster_rect.width) / iter->cluster_num_chars;

  logical_rect->width = x1 - x0;
  logical_rect->height = cluster_rect.height;
  logical_rect->y = cluster_rect.y;
  logical_rect->x = cluster_rect.x + x0;
}

/**
 * pango_layout_iter_get_cluster_extents:
 * @iter: a #PangoLayoutIter
 * @ink_rect: rectangle to fill with ink extents, or %NULL
 * @logical_rect: rectangle to fill with logical extents, or %NULL
 *
 * Gets the extents of the current cluster, in layout coordinates
 * (origin is the top left of the entire layout).
 * 
 **/
void
pango_layout_iter_get_cluster_extents (PangoLayoutIter *iter,
                                       PangoRectangle  *ink_rect,
                                       PangoRectangle  *logical_rect)
{
  if (ITER_IS_INVALID (iter))
    return;

  if (iter->run == NULL)
    {
      /* When on the NULL run, cluster, char, and run all have the
       * same extents
       */
      pango_layout_iter_get_run_extents (iter, ink_rect, logical_rect);
      return;
    }
  
  pango_glyph_string_extents_range (iter->run->glyphs,
                                    iter->cluster_start,
                                    iter->next_cluster_glyph,
                                    iter->run->item->analysis.font,
                                    ink_rect,
                                    logical_rect);

  if (ink_rect)
    {
      ink_rect->x += iter->cluster_x;
      offset_y (iter, &ink_rect->y);
    }

  if (logical_rect)
    {
      logical_rect->x += iter->cluster_x;
      offset_y (iter, &logical_rect->y);
    }
}

/**
 * pango_layout_iter_get_run_extents:
 * @iter: a #PangoLayoutIter
 * @ink_rect: rectangle to fill with ink extents, or %NULL
 * @logical_rect: rectangle to fill with logical extents, or %NULL
 * 
 * Gets the extents of the current run in layout coordinates
 * (origin is the top left of the entire layout).
 * 
 **/
void
pango_layout_iter_get_run_extents (PangoLayoutIter *iter,
                                   PangoRectangle  *ink_rect,
                                   PangoRectangle  *logical_rect)
{
  if (ITER_IS_INVALID (iter))
    return;

  if (ink_rect)
    {
      if (iter->run)
        {
          pango_layout_run_get_extents (iter->run, ink_rect, NULL);
          offset_y (iter, &ink_rect->y);
          ink_rect->x += iter->run_x;
        }
      else
        {
          PangoRectangle line_ink;

          pango_layout_iter_get_line_extents (iter, &line_ink, NULL);
          
          ink_rect->x = iter->run_x;          
          ink_rect->y = line_ink.y;
          ink_rect->width = 0;
          ink_rect->height = line_ink.height;
        }
    }
      
  if (logical_rect)
    *logical_rect = iter->run_logical_rect;
}

/**
 * pango_layout_iter_get_line_extents:
 * @iter: a #PangoLayoutIter
 * @ink_rect: rectangle to fill with ink extents, or %NULL
 * @logical_rect: rectangle to fill with logical extents, or %NULL
 *
 * Obtains the extents of the current line. @ink_rect or @logical_rect
 * can be NULL if you aren't interested in them. Extents are in layout
 * coordinates (origin is the top-left corner of the entire
 * #PangoLayout).  Thus the extents returned by this function will be
 * the same width/height but not at the same x/y as the extents
 * returned from pango_layout_line_get_extents().
 * 
 **/
void
pango_layout_iter_get_line_extents (PangoLayoutIter *iter,
                                    PangoRectangle  *ink_rect,
                                    PangoRectangle  *logical_rect)
{
  Extents *ext;
  
  if (ITER_IS_INVALID (iter))
    return;

  ext = iter->line_extents_link->data;

  if (ink_rect)
    {
      get_line_extents_layout_coords (iter->layout, iter->line,
                                      iter->layout_width,
                                      ext->logical_rect.y,
                                      NULL,
                                      ink_rect,
                                      NULL);
    }
      
  if (logical_rect)
    *logical_rect = ext->logical_rect;
}

/**
 * pango_layout_iter_get_line_yrange:
 * @iter: a #PangoLayoutIter
 * @y0_: start of line 
 * @y1_: end of line
 *
 * Divides the vertical space in the #PangoLayout being iterated over
 * between the lines in the layout, and returns the space belonging to
 * the current line.  A line's range includes the line's logical
 * extents, plus half of the spacing above and below the line, if
 * pango_layout_set_spacing() has been called to set layout spacing.
 * The Y positions are in layout coordinates (origin at top left of the
 * entire layout).
 * 
 **/
void
pango_layout_iter_get_line_yrange (PangoLayoutIter *iter,
                                   int             *y0,
                                   int             *y1)
{
  Extents *ext;
  int half_spacing;
  
  if (ITER_IS_INVALID (iter))
    return;

  ext = iter->line_extents_link->data;

  half_spacing = iter->layout->spacing / 2;

  /* Note that if layout->spacing is odd, the remainder spacing goes
   * above the line (this is pretty arbitrary of course)
   */
  
  if (y0)
    {
      /* No spacing above the first line */
      
      if (iter->line_extents_link == iter->line_extents)
        *y0 = ext->logical_rect.y;
      else
        *y0 = ext->logical_rect.y - (iter->layout->spacing - half_spacing);
    }

  if (y1)
    {
      /* No spacing below the last line */
      if (iter->line_extents_link->next == NULL)
        *y1 = ext->logical_rect.y + ext->logical_rect.height;
      else
        *y1 = ext->logical_rect.y + ext->logical_rect.height + half_spacing;
    }
}

/**
 * pango_layout_iter_get_baseline:
 * @iter: a #PangoLayoutIter
 * 
 * Gets the Y position of the current line's baseline, in layout
 * coordinates (origin at top left of the entire layout).
 * 
 * Return value: baseline of current line.
 **/
int
pango_layout_iter_get_baseline (PangoLayoutIter *iter)
{
  Extents *ext;
  
  if (ITER_IS_INVALID (iter))
    return 0;

  ext = iter->line_extents_link->data;

  return ext->baseline;
}

/**
 * pango_layout_iter_get_layout_extents:
 * @iter: a #PangoLayoutIter
 * @ink_rect: rectangle to fill with ink extents, or %NULL
 * @logical_rect: rectangle to fill with logical extents, or %NULL
 *
 * Obtains the extents of the #PangoLayout being iterated
 * over. @ink_rect or @logical_rect can be NULL if you
 * aren't interested in them.
 * 
 **/
void
pango_layout_iter_get_layout_extents  (PangoLayoutIter *iter,
                                       PangoRectangle  *ink_rect,
                                       PangoRectangle  *logical_rect)
{
  if (ITER_IS_INVALID (iter))
    return;

  if (ink_rect)
    {
      /* expensive recomputation, woo-hoo */
      pango_layout_get_extents (iter->layout,
                                ink_rect,
                                NULL);
    }

  if (logical_rect)
    *logical_rect = iter->logical_rect;
}
