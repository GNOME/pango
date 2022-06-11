#include "config.h"

#include "pango-layout.h"
#include "pango-line-breaker.h"
#include "pango-line-private.h"
#include "pango-enum-types.h"
#include "pango-markup.h"
#include "pango-context.h"

/**
 * PangoLayout:
 *
 * A `PangoLayout` structure represents an entire paragraph of text.
 *
 * While complete access to the layout capabilities of Pango is provided
 * using the detailed interfaces for itemization, segmentation and shaping,
 * using that functionality directly involves writing a fairly large amount
 * of code. `PangoLayout` provides a high-level driver for formatting entire
 * paragraphs of text at once. This includes paragraph-level functionality
 * such as line breaking, justification, alignment and ellipsization.
 *
 * A `PangoLayout` is initialized with a `PangoContext`, a UTF-8 string
 * and set of attributes for that string. Once that is done, the set of
 * formatted lines can be extracted in the form of a [class@Pango.Lines]
 * object, the layout can be rendered, and conversion between logical
 * character positions within the layout's text, and the physical position
 * of the resulting glyphs can be made.
 *
 * The most convenient way to access the visual extents and components
 * of a formatted layout is via a [struct@Pango.LineIter] iterator.
 *
 * There are a number of parameters to adjust the formatting of a
 * `PangoLayout`. The following image shows adjustable parameters
 * (on the left) and font metrics (on the right):
 *
 * <picture>
 *   <source srcset="layout-dark.png" media="(prefers-color-scheme: dark)">
 *   <img alt="Pango Layout Parameters" src="layout-light.png">
 * </picture>
 *
 * The following images demonstrate the effect of alignment and justification
 * on the layout of text:
 *
 * | | | |
 * | --- | --- | --- |
 * | ![align=left](align-left.png) | ![align=center](align-center.png) | ![align=right](align-right.png) |
 * | ![align=justify](align-left-justify.png) | | |
 *
 * It is possible, as well, to ignore the 2-D setup, and simply treat the
 * resulting `PangoLines` object as a list of lines.
 *
 * If you have more complex line breaking needs, such as shaping text
 * to flow around images, or multi-column layout, the [class@Pango.LineBreaker]
 * makes the underlying line-breaking functionality available outside of
 * `PangoLayout`.
 */


/* {{{ PangoLayout implementation */

struct _PangoLayout
{
  GObject parent_instance;

  PangoContext *context;
  char *text;
  int length;
  PangoAttrList *attrs;
  PangoFontDescription *font_desc;
  float line_spacing;
  int width;
  int height;
  PangoTabArray *tabs;
  gboolean single_paragraph;
  PangoWrapMode wrap;
  int indent;
  guint serial;
  guint context_serial;
  PangoAlignment alignment;
  PangoEllipsizeMode ellipsize;
  gboolean auto_dir;

  PangoLines *lines;
};

struct _PangoLayoutClass
{
  GObjectClass parent_class;
};

enum
{
  PROP_CONTEXT = 1,
  PROP_TEXT,
  PROP_ATTRIBUTES,
  PROP_FONT_DESCRIPTION,
  PROP_LINE_SPACING,
  PROP_WIDTH,
  PROP_HEIGHT,
  PROP_TABS,
  PROP_SINGLE_PARAGRAPH,
  PROP_WRAP,
  PROP_INDENT,
  PROP_ALIGNMENT,
  PROP_ELLIPSIZE,
  PROP_AUTO_DIR,
  PROP_LINES,
  NUM_PROPERTIES
};

static GParamSpec *props[NUM_PROPERTIES] = { NULL, };

G_DEFINE_TYPE (PangoLayout, pango_layout, G_TYPE_OBJECT)

static void
pango_layout_init (PangoLayout *layout)
{
  layout->serial = 1;
  layout->width = -1;
  layout->height = -1;
  layout->indent = 0;
  layout->wrap = PANGO_WRAP_WORD;
  layout->alignment = PANGO_ALIGN_LEFT;
  layout->ellipsize = PANGO_ELLIPSIZE_NONE;
  layout->line_spacing = 0.0;
  layout->auto_dir = TRUE;
  layout->text = g_strdup ("");
  layout->length = 0;
}

static void
pango_layout_finalize (GObject *object)
{
  PangoLayout *layout = PANGO_LAYOUT (object);

  g_clear_pointer (&layout->font_desc, pango_font_description_free);
  g_object_unref (layout->context);
  g_free (layout->text);
  g_clear_pointer (&layout->attrs, pango_attr_list_unref);
  g_clear_pointer (&layout->tabs, pango_tab_array_free);
  g_clear_object (&layout->lines);

  G_OBJECT_CLASS (pango_layout_parent_class)->finalize (object);
}

static void
pango_layout_set_property (GObject      *object,
                           guint         prop_id,
                           const GValue *value,
                           GParamSpec   *pspec)
{
  PangoLayout *layout = PANGO_LAYOUT (object);

  switch (prop_id)
    {
    case PROP_CONTEXT:
      layout->context = g_value_dup_object (value);
      layout->context_serial = pango_context_get_serial (layout->context);
      break;

    case PROP_TEXT:
      pango_layout_set_text (layout, g_value_get_string (value), -1);
      break;

    case PROP_ATTRIBUTES:
      pango_layout_set_attributes (layout, g_value_get_boxed (value));
      break;

    case PROP_FONT_DESCRIPTION:
      pango_layout_set_font_description (layout, g_value_get_boxed (value));
      break;

    case PROP_LINE_SPACING:
      pango_layout_set_line_spacing (layout, g_value_get_float (value));
      break;

    case PROP_WIDTH:
      pango_layout_set_width (layout, g_value_get_int (value));
      break;

    case PROP_HEIGHT:
      pango_layout_set_height (layout, g_value_get_int (value));
      break;

    case PROP_TABS:
      pango_layout_set_tabs (layout, g_value_get_boxed (value));
      break;

    case PROP_SINGLE_PARAGRAPH:
      pango_layout_set_single_paragraph (layout, g_value_get_boolean (value));
      break;

    case PROP_WRAP:
      pango_layout_set_wrap (layout, g_value_get_enum (value));
      break;

    case PROP_INDENT:
      pango_layout_set_indent (layout, g_value_get_int (value));
      break;

    case PROP_ALIGNMENT:
      pango_layout_set_alignment (layout, g_value_get_enum (value));
      break;

    case PROP_ELLIPSIZE:
      pango_layout_set_ellipsize (layout, g_value_get_enum (value));
      break;

    case PROP_AUTO_DIR:
      pango_layout_set_auto_dir (layout, g_value_get_boolean (value));
      break;

    default:
      G_OBJECT_WARN_INVALID_PROPERTY_ID (object, prop_id, pspec);
      break;
    }
}

