#include "config.h"

#include "pango-lines-private.h"
#include "pango-line-private.h"
#include "pango-item-private.h"
#include "pango-run-private.h"
#include "pango-line-iter-private.h"

/**
 * PangoLines:
 *
 * A `PangoLines` object represents the result of formatting an
 * entire paragraph (or more) of text.
 *
 * A `PangoLines` object contains a list of `PangoLine` objects,
 * together with information about where to position each line
 * in layout coordinates.
 *
 * `PangoLines` has APIs to query collective information about
 * its lines (such as ellipsization or unknown glyphs), to translate
 * between logical character positions within the text and the physical
 * position of the resulting glyphs, and to determine cursor positions
 * for editing the text.
 *
 * One way to obtain a `PangoLines` object is to use a [class@Pango.Layout].
 * But it is also possible to populate a `PangoLines` manually with lines
 * produced by a [class@Pango.LineBreaker] object.
 *
 * Note that the lines that make up a `PangoLines` object don't have to
 * share the same underlying text. Therefore, using byte indexes to refer
 * to positions within the `PangoLines` is, in general, ambiguous. All the
 * `PangoLines` APIs that take a byte index as argument or return one have
 * a `PangoLine*` companion argument to handle this situation. When all
 * the lines in the `PangoLines` share the same text  (such as when they
 * originate from the same `PangoLayout`), it is safe to always pass `NULL`
 * for the `PangoLines*`.
 *
 * The most convenient way to access the visual extents and components
 * of a `PangoLines` is via a [struct@Pango.LineIter] iterator.
 */

/*  {{{ PangoLines implementation */

typedef struct _Line Line;
struct _Line
{
  PangoLine *line;
  int x, y;
};

struct _PangoLinesClass
{
  GObjectClass parent_class;
};

G_DEFINE_TYPE (PangoLines, pango_lines, G_TYPE_OBJECT)

static void
pango_lines_init (PangoLines *lines)
{
  lines->serial = 1;
  lines->lines = g_array_new (FALSE, FALSE, sizeof (Line));
}

static void
pango_lines_finalize (GObject *object)
{
  PangoLines *lines = PANGO_LINES (object);

  for (int i = 0; i < lines->lines->len; i++)
    {
      Line *line = &g_array_index (lines->lines, Line, i);
      pango_line_free (line->line);
    }

  g_array_free (lines->lines, TRUE);

  G_OBJECT_CLASS (pango_lines_parent_class)->finalize (object);
}

static void
pango_lines_class_init (PangoLinesClass *class)
{
  GObjectClass *object_class = G_OBJECT_CLASS (class);

  object_class->finalize = pango_lines_finalize;
}

/* }}} */
 /* {{{ Utilities*/

typedef struct {
  int x;
  int pos;
} CursorPos;

static int
compare_cursor (gconstpointer v1,
                gconstpointer v2)
{
  const CursorPos *c1 = v1;
  const CursorPos *c2 = v2;

  return c1->x - c2->x;
}

static void
pango_line_get_cursors (PangoLines      *lines,
                        PangoLine       *line,
                        gboolean         strong,
                        GArray          *cursors)
{
  const char *start, *end;
  int start_offset;
  int j;
  const char *p;
  PangoRectangle pos;
  Line *l = NULL;

  g_assert (g_array_get_element_size (cursors) == sizeof (CursorPos));
  g_assert (cursors->len == 0);

  for (int i = 0; i < lines->lines->len; i++)
    {
      Line *ll = &g_array_index (lines->lines, Line, i);
      if (ll->line == line)
        {
          l = ll;
          break;
        }
    }

  start = line->data->text + line->start_index;
  end = start + line->length;
  start_offset = line->start_offset;

  if (line->ends_paragraph)
    end++;

  for (j = start_offset, p = start; p < end; j++, p = g_utf8_next_char (p))
    {
      int idx = p - line->data->text;

      if (line->data->log_attrs[j].is_cursor_position)
        {
          CursorPos cursor;

          pango_line_get_cursor_pos (line, idx,
                                     strong ? &pos : NULL,
                                     strong ? NULL : &pos);

          cursor.x = pos.x + l->x;
          cursor.pos = idx;
          g_array_append_val (cursors, cursor);
        }
    }

  g_array_sort (cursors, compare_cursor);
}

/* }}} */
 /* {{{ Public API */

/**
 * pango_lines_new:
 *
 * Creates an empty `PangoLines` object.
 *
 * Returns: a newly allocated `PangoLines`
 */
PangoLines *
pango_lines_new (void)
{
  return g_object_new (PANGO_TYPE_LINES, NULL);
}

