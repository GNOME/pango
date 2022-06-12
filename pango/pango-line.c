#include "config.h"

#include "pango-line-private.h"

#include "pango-tabs.h"
#include "pango-impl-utils.h"
#include "pango-attributes-private.h"
#include "pango-attr-list-private.h"
#include "pango-attr-iterator-private.h"
#include "pango-item-private.h"
#include "pango-run-private.h"
#include "pango-font-metrics-private.h"

#include <math.h>
#include <hb-ot.h>

/**
 * PangoLine:
 *
 * A `PangoLine` is an immutable object which represents a line resulting
 * from laying out text via `PangoLayout` or `PangoLineBreaker`.
 *
 * A line consists of a number of runs (i.e. ranges of text with uniform
 * script, font and attributes that are shaped as a unit). Runs are
 * represented as [struct@Pango.Run] objects.
 *
 * A `PangoLine` always has its origin at the leftmost point  of its
 * baseline. To position lines in an entire paragraph of text (i.e. in layout
 * coordinates), the `PangoLines` object stores X and Y coordinates to
 * offset each line to.
 *
 * The most convenient way to access the visual extents and components
 * of a `PangoLine` is via a [struct@Pango.LineIter] iterator.
 */

/* {{{ LineData */

void
line_data_clear (LineData *data)
{
  g_free (data->text);
  g_clear_pointer (&data->attrs, pango_attr_list_unref);
  g_free (data->log_attrs);
}

LineData *
line_data_new (void)
{
  return g_rc_box_new0 (LineData);
}

LineData *
line_data_ref (LineData *data)
{
  return g_rc_box_acquire (data);
}

void
line_data_unref (LineData *data)
{
  g_rc_box_release_full (data, (GDestroyNotify) line_data_clear);
}

/* }}} */
/* {{{ PangoLine implementation */

G_DEFINE_BOXED_TYPE (PangoLine, pango_line,
                     pango_line_copy, pango_line_free);

/* }}} */
/* {{{ Justification */

static inline void
distribute_letter_spacing (int  letter_spacing,
                           int *space_left,
                           int *space_right)
{
  *space_left = letter_spacing / 2;

  /* hinting */
  if ((letter_spacing & (PANGO_SCALE - 1)) == 0)
    *space_left = PANGO_UNITS_ROUND (*space_left);
  *space_right = letter_spacing - *space_left;
}

static int
pango_line_compute_width (PangoLine *line)
{
  int width = 0;

  /* Compute the width of the line currently - inefficient, but easier
   * than keeping the current width of the line up to date everywhere
   */
  for (GSList *l = line->runs; l; l = l->next)
    {
      PangoGlyphItem *run = l->data;
      width += pango_glyph_string_get_width (run->glyphs);
    }

  return width;
}

static void
justify_clusters (PangoLine *line,
                  int       *remaining_width)
{
  int total_remaining_width, total_gaps = 0;
  int added_so_far, gaps_so_far;
  gboolean is_hinted;
  GSList *run_iter;
  enum {
    MEASURE,
    ADJUST
  } mode;

  total_remaining_width = *remaining_width;
  if (total_remaining_width <= 0)
    return;

  /* hint to full pixel if total remaining width was so */
  is_hinted = (total_remaining_width & (PANGO_SCALE - 1)) == 0;

  for (mode = MEASURE; mode <= ADJUST; mode++)
    {
      gboolean leftedge = TRUE;
      PangoGlyphString *rightmost_glyphs = NULL;
      int rightmost_space = 0;
      int residual = 0;

      added_so_far = 0;
      gaps_so_far = 0;

      for (run_iter = line->runs; run_iter; run_iter = run_iter->next)
        {
          PangoGlyphItem *run = run_iter->data;
          PangoGlyphString *glyphs = run->glyphs;
          PangoGlyphItemIter cluster_iter;
          gboolean have_cluster;
          int dir;
          int offset;

          dir = run->item->analysis.level % 2 == 0 ? +1 : -1;
          offset = run->item->char_offset;
          for (have_cluster = dir > 0 ?
                 pango_glyph_item_iter_init_start (&cluster_iter, run, line->data->text) :
                 pango_glyph_item_iter_init_end   (&cluster_iter, run, line->data->text);
               have_cluster;
               have_cluster = dir > 0 ?
                 pango_glyph_item_iter_next_cluster (&cluster_iter) :
                 pango_glyph_item_iter_prev_cluster (&cluster_iter))
            {
              int i;
              int width = 0;

              /* don't expand in the middle of graphemes */
              if (!line->data->log_attrs[offset + cluster_iter.start_char].is_cursor_position)
                continue;

              for (i = cluster_iter.start_glyph; i != cluster_iter.end_glyph; i += dir)
                width += glyphs->glyphs[i].geometry.width;

              /* also don't expand zero-width clusters. */
              if (width == 0)
                continue;

              gaps_so_far++;

              if (mode == ADJUST)
                {
                  int leftmost, rightmost;
                  int adjustment, space_left, space_right;

                  adjustment = total_remaining_width / total_gaps + residual;
                  if (is_hinted)
                    {
                      int old_adjustment = adjustment;
                      adjustment = PANGO_UNITS_ROUND (adjustment);
                      residual = old_adjustment - adjustment;
                    }
                  /* distribute to before/after */
                  distribute_letter_spacing (adjustment, &space_left, &space_right);

                  if (cluster_iter.start_glyph < cluster_iter.end_glyph)
                    {
                      /* LTR */
                      leftmost  = cluster_iter.start_glyph;
                      rightmost = cluster_iter.end_glyph - 1;
                    }
                  else
                    {
                      /* RTL */
                      leftmost  = cluster_iter.end_glyph + 1;
                      rightmost = cluster_iter.start_glyph;
                    }
                  /* Don't add to left-side of left-most glyph of left-most non-zero run. */
                 if (leftedge)
                    leftedge = FALSE;
                  else
                  {
                    glyphs->glyphs[leftmost].geometry.width    += space_left ;
                    glyphs->glyphs[leftmost].geometry.x_offset += space_left ;
                    added_so_far += space_left;
                  }
                  /* Don't add to right-side of right-most glyph of right-most non-zero run. */
                  {
                    /* Save so we can undo later. */
                    rightmost_glyphs = glyphs;
                    rightmost_space = space_right;

                    glyphs->glyphs[rightmost].geometry.width  += space_right;
                    added_so_far += space_right;
                  }
                }
            }
        }

      if (mode == MEASURE)
        {
          total_gaps = gaps_so_far - 1;

          if (total_gaps == 0)
            {
              /* a single cluster, can't really justify it */
              return;
            }
        }
      else /* mode == ADJUST */
        {
          if (rightmost_glyphs)
           {
             rightmost_glyphs->glyphs[rightmost_glyphs->num_glyphs - 1].geometry.width -= rightmost_space;
             added_so_far -= rightmost_space;
           }
        }
    }

  *remaining_width -= added_so_far;
}