static void
pango_layout_get_property (GObject      *object,
                           guint         prop_id,
                           GValue       *value,
                           GParamSpec   *pspec)
{
  PangoLayout *layout = PANGO_LAYOUT (object);

  switch (prop_id)
    {
    case PROP_CONTEXT:
      g_value_set_object (value, layout->context);
      break;

    case PROP_TEXT:
      g_value_set_string (value, layout->text);
      break;

    case PROP_ATTRIBUTES:
      g_value_set_boxed (value, layout->attrs);
      break;

    case PROP_FONT_DESCRIPTION:
      g_value_set_boxed (value, layout->font_desc);
      break;

    case PROP_LINE_SPACING:
      g_value_set_float (value, layout->line_spacing);
      break;

    case PROP_WIDTH:
      g_value_set_int (value, layout->width);
      break;

    case PROP_HEIGHT:
      g_value_set_int (value, layout->height);
      break;

    case PROP_TABS:
      g_value_set_boxed (value, layout->tabs);
      break;

    case PROP_SINGLE_PARAGRAPH:
      g_value_set_boolean (value, layout->single_paragraph);
      break;

    case PROP_WRAP:
      g_value_set_enum (value, layout->wrap);
      break;

    case PROP_INDENT:
      g_value_set_int (value, layout->indent);
      break;

    case PROP_ALIGNMENT:
      g_value_set_enum (value, layout->alignment);
      break;

    case PROP_ELLIPSIZE:
      g_value_set_enum (value, layout->ellipsize);
      break;

    case PROP_AUTO_DIR:
      g_value_set_boolean (value, layout->auto_dir);
      break;

    case PROP_LINES:
      g_value_set_object (value, pango_layout_get_lines (layout));
      break;

    default:
      G_OBJECT_WARN_INVALID_PROPERTY_ID (object, prop_id, pspec);
      break;
    }
}