/**
 * pango_lines_get_serial:
 * @lines: a `PangoLines`
 *
 * Returns the current serial number of @lines.
 *
 * The serial number is initialized to an small number larger than zero
 * when a new layout is created and is increased whenever the @lines
 * object is changed (i.e. more lines are added).
 *
 * The serial may wrap, but will never have the value 0. Since it can
 * wrap, never compare it with "less than", always use "not equals".
 *
 * This can be used to automatically detect changes to a `PangoLines`,
 * and is useful for example to decide whether a layout needs redrawing.
 *
 * Return value: The current serial number of @lines
 */
guint
pango_lines_get_serial (PangoLines *lines)
{
  return lines->serial;
}

/**
 * pango_lines_add_line:
 * @lines: a `PangoLines`
 * @line: (transfer full): the `PangoLine` to add
 * @line_x: X coordinate of the position
 * @line_y: Y coordinate of the position
 *
 * Adds a line to the `PangoLines`.
 *
 * The coordinates are the position
 * at which @line is placed.
 *
 * Note that this function takes ownership of the line.
 */
void
pango_lines_add_line (PangoLines      *lines,
                      PangoLine       *line,
                      int              x_line,
                      int              y_line)
{
  Line l;

  l.line = line;
  l.x = x_line;
  l.y = y_line;

  g_array_append_val (lines->lines, l);

  lines->serial++;
  if (lines->serial == 0)
    lines->serial++;
}

/**
 * pango_lines_get_iter:
 * @lines: a `PangoLines`
 *
 * Returns an iterator to iterate over the visual extents of the lines.
 *
 * The returned iterator will be invaliated when more
 * lines are added to @lines, and can't be used anymore
 * after that point.
 *
 * Note that the iter holds a reference to @lines.
 *
 * Return value: the new `PangoLineIter`
 */
PangoLineIter *
pango_lines_get_iter (PangoLines *lines)
{
  return pango_line_iter_new (lines);
}

/**
 * pango_lines_get_line_count:
 * @lines: a `PangoLines`
 *
 * Gets the number of lines in @lines.
 *
 * Returns: the number of lines'
 */
int
pango_lines_get_line_count (PangoLines *lines)
{
  return lines->lines->len;
}

/**
 * pango_lines_get_line:
 * @lines: a `PangoLines`
 * @num: the position of the line to get
 * @line_x: (out) (optional): return location for the X coordinate
 * @line_y: (out) (optional): return location for the Y coordinate
 *
 * Gets the @num-th line of @lines.
 *
 * Returns: (transfer none) (nullable): the line that was found
 */
PangoLine *
pango_lines_get_line (PangoLines  *lines,
                      int          num,
                      int         *line_x,
                      int         *line_y)
{
  Line *l;

  g_return_val_if_fail (PANGO_IS_LINES (lines), NULL);

  if (num >= lines->lines->len)
    return NULL;

  l = &g_array_index (lines->lines, Line, num);

  if (line_x)
    *line_x = l->x;
  if (line_y)
    *line_y = l->y;

  return l->line;
}

/* {{{ Miscellaneous */

/**
 * pango_lines_get_unknown_glyphs_count:
 * @lines: a `PangoLines` object
 *
 * Counts the number of unknown glyphs in @lines.
 *
 * This function can be used to determine if there are any fonts
 * available to render all characters in a certain string, or when
 * used in combination with `PANGO_ATTR_FALLBACK`, to check if a
 * certain font supports all the characters in the string.
 *
 * Return value: The number of unknown glyphs in @lines
 */
int
pango_lines_get_unknown_glyphs_count (PangoLines *lines)
{
  int count = 0;

  for (int i = 0; i < lines->lines->len; i++)
    {
      Line *ll = &g_array_index (lines->lines, Line, i);

      for (GSList *l = ll->line->runs; l; l = l->next)
        {
          PangoGlyphItem *run = l->data;

          for (int j = 0; j < run->glyphs->num_glyphs; j++)
            {
              if (run->glyphs->glyphs[j].glyph & PANGO_GLYPH_UNKNOWN_FLAG)
                count++;
            }
        }
    }

  return count;
}

/**
 * pango_lines_is_wrapped:
 * @lines: a `PangoLines` object
 *
 * Returns whether any line in @lines is wrapped.
 *
 * Returns: `TRUE` if @lines is wrapped
 */
gboolean
pango_lines_is_wrapped (PangoLines *lines)
{
  for (int i = 0; i < lines->lines->len; i++)
    {
      Line *l = &g_array_index (lines->lines, Line, i);
      if (pango_line_is_wrapped (l->line))
        return TRUE;
    }

  return FALSE;
}

