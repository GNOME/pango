/* Pango
 * pango-layout.c: Highlevel layout driver
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

#include <pango/pango-layout.h>
#include <pango/pango.h>		/* For pango_shape() */
#include <string.h>
#include <unicode.h>

#define LINE_IS_VALID(line) ((line)->layout != NULL)

typedef struct _PangoLayoutLinePrivate PangoLayoutLinePrivate;

struct _PangoLayout
{
  guint ref_count;

  PangoContext *context;
  PangoAttrList *attrs;
  gchar *text;
  int length;			/* length of text in bytes */
  int width;			/* wrap width, in device units */
  int indent;			/* amount by which first line should be shorter */

  guint justify : 1;
  guint alignment : 2;

  gint n_chars;		        /* Total number of characters in layout */
  PangoLogAttr *log_attrs;	/* Logical attributes for layout's text */

  GSList *lines;
};

struct _PangoLayoutLinePrivate
{
  PangoLayoutLine line;
  guint ref_count;
};

static void pango_layout_clear_lines (PangoLayout *layout);
static void pango_layout_check_lines (PangoLayout *layout);

static PangoLayoutLine * pango_layout_line_new        (PangoLayout     *layout);
static void              pango_layout_line_reorder    (PangoLayoutLine *line);

static void pango_layout_get_item_properties (PangoItem      *item,
					      PangoUnderline *uline);

/**
 * pango_layout_new:
 * @context: a #PangoContext
 * 
 * Create a new #PangoLayout object with attributes initialized to
 * default values for a particular #PangoContext.
 * 
 * Return value: a new #PangoLayout, with a reference count of one.
 **/
PangoLayout *
pango_layout_new (PangoContext *context)
{
  PangoLayout *layout;

  g_return_val_if_fail (context != NULL, NULL);

  layout = g_new (PangoLayout, 1);

  layout->ref_count = 1;

  layout->context = context;
  pango_context_ref (context);
  
  layout->attrs = NULL;
  layout->text = NULL;
  layout->length = 0;
  layout->width = -1;
  layout->indent = 0;

  layout->alignment = PANGO_ALIGN_LEFT;
  layout->justify = FALSE;

  layout->log_attrs = NULL;
  layout->lines = NULL;

  return layout;
}

/**
 * pango_layout_ref:
 * @layout: a #PangoLayout
 * 
 * Increase the reference count of the #PangoLayout by one.
 **/
void
pango_layout_ref (PangoLayout *layout)
{
  g_return_if_fail (layout != NULL);
  
  layout->ref_count++;
}

/**
 * pango_layout_unref:
 * @layout: 
 * 
 * Decrease the reference count of the #PangoLayout by one. If the
 * result is zero, free the #PangoLayout and all associated memory.
 **/
void
pango_layout_unref (PangoLayout *layout)
{
  g_return_if_fail (layout != NULL);
  g_return_if_fail (layout->ref_count > 0);

  layout->ref_count--;
  if (layout->ref_count == 0)
    {
      pango_layout_clear_lines (layout);

      if (layout->context)
	pango_context_unref (layout->context);
	
      if (layout->attrs)
	pango_attr_list_unref (layout->attrs);
      if (layout->text)
	g_free (layout->text);
    }
}

/**
 * pango_layout_get_context:
 * @layout: a #PangoLayout
 * 
 * Retrieves the #PangoContext used for this layout.
 * 
 * Return value: The #PangoContext for the layout. This does not
 * have an additional refcount added, so if you want to keep
 * a copy of this around, you must reference it yourself.
 **/
PangoContext *
pango_layout_get_context (PangoLayout *layout)
{
  g_return_if_fail (layout != NULL);

  return layout->context;
}

/**
 * pango_layout_set_width:
 * @layout: a #PangoLayout.
 * @width: the desired width, or -1 to indicate that no wrapping should be
 *         performed.
 * 
 * Sets the width to which the lines of the #PangoLayout should be wrapped.
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
 * Gets the width to which the lines of the #PangoLayout should be wrapped.
 * 
 * Return value: the width
 **/
