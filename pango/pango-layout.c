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

#define LINE_IS_VALID(line) ((line)->layout != NULL)

typedef struct _PangoLayoutLinePrivate PangoLayoutLinePrivate;

struct _PangoLayoutLinePrivate
{
  PangoLayoutLine line;
  guint ref_count;
};

struct _PangoLayout
{
  GObject parent_instance;

  PangoContext *context;
  PangoAttrList *attrs;
  PangoFontDescription *font_desc;
  
  gchar *text;
  int length;			/* length of text in bytes */
  int width;			/* wrap width, in device units */
  int indent;			/* amount by which first line should be shorter */
  int spacing;			/* spacing between lines */

  guint justify : 1;
  guint alignment : 2;

  gint n_chars;		        /* Total number of characters in layout */
  PangoLogAttr *log_attrs;	/* Logical attributes for layout's text */

  int tab_width;		/* Cached width of a tab. -1 == not yet calculated */

  GSList *lines;
};

struct _PangoLayoutClass
{
  GObjectClass parent_class;


};

static void pango_layout_clear_lines (PangoLayout *layout);
static void pango_layout_check_lines (PangoLayout *layout);

static PangoLayoutLine * pango_layout_line_new         (PangoLayout     *layout);
static void              pango_layout_line_postprocess (PangoLayoutLine *line);

static int *pango_layout_line_get_log2vis_map (PangoLayoutLine  *line,
					       gboolean          strong);
static int *pango_layout_line_get_vis2log_map (PangoLayoutLine  *line,
					       gboolean          strong);

static void pango_layout_get_item_properties (PangoItem      *item,
					      PangoUnderline *uline);

static void pango_layout_init        (PangoLayout      *layout);
static void pango_layout_class_init  (PangoLayoutClass *klass);
static void pango_layout_finalize    (GObject          *object);

static gpointer parent_class;

GType
pango_layout_get_type (void)
{
  static GType object_type = 0;

  if (!object_type)
    {
      static const GTypeInfo object_info =
      {
        sizeof (PangoLayoutClass),
        (GBaseInitFunc) NULL,
        (GBaseFinalizeFunc) NULL,
        (GClassInitFunc) pango_layout_class_init,
        NULL,           /* class_finalize */
        NULL,           /* class_data */
        sizeof (PangoLayout),
        0,              /* n_preallocs */
        (GInstanceInitFunc) pango_layout_init,
      };
      
      object_type = g_type_register_static (G_TYPE_OBJECT,
                                            "PangoLayout",
                                            &object_info);
    }
  
  return object_type;
}

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

  layout->log_attrs = NULL;
  layout->lines = NULL;

  layout->tab_width = -1;  
}

static void
pango_layout_class_init (PangoLayoutClass *klass)
{
  GObjectClass *object_class = G_OBJECT_CLASS (klass);
  
  parent_class = g_type_class_peek_parent (klass);
  
  object_class->finalize = pango_layout_finalize;
}

static void
pango_layout_finalize (GObject *object)
{
  PangoLayout *layout;

  layout = PANGO_LAYOUT (object);

  pango_layout_clear_lines (layout);
  
  if (layout->context)
    g_object_unref (G_OBJECT (layout->context));
  
  if (layout->attrs)
    pango_attr_list_unref (layout->attrs);
  if (layout->text)
    g_free (layout->text);
  
  G_OBJECT_CLASS (parent_class)->finalize (object);
}


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

  layout = PANGO_LAYOUT (g_type_create_instance (pango_layout_get_type ()));

  layout->context = context;
  g_object_ref (G_OBJECT (context));

  return layout;
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
  g_return_val_if_fail (layout != NULL, NULL);

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
 * pango_layout_set_indent
 * @layout: a #PangoLayout.
 * @indent: the amount of spacing
 * 
 * Sets the amount of spacing between the lines of the layout.
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
 * Gets the amount of spacing between the lines of the layout.
 * 
 * Return value: the spacing (in thousandths of a device unit)
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
 * pango_layout_set_font_description:
 * @layout: a #PangoLayout
 * @desc: the new pango font description, or %NULL to unset the
 *        current font description.
 * 
 * Set the default font description for the layout. If no font 
 * description is set on the layout, the font description from
 * the layout's context is used.
 **/