/**
 * pango_lines_is_ellipsized:
 * @lines: a `PangoLines` object
 *
 * Returns whether any line in @lines is ellipsized.
 *
 * Returns: `TRUE` if @lines is ellipsized
 */
gboolean
pango_lines_is_ellipsized (PangoLines *lines)
{
  for (int i = 0; i < lines->lines->len; i++)
    {
      Line *l = &g_array_index (lines->lines, Line, i);
      if (pango_line_is_ellipsized (l->line))
        return TRUE;
    }

  return FALSE;
}

/**
 * pango_lines_is_hyphenated:
 * @lines: a `PangoLines` object
 *
 * Returns whether any line in @lines is hyphenated.
 *
 * Returns: `TRUE` if @lines is hyphenated
 */
gboolean
pango_lines_is_hyphenated (PangoLines *lines)
{
  for (int i = 0; i < lines->lines->len; i++)
    {
      Line *l = &g_array_index (lines->lines, Line, i);
      if (pango_line_is_hyphenated (l->line))
        return TRUE;
    }

  return FALSE;
}

/* }}} */
/* {{{ Extents */

/**
 * pango_lines_get_extents:
 * @lines: a `PangoLines` object
 * @ink_rect: (out) (optional): return location for the ink extents
 * @logical_rect: (out) (optional): return location for the logical extents
 *
 * Computes the extents of @lines.
 *
 * Logical extents are usually what you want for positioning things. Note
 * that the extents may have non-zero x and y. You may want to use those
 * to offset where you render the layout. Not doing that is a very typical
 * bug that shows up as right-to-left layouts not being correctly positioned
 * in a layout with a set width.
 *
 * The extents are given in layout coordinates and in Pango units; layout
 * coordinates begin at the top left corner.
 */
void
pango_lines_get_extents (PangoLines     *lines,
                         PangoRectangle *ink_rect,
                         PangoRectangle *logical_rect)
{
  for (int i = 0; i < lines->lines->len; i++)
    {
      Line *l = &g_array_index (lines->lines, Line, i);
      PangoRectangle line_ink;
      PangoRectangle line_logical;
      PangoLeadingTrim trim = PANGO_LEADING_TRIM_NONE;

      if (l->line->starts_paragraph)
        trim |= PANGO_LEADING_TRIM_START;
      if (l->line->ends_paragraph)
        trim |= PANGO_LEADING_TRIM_END;

      pango_line_get_extents (l->line, &line_ink, NULL);
      pango_line_get_trimmed_extents (l->line, trim, &line_logical);

      line_ink.x += l->x;
      line_ink.y += l->y;
      line_logical.x += l->x;
      line_logical.y += l->y;

      if (i == 0)
        {
          if (ink_rect)
            *ink_rect = line_ink;
          if (logical_rect)
            *logical_rect = line_logical;
        }
      else
        {
          int new_pos;

          if (ink_rect)
            {
              new_pos = MIN (ink_rect->x, line_ink.x);
              ink_rect->width = MAX (ink_rect->x + ink_rect->width, line_ink.x + line_ink.width) - new_pos;
              ink_rect->x = new_pos;

              new_pos = MIN (ink_rect->y, line_ink.y);
              ink_rect->height = MAX (ink_rect->y + ink_rect->height, line_ink.y + line_ink.height) - new_pos;
              ink_rect->y = new_pos;
           }
          if (logical_rect)
            {
              new_pos = MIN (logical_rect->x, line_logical.x);
              logical_rect->width = MAX (logical_rect->x + logical_rect->width, line_logical.x + line_logical.width) - new_pos;
              logical_rect->x = new_pos;

              new_pos = MIN (logical_rect->y, line_logical.y);
              logical_rect->height = MAX (logical_rect->y + logical_rect->height, line_logical.y + line_logical.height) - new_pos;
              logical_rect->y = new_pos;
           }
       }
    }
}

/**
 * pango_lines_get_size:
 * @lines: a `PangoLines`
 * @width: (out) (optional): location to store the logical width
 * @height: (out) (optional): location to store the logical height
 *
 * Determines the logical width and height of a `PangoLines`
 * in Pango units.
 *
 * This is simply a convenience function around
 * [method@Pango.Lines.get_extents].
 */
void
pango_lines_get_size (PangoLines *lines,
                      int        *width,
                      int        *height)
{
  PangoRectangle ext;

  pango_lines_get_extents (lines, NULL, &ext);

  if (width)
    *width = ext.width;
  if (height)
    *height = ext.height;
}

/**
 * pango_lines_get_baseline:
 * @lines: a `PangoLines` object
 *
 * Gets the Y position of baseline of the first line in @lines.
 *
 * Return value: baseline of first line
 */
