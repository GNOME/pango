#include <pango2/pangocairo.h>
#include <hb-ot.h>

static int
get_fallback_bound (Pango2Font  *font,
                    gunichar     ch)
{
  float f;
  hb_codepoint_t glyph;
  Pango2Rectangle ink_rect;

  switch (ch)
    {
    case '"':
    case '\'':
    case ',':
    case '.':
    case 0x2018:
    case 0x2019:
    case 0x201A:
    case 0x201B:
    case 0x201C:
    case 0x201D:
    case 0x201E:
    case 0x201F:
      f = 1.0;
      break;
    case '-':
    case 0x2010:
    case 0x2011:
    case 0x2012:
      f = 0.75;
      break;
    case '/':
    case 0x2013:
      f = 0.50;
      break;
    case 0x2014:
    case 0x2015:
      f = 0.25;
      break;
    case 'A':
    case 'T':
    case 'V':
    case 'W':
    case 'Y':
      f = 0.2;
      break;
    case 'C':
    case 'O':
    case 'c':
    case 'o':
    case 'e':
    case 'w':
    case 'y':
      f = 0.1;
      break;
    default:
      f = 0;
      break;
    }

  hb_font_get_nominal_glyph (pango2_font_get_hb_font (font), ch, &glyph);
  pango2_font_get_glyph_extents (font, glyph, &ink_rect, NULL);

  return - (int) (ink_rect.width * f);
}

static int
get_bound (Pango2Font     *font,
           gunichar        ch,
           hb_direction_t  direction)
{
  hb_codepoint_t glyph;
  hb_font_t *hb_font;
  hb_set_t *lookups;
  int res;

  hb_font = pango2_font_get_hb_font (font);

  hb_font_get_nominal_glyph (hb_font, ch, &glyph);
  lookups = hb_set_create ();

  hb_ot_layout_collect_lookups (hb_font_get_face (hb_font),
                                HB_OT_TAG_GPOS,
                                NULL, NULL,
                                (hb_tag_t[]) { direction == HB_DIRECTION_LTR
                                                 ? HB_TAG ('l','f','b','d')
                                                 : HB_TAG ('r','t','b','d'),
                                               HB_TAG_NONE },
                                lookups);

  if (hb_set_is_empty (lookups))
    res = get_fallback_bound (font, ch);
  else
    res = hb_ot_layout_lookup_get_optical_bound (hb_font,
                                                 hb_set_get_min (lookups),
                                                 direction,
                                                 glyph);

  hb_set_destroy (lookups);

  return res;
}

static void
get_optical_bounds (Pango2Line *line,
                    int        *left,
                    int        *right)
{
  int start, length;
  const char *text;
  gunichar ch;
  Pango2Run **runs;
  Pango2Run *run;
  Pango2Item *item;
  Pango2Font *font;

  /* No attempt is made to handle rtl here */

  *left = *right = 0;

  if (pango2_line_get_run_count (line) == 0)
    return;

  text = pango2_line_get_text (line, &start, &length);

  ch = g_utf8_get_char (text + start);

  runs = pango2_line_get_runs (line);

  run = runs[0];
  item = pango2_run_get_item (run);
  font = pango2_analysis_get_font (pango2_item_get_analysis (item));

  *left = get_bound (font, ch, HB_DIRECTION_LTR);

  /* Note: the hyphen isn't present in the text */
  if (pango2_line_is_hyphenated (line))
    ch = 0x2010;
  else
    {
      char *p = g_utf8_prev_char (text + start + length);
      ch = g_utf8_get_char (p);
      /* We need to avoid a possible zeroed out space at the end */
      if (g_unichar_isspace (ch))
        {
          p = g_utf8_prev_char (p);
          ch = g_utf8_get_char (p);
        }
    }

  run = runs[pango2_line_get_run_count (line) - 1];
  item = pango2_run_get_item (run);
  font = pango2_analysis_get_font (pango2_item_get_analysis (item));

  *right = get_bound (font, ch, HB_DIRECTION_RTL);
}

