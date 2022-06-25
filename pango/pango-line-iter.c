#include "config.h"

#include "pango-line-iter-private.h"
#include "pango-lines-private.h"
#include "pango-line-private.h"
#include "pango-run-private.h"

/**
 * Pango2LineIter:
 *
 * A `Pango2LineIter` can be used to iterate over the visual
 * extents of a `Pango2Layout` or `Pango2Lines`.
 *
 * To obtain a `Pango2LineIter`, use [method@Pango2.Layout.get_iter]
 * or [method@Pango2.Lines.get_iter].
 *
 * The `Pango2LineIter` structure is opaque, and has no user-visible
 * fields.
 */


/* {{{ Pango2LineIter implementation */

struct _Pango2LineIter
{
  Pango2Lines *lines;
  guint serial;

  int line_no;
  Position line_pos;
  Pango2Line *line;
  GSList *run_link;
  Pango2Run *run;
  int index;

  /* run handling */
  int run_x;
  int run_width;
  int end_x_offset;
  gboolean ltr;

  /* cluster handling */
  int cluster_x;
  int cluster_width;
  int cluster_start;
  int next_cluster_glyph;
  int cluster_num_chars;

  int character_position;
};

G_DEFINE_BOXED_TYPE (Pango2LineIter, pango2_line_iter,
                     pango2_line_iter_copy, pango2_line_iter_free);


/* }}} */
/* {{{ Utilities */

#define ITER_IS_VALID(iter) ((iter)->serial == (iter)->lines->serial)

static gboolean
line_is_terminated (Pango2LineIter *iter)
{
  if (iter->line_no + 1 < pango2_lines_get_line_count (iter->lines))
    return pango2_line_is_paragraph_end (iter->line);

  return FALSE;

}

static int
next_cluster_start (Pango2GlyphString *glyphs,
                    int                cluster_start)
{
  int i;

  i = cluster_start + 1;
  while (i < glyphs->num_glyphs)
    {
      if (glyphs->glyphs[i].attr.is_cluster_start)
        return i;

      i++;
    }

  return glyphs->num_glyphs;
}

static int
cluster_width (Pango2GlyphString *glyphs,
               int                cluster_start)
{
  int i;
  int width;

  width = glyphs->glyphs[cluster_start].geometry.width;
  i = cluster_start + 1;
  while (i < glyphs->num_glyphs)
    {
      if (glyphs->glyphs[i].attr.is_cluster_start)
        break;

      width += glyphs->glyphs[i].geometry.width;
      i++;
    }

  return width;
}

/* Sets up the iter for the start of a new cluster. cluster_start_index
 * is the byte index of the cluster start relative to the run.
 */
static void
update_cluster (Pango2LineIter *iter,
                int             cluster_start_index)
{
  Pango2GlyphItem *glyph_item;
  char *cluster_text;
  int  cluster_length;
  Pango2Run *run = iter->run_link->data;

  glyph_item = pango2_run_get_glyph_item (run);

  iter->character_position = 0;

  iter->cluster_width = cluster_width (glyph_item->glyphs, iter->cluster_start);
  iter->next_cluster_glyph = next_cluster_start (glyph_item->glyphs, iter->cluster_start);

  if (iter->ltr)
    {
      /* For LTR text, finding the length of the cluster is easy
       * since logical and visual runs are in the same direction.
       */
      if (iter->next_cluster_glyph < glyph_item->glyphs->num_glyphs)
        cluster_length = glyph_item->glyphs->log_clusters[iter->next_cluster_glyph] - cluster_start_index;
      else
        cluster_length = glyph_item->item->length - cluster_start_index;
    }
  else
    {
      /* For RTL text, we have to scan backwards to find the previous
       * visual cluster which is the next logical cluster.
       */
      int i = iter->cluster_start;
      while (i > 0 && glyph_item->glyphs->log_clusters[i - 1] == cluster_start_index)
        i--;

      if (i == 0)
        cluster_length = glyph_item->item->length - cluster_start_index;
      else
        cluster_length = glyph_item->glyphs->log_clusters[i - 1] - cluster_start_index;
    }

  cluster_text = iter->line->data->text + glyph_item->item->offset + cluster_start_index;
  iter->cluster_num_chars = g_utf8_strlen (cluster_text, cluster_length);

  if (iter->ltr)
    iter->index = cluster_text - iter->line->data->text;
  else
    iter->index = g_utf8_prev_char (cluster_text + cluster_length) - iter->line->data->text;
}