static void
pango_layout_class_init (PangoLayoutClass *class)
{
  GObjectClass *object_class = G_OBJECT_CLASS (class);

  object_class->finalize = pango_layout_finalize;
  object_class->set_property = pango_layout_set_property;
  object_class->get_property = pango_layout_get_property;

  /**
   * PangoLayout:context: (attributes org.gtk.Property.get=pango_layout_get_context)
   *
   * The context for the `PangoLayout`.
   */
  props[PROP_CONTEXT] = g_param_spec_object ("context", "context", "context",
                                             PANGO_TYPE_CONTEXT,
                                             G_PARAM_READWRITE | G_PARAM_CONSTRUCT_ONLY);

  /**
   * PangoLayout:text: (attributes org.gtk.Property.get=pango_layout_get_text org.gtk.Property.set=pango_layout_set_text)
   *
   * The text of the `PangoLayout`.
   */
  props[PROP_TEXT] = g_param_spec_string ("text", "text", "text",
                                          "",
                                          G_PARAM_READWRITE | G_PARAM_EXPLICIT_NOTIFY);

  /**
   * PangoLayout:attributes: (attributes org.gtk.Property.get=pango_layout_get_attributes org.gtk.Property.set=pango_layout_set_attributes)
   *
   * The attributes of the `PangoLayout`.
   *
   * Attributes can affect how the text is formatted.
   */
  props[PROP_ATTRIBUTES] = g_param_spec_boxed ("attributes", "attributes", "attributes",
                                               PANGO_TYPE_ATTR_LIST,
                                               G_PARAM_READWRITE | G_PARAM_EXPLICIT_NOTIFY);

  /**
   * PangoLayout:font-description: (attributes org.gtk.Property.get=pango_layout_get_font_description org.gtk.Property.set=pango_layout_set_font_description)
   *
   * The font description of the `PangoLayout`.
   */
  props[PROP_FONT_DESCRIPTION] = g_param_spec_boxed ("font-description", "font-description", "font-description",
                                                     PANGO_TYPE_FONT_DESCRIPTION,
                                                     G_PARAM_READWRITE | G_PARAM_EXPLICIT_NOTIFY);

  /**
   * PangoLayout:line-spacing: (attributes org.gtk.Property.get=pango_layout_get_line_spacing org.gtk.Property.set=pango_layout_set_line_spacing)
   *
   * The line spacing factor of the `PangoLayout`.
   */
  props[PROP_LINE_SPACING] = g_param_spec_float ("line-spacing", "line-spacing", "line-spacing",
                                                 0., G_MAXFLOAT, 0.,
                                                 G_PARAM_READWRITE | G_PARAM_EXPLICIT_NOTIFY);

  /**
   * PangoLayout:width: (attributes org.gtk.Property.get=pango_layout_get_width org.gtk.Property.set=pango_layout_set_width)
   *
   * The width to which the text of `PangoLayout` will be broken.
   *
   * The width is specified in Pango units, with -1 meaning unlimited.
   *
   * The default value is -1.
   */
  props[PROP_WIDTH] = g_param_spec_int ("width", "width", "width",
                                        -1, G_MAXINT, -1,
                                        G_PARAM_READWRITE | G_PARAM_EXPLICIT_NOTIFY);

  /**
   * PangoLayout:height: (attributes org.gtk.Property.get=pango_layout_get_height org.gtk.Property.set=pango_layout_set_height)
   *
   * The height to which the `PangoLayout` will be ellipsized.
   *
   * If @height is positive, it will be the maximum height of the
   * layout. Only lines would be shown that would fit, and if there
   * is any text omitted, an ellipsis added. At least one line is
   * included in each paragraph regardless of how small the height
   * value is. A value of zero will render exactly one line for the
   * entire layout.
   *
   * If @height is negative, it will be the (negative of) maximum
   * number of lines per paragraph. That is, the total number of lines
   * shown may well be more than this value if the layout contains
   * multiple paragraphs of text.
   *
   * The default value of -1 means that the first line of each
   * paragraph is ellipsized.
   *
   * Height setting only has effect if a positive width is set on the
   * layout and its ellipsization mode is not `PANGO_ELLIPSIZE_NONE`.
   * The behavior is undefined if a height other than -1 is set and
   * ellipsization mode is set to `PANGO_ELLIPSIZE_NONE`.
   *
   * The default value is -1.
   */
  props[PROP_HEIGHT] = g_param_spec_int ("height", "height", "height",
                                         -G_MAXINT, G_MAXINT, -1,
                                         G_PARAM_READWRITE | G_PARAM_EXPLICIT_NOTIFY);

  /**
   * PangoLayout:tabs: (attributes org.gtk.Property.get=pango_layout_get_tabs org.gtk.Property.set=pango_layout_set_tabs)
   *
   * The tabs to use when formatting the text of `PangoLayout`.
   *
   * `PangoLayout` will place content at the next tab position
   * whenever it meets a Tab character (U+0009).
   */
  props[PROP_TABS] = g_param_spec_boxed ("tabs", "tabs", "tabs",
                                         PANGO_TYPE_TAB_ARRAY,
                                         G_PARAM_READWRITE | G_PARAM_EXPLICIT_NOTIFY);

  /**
   * PangoLayout:single-paragraph: (attributes org.gtk.Property.get=pango_layout_get_single_paragraph org.gtk.Property.set=pango_layout_set_single_paragraph)
   *
   * Whether to treat newlines and similar characters as paragraph
   * separators or not. If this property is `TRUE`, all text is kept
   * in a single paragraph, and paragraph separator characters are
   * displayed with a glyph.
   *
   * This is useful to allow editing of newlines on a single text line.
   *
   * The default value is `FALSE`.
   */
  props[PROP_SINGLE_PARAGRAPH] = g_param_spec_boolean ("single-paragraph", "single-paragraph", "single-paragraph",
                                                       FALSE,
                                                       G_PARAM_READWRITE | G_PARAM_EXPLICIT_NOTIFY);

  /**
   * PangoLayout:wrap: (attributes org.gtk.Property.get=pango_layout_get_wrap org.gtk.Property.set=pango_layout_set_wrap)
   *
   * The wrap mode of this `PangoLayout.
   *
   * The wrap mode influences how Pango chooses line breaks
   * when text needs to be wrapped.
   *
   * The default value is `PANGO_WRAP_WORD`.
   */
  props[PROP_WRAP] = g_param_spec_enum ("wrap", "wrap", "wrap",
                                        PANGO_TYPE_WRAP_MODE,
                                        PANGO_WRAP_WORD,
                                        G_PARAM_READWRITE | G_PARAM_EXPLICIT_NOTIFY);

  /**
   * PangoLayout:indent: (attributes org.gtk.Property.get=pango_layout_get_indent org.gtk.Property.set=pango_layout_set_indent)
   *
   * The indent of this `PangoLayout.
   *
   * The indent is specified in Pango units.
   *
   * A negative value of @indent will produce a hanging indentation.
   * That is, the first line will have the full width, and subsequent
   * lines will be indented by the absolute value of @indent.
   *
   * The default value is 0.
   */
  props[PROP_INDENT] = g_param_spec_int ("indent", "indent", "indent",
                                         G_MININT, G_MAXINT, 0,
                                         G_PARAM_READWRITE | G_PARAM_EXPLICIT_NOTIFY);

  /**
   * PangoLayout:alignment: (attributes org.gtk.Property.get=pango_layout_get_alignment org.gtk.Property.set=pango_layout_set_alignment)
   *
   * The alignment mode of this `PangoLayout.
   *
   * The default value is `PANGO_ALIGNMENT_LEFT`.
   */
  props[PROP_ALIGNMENT] = g_param_spec_enum ("alignment", "alignment", "alignment",
                                             PANGO_TYPE_ALIGNMENT,
                                             PANGO_ALIGN_LEFT,
                                             G_PARAM_READWRITE | G_PARAM_EXPLICIT_NOTIFY);

  /**
   * PangoLayout:ellipsize: (attributes org.gtk.Property.get=pango_layout_get_ellipsize org.gtk.Property.set=pango_layout_set_ellipsize)
   *
   * The ellipsization mode of this `PangoLayout.
   *
   * The default value is `PANGO_ELLIPSIZE_NONE`.
   */
  props[PROP_ELLIPSIZE] = g_param_spec_enum ("ellipsize", "ellipsize", "ellipsize",
                                             PANGO_TYPE_ELLIPSIZE_MODE,
                                             PANGO_ELLIPSIZE_NONE,
                                             G_PARAM_READWRITE | G_PARAM_EXPLICIT_NOTIFY);

  /**
   * PangoLayout:auto-dir: (attributes org.gtk.Property.get=pango_layout_get_auto_dir org.gtk.Property.set=pango_layout_set_auto_dir)
   *
   * Whether this `PangoLayout` determines the
   * base direction from the content.
   *
   * The default value is `TRUE`.
   */
  props[PROP_AUTO_DIR] = g_param_spec_boolean ("auto-dir", "auto-dir", "auto-dir",
                                               TRUE,
                                               G_PARAM_READWRITE | G_PARAM_EXPLICIT_NOTIFY);

  /**
   * PangoLayout:lines: (attributes org.gtk.Property.get=pango_layout_get_lines)
   *
   * The `PangoLines` object holding the formatted lines.
   */
  props[PROP_LINES] = g_param_spec_object ("lines", "lines", "lines",
                                           PANGO_TYPE_LINES,
                                           G_PARAM_READABLE);

  g_object_class_install_properties (object_class, NUM_PROPERTIES, props);
}

/*  }}} */
/* {{{ Utilities */

static void
layout_changed (PangoLayout *layout)
{
  layout->serial++;
  if (layout->serial == 0)
    layout->serial++;

  g_clear_object (&layout->lines);
  g_object_notify_by_pspec (G_OBJECT (layout), props[PROP_LINES]);
}