int
pango_lines_get_baseline (PangoLines *lines)
{
  Line *l;

  g_return_val_if_fail (PANGO_IS_LINES (lines), 0);

  if (lines->lines->len == 0)
    return 0;

  l = &g_array_index (lines->lines, Line, 0);

  return l->y;
}

/**
 * pango_lines_get_x_ranges:
 * @lines: a `PangoLines` object
 * @line: the `PangoLine` in @lines whose x ranges will be reported
 * @start_line: (nullable): `PangoLine` wrt to which @start_index is
 *   interpreted or `NULL` for the first matching line
 * @start_index: Start byte index of the logical range. If this value
 *   is less than the start index for the line, then the first range
 *   will extend all the way to the leading edge of the layout. Otherwise,
 *   it will start at the leading edge of the first character.
 * @end_line: (nullable): `PangoLine` wrt to which @end_index is
 *   interpreted or `NULL` for the first matching line
 * @end_index: Ending byte index of the logical range. If this value is
 *   greater than the end index for the line, then the last range will
 *   extend all the way to the trailing edge of the layout. Otherwise,
 *   it will end at the trailing edge of the last character.
 * @ranges: (out) (array length=n_ranges) (transfer full): location to
 *   store a pointer to an array of ranges. The array will be of length
 *   `2*n_ranges`, with each range starting at `(*ranges)[2*n]` and of
 *   width `(*ranges)[2*n + 1] - (*ranges)[2*n]`. This array must be freed
 *   with g_free(). The coordinates are relative to the layout and are in
 *   Pango units.
 * @n_ranges: The number of ranges stored in @ranges
 *
 * Gets a list of visual ranges corresponding to a given logical range.
 *
 * This list is not necessarily minimal - there may be consecutive
 * ranges which are adjacent. The ranges will be sorted from left to
 * right. The ranges are with respect to the left edge of the entire
 * layout, not with respect to the line.
 */
void
pango_lines_get_x_ranges (PangoLines *lines,
                          PangoLine  *line,
                          PangoLine  *start_line,
                          int         start_index,
                          PangoLine  *end_line,
                          int         end_index,
                          int       **ranges,
                          int        *n_ranges)
{
  int x_offset;
  int line_no, start_line_no, end_line_no;
  PangoDirection dir;
  int range_count;
  int width;
  PangoRectangle ext;
  int accumulated_width;

  g_return_if_fail (PANGO_IS_LINES (lines));
  g_return_if_fail (ranges != NULL);
  g_return_if_fail (n_ranges != NULL);

  pango_lines_index_to_line (lines, line->start_index, &line, &line_no, &x_offset, NULL);
  g_return_if_fail (line != NULL);

  pango_lines_index_to_line (lines, start_index, &start_line, &start_line_no, NULL, NULL);
  g_return_if_fail (start_line != NULL);
  g_return_if_fail (start_line_no <= line_no);

  pango_lines_index_to_line (lines, end_index, &end_line, &end_line_no, NULL, NULL);
  g_return_if_fail (end_line != NULL);
  g_return_if_fail (end_line_no >= line_no);

  /* clamp start_index and end_index to be inside line, we'll use
   * the line numbers to determine overshoot.
   */
  if (start_line_no < line_no || line_no < end_line_no)
    {
      if (start_line_no < line_no)
        start_index = line->start_index;
      if (end_line_no > line_no)
        end_index = line->start_index + line->length;
    }

  *ranges = g_new (int, 2 * (2 + g_slist_length (line->runs)));
  range_count = 0;

  dir = pango_line_get_resolved_direction (line);

  if (x_offset > 0 &&
      ((dir == PANGO_DIRECTION_LTR && start_line_no < line_no) ||
       (dir == PANGO_DIRECTION_RTL && end_line_no > line_no)))
    {
      /* add range from left edge of layout to line start */
      (*ranges)[2*range_count] = 0;
      (*ranges)[2*range_count + 1] = x_offset;
      range_count++;
    }

  accumulated_width = 0;
  for (int i = 0; i < pango_line_get_run_count (line); i++)
    {
      PangoGlyphItem *run = pango_run_get_glyph_item (pango_line_get_runs (line)[i]);

      if ((start_index < run->item->offset + run->item->length &&
           end_index > run->item->offset))
        {
          int run_start_index = MAX (start_index, run->item->offset);
          int run_end_index = MIN (end_index, run->item->offset + run->item->length);
          int run_start_x, run_end_x;
          int attr_offset;

          g_assert (run_end_index > 0);

          /* Back the end_index off one since we want to find the trailing edge of the
           * preceding character
           */

          run_end_index = g_utf8_prev_char (line->data->text + run_end_index) - line->data->text;

          attr_offset = run->item->char_offset;

          pango_glyph_string_index_to_x_full (run->glyphs,
                                              line->data->text + run->item->offset,
                                              run->item->length,
                                              &run->item->analysis,
                                              line->data->log_attrs + attr_offset,
                                              run_start_index - run->item->offset, FALSE,
                                              &run_start_x);
          pango_glyph_string_index_to_x_full (run->glyphs,
                                              line->data->text + run->item->offset,
                                              run->item->length,
                                              &run->item->analysis,
                                              line->data->log_attrs + attr_offset,
                                              run_end_index - run->item->offset, TRUE,
                                              &run_end_x);

          (*ranges)[2*range_count] = x_offset + accumulated_width + MIN (run_start_x, run_end_x);
          (*ranges)[2*range_count + 1] = x_offset + accumulated_width + MAX (run_start_x, run_end_x);
       }

     range_count++;

     if (i + 1 < pango_line_get_run_count (line))
       accumulated_width += pango_glyph_string_get_width (run->glyphs);
   }

  pango_line_get_extents (line, NULL, &ext);
  pango_lines_get_size (lines, &width, NULL);

  if (x_offset + ext.width < width &&
      ((dir == PANGO_DIRECTION_LTR && start_line_no < line_no) ||
       (dir == PANGO_DIRECTION_RTL && end_line_no > line_no)))
    {
      /* add range from line end to rigth edge of layout */
      (*ranges)[2*range_count] = x_offset + ext.width;
      (*ranges)[2*range_count + 1] = width;
      range_count ++;
    }

  *n_ranges = range_count;
}

