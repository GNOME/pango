#include "config.h"

#include "pango-layout.h"
#include "pango-line-breaker.h"
#include "pango-line-private.h"
#include "pango-enum-types.h"
#include "pango-markup.h"
#include "pango-context.h"

/**
 * Pango2Layout:
 *
 * Represents an entire paragraph of text.
 *
 * While complete access to the layout capabilities of Pango is provided
 * using the detailed interfaces for itemization, segmentation and shaping,
 * using that functionality directly involves writing a fairly large amount
 * of code. `Pango2Layout` provides a high-level driver for formatting entire
 * paragraphs of text at once. This includes paragraph-level functionality
 * such as line breaking, justification, alignment and ellipsization.
 *
 * A `Pango2Layout` is initialized with a `Pango2Context`, a UTF-8 string
 * and set of attributes for that string. Once that is done, the set of
 * formatted lines can be extracted in the form of a [class@Pango2.Lines]
 * object, the layout can be rendered, and conversion between logical
 * character positions within the layout's text, and the physical position
 * of the resulting glyphs can be made.
 *
 * The most convenient way to access the visual extents and components
 * of a formatted layout is via a [struct@Pango2.LineIter] iterator.
 *
 * There are a number of parameters to adjust the formatting of a
 * `Pango2Layout`. The following image shows adjustable parameters
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
 * resulting `Pango2Lines` object as a list of lines.
 *
 * If you have more complex line breaking needs, such as shaping text
 * to flow around images, or multi-column layout, the [class@Pango2.LineBreaker]
 * makes the underlying line-breaking functionality available outside of
 * `Pango2Layout`.
 */


/* {{{ Pango2Layout implementation */

struct _Pango2Layout
{
  GObject parent_instance;

  Pango2Context *context;
  char *text;
  int length;
  Pango2AttrList *attrs;
  Pango2FontDescription *font_desc;
  float line_height;
  int spacing;
  int width;
  int height;
  Pango2TabArray *tabs;
  gboolean single_paragraph;
  Pango2WrapMode wrap;
  int indent;
  guint serial;
  guint context_serial;
  Pango2Alignment alignment;
  Pango2EllipsizeMode ellipsize;
  gboolean auto_dir;

  Pango2Lines *lines;
};

struct _Pango2LayoutClass
{
  GObjectClass parent_class;
};

enum
{
  PROP_CONTEXT = 1,
  PROP_TEXT,
  PROP_ATTRIBUTES,
  PROP_FONT_DESCRIPTION,
  PROP_LINE_HEIGHT,
  PROP_SPACING,
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

G_DEFINE_FINAL_TYPE (Pango2Layout, pango2_layout, G_TYPE_OBJECT)

static void
pango2_layout_init (Pango2Layout *layout)
{
  layout->serial = 1;
  layout->width = -1;
  layout->height = -1;
  layout->indent = 0;
  layout->wrap = PANGO2_WRAP_WORD;
  layout->alignment = PANGO2_ALIGN_NATURAL;
  layout->ellipsize = PANGO2_ELLIPSIZE_NONE;
  layout->line_height = 0.0;
  layout->spacing = 0;
  layout->auto_dir = TRUE;
  layout->text = g_strdup ("");
  layout->length = 0;
}

static void
pango2_layout_finalize (GObject *object)
{
  Pango2Layout *layout = PANGO2_LAYOUT (object);

  g_clear_pointer (&layout->font_desc, pango2_font_description_free);
  g_object_unref (layout->context);
  g_free (layout->text);
  g_clear_pointer (&layout->attrs, pango2_attr_list_unref);
  g_clear_pointer (&layout->tabs, pango2_tab_array_free);
  g_clear_object (&layout->lines);

  G_OBJECT_CLASS (pango2_layout_parent_class)->finalize (object);
}

static void
pango2_layout_set_property (GObject      *object,
                            guint         prop_id,
                            const GValue *value,
                            GParamSpec   *pspec)
{
  Pango2Layout *layout = PANGO2_LAYOUT (object);

  switch (prop_id)
    {
    case PROP_CONTEXT:
      layout->context = g_value_dup_object (value);
      layout->context_serial = pango2_context_get_serial (layout->context);
      break;

    case PROP_TEXT:
      pango2_layout_set_text (layout, g_value_get_string (value), -1);
      break;

    case PROP_ATTRIBUTES:
      pango2_layout_set_attributes (layout, g_value_get_boxed (value));
      break;

    case PROP_FONT_DESCRIPTION:
      pango2_layout_set_font_description (layout, g_value_get_boxed (value));
      break;

    case PROP_LINE_HEIGHT:
      pango2_layout_set_line_height (layout, g_value_get_float (value));
      break;

    case PROP_SPACING:
      pango2_layout_set_spacing (layout, g_value_get_int (value));
      break;

    case PROP_WIDTH:
      pango2_layout_set_width (layout, g_value_get_int (value));
      break;

    case PROP_HEIGHT:
      pango2_layout_set_height (layout, g_value_get_int (value));
      break;

    case PROP_TABS:
      pango2_layout_set_tabs (layout, g_value_get_boxed (value));
      break;

    case PROP_SINGLE_PARAGRAPH:
      pango2_layout_set_single_paragraph (layout, g_value_get_boolean (value));
      break;

    case PROP_WRAP:
      pango2_layout_set_wrap (layout, g_value_get_enum (value));
      break;

    case PROP_INDENT:
      pango2_layout_set_indent (layout, g_value_get_int (value));
      break;

    case PROP_ALIGNMENT:
      pango2_layout_set_alignment (layout, g_value_get_enum (value));
      break;

    case PROP_ELLIPSIZE:
      pango2_layout_set_ellipsize (layout, g_value_get_enum (value));
      break;

    case PROP_AUTO_DIR:
      pango2_layout_set_auto_dir (layout, g_value_get_boolean (value));
      break;

    default:
      G_OBJECT_WARN_INVALID_PROPERTY_ID (object, prop_id, pspec);
      break;
    }
}

static void
pango2_layout_get_property (GObject      *object,
                            guint         prop_id,
                            GValue       *value,
                            GParamSpec   *pspec)
{
  Pango2Layout *layout = PANGO2_LAYOUT (object);

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

    case PROP_LINE_HEIGHT:
      g_value_set_float (value, layout->line_height);
      break;

    case PROP_SPACING:
      g_value_set_int (value, layout->spacing);
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
      g_value_set_object (value, pango2_layout_get_lines (layout));
      break;

    default:
      G_OBJECT_WARN_INVALID_PROPERTY_ID (object, prop_id, pspec);
      break;
    }
}