static void
justify_words (PangoLine *line,
               int       *remaining_width)
{
  int total_remaining_width, total_space_width = 0;
  int added_so_far, spaces_so_far;
  gboolean is_hinted;
  GSList *run_iter;
  enum {
    MEASURE,
    ADJUST
  } mode;

  total_remaining_width = *remaining_width;
  if (total_remaining_width <= 0)
    return;

  /* hint to full pixel if total remaining width was so */
  is_hinted = (total_remaining_width & (PANGO_SCALE - 1)) == 0;

  for (mode = MEASURE; mode <= ADJUST; mode++)
    {
      added_so_far = 0;
      spaces_so_far = 0;

      for (run_iter = line->runs; run_iter; run_iter = run_iter->next)
        {
          PangoGlyphItem *run = run_iter->data;
          PangoGlyphString *glyphs = run->glyphs;
          PangoGlyphItemIter cluster_iter;
          gboolean have_cluster;
          int offset;

          offset = run->item->char_offset;
          for (have_cluster = pango_glyph_item_iter_init_start (&cluster_iter, run, line->data->text);
               have_cluster;
               have_cluster = pango_glyph_item_iter_next_cluster (&cluster_iter))
            {
              int i;
              int dir;

              if (!line->data->log_attrs[offset + cluster_iter.start_char].is_expandable_space)
                continue;
             dir = (cluster_iter.start_glyph < cluster_iter.end_glyph) ? 1 : -1;
              for (i = cluster_iter.start_glyph; i != cluster_iter.end_glyph; i += dir)
                {
                  int glyph_width = glyphs->glyphs[i].geometry.width;

                  if (glyph_width == 0)
                    continue;

                  spaces_so_far += glyph_width;

                  if (mode == ADJUST)
                    {
                      int adjustment;

                      adjustment = ((guint64) spaces_so_far * total_remaining_width) / total_space_width - added_so_far;
                      if (is_hinted)
                        adjustment = PANGO_UNITS_ROUND (adjustment);

                      glyphs->glyphs[i].geometry.width += adjustment;
                      added_so_far += adjustment;
                    }
                }
            }
        }

      if (mode == MEASURE)
        {
          total_space_width = spaces_so_far;

          if (total_space_width == 0)
            {
              justify_clusters (line, remaining_width);
              return;
            }
        }
    }

  *remaining_width -= added_so_far;
}

/* }}} */
/* {{{ Extents */

static void
compute_extents (PangoLine        *line,
                 PangoLeadingTrim  trim,
                 PangoRectangle   *ink,
                 PangoRectangle   *logical)
{
  int x_pos = 0;

  if (!line->runs)
    {
      memset (ink, 0, sizeof (PangoRectangle));
      pango_line_get_empty_extents (line, trim, logical);
      return;
    }

  for (GSList *l = line->runs; l; l = l->next)
    {
      PangoRun *run = l->data;
      PangoRectangle run_ink;
      PangoRectangle run_logical;
      int new_pos;

      pango_run_get_extents (run, trim, &run_ink, &run_logical);

      if (ink->width == 0 || ink->height == 0)
        {
          *ink = run_ink;
          ink->x += x_pos;
        }
      else if (run_ink.width != 0 && run_ink.height != 0)
        {
          new_pos = MIN (ink->x, x_pos + run_ink.x);
          ink->width = MAX (ink->x + ink->width,
                           x_pos + run_ink.x + run_ink.width) - new_pos;
          ink->x = new_pos;

          new_pos = MIN (ink->y, run_ink.y);
          ink->height = MAX (ink->y + ink->height,
                            run_ink.y + run_ink.height) - new_pos;
          ink->y = new_pos;
        }

      if (l == line->runs)
        {
          *logical = run_logical;
          logical->x += x_pos;
        }
      else
        {
          new_pos = MIN (logical->x, x_pos + run_logical.x);
          logical->width = MAX (logical->x + logical->width,
                               x_pos + run_logical.x + run_logical.width) - new_pos;
          logical->x = new_pos;

          new_pos = MIN (logical->y, run_logical.y);
          logical->height = MAX (logical->y + logical->height,
                                run_logical.y + run_logical.height) - new_pos;
          logical->y = new_pos;
        }

      x_pos += run_logical.width;
    }
}