int
main (int argc, char *argv[])
{
  gboolean opt_show_margins = FALSE;
  const char *opt_font = NULL;
  GOptionEntry option_entries[] = {
  { "show-margins", 0, 0, G_OPTION_ARG_NONE, &opt_show_margins, "Show margins", NULL },
  { "font", 0, 0, G_OPTION_ARG_STRING, &opt_font, "Font to use", NULL },
  { NULL, }
  };
  GOptionContext *option_context;
  const char *filename;
  Pango2Context *context;
  Pango2LineBreaker *breaker;
  int margin;
  int x, y, width;
  int x0, y0;
  Pango2Lines *lines;
  cairo_surface_t *surface;
  cairo_t *cr;
  cairo_status_t status;
  char *text;
  gsize length;
  Pango2AttrList *attrs;
  GError *error = NULL;
  int left, right;

  option_context = g_option_context_new ("");
  g_option_context_add_main_entries (option_context, option_entries, NULL);
  if (!g_option_context_parse (option_context, &argc, &argv, &error))
    {
      if (error != NULL)
        g_printerr ("%s\n", error->message);
      else
        g_printerr ("Option parse error\n");
      exit (1);
    }
  g_option_context_free (option_context);

  if (argc != 3)
    {
      g_printerr ("Usage: %s [OPTIONS] INPUT_FILENAME OUTPUT_FILENAME\n", argv[0]);
      return 1;
    }

  if (!g_file_get_contents (argv[1], &text, &length, &error))
    {
      g_printerr ("%s\n", error->message);
      return 1;
    }

  filename = argv[2];

  context = pango2_context_new ();

  margin = 20;
  width = (600 - 2 * margin);

  surface = cairo_image_surface_create (CAIRO_FORMAT_ARGB32, 600, 600);
  cr = cairo_create (surface);
  cairo_set_source_rgb (cr, 1, 1, 1);
  cairo_paint (cr);

  if (opt_show_margins)
    {
      cairo_set_source_rgb (cr, 1, 0, 0);
      cairo_set_line_width (cr, 1);
      cairo_rectangle (cr, margin + 0.5, -10, width - 1, 620);
      cairo_stroke (cr);
    }

  cairo_set_source_rgb (cr, 0, 0, 0);

  breaker = pango2_line_breaker_new (context);

  attrs = pango2_attr_list_new ();

  if (opt_font)
    {
      Pango2FontDescription *desc;

      g_print ("Using %s\n", opt_font);
      desc = pango2_font_description_from_string (opt_font);
      pango2_attr_list_insert (attrs, pango2_attr_font_desc_new (desc));
      pango2_font_description_free (desc);
    }

  pango2_line_breaker_add_text (breaker, text, -1, attrs);

  pango2_attr_list_unref (attrs);

  lines = pango2_lines_new ();

  x = x0 = margin * PANGO2_SCALE;
  y = y0 = margin * PANGO2_SCALE;
  width *= PANGO2_SCALE;

  while (pango2_line_breaker_has_line (breaker))
    {
      Pango2Line *line;
      Pango2Rectangle ext;

      line = pango2_line_breaker_next_line (breaker,
                                           x, width,
                                           PANGO2_WRAP_CHAR,
                                           PANGO2_ELLIPSIZE_NONE);

      get_optical_bounds (line, &left, &right);

      if (!pango2_line_is_paragraph_end (line))
        line = pango2_line_justify (line, width - left - right);

      pango2_line_get_extents (line, NULL, &ext);

      pango2_lines_add_line (lines, line, x + left, y - ext.y);

      y += ext.height;
    }

  pango2_cairo_show_lines (cr, lines);

#ifdef CAIRO_HAS_PNG_FUNCTIONS
  status = cairo_surface_write_to_png (surface, filename);
#else
  status = CAIRO_STATUS_PNG_ERROR; /* Not technically correct, but... */
#endif

  if (status != CAIRO_STATUS_SUCCESS)
    g_printerr ("Could not save png to '%s'\n", filename);
  else
    g_print ("Output written to %s\n", filename);

  g_object_unref (lines);
  g_object_unref (breaker);

  cairo_surface_destroy (surface);
  cairo_destroy (cr);

  g_object_unref (context);

  g_free (text);

  return 0;
}