static void
check_context_changed (PangoLayout *layout)
{
  guint old_serial = layout->context_serial;

  layout->context_serial = pango_context_get_serial (layout->context);

  if (old_serial != layout->context_serial)
    pango_layout_context_changed (layout);
}

static PangoAttrList *
ensure_attrs (PangoLayout   *layout,
              PangoAttrList *attrs)
{
  if (attrs)
    return attrs;
  else if (layout->attrs)
    return pango_attr_list_copy (layout->attrs);
  else
    return pango_attr_list_new ();
}

static PangoAttrList *
get_effective_attributes (PangoLayout *layout)
{
  PangoAttrList *attrs = NULL;

  if (layout->font_desc)
    {
      attrs = ensure_attrs (layout, attrs);
      pango_attr_list_insert_before (attrs,
                                     pango_attr_font_desc_new (layout->font_desc));
    }

  if (layout->line_spacing != 0.0)
    {
      attrs = ensure_attrs (layout, attrs);
      pango_attr_list_insert_before (attrs,
                                     pango_attr_line_height_new (layout->line_spacing));
    }

  if (layout->single_paragraph)
    {
      attrs = ensure_attrs (layout, attrs);
      pango_attr_list_insert_before (attrs,
                                     pango_attr_paragraph_new ());
    }

  if (attrs)
    return attrs;

  return pango_attr_list_ref (layout->attrs);
}

static gboolean
ends_with_paragraph_separator (PangoLayout *layout)
{
  if (layout->single_paragraph)
    return FALSE;

  return g_str_has_suffix (layout->text, "\n") ||
         g_str_has_suffix (layout->text, "\r") ||
         g_str_has_suffix (layout->text, "\r\n") ||
         g_str_has_suffix (layout->text, " ");
}

static void
ensure_lines (PangoLayout *layout)
{
  PangoLineBreaker *breaker;
  PangoAttrList *attrs;
  int x, y, width;
  int line_no;

  check_context_changed (layout);

  if (layout->lines)
    return;

  breaker = pango_line_breaker_new (layout->context);

  pango_line_breaker_set_tabs (breaker, layout->tabs);
  pango_line_breaker_set_base_dir (breaker,
                                   layout->auto_dir
                                     ? PANGO_DIRECTION_NEUTRAL
                                     : pango_context_get_base_dir (layout->context));

  attrs = get_effective_attributes (layout);
  pango_line_breaker_add_text (breaker, layout->text ? layout->text : "", -1, attrs);
  if (attrs)
    pango_attr_list_unref (attrs);

  layout->lines = pango_lines_new ();

  x = y = 0;
  line_no = 0;
  while (pango_line_breaker_has_line (breaker))
    {
      PangoLine *line;
      PangoRectangle ext;
      int offset;
      PangoEllipsizeMode ellipsize = PANGO_ELLIPSIZE_NONE;

      if ((line_no == 0) == (layout->indent > 0))
        {
          x = abs (layout->indent);
          width = layout->width - x;
        }
      else
        {
          x = 0;
          width = layout->width;
        }

      if (layout->height < 0 && line_no + 1 == - layout->height)
        ellipsize = layout->ellipsize;

retry:
      line = pango_line_breaker_next_line (breaker, x, width, layout->wrap, ellipsize);
      pango_line_get_extents (line, NULL, &ext);

      if (layout->height >= 0 && y + 2 * ext.height >= layout->height &&
          ellipsize != layout->ellipsize)
        {
          if (pango_line_breaker_undo_line (breaker, line))
            {
              g_clear_pointer (&line, pango_line_free);
              ellipsize = layout->ellipsize;
              goto retry;
            }
        }

      /* Handle alignment and justification */
      offset = 0;
      switch (layout->alignment)
        {
        case PANGO_ALIGN_LEFT:
          break;
        case PANGO_ALIGN_CENTER:
          if (ext.width < width)
            offset = (width - ext.width) / 2;
          break;
        case PANGO_ALIGN_RIGHT:
          if (ext.width < width)
            offset = width - ext.width;
          break;
        case PANGO_ALIGN_JUSTIFY:
          if (!pango_line_is_paragraph_end (line))
            line = pango_line_justify (line, width);
          break;
        case PANGO_ALIGN_JUSTIFY_ALL:
          line = pango_line_justify (line, width);
          break;
        default: g_assert_not_reached ();
        }

      pango_lines_add_line (layout->lines, line, x + offset, y - ext.y);

      y += ext.height;
      line_no++;
    }

  /* Append an empty line if we end with a newline.
   * And always provide at least one line
   */
  if (pango_lines_get_line_count (layout->lines) == 0 ||
      ends_with_paragraph_separator (layout))
    {
      LineData *data;
      int start_index;
      int start_offset;
      int offset;
      PangoLine *line;
      PangoRectangle ext;

      if (pango_lines_get_line_count (layout->lines) > 0)
        {
          PangoLine *last;

          last = pango_lines_get_line (layout->lines,
                                       pango_lines_get_line_count (layout->lines) - 1,
                                       NULL, NULL);
          data = line_data_ref (last->data);
          start_index = data->length;
          start_offset = last->data->n_chars;
          offset = MAX (layout->indent, 0);
        }
      else
        {
          data = line_data_new ();
          data->text = g_strdup ("");
          data->length = 0;
          data->attrs = get_effective_attributes (layout);
          data->log_attrs = g_new0 (PangoLogAttr, 1);
          data->log_attrs[0].is_cursor_position = TRUE;
          start_index = 0;
          start_offset = 0;
          offset = 0;
        }

      line = pango_line_new (layout->context, data);
      line->starts_paragraph = TRUE;
      line->ends_paragraph = TRUE;
      line->start_index = start_index;
      line->length = 0;
      line->start_offset = start_offset;
      line->n_chars = 0;

      pango_line_get_extents (line, NULL, &ext);

      pango_lines_add_line (layout->lines, line, x + offset, y - ext.y);

      line_data_unref (data);
    }

  g_object_unref (breaker);
}

/* }}} */
/* {{{ Public API */

/**
 * pango_layout_new:
 * @context: a `PangoContext`
 *
 * Creates a new `PangoLayout` with attribute initialized to
 * default values for a particular `PangoContext`
 *
 * Return value: a newly allocated `PangoLayout`
 */