/* }}} */
/* {{{ Private API */

void
pango_line_check_invariants (PangoLine *line)
{
  /* Check that byte and char positions agree */
  g_assert (g_utf8_strlen (line->data->text + line->start_index, line->length) == line->n_chars);
  g_assert (g_utf8_offset_to_pointer (line->data->text + line->start_index, line->n_chars) == line->data->text + line->start_index + line->length);

  /* Check that runs are sane */
  if (line->runs)
    {
      int run_min, run_max;
      int n_chars;

      run_min = G_MAXINT;
      run_max = 0;
      n_chars = 0;
      for (GSList *l = line->runs; l; l = l->next)
        {
          PangoGlyphItem *run = l->data;

          run_min = MIN (run_min, run->item->offset);
          run_max = MAX (run_max, run->item->offset + run->item->length);
          n_chars += run->item->num_chars;
        }

      g_assert (run_min == line->start_index);
      g_assert (run_max == line->start_index + line->length);
      g_assert (n_chars == line->n_chars);
    }
}

void
pango_line_get_empty_extents (PangoLine        *line,
                              PangoLeadingTrim  trim,
                              PangoRectangle   *logical_rect)
{
  PangoFontDescription *font_desc = NULL;
  gboolean free_font_desc = FALSE;
  double line_height_factor = 0.0;
  int absolute_line_height = 0;
  PangoFont *font;

  font_desc = pango_context_get_font_description (line->context);

  if (line->data->attrs)
    {
      PangoAttrIterator iter;
      int start, end;

      pango_attr_list_init_iterator (line->data->attrs, &iter);

      do
        {
          pango_attr_iterator_range (&iter, &start, &end);

          if (start <= line->start_index && line->start_index < end)
            {
              PangoAttribute *attr;

              if (!free_font_desc)
                {
                  font_desc = pango_font_description_copy_static (font_desc);
                  free_font_desc = TRUE;
                }

              pango_attr_iterator_get_font (&iter, font_desc, NULL, NULL);

              attr = pango_attr_iterator_get (&iter, PANGO_ATTR_LINE_HEIGHT);
              if (attr)
                line_height_factor = attr->double_value;

              attr = pango_attr_iterator_get (&iter, PANGO_ATTR_ABSOLUTE_LINE_HEIGHT);
              if (attr)
                absolute_line_height = attr->int_value;

              break;
            }
        }
      while (pango_attr_iterator_next (&iter));

      pango_attr_iterator_clear (&iter);
    }

  memset (logical_rect, 0, sizeof (PangoRectangle));

  font = pango_context_load_font (line->context, font_desc);
  if (font)
    {
      PangoFontMetrics *metrics;

      metrics = pango_font_get_metrics (font, pango_context_get_language (line->context));
      if (metrics)
        {
          logical_rect->y = - pango_font_metrics_get_ascent (metrics);
          logical_rect->height = - logical_rect->y + pango_font_metrics_get_descent (metrics);

          if (trim != PANGO_LEADING_TRIM_BOTH)
            {
              int leading;

              if (absolute_line_height != 0 || line_height_factor != 0.0)
                {
                  int line_height;

                  line_height = MAX (absolute_line_height, ceilf (line_height_factor * logical_rect->height));

                  leading = line_height - logical_rect->height;
                }
              else
                {
                  leading = MAX (metrics->height - (metrics->ascent + metrics->descent), 0);
                }

              if ((trim & PANGO_LEADING_TRIM_START) == 0)
                logical_rect->y -= leading / 2;
              if (trim == PANGO_LEADING_TRIM_NONE)
                logical_rect->height += leading;
              else
                logical_rect->height += (leading - leading / 2);
            }

          pango_font_metrics_unref (metrics);
        }

      g_object_unref (font);
   }

  if (free_font_desc)
    pango_font_description_free (font_desc);
}

/*< private >
 * pango_line_new:
 * @context: the `PangoContext`
 * @data: the `LineData`
 *
 * Creates a new `PangoLine`.
 *
 * The line shares the immutable `LineData` with other lines.
 *
 * The @context is needed for shape rendering.
 *
 * Returns: new `PangoLine`
 */
PangoLine *
pango_line_new (PangoContext *context,
                LineData     *data)
{
  PangoLine *line;

  line = g_new0 (PangoLine, 1);
  line->context = g_object_ref (context);
  line->data = line_data_ref (data);

  return line;
}

/*< private >
 * pango_line_index_to_run:
 * @line: a `PangoLine`
 * @idx: a byte offset in the line
 * @run: (out): return location for the run
 *
 * Finds the run in @line which contains @idx.
 */
void
pango_line_index_to_run (PangoLine  *line,
                         int         idx,
                         PangoRun  **run)
{
  *run = NULL;

  for (GSList *l = line->runs; l; l = l->next)
    {
      PangoRun *r = l->data;
      PangoItem *item;

      item = pango_run_get_glyph_item (r)->item;
      if (item->offset <= idx && idx < item->offset + item->length)
        {
          *run = r;
          break;
        }
    }
}

/* }}} */
/* {{{ Public API */

