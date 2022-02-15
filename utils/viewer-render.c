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

gboolean opt_display = TRUE;
int opt_dpi = 96;
gboolean opt_pixels = FALSE;
gboolean opt_pango_units = FALSE;
const char *opt_font = "";
gboolean opt_header = FALSE;
const char *opt_output = NULL;
int opt_margin_t = 10;
int opt_margin_r = 10;
int opt_margin_b = 10;
int opt_margin_l = 10;
int opt_markup = FALSE;
gboolean opt_rtl = FALSE;
double opt_rotate = 0;
gboolean opt_auto_dir = TRUE;
const char *opt_text = NULL;
gboolean opt_waterfall = FALSE;
int opt_width = -1;
int opt_height = -1;
int opt_indent = 0;
int opt_spacing = 0;
double opt_line_spacing = -1.0;
gboolean opt_justify = 0;
gboolean opt_justify_last_line = 0;
int opt_runs = 1;
PangoAlignment opt_align = PANGO_ALIGN_LEFT;
PangoEllipsizeMode opt_ellipsize = PANGO_ELLIPSIZE_NONE;
PangoGravity opt_gravity = PANGO_GRAVITY_SOUTH;
PangoGravityHint opt_gravity_hint = PANGO_GRAVITY_HINT_NATURAL;
HintMode opt_hinting = HINT_DEFAULT;
HintMetrics opt_hint_metrics = HINT_METRICS_DEFAULT;
SubpixelOrder opt_subpixel_order = SUBPIXEL_DEFAULT;
Antialias opt_antialias = ANTIALIAS_DEFAULT;
gboolean opt_subpixel_positions = FALSE;
PangoWrapMode opt_wrap = PANGO_WRAP_WORD_CHAR;
gboolean opt_wrap_set = FALSE;
static const char *opt_pangorc = NULL; /* Unused */
const PangoViewer *opt_viewer = NULL;
const char *opt_language = NULL;
gboolean opt_single_par = FALSE;
PangoColor opt_fg_color = {0, 0, 0};
guint16 opt_fg_alpha = 65535;
gboolean opt_bg_set = FALSE;
PangoColor opt_bg_color = {65535, 65535, 65535};
guint16 opt_bg_alpha = 65535;
gboolean opt_serialized = FALSE;
const char *opt_serialized_output;
const char *file_arg;

/* Text (or markup) to render */
static char *text;

void
fail (const char *format, ...)
{
  const char *msg;

  va_list vap;
  va_start (vap, format);
  msg = g_strdup_vprintf (format, vap);
  g_printerr ("%s: %s\n", g_get_prgname (), msg);

  exit (1);
}

