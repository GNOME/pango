/* viewer-render.c: Common code for rendering in viewers
 *
 * Copyright (C) 1999, 2004 Red Hat Software
 * Copyright (C) 2001 Sun Microsystems
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
#include "config.h"
#include <errno.h>
#include <math.h>
#include <stdarg.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>

#include <glib.h>
#include <glib/gprintf.h>
#include <pango/pango.h>

#include "viewer-render.h"

#define DEFAULT_FONT_FAMILY "Sans"
#define DEFAULT_FONT_SIZE 18

#define _MAKE_FONT_NAME(family, size) family " " #size
#define MAKE_FONT_NAME(family, size) _MAKE_FONT_NAME(family, size)

const char *prog_name;

gboolean opt_display = TRUE;
int opt_dpi = 96;
const char *opt_font = MAKE_FONT_NAME (DEFAULT_FONT_FAMILY, DEFAULT_FONT_SIZE);
gboolean opt_header = FALSE;
const char *opt_output = NULL;
int opt_margin = 10;
int opt_markup = FALSE;
gboolean opt_rtl = FALSE;
double opt_rotate = 0;
gboolean opt_auto_dir = TRUE;
const char *opt_text = NULL;
gboolean opt_waterfall = FALSE;
int opt_width = -1;
int opt_height = -1;
int opt_indent = 0;
gboolean opt_justify = 0;
int opt_runs = 1;
PangoAlignment opt_align = PANGO_ALIGN_LEFT;
PangoEllipsizeMode opt_ellipsize = PANGO_ELLIPSIZE_NONE;
PangoGravity opt_gravity = PANGO_GRAVITY_SOUTH;
PangoGravityHint opt_gravity_hint = PANGO_GRAVITY_HINT_NATURAL;
HintMode opt_hinting = HINT_DEFAULT;
PangoWrapMode opt_wrap = PANGO_WRAP_WORD_CHAR;
gboolean opt_wrap_set = FALSE;
const char *opt_pangorc = NULL;
const PangoViewer *opt_viewer = NULL;
const char *opt_language = NULL;
gboolean opt_single_par = FALSE;

/* Text (or markup) to render */
static char *text;

void
fail (const char *format, ...)
{
  const char *msg;

  va_list vap;
  va_start (vap, format);
  msg = g_strdup_vprintf (format, vap);
  g_printerr ("%s: %s\n", prog_name, msg);

  exit (1);
}

static PangoFontDescription *
get_font_description (void)
{
  PangoFontDescription *font_description = pango_font_description_from_string (opt_font);

  if ((pango_font_description_get_set_fields (font_description) & PANGO_FONT_MASK_FAMILY) == 0)
    pango_font_description_set_family (font_description, DEFAULT_FONT_FAMILY);

  if ((pango_font_description_get_set_fields (font_description) & PANGO_FONT_MASK_SIZE) == 0)
    pango_font_description_set_size (font_description, DEFAULT_FONT_SIZE * PANGO_SCALE);

  return font_description;
}

static PangoLayout *
make_layout(PangoContext *context,
	    const char   *text,
	    double        size)
{
  static PangoFontDescription *font_description;
  PangoAlignment align;
  PangoLayout *layout;

  layout = pango_layout_new (context);
  if (opt_markup)
    pango_layout_set_markup (layout, text, -1);
  else
    pango_layout_set_text (layout, text, -1);

  pango_layout_set_auto_dir (layout, opt_auto_dir);
  pango_layout_set_ellipsize (layout, opt_ellipsize);
  pango_layout_set_justify (layout, opt_justify);
  pango_layout_set_single_paragraph_mode (layout, opt_single_par);
  pango_layout_set_wrap (layout, opt_wrap);

  font_description = get_font_description ();
  if (size > 0)
    pango_font_description_set_size (font_description, size * PANGO_SCALE);

  if (opt_width > 0)
    pango_layout_set_width (layout, (opt_width * opt_dpi * PANGO_SCALE + 36) / 72);

  if (opt_height > 0)
    pango_layout_set_height (layout, (opt_height * opt_dpi * PANGO_SCALE + 36) / 72);
  else
    pango_layout_set_height (layout, opt_height);

  if (opt_indent != 0)
    pango_layout_set_indent (layout, (opt_indent * opt_dpi * PANGO_SCALE + 36) / 72);

  align = opt_align;
  if (align != PANGO_ALIGN_CENTER &&
      pango_context_get_base_dir (context) != PANGO_DIRECTION_LTR) {
    /* pango reverses left and right if base dir ir rtl.  so we should
     * reverse to cancel that.  unfortunately it also does that for
     * rtl paragraphs, so we cannot really get left/right.  all we get
     * is default/other-side. */
    align = PANGO_ALIGN_LEFT + PANGO_ALIGN_RIGHT - align;
  }
  pango_layout_set_alignment (layout, align);

  pango_layout_set_font_description (layout, font_description);

  pango_font_description_free (font_description);

  return layout;
}