PangoLayout *
pango_layout_new (PangoContext *context)
{
  g_return_val_if_fail (PANGO_IS_CONTEXT (context), NULL);

  return g_object_new (PANGO_TYPE_LAYOUT, "context", context, NULL);
}

/**
 * pango_layout_copy:
 * @layout: a `PangoLayout`
 *
 * Creates a deep copy-by-value of the layout.
 *
 * The attribute list, tab array, and text from the original layout
 * are all copied by value.
 *
 * Return value: (transfer full): the newly allocated `PangoLayout`
 */
PangoLayout *
pango_layout_copy (PangoLayout *layout)
{
  PangoLayout *copy;

  g_return_val_if_fail (PANGO_IS_LAYOUT (layout), NULL);

  copy = pango_layout_new (layout->context);

  copy->text = g_strdup (layout->text);
  copy->length = layout->length;
  if (layout->attrs)
    copy->attrs = pango_attr_list_copy (layout->attrs);
  if (layout->font_desc)
    copy->font_desc = pango_font_description_copy (layout->font_desc);
  copy->line_spacing = layout->line_spacing;
  copy->width = layout->width;
  copy->height = layout->height;
  if (layout->tabs)
    copy->tabs = pango_tab_array_copy (layout->tabs);
  copy->single_paragraph = layout->single_paragraph;
  copy->wrap = layout->wrap;
  copy->indent = layout->indent;
  copy->serial = layout->serial;
  copy->context_serial = layout->context_serial;
  copy->alignment = layout->alignment;
  copy->ellipsize = layout->ellipsize;
  copy->auto_dir = layout->auto_dir;

  return copy;
}

/**
 * pango_layout_get_serial:
 * @layout: a `PangoLayout`
 *
 * Returns the current serial number of the layout.
 *
 * The serial number is initialized to an small number larger than zero
 * when a new layout is created and is increased whenever the layout is
 * changed using any of the setter functions, or the `PangoContext` it
 * uses has changed.
 *
 * The serial may wrap, but will never have the value 0. Since it can
 * wrap, never compare it with "less than", always use "not equals".
 *
 * This can be used to automatically detect changes to a `PangoLayout`,
 * and is useful for example to decide whether a layout needs redrawing.
 *
 * Return value: The current serial number of @layout
 */
guint
pango_layout_get_serial (PangoLayout *layout)
{
  check_context_changed (layout);

  return layout->serial;
}

/**
 * pango_layout_context_changed:
 * @layout: a `PangoLayout`
 *
 * Forces recomputation of any state in the `PangoLayout` that
 * might depend on the layout's context.
 *
 * This function should be called if you make changes to the
 * context subsequent to creating the layout.
 */
void
pango_layout_context_changed (PangoLayout *layout)
{
  g_return_if_fail (PANGO_IS_LAYOUT (layout));

  layout_changed (layout);
}

/* {{{ Property getters and setters */

/**
 * pango_layout_get_context:
 * @layout: a `PangoLayout`
 *
 * Retrieves the `PangoContext` used for this layout.
 *
 * Return value: (transfer none): the `PangoContext` for the layout
 */
PangoContext *
pango_layout_get_context (PangoLayout *layout)
{
  g_return_val_if_fail (PANGO_IS_LAYOUT (layout), NULL);

  return layout->context;
}

/**
 * pango_layout_set_text:
 * @layout: a `PangoLayout`
 * @text: the text
 * @length: maximum length of @text, in bytes. -1 indicates that
 *   the string is nul-terminated
 *
 * Sets the text of the layout.
 */
void
pango_layout_set_text (PangoLayout *layout,
                       const char  *text,
                       int          length)
{
  g_return_if_fail (PANGO_IS_LAYOUT (layout));

  if (length < 0)
    length = strlen (text);

  g_free (layout->text);
  layout->text = g_strndup (text, length);
  layout->length = length;

  g_object_notify_by_pspec (G_OBJECT (layout), props[PROP_TEXT]);
  layout_changed (layout);
}

/**
 * pango_layout_get_text:
 * @layout: a `PangoLayout`
 *
 * Gets the text in the layout.
 *
 * The returned text should not be freed or modified.
 *
 * Return value: (transfer none): the text in the @layout
 */
const char *
pango_layout_get_text (PangoLayout *layout)
{
  g_return_val_if_fail (PANGO_IS_LAYOUT (layout), NULL);

  return layout->text;
}

/**
 * pango_layout_set_attributes:
 * @layout: a `PangoLayout`
 * @attrs: (nullable) (transfer none): a `PangoAttrList`
 *
 * Sets the attributes for a layout object.
 *
 * References @attrs, so the caller can unref its reference.
 */
void
pango_layout_set_attributes (PangoLayout   *layout,
                             PangoAttrList *attrs)
{
  g_return_if_fail (PANGO_IS_LAYOUT (layout));

  g_clear_pointer (&layout->attrs, pango_attr_list_unref);
  layout->attrs = attrs;
  if (layout->attrs)
    pango_attr_list_ref (layout->attrs);

  g_object_notify_by_pspec (G_OBJECT (layout), props[PROP_ATTRIBUTES]);
  layout_changed (layout);
}

/**
 * pango_layout_get_attributes:
 * @layout: a `PangoLayout`
 *
 * Gets the attribute list for the layout, if any.
 *
 * Return value: (transfer none) (nullable): a `PangoAttrList`
 */
PangoAttrList *
pango_layout_get_attributes (PangoLayout *layout)
{
  g_return_val_if_fail (PANGO_IS_LAYOUT (layout), NULL);

  return layout->attrs;
}

/**
 * pango_layout_set_font_description:
 * @layout: a `PangoLayout`
 * @desc: (nullable): the new `PangoFontDescription`
 *
 * Sets the default font description for the layout.
 *
 * If no font description is set on the layout, the
 * font description from the layout's context is used.
 *
 * This method is a shorthand for adding a font-desc attribute.
 */