int
pango_layout_get_width (PangoLayout    *layout)
{
  g_return_val_if_fail (layout != NULL, 0);
  return layout->width;
}

/**
 * pango_layout_set_indent
 * @layout: a #PangoLayout.
 * @indent: the amount by which to indent
 * 
 * Sets the amount by which the first line should be shorter than the
 * rest of the lines. This may be negative, in which case
 * the subsequent lines will be shorter than the first line. (However,
 * in either case, the entire width of the layout will be given by
 * the value 
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
 * Gets the amount by which the first line should be shorter than the
 * rest of the lines. 
 * 
 * Return value: the indent
 **/
int
pango_layout_get_indent (PangoLayout *layout)
{
  g_return_val_if_fail (layout != NULL, 0);
  return layout->indent;
}

/**
 * pango_layout_set_attributes:
 * @layout: a #PangoLayout
 * @attrs: a #PangoAttrList
 * 
 * Sets the text attributes for a layout object
 **/
void
pango_layout_set_attributes (PangoLayout   *layout,
			     PangoAttrList *attrs)
{
  g_return_if_fail (layout != NULL);

  if (layout->attrs)
    pango_attr_list_unref (layout->attrs);

  layout->attrs = attrs;
  pango_attr_list_ref (layout->attrs);
  pango_layout_clear_lines (layout);
}

/**
 * pango_layout_set_justify:
 * @layout: a #PangoLayout
 * @justify: whether the lines in the layout should be justified.
 * 
 * Sets whether or not each complete line should be stretched to
 * fill the entire width of the layout. This stretching is typically
 * done by adding whitespace, but for some scripts (such as Arabic),
 * the justification is done by extending the characters.
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
 * Gets whether or not each complete line should be stretched to
 * fill the entire width of the layout.
 * 
 * Return value: the justify
 **/
gboolean
pango_layout_get_justify (PangoLayout *layout)
{
  g_return_val_if_fail (layout != NULL, FALSE);
  return layout->justify;
}

/**
 * pango_layout_set_alignment:
 * @layout: a #PangoLayout
 * @alignment: the new alignment
 * 
 * Sets the alignment for the layout (how partial lines are
 * positioned within the horizontal space available.)
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
 * Sets the alignment for the layout (how partial lines are
 * positioned within the horizontal space available.)
 * 
 * Return value: the alignment value
 **/
PangoAlignment
pango_layout_get_alignment (PangoLayout *layout)
{
  g_return_val_if_fail (layout != NULL, PANGO_ALIGN_LEFT);
  return layout->alignment;
}

/**
 * pango_layout_set_text:
 * @layout: a #PangoLayout
 * @text: a UTF8-string
 * @length: the length of @text, in bytes.
 * 
 * Set the text of the layout.
 **/
void
pango_layout_set_text (PangoLayout *layout,
		       char        *text,
		       int          length)
{
  g_return_if_fail (layout != NULL);
  g_return_if_fail (length == 0 || text != NULL);
  
  if (layout->text)
    g_free (layout->text);

  if (length > 0)
    {
      int n_chars = unicode_strlen (text, length);
      unicode_char_t junk;
      char *p = text;
      int i;
      
      for (i=0; i<n_chars; i++)
	{
	  p = unicode_get_utf8 (p, &junk);
	  if (!p || !junk)
	    {
	      g_warning ("Invalid UTF8 string passed to pango_layout_set_text()");
	      return;
	    }
	}
      
      /* NULL-terminate the text, since we currently use unicode_next_utf8()
       * in quite a few places, and for convenience.
       */
      
      layout->text = g_malloc (length + 1);
      memcpy (layout->text, text, length);
      layout->text[length] = '\0';

      layout->n_chars = n_chars;
    }
  else
    {
      layout->text = NULL;
      layout->n_chars = 0;
    }
  
  layout->length = length;

  pango_layout_clear_lines (layout);
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
}