gchar *
get_options_string (void)
{
  PangoFontDescription *font_description = get_font_description ();
  gchar *font_name;
  gchar *result;

  if (opt_waterfall)
    pango_font_description_unset_fields (font_description, PANGO_FONT_MASK_SIZE);

  font_name = pango_font_description_to_string (font_description);
  result = g_strdup_printf ("%s: %s (%d dpi)", opt_viewer->name, font_name, opt_dpi);
  pango_font_description_free (font_description);
  g_free (font_name);

  return result;
}

static void
output_body (PangoLayout    *layout,
	     RenderCallback  render_cb,
	     gpointer        cb_context,
	     gpointer        cb_data,
	     int            *width,
	     int            *height,
	     gboolean        supports_matrix)
{
  PangoRectangle logical_rect;
  int size, start_size, end_size, increment;
  int x = 0, y = 0;

  if (!supports_matrix)
    {
      const PangoMatrix* matrix;
      const PangoMatrix identity = PANGO_MATRIX_INIT;
      PangoContext *context = pango_layout_get_context (layout);
      matrix = pango_context_get_matrix (context);
      if (matrix)
	{
	  x += matrix->x0;
	  y += matrix->y0;
	}
      pango_context_set_matrix (context, &identity);
      pango_layout_context_changed (layout);
    }

  if (opt_waterfall)
    {
      start_size = 8;
      end_size = 48;
      increment = 4;
    }
  else
    {
      start_size = end_size = -1;
      increment = 1;
    }

  *width = 0;
  *height = 0;

  for (size = start_size; size <= end_size; size += increment)
    {
      if (size > 0)
        {
	  PangoFontDescription *desc = pango_font_description_copy (pango_layout_get_font_description (layout));
	  pango_font_description_set_size (desc, size * PANGO_SCALE);
	  pango_layout_set_font_description (layout, desc);
	  pango_font_description_free (desc);
	}

      pango_layout_get_pixel_extents (layout, NULL, &logical_rect);

      if (render_cb)
	(*render_cb) (layout, x, y+*height, cb_context, cb_data);

      *width = MAX (*width, 
		    MAX (logical_rect.x + logical_rect.width,
			 PANGO_PIXELS (pango_layout_get_width (layout))));
      *height +=    MAX (logical_rect.y + logical_rect.height,
			 PANGO_PIXELS (pango_layout_get_height (layout)));
    }
}

static void
set_transform (PangoContext     *context,
	       TransformCallback transform_cb,
	       gpointer          cb_context,
	       gpointer          cb_data,
	       PangoMatrix      *matrix)
{
  pango_context_set_matrix (context, matrix);
  if (transform_cb)
    (*transform_cb) (context, matrix, cb_context, cb_data);
}