static void
pango2_layout_class_init (Pango2LayoutClass *class)
{
  GObjectClass *object_class = G_OBJECT_CLASS (class);

  object_class->finalize = pango2_layout_finalize;
  object_class->set_property = pango2_layout_set_property;
  object_class->get_property = pango2_layout_get_property;

  /**
   * Pango2Layout:context: (attributes org.gtk.Property.get=pango2_layout_get_context)
   *
   * The context for the `Pango2Layout`.
   */
  props[PROP_CONTEXT] =
      g_param_spec_object ("context", NULL, NULL,
                           PANGO2_TYPE_CONTEXT,
                           G_PARAM_READWRITE | G_PARAM_CONSTRUCT_ONLY | G_PARAM_STATIC_STRINGS);

  /**
   * Pango2Layout:text: (attributes org.gtk.Property.get=pango2_layout_get_text org.gtk.Property.set=pango2_layout_set_text)
   *
   * The text of the `Pango2Layout`.
   */
  props[PROP_TEXT] =
      g_param_spec_string ("text", NULL, NULL,
                           "",
                           G_PARAM_READWRITE | G_PARAM_EXPLICIT_NOTIFY | G_PARAM_STATIC_STRINGS);

  /**
   * Pango2Layout:attributes: (attributes org.gtk.Property.get=pango2_layout_get_attributes org.gtk.Property.set=pango2_layout_set_attributes)
   *
   * The attributes of the `Pango2Layout`.
   *
   * Attributes can affect how the text is formatted.
   */
  props[PROP_ATTRIBUTES] =
      g_param_spec_boxed ("attributes", NULL, NULL,
                          PANGO2_TYPE_ATTR_LIST,
                          G_PARAM_READWRITE | G_PARAM_EXPLICIT_NOTIFY | G_PARAM_STATIC_STRINGS);

  /**
   * Pango2Layout:font-description: (attributes org.gtk.Property.get=pango2_layout_get_font_description org.gtk.Property.set=pango2_layout_set_font_description)
   *
   * The font description of the `Pango2Layout`.
   */
  props[PROP_FONT_DESCRIPTION] =
      g_param_spec_boxed ("font-description", NULL, NULL,
                          PANGO2_TYPE_FONT_DESCRIPTION,
                          G_PARAM_READWRITE | G_PARAM_EXPLICIT_NOTIFY | G_PARAM_STATIC_STRINGS);

  /**
   * Pango2Layout:line-height: (attributes org.gtk.Property.get=pango2_layout_get_line_height org.gtk.Property.set=pango2_layout_set_line_height)
   *
   * The line height factor of the `Pango2Layout`.
   *
   * If non-zero, the line height is multiplied by this factor to determine
   * the logical extents of the text.
   *
   * The default value is 0.
   */
  props[PROP_LINE_HEIGHT] =
      g_param_spec_float ("line-height", NULL, NULL,
                          0., G_MAXFLOAT, 0.,
                          G_PARAM_READWRITE | G_PARAM_EXPLICIT_NOTIFY | G_PARAM_STATIC_STRINGS);

  /**
   * Pango2Layout:spacing: (attributes org.gtk.Property.get=pango2_layout_get_spacing org.gtk.Property.set=pango2_layout_set_spacing)
   *
   * Spacing to add between the lines of the `Pango2Layout`.
   *
   * The spacing is specified in Pango units.
   *
   * The default value is 0.
   */
  props[PROP_SPACING] =
      g_param_spec_int ("spacing", NULL, NULL,
                        0, G_MAXINT, 0,
                        G_PARAM_READWRITE | G_PARAM_EXPLICIT_NOTIFY | G_PARAM_STATIC_STRINGS);

  /**
   * Pango2Layout:width: (attributes org.gtk.Property.get=pango2_layout_get_width org.gtk.Property.set=pango2_layout_set_width)
   *
   * The width to which the text of `Pango2Layout` will be broken.
   *
   * The width is specified in Pango units, with -1 meaning unlimited.
   *
   * The default value is -1.
   */
  props[PROP_WIDTH] =
      g_param_spec_int ("width", NULL, NULL,
                        -1, G_MAXINT, -1,
                        G_PARAM_READWRITE | G_PARAM_EXPLICIT_NOTIFY | G_PARAM_STATIC_STRINGS);

  /**
   * Pango2Layout:height: (attributes org.gtk.Property.get=pango2_layout_get_height org.gtk.Property.set=pango2_layout_set_height)
   *
   * The height to which the `Pango2Layout` will be ellipsized.
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
   * layout and its ellipsization mode is not `PANGO2_ELLIPSIZE_NONE`.
   * The behavior is undefined if a height other than -1 is set and
   * ellipsization mode is set to `PANGO2_ELLIPSIZE_NONE`.
   *
   * The default value is -1.
   */
  props[PROP_HEIGHT] =
      g_param_spec_int ("height", NULL, NULL,
                        -G_MAXINT, G_MAXINT, -1,
                        G_PARAM_READWRITE | G_PARAM_EXPLICIT_NOTIFY | G_PARAM_STATIC_STRINGS);

  /**
   * Pango2Layout:tabs: (attributes org.gtk.Property.get=pango2_layout_get_tabs org.gtk.Property.set=pango2_layout_set_tabs)
   *
   * The tabs to use when formatting the text of `Pango2Layout`.
   *
   * `Pango2Layout` will place content at the next tab position
   * whenever it meets a Tab character (U+0009).
   */
  props[PROP_TABS] =
      g_param_spec_boxed ("tabs", NULL, NULL,
                          PANGO2_TYPE_TAB_ARRAY,
                          G_PARAM_READWRITE | G_PARAM_EXPLICIT_NOTIFY | G_PARAM_STATIC_STRINGS);

  /**
   * Pango2Layout:single-paragraph: (attributes org.gtk.Property.get=pango2_layout_get_single_paragraph org.gtk.Property.set=pango2_layout_set_single_paragraph)
   *
   * Whether to treat newlines and similar characters as paragraph
   * separators or not.
   *
   * If this property is `TRUE`, all text is kept in a single paragraph,
   * and paragraph separator characters are displayed with a glyph.
   *
   * This is useful to allow editing of newlines on a single text line.
   *
   * The default value is `FALSE`.
   */
  props[PROP_SINGLE_PARAGRAPH] =
      g_param_spec_boolean ("single-paragraph", NULL, NULL,
                            FALSE,
                            G_PARAM_READWRITE | G_PARAM_EXPLICIT_NOTIFY | G_PARAM_STATIC_STRINGS);

  /**
   * Pango2Layout:wrap: (attributes org.gtk.Property.get=pango2_layout_get_wrap org.gtk.Property.set=pango2_layout_set_wrap)
   *
   * The wrap mode of this `Pango2Layout`.
   *
   * The wrap mode influences how Pango chooses line breaks
   * when text needs to be wrapped.
   *
   * The default value is `PANGO2_WRAP_WORD`.
   */
  props[PROP_WRAP] =
      g_param_spec_enum ("wrap", NULL, NULL,
                         PANGO2_TYPE_WRAP_MODE,
                         PANGO2_WRAP_WORD,
                         G_PARAM_READWRITE | G_PARAM_EXPLICIT_NOTIFY | G_PARAM_STATIC_STRINGS);

  /**
   * Pango2Layout:indent: (attributes org.gtk.Property.get=pango2_layout_get_indent org.gtk.Property.set=pango2_layout_set_indent)
   *
   * The indent of this `Pango2Layout`.
   *
   * The indent is specified in Pango units.
   *
   * A negative value of @indent will produce a hanging indentation.
   * That is, the first line will have the full width, and subsequent
   * lines will be indented by the absolute value of @indent.
   *
   * The default value is 0.
   */
  props[PROP_INDENT] =
      g_param_spec_int ("indent", NULL, NULL,
                        G_MININT, G_MAXINT, 0,
                        G_PARAM_READWRITE | G_PARAM_EXPLICIT_NOTIFY | G_PARAM_STATIC_STRINGS);

  /**
   * Pango2Layout:alignment: (attributes org.gtk.Property.get=pango2_layout_get_alignment org.gtk.Property.set=pango2_layout_set_alignment)
   *
   * The alignment mode of this `Pango2Layout`.
   *
   * The default value is `PANGO2_ALIGN_NATURAL`.
   */
  props[PROP_ALIGNMENT] =
      g_param_spec_enum ("alignment", NULL, NULL,
                         PANGO2_TYPE_ALIGNMENT,
                         PANGO2_ALIGN_NATURAL,
                         G_PARAM_READWRITE | G_PARAM_EXPLICIT_NOTIFY | G_PARAM_STATIC_STRINGS);

  /**
   * Pango2Layout:ellipsize: (attributes org.gtk.Property.get=pango2_layout_get_ellipsize org.gtk.Property.set=pango2_layout_set_ellipsize)
   *
   * The ellipsization mode of this `Pango2Layout`.
   *
   * The default value is `PANGO2_ELLIPSIZE_NONE`.
   */
  props[PROP_ELLIPSIZE] =
      g_param_spec_enum ("ellipsize", NULL, NULL,
                         PANGO2_TYPE_ELLIPSIZE_MODE,
                         PANGO2_ELLIPSIZE_NONE,
                         G_PARAM_READWRITE | G_PARAM_EXPLICIT_NOTIFY | G_PARAM_STATIC_STRINGS);

  /**
   * Pango2Layout:auto-dir: (attributes org.gtk.Property.get=pango2_layout_get_auto_dir org.gtk.Property.set=pango2_layout_set_auto_dir)
   *
   * Whether this `Pango2Layout` determines the
   * base direction from the content.
   *
   * The default value is `TRUE`.
   */
  props[PROP_AUTO_DIR] =
      g_param_spec_boolean ("auto-dir", NULL, NULL,
                            TRUE,
                            G_PARAM_READWRITE | G_PARAM_EXPLICIT_NOTIFY | G_PARAM_STATIC_STRINGS);

  /**
   * Pango2Layout:lines: (attributes org.gtk.Property.get=pango2_layout_get_lines)
   *
   * The `Pango2Lines` object holding the formatted lines.
   */
  props[PROP_LINES] =
      g_param_spec_object ("lines", NULL, NULL,
                           PANGO2_TYPE_LINES,
                           G_PARAM_READABLE | G_PARAM_STATIC_STRINGS);

  g_object_class_install_properties (object_class, NUM_PROPERTIES, props);
}