PangoLine *
pango_line_copy (PangoLine *line)
{
  PangoLine *copy;

  if (line == NULL)
    return NULL;

  copy = g_new0 (PangoLine, 1);
  copy->context = g_object_ref (line->context);
  copy->data = line_data_ref (line->data);
  copy->start_index = line->start_index;
  copy->length = line->length;
  copy->start_offset = line->start_offset;
  copy->n_chars = line->n_chars;
  copy->wrapped = line->wrapped;
  copy->ellipsized = line->ellipsized;
  copy->hyphenated = line->hyphenated;
  copy->justified = TRUE;
  copy->starts_paragraph = line->starts_paragraph;
  copy->ends_paragraph = line->ends_paragraph;
  copy->has_extents = FALSE;
  copy->direction = line->direction;
  copy->runs = g_slist_copy_deep (line->runs, (GCopyFunc) pango_glyph_item_copy, NULL);
  copy->n_runs = line->n_runs;

  return copy;
}

void
pango_line_free (PangoLine *line)
{
  g_object_unref (line->context);
  line_data_unref (line->data);
  g_slist_free_full (line->runs, (GDestroyNotify)pango_glyph_item_free);
  g_free (line->run_array);
  g_free (line);
}

/* {{{ Simple getters */

/**
 * pango_line_get_run_count:
 * @line: a `PangoLine`
 *
 * Gets the number of runs in the line.
 *
 * Returns: the number of runs
 */
int
pango_line_get_run_count (PangoLine *line)
{
  g_return_val_if_fail (line != NULL, 0);

  pango_line_get_runs (line);

  return line->n_runs;
}

/**
 * pango_line_get_runs:
 * @line: a `PangoLine`
 *
 * Gets the runs of the line.
 *
 * Note that the returned list and its contents
 * are owned by Pango and must not be modified.
 *
 * The length of the returned array can be obtained
 * with [method@Pango.Line.get_run_count].
 *
 * Returns: (transfer none): an array of `PangoRun`
 */
PangoRun **
pango_line_get_runs (PangoLine *line)
{
  g_return_val_if_fail (line != NULL, NULL);

  if (!line->run_array)
    {
      GSList *l;
      int i;

      line->n_runs = g_slist_length (line->runs);

      line->run_array = g_new (PangoRun *, line->n_runs);
      for (l = line->runs, i = 0; l; l = l->next, i++)
        line->run_array[i] = l->data;
    }

  return line->run_array;
}

/**
 * pango_line_get_text:
 * @line: a `PangoLine`
 * @start_index: the byte index of the first byte of @line
 * @length: the number of bytes in @line
 *
 * Gets the text that @line presents.
 *
 * The `PangoLine` represents the slice from @start_index
 * to @start_index + @length of the returned string.
 *
 * The returned string is owned by @line and must not
 * be modified.
 *
 * Returns: the text
 */
const char *
pango_line_get_text (PangoLine *line,
                     int       *start_index,
                     int       *length)
{
  g_return_val_if_fail (line != NULL, NULL);
  g_return_val_if_fail (start_index != NULL, NULL);
  g_return_val_if_fail (length != NULL, NULL);

  *start_index = line->start_index;
  *length = line->length;

  return line->data->text;
}

/**
 * pango_line_get_start_index:
 * @line: a `PangoLine`
 *
 * Returns the start index of the line, as byte index
 * into the text of the layout.
 *
 * Returns: the start index of the line
 */
int
pango_line_get_start_index (PangoLine *line)
{
  g_return_val_if_fail (line != NULL, 0);

  return line->start_index;
}

/**
 * pango_line_get_length:
 * @line: a `PangoLine`
 *
 * Returns the length of the line, in bytes.
 *
 * Returns: the length of the line
 */
int
pango_line_get_length (PangoLine *line)
{
  g_return_val_if_fail (line != NULL, 0);

  return line->length;
}

/**
 * pango_line_get_log_attrs:
 * @line: a `PangoLine`
 * @start_offset: the character offset of the first character of @line
 * @n_attrs: the number of attributes that apply to @line
 *
 * Gets the `PangoLogAttr` array for the line.
 *
 * The `PangoLogAttrs` for @line are the slice from @start_offset
 * to @start_offset+@n_attrs of the returned array. @n_attrs is
 * be the number of characters plus one.
 *
 * The returned array is owned by @line and must not be modified.
 *
 * Returns: the `PangoLogAttr` array
 */
const PangoLogAttr *
pango_line_get_log_attrs (PangoLine *line,
                          int       *start_offset,
                          int       *n_attrs)
{
  g_return_val_if_fail (line != NULL, NULL);
  g_return_val_if_fail (start_offset != NULL, NULL);
  g_return_val_if_fail (n_attrs != NULL, NULL);

  *start_offset = line->start_offset;
  *n_attrs = line->n_chars + 1;

  return line->data->log_attrs;
}

/**
 * pango_line_is_wrapped:
 * @line: a `PangoLine`
 *
 * Gets whether the line is wrapped.
 *
 * Returns: `TRUE` if @line has been wrapped
 */
gboolean
pango_line_is_wrapped (PangoLine *line)
{
  g_return_val_if_fail (line != NULL, FALSE);

  return line->wrapped;
}

/**
 * pango_line_is_ellipsized:
 * @line: a `PangoLine`
 *
 * Gets whether the line is ellipsized.
 *
 * Returns: `TRUE` if @line has been ellipsized
 */