/**
 * pango_layout_get_log_attrs:
 * @layout: a #PangoLayout
 * @attrs: location to store a pointer to an array of logical attributes
 *         This value must be freed with g_free().
 * @n_attrs: location to store the number of the attributes in the
 *           array. (The stored value will be equal to the total number
 *           of characters in the layout.)
 * 
 * Retrieve an array of logical attributes for each character in
 * the layout. 
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
      *attrs = g_new (PangoLogAttr, layout->n_chars);
      memcpy (*attrs, layout->log_attrs, sizeof(PangoLogAttr) * layout->n_chars);
    }
  
  if (n_attrs)
    *n_attrs = layout->n_chars;
}


/**
 * pango_layout_get_line_count:
 * @layout: #PangoLayout
 * 
 * Retrieve the count of lines for the #PangoLayout
 * 
 * Return value: the line count
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
 * Return the lines of the layout as a list
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
 *        pango_layout_get_line_count(layout) - 1, inclusive.
 * 
 * Retrieves a particular line from a #PangoLayout
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
  g_return_val_if_fail (layout != NULL, NULL);
  g_return_val_if_fail (line >= 0, NULL);

  if (line < 0)
    return NULL;

  pango_layout_check_lines (layout);
  return g_slist_nth (layout->lines, line)->data;
}

/**
 * pango_layout_line_index_to_x:
 * @line:      a #PangoLayoutLine
 * @index:     byte offset within the Layout's text 
 * @trailing:  integer indicating where in the cluster the user clicked.
 *             If the script allows positioning within the cluster, it is either
 *             0 or 1; otherwise it is either 0 or the number
 *             of characters in the cluster. In either case
 *             0 represents the trailing edge of the cluster.
 * @x_pos:     location to store the x_offset (in thousandths of a device unit)
 * 
 * Convert index within a line to X pos
 *
 *
 **/
void
pango_layout_line_index_to_x (PangoLayoutLine  *line,
			      int               index,
			      gboolean          trailing,
			      int              *x_pos)
{
  GSList *run_list = line->runs;
  int width = 0;

  while (run_list)
    {
      PangoRectangle logical_rect;
      PangoLayoutRun *run = run_list->data;
      
      if (run->item->offset <= index && run->item->offset + run->item->length > index)
	{
	  pango_glyph_string_index_to_x (run->glyphs,
					 line->layout->text + run->item->offset,
					 run->item->length,
					 &run->item->analysis,
					 index - run->item->offset, trailing, x_pos);
	  
	  if (x_pos)
	    *x_pos += width;
	  
	  return;
	}
      
      pango_glyph_string_extents (run->glyphs, run->item->analysis.font,
				  NULL, &logical_rect);
      width += logical_rect.width;

      run_list = run_list->next;
    }
}
	  
/**
 * pango_layout_index_to_line_x:
 * @layout:    a #PangoLayout
 * @index:     the byte index of the character to find
 * @trailing:  whether we should compute the result for the beginning
 *             or end of the character (or cluster - the decision
 *             for which may be script dependent).
 * @line:      location to store resulting line index. (which will
 *             between 0 and pango_layout_get_line_count(layout) - 1)
 * @x_pos:     location to store resulting position within line
 *             (in thousandths of a device unit)
 *
 * Converts from byte index within the layout to line and X position.
 * (X position is measured from the left edge of the line)
 */
void
pango_layout_index_to_line_x (PangoLayout *layout,
			      int          index,
			      gboolean     trailing,
			      int         *line,
			      int         *x_pos)
{
  GSList *tmp_list;
  int tmp_line = 0;
  int bytes_seen = 0;

  g_return_if_fail (layout != NULL);

  pango_layout_check_lines (layout);

  tmp_list = layout->lines;
  while (tmp_list)
    {
      PangoLayoutLine *layout_line = tmp_list->data;

      if (bytes_seen + layout_line->length > index)
	{
	  if (line)
	    *line = tmp_line;

	  pango_layout_line_index_to_x (layout_line, index, trailing, x_pos);
	  return;
	}

      tmp_list = tmp_list->next;
      bytes_seen += layout_line->length;
    }

  if (line)
    *line = -1;
  if (x_pos)
    *x_pos = -1;
}