/*  }}} */
/* {{{ Utilities */

static void
layout_changed (Pango2Layout *layout)
{
  layout->serial++;
  if (layout->serial == 0)
    layout->serial++;

  g_clear_object (&layout->lines);
  g_object_notify_by_pspec (G_OBJECT (layout), props[PROP_LINES]);
}

static void
check_context_changed (Pango2Layout *layout)
{
  guint old_serial = layout->context_serial;

  layout->context_serial = pango2_context_get_serial (layout->context);

  if (old_serial != layout->context_serial)
    pango2_layout_context_changed (layout);
}

static Pango2AttrList *
ensure_attrs (Pango2Layout   *layout,
              Pango2AttrList *attrs)
{
  if (attrs)
    return attrs;
  else if (layout->attrs)
    return pango2_attr_list_copy (layout->attrs);
  else
    return pango2_attr_list_new ();
}

static Pango2AttrList *
get_effective_attributes (Pango2Layout *layout)
{
  Pango2AttrList *attrs = NULL;

  if (layout->font_desc)
    {
      attrs = ensure_attrs (layout, attrs);
      pango2_attr_list_insert_before (attrs,
                                      pango2_attr_font_desc_new (layout->font_desc));
    }

  if (layout->line_height != 0.0)
    {
      attrs = ensure_attrs (layout, attrs);
      pango2_attr_list_insert_before (attrs,
                                      pango2_attr_line_height_new (layout->line_height));
    }

  if (layout->spacing != 0)
    {
      attrs = ensure_attrs (layout, attrs);
      pango2_attr_list_insert_before (attrs,
                                      pango2_attr_line_spacing_new (layout->spacing));
    }

  if (layout->single_paragraph)
    {
      attrs = ensure_attrs (layout, attrs);
      pango2_attr_list_insert_before (attrs,
                                      pango2_attr_paragraph_new ());
    }

  if (attrs)
    return attrs;

  return pango2_attr_list_ref (layout->attrs);
}

static gboolean
ends_with_paragraph_separator (Pango2Layout *layout)
{
  if (layout->single_paragraph)
    return FALSE;

  return g_str_has_suffix (layout->text, "\n") ||
         g_str_has_suffix (layout->text, "\r") ||
         g_str_has_suffix (layout->text, "\r\n") ||
         g_str_has_suffix (layout->text, " ");
}