static PangoLayout *
make_layout(PangoContext *context,
	    const char   *text,
	    double        size)
{
  static PangoFontDescription *font_description;
  PangoAlignment align;
  PangoLayout *layout;

  if (opt_serialized)
    {
      char *data;
      gsize len;
      GBytes *bytes;
      GError *error = NULL;

      if (!g_file_get_contents (file_arg, &data, &len, &error))
        fail ("%s\n", error->message);
      bytes = g_bytes_new_take (data, len);
      layout = pango_layout_deserialize (context, bytes, PANGO_LAYOUT_DESERIALIZE_CONTEXT, &error);
      if (!layout)
        fail ("%s\n", error->message);
      g_bytes_unref (bytes);
      goto out;
    }

  layout = pango_layout_new (context);
  if (opt_markup)
    pango_layout_set_markup (layout, text, -1);
  else
    pango_layout_set_text (layout, text, -1);

  pango_layout_set_auto_dir (layout, opt_auto_dir);
  pango_layout_set_ellipsize (layout, opt_ellipsize);
  pango_layout_set_justify (layout, opt_justify);
  pango_layout_set_justify_last_line (layout, opt_justify_last_line);
  pango_layout_set_single_paragraph_mode (layout, opt_single_par);
  pango_layout_set_wrap (layout, opt_wrap);

  font_description = pango_font_description_from_string (opt_font);
  if (size > 0)
    pango_font_description_set_size (font_description, size * PANGO_SCALE);

  if (opt_width >= 0)
    {
      if (opt_pango_units)
        pango_layout_set_width (layout, opt_width);
      else
        pango_layout_set_width (layout, (opt_width * opt_dpi * PANGO_SCALE + 36) / 72);
    }

  if (opt_height >= 0)
    {
      if (opt_pango_units)
        pango_layout_set_width (layout, opt_height);
      else
        pango_layout_set_height (layout, (opt_height * opt_dpi * PANGO_SCALE + 36) / 72);
    }
  else
    pango_layout_set_height (layout, opt_height);

  if (opt_indent != 0)
    {
      if (opt_pango_units)
        pango_layout_set_indent (layout, opt_indent);
      else
        pango_layout_set_indent (layout, (opt_indent * opt_dpi * PANGO_SCALE + 36) / 72);
    }

  if (opt_spacing != 0)
    {
      if (opt_pango_units)
        pango_layout_set_spacing (layout, opt_spacing);
      else
        pango_layout_set_spacing (layout, (opt_spacing * opt_dpi * PANGO_SCALE + 36) / 72);
      pango_layout_set_line_spacing (layout, 0.0);
    }
  if (opt_line_spacing >= 0.0)
    pango_layout_set_line_spacing (layout, (float)opt_line_spacing);

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

out:
  if (opt_serialized_output)
    {
      GError *error = NULL;

      if (!pango_layout_write_to_file (layout,
                                       PANGO_LAYOUT_SERIALIZE_CONTEXT|PANGO_LAYOUT_SERIALIZE_OUTPUT,
                                       opt_serialized_output,
                                       &error))
        fail ("%s\n", error->message);
    }

  return layout;
}