/* }}} */
 /* {{{ Editing API */

/**
 * pango_lines_index_to_line:
 * @lines: a `PangoLines`
 * @idx: index in @lines
 * @line: (inout): if *@line is `NULL`, the found line will be returned here
 * @line_no: (out) (optional): return location for line number
 * @x_offset: (out) (optional): return location for X offset of line
 * @y_offset: (out) (optional): return location for Y offset of line
 *
 * Given an index (and possibly line), determine the line number,
 * and offset for the line.
 *
 * @idx may refer to any byte position inside @lines, as well as
 * the position before the first or after the last character (i.e.
 * line->start_index + line->length, for the last line).
 *
 * If @lines contains lines with different backing data (i.e.
 * more than one line with line->start_index of zero), then
 * *@line must be specified to disambiguate. If all lines
 * are backed by the same data, it is save to pass `NULL`
 * as *@line and use this function to find the line at @idx.
 */
void
pango_lines_index_to_line (PangoLines *lines,
                           int         idx,
                           PangoLine **line,
                           int        *line_no,
                           int        *x_offset,
                           int        *y_offset)
{
  Line *found = NULL;
  int num;
  int i;

  g_return_if_fail (PANGO_IS_LINES (lines));

  for (i = 0; i < lines->lines->len; i++)
    {
      Line *l = &g_array_index (lines->lines, Line, i);

      if (l->line->start_index > idx && found)
        break;

      found = l;
      num = i;

      if (*line && *line == found->line)
        break;

      if (l->line->start_index + l->line->length > idx)
        break;
    }

  if (found)
    {
      *line = found->line;
      if (line_no)
        *line_no = num;
      if (x_offset)
        *x_offset = found->x;
      if (y_offset)
        *y_offset = found->y;

      return;
    }

  *line = NULL;
}

/**
 * pango_lines_pos_to_line:
 * @lines: a `PangoLines` object
 * @x: the X position (in Pango units)
 * @y: the Y position (in Pango units)
 * @line_x: (out) (optional): return location for the X offset of the line
 * @line_y: (out) (optional): return location for the Y offset of the line
 *
 * Finds the line at the given position.
 *
 * If either the X or Y positions were not inside the layout, then the
 * function returns `NULL`; on an exact hit, it returns the found line.

 * Returns: (transfer none) (nullable): the line that was found
 */
PangoLine *
pango_lines_pos_to_line (PangoLines *lines,
                         int         x,
                         int         y,
                         int        *line_x,
                         int        *line_y)
{
  g_return_val_if_fail (PANGO_IS_LINES (lines), FALSE);

  for (int i = 0; i < lines->lines->len; i++)
    {
      Line *l = &g_array_index (lines->lines, Line, i);
      PangoRectangle ext;

      pango_line_get_extents (l->line, NULL, &ext);

      ext.x += l->x;
      ext.y += l->y;

      if (ext.x <= x && x <= ext.x + ext.width &&
          ext.y <= y && y <= ext.y + ext.height)
        {
          if (line_x)
            *line_x = l->x;
          if (line_y)
            *line_y = l->y;

          return l->line;
        }
    }

  if (line_x)
    *line_x = 0;
  if (line_y)
    *line_y = 0;

  return NULL;
}