/**
 * pango_layout_xy_to_index:
 * @layout:    a #PangoLayout
 * @x:         the X offset (in thousandths of a device unit)
 *             from the left edge of the layout.
 * @y:         the Y offset (in thousandths of a device unit)
 *             from the top edge of the layout
 * @index:     location to store calculated byte index
 * @trailing:  location to store a integer indicating where
 *             in the cluster the user clicked. If the script
 *             allows positioning within the cluster, it is either
 *             0 or 1; otherwise it is either 0 or the number
 *             of characters in the cluster. In either case
 *             0 represents the trailing edge of the cluster.
 *
 * Convert from X and Y position within a layout to the byte 
 * offset to the character at that logical position.
 * 
 * Return value: TRUE if the position was within the layout
 **/
gboolean
pango_layout_xy_to_index (PangoLayout *layout,
			  int          x,
			  int          y,
			  int         *index,
			  gboolean    *trailing)
{
  GSList *line_list;
  PangoRectangle logical_rect;
  int y_offset = 0;
  int width;
	  
  g_return_val_if_fail (layout != NULL, FALSE);

  pango_layout_check_lines (layout);

  width = layout->width;
  if (width == -1 && layout->alignment != PANGO_ALIGN_LEFT)
    {
      pango_layout_get_extents (layout, NULL, &logical_rect);
      width = logical_rect.width;
    }
  
  line_list = layout->lines;
  while (line_list)
    {
      PangoLayoutLine *line = line_list->data;

      pango_layout_line_get_extents (line, NULL, &logical_rect);
	      
      if (y_offset + logical_rect.height >= y)
	{
	  int x_offset;
	    
	  if (layout->alignment == PANGO_ALIGN_RIGHT)
	    x_offset = width - logical_rect.width;
	  else if (layout->alignment == PANGO_ALIGN_CENTER)
	    x_offset = (width - logical_rect.width) / 2;
	  else
	    x_offset = 0;
	  
	  return pango_layout_line_x_to_index (line, x - x_offset, index, trailing);
	}

      y_offset += logical_rect.height;
      line_list = line_list->next;
    }

  return FALSE;
}

/**
 * pango_layout_index_to_pos:
 * @layout: a #PangoLayout
 * @index: byte index within @layout
 * @pos: rectangle in which to store the position of the character
 * 
 * Convert from an index within a PangoLayout to the onscreen position
 * corresponding to that character, which is represented as rectangle.
 * Note that pos->x is always the leading edge of the character. If the
 * and pos->x + pos->width the trailing edge of the character. If the
 * directionality of the character is right-to-left, then pos->width
 * will be negative.
 **/
void
pango_layout_index_to_pos (PangoLayout    *layout,
			   int             index,
			   PangoRectangle *pos)
{
  PangoRectangle logical_rect;
  GSList *tmp_list;
  int bytes_seen = 0;
  int width;
  
  g_return_if_fail (layout != NULL);
  g_return_if_fail (index >= 0 && index < layout->length);

  pos->y = 0;
  
  pango_layout_check_lines (layout);

  width = layout->width;
  if (width == -1 && layout->alignment != PANGO_ALIGN_LEFT)
    {
      pango_layout_get_extents (layout, NULL, &logical_rect);
      width = logical_rect.width;
    }
  
  tmp_list = layout->lines;
  while (tmp_list)
    {
      PangoLayoutLine *layout_line = tmp_list->data;

      pango_layout_line_get_extents (layout_line, NULL, &logical_rect);

      if (bytes_seen + layout_line->length > index)
	{
	  int x_pos;
	  int x_offset;

	  if (layout->alignment == PANGO_ALIGN_RIGHT)
	    x_offset = width - logical_rect.width;
	  else if (layout->alignment == PANGO_ALIGN_CENTER)
	    x_offset = (width - logical_rect.width) / 2;
	  else
	    x_offset = 0;
	  
	  pos->height = logical_rect.height;

	  pango_layout_line_index_to_x (layout_line, index, FALSE, &x_pos);
	  pos->x = x_pos;
	  
	  pango_layout_line_index_to_x (layout_line, index, TRUE, &x_pos);
	  pos->width = x_pos - pos->x;

	  pos->x += x_offset;
	  
	  return;
	}

      tmp_list = tmp_list->next;
      bytes_seen += layout_line->length;
      pos->y += logical_rect.height;
    }
}