static void
ensure_lines (Pango2Layout *layout)
{
  Pango2LineBreaker *breaker;
  Pango2AttrList *attrs;
  int layout_width;
  int x, y, width;
  int line_no;
  gboolean at_paragraph_start;
  Pango2Alignment alignment;
  Pango2Alignment natural_alignment;

  check_context_changed (layout);

  if (layout->lines)
    return;

  layout_width = layout->width;

start_over:
  breaker = pango2_line_breaker_new (layout->context);

  pango2_line_breaker_set_tabs (breaker, layout->tabs);
  pango2_line_breaker_set_base_dir (breaker,
                                    layout->auto_dir
                                      ? PANGO2_DIRECTION_NEUTRAL
                                      : pango2_context_get_base_dir (layout->context));

  attrs = get_effective_attributes (layout);
  pango2_line_breaker_add_text (breaker, layout->text ? layout->text : "", -1, attrs);
  if (attrs)
    pango2_attr_list_unref (attrs);

  layout->lines = pango2_lines_new ();

  if (pango2_line_breaker_get_direction (breaker) == PANGO2_DIRECTION_LTR)
    natural_alignment = PANGO2_ALIGN_LEFT;
  else
    natural_alignment = PANGO2_ALIGN_RIGHT;

  if (layout->alignment == PANGO2_ALIGN_NATURAL)
    alignment = natural_alignment;
  else
    alignment = layout->alignment;

  x = y = 0;
  line_no = 0;
  at_paragraph_start = TRUE;
  while (pango2_line_breaker_has_line (breaker))
    {
      Pango2Line *line;
      Pango2Rectangle ext;
      int offset;
      Pango2EllipsizeMode ellipsize = PANGO2_ELLIPSIZE_NONE;
      Pango2LeadingTrim trim = PANGO2_LEADING_TRIM_NONE;

      switch (alignment)
        {
        case PANGO2_ALIGN_LEFT:
        case PANGO2_ALIGN_JUSTIFY:
          if (at_paragraph_start == (layout->indent > 0))
            {
              x = abs (layout->indent);
              if (layout_width == -1)
                width = -1;
              else
                width = layout_width - x;
            }
          else
            {
              x = 0;
              width = layout_width;
            }
          break;

        case PANGO2_ALIGN_RIGHT:
          if (at_paragraph_start == (layout->indent > 0))
            {
              x = 0;
              if (layout_width == -1)
                width = -1;
              else
                width = layout_width - abs (layout->indent);
            }
          else
            {
              x = 0;
              width = layout_width;
            }
          break;

        case PANGO2_ALIGN_CENTER:
          x = 0;
          width = layout_width;
          break;

        case PANGO2_ALIGN_NATURAL:
        default:
          g_assert_not_reached ();
        }

      if (layout->height < 0 && line_no + 1 == - layout->height)
        ellipsize = layout->ellipsize;

retry:
      line = pango2_line_breaker_next_line (breaker, x, width, layout->wrap, ellipsize);

      at_paragraph_start = line->ends_paragraph;

      if (line->starts_paragraph)
        trim |= PANGO2_LEADING_TRIM_START;
      if (line->ends_paragraph)
        trim |= PANGO2_LEADING_TRIM_END;

      pango2_line_get_trimmed_extents (line, trim, &ext);

      if (layout->height >= 0 && y + 2 * ext.height >= layout->height &&
          ellipsize != layout->ellipsize)
        {
          if (pango2_line_breaker_undo_line (breaker, line))
            {
              g_clear_pointer (&line, pango2_line_free);
              ellipsize = layout->ellipsize;
              goto retry;
            }
        }

      offset = 0;
      if (width > 0)
        {
          /* Handle alignment and justification */
          switch (alignment)
            {

            case PANGO2_ALIGN_LEFT:
              break;

            case PANGO2_ALIGN_CENTER:
              if (ext.width < width)
                offset = (width - ext.width) / 2;
              break;

            case PANGO2_ALIGN_JUSTIFY:
              if (!pango2_line_is_paragraph_end (line))
                {
                  line = pango2_line_justify (line, width);
                  break;
                }
              G_GNUC_FALLTHROUGH;

            case PANGO2_ALIGN_RIGHT:
              if (ext.width < width)
                offset = width - ext.width;
              break;

            case PANGO2_ALIGN_NATURAL:
            default:
              g_assert_not_reached ();
            }
        }

      pango2_lines_add_line (layout->lines, line, x + offset, y - ext.y);

      y += ext.height;
      line_no++;
    }

  if (width < 0 && alignment != PANGO2_ALIGN_LEFT)
    {
      /* Not the most efficient, but it works */
      pango2_lines_get_size (layout->lines, &layout_width, NULL);
      g_clear_object (&layout->lines);
      g_clear_object (&breaker);

      goto start_over;
    }

  /* Append an empty line if we end with a newline.
   * And always provide at least one line
   */
  if (pango2_lines_get_line_count (layout->lines) == 0 ||
      ends_with_paragraph_separator (layout))
    {
      LineData *data;
      int start_index;
      int start_offset;
      int offset;
      Pango2Line *line;
      Pango2Rectangle ext;

      if (pango2_lines_get_line_count (layout->lines) > 0)
        {
          Pango2Line *last;

          last = pango2_lines_get_lines (layout->lines)[pango2_lines_get_line_count (layout->lines) - 1];
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
          data->log_attrs = g_new0 (Pango2LogAttr, 1);
          data->log_attrs[0].is_cursor_position = TRUE;
          start_index = 0;
          start_offset = 0;
          offset = 0;
        }

      line = pango2_line_new (layout->context, data);
      line->starts_paragraph = TRUE;
      line->ends_paragraph = TRUE;
      line->start_index = start_index;
      line->length = 0;
      line->start_offset = start_offset;
      line->n_chars = 0;

      pango2_line_get_extents (line, NULL, &ext);

      pango2_lines_add_line (layout->lines, line, x + offset, y - ext.y);

      line_data_unref (data);
    }

  g_object_unref (breaker);
}

/* }}} */
/* {{{ Public API */ 

/**
 * pango2_layout_new:
 * @context: a `Pango2Context`
 *
 * Creates a new `Pango2Layout` initialized to default values
 * for a particular `Pango2Context`
 *
 * Return value: a newly allocated `Pango2Layout`
 */