/**
 * pango_lines_index_to_pos:
 * @lines: a `PangoLines` object
 * @line: (nullable): `PangoLine` wrt to which @idx is interpreted
 *   or `NULL` for the first matching line
 * @idx: byte index within @line
 * @pos: (out): rectangle in which to store the position of the grapheme
 *
 * Converts from an index within a `PangoLine` to the
 * position corresponding to the grapheme at that index.
 *
 * The return value is represented as rectangle. Note that `pos->x`
 * is always the leading edge of the grapheme and `pos->x + pos->width`
 * the trailing edge of the grapheme. If the directionality of the
 * grapheme is right-to-left, then `pos->width` will be negative.
 *
 * Note that @idx is allowed to be @line->start_index + @line->length
 * for the position off the end of the last line.
 */
void
pango_lines_index_to_pos (PangoLines     *lines,
                          PangoLine      *line,
                          int             idx,
                          PangoRectangle *pos)
{
  int x_offset, y_offset;

  g_return_if_fail (PANGO_IS_LINES (lines));
  g_return_if_fail (idx >= 0);
  g_return_if_fail (pos != NULL);

  pango_lines_index_to_line (lines, idx, &line, NULL, &x_offset, &y_offset);

  g_return_if_fail (line != NULL);

  pango_line_index_to_pos (line, idx, pos);
  pos->x += x_offset;
  pos->y += y_offset;
}

/**
 * pango_lines_pos_to_index:
 * @lines: a `PangoLines` object
 * @x: the X offset (in Pango units) from the left edge of the layout
 * @y: the Y offset (in Pango units) from the top edge of the layout
 * @idx: (out): location to store calculated byte index
 * @trailing: (out): location to store a integer indicating where
 *   in the grapheme the user clicked. It will either be zero, or the
 *   number of characters in the grapheme. 0 represents the leading edge
 *   of the grapheme.
 *
 * Converts from X and Y position within lines to the byte index of the
 * character at that position.
 *
 * Returns: (transfer none) (nullable): the line that was found
 */
PangoLine *
pango_lines_pos_to_index (PangoLines *lines,
                          int         x,
                          int         y,
                          int        *idx,
                          int        *trailing)
{
  PangoLine *line;
  int x_offset;

  g_return_val_if_fail (PANGO_IS_LINES (lines), FALSE);
  g_return_val_if_fail (idx != NULL, FALSE);
  g_return_val_if_fail (trailing != NULL, FALSE);

  line = pango_lines_pos_to_line (lines, x, y, &x_offset, NULL);
  if (line)
    pango_line_x_to_index (line, x - x_offset, idx, trailing);

  return line;
}

/* }}} */
/* {{{ Cursor positioning */

/**
 * pango_lines_get_cursor_pos:
 * @lines: a `PangoLines` object
 * @line: (nullable): `PangoLine` wrt to which @idx is interpreted
 *   or `NULL` for the first matching line
 * @idx: the byte index of the cursor
 * @strong_pos: (out) (optional): location to store the strong cursor position
 * @weak_pos: (out) (optional): location to store the weak cursor position
 *
 * Given an index within lines, determines the positions that of the
 * strong and weak cursors if the insertion point is at that index.
 *
 * Note that @idx is allowed to be @line->start_index + @line->length
 * for the position off the end of the last line.
 *
 * The position of each cursor is stored as a zero-width rectangle
 * with the height of the run extents.
 *
 * <picture>
 *   <source srcset="cursor-positions-dark.png" media="(prefers-color-scheme: dark)">
 *   <img alt="Cursor positions" src="cursor-positions-light.png">
 * </picture>
 *
 * The strong cursor location is the location where characters of the
 * directionality equal to the base direction of the layout are inserted.
 * The weak cursor location is the location where characters of the
 * directionality opposite to the base direction of the layout are inserted.
 *
 * The following example shows text with both a strong and a weak cursor.
 *
 * <picture>
 *   <source srcset="split-cursor-dark.png" media="(prefers-color-scheme: dark)">
 *   <img alt="Strong and weak cursors" src="split-cursor-light.png">
 * </picture>
 *
 * The strong cursor has a little arrow pointing to the right, the weak
 * cursor to the left. Typing a 'c' in this situation will insert the
 * character after the 'b', and typing another Hebrew character, like 'ג',
 * will insert it at the end.
 */