/**
 * pango_layout_get_extents:
 * @layout:   a #PangoLayout
 * @ink_rect: rectangle used to store the extents of the glyph string as drawn
 *            or %NULL to indicate that the result is not needed.
 * @logical_rect: rectangle used to store the logical extents of the glyph string
 *            or %NULL to indicate that the result is not needed.
 * 
 * Compute the logical and ink extents of a layout line. See the documentation
 * for pango_font_get_glyph_extents() for details about the interpretation
 * of the rectangles.
 */
void
pango_layout_get_extents (PangoLayout    *layout,
			  PangoRectangle *ink_rect,
			  PangoRectangle *logical_rect)
{
  GSList *line_list;
  int y_offset = 0;
  int width;
	  
  g_return_if_fail (layout != NULL);

  pango_layout_check_lines (layout);

  width = layout->width;
  if (width == -1 && layout->alignment != PANGO_ALIGN_LEFT && ink_rect != NULL)
    {
      PangoRectangle overall_logical;
      
      pango_layout_get_extents (layout, NULL, &overall_logical);
      width = overall_logical.width;
    }
  
  line_list = layout->lines;
  while (line_list)
    {
      PangoLayoutLine *line = line_list->data;
      PangoRectangle line_ink;
      PangoRectangle line_logical;

      int x_offset;
      int new_pos;
	      
      pango_layout_line_get_extents (line, ink_rect ? &line_ink : NULL, &line_logical);

      if (layout->alignment == PANGO_ALIGN_RIGHT)
	x_offset = width - line_logical.width;
      else if (layout->alignment == PANGO_ALIGN_CENTER)
	x_offset = (width - line_logical.width) / 2;
      else
	x_offset = 0;
	  
      if (ink_rect)
	{
	  if (line_list == layout->lines)
	    {
	      ink_rect->x = line_ink.x + x_offset;
	      ink_rect->y = line_ink.y;
	      ink_rect->width = line_ink.width;
	      ink_rect->height = line_ink.height;
	    }
	  else
	    {
	      new_pos = MIN (ink_rect->x, line_ink.x + x_offset);
	      ink_rect->width = MAX (ink_rect->x + ink_rect->width,
				     line_ink.x + line_ink.width + x_offset) - new_pos;
	      ink_rect->x = new_pos;

	      new_pos = MIN (ink_rect->y, line_ink.y + y_offset);
	      ink_rect->height = MAX (ink_rect->y + ink_rect->height,
				      line_ink.y + line_ink.height + y_offset) - new_pos;
	      ink_rect->y = new_pos;
	    }
	}

      if (logical_rect)
	{
	  if (line_list == layout->lines)
	    {
	      logical_rect->x = line_logical.x;
	      logical_rect->y = line_logical.y;
	      logical_rect->width = line_logical.width;
	      logical_rect->height = line_logical.height;
	    }
	  else
	    {
	      new_pos = MIN (logical_rect->x, line_logical.x);
	      logical_rect->width = MAX (logical_rect->x + logical_rect->width,
					 line_logical.x + line_logical.width) - new_pos;
	      logical_rect->x = new_pos;

	      new_pos = MIN (logical_rect->y, line_logical.y + y_offset);
	      logical_rect->height = MAX (logical_rect->y + logical_rect->height,
					  line_logical.y + line_logical.height + y_offset) - new_pos;
	      logical_rect->y = new_pos;
	    }
	}
      
      y_offset += line_logical.height;
      line_list = line_list->next;
    }
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

/*****************
 * Line Breaking *
 *****************/

static void
insert_run (PangoLayoutLine *line, PangoItem *item, PangoGlyphString *glyphs)
{
  PangoLayoutRun *run = g_new (PangoLayoutRun, 1);

  run->item = item;
  run->glyphs = glyphs;

  line->runs = g_slist_prepend (line->runs, run);
  line->length += item->length;
}

static gboolean
process_item (PangoLayoutLine *line,
	      PangoItem       *item,
	      const char      *text,
	      PangoLogAttr    *log_attrs,
	      int             *remaining_width)
{
  PangoGlyphString *glyphs = pango_glyph_string_new ();
  PangoRectangle logical_rect;
  int width;

  if (*remaining_width == 0)
    return FALSE;
  
  pango_shape (text + item->offset, item->length, &item->analysis, glyphs);
  if (*remaining_width < 0)
    {
      insert_run (line, item, glyphs);
      return TRUE;
    }
  
  pango_glyph_string_extents (glyphs, item->analysis.font, NULL, &logical_rect);
  width = logical_rect.width;
  
  if (logical_rect.width < *remaining_width)
    {
      *remaining_width -= width;
      insert_run (line, item, glyphs);

      return TRUE;
    }
  else
    {
      int length;
      int num_chars = item->num_chars;
      int new_width;
      
      PangoGlyphUnit *log_widths = g_new (PangoGlyphUnit, item->num_chars);

      pango_glyph_string_get_logical_widths (glyphs, text + item->offset, item->length, item->analysis.level, log_widths);
      
      new_width = 0;
      while (--num_chars > 0)
	{
	  /* Shorten the item by one line break
	   */
	  width -= log_widths[num_chars];
	  if (log_attrs[num_chars].is_break && width <= *remaining_width)
	    break;
	}

      g_free (log_widths);
      
      if (num_chars != 0)	/* Succesfully broke the item */
	{
	  const char *p;
	  gint n;

	  PangoItem *new_item = pango_item_copy (item);

	  /* Determine utf8 length corresponding to num_chars. Slow?
	   */
	  n = num_chars;
	  p = text + item->offset;
	  while (n-- > 0)
	    p = unicode_next_utf8 (p);
	  
	  length = p - (text + item->offset);
	  
	  new_item->length = length;
	  new_item->num_chars = num_chars;
	  
	  item->offset += length;
	  item->length -= length;
	  item->num_chars -= num_chars;

	  pango_shape (text + new_item->offset, new_item->length, &new_item->analysis, glyphs);

	  *remaining_width -= width;
	  insert_run (line, new_item, glyphs);

	  return FALSE;
	}
      else
	{
	  if (!line->runs)	/* Only item, insert it anyways */
	    {
	      *remaining_width = 0;
	      insert_run (line, item, glyphs);
	      
	      return TRUE;
	    }
	  else
	    {
	      pango_glyph_string_free (glyphs);
	      return FALSE;
	    }
	}
    }
}

static void
get_para_log_attrs (const char   *text,
		    GList        *items,
		    PangoLogAttr *log_attrs)
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
	  if (next_item->analysis.level != tmp_item.analysis.level ||
	      (next_item->analysis.lang_engine != tmp_item.analysis.lang_engine &&
	       (!next_item->analysis.lang_engine || !tmp_item.analysis.lang_engine ||
		strcmp (next_item->analysis.lang_engine->engine.id,
			tmp_item.analysis.lang_engine->engine.id) != 0)))
	    break;
	  else
	    {
	      tmp_item.length += next_item->length;
	      tmp_item.num_chars += next_item->num_chars;
	    }

	  items = items->next;
	}

      pango_break (text + index, tmp_item.length, &tmp_item.analysis, log_attrs + offset);

      offset += tmp_item.num_chars;
      index += tmp_item.length;
      
      items = items->next;
    }
}