void
do_output (PangoContext     *context,
	   RenderCallback    render_cb,
	   TransformCallback transform_cb,
	   gpointer          cb_context,
	   gpointer          cb_data,
	   int              *width_out,
	   int              *height_out)
{
  PangoLayout *layout;
  PangoRectangle rect;
  PangoMatrix matrix = PANGO_MATRIX_INIT;
  PangoMatrix *orig_matrix;
  gboolean supports_matrix;
  int rotated_width, rotated_height;
  int x = opt_margin;
  int y = opt_margin;
  int width, height;

  width = 0;
  height = 0;

  orig_matrix = pango_matrix_copy (pango_context_get_matrix (context));
  /* If the backend sets an all-zero matrix on the context,
   * means that it doesn't support transformations.
   */
  supports_matrix = !orig_matrix ||
		    (orig_matrix->xx != 0. || orig_matrix->xy != 0. ||
		     orig_matrix->yx != 0. || orig_matrix->yy != 0. ||
		     orig_matrix->x0 != 0. || orig_matrix->y0 != 0.);

  set_transform (context, transform_cb, cb_context, cb_data, NULL);

  pango_context_set_language (context,
			      opt_language ? pango_language_from_string (opt_language)
					   : pango_language_get_default ());
  pango_context_set_base_dir (context,
			      opt_rtl ? PANGO_DIRECTION_RTL : PANGO_DIRECTION_LTR);

  if (opt_header)
    {
      char *options_string = get_options_string ();
      pango_context_set_base_gravity (context, PANGO_GRAVITY_SOUTH);
      layout = make_layout (context, options_string, 10);
      pango_layout_get_extents (layout, NULL, &rect);

      width = MAX (width, PANGO_PIXELS (rect.width));
      height += PANGO_PIXELS (rect.height);

      if (render_cb)
	(*render_cb) (layout, x, y, cb_context, cb_data);

      y += PANGO_PIXELS (rect.height);

      g_object_unref (layout);
      g_free (options_string);
    }

  if (opt_rotate != 0)
    {
      if (supports_matrix)
	pango_matrix_rotate (&matrix, opt_rotate);
      else
	g_printerr ("The backend does not support rotated text\n");
    }

  pango_context_set_base_gravity (context, opt_gravity);
  pango_context_set_gravity_hint (context, opt_gravity_hint);

  layout = make_layout (context, text, -1);

  set_transform (context, transform_cb, cb_context, cb_data, &matrix);

  output_body (layout,
	       NULL, NULL, NULL,
	       &rotated_width, &rotated_height,
	       supports_matrix);

  rect.x = rect.y = 0;
  rect.width = rotated_width;
  rect.height = rotated_height;

  pango_matrix_transform_pixel_rectangle (&matrix, &rect);

  matrix.x0 = x - rect.x;
  matrix.y0 = y - rect.y;

  set_transform (context, transform_cb, cb_context, cb_data, &matrix);

  if (render_cb)
    output_body (layout,
		 render_cb, cb_context, cb_data,
		 &rotated_width, &rotated_height,
		 supports_matrix);

  width = MAX (width, rect.width);
  height += rect.height;

  width += 2 * opt_margin;
  height += 2 * opt_margin;

  if (width_out)
    *width_out = width;
  if (height_out)
    *height_out = height;

  pango_context_set_matrix (context, orig_matrix);
  pango_matrix_free (orig_matrix);
  g_object_unref (layout);
}

static gboolean
parse_enum (GType       type,
	    int        *value,
	    const char *name,
	    const char *arg,
	    gpointer    data,
	    GError **error)
{
  char *possible_values = NULL;
  gboolean ret;

  ret = pango_parse_enum (type,
			  arg,
			  value,
			  FALSE,
			  &possible_values);

  if (!ret && error)
    {
      g_set_error(error,
		  G_OPTION_ERROR,
		  G_OPTION_ERROR_BAD_VALUE,
		  "Argument for %s must be one of %s",
		  name,
		  possible_values);
      ret = FALSE;
    }

  g_free (possible_values);

  return ret;
}

static gboolean
parse_align (const char *name,
	     const char *arg,
	     gpointer    data,
	     GError **error)
{
  return parse_enum (PANGO_TYPE_ALIGNMENT, &opt_align,
		     name, arg, data, error);
}

static gboolean
parse_ellipsis (const char *name,
		const char *arg,
		gpointer    data,
		GError **error)
{
  return parse_enum (PANGO_TYPE_ELLIPSIZE_MODE, &opt_ellipsize,
		     name, arg, data, error);
}

static gboolean
parse_gravity (const char *name,
	       const char *arg,
	       gpointer    data,
	       GError **error)
{
  return parse_enum (PANGO_TYPE_GRAVITY, &opt_gravity,
		     name, arg, data, error);
}

static gboolean
parse_gravity_hint (const char *name,
		    const char *arg,
		    gpointer    data,
		    GError **error)
{
  return parse_enum (PANGO_TYPE_GRAVITY_HINT, &opt_gravity_hint,
		     name, arg, data, error);
}

static gboolean
parse_hinting (const char *name,
	       const char *arg,
	       gpointer    data,
	       GError    **error)
{
  gboolean ret = TRUE;

  if (strcmp (arg, "none") == 0)
    opt_hinting = HINT_NONE;
  else if (strcmp (arg, "auto") == 0)
    opt_hinting = HINT_AUTO;
  else if (strcmp (arg, "full") == 0)
    opt_hinting = HINT_FULL;
  else
    {
      g_set_error(error,
		  G_OPTION_ERROR,
		  G_OPTION_ERROR_BAD_VALUE,
		  "Argument for --hinting must be one of none/auto/full");
      ret = FALSE;
    }

  return ret;
}

static gboolean
parse_wrap (const char *name,
	    const char *arg,
	    gpointer    data,
	    GError    **error)
{
  gboolean ret;
  if ((ret = parse_enum (PANGO_TYPE_WRAP_MODE, &opt_wrap,
			 name, arg, data, error)))
    {
      opt_wrap_set = TRUE;
    }
  return ret;
}

