#include "config.h"

#include "pango-line-breaker.h"
#include "pango-line-private.h"

#include "pango-tabs.h"
#include "pango-impl-utils.h"
#include "pango-attributes-private.h"
#include "pango-item-private.h"
#include "pango-utils-internal.h"

#include <locale.h>

#include <hb-ot.h>

#if 0
# define DEBUG1(...) g_debug (__VA_ARGS__)
#else
# define DEBUG1(...) do { } while (0)
#endif

/**
 * PangoLineBreaker:
 *
 * A `PangoLineBreaker` breaks text into lines.
 *
 * To use a `PangoLineBreaker`, you must call [method@Pango.LineBreaker.add_text]
 * to provide text that you want to break into lines, plus possibly attribute
 * to influence the formatting.
 *
 * Then you can call [method@Pango.LineBreaker.next_line] repeatedly to obtain
 * `PangoLine` objects for the text, one by one.
 *
 * `PangoLineBreaker` is meant to enable use cases like flowing text around images,
 * or shaped paragraphs. For simple formatting needs, [class@Pango.Layout]
 * is probably more convenient to use.
 */

typedef struct _LastTabState LastTabState;
struct _LastTabState
{
  PangoGlyphString *glyphs;
  int index;
  int width;
  int pos;
  PangoTabAlign align;
  gunichar decimal;
};

struct _PangoLineBreaker
{
  GObject parent_instance;

  /* Properties */
  PangoContext *context;
  PangoDirection base_dir;
  PangoTabArray *tabs;

  /* Data that we're building lines from, shared among all the lines */
  GSList *datas;     /* Queued up LineData */
  LineData *data;    /* The LineData we're currently processing */
  GList *data_items; /* Original items for data (only used for undoing) */
  GList *items;      /* The remaining unprocessed items for data */
  PangoAttrList *render_attrs; /* Attributes to be re-added after line breaking */

  /* Arguments to next_line, for use while processing the next line */
  PangoWrapMode line_wrap;
  PangoEllipsizeMode line_ellipsize;

  int tab_width;                  /* Cached width of a tab. -1 == not yet calculated */
  int hyphen_width;               /* Cached width of a hyphen. -1 == not yet calculated */
  gunichar decimal;               /* Cached decimal point. 0 == not yet calculated */

  /* State for line breaking */
  int n_lines;                    /* Line count, starting from 0 */
  PangoGlyphString *glyphs;       /* Glyphs for the first item in self->items */
  int start_offset;               /* Character offset of first item in self->items in self->data->text */
  ItemProperties properties;      /* Properties of the first item in self->items */
  int *log_widths;                /* Logical widths for th efirst item in self->items */
  int num_log_widths;             /* Length o fo log_widths */
  int log_widths_offset;          /* Offset into log_widths to the point corresponding to
                                   * the remaining portion of the fist item
                                   */
  int line_start_index;           /* Byte offset of line in self->data->text */
  int line_start_offset;          /* Character offset of line in self->data->text */

  int line_x;                     /* X offset for current line */
  int line_width;                 /* Goal width of current line; < 0 for unlimited */
  int remaining_width;            /* Amount of spac remaining on line; < 0 for unlimited */

  gboolean at_paragraph_start;    /* TRUE if the next line starts a new paragraph */

  GList *baseline_shifts;

  LastTabState last_tab;
};

struct _PangoLineBreakerClass
{
  GObjectClass parent_class;
};

/* {{{ Utilities */

static LineData *
make_line_data (PangoLineBreaker *self,
                const char       *text,
                int               length,
                PangoAttrList    *attrs)
{
  LineData *data;

  if (length < 0)
    length = strlen (text);

  data = line_data_new ();

  if (self->base_dir == PANGO_DIRECTION_NEUTRAL)
    {
      data->direction = pango_find_base_dir (text, length);

      if (data->direction == PANGO_DIRECTION_NEUTRAL)
        data->direction = pango_context_get_base_dir (self->context);
    }
  else
    data->direction = self->base_dir;

  data->text = g_strndup (text, length);
  data->length = length;
  data->n_chars = g_utf8_strlen (text, length);
  if (attrs)
    data->attrs = pango_attr_list_copy (attrs);

  return data;
}

static gboolean
item_is_paragraph_separator (PangoLineBreaker *self,
                             PangoItem        *item)
{
  gunichar ch;

  if (self->properties.no_paragraph_break)
    return FALSE;

  ch = g_utf8_get_char (self->data->text + item->offset);

  return ch == '\r' || ch == '\n' || ch == 0x2029;
}

static void
apply_attributes_to_items (GList         *items,
                           PangoAttrList *attrs)
{
  GList *l;
  PangoAttrIterator iter;

  if (!attrs)
    return;

  _pango_attr_list_get_iterator (attrs, &iter);

  for (l = items; l; l = l->next)
    {
      PangoItem *item = l->data;
      pango_item_apply_attrs (item, &iter);
    }

  _pango_attr_iterator_destroy (&iter);
}

static PangoLogAttr *
get_log_attrs (LineData *data,
               GList    *items)
{
  PangoLogAttr *log_attrs;
  int offset;

  log_attrs = g_new0 (PangoLogAttr, (data->n_chars + 1));

  pango_default_break (data->text,
                       data->length,
                       log_attrs,
                       data->n_chars + 1);

  offset = 0;
  for (GList *l = items; l; l = l->next)
    {
      PangoItem *item = l->data;

      pango_tailor_break (data->text + item->offset,
                          item->length,
                          &item->analysis,
                          item->offset,
                          log_attrs + offset,
                          item->num_chars + 1);

      offset += item->num_chars;
    }

  if (data->attrs)
    pango_attr_break (data->text,
                      data->length,
                      data->attrs,
                      0,
                      log_attrs,
                      data->n_chars + 1);

  return log_attrs;
}

static void
ensure_items (PangoLineBreaker *self)
{
  PangoAttrList *itemize_attrs = NULL;
  PangoAttrList *shape_attrs = NULL;

  if (self->items)
    return;

  if (!self->data && self->datas)
    {
      self->data = self->datas->data;
      self->datas = g_slist_remove (self->datas, self->data);
    }

  if (!self->data)
    return;

  self->render_attrs = pango_attr_list_copy (self->data->attrs);
  if (self->render_attrs)
    {
      shape_attrs = pango_attr_list_filter (self->render_attrs, pango_attribute_affects_break_or_shape, NULL);
      itemize_attrs = pango_attr_list_filter (self->render_attrs, pango_attribute_affects_itemization, NULL);
    }

  self->items = pango_itemize_with_font (self->context,
                                         self->data->direction,
                                         self->data->text,
                                         0,
                                         self->data->length,
                                         itemize_attrs,
                                         NULL,
                                         NULL);

  apply_attributes_to_items (self->items, shape_attrs);

  pango_attr_list_unref (itemize_attrs);
  pango_attr_list_unref (shape_attrs);

  self->data->log_attrs = get_log_attrs (self->data, self->items);

  self->items = pango_itemize_post_process_items (self->context,
                                                  self->data->text,
                                                  self->data->log_attrs,
                                                  self->items);

  g_assert (self->data_items == NULL);
  self->data_items = g_list_copy_deep (self->items, (GCopyFunc) pango_item_copy, NULL);

  self->hyphen_width = -1;
  self->tab_width = -1;

  self->start_offset = 0;
  self->line_start_offset = 0;
  self->line_start_index = 0;

  g_list_free_full (self->baseline_shifts, g_free);
  self->baseline_shifts = NULL;
  g_clear_pointer (&self->glyphs, pango_glyph_string_free);
  g_clear_pointer (&self->log_widths, g_free);
  self->num_log_widths = 0;
  self->log_widths_offset = 0;

  self->remaining_width = -1;
  self->at_paragraph_start = TRUE;
}

/* The resolved direction for the line is always one
 * of LTR/RTL; not a week or neutral directions
 */
static PangoDirection
get_resolved_dir (PangoLineBreaker *self)
{
  PangoDirection dir;

  ensure_items (self);

  if (!self->data)
    return PANGO_DIRECTION_NEUTRAL;

  switch (self->data->direction)
    {
    default:
    case PANGO_DIRECTION_LTR:
    case PANGO_DIRECTION_WEAK_LTR:
    case PANGO_DIRECTION_NEUTRAL:
      dir = PANGO_DIRECTION_LTR;
      break;
    case PANGO_DIRECTION_RTL:
    case PANGO_DIRECTION_WEAK_RTL:
      dir = PANGO_DIRECTION_RTL;
      break;
    }

  /* The direction vs. gravity dance:
   *    - If gravity is SOUTH, leave direction untouched.
   *    - If gravity is NORTH, switch direction.
   *    - If gravity is EAST, set to LTR, as
   *      it's a clockwise-rotated layout, so the rotated
   *      top is unrotated left.
   *    - If gravity is WEST, set to RTL, as
   *      it's a counter-clockwise-rotated layout, so the rotated
   *      top is unrotated right.
   *
   * A similar dance is performed in pango-context.c:
   * itemize_state_add_character().  Keep in synch.
   */

  switch (pango_context_get_gravity (self->context))
    {
    default:
    case PANGO_GRAVITY_AUTO:
    case PANGO_GRAVITY_SOUTH:
      break;
    case PANGO_GRAVITY_NORTH:
      dir = PANGO_DIRECTION_LTR + PANGO_DIRECTION_RTL - dir;
      break;
    case PANGO_GRAVITY_EAST:
      dir = PANGO_DIRECTION_LTR;
      break;
    case PANGO_GRAVITY_WEST:
      dir = PANGO_DIRECTION_RTL;
      break;
    }

  return dir;
}

static gboolean
should_ellipsize_current_line (PangoLineBreaker *self,
                               PangoLine        *line)
{
  return self->line_ellipsize != PANGO_ELLIPSIZE_NONE && self->line_width >= 0;
}