gchar *
get_options_string (void)
{
  PangoFontDescription *font_description = pango_font_description_from_string (opt_font);
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
  int x = opt_margin_l;
  int y = opt_margin_t;
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
  if (opt_serialized && supports_matrix)
    {
      const PangoMatrix *context_matrix = pango_context_get_matrix (pango_layout_get_context (layout));
      matrix = context_matrix ? *context_matrix : (PangoMatrix) PANGO_MATRIX_INIT;
    }

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

  width += opt_margin_l + opt_margin_r;
  height += opt_margin_t + opt_margin_b;

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
	    gpointer    data G_GNUC_UNUSED,
	    GError **error)
{
  char *possible_values = NULL;
  gboolean ret;

G_GNUC_BEGIN_IGNORE_DEPRECATIONS
  ret = pango_parse_enum (type,
			  arg,
			  value,
			  FALSE,
			  &possible_values);
G_GNUC_END_IGNORE_DEPRECATIONS

  if (!ret && error)
    {
      g_set_error(error,
		  G_OPTION_ERROR,
		  G_OPTION_ERROR_BAD_VALUE,
		  "Argument for %s must be one of %s",
		  name,
		  possible_values);
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
  return parse_enum (PANGO_TYPE_ALIGNMENT, (int*)(void*)&opt_align,
		     name, arg, data, error);
}

static gboolean
parse_ellipsis (const char *name,
		const char *arg,
		gpointer    data,
		GError **error)
{
  return parse_enum (PANGO_TYPE_ELLIPSIZE_MODE, (int*)(void*)&opt_ellipsize,
		     name, arg, data, error);
}

static gboolean
parse_gravity (const char *name,
	       const char *arg,
	       gpointer    data,
	       GError **error)
{
  return parse_enum (PANGO_TYPE_GRAVITY, (int*)(void*)&opt_gravity,
		     name, arg, data, error);
}

static gboolean
parse_gravity_hint (const char *name,
		    const char *arg,
		    gpointer    data,
		    GError **error)
{
  return parse_enum (PANGO_TYPE_GRAVITY_HINT, (int*)(void*)&opt_gravity_hint,
		     name, arg, data, error);
}

static gboolean
parse_hinting (const char *name G_GNUC_UNUSED,
	       const char *arg,
	       gpointer    data G_GNUC_UNUSED,
	       GError    **error)
{
  gboolean ret = TRUE;

  if (strcmp (arg, "none") == 0)
    opt_hinting = HINT_NONE;
  else if (strcmp (arg, "auto") == 0)
    opt_hinting = HINT_AUTO;
  else if (strcmp (arg, "slight") == 0)
    opt_hinting = HINT_SLIGHT;
  else if (strcmp (arg, "medium") == 0)
    opt_hinting = HINT_MEDIUM;
  else if (strcmp (arg, "full") == 0)
    opt_hinting = HINT_FULL;
  else
    {
      g_set_error(error,
		  G_OPTION_ERROR,
		  G_OPTION_ERROR_BAD_VALUE,
		  "Argument for --hinting must be one of none/auto/slight/medium/full");
      ret = FALSE;
    }

  return ret;
}

static gboolean
parse_subpixel_order (const char  *name,
                      const char  *arg,
                      gpointer     data,
                      GError     **error)
{
  gboolean ret = TRUE;

  if (strcmp (arg, "rgb") == 0)
    opt_subpixel_order = SUBPIXEL_RGB;
  else if (strcmp (arg, "bgr") == 0)
    opt_subpixel_order = SUBPIXEL_BGR;
  else if (strcmp (arg, "vrgb") == 0)
    opt_subpixel_order = SUBPIXEL_VRGB;
  else if (strcmp (arg, "vbgr") == 0)
    opt_subpixel_order = SUBPIXEL_VBGR;
  else
    {
      g_set_error (error,
		   G_OPTION_ERROR,
		   G_OPTION_ERROR_BAD_VALUE,
		   "Argument for --subpixel-order must be one of rgb/bgr/vrgb/vbgr");
      ret = FALSE;
    }

  return ret;
}

static gboolean
parse_hint_metrics (const char  *name,
                    const char  *arg,
                    gpointer     data,
                    GError     **error)
{
  gboolean ret = TRUE;

  if (strcmp (arg, "on") == 0)
    opt_hint_metrics = HINT_METRICS_ON;
  else if (strcmp (arg, "off") == 0)
    opt_hint_metrics = HINT_METRICS_OFF;
  else
    {
      g_set_error (error,
		   G_OPTION_ERROR,
		   G_OPTION_ERROR_BAD_VALUE,
		   "Argument for --hint-metrics must be one of on/off");
      ret = FALSE;
    }

  return ret;
}

static gboolean
parse_antialias (const char  *name,
                 const char  *arg,
                 gpointer     data,
                 GError     **error)
{
  gboolean ret = TRUE;

  if (strcmp (arg, "none") == 0)
    opt_antialias = ANTIALIAS_NONE;
  else if (strcmp (arg, "gray") == 0)
    opt_antialias = ANTIALIAS_GRAY;
  else if (strcmp (arg, "subpixel") == 0)
    opt_antialias = ANTIALIAS_SUBPIXEL;
  else
    {
      g_set_error (error,
		   G_OPTION_ERROR,
		   G_OPTION_ERROR_BAD_VALUE,
		   "Argument for --antialias must be one of none/gray/subpixel");
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
  if ((ret = parse_enum (PANGO_TYPE_WRAP_MODE, (int*)(void*)&opt_wrap,
			 name, arg, data, error)))
    {
      opt_wrap_set = TRUE;
    }
  return ret;
}

static gboolean
parse_rgba_color (PangoColor *color,
		  guint16    *alpha,
		  const char *name,
		  const char *arg,
		  gpointer    data G_GNUC_UNUSED,
		  GError    **error)
{
  gboolean ret;
  char buf[32];
  int len;

  len = strlen (arg);
  /* handle alpha */
  if (*arg == '#' && (len == 5 || len == 9 || len == 17))
    {
      int width, bits;
      unsigned int a;

      bits = len - 1;
      width = bits >> 2;

      strcpy (buf, arg);
      arg = buf;

      if (!sscanf (buf + len - width, "%x", &a))
        {
	  ret = FALSE;
	  goto err;
	}
      buf[len - width] = '\0';

      a <<= (16 - bits);
      while (bits < 16)
        {
	  a |= (a >> bits);
	  bits *= 2;
	}
      *alpha = a;
    }
  else
    *alpha = 65535;

  ret = pango_color_parse (color, arg);

err:
  if (!ret && error)
    {
      g_set_error(error,
		  G_OPTION_ERROR,
		  G_OPTION_ERROR_BAD_VALUE,
		  "Argument for %s must be a color name like red, or CSS-style #rrggbb / #rrggbbaa",
		  name);
    }

  return ret;
}

static gboolean
parse_foreground (const char *name,
		  const char *arg,
		  gpointer    data,
		  GError **error)
{
  return parse_rgba_color (&opt_fg_color, &opt_fg_alpha,
			   name, arg, data, error);
}

static gboolean
parse_background (const char *name,
		  const char *arg,
		  gpointer    data,
		  GError **error)
{
  opt_bg_set = TRUE;

  if (0 == strcmp ("transparent", arg))
    {
      opt_bg_alpha = 0;
      return TRUE;
    }

  return parse_rgba_color (&opt_bg_color, &opt_bg_alpha,
			   name, arg, data, error);
}

static gboolean
parse_margin (const char *name G_GNUC_UNUSED,
	      const char *arg,
	      gpointer    data G_GNUC_UNUSED,
	      GError    **error)
{
  switch (sscanf (arg, "%d%*[ ,]%d%*[ ,]%d%*[ ,]%d", &opt_margin_t, &opt_margin_r, &opt_margin_b, &opt_margin_l))
  {
    default:
    case 0:
    {
      g_set_error(error,
		  G_OPTION_ERROR,
		  G_OPTION_ERROR_BAD_VALUE,
		  "Argument for --margin must be one to four space-separated numbers");
      return FALSE;
    }
    case 1: opt_margin_r = opt_margin_t;
      G_GNUC_FALLTHROUGH;
    case 2: opt_margin_b = opt_margin_t;
      G_GNUC_FALLTHROUGH;
    case 3: opt_margin_l = opt_margin_r;
  }
  return TRUE;
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
parse_backend (const char *name G_GNUC_UNUSED,
	       const char *arg,
	       gpointer    data G_GNUC_UNUSED,
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


static G_GNUC_NORETURN gboolean
show_version(const char *name G_GNUC_UNUSED,
	     const char *arg G_GNUC_UNUSED,
	     gpointer    data G_GNUC_UNUSED,
	     GError    **error G_GNUC_UNUSED)
{
  g_printf("%s (%s) %s\n", g_get_prgname (), PACKAGE_NAME, PACKAGE_VERSION);

  if (PANGO_VERSION != pango_version())
    g_printf("Linked Pango library has a different version: %s\n", pango_version_string ());

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
    {"background",	0, 0, G_OPTION_ARG_CALLBACK,			&parse_background,
     "Set the background color",     "red/#rrggbb/#rrggbbaa/transparent"},
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
    {"foreground",	0, 0, G_OPTION_ARG_CALLBACK,			&parse_foreground,
     "Set the text color",		         "red/#rrggbb/#rrggbbaa"},
    {"gravity",		0, 0, G_OPTION_ARG_CALLBACK,			&parse_gravity,
     "Base gravity: glyph rotation",		"south/east/north/west/auto"},
    {"gravity-hint",	0, 0, G_OPTION_ARG_CALLBACK,			&parse_gravity_hint,
     "Gravity hint",				       "natural/strong/line"},
    {"header",		0, 0, G_OPTION_ARG_NONE,			&opt_header,
     "Display the options in the output",				NULL},
    {"height",		0, 0, G_OPTION_ARG_INT,				&opt_height,
     "Height in points (positive) or number of lines (negative) for ellipsizing", "+points/-numlines"},
    {"hinting",		0, 0, G_OPTION_ARG_CALLBACK,			&parse_hinting,
     "Hinting style",					    "none/auto/slight/medium/full"},
    {"antialias",       0, 0, G_OPTION_ARG_CALLBACK,                    &parse_antialias,
     "Antialiasing",                                        "none/gray/subpixel"},
    {"hint-metrics",    0, 0, G_OPTION_ARG_CALLBACK,                    &parse_hint_metrics,
     "Hint metrics",                                        "on/off"},
    { "subpixel-positions", 0, 0, G_OPTION_ARG_NONE,                    &opt_subpixel_positions,
     "Subpixel positioning",                                        NULL},
    {"subpixel-order",  0, 0, G_OPTION_ARG_CALLBACK,                    &parse_subpixel_order,
     "Subpixel order",                                      "rgb/bgr/vrgb/vbgr"},
    {"indent",		0, 0, G_OPTION_ARG_INT,				&opt_indent,
     "Width in points to indent paragraphs",			    "points"},
    {"spacing",		0, 0, G_OPTION_ARG_INT,				&opt_spacing,
     "Spacing in points between lines",			            "points"},
    {"line-spacing",	0, 0, G_OPTION_ARG_DOUBLE,		        &opt_line_spacing,
     "Spread factor for line height",			            "factor"},
    {"justify",         0, 0, G_OPTION_ARG_NONE,                        &opt_justify,
     "Stretch paragraph lines to be justified",                         NULL},
    {"justify-last-line", 0, 0, G_OPTION_ARG_NONE,                      &opt_justify_last_line,
     "Justify the last line of the paragraph",                          NULL},
    {"language",	0, 0, G_OPTION_ARG_STRING,			&opt_language,
     "Language to use for font selection",			    "en_US/etc"},
    {"margin",		0, 0, G_OPTION_ARG_CALLBACK,			&parse_margin,
     "Set the margin on the output in pixels",			    "CSS-style numbers in pixels"},
    {"markup",		0, 0, G_OPTION_ARG_NONE,			&opt_markup,
     "Interpret text as Pango markup",					NULL},
    {"output",		'o', 0, G_OPTION_ARG_STRING,			&opt_output,
     "Save rendered image to output file",			      "file"},
    {"pangorc",		0, G_OPTION_FLAG_HIDDEN, G_OPTION_ARG_STRING,	&opt_pangorc,
     "Deprecated",		      "file"},
    {"pixels",		0, 0, G_OPTION_ARG_NONE,			&opt_pixels,
     "Use pixel units instead of points (sets dpi to 72)",		NULL},
    {"pango-units",	0, 0, G_OPTION_ARG_NONE,			&opt_pango_units,
     "Use Pango units instead of points",		                NULL},
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
    {"serialized",       0, 0, G_OPTION_ARG_NONE,                        &opt_serialized,
     "Create layout from a serialized file",                            "FILE"},
    {"serialize-to",     0, 0, G_OPTION_ARG_FILENAME,                  &opt_serialized_output,
     "Serialize result to a file",                                      "FILE"},
    {NULL}
  };
  GError *error = NULL;
  GError *parse_error = NULL;
  GOptionContext *context;
  size_t len;
  const PangoViewer **viewer;

  context = g_option_context_new ("- FILE");
  g_option_context_add_main_entries (context, entries, NULL);

  for (viewer = viewers; *viewer; viewer++)
    if ((*viewer)->get_option_group)
      {
        GOptionGroup *group = (*viewer)->get_option_group (*viewer);
	if (group)
	  g_option_context_add_group (context, group);
      }

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

  if (opt_pixels)
    opt_dpi = 72;

  if ((opt_text && argc != 1) || (!opt_text && argc != 2))
    {
      if (opt_text && argc != 1)
	fail ("When specifying --text, no file should be given");

      g_printerr ("Usage: %s [OPTION...] FILE\n", g_get_prgname ());
      exit (1);
    }

  if (opt_serialized && argc != 2)
    {
      g_printerr ("Usage: %s [OPTION...] FILE\n", g_get_prgname ());
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
  if (opt_serialized)
    {
      file_arg = argv[1];
      text = g_strdup ("");
      len = 0;
    }
  else if (opt_text)
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
}


void
finalize (void)
{
  g_free (text);
}