void
pango_lines_get_cursor_pos (PangoLines     *lines,
                            PangoLine      *line,
                            int             idx,
                            PangoRectangle *strong_pos,
                            PangoRectangle *weak_pos)
{
  int x_offset, y_offset;
  PangoLine *l;

  g_return_if_fail (PANGO_IS_LINES (lines));

  l = line;
  pango_lines_index_to_line (lines, idx, &l, NULL, &x_offset, &y_offset);

  g_return_if_fail (l != NULL);
  g_return_if_fail (line == NULL || l == line);

  line = l;

  pango_line_get_cursor_pos (line, idx, strong_pos, weak_pos);

  if (strong_pos)
    {
      strong_pos->x += x_offset;
      strong_pos->y += y_offset;
    }
  if (weak_pos)
    {
      weak_pos->x += x_offset;
      weak_pos->y += y_offset;
    }
}

/**
 * pango_lines_get_caret_pos:
 * @lines: a `PangoLines` object
 * @line: (nullable): `PangoLine` wrt to which @idx is interpreted
 *   or `NULL` for the first matching line
 * @idx: the byte index of the cursor
 * @strong_pos: (out) (optional): location to store the strong cursor position
 * @weak_pos: (out) (optional): location to store the weak cursor position
 *
 * Given an index within a layout, determines the positions of the
 * strong and weak cursors if the insertion point is at that index.
 *
 * Note that @idx is allowed to be @line->start_index + @line->length
 * for the position off the end of the last line.
 *
 * This is a variant of [method@Pango.Lines.get_cursor_pos] that applies
 * font metric information about caret slope and offset to the positions
 * it returns.
 *
 * <picture>
 *   <source srcset="caret-metrics-dark.png" media="(prefers-color-scheme: dark)">
 *   <img alt="Caret metrics" src="caret-metrics-light.png">
 * </picture>
 */
void
pango_lines_get_caret_pos (PangoLines     *lines,
                           PangoLine      *line,
                           int             idx,
                           PangoRectangle *strong_pos,
                           PangoRectangle *weak_pos)
{
  int x_offset, y_offset;

  g_return_if_fail (PANGO_IS_LINES (lines));

  pango_lines_index_to_line (lines, idx, &line, NULL, &x_offset, &y_offset);

  g_return_if_fail (line != NULL);

  pango_line_get_caret_pos (line, idx, strong_pos, weak_pos);

  if (strong_pos)
    {
      strong_pos->x += x_offset;
      strong_pos->y += y_offset;
    }
  if (weak_pos)
    {
      weak_pos->x += x_offset;
      weak_pos->y += y_offset;
    }
}

/**
 * pango_lines_move_cursor:
 * @lines: a `PangoLines` object
 * @strong: whether the moving cursor is the strong cursor or the
 *   weak cursor. The strong cursor is the cursor corresponding
 *   to text insertion in the base direction for the layout.
 * @line: (nullable): `PangoLine` wrt to which @idx is interpreted
 *   or `NULL` for the first matching line
 * @idx: the byte index of the current cursor position
 * @trailing: if 0, the cursor was at the leading edge of the
 *   grapheme indicated by @old_index, if > 0, the cursor
 *   was at the trailing edge.
 * @direction: direction to move cursor. A negative
 *   value indicates motion to the left
 * @new_line: (nullable): `PangoLine` wrt to which @new_idx is interpreted
 * @new_idx: (out): location to store the new cursor byte index
 *   A value of -1 indicates that the cursor has been moved off the
 *   beginning of the layout. A value of %G_MAXINT indicates that
 *   the cursor has been moved off the end of the layout.
 * @new_trailing: (out): number of characters to move forward from
 *   the location returned for @new_idx to get the position where
 *   the cursor should be displayed. This allows distinguishing the
 *   position at the beginning of one line from the position at the
 *   end of the preceding line. @new_idx is always on the line where
 *   the cursor should be displayed.
 *
 * Computes a new cursor position from an old position and a direction.
 *
 * If @direction is positive, then the new position will cause the strong
 * or weak cursor to be displayed one position to right of where it was
 * with the old cursor position. If @direction is negative, it will be
 * moved to the left.
 *
 * In the presence of bidirectional text, the correspondence between
 * logical and visual order will depend on the direction of the current
 * run, and there may be jumps when the cursor is moved off of the end
 * of a run.
 *
 * Motion here is in cursor positions, not in characters, so a single
 * call to this function may move the cursor over multiple characters
 * when multiple characters combine to form a single grapheme.
 */