static void
pango_layout_check_lines (PangoLayout *layout)
{
  GList *items, *tmp_list;
  const char *start;
  gboolean done = FALSE;
  int para_chars, start_offset;

  PangoLayoutLine *line;
  int remaining_width;

  if (layout->lines)
    return;

  g_assert (!layout->log_attrs);

  layout->log_attrs = g_new (PangoLogAttr, layout->n_chars);
  
  start_offset = 0;
  start = layout->text;
  do
    {
      const char *end = start;

      para_chars = 0;
      while (end != layout->text + layout->length && *end != '\n')
	{
	  end = unicode_next_utf8 (end);
	  para_chars++;
	}
      
      if (end == layout->text + layout->length)
	done = TRUE;
  
      /* FIXME, should we force people to set the attrs? */
      if (layout->attrs)
	items = pango_itemize (layout->context, start, end - start, layout->attrs);
      else
	{
	  PangoAttrList *attrs = pango_attr_list_new ();
	  items = pango_itemize (layout->context, start, end - start, attrs);
	  pango_attr_list_unref (attrs);
	}

      get_para_log_attrs (start, items, layout->log_attrs + start_offset);

      line = pango_layout_line_new (layout);
      remaining_width = (layout->indent >= 0) ? layout->width - layout->indent : layout->indent;

      tmp_list = items;
      while (tmp_list)
	{
	  PangoItem *item = tmp_list->data;
	  gboolean fits;
	  int old_num_chars = item->num_chars;
	  
	  fits = process_item (line, item, start, layout->log_attrs + start_offset, &remaining_width);
	  
	  if (fits)
	    {
	      tmp_list = tmp_list->next;
	      start_offset += old_num_chars;
	    }
	  
	  if (!fits)
	    {
	      /* Complete line
	       */
	      line->runs = g_slist_reverse (line->runs);
	      pango_layout_line_reorder (line);
	      
	      layout->lines = g_slist_prepend (layout->lines, line);
	      
	      line = pango_layout_line_new (layout);
	      remaining_width = (layout->indent >= 0) ? layout->width : layout->indent + layout->indent;

	      start_offset += old_num_chars - item->num_chars;
	    }
	}

      line->runs = g_slist_reverse (line->runs);
      pango_layout_line_reorder (line);
      
      layout->lines = g_slist_prepend (layout->lines, line);

      g_list_free (tmp_list);

      if (!done)
	{
	  /* Handle newline */
	  layout->log_attrs[start_offset].is_break = TRUE;
	  layout->log_attrs[start_offset].is_white = TRUE;
	  layout->log_attrs[start_offset].is_char_stop = TRUE;
	  layout->log_attrs[start_offset].is_word_stop = TRUE;
	  start_offset += 1;

	  start = end + 1;
	}
    }
  while (!done);
  
  layout->lines = g_slist_reverse (layout->lines);
}