static gchar *
backends_to_string (void)
{
  GString *backends = g_string_new (NULL);
  const PangoViewer **viewer;

  for (viewer = viewers; *viewer; viewer++)
    if ((*viewer)->id)
      {
	g_string_append (backends, (*viewer)->id);
	g_string_append_c (backends, '/');
      }
  g_string_truncate (backends, MAX (0, (gint)backends->len - 1));

  return g_string_free(backends,FALSE);
}

static int
backends_get_count (void)
{
  const PangoViewer **viewer;
  int i = 0;

  for (viewer = viewers; *viewer; viewer++)
    if ((*viewer)->id)
      i++;

  return i;
}


static gchar *
backend_description (void)
{
 GString *description  = g_string_new("Pango backend to use for rendering ");
 int backends_count = backends_get_count ();

 if (backends_count > 1)
   g_string_append_printf(description,"(default: %s)", (*viewers)->id);
 else if (backends_count == 1)
   g_string_append_printf(description,"(only available: %s)", (*viewers)->id);
 else
   g_string_append_printf(description,"(no backends found!)");

 return g_string_free(description,FALSE);

}

static gboolean
parse_backend (const char *name,
	       const char *arg,
	       gpointer    data,
	       GError    **error)
{
  gboolean ret = TRUE;
  const PangoViewer **viewer;

  for (viewer = viewers; *viewer; viewer++)
    if (!g_ascii_strcasecmp ((*viewer)->id, arg))
      break;

  if (*viewer)
    opt_viewer = *viewer;
  else
    {
      gchar *backends = backends_to_string ();

      g_set_error(error,
		  G_OPTION_ERROR,
		  G_OPTION_ERROR_BAD_VALUE,
		  "Available --backend options are: %s",
		  backends);
      g_free(backends);
      ret = FALSE;
    }

  return ret;
}


static gboolean
show_version(const char *name,
	     const char *arg,
	     gpointer    data,
	     GError    **error)
{
  g_printf("%s (%s) %s\n", prog_name, PACKAGE_NAME, PACKAGE_VERSION);
  g_printf("module interface version: %s\n", MODULE_VERSION);

  if (PANGO_VERSION != pango_version())
    g_printf("\nLinked Pango library has a different version: %s\n", pango_version_string ());

  exit(0);
}