gboolean
pango_line_is_ellipsized (PangoLine *line)
{
  g_return_val_if_fail (line != NULL, FALSE);

  return line->ellipsized;
}

/**
 * pango_line_is_hyphenated:
 * @line: a `PangoLine`
 *
 * Gets whether the line is hyphenated.
 *
 * Returns: `TRUE` if @line has been hyphenated
 */
gboolean
pango_line_is_hyphenated (PangoLine *line)
{
  g_return_val_if_fail (line != NULL, FALSE);

  return line->hyphenated;
}

/**
 * pango_line_is_justified:
 * @line: a `PangoLine`
 *
 * Gets whether the line is justified.
 *
 * See [method@Pango.Line.justify].
 *
 * Returns: `TRUE` if @line has been justified
 */
gboolean
pango_line_is_justified (PangoLine *line)
{
  g_return_val_if_fail (line != NULL, FALSE);

  return line->justified;
}

/**
 * pango_line_is_paragraph_start:
 * @line: a `PangoLine`
 *
 * Gets whether the line is the first of a paragraph.
 *
 * Returns: `TRUE` if @line starts a paragraph
 */
gboolean
pango_line_is_paragraph_start (PangoLine *line)
{
  g_return_val_if_fail (line != NULL, FALSE);

  return line->starts_paragraph;
}

/**
 * pango_line_is_paragraph_end:
 * @line: a `PangoLine`
 *
 * Gets whether the line is the last of a paragraph.
 *
 * Returns: `TRUE` if @line ends a paragraph
 */
gboolean
pango_line_is_paragraph_end (PangoLine *line)
{
  g_return_val_if_fail (line != NULL, FALSE);

  return line->ends_paragraph;
}

/**
 * pango_line_get_resolved_direction:
 * @line: a `PangoLine`
 *
 * Gets the resolved direction of the line.
 *
 * Returns: the resolved direction of @line
 */
PangoDirection
pango_line_get_resolved_direction (PangoLine *line)
{
  g_return_val_if_fail (line != NULL, PANGO_DIRECTION_LTR);

  return line->direction;
}

/* }}} */
/* {{{ Justification */

/**
 * pango_line_justify:
 * @line: (transfer full): a `PangoLine`
 * @width: the width to justify @line to
 *
 * Creates a new `PangoLine` that is justified
 * copy of @line.
 *
 * The content of the returned line is justified
 * to fill the given width, by modifying inter-word
 * spaces (and possibly intra-word spaces too).
 *
 * Note that this function consumes @line.
 *
 * Returns: (transfer full): a new `PangoLine`
 */
PangoLine *
pango_line_justify (PangoLine *line,
                    int        width)
{
  int remaining_width;
  PangoLine *copy;

  g_return_val_if_fail (line != NULL, NULL);

  remaining_width = width - pango_line_compute_width (line);
  if (remaining_width <= 0)
    return line;

  copy = pango_line_new (line->context, line->data);
  copy->start_index = line->start_index;
  copy->length = line->length;
  copy->start_offset = line->start_offset;
  copy->n_chars = line->n_chars;
  copy->wrapped = line->wrapped;
  copy->ellipsized = line->ellipsized;
  copy->hyphenated = line->hyphenated;
  copy->justified = TRUE;
  copy->starts_paragraph = line->starts_paragraph;
  copy->ends_paragraph = line->ends_paragraph;
  copy->has_extents = FALSE;
  copy->direction = line->direction;
  copy->runs = line->runs;
  line->runs = NULL;

  justify_words (copy, &remaining_width);

  pango_line_free (line);

  return copy;
}

/* }}} */
/* {{{ Extents */

/**
 * pango_line_get_extents:
 * @line: a `PangoLine`
 * @ink_rect: (out) (optional): rectangle that will be filled with ink extents
 * @logical_rect: (out) (optional): rectangle that will be filled with the logical extents
 *
 * Gets the extents of the line.
 *
 * The logical extents returned by this function always include leading.
 * If you need extents with trimmed leading, use [method@Pango.Line.get_trimmed_extents].
 *
 * Note that the origin is at the left end of the baseline.
 *
 * Pango is following CSS in splitting the external leading, and giving one half of it
 * to the line above, and the other half the the line below. Unless the line height is set
 * via attributes, the external leading is determined as the difference between the
 * height and ascent + descent in font metrics:
 *
 * <picture>
 *   <source srcset="line-height1-dark.png" media="(prefers-color-scheme: dark)">
 *   <img alt="Pango Font Metrics" src="line-height1-light.png">
 * </picture>
 *
 * If spacing is set, it also gets split, for the purpose of determining the
 * logical extents.
 *
 * <picture>
 *   <source srcset="line-height2-dark.png" media="(prefers-color-scheme: dark)">
 *   <img alt="Pango Extents and Spacing" src="line-height2-light.png">
 * </picture>
 *
 * If line height is set, it determines the logical extents.
 *
 * <picture>
 *   <source srcset="line-height3-dark.png" media="(prefers-color-scheme: dark)">
 *   <img alt="Pango Extents and Line Height" src="line-height3-light.png">
 * </picture>
 */
void
pango_line_get_extents (PangoLine      *line,
                        PangoRectangle *ink_rect,
                        PangoRectangle *logical_rect)
{
  PangoRectangle ink = { 0, };
  PangoRectangle logical  = { 0, };

  if (line->has_extents)
    goto cached;

  compute_extents (line, PANGO_LEADING_TRIM_NONE, &ink, &logical);

  line->ink_rect = ink;
  line->logical_rect = logical;
  line->has_extents = TRUE;

cached:
  if (ink_rect)
    *ink_rect = line->ink_rect;
  if (logical_rect)
    *logical_rect = line->logical_rect;
}