void
pango_layout_set_font_description (PangoLayout                *layout,
                                   const PangoFontDescription *desc)
{
  g_return_if_fail (PANGO_IS_LAYOUT (layout));

  if (desc != layout->font_desc &&
      (!desc || !layout->font_desc || !pango_font_description_equal (desc, layout->font_desc)))
    {
      if (layout->font_desc)
        pango_font_description_free (layout->font_desc);

      layout->font_desc = desc ? pango_font_description_copy (desc) : NULL;

      g_object_notify_by_pspec (G_OBJECT (layout), props[PROP_FONT_DESCRIPTION]);
      layout_changed (layout);
    }
}

/**
 * pango_layout_get_font_description:
 * @layout: a `PangoLayout`
 *
 * Gets the font description for the layout, if any.
 *
 * Return value: (transfer none) (nullable): a pointer to the
 *   layout's font description, or %NULL if the font description
 *   from the layout's context is inherited.
 *
 * Since: 1.8
 */
const PangoFontDescription *
pango_layout_get_font_description (PangoLayout *layout)
{
  g_return_val_if_fail (PANGO_IS_LAYOUT (layout), NULL);

  return layout->font_desc;
}

/**
 * pango_layout_set_line_spacing:
 * @layout: a `PangoLayout`
 * @line_spacing: the new line spacing factor
 *
 * Sets a factor for line spacing.
 *
 * Typical values are: 0, 1, 1.5, 2. The default values is 0.
 *
 * If @line_spacing is non-zero, lines are placed so that
 *
 *     baseline2 = baseline1 + factor * height2
 *
 * where height2 is the line height of the second line (as determined
 * by the font).
 *
 * This method is a shorthand for adding a line-height attribute.
 */
void
pango_layout_set_line_spacing (PangoLayout *layout,
                               float        line_spacing)
{
  g_return_if_fail (PANGO_IS_LAYOUT (layout));

  if (layout->line_spacing == line_spacing)
    return;

  layout->line_spacing = line_spacing;

  g_object_notify_by_pspec (G_OBJECT (layout), props[PROP_LINE_SPACING]);
  layout_changed (layout);
}

/**
 * pango_layout_get_line_spacing:
 * @layout: a `PangoLayout`
 *
 * Gets the line spacing factor of @layout.
 *
 * See [method@Pango.Layout.set_line_spacing].
 */
float
pango_layout_get_line_spacing (PangoLayout *layout)
{
  g_return_val_if_fail (PANGO_IS_LAYOUT (layout), 0.0);

  return layout->line_spacing;
}

/**
 * pango_layout_set_width:
 * @layout: a `PangoLayout`.
 * @width: the desired width in Pango units, or -1 to indicate that no
 *   wrapping or ellipsization should be performed.
 *
 * Sets the width to which the lines of the layout should
 * be wrapped or ellipsized.
 *
 * The default value is -1: no width set.
 */
void
pango_layout_set_width (PangoLayout *layout,
                        int          width)
{
  g_return_if_fail (PANGO_IS_LAYOUT (layout));

  if (width < -1)
    width = -1;

  if (layout->width == width)
    return;

  layout->width = width;

  g_object_notify_by_pspec (G_OBJECT (layout), props[PROP_WIDTH]);
  layout_changed (layout);
}

/**
 * pango_layout_get_width:
 * @layout: a `PangoLayout`
 *
 * Gets the width to which the lines of the layout should wrap.
 *
 * Return value: the width in Pango units, or -1 if no width set.
 */
int
pango_layout_get_width (PangoLayout *layout)
{
  g_return_val_if_fail (PANGO_IS_LAYOUT (layout), -1);

  return layout->width;
}

/**
 * pango_layout_set_height:
 * @layout: a `PangoLayout`.
 * @height: the desired height
 *
 * Sets the height to which the `PangoLayout` should be ellipsized at.
 *
 * There are two different behaviors, based on whether @height is positive
 * or negative.
 *
 * See [property@Pango.Layout:height] for details.
 */
void
pango_layout_set_height (PangoLayout *layout,
                         int          height)
{
  g_return_if_fail (PANGO_IS_LAYOUT (layout));

  if (layout->height == height)
    return;

  layout->height = height;

  g_object_notify_by_pspec (G_OBJECT (layout), props[PROP_HEIGHT]);
  layout_changed (layout);
}

/**
 * pango_layout_get_height:
 * @layout: a `PangoLayout`
 *
 * Gets the height to which the lines of the layout should ellipsize.
 *
 * See [property@Pango.Layout:height] for details.
 *
 * Return value: the height
 */
int
pango_layout_get_height (PangoLayout *layout)
{
  g_return_val_if_fail (PANGO_IS_LAYOUT (layout), -1);

  return layout->height;
}

/**
 * pango_layout_set_tabs:
 * @layout: a `PangoLayout`
 * @tabs: (nullable): a `PangoTabArray`
 *
 * Sets the tabs to use for the layout, overriding the
 * default tabs.
 *
 * Setting the tabs to `NULL` reinstates the default
 * tabs.
 *
 * See [method@Pango.LineBreaker.set_tabs] for details.
 */
void
pango_layout_set_tabs (PangoLayout   *layout,
                       PangoTabArray *tabs)
{
  g_return_if_fail (PANGO_IS_LAYOUT (layout));

  if (layout->tabs == tabs)
    return;

  g_clear_pointer (&layout->tabs, pango_tab_array_free);
  if (tabs)
    layout->tabs = pango_tab_array_copy (tabs);

  g_object_notify_by_pspec (G_OBJECT (layout), props[PROP_TABS]);
  layout_changed (layout);
}

/**
 * pango_layout_get_tabs:
 * @layout: a `PangoLayout`
 *
 * Gets the current `PangoTabArray` used by this layout.
 *
 * If no `PangoTabArray` has been set, then the default tabs are
 * in use and %NULL is returned. Default tabs are every 8 spaces.
 *
 * Return value: (transfer none) (nullable): the tabs for this layout
 */
PangoTabArray *
pango_layout_get_tabs (PangoLayout *layout)
{
  g_return_val_if_fail (PANGO_IS_LAYOUT (layout), NULL);

  return layout->tabs;
}