/**
 * pango_layout_line_ref:
 * @line: a #PangoLayoutLine
 * 
 * Increase the reference count of a #PangoLayoutLine by one.
 **/
void
pango_layout_line_ref (PangoLayoutLine *line)
{
  PangoLayoutLinePrivate *private = (PangoLayoutLinePrivate *)line;
  
  g_return_if_fail (line != NULL);

  private->ref_count++;
}

/**
 * pango_layout_line_unref:
 * @line: a #PangoLayoutLine
 * 
 * Decrease the reference count of a #PangoLayoutLine by one.
 * if the result is zero, the line and all associated memory
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
      GSList *tmp_list = line->runs;

      while (tmp_list)
	{
	  PangoLayoutRun *run = tmp_list->data;

	  pango_font_unref (run->item->analysis.font);
	  g_free (run->item);

	  pango_glyph_string_free (run->glyphs);
	  
	  tmp_list = tmp_list->next;
	}

      g_slist_free (line->runs);
      g_free (line);
    }
}

/**
 * pango_layout_line_x_to_index:
 * @line:      a #PangoLayoutLine
 * @x_pos:     the x offset (in thousands of a device unit)
 *             from the left edge of the line.
 * @index:     location to store calculated byte offset.
 * @trailing:  location to store a integer indicating where
 *             in the cluster the user clicked. If the script
 *             allows positioning within the cluster, it is either
 *             0 or 1; otherwise it is either 0 or the number
 *             of characters in the cluster. In either case
 *             0 represents the trailing edge of the cluster.
 *
 * Convert from x offset to the byte index of the corresponding
 * character within the text of the layout.
 *
 * Return value: %TRUE if the x index was within the line
 */