/* Moves to the next non-empty line. If @include_terminators
 * is set, a line with just an explicit paragraph separator
 * is considered non-empty.
 */
static gboolean
next_nonempty_line (Pango2LineIter *iter,
                    gboolean        include_terminators)
{
  gboolean result;

  while (TRUE)
    {
      result = pango2_line_iter_next_line (iter);
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
next_nonempty_run (Pango2LineIter *iter,
                   gboolean        include_terminators)
{
  gboolean result;

  while (TRUE)
    {
      result = pango2_line_iter_next_run (iter);
      if (!result)
        break;

      if (iter->run)
        break;

      if (include_terminators && line_is_terminated (iter))
        break;
    }

  return result;
}

/* Like pango2_layout_next_cluster(), but if @include_terminators
 * is set, includes the fake runs/clusters for empty lines.
 * (But not positions introduced by line wrapping).
 */
static gboolean
next_cluster_internal (Pango2LineIter *iter,
                       gboolean        include_terminators)
{
  Pango2GlyphItem *glyph_item;
  Pango2Run *run;

  if (iter->run_link == NULL)
    return next_nonempty_line (iter, include_terminators);

  run = iter->run_link->data;
  glyph_item = pango2_run_get_glyph_item (run);

  if (iter->next_cluster_glyph == glyph_item->glyphs->num_glyphs)
    {
      return next_nonempty_run (iter, include_terminators);
    }
  else
    {
      iter->cluster_start = iter->next_cluster_glyph;
      iter->cluster_x += iter->cluster_width;
      update_cluster (iter, glyph_item->glyphs->log_clusters[iter->cluster_start]);

      return TRUE;
    }
}

static void
update_run (Pango2LineIter *iter,
            int             start_index)
{
  if (iter->run)
    {
      Pango2GlyphItem *glyph_item = pango2_run_get_glyph_item (iter->run);

      if (iter->run_link == iter->line->runs)
        iter->run_x = 0;
      else
        iter->run_x += iter->run_width + iter->end_x_offset + glyph_item->start_x_offset;

      iter->run_width = pango2_glyph_string_get_width (glyph_item->glyphs);
      iter->end_x_offset = glyph_item->end_x_offset;
      iter->ltr = (glyph_item->item->analysis.level % 2) == 0;
      iter->cluster_start = 0;
      iter->cluster_x = iter->run_x;
      update_cluster (iter, glyph_item->glyphs->log_clusters[0]);
    }
  else
    {
      /* The empty run at the end of a line */
      iter->run_x = 0;

      iter->run_width = 0;
      iter->end_x_offset = 0;
      iter->ltr = TRUE;
      iter->cluster_start = 0;
      iter->cluster_x = iter->run_x;
      iter->cluster_width = 0;
      iter->character_position = 0;
      iter->cluster_num_chars = 0;
      iter->index = start_index;
    }
}

static inline void
offset_line (Pango2LineIter  *iter,
             Pango2Rectangle *ink_rect,
             Pango2Rectangle *logical_rect)
{
  if (ink_rect)
    {
      ink_rect->x += iter->line_pos.x;
      ink_rect->y += iter->line_pos.y;
    }
  if (logical_rect)
    {
      logical_rect->x += iter->line_pos.x;
      logical_rect->y += iter->line_pos.y;
    }
}

static inline void
offset_run (Pango2LineIter  *iter,
            Pango2Rectangle *ink_rect,
            Pango2Rectangle *logical_rect)
{
  if (ink_rect)
    ink_rect->x += iter->run_x;
  if (logical_rect)
    logical_rect->x += iter->run_x;
}

/* }}} */
/* {{{ Private API */

Pango2LineIter *
pango2_line_iter_new (Pango2Lines *lines)
{
  Pango2LineIter *iter;
  int run_start_index;

  g_return_val_if_fail (PANGO2_IS_LINES (lines), NULL);

  iter = g_new0 (Pango2LineIter, 1);

  iter->lines = g_object_ref (lines);
  iter->serial = pango2_lines_get_serial (lines);

  iter->line_no = 0;
  iter->line = g_ptr_array_index (lines->lines, 0);
  iter->line_pos = g_array_index (lines->positions, Position, 0);
  iter->run_link = iter->line->runs;
  if (iter->run_link)
    {
      iter->run = iter->run_link->data;
      run_start_index = pango2_run_get_glyph_item (iter->run)->item->offset;
    }
  else
    {
      iter->run = NULL;
      run_start_index = 0;
    }

  update_run (iter, run_start_index);

  return iter;
}

/* }}} */
/* {{{ Public API */

/**
 * pango2_line_iter_copy:
 * @iter: (nullable): a `Pango2LineIter`
 *
 * Copies a `Pango2LineIter`.
 *
 * Return value: (nullable): the newly allocated `Pango2LineIter`
 */
Pango2LineIter *
pango2_line_iter_copy (Pango2LineIter *iter)
{
  Pango2LineIter *copy;

  if (iter == NULL)
    return NULL;

  copy = g_new0 (Pango2LineIter, 1);
  memcpy (iter, copy, sizeof (Pango2LineIter));
  g_object_ref (copy->lines);

  return copy;
}

/**
 * pango2_line_iter_free:
 * @iter: (nullable): a `Pango2LineIter`
 *
 * Frees an iterator that's no longer in use.
 */
void
pango2_line_iter_free (Pango2LineIter *iter)
{
  if (iter == NULL)
    return;

  g_object_unref (iter->lines);
  g_free (iter);
}

/**
 * pango2_line_iter_get_lines:
 * @iter: a `Pango2LineIter`
 *
 * Gets the `Pango2Lines` object associated with a `Pango2LineIter`.
 *
 * Return value: (transfer none): the lines associated with @iter
 */
Pango2Lines *
pango2_line_iter_get_lines (Pango2LineIter *iter)
{
  return iter->lines;
}

/**
 * pango2_line_iter_get_line:
 * @iter: a `Pango2LineIter`
 *
 * Gets the current line.
 *
 * Return value: (transfer none): the current line
 */
Pango2Line *
pango2_line_iter_get_line (Pango2LineIter *iter)
{
  g_return_val_if_fail (ITER_IS_VALID (iter), NULL);

  return iter->line;
}

/**
 * pango2_line_iter_at_last_line:
 * @iter: a `Pango2LineIter`
 *
 * Determines whether @iter is on the last line.
 *
 * Return value: %TRUE if @iter is on the last line
 */
gboolean
pango2_line_iter_at_last_line (Pango2LineIter *iter)
{
  g_return_val_if_fail (ITER_IS_VALID (iter), FALSE);

  return iter->line_no + 1 == pango2_lines_get_line_count (iter->lines);
}

/**
 * pango2_line_iter_get_run:
 * @iter: a `Pango2LineIter`
 *
 * Gets the current run.
 *
 * When iterating by run, at the end of each line, there's a position
 * with a %NULL run, so this function can return %NULL. The %NULL run
 * at the end of each line ensures that all lines have at least one run,
 * even lines consisting of only a newline.
 *
 * Return value: (transfer none) (nullable): the current run
 */
Pango2Run *
pango2_line_iter_get_run (Pango2LineIter *iter)
{
  g_return_val_if_fail (ITER_IS_VALID (iter), NULL);

  return iter->run;
}

/**
 * pango2_line_iter_get_index:
 * @iter: a `Pango2LineIter`
 *
 * Gets the current byte index.
 *
 * The byte index is relative to the text backing the current
 * line.
 *
 * Note that iterating forward by char moves in visual order,
 * not logical order, so indexes may not be sequential. Also,
 * the index may be equal to the length of the text in the
 * layout, if on the %NULL run (see [method@Pango2.LineIter.get_run]).
 *
 * Return value: current byte index
 */
int
pango2_line_iter_get_index (Pango2LineIter *iter)
{
  g_return_val_if_fail (ITER_IS_VALID (iter), 0);

  return iter->index;
}

/**
 * pango2_line_iter_next_line:
 * @iter: a `Pango2LineIter`
 *
 * Moves @iter forward to the start of the next line.
 *
 * If @iter is already on the last line, returns %FALSE.
 *
 * Return value: whether motion was possible
 */
gboolean
pango2_line_iter_next_line (Pango2LineIter *iter)
{
  g_return_val_if_fail (ITER_IS_VALID (iter), FALSE);

  iter->line_no++;
  if (iter->line_no == iter->lines->lines->len)
    return FALSE;

  iter->line = g_ptr_array_index (iter->lines->lines, iter->line_no);
  iter->line_pos = g_array_index (iter->lines->positions, Position, iter->line_no);

  iter->run_link = iter->line->runs;
  if (iter->run_link)
    iter->run = iter->run_link->data;
  else
    iter->run = NULL;

  update_run (iter, iter->line->start_index);

  return TRUE;
}

/**
 * pango2_line_iter_next_run:
 * @iter: a `Pango2LineIter`
 *
 * Moves @iter forward to the next run in visual order.
 *
 * If @iter was already at the end, returns %FALSE.
 *
 * Return value: whether motion was possible
 */
gboolean
pango2_line_iter_next_run (Pango2LineIter *iter)
{
  int run_start_index;

  g_return_val_if_fail (ITER_IS_VALID (iter), FALSE);

  if (iter->run == NULL)
    return pango2_line_iter_next_line (iter);

  iter->run_link = iter->run_link->next;
  if (iter->run_link == NULL)
    {
      Pango2Item *item = pango2_run_get_glyph_item (iter->run)->item;
      run_start_index = item->offset + item->length;
      iter->run = NULL;
    }
  else
    {
      iter->run = iter->run_link->data;
      run_start_index = pango2_run_get_glyph_item (iter->run)->item->offset;
    }

  update_run (iter, run_start_index);

  return TRUE;
}

/**
 * pango2_line_iter_next_cluster:
 * @iter: a `Pango2LineIter`
 *
 * Moves @iter forward to the next cluster in visual order.
 *
 * If @iter was already at the end, returns %FALSE.
 *
 * Return value: whether motion was possible
 */
gboolean
pango2_line_iter_next_cluster (Pango2LineIter *iter)
{
  g_return_val_if_fail (ITER_IS_VALID (iter), FALSE);

  return next_cluster_internal (iter, FALSE);
}

/**
 * pango2_line_iter_next_char:
 * @iter: a `Pango2LineIter`
 *
 * Moves @iter forward to the next character in visual order.
 *
 * If @iter was already at the end, returns %FALSE.
 *
 * Return value: whether motion was possible
 */
gboolean
pango2_line_iter_next_char (Pango2LineIter *iter)
{
  const char *text;

  g_return_val_if_fail (ITER_IS_VALID (iter), FALSE);

  if (iter->run == NULL)
    {
      /* We need to fake an iterator position in the middle of a \r\n line terminator */
      if (line_is_terminated (iter) &&
          strncmp (iter->line->data->text + iter->line->start_index + iter->line->length, "\r\n", 2) == 0 &&
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

  text = iter->line->data->text;
  if (iter->ltr)
    iter->index = g_utf8_next_char (text + iter->index) - text;
  else
    iter->index = g_utf8_prev_char (text + iter->index) - text;

  return TRUE;
}

/**
 * pango2_line_iter_get_layout_extents:
 * @iter: a `Pango2LineIter`
 * @ink_rect: (out) (optional): rectangle to fill with ink extents
 * @logical_rect: (out) (optional): rectangle to fill with logical extents
 *
 * Obtains the extents of the `Pango2Lines` being iterated over.
 */
void
pango2_line_iter_get_layout_extents (Pango2LineIter  *iter,
                                     Pango2Rectangle *ink_rect,
                                     Pango2Rectangle *logical_rect)
{
  g_return_if_fail (ITER_IS_VALID (iter));

  pango2_lines_get_extents (iter->lines, ink_rect, logical_rect);
}

/**
 * pango2_line_iter_get_line_extents:
 * @iter: a `Pango2LineIter`
 * @ink_rect: (out) (optional): rectangle to fill with ink extents
 * @logical_rect: (out) (optional): rectangle to fill with logical extents
 *
 * Obtains the extents of the current line.
 *
 * Extents are in layout coordinates (origin is the top-left corner of the
 * entire `Pango2Lines`). Thus the extents returned by this function will be
 * the same width/height but not at the same x/y as the extents returned
 * from [method@Pango2.Line.get_extents].
 *
 * The logical extents returned by this function always have their leading
 * trimmed according to paragraph boundaries: if the line starts a paragraph,
 * it has its start leading trimmed; if it ends a paragraph, it has its end
 * leading trimmed. If you need other trimming, use
 * [method@Pango2.Line.get_trimmed_extents].
 */
void
pango2_line_iter_get_line_extents (Pango2LineIter  *iter,
                                   Pango2Rectangle *ink_rect,
                                   Pango2Rectangle *logical_rect)
{
  g_return_if_fail (ITER_IS_VALID (iter));

  pango2_line_get_extents (iter->line, ink_rect, logical_rect);
  offset_line (iter, ink_rect, logical_rect);
}

void
pango2_line_iter_get_trimmed_line_extents (Pango2LineIter    *iter,
                                           Pango2LeadingTrim  trim,
                                           Pango2Rectangle   *logical_rect)
{
  g_return_if_fail (ITER_IS_VALID (iter));

  pango2_line_get_trimmed_extents (iter->line, trim, logical_rect);
  offset_line (iter, NULL, logical_rect);
}

/**
 * pango2_line_iter_get_run_extents:
 * @iter: a `Pango2LineIter`
 * @ink_rect: (out) (optional): rectangle to fill with ink extents
 * @logical_rect: (out) (optional): rectangle to fill with logical extents
 *
 * Gets the extents of the current run in layout coordinates.
 *
 * Layout coordinates have the origin at the top left of the entire `Pango2Lines`.
 *
 * The logical extents returned by this function always have their leading
 * trimmed off. If you need extents that include leading, use
 * [method@Pango2.Run.get_extents].
 */
void
pango2_line_iter_get_run_extents (Pango2LineIter  *iter,
                                  Pango2Rectangle *ink_rect,
                                  Pango2Rectangle *logical_rect)
{
  g_return_if_fail (ITER_IS_VALID (iter));

  if (iter->run)
    {
      pango2_run_get_extents (iter->run, PANGO2_LEADING_TRIM_BOTH, ink_rect, logical_rect);
    }
  else
    {
      GSList *runs = iter->line->runs;
      if (runs)
        {
          /* Virtual run at the end of a nonempty line */
          Pango2Run *run = g_slist_last (runs)->data;

          pango2_run_get_extents (run, PANGO2_LEADING_TRIM_BOTH, ink_rect, logical_rect);
          if (ink_rect)
            {
              ink_rect->x += ink_rect->width;
              ink_rect->width = 0;
            }
          if (logical_rect)
            {
              logical_rect->x += logical_rect->width;
              logical_rect->width = 0;
            }
        }
      else
        {
          /* Empty line */
          Pango2Rectangle r;

          pango2_line_get_empty_extents (iter->line, PANGO2_LEADING_TRIM_BOTH, &r);

          if (ink_rect)
            *ink_rect = r;

          if (logical_rect)
            *logical_rect = r;
        }
    }

  offset_line (iter, ink_rect, logical_rect);
  offset_run (iter, ink_rect, logical_rect);
}

/**
 * pango2_line_iter_get_cluster_extents:
 * @iter: a `Pango2LineIter`
 * @ink_rect: (out) (optional): rectangle to fill with ink extents
 * @logical_rect: (out) (optional): rectangle to fill with logical extents
 *
 * Gets the extents of the current cluster, in layout coordinates.
 *
 * Layout coordinates have the origin at the top left of the entire `Pango2Lines`.
 */
void
pango2_line_iter_get_cluster_extents (Pango2LineIter  *iter,
                                      Pango2Rectangle *ink_rect,
                                      Pango2Rectangle *logical_rect)
{
  Pango2GlyphItem *glyph_item;

  g_return_if_fail (ITER_IS_VALID (iter));

  if (iter->run == NULL)
    {
      /* When on the NULL run, all extents are the same */
      pango2_line_iter_get_run_extents (iter, ink_rect, logical_rect);
      return;
    }

  glyph_item = pango2_run_get_glyph_item (iter->run);

  pango2_glyph_string_extents_range (glyph_item->glyphs,
                                     iter->cluster_start,
                                     iter->next_cluster_glyph,
                                     glyph_item->item->analysis.font,
                                     ink_rect,
                                     logical_rect);

  offset_line (iter, ink_rect, logical_rect);
  if (ink_rect)
    {
      ink_rect->x += iter->cluster_x + glyph_item->start_x_offset;
      ink_rect->y -= glyph_item->y_offset;
    }

  if (logical_rect)
    {
      g_assert (logical_rect->width == iter->cluster_width);
      logical_rect->x += iter->cluster_x + glyph_item->start_x_offset;
      logical_rect->y -= glyph_item->y_offset;
    }
}

/**
 * pango2_line_iter_get_char_extents:
 * @iter: a `Pango2LineIter`
 * @logical_rect: (out caller-allocates): rectangle to fill with logical extents
 *
 * Gets the extents of the current character, in layout coordinates.
 *
 * Layout coordinates have the origin at the top left of the entire `Pango2Lines`.
 *
 * Only logical extents can sensibly be obtained for characters;
 * ink extents make sense only down to the level of clusters.
 */
void
pango2_line_iter_get_char_extents (Pango2LineIter  *iter,
                                   Pango2Rectangle *logical_rect)
{
  Pango2Rectangle cluster_rect;
  int            x0, x1;

  g_return_if_fail (ITER_IS_VALID (iter));

  if (logical_rect == NULL)
    return;

  pango2_line_iter_get_cluster_extents (iter, NULL, &cluster_rect);

  if (iter->run == NULL)
    {
      /* When on the NULL run, all extents are the same */
      *logical_rect = cluster_rect;
      return;
    }

  if (iter->cluster_num_chars)
    {
      x0 = (iter->character_position * cluster_rect.width) / iter->cluster_num_chars;
      x1 = ((iter->character_position + 1) * cluster_rect.width) / iter->cluster_num_chars;
    }
  else
    {
      x0 = x1 = 0;
    }

  logical_rect->width = x1 - x0;
  logical_rect->height = cluster_rect.height;
  logical_rect->y = cluster_rect.y;
  logical_rect->x = cluster_rect.x + x0;
}

/**
 * pango2_line_iter_get_line_baseline:
 * @iter: a `Pango2LineIter`
 *
 * Gets the Y position of the current line's baseline, in layout
 * coordinates.
 *
 * Layout coordinates have the origin at the top left of the entire `Pango2Lines`.
 *
 * Return value: baseline of current line
 */
int
pango2_line_iter_get_line_baseline (Pango2LineIter *iter)
{
  g_return_val_if_fail (ITER_IS_VALID (iter), 0);

  return iter->line_pos.y;
}

/**
 * pango2_line_iter_get_run_baseline:
 * @iter: a `Pango2LineIter`
 *
 * Gets the Y position of the current run's baseline, in layout
 * coordinates.
 *
 * Layout coordinates have the origin at the top left of the entire `Pango2Lines`.
 *
 * The run baseline can be different from the line baseline, for
 * example due to superscript or subscript positioning.
 */
int
pango2_line_iter_get_run_baseline (Pango2LineIter *iter)
{
  g_return_val_if_fail (ITER_IS_VALID (iter), 0);

  if (iter->run)
    return pango2_line_iter_get_line_baseline (iter) - pango2_run_get_glyph_item (iter->run)->y_offset;
  else
    return pango2_line_iter_get_line_baseline (iter);
}

/* }}} */

/* vim:set foldmethod=marker expandtab: */