/**
 * pango_line_get_trimmed_extents:
 * @line: a `PangoLine`
 * @trim: `PangoLeadingTrim` flags
 * @logical_rect: (out): rectangle that will be filled with the logical extents
 *
 * Gets trimmed logical extents of the line.
 *
 * The @trim flags specify if line-height attributes are taken
 * into consideration for determining the logical height. See the
 * [CSS inline layout](https://www.w3.org/TR/css-inline-3/#inline-height)
 * specification for details.
 *
 * Note that the origin is at the left end of the baseline.
 */
void
pango_line_get_trimmed_extents (PangoLine        *line,
                                PangoLeadingTrim  trim,
                                PangoRectangle   *logical_rect)
{
  PangoRectangle ink = { 0, };

  if (line->has_extents && trim == PANGO_LEADING_TRIM_NONE)
    {
      *logical_rect = line->logical_rect;
      return;
    }

  compute_extents (line, trim, &ink, logical_rect);
}

/* }}} */
/* {{{ Editing API */

/**
 * pango_line_layout_index_to_pos:
 * @line: a `PangoLine`
 * @idx: byte index within @line
 * @pos: (out): rectangle in which to store the position of the grapheme
 *
 * Converts from an index within a `PangoLine` to the
 * position corresponding to the grapheme at that index.
 *
 * The return value is represented as rectangle. Note that `pos->x` is
 * always the leading edge of the grapheme and `pos->x + pos->width` the
 * trailing edge of the grapheme. If the directionality of the grapheme
 * is right-to-left, then `pos->width` will be negative.
 *
 * Note that @idx is allowed to be @line->start_index + @line->length.
 */
void
pango_line_index_to_pos (PangoLine      *line,
                         int             idx,
                         PangoRectangle *pos)
{
  PangoRectangle run_logical;
  PangoRectangle line_logical;
  PangoRun *run = NULL;
  int x_pos;

  pango_line_get_extents (line, NULL, &line_logical);

  if (!line->runs)
    {
      *pos = line_logical;
      return;
    }

  if (idx == line->start_index + line->length)
    run = g_slist_last (line->runs)->data;
  else
    pango_line_index_to_run (line, idx, &run);

  pango_run_get_extents (run, PANGO_LEADING_TRIM_BOTH, NULL, &run_logical);

  pos->y = run_logical.y;
  pos->height = run_logical.height;

  /* FIXME: avoid iterating through the runs multiple times */

  pango_line_index_to_x (line, idx, 0, &x_pos);
  pos->x = line_logical.x + x_pos;

  if (idx < line->start_index + line->length)
    {
      pango_line_index_to_x (line, idx, 1, &x_pos);
      pos->width = (line_logical.x + x_pos) - pos->x;
    }
  else
    pos->width  = 0;
}

/**
 * pango_line_index_to_x:
 * @line: a `PangoLine`
 * @idx: byte index within @line
 * @trailing: an integer indicating the edge of the grapheme to retrieve
 *   the position of. If > 0, the trailing edge of the grapheme,
 *   if 0, the leading of the grapheme
 * @x_pos: (out): location to store the x_offset (in Pango units)
 *
 * Converts an index within a `PangoLine` to a X position.
 *
 * Note that @idx is allowed to be @line->start_index + @line->length.
 */
void
pango_line_index_to_x (PangoLine *line,
                       int        index,
                       int        trailing,
                       int       *x_pos)
{
  GSList *run_list = line->runs;
  int width = 0;

  while (run_list)
    {
      PangoGlyphItem *run = run_list->data;

      if (run->item->offset <= index && run->item->offset + run->item->length > index)
        {
          int offset = g_utf8_pointer_to_offset (line->data->text, line->data->text + index);
          int attr_offset;

          if (trailing)
            {
              while (index < line->start_index + line->length &&
                     offset + 1 < line->data->n_chars &&
                     !line->data->log_attrs[offset + 1].is_cursor_position)
                {
                  offset++;
                  index = g_utf8_next_char (line->data->text + index) - line->data->text;
                }
            }
          else
            {
              while (index > line->start_index &&
                     !line->data->log_attrs[offset].is_cursor_position)
                {
                  offset--;
                  index = g_utf8_prev_char (line->data->text + index) - line->data->text;
                }
            }

          attr_offset = run->item->char_offset;
          pango_glyph_string_index_to_x_full (run->glyphs,
                                              line->data->text + run->item->offset,
                                              run->item->length,
                                              &run->item->analysis,
                                              line->data->log_attrs + attr_offset,
                                              index - run->item->offset, trailing, x_pos);
          if (x_pos)
            *x_pos += width;

          return;
        }

      width += pango_glyph_string_get_width (run->glyphs);

      run_list = run_list->next;
    }

  if (x_pos)
    *x_pos = width;
}