gboolean
pango_layout_line_x_to_index (PangoLayoutLine *line,
			      int              x_pos,
			      int             *index,
			      int             *trailing)
{
  GSList *tmp_list;
  gint start_pos = 0;

  g_return_val_if_fail (LINE_IS_VALID (line), FALSE);

  if (!LINE_IS_VALID (line))
    return FALSE;

  tmp_list = line->runs;
  while (tmp_list)
    {
      PangoRectangle logical_rect;
      PangoLayoutRun *run = tmp_list->data;
      
      pango_glyph_string_extents (run->glyphs, run->item->analysis.font, NULL, &logical_rect);

      if (x_pos >= start_pos && x_pos < start_pos + logical_rect.width)
	{
	  int pos;
	  
	  pango_glyph_string_x_to_index (run->glyphs,
					 line->layout->text + run->item->offset, run->item->length,
					 &run->item->analysis,
					 x_pos - start_pos,
					 &pos, trailing);

	  if (index)
	    *index = pos + run->item->offset;
	  
	  return TRUE;
	}

      start_pos += logical_rect.width;
      tmp_list = tmp_list->next;
    }

  return FALSE;
}

/**
 * pango_layout_line_get_extents:
 * @line:     a #PangoLayoutLine
 * @ink_rect: rectangle used to store the extents of the glyph string as drawn
 *            or %NULL to indicate that the result is not needed.
 * @logical_rect: rectangle used to store the logical extents of the glyph string
 *            or %NULL to indicate that the result is not needed.
 * 
 * Compute the logical and ink extents of a layout line. See the documentation
 * for pango_font_get_glyph_extents() for details about the interpretation
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
      PangoUnderline uline = PANGO_UNDERLINE_NONE;
      
      PangoRectangle run_ink;
      PangoRectangle run_logical;

      pango_layout_get_item_properties (run->item, &uline);
      pango_glyph_string_extents (run->glyphs, run->item->analysis.font,
				  (ink_rect || uline != PANGO_UNDERLINE_NONE) ? &run_ink : NULL,
				  &run_logical);

      switch (uline)
	{
	case PANGO_UNDERLINE_NONE:
	  break;
	case PANGO_UNDERLINE_SINGLE:
	  if (ink_rect)
	    run_ink.height = MAX (run_ink.height, 2 * PANGO_SCALE - run_ink.y);
	  run_logical.height = MAX (run_logical.height, 2 * PANGO_SCALE - run_logical.y);
	  break;
	case PANGO_UNDERLINE_DOUBLE:
	  if (ink_rect)
	    run_ink.height = MAX (run_ink.height, 4 * PANGO_SCALE - run_ink.y);
	  run_logical.height = MAX (run_logical.height, 4 * PANGO_SCALE - run_logical.y);
	  break;
	case PANGO_UNDERLINE_LOW:
	  if (ink_rect)
	    run_ink.height += 2 * PANGO_SCALE;
	  
	  /* FIXME: Should this simply be run_logical.height += 2 * PANGO_SCALE instead?
	   */
	  run_logical.height = MAX (run_logical.height, run_ink.y + run_ink.height - run_logical.y);
	  break;
	}
       
      if (ink_rect)
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
}

static PangoLayoutLine *
pango_layout_line_new (PangoLayout *layout)
{
  PangoLayoutLinePrivate *private = g_new (PangoLayoutLinePrivate, 1);

  private->ref_count = 1;
  private->line.layout = layout;
  private->line.runs = 0;
  private->line.length = 0;

  return (PangoLayoutLine *) private;
}


/*
 * NB: The contents of the file implement the exact same algorithm
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

/* This utility function is duplicated here and in pango-layout.c; should it be
 * public? Trouble is - what is the appropriate set of properties?
 */
static void
pango_layout_get_item_properties (PangoItem      *item,
				  PangoUnderline *uline)
{
  GSList *tmp_list = item->extra_attrs;

  while (tmp_list)
    {
      PangoAttribute *attr = tmp_list->data;

      switch (attr->klass->type)
	{
	case PANGO_ATTR_UNDERLINE:
	  if (uline)
	    *uline = ((PangoAttrInt *)attr)->value;
	  
	default:
	  break;
	}
      tmp_list = tmp_list->next;
    }
}