void
parse_options (int argc, char *argv[])
{
  gchar *backend_options = backends_to_string ();
  GOptionFlags backend_flag = backends_get_count () > 1 ? 0 : G_OPTION_FLAG_HIDDEN;
  gchar *backend_desc = backend_description ();
  GOptionEntry entries[] =
  {
    {"no-auto-dir",	0, G_OPTION_FLAG_REVERSE, G_OPTION_ARG_NONE,	&opt_auto_dir,
     "No layout direction according to contents",			NULL},
    {"backend",		0, backend_flag, G_OPTION_ARG_CALLBACK,		&parse_backend,
     backend_desc,					     backend_options},
    {"no-display",	'q', G_OPTION_FLAG_REVERSE, G_OPTION_ARG_NONE,	&opt_display,
     "Do not display (just write to file or whatever)",			NULL},
    {"dpi",		0, 0, G_OPTION_ARG_INT,				&opt_dpi,
     "Set the resolution",					    "number"},
    {"align",		0, 0, G_OPTION_ARG_CALLBACK,			&parse_align,
     "Text alignment",				         "left/center/right"},
    {"ellipsize",	0, 0, G_OPTION_ARG_CALLBACK,			&parse_ellipsis,
     "Ellipsization mode",				  "start/middle/end"},
    {"font",		0, 0, G_OPTION_ARG_STRING,			&opt_font,
     "Set the font description",			       "description"},
    {"gravity",		0, 0, G_OPTION_ARG_CALLBACK,			&parse_gravity,
     "Base gravity: glyph rotation",		"south/east/north/west/auto"},
    {"gravity-hint",	0, 0, G_OPTION_ARG_CALLBACK,			&parse_gravity_hint,
     "Gravity hint",				       "natural/strong/line"},
    {"header",		0, 0, G_OPTION_ARG_NONE,			&opt_header,
     "Display the options in the output",				NULL},
    {"height",		0, 0, G_OPTION_ARG_INT,				&opt_height,
     "Height in points (positive) or number of lines (negative) for ellipsizing", "+points/-numlines"},
    {"hinting",		0, 0, G_OPTION_ARG_CALLBACK,			&parse_hinting,
     "Hinting style",					    "none/auto/full"},
    {"indent",		0, 0, G_OPTION_ARG_INT,				&opt_indent,
     "Width in points to indent paragraphs",			    "points"},
    {"justify",		0, 0, G_OPTION_ARG_NONE,			&opt_justify,
     "Align paragraph lines to be justified",			    	NULL},
    {"language",	0, 0, G_OPTION_ARG_STRING,			&opt_language,
     "Language to use for font selection",			    "en_US/etc"},
    {"margin",		0, 0, G_OPTION_ARG_INT,				&opt_margin,
     "Set the margin on the output in pixels",			    "pixels"},
    {"markup",		0, 0, G_OPTION_ARG_NONE,			&opt_markup,
     "Interpret text as Pango markup",					NULL},
    {"output",		'o', 0, G_OPTION_ARG_STRING,			&opt_output,
     "Save rendered image to output file",			      "file"},
    {"pangorc",		0, 0, G_OPTION_ARG_STRING,			&opt_pangorc,
     "pangorc file to use (default is ./pangorc)",		      "file"},
    {"rtl",		0, 0, G_OPTION_ARG_NONE,			&opt_rtl,
     "Set base direction to right-to-left",				NULL},
    {"rotate",		0, 0, G_OPTION_ARG_DOUBLE,			&opt_rotate,
     "Angle at which to rotate results",			   "degrees"},
    {"runs",		'n', 0, G_OPTION_ARG_INT,			&opt_runs,
     "Run Pango layout engine this many times",			   "integer"},
    {"single-par",	0, 0, G_OPTION_ARG_NONE,			&opt_single_par,
     "Enable single-paragraph mode",					NULL},
    {"text",		't', 0, G_OPTION_ARG_STRING,			&opt_text,
     "Text to display (instead of a file)",			    "string"},
    {"version",		0, G_OPTION_FLAG_NO_ARG, G_OPTION_ARG_CALLBACK, &show_version,
     "Show version numbers",						NULL},
    {"waterfall",	0, 0, G_OPTION_ARG_NONE,			&opt_waterfall,
     "Create a waterfall display",					NULL},
    {"width",		'w', 0, G_OPTION_ARG_INT,			&opt_width,
     "Width in points to which to wrap lines or ellipsize",	    "points"},
    {"wrap",		0, 0, G_OPTION_ARG_CALLBACK,			&parse_wrap,
     "Text wrapping mode (needs a width to be set)",   "word/char/word-char"},
    {NULL}
  };
  GError *error = NULL;
  GError *parse_error = NULL;
  GOptionContext *context;
  size_t len;

  prog_name = g_path_get_basename (argv[0]);
  context = g_option_context_new ("- FILE");
  g_option_context_add_main_entries (context, entries, NULL);
  if (!g_option_context_parse (context, &argc, &argv, &parse_error))
  {
    if (parse_error != NULL)
      fail("%s", parse_error->message);
    else
      fail("Option parse error");
    exit(1);
  }
  g_option_context_free(context);
  g_free(backend_options);
  g_free(backend_desc);

  if ((opt_text && argc != 1) || (!opt_text && argc != 2))
    {
      if (opt_text && argc != 1)
	fail ("When specifying --text, no file should be given");

      g_printerr ("Usage: %s [OPTION...] FILE\n", prog_name);
      exit (1);
    }

  /* set up the backend */
  if (!opt_viewer)
    {
      opt_viewer = *viewers;
      if (!opt_viewer)
	fail ("No viewer backend found");
    }

  /* Get the text
   */
  if (opt_text)
    {
      text = g_strdup (opt_text);
      len = strlen (text);
    }
  else
    {
      if (!g_file_get_contents (argv[1], &text, &len, &error))
	fail ("%s\n", error->message);
    }

  /* Strip one trailing newline
   */
  if (len > 0 && text[len - 1] == '\n')
    len--;
  if (len > 0 && text[len - 1] == '\r')
    len--;
  text[len] = '\0';

  /* Make sure we have valid markup
   */
  if (opt_markup &&
      !pango_parse_markup (text, -1, 0, NULL, NULL, NULL, &error))
    fail ("Cannot parse input as markup: %s", error->message);

  /* Setup PANGO_RC_FILE
   */
  if (!opt_pangorc)
    if (g_file_test ("./pangorc", G_FILE_TEST_IS_REGULAR))
      opt_pangorc = "./pangorc";
  if (opt_pangorc)
    g_setenv ("PANGO_RC_FILE", opt_pangorc, TRUE);
}


void
finalize (void)
{
  g_free (text);
}