/**
 * pango_line_x_to_index:
 * @line: a `PangoLine`
 * @x: the X offset (in Pango units) from the left edge of the line
 * @idx: (out): location to store calculated byte index for the grapheme
 *   in which the user clicked
 * @trailing: (out): location to store an integer indicating where in the
 *   grapheme the user clicked. It will either be zero, or the number of
 *   characters in the grapheme. 0 represents the leading edge of the grapheme.
 *
 * Converts from x offset to the byte index of the corresponding character
 * within the text of the line.
 *
 * If @x is outside the line, @idx and @trailing will point to the very
 * first or very last position in the line. This determination is based on the
 * resolved direction of the paragraph; for example, if the resolved direction
 * is right-to-left, then an X position to the right of the line (after it)
 * results in 0 being stored in @idx and @trailing. An X position to the
 * left of the line results in @idx pointing to the (logical) last grapheme
 * in the line and @trailing being set to the number of characters in that
 * grapheme. The reverse is true for a left-to-right line.
 *
 * Return value: %FALSE if @x_pos was outside the line, %TRUE if inside
 */
gboolean
pango_line_x_to_index (PangoLine *line,
                       int        x_pos,
                       int       *index,
                       int       *trailing)
{
  GSList *tmp_list;
  gint start_pos = 0;
  gint first_index = 0; /* line->start_index */
  gint first_offset;
  gint last_index;      /* start of last grapheme in line */
  gint last_offset;
  gint end_index;       /* end iterator for line */
  gint end_offset;      /* end iterator for line */
  gint last_trailing;
  gboolean suppress_last_trailing;

  /* Find the last index in the line */
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

  first_offset = g_utf8_pointer_to_offset (line->data->text, line->data->text + line->start_index);

  end_index = first_index + line->length;
  end_offset = first_offset + g_utf8_pointer_to_offset (line->data->text + first_index, line->data->text + end_index);

  last_index = end_index;
  last_offset = end_offset;
  last_trailing = 0;
  do
    {
      last_index = g_utf8_prev_char (line->data->text + last_index) - line->data->text;
      last_offset--;
      last_trailing++;
    }
  while (last_offset > first_offset && !line->data->log_attrs[last_offset].is_cursor_position);

  /* This is a HACK. If a program only keeps track of cursor (etc)
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
  if (line->wrapped)
    suppress_last_trailing = TRUE;
  else
    suppress_last_trailing = FALSE;

  if (x_pos < 0)
    {
      /* pick the leftmost char */
      if (index)
        *index = (line->direction == PANGO_DIRECTION_LTR) ? first_index : last_index;
      /* and its leftmost edge */
      if (trailing)
        *trailing = (line->direction == PANGO_DIRECTION_LTR || suppress_last_trailing) ? 0 : last_trailing;

      return FALSE;
    }

  tmp_list = line->runs;
  while (tmp_list)
    {
      PangoGlyphItem *run = tmp_list->data;
      int logical_width;

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

          pango_glyph_string_x_to_index (run->glyphs,
                                         line->data->text + run->item->offset, run->item->length,
                                         &run->item->analysis,
                                         x_pos - start_pos,
                                         &pos, &char_trailing);

          char_index = run->item->offset + pos;

          /* Convert from characters to graphemes */

          offset = g_utf8_pointer_to_offset (line->data->text, line->data->text + char_index);

          grapheme_start_offset = offset;
          grapheme_start_index = char_index;
          while (grapheme_start_offset > first_offset &&
                 !line->data->log_attrs[grapheme_start_offset].is_cursor_position)
            {
              grapheme_start_index = g_utf8_prev_char (line->data->text + grapheme_start_index) - line->data->text;
              grapheme_start_offset--;
            }

          grapheme_end_offset = offset;
          do
            {
              grapheme_end_offset++;
            }

          while (grapheme_end_offset < end_offset &&
                 !line->data->log_attrs[grapheme_end_offset].is_cursor_position);

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
    *index = (line->direction == PANGO_DIRECTION_LTR) ? last_index : first_index;

  /* and its rightmost edge */
  if (trailing)
    *trailing = (line->direction == PANGO_DIRECTION_LTR && !suppress_last_trailing) ? last_trailing : 0;

  return FALSE;
}

/* }}} */
/* {{{ Cursor positioning */

/**
 * pango_line_get_cursor_pos:
 * @line: a `PangoLine`
 * @idx: the byte index of the cursor
 * @strong_pos: (out) (optional): location to store the strong cursor position
 * @weak_pos: (out) (optional): location to store the weak cursor position
 *
 * Given an index within @line, determines the positions that of the
 * strong and weak cursors if the insertion point is at that index.
 *
 * Note that @idx is allowed to be @line->start_index + @line->length.
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
pango_line_get_cursor_pos (PangoLine      *line,
                           int             idx,
                           PangoRectangle *strong_pos,
                           PangoRectangle *weak_pos)
{
  PangoRectangle line_rect = { 666, };
  PangoRectangle run_rect = { 666, };
  PangoDirection dir1, dir2;
  int level1, level2;
  PangoRun *run = NULL;
  int x1_trailing;
  int x2;

  if (idx >= line->start_index + line->length)
    {
      if (line->runs)
        run = g_slist_last (line->runs)->data;
    }
  else
    pango_line_index_to_run (line, idx, &run);

  pango_line_get_extents (line, NULL, &line_rect);
  if (run)
    pango_run_get_extents (run, PANGO_LEADING_TRIM_BOTH, NULL, &run_rect);
  else
    {
      run_rect = line_rect;
      x1_trailing = x2 = line_rect.width;
      goto done;
    }

  /* Examine the trailing edge of the character before the cursor */
  if (idx == line->start_index)
    {
      dir1 = line->direction;
      level1 = dir1 == PANGO_DIRECTION_LTR ? 0 : 1;
      if (line->direction == PANGO_DIRECTION_LTR)
        x1_trailing = 0;
      else
        x1_trailing = line_rect.width;
    }
  else
    {
      int prev_index = g_utf8_prev_char (line->data->text + idx) - line->data->text;

      if (prev_index >= line->start_index + line->length)
        {
          dir1 = line->direction;
          level1 = dir1 == PANGO_DIRECTION_LTR ? 0 : 1;
          x1_trailing = line_rect.width;
        }
      else
        {
          PangoRun *prev_run;

          pango_line_index_to_run (line, prev_index, &prev_run);
          level1 = pango_run_get_glyph_item (prev_run)->item->analysis.level;
          dir1 = level1 % 2 ? PANGO_DIRECTION_RTL : PANGO_DIRECTION_LTR;
          pango_line_index_to_x (line, prev_index, TRUE, &x1_trailing);
        }
    }

  /* Examine the leading edge of the character after the cursor */
  if (idx >= line->start_index + line->length)
    {
      dir2 = line->direction;
      level2 = dir2 == PANGO_DIRECTION_LTR ? 0 : 1;
      if (line->direction == PANGO_DIRECTION_LTR)
        x2 = line_rect.width;
      else
        x2 = 0;
    }
  else
    {
      pango_line_index_to_x (line, idx, FALSE, &x2);
      level2 = pango_run_get_glyph_item (run)->item->analysis.level;
      dir2 = level2 % 2 ? PANGO_DIRECTION_RTL : PANGO_DIRECTION_LTR;
    }