void
pango_layout_set_font_description (PangoLayout                 *layout,
				    const PangoFontDescription *desc)
{
  g_return_if_fail (layout != NULL);
  g_return_if_fail (desc != NULL);

  if (layout->font_desc)
    pango_font_description_free (layout->font_desc);
  
  if (desc)
    layout->font_desc = pango_font_description_copy (desc);
  else
    layout->font_desc = NULL;

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
 * @length: the length of @text, in bytes. -1 indicates that
 *          the string is null terminated and the length should be
 *          calculated.
 * 
 * Set the text of the layout.
 **/
void
pango_layout_set_text (PangoLayout *layout,
		       const char  *text,
		       int          length)
{
  g_return_if_fail (layout != NULL);
  g_return_if_fail (length == 0 || text != NULL);
  
  if (layout->text)
    g_free (layout->text);

  if (length == 0)
    {
      layout->text = g_strdup ("");
      layout->n_chars = 0;
    }
  else
    {
      const char *p = text;
      int n_chars = 0;
      
      while (*p && (length < 0 || p < text + length))
	{
	  if (g_utf8_get_char (p) == (gunichar)-1)
	    {
	      g_warning ("Invalid UTF8 string passed to pango_layout_set_text()");
	      return;
	    }
	  p = g_utf8_next_char (p);
	  n_chars++;
	}

      if (length < 0)
	length = p - text;

      if (length >= 0 && p != text + length)
	g_warning ("string passed to pango_layout_set_text() contains embedded NULL");

      length = p - text;
      
      /* NULL-terminate the text for convenience.
       */
      
      layout->text = g_malloc (length + 1);
      memcpy (layout->text, text, length);
      layout->text[length] = '\0';

      layout->n_chars = n_chars;
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
  layout->tab_width = -1;
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
 * pango_layout_move_cursor_visually:
 * @layout:       a #PangoLayout.
 * @old_index:    the old cursor byte index
 * @old_trailing: 
 * @direction:    direction to move cursor. A negative
 *                value indicates motion to the left.
 * @new_index:    location to store the new cursor byte index. A value of -1 
 *                indicates that the cursor has been moved off the beginning
 *                of the layout. A value of G_MAXINT indicates that
 *                the cursor has been moved off the end of the layout.
 * @new_trailing: 
 * 
 * Computes a new cursor position from an old position and
 * a count of positions to move visually. If @count is positive,
 * then the new strong cursor position will be one position
 * to the right of the old cursor position. If @count is position
 * then the new strong cursor position will be one position
 * to the left of the old cursor position. 
 *
 * In the presence of bidirection text, the correspondence
 * between logical and visual order will depend on the direction
 * of the current run, and there may be jumps when the cursor
 * is moved off of the end of a run.
 **/
void
pango_layout_move_cursor_visually (PangoLayout *layout,
				   int          old_index,
				   int          old_trailing,
				   int          direction,
				   int         *new_index,
				   int         *new_trailing)
{
  int bytes_seen = 0;
  PangoDirection base_dir;
  PangoLayoutLine *line = NULL;
  PangoLayoutLine *prev_line = NULL;
  PangoLayoutLine *next_line;
  GSList *tmp_list;

  int *log2vis_map;
  int *vis2log_map;
  int n_vis;
  int vis_pos;

  g_return_if_fail (layout != NULL);
  g_return_if_fail (old_index >= 0 && old_index <= layout->length);
  g_return_if_fail (old_index < layout->length || old_trailing == 0);
  g_return_if_fail (new_index != NULL);
  g_return_if_fail (new_trailing != NULL);

  pango_layout_check_lines (layout);

  base_dir = pango_context_get_base_dir (layout->context);

  /* Find the line the old cursor is on
   */
  tmp_list = layout->lines;
  while (tmp_list)
    {
      line = tmp_list->data;

      if (bytes_seen + line->length > old_index || !tmp_list->next)
	break;

      tmp_list = tmp_list->next;
      prev_line = line;
      bytes_seen += line->length;
    }

  if (tmp_list->next)
    next_line = tmp_list->next->data;
  else
    next_line = NULL;

  if (old_trailing)
    old_index = g_utf8_next_char (layout->text + old_index) - layout->text;

  log2vis_map = pango_layout_line_get_log2vis_map (line, TRUE);
  n_vis = g_utf8_strlen (layout->text + bytes_seen, line->length);

  vis_pos = log2vis_map[old_index - bytes_seen];
  g_free (log2vis_map);
  
  if (vis_pos == 0 && direction < 0)
    {
      if (base_dir == PANGO_DIRECTION_LTR)
	{
	  if (!prev_line)
	    {
	      *new_index = -1;
	      *new_trailing = 0;
	      return;
	    }
	  line = prev_line;
	  bytes_seen -= line->length;
	}
      else
	{
	  if (!next_line)
	    {
	      *new_index = G_MAXINT;
	      *new_trailing = 0;
	      return;
	    }
	  bytes_seen += line->length;
	  line = next_line;
	}
      
      vis_pos = g_utf8_strlen (layout->text + bytes_seen, line->length);
    }
  else if (vis_pos == n_vis && direction > 0)
    {
      if (base_dir == PANGO_DIRECTION_LTR)
	{
	  if (!next_line)
	    {
	      *new_index = G_MAXINT;
	      *new_trailing = 0;
	      return;
	    }
	  bytes_seen += line->length;
	  line = next_line;
	}
      else
	{
	  if (!prev_line)
	    {
	      *new_index = -1;
	      *new_trailing = 0;
	      return;
	    }
	  line = prev_line;
	  bytes_seen -= line->length;
	}
      
      vis_pos = 0;
    }

  vis_pos += (direction > 0) ? 1 : -1;
  
  vis2log_map = pango_layout_line_get_vis2log_map (line, TRUE);
  *new_index = bytes_seen + vis2log_map[vis_pos];
  g_free (vis2log_map);

  if (*new_index == bytes_seen + line->length && line->length > 0)
    {
      *new_index = g_utf8_prev_char (layout->text + *new_index) - layout->text;
      *new_trailing = 1;
    }
  else
    *new_trailing = 0;
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
	  
	  pango_layout_line_x_to_index (line, x - x_offset, index, trailing);
	  return TRUE;
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
  PangoDirection base_dir;
  int x_offset;
  
  g_return_if_fail (layout != NULL);
  g_return_if_fail (index >= 0);
  
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

  /* Return a zero-width rectangle on the left or right side of the
   * last line.
   */
  g_assert (index >= layout->length);

  pos->y -= logical_rect.height;
  pos->height = logical_rect.height;
  pos->width = 0;
  
  base_dir = pango_context_get_base_dir (layout->context);

  if (layout->alignment == PANGO_ALIGN_RIGHT)
    x_offset = width - logical_rect.width;
  else if (layout->alignment == PANGO_ALIGN_CENTER)
    x_offset = (width - logical_rect.width) / 2;
  else
    x_offset = 0;
  
  switch (base_dir)
    {
    case PANGO_DIRECTION_LTR:
      pos->x = x_offset + logical_rect.width;
      break;

    case PANGO_DIRECTION_RTL:
      pos->x = x_offset;
      break;

    default:
      g_assert_not_reached ();
      break;
    }
}

static void
pango_layout_line_get_range (PangoLayoutLine *line,
			     char           **start,
			     char           **end)
{
  char *p;
  GSList *tmp_list;
  
  p = line->layout->text;
  tmp_list = line->layout->lines;
  
  while (tmp_list->data != line)
    {
      p += ((PangoLayoutLine *)tmp_list->data)->length;
      tmp_list = tmp_list->next;
    }

  if (start)
    *start = p;
  if (end)
    *end = p + line->length;
}

int *
pango_layout_line_get_vis2log_map (PangoLayoutLine *line,
				   gboolean         strong)
{
  PangoLayout *layout = line->layout;
  PangoDirection base_dir = pango_context_get_base_dir (layout->context);
  PangoDirection prev_dir = base_dir;
  GSList *tmp_list;
  gchar *start, *end;
  int *result;
  int pos = 0;
  int n_chars;

  pango_layout_line_get_range (line, &start, &end);
  n_chars = g_utf8_strlen (start, end - start);
  
  result = g_new (int, n_chars + 1);

  if (strong)
    {
      if (base_dir == PANGO_DIRECTION_LTR)
	{
	  result[0] = 0;
	  result[n_chars] = end - start;
	}
      else
	{
	  result[0] = end - start;
	  result[n_chars] = 0;
	}
    }
  
  tmp_list = line->runs;
  while (tmp_list)
    {
      PangoLayoutRun *run = tmp_list->data;
      int run_n_chars = run->item->num_chars;
      PangoDirection run_dir = (run->item->analysis.level % 2) ? PANGO_DIRECTION_RTL : PANGO_DIRECTION_LTR;
      char *p = layout->text + run->item->offset;
      int i;

      if (run_dir == PANGO_DIRECTION_LTR)
	{
	  if ((strong && base_dir == run_dir) ||
	      (!strong && base_dir != run_dir) ||
	      (prev_dir == run_dir))
	    result[pos] = p - start;

	  p = g_utf8_next_char (p);

	  for (i = 1; i < run_n_chars; i++)
	    {
	      result[pos + i] = p - start;
	      p = g_utf8_next_char (p);
	    }

	  if ((strong && base_dir == run_dir) ||
	      (!strong && base_dir != run_dir))
	    result[pos + run_n_chars] = p - start;
	}
      else
	{
	  if ((strong && base_dir == run_dir) ||
	      (!strong && base_dir != run_dir))
	    result[pos + run_n_chars] = p - start;

	  p = g_utf8_next_char (p);

	  for (i = 1; i < run_n_chars; i++)
	    {
	      result[pos + run_n_chars - i] = p - start;
	      p = g_utf8_next_char (p);
	    }

	  if ((strong && base_dir == run_dir) ||
	      (!strong && base_dir != run_dir) ||
	      (prev_dir == run_dir))
	    result[pos] = p - start;
	}

      pos += run_n_chars;
      prev_dir = run_dir;
      tmp_list = tmp_list->next;
    }

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
 * @index: the byte index of the cursor
 * @strong_pos: location to store the strong cursor position (may be %NULL)
 * @weak_pos: location to store the weak cursor position (may be %NULL)
 * 
 * Given an index within a layout, determine the positions that of the
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
  PangoDirection base_dir;
  PangoDirection dir1, dir2;
  PangoRectangle logical_rect;
  PangoLayoutLine *layout_line = NULL; /* Quiet GCC */
  int x1_trailing;
  int x2;
  GSList *tmp_list;
  int width;
  int x_offset;
  int y_offset = 0;
  int bytes_seen = 0;
  
  g_return_if_fail (layout != NULL);
  g_return_if_fail (index >= 0 && index <= layout->length);
  
  base_dir = pango_context_get_base_dir (layout->context);

  pango_layout_check_lines (layout);

  width = layout->width;
  if (width == -1 && layout->alignment != PANGO_ALIGN_LEFT)
    {
      pango_layout_get_extents (layout, NULL, &logical_rect);
      width = logical_rect.width;
    }
  
  /* Find the line */
  tmp_list = layout->lines;
  while (tmp_list)
    {
      layout_line = tmp_list->data;
      
      pango_layout_line_get_extents (layout_line, NULL, &logical_rect);
      layout_line = tmp_list->data;

      if (bytes_seen + layout_line->length > index)
	break;

      tmp_list = tmp_list->next;

      if (tmp_list)		/* Want last line of layout for trailing position */
	{
	  y_offset += logical_rect.height;
	  bytes_seen += layout_line->length;
	}
    }

  /* Examine the trailing edge of the character before the cursor */
  if (index == bytes_seen)
    {
      dir1 = base_dir;
      x1_trailing = (base_dir == PANGO_DIRECTION_LTR) ? 0 : logical_rect.width;
    }
  else
    {
      gint prev_index = g_utf8_prev_char (layout->text + index) - layout->text;
      dir1 = pango_layout_line_get_char_direction (layout_line, prev_index);
      pango_layout_line_index_to_x (layout_line, prev_index, TRUE, &x1_trailing);
    }

  /* Examine the leading edge of the character after the cursor */
  if (index == bytes_seen + layout_line->length)
    {
      dir2 = base_dir;
      x2 = (base_dir == PANGO_DIRECTION_LTR) ? logical_rect.width : 0;
    }
  else
    {
      dir2 = pango_layout_line_get_char_direction (layout_line, index);
      pango_layout_line_index_to_x (layout_line, index, FALSE, &x2);
    }

  if (layout->alignment == PANGO_ALIGN_RIGHT)
    x_offset = width - logical_rect.width;
  else if (layout->alignment == PANGO_ALIGN_CENTER)
    x_offset = (width - logical_rect.width) / 2;
  else
    x_offset = 0;
	  
  if (strong_pos)
    {
      if (dir1 == base_dir)
	strong_pos->x = x_offset + x1_trailing;
      else
	strong_pos->x = x_offset + x2;

      strong_pos->y = y_offset;
      strong_pos->width = 0;
      strong_pos->height = logical_rect.height;
    }

  if (weak_pos)
    {
      if (dir1 == base_dir)
	weak_pos->x = x_offset + x2;
      else
	weak_pos->x = x_offset + x1_trailing;

      weak_pos->y = y_offset;
      weak_pos->width = 0;
      weak_pos->height = logical_rect.height;
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
 * Compute the logical and ink extents of a layout. See the documentation
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

/**
 * pango_layout_get_pixel_extents:
 * @layout:   a #PangoLayout
 * @ink_rect: rectangle used to store the extents of the glyph string as drawn
 *            or %NULL to indicate that the result is not needed.
 * @logical_rect: rectangle used to store the logical extents of the glyph string
 *            or %NULL to indicate that the result is not needed.
 * 
 * Compute the logical and ink extents of a layout. See the documentation
 * for pango_font_get_glyph_extents() for details about the interpretation
 * of the rectangles. The returned rectangles are in device units, as
 * opposed to pango_layout_get_extents(), which returns the extents in
 * units of device unit / PANGO_SCALE.
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

/**
 * pango_layout_get_size:
 * @layout: a #PangoLayout
 * @width: location to store the logical width, or %NULL
 * @height: location to store the logical height, or %NULL
 * 
 * Determine the logical width and height of a #PangoLayout
 * in Pango units. (device units divided by PANGO_SCALE). This
 * is simply a convenience function around pango_layout_get_extents.
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
 * Determine the logical width and height of a #PangoLayout
 * in device units. (pango_layout_get_size() returns the width
 * and height in thousandths of a device unit.) This
 * is simply a convenience function around pango_layout_get_extents.
 **/
void
pango_layout_get_pixel_size (PangoLayout *layout,
			     int         *width,
			     int         *height)
{
  PangoRectangle logical_rect;

  pango_layout_get_extents (layout, NULL, &logical_rect);

  if (width)
    *width = (logical_rect.width + PANGO_SCALE / 2) / PANGO_SCALE;
  if (height)
    *height = (logical_rect.height + PANGO_SCALE / 2) / PANGO_SCALE;
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

static void
free_run (PangoLayoutRun *run, gboolean free_item)
{
  if (free_item)
    pango_item_free (run->item);

  pango_glyph_string_free (run->glyphs);
  g_free (run);
}

static int
get_tab_pos (PangoLayout *layout, int index)
{
  if (layout->tab_width == -1)
    {
      /* Find out how wide 8 spaces are in the context's default font. Utter
       * performance killer. :-( 
       */
      PangoGlyphString *glyphs = pango_glyph_string_new ();
      PangoItem *item;
      GList *items;
      int i;

      PangoAttrList *attrs = pango_attr_list_new ();
      if (layout->font_desc)
	{
	  PangoAttribute *attr = pango_attr_font_desc_new (layout->font_desc);
	  attr->start_index = 0;
	  attr->end_index = layout->length;
	  
	  pango_attr_list_insert_before (attrs, attr);
	}

      items = pango_itemize (layout->context, " ", 1, attrs);
      pango_attr_list_unref (attrs);

      item = items->data;
      pango_shape ("        ", 8, &item->analysis, glyphs);
      
      pango_item_free (item);
      g_list_free (items);

      layout->tab_width = 0;
      for (i=0; i < glyphs->num_glyphs; i++)
	layout->tab_width += glyphs->glyphs[i].geometry.width;

      pango_glyph_string_free (glyphs);
    }
  
  return layout->tab_width * index;
}

static void
shape_tab (PangoLayoutLine  *line,
	   PangoGlyphString *glyphs)
{
  int i;
  GSList *tmp_list;

  int current_width = 0;

  /* Compute the width of the line currently - inefficient, but easier
   * than keeping the current width of the line up to date everywhere
   */
  tmp_list = line->runs;
  while (tmp_list)
    {
      PangoLayoutRun *run = tmp_list->data;
      for (i=0; i < run->glyphs->num_glyphs; i++)
	current_width += run->glyphs->glyphs[i].geometry.width;
      
      tmp_list = tmp_list->next;
    }
  
  pango_glyph_string_set_size (glyphs, 1);
  
  glyphs->glyphs[0].glyph = 0;
  glyphs->glyphs[0].geometry.x_offset = 0;
  glyphs->glyphs[0].geometry.y_offset = 0;
  glyphs->glyphs[0].attr.is_cluster_start = 1;
  
  glyphs->log_clusters[0] = 0;

  for (i=0;;i++)
    {
      int tab_pos = get_tab_pos (line->layout, i);
      if (tab_pos > current_width)
	{
	  glyphs->glyphs[0].geometry.width = tab_pos - current_width;
	  break;
	}
    }
}

typedef enum 
{
  BREAK_NONE_FIT,
  BREAK_SOME_FIT,
  BREAK_ALL_FIT,
} BreakResult;

static BreakResult
process_item (PangoLayoutLine *line,
	      PangoItem       *item,
	      const char      *text,
	      PangoLogAttr    *log_attrs,
	      gboolean         no_break_at_start,
	      gboolean         no_break_at_end,
	      int             *remaining_width)
{
  PangoGlyphString *glyphs = pango_glyph_string_new ();
  int width;
  int length;
  int i;

  if (text[item->offset] == '\t')
    shape_tab (line, glyphs);
  else
    pango_shape (text + item->offset, item->length, &item->analysis, glyphs);

  if (*remaining_width < 0)	/* Wrapping off */
    {
      insert_run (line, item, glyphs);
      return BREAK_ALL_FIT;
    }

  width =0;
  for (i=0; i < glyphs->num_glyphs; i++)
    width += glyphs->glyphs[i].geometry.width;

  if (width <= *remaining_width && !no_break_at_end)
    {
      *remaining_width -= width;
      insert_run (line, item, glyphs);

      return BREAK_ALL_FIT;
    }
  else
    {
      int num_chars = item->num_chars;
      PangoGlyphUnit *log_widths = g_new (PangoGlyphUnit, item->num_chars);
      pango_glyph_string_get_logical_widths (glyphs, text + item->offset, item->length, item->analysis.level, log_widths);
      
      /* Shorten the item by one line break
       */
      while (--num_chars > 0)
	{
	  width -= log_widths[num_chars];
	  
	  if (log_attrs[num_chars].is_break && width <= *remaining_width)
	    break;
	}

      g_free (log_widths);
      
      if (num_chars > 0)	/* Succesfully broke the item */
	{
	  PangoItem *new_item = pango_item_copy (item);
	  
	  length = g_utf8_offset_to_pointer (text + item->offset, num_chars) - (text + item->offset);
	  
	  new_item->length = length;
	  new_item->num_chars = num_chars;
	  
	  item->offset += length;
	  item->length -= length;
	  item->num_chars -= num_chars;
	  
	  pango_shape (text + new_item->offset, new_item->length, &new_item->analysis, glyphs);
	  
	  *remaining_width -= width;
	  insert_run (line, new_item, glyphs);

	  return BREAK_SOME_FIT;
	}
      else
	{
	  if (no_break_at_start) /* We must insert something */
	    {
	      *remaining_width = 0;
	      insert_run (line, item, glyphs);
	      
	      return BREAK_ALL_FIT;
	    }
	  else
	    {
	      pango_glyph_string_free (glyphs);
	      return BREAK_NONE_FIT;
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
  const char *start;
  gboolean done = FALSE;
  int start_offset;

  if (layout->lines)
    return;

  g_assert (!layout->log_attrs);

  /* For simplicity, we make sure at this point that layout->text
   * is non-NULL even if it is zero length
   */
  if (!layout->text)
    pango_layout_set_text (layout, NULL, 0);
  
  layout->log_attrs = g_new (PangoLogAttr, layout->n_chars);
  
  start_offset = 0;
  start = layout->text;
  do
    {
      PangoLayoutLine *line;
      PangoAttrList *attrs;
      
      GList *items, *tmp_list;
      gboolean last_cant_end = FALSE;
      gboolean current_cant_end = FALSE;
      int remaining_width;
      int last_remaining_width = 0;	/* Quiet GCC */

      const char *end = start;
      int para_chars = 0;

      while (end != layout->text + layout->length && *end != '\n')
	{
	  end = g_utf8_next_char (end);
	  para_chars++;
	}
      
      if (end == layout->text + layout->length)
	done = TRUE;
  
      if (layout->attrs)
	{
	  /* If we were being clever, we'd try to catch the case here
	   * where the set font desc doesn't change the font for any
	   * characters.
	   */
	  if (layout->font_desc)
	    attrs = pango_attr_list_copy (layout->attrs);
	  else
	    attrs = layout->attrs;
	}
      else
	attrs = pango_attr_list_new ();

      if (layout->font_desc)
	{
	  PangoAttribute *attr = pango_attr_font_desc_new (layout->font_desc);
	  attr->start_index = 0;
	  attr->end_index = layout->length;
	  
	  pango_attr_list_insert_before (attrs, attr);
	}

      items = pango_itemize (layout->context, start, end - start, attrs);

      if (attrs != layout->attrs)
	  pango_attr_list_unref (attrs);

      get_para_log_attrs (start, items, layout->log_attrs + start_offset);

      line = pango_layout_line_new (layout);
      remaining_width = (layout->indent >= 0) ? layout->width - layout->indent : layout->width;

      tmp_list = items;
      while (tmp_list)
	{
	  PangoItem *item = tmp_list->data;
	  BreakResult result;
 	  int old_num_chars = item->num_chars;

	  result = process_item (line, item, start,
				 layout->log_attrs + start_offset,
				 (line->runs == NULL) || last_cant_end,
				 current_cant_end,
				 &remaining_width);
	  current_cant_end = FALSE;
	  
	  if (result == BREAK_ALL_FIT)
	    {
	      tmp_list = tmp_list->next;
	      start_offset += old_num_chars;

	      /* We prohibit breaking a line between a non-whitespace char and the whitespace char afterwards
	       * because this would result in the possibility of normal wrapped paragraphs with leading
	       * white-space at the front of lines. We probably should have a mode where we treat all
	       * white-space as zero width - appropriate for typography but not for editing.
	       */
	      if (start_offset < layout->n_chars &&
		  (!layout->log_attrs[start_offset].is_break ||
		   (!layout->log_attrs[start_offset-1].is_white && layout->log_attrs[start_offset].is_white)))
		last_cant_end = TRUE;
	      else
		{
		  last_cant_end = FALSE;
		  last_remaining_width = remaining_width;
		}
	    }
	  else
	    {
	      /* Handle the case where the last item wasn't broken, but ended in a non-break
	       * and we didn't manage to get any of this item in the line. In that case
	       * we need to back up and break the last item.
	       */

	      if (last_cant_end && result == BREAK_NONE_FIT)
		{
		  GSList *tmp_node;
		  
		  /* Back up
		   */
		  tmp_list = tmp_list->prev;
		  item = tmp_list->data;
		  
		  start_offset -= item->num_chars;
		  remaining_width = last_remaining_width;
		  
		  current_cant_end = TRUE;

		  /* Remove last run from line
		   */
		  tmp_node = line->runs;
		  line->runs = tmp_node->next;
		  
		  free_run (tmp_node->data, FALSE);
		  g_slist_free_1 (tmp_node);

		  line->length -= item->length;
		}
	      else
		{
		  start_offset += old_num_chars - item->num_chars;
		  
		  pango_layout_line_postprocess (line);
		  
		  layout->lines = g_slist_prepend (layout->lines, line);
		  
		  line = pango_layout_line_new (layout);
		  remaining_width = (layout->indent >= 0) ? layout->width : layout->indent + layout->indent;
		}
	      
	      last_cant_end = FALSE;
	      last_remaining_width = remaining_width;
	    }
	}

      pango_layout_line_postprocess (line);
      
      layout->lines = g_slist_prepend (layout->lines, line);

      g_list_free (items);

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
	  free_run (tmp_list->data, TRUE);
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
 * character within the text of the layout. If the @x_pos
 * is negative or greater than the length of the line, then
 * then the result will be 0, or the last index in the line
 * depending on the base direction of the layout.
 */
void
pango_layout_line_x_to_index (PangoLayoutLine *line,
			      int              x_pos,
			      int             *index,
			      int             *trailing)
{
  GSList *tmp_list;
  gint start_pos = 0;
  gint first_index = 0, last_index;
  PangoDirection base_dir;
  gboolean last_trailing;

  g_return_if_fail (line != NULL);
  g_return_if_fail (LINE_IS_VALID (line));

  if (!LINE_IS_VALID (line))
    return;

  base_dir = pango_context_get_base_dir (line->layout->context);

  /* Find the last index in the line
   */
  tmp_list = line->layout->lines;
  while (tmp_list->data != line)
    {
      first_index += ((PangoLayoutLine *)tmp_list->data)->length;
      tmp_list = tmp_list->next;
    }

  if (line->length == 0)
    {
      if (index)
	*index = first_index;
      if (trailing)
	*trailing = 0;
      
      return;
    }
  
  if (line->length > 0)
    {
      last_index = first_index + line->length;
      last_index = g_utf8_prev_char (line->layout->text + last_index) - line->layout->text;
    }
  else
    last_index = first_index;	/* FIXME - does this make sense at all? */

  /* This is a HACK. If a program only keeps track if cursor (etc)
   * indices and not the trailing flag, then the trailing index of the
   * last character on a wrapped line is identical to the leading
   * index of the next line. Actually, any program keeping cursor
   * positions with wrapped lines should distinguish leading and
   * trailing cursors.
   */
  last_trailing = tmp_list->next ? 0 : 1;
  
  if (x_pos < 0)
    {
      if (index)
	*index = (base_dir == PANGO_DIRECTION_LTR) ? first_index : last_index;
      if (trailing)
	*trailing = (base_dir == PANGO_DIRECTION_LTR) ? 0 : last_trailing;
      
      return;
    }

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
	  
	  return;
	}

      start_pos += logical_rect.width;
      tmp_list = tmp_list->next;
    }

  if (index)
    *index = (base_dir == PANGO_DIRECTION_LTR) ? last_index : first_index;
  if (trailing)
    *trailing = (base_dir == PANGO_DIRECTION_LTR) ? last_trailing : 0;
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
 * @ranges:      location to store a pointer to an array of arranges.
 *               The array will be of length 2*@n_ranges, with each
 *               range including the pixels from (*ranges)[2*n] to
 *               (*ranges)[2*n + 1] - 1. This array must be freed
 *               with g_free.
 * @n_ranges: The number of ranges stored in @ranges.
 * 
 * Get a list of visual ranges corresponding to a given logical range.
 * This list is not necessarily minimal - there may be consecutive
 * ranges which are adjacent. The ranges will be sorted from left to
 * right.
 **/
void
pango_layout_line_get_x_ranges (PangoLayoutLine  *line,
				int               start_index,
				int               end_index,
				int             **ranges,
				int              *n_ranges)
{
  PangoDirection base_dir;
  PangoRectangle logical_rect;
  gint line_start_index = 0;
  GSList *tmp_list;
  int range_count = 0;
  int accumulated_width = 0;
  int x_offset;
  int width;
  
  g_return_if_fail (line != NULL);
  g_return_if_fail (line->layout != NULL);
  g_return_if_fail (start_index <= end_index);

  base_dir = pango_context_get_base_dir (line->layout->context);

  width = line->layout->width;
  if (width == -1 && line->layout->alignment != PANGO_ALIGN_LEFT)
    {
      pango_layout_get_extents (line->layout, NULL, &logical_rect);
      width = logical_rect.width;
    }

  /* FIXME: The computations here could be optimized, by moving the
   * computations of the x_offset after we go through and figure
   * out where each range is.
   */
  pango_layout_line_get_extents (line, NULL, &logical_rect);

  if (line->layout->alignment == PANGO_ALIGN_RIGHT)
    x_offset = width - logical_rect.width;
  else if (line->layout->alignment == PANGO_ALIGN_CENTER)
    x_offset = (width - logical_rect.width) / 2;
  else
    x_offset = 0;
	  
  tmp_list = line->layout->lines;
  while (tmp_list->data != line)
    {
      line_start_index += ((PangoLayoutLine *)tmp_list->data)->length;
      tmp_list = tmp_list->next;
    }

  /* Allocate the maximum possible size */
  if (ranges)
    *ranges = g_new (int, 2 * (2 + g_slist_length (line->runs)));
    
  if (x_offset > 0 &&
      ((base_dir == PANGO_DIRECTION_LTR && start_index < line_start_index) ||
       (base_dir == PANGO_DIRECTION_RTL && end_index > line_start_index + line->length)))
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
	{
	  PangoRectangle run_logical;
	  
	  pango_glyph_string_extents (run->glyphs, run->item->analysis.font,
				      NULL, &run_logical);
	  accumulated_width += run_logical.width;
	}

      tmp_list = tmp_list->next;
    }

  if (x_offset + logical_rect.width < line->layout->width &&
      ((base_dir == PANGO_DIRECTION_LTR && end_index > line_start_index + line->length) ||
       (base_dir == PANGO_DIRECTION_RTL && start_index < line_start_index)))
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
				     PangoRectangle  *ink_rect,
				     PangoRectangle  *logical_rect)
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
      char *line_start;
      int index;
      PangoLayout *layout = line->layout;
      PangoFontDescription font_desc;
      PangoFont *font;
      PangoFontMetrics metrics;

      pango_layout_line_get_range (line, &line_start, NULL);
      index = line_start - layout->text;

      /* Find the font description for this line
       */
      if (layout->attrs)
	{
	  PangoAttrIterator *iter = pango_attr_list_get_iterator (layout->attrs);
	  int start, end;
	  
	  while (TRUE)
	    {
	      pango_attr_iterator_range (iter, &start, &end);
	      
	      if (start <= index && index < end)
		{
		  PangoFontDescription *base_font_desc;

		  if (layout->font_desc)
		    base_font_desc = layout->font_desc;
		  else
		    base_font_desc = pango_context_get_font_description (layout->context);
		    
		  pango_attr_iterator_get_font (iter,
						base_font_desc,
						&font_desc,
						NULL);
		  break;
		}
	      
	      pango_attr_iterator_next (iter);
	    }
	  
	  pango_attr_iterator_destroy (iter);
	}
      else
	{
	  if (layout->font_desc)
	    font_desc = *layout->font_desc;
	  else
	    font_desc = *pango_context_get_font_description (layout->context);
	}

      font = pango_context_load_font (layout->context, &font_desc);
      if (font)
	{
	  char *lang = pango_context_get_lang (layout->context);
	  pango_font_get_metrics (font, lang, &metrics);
	  g_free (lang);

	  logical_rect->y = - metrics.ascent;
	  logical_rect->height = metrics.ascent + metrics.descent;

	  g_object_unref (G_OBJECT (font));
	}
      else
	{
	  logical_rect->y = 0;
	  logical_rect->height = 0;
	}
      
      logical_rect->x = 0;
      logical_rect->width = 0;
    }
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

  if (!line->runs)
    {
      pango_layout_line_get_empty_extents (line, ink_rect, logical_rect);
      return;
    }
  
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

/**
 * pango_layout_line_get_pixel_extents:
 * @layout:   a #PangoLayout
 * @ink_rect: rectangle used to store the extents of the glyph string as drawn
 *            or %NULL to indicate that the result is not needed.
 * @logical_rect: rectangle used to store the logical extents of the glyph string
 *            or %NULL to indicate that the result is not needed.
 * 
 * Compute the logical and ink extents of a layout line. See the documentation
 * for pango_font_get_glyph_extents() for details about the interpretation
 * of the rectangles. The returned rectangles are in device units, as
 * opposed to pango_layout_line_get_extents(), which returns the extents in
 * units of device unit / PANGO_SCALE.
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

static void
pango_layout_line_postprocess (PangoLayoutLine *line)
{
  /* NB: the runs are in reverse order at this point, since we prepended them to the list
   */
  
  /* Reverse the runs
   */
  line->runs = g_slist_reverse (line->runs);

  /* Now convert logical to visual order
   */
  pango_layout_line_reorder (line);
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