Pango2Layout *
pango2_layout_new (Pango2Context *context)
{
  g_return_val_if_fail (PANGO2_IS_CONTEXT (context), NULL);

  return g_object_new (PANGO2_TYPE_LAYOUT, "context", context, NULL);
}

/**
 * pango2_layout_copy:
 * @layout: a `Pango2Layout`
 *
 * Creates a deep copy of the layout.
 *
 * The attribute list, tab array, and text from the original layout
 * are all copied by value.
 *
 * Return value: (transfer full): the newly allocated `Pango2Layout`
 */
Pango2Layout *
pango2_layout_copy (Pango2Layout *layout)
{
  Pango2Layout *copy;

  g_return_val_if_fail (PANGO2_IS_LAYOUT (layout), NULL);

  copy = pango2_layout_new (layout->context);

  copy->text = g_strdup (layout->text);
  copy->length = layout->length;
  if (layout->attrs)
    copy->attrs = pango2_attr_list_copy (layout->attrs);
  if (layout->font_desc)
    copy->font_desc = pango2_font_description_copy (layout->font_desc);
  copy->line_height = layout->line_height;
  copy->spacing = layout->spacing;
  copy->width = layout->width;
  copy->height = layout->height;
  if (layout->tabs)
    copy->tabs = pango2_tab_array_copy (layout->tabs);
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
 * pango2_layout_get_serial:
 * @layout: a `Pango2Layout`
 *
 * Returns the current serial number of the layout.
 *
 * The serial number is initialized to an small number larger than zero
 * when a new layout is created and is increased whenever the layout is
 * changed using any of the setter functions, or the `Pango2Context` it
 * uses has changed.
 *
 * The serial may wrap, but will never have the value 0. Since it can
 * wrap, never compare it with "less than", always use "not equals".
 *
 * This can be used to automatically detect changes to a `Pango2Layout`,
 * and is useful for example to decide whether a layout needs redrawing.
 *
 * Return value: The current serial number of @layout
 */
guint
pango2_layout_get_serial (Pango2Layout *layout)
{
  check_context_changed (layout);

  return layout->serial;
}

/**
 * pango2_layout_context_changed:
 * @layout: a `Pango2Layout`
 *
 * Forces recomputation of any state in the layout that
 * might depend on the layout's context.
 *
 * This function should be called if you make changes to the
 * context subsequent to creating the layout.
 */
void
pango2_layout_context_changed (Pango2Layout *layout)
{
  g_return_if_fail (PANGO2_IS_LAYOUT (layout));

  layout_changed (layout);
}

/* {{{ Property getters and setters */

/**
 * pango2_layout_get_context:
 * @layout: a `Pango2Layout`
 *
 * Retrieves the context used for this layout.
 *
 * Return value: (transfer none): the context for the layout
 */
Pango2Context *
pango2_layout_get_context (Pango2Layout *layout)
{
  g_return_val_if_fail (PANGO2_IS_LAYOUT (layout), NULL);

  return layout->context;
}

/**
 * pango2_layout_set_text:
 * @layout: a `Pango2Layout`
 * @text: the text
 * @length: maximum length of @text, in bytes. -1 indicates that
 *   the string is nul-terminated
 *
 * Sets the text of the layout.
 */
void
pango2_layout_set_text (Pango2Layout *layout,
                        const char   *text,
                        int           length)
{
  g_return_if_fail (PANGO2_IS_LAYOUT (layout));

  if (length < 0)
    length = strlen (text);

  g_free (layout->text);
  layout->text = g_strndup (text, length);
  layout->length = length;

  g_object_notify_by_pspec (G_OBJECT (layout), props[PROP_TEXT]);
  layout_changed (layout);
}

/**
 * pango2_layout_get_text:
 * @layout: a `Pango2Layout`
 *
 * Gets the text in the layout.
 *
 * The returned text should not be freed or modified.
 *
 * Return value: (transfer none): the text in the @layout
 */
const char *
pango2_layout_get_text (Pango2Layout *layout)
{
  g_return_val_if_fail (PANGO2_IS_LAYOUT (layout), NULL);

  return layout->text;
}

/**
 * pango2_layout_set_attributes:
 * @layout: a `Pango2Layout`
 * @attrs: (nullable) (transfer none): a `Pango2AttrList`
 *
 * Sets the attributes for the layout.
 *
 * References @attrs, so the caller can unref its reference.
 */
void
pango2_layout_set_attributes (Pango2Layout   *layout,
                              Pango2AttrList *attrs)
{
  g_return_if_fail (PANGO2_IS_LAYOUT (layout));

  g_clear_pointer (&layout->attrs, pango2_attr_list_unref);
  layout->attrs = attrs;
  if (layout->attrs)
    pango2_attr_list_ref (layout->attrs);

  g_object_notify_by_pspec (G_OBJECT (layout), props[PROP_ATTRIBUTES]);
  layout_changed (layout);
}

/**
 * pango2_layout_get_attributes:
 * @layout: a `Pango2Layout`
 *
 * Gets the attribute list for the layout, if any.
 *
 * Return value: (transfer none) (nullable): a `Pango2AttrList`
 */
Pango2AttrList *
pango2_layout_get_attributes (Pango2Layout *layout)
{
  g_return_val_if_fail (PANGO2_IS_LAYOUT (layout), NULL);

  return layout->attrs;
}

/**
 * pango2_layout_set_font_description:
 * @layout: a `Pango2Layout`
 * @desc: (nullable): the new `Pango2FontDescription`
 *
 * Sets the default font description for the layout.
 *
 * If no font description is set on the layout, the
 * font description from the layout's context is used.
 *
 * This method is a shorthand for adding a font-desc attribute.
 */
void
pango2_layout_set_font_description (Pango2Layout                *layout,
                                    const Pango2FontDescription *desc)
{
  g_return_if_fail (PANGO2_IS_LAYOUT (layout));

  if (desc != layout->font_desc &&
      (!desc || !layout->font_desc || !pango2_font_description_equal (desc, layout->font_desc)))
    {
      if (layout->font_desc)
        pango2_font_description_free (layout->font_desc);

      layout->font_desc = desc ? pango2_font_description_copy (desc) : NULL;

      g_object_notify_by_pspec (G_OBJECT (layout), props[PROP_FONT_DESCRIPTION]);
      layout_changed (layout);
    }
}

/**
 * pango2_layout_get_font_description:
 * @layout: a `Pango2Layout`
 *
 * Gets the font description for the layout, if any.
 *
 * Return value: (transfer none) (nullable): a pointer to the
 *   layout's font description, or `NULL` if the font description
 *   from the layout's context is inherited.
 */
const Pango2FontDescription *
pango2_layout_get_font_description (Pango2Layout *layout)
{
  g_return_val_if_fail (PANGO2_IS_LAYOUT (layout), NULL);

  return layout->font_desc;
}

/**
 * pango2_layout_set_line_height:
 * @layout: a `Pango2Layout`
 * @line_height: the new line height factor
 *
 * Sets a factor for line height.
 *
 * Typical values are: 0, 1, 1.5, 2. The default values is 0.
 *
 * If @line_height is non-zero, lines are placed so that
 *
 *     baseline2 = baseline1 + factor * height2
 *
 * where height2 is the line height of the second line (as determined
 * by the font).
 *
 * This method is a shorthand for adding a line-height attribute.
 */
void
pango2_layout_set_line_height (Pango2Layout *layout,
                               float         line_height)
{
  g_return_if_fail (PANGO2_IS_LAYOUT (layout));

  if (layout->line_height == line_height)
    return;

  layout->line_height = line_height;

  g_object_notify_by_pspec (G_OBJECT (layout), props[PROP_LINE_HEIGHT]);
  layout_changed (layout);
}

/**
 * pango2_layout_get_line_height:
 * @layout: a `Pango2Layout`
 *
 * Gets the line height factor of the layout.
 *
 * See [method@Pango2.Layout.set_line_height].
 */
float
pango2_layout_get_line_height (Pango2Layout *layout)
{
  g_return_val_if_fail (PANGO2_IS_LAYOUT (layout), 0.0);

  return layout->line_height;
}

/**
 * pango2_layout_set_spacing:
 * @layout: a `Pango2Layout`
 * @spacing: the amount of spacing, in Pango units
 *
 * Sets the amount of spacing between the lines of the layout.
 *
 * When placing lines with spacing, Pango arranges things so that
 *
 *     line2.top = line1.bottom + spacing
 *
 * The default value is 0.
 *
 * Spacing only takes effect if the line height is not
 * overridden via [method@Pango2.Layout.set_line_height].
 */
void
pango2_layout_set_spacing (Pango2Layout *layout,
                           int           spacing)
{
  g_return_if_fail (PANGO2_IS_LAYOUT (layout));

  if (layout->spacing == spacing)
    return;

  layout->spacing = spacing;

  g_object_notify_by_pspec (G_OBJECT (layout), props[PROP_SPACING]);
  layout_changed (layout);
}

/**
 * pango2_layout_get_spacing:
 * @layout: a `Pango2Layout`
 *
 * Gets the amount of spacing between the lines of the layout.
 *
 * Return value: the spacing in Pango units
 */
int
pango2_layout_get_spacing (Pango2Layout *layout)
{
  g_return_val_if_fail (PANGO2_IS_LAYOUT (layout), 0);

  return layout->spacing;
}

/**
 * pango2_layout_set_width:
 * @layout: a `Pango2Layout`.
 * @width: the desired width in Pango units, or -1 to indicate that no
 *   wrapping or ellipsization should be performed
 *
 * Sets the width to which the lines of the layout should
 * be wrapped or ellipsized.
 *
 * The default value is -1: no width set.
 */
void
pango2_layout_set_width (Pango2Layout *layout,
                         int           width)
{
  g_return_if_fail (PANGO2_IS_LAYOUT (layout));

  if (width < -1)
    width = -1;

  if (layout->width == width)
    return;

  layout->width = width;

  g_object_notify_by_pspec (G_OBJECT (layout), props[PROP_WIDTH]);
  layout_changed (layout);
}

/**
 * pango2_layout_get_width:
 * @layout: a `Pango2Layout`
 *
 * Gets the width to which the lines of the layout should be
 * wrapped or ellipsized.
 *
 * Return value: the width in Pango units, or -1 if no width set
 */
int
pango2_layout_get_width (Pango2Layout *layout)
{
  g_return_val_if_fail (PANGO2_IS_LAYOUT (layout), -1);

  return layout->width;
}

/**
 * pango2_layout_set_height:
 * @layout: a `Pango2Layout`.
 * @height: the desired height
 *
 * Sets the height to which the `Pango2Layout` should be ellipsized.
 *
 * There are two different behaviors, based on whether @height is
 * positive or negative.
 *
 * See [property@Pango2.Layout:height] for details.
 */
void
pango2_layout_set_height (Pango2Layout *layout,
                          int           height)
{
  g_return_if_fail (PANGO2_IS_LAYOUT (layout));

  if (layout->height == height)
    return;

  layout->height = height;

  g_object_notify_by_pspec (G_OBJECT (layout), props[PROP_HEIGHT]);
  layout_changed (layout);
}

/**
 * pango2_layout_get_height:
 * @layout: a `Pango2Layout`
 *
 * Gets the height to which the lines of the layout should be ellipsized.
 *
 * See [property@Pango2.Layout:height] for details.
 *
 * Return value: the height
 */
int
pango2_layout_get_height (Pango2Layout *layout)
{
  g_return_val_if_fail (PANGO2_IS_LAYOUT (layout), -1);

  return layout->height;
}

/**
 * pango2_layout_set_tabs:
 * @layout: a `Pango2Layout`
 * @tabs: (nullable): a `Pango2TabArray`
 *
 * Sets the tabs to use for the layout, overriding the
 * default tabs.
 *
 * Setting the tabs to `NULL` reinstates the default
 * tabs.
 *
 * See [method@Pango2.LineBreaker.set_tabs] for details.
 */
void
pango2_layout_set_tabs (Pango2Layout   *layout,
                        Pango2TabArray *tabs)
{
  g_return_if_fail (PANGO2_IS_LAYOUT (layout));

  if (layout->tabs == tabs)
    return;

  g_clear_pointer (&layout->tabs, pango2_tab_array_free);
  if (tabs)
    layout->tabs = pango2_tab_array_copy (tabs);

  g_object_notify_by_pspec (G_OBJECT (layout), props[PROP_TABS]);
  layout_changed (layout);
}

/**
 * pango2_layout_get_tabs:
 * @layout: a `Pango2Layout`
 *
 * Gets the current `Pango2TabArray` used by this layout.
 *
 * If no `Pango2TabArray` has been set, then the default tabs are
 * in use and `NULL` is returned. Default tabs are every 8 spaces.
 *
 * Return value: (transfer none) (nullable): the tabs for this layout
 */
Pango2TabArray *
pango2_layout_get_tabs (Pango2Layout *layout)
{
  g_return_val_if_fail (PANGO2_IS_LAYOUT (layout), NULL);

  return layout->tabs;
}

/**
 * pango2_layout_set_single_paragraph:
 * @layout: a `Pango2Layout`
 * @single_paragraph: the new setting
 *
 * Sets the single paragraph mode of the layout.
 *
 * If @single_paragraph is true, do not treat newlines and similar
 * characters as paragraph separators; instead, keep all text in a
 * single paragraph, and display a glyph for paragraph separator
 * characters.
 *
 * Used when you want to allow editing of newlines on a single text line.
 *
 * The default value is false.
 */
void
pango2_layout_set_single_paragraph (Pango2Layout *layout,
                                    gboolean      single_paragraph)
{
  g_return_if_fail (PANGO2_IS_LAYOUT (layout));

  if (layout->single_paragraph == single_paragraph)
    return;

  layout->single_paragraph = single_paragraph;

  g_object_notify_by_pspec (G_OBJECT (layout), props[PROP_SINGLE_PARAGRAPH]);
  layout_changed (layout);
}

/**
 * pango2_layout_get_single_paragraph:
 * @layout: a `Pango2Layout`
 *
 * Obtains whether this layout is in single paragraph mode.
 *
 * See [method@Pango2.Layout.set_single_paragraph].
 *
 * Return value: true if the layout does not break paragraphs
 *   at paragraph separator characters, false otherwise
 */
gboolean
pango2_layout_get_single_paragraph (Pango2Layout *layout)
{
  g_return_val_if_fail (PANGO2_IS_LAYOUT (layout), FALSE);

  return layout->single_paragraph;
}

/**
 * pango2_layout_set_wrap:
 * @layout: a `Pango2Layout`
 * @wrap: the wrap mode
 *
 * Sets the wrap mode.
 *
 * The wrap mode only has effect if a width is set on the layout
 * with [method@Pango2.Layout.set_width]. To turn off wrapping,
 * set the width to -1.
 *
 * The default value is `PANGO2_WRAP_WORD`.
 */
void
pango2_layout_set_wrap (Pango2Layout   *layout,
                        Pango2WrapMode  wrap)
{
  g_return_if_fail (PANGO2_IS_LAYOUT (layout));

  if (layout->wrap == wrap)
    return;

  layout->wrap = wrap;

  g_object_notify_by_pspec (G_OBJECT (layout), props[PROP_WRAP]);
  layout_changed (layout);
}

/**
 * pango2_layout_get_wrap:
 * @layout: a `Pango2Layout`
 *
 * Gets the wrap mode for the layout.
 *
 * Return value: current wrap mode
 */
Pango2WrapMode
pango2_layout_get_wrap (Pango2Layout *layout)
{
  g_return_val_if_fail (PANGO2_IS_LAYOUT (layout), PANGO2_WRAP_WORD);

  return layout->wrap;
}

/**
 * pango2_layout_set_indent:
 * @layout: a `Pango2Layout`
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
pango2_layout_set_indent (Pango2Layout *layout,
                          int           indent)
{
  g_return_if_fail (PANGO2_IS_LAYOUT (layout));

  if (layout->indent == indent)
    return;

  layout->indent = indent;

  g_object_notify_by_pspec (G_OBJECT (layout), props[PROP_INDENT]);
  layout_changed (layout);
}

/**
 * pango2_layout_get_indent:
 * @layout: a `Pango2Layout`
 *
 * Gets the paragraph indent width in Pango units.
 *
 * A negative value indicates a hanging indentation.
 *
 * Return value: the indent in Pango units
 */
int
pango2_layout_get_indent (Pango2Layout *layout)
{
  g_return_val_if_fail (PANGO2_IS_LAYOUT (layout), 0);

  return layout->indent;
}

/**
 * pango2_layout_set_alignment:
 * @layout: a `Pango2Layout`
 * @alignment: the alignment
 *
 * Sets the alignment for the layout.
 *
 * The alignment determines how short lines are
 * positioned within the available horizontal space.
 *
 * The default alignment is `PANGO2_ALIGN_NATURAL`.
 */
void
pango2_layout_set_alignment (Pango2Layout    *layout,
                             Pango2Alignment  alignment)
{
  g_return_if_fail (PANGO2_IS_LAYOUT (layout));

  if (layout->alignment == alignment)
    return;

  layout->alignment = alignment;

  g_object_notify_by_pspec (G_OBJECT (layout), props[PROP_ALIGNMENT]);
  layout_changed (layout);
}

/**
 * pango2_layout_get_alignment:
 * @layout: a `Pango2Layout`
 *
 * Gets the alignment for the layout.
 *
 * The alignment determines how short lines are
 * positioned within the available horizontal space.
 *
 * Return value: the alignment
 */
Pango2Alignment
pango2_layout_get_alignment (Pango2Layout *layout)
{
  g_return_val_if_fail (PANGO2_IS_LAYOUT (layout), PANGO2_ALIGN_NATURAL);

  return layout->alignment;
}

/**
 * pango2_layout_set_ellipsize:
 * @layout: a `Pango2Layout`
 * @ellipsize: the new ellipsization mode for @layout
 *
 * Sets the type of ellipsization to use for this layout.
 *
 * Depending on the ellipsization mode @ellipsize text is removed
 * from the start, middle, or end of text so they fit within the
 * width of layout set with [method@Pango2.Layout.set_width].
 *
 * The default value is `PANGO2_ELLIPSIZE_NONE`.
 */
void
pango2_layout_set_ellipsize (Pango2Layout        *layout,
                             Pango2EllipsizeMode  ellipsize)
{
  g_return_if_fail (PANGO2_IS_LAYOUT (layout));

  if (layout->ellipsize == ellipsize)
    return;

  layout->ellipsize = ellipsize;

  g_object_notify_by_pspec (G_OBJECT (layout), props[PROP_ELLIPSIZE]);
  layout_changed (layout);
}

/**
 * pango2_layout_get_ellipsize:
 * @layout: a `Pango2Layout`
 *
 * Gets the type of ellipsization being performed for the layout.
 *
 * See [method@Pango2.Layout.set_ellipsize].
 *
 * Return value: the ellipsization mode
 */
Pango2EllipsizeMode
pango2_layout_get_ellipsize (Pango2Layout *layout)
{
  g_return_val_if_fail (PANGO2_IS_LAYOUT (layout), PANGO2_ELLIPSIZE_NONE);

  return layout->ellipsize;
}

/**
 * pango2_layout_set_auto_dir:
 * @layout: a `Pango2Layout`
 * @auto_dir: if true, compute the bidirectional base direction
 *   from the layout's contents
 *
 * Sets whether to calculate the base direction
 * for the layout according to its contents.
 *
 * When this flag is true (the default), then paragraphs in
 * @layout that begin with strong right-to-left characters
 * (Arabic and Hebrew principally), will have right-to-left
 * layout, paragraphs with letters from other scripts will
 * have left-to-right layout. Paragraphs with only neutral
 * characters get their direction from the surrounding
 * paragraphs.
 *
 * When false, the choice between left-to-right and right-to-left
 * layout is done according to the base direction of the layout's
 * `Pango2Context`. (See [method@Pango2.Context.set_base_dir]).
 *
 * When the auto-computed direction of a paragraph differs
 * from the base direction of the context, the interpretation
 * of `PANGO2_ALIGN_LEFT` and `PANGO2_ALIGN_RIGHT` are swapped.
 */
void
pango2_layout_set_auto_dir (Pango2Layout *layout,
                            gboolean      auto_dir)
{
  g_return_if_fail (PANGO2_IS_LAYOUT (layout));

  if (auto_dir == layout->auto_dir)
    return;

  layout->auto_dir = auto_dir;

  g_object_notify_by_pspec (G_OBJECT (layout), props[PROP_AUTO_DIR]);
  layout_changed (layout);
}

/**
 * pango2_layout_get_auto_dir:
 * @layout: a `Pango2Layout`
 *
 * Gets whether to calculate the base direction for the layout
 * according to its contents.
 *
 * See [method@Pango2.Layout.set_auto_dir].
 *
 * Return value: true if the bidirectional base direction
 *   is computed from the layout's contents, false otherwise
 */
gboolean
pango2_layout_get_auto_dir (Pango2Layout *layout)
{
  g_return_val_if_fail (PANGO2_IS_LAYOUT (layout), TRUE);

  return layout->auto_dir;
}

/* }}} */
/* {{{ Miscellaneous */

/**
 * pango2_layout_set_markup:
 * @layout: a `Pango2Layout`
 * @markup: marked-up text
 * @length: length of @markup in bytes, or -1 if it is `NUL`-terminated
 *
 * Sets the layout text and attribute list from marked-up text.
 *
 * See [Pango Markup](pango2_markup.html)).
 *
 * Replaces the current text and attributes.
 */
void
pango2_layout_set_markup (Pango2Layout *layout,
                          const char   *markup,
                          int           length)
{
  Pango2AttrList *attrs;
  char *text;
  GError *error = NULL;

  g_return_if_fail (PANGO2_IS_LAYOUT (layout));
  g_return_if_fail (markup != NULL);

  if (!pango2_parse_markup (markup, length, 0, &attrs, &text, NULL, &error))
    {
      g_warning ("pango2_layout_set_markup_with_accel: %s", error->message);
      g_error_free (error);
      return;
    }

  g_free (layout->text);
  layout->text = text;
  layout->length = strlen (text);

  g_clear_pointer (&layout->attrs, pango2_attr_list_unref);
  layout->attrs = attrs;

  g_object_notify_by_pspec (G_OBJECT (layout), props[PROP_TEXT]);
  g_object_notify_by_pspec (G_OBJECT (layout), props[PROP_ATTRIBUTES]);
  layout_changed (layout);
}

/**
 * pango2_layout_get_character_count:
 * @layout: a `Pango2Layout`
 *
 * Returns the number of Unicode characters in the
 * the text of the layout.
 *
 * Return value: the number of Unicode characters in @layout
 */
int
pango2_layout_get_character_count (Pango2Layout *layout)
{
  Pango2Line *line;

  g_return_val_if_fail (PANGO2_IS_LAYOUT (layout), 0);

  ensure_lines (layout);

  line = pango2_lines_get_lines (layout->lines)[0];

  return line->data->n_chars;
}

/* }}} */
/* {{{ Output getters */

/**
 * pango2_layout_get_lines:
 * @layout: a `Pango2Layout`
 *
 * Gets the lines of the layout.
 *
 * The returned object will become invalid when any
 * property of @layout is changed. Take a reference
 * to keep it.
 *
 * Return value: (transfer none): a `Pango2Lines` object
 *   with the lines of @layout
 */
Pango2Lines *
pango2_layout_get_lines (Pango2Layout *layout)
{
  g_return_val_if_fail (PANGO2_IS_LAYOUT (layout), NULL);

  ensure_lines (layout);

  return layout->lines;
}

/**
 * pango2_layout_get_log_attrs:
 * @layout: a `Pango2Layout`
 * @n_attrs: (out): return location for the length of the array
 *
 * Gets the `Pango2LogAttr` array for the content of layout.
 *
 * The returned array becomes invalid when
 * any properties of @layout change. Make a
 * copy if you want to keep it.
 *
 * Returns: (transfer none): the `Pango2LogAttr` array
 */
const Pango2LogAttr *
pango2_layout_get_log_attrs (Pango2Layout *layout,
                             int          *n_attrs)
{
  Pango2Line *line;

  g_return_val_if_fail (PANGO2_IS_LAYOUT (layout), NULL);

  ensure_lines (layout);

  line = pango2_lines_get_lines (layout->lines)[0];

  if (n_attrs)
    *n_attrs = line->data->n_chars + 1;

  return line->data->log_attrs;
}

/**
 * pango2_layout_get_iter:
 * @layout: a `Pango2Layout`
 *
 * Returns an iterator to iterate over the visual extents
 * of the layout.
 *
 * This is a convenience wrapper for [method@Pango2.Lines.get_iter].
 *
 * Returns: the new `Pango2LineIter`
 */
Pango2LineIter *
pango2_layout_get_iter (Pango2Layout *layout)
{
  g_return_val_if_fail (PANGO2_IS_LAYOUT (layout), NULL);

  ensure_lines (layout);

  return pango2_lines_get_iter (layout->lines);
}

/* }}} */
/* }}} */

/* vim:set foldmethod=marker expandtab: */