/**
 * pango_layout_set_single_paragraph:
 * @layout: a `PangoLayout`
 * @single_paragraph: the new setting
 *
 * Sets the single paragraph mode of the layout.
 *
 * If @single_paragraph is `TRUE`, do not treat newlines and similar
 * characters as paragraph separators; instead, keep all text in a
 * single paragraph, and display a glyph for paragraph separator
 * characters.
 *
 * Used when you want to allow editing of newlines on a single text line.
 *
 * The default value is %FALSE.
 */
void
pango_layout_set_single_paragraph (PangoLayout *layout,
                                   gboolean     single_paragraph)
{
  g_return_if_fail (PANGO_IS_LAYOUT (layout));

  if (layout->single_paragraph == single_paragraph)
    return;

  layout->single_paragraph = single_paragraph;

  g_object_notify_by_pspec (G_OBJECT (layout), props[PROP_SINGLE_PARAGRAPH]);
  layout_changed (layout);
}

/**
 * pango_layout_get_single_paragraph:
 * @layout: a `PangoLayout`
 *
 * Obtains whether this layout is in single paragraph mode.
 *
 * See [method@Pango.Layout.set_single_paragraph].
 *
 * Return value: `TRUE` if the layout does not break paragraphs
 *   at paragraph separator characters, %FALSE otherwise
 */
gboolean
pango_layout_get_single_paragraph (PangoLayout *layout)
{
  g_return_val_if_fail (PANGO_IS_LAYOUT (layout), FALSE);

  return layout->single_paragraph;
}

/**
 * pango_layout_set_wrap:
 * @layout: a `PangoLayout`
 * @wrap: the wrap mode
 *
 * Sets the wrap mode.
 *
 * The wrap mode only has effect if a width is set on the layout
 * with [method@Pango.Layout.set_width]. To turn off wrapping,
 * set the width to -1.
 *
 * The default value is %PANGO_WRAP_WORD.
 */
void
pango_layout_set_wrap (PangoLayout   *layout,
                       PangoWrapMode  wrap)
{
  g_return_if_fail (PANGO_IS_LAYOUT (layout));

  if (layout->wrap == wrap)
    return;

  layout->wrap = wrap;

  g_object_notify_by_pspec (G_OBJECT (layout), props[PROP_WRAP]);
  layout_changed (layout);
}

/**
 * pango_layout_get_wrap:
 * @layout: a `PangoLayout`
 *
 * Gets the wrap mode for the layout.
 *
 * Return value: active wrap mode.
 */
PangoWrapMode
pango_layout_get_wrap (PangoLayout *layout)
{
  g_return_val_if_fail (PANGO_IS_LAYOUT (layout), PANGO_WRAP_WORD);

  return layout->wrap;
}

/**
 * pango_layout_set_indent:
 * @layout: a `PangoLayout`
 * @indent: the amount by which to indent
 *
 * Sets the width in Pango units to indent each paragraph.
 *
 * A negative value of @indent will produce a hanging indentation.
 * That is, the first line will have the full width, and subsequent
 * lines will be indented by the absolute value of @indent.
 *
 * The default value is 0.
 */
void
pango_layout_set_indent (PangoLayout *layout,
                         int          indent)
{
  g_return_if_fail (PANGO_IS_LAYOUT (layout));

  if (layout->indent == indent)
    return;

  layout->indent = indent;

  g_object_notify_by_pspec (G_OBJECT (layout), props[PROP_INDENT]);
  layout_changed (layout);
}

/**
 * pango_layout_get_indent:
 * @layout: a `PangoLayout`
 *
 * Gets the paragraph indent width in Pango units.
 *
 * A negative value indicates a hanging indentation.
 *
 * Return value: the indent in Pango units
 */
int
pango_layout_get_indent (PangoLayout *layout)
{
  g_return_val_if_fail (PANGO_IS_LAYOUT (layout), 0);

  return layout->indent;
}

/**
 * pango_layout_set_alignment:
 * @layout: a `PangoLayout`
 * @alignment: the alignment
 *
 * Sets the alignment for the layout.
 *
 * The alignment determines how short lines are
 * positioned within the available horizontal space.
 *
 * The default alignment is `PANGO_ALIGN_LEFT`.
 */
void
pango_layout_set_alignment (PangoLayout    *layout,
                            PangoAlignment  alignment)
{
  g_return_if_fail (PANGO_IS_LAYOUT (layout));

  if (layout->alignment == alignment)
    return;

  layout->alignment = alignment;

  g_object_notify_by_pspec (G_OBJECT (layout), props[PROP_ALIGNMENT]);
  layout_changed (layout);
}

/**
 * pango_layout_get_alignment:
 * @layout: a `PangoLayout`
 *
 * Gets the alignment for the layout.
 *
 * The alignment determines how short lines are
 * positioned within the available horizontal space.
 *
 * Return value: the alignment
 */
PangoAlignment
pango_layout_get_alignment (PangoLayout *layout)
{
  g_return_val_if_fail (PANGO_IS_LAYOUT (layout), PANGO_ALIGN_LEFT);

  return layout->alignment;
}

/**
 * pango_layout_set_ellipsize:
 * @layout: a `PangoLayout`
 * @ellipsize: the new ellipsization mode for @layout
 *
 * Sets the type of ellipsization to use for this layout.
 *
 * Depending on the ellipsization mode @ellipsize text is removed
 * from the start, middle, or end of text so they fit within the
 * width of layout set with [method@Pango.Layout.set_width].
 *
 * The default value is `PANGO_ELLIPSIZE_NONE`.
 */
void
pango_layout_set_ellipsize (PangoLayout        *layout,
                            PangoEllipsizeMode  ellipsize)
{
  g_return_if_fail (PANGO_IS_LAYOUT (layout));

  if (layout->ellipsize == ellipsize)
    return;

  layout->ellipsize = ellipsize;

  g_object_notify_by_pspec (G_OBJECT (layout), props[PROP_ELLIPSIZE]);
  layout_changed (layout);
}

/**
 * pango_layout_get_ellipsize:
 * @layout: a `PangoLayout`
 *
 * Gets the type of ellipsization being performed for @layout.
 *
 * See [method@Pango.Layout.set_ellipsize].
 *
 * Return value: the current ellipsization mode for @layout
 */
