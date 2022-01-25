#include "config.h"

#include "pango-line-iter-private.h"
#include "pango-lines-private.h"
#include "pango-line-private.h"
#include "pango-run-private.h"

/**
 * PangoLineIter:
 *
 * A `PangoLineIter` can be used to iterate over the visual
 * extents of a `PangoLayout` or `PangoLines`.
 *
 * To obtain a `PangoLineIter`, use [method@Pango.Layout.get_iter]
 * or [method@Pango.Lines.get_iter].
 *
 * The `PangoLineIter` structure is opaque, and has no user-visible
 * fields.
 */


/* {{{ PangoLineIter implementation */

struct _PangoLineIter
{
  PangoLines *lines;
  guint serial;

  int line_no;
  int line_x;
  int line_y;
  PangoLine *line;
  GSList *run_link;
  PangoRun *run;
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

G_DEFINE_BOXED_TYPE (PangoLineIter, pango_line_iter,
                     pango_line_iter_copy, pango_line_iter_free);


/* }}} */
/* {{{ Utilities */

#define ITER_IS_VALID(iter) ((iter)->serial == (iter)->lines->serial)

static gboolean
line_is_terminated (PangoLineIter *iter)
{
  if (iter->line_no + 1 < pango_lines_get_line_count (iter->lines))
    return pango_line_is_paragraph_end (iter->line);

  return FALSE;

}

static int
next_cluster_start (PangoGlyphString *glyphs,
                    int               cluster_start)
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
cluster_width (PangoGlyphString *glyphs,
               int               cluster_start)
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
update_cluster (PangoLineIter *iter,
                int            cluster_start_index)
{
  PangoGlyphItem *glyph_item;
  char *cluster_text;
  int  cluster_length;

  glyph_item = pango_run_get_glyph_item (iter->run);

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
next_nonempty_line (PangoLineIter *iter,
                    gboolean       include_terminators)
{
  gboolean result;

  while (TRUE)
    {
      result = pango_line_iter_next_line (iter);
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
next_nonempty_run (PangoLineIter *iter,
                   gboolean       include_terminators)
{
  gboolean result;

  while (TRUE)
    {
      result = pango_line_iter_next_run (iter);
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
next_cluster_internal (PangoLineIter *iter,
                       gboolean       include_terminators)
{
  PangoGlyphItem *glyph_item;

  if (iter->run == NULL)
    return next_nonempty_line (iter, include_terminators);

  glyph_item = pango_run_get_glyph_item (iter->run);

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
update_run (PangoLineIter *iter,
            int            start_index)
{
  PangoGlyphItem *glyph_item;

  if (iter->run)
    glyph_item = pango_run_get_glyph_item (iter->run);

  if (iter->run_link == iter->line->runs)
    iter->run_x = 0;
  else
    {
      iter->run_x += iter->end_x_offset + iter->run_width;
      if (iter->run)
        iter->run_x += glyph_item->start_x_offset;
    }

  if (iter->run)
    {
      iter->run_width = pango_glyph_string_get_width (glyph_item->glyphs);
      iter->end_x_offset = glyph_item->end_x_offset;
    }
  else
    {
      /* The empty run at the end of a line */
      iter->run_width = 0;
      iter->end_x_offset = 0;
    }

  if (iter->run)
    iter->ltr = (glyph_item->item->analysis.level % 2) == 0;
  else
    iter->ltr = TRUE;

  iter->cluster_start = 0;
  iter->cluster_x = iter->run_x;

  if (iter->run)
    {
      update_cluster (iter, glyph_item->glyphs->log_clusters[0]);
    }
  else
    {
      iter->cluster_width = 0;
      iter->character_position = 0;
      iter->cluster_num_chars = 0;
      iter->index = start_index;
    }
}

static inline void
offset_line (PangoLineIter  *iter,
             PangoRectangle *ink_rect,
             PangoRectangle *logical_rect)
{
  if (ink_rect)
    {
      ink_rect->x += iter->line_x;
      ink_rect->y += iter->line_y;
    }
  if (logical_rect)
    {
      logical_rect->x += iter->line_x;
      logical_rect->y += iter->line_y;
    }
}

static inline void
offset_run (PangoLineIter  *iter,
            PangoRectangle *ink_rect,
            PangoRectangle *logical_rect)
{
  if (ink_rect)
    ink_rect->x += iter->run_x;
  if (logical_rect)
    logical_rect->x += iter->run_x;
}

/* }}} */
/*  {{{ Private API */ 

PangoLineIter *
pango_line_iter_new (PangoLines *lines)
{
  PangoLineIter *iter;
  int run_start_index;

  g_return_val_if_fail (PANGO_IS_LINES (lines), NULL);

  iter = g_new0 (PangoLineIter, 1);

  iter->lines = g_object_ref (lines);
  iter->serial = pango_lines_get_serial (lines);

  iter->line_no = 0;
  iter->line = pango_lines_get_line (iter->lines, 0, &iter->line_x, &iter->line_y);
  iter->run_link = iter->line->runs;
  if (iter->run_link)
    {
      iter->run = iter->run_link->data;
      run_start_index = pango_run_get_glyph_item (iter->run)->item->offset;
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
 * pango_line_iter_copy:
 * @iter: (nullable): a `PangoLineIter`
 *
 * Copies a `PangoLineIter`.
 *
 * Return value: (nullable): the newly allocated `PangoLineIter`
 */
PangoLineIter *
pango_line_iter_copy (PangoLineIter *iter)
{
  PangoLineIter *copy;

  if (iter == NULL)
    return NULL;

  copy = g_new0 (PangoLineIter, 1);
  memcpy (iter, copy, sizeof (PangoLineIter));
  g_object_ref (copy->lines);

  return copy;
}

/**
 * pango_line_iter_free:
 * @iter: (nullable): a `PangoLineIter`
 *
 * Frees an iterator that's no longer in use.
 */
void
pango_line_iter_free (PangoLineIter *iter)
{
  if (iter == NULL)
    return;

  g_object_unref (iter->lines);
  g_free (iter);
}

/**
 * pango_line_iter_get_lines:
 * @iter: a `PangoLineIter`
 *
 * Gets the `PangoLines` object associated with a `PangoLineIter`.
 *
 * Return value: (transfer none): the lines associated with @iter
 */
PangoLines *
pango_line_iter_get_lines (PangoLineIter *iter)
{
  return iter->lines;
}

/**
 * pango_line_iter_get_line:
 * @iter: a `PangoLineIter`
 *
 * Gets the current line.
 *
 * Return value: (transfer none): the current line
 */
PangoLine *
pango_line_iter_get_line (PangoLineIter *iter)
{
  g_return_val_if_fail (ITER_IS_VALID (iter), NULL);

  return iter->line;
}

/**
 * pango_line_iter_at_last_line:
 * @iter: a `PangoLineIter`
 *
 * Determines whether @iter is on the last line.
 *
 * Return value: %TRUE if @iter is on the last line
 */
gboolean
pango_line_iter_at_last_line (PangoLineIter *iter)
{
  g_return_val_if_fail (ITER_IS_VALID (iter), FALSE);

  return iter->line_no + 1 == pango_lines_get_line_count (iter->lines);
}

/**
 * pango_line_iter_get_run:
 * @iter: a `PangoLineIter`
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
PangoRun *
pango_line_iter_get_run (PangoLineIter *iter)
{
  g_return_val_if_fail (ITER_IS_VALID (iter), NULL);

  return iter->run;
}

/**
 * pango_line_iter_get_index:
 * @iter: a `PangoLineIter`
 *
 * Gets the current byte index.
 *
 * The byte index is relative to the text backing the current
 * line.
 *
 * Note that iterating forward by char moves in visual order,
 * not logical order, so indexes may not be sequential. Also,
 * the index may be equal to the length of the text in the
 * layout, if on the %NULL run (see [method@Pango.LineIter.get_run]).
 *
 * Return value: current byte index
 */
int
pango_line_iter_get_index (PangoLineIter *iter)
{
  g_return_val_if_fail (ITER_IS_VALID (iter), 0);

  return iter->index;
}

/**
 * pango_line_iter_next_line:
 * @iter: a `PangoLineIter`
 *
 * Moves @iter forward to the start of the next line.
 *
 * If @iter is already on the last line, returns %FALSE.
 *
 * Return value: whether motion was possible
 */
gboolean
pango_line_iter_next_line (PangoLineIter *iter)
{
  g_return_val_if_fail (ITER_IS_VALID (iter), FALSE);

  iter->line = pango_lines_get_line (iter->lines, iter->line_no + 1, &iter->line_x, &iter->line_y);
  if (!iter->line)
    return FALSE;

  iter->line_no++;
  iter->run_link = iter->line->runs;
  if (iter->run_link)
    iter->run = iter->run_link->data;
  else
    iter->run = NULL;

  update_run (iter, iter->line->start_index);

  return TRUE;
}

/**
 * pango_line_iter_next_run:
 * @iter: a `PangoLineIter`
 *
 * Moves @iter forward to the next run in visual order.
 *
 * If @iter was already at the end, returns %FALSE.
 *
 * Return value: whether motion was possible
 */
gboolean
pango_line_iter_next_run (PangoLineIter *iter)
{
  int run_start_index;

  g_return_val_if_fail (ITER_IS_VALID (iter), FALSE);

  if (iter->run == NULL)
    return pango_line_iter_next_line (iter);

  iter->run_link = iter->run_link->next;
  if (iter->run_link == NULL)
    {
      PangoItem *item = pango_run_get_glyph_item (iter->run)->item;
      run_start_index = item->offset + item->length;
      iter->run = NULL;
    }
  else
    {
      iter->run = iter->run_link->data;
      run_start_index = pango_run_get_glyph_item (iter->run)->item->offset;
    }

  update_run (iter, run_start_index);

  return TRUE;
}

/**
 * pango_line_iter_next_cluster:
 * @iter: a `PangoLineIter`
 *
 * Moves @iter forward to the next cluster in visual order.
 *
 * If @iter was already at the end, returns %FALSE.
 *
 * Return value: whether motion was possible
 */
gboolean
pango_line_iter_next_cluster (PangoLineIter *iter)
{
  g_return_val_if_fail (ITER_IS_VALID (iter), FALSE);

  return next_cluster_internal (iter, FALSE);
}

/**
 * pango_line_iter_next_char:
 * @iter: a `PangoLineIter`
 *
 * Moves @iter forward to the next character in visual order.
 *
 * If @iter was already at the end, returns %FALSE.
 *
 * Return value: whether motion was possible
 */
gboolean
pango_line_iter_next_char (PangoLineIter *iter)
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
 * pango_line_iter_get_layout_extents:
 * @iter: a `PangoLineIter`
 * @ink_rect: (out) (optional): rectangle to fill with ink extents
 * @logical_rect: (out) (optional): rectangle to fill with logical extents
 *
 * Obtains the extents of the `PangoLines` being iterated over.
 */
void
pango_line_iter_get_layout_extents (PangoLineIter  *iter,
                                    PangoRectangle *ink_rect,
                                    PangoRectangle *logical_rect)
{
  g_return_if_fail (ITER_IS_VALID (iter));

  pango_lines_get_extents (iter->lines, ink_rect, logical_rect);
}

/**
 * pango_line_iter_get_line_extents:
 * @iter: a `PangoLineIter`
 * @ink_rect: (out) (optional): rectangle to fill with ink extents
 * @logical_rect: (out) (optional): rectangle to fill with logical extents
 *
 * Obtains the extents of the current line.
 *
 * Extents are in layout coordinates (origin is the top-left corner of the
 * entire `PangoLines`). Thus the extents returned by this function will be
 * the same width/height but not at the same x/y as the extents returned
 * from [method@Pango.Line.get_extents].
 *
 * The logical extents returned by this function always have their leading
 * trimmed according to paragraph boundaries: if the line starts a paragraph,
 * it has its start leading trimmed; if it ends a paragraph, it has its end
 * leading trimmed. If you need other trimming, use
 * [method@Pango.Line.get_trimmed_extents].
 */
void
pango_line_iter_get_line_extents (PangoLineIter  *iter,
                                  PangoRectangle *ink_rect,
                                  PangoRectangle *logical_rect)
{
  g_return_if_fail (ITER_IS_VALID (iter));

  pango_line_get_extents (iter->line, ink_rect, logical_rect);
  offset_line (iter, ink_rect, logical_rect);
}

void
pango_line_iter_get_trimmed_line_extents (PangoLineIter    *iter,
                                          PangoLeadingTrim  trim,
                                          PangoRectangle   *logical_rect)
{
  g_return_if_fail (ITER_IS_VALID (iter));

  pango_line_get_trimmed_extents (iter->line, trim, logical_rect);
  offset_line (iter, NULL, logical_rect);
}

/**
 * pango_line_iter_get_run_extents:
 * @iter: a `PangoLineIter`
 * @ink_rect: (out) (optional): rectangle to fill with ink extents
 * @logical_rect: (out) (optional): rectangle to fill with logical extents
 *
 * Gets the extents of the current run in layout coordinates.
 *
 * Layout coordinates have the origin at the top left of the entire `PangoLines`.
 *
 * The logical extents returned by this function always have their leading
 * trimmed off. If you need extents that include leading, use
 * [method@Pango.Run.get_extents].
 */
void
pango_line_iter_get_run_extents (PangoLineIter  *iter,
                                 PangoRectangle *ink_rect,
                                 PangoRectangle *logical_rect)
{
  g_return_if_fail (ITER_IS_VALID (iter));

  if (iter->run)
    {
      pango_run_get_extents (iter->run, PANGO_LEADING_TRIM_BOTH, ink_rect, logical_rect);
    }
  else
    {
      GSList *runs = iter->line->runs;
      if (runs)
        {
          /* Virtual run at the end of a nonempty line */
          PangoRun *run = g_slist_last (runs)->data;

          pango_run_get_extents (run, PANGO_LEADING_TRIM_BOTH, ink_rect, logical_rect);
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
          PangoRectangle r;

          pango_line_get_empty_extents (iter->line, PANGO_LEADING_TRIM_BOTH, &r);

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
 * pango_line_iter_get_cluster_extents:
 * @iter: a `PangoLineIter`
 * @ink_rect: (out) (optional): rectangle to fill with ink extents
 * @logical_rect: (out) (optional): rectangle to fill with logical extents
 *
 * Gets the extents of the current cluster, in layout coordinates.
 *
 * Layout coordinates have the origin at the top left of the entire `PangoLines`.
 */
void
pango_line_iter_get_cluster_extents (PangoLineIter  *iter,
                                     PangoRectangle *ink_rect,
                                     PangoRectangle *logical_rect)
{
  PangoGlyphItem *glyph_item;

  g_return_if_fail (ITER_IS_VALID (iter));

  if (iter->run == NULL)
    {
      /* When on the NULL run, all extents are the same */
      pango_line_iter_get_run_extents (iter, ink_rect, logical_rect);
      return;
    }

  glyph_item = pango_run_get_glyph_item (iter->run);

  pango_glyph_string_extents_range (glyph_item->glyphs,
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
 * pango_line_iter_get_char_extents:
 * @iter: a `PangoLineIter`
 * @logical_rect: (out caller-allocates): rectangle to fill with logical extents
 *
 * Gets the extents of the current character, in layout coordinates.
 *
 * Layout coordinates have the origin at the top left of the entire `PangoLines`.
 *
 * Only logical extents can sensibly be obtained for characters;
 * ink extents make sense only down to the level of clusters.
 */
void
pango_line_iter_get_char_extents (PangoLineIter  *iter,
                                  PangoRectangle *logical_rect)
{
  PangoRectangle cluster_rect;
  int            x0, x1;

  g_return_if_fail (ITER_IS_VALID (iter));

  if (logical_rect == NULL)
    return;

  pango_line_iter_get_cluster_extents (iter, NULL, &cluster_rect);

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
 * pango_line_iter_get_line_baseline:
 * @iter: a `PangoLineIter`
 *
 * Gets the Y position of the current line's baseline, in layout
 * coordinates.
 *
 * Layout coordinates have the origin at the top left of the entire `PangoLines`.
 *
 * Return value: baseline of current line
 */
int
pango_line_iter_get_line_baseline (PangoLineIter *iter)
{
  g_return_val_if_fail (ITER_IS_VALID (iter), 0);

  return iter->line_y;
}

/**
 * pango_line_iter_get_run_baseline:
 * @iter: a `PangoLineIter`
 *
 * Gets the Y position of the current run's baseline, in layout
 * coordinates.
 *
 * Layout coordinates have the origin at the top left of the entire `PangoLines`.
 *
 * The run baseline can be different from the line baseline, for
 * example due to superscript or subscript positioning.
 */
int
pango_line_iter_get_run_baseline (PangoLineIter *iter)
{
  g_return_val_if_fail (ITER_IS_VALID (iter), 0);

  if (iter->run)
    return pango_line_iter_get_line_baseline (iter) - pango_run_get_glyph_item (iter->run)->y_offset;
  else
    return pango_line_iter_get_line_baseline (iter);
}

/* }}} */ 

/* vim:set foldmethod=marker expandtab: */