done:
  if (strong_pos)
    {
      strong_pos->x = line_rect.x;

      if (dir1 == line->direction &&
          (dir2 != dir1 || level1 < level2))
        strong_pos->x += x1_trailing;
      else
        strong_pos->x += x2;

      strong_pos->y = run_rect.y;
      strong_pos->width = 0;
      strong_pos->height = run_rect.height;
    }

  if (weak_pos)
    {
      weak_pos->x = line_rect.x;

      if (dir1 == line->direction &&
          (dir2 != dir1 || level1 < level2))
        weak_pos->x += x2;
      else
        weak_pos->x += x1_trailing;

      weak_pos->y = run_rect.y;
      weak_pos->width = 0;
      weak_pos->height = run_rect.height;
    }
}

/**
 * pango_line_get_caret_pos:
 * @line: a `PangoLine`
 * @idx: the byte index of the cursor
 * @strong_pos: (out) (optional): location to store the strong cursor position
 * @weak_pos: (out) (optional): location to store the weak cursor position
 *
 * Given an index within @line, determines the positions of the
 * strong and weak cursors if the insertion point is at that index.
 *
 * Note that @idx is allowed to be @line->start_index + @line->length.
 *
 * This is a variant of [method@Pango.Line.get_cursor_pos] that applies
 * font metric information about caret slope and offset to the positions
 * it returns.
 *
 * <picture>
 *   <source srcset="caret-metrics-dark.png" media="(prefers-color-scheme: dark)">
 *   <img alt="Caret metrics" src="caret-metrics-light.png">
 * </picture>
 */
void
pango_line_get_caret_pos (PangoLine      *line,
                          int             idx,
                          PangoRectangle *strong_pos,
                          PangoRectangle *weak_pos)
{
  PangoRun *run = NULL;
  PangoGlyphItem *glyph_item;
  hb_font_t *hb_font;
  hb_position_t caret_offset, caret_run, caret_rise, descender;

  pango_line_get_cursor_pos (line, idx, strong_pos, weak_pos);

  if (idx >= line->start_index + line->length)
    {
      if (line->runs)
        run = g_slist_last (line->runs)->data;
    }
  else
    pango_line_index_to_run (line, idx, &run);

  if (!run)
    return;

  glyph_item = pango_run_get_glyph_item (run);
  hb_font = pango_font_get_hb_font (glyph_item->item->analysis.font);

  if (hb_ot_metrics_get_position (hb_font, HB_OT_METRICS_TAG_HORIZONTAL_CARET_RISE, &caret_rise) &&
      hb_ot_metrics_get_position (hb_font, HB_OT_METRICS_TAG_HORIZONTAL_CARET_RUN, &caret_run) &&
      hb_ot_metrics_get_position (hb_font, HB_OT_METRICS_TAG_HORIZONTAL_CARET_OFFSET, &caret_offset) &&
      hb_ot_metrics_get_position (hb_font, HB_OT_METRICS_TAG_HORIZONTAL_DESCENDER, &descender))
    {
      double slope_inv;
      int x_scale, y_scale;

      if (strong_pos)
        strong_pos->x += caret_offset;

      if (weak_pos)
        weak_pos->x += caret_offset;

      if (caret_rise == 0)
        return;

      hb_font_get_scale (hb_font, &x_scale, &y_scale);
      slope_inv = (caret_run * y_scale) / (double) (caret_rise * x_scale);

      if (strong_pos)
        {
          strong_pos->x += descender * slope_inv;
          strong_pos->width = strong_pos->height * slope_inv;
          if (slope_inv < 0)
            strong_pos->x -= strong_pos->width;
        }

      if (weak_pos)
        {
          weak_pos->x += descender * slope_inv;
          weak_pos->width = weak_pos->height * slope_inv;
          if (slope_inv < 0)
            weak_pos->x -= weak_pos->width;
        }
    }
}

/* }}} */
/* }}} */

/* vim:set foldmethod=marker expandtab: */