PangoEllipsizeMode
pango_layout_get_ellipsize (PangoLayout *layout)
{
  g_return_val_if_fail (PANGO_IS_LAYOUT (layout), PANGO_ELLIPSIZE_NONE);

  return layout->ellipsize;
}

/**
 * pango_layout_set_auto_dir:
 * @layout: a `PangoLayout`
 * @auto_dir: if %TRUE, compute the bidirectional base direction
 *   from the layout's contents
 *
 * Sets whether to calculate the base direction
 * for the layout according to its contents.
 *
 * When this flag is on (the default), then paragraphs in
 * @layout that begin with strong right-to-left characters
 * (Arabic and Hebrew principally), will have right-to-left
 * layout, paragraphs with letters from other scripts will
 * have left-to-right layout. Paragraphs with only neutral
 * characters get their direction from the surrounding
 * paragraphs.
 *
 * When `FALSE`, the choice between left-to-right and right-to-left
 * layout is done according to the base direction of the layout's
 * `PangoContext`. (See [method@Pango.Context.set_base_dir]).
 *
 * When the auto-computed direction of a paragraph differs
 * from the base direction of the context, the interpretation
 * of `PANGO_ALIGN_LEFT` and `PANGO_ALIGN_RIGHT` are swapped.
 */
void
pango_layout_set_auto_dir (PangoLayout *layout,
                           gboolean     auto_dir)
{
  g_return_if_fail (PANGO_IS_LAYOUT (layout));

  if (auto_dir == layout->auto_dir)
    return;

  layout->auto_dir = auto_dir;

  g_object_notify_by_pspec (G_OBJECT (layout), props[PROP_AUTO_DIR]);
  layout_changed (layout);
}

/**
 * pango_layout_get_auto_dir:
 * @layout: a `PangoLayout`
 *
 * Gets whether to calculate the base direction for the layout
 * according to its contents.
 *
 * See [method@Pango.Layout.set_auto_dir].
 *
 * Return value: `TRUE` if the bidirectional base direction
 *   is computed from the layout's contents, `FALSE` otherwise
 */
gboolean
pango_layout_get_auto_dir (PangoLayout *layout)
{
  g_return_val_if_fail (PANGO_IS_LAYOUT (layout), TRUE);

  return layout->auto_dir;
}

/* }}} */
/* {{{ Miscellaneous */

/**
 * pango_layout_set_markup:
 * @layout: a `PangoLayout`
 * @markup: marked-up text
 * @length: length of @markup in bytes, or -1 if it is `NUL`-terminated
 *
 * Sets the layout text and attribute list from marked-up text.
 *
 * See [Pango Markup](pango_markup.html)).
 *
 * Replaces the current text and attributes.
 */
void
pango_layout_set_markup (PangoLayout  *layout,
                         const char   *markup,
                         int           length)
{
  PangoAttrList *attrs;
  char *text;
  GError *error = NULL;

  g_return_if_fail (PANGO_IS_LAYOUT (layout));
  g_return_if_fail (markup != NULL);

  if (!pango_parse_markup (markup, length, 0, &attrs, &text, NULL, &error))
    {
      g_warning ("pango_layout_set_markup_with_accel: %s", error->message);
      g_error_free (error);
      return;
    }

  g_free (layout->text);
  layout->text = text;
  layout->length = strlen (text);

  g_clear_pointer (&layout->attrs, pango_attr_list_unref);
  layout->attrs = attrs;

  g_object_notify_by_pspec (G_OBJECT (layout), props[PROP_TEXT]);
  g_object_notify_by_pspec (G_OBJECT (layout), props[PROP_ATTRIBUTES]);
  layout_changed (layout);
}

/**
 * pango_layout_get_character_count:
 * @layout: a `PangoLayout`
 *
 * Returns the number of Unicode characters in the
 * the text of the layout.
 *
 * Return value: the number of Unicode characters in @layout
 */
int
pango_layout_get_character_count (PangoLayout *layout)
{
  PangoLine *line;

  g_return_val_if_fail (PANGO_IS_LAYOUT (layout), 0);

  ensure_lines (layout);

  line = pango_lines_get_line (layout->lines, 0, NULL, NULL);

  return line->data->n_chars;
}

/* }}} */
/* {{{ Output getters */

/**
 * pango_layout_get_lines:
 * @layout: a `PangoLayout`
 *
 * Gets the lines of the @layout.
 *
 * The returned object will become invalid when any
 * property of @layout is changed. Take a reference
 * to keep it.
 *
 * Return value: (transfer none): a `PangoLines` object
 *   with the lines of @layout
 */
PangoLines *
pango_layout_get_lines (PangoLayout *layout)
{
  g_return_val_if_fail (PANGO_IS_LAYOUT (layout), NULL);

  ensure_lines (layout);

  return layout->lines;
}

/**
 * pango_layout_get_log_attrs:
 * @layout: a `PangoLayout`
 * @n_attrs: (out): return location for the length of the array
 *
 * Gets the `PangoLogAttr` array for the content
 * of @layout.
 *
 * The returned array becomes invalid when
 * any properties of @layout change. Make a
 * copy if you want to keep it.
 *
 * Returns: (transfer none): the `PangoLogAttr` array
 */
const PangoLogAttr *
pango_layout_get_log_attrs (PangoLayout *layout,
                            int         *n_attrs)
{
  PangoLine *line;

  g_return_val_if_fail (PANGO_IS_LAYOUT (layout), NULL);

  ensure_lines (layout);

  line = pango_lines_get_line (layout->lines, 0, NULL, NULL);

  if (n_attrs)
    *n_attrs = line->data->n_chars + 1;

  return line->data->log_attrs;
}

/**
 * pango_layout_get_iter:
 * @layout: a `PangoLayout`
 *
 * Returns an iterator to iterate over the visual extents
 * of the layout.
 *
 * This is a convenience wrapper for [method@Pango.Lines.get_iter].
 *
 * Returns: the new `PangoLineIter`
 */
PangoLineIter *
pango_layout_get_iter (PangoLayout *layout)
{
  g_return_val_if_fail (PANGO_IS_LAYOUT (layout), NULL);

  ensure_lines (layout);

  return pango_lines_get_iter (layout->lines);
}

/* }}} */
/* }}} */

/* vim:set foldmethod=marker expandtab: */