static void
get_decimal_prefix_width (PangoItem        *item,
                          PangoGlyphString *glyphs,
                          const char       *text,
                          gunichar          decimal,
                          int              *width,
                          gboolean         *found)
{
  PangoGlyphItem glyph_item = { item, glyphs, 0, 0, 0 };
  int *log_widths;
  int i;
  const char *p;

  log_widths = g_new (int, item->num_chars);

  pango_glyph_item_get_logical_widths (&glyph_item, text, log_widths);

  *width = 0;
  *found = FALSE;

  for (i = 0, p = text + item->offset; i < item->num_chars; i++, p = g_utf8_next_char (p))
    {
      if (g_utf8_get_char (p) == decimal)
        {
          *width += log_widths[i] / 2;
          *found = TRUE;
          break;
        }

      *width += log_widths[i];
    }

  g_free (log_widths);
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

static inline int
get_line_width (PangoLineBreaker *self,
                PangoLine        *line)
{
  if (self->remaining_width > -1)
    return self->line_width - self->remaining_width;

  return pango_line_compute_width (line);
}

static inline void
ensure_decimal (PangoLineBreaker *self)
{
  if (self->decimal == 0)
    self->decimal = g_utf8_get_char (localeconv ()->decimal_point);
}

static void
ensure_tab_width (PangoLineBreaker *self)
{
  if (self->tab_width == -1)
    {
      /* Find out how wide 8 spaces are in the context's default
       * font. Utter performance killer. :-(
       */
      PangoGlyphString *glyphs = pango_glyph_string_new ();
      PangoItem *item;
      GList *items;
      PangoAttribute *attr;
      PangoAttrList *attrs;
      PangoAttrList tmp_attrs;
      PangoFontDescription *font_desc = pango_font_description_copy_static (pango_context_get_font_description (self->context));
      PangoLanguage *language = NULL;
      PangoShapeFlags shape_flags = PANGO_SHAPE_NONE;
      PangoDirection dir;

      if (pango_context_get_round_glyph_positions (self->context))
        shape_flags |= PANGO_SHAPE_ROUND_POSITIONS;

      attrs = self->data->attrs;
      if (attrs)
        {
          PangoAttrIterator iter;

          _pango_attr_list_get_iterator (attrs, &iter);
          pango_attr_iterator_get_font (&iter, font_desc, &language, NULL);
          _pango_attr_iterator_destroy (&iter);
        }

      _pango_attr_list_init (&tmp_attrs);
      attr = pango_attr_font_desc_new (font_desc);
      pango_font_description_free (font_desc);
      pango_attr_list_insert_before (&tmp_attrs, attr);

      if (language)
        {
          attr = pango_attr_language_new (language);
          pango_attr_list_insert_before (&tmp_attrs, attr);
        }

      dir = pango_context_get_base_dir (self->context);
      items = pango_itemize (self->context, dir, " ", 0, 1, &tmp_attrs);

      if (attrs != self->data->attrs)
        {
          pango_attr_list_unref (attrs);
          attrs = NULL;
        }

      _pango_attr_list_destroy (&tmp_attrs);

      item = items->data;
      pango_shape ("        ", 8, "        ", 8, &item->analysis, glyphs, shape_flags);

      pango_item_free (item);
      g_list_free (items);

      self->tab_width = pango_glyph_string_get_width (glyphs);

      pango_glyph_string_free (glyphs);

      /* We need to make sure the tab_width is > 0 so finding tab positions
       * terminates. This check should be necessary only under extreme
       * problems with the font.
       */
      if (self->tab_width <= 0)
        self->tab_width = 50 * PANGO_SCALE; /* pretty much arbitrary */
    }
}

static int
get_item_letter_spacing (PangoItem *item)
{
  ItemProperties properties;

  pango_item_get_properties (item, &properties);

  return properties.letter_spacing;
}

static void
pad_glyphstring_right (PangoLineBreaker *self,
                       PangoGlyphString *glyphs,
                       int               adjustment)
{
  int glyph = glyphs->num_glyphs - 1;

  while (glyph >= 0 && glyphs->glyphs[glyph].geometry.width == 0)
    glyph--;

  if (glyph < 0)
    return;

  self->remaining_width -= adjustment;

  glyphs->glyphs[glyph].geometry.width += adjustment;
  if (glyphs->glyphs[glyph].geometry.width < 0)
    {
      self->remaining_width += glyphs->glyphs[glyph].geometry.width;
      glyphs->glyphs[glyph].geometry.width = 0;
    }
}

static void
pad_glyphstring_left (PangoLineBreaker *self,
                      PangoGlyphString *glyphs,
                      int               adjustment)
{
  int glyph = 0;

  while (glyph < glyphs->num_glyphs && glyphs->glyphs[glyph].geometry.width == 0)
    glyph++;

  if (glyph == glyphs->num_glyphs)
    return;

  self->remaining_width -= adjustment;

  glyphs->glyphs[glyph].geometry.width += adjustment;
  glyphs->glyphs[glyph].geometry.x_offset += adjustment;
}

static gboolean
is_tab_run (PangoLine       *line,
            PangoGlyphItem  *run)
{
  return line->data->text[run->item->offset] == '\t';
}

/*
 * NB: This implement the exact same algorithm as
 *     reorder-items.c:pango_reorder_items().
 */
static GSList *
reorder_runs_recurse (GSList *items,
                      int     n_items)
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
      PangoGlyphItem *run = tmp_list->data;

      min_level = MIN (min_level, run->item->analysis.level);

      tmp_list = tmp_list->next;
    }

  level_start_i = 0;
  level_start_node = items;
  tmp_list = items;
  for (i=0; i<n_items; i++)
    {
      PangoGlyphItem *run = tmp_list->data;

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
pango_line_reorder (PangoLine *line)
{
  GSList *logical_runs = line->runs;
  GSList *tmp_list;
  gboolean all_even, all_odd;
  guint8 level_or = 0, level_and = 1;
  int length = 0;

  /* Check if all items are in the same direction, in that case, the
   * line does not need modification and we can avoid the expensive
   * reorder runs recurse procedure.
   */
  for (tmp_list = logical_runs; tmp_list != NULL; tmp_list = tmp_list->next)
    {
      PangoGlyphItem *run = tmp_list->data;

      level_or |= run->item->analysis.level;
      level_and &= run->item->analysis.level;

      length++;
    }

  /* If none of the levels had the LSB set, all numbers were even. */
  all_even = (level_or & 0x1) == 0;

  /* If all of the levels had the LSB set, all numbers were odd. */
  all_odd = (level_and & 0x1) == 1;

  if (!all_even && !all_odd)
    {
      line->runs = reorder_runs_recurse (logical_runs, length);
      g_slist_free (logical_runs);
    }
  else if (all_odd)
      line->runs = g_slist_reverse (logical_runs);
}

static int
compute_n_chars (PangoLine *line)
{
  int n_chars = 0;

  for (GSList *l = line->runs; l; l = l->next)
    {
      PangoGlyphItem *run = l->data;
      n_chars += run->item->num_chars;
    }

  return n_chars;
}

/* }}} */
/* {{{ Line Breaking */

static void
get_tab_pos (PangoLineBreaker *self,
             PangoLine        *line,
             int               index,
             int              *tab_pos,
             PangoTabAlign    *alignment,
             gunichar         *decimal,
             gboolean         *is_default)
{
  int n_tabs;
  gboolean in_pixels;
  int offset = 0;

  offset = self->line_x;

  if (self->tabs)
    {
      n_tabs = pango_tab_array_get_size (self->tabs);
      in_pixels = pango_tab_array_get_positions_in_pixels (self->tabs);
      *is_default = FALSE;
    }
  else
    {
      n_tabs = 0;
      in_pixels = FALSE;
      *is_default = TRUE;
    }

  if (index < n_tabs)
    {
      pango_tab_array_get_tab (self->tabs, index, alignment, tab_pos);

      if (in_pixels)
        *tab_pos *= PANGO_SCALE;

      *decimal = pango_tab_array_get_decimal_point (self->tabs, index);
    }
  else if (n_tabs > 0)
    {
      /* Extrapolate tab position, repeating the last tab gap to infinity. */
      int last_pos = 0;
      int next_to_last_pos = 0;
      int tab_width;

      pango_tab_array_get_tab (self->tabs, n_tabs - 1, alignment, &last_pos);
      *decimal = pango_tab_array_get_decimal_point (self->tabs, n_tabs - 1);

      if (n_tabs > 1)
        pango_tab_array_get_tab (self->tabs, n_tabs - 2, NULL, &next_to_last_pos);
      else
        next_to_last_pos = 0;

      if (in_pixels)
        {
          next_to_last_pos *= PANGO_SCALE;
          last_pos *= PANGO_SCALE;
        }

      if (last_pos > next_to_last_pos)
        tab_width = last_pos - next_to_last_pos;
      else
        tab_width = self->tab_width;

      *tab_pos = last_pos + tab_width * (index - n_tabs + 1);
    }
  else
    {
      /* No tab array set, so use default tab width */
      *tab_pos = self->tab_width * index;
      *alignment = PANGO_TAB_LEFT;
      *decimal = 0;
    }

  *tab_pos -= offset;
}

static void
shape_tab (PangoLineBreaker *self,
           PangoLine        *line,
           int               current_width,
           PangoItem        *item,
           PangoGlyphString *glyphs)
{
  int i, space_width;
  int tab_pos;
  PangoTabAlign tab_align;
  gunichar tab_decimal;

  pango_glyph_string_set_size (glyphs, 1);

  if (self->properties.showing_space)
    glyphs->glyphs[0].glyph = PANGO_GET_UNKNOWN_GLYPH ('\t');
  else
    glyphs->glyphs[0].glyph = PANGO_GLYPH_EMPTY;

  glyphs->glyphs[0].geometry.x_offset = 0;
  glyphs->glyphs[0].geometry.y_offset = 0;
  glyphs->glyphs[0].attr.is_cluster_start = 1;
  glyphs->glyphs[0].attr.is_color = 0;

  glyphs->log_clusters[0] = 0;

  ensure_tab_width (self);
  space_width = self->tab_width / 8;

  for (i = self->last_tab.index; ; i++)
    {
      gboolean is_default;

      get_tab_pos (self, line, i, &tab_pos, &tab_align, &tab_decimal, &is_default);

      /* Make sure there is at least a space-width of space between
       * tab-aligned text and the text before it. However, only do
       * this if no tab array is set on the line breaker, ie. using default
       * tab positions. If the user has set tab positions, respect it
       * to the pixel.
       */
      if (tab_pos >= current_width + (is_default ? space_width : 1))
        {
          glyphs->glyphs[0].geometry.width = tab_pos - current_width;
          break;
        }
    }

  if (tab_decimal == 0)
    {
      ensure_decimal (self);
      tab_decimal = self->decimal;
    }

  self->last_tab.glyphs = glyphs;
  self->last_tab.index = i;
  self->last_tab.width = current_width;
  self->last_tab.pos = tab_pos;
  self->last_tab.align = tab_align;
  self->last_tab.decimal = tab_decimal;
}

static inline gboolean
can_break_at (PangoLineBreaker *self,
              gint              offset,
              PangoWrapMode     wrap)
{
  if (offset == self->data->n_chars)
    return TRUE;
  else if (wrap == PANGO_WRAP_CHAR)
    return self->data->log_attrs[offset].is_char_break;
  else
    return self->data->log_attrs[offset].is_line_break;
}

static inline gboolean
can_break_in (PangoLineBreaker *self,
              int               start_offset,
              int               num_chars,
              gboolean          allow_break_at_start)
{
  for (int i = allow_break_at_start ? 0 : 1; i < num_chars; i++)
    {
      if (can_break_at (self, start_offset + i, self->line_wrap))
        return TRUE;
    }
  return FALSE;
}

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


static PangoGlyphString *
shape_run (PangoLineBreaker *self,
           PangoLine        *line,
           PangoItem       *item)
{
  PangoGlyphString *glyphs = pango_glyph_string_new ();

  if (self->data->text[item->offset] == '\t')
    shape_tab (self, line, get_line_width (self, line), item, glyphs);
  else
    {
      PangoShapeFlags shape_flags = PANGO_SHAPE_NONE;

      if (pango_context_get_round_glyph_positions (self->context))
        shape_flags |= PANGO_SHAPE_ROUND_POSITIONS;

      if (self->properties.shape_set)
        _pango_shape_shape (self->data->text + item->offset, item->num_chars,
                            self->properties.shape_ink_rect, self->properties.shape_logical_rect,
                            glyphs);
      else
        pango_shape_item (item,
                          self->data->text, self->data->length,
                          self->data->log_attrs + self->start_offset,
                          glyphs,
                          shape_flags);

      if (self->properties.letter_spacing)
        {
          PangoGlyphItem glyph_item;
          int space_left, space_right;

          glyph_item.item = item;
          glyph_item.glyphs = glyphs;

          pango_glyph_item_letter_space (&glyph_item,
                                         self->data->text,
                                         self->data->log_attrs + self->start_offset,
                                         self->properties.letter_spacing);

          distribute_letter_spacing (self->properties.letter_spacing, &space_left, &space_right);

          glyphs->glyphs[0].geometry.width += space_left;
          glyphs->glyphs[0].geometry.x_offset += space_left;
          glyphs->glyphs[glyphs->num_glyphs - 1].geometry.width += space_right;
        }

      if (self->last_tab.glyphs != NULL)
        {
          int w;

          g_assert (self->last_tab.glyphs->num_glyphs == 1);

          /* Update the width of the current tab to position this run properly */

          w = self->last_tab.pos - self->last_tab.width;

          if (self->last_tab.align == PANGO_TAB_RIGHT)
            w -= pango_glyph_string_get_width (glyphs);
          else if (self->last_tab.align == PANGO_TAB_CENTER)
            w -= pango_glyph_string_get_width (glyphs) / 2;
          else if (self->last_tab.align == PANGO_TAB_DECIMAL)
            {
              int width;
              gboolean found;

              get_decimal_prefix_width (item, glyphs, self->data->text, self->last_tab.decimal, &width, &found);

              w -= width;
            }

          self->last_tab.glyphs->glyphs[0].geometry.width = MAX (w, 0);
        }
    }

  return glyphs;
}

static void
free_run (PangoGlyphItem *run,
          gpointer        data)
{
  gboolean free_item = data != NULL;
  if (free_item)
    pango_item_free (run->item);

  pango_glyph_string_free (run->glyphs);
  g_slice_free (PangoGlyphItem, run);
}

static PangoItem *
uninsert_run (PangoLine *line)
{
  PangoGlyphItem *run;
  PangoItem *item;

  GSList *tmp_node = line->runs;

  run = tmp_node->data;
  item = run->item;

  line->runs = tmp_node->next;
  line->length -= item->length;

  g_slist_free_1 (tmp_node);
  free_run (run, NULL);

  return item;
}

static void
insert_run (PangoLineBreaker *self,
            PangoLine        *line,
            PangoItem        *run_item,
            PangoGlyphString *glyphs,
            gboolean          last_run)
{
  PangoGlyphItem *run = g_slice_new (PangoGlyphItem);

  run->item = run_item;

  if (glyphs)
    run->glyphs = glyphs;
  else if (last_run &&
           self->log_widths_offset == 0 &&
           !(run_item->analysis.flags & PANGO_ANALYSIS_FLAG_NEED_HYPHEN))
    {
      run->glyphs = self->glyphs;
      self->glyphs = NULL;
    }
  else
    run->glyphs = shape_run (self, line, run_item);

  if (last_run && self->glyphs)
    {
      pango_glyph_string_free (self->glyphs);
      self->glyphs = NULL;
    }

  line->runs = g_slist_prepend (line->runs, run);
  line->length += run_item->length;

  if (self->last_tab.glyphs && run->glyphs != self->last_tab.glyphs)
    {
      gboolean found_decimal = FALSE;
      int width;

      /* Adjust the tab position so placing further runs will continue to
       * maintain the tab placement. In the case of decimal tabs, we are
       * done once we've placed the run with the decimal point.
       */

      if (self->last_tab.align == PANGO_TAB_RIGHT)
        self->last_tab.width += pango_glyph_string_get_width (run->glyphs);
      else if (self->last_tab.align == PANGO_TAB_CENTER)
        self->last_tab.width += pango_glyph_string_get_width (run->glyphs) / 2;
      else if (self->last_tab.align == PANGO_TAB_DECIMAL)
        {
          int width;

          get_decimal_prefix_width (run->item, run->glyphs, line->data->text, self->last_tab.decimal, &width, &found_decimal);

          self->last_tab.width += width;
        }

      width = MAX (self->last_tab.pos - self->last_tab.width, 0);
      self->last_tab.glyphs->glyphs[0].geometry.width = width;

      if (found_decimal || width == 0)
        self->last_tab.glyphs = NULL;
    }
}

static gboolean
break_needs_hyphen (PangoLineBreaker *self,
                    int               pos)
{
  return self->data->log_attrs[self->start_offset + pos].break_inserts_hyphen ||
         self->data->log_attrs[self->start_offset + pos].break_removes_preceding;
}


static int
find_hyphen_width (PangoItem *item)
{
  hb_font_t *hb_font;
  hb_codepoint_t glyph;

  if (!item->analysis.font)
    return 0;

  /* This is not technically correct, since
   * a) we may end up inserting a different hyphen
   * b) we should reshape the entire run
   * But it is close enough in practice
   */
  hb_font = pango_font_get_hb_font (item->analysis.font);
  if (hb_font_get_nominal_glyph (hb_font, 0x2010, &glyph) ||
      hb_font_get_nominal_glyph (hb_font, '-', &glyph))
    return hb_font_get_glyph_h_advance (hb_font, glyph);

  return 0;
}

static inline void
ensure_hyphen_width (PangoLineBreaker *self)
{
  if (self ->hyphen_width < 0)
    {
      PangoItem *item = self->items->data;
      self->hyphen_width = find_hyphen_width (item);
    }
}

static int
find_break_extra_width (PangoLineBreaker *self,
                        int               pos)
{
  /* Check whether to insert a hyphen,
   * or whether we are breaking after one of those
   * characters that turn into a hyphen,
   * or after a space.
  */
  if (self->data->log_attrs[self->start_offset + pos].break_inserts_hyphen)
    {
      ensure_hyphen_width (self);

      if (self->data->log_attrs[self->start_offset + pos].break_removes_preceding && pos > 0)
        return self->hyphen_width - self->log_widths[self->log_widths_offset + pos - 1];
      else
        return self->hyphen_width;
    }
  else if (pos > 0 &&
           self->data->log_attrs[self->start_offset + pos - 1].is_white)
    {
      return - self->log_widths[self->log_widths_offset + pos - 1];
    }

  return 0;
}

static inline void
compute_log_widths (PangoLineBreaker *self)
{
  PangoItem *item = self->items->data;
  PangoGlyphItem glyph_item = { item, self->glyphs };

  if (item->num_chars > self->num_log_widths)
    {
      self->log_widths = g_renew (int, self->log_widths, item->num_chars);
      self->num_log_widths = item->num_chars;
    }

  g_assert (self->log_widths_offset == 0);
  pango_glyph_item_get_logical_widths (&glyph_item, self->data->text, self->log_widths);
}

/* If last_tab is set, we've added a tab and remaining_width has been updated to
 * account for its origin width, which is last_tab_pos - last_tab_width. shape_run
 * updates the tab width, so we need to consider the delta when comparing
 * against remaining_width.
 */
static int
tab_width_change (PangoLineBreaker *self)
{
  if (self->last_tab.glyphs)
    return self->last_tab.glyphs->glyphs[0].geometry.width - (self->last_tab.pos - self->last_tab.width);

  return 0;
}

typedef enum
{
  BREAK_NONE_FIT,
  BREAK_SOME_FIT,
  BREAK_ALL_FIT,
  BREAK_EMPTY_FIT,
  BREAK_LINE_SEPARATOR,
  BREAK_PARAGRAPH_SEPARATOR
} BreakResult;

/* Tries to insert as much as possible of the item at the head of
 * self->items onto @line. Five results are possible:
 *
 *  %BREAK_NONE_FIT: Couldn't fit anything.
 *  %BREAK_SOME_FIT: The item was broken in the middle.
 *  %BREAK_ALL_FIT: Everything fit.
 *  %BREAK_EMPTY_FIT: Nothing fit, but that was ok, as we can break at the first char.
 *  %BREAK_LINE_SEPARATOR: Item begins with a line separator.
 *  %BREAK_PARAGRAPH_SEPARATOR: Item begins with a paragraph separator
 *
 * If @force_fit is %TRUE, then %BREAK_NONE_FIT will never
 * be returned, a run will be added even if inserting the minimum amount
 * will cause the line to overflow. This is used at the start of a line
 * and until we've found at least some place to break.
 *
 * If @no_break_at_end is %TRUE, then %BREAK_ALL_FIT will never be
 * returned even everything fits; the run will be broken earlier,
 * or %BREAK_NONE_FIT returned. This is used when the end of the
 * run is not a break position.
 *
 * This function is the core of our line-breaking, and it is long and involved.
 * Here is an outline of the algorithm, without all the bookkeeping:
 *
 * if item appears to fit entirely
 *   measure it
 *   if it actually fits
 *     return BREAK_ALL_FIT
 *
 * retry_break:
 *   for each position p in the item
 *     if adding more is 'obviously' not going to help and we have a breakpoint
 *       exit the loop
 *     if p is a possible break position
 *       if p is 'obviously' going to fit
 *         bc = p
 *       else
 *         measure breaking at p (taking extra break width into account
 *         if we don't have a break candidate yet
 *           bc = p
 *         else
 *           if p is better than bc
 *             bc = p
 *
 *   if bc does not fit and we can loosen break conditions
 *     loosen break conditions and retry break
 *
 * return bc
 */

static BreakResult
process_item (PangoLineBreaker *self,
              PangoLine        *line,
              gboolean         force_fit,
              gboolean         no_break_at_end,
              gboolean         is_last_item)
{
  PangoItem *item = self->items->data;
  gboolean shape_set = FALSE;
  int width;
  int extra_width;
  int orig_extra_width;
  int length;
  int i;
  int processing_new_item;
  int num_chars;
  int orig_width;
  PangoWrapMode wrap;
  int break_num_chars;
  int break_width;
  int break_extra_width;
  PangoGlyphString *break_glyphs;
  PangoFontMetrics *metrics;
  int safe_distance;

  DEBUG1 ("process item '%.*s'. Remaining width %d",
          item->length, self->data->text + item->offset,
          self->remaining_width);

  /* We don't want to shape more than necessary, so we keep the results
   * of shaping a new item in self->glyphs, self->log_widths. Once
   * we break off initial parts of the item, we update self->log_widths_offset
   * to take that into account. Note that the widths we calculate from the
   * log_widths are an approximation, because a) log_widths are just
   * evenly divided for clusters, and b) clusters may change as we
   * break in the middle (think ff- i).
   *
   * We use self->log_widths_offset != 0 to detect if we are dealing
   * with the original item, or one that has been chopped off.
   */
  if (!self->glyphs)
    {
      pango_item_get_properties (item, &self->properties);
      self->glyphs = shape_run (self, line, item);
      self->log_widths_offset = 0;
      processing_new_item = TRUE;
    }
  else
    processing_new_item = FALSE;

  if (item_is_paragraph_separator (self, item))
    {
      g_clear_pointer (&self->glyphs, pango_glyph_string_free);
      return BREAK_PARAGRAPH_SEPARATOR;
    }

  /* Only one character has type G_UNICODE_LINE_SEPARATOR in Unicode 5.0;
   * update this if that changes.
   */
#define LINE_SEPARATOR 0x2028

  if (g_utf8_get_char (self->data->text + item->offset) == LINE_SEPARATOR &&
      !should_ellipsize_current_line (self, line))
    {
      insert_run (self, line, item, NULL, TRUE);
      self->log_widths_offset += item->num_chars;

      return BREAK_LINE_SEPARATOR;
    }

  if (self->remaining_width < 0 && !no_break_at_end)  /* Wrapping off */
    {
      insert_run (self, line, item, NULL, TRUE);
      DEBUG1 ("no wrapping, all-fit");
      return BREAK_ALL_FIT;
    }

  if (processing_new_item)
    {
      compute_log_widths (self);
      processing_new_item = FALSE;
    }

  width = 0;
  g_assert (self->log_widths_offset + item->num_chars <= self->num_log_widths);
  for (i = 0; i < item->num_chars; i++)
    width += self->log_widths[self->log_widths_offset + i];

  if (self->data->text[item->offset] == '\t')
    {
      insert_run (self, line, item, NULL, TRUE);
      self->remaining_width -= width;
      self->remaining_width = MAX (self->remaining_width, 0);

      DEBUG1 ("tab run, all-fit");
      return BREAK_ALL_FIT;
    }

  wrap = self->line_wrap;
  if (!no_break_at_end &&
      can_break_at (self, self->start_offset + item->num_chars, wrap))
    {
      extra_width = find_break_extra_width (self, item->num_chars);
    }
  else
    extra_width = 0;

  if ((width + extra_width <= self->remaining_width || (item->num_chars == 1 && !line->runs) ||
      (self->last_tab.glyphs && self->last_tab.align != PANGO_TAB_LEFT)) &&
      !no_break_at_end)
    {
      PangoGlyphString *glyphs;

      DEBUG1 ("%d + %d <= %d", width, extra_width, self->remaining_width);
      glyphs = shape_run (self, line, item);

      width = pango_glyph_string_get_width (glyphs) + tab_width_change (self);

      if (width + extra_width <= self->remaining_width || (item->num_chars == 1 && !line->runs))
        {
          insert_run (self, line, item, glyphs, TRUE);

          self->remaining_width -= width;
          self->remaining_width = MAX (self->remaining_width, 0);

          DEBUG1 ("early accept '%.*s', all-fit, remaining %d",
                  item->length, self->data->text + item->offset,
                  self->remaining_width);
          return BREAK_ALL_FIT;
        }

      /* if it doesn't fit after shaping, discard and proceed to break the item */
      pango_glyph_string_free (glyphs);
    }

  /*** From here on, we look for a way to break item ***/

  orig_width = width;
  orig_extra_width = extra_width;
  break_width = width;
  break_extra_width = extra_width;
  break_num_chars = item->num_chars;
  wrap = self->line_wrap;;
  break_glyphs = NULL;

  /* Add some safety margin here. If we are farther away from the end of the
   * line than this, we don't look carefully at a break possibility.
   */
  metrics = pango_font_get_metrics (item->analysis.font, item->analysis.language);
  safe_distance = pango_font_metrics_get_approximate_char_width (metrics) * 3;
  pango_font_metrics_unref (metrics);

  if (processing_new_item)
    {
      compute_log_widths (self);
      processing_new_item = FALSE;
    }

retry_break:

  for (num_chars = 0, width = 0; num_chars < (no_break_at_end ? item->num_chars : (item->num_chars + 1)); num_chars++)
    {
      extra_width = find_break_extra_width (self, num_chars);

      /* We don't want to walk the entire item if we can help it, but
       * we need to keep going at least until we've found a breakpoint
       * that 'works' (as in, it doesn't overflow the budget we have,
       * or there is no hope of finding a better one).
       *
       * We rely on the fact that MIN(width + extra_width, width) is
       * monotonically increasing.
       */

      if (MIN (width + extra_width, width) > self->remaining_width + safe_distance &&
          break_num_chars < item->num_chars)
        {
          DEBUG1 ("at %d, MIN(%d, %d + %d) > %d + MARGIN, breaking at %d",
                  num_chars, width, extra_width, width, self->remaining_width, break_num_chars);
          break;
        }

      /* If there are no previous runs we have to take care to grab at least one char. */
      if (can_break_at (self, self->start_offset + num_chars, wrap) &&
          (num_chars > 0 || line->runs))
        {
          DEBUG1 ("possible breakpoint: %d, extra_width %d", num_chars, extra_width);
          if (num_chars == 0 ||
              width + extra_width < self->remaining_width - safe_distance)
            {
              DEBUG1 ("trivial accept");
              break_num_chars = num_chars;
              break_width = width;
              break_extra_width = extra_width;
            }
          else
            {
              int length;
              int new_break_width;
              PangoItem *new_item;
              PangoGlyphString *glyphs;

              length = g_utf8_offset_to_pointer (self->data->text + item->offset, num_chars) - (self->data->text + item->offset);

              if (num_chars < item->num_chars)
                {
                  new_item = pango_item_split (item, length, num_chars);

                  if (break_needs_hyphen (self, num_chars))
                    new_item->analysis.flags |= PANGO_ANALYSIS_FLAG_NEED_HYPHEN;
                  else
                    new_item->analysis.flags &= ~PANGO_ANALYSIS_FLAG_NEED_HYPHEN;
                }
              else
                new_item = item;

              glyphs = shape_run (self, line, new_item);

              new_break_width = pango_glyph_string_get_width (glyphs) + tab_width_change (self);

              if (num_chars > 0 &&
                  (item != new_item || !is_last_item) && /* We don't collapse space at the very end */
                  self->data->log_attrs[self->start_offset + num_chars - 1].is_white)
                extra_width = - self->log_widths[self->log_widths_offset + num_chars - 1];
              else if (item == new_item && !is_last_item &&
                       break_needs_hyphen (self, num_chars))
                extra_width = self->hyphen_width;
              else
                extra_width = 0;

              DEBUG1 ("measured breakpoint %d: %d, extra %d", num_chars, new_break_width, extra_width);

              if (new_item != item)
                {
                  pango_item_free (new_item);
                  pango_item_unsplit (item, length, num_chars);
                }

              if (break_num_chars == item->num_chars ||
                  new_break_width + extra_width <= self->remaining_width ||
                  new_break_width + extra_width < break_width + break_extra_width)
                {
                  DEBUG1 ("accept breakpoint %d: %d + %d <= %d + %d",
                          num_chars, new_break_width, extra_width, break_width, break_extra_width);
                  DEBUG1 ("replace bp %d by %d", break_num_chars, num_chars);
                  break_num_chars = num_chars;
                  break_width = new_break_width;
                  break_extra_width = extra_width;

                  if (break_glyphs)
                    pango_glyph_string_free (break_glyphs);
                  break_glyphs = glyphs;
                }
              else
                {
                  DEBUG1 ("ignore breakpoint %d", num_chars);
                  pango_glyph_string_free (glyphs);
                }
            }
        }

      DEBUG1 ("bp now %d", break_num_chars);
      if (num_chars < item->num_chars)
        width += self->log_widths[self->log_widths_offset + num_chars];
    }

   if (wrap == PANGO_WRAP_WORD_CHAR &&
       force_fit &&
       break_width + break_extra_width > self->remaining_width)
    {
      /* Try again, with looser conditions */
      DEBUG1 ("does not fit, try again with wrap-char");
      wrap = PANGO_WRAP_CHAR;
      break_num_chars = item->num_chars;
      break_width = orig_width;
      break_extra_width = orig_extra_width;
      if (break_glyphs)
        pango_glyph_string_free (break_glyphs);
      break_glyphs = NULL;
      goto retry_break;
    }

  if (force_fit || break_width + break_extra_width <= self->remaining_width) /* Successfully broke the item */
    {
      if (self->remaining_width >= 0)
        {
          self->remaining_width -= break_width + break_extra_width;
          self->remaining_width = MAX (self->remaining_width, 0);
        }

      if (break_num_chars == item->num_chars)
        {
          if (can_break_at (self, self->start_offset + break_num_chars, wrap) &&
              break_needs_hyphen (self, break_num_chars))
            item->analysis.flags |= PANGO_ANALYSIS_FLAG_NEED_HYPHEN;

          insert_run (self, line, item, NULL, TRUE);

          if (break_glyphs)
            pango_glyph_string_free (break_glyphs);

          DEBUG1 ("all-fit '%.*s', remaining %d",
                  item->length, self->data->text + item->offset,
                  self->remaining_width);
          return BREAK_ALL_FIT;
        }
      else if (break_num_chars == 0)
        {
          if (break_glyphs)
            pango_glyph_string_free (break_glyphs);

          DEBUG1 ("empty-fit, remaining %d", self->remaining_width);
          return BREAK_EMPTY_FIT;
        }
      else
        {
          PangoItem *new_item;

          length = g_utf8_offset_to_pointer (self->data->text + item->offset, break_num_chars) - (self->data->text + item->offset);

          new_item = pango_item_split (item, length, break_num_chars);

          insert_run (self, line, new_item, break_glyphs, FALSE);

          self->log_widths_offset += break_num_chars;

          /* Shaped items should never be broken */
          g_assert (!shape_set);

          DEBUG1 ("some-fit '%.*s', remaining %d",
                  new_item->length, self->data->text + new_item->offset,
                  self->remaining_width);
          return BREAK_SOME_FIT;
        }
    }
  else
    {
      pango_glyph_string_free (self->glyphs);
      self->glyphs = NULL;

      if (break_glyphs)
        pango_glyph_string_free (break_glyphs);

      DEBUG1 ("none-fit, remaining %d", self->remaining_width);
      return BREAK_NONE_FIT;
    }
}

static void
process_line (PangoLineBreaker *self,
              PangoLine        *line)
{
  gboolean have_break = FALSE;      /* If we've seen a possible break yet */
  int break_remaining_width = 0;    /* Remaining width before adding run with break */
  int break_start_offset = 0;       /* Start offset before adding run with break */
  GSList *break_link = NULL;        /* Link holding run before break */
  gboolean wrapped = FALSE;

  while (self->items)
    {
      PangoItem *item = self->items->data;
      BreakResult result;
      int old_num_chars;
      int old_remaining_width;
      gboolean first_item_in_line;
      gboolean last_item_in_line;

      old_num_chars = item->num_chars;
      old_remaining_width = self->remaining_width;
      first_item_in_line = line->runs == NULL;
      last_item_in_line = self->items->next == NULL;

      result = process_item (self, line, !have_break, FALSE, last_item_in_line);

      switch (result)
        {
        case BREAK_ALL_FIT:
          if (self->data->text[item->offset] != '\t' &&
              can_break_in (self, self->start_offset, old_num_chars, !first_item_in_line))
            {
              have_break = TRUE;
              break_remaining_width = old_remaining_width;
              break_start_offset = self->start_offset;
              break_link = line->runs->next;
            }

          self->items = g_list_delete_link (self->items, self->items);
          self->start_offset += old_num_chars;
          break;

        case BREAK_EMPTY_FIT:
          wrapped = TRUE;
          goto done;

       case BREAK_SOME_FIT:
          self->start_offset += old_num_chars - item->num_chars;
          wrapped = TRUE;
          goto done;

        case BREAK_NONE_FIT:
          /* Back up over unused runs to run where there is a break */
          while (line->runs && line->runs != break_link)
            {
              PangoGlyphItem *run = line->runs->data;

              /* Reset tab stat if we uninsert the current tab run */
              if (run->glyphs == self->last_tab.glyphs)
                {
                  self->last_tab.glyphs = NULL;
                  self->last_tab.index = 0;
                  self->last_tab.align = PANGO_TAB_LEFT;
                }

              self->items = g_list_prepend (self->items, uninsert_run (line));
            }

          self->start_offset = break_start_offset;
          self->remaining_width = break_remaining_width;
          last_item_in_line = self->items->next == NULL;

          /* Reshape run to break */
          item = self->items->data;

          old_num_chars = item->num_chars;
          result = process_item (self, line, TRUE, TRUE, last_item_in_line);
          g_assert (result == BREAK_SOME_FIT || result == BREAK_EMPTY_FIT);

          self->start_offset += old_num_chars - item->num_chars;

          wrapped = TRUE;
          goto done;

        case BREAK_LINE_SEPARATOR:
          self->items = g_list_delete_link (self->items, self->items);
          self->start_offset += old_num_chars;
          /* A line-separate is just a forced break. Set wrapped, so we justify */
          wrapped = TRUE;
          goto done;

        case BREAK_PARAGRAPH_SEPARATOR:
          /* We don't add the item as a run, so don't add to
           * line->length or line->n_chars here.
           * But we still need the next line to start after
           * the terminators, so add to self->line_start_index
           */
          line->ends_paragraph = TRUE;
          self->line_start_index += item->length;
          self->start_offset += item->num_chars;
          self->items = g_list_delete_link (self->items, self->items);
          pango_item_free (item);
          goto done;

        default:
          g_assert_not_reached ();
        }
    }

done:
  line->wrapped = wrapped;
}

/* {{{ Post-processing */

static void
add_missing_hyphen (PangoLineBreaker *self,
                    PangoLine        *line)
{
  PangoGlyphItem *run;
  PangoItem *item;

  if (!line->runs)
    return;

  run = line->runs->data;
  item = run->item;

  if (self->data->log_attrs[self->line_start_offset + line->n_chars].break_inserts_hyphen &&
      !(item->analysis.flags & PANGO_ANALYSIS_FLAG_NEED_HYPHEN))
    {
      int width;
      int start_offset;

      DEBUG1 ("add a missing hyphen");

      /* The last run fit onto the line without breaking it, but it still needs a hyphen */
      width = pango_glyph_string_get_width (run->glyphs);

      /* Ugly, shape_run uses self->start_offset, so temporarily rewind things
       * to the state before the run was inserted. Otherwise, we end up passing
       * the wrong log attrs to the shaping machinery.
       */
      start_offset = self->start_offset;
      self->start_offset = self->line_start_offset + line->n_chars - item->num_chars;

      pango_glyph_string_free (run->glyphs);
      item->analysis.flags |= PANGO_ANALYSIS_FLAG_NEED_HYPHEN;
      run->glyphs = shape_run (self, line, item);

      self->start_offset = start_offset;

      self->remaining_width += pango_glyph_string_get_width (run->glyphs) - width;
    }

  line->hyphenated = (item->analysis.flags & PANGO_ANALYSIS_FLAG_NEED_HYPHEN) != 0;
}

static void
zero_line_final_space (PangoLineBreaker *self,
                       PangoLine        *line)
{
  PangoGlyphItem *run;
  PangoItem *item;
  PangoGlyphString *glyphs;
  int glyph;

  if (!line->runs)
    return;

  run = line->runs->data;
  item = run->item;

  glyphs = run->glyphs;
  glyph = item->analysis.level % 2 ? 0 : glyphs->num_glyphs - 1;

  if (glyphs->glyphs[glyph].glyph == PANGO_GET_UNKNOWN_GLYPH (0x2028))
    {
      DEBUG1 ("zero final space: visible space");
      return; /* this LS is visible */
    }

  /* if the final char of line forms a cluster, and it's
   * a whitespace char, zero its glyph's width as it's been wrapped
   */
  if (glyphs->num_glyphs < 1 || self->start_offset == 0 ||
      !self->data->log_attrs[self->start_offset - 1].is_white)
    {
      DEBUG1 ("zero final space: not whitespace");
      return;
    }

  if (glyphs->num_glyphs >= 2 &&
      glyphs->log_clusters[glyph] == glyphs->log_clusters[glyph + (item->analysis.level % 2 ? 1 : -1)])
    {

      DEBUG1 ("zero final space: its a cluster");
      return;
    }

  DEBUG1 ("zero line final space: collapsing the space");
  glyphs->glyphs[glyph].geometry.width = 0;
  glyphs->glyphs[glyph].glyph = PANGO_GLYPH_EMPTY;
}

/* When doing shaping, we add the letter spacing value for a
 * run after every grapheme in the run. This produces ugly
 * asymmetrical results, so what this routine is redistributes
 * that space to the beginning and the end of the run.
 *
 * We also trim the letter spacing from runs adjacent to
 * tabs and from the outside runs of the lines so that things
 * line up properly. The line breaking and tab positioning
 * were computed without this trimming so they are no longer
 * exactly correct, but this won't be very noticeable in most
 * cases.
 */
static void
adjust_line_letter_spacing (PangoLineBreaker *self,
                            PangoLine        *line)
{
  gboolean reversed;
  PangoGlyphItem *last_run;
  int tab_adjustment;
  GSList *l;

  /* If we have tab stops and the resolved direction of the
   * line is RTL, then we need to walk through the line
   * in reverse direction to figure out the corrections for
   * tab stops.
   */
  reversed = FALSE;
  if (line->direction == PANGO_DIRECTION_RTL)
    {
      for (l = line->runs; l; l = l->next)
        if (is_tab_run (line, l->data))
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
      PangoGlyphItem *run = l->data;
      PangoGlyphItem *next_run = l->next ? l->next->data : NULL;

      if (is_tab_run (line, run))
        {
          pad_glyphstring_right (self, run->glyphs, tab_adjustment);
          tab_adjustment = 0;
        }
      else
        {
          PangoGlyphItem *visual_next_run = reversed ? last_run : next_run;
          PangoGlyphItem *visual_last_run = reversed ? next_run : last_run;
          int run_spacing = get_item_letter_spacing (run->item);
          int space_left, space_right;

          distribute_letter_spacing (run_spacing, &space_left, &space_right);

          if (run->glyphs->glyphs[0].geometry.width == 0)
            {
              /* we've zeroed this space glyph at the end of line, now remove
               * the letter spacing added to its adjacent glyph
               */
              pad_glyphstring_left (self, run->glyphs, - space_left);
            }
          else if (!visual_last_run || is_tab_run (line, visual_last_run))
            {
              pad_glyphstring_left (self, run->glyphs, - space_left);
              tab_adjustment += space_left;
            }

          if (run->glyphs->glyphs[run->glyphs->num_glyphs - 1].geometry.width == 0)
            {
              /* we've zeroed this space glyph at the end of line, now remove
               * the letter spacing added to its adjacent glyph
               */
              pad_glyphstring_right (self, run->glyphs, - space_right);
            }

          else if (!visual_next_run || is_tab_run (line, visual_next_run))
            {
              pad_glyphstring_right (self, run->glyphs, - space_right);
              tab_adjustment += space_right;
            }
        }

      last_run = run;
    }

  if (reversed)
    line->runs = g_slist_reverse (line->runs);
}

typedef struct {
  PangoAttribute *attr;
  int x_offset;
  int y_offset;
} BaselineItem;

static void
collect_baseline_shift (PangoLineBreaker *self,
                        PangoItem        *item,
                        PangoItem        *prev,
                        int              *start_x_offset,
                        int              *start_y_offset,
                        int              *end_x_offset,
                        int              *end_y_offset)
{
  *start_x_offset = 0;
  *start_y_offset = 0;
  *end_x_offset = 0;
  *end_y_offset = 0;

  for (GSList *l = item->analysis.extra_attrs; l; l = l->next)
    {
      PangoAttribute *attr = l->data;

      if (attr->klass->type == PANGO_ATTR_RISE)
        {
          int value = ((PangoAttrInt *)attr)->value;

          *start_y_offset += value;
          *end_y_offset -= value;
        }
      else if (attr->klass->type == PANGO_ATTR_BASELINE_SHIFT)
        {
          if (attr->start_index == item->offset)
            {
              BaselineItem *entry;
              int value;

              entry = g_new0 (BaselineItem, 1);
              entry->attr = attr;
              self->baseline_shifts = g_list_prepend (self->baseline_shifts, entry);

              value = ((PangoAttrInt *)attr)->value;

              if (value > 1024 || value < -1024)
                {
                  entry->y_offset = value;
                  /* FIXME: compute an x_offset from value to italic angle */
                }
              else
                {
                  int superscript_x_offset = 0;
                  int superscript_y_offset = 0;
                  int subscript_x_offset = 0;
                  int subscript_y_offset = 0;


                  if (prev)
                    {
                      hb_font_t *hb_font = pango_font_get_hb_font (prev->analysis.font);
                      hb_ot_metrics_get_position (hb_font, HB_OT_METRICS_TAG_SUPERSCRIPT_EM_Y_OFFSET, &superscript_y_offset);
                      hb_ot_metrics_get_position (hb_font, HB_OT_METRICS_TAG_SUPERSCRIPT_EM_X_OFFSET, &superscript_x_offset);
                      hb_ot_metrics_get_position (hb_font, HB_OT_METRICS_TAG_SUBSCRIPT_EM_Y_OFFSET, &subscript_y_offset);
                      hb_ot_metrics_get_position (hb_font, HB_OT_METRICS_TAG_SUBSCRIPT_EM_X_OFFSET, &subscript_x_offset);
                    }

                  if (superscript_y_offset == 0)
                    superscript_y_offset = 5000;
                  if (subscript_y_offset == 0)
                    subscript_y_offset = 5000;

                  switch (value)
                    {
                    case PANGO_BASELINE_SHIFT_NONE:
                      entry->x_offset = 0;
                      entry->y_offset = 0;
                      break;
                    case PANGO_BASELINE_SHIFT_SUPERSCRIPT:
                      entry->x_offset = superscript_x_offset;
                      entry->y_offset = superscript_y_offset;
                      break;
                    case PANGO_BASELINE_SHIFT_SUBSCRIPT:
                      entry->x_offset = subscript_x_offset;
                      entry->y_offset = -subscript_y_offset;
                      break;
                    default:
                      g_assert_not_reached ();
                    }
                }

              *start_x_offset += entry->x_offset;
              *start_y_offset += entry->y_offset;
            }

          if (attr->end_index == item->offset + item->length)
            {
              GList *t;

              for (t = self->baseline_shifts; t; t = t->next)
                {
                  BaselineItem *entry = t->data;

                  if (attr->start_index == entry->attr->start_index &&
                      attr->end_index == entry->attr->end_index &&
                      ((PangoAttrInt *)attr)->value == ((PangoAttrInt *)entry->attr)->value)
                    {
                      *end_x_offset -= entry->x_offset;
                      *end_y_offset -= entry->y_offset;
                    }

                  self->baseline_shifts = g_list_remove (self->baseline_shifts, entry);
                  g_free (entry);
                  break;
                }
              if (t == NULL)
                g_warning ("Baseline attributes mismatch\n");
            }
        }
    }
}

static void
apply_baseline_shift (PangoLineBreaker *self,
                      PangoLine        *line)
{
  int y_offset = 0;
  PangoItem *prev = NULL;
  hb_position_t baseline_adjustment = 0;
#if HB_VERSION_ATLEAST(4,0,0)
  hb_ot_layout_baseline_tag_t baseline_tag = 0;
  hb_position_t baseline;
  hb_position_t run_baseline;
#endif

  for (GSList *l = line->runs; l; l = l->next)
    {
      PangoGlyphItem *run = l->data;
      PangoItem *item = run->item;
      int start_x_offset, end_x_offset;
      int start_y_offset, end_y_offset;
#if HB_VERSION_ATLEAST(4,0,0)
      hb_font_t *hb_font;
      hb_script_t script;
      hb_language_t language;
      hb_direction_t direction;
      hb_tag_t script_tags[HB_OT_MAX_TAGS_PER_SCRIPT];
      hb_tag_t lang_tags[HB_OT_MAX_TAGS_PER_LANGUAGE];
      unsigned int script_count = HB_OT_MAX_TAGS_PER_SCRIPT;
      unsigned int lang_count = HB_OT_MAX_TAGS_PER_LANGUAGE;
#endif

      if (item->analysis.font == NULL)
        continue;

#if HB_VERSION_ATLEAST(4,0,0)
      hb_font = pango_font_get_hb_font (item->analysis.font);

      script = (hb_script_t) g_unicode_script_to_iso15924 (item->analysis.script);
      language = hb_language_from_string (pango_language_to_string (item->analysis.language), -1);
      hb_ot_tags_from_script_and_language (script, language,
                                           &script_count, script_tags,
                                           &lang_count, lang_tags);

      if (item->analysis.flags & PANGO_ANALYSIS_FLAG_CENTERED_BASELINE)
        direction = HB_DIRECTION_TTB;
      else
        direction = HB_DIRECTION_LTR;

      if (baseline_tag == 0)
        {
          if (item->analysis.flags & PANGO_ANALYSIS_FLAG_CENTERED_BASELINE)
            baseline_tag = HB_OT_LAYOUT_BASELINE_TAG_IDEO_EMBOX_CENTRAL;
          else
            baseline_tag = hb_ot_layout_get_horizontal_baseline_tag_for_script (script);
          hb_ot_layout_get_baseline_with_fallback (hb_font,
                                                   baseline_tag,
                                                   direction,
                                                   script_tags[script_count - 1],
                                                   lang_count ? lang_tags[lang_count - 1] : HB_TAG_NONE,
                                                   &run_baseline);
          baseline = run_baseline;
        }
      else
        {
          hb_ot_layout_get_baseline_with_fallback (hb_font,
                                                   baseline_tag,
                                                   direction,
                                                   script_tags[script_count - 1],
                                                   lang_count ? lang_tags[lang_count - 1] : HB_TAG_NONE,
                                                   &run_baseline);
        }

      /* Don't do baseline adjustment in vertical, since the renderer
       * is still doing its own baseline shifting there
       */
      if (item->analysis.flags & PANGO_ANALYSIS_FLAG_CENTERED_BASELINE)
        baseline_adjustment = 0;
      else
        baseline_adjustment = baseline - run_baseline;
#endif

      collect_baseline_shift (self, item, prev, &start_x_offset, &start_y_offset, &end_x_offset, &end_y_offset);

      y_offset += start_y_offset + baseline_adjustment;

      run->y_offset = y_offset;
      run->start_x_offset = start_x_offset;
      run->end_x_offset = end_x_offset;

      y_offset += end_y_offset - baseline_adjustment;

      prev = item;
    }
}

static void
apply_render_attributes (PangoLineBreaker *self,
                         PangoLine        *line)
{
  GSList *runs;

  if (!self->render_attrs)
    return;

  runs = g_slist_reverse (line->runs);
  line->runs = NULL;

  for (GSList *l = runs; l; l = l->next)
    {
      PangoGlyphItem *glyph_item = l->data;
      GSList *new_runs;

      new_runs = pango_glyph_item_apply_attrs (glyph_item,
                                               line->data->text,
                                               self->render_attrs);

      line->runs = g_slist_concat (new_runs, line->runs);
    }

  g_slist_free (runs);
}

static void
postprocess_line (PangoLineBreaker *self,
                  PangoLine        *line)
{
  add_missing_hyphen (self, line);

  /* Truncate the logical-final whitespace in the line if we broke the line at it */
  if (line->wrapped)
    zero_line_final_space (self, line);

  line->runs = g_slist_reverse (line->runs);

  apply_baseline_shift (self, line);

  if (should_ellipsize_current_line (self, line))
    pango_line_ellipsize (line, self->context, self->line_ellipsize, self->line_width);

  /* Now convert logical to visual order */
  pango_line_reorder (line);

  /* Fixup letter spacing between runs */
  adjust_line_letter_spacing (self, line);

  apply_render_attributes (self, line);
}

/* }}} */
/* }}} */
/* {{{ PangoLineBreaker implementation */

G_DEFINE_TYPE (PangoLineBreaker, pango_line_breaker, G_TYPE_OBJECT)

enum {
  PROP_CONTEXT = 1,
  PROP_BASE_DIR,
  PROP_TABS,
  N_PROPERTIES
};

static GParamSpec *properties[N_PROPERTIES];

static void
pango_line_breaker_init (PangoLineBreaker *self)
{
  self->tabs = NULL;
  self->tab_width = -1;
  self->decimal = 0;
  self->n_lines = 0;
}

static void
pango_line_breaker_finalize (GObject *object)
{
  PangoLineBreaker *self = PANGO_LINE_BREAKER (object);

  g_list_free_full (self->baseline_shifts, g_free);
  g_clear_pointer (&self->glyphs, pango_glyph_string_free);
  g_clear_pointer (&self->log_widths, g_free);
  g_list_free_full (self->items, (GDestroyNotify) pango_item_free);
  g_clear_pointer (&self->data, line_data_unref);
  g_list_free_full (self->data_items, (GDestroyNotify) pango_item_free);
  g_clear_pointer (&self->render_attrs, pango_attr_list_unref);
  g_slist_free_full (self->datas, (GDestroyNotify) line_data_unref);
  g_clear_pointer (&self->tabs, pango_tab_array_free);
  g_object_unref (self->context);

  G_OBJECT_CLASS (pango_line_breaker_parent_class)->finalize (object);
}

static void
pango_line_breaker_set_property (GObject      *object,
                                 guint         prop_id,
                                 const GValue *value,
                                 GParamSpec   *pspec)
{
  PangoLineBreaker *self = PANGO_LINE_BREAKER (object);

  switch (prop_id)
    {
    case PROP_CONTEXT:
      g_assert (self->context == NULL);
      self->context = g_value_dup_object (value);
      break;

    case PROP_BASE_DIR:
      pango_line_breaker_set_base_dir (self, g_value_get_enum (value));
      break;

    case PROP_TABS:
      pango_line_breaker_set_tabs (self, g_value_get_boxed (value));
      break;

    default:
      G_OBJECT_WARN_INVALID_PROPERTY_ID (object, prop_id, pspec);
      break;
    }
}

static void
pango_line_breaker_get_property (GObject    *object,
                                 guint       prop_id,
                                 GValue     *value,
                                 GParamSpec *pspec)
{
  PangoLineBreaker *self = PANGO_LINE_BREAKER (object);

  switch (prop_id)
    {
    case PROP_CONTEXT:
      g_value_set_object (value, self->context);
      break;

    case PROP_BASE_DIR:
      g_value_set_enum (value, pango_line_breaker_get_base_dir (self));
      break;

    case PROP_TABS:
      g_value_set_boxed (value, pango_line_breaker_get_tabs (self));
      break;

    default:
      G_OBJECT_WARN_INVALID_PROPERTY_ID (object, prop_id, pspec);
      break;
    }
}

static void
pango_line_breaker_class_init (PangoLineBreakerClass *class)
{
  GObjectClass *object_class = G_OBJECT_CLASS (class);

  object_class->finalize = pango_line_breaker_finalize;
  object_class->set_property = pango_line_breaker_set_property;
  object_class->get_property = pango_line_breaker_get_property;

  /**
   * PangoLineBreaker:context: (attributes org.gtk.Property.get=pango_line_breaker_get_context)
   *
   * The context for the `PangoLineBreaker`.
   */
  properties[PROP_CONTEXT] =
    g_param_spec_object ("context", "context", "context",
                         PANGO_TYPE_CONTEXT,
                         G_PARAM_READWRITE|G_PARAM_CONSTRUCT_ONLY|G_PARAM_EXPLICIT_NOTIFY);

  /**
   * PangoLineBreaker:base-dir: (attributes org.gtk.Property.get=pango_line_breaker_get_base_dir org.gtk.Property.set=pango_line_breaker_set_base_dir)
   *
   * The base direction for the `PangoLineBreaker`.
   *
   * The default value is `PANGO_DIRECTION_NEUTRAL`.
   */
  properties[PROP_BASE_DIR] =
    g_param_spec_enum ("base-dir", "base-dir", "base-dir",
                       PANGO_TYPE_DIRECTION,
                       PANGO_DIRECTION_NEUTRAL,
                       G_PARAM_READWRITE|G_PARAM_EXPLICIT_NOTIFY);

  /**
   * PangoLineBreaker:tabs: (attributes org.gtk.Property.get=pango_line_breaker_get_tabs org.gtk.Property.set=pango_line_breaker_set_tabs)
   *
   * The tabs to use when formatting the next line of the `PangoLineBreaker`.
   *
   * `PangoLineBreaker` will place content at the next tab position
   * whenever it meets a Tab character (U+0009).
   */
  properties[PROP_TABS] =
    g_param_spec_boxed ("tabs", "tabs", "tabs",
                        PANGO_TYPE_TAB_ARRAY,
                        G_PARAM_READWRITE|G_PARAM_EXPLICIT_NOTIFY);

  g_object_class_install_properties (object_class, N_PROPERTIES, properties);
}

/* }}} */
/* {{{ Public API */

/**
 * pango_line_breaker_new:
 * @context: a `PangoContext`
 *
 * Creates a new `PangoLineBreaker`.
 *
 * Returns: a newly created `PangoLineBreaker`
 */
PangoLineBreaker *
pango_line_breaker_new (PangoContext *context)
{
  g_return_val_if_fail (PANGO_IS_CONTEXT (context), NULL);

  return  g_object_new (PANGO_TYPE_LINE_BREAKER, "context", context, NULL);
}

/**
 * pango_line_breaker_get_context:
 * @self: a `PangoLineBreaker`
 *
 * Retrieves the context used for this `PangoLineBreaker`.
 *
 * Returns: (transfer none): the `PangoContext` for @self
 */
PangoContext *
pango_line_breaker_get_context (PangoLineBreaker *self)
{
  g_return_val_if_fail (PANGO_IS_LINE_BREAKER (self), NULL);

  return self->context;
}

/**
 * pango_line_breaker_set_tabs:
 * @self: a `PangoLineBreaker`
 * @tabs: (nullable): a `PangoTabArray`
 *
 * Sets the tab positions to use for lines.
 *
 * `PangoLineBreaker` will place content at the next tab position
 * whenever it meets a Tab character (U+0009).
 *
 * By default, tabs are every 8 spaces. If @tabs is %NULL, the
 * default tabs are reinstated. @tabs is copied by @self, you
 * must free your copy of @tabs yourself.
 *
 * Note that tabs and justification conflict with each other:
 * Justification will move content away from its tab-aligned
 * positions. The same is true for alignments other than
 * %PANGO_ALIGNMENT_LEFT.
 */
void
pango_line_breaker_set_tabs (PangoLineBreaker *self,
                             PangoTabArray    *tabs)
{
  g_return_if_fail (PANGO_IS_LINE_BREAKER (self));

  if (self->tabs)
    {
      pango_tab_array_free (self->tabs);
      self->tabs = NULL;
    }

  if (tabs)
    {
      self->tabs = pango_tab_array_copy (tabs);
      pango_tab_array_sort (self->tabs);
    }

  g_object_notify_by_pspec (G_OBJECT (self), properties[PROP_TABS]);
}

/**
 * pango_line_breaker_get_tabs:
 * @self: a `PangoLineBreaker`
 *
 * Gets the current `PangoTabArray` used by this `PangoLineBreaker`.
 *
 * If no `PangoTabArray` has been set, then the default tabs are
 * in use and %NULL is returned. Default tabs are every 8 spaces.
 *
 * Return value: (transfer none) (nullable): the tabs for @self
 */
PangoTabArray *
pango_line_breaker_get_tabs (PangoLineBreaker *self)
{
  g_return_val_if_fail (PANGO_IS_LINE_BREAKER (self), NULL);

  if (self->tabs)
    return self->tabs;
  else
    return NULL;
}

/**
 * pango_line_breaker_set_base_dir:
 * @self: a `PangoLineBreaker`
 * @direction: the direction
 *
 * Sets the base direction for lines produced by this `PangoLineBreaker`.
 *
 * If @direction is `PANGO_DIRECTION_NEUTRAL`, the direction is determined
 * from the content. This is the default behavior.
 */
void
pango_line_breaker_set_base_dir (PangoLineBreaker *self,
                                 PangoDirection    direction)
{
  g_return_if_fail (PANGO_IS_LINE_BREAKER (self));

  if (self->base_dir == direction)
    return;

  self->base_dir = direction;

  g_object_notify_by_pspec (G_OBJECT (self), properties[PROP_BASE_DIR]);
}

/**
 * pango_line_breaker_get_base_dir:
 * @self: a `PangoLineBreaker`
 *
 * Gets the base direction for lines produced by this `PangoLineBreaker`.
 *
 * See [method@Pango.LineBreaker.set_base_dir].
 */
PangoDirection
pango_line_breaker_get_base_dir (PangoLineBreaker *self)
{
  g_return_val_if_fail (PANGO_IS_LINE_BREAKER (self), PANGO_DIRECTION_NEUTRAL);

  return self->base_dir;
}

/**
 * pango_line_breaker_add_text:
 * @text: the text to break into lines
 * @length: length of @text in bytes, or -1 if @text is nul-terminated
 * @attrs: (nullable): a `PangoAttrList` with attributes for @text, or `NULL`
 *
 * Provides input that the `PangoLineBreaker` should break into lines.
 *
 * It is possible to call this function repeatedly to add more
 * input to an existing `PangoLineBreaker`.
 *
 * The end of @text is treated as a paragraph break.
 */
void
pango_line_breaker_add_text (PangoLineBreaker *self,
                             const char       *text,
                             int               length,
                             PangoAttrList    *attrs)
{
  g_return_if_fail (PANGO_IS_LINE_BREAKER (self));
  g_return_if_fail (text != NULL);

  self->datas = g_slist_append (self->datas, make_line_data (self, text, length, attrs));
}

/**
 * pango_line_breaker_get_direction:
 * @self: a `PangoLineBreaker`
 *
 * Obtains the resolved direction for the next line.
 *
 * If the `PangoLineBreaker` has no more input, then
 * `PANGO_DIRECTION_NEUTRAL` is returned.
 *
 * Returns: the resolved direction of the next line.
 */
PangoDirection
pango_line_breaker_get_direction (PangoLineBreaker *self)
{
  g_return_val_if_fail (PANGO_IS_LINE_BREAKER (self), PANGO_DIRECTION_NEUTRAL);

  return get_resolved_dir (self);
}

/**
 * pango_line_breaker_has_line:
 * @self: a `PangoLineBreaker`
 *
 * Returns whether the `PangoLineBreaker` has any text left to process.
 *
 * Returns: TRUE if there are more lines.
 */
gboolean
pango_line_breaker_has_line (PangoLineBreaker *self)
{
  g_return_val_if_fail (PANGO_IS_LINE_BREAKER (self), FALSE);

  ensure_items (self);

  return self->items != NULL;
}

/**
 * pango_line_breaker_next_line:
 * @self: a `PangoLineBreaker`
 * @x: the X position for the line, in Pango units
 * @width: the width for the line, or -1 for no limit, in Pango units
 * @wrap: how to wrap the text
 * @ellipsize: whether to ellipsize the text
 *
 * Gets the next line.
 *
 * The `PangoLineBreaker` will use as much of its unprocessed text
 * as will fit into @width. The @x position is used to determine
 * where tabs are located are.
 *
 * If @ellipsize is not `PANGO_ELLIPSIZE_NONE`, then all unprocessed
 * text will be made to fit by ellipsizing.
 *
 * Note that the line is not positioned - the leftmost point of its baseline
 * is at 0, 0. See [class@Pango.Lines] for a way to hold a list of positioned
 * `PangoLine` objects.
 *
 *     line = pango_line_breaker_next_line (breaker,
 *                                          x, width,
 *                                          PANGO_WRAP_MODE,
 *                                          PANGO_ELLIPSIZE_NONE);
 *     pango_line_get_extents (line, &ext);
 *     line = pango_line_justify (line, width);
 *     pango_lines_add_line (lines, line, x, y - ext.y);
 *
 * Returns: (transfer full) (nullable): the next line, or `NULL`
 *   if @self has no more input
 */
PangoLine *
pango_line_breaker_next_line (PangoLineBreaker   *self,
                              int                 x,
                              int                 width,
                              PangoWrapMode       wrap,
                              PangoEllipsizeMode  ellipsize)
{
  PangoLine *line;

  g_return_val_if_fail (PANGO_IS_LINE_BREAKER (self), NULL);

  ensure_items (self);

  if (!self->items)
    return NULL;

  line = pango_line_new (self->context, self->data);

  line->start_index = self->line_start_index;
  line->start_offset = self->line_start_offset;
  line->starts_paragraph = self->at_paragraph_start;
  line->direction = get_resolved_dir (self);

  self->line_x = x;
  self->line_width = width;
  self->line_wrap = wrap;
  self->line_ellipsize = ellipsize;

  self->last_tab.glyphs = NULL;
  self->last_tab.index = 0;
  self->last_tab.align = PANGO_TAB_LEFT;

  if (should_ellipsize_current_line (self, line))
    self->remaining_width = -1;
  else
    self->remaining_width = width;

  process_line (self, line);

  line->n_chars = compute_n_chars (line);

  postprocess_line (self, line);

  if (!self->items)
    line->ends_paragraph = TRUE;

  self->at_paragraph_start = line->ends_paragraph;
  self->n_lines++;
  self->line_start_index += line->length;
  self->line_start_offset = self->start_offset;

  if (self->items == NULL)
    {
      g_clear_pointer (&self->data, line_data_unref);
      g_list_free_full (self->data_items, (GDestroyNotify) pango_item_free);
      self->data_items = NULL;
      g_clear_pointer (&self->render_attrs, pango_attr_list_unref);
    }

  pango_line_check_invariants (line);

  return line;
}

/**
 * pango_line_breaker_undo_line:
 * @self: a `PangoLineBreaker`
 * @line: (transfer none): the most recent line produced by @self
 *
 * Re-adds the content of @line to the unprocessed
 * input of the `PangoLineBreaker`.
 *
 * This can be used to try this line again with
 * different parameters passed to
 * [method@Pango.LineBreaker.next_line].
 *
 * When undoing multiple lines, they have to be
 * undone in the reverse order in which they
 * were produced.
 *
 * Returns: `TRUE` on success, `FALSE` if Pango
 *   determines that the line can't be undone
 */
gboolean
pango_line_breaker_undo_line (PangoLineBreaker *self,
                              PangoLine        *line)
{
  if (self->data == NULL &&
      line->start_index == 0 && line->length == line->data->length)
    {
      g_assert (self->items == NULL);
      self->datas = g_slist_prepend (self->datas, line_data_ref (line->data));

      self->n_lines--;

      /* ensure_items will set up everything else */

      g_clear_pointer (&self->glyphs, pango_glyph_string_free);

      return TRUE;
    }

  if (self->data == line->data &&
      self->line_start_index == line->start_index + line->length)
    {
      GList *items = NULL;

      /* recover the original items */
      for (GList *l = self->data_items; l; l = l->next)
        {
          PangoItem *item = l->data;

          if (item->offset + item->length < line->start_index)
            continue;

          if (item->offset > self->line_start_index)
            break;

          item = pango_item_copy (item);

          if (item->offset < line->start_index)
            {
              PangoItem *new_item;
              int n_chars;

              n_chars = g_utf8_strlen (self->data->text + item->offset, line->start_index - item->offset);
              new_item = pango_item_split (item, line->start_index - item->offset, n_chars);
              pango_item_free (new_item);
            }

          if (item->offset + item->length > self->line_start_index)
            {
              PangoItem *new_item;
              int n_chars;

              n_chars = g_utf8_strlen (self->data->text + item->offset, self->line_start_index - item->offset);
              new_item = pango_item_split (item, self->line_start_index - item->offset, n_chars);
              pango_item_free (item);
              item = new_item;
            }

          items = g_list_prepend (items, item);
        }

      self->items = g_list_concat (g_list_reverse (items), self->items);

      self->n_lines--;

      self->at_paragraph_start = line->starts_paragraph;
      self->line_start_index = line->start_index;
      self->line_start_offset = line->start_offset;

      g_clear_pointer (&self->glyphs, pango_glyph_string_free);
      self->start_offset = line->start_offset;
      self->log_widths_offset = 0;

      return TRUE;
    }

  return FALSE;
}

/* }}} */

/* vim:set foldmethod=marker expandtab: */