void
pango_lines_move_cursor (PangoLines  *lines,
                         gboolean     strong,
                         PangoLine   *line,
                         int          idx,
                         int          trailing,
                         int          direction,
                         PangoLine  **new_line,
                         int         *new_idx,
                         int         *new_trailing)
{
  int line_no;
  GArray *cursors;
  int n_vis;
  int vis_pos;
  int start_offset;
  gboolean off_start = FALSE;
  gboolean off_end = FALSE;
  PangoRectangle pos;
  int j;

  g_return_if_fail (PANGO_IS_LINES (lines));
  g_return_if_fail (idx >= 0);
  g_return_if_fail (trailing >= 0);
  g_return_if_fail (new_idx != NULL);
  g_return_if_fail (new_trailing != NULL);

  direction = (direction >= 0 ? 1 : -1);

  pango_lines_index_to_line (lines, idx, &line, &line_no, NULL, NULL);

  g_return_if_fail (line != NULL);

  while (trailing--)
    idx = g_utf8_next_char (line->data->text + idx) - line->data->text;

  /* Clamp old_index to fit on the line */
  if (idx > (line->start_index + line->length))
    idx = line->start_index + line->length;

  cursors = g_array_new (FALSE, FALSE, sizeof (CursorPos));
  pango_line_get_cursors (lines, line, strong, cursors);

  pango_lines_get_cursor_pos (lines, line, idx, strong ? &pos : NULL, strong ? NULL : &pos);

  vis_pos = -1;
  for (j = 0; j < cursors->len; j++)
    {
      CursorPos *cursor = &g_array_index (cursors, CursorPos, j);
      if (cursor->x == pos.x)
        {
          vis_pos = j;

          /* If moving left, we pick the leftmost match, otherwise
           * the rightmost one. Without this, we can get stuck
           */
          if (direction < 0)
            break;
        }
    }
  if (vis_pos == -1 &&
      idx == line->start_index + line->length)
    {
      if (line->direction == PANGO_DIRECTION_LTR)
        vis_pos = cursors->len;
      else
        vis_pos = 0;
    }

  /* Handling movement between lines */
  if (line->direction == PANGO_DIRECTION_LTR)
    {
      if (idx == line->start_index && direction < 0)
        off_start = TRUE;
      if (idx == line->start_index + line->length && direction > 0)
        off_end = TRUE;
    }
  else
    {
      if (idx == line->start_index + line->length && direction < 0)
        off_start = TRUE;
      if (idx == line->start_index && direction > 0)
        off_end = TRUE;
    }
  if (off_start || off_end)
    {
      /* If we move over a paragraph boundary, count that as
       * an extra position in the motion
       */
      gboolean paragraph_boundary;

      if (off_start)
        {
          PangoLine *prev_line;

          prev_line = pango_lines_get_line (lines, line_no - 1, NULL, NULL);
          if (!prev_line)
            {
              if (new_line)
                *new_line = NULL;
              *new_idx = -1;
              *new_trailing = 0;
              g_array_unref (cursors);
              return;
            }

          line = prev_line;
          line_no--;
          paragraph_boundary = (line->start_index + line->length != idx);
        }
      else
        {
          PangoLine *next_line;

          next_line = pango_lines_get_line (lines, line_no + 1, NULL, NULL);
          if (!next_line)
            {
              if (new_line)
                *new_line = NULL;
              *new_idx = G_MAXINT;
              *new_trailing = 0;
              g_array_unref (cursors);
              return;
            }
          line = next_line;
          line_no++;
          paragraph_boundary = (line->start_index != idx);
        }

      g_array_set_size (cursors, 0);
      pango_line_get_cursors (lines, line, strong, cursors);

      n_vis = cursors->len;

      if (off_start && direction < 0)
        {
          vis_pos = n_vis;
          if (paragraph_boundary)
            vis_pos++;
        }
      else if (off_end && direction > 0)
        {
          vis_pos = 0;
          if (paragraph_boundary)
            vis_pos--;
        }
    }

 if (direction < 0)
    vis_pos--;
  else
    vis_pos++;

  if (0 <= vis_pos && vis_pos < cursors->len)
    *new_idx = g_array_index (cursors, CursorPos, vis_pos).pos;
  else if (vis_pos >= cursors->len - 1)
    *new_idx = line->start_index + line->length;

  *new_trailing = 0;

  if (*new_idx == line->start_index + line->length && line->length > 0)
    {
      int log_pos;

      start_offset = g_utf8_pointer_to_offset (line->data->text, line->data->text + line->start_index);
      log_pos = start_offset + g_utf8_strlen (line->data->text + line->start_index, line->length);
      do
        {
          log_pos--;
          *new_idx = g_utf8_prev_char (line->data->text + *new_idx) - line->data->text;
          (*new_trailing)++;
        }
      while (log_pos > start_offset && !line->data->log_attrs[log_pos].is_cursor_position);
    }

  if (new_line)
    *new_line = line;

  g_array_unref (cursors);
}

/* }}} */
/* }}} */

/* vim:set foldmethod=marker expandtab: */
